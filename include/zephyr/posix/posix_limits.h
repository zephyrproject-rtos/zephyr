/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Implementation-defined constants.
 * @ingroup posix
 *
 * Defines minimum, maximum, and runtime-invariant POSIX limits that complement
 * the C standard limits.h header and the runtime sysconf() and pathconf()
 * interfaces.
 *
 * @posix_header{limits.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_LIMITS_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_LIMITS_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

/*
 * clang-format and checkpatch disagree on formatting here, so rely on checkpatch and disable
 * clang-format since checkpatch cannot be selectively disabled.
 */

/* clang-format off */

/* Maximum values */

/**
 * @brief Maximum nanoseconds between clock ticks (20 ms).
 */
#define _POSIX_CLOCKRES_MIN (20000000L)

/* Minimum values */

/**
 * @brief Minimum number of I/O operations in a single list I/O call.
 */
#define _POSIX_AIO_LISTIO_MAX               (2)

/**
 * @brief Minimum number of outstanding asynchronous I/O operations.
 */
#define _POSIX_AIO_MAX                      (1)

/**
 * @brief Minimum length of arguments to the exec functions, including environment data.
 */
#define _POSIX_ARG_MAX                      (4096)

/**
 * @brief Minimum number of simultaneous processes per real user ID.
 */
#define _POSIX_CHILD_MAX                    (25)

/**
 * @brief Minimum number of timer expiration overruns.
 */
#define _POSIX_DELAYTIMER_MAX               (32)

/**
 * @brief Minimum length of a host name, not including the terminating null.
 */
#define _POSIX_HOST_NAME_MAX                (255)

/**
 * @brief Minimum number of links to a single file.
 */
#define _POSIX_LINK_MAX                     (8)

/**
 * @brief Minimum length of a login name.
 */
#define _POSIX_LOGIN_NAME_MAX               (9)

/**
 * @brief Minimum number of bytes in a terminal canonical input queue.
 */
#define _POSIX_MAX_CANON                    (255)

/**
 * @brief Minimum number of bytes in a terminal input queue.
 */
#define _POSIX_MAX_INPUT                    (255)

/**
 * @brief Minimum number of message queues that a process may hold open at one time.
 */
#define _POSIX_MQ_OPEN_MAX                  (8)

/**
 * @brief Minimum number of supported message priorities.
 */
#define _POSIX_MQ_PRIO_MAX                  (32)

/**
 * @brief Minimum number of bytes in a filename, not including the terminating null.
 */
#define _POSIX_NAME_MAX                     (14)

/**
 * @brief Minimum number of supplementary group IDs per process.
 */
#define _POSIX_NGROUPS_MAX                  (8)

/**
 * @brief Minimum number of files that a process may have open at one time.
 */
#define _POSIX_OPEN_MAX                     (20)

/**
 * @brief Minimum number of bytes in a pathname, including the terminating null.
 */
#define _POSIX_PATH_MAX                     (256)

/**
 * @brief Minimum number of bytes that can be written atomically to a pipe.
 */
#define _POSIX_PIPE_BUF                     (512)

/**
 * @brief Minimum number of repeated occurrences of a regular expression in an interval expression.
 */
#define _POSIX_RE_DUP_MAX                   (255)

/**
 * @brief Minimum number of realtime signals reserved for application use.
 */
#define _POSIX_RTSIG_MAX                    (8)

/**
 * @brief Minimum number of semaphores that a process may have.
 */
#define _POSIX_SEM_NSEMS_MAX                (256)

/**
 * @brief Minimum value that the maximum value of a semaphore may have.
 */
#define _POSIX_SEM_VALUE_MAX                (32767)

/**
 * @brief Minimum number of queued signals that a process may send and have pending.
 */
#define _POSIX_SIGQUEUE_MAX                 (32)

/**
 * @brief Value guaranteed to be storable in an object of type ssize_t.
 */
#define _POSIX_SSIZE_MAX                    (32767)

/**
 * @brief Minimum number of simultaneously pending replenishment operations for a sporadic server.
 */
#define _POSIX_SS_REPL_MAX                  (4)

/**
 * @brief Minimum number of streams that a process may have open at one time.
 */
#define _POSIX_STREAM_MAX                   (8)

/**
 * @brief Minimum number of bytes in a symbolic link.
 */
#define _POSIX_SYMLINK_MAX                  (255)

/**
 * @brief Minimum number of symbolic links traversable during pathname resolution.
 */
#define _POSIX_SYMLOOP_MAX                  (8)

/**
 * @brief Minimum number of attempts made to destroy thread-specific data on thread exit.
 */
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS (4)

/**
 * @brief Minimum number of thread-specific data keys per process.
 */
#define _POSIX_THREAD_KEYS_MAX              (128)

/**
 * @brief Minimum number of threads per process.
 */
#define _POSIX_THREAD_THREADS_MAX           (64)

/**
 * @brief Minimum number of per-process timers.
 */
#define _POSIX_TIMER_MAX                    (32)

/**
 * @brief Minimum length of a trace event name.
 */
#define _POSIX_TRACE_EVENT_NAME_MAX         (30)

/**
 * @brief Minimum length of a trace generation version string or stream name.
 */
#define _POSIX_TRACE_NAME_MAX               (8)

/**
 * @brief Minimum number of trace streams that may simultaneously exist in the system.
 */
#define _POSIX_TRACE_SYS_MAX                (8)

/**
 * @brief Minimum number of user trace event type identifiers per process.
 */
#define _POSIX_TRACE_USER_EVENT_MAX         (32)

/**
 * @brief Minimum length of a terminal device name.
 */
#define _POSIX_TTY_NAME_MAX                 (9)

/**
 * @brief Minimum number of bytes supported for the name of a timezone.
 */
#define _POSIX_TZNAME_MAX                   (6)

/**
 * @brief Minimum maximum obase value allowed by the bc utility.
 */
#define _POSIX2_BC_BASE_MAX                 (99)

/**
 * @brief Minimum maximum number of elements permitted in a bc array.
 */
#define _POSIX2_BC_DIM_MAX                  (2048)

/**
 * @brief Minimum maximum scale value allowed by the bc utility.
 */
#define _POSIX2_BC_SCALE_MAX                (99)

/**
 * @brief Minimum maximum length of a string constant accepted by the bc utility.
 */
#define _POSIX2_BC_STRING_MAX               (1000)

/**
 * @brief Minimum number of bytes in a character class name.
 */
#define _POSIX2_CHARCLASS_NAME_MAX          (14)

/**
 * @brief Minimum number of weights assignable to a locale collating entry.
 */
#define _POSIX2_COLL_WEIGHTS_MAX            (2)

/**
 * @brief Minimum number of expressions nestable within parentheses by the expr utility.
 */
#define _POSIX2_EXPR_NEST_MAX               (32)

/**
 * @brief Minimum length, in bytes, of a utility's input line.
 */
#define _POSIX2_LINE_MAX                    (2048)

/**
 * @brief Minimum number of iovec structures that a process may use with readv() or writev().
 */
#define _XOPEN_IOV_MAX                      (16)

/**
 * @brief Minimum number of bytes in a filename on XSI-conformant systems.
 */
#define _XOPEN_NAME_MAX                     (255)

/**
 * @brief Minimum number of bytes in a pathname on XSI-conformant systems.
 */
#define _XOPEN_PATH_MAX                     (1024)

/* Other invariant values */

/**
 * @brief Maximum number of bytes in a LANG name.
 */
#define NL_LANGMAX (14)

/**
 * @brief Maximum message number.
 */
#define NL_MSGMAX  (32767)

/**
 * @brief Maximum set number.
 */
#define NL_SETMAX  (255)

/**
 * @brief Maximum number of bytes in a message string.
 */
#define NL_TEXTMAX (_POSIX2_LINE_MAX)

/**
 * @brief Default process priority.
 */
#define NZERO      (20)

/* Runtime invariant values */

/**
 * @brief Maximum number of I/O operations in a single list I/O call supported by the
 * implementation.
 */
#define AIO_LISTIO_MAX                _POSIX_AIO_LISTIO_MAX

/**
 * @brief Maximum number of outstanding asynchronous I/O operations supported by the implementation.
 */
#define AIO_MAX                       _POSIX_AIO_MAX

/**
 * @brief The maximum amount by which a process can decrease its asynchronous I/O priority level
 * from its own scheduling priority.
 */
#define AIO_PRIO_DELTA_MAX            (0)

/**
 * @brief Maximum length of argument to the exec functions including environment data.
 */
#define ARG_MAX                       _POSIX_ARG_MAX

/**
 * @brief Maximum number of functions that may be registered with atexit().
 */
#define ATEXIT_MAX                    (32)

/**
 * @brief Maximum number of timer expiration overruns.
 */
#define DELAYTIMER_MAX \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_DELAYTIMER_MAX), (0))

/**
 * @brief Maximum length of a host name (not including the terminating null) as returned from the
 * gethostname() function.
 */
#define HOST_NAME_MAX \
	COND_CODE_1(CONFIG_POSIX_NETWORKING, (CONFIG_POSIX_HOST_NAME_MAX), (0))

/**
 * @brief Maximum length of a login name.
 */
#define LOGIN_NAME_MAX                _POSIX_LOGIN_NAME_MAX

/**
 * @brief The maximum number of open message queue descriptors a process may hold.
 */
#define MQ_OPEN_MAX \
	COND_CODE_1(CONFIG_POSIX_MESSAGE_PASSING, (CONFIG_POSIX_MQ_OPEN_MAX), (0))

/**
 * @brief The maximum number of message priorities supported by the implementation.
 */
#define MQ_PRIO_MAX                   _POSIX_MQ_PRIO_MAX

/**
 * @brief A value one greater than the maximum value that the system may assign to a newly-created
 * file descriptor.
 */
#define OPEN_MAX                      CONFIG_POSIX_OPEN_MAX

/**
 * @brief Equivalent to {PAGESIZE}.
 */
#define PAGE_SIZE                     CONFIG_POSIX_PAGE_SIZE

/**
 * @brief Size in bytes of a page.
 */
#define PAGESIZE                      CONFIG_POSIX_PAGE_SIZE

/**
 * @brief Maximum number of bytes the implementation will store as a pathname in a user-supplied
 * buffer of unspecified size, including the terminating null character.
 */
#define PATH_MAX                      _POSIX_PATH_MAX

/**
 * @brief Maximum number of attempts made to destroy a thread's thread-specific data values on
 * thread exit.
 */
#define PTHREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS

/**
 * @brief Maximum number of data keys that can be created by a process.
 */
#define PTHREAD_KEYS_MAX \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_KEYS_MAX), (0))

/**
 * @brief Maximum number of threads that can be created per process.
 */
#define PTHREAD_THREADS_MAX \
	COND_CODE_1(CONFIG_POSIX_THREADS, (CONFIG_POSIX_THREAD_THREADS_MAX), (0))

/**
 * @brief Maximum number of realtime signals reserved for application use in this implementation.
 */
#define RTSIG_MAX \
	COND_CODE_1(CONFIG_POSIX_REALTIME_SIGNALS, (CONFIG_POSIX_RTSIG_MAX), (0))

/**
 * @brief Maximum number of semaphores that a process may have.
 */
#define SEM_NSEMS_MAX \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_NSEMS_MAX), (0))

/**
 * @brief The maximum value a semaphore may have.
 */
#define SEM_VALUE_MAX \
	COND_CODE_1(CONFIG_POSIX_SEMAPHORES, (CONFIG_POSIX_SEM_VALUE_MAX), (0))

/**
 * @brief Maximum number of queued signals that a process may send and have pending at the
 * receiver(s) at any time.
 */
#define SIGQUEUE_MAX                  _POSIX_SIGQUEUE_MAX

/**
 * @brief Maximum number of streams that one process can have open at one time.
 */
#define STREAM_MAX                    _POSIX_STREAM_MAX

/**
 * @brief Maximum number of symbolic links that can be reliably traversed in the resolution of a
 * pathname in the absence of a loop.
 */
#define SYMLOOP_MAX                   _POSIX_SYMLOOP_MAX

/**
 * @brief Maximum number of timers per process supported by the implementation.
 */
#define TIMER_MAX \
	COND_CODE_1(CONFIG_POSIX_TIMERS, (CONFIG_POSIX_TIMER_MAX), (0))

/**
 * @brief Maximum length of terminal device name.
 */
#define TTY_NAME_MAX                  _POSIX_TTY_NAME_MAX

/**
 * @brief Maximum number of bytes supported for the name of a timezone (not of the TZ variable).
 */
#define TZNAME_MAX                    _POSIX_TZNAME_MAX

/* Pathname variable values */

/**
 * @brief Minimum number of bits needed to represent, as a signed integer value, the maximum size of
 * a regular file allowed in the specified directory.
 */
#define FILESIZEBITS             (32)

/**
 * @brief Minimum number of bytes of storage actually allocated for any portion of a file.
 */
#define POSIX_ALLOC_SIZE_MIN     (256)

/**
 * @brief Recommended increment for file transfer sizes between the {POSIX_REC_MIN_XFER_SIZE} and
 * {POSIX_REC_MAX_XFER_SIZE} values.
 */
#define POSIX_REC_INCR_XFER_SIZE (1024)

/**
 * @brief Maximum recommended file transfer size.
 */
#define POSIX_REC_MAX_XFER_SIZE  (32767)

/**
 * @brief Minimum recommended file transfer size.
 */
#define POSIX_REC_MIN_XFER_SIZE  (1)

/**
 * @brief Recommended file transfer buffer alignment.
 */
#define POSIX_REC_XFER_ALIGN     (4)

/**
 * @brief Maximum number of bytes in a symbolic link.
 */
#define SYMLINK_MAX              _POSIX_SYMLINK_MAX

/* clang-format on */

#endif

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_LIMITS_H_ */
