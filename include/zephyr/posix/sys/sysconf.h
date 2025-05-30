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

/* Values assigned are intended to match the values assigned in newlib and picolib
 * Even though the POSIX standard does not require specific values, this seems to be
 * required for proper sysconf() operation when called from within newlib/picolib itself.
 */
#define _SC_ARG_MAX                      0
#define _SC_CHILD_MAX                    1
#define _SC_CLK_TCK                      2
#define _SC_NGROUPS_MAX                  3
#define _SC_OPEN_MAX                     4
#define _SC_JOB_CONTROL                  5
#define _SC_SAVED_IDS                    6
#define _SC_VERSION                      7
#define _SC_PAGESIZE                     8
#define _SC_PAGE_SIZE                    _SC_PAGESIZE
/* These are non-POSIX values we accidentally introduced in 2000 without
 * guarding them.  Keeping them unguarded for backward compatibility.
 */
#define _SC_NPROCESSORS_CONF             9
#define _SC_NPROCESSORS_ONLN             10
#define _SC_PHYS_PAGES                   11
#define _SC_AVPHYS_PAGES                 12
/* End of non-POSIX values. */
#define _SC_MQ_OPEN_MAX                  13
#define _SC_MQ_PRIO_MAX                  14
#define _SC_RTSIG_MAX                    15
#define _SC_SEM_NSEMS_MAX                16
#define _SC_SEM_VALUE_MAX                17
#define _SC_SIGQUEUE_MAX                 18
#define _SC_TIMER_MAX                    19
#define _SC_TZNAME_MAX                   20
#define _SC_ASYNCHRONOUS_IO              21
#define _SC_FSYNC                        22
#define _SC_MAPPED_FILES                 23
#define _SC_MEMLOCK                      24
#define _SC_MEMLOCK_RANGE                25
#define _SC_MEMORY_PROTECTION            26
#define _SC_MESSAGE_PASSING              27
#define _SC_PRIORITIZED_IO               28
#define _SC_REALTIME_SIGNALS             29
#define _SC_SEMAPHORES                   30
#define _SC_SHARED_MEMORY_OBJECTS        31
#define _SC_SYNCHRONIZED_IO              32
#define _SC_TIMERS                       33
#define _SC_AIO_LISTIO_MAX               34
#define _SC_AIO_MAX                      35
#define _SC_AIO_PRIO_DELTA_MAX           36
#define _SC_DELAYTIMER_MAX               37
#define _SC_THREAD_KEYS_MAX              38
#define _SC_THREAD_STACK_MIN             39
#define _SC_THREAD_THREADS_MAX           40
#define _SC_TTY_NAME_MAX                 41
#define _SC_THREADS                      42
#define _SC_THREAD_ATTR_STACKADDR        43
#define _SC_THREAD_ATTR_STACKSIZE        44
#define _SC_THREAD_PRIORITY_SCHEDULING   45
#define _SC_THREAD_PRIO_INHERIT          46
/* _SC_THREAD_PRIO_PROTECT was _SC_THREAD_PRIO_CEILING in early drafts */
#define _SC_THREAD_PRIO_PROTECT          47
#define _SC_THREAD_PRIO_CEILING          _SC_THREAD_PRIO_PROTECT
#define _SC_THREAD_PROCESS_SHARED        48
#define _SC_THREAD_SAFE_FUNCTIONS        49
#define _SC_GETGR_R_SIZE_MAX             50
#define _SC_GETPW_R_SIZE_MAX             51
#define _SC_LOGIN_NAME_MAX               52
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 53
#define _SC_ADVISORY_INFO                54
#define _SC_ATEXIT_MAX                   55
#define _SC_BARRIERS                     56
#define _SC_BC_BASE_MAX                  57
#define _SC_BC_DIM_MAX                   58
#define _SC_BC_SCALE_MAX                 59
#define _SC_BC_STRING_MAX                60
#define _SC_CLOCK_SELECTION              61
#define _SC_COLL_WEIGHTS_MAX             62
#define _SC_CPUTIME                      63
#define _SC_EXPR_NEST_MAX                64
#define _SC_HOST_NAME_MAX                65
#define _SC_IOV_MAX                      66
#define _SC_IPV6                         67
#define _SC_LINE_MAX                     68
#define _SC_MONOTONIC_CLOCK              69
#define _SC_RAW_SOCKETS                  70
#define _SC_READER_WRITER_LOCKS          71
#define _SC_REGEXP                       72
#define _SC_RE_DUP_MAX                   73
#define _SC_SHELL                        74
#define _SC_SPAWN                        75
#define _SC_SPIN_LOCKS                   76
#define _SC_SPORADIC_SERVER              77
#define _SC_SS_REPL_MAX                  78
#define _SC_SYMLOOP_MAX                  79
#define _SC_THREAD_CPUTIME               80
#define _SC_THREAD_SPORADIC_SERVER       81
#define _SC_TIMEOUTS                     82
#define _SC_TRACE                        83
#define _SC_TRACE_EVENT_FILTER           84
#define _SC_TRACE_EVENT_NAME_MAX         85
#define _SC_TRACE_INHERIT                86
#define _SC_TRACE_LOG                    87
#define _SC_TRACE_NAME_MAX               88
#define _SC_TRACE_SYS_MAX                89
#define _SC_TRACE_USER_EVENT_MAX         90
#define _SC_TYPED_MEMORY_OBJECTS         91
#define _SC_V7_ILP32_OFF32               92
#define _SC_V6_ILP32_OFF32               _SC_V7_ILP32_OFF32
#define _SC_XBS5_ILP32_OFF32             _SC_V7_ILP32_OFF32
#define _SC_V7_ILP32_OFFBIG              93
#define _SC_V6_ILP32_OFFBIG              _SC_V7_ILP32_OFFBIG
#define _SC_XBS5_ILP32_OFFBIG            _SC_V7_ILP32_OFFBIG
#define _SC_V7_LP64_OFF64                94
#define _SC_V6_LP64_OFF64                _SC_V7_LP64_OFF64
#define _SC_XBS5_LP64_OFF64              _SC_V7_LP64_OFF64
#define _SC_V7_LPBIG_OFFBIG              95
#define _SC_V6_LPBIG_OFFBIG              _SC_V7_LPBIG_OFFBIG
#define _SC_XBS5_LPBIG_OFFBIG            _SC_V7_LPBIG_OFFBIG
#define _SC_XOPEN_CRYPT                  96
#define _SC_XOPEN_ENH_I18N               97
#define _SC_XOPEN_LEGACY                 98
#define _SC_XOPEN_REALTIME               99
#define _SC_STREAM_MAX                   100
#define _SC_PRIORITY_SCHEDULING          101
#define _SC_XOPEN_REALTIME_THREADS       102
#define _SC_XOPEN_SHM                    103
#define _SC_XOPEN_STREAMS                104
#define _SC_XOPEN_UNIX                   105
#define _SC_XOPEN_VERSION                106
#define _SC_2_CHAR_TERM                  107
#define _SC_2_C_BIND                     108
#define _SC_2_C_DEV                      109
#define _SC_2_FORT_DEV                   110
#define _SC_2_FORT_RUN                   111
#define _SC_2_LOCALEDEF                  112
#define _SC_2_PBS                        113
#define _SC_2_PBS_ACCOUNTING             114
#define _SC_2_PBS_CHECKPOINT             115
#define _SC_2_PBS_LOCATE                 116
#define _SC_2_PBS_MESSAGE                117
#define _SC_2_PBS_TRACK                  118
#define _SC_2_SW_DEV                     119
#define _SC_2_UPE                        120
#define _SC_2_VERSION                    121
#define _SC_THREAD_ROBUST_PRIO_INHERIT   122
#define _SC_THREAD_ROBUST_PRIO_PROTECT   123
#define _SC_XOPEN_UUCP                   124
#define _SC_LEVEL1_ICACHE_SIZE           125
#define _SC_LEVEL1_ICACHE_ASSOC          126
#define _SC_LEVEL1_ICACHE_LINESIZE       127
#define _SC_LEVEL1_DCACHE_SIZE           128
#define _SC_LEVEL1_DCACHE_ASSOC          129
#define _SC_LEVEL1_DCACHE_LINESIZE       130
#define _SC_LEVEL2_CACHE_SIZE            131
#define _SC_LEVEL2_CACHE_ASSOC           132
#define _SC_LEVEL2_CACHE_LINESIZE        133
#define _SC_LEVEL3_CACHE_SIZE            134
#define _SC_LEVEL3_CACHE_ASSOC           135
#define _SC_LEVEL3_CACHE_LINESIZE        136
#define _SC_LEVEL4_CACHE_SIZE            137
#define _SC_LEVEL4_CACHE_ASSOC           138
#define _SC_LEVEL4_CACHE_LINESIZE        139
#define _SC_POSIX_26_VERSION             140

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SYSCONF_H_ */
