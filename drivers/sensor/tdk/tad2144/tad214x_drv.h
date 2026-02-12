/*
 * Copyright (c) 2023 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TAD2144_H_
#define ZEPHYR_DRIVERS_SENSOR_TAD2144_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include "tad214x.h"
#include "tad214xSerif.h"

union tad214x_bus {
#if CONFIG_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*tad214x_bus_check_fn)(const union tad214x_bus *bus);
typedef int (*tad214x_reg_read_fn)(const union tad214x_bus *bus, uint8_t reg, uint16_t *buf,
				    uint32_t size);
typedef int (*tad214x_reg_write_fn)(const union tad214x_bus *bus, uint8_t reg, uint16_t *buf,
				     uint32_t size);

struct tad214x_bus_io {
	tad214x_bus_check_fn check;
	tad214x_reg_read_fn read;
	tad214x_reg_write_fn write;
};

extern const struct tad214x_bus_io tad214x_bus_io_spi;
extern const struct tad214x_bus_io tad214x_bus_io_i2c;

struct tad214x_data {
    struct tad214x_serif serif;
    struct tad214x tad214x_device;
	int32_t angle;
	int32_t temperature;
    uint32_t encoder_position;
#ifdef CONFIG_TAD2144_TRIGGER
    const struct device *dev;
    struct gpio_callback gpio_cb;
    struct k_mutex mutex;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
#endif
#ifdef CONFIG_TAD2144_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TAD2144_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_TAD2144_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct tad214x_config {
	union tad214x_bus bus;
	const struct tad214x_bus_io *bus_io;
	struct gpio_dt_spec gpio_int;
    struct gpio_dt_spec sa1_gpio;
    struct gpio_dt_spec sa2_gpio;
    tad214x_serif_type_t if_mode;
    struct gpio_dt_spec cs_gpio;
    struct gpio_dt_spec miso_gpio;
    struct gpio_dt_spec mosi_gpio;
    struct gpio_dt_spec sck_gpio;
};

int tad214x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int tad214x_trigger_init(const struct device *dev);

void memswap16( void* ptr1, unsigned int bytes );
unsigned char crc8_sae_j1850(const unsigned char *data, unsigned int length);


#endif
