/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Data types.
 * @ingroup posix
 *
 * Provides POSIX data types such as clock, user/group ID, and pthread object
 * types for toolchains whose C library does not already define them.
 *
 * @posix_header{sys_types.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_TYPES_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_TYPES_H_

#if !(defined(CONFIG_NATIVE_LIBC))
#include <sys/types.h>
#endif

#if !defined(_CLOCK_T_DECLARED) && !defined(__clock_t_defined)
/**
 * @brief Used for system times in clock ticks or CLOCKS_PER_SEC; see <time.h>.
 */
typedef unsigned long clock_t;
/** @cond INTERNAL_HIDDEN */
#define _CLOCK_T_DECLARED
#define __clock_t_defined
/** @endcond */
#endif

#if !defined(_CLOCKID_T_DECLARED) && !defined(__clockid_t_defined)
/**
 * @brief Identifies a clock (such as @c CLOCK_REALTIME, @c CLOCK_MONOTONIC).
 */
typedef unsigned long clockid_t;
/** @cond INTERNAL_HIDDEN */
#define _CLOCKID_T_DECLARED
#define __clockid_t_defined
/** @endcond */
#endif

#ifdef CONFIG_NEWLIB_LIBC
#include <sys/_pthreadtypes.h>
#endif

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_DEV_T_DECLARED) && !defined(__dev_t_defined)
typedef int dev_t;                /**< Used for device IDs. */
/** @cond INTERNAL_HIDDEN */
#define _DEV_T_DECLARED
#define __dev_t_defined
/** @endcond */
#endif

#if !defined(_INO_T_DECLARED) && !defined(__ino_t_defined)
typedef int ino_t;                /**< Used for file serial numbers. */
/** @cond INTERNAL_HIDDEN */
#define _INO_T_DECLARED
#define __ino_t_defined
/** @endcond */
#endif

#if !defined(_NLINK_T_DECLARED) && !defined(__nlink_t_defined)
typedef unsigned short nlink_t;   /**< Used for link counts. */
/** @cond INTERNAL_HIDDEN */
#define _NLINK_T_DECLARED
#define __nlink_t_defined
/** @endcond */
#endif

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
typedef unsigned short uid_t;     /**< Used for user IDs. */
/** @cond INTERNAL_HIDDEN */
#define _UID_T_DECLARED
#define __uid_t_defined
/** @endcond */
#endif

#if !defined(_GID_T_DECLARED) && !defined(__gid_t_defined)
typedef unsigned short gid_t;     /**< Used for group IDs. */
/** @cond INTERNAL_HIDDEN */
#define _GID_T_DECLARED
#define __gid_t_defined
/** @endcond */
#endif

#if !defined(_BLKSIZE_T_DECLARED) && !defined(__blksize_t_defined)
typedef unsigned long blksize_t;  /**< Used for block sizes. */
/** @cond INTERNAL_HIDDEN */
#define _BLKSIZE_T_DECLARED
#define __blksize_t_defined
/** @endcond */
#endif

#if !defined(_BLKCNT_T_DECLARED) && !defined(__blkcnt_t_defined)
typedef unsigned long blkcnt_t;   /**< Used for file block counts. */
/** @cond INTERNAL_HIDDEN */
#define _BLKCNT_T_DECLARED
#define __blkcnt_t_defined
/** @endcond */
#endif

#if !defined(CONFIG_ARCMWDT_LIBC)
typedef int pid_t;                /**< Used for process IDs and process group IDs. */
#endif

#if !defined(_USECONDS_T_DECLARED) && !defined(__useconds_t_defined)
typedef unsigned long useconds_t; /**< Used for time in microseconds. */
#endif

/* time related attributes */
#if !defined(__timer_t_defined) && !defined(_TIMER_T_DECLARED)
typedef unsigned long timer_t;    /**< Opaque handle for a POSIX interval timer. */
#endif

/* Thread attributes */
#if !defined(CONFIG_NEWLIB_LIBC)
#if !defined(_PTHREAD_ATTR_T_DECLARED) && !defined(__pthread_attr_t_defined)
/**
 * @brief Thread creation attributes.
 */
typedef struct {
	void *stack;             /**< Thread stack address. */
	unsigned int details[2]; /**< Implementation-defined thread attributes. */
} pthread_attr_t;
/** @cond INTERNAL_HIDDEN */
#define _PTHREAD_ATTR_T_DECLARED
#define __pthread_attr_t_defined
/** @endcond */
#endif
#endif

typedef uint32_t pthread_t;          /**< Used to identify a thread. */

typedef uint32_t pthread_spinlock_t; /**< Used to identify a spin lock. */

/* Semaphore */

typedef struct k_sem sem_t;          /**< Used for semaphores. */

/* Mutex */

typedef uint32_t pthread_mutex_t;    /**< Used for mutexes. */

/**
 * @brief Implementation-specific storage for mutex attributes.
 */
struct pthread_mutexattr {
	unsigned char type: 2; /**< Mutex type (normal, recursive, or error-checking). */
	bool initialized: 1;   /**< True if the attributes object has been initialized. */
};
#if !defined(CONFIG_NEWLIB_LIBC)
/**
 * @brief Used to identify a mutex attribute object.
 */
typedef struct pthread_mutexattr pthread_mutexattr_t;
BUILD_ASSERT(sizeof(pthread_mutexattr_t) >= sizeof(struct pthread_mutexattr));
#endif

/* Condition variables */

typedef uint32_t pthread_cond_t; /**< Used for condition variables. */

/**
 * @brief Implementation-specific storage for condition variable attributes.
 */
struct pthread_condattr {
	clockid_t clock; /**< ID of the clock used to measure timed waits. */
};

#if !defined(CONFIG_NEWLIB_LIBC)
/**
 * @brief Used to identify a condition attribute object.
 */
typedef struct pthread_condattr pthread_condattr_t;
BUILD_ASSERT(sizeof(pthread_condattr_t) >= sizeof(struct pthread_condattr));
#endif

/* Barrier */

typedef uint32_t pthread_barrier_t; /**< Used to identify a barrier. */

/**
 * @brief Used to define a barrier attributes object.
 */
typedef struct pthread_barrierattr {
	int pshared; /**< Process-shared attribute value. */
} pthread_barrierattr_t;

typedef uint32_t pthread_rwlockattr_t; /**< Used for read-write lock attributes. */

typedef uint32_t pthread_rwlock_t;     /**< Used for read-write locks. */

/**
 * @brief Implementation-specific storage for dynamic package initialization state.
 */
struct pthread_once {
	bool flag; /**< Flag set once the initialization routine has run. */
};

#if !defined(CONFIG_NEWLIB_LIBC)
typedef uint32_t pthread_key_t;             /**< Used for thread-specific data keys. */

typedef struct pthread_once pthread_once_t; /**< Used for dynamic package initialization. */
/* Newlib typedefs pthread_once_t as a struct with two ints */
BUILD_ASSERT(sizeof(pthread_once_t) >= sizeof(struct pthread_once));
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_TYPES_H_ */
