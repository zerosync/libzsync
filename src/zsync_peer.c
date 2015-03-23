/*  =========================================================================
    zsync_peer - Internal peer representation

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
    zsync_peer - Internal peer representation
@discuss
@end
*/

#include "../include/libzsync.h"
#include "libzsync_classes.h"

//  Structure of our class

struct _zsync_peer_t {
    char *uuid;
    uint64_t state;
};


//  --------------------------------------------------------------------------
//  Create a new zsync_peer.

zsync_peer_t *
zsync_peer_new (char *uuid, uint64_t state)
{
    zsync_peer_t *self = (zsync_peer_t *) zmalloc (sizeof (zsync_peer_t));
    assert (self);

    self->uuid = (char *) zmalloc (sizeof (char) * 32 + 1);
    strcpy (self->uuid, uuid);
    self->state = state;

    return self;
}

//  --------------------------------------------------------------------------
//  Destroy the zsync_peer.

void
zsync_peer_destroy (zsync_peer_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_peer_t *self = *self_p;

        //  Free class properties
        free (self->uuid);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Get the UUID of this peer

char *
zsync_peer_uuid (zsync_peer_t *self)
{
   assert (self);
   return self->uuid;
}


//  --------------------------------------------------------------------------
//  Get the state of this peer

uint64_t
zsync_peer_state (zsync_peer_t *self)
{
   assert (self);
   return self->state;
}

//  --------------------------------------------------------------------------
//  Print properties of the zsync_peer object.

void
zsync_peer_print (zsync_peer_t *self)
{
    assert (self);
}


//  --------------------------------------------------------------------------
//  Self test of this class.

void
zsync_peer_test (bool verbose)
{
    printf (" * zsync_peer: ");

    //  @selftest
    //  Simple create/destroy test
    zsync_peer_t *self = zsync_peer_new ("123456", 0);
    assert (self);
    zsync_peer_destroy (&self);
    //  @end

    printf ("OK\n");
}
