/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/sys/slist.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

static sys_slist_t callbacks = SYS_SLIST_STATIC_INIT(&callbacks);
static K_MUTEX_DEFINE(callbacks_lock);

void bmc_callback_register(struct bmc_callback *cb)
{
	k_mutex_lock(&callbacks_lock, K_FOREVER);
	sys_slist_append(&callbacks, &cb->node);
	k_mutex_unlock(&callbacks_lock);
}

void bmc_callback_unregister(struct bmc_callback *cb)
{
	k_mutex_lock(&callbacks_lock, K_FOREVER);
	(void)sys_slist_find_and_remove(&callbacks, &cb->node);
	k_mutex_unlock(&callbacks_lock);
}

void bmc_event_notify(uint32_t event, void *data, size_t len)
{
	struct bmc_callback *cb;

	k_mutex_lock(&callbacks_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&callbacks, cb, node) {
		int ret;

		if ((cb->event_mask & event) == 0) {
			continue;
		}

		ret = cb->handler(event, data, len);
		if (ret < 0) {
			LOG_WRN("Callback for event 0x%08x failed (err=%d)", event, ret);
		}
	}

	k_mutex_unlock(&callbacks_lock);
}
