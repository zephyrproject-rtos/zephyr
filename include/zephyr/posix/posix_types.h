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
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __clock_t_defined
/** @endcond */
#endif

#if !defined(_CLOCKID_T_DECLARED) && !defined(__clockid_t_defined)
/**
 * @brief Used for clock ID type in the clock and timer functions.
 */
typedef unsigned long clockid_t;
/** @cond INTERNAL_HIDDEN */
#define _CLOCKID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
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
/**
 * @brief Used for device IDs.
 */
typedef int dev_t;
/** @cond INTERNAL_HIDDEN */
#define _DEV_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __dev_t_defined
/** @endcond */
#endif

#if !defined(_INO_T_DECLARED) && !defined(__ino_t_defined)
/**
 * @brief Used for file serial numbers.
 */
typedef int ino_t;
/** @cond INTERNAL_HIDDEN */
#define _INO_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __ino_t_defined
/** @endcond */
#endif

#if !defined(_NLINK_T_DECLARED) && !defined(__nlink_t_defined)
/**
 * @brief Used for link counts.
 */
typedef unsigned short nlink_t;
/** @cond INTERNAL_HIDDEN */
#define _NLINK_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __nlink_t_defined
/** @endcond */
#endif

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
/**
 * @brief Used for user IDs.
 */
typedef unsigned short uid_t;
/** @cond INTERNAL_HIDDEN */
#define _UID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __uid_t_defined
/** @endcond */
#endif

#if !defined(_GID_T_DECLARED) && !defined(__gid_t_defined)
/**
 * @brief Used for group IDs.
 */
typedef unsigned short gid_t;
/** @cond INTERNAL_HIDDEN */
#define _GID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __gid_t_defined
/** @endcond */
#endif

#if !defined(_BLKSIZE_T_DECLARED) && !defined(__blksize_t_defined)
/**
 * @brief Used for block sizes.
 */
typedef unsigned long blksize_t;
/** @cond INTERNAL_HIDDEN */
#define _BLKSIZE_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __blksize_t_defined
/** @endcond */
#endif

#if !defined(_BLKCNT_T_DECLARED) && !defined(__blkcnt_t_defined)
/**
 * @brief Used for file block counts.
 */
typedef unsigned long blkcnt_t;
/** @cond INTERNAL_HIDDEN */
#define _BLKCNT_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __blkcnt_t_defined
/** @endcond */
#endif

#if !defined(CONFIG_ARCMWDT_LIBC)
/**
 * @brief Used for process IDs and process group IDs.
 */
typedef int pid_t;
#endif

#if !defined(_USECONDS_T_DECLARED) && !defined(__useconds_t_defined)
/**
 * @brief Used for time in microseconds.
 */
typedef unsigned long useconds_t;
#endif

/* time related attributes */
#if !defined(__timer_t_defined) && !defined(_TIMER_T_DECLARED)
/**
 * @brief Used for timer ID returned by timer_create().
 */
typedef unsigned long timer_t;
#endif

/* Thread attributes */
#if !defined(CONFIG_NEWLIB_LIBC)
#if !defined(_PTHREAD_ATTR_T_DECLARED) && !defined(__pthread_attr_t_defined)
typedef struct {
	void *stack;
	unsigned int details[2];
} pthread_attr_t;
/** @cond INTERNAL_HIDDEN */
#define _PTHREAD_ATTR_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __pthread_attr_t_defined
/** @endcond */
#endif
#endif

/**
 * @brief Used to identify a thread.
 */
typedef uint32_t pthread_t;

/**
 * @brief Used to identify a spin lock.
 */
typedef uint32_t pthread_spinlock_t;

/* Semaphore */

/**
 * @brief Used for semaphores.
 */
typedef struct k_sem sem_t;

/* Mutex */

/**
 * @brief Used for mutexes.
 */
typedef uint32_t pthread_mutex_t;

/**
 * @brief Implementation-specific storage for mutex attributes.
 */
struct pthread_mutexattr {
	/**
	 * @brief Mutex type (normal, recursive, or error-checking).
	 */
	unsigned char type: 2;
	/**
	 * @brief True if the attributes object has been initialized.
	 */
	bool initialized: 1;
};
#if !defined(CONFIG_NEWLIB_LIBC)
/**
 * @brief Used to identify a mutex attribute object.
 */
typedef struct pthread_mutexattr pthread_mutexattr_t;
BUILD_ASSERT(sizeof(pthread_mutexattr_t) >= sizeof(struct pthread_mutexattr));
#endif

/* Condition variables */

/**
 * @brief Used for condition variables.
 */
typedef uint32_t pthread_cond_t;

/**
 * @brief Implementation-specific storage for condition variable attributes.
 */
struct pthread_condattr {
	/**
	 * @brief ID of the clock used to measure timed waits.
	 */
	clockid_t clock;
};

#if !defined(CONFIG_NEWLIB_LIBC)
/**
 * @brief Used to identify a condition attribute object.
 */
typedef struct pthread_condattr pthread_condattr_t;
BUILD_ASSERT(sizeof(pthread_condattr_t) >= sizeof(struct pthread_condattr));
#endif

/* Barrier */

/**
 * @brief Used to identify a barrier.
 */
typedef uint32_t pthread_barrier_t;

/**
 * @brief Used to define a barrier attributes object.
 */
typedef struct pthread_barrierattr {
	/**
	 * @brief Process-shared attribute value.
	 */
	int pshared;
} pthread_barrierattr_t;

/**
 * @brief Used for read-write lock attributes.
 */
typedef uint32_t pthread_rwlockattr_t;

/**
 * @brief Used for read-write locks.
 */
typedef uint32_t pthread_rwlock_t;

/**
 * @brief Implementation-specific storage for dynamic package initialization state.
 */
struct pthread_once {
	/**
	 * @brief Flag set once the initialization routine has run.
	 */
	bool flag;
};

#if !defined(CONFIG_NEWLIB_LIBC)
/**
 * @brief Used for thread-specific data keys.
 */
typedef uint32_t pthread_key_t;

/**
 * @brief Used for dynamic package initialization.
 */
typedef struct pthread_once pthread_once_t;
/* Newlib typedefs pthread_once_t as a struct with two ints */
BUILD_ASSERT(sizeof(pthread_once_t) >= sizeof(struct pthread_once));
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_TYPES_H_ */
