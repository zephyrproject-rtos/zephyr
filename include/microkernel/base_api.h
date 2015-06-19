/* microkernel/base_api.h */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BASE_API_H
#define _BASE_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t ktask_t;
typedef uint32_t ktask_group_t;
typedef uint32_t kmutex_t;
typedef uint32_t kmemory_map_t;
typedef uint32_t kfifo_t;
typedef uint32_t kmbox_t;
typedef uint32_t kpipe_t;
typedef int32_t ksem_t;
typedef ksem_t *ksemg_t;
typedef uint32_t ktimer_t;
typedef uint32_t kpriority_t;
typedef uint32_t kmemory_pool_t;
typedef unsigned int kevent_t;
typedef uint32_t kirq_t;

typedef int (*kevent_handler_t)(int event);

#define RC_OK 0
#define RC_FAIL 1
#define RC_TIME 2
#define RC_ALIGNMENT 3
#define RC_INCOMPLETE 4

#define ANYTASK (-1)  /* for mail sender or receiver parameter  */
#define ENDLIST (-1)  /* this value terminates a semaphore list */

struct k_args;

struct k_block {
	kmemory_pool_t poolid;
	void *address_in_pool;
	void *pointer_to_data;
	uint32_t req_size;
};

struct k_msg {
	kmbox_t mailbox;   /* Mailbox ID				  */
	uint32_t size;    /* size of message (bytes)		  */
	uint32_t info;    /* information field, free for user	  */
	void *tx_data;    /* pointer to message data at sender side */
	void *rx_data;    /* pointer to message data at receiver    */
	struct k_block tx_block; /* for async message posting		  */
	ktask_t tx_task;   /* sending task				  */
	ktask_t rx_task;   /* receiving task			  */
	union {			  /* internal use only			  */
		struct k_args *transfer; /* for 2-steps data transfer operation
				     */
		ksem_t sema; /* semaphore to signal when asynchr. call */
	} extra;
};

typedef enum {
	_0_TO_N = 0x00000001,
	_1_TO_N = 0x00000002,
	_ALL_N = 0x00000004
} K_PIPE_OPTION;

#ifdef __cplusplus
}
#endif

#endif /* _BASE_API_H */
