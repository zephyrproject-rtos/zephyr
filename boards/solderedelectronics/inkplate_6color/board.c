/*
 * Copyright (c) 2026 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>

#if defined(CONFIG_I2C) && defined(GPIO_PCAL64XXA)

#define I2C_RECOVER_INIT_PRIORITY 69

static int board_inkplate_6color_i2c_recover(void)
{
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	return i2c_recover_bus(i2c_dev);
}

/* needs to be done before PCAL64XXA init, avoid SCL glitch on first write */
BUILD_ASSERT(I2C_RECOVER_INIT_PRIORITY < CONFIG_GPIO_PCAL64XXA_INIT_PRIORITY);
SYS_INIT(board_inkplate_6color_i2c_recover, POST_KERNEL, I2C_RECOVER_INIT_PRIORITY);

#endif /* defined(CONFIG_I2C) && defined(GPIO_PCAL64XXA) */
