/*  =========================================================================
    zsync_msg - the API which is used to comminicate with user interface clients

    Codec class for zsync_msg.

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
    /*                */
    uint64_t state;
    /* UUID that identifies the sender  */
    char sender [256];
    /* List of updated files and their metadata  */
    zmsg_t *update_msg;
    /* UUID that identifies the receiver  */
    char receiver [256];
    /* List of file names  */
    zlist_t *files;
    /* Total size of all files in bytes  */
    uint64_t size;
    /* Path of file that the 'chunk' belongs to   */
    char path [256];
    /* Size of the requested chunk in bytes  */
    uint64_t chunk_size;
    /* File offset for for the chunk in bytes  */
    uint64_t offset;
    /* Requested chunk  */
    zchunk_t *chunk;
    /* Defines which chunk of the file at 'path' this is!  */
    uint64_t sequence;
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
    if (self->needle + size > self->ceiling) { \
        zsys_warning ("zsync_msg: GET_OCTETS failed"); \
        goto malformed; \
    } \
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
    if (self->needle + 1 > self->ceiling) { \
        zsys_warning ("zsync_msg: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("zsync_msg: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("zsync_msg: GET_NUMBER4 failed"); \
        goto malformed; \
    } \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) { \
        zsys_warning ("zsync_msg: GET_NUMBER8 failed"); \
        goto malformed; \
    } \
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
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("zsync_msg: GET_STRING failed"); \
        goto malformed; \
    } \
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
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("zsync_msg: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new zsync_msg

zsync_msg_t *
zsync_msg_new (void)
{
    zsync_msg_t *self = (zsync_msg_t *) zmalloc (sizeof (zsync_msg_t));
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
        zmsg_destroy (&self->update_msg);
        if (self->files)
            zlist_destroy (&self->files);
        zchunk_destroy (&self->chunk);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive a zsync_msg from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.

int
zsync_msg_recv (zsync_msg_t *self, zsock_t *input)
{
    assert (input);
    
    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("zsync_msg: no routing ID");
            return -1;          //  Interrupted or malformed
        }
    }
    zmq_msg_t frame;
    zmq_msg_init (&frame);
    int size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("zsync_msg: interrupted");
        goto malformed;         //  Interrupted
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);
    
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0)) {
        zsys_warning ("zsync_msg: invalid signature");
        //  TODO: discard invalid messages and loop, and return
        //  -1 only on interrupt
        goto malformed;         //  Interrupted
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            break;

        case ZSYNC_MSG_RES_STATE:
            GET_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_REQ_UPDATE:
            GET_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_UPDATE:
            GET_STRING (self->sender);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->update_msg);
            if (zsock_rcvmore (input))
                self->update_msg = zmsg_recv (input);
            else
                self->update_msg = zmsg_new ();
            break;

        case ZSYNC_MSG_REQ_FILES:
            GET_STRING (self->receiver);
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                self->files = zlist_new ();
                zlist_autofree (self->files);
                while (list_size--) {
                    char *string = NULL;
                    GET_LONGSTR (string);
                    zlist_append (self->files, string);
                    free (string);
                }
            }
            GET_NUMBER8 (self->size);
            break;

        case ZSYNC_MSG_REQ_CHUNK:
            GET_STRING (self->path);
            GET_NUMBER8 (self->chunk_size);
            GET_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_RES_CHUNK:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("zsync_msg: chunk is missing data");
                    goto malformed;
                }
                zchunk_destroy (&self->chunk);
                self->chunk = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case ZSYNC_MSG_CHUNK:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("zsync_msg: chunk is missing data");
                    goto malformed;
                }
                zchunk_destroy (&self->chunk);
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

        case ZSYNC_MSG_TERMINATE:
            break;

        default:
            zsys_warning ("zsync_msg: bad message ID");
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return 0;

    //  Error returns
    malformed:
        zsys_warning ("zsync_msg: zsync_msg malformed message, fail");
        zmq_msg_close (&frame);
        return -1;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the zsync_msg to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
