/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PTHREAD_H__
#define __PTHREAD_H__
#include "sched.h"
#ifdef CONFIG_NEWLIB_LIBC
#include <time.h>
#else
/* This should probably live somewhere else but Zephyr doesn't
 * currently have a stdc layer to provide it
 */
struct timespec {
	s32_t tv_sec;
	s32_t tv_nsec;
};
#endif

/* Pthread cancellation */
#define PTHREAD_CANCEL_POS	0
/* Non Essential Thread */
#define PTHREAD_CANCELABLE	(0 << PTHREAD_CANCEL_POS)
/* Essential Thread */
#define PTHREAD_NON_CANCELABLE	(1 << PTHREAD_CANCEL_POS)


/* FIXME: Following typedef should be part of sys/types.h */
/* Thread Attributes */
typedef struct pthread_attr_t {
	int priority;
	void *stack;
	size_t stacksize;
	u32_t options;      /* Thread related Flags */
	u32_t delayedstart;
} pthread_attr_t;

typedef void *pthread_t;

/* Mutex */
typedef struct pthread_mutex {
	struct k_sem *sem;
} pthread_mutex_t;

typedef struct pthread_mutexattr {
	int unused;
} pthread_mutexattr_t;

/* Condition Variable */
typedef struct pthread_cond {
	_wait_q_t wait_q;
} pthread_cond_t;

typedef struct pthread_condattr {
	int unused;
} pthread_condattr_t;

/* Pthread Barrier */
typedef struct pthread_barrier {
	_wait_q_t wait_q;
	int max;
	int count;
} pthread_barrier_t;

typedef struct pthread_barrierattr {
	int unused;
} pthread_barrierattr_t;


#define K_LOWEST_STACK_SIZE     256
#define THREAD_INIT_FLAGS       PTHREAD_CANCELABLE
/*
 *  Default pthread attributes on attributes initialization.
 */
static pthread_attr_t initpthreadattrs = {
	K_LOWEST_APPLICATION_THREAD_PRIO,/* priority */
	NULL,                   /* stack */
	K_LOWEST_STACK_SIZE,
	THREAD_INIT_FLAGS,   /* Thread Flags */
	K_NO_WAIT            /* Start on initialization */
};


static inline s32_t _ts_to_ms(const struct timespec *to)
{
	return (to->tv_sec * 1000) + (to->tv_nsec / 1000000);
}

/**
 * @brief Declare a pthread condition variable
 *
 * Declaration API for a pthread condition variable.  This is not a
 * POSIX API, it's provided to better conform with Zephyr's allocation
 * strategies for kernel objects.
 *
 * @param name Symbol name of the condition variable
 */
