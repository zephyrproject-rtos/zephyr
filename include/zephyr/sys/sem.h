/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief public sys_sem APIs.
 */

#ifndef ZEPHYR_INCLUDE_SYS_SEM_H_
#define ZEPHYR_INCLUDE_SYS_SEM_H_

/*
 * sys_sem exists in user memory working as counter semaphore for
 * user mode thread when user mode enabled. When user mode isn't
 * enabled, sys_sem behaves like k_sem.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * sys_sem structure
 */
struct sys_sem {
#ifdef CONFIG_USERSPACE
	struct k_futex futex;
	int limit;
#else
	struct k_sem kernel_sem;
#endif
};

/**
 * @defgroup user_semaphore_apis User mode semaphore APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a sys_sem
 *
 * The semaphore can be accessed outside the module where it is defined using:
 *
 * @code extern struct sys_sem <name>; @endcode
 *
 * Route this to memory domains using K_APP_DMEM().
 *
 * @param _name Name of the semaphore.
 * @param _initial_count Initial semaphore count.
 * @param _count_limit Maximum permitted semaphore count.
 */
#ifdef CONFIG_USERSPACE
#define SYS_SEM_DEFINE(_name, _initial_count, _count_limit) \
	struct sys_sem _name = { \
		.futex = { _initial_count }, \
		.limit = _count_limit \
	}; \
	BUILD_ASSERT(((_count_limit) != 0) && \
		     ((_initial_count) <= (_count_limit)))
#else
/* Stuff this in the section with the rest of the k_sem objects, since they
 * are identical and can be treated as a k_sem in the boot initialization code
 */
#define SYS_SEM_DEFINE(_name, _initial_count, _count_limit) \
	STRUCT_SECTION_ITERABLE_ALTERNATE(k_sem, sys_sem, _name) = { \
		.kernel_sem = Z_SEM_INITIALIZER(_name.kernel_sem, \
						_initial_count, _count_limit) \
	}; \
	BUILD_ASSERT(((_count_limit) != 0) && \
		     ((_initial_count) <= (_count_limit)))
#endif

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore instance, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @retval 0 Initial success.
 * @retval -EINVAL Bad parameters, the value of limit should be located in
 *         (0, INT_MAX] and initial_count shouldn't be greater than limit.
 */
int sys_sem_init(struct sys_sem *sem, unsigned int initial_count,
		unsigned int limit);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its
 * maximum permitted count.
 *
 * @param sem Address of the semaphore.
 *
 * @retval 0 Semaphore given.
 * @retval -EINVAL Parameter address not recognized.
 * @retval -EACCES Caller does not have enough access.
 * @retval -EAGAIN Count reached Maximum permitted count and try again.
 */
int sys_sem_give(struct sys_sem *sem);

/**
 * @brief Take a sys_sem.
 *
 * This routine takes @a sem.
 *
 * @param sem Address of the sys_sem.
 * @param timeout Waiting period to take the sys_sem,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 sys_sem taken.
 * @retval -EINVAL Parameter address not recognized.
 * @retval -ETIMEDOUT Waiting period timed out.
 * @retval -EACCES Caller does not have enough access.
 */
int sys_sem_take(struct sys_sem *sem, k_timeout_t timeout);

/**
 * @brief Get sys_sem's value
 *
 * This routine returns the current value of @a sem.
 *
 * @param sem Address of the sys_sem.
 *
 * @return Current value of sys_sem.
 */
unsigned int sys_sem_count_get(struct sys_sem *sem);

/**
 * @cond INTERNAL_HIDDEN
 */

#if defined(__GNUC__)
static ALWAYS_INLINE void z_sys_sem_lock_onexit(__maybe_unused int *rc)
{
	__ASSERT(*rc == 1, "SYS_SEM_LOCK exited with goto, break or return, "
			   "use SYS_SEM_LOCK_BREAK instead.");
}
#define SYS_SEM_LOCK_ONEXIT __attribute__((__cleanup__(z_sys_sem_lock_onexit)))
#else
#define SYS_SEM_LOCK_ONEXIT
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Leaves a code block guarded with @ref SYS_SEM_LOCK after releasing the
 * lock.
 *
 * See @ref SYS_SEM_LOCK for details.
 */
#define SYS_SEM_LOCK_BREAK continue

/**
 * @brief Guards a code block with the given sys_sem, automatically acquiring
 * the semaphore before executing the code block. The semaphore will be
 * released either when reaching the end of the code block or when leaving the
 * block with @ref SYS_SEM_LOCK_BREAK.
 *
 * @details Example usage:
 *
 * @code{.c}
 * SYS_SEM_LOCK(&sem) {
 *
 *   ...execute statements with the semaphore held...
 *
 *   if (some_condition) {
 *     ...release the lock and leave the guarded section prematurely:
 *     SYS_SEM_LOCK_BREAK;
 *   }
 *
 *   ...execute statements with the lock held...
 *
 * }
 * @endcode
 *
 * Behind the scenes this pattern expands to a for-loop whose body is executed
 * exactly once:
 *
 * @code{.c}
 * for (sys_sem_take(&sem, K_FOREVER); ...; sys_sem_give(&sem)) {
 *     ...
 * }
 * @endcode
 *
 * @warning The code block must execute to its end or be left by calling
 * @ref SYS_SEM_LOCK_BREAK. Otherwise, e.g. if exiting the block with a break,
 * goto or return statement, the semaphore will not be released on exit.
 *
 * @param sem Semaphore (@ref sys_sem) used to guard the enclosed code block.
 */
#define SYS_SEM_LOCK(sem)                                                                          \
	for (int __rc SYS_SEM_LOCK_ONEXIT = sys_sem_take((sem), K_FOREVER); ({                     \
		     __ASSERT(__rc >= 0, "Failed to take sem: %d", __rc);                          \
		     __rc == 0;                                                                    \
	     });                                                                                   \
	     ({                                                                                    \
		     __rc = sys_sem_give((sem));                                                   \
		     __ASSERT(__rc == 0, "Failed to give sem: %d", __rc);                          \
	     }),                                                                                   \
		      __rc = 1)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
