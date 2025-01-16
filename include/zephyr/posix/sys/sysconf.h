/*
 * Copyright (c) 2024, Meta
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	COND_CODE_1(CONFIG_POSIX_READER_WRITER_LOCKS, (_POSIX_READER_WRITER_LOCKS), (-1L))
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
#define __z_posix_sysconf_SC_DELAYTIMER_MAX               _POSIX_DELAYTIMER_MAX
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
#define __z_posix_sysconf_SC_AIO_LISTIO_MAX               AIO_LISTIO_MAX
#define __z_posix_sysconf_SC_AIO_MAX                      AIO_MAX
#define __z_posix_sysconf_SC_AIO_PRIO_DELTA_MAX           AIO_PRIO_DELTA_MAX
#define __z_posix_sysconf_SC_ARG_MAX                      ARG_MAX
#define __z_posix_sysconf_SC_ATEXIT_MAX                   ATEXIT_MAX
#define __z_posix_sysconf_SC_CHILD_MAX                    CHILD_MAX
#define __z_posix_sysconf_SC_HOST_NAME_MAX                HOST_NAME_MAX
#define __z_posix_sysconf_SC_IOV_MAX                      IOV_MAX
#define __z_posix_sysconf_SC_LOGIN_NAME_MAX               LOGIN_NAME_MAX
#define __z_posix_sysconf_SC_NGROUPS_MAX                  _POSIX_NGROUPS_MAX
#define __z_posix_sysconf_SC_MQ_OPEN_MAX                  MQ_OPEN_MAX
#define __z_posix_sysconf_SC_MQ_PRIO_MAX                  MQ_PRIO_MAX
#define __z_posix_sysconf_SC_OPEN_MAX                     CONFIG_ZVFS_OPEN_MAX
#define __z_posix_sysconf_SC_PAGE_SIZE                    PAGE_SIZE
#define __z_posix_sysconf_SC_PAGESIZE                     PAGESIZE
#define __z_posix_sysconf_SC_THREAD_DESTRUCTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#define __z_posix_sysconf_SC_THREAD_KEYS_MAX              PTHREAD_KEYS_MAX
#define __z_posix_sysconf_SC_THREAD_STACK_MIN             PTHREAD_STACK_MIN
#define __z_posix_sysconf_SC_THREAD_THREADS_MAX           PTHREAD_THREADS_MAX
#define __z_posix_sysconf_SC_RTSIG_MAX                    RTSIG_MAX
#define __z_posix_sysconf_SC_SEM_NSEMS_MAX                SEM_NSEMS_MAX
#define __z_posix_sysconf_SC_SEM_VALUE_MAX                SEM_VALUE_MAX
#define __z_posix_sysconf_SC_SIGQUEUE_MAX                 SIGQUEUE_MAX
#define __z_posix_sysconf_SC_STREAM_MAX                   STREAM_MAX
#define __z_posix_sysconf_SC_SYMLOOP_MAX                  SYMLOOP_MAX
#define __z_posix_sysconf_SC_TIMER_MAX                    TIMER_MAX
#define __z_posix_sysconf_SC_TTY_NAME_MAX                 TTY_NAME_MAX
#define __z_posix_sysconf_SC_TZNAME_MAX                   TZNAME_MAX

#ifdef CONFIG_POSIX_SYSCONF_IMPL_MACRO
#define sysconf(x) (long)CONCAT(__z_posix_sysconf, x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_ */
