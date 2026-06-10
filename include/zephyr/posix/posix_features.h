/*
 * Copyright (c) 2024 BayLibre SAS
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Standard symbolic constants and types.
 * @ingroup posix
 *
 * Defines the @c _POSIX_* and @c _XOPEN_* symbolic constants for options and
 * option groups that indicate which POSIX features the implementation
 * supports.
 *
 * @posix_header{unistd.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_FEATURES_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_FEATURES_H_

#include <zephyr/autoconf.h>       /* CONFIG_* */
#include <zephyr/sys/util_macro.h> /* COND_CODE_1() */

/*
 * POSIX Application Environment Profiles (AEP - IEEE Std 1003.13-2003)
 */

#ifdef CONFIG_POSIX_AEP_REALTIME_MINIMAL
/**
 * @brief The implementation supports the Minimal Realtime System Profile (PSE51).
 */
#define _POSIX_AEP_REALTIME_MINIMAL 200312L
#endif

#ifdef CONFIG_POSIX_AEP_REALTIME_CONTROLLER
/**
 * @brief The implementation supports the Realtime Controller System Profile (PSE52).
 */
#define _POSIX_AEP_REALTIME_CONTROLLER 200312L
#endif

#ifdef CONFIG_POSIX_AEP_REALTIME_DEDICATED
/**
 * @brief The implementation supports the Dedicated Realtime System Profile (PSE53).
 */
#define _POSIX_AEP_REALTIME_DEDICATED 200312L
#endif

/*
 * Subprofiling Considerations
 */

/**
 * @brief The implementation is a subprofile and need not provide all POSIX interfaces.
 */
#define _POSIX_SUBPROFILE 1

/*
 * POSIX System Interfaces
 */

/**
 * @brief The version of POSIX.1 to which the implementation conforms (POSIX.1-2008).
 */
#define _POSIX_VERSION 200809L

/**
 * @brief Use of chown() is restricted to a process with appropriate privileges.
 */
#define _POSIX_CHOWN_RESTRICTED (0)

/**
 * @brief Pathname components longer than @c NAME_MAX generate an error.
 */
#define _POSIX_NO_TRUNC         (0)

/**
 * @brief Character value that disables terminal special character handling.
 */
#define _POSIX_VDISABLE         ('\0')

/* #define _POSIX_ADVISORY_INFO (-1L) */

#ifdef CONFIG_POSIX_ASYNCHRONOUS_IO
/**
 * @brief The implementation supports asynchronous input and output.
 */
#define _POSIX_ASYNCHRONOUS_IO _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_BARRIERS
/**
 * @brief The implementation supports barriers.
 */
#define _POSIX_BARRIERS _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_FSYNC
/**
 * @brief The implementation supports the File Synchronization option.
 */
#define _POSIX_FSYNC _POSIX_VERSION
#endif

#ifdef CONFIG_NET_IPV6
/**
 * @brief The implementation supports the IPv6 option.
 */
#define _POSIX_IPV6 _POSIX_VERSION
#endif

/* #define _POSIX_JOB_CONTROL (-1L) */

#ifdef CONFIG_POSIX_MAPPED_FILES
/**
 * @brief The implementation supports memory mapped files.
 */
#define _POSIX_MAPPED_FILES _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMLOCK
/**
 * @brief The implementation supports the Process Memory Locking option.
 */
#define _POSIX_MEMLOCK _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMLOCK_RANGE
/**
 * @brief The implementation supports the Range Memory Locking option.
 */
#define _POSIX_MEMLOCK_RANGE _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MEMORY_PROTECTION
/**
 * @brief The implementation supports memory protection.
 */
#define _POSIX_MEMORY_PROTECTION _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_MESSAGE_PASSING
/**
 * @brief The implementation supports the Message Passing option.
 */
#define _POSIX_MESSAGE_PASSING _POSIX_VERSION
#endif

/* #define _POSIX_PRIORITIZED_IO (-1L) */

#ifdef CONFIG_POSIX_PRIORITY_SCHEDULING
/**
 * @brief The implementation supports the Process Scheduling option.
 */
#define _POSIX_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

#ifdef CONFIG_NET_SOCKETS_PACKET
/**
 * @brief The implementation supports the Raw Sockets option.
 */
#define _POSIX_RAW_SOCKETS _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_RW_LOCKS
/**
 * @brief The implementation supports read-write locks.
 */
#define _POSIX_READER_WRITER_LOCKS _POSIX_VERSION
#endif

