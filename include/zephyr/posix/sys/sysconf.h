/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __z_posix_sysconf_SC_ADVISORY_INFO                                                         \
	(COND_CODE_1(CONFIG_POSIX_ADVISORY_INFO, (_POSIX_ADVISORY_INFO), (-1)))
#define __z_posix_sysconf_SC_ASYNCHRONOUS_IO                                                       \
	(COND_CODE_1(CONFIG_POSIX_ASYNCHRONOUS_IO, (_POSIX_ASYNCHRONOUS_IO), (-1)))
#define __z_posix_sysconf_SC_BARRIERS (COND_CODE_1(CONFIG_POSIX_BARRIERS, (_POSIX_BARRIERS), (-1)))
#define __z_posix_sysconf_SC_CHOWN_RESTRICTED                                                      \
	(COND_CODE_1(CONFIG_POSIX_CHOWN_RESTRICTED, (_POSIX_CHOWN_RESTRICTED), (-1)))
#define __z_posix_sysconf_SC_CLOCK_SELECTION                                                       \
	(COND_CODE_1(CONFIG_POSIX_CLOCK_SELECTION, (_POSIX_CLOCK_SELECTION), (-1)))
#define __z_posix_sysconf_SC_CPUTIME (COND_CODE_1(CONFIG_POSIX_CPUTIME, (_POSIX_CPUTIME), (-1)))
#define __z_posix_sysconf_SC_FSYNC   (COND_CODE_1(CONFIG_POSIX_FSYNC, (_POSIX_FSYNC), (-1)))
#define __z_posix_sysconf_SC_IPV6    (COND_CODE_1(CONFIG_POSIX_IPV6, (_POSIX_IPV6), (-1)))
#define __z_posix_sysconf_SC_MAPPED_FILES                                                          \
	(COND_CODE_1(CONFIG_POSIX_MAPPED_FILES, (_POSIX_MAPPED_FILES), (-1)))
#define __z_posix_sysconf_SC_MEMLOCK (COND_CODE_1(CONFIG_POSIX_MEMLOCK, (_POSIX_MEMLOCK), (-1)))
#define __z_posix_sysconf_SC_MEMLOCK_RANGE                                                         \
	(COND_CODE_1(CONFIG_POSIX_MEMLOCK_RANGE, (_POSIX_MEMLOCK_RANGE), (-1)))
#define __z_posix_sysconf_SC_MEMORY_PROTECTION                                                     \
	(COND_CODE_1(CONFIG_POSIX_MEMORY_PROTECTION, (_POSIX_MEMORY_PROTECTION), (-1)))
#define __z_posix_sysconf_SC_MESSAGE_PASSING                                                       \
	(COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (_POSIX_MESSAGE_PASSING), (-1)))
#define __z_posix_sysconf_SC_MONOTONIC_CLOCK                                                       \
	(COND_CODE_1(CONFIG_POSIX_MONOTONIC_CLOCK, (_POSIX_MONOTONIC_CLOCK), (-1)))
#define __z_posix_sysconf_SC_MQ_OPEN_MAX                                                           \
	(COND_CODE_1(CONFIG_POSIX_MQ_OPEN_MAX, (_POSIX_MQ_OPEN_MAX), (-1)))
#define __z_posix_sysconf_SC_MQ_PRIO_MAX                                                           \
	(COND_CODE_1(CONFIG_POSIX_MQ_PRIO_MAX, (_POSIX_MQ_PRIO_MAX), (-1)))
#define __z_posix_sysconf_SC_NO_TRUNC (COND_CODE_1(CONFIG_POSIX_NO_TRUNC, (_POSIX_NO_TRUNC), (-1)))
#define __z_posix_sysconf_SC_OPEN_MAX (COND_CODE_1(CONFIG_POSIX_OPEN_MAX, (_POSIX_OPEN_MAX), (-1)))
#define __z_posix_sysconf_SC_PRIORITIZED_IO                                                        \
	(COND_CODE_1(CONFIG_POSIX_PRIORITIZED_IO, (_POSIX_PRIORITIZED_IO), (-1)))
