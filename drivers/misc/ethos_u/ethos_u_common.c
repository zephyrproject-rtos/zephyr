/*
 * SPDX-FileCopyrightText: <text>Copyright 2021-2022, 2024 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys_clock.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>

#include <ethosu_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ethos_u, CONFIG_ETHOS_U_LOG_LEVEL);

/*******************************************************************************
 * Re-implementation/Overrides __((weak)) symbol functions from ethosu_driver.c
 * To handle mutex and semaphores
 *******************************************************************************/

void *ethosu_mutex_create(void)
{
	struct k_mutex *mutex;

	mutex = k_malloc(sizeof(*mutex));
	if (mutex == NULL) {
		LOG_ERR("Failed allocate mutex");
		return NULL;
	}

	k_mutex_init(mutex);

	return (void *)mutex;
}

int ethosu_mutex_lock(void *mutex)
{
	int status;

	status = k_mutex_lock((struct k_mutex *)mutex, K_FOREVER);
	if (status != 0) {
		LOG_ERR("Failed to lock mutex with error - %d", status);
		return -1;
	}

	return 0;
}

int ethosu_mutex_unlock(void *mutex)
{
	k_mutex_unlock((struct k_mutex *)mutex);
	return 0;
}

void *ethosu_semaphore_create(void)
{
	struct k_sem *sem;

	sem = k_malloc(sizeof(*sem));
	if (sem == NULL) {
		LOG_ERR("Failed to allocate semaphore");
		return NULL;
	}

	k_sem_init(sem, 0, 100);

	return (void *)sem;
}

int ethosu_semaphore_take(void *sem, uint64_t timeout)
{
	int status;

	status = k_sem_take((struct k_sem *)sem, (timeout == ETHOSU_SEMAPHORE_WAIT_FOREVER)
							 ? K_FOREVER
							 : Z_TIMEOUT_TICKS(timeout));

	if (status != 0) {
		/* The Ethos-U driver expects the semaphore implementation to never fail except for
		 * when a timeout occurs, and the current ethosu_semaphore_take implementation makes
		 * no distinction, in terms of return codes, between a timeout and other semaphore
		 * take failures. Also, note that a timeout is virtually indistinguishable from
		 * other failures if the driver logging is disabled. Handling errors other than a
		 * timeout is therefore not covered here and is deferred to the application
		 * developer if necessary.
		 */
		if (status != -EAGAIN) {
			LOG_ERR("Failed to take semaphore with error - %d", status);
		}
		return -1;
	}

	return 0;
}

int ethosu_semaphore_give(void *sem)
{
	k_sem_give((struct k_sem *)sem);
	return 0;
}

#if defined(CONFIG_ETHOS_U_DCACHE)
void ethosu_flush_dcache(uint32_t *p, size_t bytes)
{
	if (p && bytes) {
		sys_cache_data_flush_range((void *)p, bytes);
	}
}

void ethosu_invalidate_dcache(uint32_t *p, size_t bytes)
{
	if (p && bytes) {
		sys_cache_data_invd_range((void *)p, bytes);
	}
}
#endif
