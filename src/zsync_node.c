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
    zactor_t *inbox;           //  Actor for incomming traffic
    zactor_t *outbox;          //  Actor for outgoing traffic
    zyre_t *zyre;              //  Zyre instance for P2P communication
    zpoller_t *poller;

    char *name;
    bool terminated;
    bool verbose;

    zlist_t *peers;
};


//  --------------------------------------------------------------------------
//  Create a new zsync_node

static zsync_node_t *
zsync_node_new (zsock_t *pipe, void *args)
{
    zsync_node_t *self = (zsync_node_t *) zmalloc (sizeof (zsync_node_t));
    assert (self);

    //  Initialize class properties
    self->pipe = pipe;
    self->inbox = zactor_new (zsync_inbox_actor, self->zyre);
    self->outbox = zactor_new (zsync_outbox_actor, self->zyre);
    self->zyre = zyre_new ("test");

    self->poller = zpoller_new (self->pipe, NULL);
    self->peers = zlist_new ();

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
        zactor_destroy (&self->inbox);
        zactor_destroy (&self->outbox);
        zyre_destroy (&self->zyre);
        zlist_destroy (&self->peers);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  Start node, return 0 if OK, 1 if not possible

static int
zsync_node_start (zsync_node_t *self)
{
    //  Load known peers
    zconfig_t *config = zconfig_load (zsys_sprintf ("%s.cfg", self->name));
    if (config) {
        zconfig_t *peer_conf = zconfig_child (config);
        while (peer_conf) {
            char *peer_name = zconfig_name (peer_conf);
            uint64_t peer_state;
            sscanf (zconfig_value (peer_conf), "%"SCNu64, &peer_state);
            //  Add peer to list of known peers
            zlist_append (self->peers, zsync_peer_new (peer_name, "", peer_state));
            //  Next peer conf sibling
            peer_conf = zconfig_next (peer_conf);
        }
    }

    //  Setup and start inbox
    zstr_sendm (self->inbox, "START");
    zstr_send (self->inbox, self->name);
    int rc = zsock_wait (self->inbox);
    assert (rc == 0);

    //  Setup and start outbox
    zstr_sendm (self->outbox, "START");
    zstr_send (self->outbox, self->name);
    rc = zsock_wait (self->outbox);
    assert (rc == 0);
    
    //  Setup and start zyre
    zyre_set_header (self->zyre, "ZS-NAME", "%s", self->name);
    rc = zyre_start (self->zyre);
    assert (rc == 0);
    zyre_join (self->zyre, "ZSYNC");

    //  Start polling on sockets
    zpoller_add (self->poller, zyre_socket (self->zyre));
    zpoller_add (self->poller, self->inbox);
    zpoller_add (self->poller, self->outbox);
    return 0;
}


//  Stop node, retrun 0 if OK, 1 if not possible

static int
zsync_node_stop (zsync_node_t *self)
{   
    int rc = 0;
    //  Stop zyre
    zyre_leave (self->zyre, "ZSYNC");
    zyre_stop (self->zyre);
    
    //  Stop inbox
    zstr_sendm (self->inbox, "STOP");
    zstr_send (self->inbox, self->name);
    rc = zsock_wait (self->inbox);
    assert (rc == 0);
    
    //  Stop outbox
    zstr_sendm (self->outbox, "STOP");
    zstr_send (self->outbox, self->name);
    rc = zsock_wait (self->outbox);
    assert (rc == 0);

    //  Stop polling on sockets
    zpoller_remove (self->poller, zyre_socket (self->zyre));
    zpoller_remove (self->poller, self->inbox);
    zpoller_remove (self->poller, self->outbox);

    //  Save the current state of known peers
    zconfig_t *config = zconfig_new ("peers", NULL);
    zsync_peer_t *peer = (zsync_peer_t *) zlist_first (self->peers);
    while (peer) {
        char *peer_name = zsync_peer_name (peer);
        char *peer_state = zsys_sprintf ("%"PRIu64, zsync_peer_state (peer));
        zconfig_put (config, peer_name, peer_state);
        //  Next peer
        peer = (zsync_peer_t *) zlist_next (self->peers);
    }
    zconfig_save (config, zsys_sprintf ("%s.cfg", self->name));

    return 0;
}


//  Find peers by their name. Return the peer or NULL if none 
//  for the name exists.

static zsync_peer_t *
zsync_find_peer_by_name (zsync_node_t *self, char *name)
{
   assert (self);
   assert (name);

   zsync_peer_t *peer = (zsync_peer_t *) zlist_first (self->peers);
   while (peer) {
      if (streq (name, zsync_peer_zyre_id (peer)))
         return peer;
      //  Get next peer in list
      peer = (zsync_peer_t *) zlist_next (self->peers);
   }
   return NULL;
}


//  Find peers by their zyre_id. Return the peer or NULL if none 
//  for the zyre_id exists.

static zsync_peer_t *
zsync_find_peer_by_zyre_id (zsync_node_t *self, char *zyre_id)
{
   assert (self);
   assert (zyre_id);

   zsync_peer_t *peer = (zsync_peer_t *) zlist_first (self->peers);
   while (peer) {
      if (streq (zyre_id, zsync_peer_zyre_id (peer)))
         return peer;
      //  Get next peer in list
      peer = (zsync_peer_t *) zlist_next (self->peers);
   }
   return NULL;
}


//  Here we handle the different control messages from the front-end

static void
zsync_node_recv_api (zsync_node_t *self, zsock_t *socket)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (socket);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "NAME"))
       zstr_send ("%s", self->name);
    if (streq (command, "SET NAME")) {
       free (self->name);
       self->name = zmsg_popstr (request);
       assert (self->name);    
       zsock_signal (self->pipe, 0);
    }
    else
    if (streq (command, "START"))
        zsock_signal (self->pipe, zsync_node_start (self));
    else
    if (streq (command, "STOP"))
        zsock_signal (self->pipe, zsync_node_stop (self));
    else
    if (streq (command, "PEERS"))
        zsock_send (self->pipe, "p", self->peers);
    else
    if (streq (command, "$TERM"))
       self->terminated = true;
    else {
       zsys_error ("invalid command '%s'", command);
       assert (false);
    }
}


