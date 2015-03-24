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

/*
@header
    zsync - Public API
@discuss
@end
*/

#include "../include/libzsync.h"
#include "libzsync_classes.h"

//  Structure of our class

struct _zsync_t {
   zactor_t *actor;           // A zsync instance wraps the actor instance
};


//  --------------------------------------------------------------------------
//  Create a new zsync

zsync_t *
zsync_new ()
{
    zsync_t *self = (zsync_t *) zmalloc (sizeof (zsync_t));
    assert (self);

    //  Start node engine and wait for it to be ready
    self->actor = zactor_new (zsync_node_actor, NULL);
    assert (self->actor);

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync

void
zsync_destroy (zsync_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_t *self = *self_p;

        //  Free class properties
        zactor_destroy (&self->actor);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Sets the name of this node. This is required before starting the node.

void
zsync_set_name (zsync_t *self, const char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "SET NAME", string, NULL);
    zstr_free (&string);
    zsock_wait (self->actor) == 0? 0: -1;
}

//  --------------------------------------------------------------------------
//  Start the zsync node. Wait until node is started!

int
zsync_start (zsync_t *self)
{
    assert (self);
    zstr_sendx (self->actor, "START", NULL);
    return zsock_wait (self->actor) == 0? 0: -1;;
}


//  --------------------------------------------------------------------------
//  Stop the zsync node. Wait until node is stopped!

void
zsync_stop (zsync_t *self)
{
   assert (self);
   zstr_sendx (self->actor, "STOP", NULL);
   zsock_wait (self->actor);
}

//  --------------------------------------------------------------------------
//  Print properties of object

void
zsync_print (zsync_t *self)
{
    assert (self);
}


//  --------------------------------------------------------------------------
//  Selftest

int
zsync_test (bool verbose)
{
    printf (" * zsync: \n");

    //  @selftest
    //  Simple create/destroy test
    zsync_t *self1 = zsync_new ();
    zsync_t *self2 = zsync_new ();
    zsync_t *self3 = zsync_new ();
    assert (self1);
    assert (self2);
    assert (self3);

    zsync_set_name (self1, "%s", "node1");
    zsync_set_name (self2, "%s", "node2");
    zsync_set_name (self3, "%s", "node3");
    
    zsync_start (self1);
    zsync_start (self2);
    zsync_start (self3);
    zclock_sleep (2000);
    
    zsync_stop (self1);
    zclock_sleep (200);
    zsync_stop (self2);
    zclock_sleep (200);
    zsync_stop (self3);
    
    zsync_destroy (&self1);
    zsync_destroy (&self2);
    zsync_destroy (&self3);

    //  @end

    printf ("OK\n");
    return 0;
}
