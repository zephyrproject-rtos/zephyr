/* config.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
