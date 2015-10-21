#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include "avputil.h"
#include "diameter.h"
#include "rocksdb/db.h"

#define MAX_SIZE 50
#define NUM_CLIENT 2
#define NUM_SUB 2
struct arg_struct {
    int arg1;
    int arg2;
};
struct sub_struct {
    int arg1;
    int arg2;
    int arg3;
};
void *connection_handler(void *socket_desc);
void *listenup(void *sock);
void *subs(void *sock);
rocksdb::DB* db;
rocksdb::Options options;
int main()
{
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, "/Users/dayat81/dbfile/pgw", &db);
    assert(status.ok());
    
    int i;
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
void createCCR(int trnum,char* subsid,avp* &allavp,int &l,int &total,char* ccode){
    *ccode=0x00;
    *(ccode+1)=0x01;
    *(ccode+2)=0x10;
    avputil util=avputil();
    char f=0x40;
    char* host="test_host";
    const char* hostid=std::to_string(trnum).c_str();
    char h[strlen(host)+strlen(hostid)];
    strcpy(h,host); // copy string one into the result.
    strcat(h,hostid); // append string two to the result.
    //const char* sid=std::to_string(subsid).c_str();
    char s[strlen(h)+strlen(subsid)+1];
    strcpy(s, h);
    strcat(s, ";");
    strcat(s, subsid);
    
    avp sessid=util.encodeString(263, 0, f, s);
    avp o=util.encodeString(264,0,f,h);
    avp realm=util.encodeString(296,0,f,"test_realm");
    avp reqtype=util.encodeInt32(416, 0, f, 1);
    avp reqnum=util.encodeInt32(415, 0, f, 0);
    avp id_t1=util.encodeInt32(450, 0, 0x40, 0);
    avp id_d1=util.encodeString(444, 0, 0x40, subsid);
    avp* listavp1[2]={&id_t1,&id_d1};
    avp sid1=util.encodeAVP(443, 0, 0x40, listavp1, 2);
    //sessid,reqtype,reqnum,msid
    l=6;
    total=sessid.len+o.len+realm.len+reqtype.len+reqnum.len+sid1.len;
    allavp=new avp[l];
    allavp[0]=sessid;
    allavp[1]=o;
    allavp[2]=realm;
    allavp[3]=reqtype;
    allavp[4]=reqnum;
    allavp[5]=sid1;
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

diameter createReq(int trnum,int type,char* subsid){
    avp* allavp=new avp[1];
    int l;
    int total;
    char* ccode=new char[3];
    char appId[12];//=new char[12];
    bzero(appId, 12);
    if(type==0){
        createCER(trnum,allavp,l,total,ccode);
    }else if(type==1){
        createDWR(trnum,allavp,l,total,ccode);
    }else{
        createCCR(trnum,subsid,allavp,l,total,ccode);
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
void *handlediam(void *diam){
    avputil util=avputil();
    diameter d = *(diameter*)diam;
    d.populateHeader();
    int ccode=((*(d.ccode) & 0xff) << 16)| ((*(d.ccode+1) & 0xff) << 8) | ((*(d.ccode+2)& 0xff));
    //printf("ccode %i\n",ccode);
    //send signal to sub
    if(ccode==272){//cea
        std::string sessidval="";
        avp sessid=d.getAVP(263, 0);
        if(sessid.len>0){
            sessidval=util.decodeAsString(sessid);
            std::cout<<"sessid : "<<sessidval<<std::endl;
            //set sessid status in rocksdb
            rocksdb::Status status = db->Put(rocksdb::WriteOptions(),sessidval ,"OK");
        }
    }
    
    return 0;
}
void *listenup(void *sock){
    int newsock = *(int*)sock;
    char* h=new char[4];
    int n;
    while (1) {
        while((n=read(newsock,h,4))>0){
            int32_t l =(((0x00 & 0xff) << 24) | ((*(h+1) & 0xff) << 16)| ((*(h+2) & 0xff) << 8) | ((*(h+3) & 0xff)))-4;
            char* b=new char[l];
            n = read(newsock,b,l);
            diameter d=diameter(h,b,l);
            //create thread to handle diameter answer
            pthread_t diamthread;
            int dret = pthread_create( &diamthread, NULL, handlediam, (void*)&d);
            if(dret)
            {
                fprintf(stderr,"Error - pthread_create() return code: %d\n",dret);
                exit(EXIT_FAILURE);
            }
        }
        if(n == 0)
        {
            //socket was gracefully closed
            break;
        }
        else if(n < 0)
        {
            //socket error occurred
            break;
        }
    }
    return 0;
}
void *sub(void *args){
    
    struct sub_struct arg = *(struct sub_struct*)args;
    //printf("id %i sock %i extra %i \n",arg.arg1,arg.arg2,arg.arg3);
    //int i=0;
    const char* id=std::to_string(arg.arg1).c_str();
    const char* extra=std::to_string(arg.arg3).c_str();
    char s[strlen(id)+strlen(extra)+1];
    strcpy(s,id); // copy string one into the result.
    strcat(s, "_");
    strcat(s,extra); // append string two to the result.
    //create CCR
    diameter ccr=createReq(arg.arg1, 2,s);
    char* r=new char[ccr.len+4];
    ccr.compose(r);
    delete ccr.h;
    delete ccr.b;
    int w = write(arg.arg2,r,ccr.len+4);
    if(w<=0){
        //fail write
    }
    delete r;
    
    //read status sessid in roksdb
    char* host="test_host";
    //const char* hostid=std::to_string(trnum).c_str();
    char h[strlen(host)+strlen(id)];
    strcpy(h,host); // copy string one into the result.
    strcat(h,id); // append string two to the result.
    //const char* sid=std::to_string(subsid).c_str();
    char sessid[strlen(h)+strlen(s)+1];
    strcpy(sessid, h);
    strcat(sessid, ";");
    strcat(sessid, s);
    
    std::string val;
    rocksdb::Status status;
    bool found=false;
    while (!found) {
        status = db->Get(rocksdb::ReadOptions(),sessid ,&val);
        if (val=="OK") {
            found=true;
        }
    }
    printf("%s completed\n",sessid);
    
    return 0;
}
void *subs(void *args){
    struct arg_struct arg = *(struct arg_struct*)args;
    //printf("id %i sock %i\n",arg.arg1,arg.arg2);
    
    pthread_t sub_thread;
    for (int i=1; i<=NUM_CLIENT; i++) {
        struct sub_struct subarg;
        subarg.arg1=arg.arg1;
        subarg.arg2=arg.arg2;
        subarg.arg3=i;
        if( pthread_create( &sub_thread , NULL ,  sub , (void*) &subarg))
        {
            perror("could not create thread");
        }
        sleep(1);
    }
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
    diameter req=createReq(threadnum,0,"");
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
    //create thread to simulate subsriber
    pthread_t threads;
    
    struct arg_struct args;
    args.arg1=threadnum;
    args.arg2=sock_desc;
    int iret = pthread_create( &threads, NULL, subs, (void*)&args);
    if(iret)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        exit(EXIT_FAILURE);
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
        diameter dwr=createReq(threadnum, 1,"");
        char* r=new char[dwr.len+4];
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