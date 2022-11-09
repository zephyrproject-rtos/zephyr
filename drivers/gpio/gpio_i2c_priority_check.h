/*
 * Copyright 2022 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain/common.h>

BUILD_ASSERT(CONFIG_GPIO_I2C_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY,
	     "GPIO_I2C_INIT_PRIORITY has to be lower (higher value) than I2C_PRIORITY");
