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

// MAX size of UDP packets
#define BUFSIZE 65536

typedef struct dnsHeader{
    u_int16_t id;
    u_int16_t RD:1;
    u_int16_t QR:1;
    u_int16_t TC:1;
    u_int16_t AA:1;
    u_int16_t opcode:4;
    u_int16_t rcode:4;
    u_int16_t CD:1;
    u_int16_t AD:1;
    u_int16_t Z:1;
    u_int16_t RA:1;
    u_int16_t qdcount;
    u_int16_t ancount;
    u_int16_t nscount;
    u_int16_t arcount;
}dnsHeader;

typedef struct recordDetails{
    u_int16_t type;
    u_int16_t class;
    u_int32_t ttl;
    u_int16_t rdlength;
}recordDetails;

char* getName(char* response,int* length){
    char* result = (char*)malloc(sizeof(char)*100);
    char* ptr = result;
    memset(result,100,0);


    *length = 0;
    char clen;
    int len = 0;
    while(1){
        clen = *response;
        *length += 1;
        response+=1;
        len = (int)clen;
        *length+=len;

        if(len==0){
            break;
        }

        for(int i = 0;i<len;i++){
            *ptr = response[i];
            ptr+=1;
        }
        *ptr = '.';
        ptr+=1;
    }

    return result;
}

int main(int argc,char* argv[]){
    // if(argc!=3){
    //     printf("./a.out <hostname> <query-type>\n");
    //     exit(0);
    // }

    struct sockaddr_in serverAddr;
    memset(&serverAddr,sizeof(serverAddr),0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("8.8.8.8");
    // serverAddr.sin_addr.s_addr = inet_addr("127.0.0.53");
    serverAddr.sin_port = htons(53);

    struct sockaddr_in recvAddr;
    int recvAddrSize = sizeof(serverAddr);

    int dnsSock = socket(AF_INET,SOCK_DGRAM,0);

    while(1){
        printf("Input: <hostname> <query-type>\n");
        char inputBuf[50];
        memset(inputBuf,50,'\0');
        scanf("%[^\n]",inputBuf);
        getchar();

        char* hostname = inputBuf;
        char* queryType = strstr(inputBuf," ");
        *queryType = '\0';
        queryType = queryType+1;

        u_int16_t qtype;
        // printf("%s\n",queryType);
        if(strcmp(queryType,"A")==0){
            qtype = 1;
        }
        else if(strcmp(queryType,"AAAA")==0){
            // How to do this??
            qtype = 100;
        }
        else if(strcmp(queryType,"MX")==0){
            qtype = 15;
        }
        else if(strcmp(queryType,"CNAME")==0){
            qtype = 5;
        }
        else{
            printf("Not valid query type.\n");
            exit(0);
        }

        unsigned char sendbuf[BUFSIZE];
        unsigned char recvbuf[BUFSIZE];

        memset(sendbuf,BUFSIZE,0);
        memset(recvbuf,BUFSIZE,0);

        dnsHeader* header = NULL;
        header = (dnsHeader*)sendbuf;

        header->id = (u_int16_t)getpid();
        header->QR = 0;
        header->opcode = 0;
        header->AA = 0;
        header->TC = 0;
        header->RD = 1;
        header->RA = 0;
        header->Z = 0;
        header->AD = 0;
        header->CD = 0;
        header->rcode = 0;
        header->qdcount = htons(1);
        header->ancount = 0;
        header->nscount = 0;
        header->arcount = 0;

        // printf("%d\n",*(u_int16_t*)sendbuf);

        unsigned char* question = sendbuf + sizeof(dnsHeader);
        unsigned char* check = question;

        char* service = strstr(hostname,":");
        if(service!=NULL){
            hostname = service+1;

        }

        char* dot = strstr(hostname,".");
        if(dot==NULL){
            exit(0);
        }
        char* str = hostname;
        // printf("%s \n",str);
        int totalLen = 0;
        while(*str!='\0'){
            if(dot!=NULL){
                *dot = '\0';
            }
            
            int len = strlen(str);
            totalLen+=len;
            totalLen+=1;

            *question = (char)len;
            // printf("%d\n",*question);
            question = question+1;
            strcpy(question,str);
            question = question + len;
            // printf("%s\n",question);

            if(dot==NULL){
                break;
            }
            str = dot+1;
            dot = strstr(str,".");
        }
        totalLen += 1;
        // *question = '\0';
        question+=1;

        // printf("%s\n",check);
        // query is done. Add query details.

        *(u_int16_t*)question = htons(qtype);
        question = (char*)question + sizeof(u_int16_t);
        *(u_int16_t*)question = htons(1);

        // printf("%d\n",ntohs(*(u_int16_t*)question));
        
        int size = sizeof(dnsHeader) + totalLen + 2*sizeof(u_int16_t);

        // printf("%d\n",size);

        // printf("%d\n",ntohs(*(u_int16_t*)(sendbuf+sizeof(dnsHeader)+18)));

        int n;
        if((n=sendto(dnsSock,(char*)sendbuf,size,0,(struct sockaddr*)&serverAddr,sizeof(serverAddr)))<0){
            perror("sendto");
        }

        // printf("%d\n",n);
        printf("DNS request packet sent.\n");

        if(recvfrom(dnsSock,(char*)recvbuf,BUFSIZE,0,(struct sockaddr*)&serverAddr,&recvAddrSize)<0){
            perror("recvfrom");
        }

        printf("DNS request packet received.\n");

        header = (dnsHeader*)recvbuf;

        char* response = recvbuf+size;

        // Getting to the response of this query. Basically skipping the name.
        // response = response+totalLen;

        int lengthStr;
        char* result = getName(response,&lengthStr);
        printf("%s\n",result);
        // int lengthStr = strlen(result);
        // lengthStr+=1;
        response = response+lengthStr;

        printf("%d\n",lengthStr);

        // response = response+2;


        recordDetails* rDetails = (recordDetails*)response;
        response = response+sizeof(rDetails);
        
        printf("%d\n",ntohs(rDetails->type));


        if(ntohs(rDetails->type)==1){
            struct in_addr aaResponse;
            aaResponse.s_addr = ntohl(*(u_int32_t*)response);
            printf("Reponse: %s\n",inet_ntoa(aaResponse));

        }
        else if(ntohs(rDetails->type)==5){
            char* cname = getName(response,&lengthStr);

            printf("Response: %s\n",cname);
        }
        else if(ntohs(rDetails->type)==15){
            response = response+sizeof(u_int16_t);
            char* mailserver = getName(response,&lengthStr);

            printf("Response: %s\n",mailserver);
        }
        else{
            ;
        }

    }

}