#define __z_posix_sysconf_SC_PRIORITY_SCHEDULING                                                   \
	(COND_CODE_1(CONFIG_POSIX_PRIORITY_SCHEDULING, (_POSIX_PRIORITY_SCHEDULING), (-1)))
#define __z_posix_sysconf_SC_RAW_SOCKETS                                                           \
	(COND_CODE_1(CONFIG_POSIX_RAW_SOCKETS, (_POSIX_RAW_SOCKETS), (-1)))
#define __z_posix_sysconf_SC_READER_WRITER_LOCKS                                                   \
	(COND_CODE_1(CONFIG_POSIX_READER_WRITER_LOCKS, (_POSIX_READER_WRITER_LOCKS), (-1)))
#define __z_posix_sysconf_SC_REALTIME_SIGNALS                                                      \
	(COND_CODE_1(CONFIG_POSIX_REALTIME_SIGNALS, (_POSIX_REALTIME_SIGNALS), (-1)))
#define __z_posix_sysconf_SC_REENTRANT_FUNCTIONS                                                   \
	(COND_CODE_1(CONFIG_POSIX_REENTRANT_FUNCTIONS, (_POSIX_REENTRANT_FUNCTIONS), (-1)))
#define __z_posix_sysconf_SC_REGEXP (COND_CODE_1(CONFIG_POSIX_REGEXP, (_POSIX_REGEXP), (-1)))
#define __z_posix_sysconf_SC_SEMAPHORES                                                            \
	(COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (_POSIX_SEMAPHORES), (-1)))
#define __z_posix_sysconf_SC_SHARED_MEMORY_OBJECTS                                                 \
	(COND_CODE_1(CONFIG_POSIX_SHARED_MEMORY_OBJECTS, (_POSIX_SHARED_MEMORY_OBJECTS), (-1)))
#define __z_posix_sysconf_SC_SHELL (COND_CODE_1(CONFIG_POSIX_SHELL, (_POSIX_SHELL), (-1)))
#define __z_posix_sysconf_SC_SPAWN (COND_CODE_1(CONFIG_POSIX_SPAWN, (_POSIX_SPAWN), (-1)))
#define __z_posix_sysconf_SC_SPIN_LOCKS                                                            \
	(COND_CODE_1(CONFIG_POSIX_SPIN_LOCKS, (_POSIX_SPIN_LOCKS), (-1)))
#define __z_posix_sysconf_SC_SPORADIC_SERVER                                                       \
	(COND_CODE_1(CONFIG_POSIX_SPORADIC_SERVER, (_POSIX_SPORADIC_SERVER), (-1)))
#define __z_posix_sysconf_SC_SYNCHRONIZED_IO                                                       \
	(COND_CODE_1(CONFIG_POSIX_SYNCHRONIZED_IO, (_POSIX_SYNCHRONIZED_IO), (-1)))
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKADDR                                                 \
	(COND_CODE_1(CONFIG_POSIX_THREAD_ATTR_STACKADDR, (_POSIX_THREAD_ATTR_STACKADDR), (-1)))
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKSIZE                                                 \
	(COND_CODE_1(CONFIG_POSIX_THREAD_ATTR_STACKSIZE, (_POSIX_THREAD_ATTR_STACKSIZE), (-1)))
#define __z_posix_sysconf_SC_THREAD_CPUTIME                                                        \
	(COND_CODE_1(CONFIG_POSIX_THREAD_CPUTIME, (_POSIX_THREAD_CPUTIME), (-1)))
#define __z_posix_sysconf_SC_THREAD_PRIO_INHERIT                                                   \
	(COND_CODE_1(CONFIG_POSIX_THREAD_PRIO_INHERIT, (_POSIX_THREAD_PRIO_INHERIT), (-1)))
