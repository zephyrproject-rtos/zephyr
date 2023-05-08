/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <pthread.h>
#else
#include <zephyr/posix/pthread.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
/**
 * @brief existence test for `<pthread.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html">pthread.h</a>
 */
ZTEST(posix_headers, test_pthread_h)
{
	zassert_not_equal(-1, PTHREAD_BARRIER_SERIAL_THREAD);

	/* zassert_not_equal(-1, PTHREAD_CANCEL_ASYNCHRONOUS); */ /* not implemented */
	zassert_not_equal(-1, PTHREAD_CANCEL_ENABLE);
	/* zassert_not_equal(-1, PTHREAD_CANCEL_DEFERRED); */ /* not implemented */
	zassert_not_equal(-1, PTHREAD_CANCEL_DISABLE);

	/* zassert_not_equal(-1, PTHREAD_CANCELED); */ /* not implemented */

	zassert_not_equal(-1, PTHREAD_CREATE_DETACHED);
	zassert_not_equal(-1, PTHREAD_CREATE_JOINABLE);

	/* zassert_not_equal(-1, PTHREAD_EXPLICIT_SCHED); */ /* not implemented */
	/* zassert_not_equal(-1, PTHREAD_INHERIT_SCHED); */ /* not implemented */

	zassert_not_equal(-1, PTHREAD_MUTEX_DEFAULT);
	zassert_not_equal(-1, PTHREAD_MUTEX_ERRORCHECK);
	zassert_not_equal(-1, PTHREAD_MUTEX_ERRORCHECK);
	zassert_not_equal(-1, PTHREAD_MUTEX_RECURSIVE);
	/* zassert_not_equal(-1, PTHREAD_MUTEX_ROBUST); */ /* not implemented */
	/* zassert_not_equal(-1, PTHREAD_MUTEX_STALLED); */ /* not implemented */

	pthread_once_t once = PTHREAD_ONCE_INIT;

	/* zassert_not_equal(-1, PTHREAD_PRIO_INHERIT); */ /* not implemented */
	zassert_not_equal(-1, PTHREAD_PRIO_NONE);
	/* zassert_not_equal(-1, PTHREAD_PRIO_PROTECT); */ /* not implemented */

	/* zassert_not_equal(-1, PTHREAD_PROCESS_SHARED); */ /* not implemented */
	/* zassert_not_equal(-1, PTHREAD_PROCESS_PRIVATE); */ /* not implemented */

	/* zassert_not_equal(-1, PTHREAD_SCOPE_PROCESS); */ /* not implemented */
	/* zassert_not_equal(-1, PTHREAD_SCOPE_SYSTEM); */ /* not implemented */

	/* pthread_cond_t cond = PTHREAD_COND_INITIALIZER; */ /* not implemented */
	/* pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER; */ /* not implemented */
	/* pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER; */ /* not implemented */

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		/* zassert_not_null(pthread_atfork); */ /* not implemented */
		zassert_not_null(pthread_attr_destroy);
		zassert_not_null(pthread_attr_getdetachstate);
		/* zassert_not_null(pthread_attr_getguardsize); */ /* not implemented */
		/* zassert_not_null(pthread_attr_getinheritsched); */ /* not implemented */
		zassert_not_null(pthread_attr_getschedparam);
		zassert_not_null(pthread_attr_getschedpolicy);
		/* zassert_not_null(pthread_attr_getscope); */ /* not implemented */
		zassert_not_null(pthread_attr_getstack);
		zassert_not_null(pthread_attr_getstacksize);
		zassert_not_null(pthread_attr_init);
		zassert_not_null(pthread_attr_setdetachstate);
		/* zassert_not_null(pthread_attr_setguardsize); */ /* not implemented */
		/* zassert_not_null(pthread_attr_setinheritsched); */ /* not implemented */
		zassert_not_null(pthread_attr_setschedparam);
		zassert_not_null(pthread_attr_setschedpolicy);
		/* zassert_not_null(pthread_attr_setscope); */ /* not implemented */
		zassert_not_null(pthread_attr_setstack);
		zassert_not_null(pthread_attr_setstacksize);
		zassert_not_null(pthread_barrier_destroy);
		zassert_not_null(pthread_barrier_init);
		zassert_not_null(pthread_barrier_wait);
		zassert_not_null(pthread_barrierattr_destroy);
		/* zassert_not_null(pthread_barrierattr_getpshared); */ /* not implemented */
		zassert_not_null(pthread_barrierattr_init);
		/* zassert_not_null(pthread_barrierattr_setpshared); */ /* not implemented */
		zassert_not_null(pthread_cancel);
		zassert_not_null(pthread_cond_broadcast);
		zassert_not_null(pthread_cond_destroy);
		zassert_not_null(pthread_cond_init);
		zassert_not_null(pthread_cond_signal);
		zassert_not_null(pthread_cond_timedwait);
		zassert_not_null(pthread_cond_wait);
		zassert_not_null(pthread_condattr_destroy);
		/* zassert_not_null(pthread_condattr_getclock); */ /* not implemented */
		/* zassert_not_null(pthread_condattr_getpshared); */ /* not implemented */
		zassert_not_null(pthread_condattr_init);
		/* zassert_not_null(pthread_condattr_setclock); */ /* not implemented */
		/* zassert_not_null(pthread_condattr_setpshared); */ /* not implemented */
		zassert_not_null(pthread_create);
		zassert_not_null(pthread_detach);
		zassert_not_null(pthread_equal);
		zassert_not_null(pthread_exit);
		/* zassert_not_null(pthread_getconcurrency); */ /* not implemented */
		/* zassert_not_null(pthread_getcpuclockid); */ /* not implemented */
		zassert_not_null(pthread_getschedparam);
		zassert_not_null(pthread_getspecific);
		zassert_not_null(pthread_join);
		zassert_not_null(pthread_key_create);
		zassert_not_null(pthread_key_delete);
		/* zassert_not_null(pthread_mutex_consistent); */ /* not implemented */
		zassert_not_null(pthread_mutex_destroy);
		/* zassert_not_null(pthread_mutex_getprioceiling); */ /* not implemented */
		zassert_not_null(pthread_mutex_init);
		zassert_not_null(pthread_mutex_lock);
		/* zassert_not_null(pthread_mutex_setprioceiling); */ /* not implemented */
		zassert_not_null(pthread_mutex_timedlock);
		zassert_not_null(pthread_mutex_trylock);
		zassert_not_null(pthread_mutex_unlock);
		zassert_not_null(pthread_mutexattr_destroy);
		/* zassert_not_null(pthread_mutexattr_getprioceiling); */ /* not implemented */
		zassert_not_null(pthread_mutexattr_getprotocol);
		/* zassert_not_null(pthread_mutexattr_getpshared); */ /* not implemented */
		/* zassert_not_null(pthread_mutexattr_getrobust); */ /* not implemented */
		zassert_not_null(pthread_mutexattr_gettype);
		zassert_not_null(pthread_mutexattr_init);
		/* zassert_not_null(pthread_mutexattr_setprioceiling); */ /* not implemented */
		/* zassert_not_null(pthread_mutexattr_setprotocol); */ /* not implemented */
		/* zassert_not_null(pthread_mutexattr_setpshared); */ /* not implemented */
		/* zassert_not_null(pthread_mutexattr_setrobust); */ /* not implemented */
		zassert_not_null(pthread_mutexattr_settype);
		zassert_not_null(pthread_once);
		zassert_not_null(pthread_rwlock_destroy);
		zassert_not_null(pthread_rwlock_init);
		zassert_not_null(pthread_rwlock_rdlock);
		zassert_not_null(pthread_rwlock_timedrdlock);
		zassert_not_null(pthread_rwlock_timedwrlock);
		zassert_not_null(pthread_rwlock_tryrdlock);
		zassert_not_null(pthread_rwlock_trywrlock);
		zassert_not_null(pthread_rwlock_unlock);
		zassert_not_null(pthread_rwlock_wrlock);
		zassert_not_null(pthread_rwlockattr_destroy);
		/* zassert_not_null(pthread_rwlockattr_getpshared); */ /* not implemented */
		zassert_not_null(pthread_rwlockattr_init);
		/* zassert_not_null(pthread_rwlockattr_setpshared); */ /* not implemented */
		zassert_not_null(pthread_self);
		/* zassert_not_null(pthread_setcancelstate); */ /* not implemented */
		/* zassert_not_null(pthread_setcanceltype); */ /* not implemented */
		/* zassert_not_null(pthread_setconcurrency); */ /* not implemented */
		zassert_not_null(pthread_setschedparam);
		/* zassert_not_null(pthread_setschedprio); */ /* not implemented */
		zassert_not_null(pthread_setspecific);
		/* zassert_not_null(pthread_spin_destroy); */ /* not implemented */
		/* zassert_not_null(pthread_spin_init); */ /* not implemented */
		/* zassert_not_null(pthread_spin_lock); */ /* not implemented */
		/* zassert_not_null(pthread_spin_unlock); */ /* not implemented */
		/* zassert_not_null(pthread_testcancel); */ /* not implemented */
	}
}
#pragma GCC diagnostic pop
