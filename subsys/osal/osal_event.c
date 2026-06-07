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

osal_event_t osal_event_create(void)
{
	struct k_event *e = osal_cb_alloc(sizeof(struct k_event));

	if (e == NULL) {
		LOG_ERR("event alloc failed");
		return NULL;
	}

	k_event_init(e);

	return (osal_event_t)e;
}

uint32_t osal_event_set(osal_event_t event, uint32_t bits)
{
	if (event == NULL) {
		return 0;
	}

	return k_event_post((struct k_event *)event, bits);
}

uint32_t osal_event_clear(osal_event_t event, uint32_t bits)
{
	if (event == NULL) {
		return 0;
	}

	return k_event_clear((struct k_event *)event, bits);
}

uint32_t osal_event_wait(osal_event_t event, uint32_t bits, bool wait_all, bool clear,
			 uint32_t timeout_ms)
{
	struct k_event *e = event;
	k_timeout_t timeout = osal_ms_to_timeout(timeout_ms);
	uint32_t matched;

	if (e == NULL) {
		return 0;
	}

	/*
	 * The *_safe variants atomically clear the matched bits before
	 * returning, closing the race where a concurrent set of a matched bit
	 * lands between the wait and a separate clear and is silently lost.
	 */
	if (wait_all) {
		matched = clear ? k_event_wait_all_safe(e, bits, false, timeout)
				: k_event_wait_all(e, bits, false, timeout);
	} else {
		matched = clear ? k_event_wait_safe(e, bits, false, timeout)
				: k_event_wait(e, bits, false, timeout);
	}

	return matched;
}

void osal_event_delete(osal_event_t event)
{
	if (event == NULL) {
		return;
	}

	osal_cb_free(event);
}