#define PTHREAD_COND_DEFINE(name)					\
	struct pthread_cond name = {					\
		.wait_q = SYS_DLIST_STATIC_INIT(&name.wait_q),		\
	}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_cond_init(pthread_cond_t *cv,
				    const pthread_condattr_t *att)
{
	ARG_UNUSED(att);
	sys_dlist_init(&cv->wait_q);
	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_cond_destroy(pthread_cond_t *cv)
{
	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
int pthread_cond_signal(pthread_cond_t *cv);

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
int pthread_cond_broadcast(pthread_cond_t *cv);

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mut);

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mut,
			   const struct timespec *to);

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1.
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 */
static inline int pthread_condattr_init(pthread_condattr_t *att)
{
	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 */
static inline int pthread_condattr_destroy(pthread_condattr_t *att)
{
	return 0;
}

/**
 * @brief Declare a pthread mutex
 *
 * Declaration API for a pthread mutex.  This is not a POSIX API, it's
 * provided to better conform with Zephyr's allocation strategies for
 * kernel objects.
 *
 * @param name Symbol name of the mutex
 */
#define PTHREAD_MUTEX_DEFINE(name)		\
	K_SEM_DEFINE(name##_psem, 1, 1);	\
	struct pthread_mutex name = {		\
		.sem = &name##_psem,		\
	}


/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_mutex_init(pthread_mutex_t *m,
				     const pthread_mutexattr_t *att)
{
	ARG_UNUSED(att);

	k_sem_init(m->sem, 1, 1);

	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_mutex_destroy(pthread_mutex_t *m)
{
	ARG_UNUSED(m);

	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_mutex_lock(pthread_mutex_t *m)
{
	return k_sem_take(m->sem, K_FOREVER);
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_mutex_timedlock(pthread_mutex_t *m,
					  const struct timespec *to)
{
	int ret = k_sem_take(m->sem, _ts_to_ms(to));

	return ret == -EAGAIN ? -ETIMEDOUT : ret;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
int pthread_mutex_trylock(pthread_mutex_t *m);

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_mutex_unlock(pthread_mutex_t *m)
{
	k_sem_give(m->sem);
	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 */
static inline int pthread_mutexattr_init(pthread_mutexattr_t *m)
{
	ARG_UNUSED(m);

	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 */
static inline int pthread_mutexattr_destroy(pthread_mutexattr_t *m)
{
	ARG_UNUSED(m);

	return 0;
}

/* FIXME: these are going to be tricky to implement.  Zephyr has (for
 * good reason) deprecated its own "initializer" macros in favor of a
 * static "declaration" macros instead.  Using such a macro inside a
 * gcc compound expression to declare and object then reference it
 * would work, but gcc limits such expressions to function context
 * (because they may need to generate code that runs at assignment
 * time) and much real-world use of these initializers is for static
 * variables.  The best trick I can think of would be to declare it in
 * a special section and then initialize that section at runtime
 * startup, which sort of defeats the purpose of having these be
 * static...
 *
 * Instead, see the nonstandard PTHREAD_*_DEFINE macros instead, which
 * work similarly but conform to Zephyr's paradigms.
 */
/* #define PTHREAD_MUTEX_INITIALIZER */
/* #define PTHREAD_COND_INITIALIZER */


/**
 * @brief Declare a pthread barrier
 *
 * Declaration API for a pthread barrier.  This is not a
 * POSIX API, it's provided to better conform with Zephyr's allocation
 * strategies for kernel objects.
 *
 * @param name Symbol name of the barrier
 * @param count Thread count, same as the "count" argument to
 *             pthread_barrier_init()
 */
#define PTHREAD_BARRIER_DEFINE(name, count)			\
	struct pthread_barrier name = {				\
		.wait_q = SYS_DLIST_STATIC_INIT(&name.wait_q),	\
		.max = count,					\
	}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
int pthread_barrier_wait(pthread_barrier_t *b);

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_barrier_init(pthread_barrier_t *b,
				       const pthread_barrierattr_t *attr,
				       unsigned int count)
{
	ARG_UNUSED(attr);

	b->max = count;
	b->count = 0;
	sys_dlist_init(&b->wait_q);

	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 */
static inline int pthread_barrier_destroy(pthread_barrier_t *b)
{
	ARG_UNUSED(b);

	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 */
static inline int pthread_barrierattr_init(pthread_barrierattr_t *b)
{
	ARG_UNUSED(b);

	return 0;
}

/**
 * @brief POSIX threading compatibility API
 *
 * See IEEE 1003.1
 *
 * Note that pthread attribute structs are currently noops in Zephyr.
 */
static inline int pthread_barrierattr_destroy(pthread_barrierattr_t *b)
{
	ARG_UNUSED(b);

	return 0;
}

/* Predicates and setters for various pthread attribute values that we
 * don't support (or always support: the "process shared" attribute
 * can only be true given the way Zephyr implements these
 * objects). Leave these undefined for simplicity instead of defining
 * stubs to return an error that would have to be logged and
 * interpreted just to figure out that we didn't support it in the
 * first place. These APIs are very rarely used even in production
 * Unix code.  Leave the declarations here so they can be easily
 * uncommented and implemented as needed.

int pthread_condattr_getclock(const pthread_condattr_t * clockid_t *);
int pthread_condattr_getpshared(const pthread_condattr_t * int *);
int pthread_condattr_setclock(pthread_condattr_t *, clockid_t);
int pthread_condattr_setpshared(pthread_condattr_t *, int);
int pthread_mutex_consistent(pthread_mutex_t *);
int pthread_mutex_getprioceiling(const pthread_mutex_t * int *);
int pthread_mutex_setprioceiling(pthread_mutex_t *, int int *);
int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *, int *);
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t * int *);
int pthread_mutexattr_getpshared(const pthread_mutexattr_t * int *);
int pthread_mutexattr_getrobust(const pthread_mutexattr_t * int *);
int pthread_mutexattr_gettype(const pthread_mutexattr_t * int *);
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *, int);
int pthread_mutexattr_settype(pthread_mutexattr_t *, int);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *, int *);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *, int);
*/


/* Base Pthread related APIs */

/**
 * @brief POSIX pthread attributes initialization
 *
 * See IEEE 1003.1
 */
static inline int pthread_attr_init(pthread_attr_t *attr)
{
	*attr = initpthreadattrs;
	return 0;
}

/**
 * @brief POSIX pthread attributes destroy
 *
 * See IEEE 1003.1
 */
static inline int pthread_attr_destroy(pthread_attr_t *attr)
{
	return 0;
}

/**
 * @brief POSIX get pthread scheduling parameter
 *
 * See IEEE 1003.1
 */
static inline int pthread_attr_getschedparam(const pthread_attr_t *attr,
		struct sched_param *schedparam)
{
	schedparam->priority = attr->priority;
	return 0;
}

/**
 * @brief POSIX get pthread stack inforamtion from thread attribute
 *
 * See IEEE 1003.1
 */
static inline int pthread_attr_getstack(const pthread_attr_t *attr,
		void **stackaddr, size_t *stacksize)
{
	*stackaddr = attr->stack;
	*stacksize = attr->stacksize;
	return 0;
}

/**
 * @brief POSIX get stack size from thread attribute
 *
 * See IEEE 1003.1
 */
static inline int pthread_attr_getstacksize(const pthread_attr_t *attr,
	size_t *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

/**
 * @brief are Both pthreads same
 *
 * See IEEE 1003.1
 */
static inline int pthread_equal(pthread_t pt1, pthread_t pt2)
{
	return (pt1 == pt2);
}

/**
 * @brief pthread exits the system.
 *
 * See IEEE 1003.1
 */
static inline void pthread_exit(void *retval)
{
	k_thread_abort(k_current_get());
}

/**
 * @brief get scheduling information about a pthread.
 *
 * See IEEE 1003.1
 */
static inline int pthread_getschedparam(pthread_t pthread, int *policy,
		struct sched_param *param)
{
	k_tid_t *thread = (k_tid_t *)pthread;

	param->priority = k_thread_priority_get(*thread);
	*policy =  (param->priority < 0) ? SCHED_FIFO : SCHED_RR;
	return 0;
}

/**
 * @brief get pid of current execueting thread.
 *
 * See IEEE 1003.1
 */
static inline pthread_t pthread_self(void)
{
	return k_current_get();
}

int pthread_attr_setschedparam(pthread_attr_t *attr,
		const struct sched_param *schedparam);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr,
		size_t stacksize);
int pthread_create(pthread_t *newthread, const pthread_attr_t *attr,
		void *(*threadroutine)(void *), void *arg);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setschedparam(pthread_t pthread, int policy,
		const struct sched_param *param);

#endif /* __PTHREAD_H__ */
