/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_
#define ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/signal.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>

/*
 * Bit used to mark a pthread object as initialized. Initialization status is
 * verified (against internal status) in lock / unlock / destroy functions.
 */
#define PTHREAD_OBJ_MASK_INIT 0x80000000

struct posix_thread_attr {
	void *stack;
	/* the following two bitfields should combine to be 32-bits in size */
	uint32_t stacksize : CONFIG_POSIX_PTHREAD_ATTR_STACKSIZE_BITS;
	uint16_t guardsize : CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_BITS;
	int8_t priority;
	uint8_t schedpolicy: 2;
	bool contentionscope: 1;
	bool inheritsched: 1;
	union {
		bool caller_destroys: 1;
		bool initialized: 1;
	};
	bool cancelpending: 1;
	bool cancelstate: 1;
	bool canceltype: 1;
	bool detachstate: 1;
};

struct posix_thread {
	struct k_thread thread;

	/* List nodes for pthread_cleanup_push() / pthread_cleanup_pop() */
	sys_slist_t cleanup_list;

	/* List node for ready_q, run_q, or done_q */
	sys_dnode_t q_node;

	/* List of keys that thread has called pthread_setspecific() on */
	sys_slist_t key_list;

	/* pthread_attr_t */
	struct posix_thread_attr attr;

	/* Exit status */
	void *retval;

	/* Signal mask */
	sigset_t sigset;

	/* Queue ID (internal-only) */
	uint8_t qid;
};

struct posix_condattr {
	/* leaves room for CLOCK_REALTIME (1, default) and CLOCK_MONOTONIC (4) */
	unsigned int clock: 3;
	bool initialized: 1;
#ifdef _POSIX_THREAD_PROCESS_SHARED
	unsigned int pshared: 1;
#endif
};

struct posix_cond {
	struct k_condvar condvar;
	struct posix_condattr attr;
};

typedef struct pthread_key_obj {
	/* List of pthread_key_data objects that contain thread
	 * specific data for the key
	 */
	sys_slist_t key_data_l;

	/* Optional destructor that is passed to pthread_key_create() */
	void (*destructor)(void *value);
} pthread_key_obj;

typedef struct pthread_thread_data {
	sys_snode_t node;

	/* Key and thread specific data passed to pthread_setspecific() */
	pthread_key_obj *key;
	void *spec_data;
} pthread_thread_data;

struct pthread_key_data {
	sys_snode_t node;
	pthread_thread_data thread_data;
};

static inline bool is_pthread_obj_initialized(uint32_t obj)
{
	return (obj & PTHREAD_OBJ_MASK_INIT) != 0;
}

static inline uint32_t mark_pthread_obj_initialized(uint32_t obj)
{
	return obj | PTHREAD_OBJ_MASK_INIT;
}

static inline uint32_t mark_pthread_obj_uninitialized(uint32_t obj)
{
	return obj & ~PTHREAD_OBJ_MASK_INIT;
}

struct posix_thread *to_posix_thread(pthread_t pth);

/* get and possibly initialize a posix_mutex */
struct k_mutex *to_posix_mutex(pthread_mutex_t *mu);

int posix_to_zephyr_priority(int priority, int policy);
int zephyr_to_posix_priority(int priority, int *policy);

#endif
