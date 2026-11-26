/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bus access for the BMI260. Every register access in the driver goes
 * through one of these, so that no other file has to know which bus the
 * device is on.
 */

#include <zephyr/kernel.h>

#include "bmi260.h"

int bmi260_bus_check(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

int bmi260_bus_init(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->init(&cfg->bus);
}

int bmi260_reg_read(const struct device *dev, uint8_t start, uint8_t *data, uint16_t len)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, data, len);
}

int bmi260_reg_write(const struct device *dev, uint8_t start, const uint8_t *data, uint16_t len)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, start, data, len);
}

int bmi260_reg_write_with_delay(const struct device *dev, uint8_t start, const uint8_t *data,
				uint16_t len, uint32_t delay_us)
{
	int ret;

	ret = bmi260_reg_write(dev, start, data, len);
	if (ret != 0) {
		return ret;
	}

	k_usleep(delay_us);

	return 0;
}
