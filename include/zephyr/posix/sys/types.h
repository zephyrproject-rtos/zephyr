/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_TYPES_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_TYPES_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_BLKCNT_T_DECLARED) && !defined(__blkcnt_t_defined)
typedef long blkcnt_t;
#define _BLKCNT_T_DECLARED
#define __blkcnt_t_defined
#endif

#if !defined(_BLKSIZE_T_DECLARED) && !defined(__blksize_t_defined)
typedef unsigned long blksize_t;
#define _BLKSIZE_T_DECLARED
#define __blksize_t_defined
#endif

#if !defined(_CLOCK_T_DECLARED) && !defined(__clock_t_defined)
typedef unsigned long clock_t;
#define _CLOCK_T_DECLARED
#define __clock_t_defined
#endif

#if !defined(_CLOCKID_T_DECLARED) && !defined(__clockid_t_defined)
typedef unsigned long clockid_t;
#define _CLOCKID_T_DECLARED
#define __clockid_t_defined
#endif

#if !defined(_DEV_T_DECLARED) && !defined(__dev_t_defined)
typedef int dev_t;
#define _DEV_T_DECLARED
#define __dev_t_defined
#endif

#if !defined(_FSBLKCNT_T_DECLARED) && !defined(__fsblkcnt_t_defined)
typedef unsigned long fsblkcnt_t;
#define _FSBLKCNT_T_DECLARED
#define __fsblkcnt_t_defined
#endif

#if !defined(_FSFILCNT_T_DECLARED) && !defined(__fsfilcnt_t_defined)
typedef unsigned long fsfilcnt_t;
#define _FSFILCNT_T_DECLARED
#define __fsfilcnt_t_defined
#endif

#if !defined(_GID_T_DECLARED) && !defined(__gid_t_defined)
typedef unsigned short gid_t;
#define _GID_T_DECLARED
#define __gid_t_defined
#endif

#if !defined(_INO_T_DECLARED) && !defined(__ino_t_defined)
typedef long ino_t;
#define _INO_T_DECLARED
#define __ino_t_defined
#endif

/* Maybe limit to when _XOPEN_SOURCE is defined? */
#if !defined(_KEY_T_DECLARED) && !defined(__key_t_defined)
typedef unsigned long key_t;
#define _KEY_T_DECLARED
#define __key_t_defined
#endif

#if !defined(_MODE_T_DECLARED) && !defined(__mode_t_defined)
typedef int mode_t;
#define _MODE_T_DECLARED
#define __mode_t_defined
#endif

#if !defined(_NLINK_T_DECLARED) && !defined(__nlink_t_defined)
typedef unsigned short nlink_t;
#define _NLINK_T_DECLARED
#define __nlink_t_defined
#endif

#if !defined(_OFF_T_DECLARED) && !defined(__off_t_defined)
typedef long off_t;
#define _OFF_T_DECLARED
#define __off_t_defined
#endif

#if !defined(_PID_T_DECLARED) && !defined(__pid_t_defined)
/* TODO: it would be nice to convert this to a long */
typedef int pid_t;
#define _PID_T_DECLARED
#define __pid_t_defined
#endif

/* size_t must be defined by the libc stddef.h */
#include <stddef.h>

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long
#endif

#if !defined(_SSIZE_T_DECLARED) && !defined(__ssize_t_defined)
#define unsigned signed /* parasoft-suppress MISRAC2012-RULE_20_4-a MISRAC2012-RULE_20_4-b */
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned
#define _SSIZE_T_DECLARED
#define __ssize_t_defined
#endif

#if !defined(_SUSECONDS_T_DECLARED) && !defined(__suseconds_t_defined)
typedef long suseconds_t;
#define _SUSECONDS_T_DECLARED
#define __suseconds_t_defined
#endif

/* time_t must be defined by the libc time.h */
#include <time.h>

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
/*
 * there is a workaround needed for picolibc because it doesn't have guards around the definition
 * of struct timespec
 */
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined) && !defined(CONFIG_PICOLIBC)
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#define _TIMESPEC_DECLARED
#define __timespec_defined
#endif
#endif

/* TODO: trace_attr_t, trace_event_id_t, trace_event_set_t, trace_id_t */

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
typedef unsigned short uid_t;
#define _UID_T_DECLARED
#define __uid_t_defined
#endif

#if !defined(_USECONDS_T_DECLARED) && !defined(__useconds_t_defined)
typedef unsigned long useconds_t;
#define _USECONDS_T_DECLARED
#define __useconds_t_defined
#endif

#ifdef __cplusplus
}
#endif

#include <sys/_pthreadtypes.h>

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_TYPES_H_ */
