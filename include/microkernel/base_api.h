/* microkernel/base_api.h */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#ifndef _BASE_API_H
#define _BASE_API_H

#include <stdbool.h>
#include <stdint.h>
#include <toolchain.h>

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

/** for mail sender or receiver parameter  */
#define ANYTASK (-1)
/** this value terminates a semaphore list */
#define ENDLIST (-1)

struct k_args;

struct k_block {
	kmemory_pool_t pool_id;
	void *address_in_pool;
	void *pointer_to_data;
	uint32_t req_size;
};

struct k_msg {
	/** Mailbox ID */
	kmbox_t mailbox;
	/** size of message (bytes) */
	uint32_t size;
	/** information field, free for user   */
	uint32_t info;
	/** pointer to message data at sender side */
	void *tx_data;
	/** pointer to message data at receiver    */
	void *rx_data;
	/** for async message posting   */
	struct k_block tx_block;
	/** sending task */
	ktask_t tx_task;
	/** receiving task */
	ktask_t rx_task;
	/** internal use only */
	union {
		/** for 2-steps data transfer operation */
		struct k_args *transfer;
		/** semaphore to signal when asynchr. call */
		ksem_t sema;
	} extra;
};

/* Task control block */

struct k_task {
	struct k_task *next;
	struct k_task *prev;
	kpriority_t priority;
	ktask_t id;
	uint32_t state;
	uint32_t group;
	void (*fn_start)(void);
	char *workspace;
	int worksize;
	void (*fn_abort)(void);
	struct k_args *args;
};

/**
 * @cond internal
 */
struct _k_mbox_struct {
	struct k_args *writers;
	struct k_args *readers;
	int count;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct _k_mbox_struct *next;
#endif
};

struct _k_mutex_struct {
	ktask_t owner;
	kpriority_t current_owner_priority;
	kpriority_t original_owner_priority;
	int level;
	struct k_args *waiters;
	int count;
	int num_conflicts;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct _k_mutex_struct *next;
#endif
};

/*
 * Semaphore structure. Must be aligned on a 4-byte boundary, since this is what
 * the microkernel server's command stack processing requires.
 */
struct _k_sem_struct {
	struct k_args *waiters;
	int level;
	int count;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct _k_sem_struct *next;
#endif
} __aligned(4);

struct _k_fifo_struct {
	int Nelms;
	int element_size;
	char *base;
	char *end_point;
	char *enqueue_point;
	char *dequeue_point;
	struct k_args *waiters;
	int num_used;
	int high_watermark;
	int count;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct _k_fifo_struct *next;
#endif
};

/* Pipe-related structures */

#define MAXNBR_PIPE_MARKERS 10 /* 1==disable parallel transfers */

struct _k_pipe_marker {
	unsigned char *pointer; /* NULL == non valid marker == free */
	int size;
	bool buffer_xfer_busy;
	int prev; /* -1 == no predecessor */
	int next; /* -1 == no successor */
};

struct _k_pipe_marker_list {
	int num_markers;   /* Only used if STORE_NBR_MARKERS is defined */
	int first_marker;
	int last_marker;
	int post_wrap_around_marker; /* -1 means no post wrap around markers */
	struct _k_pipe_marker markers[MAXNBR_PIPE_MARKERS];
};

typedef enum {
	BUFF_EMPTY, /* buffer is empty, disregarding the pending data Xfers
		     * (reads) still finishing up
		     */
	BUFF_FULL, /* buffer is full, disregarding the pending data Xfers
		    * (writes) still finishing up
		    */
	BUFF_OTHER
} _K_PIPE_BUFF_STATE;

struct _k_pipe_desc {
	int buffer_size;
	unsigned char *begin_ptr;
	unsigned char *write_ptr;
	unsigned char *read_ptr;
	unsigned char *write_guard; /* can be NULL --> invalid */
	unsigned char *read_guard;  /* can be NULL --> invalid */
	int free_space_count;
	int free_space_post_wrap_around;
	int num_pending_reads;
	int available_data_count;
	int available_data_post_wrap_around; /* AWA == After Wrap Around */
	int num_pending_writes;
	bool wrap_around_write;
	bool wrap_around_read;
	_K_PIPE_BUFF_STATE buffer_state;
	struct _k_pipe_marker_list write_markers;
	struct _k_pipe_marker_list read_markers;
	unsigned char *end_ptr;
	unsigned char *original_end_ptr;
};

struct _k_pipe_struct {
	int buffer_size; /* size in bytes, must be first for sysgen */
	char *Buffer;    /* pointer to statically allocated buffer  */
	struct k_args *writers;
	struct k_args *readers;
	struct _k_pipe_desc desc;
	int count;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct _k_pipe_struct *next;
#endif
};

/* Memory map related structure */

struct _k_mem_map_struct {
	int Nelms;
	int element_size;
	char *base;
	char *free;
	struct k_args *waiters;
	int num_used;
	int high_watermark;
	int count;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct _k_mem_map_struct *next;
#endif
};

/*
 * Event structure. Must be aligned on a 4-byte boundary, since this is what
 * the microkernel server's command stack processing requires.
 */
struct _k_event_struct {
	int status;
	kevent_handler_t func;
	struct k_args *waiter;
	int count;
} __aligned(4);

#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
struct _k_mbox_struct *_track_list_micro_mbox;

struct _k_mutex_struct *_track_list_micro_mutex;

struct _k_sem_struct *_track_list_micro_sem;

struct _k_fifo_struct *_track_list_micro_fifo;

struct _k_pipe_struct *_track_list_micro_pipe;

struct _k_mem_map_struct *_track_list_micro_mem_map;
#endif

/**
 * @endcond
 */

typedef enum {
	_0_TO_N = 0x00000001,
	_1_TO_N = 0x00000002,
	_ALL_N = 0x00000004
} K_PIPE_OPTION;


#ifdef __cplusplus
}
#endif

#endif /* _BASE_API_H */
