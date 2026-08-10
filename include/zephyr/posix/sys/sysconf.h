/*
 * Copyright (c) 2024, Meta
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POSIX runtime configuration name declarations.
 * @ingroup posix
 *
 * Provides the @c _SC_* name constants accepted by sysconf() and, when
 * @c CONFIG_POSIX_SYSCONF_IMPL_MACRO is enabled, the macro implementation of
 * sysconf() that resolves each variable at compile time.
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_

#include <limits.h>

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief sysconf() name constants.
 */
enum {
	_SC_ADVISORY_INFO,          /**< Advisory information option. */
	_SC_ASYNCHRONOUS_IO,        /**< Asynchronous I/O option. */
	_SC_BARRIERS,               /**< Barriers option. */
	_SC_CLOCK_SELECTION,        /**< Clock Selection option. */
	_SC_CPUTIME,                /**< Process CPU-Time Clocks option. */
	_SC_FSYNC,                  /**< File Synchronization option. */
	_SC_IPV6,                   /**< IPv6 option. */
	_SC_JOB_CONTROL,            /**< Job control. */
	_SC_MAPPED_FILES,           /**< Mapped Files option. */
	_SC_MEMLOCK,                /**< Process Memory Locking option. */
	_SC_MEMLOCK_RANGE,          /**< Range Memory Locking option. */
	_SC_MEMORY_PROTECTION,      /**< Memory Protection option. */
	_SC_MESSAGE_PASSING,        /**< Message Passing option. */
	_SC_MONOTONIC_CLOCK,        /**< Monotonic Clock option. */
	_SC_PRIORITIZED_IO,         /**< Prioritized Input and Output option. */
	_SC_PRIORITY_SCHEDULING,    /**< Process Scheduling option. */
	_SC_RAW_SOCKETS,            /**< Raw Sockets option. */
	_SC_RE_DUP_MAX,             /**< Maximum number of repeated occurrences in BRE. */
	_SC_READER_WRITER_LOCKS,    /**< Read-Write Locks option. */
	_SC_REALTIME_SIGNALS,       /**< Realtime Signals extension. */
	_SC_REGEXP,                 /**< Regular Expressions option. */
	_SC_SAVED_IDS,              /**< Saved IDs option. */
	_SC_SEMAPHORES,             /**< Semaphores option. */
	_SC_SHARED_MEMORY_OBJECTS,  /**< Shared Memory Objects option. */
	_SC_SHELL,                  /**< Shell option. */
	_SC_SPAWN,                  /**< Spawn option. */
	_SC_SPIN_LOCKS,             /**< Spin Locks option. */
	_SC_SPORADIC_SERVER,        /**< Process Sporadic Server option. */
	_SC_SS_REPL_MAX,            /**< Maximum number of replenishments for SS. */
	_SC_SYNCHRONIZED_IO,        /**< Synchronized Input and Output option. */
	_SC_THREAD_ATTR_STACKADDR,  /**< Thread Stack Address Attribute option. */
	_SC_THREAD_ATTR_STACKSIZE,  /**< Thread Stack Size Attribute option. */
	_SC_THREAD_CPUTIME,         /**< Thread CPU-Time Clocks option. */
	_SC_THREAD_PRIO_INHERIT,    /**< Thread Priority Inheritance option. */
	_SC_THREAD_PRIO_PROTECT,    /**< Thread Priority Protection option. */
	_SC_THREAD_PRIORITY_SCHEDULING, /**< Thread Execution Scheduling option. */
	_SC_THREAD_PROCESS_SHARED,  /**< Thread Process-Shared Synchronization option. */
	_SC_THREAD_ROBUST_PRIO_INHERIT, /**< Robust Mutex Priority Inheritance option. */
	_SC_THREAD_ROBUST_PRIO_PROTECT, /**< Robust Mutex Priority Protection option. */
	_SC_THREAD_SAFE_FUNCTIONS,  /**< Thread-Safe Functions option. */
	_SC_THREAD_SPORADIC_SERVER, /**< Thread Sporadic Server option. */
	_SC_THREADS,                /**< Threads option. */
	_SC_TIMEOUTS,               /**< Timeouts option. */
	_SC_TIMERS,                 /**< Timers option. */
	_SC_TRACE,                  /**< Trace option. */
	_SC_TRACE_EVENT_FILTER,     /**< Trace Event Filter option. */
	_SC_TRACE_EVENT_NAME_MAX,   /**< Maximum length of trace event names. */
	_SC_TRACE_INHERIT,          /**< Trace Inherit option. */
	_SC_TRACE_LOG,              /**< Trace Log option. */
	_SC_TRACE_NAME_MAX,         /**< Maximum length of trace names. */
	_SC_TRACE_SYS_MAX,          /**< Maximum number of system trace streams. */
	_SC_TRACE_USER_EVENT_MAX,   /**< Maximum number of user trace event type identifiers. */
	_SC_TYPED_MEMORY_OBJECTS,   /**< Typed Memory Objects option. */
	_SC_VERSION,                /**< POSIX.1 standard version (@c _POSIX_VERSION). */
	_SC_V7_ILP32_OFF32,         /**< ILP32 data model, 32-bit offsets (POSIX.1-2008). */
	_SC_V7_ILP32_OFFBIG,        /**< ILP32 data model, large file offsets (POSIX.1-2008). */
	_SC_V7_LP64_OFF64,          /**< LP64 data model, 64-bit offsets (POSIX.1-2008). */
	_SC_V7_LPBIG_OFFBIG,        /**< LPBIG data model, large offsets (POSIX.1-2008). */
	_SC_V6_ILP32_OFF32,         /**< ILP32 data model, 32-bit offsets (POSIX.1-2001). */
	_SC_V6_ILP32_OFFBIG,        /**< ILP32 data model, large file offsets (POSIX.1-2001). */
	_SC_V6_LP64_OFF64,          /**< LP64 data model, 64-bit offsets (POSIX.1-2001). */
	_SC_V6_LPBIG_OFFBIG,        /**< LPBIG data model, large offsets (POSIX.1-2001). */
	_SC_BC_BASE_MAX,            /**< Maximum value of obase in bc. */
	_SC_BC_DIM_MAX,             /**< Maximum number of elements in bc arrays. */
	_SC_BC_SCALE_MAX,           /**< Maximum value of scale in bc. */
	_SC_BC_STRING_MAX,          /**< Maximum length of string in bc. */
	_SC_2_C_BIND,               /**< C-Language Binding option. */
	_SC_2_C_DEV,                /**< C-Language Development Utilities option. */
	_SC_2_CHAR_TERM,            /**< Terminal Characteristics option. */
	_SC_COLL_WEIGHTS_MAX,       /**< Maximum weights in locale collating. */
	_SC_DELAYTIMER_MAX,         /**< Maximum number of timer overruns. */
	_SC_EXPR_NEST_MAX,          /**< Maximum number of expr()s in expr. */
	_SC_2_FORT_DEV,             /**< FORTRAN Development Utilities option. */
	_SC_2_FORT_RUN,             /**< FORTRAN Runtime Utilities option. */
	_SC_LINE_MAX,               /**< Maximum length of input line in utilities. */
	_SC_2_LOCALEDEF,            /**< Locale Creation option. */
	_SC_2_PBS,                  /**< Batch Environment Services and Utilities option. */
	_SC_2_PBS_ACCOUNTING,       /**< Batch Accounting option. */
	_SC_2_PBS_CHECKPOINT,       /**< Batch Checkpoint/Restart option. */
	_SC_2_PBS_LOCATE,           /**< Locate Batch Job Request option. */
	_SC_2_PBS_MESSAGE,          /**< Batch Job Message Request option. */
	_SC_2_PBS_TRACK,            /**< Track Batch Job Request option. */
	_SC_2_SW_DEV,               /**< Software Development Utilities option. */
	_SC_2_UPE,                  /**< User Portability Utilities option. */
	_SC_2_VERSION,              /**< POSIX.2 standard version. */
	_SC_XOPEN_CRYPT,            /**< Encryption option. */
	_SC_XOPEN_ENH_I18N,         /**< Enhanced Internationalization option. */
	_SC_XOPEN_REALTIME,         /**< X/Open Realtime option. */
	_SC_XOPEN_REALTIME_THREADS, /**< X/Open Realtime Threads option. */
	_SC_XOPEN_SHM,              /**< Shared Memory option. */
	_SC_XOPEN_STREAMS,          /**< XSI STREAMS option. */
	_SC_XOPEN_UNIX,             /**< XSI option. */
	_SC_XOPEN_UUCP,             /**< UUCP option. */
	_SC_XOPEN_VERSION,          /**< X/Open version. */
	_SC_CLK_TCK,                /**< Number of clock ticks per second. */
	_SC_GETGR_R_SIZE_MAX,       /**< Suggested buffer size for getgrgid_r(). */
	_SC_GETPW_R_SIZE_MAX,       /**< Suggested buffer size for getpwuid_r(). */
	_SC_AIO_LISTIO_MAX,         /**< Maximum operations in a single lio_listio() call. */
	_SC_AIO_MAX,                /**< Maximum outstanding asynchronous I/O operations. */
	_SC_AIO_PRIO_DELTA_MAX,     /**< Maximum amount the AIO priority can be decreased. */
	_SC_ARG_MAX,                /**< Maximum length of argument to exec functions. */
	_SC_ATEXIT_MAX,             /**< Maximum number of atexit() functions. */
	_SC_CHILD_MAX,              /**< Maximum number of simultaneous processes per user. */
	_SC_HOST_NAME_MAX,          /**< Maximum length of a host name. */
	_SC_IOV_MAX,                /**< Maximum number of iovec structures for readv()/writev(). */
	_SC_LOGIN_NAME_MAX,         /**< Maximum length of login name. */
	_SC_NGROUPS_MAX,            /**< Maximum number of supplemental groups. */
	_SC_MQ_OPEN_MAX,            /**< Maximum number of open message queues. */
	_SC_MQ_PRIO_MAX,            /**< Maximum message priority. */
	_SC_OPEN_MAX,               /**< Maximum number of open file descriptors. */
	_SC_PAGE_SIZE,              /**< System memory page size. */
	_SC_PAGESIZE,               /**< Alias for @c _SC_PAGE_SIZE. */
	_SC_THREAD_DESTRUCTOR_ITERATIONS, /**< Maximum TSD destructor iterations. */
	_SC_THREAD_KEYS_MAX,        /**< Maximum number of thread-specific data keys. */
	_SC_THREAD_STACK_MIN,       /**< Minimum thread stack size. */
	_SC_THREAD_THREADS_MAX,     /**< Maximum number of threads per process. */
	_SC_RTSIG_MAX,              /**< Maximum number of real-time signals. */
	_SC_SEM_NSEMS_MAX,          /**< Maximum number of semaphores per process. */
	_SC_SEM_VALUE_MAX,          /**< Maximum value of a semaphore. */
	_SC_SIGQUEUE_MAX,           /**< Maximum number of queued signals. */
	_SC_STREAM_MAX,             /**< Maximum number of open stdio streams. */
	_SC_SYMLOOP_MAX,            /**< Maximum number of symbolic links to follow. */
	_SC_TIMER_MAX,              /**< Maximum number of timer objects per process. */
	_SC_TTY_NAME_MAX,           /**< Maximum length of terminal device name. */
	_SC_TZNAME_MAX,             /**< Maximum length of timezone name. */
};

