/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSO_LSM6DSO_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSO_LSM6DSO_H_

#include <sensor.h>
#include <zephyr/types.h>
#include <gpio.h>
#include <drivers/spi.h>
#include <sys/util.h>
#include "lsm6dso_reg.h"

union axis3bit16_t {
	s16_t i16bit[3];
	u8_t u8bit[6];
};

union axis1bit16_t {
	s16_t i16bit;
	u8_t u8bit[2];
};

#define LSM6DSO_EN_BIT					0x01
#define LSM6DSO_DIS_BIT					0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define GAIN_UNIT_XL				(61LL)

/* Gyro sensor sensitivity grain is 4.375 udps/LSB */
#define GAIN_UNIT_G				(4375LL)

#define SENSOR_PI_DOUBLE			(SENSOR_PI / 1000000.0)
#define SENSOR_DEG2RAD_DOUBLE			(SENSOR_PI_DOUBLE / 180)
#define SENSOR_G_DOUBLE				(SENSOR_G / 1000000.0)

#if CONFIG_LSM6DSO_ACCEL_FS == 0
	#define LSM6DSO_ACCEL_FS_RUNTIME 1
	#define LSM6DSO_DEFAULT_ACCEL_FULLSCALE		0
	#define LSM6DSO_DEFAULT_ACCEL_SENSITIVITY	GAIN_UNIT_XL
#elif CONFIG_LSM6DSO_ACCEL_FS == 2
	#define LSM6DSO_DEFAULT_ACCEL_FULLSCALE		0
	#define LSM6DSO_DEFAULT_ACCEL_SENSITIVITY	GAIN_UNIT_XL
#elif CONFIG_LSM6DSO_ACCEL_FS == 4
	#define LSM6DSO_DEFAULT_ACCEL_FULLSCALE		2
	#define LSM6DSO_DEFAULT_ACCEL_SENSITIVITY	(2.0 * GAIN_UNIT_XL)
#elif CONFIG_LSM6DSO_ACCEL_FS == 8
	#define LSM6DSO_DEFAULT_ACCEL_FULLSCALE		3
	#define LSM6DSO_DEFAULT_ACCEL_SENSITIVITY	(4.0 * GAIN_UNIT_XL)
#elif CONFIG_LSM6DSO_ACCEL_FS == 16
	#define LSM6DSO_DEFAULT_ACCEL_FULLSCALE		1
	#define LSM6DSO_DEFAULT_ACCEL_SENSITIVITY	(8.0 * GAIN_UNIT_XL)
#endif

#if (CONFIG_LSM6DSO_ACCEL_ODR == 0)
#define LSM6DSO_ACCEL_ODR_RUNTIME 1
#endif

#define GYRO_FULLSCALE_125 4

#if CONFIG_LSM6DSO_GYRO_FS == 0
	#define LSM6DSO_GYRO_FS_RUNTIME 1
	#define LSM6DSO_DEFAULT_GYRO_FULLSCALE		4
	#define LSM6DSO_DEFAULT_GYRO_SENSITIVITY	GAIN_UNIT_G
#elif CONFIG_LSM6DSO_GYRO_FS == 125
	#define LSM6DSO_DEFAULT_GYRO_FULLSCALE		4
	#define LSM6DSO_DEFAULT_GYRO_SENSITIVITY	GAIN_UNIT_G
#elif CONFIG_LSM6DSO_GYRO_FS == 250
	#define LSM6DSO_DEFAULT_GYRO_FULLSCALE		0
	#define LSM6DSO_DEFAULT_GYRO_SENSITIVITY	(2.0 * GAIN_UNIT_G)
#elif CONFIG_LSM6DSO_GYRO_FS == 500
	#define LSM6DSO_DEFAULT_GYRO_FULLSCALE		1
	#define LSM6DSO_DEFAULT_GYRO_SENSITIVITY	(4.0 * GAIN_UNIT_G)
#elif CONFIG_LSM6DSO_GYRO_FS == 1000
	#define LSM6DSO_DEFAULT_GYRO_FULLSCALE		2
	#define LSM6DSO_DEFAULT_GYRO_SENSITIVITY	(8.0 * GAIN_UNIT_G)
