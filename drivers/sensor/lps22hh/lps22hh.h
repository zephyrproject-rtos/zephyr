/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22HH_LPS22HH_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22HH_LPS22HH_H_

#include <stdint.h>
#include <i2c.h>
#include <spi.h>
#include <gpio.h>
#include <sensor.h>
#include <zephyr/types.h>
#include <misc/util.h>

#define LPS22HH_EN_BIT				0x01
#define LPS22HH_DIS_BIT				0x00

#define LPS22HH_REG_WHO_AM_I			0x0F
#define LPS22HH_VAL_WHO_AM_I			0xB3

#define LPS22HH_REG_INTERRUPT_CFG		0x0B
#define LPS22HH_LIR_MASK			BIT(2)
#define LPS22HH_LIR_SHIFT			2

#define LPS22HH_REG_CTRL_REG1			0x10
#define LPS22HH_MASK_CTRL_REG1_ODR		(BIT(6) | BIT(5) | BIT(4))
#define LPS22HH_SHIFT_CTRL_REG1_ODR		4
#define LPS22HH_MASK_CTRL_REG1_BDU		BIT(1)
#define LPS22HH_SHIFT_CTRL_REG1_BDU		1

#define LPS22HH_REG_CTRL_REG2			0x11
#define LPS22HH_INT_H_L				BIT(6)

#define LPS22HH_REG_CTRL_REG3			0x12
#define LPS22HH_INT1_DRDY			BIT(2)

#define LPS22HH_REG_PRESS_OUT_XL		0x28
#define LPS22HH_REG_PRESS_OUT_L			0x29
#define LPS22HH_REG_PRESS_OUT_H			0x2A

#define LPS22HH_REG_TEMP_OUT_L			0x2B
#define LPS22HH_REG_TEMP_OUT_H			0x2C

#define LPS22HH_REG_LPFP_RES			0x33

struct lps22hh_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
};

/* sensor data forward declaration (member definition is below) */
struct lps22hh_data;

/* transmission function interface */
struct lps22hh_tf {
	int (*read_data)(struct lps22hh_data *data, u8_t reg_addr,
			 u8_t *value, u8_t len);
	int (*write_data)(struct lps22hh_data *data, u8_t reg_addr,
			 u8_t *value, u8_t len);
	int (*read_reg)(struct lps22hh_data *data, u8_t reg_addr,
			u8_t *value);
	int (*write_reg)(struct lps22hh_data *data, u8_t reg_addr,
			 u8_t value);
	int (*update_reg)(struct lps22hh_data *data, u8_t reg_addr,
			  u8_t mask, u8_t value);
};

struct lps22hh_data {
	struct device *bus;
	s32_t sample_press;
	s16_t sample_temp;

	const struct lps22hh_tf *hw_tf;
#ifdef CONFIG_LPS22HH_TRIGGER
	struct device *gpio;
	u32_t pin;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LPS22HH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_LPS22HH_TRIGGER */
#if defined(DT_ST_LPS22HH_0_CS_GPIO_CONTROLLER)
	struct spi_cs_control cs_ctrl;
#endif
};

int lps22hh_i2c_init(struct device *dev);
int lps22hh_spi_init(struct device *dev);

#ifdef CONFIG_LPS22HH_TRIGGER
int lps22hh_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lps22hh_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS22HH_LPS22HH_H_ */
