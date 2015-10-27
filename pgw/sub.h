//
//  sub.h
//  pgw
//
//  Created by hidayat on 10/27/15.
//  Copyright © 2015 hidayat. All rights reserved.
//

#ifndef sub_h
#define sub_h

class sub{
public:
    char* id;
    int sock;
    sub(char* id,int sock);
    void start();
};

#endif /* sub_h */
