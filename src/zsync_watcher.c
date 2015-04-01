/*  =========================================================================
    zsync_watcher - Actor which watches the filesystem for changes

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
    zsync_watcher - Actor which watches the filesystem for changes
@discuss
@end
*/

#include "../include/libzsync.h"
#include "libzsync_classes.h"
#if defined (__UNIX__)
#include <poll.h>
#include <sys/inotify.h>
#endif

//  Structure of our actor

struct _zsync_watcher_t {
    zsock_t *pipe;                //  Pipe back to creator
    bool terminated;              //  Flag indicating termination
    zpoller_t *poller;            
    //  Declare properties
    int inotify_fd;               //  File descriptor for inotify events
    int wd;
    struct pollfd inotify_poller[1];
    zhash_t *watchlist;           //  List with all watched directories

    zlist_t *index;               //  List of indexed files
};


//  --------------------------------------------------------------------------
//  Create a new zsync_watcher.

static zsync_watcher_t *
zsync_watcher_new (zsock_t *pipe, void *args)
{
    zsync_watcher_t *self = (zsync_watcher_t *) zmalloc (sizeof (zsync_watcher_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->poller = zpoller_new (self->pipe, NULL);

    //  Initialize properties
    self->inotify_fd = inotify_init1(IN_NONBLOCK);
    self->inotify_poller[0].fd = self->inotify_fd;
    self->inotify_poller[0].events = POLLIN;
    assert (self->inotify_fd != -1);
    self->watchlist = zhash_new ();

    self->index = zlist_new ();
    zlist_equalsfn (self->index, zsync_equals);

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync_watcher.

static void
zsync_watcher_destroy (zsync_watcher_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_watcher_t *self = *self_p;

        //  Free actor properties
        close(self->inotify_fd);
        zhash_destroy (&self->watchlist);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  Start this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
zsync_watcher_start (zsync_watcher_t *self)
{
    assert (self);

    //  Startup actions
    self->wd = inotify_add_watch (self->inotify_fd, 
                                  "/home/ksapper/Workspace/zerosync/libzsync", 
                                  IN_CLOSE_WRITE | IN_DELETE |
                                  IN_DELETE_SELF | IN_IGNORED |
                                  IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO |
                                  IN_UNMOUNT);
    assert (self->wd != -1);
        
    return 0;
}


//  Stop this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
zsync_watcher_stop (zsync_watcher_t *self)
{
    assert (self);

    //  Shutdown actions
    inotify_rm_watch (self->inotify_fd, self->wd);

    return 0;
}


//  Here we handle incomming message from the node
           
static void
zsync_watcher_recv_api (zsync_watcher_t *self)
{      
//  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted
    
    char *command = zmsg_popstr (request);
    if (streq (command, "START")) 
        zsock_signal (self->pipe, zsync_watcher_start (self));
    else
    if (streq (command, "STOP")) 
        zsock_signal (self->pipe, zsync_watcher_stop (self));
    else
    if (streq (command, "$TERM"))
        //  The $TERM command is send by zactor_destroy() method
        self->terminated = true;
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
}


static zsync_file_t *
zsync_watcher_indexed_file (zsync_watcher_t *self, char *name)
{
    zsync_file_t *file = (zsync_file_t *) zlist_first (self->index);
    while (file) {
        if (streq (zsync_file_path (file), name))
            return file;
        file = (zsync_file_t *) zlist_next (self->index);   
    }
    return NULL;   
}

//  Here we handle inotify events

static void
zsync_watcher_inotify_recv (zsync_watcher_t *self)
{
    assert (self);
    int poll_items = 1;
    int poll_num = poll(self->inotify_poller, poll_items, 0);
    assert (poll_num != -1);

    if (poll_num > 0 && self->inotify_poller[0].revents & POLLIN) {
        /* Some systems cannot read integer variables if they are not
           properly aligned. On other systems, incorrect alignment may
           decrease performance. Hence, the buffer used for reading from
           the inotify file descriptor should have the same alignment as
           struct inotify_event. */

        char buf[16384] __attribute__ ((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event *event;
        ssize_t len;
        char *ptr;

        zlist_t *updates = zlist_new ();
        zlist_equalsfn (updates, zsync_equals);
        zlist_t *moves = zlist_new ();
        zlist_equalsfn (moves, zsync_equals);
        zlist_t *deletes = zlist_new ();
        zlist_equalsfn (deletes, zsync_equals);
        /* Loop while events can be read from inotify file descriptor. */
        for (;;) {

            /* Read some events. */
            len = read(self->inotify_fd, buf, sizeof buf);
            if (len == -1 && errno != EAGAIN) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            /* If the nonblocking read() found no events to read, then
               it returns -1 with errno set to EAGAIN. In that case,
               we exit the loop. */
            if (len <= 0)
                break;

            /* Loop over all events in the buffer */
            zsync_file_t *renamed;
            for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {

                //  Get event information
                event = (const struct inotify_event *) ptr;
                char *event_name = zsys_sprintf ("%s", event->name);
                //  Loopup existing file
                zsync_file_t *synced_file = zsync_watcher_indexed_file (self,  event_name);

                /* Print event type */
                if (synced_file) {
                    if (event->mask & IN_CLOSE_WRITE) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_UPD);
                        if (!zlist_exists (updates, synced_file))
                            zlist_append (updates, synced_file);
                        if (zlist_exists (moves, synced_file))
                            zlist_remove (moves, synced_file);
                    }
                    else
                    if (event->mask & IN_MOVED_FROM) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_REN);
                        renamed = synced_file;
                        if (!zlist_exists (moves, synced_file))
                            zlist_append (moves, synced_file);
                    }
                    else
                    if (event->mask & IN_MOVED_TO) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_UPD);
                        if (renamed) {
                            zsync_file_set_renamed_path (renamed, "%s", event->name);
                            renamed = NULL;
                        }
                        if (!zlist_exists (updates, synced_file))
                            zlist_append (updates, synced_file);
                    }
                    else
                    if (event->mask & IN_DELETE) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_DEL);
                        if (zlist_exists (updates, synced_file))
                            zlist_remove (updates, synced_file); 
                        if (zlist_exists (moves, synced_file))
                            zlist_remove (moves, synced_file); 
                        zlist_append (deletes, synced_file); 
                    }
                }
                else {
                    synced_file = zsync_file_new ();
                    zsync_file_set_path (synced_file, "%s", event->name);

                    if (event->mask & IN_CLOSE_WRITE) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_UPD);
                        if (!zlist_exists (updates, synced_file))
                            zlist_append (updates, synced_file);
                    }
                    else
                    if (event->mask & IN_MOVED_TO) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_UPD);
                        if (renamed) {
                            zsync_file_set_renamed_path (renamed, "%s", event->name);
                            renamed = NULL;
                        }
                        if (!zlist_exists (updates, synced_file))
                            zlist_append (updates, synced_file);
                    }
                    else
                    if (event->mask & IN_DELETE) {
                        zsync_file_set_operation (synced_file, ZS_FILE_OP_DEL);
                        if (zlist_exists (updates, synced_file)) {
                            zlist_remove (updates, synced_file);
                        }
                    }
                }
                    
                if (event->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
                if (event->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
                if (event->mask & IN_IGNORED)       printf("IN_IGNORED ");
                if (event->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
            }

            zsync_file_t *file = (zsync_file_t *) zlist_first (updates);
            while (file) {
               zsys_info ("File %s updated", zsync_file_path (file));
               if (!zlist_exists (self->index, file))
                    zlist_append (self->index, file);
               file  = (zsync_file_t *) zlist_next (updates);
            }
            zsync_file_t *filem = (zsync_file_t *) zlist_first (moves);
            while (filem) {
               zsys_info ("File %s moved", zsync_file_path (filem));
               if (!zlist_exists (self->index, filem))
                    zlist_append (self->index, filem);
               filem  = (zsync_file_t *) zlist_next (moves);
            }
            zsync_file_t *filed = (zsync_file_t *) zlist_first (deletes);
            while (filed) {
               zsys_info ("File %s deleted", zsync_file_path (filed));
               if (!zlist_exists (self->index, filed))
                    zlist_append (self->index, filed);
               filed  = (zsync_file_t *) zlist_next (deletes);
            }
            //  Print Index
            zsync_file_t *filei = (zsync_file_t *) zlist_first (self->index);
            zsys_info ("===============INDEX=================");
            while (filei) {
               zsys_info ("File %s (%d)", zsync_file_path (filei), zsync_file_operation (filei));
               filei  = (zsync_file_t *) zlist_next (self->index);
            }
            zsys_info ("======================================");

            zlist_destroy (&updates);
            zlist_destroy (&moves);
            zlist_destroy (&deletes);
        }
    } 
}


//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zsync_watcher_actor (zsock_t *pipe, void *args)
{
    zsync_watcher_t * self = zsync_watcher_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
       zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 100);
       if (which == self->pipe)
          zsync_watcher_recv_api (self);
       //  Add other sockets when you need them.
       zsync_watcher_inotify_recv (self);
    }

    zsync_watcher_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Self test of this actor.

void
zsync_watcher_test (bool verbose)
{
    printf (" * zsync_watcher: ");

    int rc = 0;
    //  @selftest
    //  Simple create/destroy test
    zactor_t *zsync_watcher = zactor_new (zsync_watcher_actor, NULL);
 
    zstr_send (zsync_watcher, "START");
    rc = zsock_wait (zsync_watcher);               //  Wait until actor started
    assert (rc == 0);
 
    zstr_send (zsync_watcher, "STOP");
    rc = zsock_wait (zsync_watcher);               //  Wait until actor stopped
    assert (rc == 0);
 
    zactor_destroy (&zsync_watcher);
    //  @end
 
    printf ("OK\n");
}
