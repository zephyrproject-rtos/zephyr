/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwspinlock.h>

#define DT_DRV_COMPAT vnd_hwspinlock

static void vnd_hwspinlock_lock(const struct device *dev, uint32_t id)
{
}

static void vnd_hwspinlock_unlock(const struct device *dev, uint32_t id)
{
}

static uint32_t vnd_hwspinlock_get_max_id(const struct device *dev)
{
	return 0;
}

static DEVICE_API(hwspinlock, vnd_hwspinlock_api) = {
	.lock = vnd_hwspinlock_lock,
	.unlock = vnd_hwspinlock_unlock,
	.get_max_id = vnd_hwspinlock_get_max_id,
};

#define VND_HWSPINLOCK_INIT(idx)					\
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, NULL, NULL, POST_KERNEL,	\
			      CONFIG_HWSPINLOCK_INIT_PRIORITY,		\
			      &vnd_hwspinlock_api)

DT_INST_FOREACH_STATUS_OKAY(VND_HWSPINLOCK_INIT);
