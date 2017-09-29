/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <i2c.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "FXAS21002"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

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

struct fxas21002_config {
	char *i2c_name;
#ifdef CONFIG_FXAS21002_TRIGGER
	char *gpio_name;
	u8_t gpio_pin;
#endif
	u8_t i2c_address;
	u8_t whoami;
	enum fxas21002_range range;
	u8_t dr;
};

struct fxas21002_data {
	struct device *i2c;
	struct k_sem sem;
#ifdef CONFIG_FXAS21002_TRIGGER
	struct device *gpio;
	u8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#endif
#ifdef CONFIG_FXAS21002_TRIGGER_OWN_THREAD
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_FXAS21002_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_FXAS21002_TRIGGER_GLOBAL_THREAD
	struct k_work work;
	struct device *dev;
#endif
	s16_t raw[FXAS21002_MAX_NUM_CHANNELS];
};

int fxas21002_get_power(struct device *dev, enum fxas21002_power *power);
int fxas21002_set_power(struct device *dev, enum fxas21002_power power);

u32_t fxas21002_get_transition_time(enum fxas21002_power start,
				       enum fxas21002_power end,
				       u8_t dr);

#if CONFIG_FXAS21002_TRIGGER
int fxas21002_trigger_init(struct device *dev);
int fxas21002_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif
