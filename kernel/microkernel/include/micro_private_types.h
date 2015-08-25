/* major non-public microkernel structures */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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

#ifndef _MICRO_PRIVATE_TYPES_H
#define _MICRO_PRIVATE_TYPES_H

#include <microkernel/base_api.h>
#include <nanokernel.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union k_args_args K_ARGS_ARGS;

/* Kernel timer structure */

struct k_timer {
	struct k_timer *next;
	struct k_timer *prev;
	int32_t duration;
	int32_t period;
	struct k_args *args;
};

/* Kernel server command codes */

#define _K_SVC_UNDEFINED				(NULL)

#define _K_SVC_BLOCK_WAITERS_GET			_k_block_waiters_get
#define _K_SVC_DEFRAG					_k_defrag
#define _K_SVC_MOVEDATA_REQ				_k_movedata_request
#define _K_SVC_NOP					_k_nop
#define _K_SVC_OFFLOAD_TO_FIBER				_k_offload_to_fiber
#define _K_SVC_TIME_ELAPSE				_k_time_elapse
#define _K_SVC_WORKLOAD_GET				_k_workload_get

#define _K_SVC_EVENT_HANDLER_SET			_k_event_handler_set
#define _K_SVC_EVENT_SIGNAL				_k_event_signal
#define _K_SVC_EVENT_TEST				_k_event_test
#define _K_SVC_EVENT_TEST_TIMEOUT			_k_event_test_timeout

#define _K_SVC_SEM_INQUIRY				_k_sem_inquiry
#define _K_SVC_SEM_SIGNAL				_k_sem_signal
#define _K_SVC_SEM_RESET				_k_sem_reset
#define _K_SVC_SEM_WAIT_REQUEST				_k_sem_wait_request
#define _K_SVC_SEM_WAIT_REPLY				_k_sem_wait_reply
#define _K_SVC_SEM_WAIT_REPLY_TIMEOUT			_k_sem_wait_reply_timeout
#define _K_SVC_SEM_GROUP_SIGNAL				_k_sem_group_signal
#define _K_SVC_SEM_GROUP_RESET				_k_sem_group_reset
#define _K_SVC_SEM_GROUP_WAIT				_k_sem_group_wait
#define _K_SVC_SEM_GROUP_WAIT_ANY			_k_sem_group_wait_any
#define _K_SVC_SEM_GROUP_WAIT_ACCEPT			_k_sem_group_wait_accept
#define _K_SVC_SEM_GROUP_WAIT_CANCEL			_k_sem_group_wait_cancel
#define _K_SVC_SEM_GROUP_WAIT_READY			_k_sem_group_ready
#define _K_SVC_SEM_GROUP_WAIT_REQUEST			_k_sem_group_wait_request
#define _K_SVC_SEM_GROUP_WAIT_TIMEOUT			_k_sem_group_wait_timeout

#define _K_SVC_MUTEX_LOCK_REQUEST			_k_mutex_lock_request
#define _K_SVC_MUTEX_LOCK_REPLY				_k_mutex_lock_reply
#define _K_SVC_MUTEX_LOCK_REPLY_TIMEOUT			_k_mutex_lock_reply_timeout
#define _K_SVC_MUTEX_UNLOCK				_k_mutex_unlock

#define _K_SVC_FIFO_ENQUE_REQUEST			_k_fifo_enque_request
#define _K_SVC_FIFO_ENQUE_REPLY				_k_fifo_enque_reply
#define _K_SVC_FIFO_ENQUE_REPLY_TIMEOUT			_k_fifo_enque_reply_timeout
#define _K_SVC_FIFO_DEQUE_REQUEST			_k_fifo_deque_request
#define _K_SVC_FIFO_DEQUE_REPLY				_k_fifo_deque_reply
#define _K_SVC_FIFO_DEQUE_REPLY_TIMEOUT			_k_fifo_deque_reply_timeout
#define _K_SVC_FIFO_IOCTL				_k_fifo_ioctl

#define _K_SVC_MBOX_SEND_REQUEST			_k_mbox_send_request
#define _K_SVC_MBOX_SEND_REPLY				_k_mbox_send_reply
#define _K_SVC_MBOX_SEND_ACK				_k_mbox_send_ack
#define _K_SVC_MBOX_SEND_DATA				_k_mbox_send_data
#define _K_SVC_MBOX_RECEIVE_REQUEST			_k_mbox_receive_request
#define _K_SVC_MBOX_RECEIVE_REPLY			_k_mbox_receive_reply
#define _K_SVC_MBOX_RECEIVE_ACK				_k_mbox_receive_ack
#define _K_SVC_MBOX_RECEIVE_DATA			_k_mbox_receive_data