// --------------------------------------------------------------------------
// Here we handle messages received by other peers.

static void
zsync_node_recv_peer (zsync_node_t *self)
{
    //  Message from a zyre peer  
    zyre_event_t *msg = zyre_event_new (self->zyre);

    char *zyre_sender = (char *) zyre_event_sender (msg);
    zsync_peer_t *peer = zsync_find_peer_by_zyre_id (self, zyre_sender);

    if (zyre_event_type (msg) == ZYRE_EVENT_ENTER) {
       char *peer_name = (char *) zyre_event_header (msg, "ZS-NAME");
       if (!peer) {
          peer = zsync_find_peer_by_name (self, peer_name);
          if (peer)
              //  Update zyre id of existing peer.
              zsync_peer_set_zyre_id (peer, zyre_sender);
          else  {
              //  Add new peer to the list.
              peer = zsync_peer_new (peer_name, zyre_sender, 0);
              zlist_append (self->peers, peer);
          }
       }

       zsys_debug ("[%s]-->[%s] ENTER", zsync_peer_name (peer), self->name);
    }
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_EXIT)
       zsys_debug ("[%s]-->[%s] EXIT", zsync_peer_name (peer), self->name);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_JOIN) {
       zsys_debug ("[%s]-->[%s] JOIN", zsync_peer_name (peer), self->name);
       zyre_whispers (self->zyre, zsync_peer_zyre_id (peer), "Hello %s", zsync_peer_name (peer));
    }
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_LEAVE)
       zsys_debug ("[%s]-->[%s] LEAVE", zsync_peer_name (peer), self->name);
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_WHISPER) {
       zmsg_t *zsmsg = zyre_event_msg (msg);
       char *news = zmsg_popstr (zsmsg);
       zsys_debug ("[%s]-->[%s] WHISPER: %s", zsync_peer_name (peer), self->name, news);
       zstr_send (self->inbox, "FILES");
    }
    else
    if (zyre_event_type (msg) == ZYRE_EVENT_SHOUT) {
       zsys_debug ("[%s]-->[%s] SHOUT", zsync_peer_name (peer), self->name);
    }

    zyre_event_destroy (&msg);
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
            zsync_node_recv_api (self, self->pipe);
        else
        if (which == zactor_sock (self->inbox)) 
            zsync_node_recv_api (self, zactor_sock (self->inbox));
        else
        if (which == zactor_sock (self->outbox))
            zsync_node_recv_api (self, zactor_sock (self->outbox));
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
