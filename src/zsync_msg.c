/*  =========================================================================
    zsync_msg - the API which is used to comminicate with user interface clients

    Codec class for zsync_msg.

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

/*
@header
    zsync_msg - the API which is used to comminicate with user interface clients
@discuss
@end
*/

#include "../include/libzsync.h"
#include "./zsync_msg.h"

//  Structure of our class

struct _zsync_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  zsync_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    uint64_t state;                     //  State of the peer we're greeting
    char *sender;                       //  UUID that identifies the sender
    zmsg_t *update_msg;                 //  List of updated files and their metadata
    char *receiver;                     //  UUID that identifies the receiver
    zlist_t *files;                     //  List of file names
    uint64_t size;                      //  Total size of all files in bytes
    uint64_t amount;                    //  Credit amount in bytes
    zchunk_t *chunk;                    //  This chunk is part of the file at 'path'
    char *path;                         //  Path of file that the 'chunk' belongs to 
    uint64_t sequence;                  //  Defines which chunk of the file at 'path' this is!
    uint64_t offset;                    //  Offset for this 'chunk' in bytes
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new zsync_msg

zsync_msg_t *
zsync_msg_new (int id)
{
    zsync_msg_t *self = (zsync_msg_t *) zmalloc (sizeof (zsync_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync_msg

void
zsync_msg_destroy (zsync_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->sender);
        zmsg_destroy (&self->update_msg);
        free (self->receiver);
        if (self->files)
            zlist_destroy (&self->files);
        zchunk_destroy (&self->chunk);
        free (self->path);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  Parse a zmsg_t and decides whether it is zsync_msg. Returns
//  true if it is, false otherwise. Doesn't destroy or modify the
//  original message.
bool
is_zsync_msg (zmsg_t *msg)
{
    if (msg == NULL)
        return false;

    zframe_t *frame = zmsg_first (msg);

    //  Get and check protocol signature
    zsync_msg_t *self = zsync_msg_new (0);
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0))
        goto fail;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_MSG_HELLO:
        case ZSYNC_MSG_UPDATE:
        case ZSYNC_MSG_FILES:
        case ZSYNC_MSG_CREDIT:
        case ZSYNC_MSG_CHUNK:
        case ZSYNC_MSG_ABORT:
            zsync_msg_destroy (&self);
            return true;
        default:
            goto fail;
    }
    fail:
    malformed:
        zsync_msg_destroy (&self);
        return false;
}

//  --------------------------------------------------------------------------
//  Parse a zsync_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.

