#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "avputil.h"
#include "diameter.h"

#define MAX_SIZE 50
#define NUM_CLIENT 2
void *connection_handler(void *socket_desc);
void *listenup(void *sock);
int main()
{
    int socket_desc , new_socket , c , *new_sock, i;
    pthread_t sniffer_thread;
    for (i=1; i<=NUM_CLIENT; i++) {
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) &i) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        sleep(1);
    }
    pthread_exit(NULL);
    return 0;
}
void createDWR(int trnum,avp* &allavp,int &l,int &total,char* ccode){
    *ccode=0x00;
    *(ccode+1)=0x01;
    *(ccode+2)=0x18;
    avputil util=avputil();
    char f=0x40;
    char* host="test_host";
    const char* hostid=std::to_string(trnum).c_str();
    char h[strlen(host)+strlen(hostid)];
    strcpy(h,host); // copy string one into the result.
    strcat(h,hostid); // append string two to the result.
    
    avp o=util.encodeString(264,0,f,h);
    avp realm=util.encodeString(296,0,f,"test_realm");
    l=2;
    total=o.len+realm.len;
    allavp=new avp[l];
    allavp[0]=o;
    allavp[1]=realm;
}
void createCER(int trnum,avp* &allavp,int &l,int &total,char* ccode){
    *ccode=0x00;
    *(ccode+1)=0x01;
    *(ccode+2)=0x01;
    avputil util=avputil();
    char f=0x40;
    char* host="test_host";
    const char* hostid=std::to_string(trnum).c_str();
    char h[strlen(host)+strlen(hostid)];
    strcpy(h,host); // copy string one into the result.
    strcat(h,hostid); // append string two to the result.
    
    avp o=util.encodeString(264,0,f,h);
    avp realm=util.encodeString(296,0,f,"test_realm");
    l=2;
    total=o.len+realm.len;
    allavp=new avp[l];
    allavp[0]=o;
    allavp[1]=realm;
}

diameter createReq(int trnum,int type){
    avp* allavp=new avp[1];
    int l;
    int total;
    char* ccode=new char[3];
    char appId[12];//=new char[12];
    bzero(appId, 12);
    if(type==0){
        createCER(trnum,allavp,l,total,ccode);
    }else{
        createDWR(trnum,allavp,l,total,ccode);
    }
    
    char* h=new char[4];
    *h=0x01;
    
    char cflags=0x80;
    //printf(" avp len %i",o.len);
    //int l_resp=o.len+20+sid.len;
    int l_resp=20+total;
    
    char *ptr1 = (char*)&l_resp;
    char l_byte[3];
    char* lp=l_byte;
    ptr1=ptr1+2;
    int i=0;
    while(i<3){
        *lp=*ptr1;
        lp++;
        ptr1--;
        i++;
    }
    //printf(" lbyte %02X %02X %02X ",l_byte[0],l_byte[1],l_byte[2]);
    *(h+1)=l_byte[0];
    *(h+2)=l_byte[1];
    *(h+3)=l_byte[2];
    //char* h=head;
    //printf(" msg len %i ",l_resp);
    char *b=new char[l_resp-4];
    
    *b=cflags;
    //printf(" ccode %02X %02X %02X ",*d.ccode,*(d.ccode+1),*(d.ccode+2));
    *(b+1)=*ccode;
    *(b+2)=*(ccode+1);
    *(b+3)=*(ccode+2);
    //printf(" copy ccode %02X %02X %02X \n",body[1],body[2],body[3]);
    //copy appid hbh e2e to body
    i=0;
    while (i<12) {
        *(b+i+4)=appId[i];
        i++;
    }
    b=b+16;
    for (i=0; i<l; i++) {
        //copy avp
        char *temp=allavp[i].val;
        //allavp[i].dump();
        //printf("\n");
        for (int j=0; j<allavp[i].len; j++) {
            *b=*temp;
            b++;
            temp++;
        }
        delete allavp[i].val;
    }
    delete allavp;
    b=b-l_resp+4;
    delete ccode;
    diameter answer=diameter(h, b, l_resp-4);
    //answer.dump();
    
    return answer;
}
void *listenup(void *sock){
    return 0;
}
void *connection_handler(void *threadid)
{
    int threadnum = *(int*)threadid;
    int sock_desc;
    struct sockaddr_in serv_addr;
    char sbuff[MAX_SIZE],rbuff[MAX_SIZE];
    
    if((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");
    
    bzero((char *) &serv_addr, sizeof (serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(3868);
    
    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Failed to connect to server\n");
    }
    
    printf("Connected successfully client:%d\n", threadnum);
    //send cer
    diameter req=createReq(threadnum,0);
    char* r=new char[req.len+4];
    req.compose(r);
    delete req.h;
    delete req.b;
    int w = write(sock_desc,r,req.len+4);
    if(w<=0){
        //fail write
    }
    delete r;
    //wait cea
    char* h=new char[4];
    int n=read(sock_desc,h,4);
    if(n>0){
        int32_t l =(((0x00 & 0xff) << 24) | ((*(h+1) & 0xff) << 16)| ((*(h+2) & 0xff) << 8) | ((*(h+3) & 0xff)))-4;
        char* b=new char[l];
        n = read(sock_desc,b,l);
        diameter d=diameter(h,b,l);
        //d.dump();
    }
    //create thread to listen data from upstream
    pthread_t thread;
    int iret1 = pthread_create( &thread, NULL, listenup, (void*)&sock_desc);
    if(iret1)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        sleep(3);
        //sending watchdog every second
        diameter dwr=createReq(threadnum, 1);
        char* r=new char[req.len+4];
        dwr.compose(r);
        delete dwr.h;
        delete dwr.b;
        int w = write(sock_desc,r,dwr.len+4);
        if(w<=0){
            //fail write
        }
        delete r;
        
    }
    close(sock_desc);
    return 0;
}