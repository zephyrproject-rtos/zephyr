/*
 * Copyright (c) 2021, Silvano Cortesi
 * Copyright (c) 2021, ETH Zürich
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MAX30208_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX30208_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/ring_buffer.h>
#include <drivers/sensor.h>
#include <drivers/sensor/max30208.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

/* Macros for struct to byte conversion */
#define MAX30208_STC_TO_P(STRUCT) (uint8_t *)&(STRUCT)
#define MAX30208_STC_TO_B(STRUCT) *MAX30208_STC_TO_P(STRUCT)

/* Interrupt and Status */
#define MAX30208_REG_INT_STS 0x00
#define MAX30208_REG_INT_EN 0x01

/* FIFO */
#define MAX30208_REG_FIFO_WR 0x04
#define MAX30208_REG_FIFO_RD 0x05
#define MAX30208_REG_FIFO_OVF 0x06
#define MAX30208_REG_FIFO_DATA_CTR 0x07
#define MAX30208_REG_FIFO_DATA 0x08
#define MAX30208_REG_FIFO_CFG1 0x09
#define MAX30208_REG_FIFO_CFG2 0x0A

/* System */
#define MAX30208_REG_SYS_CTRL 0x0C

/* Temperature */
#define MAX30208_REG_ALRM_H_MSB 0x10
#define MAX30208_REG_ALRM_H_LSB 0x11
#define MAX30208_REG_ALRM_L_MSB 0x12
#define MAX30208_REG_ALRM_L_LSB 0x13
#define MAX30208_REG_TEMP_SETUP 0x14

/* GPIO */
#define MAX30208_REG_GPIO_SETUP 0x20
#define MAX30208_REG_GPIO_CTRL 0x21

/* Identifiers */
#define MAX30208_REG_PART_ID1 0x31
#define MAX30208_REG_PART_ID2 0x32
#define MAX30208_REG_PART_ID3 0x33
#define MAX30208_REG_PART_ID4 0x34
#define MAX30208_REG_PART_ID5 0x35
#define MAX30208_REG_PART_ID6 0x36
#define MAX30208_REG_PART_ID 0xFF

#define MAX30208_PART_ID 0x30

/* defines for conversion */
#define MAX30208_ONE_DEGREE 0xC8
#define MAX30208_LSB_e6 5000

/* enums and definitions */
#define MAX30208_RESET_MASK 0x01
#define MAX30208_CONVERT_T_MASK 0xFF
#define MAX30208_INT_A_FULL_MASK BIT(7)
#define MAX30208_INT_TEMP_LO_MASK BIT(2)
#define MAX30208_INT_TEMP_HI_MASK BIT(1)
#define MAX30208_INT_TEMP_RDY_MASK BIT(0)
#define MAX30208_GPIO_MODE_MASK(gpioX, mode) (mode << (gpioX * 6U))

#define MAX30208_POLL_TRIES 10
#define MAX30208_POLL_TIME 10
#define MAX30208_RESET_TIME 10
#define MAX30208_TMP_MEAS_TIME 10
#define MAX30208_BYTES_PER_VAL 2
#define MAX30208_FIFO_SIZE 32
#define MAX30208_ARRAY_SIZE CONFIG_MAX30208_RINGBUFFER_SIZE * MAX30208_BYTES_PER_VAL

enum max30208_gpio_mode {
	MAX30208_GPIO_INPUT = 0,
	MAX30208_GPIO_OUTPUT = 1,
	MAX30208_GPIO_INPUT_PULLDOWN = 2,
	MAX30208_GPIO_CONV_TEMP = 3,
	MAX30208_GPIO_INTB = 3,
};

/* register structs */
#ifdef CONFIG_BIG_ENDIAN
struct max30208_status {
	uint8_t a_full : 1;
	uint8_t unused : 4;
	uint8_t temp_low : 1;
	uint8_t temp_hi : 1;
	uint8_t temp_rdy : 1;
};

struct max30208_int_en {
	uint8_t a_full_en : 1;
	uint8_t unused : 4;
	uint8_t temp_low_en : 1;
	uint8_t temp_hi_en : 1;
	uint8_t temp_rdy_en : 1;
};

struct max30208_fifo_config2 {
	uint8_t flush_fifo : 1;
	uint8_t fifo_stat_clr : 1;
	uint8_t a_full_type : 1;
	uint8_t fifo_ro : 1;
};

struct max30208_fifo_config {
	uint8_t fifo_a_full;
	struct max30208_fifo_config2 config2;
};

struct max30208_gpio_setup {
	enum max30208_gpio_mode gpio1_mode : 2;
	uint8_t unused : 4;
	enum max30208_gpio_mode gpio0_mode : 2;
};

struct max30208_gpio_ctrl {
	uint8_t gpio1_ll : 1;
	uint8_t gpio0_ll : 1;
};
#else
struct max30208_status {
	uint8_t temp_rdy : 1;
	uint8_t temp_hi : 1;
	uint8_t temp_low : 1;
	uint8_t unused : 4;
	uint8_t a_full : 1;
};

struct max30208_int_en {
	uint8_t temp_rdy_en : 1;
	uint8_t temp_hi_en : 1;
	uint8_t temp_low_en : 1;
	uint8_t unused : 4;
	uint8_t a_full_en : 1;
};

struct max30208_fifo_config2 {
	uint8_t fifo_ro : 1;
	uint8_t a_full_type : 1;
	uint8_t fifo_stat_clr : 1;
	uint8_t flush_fifo : 1;
};

struct max30208_fifo_config {
	uint8_t fifo_a_full;
	struct max30208_fifo_config2 config2;
};

struct max30208_gpio_setup {
	enum max30208_gpio_mode gpio0_mode : 2;
	uint8_t unused : 4;
	enum max30208_gpio_mode gpio1_mode : 2;
};

struct max30208_gpio_ctrl {
	uint8_t gpio0_ll : 1;
	uint8_t gpio1_ll : 1;
};
#endif /* CONFIG_BIG_ENDIAN */

/* device structs */
struct max30208_config {
	struct i2c_dt_spec bus;
	struct max30208_fifo_config fifo;
	struct max30208_gpio_setup gpio_setup;
#ifdef CONFIG_MAX30208_TRIGGER
	struct gpio_dt_spec gpio_int;
#endif
};

struct max30208_data {
	uint8_t _ring_buffer_data_raw_buffer[MAX30208_ARRAY_SIZE];
	struct ring_buf raw_buffer;
#ifdef CONFIG_MAX30208_TRIGGER
	uint8_t status;

	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler[3];
	struct sensor_trigger th_trigger[3];

	const struct device *dev;

#if defined(CONFIG_MAX30208_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MAX30208_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_MAX30208_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_MAX30208_TRIGGER */
};

#ifdef CONFIG_MAX30208_TRIGGER
int max30208_attr_set(const struct device *dev, enum sensor_channel chan,
		      enum sensor_attribute attr, const struct sensor_value *val);

int max30208_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int max30208_init_interrupt(const struct device *dev);
#endif /* CONFIG_MAX30208_TRIGGER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_MAX30208_H_ */
