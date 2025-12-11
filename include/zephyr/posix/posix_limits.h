/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_POSIX_POSIX_LIMITS_H_
#define ZEPHYR_INCLUDE_ZEPHYR_POSIX_POSIX_LIMITS_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

/*
 * clang-format and checkpatch disagree on formatting here, so rely on checkpatch and disable
 * clang-format since checkpatch cannot be selectively disabled.
 */

/* clang-format off */

/* Maximum values */
#define _POSIX_CLOCKRES_MIN (20000000L)

/* Minimum values */
#define _POSIX_AIO_LISTIO_MAX               (2)
#define _POSIX_AIO_MAX                      (1)
#define _POSIX_ARG_MAX                      (4096)
#define _POSIX_CHILD_MAX                    (25)
#define _POSIX_DELAYTIMER_MAX               (32)
#define _POSIX_HOST_NAME_MAX                (255)
#define _POSIX_LINK_MAX                     (8)
#define _POSIX_LOGIN_NAME_MAX               (9)
#define _POSIX_MAX_CANON                    (255)
#define _POSIX_MAX_INPUT                    (255)
#define _POSIX_MQ_OPEN_MAX                  (8)
#define _POSIX_MQ_PRIO_MAX                  (32)
#define _POSIX_NAME_MAX                     (14)
#define _POSIX_NGROUPS_MAX                  (8)
#define _POSIX_OPEN_MAX                     (20)
#define _POSIX_PATH_MAX                     (256)
#define _POSIX_PIPE_BUF                     (512)
#define _POSIX_RE_DUP_MAX                   (255)
#define _POSIX_RTSIG_MAX                    (8)
#define _POSIX_SEM_NSEMS_MAX                (256)
#define _POSIX_SEM_VALUE_MAX                (32767)
#define _POSIX_SIGQUEUE_MAX                 (32)
#define _POSIX_SSIZE_MAX                    (32767)
#define _POSIX_SS_REPL_MAX                  (4)
#define _POSIX_STREAM_MAX                   (8)
#define _POSIX_SYMLINK_MAX                  (255)
#define _POSIX_SYMLOOP_MAX                  (8)
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS (4)
#define _POSIX_THREAD_KEYS_MAX              (128)
#define _POSIX_THREAD_THREADS_MAX           (64)
#define _POSIX_TIMER_MAX                    (32)
#define _POSIX_TRACE_EVENT_NAME_MAX         (30)
#define _POSIX_TRACE_NAME_MAX               (8)
#define _POSIX_TRACE_SYS_MAX                (8)
#define _POSIX_TRACE_USER_EVENT_MAX         (32)
#define _POSIX_TTY_NAME_MAX                 (9)
#define _POSIX_TZNAME_MAX                   (6)
#define _POSIX2_BC_BASE_MAX                 (99)
#define _POSIX2_BC_DIM_MAX                  (2048)
#define _POSIX2_BC_SCALE_MAX                (99)
#define _POSIX2_BC_STRING_MAX               (1000)
#define _POSIX2_CHARCLASS_NAME_MAX          (14)
#define _POSIX2_COLL_WEIGHTS_MAX            (2)
#define _POSIX2_EXPR_NEST_MAX               (32)
#define _POSIX2_LINE_MAX                    (2048)
#define _XOPEN_IOV_MAX                      (16)
#define _XOPEN_NAME_MAX                     (255)
#define _XOPEN_PATH_MAX                     (1024)

/* Other invariant values */
#define NL_LANGMAX (14)
#define NL_MSGMAX  (32767)
#define NL_SETMAX  (255)
#define NL_TEXTMAX (_POSIX2_LINE_MAX)
#define NZERO      (20)

/* Runtime invariant values */
#define AIO_LISTIO_MAX                _POSIX_AIO_LISTIO_MAX
#define AIO_MAX                       _POSIX_AIO_MAX
#define AIO_PRIO_DELTA_MAX            (0)
#define ARG_MAX                       _POSIX_ARG_MAX
#define ATEXIT_MAX                    (32)
#define DELAYTIMER_MAX \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_DELAYTIMER_MAX), (0))
#define HOST_NAME_MAX \
	COND_CODE_1(CONFIG_POSIX_NETWORKING, (CONFIG_POSIX_HOST_NAME_MAX), (0))
#define LOGIN_NAME_MAX                _POSIX_LOGIN_NAME_MAX
#define MQ_OPEN_MAX \
	COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (CONFIG_POSIX_MQ_OPEN_MAX), (0))
#define MQ_PRIO_MAX                   _POSIX_MQ_PRIO_MAX
#define OPEN_MAX                      CONFIG_POSIX_OPEN_MAX
#define PAGE_SIZE                     CONFIG_POSIX_PAGE_SIZE
#define PAGESIZE                      CONFIG_POSIX_PAGE_SIZE
#define PATH_MAX                      _POSIX_PATH_MAX
#define PTHREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_KEYS_MAX \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_KEYS_MAX), (0))
#define PTHREAD_THREADS_MAX \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_THREADS_MAX), (0))
#define RTSIG_MAX \
	COND_CODE_1(CONFIG_POSIX_REALTIME_SIGNALS, (CONFIG_POSIX_RTSIG_MAX), (0))
#define SEM_NSEMS_MAX \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_NSEMS_MAX), (0))
#define SEM_VALUE_MAX \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_VALUE_MAX), (0))
#define SIGQUEUE_MAX                  _POSIX_SIGQUEUE_MAX
#define STREAM_MAX                    _POSIX_STREAM_MAX
#define SYMLOOP_MAX                   _POSIX_SYMLOOP_MAX
#define TIMER_MAX \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_TIMER_MAX), (0))
#define TTY_NAME_MAX                  _POSIX_TTY_NAME_MAX
#define TZNAME_MAX                    _POSIX_TZNAME_MAX

/* Pathname variable values */
#define FILESIZEBITS             (32)
#define POSIX_ALLOC_SIZE_MIN     (256)
#define POSIX_REC_INCR_XFER_SIZE (1024)
#define POSIX_REC_MAX_XFER_SIZE  (32767)
#define POSIX_REC_MIN_XFER_SIZE  (1)
#define POSIX_REC_XFER_ALIGN     (4)
#define SYMLINK_MAX              _POSIX_SYMLINK_MAX

/* clang-format on */

#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_POSIX_POSIX_LIMITS_H_ */
