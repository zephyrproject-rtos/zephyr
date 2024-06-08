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
	/* zassert_not_equal(-1, F_OK); */ /* not implemented */
	/* zassert_not_equal(-1, R_OK); */ /* not implemented */
	/* zassert_not_equal(-1, W_OK); */ /* not implemented */
	/* zassert_not_equal(-1, X_OK); */ /* not implemented */

	zassert_not_equal(INT_MIN, _CS_PATH);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFF32_CFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFF32_LDFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFF32_LIBS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_ILP32_OFFBIG_LIBS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_LP64_OFF64_CFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_LP64_OFF64_LDFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_LP64_OFF64_LIBS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_LPBIG_OFFBIG_LIBS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_THREADS_CFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_THREADS_LDFLAGS);
	zassert_not_equal(INT_MIN, _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS);
	zassert_not_equal(INT_MIN, _CS_V7_ENV);

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

	zassert_not_equal(INT_MIN, _SC_2_C_BIND);
	zassert_not_equal(INT_MIN, _SC_2_C_DEV);
	zassert_not_equal(INT_MIN, _SC_2_CHAR_TERM);
	zassert_not_equal(INT_MIN, _SC_2_FORT_DEV);
	zassert_not_equal(INT_MIN, _SC_2_FORT_RUN);
	zassert_not_equal(INT_MIN, _SC_2_LOCALEDEF);
	zassert_not_equal(INT_MIN, _SC_2_PBS);
	zassert_not_equal(INT_MIN, _SC_2_PBS_ACCOUNTING);
	zassert_not_equal(INT_MIN, _SC_2_PBS_CHECKPOINT);
	zassert_not_equal(INT_MIN, _SC_2_PBS_LOCATE);
	zassert_not_equal(INT_MIN, _SC_2_PBS_MESSAGE);
	zassert_not_equal(INT_MIN, _SC_2_PBS_TRACK);
	zassert_not_equal(INT_MIN, _SC_2_SW_DEV);
	zassert_not_equal(INT_MIN, _SC_2_UPE);
	zassert_not_equal(INT_MIN, _SC_2_VERSION);
	zassert_not_equal(INT_MIN, _SC_ADVISORY_INFO);
	zassert_not_equal(INT_MIN, _SC_AIO_LISTIO_MAX);
	zassert_not_equal(INT_MIN, _SC_AIO_MAX);
	zassert_not_equal(INT_MIN, _SC_AIO_PRIO_DELTA_MAX);
	zassert_not_equal(INT_MIN, _SC_ARG_MAX);
	zassert_not_equal(INT_MIN, _SC_ASYNCHRONOUS_IO);
	zassert_not_equal(INT_MIN, _SC_ATEXIT_MAX);
	zassert_not_equal(INT_MIN, _SC_BARRIERS);
	zassert_not_equal(INT_MIN, _SC_BC_BASE_MAX);
	zassert_not_equal(INT_MIN, _SC_BC_DIM_MAX);
	zassert_not_equal(INT_MIN, _SC_BC_SCALE_MAX);
	zassert_not_equal(INT_MIN, _SC_BC_STRING_MAX);
	zassert_not_equal(INT_MIN, _SC_CHILD_MAX);
	zassert_not_equal(INT_MIN, _SC_CLK_TCK);
	zassert_not_equal(INT_MIN, _SC_CLOCK_SELECTION);
	zassert_not_equal(INT_MIN, _SC_COLL_WEIGHTS_MAX);
	zassert_not_equal(INT_MIN, _SC_CPUTIME);
	zassert_not_equal(INT_MIN, _SC_DELAYTIMER_MAX);
	zassert_not_equal(INT_MIN, _SC_EXPR_NEST_MAX);
	zassert_not_equal(INT_MIN, _SC_FSYNC);
	zassert_not_equal(INT_MIN, _SC_GETGR_R_SIZE_MAX);
	zassert_not_equal(INT_MIN, _SC_GETPW_R_SIZE_MAX);
	zassert_not_equal(INT_MIN, _SC_HOST_NAME_MAX);
	zassert_not_equal(INT_MIN, _SC_IOV_MAX);
	zassert_not_equal(INT_MIN, _SC_IPV6);
	zassert_not_equal(INT_MIN, _SC_JOB_CONTROL);
	zassert_not_equal(INT_MIN, _SC_LINE_MAX);
	zassert_not_equal(INT_MIN, _SC_LOGIN_NAME_MAX);
	zassert_not_equal(INT_MIN, _SC_MAPPED_FILES);
	zassert_not_equal(INT_MIN, _SC_MEMLOCK);
	zassert_not_equal(INT_MIN, _SC_MEMLOCK_RANGE);
	zassert_not_equal(INT_MIN, _SC_MEMORY_PROTECTION);
	zassert_not_equal(INT_MIN, _SC_MESSAGE_PASSING);
	zassert_not_equal(INT_MIN, _SC_MONOTONIC_CLOCK);
	zassert_not_equal(INT_MIN, _SC_MQ_OPEN_MAX);
	zassert_not_equal(INT_MIN, _SC_MQ_PRIO_MAX);
	zassert_not_equal(INT_MIN, _SC_NGROUPS_MAX);
	zassert_not_equal(INT_MIN, _SC_OPEN_MAX);
	zassert_not_equal(INT_MIN, _SC_PAGE_SIZE);
	zassert_not_equal(INT_MIN, _SC_PAGESIZE);
	zassert_not_equal(INT_MIN, _SC_PRIORITIZED_IO);
	zassert_not_equal(INT_MIN, _SC_PRIORITY_SCHEDULING);
	zassert_not_equal(INT_MIN, _SC_RAW_SOCKETS);
	zassert_not_equal(INT_MIN, _SC_RE_DUP_MAX);
	zassert_not_equal(INT_MIN, _SC_READER_WRITER_LOCKS);
	zassert_not_equal(INT_MIN, _SC_REALTIME_SIGNALS);
	zassert_not_equal(INT_MIN, _SC_REGEXP);
	zassert_not_equal(INT_MIN, _SC_RTSIG_MAX);
	zassert_not_equal(INT_MIN, _SC_SAVED_IDS);
	zassert_not_equal(INT_MIN, _SC_SEM_NSEMS_MAX);
	zassert_not_equal(INT_MIN, _SC_SEM_VALUE_MAX);
	zassert_not_equal(INT_MIN, _SC_SEMAPHORES);
	zassert_not_equal(INT_MIN, _SC_SHARED_MEMORY_OBJECTS);
	zassert_not_equal(INT_MIN, _SC_SHELL);
	zassert_not_equal(INT_MIN, _SC_SIGQUEUE_MAX);
	zassert_not_equal(INT_MIN, _SC_SPAWN);
	zassert_not_equal(INT_MIN, _SC_SPIN_LOCKS);
	zassert_not_equal(INT_MIN, _SC_SPORADIC_SERVER);
	zassert_not_equal(INT_MIN, _SC_SS_REPL_MAX);
	zassert_not_equal(INT_MIN, _SC_STREAM_MAX);
	zassert_not_equal(INT_MIN, _SC_SYMLOOP_MAX);
	zassert_not_equal(INT_MIN, _SC_SYNCHRONIZED_IO);
	zassert_not_equal(INT_MIN, _SC_THREAD_ATTR_STACKADDR);
	zassert_not_equal(INT_MIN, _SC_THREAD_ATTR_STACKSIZE);
	zassert_not_equal(INT_MIN, _SC_THREAD_CPUTIME);
	zassert_not_equal(INT_MIN, _SC_THREAD_DESTRUCTOR_ITERATIONS);
	zassert_not_equal(INT_MIN, _SC_THREAD_KEYS_MAX);
	zassert_not_equal(INT_MIN, _SC_THREAD_PRIO_INHERIT);
	zassert_not_equal(INT_MIN, _SC_THREAD_PRIO_PROTECT);
	zassert_not_equal(INT_MIN, _SC_THREAD_PRIORITY_SCHEDULING);
	zassert_not_equal(INT_MIN, _SC_THREAD_PROCESS_SHARED);
	zassert_not_equal(INT_MIN, _SC_THREAD_ROBUST_PRIO_INHERIT);
	zassert_not_equal(INT_MIN, _SC_THREAD_ROBUST_PRIO_PROTECT);
	zassert_not_equal(INT_MIN, _SC_THREAD_SAFE_FUNCTIONS);
	zassert_not_equal(INT_MIN, _SC_THREAD_SPORADIC_SERVER);
	zassert_not_equal(INT_MIN, _SC_THREAD_STACK_MIN);
	zassert_not_equal(INT_MIN, _SC_THREAD_THREADS_MAX);
	zassert_not_equal(INT_MIN, _SC_THREADS);
	zassert_not_equal(INT_MIN, _SC_TIMEOUTS);
	zassert_not_equal(INT_MIN, _SC_TIMER_MAX);
	zassert_not_equal(INT_MIN, _SC_TIMERS);
	zassert_not_equal(INT_MIN, _SC_TRACE);
	zassert_not_equal(INT_MIN, _SC_TRACE_EVENT_FILTER);
	zassert_not_equal(INT_MIN, _SC_TRACE_EVENT_NAME_MAX);
	zassert_not_equal(INT_MIN, _SC_TRACE_INHERIT);
	zassert_not_equal(INT_MIN, _SC_TRACE_LOG);
	zassert_not_equal(INT_MIN, _SC_TRACE_NAME_MAX);
	zassert_not_equal(INT_MIN, _SC_TRACE_SYS_MAX);
	zassert_not_equal(INT_MIN, _SC_TRACE_USER_EVENT_MAX);
	zassert_not_equal(INT_MIN, _SC_TTY_NAME_MAX);
	zassert_not_equal(INT_MIN, _SC_TYPED_MEMORY_OBJECTS);
	zassert_not_equal(INT_MIN, _SC_TZNAME_MAX);
	zassert_not_equal(INT_MIN, _SC_V7_ILP32_OFF32);
	zassert_not_equal(INT_MIN, _SC_V7_ILP32_OFFBIG);
	zassert_not_equal(INT_MIN, _SC_V7_LP64_OFF64);
	zassert_not_equal(INT_MIN, _SC_V7_LPBIG_OFFBIG);
	zassert_not_equal(INT_MIN, _SC_V6_ILP32_OFF32);
	zassert_not_equal(INT_MIN, _SC_V6_ILP32_OFFBIG);
	zassert_not_equal(INT_MIN, _SC_V6_LP64_OFF64);
	zassert_not_equal(INT_MIN, _SC_V6_LPBIG_OFFBIG);
	zassert_not_equal(INT_MIN, _SC_VERSION);
	zassert_not_equal(INT_MIN, _SC_XOPEN_CRYPT);
	zassert_not_equal(INT_MIN, _SC_XOPEN_ENH_I18N);
	zassert_not_equal(INT_MIN, _SC_XOPEN_REALTIME);
	zassert_not_equal(INT_MIN, _SC_XOPEN_REALTIME_THREADS);
	zassert_not_equal(INT_MIN, _SC_XOPEN_SHM);
	zassert_not_equal(INT_MIN, _SC_XOPEN_STREAMS);
	zassert_not_equal(INT_MIN, _SC_XOPEN_UNIX);
	zassert_not_equal(INT_MIN, _SC_XOPEN_UUCP);
	zassert_not_equal(INT_MIN, _SC_XOPEN_VERSION);

	/* zassert_equal(STDERR_FILENO, 2); */ /* not implemented */
	/* zassert_equal(STDIN_FILENO, 0); */ /* not implemented */
	/* zassert_equal(STDOUT_FILENO, 1); */ /* not implemented */

	zassert_not_equal(INT_MIN, _POSIX_VDISABLE);

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
