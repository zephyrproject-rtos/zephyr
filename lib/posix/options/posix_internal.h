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
#include <signal.h>
#include <zephyr/posix/sys/features.h>
#include <time.h>
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

struct z_sigset {
	unsigned long sig[DIV_ROUND_UP(32 + CONFIG_POSIX_RTSIG_MAX, BITS_PER_LONG)];
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

static inline int64_t timespec_to_timeoutms(const struct timespec *abstime)
{
	int64_t milli_secs, secs, nsecs;
	struct timespec curtime;

	/* FIXME: Zephyr does have CLOCK_REALTIME to get time.
	 * As per POSIX standard time should be calculated wrt CLOCK_REALTIME.
	 * Zephyr deviates from POSIX 1003.1 standard on this aspect.
	 */
	clock_gettime(CLOCK_MONOTONIC, &curtime);
	secs = abstime->tv_sec - curtime.tv_sec;
	nsecs = abstime->tv_nsec - curtime.tv_nsec;

	if (secs < 0 || (secs == 0 && nsecs < NSEC_PER_MSEC)) {
		milli_secs = 0;
	} else {
		milli_secs =  secs * MSEC_PER_SEC + nsecs / NSEC_PER_MSEC;
	}

	return milli_secs;
}

static inline int32_t _ts_to_ms(const struct timespec *to)
{
	return (to->tv_sec * MSEC_PER_SEC) + (to->tv_nsec / NSEC_PER_MSEC);
}

#define SIGNO_WORD_IDX(_signo) (signo / BITS_PER_LONG)
#define SIGNO_WORD_BIT(_signo) (signo & BIT_MASK(LOG2(BITS_PER_LONG)))

static inline void z_sigemptyset(struct z_sigset *dst)
{
	*dst = (struct z_sigset){0};
}

static inline void z_sigfillset(struct z_sigset *dst)
{
	for (size_t i = 0; i < ARRAY_SIZE(dst->sig); ++i) {
		dst->sig[i] = -1;
	}
}

static inline bool z_sigismember(const struct z_sigset *dst, int signo)
{
	return 1 & (dst->sig[SIGNO_WORD_IDX(signo)] >> SIGNO_WORD_BIT(signo));
}

static inline void z_sigaddset(struct z_sigset *dst, int signo)
{
	dst->sig[SIGNO_WORD_IDX(signo)] |= BIT(SIGNO_WORD_BIT(signo));
}

static inline void z_sigdelset(struct z_sigset *dst, int signo)
{
	dst->sig[SIGNO_WORD_IDX(signo)] &= ~BIT(SIGNO_WORD_BIT(signo));
}

static inline void z_signotset(struct z_sigset *dst, const struct z_sigset *src)
{
	for (size_t i = 0; i < ARRAY_SIZE(dst->sig); ++i) {
		dst->sig[i] = ~src->sig[i];
	}
}

static inline void z_sigandset(struct z_sigset *dst, const struct z_sigset *a,
			       const struct z_sigset *b)
{
	for (size_t i = 0; i < ARRAY_SIZE(dst->sig); ++i) {
		dst->sig[i] = a->sig[i] & b->sig[i];
	}
}

static inline void z_sigorset(struct z_sigset *dst, const struct z_sigset *a,
			      const struct z_sigset *b)
{
	for (size_t i = 0; i < ARRAY_SIZE(dst->sig); ++i) {
		dst->sig[i] = a->sig[i] | b->sig[i];
	}
}

#endif /* ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_ */
