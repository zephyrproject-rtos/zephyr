/* ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/ism330dhcx.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ISM330DHCX_ISM330DHCX_H_
#define ZEPHYR_DRIVERS_SENSOR_ISM330DHCX_ISM330DHCX_H_

#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/util.h>
#include "ism330dhcx_reg.h"

union axis3bit16_t {
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

union axis1bit16_t {
	int16_t i16bit;
	uint8_t u8bit[2];
};

#define ISM330DHCX_EN_BIT					0x01
#define ISM330DHCX_DIS_BIT					0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define GAIN_UNIT_XL				(61LL)

/* Gyro sensor sensitivity grain is 4.375 udps/LSB */
#define GAIN_UNIT_G				(4375LL)

#define SENSOR_PI_DOUBLE			(SENSOR_PI / 1000000.0)
#define SENSOR_DEG2RAD_DOUBLE			(SENSOR_PI_DOUBLE / 180)
#define SENSOR_G_DOUBLE				(SENSOR_G / 1000000.0)

#if CONFIG_ISM330DHCX_ACCEL_FS == 0
	#define ISM330DHCX_ACCEL_FS_RUNTIME 1
	#define ISM330DHCX_DEFAULT_ACCEL_FULLSCALE		0
	#define ISM330DHCX_DEFAULT_ACCEL_SENSITIVITY	GAIN_UNIT_XL
#elif CONFIG_ISM330DHCX_ACCEL_FS == 2
	#define ISM330DHCX_DEFAULT_ACCEL_FULLSCALE		0
	#define ISM330DHCX_DEFAULT_ACCEL_SENSITIVITY	GAIN_UNIT_XL
#elif CONFIG_ISM330DHCX_ACCEL_FS == 4
	#define ISM330DHCX_DEFAULT_ACCEL_FULLSCALE		2
	#define ISM330DHCX_DEFAULT_ACCEL_SENSITIVITY	(2.0 * GAIN_UNIT_XL)
#elif CONFIG_ISM330DHCX_ACCEL_FS == 8
	#define ISM330DHCX_DEFAULT_ACCEL_FULLSCALE		3
	#define ISM330DHCX_DEFAULT_ACCEL_SENSITIVITY	(4.0 * GAIN_UNIT_XL)
#elif CONFIG_ISM330DHCX_ACCEL_FS == 16
	#define ISM330DHCX_DEFAULT_ACCEL_FULLSCALE		1
	#define ISM330DHCX_DEFAULT_ACCEL_SENSITIVITY	(8.0 * GAIN_UNIT_XL)
#endif

#if (CONFIG_ISM330DHCX_ACCEL_ODR == 0)
#define ISM330DHCX_ACCEL_ODR_RUNTIME 1
#endif

#define GYRO_FULLSCALE_125 4

#if CONFIG_ISM330DHCX_GYRO_FS == 0
	#define ISM330DHCX_GYRO_FS_RUNTIME 1
	#define ISM330DHCX_DEFAULT_GYRO_FULLSCALE		4
	#define ISM330DHCX_DEFAULT_GYRO_SENSITIVITY	GAIN_UNIT_G
#elif CONFIG_ISM330DHCX_GYRO_FS == 125
	#define ISM330DHCX_DEFAULT_GYRO_FULLSCALE		4
	#define ISM330DHCX_DEFAULT_GYRO_SENSITIVITY	GAIN_UNIT_G
#elif CONFIG_ISM330DHCX_GYRO_FS == 250
	#define ISM330DHCX_DEFAULT_GYRO_FULLSCALE		0
	#define ISM330DHCX_DEFAULT_GYRO_SENSITIVITY	(2.0 * GAIN_UNIT_G)
#elif CONFIG_ISM330DHCX_GYRO_FS == 500
	#define ISM330DHCX_DEFAULT_GYRO_FULLSCALE		1
	#define ISM330DHCX_DEFAULT_GYRO_SENSITIVITY	(4.0 * GAIN_UNIT_G)
