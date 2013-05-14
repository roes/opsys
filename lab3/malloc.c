#define _GNU_SOURCE

#include "brk.h"
#include <errno.h> 
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define NALLOC 1024                     /* minimum #units to request */

typedef long Align;                     /* for alignment to long boundary */

union header {                          /* block header */
  struct {
    union header *ptr;                  /* next block if on free list */
    unsigned size;                      /* size of this block in sizeof(Header) units */ 
  } s;
  Align x;                              /* force alignment of blocks */
};

typedef union header Header;

static Header base;                     /* empty list to get started */
static Header *freep = NULL;            /* start of free list */


/* free: put block ap in the free list
 *
 */
void free(void * ap){
  Header *bp, *p;

  if(ap == NULL) return;                /* Nothing to do */


  /*
   * Find the place in the free list where the block should be.
   */

  bp = (Header *) ap - 1;               /* point to block header */
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;                            /* freed block at atrt or end of arena */


  if(bp + bp->s.size == p->s.ptr) {     /* merge freed memory with higher nb */
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  }
  else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp) {             /* merge freed memory with lower nb */
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } 
  else
    p->s.ptr = bp;
  freep = p;
}


/* morecore: ask system for more memory */

#ifdef MMAP
static void * __endHeap = 0;

void * endHeap(void)
{
  if(__endHeap == 0) __endHeap = sbrk(0);
  return __endHeap;
}
#endif


static Header *morecore(unsigned nu){
  void *cp;
  Header *up;
#ifdef MMAP
  unsigned noPages;
  if(__endHeap == 0) __endHeap = sbrk(0);
#endif

  if(nu < NALLOC)
    nu = NALLOC;
#ifdef MMAP
  noPages = ((nu*sizeof(Header))-1)/getpagesize() + 1;
  cp = mmap(__endHeap, noPages*getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  nu = (noPages*getpagesize())/sizeof(Header);
  __endHeap += noPages*getpagesize();
#else
  cp = sbrk(nu*sizeof(Header));
#endif
  if(cp == (void *) -1){                /* no space at all */
    perror("failed to get more memory");
    return NULL;
  }
  up = (Header *) cp;
  up->s.size = nu;
  free((void *)(up+1));
  return freep;
}


/*
 *
 */
void * malloc(size_t nbytes){
  Header *p, *prevp;
  Header * morecore(unsigned);
  unsigned nunits;

  if(nbytes == 0) return NULL;

  nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) +1;

  if((prevp = freep) == NULL) {
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }


  /* First fit */
#if STRATEGY == 1
  for(p= prevp->s.ptr;  ; prevp = p, p = p->s.ptr) {
    if(p->s.size >= nunits) {           /* big enough */
      if (p->s.size == nunits)          /* exactly */
        prevp->s.ptr = p->s.ptr;
      else {                            /* allocate tail end */
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void *)(p+1);
    }
    if(p == freep)                      /* wrapped around free list */
      if((p = morecore(nunits)) == NULL)
        return NULL;                    /* none left */
  }
#endif

  /* Best fit */
#if STRATEGY == 2
  Header *bestp = NULL,                 /* Pointer to best fitting block found so far */
         *bestprev = NULL;              /* Pointer to the block before *bestp */
  unsigned bestSize = UINT_MAX;         /* The size of the best block found so far */

  for(p= prevp->s.ptr;  ; prevp = p, p = p->s.ptr) {
    if(p->s.size >= nunits) {           /* big enough */
      if(p->s.size < bestSize || bestp == NULL) {/* Better fit */
        bestp = p;
        bestSize = p->s.size;
        bestprev = prevp;
      }
      if(bestSize == nunits) break;     /* Break if perfect block is found */
    }
    if(p == freep) { 	                /* wrapped around free list */
      if(bestp != NULL) break;          /* found a big enough block */
      if((p = morecore(nunits)) == NULL)
        return NULL;                    /* no memory left */
    }
  }

  if (bestp->s.size == nunits) {        /* exactly */
    bestprev->s.ptr = bestp->s.ptr;     /* remove bestp from the freelist */
  }
  else {                                /* allocate tail end */
    bestp->s.size -= nunits;
    bestp += bestp->s.size;
    bestp->s.size = nunits;
  }

  freep = bestprev;
  return (void *)(bestp+1);
#endif

  /* Worst fit */
#if STRATEGY == 3
  Header *worstp = NULL,                /* Pointer to worst fitting block */
         *worstprev = NULL;             /* Pointer to the block before worstp */
  unsigned worstSize = 0;               /* The size of the worst block */

  for(p= prevp->s.ptr;  ; prevp = p, p = p->s.ptr) {
    if(p->s.size >= nunits) {           /* big enough */
      if(p->s.size > worstSize) {
        worstp = p;
        worstSize = p->s.size;
        worstprev = prevp;
      }
    }
    if(p == freep) { 	                /* wrapped around free list */
      if(worstp != NULL) break;         /* found a big enough block */
      if((p = morecore(nunits)) == NULL)
        return NULL;                    /* no memory left */
    }
  }

  if (worstp->s.size == nunits) {       /* exactly */
    worstprev->s.ptr = worstp->s.ptr;   /* remove bestp from the freelist */
  }
  else {                                /* allocate tail end */
    worstp->s.size -= nunits;
    worstp += worstp->s.size;
    worstp->s.size = nunits;
  }

  freep = worstprev;
  return (void *)(worstp+1);
#endif

}


/*
 *
 */
void *realloc(void * p,                 /* Pointer to memory to realloc */
              size_t nbytes){           /* Amount of memory to allocate */
  void *newp;                           /* Holds newly allocated memory */
  size_t old_nbytes;                    /* Number of bytes allocated for p */

  if (p == NULL) return malloc(nbytes); /* If p is NULL, realloc should behave
                                           like malloc */
  old_nbytes = (((Header*) p-1)->s.size - 1) * sizeof(Header);

  newp = malloc(nbytes);
  if (newp == NULL && nbytes != 0){     /* If the space cannot be allocated */
    return p;                           /* the object pointed to by p is unchanged */
  }
  memcpy(newp, p, (nbytes < old_nbytes) ? nbytes : old_nbytes);
  free(p);
  return newp;
}

