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

/*
@header
    zsync_outbox - Actor which handles outgoing data.
@discuss
@end
*/

#include "../include/libzsync.h"
#include "libzsync_classes.h"

//  Structure of our class

struct _zsync_outbox_t {
    zsock_t *pipe;          //  Pipe back to node
    zsock_t *signal;        //  Send events
    zsock_t *slot;          //  Receive events
    zyre_t *zyre;           //  Zyre instance to directly send to

    zpoller_t *poller;
    bool terminated;
};


//  --------------------------------------------------------------------------
//  Create a new zsync_outbox.

static zsync_outbox_t *
zsync_outbox_new (zsock_t *pipe, void *args)
{
    zsync_outbox_t *self = (zsync_outbox_t *) zmalloc (sizeof (zsync_outbox_t));
    assert (self);

    self->pipe = pipe;

    self->signal = zsock_new (ZMQ_PUB);
    self->slot = zsock_new (ZMQ_SUB);
    assert (self->signal);
    assert (self->slot);

    self->zyre = (zyre_t *) args;

    self->poller = zpoller_new (self->pipe, NULL);

    self->terminated = false;

    return self;
}

//  --------------------------------------------------------------------------
//  Destroy the zsync_outbox.

static void
zsync_outbox_destroy (zsync_outbox_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_outbox_t *self = *self_p;

        // Free class properties
        zsock_destroy (&self->signal);
        zsock_destroy (&self->slot);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  Start this actor

static int
zsync_outbox_start (zsync_outbox_t *self, char *node_name)
{
    assert (self);
    int rc = 0;
    //  Setup signal and slot sockets
    rc = zsock_bind (self->signal, "inproc://outbox_%s_events", node_name);
    assert (rc == 0);
    rc = zsock_connect (self->slot, "inproc://inbox_%s_events", node_name);
    //  Subscribe to signal without filtering
    zsock_set_subscribe (self->slot, "");
    assert (rc == 0);

    zpoller_add (self->poller, self->slot);
    return 0;
}


//  Stop this actor

static int
zsync_outbox_stop (zsync_outbox_t *self, char *node_name)
{
    assert (self);
    int rc = 0;
    rc = zsock_unbind (self->signal, "inproc://outbox_%s_events", node_name);
    assert (rc == 0);
    rc = zsock_disconnect (self->slot, "inproc://inbox_%s_events", node_name);
    assert (rc == 0);

    zpoller_remove (self->poller, self->slot);
    return 0;
}


//  Here we handle incomming message from the node

static void
zsync_outbox_recv_node (zsync_outbox_t *self)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "START")) {
        char *node_name = zmsg_popstr (request);
        zsock_signal (self->pipe, zsync_outbox_start (self, node_name));
    }
    else
    if (streq (command, "STOP")) {
        char *node_name = zmsg_popstr (request);
        zsock_signal (self->pipe, zsync_outbox_stop (self, node_name));
    }
    else
    if (streq (command, "$TERM"))
       self->terminated = true;
    else {
       zsys_error ("invalid command '%s'", command);
       assert (false);
    }
}


//  Here we handle incomming signals

static void
zsync_outbox_recv_signal (zsync_outbox_t *self)
{
    //  Get the signal
    zmsg_t *signal = zmsg_recv (self->slot);
    if (!signal)
       return;        //  Interrupted

    zsys_info ("%s", zmsg_popstr (signal));
    //  TODO
}


//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zsync_outbox_actor (zsock_t *pipe, void *args)
{
    zsync_outbox_t * self = zsync_outbox_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
       zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 0);
       if (which == self->pipe)
          zsync_outbox_recv_node (self);
       else
       if (which == self->slot)
          zsync_outbox_recv_signal (self);
    }

    zsync_outbox_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this class.

void
zsync_outbox_test (bool verbose)
{
    printf (" * zsync_outbox: ");

    //  @selftest
    //  Simple create/destroy test
    //  @end

    printf ("OK\n");
}