#elif CONFIG_ISM330DHCX_GYRO_FS == 1000
	#define ISM330DHCX_DEFAULT_GYRO_FULLSCALE		2
	#define ISM330DHCX_DEFAULT_GYRO_SENSITIVITY	(8.0 * GAIN_UNIT_G)
#elif CONFIG_ISM330DHCX_GYRO_FS == 2000
	#define ISM330DHCX_DEFAULT_GYRO_FULLSCALE		3
	#define ISM330DHCX_DEFAULT_GYRO_SENSITIVITY	(16.0 * GAIN_UNIT_G)
#endif


#if (CONFIG_ISM330DHCX_GYRO_ODR == 0)
#define ISM330DHCX_GYRO_ODR_RUNTIME 1
#endif

struct ism330dhcx_config {
	char *bus_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_ISM330DHCX_TRIGGER
	const char *int_gpio_port;
	uint8_t int_gpio_pin;
	uint8_t int_gpio_flags;
	uint8_t int_pin;
#endif /* CONFIG_ISM330DHCX_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	uint16_t i2c_slv_addr;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_config spi_conf;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	const char *gpio_cs_port;
	uint8_t cs_gpio;
	uint8_t cs_gpio_flags;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
};

union samples {
	uint8_t raw[6];
	struct {
		int16_t axis[3];
	};
} __aligned(2);

/* sensor data forward declaration (member definition is below) */
struct ism330dhcx_data;

struct ism330dhcx_tf {
	int (*read_data)(struct ism330dhcx_data *data, uint8_t reg_addr,
			 uint8_t *value, uint8_t len);
	int (*write_data)(struct ism330dhcx_data *data, uint8_t reg_addr,
			  uint8_t *value, uint8_t len);
	int (*read_reg)(struct ism330dhcx_data *data, uint8_t reg_addr,
			uint8_t *value);
	int (*write_reg)(struct ism330dhcx_data *data, uint8_t reg_addr,
			uint8_t value);
	int (*update_reg)(struct ism330dhcx_data *data, uint8_t reg_addr,
			  uint8_t mask, uint8_t value);
};

#define ISM330DHCX_SHUB_MAX_NUM_SLVS			2

struct ism330dhcx_data {
	struct device *bus;
	int16_t acc[3];
	uint32_t acc_gain;
	int16_t gyro[3];
	uint32_t gyro_gain;
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
	int temp_sample;
#endif
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
	uint8_t ext_data[2][6];
	uint16_t magn_gain;

	struct hts221_data {
		int16_t x0;
		int16_t x1;
		int16_t y0;
		int16_t y1;
	} hts221;
#endif /* CONFIG_ISM330DHCX_SENSORHUB */

	stmdev_ctx_t *ctx;

	#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
	#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
	#endif

	uint16_t accel_freq;
	uint8_t accel_fs;
	uint16_t gyro_freq;
	uint8_t gyro_fs;

#ifdef CONFIG_ISM330DHCX_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	sensor_trigger_handler_t handler_drdy_gyr;
	sensor_trigger_handler_t handler_drdy_temp;

	struct device *dev;

#if defined(CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ISM330DHCX_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ISM330DHCX_TRIGGER */

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif
};

int ism330dhcx_spi_init(struct device *dev);
int ism330dhcx_i2c_init(struct device *dev);
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
int ism330dhcx_shub_init(struct device *dev);
int ism330dhcx_shub_fetch_external_devs(struct device *dev);
int ism330dhcx_shub_get_idx(enum sensor_channel type);
int ism330dhcx_shub_config(struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr,
			const struct sensor_value *val);
#endif /* CONFIG_ISM330DHCX_SENSORHUB */

#ifdef CONFIG_ISM330DHCX_TRIGGER
int ism330dhcx_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int ism330dhcx_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ISM330DHCX_ISM330DHCX_H_ */
