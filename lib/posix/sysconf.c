/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define SUPPORTED_POSIX1_VERSION 200809L

long sysconf(int name)
{
	int errcode = ENOSYS;
	long retval = -1;

	switch (name) {
	case _SC_2_C_BIND:
		break;
	case _SC_2_C_DEV:
		break;
	case _SC_2_CHAR_TERM:
		break;
	case _SC_2_FORT_DEV:
		break;
	case _SC_2_FORT_RUN:
		break;
	case _SC_2_LOCALEDEF:
		break;
	case _SC_2_PBS:
		break;
	case _SC_2_PBS_ACCOUNTING:
		break;
	case _SC_2_PBS_CHECKPOINT:
		break;
	case _SC_2_PBS_LOCATE:
		break;
	case _SC_2_PBS_MESSAGE:
		break;
	case _SC_2_PBS_TRACK:
		break;
	case _SC_2_SW_DEV:
		break;
	case _SC_2_UPE:
		break;
	case _SC_2_VERSION:
		break;
	case _SC_ADVISORY_INFO:
		break;
	case _SC_AIO_LISTIO_MAX:
		break;
	case _SC_AIO_MAX:
		break;
	case _SC_AIO_PRIO_DELTA_MAX:
		break;
	case _SC_ARG_MAX:
		break;
	case _SC_ASYNCHRONOUS_IO:
		break;
	case _SC_ATEXIT_MAX:
		break;
	case _SC_BARRIERS:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = SUPPORTED_POSIX1_VERSION;
#endif
		break;
	case _SC_BC_BASE_MAX:
		break;
	case _SC_BC_DIM_MAX:
		break;
	case _SC_BC_SCALE_MAX:
		break;
	case _SC_BC_STRING_MAX:
		break;
	case _SC_CHILD_MAX:
		break;
	case _SC_CLK_TCK:
		retval = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	case _SC_CLOCK_SELECTION:
		break;
	case _SC_COLL_WEIGHTS_MAX:
		break;
	case _SC_CPUTIME:
		break;
	case _SC_DELAYTIMER_MAX:
		break;
	case _SC_EXPR_NEST_MAX:
		break;
	case _SC_FSYNC:
		break;
	case _SC_GETGR_R_SIZE_MAX:
		break;
	case _SC_GETPW_R_SIZE_MAX:
		break;
	case _SC_HOST_NAME_MAX:
		break;
	case _SC_IOV_MAX:
		break;
	case _SC_IPV6:
		break;
	case _SC_JOB_CONTROL:
		break;
	case _SC_LINE_MAX:
		break;
	case _SC_LOGIN_NAME_MAX:
		break;
	case _SC_MAPPED_FILES:
		break;
	case _SC_MEMLOCK:
		break;
	case _SC_MEMLOCK_RANGE:
		break;
	case _SC_MEMORY_PROTECTION:
		break;
	case _SC_MESSAGE_PASSING:
		break;
	case _SC_MONOTONIC_CLOCK:
		break;
	case _SC_MQ_OPEN_MAX:
		break;
	case _SC_MQ_PRIO_MAX:
		break;
	case _SC_NGROUPS_MAX:
		break;
	case _SC_OPEN_MAX:
		break;
	case _SC_PAGE_SIZE:
		/* same as _SC_PAGESIZE */
	case _SC_PAGESIZE:
		break;
	case _SC_PRIORITIZED_IO:
		break;
	case _SC_PRIORITY_SCHEDULING:
		break;
	case _SC_RAW_SOCKETS:
		break;
	case _SC_RE_DUP_MAX:
		break;
	case _SC_READER_WRITER_LOCKS:
		break;
	case _SC_REALTIME_SIGNALS:
		break;
	case _SC_REGEXP:
		break;
	case _SC_RTSIG_MAX:
		break;
	case _SC_SAVED_IDS:
		break;
	case _SC_SEM_NSEMS_MAX:
		break;
	case _SC_SEM_VALUE_MAX:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = CONFIG_SEM_VALUE_MAX;
#endif
		break;
	case _SC_SEMAPHORES:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = SUPPORTED_POSIX1_VERSION;
#endif
		break;
	case _SC_SHARED_MEMORY_OBJECTS:
		break;
	case _SC_SHELL:
		break;
	case _SC_SIGQUEUE_MAX:
		break;
	case _SC_SPAWN:
		break;
	case _SC_SPIN_LOCKS:
		break;
	case _SC_SPORADIC_SERVER:
		break;
	case _SC_SS_REPL_MAX:
		break;
	case _SC_STREAM_MAX:
		break;
	case _SC_SYMLOOP_MAX:
		break;
	case _SC_SYNCHRONIZED_IO:
		break;
	case _SC_THREAD_ATTR_STACKADDR:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = SUPPORTED_POSIX1_VERSION;
#endif
		break;
	case _SC_THREAD_ATTR_STACKSIZE:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = SUPPORTED_POSIX1_VERSION;
#endif
		break;
	case _SC_THREAD_CPUTIME:
		break;
	case _SC_THREAD_DESTRUCTOR_ITERATIONS:
		break;
	case _SC_THREAD_KEYS_MAX:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = CONFIG_MAX_PTHREAD_KEY_COUNT;
#endif
		break;
	case _SC_THREAD_PRIO_INHERIT:
		break;
	case _SC_THREAD_PRIO_PROTECT:
		break;
	case _SC_THREAD_PRIORITY_SCHEDULING:
#ifdef CONFIG_PTHREAD_IPC
		errcode = 0;
		retval = SUPPORTED_POSIX1_VERSION;
#endif
		break;
	case _SC_THREAD_PROCESS_SHARED:
		break;
	case _SC_THREAD_ROBUST_PRIO_INHERIT:
		break;
	case _SC_THREAD_ROBUST_PRIO_PROTECT:
		break;
	case _SC_THREAD_SAFE_FUNCTIONS:
		break;
	case _SC_THREAD_SPORADIC_SERVER:
		break;
	case _SC_THREAD_STACK_MIN:
#ifdef CONFIG_POSIX_API
		errcode = 0;
		retval = PTHREAD_STACK_MIN;
#endif
		break;
	case _SC_THREAD_THREADS_MAX:
		break;
	case _SC_THREADS:
		break;
	case _SC_TIMEOUTS:
		break;
	case _SC_TIMER_MAX:
#ifdef CONFIG_POSIX_CLOCK
		errno = 0;
		retval = CONFIG_MAX_TIMER_COUNT;
#endif
		break;
	case _SC_TIMERS:
		break;
	case _SC_TRACE:
		break;
	case _SC_TRACE_EVENT_FILTER:
		break;
	case _SC_TRACE_EVENT_NAME_MAX:
		break;
	case _SC_TRACE_INHERIT:
		break;
	case _SC_TRACE_LOG:
		break;
	case _SC_TRACE_NAME_MAX:
		break;
	case _SC_TRACE_SYS_MAX:
		break;
	case _SC_TRACE_USER_EVENT_MAX:
		break;
	case _SC_TTY_NAME_MAX:
		break;
	case _SC_TYPED_MEMORY_OBJECTS:
		break;
	case _SC_TZNAME_MAX:
		break;
	case _SC_V7_ILP32_OFF32:
		break;
	case _SC_V7_ILP32_OFFBIG:
		break;
	case _SC_V7_LP64_OFF64:
		break;
	case _SC_V7_LPBIG_OFFBIG:
		break;
	case _SC_V6_ILP32_OFF32:
		break;
	case _SC_V6_ILP32_OFFBIG:
		break;
	case _SC_V6_LP64_OFF64:
		break;
	case _SC_V6_LPBIG_OFFBIG:
		break;
	case _SC_VERSION:
		errcode = 0;
		retval = 200809L;
		break;
	case _SC_XOPEN_CRYPT:
		break;
	case _SC_XOPEN_ENH_I18N:
		break;
	case _SC_XOPEN_REALTIME:
		break;
	case _SC_XOPEN_REALTIME_THREADS:
		break;
	case _SC_XOPEN_SHM:
		break;
	case _SC_XOPEN_STREAMS:
		break;
	case _SC_XOPEN_UNIX:
		break;
	case _SC_XOPEN_UUCP:
		break;
	case _SC_XOPEN_VERSION:
		break;
	default:
		errcode = EINVAL;
		break;
	}

	errno = errcode;
	return retval;
}
