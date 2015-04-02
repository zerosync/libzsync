/*  =========================================================================
    zsync_file - Representation of a syncronized file.

    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef __ZSYNC_FILE_H_INCLUDED__
#define __ZSYNC_FILE_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

#define ZS_FILE_OP_UPD 0x1
#define ZS_FILE_OP_DEL 0x2
#define ZS_FILE_OP_REN 0x3

// Opaque class structure
typedef struct _zsync_file_t zsync_file_t;

// @interface
LIBZSYNC_EXPORT int
    zsync_compare (void *item1, void *item2);

// Constructor, creates new zs file meta data
LIBZSYNC_EXPORT zsync_file_t *
    zsync_file_new ();

// Destructor, destroys file meta data
LIBZSYNC_EXPORT void
    zsync_file_destroy (zsync_file_t **self_p);

// getter/setter file path
LIBZSYNC_EXPORT void
    zsync_file_set_path (zsync_file_t *self, char* format, ...);

LIBZSYNC_EXPORT char *
    zsync_file_path (zsync_file_t *self);

// getter/setter renamed file path
LIBZSYNC_EXPORT void
    zsync_file_set_renamed_path (zsync_file_t *self, char* format, ...);

LIBZSYNC_EXPORT char *
    zsync_file_renamed_path (zsync_file_t *self);


// getter/setter file operation
LIBZSYNC_EXPORT void
    zsync_file_set_operation(zsync_file_t *self, int operation);
     
LIBZSYNC_EXPORT int
    zsync_file_operation(zsync_file_t *self);

// getter/setter file size
LIBZSYNC_EXPORT void
    zsync_file_set_size (zsync_file_t *self, uint64_t size);

LIBZSYNC_EXPORT uint64_t 
    zsync_file_size (zsync_file_t *self);

// getter/setter timestamp
LIBZSYNC_EXPORT void
    zsync_file_set_timestamp (zsync_file_t *self, uint64_t timestamp);

LIBZSYNC_EXPORT uint64_t
    zsync_file_timestamp (zsync_file_t *self);

// getter/setter checksum
LIBZSYNC_EXPORT void
    zsync_file_set_checksum (zsync_file_t *self, uint64_t checksum);

LIBZSYNC_EXPORT uint64_t
    zsync_file_checksum (zsync_file_t *self);

// Self test this class
LIBZSYNC_EXPORT int
    zsync_file_test ();
// @end

#ifdef __cplusplus
}
#endif

#endif