zsync_msg_t *
zsync_msg_decode (zmsg_t **msg_p)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;
        
    zsync_msg_t *self = zsync_msg_new (0);
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_MSG_HELLO:
            GET_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_UPDATE:
            GET_STRING (self->sender);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->update_msg = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->update_msg, zmsg_pop (msg));
            break;

        case ZSYNC_MSG_FILES:
            GET_STRING (self->receiver);
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                self->files = zlist_new ();
                zlist_autofree (self->files);
                while (list_size--) {
                    char *string;
                    GET_LONGSTR (string);
                    zlist_append (self->files, string);
                    free (string);
                }
            }
            GET_NUMBER8 (self->size);
            break;

        case ZSYNC_MSG_CREDIT:
            GET_NUMBER8 (self->amount);
            break;

        case ZSYNC_MSG_CHUNK:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->chunk = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            GET_STRING (self->path);
            GET_NUMBER8 (self->sequence);
            GET_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_ABORT:
            GET_STRING (self->receiver);
            GET_STRING (self->path);
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        zsys_error ("malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        zsync_msg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Encode zsync_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.

zmsg_t *
zsync_msg_encode (zsync_msg_t **self_p)
{
    assert (self_p);
    assert (*self_p);
    
    zsync_msg_t *self = *self_p;
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZSYNC_MSG_HELLO:
            //  state is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_UPDATE:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            break;
            
        case ZSYNC_MSG_FILES:
            //  receiver is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->receiver)
                frame_size += strlen (self->receiver);
            //  files is an array of strings
            frame_size += 4;    //  Size is 4 octets
            if (self->files) {
                //  Add up size of list contents
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    frame_size += 4 + strlen (files);
                    files = (char *) zlist_next (self->files);
                }
            }
            //  size is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_CREDIT:
            //  amount is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_CHUNK:
            //  chunk is a chunk with 4-byte length
            frame_size += 4;
            if (self->chunk)
                frame_size += zchunk_size (self->chunk);
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            //  sequence is a 8-byte integer
            frame_size += 8;
            //  offset is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_ABORT:
            //  receiver is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->receiver)
                frame_size += strlen (self->receiver);
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            break;
            
        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_MSG_HELLO:
            PUT_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_UPDATE:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case ZSYNC_MSG_FILES:
            if (self->receiver) {
                PUT_STRING (self->receiver);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->files) {
                PUT_NUMBER4 (zlist_size (self->files));
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    PUT_LONGSTR (files);
                    files = (char *) zlist_next (self->files);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty string array
            PUT_NUMBER8 (self->size);
            break;

        case ZSYNC_MSG_CREDIT:
            PUT_NUMBER8 (self->amount);
            break;

        case ZSYNC_MSG_CHUNK:
            if (self->chunk) {
                PUT_NUMBER4 (zchunk_size (self->chunk));
                memcpy (self->needle,
                        zchunk_data (self->chunk),
                        zchunk_size (self->chunk));
                self->needle += zchunk_size (self->chunk);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            if (self->path) {
                PUT_STRING (self->path);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->sequence);
            PUT_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_ABORT:
            if (self->receiver) {
                PUT_STRING (self->receiver);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->path) {
                PUT_STRING (self->path);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        zsync_msg_destroy (self_p);
        return NULL;
    }
    //  Now send the message field if there is any
    if (self->id == ZSYNC_MSG_UPDATE) {
        if (self->update_msg) {
            zframe_t *frame = zmsg_pop (self->update_msg);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->update_msg);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Destroy zsync_msg object
    zsync_msg_destroy (self_p);
    return msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a zsync_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zsync_msg_t *
zsync_msg_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    if (!msg)
        return NULL;            //  Interrupted

    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsock_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    zsync_msg_t *zsync_msg = zsync_msg_decode (&msg);
    if (zsync_msg && zsock_type (zsock_resolve (input)) == ZMQ_ROUTER)
        zsync_msg->routing_id = routing_id;

    return zsync_msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a zsync_msg from the socket. Returns new object,
//  or NULL either if there was no input waiting, or the recv was interrupted.

zsync_msg_t *
zsync_msg_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsock_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    zsync_msg_t *zsync_msg = zsync_msg_decode (&msg);
    if (zsync_msg && zsock_type (zsock_resolve (input)) == ZMQ_ROUTER)
        zsync_msg->routing_id = routing_id;

    return zsync_msg;
}


//  --------------------------------------------------------------------------
//  Send the zsync_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zsync_msg_send (zsync_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    //  Save routing_id if any, as encode will destroy it
    zsync_msg_t *self = *self_p;
    zframe_t *routing_id = self->routing_id;
    self->routing_id = NULL;

    //  Encode zsync_msg message to a single zmsg
    zmsg_t *msg = zsync_msg_encode (self_p);
    
    //  If we're sending to a ROUTER, send the routing_id first
    if (zsock_type (zsock_resolve (output)) == ZMQ_ROUTER) {
        assert (routing_id);
        zmsg_prepend (msg, &routing_id);
    }
    else
        zframe_destroy (&routing_id);
        
    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the zsync_msg to the output, and do not destroy it

int
zsync_msg_send_again (zsync_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = zsync_msg_dup (self);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Encode HELLO message

zmsg_t * 
zsync_msg_encode_hello (
    uint64_t state)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_HELLO);
    zsync_msg_set_state (self, state);
    return zsync_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode UPDATE message

zmsg_t * 
zsync_msg_encode_update (
    const char *sender,
    zmsg_t *update_msg)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_UPDATE);
    zsync_msg_set_sender (self, "%s", sender);
    zmsg_t *update_msg_copy = zmsg_dup (update_msg);
    zsync_msg_set_update_msg (self, &update_msg_copy);
    return zsync_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode FILES message

zmsg_t * 
zsync_msg_encode_files (
    const char *receiver,
    zlist_t *files,
    uint64_t size)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_FILES);
    zsync_msg_set_receiver (self, "%s", receiver);
    zlist_t *files_copy = zlist_dup (files);
    zsync_msg_set_files (self, &files_copy);
    zsync_msg_set_size (self, size);
    return zsync_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CREDIT message

zmsg_t * 
zsync_msg_encode_credit (
    uint64_t amount)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_CREDIT);
    zsync_msg_set_amount (self, amount);
    return zsync_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CHUNK message

zmsg_t * 
zsync_msg_encode_chunk (
    zchunk_t *chunk,
    const char *path,
    uint64_t sequence,
    uint64_t offset)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_CHUNK);
    zchunk_t *chunk_copy = zchunk_dup (chunk);
    zsync_msg_set_chunk (self, &chunk_copy);
    zsync_msg_set_path (self, "%s", path);
    zsync_msg_set_sequence (self, sequence);
    zsync_msg_set_offset (self, offset);
    return zsync_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode ABORT message

