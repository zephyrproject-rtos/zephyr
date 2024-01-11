/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include "posix_types.h"
#include <zephyr/posix/sys/stat.h>
#ifdef CONFIG_NETWORKING
/* For zsock_gethostname() */
#include <zephyr/net/socket.h>
#include <zephyr/net/hostname.h>
#endif

#ifdef CONFIG_POSIX_API
#include <zephyr/fs/fs.h>
#endif

#ifdef CONFIG_POSIX_SYSCONF
#include <zephyr/posix/signal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Version test macros */
#define _POSIX_VERSION 200809L
#define _POSIX2_VERSION (-1L)
#define _XOPEN_VERSION (-1L)

/* Internal helper macro to set constant if required Kconfig symbol is enabled */
#define Z_SC_VAL_IFDEF(_def, _val) COND_CODE_1(_def, (_val), (-1L))

/* Constants for Opitions and Option Groups */
#define _POSIX_ADVISORY_INFO		  (-1L)
#define _POSIX_ASYNCHRONOUS_IO		  (-1L)
#define _POSIX_BARRIERS			  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_CHOWN_RESTRICTED		  (-1L)
#define _POSIX_CLOCK_SELECTION		  Z_SC_VAL_IFDEF(CONFIG_POSIX_CLOCK, _POSIX_VERSION)
#define _POSIX_CPUTIME			  (-1L)
#define _POSIX_FSYNC			  (-1L)
#define _POSIX_IPV6			  Z_SC_VAL_IFDEF(CONFIG_NET_IPV6, _POSIX_VERSION)
#define _POSIX_JOB_CONTROL		  (-1L)
#define _POSIX_MAPPED_FILES		  _POSIX_VERSION
#define _POSIX_MEMLOCK			  (-1L)
#define _POSIX_MEMLOCK_RANGE		  (-1L)
#define _POSIX_MEMORY_PROTECTION	  (-1L)
#define _POSIX_MESSAGE_PASSING		  Z_SC_VAL_IFDEF(CONFIG_POSIX_MQUEUE, _POSIX_VERSION)
#define _POSIX_MONOTONIC_CLOCK		  Z_SC_VAL_IFDEF(CONFIG_POSIX_CLOCK, _POSIX_VERSION)
#define _POSIX_NO_TRUNC			  (-1L)
#define _POSIX_PRIORITIZED_IO		  (-1L)
#define _POSIX_PRIORITY_SCHEDULING	  (-1L)
#define _POSIX_RAW_SOCKETS		  Z_SC_VAL_IFDEF(CONFIG_NET_SOCKETS_PACKET, _POSIX_VERSION)
#define _POSIX_READER_WRITER_LOCKS	  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_REALTIME_SIGNALS		  (-1L)
#define _POSIX_REGEXP			  (-1L)
#define _POSIX_SAVED_IDS		  (-1L)
#define _POSIX_SEMAPHORES		  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_SHARED_MEMORY_OBJECTS	  (-1L)
#define _POSIX_SHELL			  (-1L)
#define _POSIX_SPAWN			  (-1L)
#define _POSIX_SPIN_LOCKS		  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_SPINLOCK, _POSIX_VERSION)
#define _POSIX_SPORADIC_SERVER		  (-1L)
#define _POSIX_SYNCHRONIZED_IO		  (-1L)
#define _POSIX_THREAD_ATTR_STACKADDR	  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_THREAD_ATTR_STACKSIZE	  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_THREAD_CPUTIME		  (-1L)
#define _POSIX_THREAD_PRIO_INHERIT	  _POSIX_VERSION
#define _POSIX_THREAD_PRIO_PROTECT	  (-1L)
#define _POSIX_THREAD_PRIORITY_SCHEDULING Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_THREAD_PROCESS_SHARED	  (-1L)
#define _POSIX_THREAD_ROBUST_PRIO_INHERIT (-1L)
#define _POSIX_THREAD_ROBUST_PRIO_PROTECT (-1L)
#define _POSIX_THREAD_SAFE_FUNCTIONS	  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#define _POSIX_THREAD_SPORADIC_SERVER	  (-1L)

