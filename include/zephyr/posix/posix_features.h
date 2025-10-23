/*
 * Copyright (c) 2024 BayLibre SAS
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_POSIX_POSIX_FEATURES_H_
#define INCLUDE_ZEPHYR_POSIX_POSIX_FEATURES_H_

/*
 * Note: this header is included (currently) via
 * `-imacros ${ZEPHYR_BASE}/include/zephyr/posix/posix_features.h`.
 *
 * It is not appropriate to place anything other than "symbolic constants" in this file. In this
 * context, "symbolic constants" refers to those listed in the section "Constants for Options and
 * Option Groups", in the POSIX specification.
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 *
 * If there is a better alternative, to ensure that the contents of this file are available to
 * every compilation unit, for arbitrary C compilers, C libraries, toolchains, and so on, it is
 * to be investigated as part of the issue below.
 *
 * https://github.com/zephyrproject-rtos/zephyr/issues/98525
 */
#if defined(_POSIX_C_SOURCE) && defined(CONFIG_POSIX_SYSTEM_INTERFACES)

/*
 * POSIX Application Environment Profiles (AEP - IEEE Std 1003.13-2003)
 */

#undef _POSIX_AEP_REALTIME_MINIMAL
#ifdef CONFIG_POSIX_AEP_REALTIME_MINIMAL
#define _POSIX_AEP_REALTIME_MINIMAL 200312L
#endif

#undef _POSIX_AEP_REALTIME_CONTROLLER
#ifdef CONFIG_POSIX_AEP_REALTIME_CONTROLLER
#define _POSIX_AEP_REALTIME_CONTROLLER 200312L
#endif

#undef _POSIX_AEP_REALTIME_DEDICATED
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

#undef _POSIX_CHOWN_RESTRICTED
#define _POSIX_CHOWN_RESTRICTED (0)
#undef _POSIX_NO_TRUNC
#define _POSIX_NO_TRUNC (0)
#undef _POSIX_VDISABLE
#define _POSIX_VDISABLE ('\0')

/* #define _POSIX_ADVISORY_INFO (-1L) */

#undef _POSIX_ASYNCHRONOUS_IO
#ifdef CONFIG_POSIX_ASYNCHRONOUS_IO
#define _POSIX_ASYNCHRONOUS_IO _POSIX_VERSION
#endif

#undef _POSIX_BARRIERS
#ifdef CONFIG_POSIX_BARRIERS
#define _POSIX_BARRIERS _POSIX_VERSION
#endif

#undef _POSIX_CLOCK_SELECTION
#ifdef CONFIG_POSIX_CLOCK_SELECTION
#define _POSIX_CLOCK_SELECTION _POSIX_VERSION
#endif

#undef _POSIX_CPUTIME
#ifdef CONFIG_POSIX_CPUTIME
#define _POSIX_CPUTIME _POSIX_VERSION
#endif

#undef _POSIX_FSYNC
#ifdef CONFIG_POSIX_FSYNC
#define _POSIX_FSYNC _POSIX_VERSION
#endif

#undef _POSIX_IPV6
#ifdef CONFIG_POSIX_IPV6
#define _POSIX_IPV6 _POSIX_VERSION
#endif

/* #define _POSIX_JOB_CONTROL (-1L) */

#undef _POSIX_MAPPED_FILES
#ifdef CONFIG_POSIX_MAPPED_FILES
#define _POSIX_MAPPED_FILES _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMLOCK
#define _POSIX_MEMLOCK _POSIX_VERSION
#endif

#undef _POSIX_MEMLOCK_RANGE
#ifdef CONFIG_POSIX_MEMLOCK_RANGE
#define _POSIX_MEMLOCK_RANGE _POSIX_VERSION
#endif

#undef _POSIX_MEMORY_PROTECTION
#ifdef CONFIG_POSIX_MEMORY_PROTECTION
#define _POSIX_MEMORY_PROTECTION _POSIX_VERSION
#endif

#undef _POSIX_MESSAGE_PASSING
#ifdef CONFIG_POSIX_MESSAGE_PASSING
#define _POSIX_MESSAGE_PASSING _POSIX_VERSION
#endif

#undef _POSIX_MONOTONIC_CLOCK
#ifdef CONFIG_POSIX_MONOTONIC_CLOCK
#define _POSIX_MONOTONIC_CLOCK _POSIX_VERSION
#endif

/* #define _POSIX_PRIORITIZED_IO (-1L) */

#undef _POSIX_PRIORITY_SCHEDULING
#ifdef CONFIG_POSIX_PRIORITY_SCHEDULING
#define _POSIX_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

#undef _POSIX_RAW_SOCKETS
#ifdef CONFIG_POSIX_RAW_SOCKETS
#define _POSIX_RAW_SOCKETS _POSIX_VERSION
#endif

#undef _POSIX_READER_WRITER_LOCKS
#ifdef CONFIG_POSIX_RW_LOCKS
#define _POSIX_READER_WRITER_LOCKS _POSIX_VERSION
#endif

#undef _POSIX_REALTIME_SIGNALS
#ifdef CONFIG_POSIX_REALTIME_SIGNALS
#define _POSIX_REALTIME_SIGNALS _POSIX_VERSION
#endif

/* #define _POSIX_REGEXP (-1L) */
/* #define _POSIX_SAVED_IDS (-1L) */

