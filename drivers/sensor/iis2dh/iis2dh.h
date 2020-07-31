/* ST Microelectronics IIS2DH 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dh.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS2DH_IIS2DH_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS2DH_IIS2DH_H_

#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include "iis2dh_reg.h"

union axis3bit16_t {
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

/*
 * Return ODR reg value based on data rate set
 */
#define IIS2DH_ODR_TO_REG_HR(_lp, _odr) \
	((_odr == 0) ? IIS2DH_POWER_DOWN : \
	((_odr < 10) ? IIS2DH_ODR_1Hz : \
	((_odr < 25) ? IIS2DH_ODR_10Hz : \
	((_lp == IIS2DH_LP_8bit) && (_odr >= 5376) ? IIS2DH_ODR_5kHz376_LP_1kHz344_NM_HP : \
	((_lp != IIS2DH_LP_8bit) && (_odr >= 1344) ? IIS2DH_ODR_5kHz376_LP_1kHz344_NM_HP : \
	((_lp == IIS2DH_LP_8bit) && (_odr >= 1600) ? IIS2DH_ODR_1kHz620_LP : \
	((_lp != IIS2DH_LP_8bit) && (_odr >= 800) ? IIS2DH_ODR_400Hz : \
	((31 - __builtin_clz(_odr / 25))) + 3)))))))

/* FS reg value from Full Scale */
#define IIS2DH_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

/**
 * struct iis2dh_device_config - iis2dh hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @int_gpio_port: Pointer to GPIO PORT identifier.
 * @int_gpio_pin: GPIO pin number connecter to sensor int pin.
 */
struct iis2dh_device_config {
	const char *bus_name;
	uint8_t pm;
#ifdef CONFIG_IIS2DH_TRIGGER
	const char *int_gpio_port;
	uint8_t int_gpio_pin;
	uint8_t int_gpio_flags;
	uint8_t int_pin;
#endif /* CONFIG_IIS2DH_TRIGGER */
};

/* sensor data */
struct iis2dh_data {
	struct device *bus;
	int16_t acc[3];
	uint32_t gain;

	stmdev_ctx_t *ctx;
#ifdef CONFIG_IIS2DH_TRIGGER
	struct device *gpio;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#if defined(CONFIG_IIS2DH_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2DH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2DH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif /* CONFIG_IIS2DH_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_IIS2DH_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif
};

int iis2dh_i2c_init(struct device *dev);
int iis2dh_spi_init(struct device *dev);

#ifdef CONFIG_IIS2DH_TRIGGER
int iis2dh_init_interrupt(struct device *dev);
int iis2dh_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS2DH_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS2DH_IIS2DH_H_ */
