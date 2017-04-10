/* config.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/* Max size of a message string */
#define MAX_MSG 256

/* flag for performing the Mailbox benchmark */
#define MAILBOX_BENCH

/* flag for performing the Sema benchmark */
#define SEMA_BENCH

/* flag for performing the FIFO benchmark */
#define FIFO_BENCH

/* flag for performing the Mutex benchmark */
#define MUTEX_BENCH

/* flag for performing the Memory Map benchmark */
#define MEMMAP_BENCH

/* flag for performing the Memory Pool benchmark*/
#define MEMPOOL_BENCH

/* flag for performing the Pipes benchmark */
#define PIPE_BENCH

/* flag for performing the Event benchmark */
#define EVENT_BENCH

#endif /* _CONFIG_H */
