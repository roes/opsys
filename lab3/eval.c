#include <stdio.h>
#include <stdlib.h>

#include "tst.h"

#define TIMES 100

#define MAX(a,b) ((a > b)?(a):(b))
#define MIN(a,b) ((a > b)?(b):(a))

union header {
  struct {
    union header *ptr;
    unsigned size;
  } s;
};
typedef union header Header;

void test(int data_size, char* test_name, int size){
  int i, j;
  double avg_memusage = 0;
  char *a[size];
  void *lowmem, *highmem, *maxmem = 0, *minmem = 0;

  fprintf(stderr, "Running predicted %s case evaluation.\n", test_name);

#ifdef MMAP
  lowmem = endHeap();
#else
  lowmem = (void *) sbrk(0);
#endif

  for(i = 0; i < TIMES; i++){
    /* Start timing */
    for(j = 0; j < size; j++){
      a[j] = malloc(data_size);
    }
    /* Stop timing */
#ifdef MMAP
    highmem = endHeap();
#else
    highmem = (void *) sbrk(0);
#endif
    maxmem = MAX(maxmem, highmem);
    minmem = (minmem == 0)?maxmem:MIN(minmem,highmem);
    avg_memusage += (highmem-lowmem)*1.0/TIMES;

    for(j = 0; j < size; j++){
      free(a[j]);
    }
  }

  fprintf(stderr, "Memory usage: minimum usage - 0x%x\t maximum usage - 0x%x \taverage usage - 0x%x\n", (unsigned) (minmem-lowmem), (unsigned) (maxmem-lowmem), (unsigned) avg_memusage);
  fprintf(stderr, "Memory needed: 0x%x\n", (unsigned) data_size*size);
}

void bestCase(){
  test(sizeof(Header)*1023, "best", 10000);
}

void worstCase(){
  test(sizeof(Header)*512, "worst", 10000);
}

void randomSizes(){

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
  }
  return 0;
}


