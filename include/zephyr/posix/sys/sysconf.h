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
	/**
	 * @brief Advisory information option.
	 */
	_SC_ADVISORY_INFO,
	/**
	 * @brief Asynchronous I/O option.
	 */
	_SC_ASYNCHRONOUS_IO,
	/**
	 * @brief Barriers option.
	 */
	_SC_BARRIERS,
	/**
	 * @brief Clock Selection option.
	 */
	_SC_CLOCK_SELECTION,
	/**
	 * @brief Process CPU-Time Clocks option.
	 */
	_SC_CPUTIME,
	/**
	 * @brief File Synchronization option.
	 */
	_SC_FSYNC,
	/**
	 * @brief IPv6 option.
	 */
	_SC_IPV6,
	/**
	 * @brief Job control.
	 */
	_SC_JOB_CONTROL,
	/**
	 * @brief Mapped Files option.
	 */
	_SC_MAPPED_FILES,
	/**
	 * @brief Process Memory Locking option.
	 */
	_SC_MEMLOCK,
	/**
	 * @brief Range Memory Locking option.
	 */
	_SC_MEMLOCK_RANGE,
	/**
	 * @brief Memory Protection option.
	 */
	_SC_MEMORY_PROTECTION,
	/**
	 * @brief Message Passing option.
	 */
	_SC_MESSAGE_PASSING,
	/**
	 * @brief Monotonic Clock option.
	 */
	_SC_MONOTONIC_CLOCK,
	/**
	 * @brief Prioritized Input and Output option.
	 */
	_SC_PRIORITIZED_IO,
	/**
	 * @brief Process Scheduling option.
	 */
	_SC_PRIORITY_SCHEDULING,
	/**
	 * @brief Raw Sockets option.
	 */
	_SC_RAW_SOCKETS,
	/**
	 * @brief Maximum number of repeated occurrences in BRE.
	 */
	_SC_RE_DUP_MAX,
	/**
	 * @brief Read-Write Locks option.
	 */
	_SC_READER_WRITER_LOCKS,
	/**
	 * @brief Realtime Signals extension.
	 */
	_SC_REALTIME_SIGNALS,
	/**
	 * @brief Regular Expressions option.
	 */
	_SC_REGEXP,
	/**
	 * @brief Saved IDs option.
	 */
	_SC_SAVED_IDS,
	/**
	 * @brief Semaphores option.
	 */
	_SC_SEMAPHORES,
	/**
	 * @brief Shared Memory Objects option.
	 */
	_SC_SHARED_MEMORY_OBJECTS,
	/**
	 * @brief Shell option.
	 */
	_SC_SHELL,
	/**
	 * @brief Spawn option.
	 */
	_SC_SPAWN,
	/**
	 * @brief Spin Locks option.
	 */
	_SC_SPIN_LOCKS,
	/**
	 * @brief Process Sporadic Server option.
	 */
	_SC_SPORADIC_SERVER,
	/**
	 * @brief Maximum number of replenishments for SS.
	 */
	_SC_SS_REPL_MAX,
	/**
	 * @brief Synchronized Input and Output option.
	 */
	_SC_SYNCHRONIZED_IO,
	/**
	 * @brief Thread Stack Address Attribute option.
	 */
	_SC_THREAD_ATTR_STACKADDR,
	/**
	 * @brief Thread Stack Size Attribute option.
	 */
	_SC_THREAD_ATTR_STACKSIZE,
	/**
	 * @brief Thread CPU-Time Clocks option.
	 */
	_SC_THREAD_CPUTIME,
	/**
	 * @brief Thread Priority Inheritance option.
	 */
	_SC_THREAD_PRIO_INHERIT,
	/**
	 * @brief Thread Priority Protection option.
	 */
	_SC_THREAD_PRIO_PROTECT,
	/**
	 * @brief Thread Execution Scheduling option.
	 */
	_SC_THREAD_PRIORITY_SCHEDULING,
	/**
	 * @brief Thread Process-Shared Synchronization option.
	 */
	_SC_THREAD_PROCESS_SHARED,
	/**
	 * @brief Robust Mutex Priority Inheritance option.
	 */
	_SC_THREAD_ROBUST_PRIO_INHERIT,
	/**
	 * @brief Robust Mutex Priority Protection option.
	 */
	_SC_THREAD_ROBUST_PRIO_PROTECT,
	/**
	 * @brief Thread-Safe Functions option.
	 */
	_SC_THREAD_SAFE_FUNCTIONS,
	/**
	 * @brief Thread Sporadic Server option.
	 */
	_SC_THREAD_SPORADIC_SERVER,
	/**
	 * @brief Threads option.
	 */
	_SC_THREADS,
	/**
	 * @brief Timeouts option.
	 */
	_SC_TIMEOUTS,
	/**
	 * @brief Timers option.
	 */
	_SC_TIMERS,
	/**
	 * @brief Trace option.
	 */
	_SC_TRACE,
	/**
	 * @brief Trace Event Filter option.
	 */
	_SC_TRACE_EVENT_FILTER,
	/**
	 * @brief Maximum length of trace event names.
	 */
	_SC_TRACE_EVENT_NAME_MAX,
	/**
	 * @brief Trace Inherit option.
	 */
	_SC_TRACE_INHERIT,
	/**
	 * @brief Trace Log option.
	 */
	_SC_TRACE_LOG,
	/**
	 * @brief Maximum length of trace names.
	 */
	_SC_TRACE_NAME_MAX,
	/**
	 * @brief Maximum number of system trace streams.
	 */
	_SC_TRACE_SYS_MAX,
	/**
	 * @brief Maximum number of user trace event type identifiers.
	 */
	_SC_TRACE_USER_EVENT_MAX,
	/**
	 * @brief Typed Memory Objects option.
	 */
	_SC_TYPED_MEMORY_OBJECTS,
	/**
	 * @brief POSIX.1 standard version (@c _POSIX_VERSION).
	 */
	_SC_VERSION,
	/**
	 * @brief ILP32 data model, 32-bit offsets (POSIX.1-2008).
	 */
	_SC_V7_ILP32_OFF32,
	/**
	 * @brief ILP32 data model, large file offsets (POSIX.1-2008).
	 */
	_SC_V7_ILP32_OFFBIG,
	/**
	 * @brief LP64 data model, 64-bit offsets (POSIX.1-2008).
	 */
	_SC_V7_LP64_OFF64,
	/**
	 * @brief LPBIG data model, large offsets (POSIX.1-2008).
	 */
	_SC_V7_LPBIG_OFFBIG,
	/**
	 * @brief ILP32 data model, 32-bit offsets (POSIX.1-2001).
	 */
	_SC_V6_ILP32_OFF32,
	/**
	 * @brief ILP32 data model, large file offsets (POSIX.1-2001).
	 */
	_SC_V6_ILP32_OFFBIG,
	/**
	 * @brief LP64 data model, 64-bit offsets (POSIX.1-2001).
	 */
	_SC_V6_LP64_OFF64,
	/**
	 * @brief LPBIG data model, large offsets (POSIX.1-2001).
	 */
	_SC_V6_LPBIG_OFFBIG,
	/**
	 * @brief Maximum value of obase in bc.
	 */
	_SC_BC_BASE_MAX,
	/**
	 * @brief Maximum number of elements in bc arrays.
	 */
	_SC_BC_DIM_MAX,
	/**
	 * @brief Maximum value of scale in bc.
	 */
	_SC_BC_SCALE_MAX,
	/**
	 * @brief Maximum length of string in bc.
	 */
	_SC_BC_STRING_MAX,
	/**
	 * @brief C-Language Binding option.
	 */
	_SC_2_C_BIND,
	/**
	 * @brief C-Language Development Utilities option.
	 */
	_SC_2_C_DEV,
	/**
	 * @brief Terminal Characteristics option.
	 */
	_SC_2_CHAR_TERM,
	/**
	 * @brief Maximum weights in locale collating.
	 */
	_SC_COLL_WEIGHTS_MAX,
	/**
	 * @brief Maximum number of timer overruns.
	 */
	_SC_DELAYTIMER_MAX,
	/**
	 * @brief Maximum number of expr()s in expr.
	 */
	_SC_EXPR_NEST_MAX,
	/**
	 * @brief FORTRAN Development Utilities option.
	 */
	_SC_2_FORT_DEV,
	/**
	 * @brief FORTRAN Runtime Utilities option.
	 */
	_SC_2_FORT_RUN,
	/**
	 * @brief Maximum length of input line in utilities.
	 */
	_SC_LINE_MAX,
	/**
	 * @brief Locale Creation option.
	 */
	_SC_2_LOCALEDEF,
	/**
	 * @brief Batch Environment Services and Utilities option.
	 */
	_SC_2_PBS,
	/**
	 * @brief Batch Accounting option.
	 */
	_SC_2_PBS_ACCOUNTING,
	/**
	 * @brief Batch Checkpoint/Restart option.
	 */
	_SC_2_PBS_CHECKPOINT,
	/**
	 * @brief Locate Batch Job Request option.
	 */
	_SC_2_PBS_LOCATE,
	/**
	 * @brief Batch Job Message Request option.
	 */
	_SC_2_PBS_MESSAGE,
	/**
	 * @brief Track Batch Job Request option.
	 */
	_SC_2_PBS_TRACK,
	/**
	 * @brief Software Development Utilities option.
	 */
	_SC_2_SW_DEV,
	/**
	 * @brief User Portability Utilities option.
	 */
	_SC_2_UPE,
	/**
	 * @brief POSIX.2 standard version.
	 */
	_SC_2_VERSION,
	/**
	 * @brief Encryption option.
	 */
	_SC_XOPEN_CRYPT,
	/**
	 * @brief Enhanced Internationalization option.
	 */
	_SC_XOPEN_ENH_I18N,
	/**
	 * @brief X/Open Realtime option.
	 */
	_SC_XOPEN_REALTIME,
	/**
	 * @brief X/Open Realtime Threads option.
	 */
	_SC_XOPEN_REALTIME_THREADS,
	/**
	 * @brief Shared Memory option.
	 */
	_SC_XOPEN_SHM,
	/**
	 * @brief XSI STREAMS option.
	 */
	_SC_XOPEN_STREAMS,
	/**
	 * @brief XSI option.
	 */
	_SC_XOPEN_UNIX,
	/**
	 * @brief UUCP option.
	 */
	_SC_XOPEN_UUCP,
	/**
	 * @brief X/Open version.
	 */
	_SC_XOPEN_VERSION,
	/**
	 * @brief Number of clock ticks per second.
	 */
	_SC_CLK_TCK,
	/**
	 * @brief Suggested buffer size for getgrgid_r().
	 */
	_SC_GETGR_R_SIZE_MAX,
	/**
	 * @brief Suggested buffer size for getpwuid_r().
	 */
	_SC_GETPW_R_SIZE_MAX,
	/**
	 * @brief Maximum operations in a single lio_listio() call.
	 */
	_SC_AIO_LISTIO_MAX,
	/**
	 * @brief Maximum outstanding asynchronous I/O operations.
	 */
	_SC_AIO_MAX,
	/**
	 * @brief Maximum amount the AIO priority can be decreased.
	 */
	_SC_AIO_PRIO_DELTA_MAX,
	/**
	 * @brief Maximum length of argument to exec functions.
	 */
	_SC_ARG_MAX,
	/**
	 * @brief Maximum number of atexit() functions.
	 */
	_SC_ATEXIT_MAX,
	/**
	 * @brief Maximum number of simultaneous processes per user.
	 */
	_SC_CHILD_MAX,
	/**
	 * @brief Maximum length of a host name.
	 */
	_SC_HOST_NAME_MAX,
	/**
	 * @brief Maximum number of iovec structures for readv()/writev().
	 */
	_SC_IOV_MAX,
	/**
	 * @brief Maximum length of login name.
	 */
	_SC_LOGIN_NAME_MAX,
	/**
	 * @brief Maximum number of supplemental groups.
	 */
	_SC_NGROUPS_MAX,
	/**
	 * @brief Maximum number of open message queues.
	 */
	_SC_MQ_OPEN_MAX,
	/**
	 * @brief Maximum message priority.
	 */
	_SC_MQ_PRIO_MAX,
	/**
	 * @brief Maximum number of open file descriptors.
	 */
	_SC_OPEN_MAX,
	/**
	 * @brief System memory page size.
	 */
	_SC_PAGE_SIZE,
	/**
	 * @brief Alias for @c _SC_PAGE_SIZE.
	 */
	_SC_PAGESIZE,
	/**
	 * @brief Maximum TSD destructor iterations.
	 */
	_SC_THREAD_DESTRUCTOR_ITERATIONS,
	/**
	 * @brief Maximum number of thread-specific data keys.
	 */
	_SC_THREAD_KEYS_MAX,
	/**
	 * @brief Minimum thread stack size.
	 */
	_SC_THREAD_STACK_MIN,
	/**
	 * @brief Maximum number of threads per process.
	 */
	_SC_THREAD_THREADS_MAX,
	/**
	 * @brief Maximum number of real-time signals.
	 */
	_SC_RTSIG_MAX,
	/**
	 * @brief Maximum number of semaphores per process.
	 */
	_SC_SEM_NSEMS_MAX,
	/**
	 * @brief Maximum value of a semaphore.
	 */
	_SC_SEM_VALUE_MAX,
	/**
	 * @brief Maximum number of queued signals.
	 */
	_SC_SIGQUEUE_MAX,
	/**
	 * @brief Maximum number of open stdio streams.
	 */
	_SC_STREAM_MAX,
	/**
	 * @brief Maximum number of symbolic links to follow.
	 */
	_SC_SYMLOOP_MAX,
	/**
	 * @brief Maximum number of timer objects per process.
	 */
	_SC_TIMER_MAX,
	/**
	 * @brief Maximum length of terminal device name.
	 */
	_SC_TTY_NAME_MAX,
	/**
	 * @brief Maximum length of timezone name.
	 */
	_SC_TZNAME_MAX,
};

