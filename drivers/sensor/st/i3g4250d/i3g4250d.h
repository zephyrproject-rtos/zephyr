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
#include <zephyr/types.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "i3g4250d_reg.h"

#define DT_DRV_COMPAT_I3G4250D st_i3g4250d

#define I3G4250D_ANY_INST_ON_BUS_STATUS_OKAY(bus)         \
(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(DT_DRV_COMPAT_I3G4250D, bus))


#if I3G4250D_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* I3G4250D_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

struct i3g4250d_device_config {
	stmdev_ctx_t ctx;
	union {
	#if I3G4250D_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
	#endif
	} stmemsc_cfg;
};

/* sensor data */
struct i3g4250d_data {
	int16_t angular_rate[3];
};

#endif /* __SENSOR_I3G4250D__ */
