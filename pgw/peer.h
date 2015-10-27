//
//  peer.h
//  pgw
//
//  Created by hidayat on 10/26/15.
//  Copyright © 2015 hidayat. All rights reserved.
//

#ifndef peer_h
#define peer_h

class peer{
public:
    int id;
    int sockup;
    int sockdown;
    peer(int id,int sockup,int sockdown);
    void start();
};

#endif /* peer_h */