#define _K_SVC_TASK_SLEEP				_k_task_sleep
#define _K_SVC_TASK_WAKEUP				_k_task_wakeup
#define _K_SVC_TASK_OP					_k_task_op
#define _K_SVC_TASK_GROUP_OP				_k_task_group_op
#define _K_SVC_TASK_PRIORITY_SET			_k_task_priority_set
#define _K_SVC_TASK_YIELD				_k_task_yield

#define _K_SVC_MEM_MAP_ALLOC				_k_mem_map_alloc
#define _K_SVC_MEM_MAP_ALLOC_TIMEOUT			_k_mem_map_alloc_timeout
#define _K_SVC_MEM_MAP_DEALLOC				_k_mem_map_dealloc

#define _K_SVC_TIMER_ALLOC				_k_timer_alloc
#define _K_SVC_TIMER_DEALLOC				_k_timer_dealloc
#define _K_SVC_TIMER_START				_k_timer_start
#define _K_SVC_TIMER_STOP				_k_timer_stop

#define _K_SVC_MEM_POOL_BLOCK_GET			_k_mem_pool_block_get
#define _K_SVC_MEM_POOL_BLOCK_GET_TIMEOUT_HANDLE	_k_mem_pool_block_get_timeout_handle
#define _K_SVC_MEM_POOL_BLOCK_RELEASE			_k_mem_pool_block_release

#define _K_SVC_PIPE_PUT_REQUEST				_k_pipe_put_request
#define _K_SVC_PIPE_PUT_TIMEOUT				_k_pipe_put_timeout
#define _K_SVC_PIPE_PUT_REPLY				_k_pipe_put_reply
#define _K_SVC_PIPE_PUT_ACK				_k_pipe_put_ack
#define _K_SVC_PIPE_GET_REQUEST				_k_pipe_get_request
#define _K_SVC_PIPE_GET_TIMEOUT				_k_pipe_get_timeout
#define _K_SVC_PIPE_GET_REPLY				_k_pipe_get_reply
#define _K_SVC_PIPE_GET_ACK				_k_pipe_get_ack
#define _K_SVC_PIPE_MOVEDATA_ACK			_k_pipe_movedata_ack

/* Task queue header */

struct k_tqhd {
	struct k_task *head;
	struct k_task *tail;
};

/* Monitor record */

struct k_mrec {
	uint32_t time;
	uint32_t data1;
	uint32_t data2;
};

typedef enum {
	XFER_UNDEFINED,
	XFER_W2B,
	XFER_B2R,
	XFER_W2R
} XFER_TYPE;

typedef enum {
	XFER_IDLE = 0x0001,
	XFER_BUSY = 0x0002,
	TERM_FORCED = 0x0010,
	TERM_SATISFIED = 0x0020,
	TERM_TMO = 0x0040,
	TERM_XXX = TERM_FORCED | TERM_SATISFIED | TERM_TMO
} PIPE_REQUEST_STATUS;

struct req_info {
	union {
		kpipe_t id;
		struct _k_pipe_struct *ptr;
	} pipe;
	int params;
};

struct sync_req {
	void *data_ptr;
	int total_size;
};

struct async_req {
	struct k_block block;
	int total_size;
	ksem_t sema;
};

struct _pipe_req_arg {
	struct req_info req_info;
	union {
		struct sync_req sync;
		struct async_req async;
	} req_type;
	int dummy;
};

struct _pipe_xfer_req_arg {
	struct req_info req_info;
	void *data_ptr; /* if NULL, data is embedded in
			     cmd packet		    */
	int total_size;      /* total size of data/free space    */
	int xferred_size;    /* size of data ALREADY Xferred	    */
	PIPE_REQUEST_STATUS status; /* status of processing of request  */
	int num_pending_xfers;   /* # data Xfers (still) in progress */
};

struct _pipe_ack_arg {
	struct req_info req_info;
	union {
		struct sync_req dummy;
		struct async_req async;
	} req_type;
	int xferred_size;
};

struct _pipe_xfer_ack_arg {
	struct _k_pipe_struct *pipe_ptr;
	XFER_TYPE xfer_type; /* W2B, B2R or W2R		    */
	struct k_args *writer_ptr;    /* if there's a writer involved,
				 this is the link to it      */
	struct k_args *reader_ptr; /* if there's a reader involved,
				 this is the link to it      */
	int id; /* if it is a Xfer to/from a buffer,
		   this is the registered Xfer's ID */
	int size; /* amount of data Xferred	    */
};

/* COMMAND PACKET STRUCTURES */

typedef union {
	ktask_t task_id;
	struct k_task *task;
	struct k_args *args;
} K_CREF;

