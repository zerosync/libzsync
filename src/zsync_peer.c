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
    char *name;             //  Name of ths node, restricted to 6 chars.
    char *zyre_id;
    uint64_t state;
};


//  --------------------------------------------------------------------------
//  Create a new zsync_peer.

zsync_peer_t *
zsync_peer_new (char *name, char *zyre_id, uint64_t state)
{
    zsync_peer_t *self = (zsync_peer_t *) zmalloc (sizeof (zsync_peer_t));
    assert (self);

    self->name = (char *) zmalloc (7);
    memcpy (self->name, name, 6);
    self->zyre_id = (char *) zmalloc (sizeof (char) * 32 + 1);
    strcpy (self->zyre_id, zyre_id);
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
        free (self->name);
        free (self->zyre_id);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Get the name of this peer

char *
zsync_peer_name (zsync_peer_t *self)
{
    assert (self);
    return self->name;
}


//  --------------------------------------------------------------------------
//  Get the zyre_id of this peer

char *
zsync_peer_zyre_id (zsync_peer_t *self)
{
    assert (self);
    return self->zyre_id;
}


//  --------------------------------------------------------------------------
//  Set the zyre_id of this peer

void
zsync_peer_set_zyre_id (zsync_peer_t *self, char *zyre_id)
{
    assert (self);
    assert (zyre_id);

    free (self->zyre_id);
    self->zyre_id = (char *) zmalloc (sizeof (char) * 32 + 1);
    strcpy (self->zyre_id, zyre_id);
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
    zsync_peer_t *self = zsync_peer_new ("123456", "AFLK45FDSLAF4543", 0);
    assert (self);
    zsync_peer_destroy (&self);
    //  @end

    printf ("OK\n");
}