zmsg_t * 
zsync_msg_encode_abort (
    const char *receiver,
    const char *path)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_ABORT);
    zsync_msg_set_receiver (self, "%s", receiver);
    zsync_msg_set_path (self, "%s", path);
    return zsync_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the HELLO to the socket in one step

int
zsync_msg_send_hello (
    void *output,
    uint64_t state)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_HELLO);
    zsync_msg_set_state (self, state);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the UPDATE to the socket in one step

int
zsync_msg_send_update (
    void *output,
    const char *sender,
    zmsg_t *update_msg)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_UPDATE);
    zsync_msg_set_sender (self, sender);
    zmsg_t *update_msg_copy = zmsg_dup (update_msg);
    zsync_msg_set_update_msg (self, &update_msg_copy);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the FILES to the socket in one step

int
zsync_msg_send_files (
    void *output,
    const char *receiver,
    zlist_t *files,
    uint64_t size)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_FILES);
    zsync_msg_set_receiver (self, receiver);
    zlist_t *files_copy = zlist_dup (files);
    zsync_msg_set_files (self, &files_copy);
    zsync_msg_set_size (self, size);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CREDIT to the socket in one step

int
zsync_msg_send_credit (
    void *output,
    uint64_t amount)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_CREDIT);
    zsync_msg_set_amount (self, amount);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CHUNK to the socket in one step

int
zsync_msg_send_chunk (
    void *output,
    zchunk_t *chunk,
    const char *path,
    uint64_t sequence,
    uint64_t offset)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_CHUNK);
    zchunk_t *chunk_copy = zchunk_dup (chunk);
    zsync_msg_set_chunk (self, &chunk_copy);
    zsync_msg_set_path (self, path);
    zsync_msg_set_sequence (self, sequence);
    zsync_msg_set_offset (self, offset);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ABORT to the socket in one step

int
zsync_msg_send_abort (
    void *output,
    const char *receiver,
    const char *path)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_ABORT);
    zsync_msg_set_receiver (self, receiver);
    zsync_msg_set_path (self, path);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zsync_msg message

