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
	bool initialized: 1;
	/* leaves room for CLOCK_REALTIME (1, default) and CLOCK_MONOTONIC (4) */
	unsigned int clock: 3;
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

static inline bool is_timespec_valid(const struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);
	return (ts->tv_nsec >= 0) && (ts->tv_nsec < NSEC_PER_SEC);
}

struct posix_thread *to_posix_thread(pthread_t pth);

/* get and possibly initialize a posix_mutex */
struct k_mutex *to_posix_mutex(pthread_mutex_t *mu);

int posix_to_zephyr_priority(int priority, int policy);
int zephyr_to_posix_priority(int priority, int *policy);

static inline int64_t ts_to_ns(const struct timespec *ts)
{
	return ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

#define _tp_op(_a, _b, _op) (ts_to_ns(_a) _op ts_to_ns(_b))

#define _decl_op(_type, _name, _op)                                                                \
	static inline _type _name(const struct timespec *_a, const struct timespec *_b)            \
	{                                                                                          \
		return _tp_op(_a, _b, _op);                                                        \
	}

_decl_op(bool, tp_eq, ==);     /* a == b */
_decl_op(bool, tp_lt, <);      /* a < b */
_decl_op(bool, tp_gt, >);      /* a > b */
_decl_op(bool, tp_le, <=);     /* a <= b */
_decl_op(bool, tp_ge, >=);     /* a >= b */
_decl_op(int64_t, tp_diff, -); /* a - b */

/* lo <= (a - b) < hi */
static inline bool tp_diff_in_range_ns(const struct timespec *a, const struct timespec *b,
				       int64_t lo, int64_t hi)
{
	int64_t diff = tp_diff(a, b);

	return diff >= lo && diff < hi;
}

/**
 * @brief Convert an absolute time to a relative timeout in milliseconds.
 *
 * Convert the absolute time specified by @p abstime with respect to @p clock to a relative
 * timeout in milliseconds. The result is the number of milliseconds until the specified time,
<<<<<<< HEAD
 * clamped to the range [0, `INT32_MAX`] so that if @p abstime specifies a timepoint in the past,
=======
 * clamped to the range [0, `INT64_MAX`] so that if @p abstime specifies a timepoint in the past,
>>>>>>> 4def3d81eaf (posix + tests: correct timing functions to use CLOCK_REALTIME)
 * the result timeout is 0. If the clock specified by @p clock is not supported, the function
 * returns 0.
 *
 * @param clock The clock ID to use for conversion, e.g. `CLOCK_REALTIME`, or `CLOCK_MONOTONIC`.
 * @param abstime The absolute time to convert.
<<<<<<< HEAD
 * @return The relative timeout in milliseconds clamped to the range [0, `INT32_MAX`].
 */
uint32_t timespec_to_timeoutms(clockid_t clock, const struct timespec *abstime);
=======
 * @return The relative timeout in milliseconds clamped to the range [0, `INT64_MAX`].
 */
uint64_t timespec_to_timeoutms(clockid_t clock, const struct timespec *abstime);
>>>>>>> 4def3d81eaf (posix + tests: correct timing functions to use CLOCK_REALTIME)

#endif