#define __z_posix_sysconf_SC_THREAD_PRIO_PROTECT                                                   \
	(COND_CODE_1(CONFIG_POSIX_THREAD_PRIO_PROTECT, (_POSIX_THREAD_PRIO_PROTECT), (-1)))
#define __z_posix_sysconf_SC_THREAD_PRIORITY_SCHEDULING                                            \
	(COND_CODE_1(CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING, (_POSIX_THREAD_PRIORITY_SCHEDULING), \
		     (-1)))
#define __z_posix_sysconf_SC_THREAD_PROCESS_SHARED                                                 \
	(COND_CODE_1(CONFIG_POSIX_THREAD_PROCESS_SHARED, (_POSIX_THREAD_PROCESS_SHARED), (-1)))
#define __z_posix_sysconf_SC_THREADS                                                               \
	(COND_CODE_1(CONFIG_POSIX_THREADS_BASE, (_POSIX_THREADS), (-1)))
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_INHERIT                                            \
	(COND_CODE_1(CONFIG_POSIX_THREAD_ROBUST_PRIO_INHERIT, (_POSIX_THREAD_ROBUST_PRIO_INHERIT), \
		     (-1)))
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_PROTECT                                            \
	(COND_CODE_1(CONFIG_POSIX_THREAD_ROBUST_PRIO_PROTECT, (_POSIX_THREAD_ROBUST_PRIO_PROTECT), \
		     (-1)))
#define __z_posix_sysconf_SC_THREAD_SAFE_FUNCTIONS                                                 \
	(COND_CODE_1(CONFIG_POSIX_THREAD_SAFE_FUNCTIONS, (_POSIX_THREAD_SAFE_FUNCTIONS), (-1)))
#define __z_posix_sysconf_SC_THREAD_SPORADIC_SERVER                                                \
	(COND_CODE_1(CONFIG_POSIX_THREAD_SPORADIC_SERVER, (_POSIX_THREAD_SPORADIC_SERVER), (-1)))
#define __z_posix_sysconf_SC_THREAD_THREADS_MAX                                                    \
	(COND_CODE_1(CONFIG_POSIX_THREAD_THREADS_MAX, (_POSIX_THREAD_THREADS_MAX), (-1)))
#define __z_posix_sysconf_SC_TIMER_MAX                                                             \
	(COND_CODE_1(CONFIG_POSIX_TIMER_MAX, (_POSIX_TIMER_MAX), (-1)))
#define __z_posix_sysconf_SC_TIMEOUTS (COND_CODE_1(CONFIG_POSIX_TIMEOUTS, (_POSIX_TIMEOUTS), (-1)))
#define __z_posix_sysconf_SC_TIMERS   (COND_CODE_1(CONFIG_POSIX_TIMERS, (_POSIX_TIMERS), (-1)))
#define __z_posix_sysconf_SC_TRACE    (COND_CODE_1(CONFIG_POSIX_TRACE, (_POSIX_TRACE), (-1)))
#define __z_posix_sysconf_SC_TRACE_EVENT_FILTER                                                    \
	(COND_CODE_1(CONFIG_POSIX_TRACE_EVENT_FILTER, (_POSIX_TRACE_EVENT_FILTER), (-1)))
#define __z_posix_sysconf_SC_TRACE_EVENT_NAME_MAX                                                  \
	(COND_CODE_1(CONFIG_POSIX_TRACE_EVENT_NAME_MAX, (_POSIX_TRACE_EVENT_NAME_MAX), (-1)))
#define __z_posix_sysconf_SC_TRACE_INHERIT                                                         \
	(COND_CODE_1(CONFIG_POSIX_TRACE_INHERIT, (_POSIX_TRACE_INHERIT), (-1)))
#define __z_posix_sysconf_SC_TRACE_LOG                                                             \
	(COND_CODE_1(CONFIG_POSIX_TRACE_LOG, (_POSIX_TRACE_LOG), (-1)))
