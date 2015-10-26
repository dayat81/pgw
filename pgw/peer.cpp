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
diameter createReq(int trnum,int type,char* subsid){
    avp* allavp=new avp[1];
    int l;
    int total;
    char* ccode=new char[3];
    char appId[12];//=new char[12];
    bzero(appId, 12);

    createDWR(trnum,allavp,l,total,ccode);
 
    
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
    printf("id:%i\n",arg.id);
    printf("sock:%i\n",arg.sock);
    while(1)
    {
        sleep(1);
        //sending watchdog every second
        diameter dwr=createReq(arg.id, 1,"");
        //dwr.dump();
        char* r=new char[dwr.len+4];
        dwr.compose(r);
        delete dwr.h;
        delete dwr.b;
        int w = write(arg.sock,r,dwr.len+4);
        printf("sending dwr\n");
        if(w<=0){
            //fail write
            printf("dwr fail\n");
            break;
        }
        delete r;
        
    }
    close(arg.sock);
    pthread_exit(NULL);
    return 0;
}
void peer::watchdog(){
    //start dwr thread
    struct arg_struct *args=new arg_struct();
    args->id=peer::id;
    args->sock=peer::sockup;
    printf("peer id:%i\n",args->id);
    printf("peer sock:%i\n",args->sock);
    pthread_t threads;
    int iret = pthread_create( &threads, NULL, dwr, (void*)args);
    if(iret)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
    }
}
