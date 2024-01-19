/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif

/**
 * @brief existence test for `<unistd.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html">unistd.h</a>
 */
ZTEST(posix_headers, test_unistd_h)
{
	/* zassert_not_equal(-1, _POSIX_VERSION); */ /* not implemented */
	/* zassert_not_equal(-1, _POSIX2_VERSION); */ /* not implemented */
	/* zassert_not_equal(-1, _XOPEN_VERSION); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _POSIX_ADVISORY_INFO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_ASYNCHRONOUS_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_BARRIERS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_CHOWN_RESTRICTED); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_CPUTIME); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_FSYNC); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_IPV6); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_JOB_CONTROL); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_MAPPED_FILES); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_MEMLOCK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_MEMLOCK_RANGE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_MEMORY_PROTECTION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_MESSAGE_PASSING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_MONOTONIC_CLOCK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_NO_TRUNC); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_PRIORITIZED_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_PRIORITY_SCHEDULING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_RAW_SOCKETS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_READER_WRITER_LOCKS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_REALTIME_SIGNALS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_REGEXP); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SAVED_IDS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SHARED_MEMORY_OBJECTS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SHELL); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SPAWN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SPIN_LOCKS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SPORADIC_SERVER); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SYNCHRONIZED_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_ATTR_STACKADDR); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_ATTR_STACKSIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_CPUTIME); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_PRIO_INHERIT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_PRIO_PROTECT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_PRIORITY_SCHEDULING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_PROCESS_SHARED); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_ROBUST_PRIO_INHERIT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_ROBUST_PRIO_PROTECT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_SAFE_FUNCTIONS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREAD_SPORADIC_SERVER); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_THREADS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TIMEOUTS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TIMERS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TRACE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TRACE_EVENT_FILTER); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TRACE_INHERIT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TRACE_LOG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_TYPED_MEMORY_OBJECTS); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _POSIX_V6_ILP32_OFF32); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_V6_ILP32_OFFBIG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_V6_LP64_OFF64); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_V6_LPBIG_OFFBIG); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _POSIX_V7_ILP32_OFF32); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_V7_ILP32_OFFBIG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_V7_LP64_OFF64); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_V7_LPBIG_OFFBIG); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _POSIX2_C_BIND); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_C_DEV); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_CHAR_TERM); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_FORT_DEV); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_FORT_RUN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_LOCALEDEF); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_PBS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_PBS_ACCOUNTING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_PBS_CHECKPOINT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_PBS_LOCATE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_PBS_MESSAGE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_PBS_TRACK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_SW_DEV); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_UPE); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _XOPEN_CRYPT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_ENH_I18N); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_REALTIME); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_REALTIME_THREADS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_SHM); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_STREAMS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_UNIX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _XOPEN_UUCP); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _POSIX_ASYNC_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_PRIO_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX_SYNC_IO); */ /* not implemented */

	/* zassert_not_equal(-1, _POSIX_TIMESTAMP_RESOLUTION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _POSIX2_SYMLINKS); */ /* not implemented */

	/* zassert_not_equal(-1, F_OK); */ /* not implemented */
	/* zassert_not_equal(-1, R_OK); */ /* not implemented */
	/* zassert_not_equal(-1, W_OK); */ /* not implemented */
	/* zassert_not_equal(-1, X_OK); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _CS_PATH); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFF32_CFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFF32_LDFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFF32_LIBS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFFBIG_LIBS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_LP64_OFF64_CFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_LP64_OFF64_LDFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_LP64_OFF64_LIBS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_LPBIG_OFFBIG_LIBS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_THREADS_CFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_THREADS_LDFLAGS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _CS_V7_ENV); */ /* not implemented */

	/* zassert_not_equal(-1, F_LOCK); */ /* not implemented */
	/* zassert_not_equal(-1, F_TEST); */ /* not implemented */
	/* zassert_not_equal(-1, F_TLOCK); */ /* not implemented */
	/* zassert_not_equal(-1, F_ULOCK); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _PC_2_SYMLINKS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_ALLOC_SIZE_MIN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_ASYNC_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_CHOWN_RESTRICTED); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_FILESIZEBITS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_LINK_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_MAX_CANON); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_MAX_INPUT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_NAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_NO_TRUNC); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_PATH_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_PIPE_BUF); */ /* not imp``lemented */
	/* zassert_not_equal(INT_MIN, _PC_PRIO_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_REC_INCR_XFER_SIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_REC_MAX_XFER_SIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_REC_MIN_XFER_SIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_REC_XFER_ALIGN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_SYMLINK_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_SYNC_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_TIMESTAMP_RESOLUTION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _PC_VDISABLE); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _SC_2_C_BIND); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_C_DEV); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_CHAR_TERM); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_FORT_DEV); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_FORT_RUN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_LOCALEDEF); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_PBS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_PBS_ACCOUNTING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_PBS_CHECKPOINT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_PBS_LOCATE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_PBS_MESSAGE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_PBS_TRACK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_SW_DEV); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_UPE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_2_VERSION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_ADVISORY_INFO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_AIO_LISTIO_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_AIO_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_AIO_PRIO_DELTA_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_ARG_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_ASYNCHRONOUS_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_ATEXIT_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_BARRIERS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_BC_BASE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_BC_DIM_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_BC_SCALE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_BC_STRING_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_CHILD_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_CLK_TCK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_CLOCK_SELECTION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_COLL_WEIGHTS_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_CPUTIME); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_DELAYTIMER_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_EXPR_NEST_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_FSYNC); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_GETGR_R_SIZE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_GETPW_R_SIZE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_HOST_NAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_IOV_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_IPV6); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_JOB_CONTROL); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_LINE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_LOGIN_NAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MAPPED_FILES); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MEMLOCK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MEMLOCK_RANGE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MEMORY_PROTECTION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MESSAGE_PASSING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MONOTONIC_CLOCK); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MQ_OPEN_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_MQ_PRIO_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_NGROUPS_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_OPEN_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_PAGE_SIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_PAGESIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_PRIORITIZED_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_PRIORITY_SCHEDULING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_RAW_SOCKETS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_RE_DUP_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_READER_WRITER_LOCKS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_REALTIME_SIGNALS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_REGEXP); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_RTSIG_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SAVED_IDS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SEM_NSEMS_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SEM_VALUE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SEMAPHORES); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SHARED_MEMORY_OBJECTS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SHELL); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SIGQUEUE_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SPAWN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SPIN_LOCKS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SPORADIC_SERVER); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SS_REPL_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_STREAM_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SYMLOOP_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_SYNCHRONIZED_IO); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_ATTR_STACKADDR); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_ATTR_STACKSIZE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_CPUTIME); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_DESTRUCTOR_ITERATIONS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_KEYS_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_PRIO_INHERIT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_PRIO_PROTECT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_PRIORITY_SCHEDULING); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_PROCESS_SHARED); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_ROBUST_PRIO_INHERIT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_ROBUST_PRIO_PROTECT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_SAFE_FUNCTIONS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_SPORADIC_SERVER); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_STACK_MIN); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREAD_THREADS_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_THREADS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TIMEOUTS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TIMER_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TIMERS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_EVENT_FILTER); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_EVENT_NAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_INHERIT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_LOG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_NAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_SYS_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TRACE_USER_EVENT_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TTY_NAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TYPED_MEMORY_OBJECTS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_TZNAME_MAX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V7_ILP32_OFF32); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V7_ILP32_OFFBIG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V7_LP64_OFF64); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V7_LPBIG_OFFBIG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V6_ILP32_OFF32); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V6_ILP32_OFFBIG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V6_LP64_OFF64); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_V6_LPBIG_OFFBIG); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_VERSION); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_CRYPT); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_ENH_I18N); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_REALTIME); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_REALTIME_THREADS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_SHM); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_STREAMS); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_UNIX); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_UUCP); */ /* not implemented */
	/* zassert_not_equal(INT_MIN, _SC_XOPEN_VERSION); */ /* not implemented */

	/* zassert_equal(STDERR_FILENO, 2); */ /* not implemented */
	/* zassert_equal(STDIN_FILENO, 0); */ /* not implemented */
	/* zassert_equal(STDOUT_FILENO, 1); */ /* not implemented */

	/* zassert_not_equal(INT_MIN, _POSIX_VDISABLE); */ /* not implemented */

/*
 * FIXME: this should really use IS_ENABLED()
 * When CONFIG_POSIX_API is n-selected (i.e. POSIX headers can only be
 * included with <zephyr/posix/...>, i.e. are namespaced), then there
 * should be no reason to conditionally declare standard posix
 * function prototypes.
 */
#ifdef CONFIG_POSIX_API
	/* zassert_not_null(access); */ /* not implemented */
	/* zassert_not_null(alarm); */ /* not implemented */
	/* zassert_not_null(chdir); */ /* not implemented */
	/* zassert_not_null(chown); */ /* not implemented */
	zassert_not_null(close);
	/* zassert_not_null(confstr); */ /* not implemented */
	/* zassert_not_null(crypt); */ /* not implemented */
	/* zassert_not_null(dup); */ /* not implemented */
	/* zassert_not_null(dup2); */ /* not implemented */
	zassert_not_null(_exit);
	/* zassert_not_null(encrypt); */ /* not implemented */
	/* zassert_not_null(execl); */ /* not implemented */
	/* zassert_not_null(execle); */ /* not implemented */
	/* zassert_not_null(execlp); */ /* not implemented */
	/* zassert_not_null(execv); */ /* not implemented */
	/* zassert_not_null(execve); */ /* not implemented */
	/* zassert_not_null(execvp); */ /* not implemented */
	/* zassert_not_null(faccessat); */ /* not implemented */
	/* zassert_not_null(fchdir); */ /* not implemented */
	/* zassert_not_null(fchown); */ /* not implemented */
	/* zassert_not_null(fchownat); */ /* not implemented */
	/* zassert_not_null(fdatasync); */ /* not implemented */
	/* zassert_not_null(fexecve); */ /* not implemented */
	/* zassert_not_null(fork); */ /* not implemented */
	/* zassert_not_null(fpathconf); */ /* not implemented */
	/* zassert_not_null(fsync); */ /* not implemented */
	/* zassert_not_null(ftruncate); */ /* not implemented */
	/* zassert_not_null(getcwd); */ /* not implemented */
	/* zassert_not_null(getegid); */ /* not implemented */
	/* zassert_not_null(geteuid); */ /* not implemented */
	/* zassert_not_null(getgid); */ /* not implemented */
	/* zassert_not_null(getgroups); */ /* not implemented */
	/* zassert_not_null(gethostid); */ /* not implemented */
	/* zassert_not_null(gethostname); */ /* not implemented */
	/* zassert_not_null(getlogin); */ /* not implemented */
	/* zassert_not_null(getlogin_r); */ /* not implemented */
	zassert_not_null(getopt);
	/* zassert_not_null(getpgid); */ /* not implemented */
	/* zassert_not_null(getpgrp); */ /* not implemented */
	zassert_not_null(getpid);
	/* zassert_not_null(getppid); */ /* not implemented */
	/* zassert_not_null(getsid); */ /* not implemented */
	/* zassert_not_null(getuid); */ /* not implemented */
	/* zassert_not_null(isatty); */ /* not implemented */
	/* zassert_not_null(lchown); */ /* not implemented */
	/* zassert_not_null(link); */ /* not implemented */
	/* zassert_not_null(linkat); */ /* not implemented */
	/* zassert_not_null(lockf); */ /* not implemented */
	zassert_not_null(lseek);
	/* zassert_not_null(nice); */ /* not implemented */
	/* zassert_not_null(pathconf); */ /* not implemented */
	/* zassert_not_null(pause); */ /* not implemented */
	/* zassert_not_null(pipe); */ /* not implemented */
	/* zassert_not_null(pread); */ /* not implemented */
	/* zassert_not_null(pwrite); */ /* not implemented */
	zassert_not_null(read);
	/* zassert_not_null(readlink); */ /* not implemented */
	/* zassert_not_null(readlinkat); */ /* not implemented */
	/* zassert_not_null(rmdir); */ /* not implemented */
	/* zassert_not_null(setegid); */ /* not implemented */
	/* zassert_not_null(seteuid); */ /* not implemented */
	/* zassert_not_null(setgid); */ /* not implemented */
	/* zassert_not_null(setpgid); */ /* not implemented */
	/* zassert_not_null(setpgrp); */ /* not implemented */
	/* zassert_not_null(setregid); */ /* not implemented */
	/* zassert_not_null(setreuid); */ /* not implemented */
	/* zassert_not_null(setsid); */ /* not implemented */
	/* zassert_not_null(setuid); */ /* not implemented */
	zassert_not_null(sleep);
	/* zassert_not_null(swab); */ /* not implemented */
	/* zassert_not_null(symlink); */ /* not implemented */
	/* zassert_not_null(symlinkat); */ /* not implemented */
	/* zassert_not_null(sync); */ /* not implemented */
	/* zassert_not_null(sysconf); */ /* not implemented */
	/* zassert_not_null(tcgetpgrp); */ /* not implemented */
	/* zassert_not_null(tcsetpgrp); */ /* not implemented */
	/* zassert_not_null(truncate); */ /* not implemented */
	/* zassert_not_null(ttyname); */ /* not implemented */
	/* zassert_not_null(ttyname_r); */ /* not implemented */
	zassert_not_null(unlink);
	/* zassert_not_null(unlinkat); */ /* not implemented */
	zassert_not_null(write);
#endif
}