#define __z_posix_sysconf_SC_TRACE_NAME_MAX                                                        \
	(COND_CODE_1(CONFIG_POSIX_TRACE_NAME_MAX, (_POSIX_TRACE_NAME_MAX), (-1)))
#define __z_posix_sysconf_SC_TRACE_SYS_MAX                                                         \
	(COND_CODE_1(CONFIG_POSIX_TRACE_SYS_MAX, (_POSIX_TRACE_SYS_MAX), (-1)))
#define __z_posix_sysconf_SC_TRACE_USER_EVENT_MAX                                                  \
	(COND_CODE_1(CONFIG_POSIX_TRACE_USER_EVENT_MAX, (_POSIX_TRACE_USER_EVENT_MAX), (-1)))
#define __z_posix_sysconf_SC_TYPED_MEMORY_OBJECTS                                                  \
	(COND_CODE_1(CONFIG_POSIX_TYPED_MEMORY_OBJECTS, (_POSIX_TYPED_MEMORY_OBJECTS), (-1)))
#define __z_posix_sysconf_SC_VERSION _POSIX_VERSION
#define __z_posix_sysconf_SC_XOPEN_REALTIME                                                        \
	(COND_CODE_1(CONFIG_XOPEN_REALTIME, (_XOPEN_REALTIME), (-1)))
#define __z_posix_sysconf_SC_XOPEN_STREAMS                                                         \
	(COND_CODE_1(CONFIG_XOPEN_STREAMS, (_XOPEN_STREAMS), (-1)))
#define __z_posix_sysconf_SC_XOPEN_REALTIME_THREADS                                                \
	(COND_CODE_1(CONFIG_XOPEN_REALTIME_THREADS, (_XOPEN_REALTIME_THREADS), (-1)))
#define __z_posix_sysconf_SC_XOPEN_UNIX (COND_CODE_1(CONFIG_XOPEN_UNIX, (_XOPEN_UNIX), (-1)))
#define __z_posix_sysconf_SC_XOPEN_UUCP (COND_CODE_1(CONFIG_XOPEN_UUCP, (_XOPEN_UUCP), (-1)))

