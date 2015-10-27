//
//  sub.cpp
//  pgw
//
//  Created by hidayat on 10/27/15.
//  Copyright © 2015 hidayat. All rights reserved.
//

#include "sub.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

sub::sub(char* id,int sock){
    sub::id=id;
    sub::sock=sock;
}

void sub::start(){
    //send pdpstart
    char buffer[256];
    bzero(buffer, 256);
    //char* msg="pdpstart:1_1:";
    char* host="pdpstart:";
    char h[strlen(host)+strlen(sub::id)];
    strcpy(h,host); // copy string one into the result.
    strcat(h,sub::id); // append string two to the result.
    strcat(h, ":");
    int res=write(sub::sock, h, strlen(h));
    int n = read(sub::sock,buffer,255);
    if (n < 0)
        printf("ERROR reading from socket\n");
    printf("reply %s %s\n",sub::id,buffer);
    close(sub::sock);
}