/*
 * clang-format and checkpatch disagree on formatting here, so rely on checkpatch and disable
 * clang-format since checkpatch cannot be selectively disabled.
 */

/* clang-format off */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_ADVISORY_INFO (-1L)
#define __z_posix_sysconf_SC_ASYNCHRONOUS_IO                                                       \
	COND_CODE_1(CONFIG_POSIX_ASYNCHRONOUS_IO, (_POSIX_ASYNCHRONOUS_IO), (-1L))
#define __z_posix_sysconf_SC_BARRIERS COND_CODE_1(CONFIG_POSIX_BARRIERS, (_POSIX_BARRIERS), (-1L))
#define __z_posix_sysconf_SC_CLOCK_SELECTION                                                       \
	COND_CODE_1(CONFIG_POSIX_CLOCK_SELECTION, (_POSIX_CLOCK_SELECTION), (-1L))
#define __z_posix_sysconf_SC_CPUTIME \
	COND_CODE_1(CONFIG_POSIX_CPUTIME, (_POSIX_CPUTIME), (-1L))
#define __z_posix_sysconf_SC_FSYNC                                                                 \
	COND_CODE_1(CONFIG_POSIX_FSYNC, (_POSIX_FSYNC), (-1L))
#define __z_posix_sysconf_SC_IPV6              COND_CODE_1(CONFIG_NET_IPV6, (_POSIX_IPV6), (-1L))
#define __z_posix_sysconf_SC_JOB_CONTROL       (-1L)
#define __z_posix_sysconf_SC_MAPPED_FILES                                                          \
	COND_CODE_1(CONFIG_POSIX_MAPPED_FILES, (_POSIX_MAPPED_FILES), (-1L))
