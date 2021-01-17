#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>

#define MAXCLIENTS 10
#define MAXMSGS 20

int sfd;

void sigIntHandler(int signo){

    printf("Terminating process %d.\n",getpid());
    if(sfd!=-1){
        close(sfd);
    }
    exit(0);
}

typedef struct buffStruct{
    char buffer[6];
    int index;
}buffStruct;

void clientFunc(unsigned short portno){
    struct sockaddr_in servAddr;
    bzero(&servAddr,sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(portno);

    int mySock = socket(AF_INET,SOCK_STREAM,0);
    if(mySock<0){
        perror("socket error");
        exit(0);
    }

    if(connect(mySock,(struct sockaddr*)&servAddr,sizeof(servAddr))<0){
        perror("connect error");
        exit(0);
    }

    fd_set rset,allset;
    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(mySock,&allset);
    int maxfd = mySock;
    struct timeval tv;
    struct timeval* tp = &tv;
    tp->tv_sec = 2;
    tp->tv_usec = 0;

    while(1){
        rset = allset;
        int n = select(maxfd+1,&rset,NULL,NULL,tp);
        if(n<0){
            perror("select error");
            exit(0);
        }

        if(n==0){

            char buffer[6];
            strcpy(buffer,"rnstr");
            buffer[5] = '\0';

            if((write(mySock,buffer,5))<0){
                perror("write error");
                exit(0);
            }

            printf("Client %d: Sent random message to clients: %s\n",getpid(),buffer);

            tp->tv_sec = 2;
            tp->tv_usec = 0;
        }

        if(FD_ISSET(mySock,&rset)){
            char buffer[6];
            int nRead;
            if((nRead=read(mySock,buffer,5))<0){
                perror("read error");
            }

            if(nRead==0){
                close(mySock);
                printf("Client %d: Connection closed.\n",getpid());
                exit(0);
            }

            buffer[5] = '\0';

            if(strcmp(buffer,"hello")==0){
                printf("Client %d: Received check from server.\n",getpid());
                if((write(mySock,buffer,5))<0){
                    perror("write error");
                    exit(0);
                }
                printf("Client %d: Responded to server's check.\n",getpid());

            }
            else{
                printf("Client %d: Received random message: %s\n",getpid(),buffer);
            }
        }
    }
}

int main(int argc, char* argv[]){

    signal(SIGINT,sigIntHandler);

    if(argc!=3){
        printf("./a.out <port-no> <N>\n");
        exit(0);
    }

    unsigned short portno = (unsigned short)atoi(argv[1]);
    int N = atoi(argv[2]);

    if((sfd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("sockfd error");
        exit(0);
    }

    int enable = 1;
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEPORT,&enable,sizeof(int))<0){
        perror("setsockopt error");
    }

    struct sockaddr_in servAddr;
    bzero(&servAddr,sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(portno);

    if((bind(sfd,(struct sockaddr*)&servAddr,sizeof(struct sockaddr_in)))<0){
        perror("bind error");
        exit(0);
    }

    listen(sfd,MAXCLIENTS);

    for(int i = 0;i<N;i++){
        if(fork()==0){
            close(sfd);
            sfd = -1;
            clientFunc(portno);    
            exit(0);
        }
    }

    fd_set rset,allset,waitset;
    int maxfd,maxi,i;
    maxfd = sfd;

    int clientArr[FD_SETSIZE];
    struct sockaddr_in cliAddrArr[FD_SETSIZE];
    int nready;

    struct sockaddr_in cliaddr;

    for(int i = 0;i<FD_SETSIZE;i++){
        clientArr[i] = -1;
    }

    FD_ZERO(&allset);
    FD_ZERO(&waitset);
    FD_SET(sfd,&allset);
    struct timeval tv;
    struct timeval* tp = &tv;
    tp->tv_sec = 20;
    tp->tv_usec = 0;
    int haveToSend = 1;
    while(1){

        rset =  allset;
        nready = select(maxfd+1,&rset,NULL,NULL,tp);
        if(nready<0){
            perror("select error");
            exit(0);
        }

        if(nready==0){
            if(haveToSend==1){
                printf("Server: Sending checks to clients.\n");
                char buffer[6];
                int n;
                strcpy(buffer,"hello");
                buffer[5] = '\0';
                for(i=0;i<=maxi;i++){
                    if(clientArr[i]<0){
                        continue;
                    }
                    
                    if((n=write(clientArr[i],buffer,5)<0)){
                        perror("write error");
                    }

                    FD_SET(clientArr[i],&waitset);

                }
                haveToSend = 0;
            }
            else{

                for(i=0;i<=maxi;i++){
                    if(clientArr[i]<0)continue;

                    if(FD_ISSET(clientArr[i],&waitset)){
                        printf("Server: Closing connection to client IP:%s,PORT:%d\n",inet_ntoa(cliAddrArr[i].sin_addr),cliAddrArr[i].sin_port);
                        close(clientArr[i]);
                        FD_CLR(clientArr[i],&waitset);
                        FD_CLR(clientArr[i],&allset);
                        clientArr[i] = -1;
                        
                    }
                }

                haveToSend = 1;
            }
            tp->tv_sec = 10;
            tp->tv_usec = 0;
        }

        if(FD_ISSET(sfd,&rset)){
            int clilen = sizeof(cliaddr);
            int connfd = accept(sfd,(struct sockaddr*)&cliaddr,&clilen);
            printf("Server: Connected to IP:%s,PORT:%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
            for(i = 0;i<FD_SETSIZE;i++){
                if(clientArr[i]<0){
                    clientArr[i] = connfd;
                    cliAddrArr[i].sin_addr = cliaddr.sin_addr;
                    cliAddrArr[i].sin_port = ntohs(cliaddr.sin_port);
                    break;
                }
            }

            if(i==FD_SETSIZE){
                printf("To many clients.\n");
                exit(0);
            }

            FD_SET(connfd,&allset);
            if(connfd>maxfd){
                maxfd = connfd;
            }
            if(i>maxi){
                maxi = i;
            }
            if(--nready<=0){
                continue;
            }
        }

        buffStruct buffArr[MAXMSGS];
        int currInd = 0;
        for(i=0;i<=maxi;i++){
            if(clientArr[i]<0){
                continue;
            }
            int sockfd =clientArr[i];
            if(FD_ISSET(sockfd,&rset)){
                int n;
                char buffer[6];
                if((n=read(sockfd,buffer,5))<0){
                    perror("read");
                    exit(0);
                }

                if(n==0){
                    printf("Server: Closing connection to client IP:%s,PORT:%d\n",inet_ntoa(cliAddrArr[i].sin_addr),cliAddrArr[i].sin_port);
                    close(sockfd);
                    clientArr[i] = -1;
                    FD_CLR(sockfd,&allset);
                    if(FD_ISSET(sockfd,&waitset)){
                        FD_CLR(sockfd,&waitset);
                    }
                }
                else{
                    buffer[5] = '\0';
                    if(strcmp(buffer,"hello")==0){
                        printf("Server: Received response to check from client IP:%s,PORT:%d\n",inet_ntoa(cliAddrArr[i].sin_addr),cliAddrArr[i].sin_port);
                        if(FD_ISSET(sockfd,&waitset)){
                            FD_CLR(sockfd,&waitset);
                        }
                    }
                    else{
                        if(currInd==MAXMSGS){
                            printf("Too many messages.\n");
                            continue;
                        }
                        strcpy(buffArr[currInd].buffer,buffer);
                        buffArr[currInd].index = i;
                        currInd++;
                    }
                    // if(FD_ISSET(sockfd,&waitset)){
                    //     FD_CLR(sockfd,&waitset);
                    // }
                    // else{
                    //     if(currInd==MAXCLIENTS){
                    //         printf("Too many clients.\n");
                    //         continue;
                    //     }
                    //     strcpy(buffArr[currInd].buffer,buffer);
                    //     buffArr[currInd].index = i;
                    //     currInd++;
                    // }
                }
            }
        }

        if(currInd!=0){
            for(int j = 0;j<currInd;j++){
                for(i = 0;i<=maxi;i++){
                    if((clientArr[i]<0)||(buffArr[j].index==i)){
                        continue;
                    }

                    int n;
                    if((n=write(clientArr[i],buffArr[j].buffer,5))<0){
                        perror("write error");
                    }
                }
            }
        }
    }
}