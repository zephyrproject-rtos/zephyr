/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_prbs

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <errno.h>
#include <zephyr/init.h>
#include <soc.h>
#include <string.h>
#include <zephyr/kernel.h>

#define PRBS_STATUS     DT_INST_REG_ADDR(0)
#define PRBS_WIDTH      DT_INST_REG_SIZE(0)

static int entropy_prbs_get_entropy(const struct device *dev, uint8_t *buffer,
					 uint16_t length)
{
	while (length > 0) {
		size_t to_copy;
		uint32_t value;

		value = litex_read(PRBS_STATUS, PRBS_WIDTH);
		to_copy = MIN(length, sizeof(value));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}
	return 0;
}

static int entropy_prbs_init(const struct device *dev)
{
	return 0;
}

static const struct entropy_driver_api entropy_prbs_api = {
	.get_entropy = entropy_prbs_get_entropy
};

DEVICE_DT_INST_DEFINE(0,
		    entropy_prbs_init, NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_prbs_api);