#define __z_posix_sysconf_SC_MEMLOCK                                                               \
	COND_CODE_1(CONFIG_POSIX_MEMLOCK, (_POSIX_MEMLOCK), (-1L))
#define __z_posix_sysconf_SC_MEMLOCK_RANGE                                                         \
	COND_CODE_1(CONFIG_POSIX_MEMLOCK_RANGE, (_POSIX_MEMLOCK_RANGE), (-1L))
#define __z_posix_sysconf_SC_MEMORY_PROTECTION                                                     \
	COND_CODE_1(CONFIG_POSIX_MEMORY_PROTECTION, (_POSIX_MEMORY_PROTECTION), (-1L))
#define __z_posix_sysconf_SC_MESSAGE_PASSING                                                       \
	COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (_POSIX_MESSAGE_PASSING), (-1L))
#define __z_posix_sysconf_SC_MONOTONIC_CLOCK                                                       \
	COND_CODE_1(CONFIG_POSIX_MONOTONIC_CLOCK, (_POSIX_MONOTONIC_CLOCK), (-1L))
#define __z_posix_sysconf_SC_PRIORITIZED_IO (-1L)
#define __z_posix_sysconf_SC_PRIORITY_SCHEDULING                                                   \
	COND_CODE_1(CONFIG_POSIX_PRIORITY_SCHEDULING, (_POSIX_PRIORITY_SCHEDULING), (-1L))
#define __z_posix_sysconf_SC_RAW_SOCKETS                                                           \
	COND_CODE_1(CONFIG_NET_SOCKETS_PACKET, (_POSIX_RAW_SOCKETS), (-1L))
#define __z_posix_sysconf_SC_RE_DUP_MAX _POSIX_RE_DUP_MAX
#define __z_posix_sysconf_SC_READER_WRITER_LOCKS                                                   \
	COND_CODE_1(CONFIG_POSIX_RW_LOCKS, (_POSIX_READER_WRITER_LOCKS), (-1L))
#define __z_posix_sysconf_SC_REALTIME_SIGNALS      (-1L)
#define __z_posix_sysconf_SC_REGEXP                (-1L)
#define __z_posix_sysconf_SC_SAVED_IDS             (-1L)
#define __z_posix_sysconf_SC_SEMAPHORES                                                            \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (_POSIX_SEMAPHORES), (-1L))
#define __z_posix_sysconf_SC_SHARED_MEMORY_OBJECTS                                                 \
	COND_CODE_1(CONFIG_POSIX_SHARED_MEMORY_OBJECTS, (_POSIX_SHARED_MEMORY_OBJECTS), (-1L))
#define __z_posix_sysconf_SC_SHELL                 (-1L)
#define __z_posix_sysconf_SC_SPAWN                 (-1L)
#define __z_posix_sysconf_SC_SPIN_LOCKS                                                            \
	COND_CODE_1(CONFIG_POSIX_SPIN_LOCKS, (_POSIX_SPIN_LOCKS), (-1L))
