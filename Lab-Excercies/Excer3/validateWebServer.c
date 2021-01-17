// Jithin Kallukalam Sojan
// 2017A7PS0163P

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BUFSIZE 100

// This is the timeout after which UDP stops waiting for a response. 
#define TIMEOUT 5

#define MAX 50

int checkPort(struct addrinfo* res,int port,int sockType){
    int save;
    int saveSockType = res->ai_socktype;
    res->ai_protocol = 0;
    if(sockType==1){
        res->ai_socktype = SOCK_STREAM;
    }
    else{
        res->ai_socktype = SOCK_DGRAM;
    }

    if(res->ai_family==AF_INET){
        save = ntohs(((struct sockaddr_in*)res->ai_addr)->sin_port);
        ((struct sockaddr_in*)res->ai_addr)->sin_port = htons(port);
    }
    else{
        save = ntohs(((struct sockaddr_in6*)res->ai_addr)->sin6_port);
        ((struct sockaddr_in6*)res->ai_addr)->sin6_port = htons(port);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    int connectCheck = 0;

    if(sockType==1){
        if (sockfd < 0){
            connectCheck = 0;
        }
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0){
            connectCheck = 1;
        }
    }
    else{
        struct timeval time;
        time.tv_sec = TIMEOUT;
        time.tv_usec = 0;

        // Empty payload.
        char sendVal[MAX] = "";
        char recvVal[MAX];

        sendto(sockfd,sendVal,strlen(sendVal),0,res->ai_addr,sizeof(res->ai_addr));
        
        fd_set read;
        FD_ZERO(&read);
        FD_SET(sockfd,&read);
        int selectRet = select(sockfd+1,&read,NULL,NULL,&time);

        if(selectRet<=0){
            connectCheck = -1;
        }
        else{
            if(recvfrom(sockfd,recvVal,MAX,0,NULL,NULL)>=0){
                connectCheck = 1;
            }
            else{
                connectCheck = 0;
            }
        }

    }

    close(sockfd);

    if(res->ai_family==AF_INET){
        ((struct sockaddr_in*)res->ai_addr)->sin_port = htons(save);
    }
    else{
        ((struct sockaddr_in6*)res->ai_addr)->sin6_port = htons(save);
    }
    res->ai_socktype = saveSockType;

    return connectCheck;
}

int main(int argc, char *argv[]){
    int fd = open("webpages.txt",O_RDONLY);
    if(fd==-1){
        perror("open");
        exit(0);
    }
    close(0);
    dup(fd);

    char buffer[BUFSIZE];
    ssize_t size;
    while(1){
        if(!gets(buffer)){
            break;
        }
        if(buffer[0]=='\0')continue;

        printf("%s\n",buffer);

        char* host = buffer;
        char* serv = NULL;
        char* check = strstr(buffer,"://");
        if(check!=NULL){
            host = check+3;
            *check = '\0';
            serv = buffer;
        }
        else{
            printf("No service given!\n");
            printf("\n");
            continue;
        }
        
        check = strstr(host,"/");
        if(check!=NULL){
            *check='\0';
        }

        struct addrinfo hints,*res,*ressave;
        bzero(&hints,sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;

        int n;
        if((n=getaddrinfo(host,serv,&hints,&res))!=0){
            printf("Error on name/service lookup: %s.\n",gai_strerror(n));
            printf("\n");
            continue;
        }
        else{
            printf("Given domain exists.\n");
        }

        ressave = res;

        int portCheck = 0;
        do{

            // Http runs over a TCP connection.
            portCheck = checkPort(res,80,1);
            if(portCheck==1){
                break;
            }
        }
        while((res=res->ai_next)!=NULL);

        if(portCheck==0){
            printf("Given domain does not run webserver at port 80.\n");
        }
        else{
            printf("Given domain runs webserver at port 80.\n");
        }

        // Checking for all the ports that the webserver has open.
        res = ressave;

        int portsTCP[1024];
        int portsUDP[1024];
        for(int i = 0;i<1024;i++)portsTCP[i] = 0;
        for(int i = 0;i<1024;i++)portsUDP[i] = 0;

        char ipAddr[MAX];
        char* ipPresent;

        do{
            if(res->ai_family==AF_INET){
                ipPresent = inet_ntop(res->ai_family,(void*)(&((struct sockaddr_in*)res->ai_addr)->sin_addr),ipAddr,MAX);
            }
            else{
                ipPresent = inet_ntop(res->ai_family,(void*)(&((struct sockaddr_in6*)res->ai_addr)->sin6_addr),ipAddr,MAX);
            }
            printf("Checking ports(1-1024)(TCP & UDP) of ip: ");
            printf("%s\n",ipPresent);

            for(int i = 0;i<1024;i++){
                
                if(!portsTCP[i]){
                    printf("Checking for TCP port %d\n",i+1);
                    int tcpCheck = checkPort(res,i+1,1);
                    if(tcpCheck==0)printf("Port not active.\n");
                    else printf("Port is active.\n");
                    portsTCP[i] |= tcpCheck;
                }
                if(portsUDP[i]!=1){
                    printf("Checking for UDP port %d\n",i+1);
                    int udpCheck = checkPort(res,i+1,0);

                    if(udpCheck==-1)printf("Timeout(5-second wait)! Do not know status of port.\n");
                    else if(udpCheck==0)printf("Port is not active.\n");
                    else printf("Port is active.\n");

                    if(udpCheck==1) portsUDP[i] = 1;
                }
                
            }
        }
        while((res=res->ai_next)!=NULL);

        freeaddrinfo(ressave);
        
        printf("Ports Used\n");
        printf("Active TCP ports: ");
        for(int i = 0;i<1024;i++){
            if(portsTCP[i]==1){
                printf("%d ",i+1);
            }
        }
        printf("\n");

        printf("Active UDP ports: ");
        for(int i = 0;i<1024;i++){
            if(portsUDP[i]==1){
                printf("%d ",i+1);
            }

        }
        printf("\n");

        printf("\n");
        
    }
}