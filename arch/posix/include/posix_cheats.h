/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/*
 * Header to be able to compile the Zephyr kernel on top of a POSIX OS
 */

#if !defined(ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CHEATS_H_) && !defined(NO_POSIX_CHEATS)
#define ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CHEATS_H_

#ifdef CONFIG_ARCH_POSIX

#ifndef main
#define main(...) zephyr_app_main(__VA_ARGS__)
#endif

/* For the include/posix/pthreads.h provided with Zephyr,
 * in case somebody would use it, we rename all symbols here adding
 * some prefix, and we ensure this header is included
 */

#ifdef CONFIG_PTHREAD_IPC

#define timespec zap_timespec
#define timeval  zap_timeval
#define pthread_mutex_t zap_pthread_mutex_t
#define pthread_mutexattr_t    zap_pthread_mutexattr_t
#define pthread_cond_t         zap_pthread_cond_t
#define pthread_condattr_t     zap_pthread_condattr_t
#define pthread_barrier_t      zap_pthread_barrier_t
#define pthread_barrierattr_t  zap_pthread_barrierattr_t
#define pthread_attr_t         zap_pthread_attr_t
#define clockid_t              zap_clockid_t
#define sched_param	       zap_sched_param
#define itimerspe	       zap_sched_param
#define timer_t                zap_timer_t
#define sigval		       zap_sigval
#define sigevent	       zap_sigevent
#define pthread_rwlock_obj     zap_pthread_rwlock_obj
#define pthread_rwlockattr_t   zap_pthread_rwlockattr_t
#define mqueue_object	       zap_mqueue_object
#define mqueue_desc	       zap_mqueue_desc
#define mqd_t		       zap_mqd_t
#define mq_attr		       zap_mq_attr
#define dirent		       zap_dirent
#define DIR		       zap_DIR
#define pthread_once_t	       zap_pthread_once_t
#define pthread_key_t	       zap_pthread_key_t

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

#endif /* CONFIG_PTHREAD_IPC */

#endif /* CONFIG_ARCH_POSIX */

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CHEATS_H_ */
