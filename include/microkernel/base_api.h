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

#include <stdbool.h>
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

struct _k_mbox_struct {
	struct k_args *writers;
	struct k_args *readers;
	int count;
};

struct _k_mutex_struct {
	ktask_t owner;
	kpriority_t current_owner_priority;
	kpriority_t original_owner_priority;
	int level;
	struct k_args *waiters;
	int count;
	int num_conflicts;
};

struct _k_sem_struct {
	struct k_args *waiters;
	int level;
	int count;
};

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
		       (reads) still finishing up */
	BUFF_FULL, /* buffer is full, disregarding the pending data Xfers
		      (writes) still finishing up */
	BUFF_OTHER
} _K_PIPE_BUFF_STATE;

struct _k_pipe_desc {
	int buffer_size;
	unsigned char *pBegin;
	unsigned char *pWrite;
	unsigned char *pRead;
	unsigned char *pWriteGuard; /* can be NULL --> invalid */
	unsigned char *pReadGuard;  /* can be NULL --> invalid */
	int iFreeSpaceCont;
	int iFreeSpaceAWA;
	int iNbrPendingReads;
	int iAvailDataCont;
	int iAvailDataAWA; /* AWA == After Wrap Around */
	int iNbrPendingWrites;
	bool bWriteWA;
	bool bReadWA;
	_K_PIPE_BUFF_STATE BuffState;
	struct _k_pipe_marker_list WriteMarkers;
	struct _k_pipe_marker_list ReadMarkers;
	unsigned char *pEnd;
	unsigned char *pEndOrig;
};

struct _k_pipe_struct {
	int iBufferSize; /* size in bytes, must be first for sysgen */
	char *Buffer;    /* pointer to statically allocated buffer  */
	struct k_args *writers;
	struct k_args *readers;
	struct _k_pipe_desc desc;
	int count;
};

typedef enum {
	_0_TO_N = 0x00000001,
	_1_TO_N = 0x00000002,
	_ALL_N = 0x00000004
} K_PIPE_OPTION;

/* Memory map related structure */

struct _k_mem_map_struct {
	int Nelms;
	int element_size;
	char *base;
	char *Free;
	struct k_args *waiters;
	int num_used;
	int high_watermark;
	int count;
};

#ifdef __cplusplus
}
#endif

#endif /* _BASE_API_H */
