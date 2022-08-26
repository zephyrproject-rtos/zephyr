/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Header to be able to compile the Zephyr kernel on top of a POSIX OS via the
 * POSIX ARCH
 *
 * This file is only used in the POSIX ARCH, and not in any other architecture
 *
 * Most users will be normally unaware of this file existence, unless they have
 * a link issue in which their POSIX functions calls are reported in errors (as
 * zap_<original_func_name>).
 * If you do see a link error telling you that zap_something is undefined, it is
 * likely that you forgot to select the corresponding Zephyr POSIX API.
 *
 * This header is included automatically when targeting POSIX ARCH boards
 * (for ex. native_posix).
 * It will be included in _all_ Zephyr and application source files
 * (it is passed with the option "-include" to the compiler call)
 *
 * A few files (those which need to access the host OS APIs) will set
 * NO_POSIX_CHEATS to avoid including this file. These are typically only
 * the POSIX arch private files and some of the drivers meant only for the POSIX
 * architecture.
 * No file which is meant to run in an embedded target should set
 * NO_POSIX_CHEATS
 */

#if !defined(ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CHEATS_H_) && !defined(NO_POSIX_CHEATS)
#define ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CHEATS_H_

/*
 * Normally main() is the main entry point of a C executable.
 * When compiling for native_posix, the Zephyr "application" is not the actual
 * entry point of the executable but something the Zephyr OS calls during
 * boot.
 * Therefore we need to rename this application main something else, so
 * we free the function name "main" for its normal purpose
 */
#ifndef main
#define main(...) _posix_zephyr_main(__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* To be able to define main() in C++ code we need to have its prototype
 * defined somewhere visibly. Otherwise name mangling will prevent the linker
 * from finding it. Zephyr assumes a void main(void) prototype and therefore
 * this will be the prototype after renaming:
 */
void _posix_zephyr_main(void);

#ifdef __cplusplus
}
#endif

#ifdef CONFIG_POSIX_API

/*
 * The defines below in this header exist only to enable the Zephyr POSIX API
 * (include/posix/), and applications using it, to be compiled on top of
 * native_posix.
 *
 * Without this header, both the Zephyr POSIX API functions and the equivalent
 * host OS functions would have the same name. This would result in the linker
 * not picking the correct ones.
 *
 * Renaming these functions allows the linker to distinguish
 * which calls are meant for the Zephyr POSIX API (zap_something), and
 * which are meant for the host OS.
 *
 * The zap_ prefix should be understood as an attempt to namespace them
 * into something which is unlikely to collide with other real functions
 * (Any unlikely string would have done)
 *
 * If you want to link an external library together with Zephyr code for the
 * native_posix target, where that external library calls into the Zephyr
 * POSIX API, you may want to include this header when compiling that library,
 * or rename the calls to match the ones in the defines below.
 */

/* Condition variables */
#define pthread_cond_init(...)        zap_pthread_cond_init(__VA_ARGS__)
#define pthread_cond_destroy(...)     zap_pthread_cond_destroy(__VA_ARGS__)
#define pthread_cond_signal(...)      zap_pthread_cond_signal(__VA_ARGS__)
#define pthread_cond_broadcast(...)   zap_pthread_cond_broadcast(__VA_ARGS__)
#define pthread_cond_wait(...)        zap_pthread_cond_wait(__VA_ARGS__)
#define pthread_cond_timedwait(...)   zap_pthread_cond_timedwait(__VA_ARGS__)
#define pthread_condattr_init(...)    zap_pthread_condattr_init(__VA_ARGS__)
#define pthread_condattr_destroy(...) zap_pthread_condattr_destroy(__VA_ARGS__)

/* Semaphore */
#define sem_destroy(...)	      zap_sem_destroy(__VA_ARGS__)
#define sem_getvalue(...)	      zap_sem_getvalue(__VA_ARGS__)
#define sem_init(...)		      zap_sem_init(__VA_ARGS__)
#define sem_post(...)		      zap_sem_post(__VA_ARGS__)
#define sem_timedwait(...)	      zap_sem_timedwait(__VA_ARGS__)
#define sem_trywait(...)	      zap_sem_trywait(__VA_ARGS__)
#define sem_wait(...)		      zap_sem_wait(__VA_ARGS__)

