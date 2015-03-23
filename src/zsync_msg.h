/*  =========================================================================
    zsync_msg - the API which is used to comminicate with user interface clients
    
    Codec header for zsync_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zsync_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.         
    This file is part of libzsync, the peer to peer file sharing library:
    http://zerosync.org.                                                 
                                                                         
    This Source Code Form is subject to the terms of the Mozilla Public  
    License, v. 2.0. If a copy of the MPL was not distributed with this  
    file, You can obtain one at http://mozilla.org/MPL/2.0/.             
    =========================================================================
*/

#ifndef ZSYNC_MSG_H_INCLUDED
#define ZSYNC_MSG_H_INCLUDED

/*  These are the zsync_msg messages:

    REQ_STATE - Requests the current state.

    RES_STATE - Responds to REQ_STATE with current state.
        state               number 8    

    REQ_UPDATE - Requests an update for all changes with a newer state then 'state'.
        state               number 8    

    UPDATE - Sends a list of updated files to the client.
        sender              string      UUID that identifies the sender
        update_msg          msg         List of updated files and their metadata

    REQ_FILES - Requests a list of files from receiver.
        receiver            string      UUID that identifies the receiver
        files               strings     List of file names
        size                number 8    Total size of all files in bytes

    REQ_CHUNK - Requests a chunk of 'chunk_size' data from 'path' at 'offset'.
        path                string      Path of file that the 'chunk' belongs to 
        chunk_size          number 8    Size of the requested chunk in bytes
        offset              number 8    File offset for for the chunk in bytes

    RES_CHUNK - Responds with the requested chunk.
        chunk               chunk       Requested chunk

    CHUNK - Sends one 'chunk' of data of a file at the 'path'.
        chunk               chunk       This chunk is part of the file at 'path'
        path                string      Path of file that the 'chunk' belongs to 
        sequence            number 8    Defines which chunk of the file at 'path' this is!
        offset              number 8    Offset for this 'chunk' in bytes

    ABORT - Sends an abort for one file at path.
        receiver            string      UUID that identifies the receiver
        path                string      

    TERMINATE - Terminate all worker threads.
*/


#define ZSYNC_MSG_REQ_STATE                 1
#define ZSYNC_MSG_RES_STATE                 2
#define ZSYNC_MSG_REQ_UPDATE                3
#define ZSYNC_MSG_UPDATE                    4
#define ZSYNC_MSG_REQ_FILES                 5
#define ZSYNC_MSG_REQ_CHUNK                 6
#define ZSYNC_MSG_RES_CHUNK                 7
#define ZSYNC_MSG_CHUNK                     8
#define ZSYNC_MSG_ABORT                     9
#define ZSYNC_MSG_TERMINATE                 10

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef ZSYNC_MSG_T_DEFINED
typedef struct _zsync_msg_t zsync_msg_t;
#define ZSYNC_MSG_T_DEFINED
#endif

//  @interface
//  Create a new empty zsync_msg
LIBZSYNC_EXPORT zsync_msg_t *
    zsync_msg_new (void);

//  Destroy a zsync_msg instance
LIBZSYNC_EXPORT void
    zsync_msg_destroy (zsync_msg_t **self_p);

//  Receive a zsync_msg from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.
LIBZSYNC_EXPORT int
    zsync_msg_recv (zsync_msg_t *self, zsock_t *input);

//  Send the zsync_msg to the output socket, does not destroy it
LIBZSYNC_EXPORT int
    zsync_msg_send (zsync_msg_t *self, zsock_t *output);
    
//  Print contents of message to stdout
LIBZSYNC_EXPORT void
    zsync_msg_print (zsync_msg_t *self);

//  Get/set the message routing id
LIBZSYNC_EXPORT zframe_t *
    zsync_msg_routing_id (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_routing_id (zsync_msg_t *self, zframe_t *routing_id);

//  Get the zsync_msg id and printable command
LIBZSYNC_EXPORT int
    zsync_msg_id (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_id (zsync_msg_t *self, int id);
LIBZSYNC_EXPORT const char *
    zsync_msg_command (zsync_msg_t *self);

//  Get/set the state field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_state (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_state (zsync_msg_t *self, uint64_t state);

//  Get/set the sender field
LIBZSYNC_EXPORT const char *
    zsync_msg_sender (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_sender (zsync_msg_t *self, const char *value);

//  Get a copy of the update_msg field
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_update_msg (zsync_msg_t *self);
//  Get the update_msg field and transfer ownership to caller
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_get_update_msg (zsync_msg_t *self);
//  Set the update_msg field, transferring ownership from caller
LIBZSYNC_EXPORT void
    zsync_msg_set_update_msg (zsync_msg_t *self, zmsg_t **msg_p);

//  Get/set the receiver field
LIBZSYNC_EXPORT const char *
    zsync_msg_receiver (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_receiver (zsync_msg_t *self, const char *value);

//  Get/set the files field
LIBZSYNC_EXPORT zlist_t *
    zsync_msg_files (zsync_msg_t *self);
//  Get the files field and transfer ownership to caller
LIBZSYNC_EXPORT zlist_t *
    zsync_msg_get_files (zsync_msg_t *self);
//  Set the files field, transferring ownership from caller
LIBZSYNC_EXPORT void
    zsync_msg_set_files (zsync_msg_t *self, zlist_t **files_p);

//  Get/set the size field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_size (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_size (zsync_msg_t *self, uint64_t size);

//  Get/set the path field
LIBZSYNC_EXPORT const char *
    zsync_msg_path (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_path (zsync_msg_t *self, const char *value);

//  Get/set the chunk_size field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_chunk_size (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_chunk_size (zsync_msg_t *self, uint64_t chunk_size);

//  Get/set the offset field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_offset (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_offset (zsync_msg_t *self, uint64_t offset);

//  Get a copy of the chunk field
LIBZSYNC_EXPORT zchunk_t *
    zsync_msg_chunk (zsync_msg_t *self);
//  Get the chunk field and transfer ownership to caller
LIBZSYNC_EXPORT zchunk_t *
    zsync_msg_get_chunk (zsync_msg_t *self);
//  Set the chunk field, transferring ownership from caller
LIBZSYNC_EXPORT void
    zsync_msg_set_chunk (zsync_msg_t *self, zchunk_t **chunk_p);

//  Get/set the sequence field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_sequence (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_sequence (zsync_msg_t *self, uint64_t sequence);

//  Self test of this class
LIBZSYNC_EXPORT int
    zsync_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define zsync_msg_dump      zsync_msg_print

#ifdef __cplusplus
}
#endif

#endif
