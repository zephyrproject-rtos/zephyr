/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv16x.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSV16X_LSM6DSV16X_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSV16X_LSM6DSV16X_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lsm6dsv16x_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define LSM6DSV16X_EN_BIT					0x01
#define LSM6DSV16X_DIS_BIT					0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define GAIN_UNIT_XL				(61LL)

/* Gyro sensor sensitivity grain is 4.375 udps/LSB */
#define GAIN_UNIT_G				(4375LL)

int lsm6dsv16x_calc_accel_gain(uint8_t fs);
int lsm6dsv16x_calc_gyro_gain(uint8_t fs);

struct lsm6dsv16x_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t accel_pm;
	uint8_t accel_odr;
	uint8_t accel_range;
	uint8_t gyro_pm;
	uint8_t gyro_odr;
	uint8_t gyro_range;
	uint8_t drdy_pulsed;
#ifdef CONFIG_LSM6DSV16X_STREAM
	uint8_t fifo_wtm;
	uint8_t accel_batch : 4;
	uint8_t gyro_batch : 4;
	uint8_t temp_batch : 2;
#endif
#ifdef CONFIG_LSM6DSV16X_TRIGGER
	const struct gpio_dt_spec int1_gpio;
	const struct gpio_dt_spec int2_gpio;
	uint8_t drdy_pin;
	bool trig_enabled;
#endif /* CONFIG_LSM6DSV16X_TRIGGER */
};

union samples {
	uint8_t raw[6];
	struct {
		int16_t axis[3];
	};
} __aligned(2);

#define LSM6DSV16X_SHUB_MAX_NUM_TARGETS			3

struct lsm6dsv16x_data {
	const struct device *dev;
	int16_t acc[3];
	uint32_t acc_gain;
	int16_t gyro[3];
	uint32_t gyro_gain;
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	int16_t temp_sample;
#endif
#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
	uint8_t ext_data[LSM6DSV16X_SHUB_MAX_NUM_TARGETS][6];
	uint16_t magn_gain;

	struct hts221_data {
		int16_t x0;
		int16_t x1;
		int16_t y0;
		int16_t y1;
	} hts221;
	bool shub_inited;
	uint8_t num_ext_dev;
	uint8_t shub_ext[LSM6DSV16X_SHUB_MAX_NUM_TARGETS];
#endif /* CONFIG_LSM6DSV16X_SENSORHUB */

	uint8_t accel_freq;
	uint8_t accel_fs;
	uint8_t gyro_freq;
	uint8_t gyro_fs;

#ifdef CONFIG_LSM6DSV16X_STREAM
	struct rtio_iodev_sqe *streaming_sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint64_t fifo_timestamp;
	uint8_t fifo_status[2];
	uint16_t fifo_count;
	uint8_t fifo_irq;
	uint8_t accel_batch_odr : 4;
	uint8_t gyro_batch_odr : 4;
	uint8_t temp_batch_odr : 2;
	uint8_t bus_type : 1; /* I2C is 0, SPI is 1 */
	uint8_t reserved : 5;
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
	struct gpio_dt_spec *drdy_gpio;

	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;
	sensor_trigger_handler_t handler_drdy_gyr;
	const struct sensor_trigger *trig_drdy_gyr;
	sensor_trigger_handler_t handler_drdy_temp;
	const struct sensor_trigger *trig_drdy_temp;

#if defined(CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LSM6DSV16X_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_LSM6DSV16X_TRIGGER */
};

#ifdef CONFIG_LSM6DSV16X_STREAM
#define BUS_I2C 0
#define BUS_SPI 1

static inline uint8_t lsm6dsv16x_bus_reg(struct lsm6dsv16x_data *data, uint8_t x)
{
	return (data->bus_type == BUS_SPI) ? x | 0x80 : x;
}

#define LSM6DSV16X_FIFO_ITEM_LEN 7
#define LSM6DSV16X_FIFO_SIZE(x) (x * LSM6DSV16X_FIFO_ITEM_LEN)
#endif

#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
int lsm6dsv16x_shub_init(const struct device *dev);
int lsm6dsv16x_shub_fetch_external_devs(const struct device *dev);
int lsm6dsv16x_shub_get_idx(const struct device *dev, enum sensor_channel type);
int lsm6dsv16x_shub_config(const struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr,
			const struct sensor_value *val);
#endif /* CONFIG_LSM6DSV16X_SENSORHUB */

#ifdef CONFIG_LSM6DSV16X_TRIGGER
int lsm6dsv16x_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lsm6dsv16x_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSV16X_LSM6DSV16X_H_ */