/* Mutex */
#define pthread_mutex_init(...)       zap_pthread_mutex_init(__VA_ARGS__)
#define pthread_mutex_destroy(...)    zap_pthread_mutex_destroy(__VA_ARGS__)
#define pthread_mutex_lock(...)       zap_pthread_mutex_lock(__VA_ARGS__)
#define pthread_mutex_timedlock(...)  zap_pthread_mutex_timedlock(__VA_ARGS__)
#define pthread_mutex_trylock(...)    zap_pthread_mutex_trylock(__VA_ARGS__)
#define pthread_mutex_unlock(...)     zap_pthread_mutex_unlock(__VA_ARGS__)
#define pthread_mutexattr_init(...)   zap_pthread_mutexattr_init(__VA_ARGS__)
#define pthread_mutexattr_destroy(...) \
		zap_pthread_mutexattr_destroy(__VA_ARGS__)
/* Barrier */
#define pthread_barrier_wait(...)     zap_pthread_barrier_wait(__VA_ARGS__)
#define pthread_barrier_init(...)     zap_pthread_barrier_init(__VA_ARGS__)
#define pthread_barrier_destroy(...)  zap_pthread_barrier_destroy(__VA_ARGS__)
#define pthread_barrierattr_init(...) zap_pthread_barrierattr_init(__VA_ARGS__)
#define pthread_barrierattr_destroy(...) \
	zap_pthread_barrierattr_destroy(__VA_ARGS__)

/* Pthread */
#define pthread_attr_init(...)		zap_pthread_attr_init(__VA_ARGS__)
#define pthread_attr_destroy(...)	zap_pthread_attr_destroy(__VA_ARGS__)
#define pthread_attr_getschedparam(...) \
			zap_pthread_attr_getschedparam(__VA_ARGS__)
#define pthread_attr_getstack(...)	zap_pthread_attr_getstack(__VA_ARGS__)
#define pthread_attr_getstacksize(...)	\
			zap_pthread_attr_getstacksize(__VA_ARGS__)
#define pthread_equal(...)		zap_pthread_equal(__VA_ARGS__)
#define pthread_self(...)		zap_pthread_self(__VA_ARGS__)
#define pthread_getschedparam(...)	zap_pthread_getschedparam(__VA_ARGS__)
#define pthread_once(...)		zap_pthread_once(__VA_ARGS__)
#define pthread_exit(...)		zap_pthread_exit(__VA_ARGS__)
#define pthread_join(...)		zap_pthread_join(__VA_ARGS__)
#define pthread_detach(...)		zap_pthread_detach(__VA_ARGS__)
#define pthread_cancel(...)		zap_pthread_cancel(__VA_ARGS__)
#define pthread_attr_getdetachstate(...)	\
		zap_pthread_attr_getdetachstate(__VA_ARGS__)
#define pthread_attr_setdetachstate(...)	\
		zap_pthread_attr_setdetachstate(__VA_ARGS__)
#define pthread_attr_setschedparam(...)	\
		zap_pthread_attr_setschedparam(__VA_ARGS__)
#define pthread_attr_setschedpolicy(...)\
		zap_pthread_attr_setschedpolicy(__VA_ARGS__)
#define pthread_attr_getschedpolicy(...)\
		zap_pthread_attr_getschedpolicy(__VA_ARGS__)

#define pthread_attr_setstack(...)	zap_pthread_attr_setstack(__VA_ARGS__)
#define pthread_create(...)		zap_pthread_create(__VA_ARGS__)
#define pthread_setcancelstate(...)	zap_pthread_setcancelstate(__VA_ARGS__)
#define pthread_setschedparam(...)	zap_pthread_setschedparam(__VA_ARGS__)

