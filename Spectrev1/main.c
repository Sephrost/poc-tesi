// import standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/wait.h>
#ifdef _MSC_VER
#include <intrin.h>
#pragma optimize("gt",on)
#else
#include <x86intrin.h>
#endif

#define ARRAY1_SIZE 16




int main(int argc, char **argv){
  // declare a string 
  char* secret = "Skiddadle skiddodle";
  int size = strlen(secret);
  printf("starting victim\n");
  
  // make a child process to run the spectre attack
  if(fork() == 0){
    // pass the secret address to the child process an its length
    printf("starting attacker\n");
    // make an array called args with the address of secret and its length
    char *args[] = {secret, "16", NULL};
    /* print args; */

    execv("attacker", args);
  }else{
    // wait for the child process to finish
    wait(NULL);
  }

}