zsync_msg_t *
zsync_msg_dup (zsync_msg_t *self)
{
    if (!self)
        return NULL;
        
    zsync_msg_t *copy = zsync_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case ZSYNC_MSG_HELLO:
            copy->state = self->state;
            break;

        case ZSYNC_MSG_UPDATE:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->update_msg = self->update_msg? zmsg_dup (self->update_msg): NULL;
            break;

        case ZSYNC_MSG_FILES:
            copy->receiver = self->receiver? strdup (self->receiver): NULL;
            copy->files = self->files? zlist_dup (self->files): NULL;
            copy->size = self->size;
            break;

        case ZSYNC_MSG_CREDIT:
            copy->amount = self->amount;
            break;

        case ZSYNC_MSG_CHUNK:
            copy->chunk = self->chunk? zchunk_dup (self->chunk): NULL;
            copy->path = self->path? strdup (self->path): NULL;
            copy->sequence = self->sequence;
            copy->offset = self->offset;
            break;

        case ZSYNC_MSG_ABORT:
            copy->receiver = self->receiver? strdup (self->receiver): NULL;
            copy->path = self->path? strdup (self->path): NULL;
            break;

    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zsync_msg_print (zsync_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_MSG_HELLO:
            zsys_debug ("ZSYNC_MSG_HELLO:");
            zsys_debug ("    state=%ld", (long) self->state);
            break;
            
        case ZSYNC_MSG_UPDATE:
            zsys_debug ("ZSYNC_MSG_UPDATE:");
            if (self->sender)
                zsys_debug ("    sender='%s'", self->sender);
            else
                zsys_debug ("    sender=");
            zsys_debug ("    update_msg=");
            if (self->update_msg)
                zmsg_print (self->update_msg);
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZSYNC_MSG_FILES:
            zsys_debug ("ZSYNC_MSG_FILES:");
            if (self->receiver)
                zsys_debug ("    receiver='%s'", self->receiver);
            else
                zsys_debug ("    receiver=");
            zsys_debug ("    files=");
            if (self->files) {
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    zsys_debug ("        '%s'", files);
                    files = (char *) zlist_next (self->files);
                }
            }
            zsys_debug ("    size=%ld", (long) self->size);
            break;
            
        case ZSYNC_MSG_CREDIT:
            zsys_debug ("ZSYNC_MSG_CREDIT:");
            zsys_debug ("    amount=%ld", (long) self->amount);
            break;
            
        case ZSYNC_MSG_CHUNK:
            zsys_debug ("ZSYNC_MSG_CHUNK:");
            zsys_debug ("    chunk=[ ... ]");
            if (self->path)
                zsys_debug ("    path='%s'", self->path);
            else
                zsys_debug ("    path=");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            zsys_debug ("    offset=%ld", (long) self->offset);
            break;
            
        case ZSYNC_MSG_ABORT:
            zsys_debug ("ZSYNC_MSG_ABORT:");
            if (self->receiver)
                zsys_debug ("    receiver='%s'", self->receiver);
            else
                zsys_debug ("    receiver=");
            if (self->path)
                zsys_debug ("    path='%s'", self->path);
            else
                zsys_debug ("    path=");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zsync_msg_routing_id (zsync_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zsync_msg_set_routing_id (zsync_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zsync_msg id

int
zsync_msg_id (zsync_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zsync_msg_set_id (zsync_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
zsync_msg_command (zsync_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_MSG_HELLO:
            return ("HELLO");
            break;
        case ZSYNC_MSG_UPDATE:
            return ("UPDATE");
            break;
        case ZSYNC_MSG_FILES:
            return ("FILES");
            break;
        case ZSYNC_MSG_CREDIT:
            return ("CREDIT");
            break;
        case ZSYNC_MSG_CHUNK:
            return ("CHUNK");
            break;
        case ZSYNC_MSG_ABORT:
            return ("ABORT");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the state field

uint64_t
zsync_msg_state (zsync_msg_t *self)
{
    assert (self);
    return self->state;
}

void
zsync_msg_set_state (zsync_msg_t *self, uint64_t state)
{
    assert (self);
    self->state = state;
}


//  --------------------------------------------------------------------------
//  Get/set the sender field

const char *
zsync_msg_sender (zsync_msg_t *self)
{
    assert (self);
    return self->sender;
}

void
zsync_msg_set_sender (zsync_msg_t *self, const char *format, ...)
{
    //  Format sender from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->sender);
    self->sender = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the update_msg field without transferring ownership

zmsg_t *
zsync_msg_update_msg (zsync_msg_t *self)
{
    assert (self);
    return self->update_msg;
}

//  Get the update_msg field and transfer ownership to caller

zmsg_t *
zsync_msg_get_update_msg (zsync_msg_t *self)
{
    zmsg_t *update_msg = self->update_msg;
    self->update_msg = NULL;
    return update_msg;
}

//  Set the update_msg field, transferring ownership from caller

void
zsync_msg_set_update_msg (zsync_msg_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->update_msg);
    self->update_msg = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the receiver field

const char *
zsync_msg_receiver (zsync_msg_t *self)
{
    assert (self);
    return self->receiver;
}

void
zsync_msg_set_receiver (zsync_msg_t *self, const char *format, ...)
{
    //  Format receiver from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->receiver);
    self->receiver = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the files field, without transferring ownership

zlist_t *
zsync_msg_files (zsync_msg_t *self)
{
    assert (self);
    return self->files;
}

//  Get the files field and transfer ownership to caller

zlist_t *
zsync_msg_get_files (zsync_msg_t *self)
{
    assert (self);
    zlist_t *files = self->files;
    self->files = NULL;
    return files;
}

//  Set the files field, transferring ownership from caller

void
zsync_msg_set_files (zsync_msg_t *self, zlist_t **files_p)
{
    assert (self);
    assert (files_p);
    zlist_destroy (&self->files);
    self->files = *files_p;
    *files_p = NULL;
}

//  --------------------------------------------------------------------------
//  Iterate through the files field, and append a files value

const char *
zsync_msg_files_first (zsync_msg_t *self)
{
    assert (self);
    if (self->files)
        return (char *) (zlist_first (self->files));
    else
        return NULL;
}

const char *
zsync_msg_files_next (zsync_msg_t *self)
{
    assert (self);
    if (self->files)
        return (char *) (zlist_next (self->files));
    else
        return NULL;
}

void
zsync_msg_files_append (zsync_msg_t *self, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Attach string to list
    if (!self->files) {
        self->files = zlist_new ();
        zlist_autofree (self->files);
    }
    zlist_append (self->files, string);
    free (string);
}

size_t
zsync_msg_files_size (zsync_msg_t *self)
{
    return zlist_size (self->files);
}


//  --------------------------------------------------------------------------
//  Get/set the size field

uint64_t
zsync_msg_size (zsync_msg_t *self)
{
    assert (self);
    return self->size;
}

void
zsync_msg_set_size (zsync_msg_t *self, uint64_t size)
{
    assert (self);
    self->size = size;
}


//  --------------------------------------------------------------------------
//  Get/set the amount field

uint64_t
zsync_msg_amount (zsync_msg_t *self)
{
    assert (self);
    return self->amount;
}

void
zsync_msg_set_amount (zsync_msg_t *self, uint64_t amount)
{
    assert (self);
    self->amount = amount;
}


//  --------------------------------------------------------------------------
//  Get the chunk field without transferring ownership

zchunk_t *
zsync_msg_chunk (zsync_msg_t *self)
{
    assert (self);
    return self->chunk;
}

//  Get the chunk field and transfer ownership to caller

zchunk_t *
zsync_msg_get_chunk (zsync_msg_t *self)
{
    zchunk_t *chunk = self->chunk;
    self->chunk = NULL;
    return chunk;
}

//  Set the chunk field, transferring ownership from caller

void
zsync_msg_set_chunk (zsync_msg_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->chunk);
    self->chunk = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the path field

const char *
zsync_msg_path (zsync_msg_t *self)
{
    assert (self);
    return self->path;
}

void
zsync_msg_set_path (zsync_msg_t *self, const char *format, ...)
{
    //  Format path from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->path);
    self->path = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the sequence field

uint64_t
zsync_msg_sequence (zsync_msg_t *self)
{
    assert (self);
    return self->sequence;
}

void
zsync_msg_set_sequence (zsync_msg_t *self, uint64_t sequence)
{
    assert (self);
    self->sequence = sequence;
}


//  --------------------------------------------------------------------------
//  Get/set the offset field

uint64_t
zsync_msg_offset (zsync_msg_t *self)
{
    assert (self);
    return self->offset;
}

void
zsync_msg_set_offset (zsync_msg_t *self, uint64_t offset)
{
    assert (self);
    self->offset = offset;
}



//  --------------------------------------------------------------------------
//  Selftest

int
zsync_msg_test (bool verbose)
{
    printf (" * zsync_msg: ");

    //  Silence an "unused" warning by "using" the verbose variable
    if (verbose) {;}

    //  @selftest
    //  Simple create/destroy test
    zsync_msg_t *self = zsync_msg_new (0);
    assert (self);
    zsync_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-zsync_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-zsync_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    zsync_msg_t *copy;
    self = zsync_msg_new (ZSYNC_MSG_HELLO);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_state (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zsync_msg_recv (input);
        assert (self);
        assert (zsync_msg_routing_id (self));
        
        assert (zsync_msg_state (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_UPDATE);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_sender (self, "Life is short but Now lasts for ever");
    zmsg_t *update_update_msg = zmsg_new ();
    zsync_msg_set_update_msg (self, &update_update_msg);
    zmsg_addstr (zsync_msg_update_msg (self), "Hello, World");
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zsync_msg_recv (input);
        assert (self);
        assert (zsync_msg_routing_id (self));
        
        assert (streq (zsync_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (zsync_msg_update_msg (self)) == 1);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_FILES);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zsync_msg_files_append (self, "Name: %s", "Brutus");
    zsync_msg_files_append (self, "Age: %d", 43);
    zsync_msg_set_size (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zsync_msg_recv (input);
        assert (self);
        assert (zsync_msg_routing_id (self));
        
        assert (streq (zsync_msg_receiver (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_files_size (self) == 2);
        assert (streq (zsync_msg_files_first (self), "Name: Brutus"));
        assert (streq (zsync_msg_files_next (self), "Age: 43"));
        assert (zsync_msg_size (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_CREDIT);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_amount (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zsync_msg_recv (input);
        assert (self);
        assert (zsync_msg_routing_id (self));
        
        assert (zsync_msg_amount (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_CHUNK);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zchunk_t *chunk_chunk = zchunk_new ("Captcha Diem", 12);
    zsync_msg_set_chunk (self, &chunk_chunk);
    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    zsync_msg_set_sequence (self, 123);
    zsync_msg_set_offset (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zsync_msg_recv (input);
        assert (self);
        assert (zsync_msg_routing_id (self));
        
        assert (memcmp (zchunk_data (zsync_msg_chunk (self)), "Captcha Diem", 12) == 0);
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_sequence (self) == 123);
        assert (zsync_msg_offset (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_ABORT);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zsync_msg_recv (input);
        assert (self);
        assert (zsync_msg_routing_id (self));
        
        assert (streq (zsync_msg_receiver (self), "Life is short but Now lasts for ever"));
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        zsync_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
