/*
 * Copyright (c) 2025 Mohammed Billoo <mab@mab-labs.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/drivers/i2c.h>

#define REGISTER_VCELL   0x02
#define REGISTER_SOC     0x04
#define REGISTER_MODE    0x06
#define REGISTER_VERSION 0x08
#define REGISTER_HIBRT   0x0A
#define REGISTER_CONFIG  0x0C
#define REGISTER_COMMAND 0xFE

#define RESET_COMMAND   0x5400
#define QUICKSTART_MODE 0x4000

struct max17043_config {
	struct i2c_dt_spec i2c;
};
