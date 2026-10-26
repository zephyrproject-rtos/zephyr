/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) semaphore runtime interface
 * implementation
 *
 */
#include <xenlib_osal_platform.h>

int xenlib_osal_semaphore_init(xenlib_osal_semaphore_t *sem,
					int init_count,
					int max_count)
{
	return k_sem_init(sem, init_count, max_count);
}

int xenlib_osal_semaphore_wait(xenlib_osal_semaphore_t *sem,
					xenlib_osal_timeout_t timeout)
{
	return k_sem_take(sem, timeout);
}

int xenlib_osal_semaphore_signal(xenlib_osal_semaphore_t *sem)
{
	k_sem_give(sem);
	return 0;
}
