/*  =========================================================================
    zsync - Public API

    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef __ZSYNC_H_INCLUDED__
#define __ZSYNC_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif


//  @interface
//  Create a new zsync
LIBZSYNC_EXPORT zsync_t *
    zsync_new ();

//  Destroy the zsync
LIBZSYNC_EXPORT void
    zsync_destroy (zsync_t **self_p);

//  Start the zsync node
LIBZSYNC_EXPORT int
    zsync_start (zsync_t *self);

//  Stop the zsync node
LIBZSYNC_EXPORT void
    zsync_stop (zsync_t *self);

//  Print properties of object
LIBZSYNC_EXPORT void
    zsync_print (zsync_t *self);

//  Self test of this class
LIBZSYNC_EXPORT int
    zsync_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
