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

/*
 * Zephyr k_mutex is recursive and priority-inheriting, so the plain and
 * recursive create paths are identical.
 */
static osal_mutex_t mutex_create(void)
{
	struct k_mutex *mtx = osal_cb_alloc(sizeof(struct k_mutex));

	if (mtx == NULL) {
		LOG_ERR("mutex alloc failed");
		return NULL;
	}

	k_mutex_init(mtx);

	return (osal_mutex_t)mtx;
}

osal_mutex_t osal_mutex_create(void)
{
	return mutex_create();
}

osal_mutex_t osal_mutex_create_recursive(void)
{
	return mutex_create();
}

int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
	int ret;

	if (mutex == NULL) {
		return OSAL_INVAL;
	}

	ret = k_mutex_lock((struct k_mutex *)mutex, osal_ms_to_timeout(timeout_ms));
	if (ret == 0) {
		return OSAL_OK;
	}

	/* -EBUSY: contended with no wait. -EAGAIN: timed out. Both mean "not held". */
	return (ret == -EAGAIN || ret == -EBUSY) ? OSAL_TIMEOUT : OSAL_ERR;
}

int osal_mutex_unlock(osal_mutex_t mutex)
{
	if (mutex == NULL) {
		return OSAL_INVAL;
	}

	return (k_mutex_unlock((struct k_mutex *)mutex) == 0) ? OSAL_OK : OSAL_ERR;
}

void osal_mutex_delete(osal_mutex_t mutex)
{
	if (mutex == NULL) {
		return;
	}

	osal_cb_free(mutex);
}