#ifndef _POSIX_THREADS
#define _POSIX_THREADS			  Z_SC_VAL_IFDEF(CONFIG_PTHREAD_IPC, _POSIX_VERSION)
#endif

#define _POSIX_TIMEOUTS			  Z_SC_VAL_IFDEF(CONFIG_POSIX_CLOCK, _POSIX_VERSION)
#define _POSIX_TIMERS			  Z_SC_VAL_IFDEF(CONFIG_POSIX_CLOCK, _POSIX_VERSION)
#define _POSIX_TRACE			  (-1L)
#define _POSIX_TRACE_EVENT_FILTER	  (-1L)
#define _POSIX_TRACE_INHERIT		  (-1L)
#define _POSIX_TRACE_LOG		  (-1L)
#define _POSIX_TYPED_MEMORY_OBJECTS	  (-1L)
#define _POSIX_V6_ILP32_OFF32		  (-1L)
#define _POSIX_V6_ILP32_OFFBIG		  (-1L)
#define _POSIX_V6_LP64_OFF64		  (-1L)
#define _POSIX_V6_LPBIG_OFFBIG		  (-1L)
#define _POSIX_V7_ILP32_OFF32		  (-1L)
#define _POSIX_V7_ILP32_OFFBIG		  (-1L)
#define _POSIX_V7_LP64_OFF64		  (-1L)
#define _POSIX_V7_LPBIG_OFFBIG		  (-1L)
#define _POSIX2_C_BIND			  _POSIX_VERSION
#define _POSIX2_C_DEV			  (-1L)
#define _POSIX2_CHAR_TERM		  (-1L)
#define _POSIX2_FORT_DEV		  (-1L)
#define _POSIX2_FORT_RUN		  (-1L)
#define _POSIX2_LOCALEDEF		  (-1L)
#define _POSIX2_PBS			  (-1L)
#define _POSIX2_PBS_ACCOUNTING		  (-1L)
#define _POSIX2_PBS_CHECKPOINT		  (-1L)
#define _POSIX2_PBS_LOCATE		  (-1L)
#define _POSIX2_PBS_MESSAGE		  (-1L)
#define _POSIX2_PBS_TRACK		  (-1L)
#define _POSIX2_SW_DEV			  (-1L)
#define _POSIX2_UPE			  (-1L)
#define _XOPEN_CRYPT			  (-1L)
#define _XOPEN_ENH_I18N			  (-1L)
#define _XOPEN_REALTIME			  (-1L)
#define _XOPEN_REALTIME_THREADS		  (-1L)
#define _XOPEN_SHM			  (-1L)
#define _XOPEN_STREAMS			  (-1L)
#define _XOPEN_UNIX			  (-1L)
#define _XOPEN_UUCP			  (-1L)

/* Maximum values */
#define _POSIX_CLOCKRES_MIN (20000000L)

