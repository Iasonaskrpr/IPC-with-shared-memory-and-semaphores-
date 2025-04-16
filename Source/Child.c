#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>

#include "shm.h"

int main(int argc,char* argv[]){
    FILE* sentc = fopen("sentlineschild.txt","a");//We open the file to append at the end of the file
    int index = atoi(argv[1])- ADJUST; //We make sure we have the right index
    int total_messages = 0;//Used to track how many messages we have received
    //Open the shared memory segment
    Shared_Memory *shmp;
    int fd = shm_open(SHARED_MEMORY_NAME,O_RDWR,0600);
    if(fd == -1){
        errExit("Couldn't open shared memory\n");
    }
    shmp = mmap(NULL,sizeof(*shmp),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    int start = shmp->loop_num;
    //Signal to parent that we are done and wait for the parent to signal that there are lines to be processed
    sem_post(&(shmp->sem2));
    sem_wait(&(shmp->sem1[index]));  
    while(shmp->Terminate_Signal[index] != 1){//If terminate signal is 1 we begin to terminate
        total_messages++;//+1 for every line we have received
        fprintf(sentc,"Parent sent this line to child %d:\n %s",index,shmp->buf);//Put the line we received to the file
        shmp->buf[sizeof(shmp->buf) - 1] = '\0';//Make sure that the last element of the array signals the end of the line
        //Signal parent we are finished and wait for the next line
        sem_post(&(shmp->sem2));
        sem_wait(&(shmp->sem1[index])); 
    }
    int end = start - (shmp->loop_num);//Calculate for how many loops we were active
    //Print how many messages the process received and for how many loops it was active
    printf("Process %d terminated - Total messages received-> %d - Total Loops->%d\n",index,total_messages,end);
    sem_post(&(shmp->sem2));//Signal parent that we are done exiting
    exit(0);//Exit smoothly
}