/* #define _POSIX_REGEXP (-1L) */
/* #define _POSIX_SAVED_IDS (-1L) */

#ifdef CONFIG_POSIX_SEMAPHORES
/**
 * @brief The implementation supports semaphores.
 */
#define _POSIX_SEMAPHORES _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_SHARED_MEMORY_OBJECTS
/**
 * @brief The implementation supports the Shared Memory Objects option.
 */
#define _POSIX_SHARED_MEMORY_OBJECTS _POSIX_VERSION
#endif

/* #define _POSIX_SHELL (-1L) */
/* #define _POSIX_SPAWN (-1L) */

#ifdef CONFIG_POSIX_SPIN_LOCKS
/**
 * @brief The implementation supports spin locks.
 */
#define _POSIX_SPIN_LOCKS _POSIX_VERSION
#endif

/* #define _POSIX_SPORADIC_SERVER (-1L) */

#ifdef CONFIG_POSIX_SYNCHRONIZED_IO
/**
 * @brief The implementation supports the Synchronized Input and Output option.
 */
#define _POSIX_SYNCHRONIZED_IO _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_ATTR_STACKADDR
/**
 * @brief The implementation supports the Thread Stack Address Attribute option.
 */
#define _POSIX_THREAD_ATTR_STACKADDR _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_ATTR_STACKSIZE
/**
 * @brief The implementation supports the Thread Stack Size Attribute option.
 */
#define _POSIX_THREAD_ATTR_STACKSIZE _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_CPUTIME
/**
 * @brief The implementation supports the Thread CPU-Time Clocks option.
 */
#define _POSIX_THREAD_CPUTIME _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_PRIO_INHERIT
/**
 * @brief The implementation supports the Non-Robust Mutex Priority Inheritance option.
 */
#define _POSIX_THREAD_PRIO_INHERIT _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_PRIO_PROTECT
/**
 * @brief The implementation supports the Non-Robust Mutex Priority Protection option.
 */
#define _POSIX_THREAD_PRIO_PROTECT _POSIX_VERSION
#endif

#ifdef CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING
/**
 * @brief The implementation supports the Thread Execution Scheduling option.
 */
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_VERSION
#endif

/* #define _POSIX_THREAD_PROCESS_SHARED (-1L) */
/* #define _POSIX_THREAD_ROBUST_PRIO_INHERIT (-1L) */
/* #define _POSIX_THREAD_ROBUST_PRIO_PROTECT (-1L) */

/* #define _POSIX_THREAD_SPORADIC_SERVER (-1L) */

#ifdef CONFIG_POSIX_THREADS
#ifndef _POSIX_THREADS
/**
 * @brief The implementation supports threads.
 */
#define _POSIX_THREADS _POSIX_VERSION
#endif
#endif

#ifdef CONFIG_POSIX_TIMEOUTS
/**
 * @brief The implementation supports timeouts.
 */
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

/**
 * @brief The implementation supports the C-Language Binding option.
 */
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

/**
 * @brief The version of the X/Open Portability Guide to which the implementation conforms.
 */
#define _XOPEN_VERSION 700
/* #define _XOPEN_CRYPT (-1L) */
/* #define _XOPEN_ENH_I18N (-1L) */
#if defined(CONFIG_XSI_REALTIME) ||                                                                \
	(defined(CONFIG_POSIX_FSYNC) && defined(CONFIG_POSIX_MEMLOCK) &&                           \
	 defined(CONFIG_POSIX_MEMLOCK_RANGE) && defined(CONFIG_POSIX_MESSAGE_PASSING) &&           \
	 defined(CONFIG_POSIX_PRIORITY_SCHEDULING) &&                                              \
	 defined(CONFIG_POSIX_SHARED_MEMORY_OBJECTS) && defined(CONFIG_POSIX_SYNCHRONIZED_IO))

/**
 * @brief The implementation supports the X/Open Realtime Option Group.
 */
#define _XOPEN_REALTIME _XOPEN_VERSION
#endif
/* #define _XOPEN_REALTIME_THREADS (-1L) */
/* #define _XOPEN_SHM (-1L) */

#ifdef CONFIG_XOPEN_STREAMS
/**
 * @brief The implementation supports the XSI STREAMS Option Group.
 */
#define _XOPEN_STREAMS _XOPEN_VERSION
#endif

/* #define _XOPEN_UNIX (-1L) */
/* #define _XOPEN_UUCP (-1L) */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_FEATURES_H_ */