/* Minimum values */
#define _POSIX_AIO_LISTIO_MAX		    (2)
#define _POSIX_AIO_MAX			    (1)
#define _POSIX_ARG_MAX			    (4096)
#define _POSIX_CHILD_MAX		    (25)
#define _POSIX_DELAYTIMER_MAX		    (32)
#define _POSIX_HOST_NAME_MAX		    (255)
#define _POSIX_LINK_MAX			    (8)
#define _POSIX_LOGIN_NAME_MAX		    (9)
#define _POSIX_MAX_CANON		    (255)
#define _POSIX_MAX_INPUT		    (255)
#define _POSIX_MQ_OPEN_MAX		    CONFIG_MSG_COUNT_MAX
#define _POSIX_MQ_PRIO_MAX		    (32)
#define _POSIX_NAME_MAX			    (14)
#define _POSIX_NGROUPS_MAX		    (8)
#define _POSIX_OPEN_MAX			    CONFIG_POSIX_MAX_FDS
#define _POSIX_PATH_MAX			    (256)
#define _POSIX_PIPE_BUF			    (512)
#define _POSIX_RE_DUP_MAX		    (255)
#define _POSIX_RTSIG_MAX		    CONFIG_POSIX_RTSIG_MAX
#define _POSIX_SEM_NSEMS_MAX		    CONFIG_SEM_NAMELEN_MAX
#define _POSIX_SEM_VALUE_MAX		    CONFIG_SEM_VALUE_MAX
#define _POSIX_SIGQUEUE_MAX		    (32)
#define _POSIX_SSIZE_MAX		    (32767)
#define _POSIX_SS_REPL_MAX		    (4)
#define _POSIX_STREAM_MAX		    (8)
#define _POSIX_SYMLINK_MAX		    (255)
#define _POSIX_SYMLOOP_MAX		    (8)
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS (4)
#define _POSIX_THREAD_KEYS_MAX		    (128)
#define _POSIX_THREAD_THREADS_MAX	    (64)
#define _POSIX_TIMER_MAX		    (32)
#define _POSIX_TRACE_EVENT_NAME_MAX	    (30)
#define _POSIX_TRACE_NAME_MAX		    (8)
#define _POSIX_TRACE_SYS_MAX		    (8)
#define _POSIX_TRACE_USER_EVENT_MAX	    (32)
#define _POSIX_TTY_NAME_MAX		    (9)
#define _POSIX_TZNAME_MAX		    (6)
#define _POSIX2_BC_BASE_MAX		    (99)
#define _POSIX2_BC_DIM_MAX		    (2048)
#define _POSIX2_BC_SCALE_MAX		    (99)
#define _POSIX2_BC_STRING_MAX		    (1000)
#define _POSIX2_CHARCLASS_NAME_MAX	    (14)
#define _POSIX2_COLL_WEIGHTS_MAX	    (2)
#define _POSIX2_EXPR_NEST_MAX		    (32)
#define _POSIX2_LINE_MAX		    (2048)
#define _XOPEN_IOV_MAX			    (16)
#define _XOPEN_NAME_MAX			    (255)
#define _XOPEN_PATH_MAX			    (1024)

/* Other invariant values */
#define NL_LANGMAX (14)
#define NL_MSGMAX  (32767)
#define NL_SETMAX  (255)
#define NL_TEXTMAX (_POSIX2_LINE_MAX)
#define NZERO	   (20)

/* Runtime invariant values */
#define AIO_LISTIO_MAX		      _POSIX_AIO_LISTIO_MAX
#define AIO_MAX			      _POSIX_AIO_MAX
#define AIO_PRIO_DELTA_MAX	      (0)
#define DELAYTIMER_MAX		      _POSIX_DELAYTIMER_MAX
#define HOST_NAME_MAX		      COND_CODE_1(CONFIG_NETWORKING,	  \
						  (NET_HOSTNAME_MAX_LEN), \
						  (_POSIX_HOST_NAME_MAX))
#define LOGIN_NAME_MAX		      _POSIX_LOGIN_NAME_MAX
#define MQ_OPEN_MAX		      _POSIX_MQ_OPEN_MAX
#define MQ_PRIO_MAX		      _POSIX_MQ_PRIO_MAX

#ifndef PAGE_SIZE
#define PAGE_SIZE		      BIT(CONFIG_POSIX_PAGE_SIZE_BITS)
#endif

#ifndef PAGESIZE
#define PAGESIZE		      PAGE_SIZE
#endif

#define PTHREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_KEYS_MAX	      COND_CODE_1(CONFIG_PTHREAD_IPC,		  \
						  (CONFIG_MAX_PTHREAD_KEY_COUNT), \
						  (_POSIX_THREAD_KEYS_MAX))
