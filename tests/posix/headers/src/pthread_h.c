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

/**
 * @brief existence test for `<pthread.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html">pthread.h</a>
 */
ZTEST(posix_headers, test_pthread_h)
{
#ifdef CONFIG_POSIX_API
	zassert_not_equal(-1, PTHREAD_BARRIER_SERIAL_THREAD);

	zassert_not_equal(-1, PTHREAD_CANCEL_ASYNCHRONOUS);
	zassert_not_equal(-1, PTHREAD_CANCEL_DEFERRED);

	zassert_not_equal(-1, PTHREAD_CANCEL_ENABLE);
	zassert_not_equal(-1, PTHREAD_CANCEL_DISABLE);

	zassert_not_equal((void *)-42, PTHREAD_CANCELED);

	zassert_not_equal(-1, PTHREAD_CREATE_DETACHED);
	zassert_not_equal(-1, PTHREAD_CREATE_JOINABLE);

	zassert_not_equal(-1, PTHREAD_EXPLICIT_SCHED);
	zassert_not_equal(-1, PTHREAD_INHERIT_SCHED);

	zassert_not_equal(-1, PTHREAD_MUTEX_DEFAULT);
	zassert_not_equal(-1, PTHREAD_MUTEX_ERRORCHECK);
	zassert_not_equal(-1, PTHREAD_MUTEX_ERRORCHECK);
	zassert_not_equal(-1, PTHREAD_MUTEX_RECURSIVE);
	/* zassert_not_equal(-1, PTHREAD_MUTEX_ROBUST); */  /* not implemented */
	/* zassert_not_equal(-1, PTHREAD_MUTEX_STALLED); */ /* not implemented */

	__unused pthread_once_t once = PTHREAD_ONCE_INIT;

	/* zassert_not_equal(-1, PTHREAD_PRIO_INHERIT); */ /* not implemented */
	zassert_not_equal(-1, PTHREAD_PRIO_NONE);
	/* zassert_not_equal(-1, PTHREAD_PRIO_PROTECT); */ /* not implemented */

	zassert_not_equal(-1, PTHREAD_PROCESS_SHARED);
	zassert_not_equal(-1, PTHREAD_PROCESS_PRIVATE);

	zassert_not_equal(-1, PTHREAD_SCOPE_PROCESS);
	zassert_not_equal(-1, PTHREAD_SCOPE_SYSTEM);

	__unused pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	__unused pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
	/* pthread_rwlock_t lock = POSIX_READER_WRITER_LOCKS_INITIALIZER; */ /* not implemented */

	zassert_not_null(pthread_atfork);
	zassert_not_null(pthread_attr_destroy);
	zassert_not_null(pthread_attr_getdetachstate);
	zassert_not_null(pthread_attr_getguardsize);
	zassert_not_null(pthread_attr_getinheritsched);
	zassert_not_null(pthread_attr_getschedparam);
	zassert_not_null(pthread_attr_getschedpolicy);
	zassert_not_null(pthread_attr_getscope);
	zassert_not_null(pthread_attr_getstack);
	zassert_not_null(pthread_attr_getstacksize);
	zassert_not_null(pthread_attr_init);
	zassert_not_null(pthread_attr_setdetachstate);
	zassert_not_null(pthread_attr_setguardsize);
	zassert_not_null(pthread_attr_setinheritsched);
	zassert_not_null(pthread_attr_setschedparam);
	zassert_not_null(pthread_attr_setschedpolicy);
	zassert_not_null(pthread_attr_setscope);
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
	zassert_not_null(pthread_condattr_getclock);
	/* zassert_not_null(pthread_condattr_getpshared); */ /* not implemented */
	zassert_not_null(pthread_condattr_init);
	zassert_not_null(pthread_condattr_setclock);
	/* zassert_not_null(pthread_condattr_setpshared); */ /* not implemented */
	zassert_not_null(pthread_create);
	zassert_not_null(pthread_detach);
	zassert_not_null(pthread_equal);
	zassert_not_null(pthread_exit);
	zassert_not_null(pthread_getconcurrency);
	/* zassert_not_null(pthread_getcpuclockid); */ /* not implemented */
	zassert_not_null(pthread_getschedparam);
	zassert_not_null(pthread_getspecific);
	zassert_not_null(pthread_join);
	zassert_not_null(pthread_key_create);
	zassert_not_null(pthread_key_delete);
	/* zassert_not_null(pthread_mutex_consistent); */ /* not implemented */
	zassert_not_null(pthread_mutex_destroy);
	zassert_not_null(pthread_mutex_getprioceiling);
	zassert_not_null(pthread_mutex_init);
	zassert_not_null(pthread_mutex_lock);
	zassert_not_null(pthread_mutex_setprioceiling);
	zassert_not_null(pthread_mutex_timedlock);
	zassert_not_null(pthread_mutex_trylock);
	zassert_not_null(pthread_mutex_unlock);
	zassert_not_null(pthread_mutexattr_destroy);
	zassert_not_null(pthread_mutexattr_getprioceiling);
	zassert_not_null(pthread_mutexattr_getprotocol);
	/* zassert_not_null(pthread_mutexattr_getpshared); */ /* not implemented */
	/* zassert_not_null(pthread_mutexattr_getrobust); */  /* not implemented */
	zassert_not_null(pthread_mutexattr_gettype);
	zassert_not_null(pthread_mutexattr_init);
	zassert_not_null(pthread_mutexattr_setprioceiling);
	zassert_not_null(pthread_mutexattr_setprotocol);
	/* zassert_not_null(pthread_mutexattr_setpshared); */     /* not implemented */
	/* zassert_not_null(pthread_mutexattr_setrobust); */      /* not implemented */
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
	zassert_not_null(pthread_rwlockattr_getpshared);
	zassert_not_null(pthread_rwlockattr_init);
	zassert_not_null(pthread_rwlockattr_setpshared);
	zassert_not_null(pthread_self);
	zassert_not_null(pthread_setcancelstate);
	zassert_not_null(pthread_setcanceltype);
	zassert_not_null(pthread_setconcurrency);
	zassert_not_null(pthread_setschedparam);
	zassert_not_null(pthread_setschedprio);
	zassert_not_null(pthread_setspecific);
	zassert_not_null(pthread_spin_destroy);
	zassert_not_null(pthread_spin_init);
	zassert_not_null(pthread_spin_lock);
	zassert_not_null(pthread_spin_trylock);
	zassert_not_null(pthread_spin_unlock);
	zassert_not_null(pthread_testcancel);
#endif
}
