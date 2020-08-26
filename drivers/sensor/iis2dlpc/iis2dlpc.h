/* ST Microelectronics IIS2DLPC 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dlpc.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_

#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include "iis2dlpc_reg.h"

union axis3bit16_t {
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

#if defined(CONFIG_IIS2DLPC_ODR_1_6)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_1Hz6_LP_ONLY
#elif defined(CONFIG_IIS2DLPC_ODR_12_5)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_12Hz5
#elif defined(CONFIG_IIS2DLPC_ODR_25)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_25Hz
#elif defined(CONFIG_IIS2DLPC_ODR_50)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_50Hz
#elif defined(CONFIG_IIS2DLPC_ODR_100) || \
	defined(CONFIG_IIS2DLPC_ODR_RUNTIME)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_100Hz
#elif defined(CONFIG_IIS2DLPC_ODR_200)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_200Hz
#elif defined(CONFIG_IIS2DLPC_ODR_400)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_400Hz
#elif defined(CONFIG_IIS2DLPC_ODR_800)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_800Hz
#elif defined(CONFIG_IIS2DLPC_ODR_1600)
	#define IIS2DLPC_DEFAULT_ODR	IIS2DLPC_XL_ODR_1k6Hz
#endif

/* Return ODR reg value based on data rate set */
#define IIS2DLPC_ODR_TO_REG(_odr) \
	((_odr <= 1) ? IIS2DLPC_XL_ODR_1Hz6_LP_ONLY : \
	 (_odr <= 12) ? IIS2DLPC_XL_ODR_12Hz5 : \
	 ((31 - __builtin_clz(_odr / 25))) + 3)

/* FS reg value from Full Scale */
#define IIS2DLPC_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

#if defined(CONFIG_IIS2DLPC_ACCEL_RANGE_RUNTIME) || \
	defined(CONFIG_IIS2DLPC_ACCEL_RANGE_2G)
	#define IIS2DLPC_ACC_FS		IIS2DLPC_2g
#elif defined(CONFIG_IIS2DLPC_ACCEL_RANGE_4G)
	#define IIS2DLPC_ACC_FS		IIS2DLPC_4g
#elif defined(CONFIG_IIS2DLPC_ACCEL_RANGE_8G)
	#define IIS2DLPC_ACC_FS		IIS2DLPC_8g
#elif defined(CONFIG_IIS2DLPC_ACCEL_RANGE_16G)
	#define IIS2DLPC_ACC_FS		IIS2DLPC_16g
#endif

/* Acc Gain value in ug/LSB in High Perf mode */
#define IIS2DLPC_FS_2G_GAIN		244
#define IIS2DLPC_FS_4G_GAIN		488
#define IIS2DLPC_FS_8G_GAIN		976
#define IIS2DLPC_FS_16G_GAIN		1952

#define IIS2DLPC_SHFT_GAIN_NOLP1	2
#define IIS2DLPC_ACCEL_GAIN_DEFAULT_VAL IIS2DLPC_FS_2G_GAIN
#define IIS2DLPC_FS_TO_GAIN(_fs, _lp1) \
		(IIS2DLPC_FS_2G_GAIN << ((_fs) + (_lp1)))

/* shift value for power mode */
#define IIS2DLPC_SHIFT_PM1		4
#define IIS2DLPC_SHIFT_PMOTHER		2

/**
 * struct iis2dlpc_device_config - iis2dlpc hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @int_gpio_port: Pointer to GPIO PORT identifier.
 * @int_gpio_pin: GPIO pin number connecter to sensor int pin.
 * @int_pin: Sensor int pin (int1/int2).
 */
struct iis2dlpc_device_config {
	const char *bus_name;
	iis2dlpc_mode_t pm;
#ifdef CONFIG_IIS2DLPC_TRIGGER
	const char *int_gpio_port;
	uint8_t int_gpio_pin;
	uint8_t int_gpio_flags;
	uint8_t int_pin;
#ifdef CONFIG_IIS2DLPC_PULSE
	uint8_t pulse_trigger;
	uint8_t pulse_ths[3];
	uint8_t pulse_shock;
	uint8_t pulse_ltncy;
	uint8_t pulse_quiet;
#endif /* CONFIG_IIS2DLPC_PULSE */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
};

/* sensor data */
struct iis2dlpc_data {
	struct device *bus;
	int16_t acc[3];

	 /* save sensitivity */
	uint16_t gain;

	stmdev_ctx_t *ctx;
#ifdef CONFIG_IIS2DLPC_TRIGGER
	struct device *gpio;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#ifdef CONFIG_IIS2DLPC_PULSE
	sensor_trigger_handler_t tap_handler;
	sensor_trigger_handler_t double_tap_handler;
#endif /* CONFIG_IIS2DLPC_PULSE */
#if defined(CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2DLPC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif /* CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif
};

int iis2dlpc_i2c_init(struct device *dev);
int iis2dlpc_spi_init(struct device *dev);

#ifdef CONFIG_IIS2DLPC_TRIGGER
int iis2dlpc_init_interrupt(struct device *dev);
int iis2dlpc_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS2DLPC_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_ */
