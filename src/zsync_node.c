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

/*
@header
    zsync_node - Internal communication HUB
@discuss
@end
*/

#include "../include/libzsync.h"
#include "libzsync_classes.h"

//  Structure of our class

struct _zsync_node_t {
    zsock_t *pipe;             //  Pipe back to application
    zyre_t *zyre;              //  Zyre instance for P2P communication
    zpoller_t *poller;

    bool terminated;
    bool verbose;

    zhash_t *peers;
};


//  --------------------------------------------------------------------------
//  Create a new zsync_node

static zsync_node_t *
zsync_node_new (zsock_t *pipe, void *args)
{
    zsync_node_t *self = (zsync_node_t *) zmalloc (sizeof (zsync_node_t));
    assert (self);

    self->zyre = zyre_new ("test");
    self->pipe = pipe;
    self->poller = zpoller_new (self->pipe, NULL);
    self->peers = zhash_new ();

    self->terminated = false;

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync_node

static void
zsync_node_destroy (zsync_node_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_node_t *self = *self_p;

        //  Free class properties
        zyre_destroy (&self->zyre);
        zhash_destroy (&self->peers);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


// Start node, return 0 if OK, 1 if not possible

static int
zyre_node_start (zsync_node_t *self)
{

    zyre_set_header (self->zyre, "NAME", "%s", "myName");

    int rc = zyre_start (self->zyre);
    assert (rc == 0);
    zyre_join (self->zyre, "ZSYNC");
    
    zpoller_add (self->poller, zyre_socket (self->zyre));
    return 0;
}


// Stop node, retrun 0 if OK, 1 if not possible

static int
zyre_node_stop (zsync_node_t *self)
{
    zyre_leave (self->zyre, "ZSYNC");
    zyre_stop (self->zyre);
   
    //  Stop polling on zyre socket
    zpoller_remove (self->poller, zyre_socket (self->zyre));
    return 0;
}


// Here we handle the different control messages from the front-end

static void
zsync_node_recv_api (zsync_node_t *self)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "START"))
        zsock_signal (self->pipe, zyre_node_start (self));
    else
    if (streq (command, "STOP"))
        zsock_signal (self->pipe, zyre_node_stop (self));
    else
    if (streq (command, "PEERS"))
        zsock_send (self->pipe, "p", zhash_keys (self->peers));
    else
    if (streq (command, "$TERM"))
       self->terminated = true;
    else {
       zsys_error ("invalid command '%s'", command);
       assert (false);
    }
}


// --------------------------------------------------------------------------

static void
zsync_node_recv_peer (zsync_node_t *self)
{
    //  Message from a zyre peer  
    zyre_event_t *msg = zyre_event_new (self->zyre);

    const char *zyre_sender = zyre_event_sender (msg);
    (void) zyre_sender;

    if (zyre_event_type (msg) == ZYRE_EVENT_ENTER)
       printf ("ENTER %s\n", zyre_sender);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_EXIT)
       printf ("EXIT %s\n", zyre_sender);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_JOIN)
       printf ("JOIN %s\n", zyre_sender);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_LEAVE)
       printf ("LEAVE %s\n", zyre_sender);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_WHISPER)
       printf ("WHISPER %s\n", zyre_sender);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_SHOUT)
       printf ("SHOUT %s\n", zyre_sender);
}


// --------------------------------------------------------------------------
// This is the actor that runs a single node; it uses one thread, creates
void
zsync_node_actor (zsock_t *pipe, void *args)
{
    zsync_node_t *self = zsync_node_new (pipe, args);
    if (!self)        //  Interrupted
        return;

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 0);
        if (which == self->pipe)
            zsync_node_recv_api (self);
        else
        if (which == zyre_socket (self->zyre))
           zsync_node_recv_peer (self);
    }
    zsync_node_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Selftest

int
zsync_node_test (bool verbose)
{
    printf (" * zsync_node: ");

    //  @selftest
    //  Simple create/destroy test
    //  @end

    printf ("OK\n");
    return 0;
}
