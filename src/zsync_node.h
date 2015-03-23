/*  =========================================================================
    zsync_node - Internal communication HUB

    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef __ZSYNC_NODE_H_INCLUDED__
#define __ZSYNC_NODE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zsync_node_t zsync_node_t;


// This is the actor that runs a single node; it uses one thread. 
void
zsync_node_actor (zsock_t *pipe, void *args);

//  Self test of this class
LIBZSYNC_EXPORT int
    zsync_node_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
