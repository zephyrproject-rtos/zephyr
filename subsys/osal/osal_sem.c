/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>
#include <zephyr/logging/log.h>

#include "osal_priv.h"

LOG_MODULE_DECLARE(osal, CONFIG_OSAL_LOG_LEVEL);

osal_sem_t osal_sem_create(uint32_t max_count, uint32_t init_count)
{
	struct k_sem *sem = osal_cb_alloc(sizeof(struct k_sem));

	if (sem == NULL) {
		LOG_ERR("sem alloc failed");
		return NULL;
	}

	if (k_sem_init(sem, init_count, max_count) != 0) {
		osal_cb_free(sem);
		return NULL;
	}

	return (osal_sem_t)sem;
}

int osal_sem_take(osal_sem_t sem, uint32_t timeout_ms)
{
	int ret;

	if (sem == NULL) {
		return OSAL_INVAL;
	}

	ret = k_sem_take((struct k_sem *)sem, osal_ms_to_timeout(timeout_ms));
	if (ret == 0) {
		return OSAL_OK;
	}

	return (ret == -EAGAIN || ret == -EBUSY) ? OSAL_TIMEOUT : OSAL_ERR;
}

int osal_sem_give(osal_sem_t sem)
{
	if (sem == NULL) {
		return OSAL_INVAL;
	}

	k_sem_give((struct k_sem *)sem);

	return OSAL_OK;
}

uint32_t osal_sem_count(osal_sem_t sem)
{
	if (sem == NULL) {
		return 0;
	}

	return k_sem_count_get((struct k_sem *)sem);
}

void osal_sem_reset(osal_sem_t sem)
{
	if (sem == NULL) {
		return;
	}

	k_sem_reset((struct k_sem *)sem);
}

void osal_sem_delete(osal_sem_t sem)
{
	if (sem == NULL) {
		return;
	}

	osal_cb_free(sem);
}
