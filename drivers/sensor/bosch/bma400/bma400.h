/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include "bma400_defs.h"
#include <zephyr/drivers/spi.h>

/*
 * Types
 */

#define BMA400_BUS_SPI 1

union bma400_bus_cfg {
	struct spi_dt_spec spi;
};

struct bma400_config {
	int (*bus_init)(const struct device *dev);
	const union bma400_bus_cfg bus_cfg;
	uint8_t bus_type;

	const struct gpio_dt_spec gpio_interrupt1;
	const struct gpio_dt_spec gpio_interrupt2;
};

struct bma400_hw_operations {
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr, uint8_t value);
};

struct bma400_runtime_config {
	int32_t batch_ticks;
	bool fifo_en;
#ifdef CONFIG_BMA400_WAKEUP
	const bool auto_wakeup_en;
	const bool auto_lowpower_en;
	uint8_t auto_wakeup_threshold;
	uint8_t wakeup_num_of_samples;
	uint8_t auto_lowpower_threshold;
	uint16_t auto_lowpower_duration;
#endif

#ifdef CONFIG_BMA400_STREAM
	const uint8_t int1_map;
	const uint8_t int2_map;
	bool fifo_wm_en;
	bool fifo_full_en;
	bool motion_detection_en;
	enum sensor_stream_data_opt fifo_wm_data_opt;
	enum sensor_stream_data_opt fifo_full_data_opt;
	uint8_t motion_duration;
	uint8_t motion_threshold;
	uint16_t fifo_wm;
#endif

	/** Current full-scale range setting as a register value */
	uint8_t accel_fs_range;
	/** Current bandwidth parameter (BWP) as a register value */
	uint8_t accel_bwp;
	/** Current oversampling ratio */
	uint8_t osr_lp;
	/** Current output data rate as a register value */
	uint8_t accel_odr;
};

struct bma400_data {
	struct bma400_runtime_config cfg;
	/** Pointer to bus-specific I/O API */
	const struct bma400_hw_operations *hw_ops;
	/** Chip ID value stored in BMA400_REG_CHIP_ID */
	uint8_t chip_id;
	struct rtio *r;
	struct rtio_iodev *iodev;
#ifdef CONFIG_BMA400_STREAM
	struct rtio_iodev_sqe *streaming_sqe;
	uint8_t int_status;
	uint8_t read_buf[3];
	uint16_t fifo_count;
	uint64_t timestamp;
	const struct device *dev;
	struct gpio_callback gpio_cb1;
	struct gpio_callback gpio_cb2;
	const struct gpio_dt_spec *pending_gpio_interrupt;
#endif /* CONFIG_BMA400_STREAM */
};

/* Convert BMA400 ODR register value to Hz
 * Value from register BMA400_REG_ACC_CONFIG1
 */
static inline int bma400_accel_reg_to_hz(uint8_t odr, struct sensor_value *out)
{
	static const struct sensor_value odr_values[] = {
		{0, 0},  {0, 0},  {0, 0},   {0, 0},   {0, 0},   {12, 500000},
		{25, 0}, {50, 0}, {100, 0}, {200, 0}, {400, 0}, {800, 0}};

	if (odr < BMA400_ODR_12_5HZ || odr > BMA400_ODR_800HZ) {
		return -EINVAL;
	}

	*out = odr_values[odr];

	return 0;
}

/**
 * @brief Initialize the SPI bus
 *
 * @param dev bma400 device pointer
 *
 * @retval 0 success
 * @retval -errdev Error
 */
int bma400_spi_init(const struct device *dev);

/**
 * @brief (Re)Configure the sensor for streaming with the given configuration
 *
 * @param dev bma400 device pointer
 * @param cfg bma400_runtime_config pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int bma400_stream_configure(const struct device *dev, struct bma400_runtime_config *cfg);

/**
 * @brief (Re)Configure the sensor with the given configuration
 *
 * @param dev bma400 device pointer
 * @param cfg bma400_runtime_config pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int bma400_configure(const struct device *dev, struct bma400_runtime_config *cfg);

int bma400_enter_low_power(const struct device *dev);

/**
 * @brief Safely (re)Configure the sensor with the given configuration
 *
 * Will rollback to prior configuration if new configuration is invalid
 *
 * @param dev bma400 device pointer
 * @param cfg bma400_runtime_config pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int bma400_safely_configure(const struct device *dev, struct bma400_runtime_config *cfg);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_ */
