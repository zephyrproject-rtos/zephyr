/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_MULTITHREADING

#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/logging/log.h>

#ifndef CONFIG_USERSPACE
#define ARCMWDT_DYN_LOCK_SZ	(sizeof(struct k_mutex))
/* The library wants 2 locks per available FILE entry, and then some more */
#define ARCMWDT_MAX_DYN_LOCKS	(FOPEN_MAX * 2 + 5)

K_MEM_SLAB_DEFINE(z_arcmwdt_lock_slab, ARCMWDT_DYN_LOCK_SZ, ARCMWDT_MAX_DYN_LOCKS, sizeof(void *));
#endif /* !CONFIG_USERSPACE */

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

typedef void *_lock_t;

void _mwmutex_create(_lock_t *mutex_ptr)
{
	bool alloc_fail;
#ifdef CONFIG_USERSPACE
	*mutex_ptr = k_object_alloc(K_OBJ_MUTEX);
	alloc_fail = (*mutex_ptr == NULL);
#else
	alloc_fail = !!k_mem_slab_alloc(&z_arcmwdt_lock_slab, mutex_ptr, K_NO_WAIT);
#endif /* CONFIG_USERSPACE */

	if (alloc_fail) {
		LOG_ERR("MWDT lock allocation failed");
		k_panic();
	}

	k_mutex_init((struct k_mutex *)*mutex_ptr);
}

void _mwmutex_delete(_lock_t *mutex_ptr)
{
	__ASSERT_NO_MSG(mutex_ptr != NULL);
#ifdef CONFIG_USERSPACE
	k_object_release(mutex_ptr);
#else
	k_mem_slab_free(&z_arcmwdt_lock_slab, *mutex_ptr);
#endif /* CONFIG_USERSPACE */
}

void _mwmutex_lock(_lock_t mutex)
{
	__ASSERT_NO_MSG(mutex != NULL);
	k_mutex_lock((struct k_mutex *)mutex, K_FOREVER);
}

void _mwmutex_unlock(_lock_t mutex)
{
	__ASSERT_NO_MSG(mutex != NULL);
	k_mutex_unlock((struct k_mutex *)mutex);
}
#endif /* CONFIG_MULTITHREADING */
