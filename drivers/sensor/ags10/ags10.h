/*
 * Copyright (c) 2023 Balthazar Deliers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_AGS10_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_AGS10_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define AGS10_CMD_DATA_ACQUISITION       0x00
#define AGS10_CMD_ZERO_POINT_CALIBRATION 0x01
#define AGS10_CMD_READ_VERSION           0x11
#define AGS10_CMD_READ_RESISTANCE        0x20
#define AGS10_CMD_MODIFY_SLAVE_ADDRESS   0x21

#define AGS10_REG_ZERO_POINT_CALIBRATION_RESET 0xFFFF /* Reset to the factory value */
#define AGS10_REG_ZERO_POINT_CALIBRATION_SET   0x0000 /* Set sensor resistance to zero-point */
#define AGS10_REG_STATUS_NRDY_READY            0x00   /* Device is ready */
#define AGS10_REG_STATUS_CH_PPB                0x00   /* Unit is PPB */

#define AGS10_MSK_STATUS      0x0F
#define AGS10_MSK_STATUS_NRDY 0x01
#define AGS10_MSK_STATUS_CH   0x0E

struct ags10_config {
	struct i2c_dt_spec bus;
};

struct ags10_data {
	uint32_t tvoc_ppb;
	uint8_t status;
	uint32_t version;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_AGS10_H_ */
