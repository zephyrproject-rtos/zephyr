/* Copyright (c) 2026 NBuiLe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet: PS-ITG-3200A-00-01.4
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ITG3200_ITG3200_H_
#define ZEPHYR_DRIVERS_SENSOR_ITG3200_ITG3200_H_

#define ITG3200_I2C_ADDRESS 0x68U
#define ITG3200_WHO_AM_I    0x00U /* Expected value: 0x68 */
#define ITG3200_SMPLRT_DIV  0x15U /* Sample Rate Divider */
#define ITG3200_DLPF_FS     0x16U /* Digital Low Pass Filter and Full Scale */
#define ITG3200_INT_CFG     0x17U /* Interrupt Configuration */
#define ITG3200_INT_STATUS  0x1AU /* Interrupt Status */
#define ITG3200_TEMP_OUT_H  0x1BU /* Temperature Output High Byte */
#define ITG3200_TEMP_OUT_L  0x1CU /* Temperature Output Low Byte */
#define ITG3200_GYRO_XOUT_H 0x1DU /* Gyroscope X Output High Byte */
#define ITG3200_GYRO_XOUT_L 0x1EU /* Gyroscope X Output Low Byte */
#define ITG3200_GYRO_YOUT_H 0x1FU /* Gyroscope Y Output */
#define ITG3200_GYRO_YOUT_L 0x20U /* Gyroscope Y Output Low Byte */
#define ITG3200_GYRO_ZOUT_H 0x21U /* Gyroscope Z Output High Byte */
#define ITG3200_GYRO_ZOUT_L 0x22U /* Gyroscope Z Output Low Byte */
#define ITG3200_PWR_MGM     0x3EU /* Power Management */

struct itg3200_config {
	const struct i2c_dt_spec i2c;
};

struct itg3200_sample {
	int16_t raw_temp;
	int16_t raw_gyro_x;
	int16_t raw_gyro_y;
	int16_t raw_gyro_z;
};

struct itg3200_data {
	uint8_t smplrt_div;
	uint8_t fs_sel;
	uint8_t dlpf_cfg;
	uint8_t int_cfg;
	uint8_t pwr_mgm;
	struct itg3200_sample sample;
};

#endif /* __SENSOR_ITG3200__ */