/* Scheduler */
#define sched_yield(...)		zap_sched_yield(__VA_ARGS__)
#define sched_get_priority_min(...)	zap_sched_get_priority_min(__VA_ARGS__)
#define sched_get_priority_max(...)	zap_sched_get_priority_max(__VA_ARGS__)

/* Sleep */
#define sleep(...)			zap_sleep(__VA_ARGS__)
#define usleep(...)			zap_usleep(__VA_ARGS__)

/* Clock */
#define clock_gettime(...)		zap_clock_gettime(__VA_ARGS__)
#define clock_settime(...)		zap_clock_settime(__VA_ARGS__)
#define gettimeofday(...)		zap_clock_gettimeofday(__VA_ARGS__)

/* Timer */
#define timer_create(...)	zap_timer_create(__VA_ARGS__)
#define timer_delete(...)	zap_timer_delete(__VA_ARGS__)
#define timer_gettime(...)	zap_timer_gettime(__VA_ARGS__)
#define timer_settime(...)	zap_timer_settime(__VA_ARGS__)

/* Read/Write lock */
#define pthread_rwlock_destroy(...)	zap_pthread_rwlock_destroy(__VA_ARGS__)
#define pthread_rwlock_init(...)	zap_pthread_rwlock_init(__VA_ARGS__)
#define pthread_rwlock_rdlock(...)	zap_pthread_rwlock_rdlock(__VA_ARGS__)
#define pthread_rwlock_unlock(...)	zap_pthread_rwlock_unlock(__VA_ARGS__)
#define pthread_rwlock_wrlock(...)	zap_pthread_rwlock_wrlock(__VA_ARGS__)
#define pthread_rwlockattr_init(...)	zap_pthread_rwlockattr_init(__VA_ARGS__)
#define pthread_rwlock_timedrdlock(...)\
		zap_pthread_rwlock_timedrdlock(__VA_ARGS__)
#define pthread_rwlock_timedwrlock(...)\
		zap_pthread_rwlock_timedwrlock(__VA_ARGS__)
#define pthread_rwlock_tryrdlock(...)\
		zap_pthread_rwlock_tryrdlock(__VA_ARGS__)
#define pthread_rwlock_trywrlock(...)\
		zap_pthread_rwlock_trywrlock(__VA_ARGS__)
#define pthread_rwlockattr_destroy(...)\
		zap_pthread_rwlockattr_destroy(__VA_ARGS__)

/* Pthread key */
#define pthread_key_create(...)		zap_pthread_key_create(__VA_ARGS__)
#define pthread_key_delete(...)		zap_pthread_key_delete(__VA_ARGS__)
#define pthread_setspecific(...)	zap_pthread_setspecific(__VA_ARGS__)
#define pthread_getspecific(...)	zap_pthread_getspecific(__VA_ARGS__)

/* message queue */
#define mq_open(...)	zap_mq_open(__VA_ARGS__)
#define mq_close(...)	zap_mq_close(__VA_ARGS__)
#define mq_unlink(...)	zap_mq_unlink(__VA_ARGS__)
#define mq_getattr(...)	zap_mq_getattr(__VA_ARGS__)
#define mq_receive(...)	zap_mq_receive(__VA_ARGS__)
#define mq_send(...)	zap_mq_send(__VA_ARGS__)
#define mq_setattr(...)	zap_mq_setattr(__VA_ARGS__)
#define mq_timedreceive(...)	zap_mq_timedreceive(__VA_ARGS__)
#define mq_timedsend(...)	zap_mq_timedsend(__VA_ARGS__)

/* File system */
#define open		zap_open
#define close		zap_close
#define write		zap_write
#define read		zap_read
#define lseek		zap_lseek
#define opendir		zap_opendir
#define closedir	zap_closedir
#define readdir		zap_readdir
#define rename		zap_rename
#define unlink		zap_unlink
#define stat		zap_stat
#define mkdir		zap_mkdir

/* eventfd */
#define eventfd		zap_eventfd
#define eventfd_read	zap_eventfd_read
#define eventfd_write	zap_eventfd_write

#endif /* CONFIG_POSIX_API */

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CHEATS_H_ */
