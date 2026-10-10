/*
 * Copyright (c) 2026 Yavuz Murat Tas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KSEM_H_
#define ZEPHYR_KERNEL_INCLUDE_KSEM_H_

#include <zephyr/kernel.h>

/**
 * @brief Give a semaphore unless its count is already at the limit.
 *
 * Like k_sem_give(), but the limit check and the increment happen under the
 * semaphore lock, so concurrent callers cannot both succeed on the last free
 * slot. For supervisor-mode kernel subsystems only (e.g. POSIX sem_post()).
 *
 * @retval 0 the semaphore was given
 * @retval -EOVERFLOW the count is already at the limit; nothing was changed
 */
int z_sem_give_checked(struct k_sem *sem);

#endif /* ZEPHYR_KERNEL_INCLUDE_KSEM_H_ */
