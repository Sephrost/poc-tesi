#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#ifdef _MSC_VER
#include <intrin.h>
#pragma optimize("gt",on)
#else
#include <x86intrin.h>
#endif

#ifndef L1_CACHE_LINE_SIZE
#define L1_CACHE_LINE_SIZE 64
#endif

#define CACHE_HIT_TRESHOLD 80
#define ARRAY1_SIZE 16
#define TRIES 1000


void exit(int status);

void exit(int status){
  // kill the parent
  kill(getppid(), SIGKILL);
  exit(status);
}

// Global variables
unsigned int array1_size = 16;
uint8_t unused1[L1_CACHE_LINE_SIZE];// used to ensure that array1 and 2 are in different cache lines uint8_t array1[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
uint8_t array1[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                      10, 11, 12, 13, 14, 15};
uint8_t unused2[L1_CACHE_LINE_SIZE];// same thing as unused1
uint8_t array2[256 * 512];
int results[256];

int probe_memory_byte(){
  memset(results, 0, sizeof(results));
  
  for(int i=0; i<TRIES; i++){
    for (int j=0; j<256; j++){
      // flush the cache line
      /* _mm_clflush(&array2[j*page_size]); */
    }
  }
}


int main(int argc,char** argv){
  // print argv 
  printf("argv: %s\n", argv);

  uint8_t* secret_ptr = argv[0];
  uint8_t size = atoi(argv[1]);

  long page_size;
  page_size = sysconf(_SC_PAGESIZE);
  printf("Page size: %d bytes \n", page_size);

  
}
