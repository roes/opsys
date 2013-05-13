/*
 * Mallocs many ints and longs. Then frees them
 * No checks for correctness. A best case scenario for our implementation of malloc
 */


#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include "malloc.h"
#include "tst.h"
#include "brk.h"

#define MAXSIZE 100000

int *ints[MAXSIZE];
long *longs[MAXSIZE];


struct timeval time_passed(struct timeval start, struct timeval end){
  struct timeval diff;                  /* Holds time difference end-start */
  diff.tv_sec = end.tv_sec-start.tv_sec;
  diff.tv_usec = end.tv_usec-start.tv_usec;
  if(diff.tv_usec<0){ diff.tv_usec += 1000000; diff.tv_sec--; }
  return diff;
}



int main(int argc, char *argv[]) {
  int i, return_value, size;
  struct timeval start, end, diff;/* Holds start, end and run time */
  
  if(argc <2) size = MAXSIZE;
  else {
  	size = atoi(argv[1]);  	
  }
  if(size>MAXSIZE) size = MAXSIZE;  
  
  fprintf(stderr, "size: %d. ", size);
  return_value = gettimeofday(&start, NULL);
  if(return_value == -1){ perror("Couldn't get time"); exit(1); }
  
  /* Malloc ints */
  for(i=0; i<size; i++) {
  	ints[i] = malloc(sizeof(int));
  }
  
  /* Malloc longs */
  for(i=0; i<size; i++) {
  	longs[i] = malloc(sizeof(long));
  }
  
   /* Free everything */
   for(i=0; i<size; i++) {
  	free(longs[i]);
  	free(ints[i]);
  } 
  
  return_value = gettimeofday(&end, NULL);
  if(return_value == -1){ perror("Couldn't get time"); exit(1); }
  diff = time_passed(start, end);
  fprintf(stderr, "Exe time: %d:%.6d s\n",
          (int)diff.tv_sec, (int)diff.tv_usec);
  return 0;
}