#define __z_posix_sysconf_SC_SPORADIC_SERVER (-1L)
#define __z_posix_sysconf_SC_SS_REPL_MAX     _POSIX_SS_REPL_MAX
#define __z_posix_sysconf_SC_SYNCHRONIZED_IO (-1L)
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKADDR                                                 \
	COND_CODE_1(CONFIG_POSIX_THREAD_ATTR_STACKADDR, (_POSIX_THREAD_ATTR_STACKADDR), (-1))
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKSIZE                                                 \
	COND_CODE_1(CONFIG_POSIX_THREAD_ATTR_STACKSIZE, (_POSIX_THREAD_ATTR_STACKSIZE), (-1L))
#define __z_posix_sysconf_SC_THREAD_CPUTIME (-1L)
#define __z_posix_sysconf_SC_THREAD_PRIO_INHERIT                                                   \
	COND_CODE_1(CONFIG_POSIX_THREAD_PRIO_INHERIT, (_POSIX_THREAD_PRIO_INHERIT), (-1L))
#define __z_posix_sysconf_SC_THREAD_PRIO_PROTECT        (-1L)
#define __z_posix_sysconf_SC_THREAD_PRIORITY_SCHEDULING                                            \
	COND_CODE_1(CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING, (_POSIX_THREAD_PRIORITY_SCHEDULING),  \
		    (-1L))
#define __z_posix_sysconf_SC_THREAD_PROCESS_SHARED      (-1L)
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_INHERIT (-1L)
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_PROTECT (-1L)
#define __z_posix_sysconf_SC_THREAD_SAFE_FUNCTIONS                                                 \
	COND_CODE_1(CONFIG_POSIX_THREAD_SAFE_FUNCTIONS, (_POSIX_THREAD_SAFE_FUNCTIONS), (-1L))
#define __z_posix_sysconf_SC_THREAD_SPORADIC_SERVER       (-1L)
#define __z_posix_sysconf_SC_THREADS                                                               \
	COND_CODE_1(CONFIG_POSIX_THREADS, (_POSIX_THREADS), (-1L))
#define __z_posix_sysconf_SC_TIMEOUTS                                                              \
	COND_CODE_1(CONFIG_POSIX_TIMEOUTS, (_POSIX_TIMEOUTS), (-1L))
#define __z_posix_sysconf_SC_TIMERS                                                                \
	COND_CODE_1(CONFIG_POSIX_TIMEOUTS, (_POSIX_TIMERS), (-1))
#define __z_posix_sysconf_SC_TRACE                        (-1L)
#define __z_posix_sysconf_SC_TRACE_EVENT_FILTER           (-1L)
#define __z_posix_sysconf_SC_TRACE_EVENT_NAME_MAX         _POSIX_TRACE_NAME_MAX
#define __z_posix_sysconf_SC_TRACE_INHERIT                (-1L)
#define __z_posix_sysconf_SC_TRACE_LOG                    (-1L)
#define __z_posix_sysconf_SC_TRACE_NAME_MAX               _POSIX_TRACE_NAME_MAX
#define __z_posix_sysconf_SC_TRACE_SYS_MAX                _POSIX_TRACE_SYS_MAX
#define __z_posix_sysconf_SC_TRACE_USER_EVENT_MAX         _POSIX_TRACE_USER_EVENT_MAX
#define __z_posix_sysconf_SC_TYPED_MEMORY_OBJECTS         (-1L)
#define __z_posix_sysconf_SC_VERSION                      _POSIX_VERSION
#define __z_posix_sysconf_SC_V6_ILP32_OFF32               (-1L)
#define __z_posix_sysconf_SC_V6_ILP32_OFFBIG              (-1L)
#define __z_posix_sysconf_SC_V6_LP64_OFF64                (-1L)
#define __z_posix_sysconf_SC_V6_LPBIG_OFFBIG              (-1L)
#define __z_posix_sysconf_SC_V7_ILP32_OFF32               (-1L)
#define __z_posix_sysconf_SC_V7_ILP32_OFFBIG              (-1L)
#define __z_posix_sysconf_SC_V7_LP64_OFF64                (-1L)
#define __z_posix_sysconf_SC_V7_LPBIG_OFFBIG              (-1L)
#define __z_posix_sysconf_SC_BC_BASE_MAX                  _POSIX2_BC_BASE_MAX
#define __z_posix_sysconf_SC_BC_DIM_MAX                   _POSIX2_BC_DIM_MAX
#define __z_posix_sysconf_SC_BC_SCALE_MAX                 _POSIX2_BC_SCALE_MAX
#define __z_posix_sysconf_SC_BC_STRING_MAX                _POSIX2_BC_STRING_MAX
#define __z_posix_sysconf_SC_2_C_BIND                     _POSIX2_C_BIND
#define __z_posix_sysconf_SC_2_C_DEV                                                               \
	COND_CODE_1(_POSIX2_C_DEV > 0, (_POSIX2_C_DEV), (-1))
#define __z_posix_sysconf_SC_2_CHAR_TERM                  (-1L)
#define __z_posix_sysconf_SC_COLL_WEIGHTS_MAX             _POSIX2_COLL_WEIGHTS_MAX
#define __z_posix_sysconf_SC_DELAYTIMER_MAX                                                        \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_DELAYTIMER_MAX), (0))
#define __z_posix_sysconf_SC_EXPR_NEST_MAX                _POSIX2_EXPR_NEST_MAX
#define __z_posix_sysconf_SC_2_FORT_DEV                   (-1L)
#define __z_posix_sysconf_SC_2_FORT_RUN                   (-1L)
#define __z_posix_sysconf_SC_LINE_MAX                     (-1L)
#define __z_posix_sysconf_SC_2_LOCALEDEF                  (-1L)
#define __z_posix_sysconf_SC_2_PBS                        (-1L)
#define __z_posix_sysconf_SC_2_PBS_ACCOUNTING             (-1L)
#define __z_posix_sysconf_SC_2_PBS_CHECKPOINT             (-1L)
#define __z_posix_sysconf_SC_2_PBS_LOCATE                 (-1L)
#define __z_posix_sysconf_SC_2_PBS_MESSAGE                (-1L)
#define __z_posix_sysconf_SC_2_PBS_TRACK                  (-1L)
#define __z_posix_sysconf_SC_2_SW_DEV                     (-1L)
#define __z_posix_sysconf_SC_2_UPE                        (-1L)
#define __z_posix_sysconf_SC_2_VERSION                                                             \
	COND_CODE_1(_POSIX2_VERSION > 0, (_POSIX2_VERSION), (-1))
#define __z_posix_sysconf_SC_XOPEN_CRYPT                  (-1L)
#define __z_posix_sysconf_SC_XOPEN_ENH_I18N               (-1L)
#define __z_posix_sysconf_SC_XOPEN_REALTIME               (-1L)
#define __z_posix_sysconf_SC_XOPEN_REALTIME_THREADS       (-1L)
#define __z_posix_sysconf_SC_XOPEN_SHM                    (-1L)
#define __z_posix_sysconf_SC_XOPEN_STREAMS                                                         \
	COND_CODE_1(CONFIG_XOPEN_STREAMS, (_XOPEN_STREAMS), (-1))
