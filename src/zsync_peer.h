/*  =========================================================================
    zsync_peer - Internal peer representation

    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef ZSYNC_PEER_H_INCLUDED
#define ZSYNC_PEER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zsync_peer_t zsync_peer_t;

//  @interface
//  Create a new zsync_peer
LIBZSYNC_EXPORT zsync_peer_t *
    zsync_peer_new (char *uuid, uint64_t state);

//  Destroy the zsync_peer
LIBZSYNC_EXPORT void
    zsync_peer_destroy (zsync_peer_t **self_p);

//  Get the UUID of this peer
LIBZSYNC_EXPORT char *
    zsync_peer_uuid (zsync_peer_t *self);

//  Get the state of this peer
LIBZSYNC_EXPORT uint64_t
    zsync_peer_state (zsync_peer_t *self);

//  Print properties of object
LIBZSYNC_EXPORT void
    zsync_peer_print (zsync_peer_t *self);

//  Self test of this class
LIBZSYNC_EXPORT void
    zsync_peer_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