/*
 * clang-format and checkpatch disagree on formatting here, so rely on checkpatch and disable
 * clang-format since checkpatch cannot be selectively disabled.
 */

/* clang-format off */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_ADVISORY_INFO (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_ASYNCHRONOUS_IO                                                       \
	COND_CODE_1(CONFIG_POSIX_ASYNCHRONOUS_IO, (_POSIX_ASYNCHRONOUS_IO), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_BARRIERS COND_CODE_1(CONFIG_POSIX_BARRIERS, (_POSIX_BARRIERS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_CLOCK_SELECTION                                                       \
	COND_CODE_1(CONFIG_POSIX_CLOCK_SELECTION, (_POSIX_CLOCK_SELECTION), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_CPUTIME \
	COND_CODE_1(CONFIG_POSIX_CPUTIME, (_POSIX_CPUTIME), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_FSYNC                                                                 \
	COND_CODE_1(CONFIG_POSIX_FSYNC, (_POSIX_FSYNC), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_IPV6              COND_CODE_1(CONFIG_NET_IPV6, (_POSIX_IPV6), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_JOB_CONTROL       (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MAPPED_FILES                                                          \
	COND_CODE_1(CONFIG_POSIX_MAPPED_FILES, (_POSIX_MAPPED_FILES), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MEMLOCK                                                               \
	COND_CODE_1(CONFIG_POSIX_MEMLOCK, (_POSIX_MEMLOCK), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MEMLOCK_RANGE                                                         \
	COND_CODE_1(CONFIG_POSIX_MEMLOCK_RANGE, (_POSIX_MEMLOCK_RANGE), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MEMORY_PROTECTION                                                     \
	COND_CODE_1(CONFIG_POSIX_MEMORY_PROTECTION, (_POSIX_MEMORY_PROTECTION), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MESSAGE_PASSING                                                       \
	COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (_POSIX_MESSAGE_PASSING), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MONOTONIC_CLOCK                                                       \
	COND_CODE_1(CONFIG_POSIX_MONOTONIC_CLOCK, (_POSIX_MONOTONIC_CLOCK), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_PRIORITIZED_IO (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_PRIORITY_SCHEDULING                                                   \
	COND_CODE_1(CONFIG_POSIX_PRIORITY_SCHEDULING, (_POSIX_PRIORITY_SCHEDULING), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_RAW_SOCKETS                                                           \
	COND_CODE_1(CONFIG_NET_SOCKETS_PACKET, (_POSIX_RAW_SOCKETS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_RE_DUP_MAX _POSIX_RE_DUP_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_READER_WRITER_LOCKS                                                   \
	COND_CODE_1(CONFIG_POSIX_RW_LOCKS, (_POSIX_READER_WRITER_LOCKS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_REALTIME_SIGNALS      (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_REGEXP                (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SAVED_IDS             (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SEMAPHORES                                                            \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (_POSIX_SEMAPHORES), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SHARED_MEMORY_OBJECTS                                                 \
	COND_CODE_1(CONFIG_POSIX_SHARED_MEMORY_OBJECTS, (_POSIX_SHARED_MEMORY_OBJECTS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SHELL                 (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SPAWN                 (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SPIN_LOCKS                                                            \
	COND_CODE_1(CONFIG_POSIX_SPIN_LOCKS, (_POSIX_SPIN_LOCKS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SPORADIC_SERVER (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SS_REPL_MAX     _POSIX_SS_REPL_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SYNCHRONIZED_IO (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKADDR                                                 \
	COND_CODE_1(CONFIG_POSIX_THREAD_ATTR_STACKADDR, (_POSIX_THREAD_ATTR_STACKADDR), (-1))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKSIZE                                                 \
	COND_CODE_1(CONFIG_POSIX_THREAD_ATTR_STACKSIZE, (_POSIX_THREAD_ATTR_STACKSIZE), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_CPUTIME (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_PRIO_INHERIT                                                   \
	COND_CODE_1(CONFIG_POSIX_THREAD_PRIO_INHERIT, (_POSIX_THREAD_PRIO_INHERIT), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_PRIO_PROTECT        (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_PRIORITY_SCHEDULING                                            \
	COND_CODE_1(CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING, (_POSIX_THREAD_PRIORITY_SCHEDULING),  \
		    (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_PROCESS_SHARED      (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_INHERIT (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_PROTECT (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_SAFE_FUNCTIONS                                                 \
	COND_CODE_1(CONFIG_POSIX_THREAD_SAFE_FUNCTIONS, (_POSIX_THREAD_SAFE_FUNCTIONS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_SPORADIC_SERVER       (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREADS                                                               \
	COND_CODE_1(CONFIG_POSIX_THREADS, (_POSIX_THREADS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TIMEOUTS                                                              \
	COND_CODE_1(CONFIG_POSIX_TIMEOUTS, (_POSIX_TIMEOUTS), (-1L))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TIMERS                                                                \
	COND_CODE_1(CONFIG_POSIX_TIMEOUTS, (_POSIX_TIMERS), (-1))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE                        (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_EVENT_FILTER           (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_EVENT_NAME_MAX         _POSIX_TRACE_NAME_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_INHERIT                (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_LOG                    (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_NAME_MAX               _POSIX_TRACE_NAME_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_SYS_MAX                _POSIX_TRACE_SYS_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TRACE_USER_EVENT_MAX         _POSIX_TRACE_USER_EVENT_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TYPED_MEMORY_OBJECTS         (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_VERSION                      _POSIX_VERSION
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V6_ILP32_OFF32               (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V6_ILP32_OFFBIG              (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V6_LP64_OFF64                (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V6_LPBIG_OFFBIG              (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V7_ILP32_OFF32               (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V7_ILP32_OFFBIG              (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V7_LP64_OFF64                (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_V7_LPBIG_OFFBIG              (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_BC_BASE_MAX                  _POSIX2_BC_BASE_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_BC_DIM_MAX                   _POSIX2_BC_DIM_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_BC_SCALE_MAX                 _POSIX2_BC_SCALE_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_BC_STRING_MAX                _POSIX2_BC_STRING_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_C_BIND                     _POSIX2_C_BIND
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_C_DEV                                                               \
	COND_CODE_1(_POSIX2_C_DEV > 0, (_POSIX2_C_DEV), (-1))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_CHAR_TERM                  (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_COLL_WEIGHTS_MAX             _POSIX2_COLL_WEIGHTS_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_DELAYTIMER_MAX                                                        \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_DELAYTIMER_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_EXPR_NEST_MAX                _POSIX2_EXPR_NEST_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_FORT_DEV                   (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_FORT_RUN                   (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_LINE_MAX                     (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_LOCALEDEF                  (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_PBS                        (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_PBS_ACCOUNTING             (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_PBS_CHECKPOINT             (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_PBS_LOCATE                 (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_PBS_MESSAGE                (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_PBS_TRACK                  (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_SW_DEV                     (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_UPE                        (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_2_VERSION                                                             \
	COND_CODE_1(_POSIX2_VERSION > 0, (_POSIX2_VERSION), (-1))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_CRYPT                  (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_ENH_I18N               (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_REALTIME               (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_REALTIME_THREADS       (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_SHM                    (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_STREAMS                                                         \
	COND_CODE_1(CONFIG_XOPEN_STREAMS, (_XOPEN_STREAMS), (-1))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_UNIX                   (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_UUCP                   (-1L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_XOPEN_VERSION                _XOPEN_VERSION
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_CLK_TCK                      (100L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_GETGR_R_SIZE_MAX             (0L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_GETPW_R_SIZE_MAX             (0L)
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_AIO_LISTIO_MAX               _POSIX_AIO_LISTIO_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_AIO_MAX                      _POSIX_AIO_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_AIO_PRIO_DELTA_MAX           0
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_ARG_MAX                      _POSIX_ARG_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_ATEXIT_MAX                   32
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_CHILD_MAX                    _POSIX_CHILD_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_HOST_NAME_MAX                                                         \
	COND_CODE_1(CONFIG_POSIX_NETWORKING, (CONFIG_POSIX_HOST_NAME_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_IOV_MAX                      16 /* _XOPEN_IOV_MAX */
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_LOGIN_NAME_MAX               _POSIX_LOGIN_NAME_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_NGROUPS_MAX                  _POSIX_NGROUPS_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MQ_OPEN_MAX                                                           \
	COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (CONFIG_POSIX_MQ_OPEN_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_MQ_PRIO_MAX                  _POSIX_MQ_PRIO_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_OPEN_MAX                     CONFIG_POSIX_OPEN_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_PAGE_SIZE                    CONFIG_POSIX_PAGE_SIZE
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_PAGESIZE                     CONFIG_POSIX_PAGE_SIZE
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_KEYS_MAX                                                       \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_KEYS_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_STACK_MIN             0
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_THREAD_THREADS_MAX                                                    \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_THREADS_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_RTSIG_MAX                                                             \
	COND_CODE_1(CONFIG_POSIX_REALTIME_SIGNALS, (CONFIG_POSIX_RTSIG_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SEM_NSEMS_MAX                                                         \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_NSEMS_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SEM_VALUE_MAX                                                         \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_VALUE_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SIGQUEUE_MAX                 _POSIX_SIGQUEUE_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_STREAM_MAX                   _POSIX_STREAM_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_SYMLOOP_MAX                  _POSIX_SYMLOOP_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TIMER_MAX                                                             \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_TIMER_MAX), (0))
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __z_posix_sysconf_SC_TTY_NAME_MAX                 _POSIX_TTY_NAME_MAX
/** @endcond */
/** @cond INTERNAL_HIDDEN */
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
