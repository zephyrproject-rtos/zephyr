/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define FXAS21002_BUS_I2C		(1<<0)
#define FXAS21002_BUS_SPI		(1<<1)
#define FXAS21002_REG_STATUS		0x00
#define FXAS21002_REG_OUTXMSB		0x01
#define FXAS21002_REG_INT_SOURCE	0x0b
#define FXAS21002_REG_WHOAMI		0x0c
#define FXAS21002_REG_CTRLREG0		0x0d
#define FXAS21002_REG_CTRLREG1		0x13
#define FXAS21002_REG_CTRLREG2		0x14
#define FXAS21002_REG_CTRLREG3		0x15

#define FXAS21002_INT_SOURCE_DRDY_MASK	(1 << 0)

#define FXAS21002_CTRLREG0_FS_MASK	(3 << 0)

#define FXAS21002_CTRLREG1_DR_SHIFT	2

#define FXAS21002_CTRLREG1_POWER_MASK	(3 << 0)
#define FXAS21002_CTRLREG1_DR_MASK	(7 << FXAS21002_CTRLREG1_DR_SHIFT)
#define FXAS21002_CTRLREG1_RST_MASK	(1 << 6)

#define FXAS21002_CTRLREG2_CFG_EN_MASK		(1 << 2)
#define FXAS21002_CTRLREG2_CFG_DRDY_MASK	(1 << 3)

#define FXAS21002_MAX_NUM_CHANNELS	3

#define FXAS21002_BYTES_PER_CHANNEL	2

#define FXAS21002_MAX_NUM_BYTES		(FXAS21002_BYTES_PER_CHANNEL * \
					 FXAS21002_MAX_NUM_CHANNELS)

enum fxas21002_power {
	FXAS21002_POWER_STANDBY		= 0,
	FXAS21002_POWER_READY           = 1,
	FXAS21002_POWER_ACTIVE          = 3,
};

enum fxas21002_range {
	FXAS21002_RANGE_2000DPS		= 0,
	FXAS21002_RANGE_1000DPS,
	FXAS21002_RANGE_500DPS,
	FXAS21002_RANGE_250DPS,
};

enum fxas21002_channel {
	FXAS21002_CHANNEL_GYRO_X	= 0,
	FXAS21002_CHANNEL_GYRO_Y,
	FXAS21002_CHANNEL_GYRO_Z,
};

struct fxas21002_io_ops {
	int (*read)(const struct device *dev,
		    uint8_t reg,
		    void *data,
		    size_t length);
	int (*byte_read)(const struct device *dev,
			 uint8_t reg,
			 uint8_t *byte);
	int (*byte_write)(const struct device *dev,
			  uint8_t reg,
			  uint8_t byte);
	int (*reg_field_update)(const struct device *dev,
				uint8_t reg,
				uint8_t mask,
				uint8_t val);
};

union fxas21002_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
};

struct fxas21002_config {
	const union fxas21002_bus_cfg bus_cfg;
	const struct fxas21002_io_ops *ops;
#ifdef CONFIG_FXAS21002_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
	struct gpio_dt_spec reset_gpio;
	uint8_t whoami;
	enum fxas21002_range range;
	uint8_t dr;
	uint8_t inst_on_bus;
};

struct fxas21002_data {
	struct k_sem sem;
#ifdef CONFIG_FXAS21002_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#endif
#ifdef CONFIG_FXAS21002_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_FXAS21002_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_FXAS21002_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
	int16_t raw[FXAS21002_MAX_NUM_CHANNELS];
};

int fxas21002_get_power(const struct device *dev, enum fxas21002_power *power);
int fxas21002_set_power(const struct device *dev, enum fxas21002_power power);

uint32_t fxas21002_get_transition_time(enum fxas21002_power start,
				       enum fxas21002_power end,
				       uint8_t dr);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
int fxas21002_byte_write_spi(const struct device *dev,
			     uint8_t reg,
			     uint8_t byte);

int fxas21002_byte_read_spi(const struct device *dev,
			    uint8_t reg,
			    uint8_t *byte);

int fxas21002_reg_field_update_spi(const struct device *dev,
				   uint8_t reg,
				   uint8_t mask,
				   uint8_t val);

int fxas21002_read_spi(const struct device *dev,
		       uint8_t reg,
		       void *data,
		       size_t length);
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
int fxas21002_byte_write_i2c(const struct device *dev,
			     uint8_t reg,
			     uint8_t byte);

int fxas21002_byte_read_i2c(const struct device *dev,
			    uint8_t reg,
			    uint8_t *byte);

int fxas21002_reg_field_update_i2c(const struct device *dev,
				   uint8_t reg,
				   uint8_t mask,
				   uint8_t val);

int fxas21002_read_i2c(const struct device *dev,
		       uint8_t reg,
		       void *data,
		       size_t length);
#endif
#if CONFIG_FXAS21002_TRIGGER
int fxas21002_trigger_init(const struct device *dev);
int fxas21002_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif
