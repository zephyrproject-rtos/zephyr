/*
 * Copyright 2022 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain/common.h>

BUILD_ASSERT(CONFIG_GPIO_SPI_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "GPIO_SPI_INIT_PRIORITY has to be lower (higher value) than SPI_PRIORITY");
