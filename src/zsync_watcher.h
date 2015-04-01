/*  =========================================================================
    zsync_watcher - Actor which watches the filesystem for changes

    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef ZSYNC_WATCHER_H_INCLUDED
#define ZSYNC_WATCHER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zsync_watcher_t zsync_watcher_t;

//  @interface
// This is the actor that runs a single node; it uses one thread.
LIBZSYNC_EXPORT void
    zsync_watcher_actor (zsock_t *pipe, void *args);

//  Self test of this actor
LIBZSYNC_EXPORT void
    zsync_watcher_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
