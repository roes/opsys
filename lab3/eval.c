#include <stdio.h>
#include <stdlib.h>

#include "tst.h"

#define TIMES 100                       /* Number of times to run the test cases */

#define MAX(a,b) ((a>b)?(a):(b))

union header {
  struct {
    union header *ptr;
    unsigned size;
  } s;
};
typedef union header Header;


/*
 *
 */
void test(char* test_name,              /* Name of the test */
          char mode,                    /* increase (+), decrease (-), random (?) or constant (=) */
          int start_size,               /* Initial size to malloc */
          int num_vars){                /* Number of variables to malloc */
  int i, j;
  unsigned long max_memusage = 0;       /* Over TIMES runs, maximum memory requested */
  char *a[num_vars];                    /* Pointers to the allocated memory */
  void *lowmem, *highmem;               /* Pointers to lower and upper usage on the heap */

  fprintf(stderr, "Running %s case evaluation.\n", test_name);

#ifdef MMAP                             /* Set lower heap pointer */
  lowmem = endHeap();
#else
  lowmem = (void *) sbrk(0);
#endif

  for(i = 0; i < TIMES; i++){
    int data_size = start_size;         /* Parameter to malloc request */
    unsigned long memusage = 0;         /* Total memory requested through malloc() */
    /* Start timing */
    for(j = 0; j < num_vars; j++){
      switch(mode){
        case '=':                       /* Constant malloc allocation */
          break;
        case '+':                       /* Amount allocated by malloc is increasing */
          data_size += sizeof(Header);
          break;
        case '-':                       /* Amount allocated by malloc is decreasing */
          data_size -= sizeof(Header);
          break;
        case '?':                       /* Amount allocated by malloc is random */
          data_size = 1;                /* TODO: randomize */
          break;
      }
      a[j] = malloc(data_size);
      memusage += data_size;
    }
    /* Stop timing */
    for(j = 0; j < num_vars; j++){
      free(a[j]);
    }
    max_memusage = MAX(max_memusage, memusage);/* Only varies for random mode */
  }

#ifdef MMAP                             /* Set upper heap pointer*/
  highmem = endHeap();
#else
  highmem = (void *) sbrk(0);
#endif

  fprintf(stderr, "Memory usage: maximum usage - 0x%x\n", (unsigned) (highmem-lowmem));
  fprintf(stderr, "Memory needed: 0x%x\n", (unsigned) max_memusage);
}

void bestCase(){
  test("predicted best", '=', sizeof(Header)*1023, 10000);
}

void worstCase(){
  test("predicted worst", '=', sizeof(Header)*512, 10000);
}

void randomSizes(){
  test("randomized", '?', sizeof(Header)*1, 2000);
}

void incrementSizes(){
  test("increment", '+', 0, 2000);
}

void decrementSizes(){
  test("decrement", '-', sizeof(Header)*2001, 2000);
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


