/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/*
 * Header to be able to compile the Zephyr kernel on top of a POSIX OS
 */

#ifndef _POSIX_CHEATS_H
#define _POSIX_CHEATS_H

#ifdef CONFIG_ARCH_POSIX

#ifndef main
#define main(...) zephyr_app_main(__VA_ARGS__)
#endif

/* For the include/posix/pthreads.h provided with Zephyr,
 * in case somebody would use it, we rename all symbols here adding
 * some prefix, and we ensure this header is included
 */

#define timespec zap_timespec
#define pthread_mutex_t zap_pthread_mutex_t
#define pthread_mutexattr_t    zap_pthread_mutexattr_t
#define pthread_cond_t         zap_pthread_cond_t
#define pthread_condattr_t     zap_pthread_condattr_t
#define pthread_barrier_t      zap_pthread_barrier_t
#define pthread_barrierattr_t  zap_pthread_barrierattr_t

#define pthread_cond_init(...)        zap_pthread_cond_init(__VA_ARGS__)
#define pthread_cond_destroy(...)     zap_pthread_cond_destroy(__VA_ARGS__)
#define pthread_cond_signal(...)      zap_pthread_cond_signal(__VA_ARGS__)
#define pthread_cond_broadcast(...)   zap_pthread_cond_broadcast(__VA_ARGS__)
#define pthread_cond_wait(...)        zap_pthread_cond_wait(__VA_ARGS__)
#define pthread_cond_timedwait(...)   zap_pthread_cond_timedwait(__VA_ARGS__)
#define pthread_condattr_init(...)    zap_pthread_condattr_init(__VA_ARGS__)
#define pthread_condattr_destroy(...) zap_pthread_condattr_destroy(__VA_ARGS__)
#define pthread_mutex_init(...)       zap_pthread_mutex_init(__VA_ARGS__)
#define pthread_mutex_destroy(...)    zap_pthread_mutex_destroy(__VA_ARGS__)
#define pthread_mutex_lock(...)       zap_pthread_mutex_lock(__VA_ARGS__)
#define pthread_mutex_timedlock(...)  zap_pthread_mutex_timedlock(__VA_ARGS__)
#define pthread_mutex_trylock(...)    zap_pthread_mutex_trylock(__VA_ARGS__)
#define pthread_mutex_unlock(...)     zap_pthread_mutex_unlock(__VA_ARGS__)
#define pthread_mutexattr_init(...)   zap_pthread_mutexattr_init(__VA_ARGS__)
#define pthread_mutexattr_destroy(...) \
		zap_pthread_mutexattr_destroy(__VA_ARGS__)
#define pthread_barrier_wait(...)     zap_pthread_barrier_wait(__VA_ARGS__)
#define pthread_barrier_init(...)     zap_pthread_barrier_init(__VA_ARGS__)
#define pthread_barrier_destroy(...)  zap_pthread_barrier_destroy(__VA_ARGS__)
#define pthread_barrierattr_init(...) zap_pthread_barrierattr_init(__VA_ARGS__)
#define pthread_barrierattr_destroy(...) \
	zap_pthread_barrierattr_destroy(__VA_ARGS__)

#endif /* CONFIG_ARCH_POSIX */

#endif