#define __z_posix_sysconf_SC_XOPEN_UNIX                   (-1L)
#define __z_posix_sysconf_SC_XOPEN_UUCP                   (-1L)
#define __z_posix_sysconf_SC_XOPEN_VERSION                _XOPEN_VERSION
#define __z_posix_sysconf_SC_CLK_TCK                      (100L)
#define __z_posix_sysconf_SC_GETGR_R_SIZE_MAX             (0L)
#define __z_posix_sysconf_SC_GETPW_R_SIZE_MAX             (0L)
#define __z_posix_sysconf_SC_AIO_LISTIO_MAX               _POSIX_AIO_LISTIO_MAX
#define __z_posix_sysconf_SC_AIO_MAX                      _POSIX_AIO_MAX
#define __z_posix_sysconf_SC_AIO_PRIO_DELTA_MAX           0
#define __z_posix_sysconf_SC_ARG_MAX                      _POSIX_ARG_MAX
#define __z_posix_sysconf_SC_ATEXIT_MAX                   32
#define __z_posix_sysconf_SC_CHILD_MAX                    _POSIX_CHILD_MAX
#define __z_posix_sysconf_SC_HOST_NAME_MAX                                                         \
	COND_CODE_1(CONFIG_POSIX_NETWORKING, (CONFIG_POSIX_HOST_NAME_MAX), (0))
#define __z_posix_sysconf_SC_IOV_MAX                      16 /* _XOPEN_IOV_MAX */
#define __z_posix_sysconf_SC_LOGIN_NAME_MAX               _POSIX_LOGIN_NAME_MAX
#define __z_posix_sysconf_SC_NGROUPS_MAX                  _POSIX_NGROUPS_MAX
#define __z_posix_sysconf_SC_MQ_OPEN_MAX                                                           \
	COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (CONFIG_POSIX_MQ_OPEN_MAX), (0))
#define __z_posix_sysconf_SC_MQ_PRIO_MAX                  _POSIX_MQ_PRIO_MAX
#define __z_posix_sysconf_SC_OPEN_MAX                     CONFIG_POSIX_OPEN_MAX
#define __z_posix_sysconf_SC_PAGE_SIZE                    CONFIG_POSIX_PAGE_SIZE
#define __z_posix_sysconf_SC_PAGESIZE                     CONFIG_POSIX_PAGE_SIZE
#define __z_posix_sysconf_SC_THREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define __z_posix_sysconf_SC_THREAD_KEYS_MAX                                                       \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_KEYS_MAX), (0))
#define __z_posix_sysconf_SC_THREAD_STACK_MIN             0
#define __z_posix_sysconf_SC_THREAD_THREADS_MAX                                                    \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_THREADS_MAX), (0))
#define __z_posix_sysconf_SC_RTSIG_MAX                                                             \
	COND_CODE_1(CONFIG_POSIX_REALTIME_SIGNALS, (CONFIG_POSIX_RTSIG_MAX), (0))
#define __z_posix_sysconf_SC_SEM_NSEMS_MAX                                                         \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_NSEMS_MAX), (0))
#define __z_posix_sysconf_SC_SEM_VALUE_MAX                                                         \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_VALUE_MAX), (0))
#define __z_posix_sysconf_SC_SIGQUEUE_MAX                 _POSIX_SIGQUEUE_MAX
#define __z_posix_sysconf_SC_STREAM_MAX                   _POSIX_STREAM_MAX
#define __z_posix_sysconf_SC_SYMLOOP_MAX                  _POSIX_SYMLOOP_MAX
#define __z_posix_sysconf_SC_TIMER_MAX                                                             \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_TIMER_MAX), (0))
#define __z_posix_sysconf_SC_TTY_NAME_MAX                 _POSIX_TTY_NAME_MAX
#define __z_posix_sysconf_SC_TZNAME_MAX                   _POSIX_TZNAME_MAX
/** @endcond */

#ifdef CONFIG_POSIX_SYSCONF_IMPL_MACRO
/**
 * @brief Get the value of a configurable system variable.
 *
 * @param x @c _SC_ constant naming the variable to query (e.g. @c _SC_PAGESIZE).
 *
 * @return Value of the variable, or -1 if the associated option or limit is unsupported.
 */
#define sysconf(x) (long)CONCAT(__z_posix_sysconf, x)
#endif

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_ */