#define PTHREAD_THREADS_MAX	      COND_CODE_1(CONFIG_PTHREAD_IPC, CONFIG_MAX_PTHREAD_COUNT, 0)
#define SEM_NSEMS_MAX		      _POSIX_SEM_NSEMS_MAX
#define SEM_VALUE_MAX		      CONFIG_SEM_VALUE_MAX
#define SIGQUEUE_MAX		      _POSIX_SIGQUEUE_MAX
#define STREAM_MAX		      _POSIX_STREAM_MAX
#define SYMLOOP_MAX		      _POSIX_SYMLOOP_MAX
#define TIMER_MAX		      CONFIG_MAX_TIMER_COUNT
#define TTY_NAME_MAX		      _POSIX_TTY_NAME_MAX
#define TZNAME_MAX		      _POSIX_TZNAME_MAX

/* Pathname variable values */
#define FILESIZEBITS		 (32)
#define POSIX_ALLOC_SIZE_MIN	 (256)
#define POSIX_REC_INCR_XFER_SIZE (1024)
#define POSIX_REC_MAX_XFER_SIZE	 (32767)
#define POSIX_REC_MIN_XFER_SIZE	 (1)
#define POSIX_REC_XFER_ALIGN	 (4)
#define SYMLINK_MAX		 _POSIX_SYMLINK_MAX

#ifdef CONFIG_POSIX_API
/* File related operations */
int close(int file);
ssize_t write(int file, const void *buffer, size_t count);
ssize_t read(int file, void *buffer, size_t count);
off_t lseek(int file, off_t offset, int whence);

/* File System related operations */
int rename(const char *old, const char *newp);
int unlink(const char *path);
int stat(const char *path, struct stat *buf);
int mkdir(const char *path, mode_t mode);

FUNC_NORETURN void _exit(int status);

#ifdef CONFIG_NETWORKING
static inline int gethostname(char *buf, size_t len)
{
	return zsock_gethostname(buf, len);
}
#endif /* CONFIG_NETWORKING */

#endif /* CONFIG_POSIX_API */

#ifdef CONFIG_GETOPT
int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int opterr, optind, optopt;
#endif

