//
//  peer.cpp
//  pgw
//
//  Created by hidayat on 10/26/15.
//  Copyright © 2015 hidayat. All rights reserved.
//

#include "peer.h"
#include "diameter.h"
#include "avputil.h"
#include <pthread.h>
#include <netinet/in.h>

struct arg_struct {
    int id;
    int sock;
};

peer::peer(int id,int sockup,int sockdown){
    peer::id=id;
    peer::sockup=sockup;
    peer::sockdown=sockdown;
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
void *dwr(void *args){
    struct arg_struct arg = *(struct arg_struct*)args;
//    printf("id:%i\n",arg.id);
//    printf("sock:%i\n",arg.sock);
    while(1)
    {
        sleep(5);
        //sending watchdog every second
        diameter dwr=createReq(arg.id, 1,"");
        //dwr.dump();
        char* r=new char[dwr.len+4];
        dwr.compose(r);
        delete dwr.h;
        delete dwr.b;
        int w = write(arg.sock,r,dwr.len+4);
        //printf("sending dwr\n");
        if(w<=0){
            //fail write
            printf("sending dwr fail\n");
            break;
        }
        delete r;
        
    }
    close(arg.sock);
    pthread_exit(NULL);
    return 0;
}
void *handlediam(void *diam){
    avputil util=avputil();
    diameter d = *(diameter*)diam;
    d.populateHeader();
    int ccode=((*(d.ccode) & 0xff) << 16)| ((*(d.ccode+1) & 0xff) << 8) | ((*(d.ccode+2)& 0xff));
    printf("ccode %i\n",ccode);
    //send signal to sub
    if(ccode==272){//cea
        std::string sessidval="";
        avp sessid=d.getAVP(263, 0);
        if(sessid.len>0){
            sessidval=util.decodeAsString(sessid);
            //std::cout<<"sessid : "<<sessidval<<std::endl;
        }
    }
//    delete d.h;
//    delete d.b;
    pthread_exit(NULL);
    return 0;
}
void *listenup(void *sock){
    int newsock = *(int*)sock;
    char* h=new char[4];
    int n;
    //while (1) {
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
            }
        }
        if(n == 0)
        {
            //socket was gracefully closed
            //break;
        }
        else if(n <= 0)
        {
            //socket error occurred
            //break;
        }
    //}
    pthread_exit(NULL);
    return 0;
}
void *handlesub(void *sock){
    int newsock = *(int*)sock;
    int bytes;
    char cClientMessage[32];
    char result[1024];
    while((bytes = recv(newsock, cClientMessage, sizeof(cClientMessage), 0)) > 0)
    {
        printf("submsg:%s\n",cClientMessage);
    }
    pthread_exit(NULL);
    return 0;
}
void *listendown(void *socket){
    struct sockaddr cli_addr;
    socklen_t clilen;
    int sock = *(int*)socket;
    while(1){
        int newsock = accept(sock, &cli_addr, &clilen);
        if (newsock == -1) {
            perror("accept");
            return 0;
        }
        printf("sub connected\n");
        pthread_t thread3;
        int iret3 = pthread_create( &thread3, NULL, handlesub, (void*)&newsock);
        if(iret3)
        {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",iret3);
        }
    }
    pthread_exit(NULL);
    return 0;
}
void peer::start(){
    struct arg_struct *args=new arg_struct();
    //send cer
    diameter req=createReq(peer::id,0,"");
    char* r=new char[req.len+4];
    req.compose(r);
    delete req.h;
    delete req.b;
    int w = write(peer::sockup,r,req.len+4);
    if(w<=0){
        //fail write
    }
    delete r;
    //wait cea
    char* h=new char[4];
    int n=read(peer::sockup,h,4);
    if(n>0){
        int32_t l =(((0x00 & 0xff) << 24) | ((*(h+1) & 0xff) << 16)| ((*(h+2) & 0xff) << 8) | ((*(h+3) & 0xff)))-4;
        char* b=new char[l];
        n = read(peer::sockup,b,l);
        diameter d=diameter(h,b,l);
        //d.dump();
    }
    //start dwr thread
    args->id=peer::id;
    args->sock=peer::sockup;
//    printf("peer id:%i\n",args->id);
//    printf("peer sock:%i\n",args->sock);
    pthread_t threads;
    int iret = pthread_create( &threads, NULL, dwr, (void*)args);
    if(iret)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
    }
    //create thread to listenup
    pthread_t thread1;
    int sock=peer::sockup;
    int iret1 = pthread_create( &thread1, NULL, listenup, (void*)&sock);
    if(iret1)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
    }
    //create thread to listendown
    pthread_t thread2;
    int sockdown=peer::sockdown;
    int iret2 = pthread_create( &thread2, NULL, listendown, (void*)&sockdown);
    if(iret2)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
    }
}
