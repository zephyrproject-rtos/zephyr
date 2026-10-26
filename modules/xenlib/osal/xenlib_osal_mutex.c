/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) mutex runtime interface
 * implementation
 *
 */
#include <xenlib_osal_platform.h>

int xenlib_osal_mutex_init(xenlib_osal_mutex_t *mutex)
{
	return k_mutex_init(mutex);
}

int xenlib_osal_mutex_lock(xenlib_osal_mutex_t *mutex,
					xenlib_osal_timeout_t timeout)
{
	return k_mutex_lock(mutex, timeout);
}

int xenlib_osal_mutex_unlock(xenlib_osal_mutex_t *mutex)
{
	return k_mutex_unlock(mutex);
}
