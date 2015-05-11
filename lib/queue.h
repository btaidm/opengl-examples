/* Copyright (c) 2015 Scott Kuhl. All rights reserved.
 * License: This code is licensed under a 3-clause BSD license. See
 * the file named "LICENSE" for a full copy of the license.
 */

/** @file

    Provides a queue structure which is implemented with a circular
    buffer using the list structure. The list will be resized as
    needed as items are added to the queue.

    For example, if you want to store an int in the list, you would use:

    TODO

    Note that the queue will store a *copy* of the int inside of the
    queue---the queue does not store a list of pointers. If you want to
    make a queue of pointers, you should pass a pointer to a pointer
    into queue_enqueue();

    
    It is important to note that the list struct contains three
    variables that can be read but should not be changed by functions
    outside of queue.c:

    TODO

    @author Scott Kuhl
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
	
typedef struct {
	int read; /*< The next index we will read from. It is set to -1 if there is nothing to read from. */
	int write; /*< The next index we will write to. */
	int length; /*< Number of items in the queue. */
	list *l;
} queue;
	
queue* queue_new(int capacity, int itemSize);
int queue_reset(queue *q, int capacity, int itemSize);
void queue_free(queue *q);
queue* queue_copy(const queue *q);

int queue_add(queue *q, void *item);
int queue_remove(queue *q, void *item);
int queue_peek(queue *q, void *result);

int queue_length(const queue *q);
int queue_capacity(const queue *q);
int queue_reclaim(queue *q);

int queue_set_capacity(queue *q, int capacity);
	
void queue_sanity_check(const queue *q);
void queue_print_stats(const queue *q);
	
#ifdef __cplusplus
} // end extern "C"
#endif
#endif // end __QUEUE_H__
