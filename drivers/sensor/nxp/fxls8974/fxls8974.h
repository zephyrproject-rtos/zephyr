/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define FXLS8974_BUS_I2C				(1<<0)
#define FXLS8974_BUS_SPI				(1<<1)
#define FXLS8974_REG_OUTXLSB				0x04
#define FXLS8974_REG_OUTTEMP				0x01
#define FXLS8974_REG_WHOAMI				0x13
#define FXLS8974_REG_CTRLREG1				0x15
#define FXLS8974_REG_CTRLREG2				0x16
#define FXLS8974_REG_CTRLREG3				0x17
#define FXLS8974_REG_CTRLREG4				0x18
#define FXLS8974_REG_CTRLREG5				0x19

#define WHOAMI_ID_FXLS8964				0x84
#define WHOAMI_ID_FXLS8974				0x86

#define FXLS8974_CTRLREG1_ACTIVE_MASK			0x01
#define FXLS8974_CTRLREG1_RST_MASK			0x80
#define FXLS8974_CTRLREG1_FSR_MASK			0x06
#define FXLS8974_CTRLREG1_FSR_2G			0x00
#define FXLS8974_CTRLREG1_FSR_4G			0x02
#define FXLS8974_CTRLREG1_FSR_8G			0x04
#define FXLS8974_CTRLREG1_FSR_16G			0x06

#define FXLS8974_CTRLREG2_WAKE_PM_MASK			0xC0
#define FXLS8974_CTRLREG2_SLEEP_PM_MASK			0x30

#define FXLS8974_CTRLREG3_WAKE_ODR_MASK			0xF0
#define FXLS8974_CTRLREG3_SLEEP_ODR_MASK		0x0F

#define FXLS8974_CTRLREG3_ODR_RATE_3200			0x00
#define FXLS8974_CTRLREG3_ODR_RATE_1600			0x01
#define FXLS8974_CTRLREG3_ODR_RATE_800			0x02
#define FXLS8974_CTRLREG3_ODR_RATE_400			0x03
#define FXLS8974_CTRLREG3_ODR_RATE_200			0x04
#define FXLS8974_CTRLREG3_ODR_RATE_100			0x05
#define FXLS8974_CTRLREG3_ODR_RATE_50			0x06
#define FXLS8974_CTRLREG3_ODR_RATE_25			0x07
#define FXLS8974_CTRLREG3_ODR_RATE_12_5			0x08
#define FXLS8974_CTRLREG3_ODR_RATE_6_25			0x09
#define FXLS8974_CTRLREG3_ODR_RATE_3_125		0x0A
#define FXLS8974_CTRLREG3_ODR_RATE_1_563		0x0B
#define FXLS8974_CTRLREG3_ODR_RATE_0_781		0x0C

#define FXLS8974_CTRLREG4_INT_POL_HIGH			0x01

#define FXLS8974_INTREG_EN				0x20
#define FXLS8974_INT_PIN_SEL_REG			0x21

#define FXLS8974_DATA_ACCEL_X_OFFSET			0
#define FXLS8974_DATA_ACCEL_Y_OFFSET			FXLS8974_BYTES_PER_CHANNEL_NORMAL
#define FXLS8974_DATA_ACCEL_Z_OFFSET			2*FXLS8974_BYTES_PER_CHANNEL_NORMAL
#define FXLS8974_DATA_TEMP_OFFSET			3*FXLS8974_BYTES_PER_CHANNEL_NORMAL
#define FXLS8974_ZERO_TEMP				25

#define FXLS8974_MAX_NUM_CHANNELS			4
#define FXLS8974_MAX_ACCEL_CHANNELS			3
#define FXLS8974_MAX_TEMP_CHANNELS			1

#define FXLS8974_BYTES_PER_CHANNEL_NORMAL		2
#define FXLS8974_BYTES_PER_CHANNEL_FAST			1

#define FXLS8974_MAX_ACCEL_BYTES			(FXLS8974_MAX_ACCEL_CHANNELS*2)
#define FXLS8974_MAX_NUM_BYTES		(FXLS8974_MAX_ACCEL_BYTES + FXLS8974_MAX_TEMP_CHANNELS)

#define FXLS8974_DRDY_MASK				0x80

enum fxls8974_active {
	FXLS8974_ACTIVE_OFF		= 0,
	FXLS8974_ACTIVE_ON,
};

enum fxls8974_wake {
	FXLS8974_WAKE			= 0,
	FXLS8974_SLEEP,
};

enum fxls8974_channel {
	FXLS8974_CHANNEL_ACCEL_X	= 0,
	FXLS8974_CHANNEL_ACCEL_Y,
	FXLS8974_CHANNEL_ACCEL_Z,
	FXLS8974_CHANNEL_TEMP,
};

struct fxls8974_io_ops {
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

union fxls8974_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
};

struct fxls8974_config {
	const union fxls8974_bus_cfg bus_cfg;
	const struct fxls8974_io_ops *ops;
	struct gpio_dt_spec reset_gpio;
	uint8_t range;
	uint8_t inst_on_bus;
#ifdef CONFIG_FXLS8974_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif

};

struct fxls8974_data {
	struct k_sem sem;
	int16_t raw[FXLS8974_MAX_NUM_CHANNELS];
	uint8_t whoami;
#ifdef CONFIG_FXLS8974_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trig;
#endif
#ifdef CONFIG_FXLS8974_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_FXLS8974_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

int fxls8974_get_active(const struct device *dev, uint8_t *active);
int fxls8974_set_active(const struct device *dev, uint8_t active);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
int fxls8974_byte_write_spi(const struct device *dev,
		uint8_t reg,
		uint8_t byte);

int fxls8974_byte_read_spi(const struct device *dev,
		uint8_t reg,
		uint8_t *byte);

int fxls8974_reg_field_update_spi(const struct device *dev,
		uint8_t reg,
		uint8_t mask,
		uint8_t val);

int fxls8974_read_spi(const struct device *dev,
		uint8_t reg,
		void *data,
		size_t length);
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
int fxls8974_byte_write_i2c(const struct device *dev,
		uint8_t reg,
		uint8_t byte);

int fxls8974_byte_read_i2c(const struct device *dev,
		uint8_t reg,
		uint8_t *byte);

int fxls8974_reg_field_update_i2c(const struct device *dev,
		uint8_t reg,
		uint8_t mask,
		uint8_t val);

int fxls8974_read_i2c(const struct device *dev,
		uint8_t reg,
		void *data,
		size_t length);
#endif
#if CONFIG_FXLS8974_TRIGGER
int fxls8974_trigger_init(const struct device *dev);
int fxls8974_trigger_set(const struct device *dev,
		const struct sensor_trigger *trig,
		sensor_trigger_handler_t handler);
#endif
