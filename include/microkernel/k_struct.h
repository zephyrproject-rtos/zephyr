/* microkernel/k_struct.h */

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

#ifndef _K_STRUCT_H
#define _K_STRUCT_H

#include <microkernel/k_types.h>
#include <microkernel/k_chstr.h>
#include <nanokernel.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union k_args_args K_ARGS_ARGS;

struct k_timer;
typedef struct k_timer K_TIMER;

struct k_timer { /* For pointer declarations only !!	  */
	      /* Do not access fields in user code.	  */
	K_TIMER *Forw;
	K_TIMER *Back;
	int32_t Ti;
	int32_t Tr;
	struct k_args *Args;
}; /* use K_TIMER as the timer type name */

/* ---------------------------------------------------------------------- */
/* PROCESSOR SPECIFIC TYPES & DATA STRUCTURES */

/* Service codes */

typedef enum {
	NOP,
	MVD_REQ,
	MVD_VOID, /* obsolete now */
	RAWDATA,
	OFFLOAD,
	READWL,
	SIGNALS,
	SIGNALM,
	RESETS,
	RESETM,
	WAITSREQ,
	WAITSRPL,
	WAITSTMO,
	WAITMANY,
	WAITMREQ,
	WAITMRDY,
	WAITMCAN,
	WAITMACC,
	WAITMEND,
	WAITMTMO,
	INQSEMA,
	LOCK_REQ,
	LOCK_RPL,
	LOCK_TMO,
	UNLOCK,
	ENQ_REQ,
	ENQ_RPL,
	ENQ_TMO,
	DEQ_REQ,
	DEQ_RPL,
	DEQ_TMO,
	QUEUE,
	SEND_REQ,
	SEND_TMO,
	SEND_ACK,
	SEND_DATA,
	RECV_REQ,
	RECV_TMO,
	RECV_ACK,
	RECV_DATA,
	ELAPSE,
	SLEEP,
	WAKEUP,
	TSKOP,
	GRPOP,
	SPRIO,
	YIELD,
	ALLOC,
	DEALLOC,
	TALLOC,
	TDEALLOC,
	TSTART,
	TSTOP,
	ALLOCTMO,
	REMREPLY,
	DEBUG_REQ,
	DEBUG_ACK,
	EVENTENABLE, /* obsolete now */
	EVENTTEST,
	EVENTHANDLER,
	EVENTSIGNAL,
	GET_BLOCK,
	REL_BLOCK,
	GET_BLOCK_WAIT,
	GTBLTMO,
	POOL_DEFRAG,
	MVDSND_REQ,
	MVDRCV_REQ,
	MVDSND_ACK,
	MVDRCV_ACK,
	MEMCPY_REQ,
	MEMCPY_RPL,
	CHENQ_REQ,
	CHENQ_TMO,
	CHENQ_RPL,
	CHENQ_ACK,
	CHDEQ_REQ,
	CHDEQ_TMO,
	CHDEQ_RPL,
	CHDEQ_ACK,
	CH_MOVED_ACK,
	EVENT_TMO,
	UNDEFINED = -1
} K_COMM;

/* Task queue header */

struct k_tqhd {
	struct k_proc *Head;
	struct k_proc *Tail;
};

/* Task control block */

struct k_proc {
	struct k_proc *Forw;
	struct k_proc *Back;
	kpriority_t Prio;
	ktask_t Ident;
	uint32_t State;
	uint32_t Group;
	void (*fstart)(void);
	char *workspace;
	int worksize;
	void (*fabort)(void);
	struct k_args *Args;
};

/* Monitor record */

struct k_mrec {
	uint32_t time;
	uint32_t data1;
	uint32_t data2;
};

/* target channels types: */

struct pipe_struct {
	int iBufferSize; /* size in bytes, must be first for sysgen */
	char *Buffer;    /* pointer to statically allocated buffer  */
	struct k_args *Writers;
	struct k_args *Readers;
	struct chbuff Buff;
	int Count;
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
} CHREQ_STATUS;

struct req_info {
	union {
		kpipe_t Id;
		struct pipe_struct *pPipe;
	} ChRef;
	int Params;
};

struct sync_req {
	void *pData;
	int iSizeTotal;
};

struct async_req {
	struct k_block block;
	int iSizeTotal;
	ksem_t sema;
};

