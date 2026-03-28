/*
 * Copyright (c) 2024 BayLibre SAS
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_POSIX_POSIX_FEATURES_H_
#define INCLUDE_ZEPHYR_POSIX_POSIX_FEATURES_H_

#include <zephyr/autoconf.h>       /* CONFIG_* */
#include <zephyr/sys/util_macro.h> /* COND_CODE_1() */

/*
 * POSIX Application Environment Profiles (AEP - IEEE Std 1003.13-2003)
 */

#ifdef CONFIG_POSIX_AEP_REALTIME_MINIMAL
#define _POSIX_AEP_REALTIME_MINIMAL 200312L
#endif

#ifdef CONFIG_POSIX_AEP_REALTIME_CONTROLLER
#define _POSIX_AEP_REALTIME_CONTROLLER 200312L
#endif

#ifdef CONFIG_POSIX_AEP_REALTIME_DEDICATED
#define _POSIX_AEP_REALTIME_DEDICATED 200312L
#endif

/*
 * Subprofiling Considerations
 */
#define _POSIX_SUBPROFILE 1

/*
 * POSIX System Interfaces
 */

#define _POSIX_VERSION 200809L

#define _POSIX_CHOWN_RESTRICTED (0)
#define _POSIX_NO_TRUNC         (0)
#define _POSIX_VDISABLE         ('\0')

/* #define _POSIX_ADVISORY_INFO (-1L) */

#ifdef CONFIG_POSIX_ASYNCHRONOUS_IO
#define _POSIX_ASYNCHRONOUS_IO _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_BARRIERS
#define _POSIX_BARRIERS _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_FSYNC
#define _POSIX_FSYNC _POSIX_VERSION
#endif

#ifdef CONFIG_NET_IPV6
#define _POSIX_IPV6 _POSIX_VERSION
#endif

/* #define _POSIX_JOB_CONTROL (-1L) */

#ifdef CONFIG_POSIX_MAPPED_FILES
#define _POSIX_MAPPED_FILES _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMLOCK
#define _POSIX_MEMLOCK _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMLOCK_RANGE
#define _POSIX_MEMLOCK_RANGE _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMORY_PROTECTION
#define _POSIX_MEMORY_PROTECTION _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MESSAGE_PASSING
#define _POSIX_MESSAGE_PASSING _POSIX_VERSION
#endif

/* #define _POSIX_PRIORITIZED_IO (-1L) */

#ifdef CONFIG_POSIX_PRIORITY_SCHEDULING
#define _POSIX_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

#ifdef CONFIG_NET_SOCKETS_PACKET
#define _POSIX_RAW_SOCKETS _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_RW_LOCKS
#define _POSIX_READER_WRITER_LOCKS _POSIX_VERSION
#endif

/* #define _POSIX_REGEXP (-1L) */
/* #define _POSIX_SAVED_IDS (-1L) */

#ifdef CONFIG_POSIX_SEMAPHORES
#define _POSIX_SEMAPHORES _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_SHARED_MEMORY_OBJECTS
#define _POSIX_SHARED_MEMORY_OBJECTS _POSIX_VERSION
#endif

/* #define _POSIX_SHELL (-1L) */
/* #define _POSIX_SPAWN (-1L) */

#ifdef CONFIG_POSIX_SPIN_LOCKS
#define _POSIX_SPIN_LOCKS _POSIX_VERSION
#endif

/* #define _POSIX_SPORADIC_SERVER (-1L) */

#ifdef CONFIG_POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_ATTR_STACKADDR _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_ATTR_STACKSIZE _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_CPUTIME
#define _POSIX_THREAD_CPUTIME _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_INHERIT _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PRIO_PROTECT _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

/* #define _POSIX_THREAD_PROCESS_SHARED (-1L) */
/* #define _POSIX_THREAD_ROBUST_PRIO_INHERIT (-1L) */
/* #define _POSIX_THREAD_ROBUST_PRIO_PROTECT (-1L) */

