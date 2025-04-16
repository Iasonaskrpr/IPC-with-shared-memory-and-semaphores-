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

#define MAX_LINE_LENGTH 256
//Function that reads config file and returns an array of characters that we will use to determine what command to execute and in which loop number
char** ReadConfig(FILE* config){
    char command[100];
    char com;
    int i;
    int j;
    char temp[100];
    char** um = malloc(3*sizeof(char*)); //Using malloc and freeing later in the mai program so we can transfer the array onto the main program
    //We read it if it is not empty
    if(fgets(command,100,config)!=NULL){
        i=0;
        j=0;
        while(command[i]!=' '){
            temp[j]=command[i];
            i+=1;
            j+=1;
        }
        um[0] = malloc(strlen(temp) + 1);
        strcpy(um[0],temp);
        i+=1;
        j=0;
        if(command[i] == 'E'){ //If it is an exit command we return just the loop number and the E for the main program to recognise that it is time to end
            um[1] = malloc(strlen(&command[i]) + 1);
            strcpy(um[1],&command[i]);
            return um;
        }
        memset(temp, 0, sizeof(temp));//We set all elements of our temporary array to 0 so we can have a fresh array
        while(command[i]!=' '){
            if(command[i] == 'C'){//If it is C we continue because we only care about the number and we know it is not an exit command because we checked earlier
                i+=1;
                continue;
            }
            temp[j]=command[i];
            i+=1;
            j+=1;
        }
        um[1] = malloc(strlen(temp) + 1);
        strcpy(um[1],temp);
        i+=1;
        com = command[i];
        um[2] = malloc(strlen(&command[i]) + 1);
        strcpy(um[2],&command[i]);
    }
    return um;
}
int main(){
    int fd = shm_open(SHARED_MEMORY_NAME,O_CREAT|O_RDWR,0600);
    //We open all the files we need
    FILE* book = fopen("Data/mobydick.txt","r");
    FILE* config = fopen(FILE_NAME,"r");
    FILE* sent = fopen("Data/sentlinespar.txt","w");
    /* We open the file here because we want to have an empty file for the children, If we did this in the child program every time
    a child would open the file the contents would be overwriten*/
    FILE* rec = fopen("Data/sentlineschild.txt","w"); 
    //Intializing variables we will need later in the program
    Shared_Memory *shmp;
    char** Conf;//Used to store the commands from each line from the configuration file
    long long int offset = 0; //Used to calculate the offset from each line in our book text file
    int total_lines = 0;//Used to measure how many total lines our book has
    int loop;//Used to know at which line we should execute a command from the config file
    long *line_offsets = NULL;//This is going to be the array where we store the offsets of each line in our book text file
    int Open_Process = 0;//Used to keep track of how many processes we have active
    int child_num;//Used to store the number of the process we will spawn or terminate
    int random_line;//We use this to store the random line number using the rand() function
    int Processes[CONSUME_NUM]; //Array to keep hold of which processes are active and which are not
    int status; //We use it so we can acquire the status of the exited child, we get it using wait()
    int exit_code;//We store the exit code of the child
    int random_child;//Used to calculate whichi child we are going to send a random line to
    char buffer[1000];//Buffer used to temporarly store the random line that will be written to the shared memory buffer
    srand(time(NULL)); // Seed the random number generator
    if (ftruncate(fd, sizeof(struct shmbuf)) != 0 ){errExit("Couldn't allocate memory\n");} //If truncate fails we exit
    shmp = mmap(NULL,sizeof(*shmp),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);//Mapping the named shared memory segment
    if(sem_init(&(shmp->sem2),1,0) != 0){//If we can't initialize the semaphore we exit
        fclose(book);
        fclose(config);
        fclose(sent);
        fclose(rec);
        close(fd);
        munmap(shmp,sizeof(*shmp));
        shm_unlink(SHARED_MEMORY_NAME);
        errExit("Couldn't initialize semaphore\n");
    }
    for(int i = 0; i < CONSUME_NUM; i++){ //We initialize the semaphores for each child
        if(sem_init(&(shmp->sem1[i]),1,0) != 0){
            fclose(book);
            fclose(config);
            fclose(sent);
            fclose(rec);
            close(fd);
            munmap(shmp,sizeof(*shmp));
            shm_unlink(SHARED_MEMORY_NAME);
            errExit("Couldn't initialize semaphore\n");
        }
    }
    //We read all the lines of the book and we calculate their offset, this is so we can later get a random line without having to go through every line before the random line we want
    while (fgets(buffer, MAX_LINE_LENGTH, book) != NULL) {
        total_lines++;
        line_offsets = realloc(line_offsets, total_lines * sizeof(long));
        if (line_offsets == NULL) {
            fclose(book);
            fclose(config);
            fclose(sent);
            fclose(rec);
            close(fd);
            munmap(shmp,sizeof(*shmp));
            shm_unlink(SHARED_MEMORY_NAME);
            errExit("Couldn't allocate variable\n");
        }
        line_offsets[total_lines - 1] = offset;
        offset = ftell(book); // Get current file position
    }
    //We read the first line of the configuration file so we can know what we should do before we start the loop
    Conf = ReadConfig(config); 
    loop = atoi(Conf[0]);
    for (int i = 0;i<CONSUME_NUM;i++){
        Processes[i] = 0;
    }
    shmp->loop_num = 0; //This is so we can know at which loop we are
    while(1){ //We go until we get an exit signal
        if(loop == shmp->loop_num){
            if(*Conf[1] == 'E'){ //If command is exit begin to exit
                printf("Parent begining to exit...\n");
                break;
            }
            else if(*Conf[2] == 'S'){//If the command is S we spawn a new child,if it isnt spawned already, if it is we do nothing
                child_num = atoi(Conf[1]);
                /*We use an array that tells us if we have already spawned a child or not,ADJUST is used because in the config files some start the child processes 
                with 1 and some don't, we need to know how we should access the array*/
                if (Processes[child_num-ADJUST] == 0){ 
                    shmp->Terminate_Signal[child_num-ADJUST] = 0; //Terminate signal is used to signal to the child that it must exit
                    if(fork() == 0){ 
                        execl(CHILD_NAME,CHILD_NAME,Conf[1],(char *)NULL);
                        errExit("Exec failed");
                        }
                    sem_wait(&(shmp->sem2)); //We wait for the child to finish initializing
                    Open_Process++;//We increase this variable when a child is spawned
                    Processes[child_num-ADJUST] = 1;//1 so we can know that this process is active and can receive random lines
                }
            }
            else{//Else condition is for when we are about to terminate child
                child_num = atoi(Conf[1]);//Get the child number
                if (Processes[child_num-ADJUST] != 0){//If it is active we terminate it,if not we do nothing
                    Open_Process--;//We decrease it beacuse we have one less active process
                    Processes[child_num-ADJUST] = 0;//We set it to 0 to know that there is no active process to sent lines to
                    shmp->Terminate_Signal[child_num-ADJUST] = 1;//The child will read that and will begin exiting
                    if(sem_post(&shmp->sem1[child_num-ADJUST]) !=0 ){//We post the semaphore to wake the child
                        fclose(book);
                        fclose(config);
                        fclose(sent);
                        fclose(rec);
                        close(fd);
                        munmap(shmp,sizeof(*shmp));
                        shm_unlink(SHARED_MEMORY_NAME);
                        errExit("Couldn't post semaphore\n");
                    }
                    if(sem_wait(&shmp->sem2) != 0){//We wait for the child to finish exiting
                        fclose(book);
                        fclose(config);
                        fclose(sent);
                        fclose(rec);
                        close(fd);
                        munmap(shmp,sizeof(*shmp));
                        shm_unlink(SHARED_MEMORY_NAME);
                        errExit("Couldn't down semaphore\n");
                    }
                wait(&status);//We acquire the status in which the child exited
                exit_code = WEXITSTATUS(status);//We get the exit code
                if(exit_code != 0){//If it isn't a smooth exit parent exits too
                    errExit("Child had a failure\n");
                }
                }
            }
            //Make sure we free the space because we dont need this command anymore
            free(Conf[0]);
            free(Conf[1]);
            free(Conf[2]);
            free(Conf);
            //Read the next line
            Conf = ReadConfig(config);
            loop = atoi(Conf[0]);
        }
        if(Open_Process == 0){
            shmp->loop_num++;
            continue;
        }
        //Get random line
        random_line = rand() % total_lines;
        fseek(book, line_offsets[random_line], SEEK_SET);//Go to the line using seek and the offset of the line from the start
        fgets(buffer, MAX_LINE_LENGTH, book);//Get the line and put in the buffer variable
        strncpy(shmp->buf, buffer, sizeof(shmp->buf) - 1);//Write it into the shared memory buffer
        random_child= rand() % CONSUME_NUM;//Get random child, we make sure the random child is one that is active
        while(Processes[random_child] == 0){ 
            random_child = rand() % CONSUME_NUM;
        }
        fprintf(sent,"Sending following line to Child %d:\n%s ",random_child,buffer);//Write the line we sent to the file we use to see which lines got sent and if the children received them
        if(sem_post(&shmp->sem1[random_child])!= 0){//Post semaphore linked with the child
            fclose(book);
            fclose(config);
            fclose(sent);
            fclose(rec);
            close(fd);
            munmap(shmp,sizeof(*shmp));
            shm_unlink(SHARED_MEMORY_NAME);
            errExit("Couldn't post semaphore\n");
        }
        if(sem_wait(&shmp->sem2)!= 0){//We wait for the child to finish processing the line we sent it
            fclose(book);
            fclose(config);
            fclose(sent);
            fclose(rec);
            close(fd);
            munmap(shmp,sizeof(*shmp));
            shm_unlink(SHARED_MEMORY_NAME);
            errExit("Couldn't down semaphore\n");
        }
        shmp->loop_num++;//Increase the loop num
    }
    //Here we make sure we terminate every child that is active and we wait for each child to terminate before we terminate the next one
    for(int i = 0;i<CONSUME_NUM;i++){
        if(Processes[i] == 1){
            shmp->Terminate_Signal[i] = 1;
            if(sem_post(&shmp->sem1[i])!= 0){
                fclose(book);
                fclose(config);
                fclose(sent);
                fclose(rec);
                close(fd);
                munmap(shmp,sizeof(*shmp));
                shm_unlink(SHARED_MEMORY_NAME);
                errExit("Sem post failed\n");
            }
            if(sem_wait(&shmp->sem2)!= 0){
                fclose(book);
                fclose(config);
                fclose(sent);
                fclose(rec);
                close(fd);
                munmap(shmp,sizeof(*shmp));
                shm_unlink(SHARED_MEMORY_NAME);
                errExit("Sem post failed\n");
            }
        }
    }
    //We free all the assets and we close all the files we have opened and we make sure to unlink the shared memory segment
    free(Conf[0]);
    free(Conf[1]);
    free(Conf);
    free(line_offsets);
    fclose(book);
    fclose(config);
    fclose(rec);
    close(fd);
    munmap(shmp,sizeof(*shmp));
    shm_unlink(SHARED_MEMORY_NAME);
    printf("All children exited Parent exiting now...\n");
    exit(0);//We exit once everything is finished
}