struct _a1arg {
	kmemory_map_t mmap;
	void **mptr;
};

struct _c1arg {
	int64_t time1;
	int64_t time2;
	struct k_timer *timer;
	ksem_t sema;
	ktask_t task;
};

struct _e1arg {
	kevent_t event;
	int opt;
	kevent_handler_t func;
};

struct moved_req_args_setup {
	struct k_args *continuation_send;
	struct k_args *continuation_receive;
	ksem_t sema;
	uint32_t dummy;
};

#define MVDACT_NONE 0

/* notify when data has been sent */
#define MVDACT_SNDACK 0x0001

/* notify when data has been received */
#define MVDACT_RCVACK 0x0002

/* Resume On Send (completion): the SeNDer (task) */
#define MVDACT_ROS_SND 0x0004

/* Resume On Recv (completion): the ReCeiVing (task) */
#define MVDACT_ROR_RCV 0x0008

#define MVDACT_VALID (MVDACT_SNDACK | MVDACT_RCVACK)
#define MVDACT_INVALID (~(MVDACT_VALID))

typedef uint32_t MovedAction;

struct moved_req {
	MovedAction action;
	void *source;
	void *destination;
	uint32_t total_size;
	union {
		struct moved_req_args_setup setup;
	} extra;
};

struct _g1arg {
	ktask_t task;
	ktask_group_t group;
	kpriority_t prio;
	int opt;
	int val;
};

struct _l1arg {
	kmutex_t mutex;
	ktask_t task;
};

struct _m1arg {
	struct k_msg mess;
};

struct _p1arg {
	kmemory_pool_t pool_id;
	int req_size;
	void *rep_poolptr;
	void *rep_dataptr;
};

struct _q1arg {
	kfifo_t queue;
	int size;
	char *data;
};

struct _q2arg {
	kfifo_t queue;
	int size;
	char data[OCTET_TO_SIZEOFUNIT(40)];
};

struct _s1arg {
	ksem_t sema;
	ksemg_t list;
	int nsem;
};

struct _u1arg {
	int (*func)();
	void *argp;
	int rval;
};

struct _z4arg {
	int Comm;
	int rind;
	int nrec;
	struct k_mrec mrec;
};

union k_args_args {
	struct _a1arg a1;
	struct _c1arg c1;
	struct moved_req moved_req;
	struct _e1arg e1;
	struct _g1arg g1;
	struct _l1arg l1;
	struct _m1arg m1;
	struct _p1arg p1;
	struct _q1arg q1;
	struct _q2arg q2;
	struct _s1arg s1;
	struct _u1arg u1;
	struct _z4arg z4;
	struct _pipe_xfer_req_arg pipe_xfer_req;
	struct _pipe_xfer_ack_arg pipe_xfer_ack;
	struct _pipe_req_arg pipe_req;
	struct _pipe_ack_arg pipe_ack;
};

/*
 * The size of the k_args structure must be equivalent to ...
 *     CMD_PKT_SIZE_IN_WORDS * sizeof(uint32_t)
 * To this end the entire structure is packed.  This ensures that the compiler
 * aligns 'args' to a 4-byte boundary.  If left unpacked, then some compilers
 * may provide an extra 4 bytes of padding to align it to an 8-byte boundary,
 * thereby violating the previously stated equivalence.
 */
struct k_args {
	struct k_args *next;
	struct k_args **head;
	kpriority_t priority;

	/* 'alloc' is true if k_args is allocated via GETARGS() */
	bool   alloc;

	/*
	 * Align the next structure element if alloc is just one byte.
	 * Otherwise on ARM it leads to "unaligned write" exception.
	 */
	void   (*Comm)(struct k_args *) __aligned(4);
	K_CREF Ctxt;
	union {
		int32_t ticks;
		struct k_timer *timer;
		int rcode;
	} Time;
	K_ARGS_ARGS args;
} __packed;

/* ---------------------------------------------------------------------- */
/* KERNEL OBJECT STRUCTURES */

struct block_stat {
	char *mem_blocks;
	uint32_t mem_status;
};

struct pool_block {
	int block_size;
	int nr_of_entries;
	struct block_stat *blocktable;
	int count;
};

struct pool_struct {
	int maxblock_size;
	int minblock_size;
	int min_nr_blocks;
	int total_mem;
	int nr_of_maxblocks;
	int nr_of_frags;

	struct k_args *waiters;

	struct pool_block *frag_tab;

	char *bufblock;
};

struct evstr {
	int status;
	kevent_handler_t func;
	struct k_args *waiter;
	int count;
};

#ifdef __cplusplus
}
#endif

#endif /* _MICRO_PRIVATE_TYPES_H */
