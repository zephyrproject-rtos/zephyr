/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_hsm_trng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/drivers/misc/ti_cc35xx_hsm/ti_cc35xx_hsm.h>

struct entropy_ti_cc35xx_data {
	const struct device *hsm_dev;
};

static int ti_cc35xx_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	struct entropy_ti_cc35xx_data *data = dev->data;

	if (len == 0 || buf == NULL) {
		return -EINVAL;
	}

	const struct hsm_ti_cc35xx_driver_api *api =
		(const struct hsm_ti_cc35xx_driver_api *)data->hsm_dev->api;

	return api->get_entropy(data->hsm_dev, buf, len);
}

static int entropy_ti_cc35xx_init(const struct device *dev)
{
	const struct device *parent = DEVICE_DT_GET(DT_NODELABEL(hsm));
	struct entropy_ti_cc35xx_data *data = dev->data;

	if (!device_is_ready(parent)) {
		return -ENODEV;
	}

	data->hsm_dev = parent;

	return 0;
}

static DEVICE_API(entropy, entropy_ti_cc35xx_driver_api) = {
	.get_entropy = ti_cc35xx_get_entropy,
};

static struct entropy_ti_cc35xx_data entropy_data;

DEVICE_DT_INST_DEFINE(0, entropy_ti_cc35xx_init, PM_DEVICE_DT_INST_GET(0), &entropy_data, NULL,
		      PRE_KERNEL_2, CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ti_cc35xx_driver_api);