/* #define _POSIX_THREAD_SPORADIC_SERVER (-1L) */

#ifdef CONFIG_POSIX_THREADS
#ifndef _POSIX_THREADS
#define _POSIX_THREADS _POSIX_VERSION
#endif
#endif

#ifdef CONFIG_POSIX_TIMEOUTS
#define _POSIX_TIMEOUTS _POSIX_VERSION
#endif

/* #define _POSIX_TRACE (-1L) */
/* #define _POSIX_TRACE_EVENT_FILTER (-1L) */
/* #define _POSIX_TRACE_INHERIT (-1L) */
/* #define _POSIX_TRACE_LOG (-1L) */
/* #define _POSIX_TYPED_MEMORY_OBJECTS (-1L) */

/*
 * POSIX v6 Options
 */
/* #define _POSIX_V6_ILP32_OFF32 (-1L) */
/* #define _POSIX_V6_ILP32_OFFBIG (-1L) */
/* #define _POSIX_V6_LP64_OFF64 (-1L) */
/* #define _POSIX_V6_LPBIG_OFFBIG (-1L) */

/*
 * POSIX v7 Options
 */
/* #define _POSIX_V7_ILP32_OFF32 (-1L) */
/* #define _POSIX_V7_ILP32_OFFBIG (-1L) */
/* #define _POSIX_V7_LP64_OFF64 (-1L) */
/* #define _POSIX_V7_LPBIG_OFFBIG (-1L) */

/*
 * POSIX2 Options
 */
/* #define _POSIX2_VERSION (-1) */
#define _POSIX2_C_BIND _POSIX_VERSION
/* #define _POSIX2_C_DEV (-1) */
/* #define _POSIX2_CHAR_TERM (-1L) */
/* #define _POSIX2_FORT_DEV (-1L) */
/* #define _POSIX2_FORT_RUN (-1L) */
/* #define _POSIX2_LOCALEDEF (-1L) */
/* #define _POSIX2_PBS (-1L) */
/* #define _POSIX2_PBS_ACCOUNTING (-1L) */
/* #define _POSIX2_PBS_CHECKPOINT (-1L) */
/* #define _POSIX2_PBS_LOCATE (-1L) */
/* #define _POSIX2_PBS_MESSAGE (-1L) */
/* #define _POSIX2_PBS_TRACK (-1L) */
/* #define _POSIX2_SW_DEV (-1L) */
/* #define _POSIX2_UPE (-1L) */

/*
 * X/Open System Interfaces
 */
#define _XOPEN_VERSION 700
/* #define _XOPEN_CRYPT (-1L) */
/* #define _XOPEN_ENH_I18N (-1L) */
#if defined(CONFIG_XSI_REALTIME) ||                                                                \
	(defined(CONFIG_POSIX_FSYNC) && defined(CONFIG_POSIX_MEMLOCK) &&                           \
	 defined(CONFIG_POSIX_MEMLOCK_RANGE) && defined(CONFIG_POSIX_MESSAGE_PASSING) &&           \
	 defined(CONFIG_POSIX_PRIORITY_SCHEDULING) &&                                              \
	 defined(CONFIG_POSIX_SHARED_MEMORY_OBJECTS) && defined(CONFIG_POSIX_SYNCHRONIZED_IO))
#define _XOPEN_REALTIME _XOPEN_VERSION
#endif
/* #define _XOPEN_REALTIME_THREADS (-1L) */
/* #define _XOPEN_SHM (-1L) */

#ifdef CONFIG_XOPEN_STREAMS
#define _XOPEN_STREAMS _XOPEN_VERSION
#endif

/* #define _XOPEN_UNIX (-1L) */
/* #define _XOPEN_UUCP (-1L) */

#endif /* INCLUDE_ZEPHYR_POSIX_POSIX_FEATURES_H_ */