pid_t getpid(void);
unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#ifdef CONFIG_POSIX_SYSCONF
#define __z_posix_sysconf_SC_ADVISORY_INFO		  _POSIX_ADVISORY_INFO
#define __z_posix_sysconf_SC_ASYNCHRONOUS_IO		  _POSIX_ASYNCHRONOUS_IO
#define __z_posix_sysconf_SC_BARRIERS			  _POSIX_BARRIERS
#define __z_posix_sysconf_SC_CLOCK_SELECTION		  _POSIX_CLOCK_SELECTION
#define __z_posix_sysconf_SC_CPUTIME			  _POSIX_CPUTIME
#define __z_posix_sysconf_SC_FSYNC			  _POSIX_FSYNC
#define __z_posix_sysconf_SC_IPV6			  _POSIX_IPV6
#define __z_posix_sysconf_SC_JOB_CONTROL		  _POSIX_JOB_CONTROL
#define __z_posix_sysconf_SC_MAPPED_FILE		  _POSIX_MAPPED_FILES
#define __z_posix_sysconf_SC_MEMLOCK			  _POSIX_MEMLOCK
#define __z_posix_sysconf_SC_MEMLOCK_RANGE		  _POSIX_MEMLOCK_RANGE
#define __z_posix_sysconf_SC_MEMORY_PROTECTION		  _POSIX_MEMORY_PROTECTION
#define __z_posix_sysconf_SC_MESSAGE_PASSING		  _POSIX_MESSAGE_PASSING
#define __z_posix_sysconf_SC_MONOTONIC_CLOCK		  _POSIX_MONOTONIC_CLOCK
#define __z_posix_sysconf_SC_PRIORITIZED_IO		  _POSIX_PRIORITIZED_IO
#define __z_posix_sysconf_SC_PRIORITY_SCHEDULING	  _POSIX_PRIORITY_SCHEDULING
#define __z_posix_sysconf_SC_RAW_SOCKETS		  _POSIX_RAW_SOCKETS
#define __z_posix_sysconf_SC_RE_DUP_MAX			  _POSIX_RE_DUP_MAX
#define __z_posix_sysconf_SC_READER_WRITER_LOCKS	  _POSIX_READER_WRITER_LOCKS
#define __z_posix_sysconf_SC_REALTIME_SIGNALS		  _POSIX_REALTIME_SIGNALS
#define __z_posix_sysconf_SC_REGEXP			  _POSIX_REGEXP
#define __z_posix_sysconf_SC_SAVED_IDS			  _POSIX_SAVED_IDS
#define __z_posix_sysconf_SC_SEMAPHORES			  _POSIX_SEMAPHORES
#define __z_posix_sysconf_SC_SHARED_MEMORY_OBJECTS	  _POSIX_SHARED_MEMORY_OBJECTS
#define __z_posix_sysconf_SC_SHELL			  _POSIX_SHELL
#define __z_posix_sysconf_SC_SPAWN			  _POSIX_SPAWN
#define __z_posix_sysconf_SC_SPIN_LOCKS			  _POSIX_SPIN_LOCKS
#define __z_posix_sysconf_SC_SPORADIC_SERVER		  _POSIX_SPORADIC_SERVER
#define __z_posix_sysconf_SC_SS_REPL_MAX		  _POSIX_SS_REPL_MAX
#define __z_posix_sysconf_SC_SYNCHRONIZED_IO		  _POSIX_SYNCHRONIZED_IO
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKADDR	  _POSIX_THREAD_ATTR_STACKADDR
#define __z_posix_sysconf_SC_THREAD_ATTR_STACKSIZE	  _POSIX_THREAD_ATTR_STACKSIZE
#define __z_posix_sysconf_SC_THREAD_CPUTIME		  _POSIX_THREAD_CPUTIME
#define __z_posix_sysconf_SC_THREAD_PRIO_INHERIT	  _POSIX_THREAD_PRIO_INHERIT
#define __z_posix_sysconf_SC_THREAD_PRIO_PROTECT	  _POSIX_THREAD_PRIO_PROTECT
#define __z_posix_sysconf_SC_THREAD_PRIORITY_SCHEDULING	  _POSIX_THREAD_PRIORITY_SCHEDULING
#define __z_posix_sysconf_SC_THREAD_PROCESS_SHARED	  _POSIX_THREAD_PROCESS_SHARED
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_INHERIT	  _POSIX_THREAD_ROBUST_PRIO_INHERIT
#define __z_posix_sysconf_SC_THREAD_ROBUST_PRIO_PROTECT	  _POSIX_THREAD_ROBUST_PRIO_PROTECT
#define __z_posix_sysconf_SC_THREAD_SAFE_FUNCTIONS	  _POSIX_THREAD_SAFE_FUNCTIONS
#define __z_posix_sysconf_SC_THREAD_SPORADIC_SERVER	  _POSIX_THREAD_SPORADIC_SERVER
#define __z_posix_sysconf_SC_THREADS			  _POSIX_THREADS
#define __z_posix_sysconf_SC_TIMEOUTS			  _POSIX_TIMEOUTS
#define __z_posix_sysconf_SC_TIMERS			  _POSIX_TIMERS
#define __z_posix_sysconf_SC_TRACE			  _POSIX_TRACE
#define __z_posix_sysconf_SC_TRACE_EVENT_FILTER		  _POSIX_TRACE_EVENT_FILTER
#define __z_posix_sysconf_SC_TRACE_EVENT_NAME_MAX	  _POSIX_TRACE_EVENT_NAME_MAX
#define __z_posix_sysconf_SC_TRACE_INHERIT		  _POSIX_TRACE_INHERIT
#define __z_posix_sysconf_SC_TRACE_LOG			  _POSIX_TRACE_LOG
#define __z_posix_sysconf_SC_TRACE_NAME_MAX		  _POSIX_TRACE_NAME_MAX
#define __z_posix_sysconf_SC_TRACE_SYS_MAX		  _POSIX_TRACE_SYS_MAX
#define __z_posix_sysconf_SC_TRACE_USER_EVENT_MAX	  _POSIX_TRACE_USER_EVENT_MAX
#define __z_posix_sysconf_SC_TYPED_MEMORY_OBJECTS	  _POSIX_TYPED_MEMORY_OBJECTS
#define __z_posix_sysconf_SC_VERSION			  _POSIX_VERSION
#define __z_posix_sysconf_SC_V7_ILP32_OFF32		  _POSIX_V7_ILP32_OFF32
#define __z_posix_sysconf_SC_V7_ILP32_OFFBIG		  _POSIX_V7_ILP32_OFFBIG
#define __z_posix_sysconf_SC_V7_LP64_OFF64		  _POSIX_V7_LP64_OFF64
#define __z_posix_sysconf_SC_V7_LPBIG_OFFBIG		  _POSIX_V7_LPBIG_OFFBIG
#define __z_posix_sysconf_SC_V6_ILP32_OFF32		  _POSIX_V6_ILP32_OFF32
#define __z_posix_sysconf_SC_V6_ILP32_OFFBIG		  _POSIX_V6_ILP32_OFFBIG
#define __z_posix_sysconf_SC_V6_LP64_OFF64		  _POSIX_V6_LP64_OFF64
#define __z_posix_sysconf_SC_V6_LPBIG_OFFBIG		  _POSIX_V6_LPBIG_OFFBIG
#define __z_posix_sysconf_SC_BC_BASE_MAX		  _POSIX2_BC_BASE_MAX
#define __z_posix_sysconf_SC_BC_DIM_MAX			  _POSIX2_BC_DIM_MAX
#define __z_posix_sysconf_SC_BC_SCALE_MAX		  _POSIX2_BC_SCALE_MAX
#define __z_posix_sysconf_SC_BC_STRING_MAX		  _POSIX2_BC_STRING_MAX
#define __z_posix_sysconf_SC_2_C_BIND			  _POSIX2_C_BIND
#define __z_posix_sysconf_SC_2_C_DEV			  _POSIX2_C_DEV
#define __z_posix_sysconf_SC_2_CHAR_TERM		  _POSIX2_CHAR_TERM
#define __z_posix_sysconf_SC_COLL_WEIGHTS_MAX		  _POSIX2_COLL_WEIGHTS_MAX
#define __z_posix_sysconf_SC_DELAYTIMER_MAX		  _POSIX2_DELAYTIMER_MAX
#define __z_posix_sysconf_SC_EXPR_NEST_MAX		  _POSIX2_EXPR_NEST_MAX
#define __z_posix_sysconf_SC_2_FORT_DEV			  _POSIX2_FORT_DEV
#define __z_posix_sysconf_SC_2_FORT_RUN			  _POSIX2_FORT_RUN
#define __z_posix_sysconf_SC_LINE_MAX			  _POSIX2_LINE_MAX
#define __z_posix_sysconf_SC_2_LOCALEDEF		  _POSIX2_LOCALEDEF
#define __z_posix_sysconf_SC_2_PBS			  _POSIX2_PBS
#define __z_posix_sysconf_SC_2_PBS_ACCOUNTING		  _POSIX2_PBS_ACCOUNTING
#define __z_posix_sysconf_SC_2_PBS_CHECKPOINT		  _POSIX2_PBS_CHECKPOINT
#define __z_posix_sysconf_SC_2_PBS_LOCATE		  _POSIX2_PBS_LOCATE
#define __z_posix_sysconf_SC_2_PBS_MESSAGE		  _POSIX2_PBS_MESSAGE
#define __z_posix_sysconf_SC_2_PBS_TRACK		  _POSIX2_PBS_TRACK
#define __z_posix_sysconf_SC_2_SW_DEV			  _POSIX2_SW_DEV
#define __z_posix_sysconf_SC_2_UPE			  _POSIX2_UPE
#define __z_posix_sysconf_SC_2_VERSION			  _POSIX2_VERSION
#define __z_posix_sysconf_SC_XOPEN_CRYPT		  _XOPEN_CRYPT
#define __z_posix_sysconf_SC_XOPEN_ENH_I18N		  _XOPEN_ENH_I18N
#define __z_posix_sysconf_SC_XOPEN_REALTIME		  _XOPEN_REALTIME
#define __z_posix_sysconf_SC_XOPEN_REALTIME_THREADS	  _XOPEN_REALTIME_THREADS
#define __z_posix_sysconf_SC_XOPEN_SHM			  _XOPEN_SHM
#define __z_posix_sysconf_SC_XOPEN_STREAMS		  _XOPEN_STREAMS
#define __z_posix_sysconf_SC_XOPEN_UNIX			  _XOPEN_UNIX
#define __z_posix_sysconf_SC_XOPEN_UUCP			  _XOPEN_UUCP
#define __z_posix_sysconf_SC_XOPEN_VERSION		  _XOPEN_VERSION
#define __z_posix_sysconf_SC_CLK_TCK			  (100L)
#define __z_posix_sysconf_SC_GETGR_R_SIZE_MAX		  (0L)
#define __z_posix_sysconf_SC_GETPW_R_SIZE_MAX		  (0L)
#define __z_posix_sysconf_SC_AIO_LISTIO_MAX		  AIO_LISTIO_MAX
#define __z_posix_sysconf_SC_AIO_MAX			  AIO_MAX
#define __z_posix_sysconf_SC_AIO_PRIO_DELTA_MAX		  AIO_PRIO_DELTA_MAX
#define __z_posix_sysconf_SC_ARG_MAX			  ARG_MAX
#define __z_posix_sysconf_SC_ATEXIT_MAX			  ATEXIT_MAX
#define __z_posix_sysconf_SC_CHILD_MAX			  CHILD_MAX
#define __z_posix_sysconf_SC_HOST_NAME_MAX		  HOST_NAME_MAX
#define __z_posix_sysconf_SC_IOV_MAX			  IOV_MAX
#define __z_posix_sysconf_SC_LOGIN_NAME_MAX		  LOGIN_NAME_MAX
#define __z_posix_sysconf_SC_NGROUPS_MAX		  _POSIX_NGROUPS_MAX
#define __z_posix_sysconf_SC_MQ_OPEN_MAX		  MQ_OPEN_MAX
#define __z_posix_sysconf_SC_MQ_PRIO_MAX		  MQ_PRIO_MAX
#define __z_posix_sysconf_SC_OPEN_MAX			  CONFIG_POSIX_MAX_FDS
#define __z_posix_sysconf_SC_PAGE_SIZE			  PAGE_SIZE
#define __z_posix_sysconf_SC_PAGESIZE			  PAGESIZE
#define __z_posix_sysconf_SC_THREAD_DESTRUCTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#define __z_posix_sysconf_SC_THREAD_KEYS_MAX		  PTHREAD_KEYS_MAX
#define __z_posix_sysconf_SC_THREAD_STACK_MIN		  PTHREAD_STACK_MIN
#define __z_posix_sysconf_SC_THREAD_THREADS_MAX		  PTHREAD_THREADS_MAX
#define __z_posix_sysconf_SC_RTSIG_MAX			  RTSIG_MAX
#define __z_posix_sysconf_SC_SEM_NSEMS_MAX		  SEM_NSEMS_MAX
#define __z_posix_sysconf_SC_SEM_VALUE_MAX		  SEM_VALUE_MAX
#define __z_posix_sysconf_SC_SIGQUEUE_MAX		  SIGQUEUE_MAX
#define __z_posix_sysconf_SC_STREAM_MAX			  STREAM_MAX
#define __z_posix_sysconf_SC_SYMLOOP_MAX		  SYMLOOP_MAX
#define __z_posix_sysconf_SC_TIMER_MAX			  TIMER_MAX
#define __z_posix_sysconf_SC_TTY_NAME_MAX		  TTY_NAME_MAX
#define __z_posix_sysconf_SC_TZNAME_MAX			  TZNAME_MAX

#define sysconf(x) (long)CONCAT(__z_posix_sysconf, x)

#endif /* CONFIG_POSIX_SYSCONF */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
