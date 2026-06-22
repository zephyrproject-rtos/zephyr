/*
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MLX90394_MLX90394_H_
#define ZEPHYR_DRIVERS_SENSOR_MLX90394_MLX90394_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#include "mlx90394_reg.h"

/*
 * Time it takes to start-up the device and switch to powerdown mode (after powercycle or soft
 * reset)
 */
#define MLX90394_STARTUP_TIME_US 400

/* Conversion values */
#define MLX90394_HIGH_RANGE_MICRO_GAUSS_PER_BIT       INT64_C(15000)
#define MLX90394_HIGH_SENSITIVITY_MICRO_GAUSS_PER_BIT INT64_C(1500)
#define MLX90394_MICRO_CELSIUS_PER_BIT                INT64_C(20000)

/* values for setting SENSOR_ATTR_FULL_SCALE */
#define MLX90394_ATTR_FS_HIGH_G INT32_C(500)
#define MLX90394_ATTR_FS_LOW_G  INT32_C(50)

struct mlx90394_data {
	struct __packed {
		uint8_t stat1;
		uint8_t x_l;
		uint8_t x_h;
		uint8_t y_l;
		uint8_t y_h;
		uint8_t z_l;
		uint8_t z_h;
		uint8_t stat2;
		uint8_t temp_l;
		uint8_t temp_h;
	} sample;
	enum sensor_channel channel;
	enum mlx90394_reg_config_val config_val;
	int32_t measurement_time_us;
	struct __packed {
		uint8_t ctrl1;
		uint8_t ctrl2;
		uint8_t ctrl3;
		uint8_t ctrl4;
	} ctrl_reg_values;
	bool initialized;
#ifdef CONFIG_SENSOR_ASYNC_API
	struct {
		struct rtio_iodev_sqe *iodev_sqe;
		uint64_t timestamp;
		enum mlx90394_reg_config_val config_val;
	} work_ctx;
	struct k_work_delayable async_fetch_work;
	const struct device *dev;
#endif
};

struct mlx90394_config {
	struct i2c_dt_spec i2c;
};

int mlx90394_sample_fetch_internal(const struct device *dev, enum sensor_channel chan);
int mlx90394_trigger_measurement_internal(const struct device *dev, enum sensor_channel chan);

/* RTIO types and defines */
#ifdef CONFIG_SENSOR_ASYNC_API

/* shift value to use. */
#define MLX90394_SHIFT_MAGN_HIGH_SENSITIVITY (6)
#define MLX90394_SHIFT_MAGN_HIGH_RANGE       (9)
#define MLX90394_SHIFT_TEMP                  (10)

void mlx90394_async_fetch(struct k_work *work);

struct mlx90394_decoder_header {
	uint64_t timestamp;
	enum mlx90394_reg_config_val config_val;
};

struct mlx90394_encoded_data {
	struct mlx90394_decoder_header header;
	int16_t readings[4];
};

int mlx90394_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
void mlx90394_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_MLX90394_MLX90394_H_ */
