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
    char* msg="pdpstart\n";
    int res=write(sub::sock, msg, strlen(msg));
    int n = read(sub::sock,buffer,255);
    if (n < 0)
        printf("ERROR reading from socket\n");
    printf("reply %s\n",buffer);
    close(sub::sock);
}