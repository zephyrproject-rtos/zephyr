/* ST Microelectronics I3G4250D gyro driver
 *
 * Copyright (c) 2021 Jonathan Hahn
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/i3g4250d.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_I3G4250D_I3G4250D_H_
#define ZEPHYR_DRIVERS_SENSOR_I3G4250D_I3G4250D_H_

#include <stdint.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include "i3g4250d_reg.h"

struct i3g4250d_device_config {
	struct spi_dt_spec spi;
};

/* sensor data */
struct i3g4250d_data {
	int16_t angular_rate[3];
	stmdev_ctx_t *ctx;
};

int i3g4250d_spi_init(const struct device *dev);

#endif /* __SENSOR_I3G4250D__ */
