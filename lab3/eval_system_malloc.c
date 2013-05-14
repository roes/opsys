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
 *
 */
void test(char mode,                    /* increase (+), decrease (-), random (?) or constant (=) */
          int start_size,               /* Initial size to malloc */
          int num_vars){                /* Number of variables to malloc */
  int i, data_size = start_size;        /* Parameter to malloc request */
  void *lowmem, *highmem;               /* Pointers to lower and upper usage on the heap */
  unsigned memusage = 0;                /* Total memory requested through malloc() */
  char *a[num_vars];                    /* Pointers to the allocated memory */
  struct timeval start, end, diff;      /* Start and end time of malloc loop */

  lowmem = (void *) sbrk(0);
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
  highmem = (void *) sbrk(0);

  for(i = 0; i < num_vars; i++){
    free(a[i]);
  }

  diff = time_passed(start, end);
  fprintf(stderr, "Time consumed: %d:%.6d s\n", (int) diff.tv_sec, (int) diff.tv_usec);
  fprintf(stderr, "Memory usage: 0x%x\n", (unsigned)(highmem-lowmem));
  fprintf(stderr, "Memory needed: 0x%x\n", memusage);
}

void bestCase(){
  test('=', sizeof(Header)*1023, 10000);
}

void worstCase(){
  test('=', sizeof(Header)*512, 10000);
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


