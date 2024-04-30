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

#ifdef CONFIG_POSIX_SYSCONF

enum {
	_SC_ADVISORY_INFO,
	_SC_ASYNCHRONOUS_IO,
	_SC_BARRIERS,
	_SC_CLOCK_SELECTION,
	_SC_CPUTIME,
	_SC_FSYNC,
	_SC_IPV6,
	_SC_JOB_CONTROL,
	_SC_MAPPED_FILE,
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

#ifdef CONFIG_POSIX_SYSCONF_IMPL_MACRO
#define __z_posix_sysconf_SC_ADVISORY_INFO                _POSIX_ADVISORY_INFO
#define __z_posix_sysconf_SC_ASYNCHRONOUS_IO              _POSIX_ASYNCHRONOUS_IO
#define __z_posix_sysconf_SC_BARRIERS                     _POSIX_BARRIERS
#define __z_posix_sysconf_SC_CLOCK_SELECTION              _POSIX_CLOCK_SELECTION
#define __z_posix_sysconf_SC_CPUTIME                      _POSIX_CPUTIME
#define __z_posix_sysconf_SC_FSYNC                        _POSIX_FSYNC
#define __z_posix_sysconf_SC_IPV6                         _POSIX_IPV6
#define __z_posix_sysconf_SC_JOB_CONTROL                  _POSIX_JOB_CONTROL
#define __z_posix_sysconf_SC_MAPPED_FILE                  _POSIX_MAPPED_FILES
#define __z_posix_sysconf_SC_MEMLOCK                      _POSIX_MEMLOCK
#define __z_posix_sysconf_SC_MEMLOCK_RANGE                _POSIX_MEMLOCK_RANGE
#define __z_posix_sysconf_SC_MEMORY_PROTECTION            _POSIX_MEMORY_PROTECTION
#define __z_posix_sysconf_SC_MESSAGE_PASSING              _POSIX_MESSAGE_PASSING
#define __z_posix_sysconf_SC_MONOTONIC_CLOCK              _POSIX_MONOTONIC_CLOCK
#define __z_posix_sysconf_SC_PRIORITIZED_IO               _POSIX_PRIORITIZED_IO
#define __z_posix_sysconf_SC_PRIORITY_SCHEDULING          _POSIX_PRIORITY_SCHEDULING
#define __z_posix_sysconf_SC_RAW_SOCKETS                  _POSIX_RAW_SOCKETS
#define __z_posix_sysconf_SC_RE_DUP_MAX                   _POSIX_RE_DUP_MAX
#define __z_posix_sysconf_SC_READER_WRITER_LOCKS          _POSIX_READER_WRITER_LOCKS
#define __z_posix_sysconf_SC_REALTIME_SIGNALS             _POSIX_REALTIME_SIGNALS
#define __z_posix_sysconf_SC_REGEXP                       _POSIX_REGEXP
#define __z_posix_sysconf_SC_SAVED_IDS                    _POSIX_SAVED_IDS
#define __z_posix_sysconf_SC_SEMAPHORES                   _POSIX_SEMAPHORES
#define __z_posix_sysconf_SC_SHARED_MEMORY_OBJECTS        _POSIX_SHARED_MEMORY_OBJECTS
#define __z_posix_sysconf_SC_SHELL                        _POSIX_SHELL
#define __z_posix_sysconf_SC_SPAWN                        _POSIX_SPAWN
#define __z_posix_sysconf_SC_SPIN_LOCKS                   _POSIX_SPIN_LOCKS
#define __z_posix_sysconf_SC_SPORADIC_SERVER              _POSIX_SPORADIC_SERVER
#define __z_posix_sysconf_SC_SS_REPL_MAX                  _POSIX_SS_REPL_MAX
#define __z_posix_sysconf_SC_SYNCHRONIZED_IO              _POSIX_SYNCHRONIZED_IO
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKADDR        _POSIX_THREAD_ATTR_STACKADDR
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKSIZE        _POSIX_THREAD_ATTR_STACKSIZE
#define __z_posix_sysconf_SC_THREAD_CPUTIME               _POSIX_THREAD_CPUTIME
#define __z_posix_sysconf_SC_THREAD_PRIO_INHERIT          _POSIX_THREAD_PRIO_INHERIT
#define __z_posix_sysconf_SC_THREAD_PRIO_PROTECT          _POSIX_THREAD_PRIO_PROTECT
#define __z_posix_sysconf_SC_THREAD_PRIORITY_SCHEDULING   _POSIX_THREAD_PRIORITY_SCHEDULING
#define __z_posix_sysconf_SC_THREAD_PROCESS_SHARED        _POSIX_THREAD_PROCESS_SHARED
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_INHERIT   _POSIX_THREAD_ROBUST_PRIO_INHERIT
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_PROTECT   _POSIX_THREAD_ROBUST_PRIO_PROTECT
#define __z_posix_sysconf_SC_THREAD_SAFE_FUNCTIONS        _POSIX_THREAD_SAFE_FUNCTIONS
#define __z_posix_sysconf_SC_THREAD_SPORADIC_SERVER       _POSIX_THREAD_SPORADIC_SERVER
#define __z_posix_sysconf_SC_THREADS                      _POSIX_THREADS
#define __z_posix_sysconf_SC_TIMEOUTS                     _POSIX_TIMEOUTS
#define __z_posix_sysconf_SC_TIMERS                       _POSIX_TIMERS
#define __z_posix_sysconf_SC_TRACE                        _POSIX_TRACE
#define __z_posix_sysconf_SC_TRACE_EVENT_FILTER           _POSIX_TRACE_EVENT_FILTER
#define __z_posix_sysconf_SC_TRACE_EVENT_NAME_MAX         _POSIX_TRACE_EVENT_NAME_MAX
#define __z_posix_sysconf_SC_TRACE_INHERIT                _POSIX_TRACE_INHERIT
#define __z_posix_sysconf_SC_TRACE_LOG                    _POSIX_TRACE_LOG
#define __z_posix_sysconf_SC_TRACE_NAME_MAX               _POSIX_TRACE_NAME_MAX
#define __z_posix_sysconf_SC_TRACE_SYS_MAX                _POSIX_TRACE_SYS_MAX
#define __z_posix_sysconf_SC_TRACE_USER_EVENT_MAX         _POSIX_TRACE_USER_EVENT_MAX
#define __z_posix_sysconf_SC_TYPED_MEMORY_OBJECTS         _POSIX_TYPED_MEMORY_OBJECTS
#define __z_posix_sysconf_SC_VERSION                      _POSIX_VERSION
#define __z_posix_sysconf_SC_V7_ILP32_OFF32               _POSIX_V7_ILP32_OFF32
#define __z_posix_sysconf_SC_V7_ILP32_OFFBIG              _POSIX_V7_ILP32_OFFBIG
#define __z_posix_sysconf_SC_V7_LP64_OFF64                _POSIX_V7_LP64_OFF64
#define __z_posix_sysconf_SC_V7_LPBIG_OFFBIG              _POSIX_V7_LPBIG_OFFBIG
#define __z_posix_sysconf_SC_V6_ILP32_OFF32               _POSIX_V6_ILP32_OFF32
#define __z_posix_sysconf_SC_V6_ILP32_OFFBIG              _POSIX_V6_ILP32_OFFBIG
#define __z_posix_sysconf_SC_V6_LP64_OFF64                _POSIX_V6_LP64_OFF64
#define __z_posix_sysconf_SC_V6_LPBIG_OFFBIG              _POSIX_V6_LPBIG_OFFBIG
#define __z_posix_sysconf_SC_BC_BASE_MAX                  _POSIX2_BC_BASE_MAX
#define __z_posix_sysconf_SC_BC_DIM_MAX                   _POSIX2_BC_DIM_MAX
#define __z_posix_sysconf_SC_BC_SCALE_MAX                 _POSIX2_BC_SCALE_MAX
#define __z_posix_sysconf_SC_BC_STRING_MAX                _POSIX2_BC_STRING_MAX
#define __z_posix_sysconf_SC_2_C_BIND                     _POSIX2_C_BIND
#define __z_posix_sysconf_SC_2_C_DEV                      _POSIX2_C_DEV
#define __z_posix_sysconf_SC_2_CHAR_TERM                  _POSIX2_CHAR_TERM
#define __z_posix_sysconf_SC_COLL_WEIGHTS_MAX             _POSIX2_COLL_WEIGHTS_MAX
#define __z_posix_sysconf_SC_DELAYTIMER_MAX               _POSIX2_DELAYTIMER_MAX
#define __z_posix_sysconf_SC_EXPR_NEST_MAX                _POSIX2_EXPR_NEST_MAX
#define __z_posix_sysconf_SC_2_FORT_DEV                   _POSIX2_FORT_DEV
#define __z_posix_sysconf_SC_2_FORT_RUN                   _POSIX2_FORT_RUN
#define __z_posix_sysconf_SC_LINE_MAX                     _POSIX2_LINE_MAX
#define __z_posix_sysconf_SC_2_LOCALEDEF                  _POSIX2_LOCALEDEF
#define __z_posix_sysconf_SC_2_PBS                        _POSIX2_PBS
#define __z_posix_sysconf_SC_2_PBS_ACCOUNTING             _POSIX2_PBS_ACCOUNTING
#define __z_posix_sysconf_SC_2_PBS_CHECKPOINT             _POSIX2_PBS_CHECKPOINT
#define __z_posix_sysconf_SC_2_PBS_LOCATE                 _POSIX2_PBS_LOCATE
#define __z_posix_sysconf_SC_2_PBS_MESSAGE                _POSIX2_PBS_MESSAGE
#define __z_posix_sysconf_SC_2_PBS_TRACK                  _POSIX2_PBS_TRACK
#define __z_posix_sysconf_SC_2_SW_DEV                     _POSIX2_SW_DEV
#define __z_posix_sysconf_SC_2_UPE                        _POSIX2_UPE
#define __z_posix_sysconf_SC_2_VERSION                    _POSIX2_VERSION
#define __z_posix_sysconf_SC_XOPEN_CRYPT                  _XOPEN_CRYPT
#define __z_posix_sysconf_SC_XOPEN_ENH_I18N               _XOPEN_ENH_I18N
#define __z_posix_sysconf_SC_XOPEN_REALTIME               _XOPEN_REALTIME
#define __z_posix_sysconf_SC_XOPEN_REALTIME_THREADS       _XOPEN_REALTIME_THREADS
#define __z_posix_sysconf_SC_XOPEN_SHM                    _XOPEN_SHM
#define __z_posix_sysconf_SC_XOPEN_STREAMS                _XOPEN_STREAMS
#define __z_posix_sysconf_SC_XOPEN_UNIX                   _XOPEN_UNIX
#define __z_posix_sysconf_SC_XOPEN_UUCP                   _XOPEN_UUCP
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
#define __z_posix_sysconf_SC_OPEN_MAX                     CONFIG_POSIX_MAX_FDS
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

#define sysconf(x) (long)CONCAT(__z_posix_sysconf, x)
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_ */
