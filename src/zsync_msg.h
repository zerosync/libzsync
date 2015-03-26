/*  =========================================================================
    zsync_msg - the API which is used to comminicate with user interface clients
    
    Codec header for zsync_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zsync_msg.xml, or
     * The code generation script that built this file: zproto_codec_c_v1
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

    HELLO - Greet a new peer with it's known state.
        state               number 8    State of the peer we're greeting

    UPDATE - Sends a list of updated files to the client.
        sender              string      UUID that identifies the sender
        update_msg          msg         List of updated files and their metadata

    FILES - Requests a list of files from receiver.
        receiver            string      UUID that identifies the receiver
        files               strings     List of file names
        size                number 8    Total size of all files in bytes

    CREDIT - Sends a credit amount for requested files.
        amount              number 8    Credit amount in bytes

    CHUNK - Sends one 'chunk' of data of a file at the 'path'.
        chunk               chunk       This chunk is part of the file at 'path'
        path                string      Path of file that the 'chunk' belongs to 
        sequence            number 8    Defines which chunk of the file at 'path' this is!
        offset              number 8    Offset for this 'chunk' in bytes

    ABORT - Sends an abort for one file at path.
        receiver            string      UUID that identifies the receiver
        path                string      
*/


#define ZSYNC_MSG_HELLO                     1
#define ZSYNC_MSG_UPDATE                    2
#define ZSYNC_MSG_FILES                     3
#define ZSYNC_MSG_CREDIT                    4
#define ZSYNC_MSG_CHUNK                     5
#define ZSYNC_MSG_ABORT                     6

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
//  Create a new zsync_msg
LIBZSYNC_EXPORT zsync_msg_t *
    zsync_msg_new (int id);

//  Destroy the zsync_msg
LIBZSYNC_EXPORT void
    zsync_msg_destroy (zsync_msg_t **self_p);

//  Parse a zmsg_t and decides whether it is zsync_msg. Returns
//  true if it is, false otherwise. Doesn't destroy or modify the
//  original message.
LIBZSYNC_EXPORT bool
    is_zsync_msg (zmsg_t *msg_p);

//  Parse a zsync_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.
LIBZSYNC_EXPORT zsync_msg_t *
    zsync_msg_decode (zmsg_t **msg_p);

//  Encode zsync_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode (zsync_msg_t **self_p);

//  Receive and parse a zsync_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
LIBZSYNC_EXPORT zsync_msg_t *
    zsync_msg_recv (void *input);

//  Receive and parse a zsync_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
LIBZSYNC_EXPORT zsync_msg_t *
    zsync_msg_recv_nowait (void *input);

//  Send the zsync_msg to the output, and destroy it
LIBZSYNC_EXPORT int
    zsync_msg_send (zsync_msg_t **self_p, void *output);

//  Send the zsync_msg to the output, and do not destroy it
LIBZSYNC_EXPORT int
    zsync_msg_send_again (zsync_msg_t *self, void *output);

//  Encode the HELLO 
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode_hello (
        uint64_t state);

//  Encode the UPDATE 
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode_update (
        const char *sender,
        zmsg_t *update_msg);

//  Encode the FILES 
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode_files (
        const char *receiver,
        zlist_t *files,
        uint64_t size);

//  Encode the CREDIT 
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode_credit (
        uint64_t amount);

//  Encode the CHUNK 
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode_chunk (
        zchunk_t *chunk,
        const char *path,
        uint64_t sequence,
        uint64_t offset);

//  Encode the ABORT 
LIBZSYNC_EXPORT zmsg_t *
    zsync_msg_encode_abort (
        const char *receiver,
        const char *path);


//  Send the HELLO to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
LIBZSYNC_EXPORT int
    zsync_msg_send_hello (void *output,
        uint64_t state);
    
//  Send the UPDATE to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
LIBZSYNC_EXPORT int
    zsync_msg_send_update (void *output,
        const char *sender,
        zmsg_t *update_msg);
    
//  Send the FILES to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
LIBZSYNC_EXPORT int
    zsync_msg_send_files (void *output,
        const char *receiver,
        zlist_t *files,
        uint64_t size);
    
//  Send the CREDIT to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
LIBZSYNC_EXPORT int
    zsync_msg_send_credit (void *output,
        uint64_t amount);
    
//  Send the CHUNK to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
LIBZSYNC_EXPORT int
    zsync_msg_send_chunk (void *output,
        zchunk_t *chunk,
        const char *path,
        uint64_t sequence,
        uint64_t offset);
    
//  Send the ABORT to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
LIBZSYNC_EXPORT int
    zsync_msg_send_abort (void *output,
        const char *receiver,
        const char *path);
    
//  Duplicate the zsync_msg message
LIBZSYNC_EXPORT zsync_msg_t *
    zsync_msg_dup (zsync_msg_t *self);

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
    zsync_msg_set_sender (zsync_msg_t *self, const char *format, ...);

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
    zsync_msg_set_receiver (zsync_msg_t *self, const char *format, ...);

//  Get/set the files field
LIBZSYNC_EXPORT zlist_t *
    zsync_msg_files (zsync_msg_t *self);
//  Get the files field and transfer ownership to caller
LIBZSYNC_EXPORT zlist_t *
    zsync_msg_get_files (zsync_msg_t *self);
//  Set the files field, transferring ownership from caller
LIBZSYNC_EXPORT void
    zsync_msg_set_files (zsync_msg_t *self, zlist_t **files_p);

//  Iterate through the files field, and append a files value
LIBZSYNC_EXPORT const char *
    zsync_msg_files_first (zsync_msg_t *self);
LIBZSYNC_EXPORT const char *
    zsync_msg_files_next (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_files_append (zsync_msg_t *self, const char *format, ...);
LIBZSYNC_EXPORT size_t
    zsync_msg_files_size (zsync_msg_t *self);

//  Get/set the size field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_size (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_size (zsync_msg_t *self, uint64_t size);

//  Get/set the amount field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_amount (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_amount (zsync_msg_t *self, uint64_t amount);

//  Get a copy of the chunk field
LIBZSYNC_EXPORT zchunk_t *
    zsync_msg_chunk (zsync_msg_t *self);
//  Get the chunk field and transfer ownership to caller
LIBZSYNC_EXPORT zchunk_t *
    zsync_msg_get_chunk (zsync_msg_t *self);
//  Set the chunk field, transferring ownership from caller
LIBZSYNC_EXPORT void
    zsync_msg_set_chunk (zsync_msg_t *self, zchunk_t **chunk_p);

//  Get/set the path field
LIBZSYNC_EXPORT const char *
    zsync_msg_path (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_path (zsync_msg_t *self, const char *format, ...);

//  Get/set the sequence field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_sequence (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_sequence (zsync_msg_t *self, uint64_t sequence);

//  Get/set the offset field
LIBZSYNC_EXPORT uint64_t
    zsync_msg_offset (zsync_msg_t *self);
LIBZSYNC_EXPORT void
    zsync_msg_set_offset (zsync_msg_t *self, uint64_t offset);

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
