/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_SENSOR_MAX17048_MAX17048_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX17048_MAX17048_H_

#include <zephyr/drivers/i2c.h>

#define REGISTER_VCELL      0x02
#define REGISTER_SOC        0x04
#define REGISTER_MODE       0x06
#define REGISTER_VERSION    0x08
#define REGISTER_HIBRT      0x0A
#define REGISTER_CONFIG     0x0C
#define REGISTER_VALRT      0x14
#define REGISTER_CRATE      0x16
#define REGISTER_VRESET     0x18
#define REGISTER_CHIP_ID    0x19
#define REGISTER_STATUS     0x1A
#define REGISTER_TABLE      0x40
#define REGISTER_COMMAND    0xFE

#define RESET_COMMAND       0x5400
#define QUICKSTART_MODE     0x4000

struct max17048_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MAX17048_MAX17048_H_ */
