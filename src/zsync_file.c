/*  =========================================================================
    zsync_file - Representation of a syncronized file.

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
    ZeroSync - Representation of a synchronized file.
    
@discuss
@end
*/

#include "../include/libzsync.h"
#include "libzsync_classes.h"

#define STRING_MAX 255

struct _zsync_file_t {
    char *path;             // file path + file name
    char *path_renamed;     // file path + file name of renamed file
    int operation;
    uint64_t size;          // file size in bytes
    uint64_t timestamp;     // UNIX timestamp
    uint64_t checksum;      // SHA-3 512
};

//  Compares to files for equality.

int 
zsync_compare (void *item1, void *item2)
{
   assert (item1);
   assert (item2);

   zsync_file_t *file1 = (zsync_file_t *) item1;
   zsync_file_t *file2 = (zsync_file_t *) item2;

   return strcmp (file1->path, file2->path);
}


// --------------------------------------------------------------------------
// Create a new zsync_file

zsync_file_t * 
zsync_file_new () 
{
    zsync_file_t *self = (zsync_file_t *) zmalloc (sizeof (zsync_file_t));
    self->path = NULL;
    self->path_renamed = NULL;
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zsync_file

void 
zsync_file_destroy (zsync_file_t **self_p) 
{
    assert (self_p);

    if (*self_p) {
        zsync_file_t *self = *self_p;
        
        free (self->path);
        free (self->path_renamed);
        // Free object itself
        free (self);
        *self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Duplicate the zsync_file

zsync_file_t *
zsync_file_dup (zsync_file_t *self)
{
    assert (self);

    zsync_file_t *self_dup = zsync_file_new ();
    
    zsync_file_set_path (self_dup, "%s", self->path);
    zsync_file_set_renamed_path (self_dup, "%s", self->path_renamed);
    zsync_file_set_operation (self_dup, self->operation);
    zsync_file_set_size (self_dup, self->size);
    zsync_file_set_timestamp (self_dup, self->timestamp);
    zsync_file_set_checksum (self_dup, self->checksum);

    return self_dup;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data path

void
zsync_file_set_path (zsync_file_t *self, char *format, ...) 
{
    assert (self);
    // Format into newly allocated string
    va_list argptr;
    va_start (argptr, format);
    free (self->path);
    self->path = (char *) malloc (STRING_MAX + 1);
    assert (self->path);
    vsnprintf (self->path, STRING_MAX, format, argptr);
    va_end (argptr);
}

char *
zsync_file_path (zsync_file_t *self)
{
    assert (self);
    if (!self->path)
        return NULL;

    // copy string from struct 
    char *path = strdup (self->path);
    return path;
}    

// --------------------------------------------------------------------------
// Get/Set the renamed file meta data path

void
zsync_file_set_renamed_path (zsync_file_t *self, char *format, ...) 
{
    assert (self);
    // Format into newly allocated string
    va_list argptr;
    va_start (argptr, format);
    free (self->path_renamed);
    self->path_renamed = (char *) malloc (STRING_MAX + 1);
    assert (self->path_renamed);
    vsnprintf (self->path_renamed, STRING_MAX, format, argptr);
    va_end (argptr);
}

char *
zsync_file_renamed_path (zsync_file_t *self)
{
    assert (self);
    if (!self->path_renamed)
        return NULL;

    // copy string from struct 
    char *path = strdup (self->path_renamed);
    return path;
}    


// --------------------------------------------------------------------------
// Get/Set the file operation

void
zsync_file_set_operation(zsync_file_t *self, int operation)
{
    assert (self);
    self->operation = operation;
}

int
zsync_file_operation(zsync_file_t *self)
{
    assert (self);
    return self->operation;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data size

void
zsync_file_set_size (zsync_file_t *self, uint64_t size) 
{
    assert (self);
    self->size = size;
}    

uint64_t 
zsync_file_size (zsync_file_t *self) 
{
    assert (self);
    return self->size;
}    

// --------------------------------------------------------------------------
// Get/Set the file meta data timestamp

void
zsync_file_set_timestamp (zsync_file_t *self, uint64_t timestamp)
{
    assert (self);
    self->timestamp = timestamp;
}

uint64_t 
zsync_file_timestamp (zsync_file_t *self) 
{
    assert (self);
    return self->timestamp;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data checksum

void
zsync_file_set_checksum (zsync_file_t *self, uint64_t checksum)
{
    assert (self);
    self->checksum = checksum;
}

uint64_t 
zsync_file_checksum (zsync_file_t *self) 
{
    assert (self);
    return self->checksum;
}

// --------------------------------------------------------------------------
// Self test this class

int 
zsync_file_test () 
{
    return 0;
}