enum {
	_SC_ADVISORY_INFO,
	_SC_ASYNCHRONOUS_IO,
	_SC_BARRIERS,
	_SC_CLOCK_SELECTION,
	_SC_CPUTIME,
	_SC_FSYNC,
	_SC_IPV6,
	_SC_JOB_CONTROL,
	_SC_MAPPED_FILES,
	_SC_MEMLOCK,
	_SC_MEMLOCK_RANGE,
	_SC_MEMORY_PROTECTION,
	_SC_MESSAGE_PASSING,
	_SC_MONOTONIC_CLOCK,
	_SC_PRIORITIZED_IO,
	_SC_PRIORITY_SCHEDULING,
	_SC_RAW_SOCKETS,
	_SC_RE_DUP_MAX,
	_SC_READER_WRITER_LOCKS,
	_SC_REALTIME_SIGNALS,
	_SC_REGEXP,
	_SC_SAVED_IDS,
	_SC_SEMAPHORES,
	_SC_SHARED_MEMORY_OBJECTS,
	_SC_SHELL,
	_SC_SPAWN,
	_SC_SPIN_LOCKS,
	_SC_SPORADIC_SERVER,
	_SC_SS_REPL_MAX,
	_SC_SYNCHRONIZED_IO,
	_SC_THREAD_ATTR_STACKADDR,
	_SC_THREAD_ATTR_STACKSIZE,
	_SC_THREAD_CPUTIME,
	_SC_THREAD_PRIO_INHERIT,
	_SC_THREAD_PRIO_PROTECT,
	_SC_THREAD_PRIORITY_SCHEDULING,
	_SC_THREAD_PROCESS_SHARED,
	_SC_THREAD_ROBUST_PRIO_INHERIT,
	_SC_THREAD_ROBUST_PRIO_PROTECT,
	_SC_THREAD_SAFE_FUNCTIONS,
	_SC_THREAD_SPORADIC_SERVER,
	_SC_THREADS,
	_SC_TIMEOUTS,
	_SC_TIMERS,
	_SC_TRACE,
	_SC_TRACE_EVENT_FILTER,
	_SC_TRACE_EVENT_NAME_MAX,
	_SC_TRACE_INHERIT,
	_SC_TRACE_LOG,
	_SC_TRACE_NAME_MAX,
	_SC_TRACE_SYS_MAX,
	_SC_TRACE_USER_EVENT_MAX,
	_SC_TYPED_MEMORY_OBJECTS,
	_SC_VERSION,
	_SC_V7_ILP32_OFF32,
	_SC_V7_ILP32_OFFBIG,
	_SC_V7_LP64_OFF64,
	_SC_V7_LPBIG_OFFBIG,
	_SC_V6_ILP32_OFF32,
	_SC_V6_ILP32_OFFBIG,
	_SC_V6_LP64_OFF64,
	_SC_V6_LPBIG_OFFBIG,
	_SC_BC_BASE_MAX,
	_SC_BC_DIM_MAX,
	_SC_BC_SCALE_MAX,
	_SC_BC_STRING_MAX,
	_SC_2_C_BIND,
	_SC_2_C_DEV,
	_SC_2_CHAR_TERM,
	_SC_COLL_WEIGHTS_MAX,
	_SC_DELAYTIMER_MAX,
	_SC_EXPR_NEST_MAX,
	_SC_2_FORT_DEV,
	_SC_2_FORT_RUN,
	_SC_LINE_MAX,
	_SC_2_LOCALEDEF,
	_SC_2_PBS,
	_SC_2_PBS_ACCOUNTING,
	_SC_2_PBS_CHECKPOINT,
	_SC_2_PBS_LOCATE,
	_SC_2_PBS_MESSAGE,
	_SC_2_PBS_TRACK,
	_SC_2_SW_DEV,
	_SC_2_UPE,
	_SC_2_VERSION,
	_SC_XOPEN_CRYPT,
	_SC_XOPEN_ENH_I18N,
	_SC_XOPEN_REALTIME,
	_SC_XOPEN_REALTIME_THREADS,
	_SC_XOPEN_SHM,
	_SC_XOPEN_STREAMS,
	_SC_XOPEN_UNIX,
	_SC_XOPEN_UUCP,
	_SC_XOPEN_VERSION,
	_SC_CLK_TCK,
	_SC_GETGR_R_SIZE_MAX,
	_SC_GETPW_R_SIZE_MAX,
	_SC_AIO_LISTIO_MAX,
	_SC_AIO_MAX,
	_SC_AIO_PRIO_DELTA_MAX,
	_SC_ARG_MAX,
	_SC_ATEXIT_MAX,
	_SC_CHILD_MAX,
	_SC_HOST_NAME_MAX,
	_SC_IOV_MAX,
	_SC_LOGIN_NAME_MAX,
	_SC_NGROUPS_MAX,
	_SC_MQ_OPEN_MAX,
	_SC_MQ_PRIO_MAX,
	_SC_OPEN_MAX,
	_SC_PAGE_SIZE,
	_SC_PAGESIZE,
	_SC_THREAD_DESTRUCTOR_ITERATIONS,
	_SC_THREAD_KEYS_MAX,
	_SC_THREAD_STACK_MIN,
	_SC_THREAD_THREADS_MAX,
	_SC_RTSIG_MAX,
	_SC_SEM_NSEMS_MAX,
	_SC_SEM_VALUE_MAX,
	_SC_SIGQUEUE_MAX,
	_SC_STREAM_MAX,
	_SC_SYMLOOP_MAX,
	_SC_TIMER_MAX,
	_SC_TTY_NAME_MAX,
	_SC_TZNAME_MAX,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_ */
