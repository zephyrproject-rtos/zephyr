/* limits.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_

#ifdef __cplusplus
extern "C" {
#endif

#if __CHAR_BIT__ == 8
#define UCHAR_MAX	0xFFU
#else
#error "unexpected __CHAR_BIT__ value"
#endif

#define SCHAR_MAX	__SCHAR_MAX__
#define SCHAR_MIN	(-SCHAR_MAX - 1)

#ifdef __CHAR_UNSIGNED__
	/* 'char' is an unsigned type */
	#define CHAR_MAX  UCHAR_MAX
	#define CHAR_MIN  0
#else
	/* 'char' is a signed type */
	#define CHAR_MAX  SCHAR_MAX
	#define CHAR_MIN  SCHAR_MIN
#endif

#define CHAR_BIT	__CHAR_BIT__
#define LONG_BIT	(__SIZEOF_LONG__ * CHAR_BIT)
#define WORD_BIT	(__SIZEOF_POINTER__ * CHAR_BIT)

#define INT_MAX		__INT_MAX__
#define SHRT_MAX	__SHRT_MAX__
#define LONG_MAX	__LONG_MAX__
#define LLONG_MAX	__LONG_LONG_MAX__

#define INT_MIN		(-INT_MAX - 1)
#define SHRT_MIN	(-SHRT_MAX - 1)
#define LONG_MIN	(-LONG_MAX - 1L)
#define LLONG_MIN	(-LLONG_MAX - 1LL)

#if __SIZE_MAX__ == __UINT32_MAX__
#define SSIZE_MAX	__INT32_MAX__
#elif __SIZE_MAX__ == __UINT64_MAX__
#define SSIZE_MAX	__INT64_MAX__
#else
#error "unexpected __SIZE_MAX__ value"
#endif

#if __SIZEOF_SHORT__ == 2
#define USHRT_MAX	0xFFFFU
#else
#error "unexpected __SIZEOF_SHORT__ value"
#endif

#if __SIZEOF_INT__ == 4
#define UINT_MAX	0xFFFFFFFFU
#else
#error "unexpected __SIZEOF_INT__ value"
#endif

#if __SIZEOF_LONG__ == 4
#define ULONG_MAX	0xFFFFFFFFUL
#elif __SIZEOF_LONG__ == 8
#define ULONG_MAX	0xFFFFFFFFFFFFFFFFUL
#else
#error "unexpected __SIZEOF_LONG__ value"
#endif

#if __SIZEOF_LONG_LONG__ == 8
#define ULLONG_MAX	0xFFFFFFFFFFFFFFFFULL
#else
#error "unexpected __SIZEOF_LONG_LONG__ value"
#endif

#if defined(_POSIX_C_SOURCE)

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

#else /* defined(_POSIX_C_SOURCE) */

#define PATH_MAX 256

#endif /* defined(_POSIX_C_SOURCE) */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_ */
