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

#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include "iis2dlpc_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

union axis3bit16_t {
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

/* Return ODR reg value based on data rate set */
#define IIS2DLPC_ODR_TO_REG(_odr) \
	((_odr <= 1) ? IIS2DLPC_XL_ODR_1Hz6_LP_ONLY : \
	 (_odr <= 12) ? IIS2DLPC_XL_ODR_12Hz5 : \
	 ((31 - __builtin_clz(_odr / 25))) + 3)

/* FS reg value from Full Scale */
#define IIS2DLPC_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

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

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
struct iis2dlpc_spi_cfg {
	struct spi_config spi_conf;
	const char *cs_gpios_label;
};
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

union iis2dlpc_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	uint16_t i2c_slv_addr;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	const struct iis2dlpc_spi_cfg *spi_cfg;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

/**
 * struct iis2dlpc_dev_config - iis2dlpc hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @irq_dev_name: Pointer to GPIO PORT identifier.
 * @irq_pin: GPIO pin number connecter to sensor int pin.
 * @drdy_int: Sensor drdy int (int1/int2).
 */
struct iis2dlpc_dev_config {
	const char *bus_name;
	int (*bus_init)(const struct device *dev);
	const union iis2dlpc_bus_cfg bus_cfg;
	iis2dlpc_mode_t pm;
	uint8_t range;
#ifdef CONFIG_IIS2DLPC_TRIGGER
	const char *irq_dev_name;
	gpio_pin_t irq_pin;
	gpio_flags_t irq_flags;
	uint8_t drdy_int;
#ifdef CONFIG_IIS2DLPC_TAP
	uint8_t tap_mode;
	uint8_t tap_threshold[3];
	uint8_t tap_shock;
	uint8_t tap_latency;
	uint8_t tap_quiet;
#endif /* CONFIG_IIS2DLPC_TAP */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
};

/* sensor data */
struct iis2dlpc_data {
	const struct device *dev;
	const struct device *bus;
	int16_t acc[3];

	 /* save sensitivity */
	uint16_t gain;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_IIS2DLPC_TRIGGER
	const struct device *gpio;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#ifdef CONFIG_IIS2DLPC_TAP
	sensor_trigger_handler_t tap_handler;
	sensor_trigger_handler_t double_tap_handler;
#endif /* CONFIG_IIS2DLPC_TAP */
#if defined(CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2DLPC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_cs_control cs_ctrl;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

int iis2dlpc_i2c_init(const struct device *dev);
int iis2dlpc_spi_init(const struct device *dev);

#ifdef CONFIG_IIS2DLPC_TRIGGER
int iis2dlpc_init_interrupt(const struct device *dev);
int iis2dlpc_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS2DLPC_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_ */