#undef _POSIX_SEMAPHORES
#ifdef CONFIG_POSIX_SEMAPHORES
#define _POSIX_SEMAPHORES _POSIX_VERSION
#endif

#undef _POSIX_SHARED_MEMORY_OBJECTS
#ifdef CONFIG_POSIX_SHARED_MEMORY_OBJECTS
#define _POSIX_SHARED_MEMORY_OBJECTS _POSIX_VERSION
#endif

/* #define _POSIX_SHELL (-1L) */
/* #define _POSIX_SPAWN (-1L) */

#undef _POSIX_SPIN_LOCKS
#ifdef CONFIG_POSIX_SPIN_LOCKS
#define _POSIX_SPIN_LOCKS _POSIX_VERSION
#endif

/* #define _POSIX_SPORADIC_SERVER (-1L) */

#undef _POSIX_SYNCHRONIZED_IO
#ifdef CONFIG_POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO _POSIX_VERSION
#endif

#undef _POSIX_THREADS
#undef _POSIX_THREAD_PROCESS_SHARED
#undef _POSIX_PRIORITY_SCHEDULING
#undef _POSIX_THREAD_PRIORITY_SCHEDULING
#ifdef CONFIG_POSIX_THREADS
#define _POSIX_THREADS                    _POSIX_VERSION
#define _POSIX_THREAD_PROCESS_SHARED      _POSIX_VERSION
#define _POSIX_PRIORITY_SCHEDULING        _POSIX_VERSION
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

#undef _POSIX_THREAD_ATTR_STACKADDR
#ifdef CONFIG_POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_ATTR_STACKADDR _POSIX_VERSION
#endif

#undef _POSIX_THREAD_ATTR_STACKSIZE
#ifdef CONFIG_POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_ATTR_STACKSIZE _POSIX_VERSION
#endif

#undef _POSIX_THREAD_CPUTIME
#ifdef CONFIG_POSIX_THREAD_CPUTIME
#define _POSIX_THREAD_CPUTIME _POSIX_VERSION
#endif

#undef _POSIX_THREAD_PRIO_INHERIT
#ifdef CONFIG_POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_INHERIT _POSIX_VERSION
#endif

#undef _POSIX_THREAD_PRIO_PROTECT
#ifdef CONFIG_POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PRIO_PROTECT _POSIX_VERSION
#endif

#undef _POSIX_THREAD_PRIORITY_SCHEDULING
#ifdef CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

/* #define _POSIX_THREAD_PROCESS_SHARED (-1L) */
/* #define _POSIX_THREAD_ROBUST_PRIO_INHERIT (-1L) */
/* #define _POSIX_THREAD_ROBUST_PRIO_PROTECT (-1L) */

#undef _POSIX_THREAD_SAFE_FUNCTIONS
#if defined(CONFIG_POSIX_FILE_SYSTEM_R) && defined(CONFIG_POSIX_C_LANG_SUPPORT_R)
#define _POSIX_THREAD_SAFE_FUNCTIONS _POSIX_VERSION
#endif

/* #define _POSIX_THREAD_SPORADIC_SERVER (-1L) */

#undef _POSIX_THREADS
#undef _POSIX_THREAD_PROCESS_SHARED
#undef _POSIX_PRIORITY_SCHEDULING
#undef _POSIX_THREAD_PRIORITY_SCHEDULING
#ifdef CONFIG_POSIX_THREADS
#define _POSIX_THREADS                    _POSIX_VERSION
#define _POSIX_THREAD_PROCESS_SHARED      _POSIX_VERSION
#define _POSIX_PRIORITY_SCHEDULING        _POSIX_VERSION
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

#undef _POSIX_TIMEOUTS
#ifdef CONFIG_POSIX_TIMEOUTS
#define _POSIX_TIMEOUTS _POSIX_VERSION
#endif

#undef _POSIX_CPUTIME
#undef _POSIX_MONOTONIC_CLOCK
#undef _POSIX_TIMEOUTS
#undef _POSIX_TIMERS
#ifdef CONFIG_POSIX_TIMERS
#define _POSIX_TIMERS          _POSIX_VERSION
#define _POSIX_TIMEOUTS        _POSIX_VERSION
/*
 * FIXME: Until we have a Kconfig for XSI_ADVANCED_REALTIME, define _POSIX_CPUTIME and
 * _POSIX_MONOTONIC_CLOCK with _POSIX_TIMERS.
 * For more information on the Advanced Realtime Option Group, please see
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap02.html
 */
#define _POSIX_CPUTIME         _POSIX_VERSION
#define _POSIX_MONOTONIC_CLOCK _POSIX_VERSION
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
#undef _POSIX2_C_BIND
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

#undef _XOPEN_VERSION
#ifdef CONFIG_XSI
#define _XOPEN_VERSION 700
#endif

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

#undef _XOPEN_STREAMS
#ifdef CONFIG_XSI_STREAMS
#define _XOPEN_STREAMS _XOPEN_VERSION
#endif

#undef _XOPEN_UNIX
#ifdef CONFIG_XSI
#define _XOPEN_UNIX _XOPEN_VERSION
#endif

/* #define _XOPEN_UUCP (-1L) */

#endif /* defined(_POSIX_C_SOURCE) || defined(CONFIG_POSIX_SYSTEM_INTERFACES) */

#endif /* INCLUDE_ZEPHYR_POSIX_POSIX_FEATURES_H_ */