struct k_chreq {
	struct req_info ReqInfo;
	union {
		struct sync_req Sync;
		struct async_req Async;
	} ReqType;
	int Dummy;
};

struct k_chproc {
	struct req_info ReqInfo;
	void *pData; /* if NULL, data is embedded in
			     cmd packet		    */
	int iSizeTotal;      /* total size of data/free space    */
	int iSizeXferred;    /* size of data ALREADY Xferred	    */
	CHREQ_STATUS Status; /* status of processing of request  */
	int iNbrPendXfers;   /* # data Xfers (still) in progress */
};

struct k_chack {
	struct req_info ReqInfo;
	union {
		struct sync_req Dummy;
		struct async_req Async;
	} ReqType;
	int iSizeXferred;
};

struct k_chmovedack {
	struct pipe_struct *pPipe;
	XFER_TYPE XferType; /* W2B, B2R or W2R		    */
	struct k_args *pWriter;    /* if there's a writer involved,
				 this is the link to it      */
	struct k_args *pReader; /* if there's a reader involved,
				 this is the link to it      */
	int ID; /* if it is a Xfer to/from a buffer,
		   this is the registered Xfer's ID */
	int iSize; /* amount of data Xferred	    */
};

/* COMMAND PACKET STRUCTURES */

typedef union {
	ktask_t task;
	struct k_proc *proc;
	struct k_args *args;
} K_CREF;

struct _a1arg {
	kmemory_map_t mmap;
	void **mptr;
};

struct _c1arg {
	int64_t time1;
	int32_t time2;
	K_TIMER *timer;
	ksem_t sema;
	ktask_t task;
};

struct _e1arg {
	kevent_t event;
	int opt;
	kevent_handler_t func;
};

struct moved_req_args_setup {
	struct k_args *ContSnd;
	struct k_args *ContRcv;
	ksem_t Sema;
	uint32_t Dummy;
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
	MovedAction Action;
	void *source;
	void *destination;
	uint32_t iTotalSize;
	union {
		struct moved_req_args_setup Setup;
	} Extra;
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
	kmemory_pool_t poolid;
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
	struct moved_req MovedReq;
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
	struct k_chproc ChProc;
	struct k_chmovedack ChMovedAck;
	struct k_chreq ChReq;
	struct k_chack ChAck;
};

struct k_args {
	struct k_args *Forw;
	struct k_args **Head;
	kpriority_t Prio;
	bool    alloc;          /* true if allocated via GETARGS(); else false */
	K_COMM Comm;
	K_CREF Ctxt;
	union {
		int32_t ticks;
		K_TIMER *timer;
		int rcode;
	} Time;
	K_ARGS_ARGS Args;
};

/* ---------------------------------------------------------------------- */
/* KERNEL OBJECT STRUCTURES */

struct map_struct {
	int Nelms;
	int Esize;
	char *Base;
	char *Free;
	struct k_args *Waiters;
	int Nused;
	int Hmark;
	int Count;
};

struct que_struct {
	int Nelms;
	int Esize;
	char *Base;
	char *Endp;
	char *Enqp;
	char *Deqp;
	struct k_args *Waiters;
	int Nused;
	int Hmark;
	int Count;
};

struct mbx_struct {
	struct k_args *Writers;
	struct k_args *Readers;
	int Count;
};

struct mutex_struct {
	ktask_t Owner;
	kpriority_t OwnerCurrentPrio;
	kpriority_t OwnerOriginalPrio;
	int Level;
	struct k_args *Waiters;
	int Count;
	int Confl;
};

struct sem_struct {
	struct k_args *Waiters;
	int Level;
	int Count;
};

struct mon_struct {
	int timer;
	int action;
	ktask_t task;
};

struct block_stat {
	char *mem_blocks;
	uint32_t mem_status;
};

struct pool_block {
	int block_size;
	int nr_of_entries;
	struct block_stat *blocktable;
	int Count;
};

struct pool_struct {
	int maxblock_size;
	int minblock_size;
	int min_nr_blocks;
	int total_mem;
	int nr_of_maxblocks;
	int nr_of_frags;

	struct k_args *Waiters;

	struct pool_block *frag_tab;

	char *bufblock;
};

struct evstr {
	int status;
	kevent_handler_t func;
	struct k_args *waiter;
	int Count;
};

#ifdef __cplusplus
}
#endif

#endif /* _K_STRUCT_H */
