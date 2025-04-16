#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_SIZE 1024   /* Maximum size for exchanged string */
#define CONSUME_NUM 10 //Change for different configuration files, marks the number of active child processes we can have
/*Set ADJUST to 1 for config_3_100.txt and config_10_10000.txt and set to 0 for config_3_1000
In general set to 1 if processes start from C1 and 0 if they start from C0*/
#define ADJUST 1 
#define CHILD_NAME "./child" 
#define FILE_NAME "config_10_10000.txt" //Change for different configuration files
#define SHARED_MEMORY_NAME "Shm_Buf" 
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

typedef struct shmbuf{
    sem_t sem1[CONSUME_NUM];//Used to signal to child that it needs to wake
    sem_t sem2;//Signals parent to stop being blocked because the child is done
    char buf[BUF_SIZE];//Shared memory buffer
    int Terminate_Signal[CONSUME_NUM];//Used to signal to child that it needs to terminate
    long int loop_num;//Current number of loop in the parent, used by child to know for how many loops it was active
}Shared_Memory;