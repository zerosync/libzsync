/*  =========================================================================
    zsync_outbox - Actor which handles outgoing data.

    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef ZSYNC_OUTBOX_H_INCLUDED
#define ZSYNC_OUTBOX_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zsync_outbox_t zsync_outbox_t;

//  This is the actor that runs in a thread.
void
    zsync_outbox_actor (zsock_t *pipe, void *args);

//  Self test of this class
LIBZSYNC_EXPORT void
    zsync_outbox_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
