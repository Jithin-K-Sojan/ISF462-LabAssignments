// Jithin Kallukalam Sojan
// 2017A7PS0163P

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// This define the amount of time that the philosopher spends eating.
#define EAT_TIME 0

typedef enum states{THINKING,EATING} states;

void signalHandler(int sig){
    printf("SIGINT: Exiting.\n");
    exit(0);
}

void doActions(int myIndex,int N, int cntlId,int chopStickId,states* mem){
    int leftCS = myIndex;
    int rightCS = (myIndex+1)%N;

    struct sembuf sops[2];
    while(1){

        // Entering the area where process checks if it is allowed to eat
        sops[0].sem_num = 0;
        sops[0].sem_op = -1;
        sops[0].sem_flg = 0;

        int ret = semop(cntlId,sops,1);
        if(ret==0){
            if((mem[myIndex]==THINKING) && (mem[(myIndex+N-1)%N]!=EATING) && (mem[(myIndex+1)%N]!=EATING)){
                mem[myIndex] = EATING;
            }
        }
        else{
            perror("semop");
            exit(0);
        }

        sops[0].sem_num = 0;
        sops[0].sem_op = 1;
        sops[0].sem_flg = 0;

        ret = semop(cntlId,sops,1);
        if(ret==-1){
            perror("semop");
            exit(0);
        }

        if(mem[myIndex]==EATING){
            sops[0].sem_num = leftCS;
            sops[0].sem_op = -1;
            sops[0].sem_flg = 0;
            sops[1].sem_num = rightCS;
            sops[1].sem_op = -1;
            sops[1].sem_flg = 0;

            ret = semop(chopStickId,sops,2);
            if(ret==0){
                // Eating
                sleep(EAT_TIME);
                printf("Process %d: Eating.\n",getpid());
            }
            else{
                perror("semop");
                exit(0);
            }

            sops[0].sem_num = leftCS;
            sops[0].sem_op = 1;
            sops[0].sem_flg = 0;
            sops[1].sem_num = rightCS;
            sops[1].sem_op = 1;
            sops[1].sem_flg = 0;

            ret = semop(chopStickId,sops,2);
            if(ret==-1){
                perror("semop");
                exit(0);
            }
            
            // Return to thinking.
            // We dont need to use a semaphore here as THINKING is a passive action.
            mem[myIndex] = THINKING;
        }
    }
}

int main(int argc, char *argv[]){
    if(argc!=2){
        if(argc==1)printf("Input a number N as CLA.\n");
        else printf("Invalid input.\n");
        exit(0);
    }

    if(signal(SIGINT,signalHandler)==SIG_ERR){
        perror("signal");
        exit(0);
    }

    int N = atoi(argv[1]);
    printf("%d\n",N);

    int shmid = shmget(IPC_PRIVATE,sizeof(states)*N,IPC_CREAT);
    if(shmid==-1){
        perror("shmget");
    }
    states* mem = (states*)shmat(shmid,NULL,0);

    for(int i = 0;i<N;i++)mem[i] = THINKING;

    int cntlId,chopStickId;

    if((cntlId = semget(IPC_PRIVATE,1,IPC_CREAT|0666))==-1){
        perror("semget");
        exit(0);
    }
    if((chopStickId = semget(IPC_PRIVATE,N,IPC_CREAT|0666))==-1){
        perror("semget");
        exit(0);
    }

    unsigned short* array = (unsigned short*)malloc(sizeof(unsigned short)*N);
    for(int i = 0;i<N;i++)array[i] = 1;
    
    if(semctl(cntlId,0,SETVAL,1)==-1){
        perror("semctl");
        exit(0);
    }

    if(semctl(chopStickId,0,SETALL,array)==-1){
        perror("semctl");
        exit(0);
    }

    int myIndex = -1;
    for(int i = 0;i<N;i++){
        if(fork()==0){
            if(signal(SIGINT,SIG_DFL)==SIG_ERR){
                perror("signal");
                exit(0);
            }
            myIndex = i;
            break;
        }
    }
    
    if(myIndex==-1){
        int waitVal,status;

        while((waitVal = wait(&status))>0);
        printf("Parent Exiting");
    }
    else{
        doActions(myIndex,N, cntlId,chopStickId,mem);
    }
}