// import standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/wait.h>
#ifdef _MSC_VER
#include <intrin.h>
#pragma optimize("gt", on)
#else
#include <x86intrin.h>
#endif

#define ARRAY1_SIZE 16

int main(int argc, char **argv)
{
	char *secret = "Processes dies when they are killed";
	char size[3];
	snprintf(size, 3, "%d", strlen(secret));

	if (fork() == 0)
	{
		char *args[] = {secret, size, NULL};
		execv("attacker", args);
	}
	else
	{
		// wait for the child process to finish
		wait(NULL);
	}
}