#elif CONFIG_LSM6DSO_GYRO_FS == 2000
	#define LSM6DSO_DEFAULT_GYRO_FULLSCALE		3
	#define LSM6DSO_DEFAULT_GYRO_SENSITIVITY	(16.0 * GAIN_UNIT_G)
#endif


#if (CONFIG_LSM6DSO_GYRO_ODR == 0)
#define LSM6DSO_GYRO_ODR_RUNTIME 1
#endif

struct lsm6dso_config {
	char *bus_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_LSM6DSO_TRIGGER
	const char *int_gpio_port;
	u8_t int_gpio_pin;
	u8_t int_pin;
#endif /* CONFIG_LSM6DSO_TRIGGER */
#ifdef DT_ST_LSM6DSO_BUS_I2C
	u16_t i2c_slv_addr;
#elif DT_ST_LSM6DSO_BUS_SPI
	struct spi_config spi_conf;
#if defined(DT_INST_0_ST_LSM6DSO_CS_GPIOS_CONTROLLER)
	const char *gpio_cs_port;
	u8_t cs_gpio;
#endif /* DT_INST_0_ST_LSM6DSO_CS_GPIOS_CONTROLLER */
#endif /* DT_ST_LSM6DSO_BUS_I2C */
};

union samples {
	u8_t raw[6];
	struct {
		s16_t axis[3];
	};
} __aligned(2);

/* sensor data forward declaration (member definition is below) */
struct lsm6dso_data;

struct lsm6dso_tf {
	int (*read_data)(struct lsm6dso_data *data, u8_t reg_addr,
			 u8_t *value, u8_t len);
	int (*write_data)(struct lsm6dso_data *data, u8_t reg_addr,
			  u8_t *value, u8_t len);
	int (*read_reg)(struct lsm6dso_data *data, u8_t reg_addr,
			u8_t *value);
	int (*write_reg)(struct lsm6dso_data *data, u8_t reg_addr,
			u8_t value);
	int (*update_reg)(struct lsm6dso_data *data, u8_t reg_addr,
			  u8_t mask, u8_t value);
};

#define LSM6DSO_SHUB_MAX_NUM_SLVS			2

struct lsm6dso_data {
	struct device *bus;
	s16_t acc[3];
	u32_t acc_gain;
	s16_t gyro[3];
	u32_t gyro_gain;
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
	int temp_sample;
#endif
#if defined(CONFIG_LSM6DSO_SENSORHUB)
	u8_t ext_data[2][6];
	u16_t magn_gain;

	struct hts221_data {
		s16_t x0;
		s16_t x1;
		s16_t y0;
		s16_t y1;
	} hts221;
#endif /* CONFIG_LSM6DSO_SENSORHUB */

	stmdev_ctx_t *ctx;

	#ifdef DT_ST_LSM6DSO_BUS_I2C
	stmdev_ctx_t ctx_i2c;
	#elif DT_ST_LSM6DSO_BUS_SPI
	stmdev_ctx_t ctx_spi;
	#endif

	u16_t accel_freq;
	u8_t accel_fs;
	u16_t gyro_freq;
	u8_t gyro_fs;

#ifdef CONFIG_LSM6DSO_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	sensor_trigger_handler_t handler_drdy_gyr;
	sensor_trigger_handler_t handler_drdy_temp;

#if defined(CONFIG_LSM6DSO_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LSM6DSO_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LSM6DSO_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif
#endif /* CONFIG_LSM6DSO_TRIGGER */

#if defined(DT_INST_0_ST_LSM6DSO_CS_GPIOS_CONTROLLER)
	struct spi_cs_control cs_ctrl;
#endif
};

int lsm6dso_spi_init(struct device *dev);
int lsm6dso_i2c_init(struct device *dev);
#if defined(CONFIG_LSM6DSO_SENSORHUB)
int lsm6dso_shub_init(struct device *dev);
int lsm6dso_shub_fetch_external_devs(struct device *dev);
int lsm6dso_shub_get_idx(enum sensor_channel type);
int lsm6dso_shub_config(struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr,
			const struct sensor_value *val);
#endif /* CONFIG_LSM6DSO_SENSORHUB */

#ifdef CONFIG_LSM6DSO_TRIGGER
int lsm6dso_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lsm6dso_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSO_LSM6DSO_H_ */