zsync_msg_send (zsync_msg_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZSYNC_MSG_RES_STATE:
            frame_size += 8;            //  state
            break;
        case ZSYNC_MSG_REQ_UPDATE:
            frame_size += 8;            //  state
            break;
        case ZSYNC_MSG_UPDATE:
            frame_size += 1 + strlen (self->sender);
            break;
        case ZSYNC_MSG_REQ_FILES:
            frame_size += 1 + strlen (self->receiver);
            frame_size += 4;            //  Size is 4 octets
            if (self->files) {
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    frame_size += 4 + strlen (files);
                    files = (char *) zlist_next (self->files);
                }
            }
            frame_size += 8;            //  size
            break;
        case ZSYNC_MSG_REQ_CHUNK:
            frame_size += 1 + strlen (self->path);
            frame_size += 8;            //  chunk_size
            frame_size += 8;            //  offset
            break;
        case ZSYNC_MSG_RES_CHUNK:
            frame_size += 4;            //  Size is 4 octets
            if (self->chunk)
                frame_size += zchunk_size (self->chunk);
            break;
        case ZSYNC_MSG_CHUNK:
            frame_size += 4;            //  Size is 4 octets
            if (self->chunk)
                frame_size += zchunk_size (self->chunk);
            frame_size += 1 + strlen (self->path);
            frame_size += 8;            //  sequence
            frame_size += 8;            //  offset
            break;
        case ZSYNC_MSG_ABORT:
            frame_size += 1 + strlen (self->receiver);
            frame_size += 1 + strlen (self->path);
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);
    bool send_update_msg = false;
    size_t nbr_frames = 1;              //  Total number of frames to send
    
    switch (self->id) {
        case ZSYNC_MSG_RES_STATE:
            PUT_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_REQ_UPDATE:
            PUT_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_UPDATE:
            PUT_STRING (self->sender);
            nbr_frames += self->update_msg? zmsg_size (self->update_msg): 1;
            send_update_msg = true;
            break;

        case ZSYNC_MSG_REQ_FILES:
            PUT_STRING (self->receiver);
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

        case ZSYNC_MSG_REQ_CHUNK:
            PUT_STRING (self->path);
            PUT_NUMBER8 (self->chunk_size);
            PUT_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_RES_CHUNK:
            if (self->chunk) {
                PUT_NUMBER4 (zchunk_size (self->chunk));
                memcpy (self->needle,
                        zchunk_data (self->chunk),
                        zchunk_size (self->chunk));
                self->needle += zchunk_size (self->chunk);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
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
            PUT_STRING (self->path);
            PUT_NUMBER8 (self->sequence);
            PUT_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_ABORT:
            PUT_STRING (self->receiver);
            PUT_STRING (self->path);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);
    
    //  Now send the update_msg if necessary
    if (send_update_msg) {
        if (self->update_msg) {
            zframe_t *frame = zmsg_first (self->update_msg);
            while (frame) {
                zframe_send (&frame, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
                frame = zmsg_next (self->update_msg);
            }
        }
        else
            zmq_send (zsock_resolve (output), NULL, 0, 0);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zsync_msg_print (zsync_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            zsys_debug ("ZSYNC_MSG_REQ_STATE:");
            break;
            
        case ZSYNC_MSG_RES_STATE:
            zsys_debug ("ZSYNC_MSG_RES_STATE:");
            zsys_debug ("    state=%ld", (long) self->state);
            break;
            
        case ZSYNC_MSG_REQ_UPDATE:
            zsys_debug ("ZSYNC_MSG_REQ_UPDATE:");
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
            
        case ZSYNC_MSG_REQ_FILES:
            zsys_debug ("ZSYNC_MSG_REQ_FILES:");
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
            
        case ZSYNC_MSG_REQ_CHUNK:
            zsys_debug ("ZSYNC_MSG_REQ_CHUNK:");
            if (self->path)
                zsys_debug ("    path='%s'", self->path);
            else
                zsys_debug ("    path=");
            zsys_debug ("    chunk_size=%ld", (long) self->chunk_size);
            zsys_debug ("    offset=%ld", (long) self->offset);
            break;
            
        case ZSYNC_MSG_RES_CHUNK:
            zsys_debug ("ZSYNC_MSG_RES_CHUNK:");
            zsys_debug ("    chunk=[ ... ]");
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
            
        case ZSYNC_MSG_TERMINATE:
            zsys_debug ("ZSYNC_MSG_TERMINATE:");
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
        case ZSYNC_MSG_REQ_STATE:
            return ("REQ_STATE");
            break;
        case ZSYNC_MSG_RES_STATE:
            return ("RES_STATE");
            break;
        case ZSYNC_MSG_REQ_UPDATE:
            return ("REQ_UPDATE");
            break;
        case ZSYNC_MSG_UPDATE:
            return ("UPDATE");
            break;
        case ZSYNC_MSG_REQ_FILES:
            return ("REQ_FILES");
            break;
        case ZSYNC_MSG_REQ_CHUNK:
            return ("REQ_CHUNK");
            break;
        case ZSYNC_MSG_RES_CHUNK:
            return ("RES_CHUNK");
            break;
        case ZSYNC_MSG_CHUNK:
            return ("CHUNK");
            break;
        case ZSYNC_MSG_ABORT:
            return ("ABORT");
            break;
        case ZSYNC_MSG_TERMINATE:
            return ("TERMINATE");
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
zsync_msg_set_sender (zsync_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->sender)
        return;
    strncpy (self->sender, value, 255);
    self->sender [255] = 0;
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
zsync_msg_set_receiver (zsync_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->receiver)
        return;
    strncpy (self->receiver, value, 255);
    self->receiver [255] = 0;
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
//  Get/set the path field

const char *
zsync_msg_path (zsync_msg_t *self)
{
    assert (self);
    return self->path;
}

void
zsync_msg_set_path (zsync_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->path)
        return;
    strncpy (self->path, value, 255);
    self->path [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the chunk_size field

uint64_t
zsync_msg_chunk_size (zsync_msg_t *self)
{
    assert (self);
    return self->chunk_size;
}

void
zsync_msg_set_chunk_size (zsync_msg_t *self, uint64_t chunk_size)
{
    assert (self);
    self->chunk_size = chunk_size;
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
//  Selftest

int
zsync_msg_test (bool verbose)
{
    printf (" * zsync_msg: ");

    //  Silence an "unused" warning by "using" the verbose variable
    if (verbose) {;}

    //  @selftest
    //  Simple create/destroy test
    zsync_msg_t *self = zsync_msg_new ();
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
    self = zsync_msg_new ();
    zsync_msg_set_id (self, ZSYNC_MSG_REQ_STATE);

    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
    }
    zsync_msg_set_id (self, ZSYNC_MSG_RES_STATE);

    zsync_msg_set_state (self, 123);
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (zsync_msg_state (self) == 123);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_REQ_UPDATE);

    zsync_msg_set_state (self, 123);
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (zsync_msg_state (self) == 123);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_UPDATE);

    zsync_msg_set_sender (self, "Life is short but Now lasts for ever");
    zmsg_t *update_update_msg = zmsg_new ();
    zsync_msg_set_update_msg (self, &update_update_msg);
    zmsg_addstr (zsync_msg_update_msg (self), "Hello, World");
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (streq (zsync_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (zsync_msg_update_msg (self)) == 1);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_REQ_FILES);

    zsync_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zlist_t *req_files_files = zlist_new ();
    zlist_append (req_files_files, "Name: Brutus");
    zlist_append (req_files_files, "Age: 43");
    zsync_msg_set_files (self, &req_files_files);
    zsync_msg_set_size (self, 123);
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (streq (zsync_msg_receiver (self), "Life is short but Now lasts for ever"));
        zlist_t *files = zsync_msg_get_files (self);
        assert (zlist_size (files) == 2);
        assert (streq ((char *) zlist_first (files), "Name: Brutus"));
        assert (streq ((char *) zlist_next (files), "Age: 43"));
        zlist_destroy (&files);
        assert (zsync_msg_size (self) == 123);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_REQ_CHUNK);

    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    zsync_msg_set_chunk_size (self, 123);
    zsync_msg_set_offset (self, 123);
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_chunk_size (self) == 123);
        assert (zsync_msg_offset (self) == 123);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_RES_CHUNK);

    zchunk_t *res_chunk_chunk = zchunk_new ("Captcha Diem", 12);
    zsync_msg_set_chunk (self, &res_chunk_chunk);
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (memcmp (zchunk_data (zsync_msg_chunk (self)), "Captcha Diem", 12) == 0);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_CHUNK);

    zchunk_t *chunk_chunk = zchunk_new ("Captcha Diem", 12);
    zsync_msg_set_chunk (self, &chunk_chunk);
    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    zsync_msg_set_sequence (self, 123);
    zsync_msg_set_offset (self, 123);
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (memcmp (zchunk_data (zsync_msg_chunk (self)), "Captcha Diem", 12) == 0);
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_sequence (self) == 123);
        assert (zsync_msg_offset (self) == 123);
    }
    zsync_msg_set_id (self, ZSYNC_MSG_ABORT);

    zsync_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
        assert (streq (zsync_msg_receiver (self), "Life is short but Now lasts for ever"));
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
    }
    zsync_msg_set_id (self, ZSYNC_MSG_TERMINATE);

    //  Send twice
    zsync_msg_send (self, output);
    zsync_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zsync_msg_recv (self, input);
        assert (zsync_msg_routing_id (self));
    }

    zsync_msg_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
