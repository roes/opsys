/*
 * Runs tests that evaluate malloc performance.
 *
 * It tests predicted best case (data fills up each memory block, leaving no list
 * to traverse), predicted worst case (data fills half of each memory block, with
 * the header this makes it so that the blocks won't fit more and a list of the
 * same length as the number of data blocks is created), performance using random
 * sized data and increasing/decreasing sized data.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "tst.h"

#define MINSIZE 1
#define MAXSIZE 2048

union header {
  struct {
    union header *ptr;
    unsigned size;
  } s;
};
typedef union header Header;

struct timeval time_passed(struct timeval start, struct timeval end){
  struct timeval diff;
  diff.tv_sec = end.tv_sec-start.tv_sec;
  diff.tv_usec = end.tv_usec-start.tv_usec;
  if(diff.tv_usec<0){ diff.tv_usec += 1000000; diff.tv_sec--; }
  return diff;
}


/*
 * Runs the evaluation tests.
 */
void test(char mode,                    /* increase (+), decrease (-), random (?) or constant (=) */
          int start_size,               /* Initial size to malloc */
          int num_vars){                /* Number of variables to malloc */
  int i, data_size = start_size;        /* Parameter to malloc request */
  void *lowmem, *highmem;               /* Pointers to lower and upper usage on the heap */
  unsigned memusage = 0;                /* Total memory requested through malloc() */
  char *a[num_vars];                    /* Pointers to the allocated memory */
  struct timeval start, end, diff;      /* Start and end time of malloc loop */

#ifdef MMAP                             /* Set lower heap pointer */
  lowmem = endHeap();
#else
  lowmem = (void *) sbrk(0);
#endif
  gettimeofday(&start, NULL);

  for(i = 0; i < num_vars; i++){
    switch(mode){
      case '=':                         /* Constant malloc allocation */
        break;
      case '+':                         /* Amount allocated by malloc is increasing */
        data_size += sizeof(Header);
        break;
      case '-':                         /* Amount allocated by malloc is decreasing */
        data_size -= sizeof(Header);
        break;
      case '?':                         /* Amount allocated by malloc is random */
        data_size = rand()%(MAXSIZE-MINSIZE) + MINSIZE;
        break;
    }

    a[i] = malloc(data_size);
    memusage += data_size;
  }

  gettimeofday(&end, NULL);
#ifdef MMAP                             /* Set upper heap pointer */
  highmem = endHeap();
#else
  highmem = (void *) sbrk(0);
#endif

  for(i = 0; i < num_vars; i++){
    free(a[i]);
  }

  diff = time_passed(start, end);
  fprintf(stderr, "Time consumed: %d:%.6d s\n", (int) diff.tv_sec, (int) diff.tv_usec);
  fprintf(stderr, "Memory usage: 0x%x\n", (unsigned)(highmem-lowmem));
  fprintf(stderr, "Memory needed: 0x%x\n", memusage);
}

void bestCase(){                        /* With header perfect fit in a block */
  test('=', sizeof(Header)*1023, 10000);/* which means there's never a list */
}

void worstCase(){                       /* Takes up half a block without header */
  test('=', sizeof(Header)*512, 10000); /* so next block won't fit -> long list */
}

void randomSizes(){
  test('?', 0, 10000);
}

void incrementSizes(){
  test('+', 0, 2000);
}

void decrementSizes(){
  test('-', sizeof(Header)*2001, 2000);
}

int main(int argc, char *argv[]){
  char *progname;

  if (argc > 0)
    progname = argv[0];
  else
    progname = "";

  if (argc < 2) return 0;

  switch(argv[1][0]){
    case 'w':
      MESSAGE("Starting evaluation of worst case performance\n");
      worstCase();
      break;
    case 'b':
      MESSAGE("Starting evaluation of best case performance\n");
      bestCase();
      break;
    case 'r':
      MESSAGE("Starting evaluation of malloc performance given random data\n");
      randomSizes();
      break;
    case '+':
      MESSAGE("Starting evaluation of malloc performance given increasing malloc sizes\n");
      incrementSizes();
      break;
    case '-':
      MESSAGE("Starting evaluation of malloc performance given decreasing malloc sizes\n");
      decrementSizes();
      break;
  }
  return 0;
}


