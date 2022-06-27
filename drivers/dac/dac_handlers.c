/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dac.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/kernel.h>

static inline int z_vrfy_dac_channel_setup(const struct device *dev,
					   const struct dac_channel_cfg *user_channel_cfg)
{
	struct dac_channel_cfg channel_cfg;

	Z_OOPS(Z_SYSCALL_DRIVER_DAC(dev, channel_setup));
	Z_OOPS(z_user_from_copy(&channel_cfg,
				(struct dac_channel_cfg *)user_channel_cfg,
				sizeof(struct dac_channel_cfg)));

	return z_impl_dac_channel_setup((const struct device *)dev,
					&channel_cfg);
}
#include <syscalls/dac_channel_setup_mrsh.c>

static inline int z_vrfy_dac_write_value(const struct device *dev,
					 uint8_t channel, uint32_t value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_DAC(dev, write_value));

	return z_impl_dac_write_value((const struct device *)dev, channel,
				      value);
}
#include <syscalls/dac_write_value_mrsh.c>
