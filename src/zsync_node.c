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
    zactor_t *watcher;
    zyre_t *zyre;              //  Zyre instance for P2P communication
    zpoller_t *poller;

    uint64_t state;
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

    self->name = (char *) args;
    self->state = 0;

    //  Initialize class properties
    self->pipe = pipe;
    self->inbox = zactor_new (zsync_inbox_actor, self->zyre);
    self->outbox = zactor_new (zsync_outbox_actor, self->zyre);
    self->watcher = zactor_new (zsync_watcher_actor, NULL);

    self->zyre = zyre_new (self->name);
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
        zactor_destroy (&self->watcher);

        zyre_destroy (&self->zyre);
        zpoller_destroy (&self->poller);
        zlist_destroy (&self->peers);

        free (self->name);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  Start node, return 0 if OK, 1 if not possible

static int
zsync_node_start (zsync_node_t *self)
{
    if (!self->name)
        return 1;       // Need name in order to start

    //  Load known peers
    zconfig_t *config = zconfig_load (zsys_sprintf ("%s.cfg", self->name));
    if (config) {
        //  Get last state of this node
        zconfig_t *node_conf = zconfig_child (config);
        sscanf (zconfig_resolve (node_conf, "state", "0"), "%"SCNu64, &self->state);
        zconfig_t *peers_conf = zconfig_next (node_conf);
        zconfig_t *peer_conf = zconfig_child (peers_conf);
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
    
    //  Setup and start watcher
    zstr_sendm (self->watcher, "START");
    zstr_send (self->watcher, self->name);
    rc = zsock_wait (self->watcher);
    assert (rc == 0);
    
    //  Setup and start zyre
    rc = zyre_start (self->zyre);
    assert (rc == 0);
    zyre_join (self->zyre, "ZSYNC");

    //  Start polling on sockets
    zpoller_add (self->poller, zyre_socket (self->zyre));
    zpoller_add (self->poller, self->inbox);
    zpoller_add (self->poller, self->outbox);
    zpoller_add (self->poller, self->watcher);
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

    //  Stop watcher
    zstr_sendm (self->watcher, "STOP");
    zstr_send (self->watcher, self->name);
    rc = zsock_wait (self->watcher);
    assert (rc == 0);

    //  Stop polling on sockets
    zpoller_remove (self->poller, zyre_socket (self->zyre));
    zpoller_remove (self->poller, self->inbox);
    zpoller_remove (self->poller, self->outbox);
    zpoller_remove (self->poller, self->watcher);

    zconfig_t *config = zconfig_new ("root", NULL);
    //  Save the peers current state
    char *node_state = zsys_sprintf ("%"PRIu64, self->state);
    zconfig_t *node_conf = zconfig_new ("node", config);
    zconfig_put (node_conf, "state", node_state);
    //  Save the current state of known peers
    zconfig_t *peer_conf = zconfig_new ("peers", config);
    zsync_peer_t *peer = (zsync_peer_t *) zlist_first (self->peers);
    while (peer) {
        char *peer_name = zsync_peer_name (peer);
        char *peer_state = zsys_sprintf ("%"PRIu64, zsync_peer_state (peer));
        zconfig_put (peer_conf, peer_name, peer_state);
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
      if (streq (name, zsync_peer_name (peer)))
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
// Here we process messages received from other peers

static void
zsync_node_process_peer_message (zsync_node_t *self, zsync_msg_t *msg, zsync_peer_t *peer)
{
    assert (self);
    assert (msg);
    assert (peer);

    switch (zsync_msg_id (msg)) {
       case ZSYNC_MSG_HELLO:
       {
          uint64_t state = zsync_msg_state (msg);
          zsys_debug ("[%s]-->[%s] HELLO state: %"PRIu64, zsync_peer_name (peer), self->name, state); 
          //  Peer is out of date
          if (state < self->state)
              NULL; // TODO: request indexer to compose an update
          break;
       }
       case ZSYNC_MSG_UPDATE:
          break;
       case ZSYNC_MSG_FILES:
          break;
       case ZSYNC_MSG_CREDIT:
          break;
       case ZSYNC_MSG_CHUNK:
          break;
       case ZSYNC_MSG_ABORT:
          break;
       default:
           zsys_error ("invalid msg command '%d'", zsync_msg_id (msg));
           assert (false);
    }

    zsync_msg_destroy (&msg);
}


// --------------------------------------------------------------------------
// Here we handle messages received by other peers.

static void
zsync_node_recv_peer (zsync_node_t *self)
{
    //  Message from a zyre peer  
    zmsg_t *zyre_msg = zyre_recv (self->zyre);
    //  Extract the metadata from the zyre message
    char *command = zmsg_popstr (zyre_msg);
    char *zyre_id = zmsg_popstr (zyre_msg);
    char *peer_name = zmsg_popstr (zyre_msg);
    
    //  Try to get an already connected peer
    zsync_peer_t *peer = zsync_find_peer_by_zyre_id (self, zyre_id);
    if (streq (command, "ENTER")) {
       //  Loopup if this peer is known from a previous session
       peer = zsync_find_peer_by_name (self, peer_name);
       if (peer) {
           //  Update the temporary zyre id of known peer.
           zsync_peer_set_zyre_id (peer, zyre_id);
       }
       else  {
           //  Add new peer to the list.
           peer = zsync_peer_new (peer_name, zyre_id, 0);
           zlist_append (self->peers, peer);
       }
       //  [GREET] the other peer with it's known state
       zmsg_t *hello_msg = zsync_msg_encode_hello (zsync_peer_state (peer));
       zyre_whisper (self->zyre, zsync_peer_zyre_id (peer), &hello_msg);

       zsys_debug ("[%s]-->[%s] ENTER", zsync_peer_name (peer), self->name);
    }
    else
    if (streq (command, "EXIT"))       
       zsys_debug ("[%s]-->[%s] EXIT", zsync_peer_name (peer), self->name);
    else
    if (streq (command, "JOIN")) {    
       zsys_debug ("[%s]-->[%s] JOIN", zsync_peer_name (peer), self->name);
    }
    else
    if (streq (command, "LEAVE"))
       zsys_debug ("[%s]-->[%s] LEAVE", zsync_peer_name (peer), self->name);
    else
    if (streq (command, "WHISPER")) {
       zsys_debug ("[%s]-->[%s] WHISPER", zsync_peer_name (peer), self->name);
       zsync_node_process_peer_message (self, zsync_msg_decode (&zyre_msg), peer);
    }
    else
    if (streq (command, "SHOUT")) {    
       zsys_debug ("[%s]-->[%s] SHOUT", zsync_peer_name (peer), self->name);
       zsync_node_process_peer_message (self, zsync_msg_decode (&zyre_msg), peer);
    }

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
        if (which == zactor_sock (self->watcher))
            zsync_node_recv_api (self, zactor_sock (self->watcher));
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
