/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_prbs

#include <device.h>
#include <drivers/entropy.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include <string.h>
#include <zephyr.h>

#define PRBS_STATUS     ((volatile uint32_t *)DT_INST_REG_ADDR(0))
#define PRBS_WIDTH      DT_INST_REG_SIZE(0)
#define SUBREG_SIZE_BIT 8

static inline unsigned int prbs_read(volatile uint32_t *reg_status,
					 volatile uint32_t reg_width)
{
	uint32_t shifted_data, shift, i;
	uint32_t result = 0;

	for (i = 0; i < reg_width; ++i) {
		shifted_data = *(reg_status + i);
		shift = (reg_width - i - 1) * SUBREG_SIZE_BIT;
		result |= (shifted_data << shift);
	}

	return result;
}

static int entropy_prbs_get_entropy(const struct device *dev, uint8_t *buffer,
					 uint16_t length)
{
	while (length > 0) {
		size_t to_copy;
		uint32_t value;

		value = prbs_read(PRBS_STATUS, PRBS_WIDTH);
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
		    entropy_prbs_init, device_pm_control_nop, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_prbs_api);
