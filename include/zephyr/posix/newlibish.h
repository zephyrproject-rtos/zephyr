/*
 * Copyright (c) 2022 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NEWLIBISH_H_
#define ZEPHYR_INCLUDE_POSIX_NEWLIBISH_H_

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_PICOLIBC)
#define _SYS__PTHREADTYPES_H_
#define MISSING_SYSCALL_NAMES
#endif

struct pthread_attr_s;
typedef struct pthread_attr_s pthread_attr_t;
typedef void *pthread_t;

#define _POSIX_VERSION				200809L
#define _POSIX2_VERSION				200809L
#define _POSIX_CLOCK_SELECTION                  200809L
#define _POSIX_FSYNC                            200809L
#define _POSIX_MEMLOCK                          200809L
#define _POSIX_MEMLOCK_RANGE                    200809L
#define _POSIX_MONOTONIC_CLOCK                  200809L
#define _POSIX_NO_TRUNC                         200809L
#define _POSIX_REALTIME_SIGNALS                 200809L
#define _POSIX_SEMAPHORES                       200809L
#define _POSIX_SHARED_MEMORY_OBJECTS            200809L
#define _POSIX_SYNCHRONIZED_IO                  200809L
#define _POSIX_THREAD_ATTR_STACKADDR            200809L
#define _POSIX_THREAD_ATTR_STACKSIZE            200809L
#define _POSIX_THREAD_CPUTIME                   200809L
#define _POSIX_THREAD_PRIO_INHERIT              200809L
#define _POSIX_THREAD_PRIO_PROTECT              200809L
#define _POSIX_THREAD_PRIORITY_SCHEDULING       200809L
#define _POSIX_THREAD_SPORADIC_SERVER           200809L
#define _POSIX_THREADS                          200809L
#define _POSIX_TIMEOUTS                         200809L
#define _POSIX_TIMERS                           200809L

#endif /* ZEPHYR_INCLUDE_POSIX_NEWLIBISH_H_ */
