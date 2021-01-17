// Jithin Kallukalam Sojan
// 2017A7PS0163P

// NOTE: 1.The commented code throughout the program is for better understanding of the inner workings of the program.
//         It will print information related to corner cases that the question doesnt require to implement.
//       2.The child processes sleep for a second after creation, so that the parent can create all/most of the children before the children starts executing.
//         This can be removed if neccessary.


#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX 50

void sigTermHandler(int sig, siginfo_t *siginfo, void* context){
    printf("Proc %d: Terminated by %ld(received SIGTERM).\n",getpid(),(long)siginfo->si_pid);
    //Waiting for SIGKILL.
    //While loop incase of multiple SIGTERMS from different even processes.
    while(1) pause();
}

void oddProc(pid_t pidArr[]){
    // printf("Entry Proc: %d\n",getpid());
    int evenCount = 0;
    int i = 0;

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = &sigTermHandler;
    act.sa_flags = SA_SIGINFO;

    if(sigaction(SIGTERM, &act,NULL) < 0){
        printf("Proc %d: sigaction error.\n",getpid());
        exit(EXIT_SUCCESS);
    }

    while(1){
        if(pidArr[i]==-1){
            break;
        }
        if(pidArr[i]%2==0){
            evenCount++;
        }
        i++;
    }

    if(evenCount==0){
        printf("Proc %d: No even processes before me.\n",getpid());
        //exit(EXIT_SUCCESS);

        //No use executing because no even processes before this process.
        while(1);
    }

    int generateNewRandom = 1;
    i = 0;
    int num,evenIndex;
    //generating seed for random function.
    srand(time(0));

    while(1){
        if(generateNewRandom){
            // To get the random ith even number out of set of even numbers.
            // Between (0..evenCount)
            num = rand()%evenCount;

            evenIndex = 0;
            i = 0;
            generateNewRandom = 0;
        }
        if(pidArr[i]==-1){
            // printf("Either there are no even pid processes before %d or all of them have already terminated.\n",getpid());
            // exit(EXIT_SUCCESS);

            // This means there are no even processes before this process. It will just keep looping.
            generateNewRandom = 1;
            continue;
        }
        else if(pidArr[i]%2==0){
            if(evenIndex==num){
                if(kill(pidArr[i],SIGUSR1)<0){
                    // printf("%d : Pid %d has already been terminated.\n",getpid(),pidArr[i]);
                    // exit(EXIT_SUCCESS);
                }
                else{
                    // printf("%d: Signal sent to %d.\n",getpid(),pidArr[i]);
                }
                generateNewRandom = 1;
                continue;
            }
            evenIndex++;
        }
        i++;
    }

}


void evenProc(int M){
    // printf("Entry Proc: %d\n",getpid());
    int count = 0;

    sigset_t mask;
    siginfo_t info;

    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    if(sigprocmask(SIG_SETMASK,&mask,NULL)==-1){
        printf("Proc %d: sigprocmask\n",getpid());
        exit(EXIT_SUCCESS);
    }

    int sig;

    while(1){
        sig = sigwaitinfo(&mask,&info);
        if(sig==-1){
            printf("Proc %d: sigwaitinfo\n",getpid());
            exit(EXIT_SUCCESS);
        }

        count++;
        printf("Proc %d : Signal received from %d, total signals %d/%d.\n",getpid(),info.si_pid,count,M);
        
        if(count==M){
            if(kill(info.si_pid,SIGTERM)==-1){
                printf("Proc %d: sigterm\n",getpid());
                // exit(EXIT_SUCCESS);
            }
            sleep(1);
            if(kill(info.si_pid,SIGKILL)==-1){
                printf("Proc %d: sigkill\n",getpid());
                // exit(EXIT_SUCCESS);
            }

            printf("Proc %d: Terminated self.\n",getpid());
            exit(EXIT_SUCCESS);
        }
    }

}

void createProc(int N, pid_t pidArr[], int M){
    pid_t pid;
    int current = 0;

    int status = 0;

    for(int i = 0;i<N;i++){
        pidArr[current] = -1;
        if((pid=fork())<0){
            printf("Error in creating child process.\n");
            exit(-1);
        }
        else if(pid){
            printf("Parent: Created %d.\n",pid);
            pidArr[current] = pid;
            current++;
        }
        else{
            break;
        }
    }

    if(pid == 0){
        // Waiting for all/most of the processes to get created. sleep(1) can be removed if needed.
        sleep(1);

        pid_t checkPid = getpid();
        if(checkPid%2==0){
            evenProc(M);
        }
        else{
            oddProc(pidArr);
        }
    }

    int waitVal;
    while(waitVal = wait(&status)>0);
    printf("Parent Exiting.\n");
}

int main(int argc, char* argv[]){

    if(argc<3){
        printf("Error: N and M have to be given as command line arguments.\n");
        printf("./a.out <N> <M>\n");
        exit(EXIT_SUCCESS);
    }
    
    pid_t pidArr[MAX];
    pidArr[0] = -1;

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);  
    
    createProc(N, pidArr, M);
    exit(EXIT_SUCCESS);
}
