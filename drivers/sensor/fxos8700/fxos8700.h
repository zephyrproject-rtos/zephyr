/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

#define FXOS8700_REG_STATUS			0x00
#define FXOS8700_REG_OUTXMSB			0x01
#define FXOS8700_REG_INT_SOURCE			0x0c
#define FXOS8700_REG_WHOAMI			0x0d
#define FXOS8700_REG_XYZ_DATA_CFG		0x0e
#define FXOS8700_REG_FF_MT_CFG			0x15
#define FXOS8700_REG_FF_MT_SRC			0x16
#define FXOS8700_REG_FF_MT_THS			0x17
#define FXOS8700_REG_FF_MT_COUNT		0x18
#define FXOS8700_REG_PULSE_CFG			0x21
#define FXOS8700_REG_PULSE_SRC			0x22
#define FXOS8700_REG_PULSE_THSX			0x23
#define FXOS8700_REG_PULSE_THSY			0x24
#define FXOS8700_REG_PULSE_THSZ			0x25
#define FXOS8700_REG_PULSE_TMLT			0x26
#define FXOS8700_REG_PULSE_LTCY			0x27
#define FXOS8700_REG_PULSE_WIND			0x28
#define FXOS8700_REG_CTRLREG1			0x2a
#define FXOS8700_REG_CTRLREG2			0x2b
#define FXOS8700_REG_CTRLREG3			0x2c
#define FXOS8700_REG_CTRLREG4			0x2d
#define FXOS8700_REG_CTRLREG5			0x2e
#define FXOS8700_REG_M_OUTXMSB			0x33
#define FXOS8700_REG_TEMP			0x51
#define FXOS8700_REG_M_CTRLREG1			0x5b
#define FXOS8700_REG_M_CTRLREG2			0x5c
#define FXOS8700_REG_M_INT_SRC			0x5e
#define FXOS8700_REG_M_VECM_CFG			0x69
#define FXOS8700_REG_M_VECM_THS_MSB		0x6a
#define FXOS8700_REG_M_VECM_THS_LSB		0x6b

/* Devices that are compatible with this driver: */
#define WHOAMI_ID_MMA8451			0x1A
#define WHOAMI_ID_MMA8652			0x4A
#define WHOAMI_ID_MMA8653			0x5A
#define WHOAMI_ID_FXOS8700			0xC7

#define FXOS8700_DRDY_MASK			(1 << 0)
#define FXOS8700_MAG_VECM_INT1_MASK		(1 << 0)
#define FXOS8700_VECM_MASK			(1 << 1)
#define FXOS8700_MOTION_MASK			(1 << 2)
#define FXOS8700_PULSE_MASK			(1 << 3)

#define FXOS8700_XYZ_DATA_CFG_FS_MASK		0x03

#define FXOS8700_PULSE_SRC_DPE			(1 << 3)

#define FXOS8700_CTRLREG1_ACTIVE_MASK		0x01
#define FXOS8700_CTRLREG1_DR_MASK		(7 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_800		0
#define FXOS8700_CTRLREG1_DR_RATE_400		(1 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_200		(2 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_100		(3 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_50		(4 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_12_5		(5 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_6_25		(6 << 3)
#define FXOS8700_CTRLREG1_DR_RATE_1_56		(7 << 3)

#define FXOS8700_CTRLREG2_RST_MASK		0x40
#define FXOS8700_CTRLREG2_MODS_MASK		0x03

#define FXOS8700_FF_MT_CFG_ELE			BIT(7)
#define FXOS8700_FF_MT_CFG_OAE			BIT(6)
#define FXOS8700_FF_MT_CFG_ZEFE			BIT(5)
#define FXOS8700_FF_MT_CFG_YEFE			BIT(4)
#define FXOS8700_FF_MT_CFG_XEFE			BIT(3)
#define FXOS8700_FF_MT_THS_MASK			0x7f
#define FXOS8700_FF_MT_THS_SCALE		(SENSOR_G * 63000LL / 1000000LL)

#define FXOS8700_M_CTRLREG1_MODE_MASK		0x03

#define FXOS8700_M_CTRLREG2_AUTOINC_MASK	(1 << 5)

#define FXOS8700_NUM_ACCEL_CHANNELS		3
#define FXOS8700_NUM_MAG_CHANNELS		3
#define FXOS8700_NUM_HYBRID_CHANNELS		6
#define FXOS8700_MAX_NUM_CHANNELS		6

#define FXOS8700_BYTES_PER_CHANNEL_NORMAL	2
#define FXOS8700_BYTES_PER_CHANNEL_FAST		1

#define FXOS8700_MAX_NUM_BYTES		(FXOS8700_BYTES_PER_CHANNEL_NORMAL * \
					 FXOS8700_MAX_NUM_CHANNELS)

enum fxos8700_power {
	FXOS8700_POWER_STANDBY		= 0,
	FXOS8700_POWER_ACTIVE,
};

enum fxos8700_mode {
	FXOS8700_MODE_ACCEL		= 0,
	FXOS8700_MODE_MAGN		= 1,
	FXOS8700_MODE_HYBRID		= 3,
};

enum fxos8700_power_mode {
	FXOS8700_PM_NORMAL		= 0,
	FXOS8700_PM_LOW_NOISE_LOW_POWER,
	FXOS8700_PM_HIGH_RESOLUTION,
	FXOS8700_PM_LOW_POWER,
};

enum fxos8700_channel {
	FXOS8700_CHANNEL_ACCEL_X	= 0,
	FXOS8700_CHANNEL_ACCEL_Y,
	FXOS8700_CHANNEL_ACCEL_Z,
	FXOS8700_CHANNEL_MAGN_X,
	FXOS8700_CHANNEL_MAGN_Y,
	FXOS8700_CHANNEL_MAGN_Z,
};

/* FXOS8700 specific triggers */
enum fxos_trigger_type {
	FXOS8700_TRIG_M_VECM,
};

struct fxos8700_config {
	char *i2c_name;
#ifdef CONFIG_FXOS8700_TRIGGER
	char *gpio_name;
	uint8_t gpio_pin;
	gpio_dt_flags_t gpio_flags;
#endif
	uint8_t i2c_address;
	char *reset_name;
	uint8_t reset_pin;
	gpio_dt_flags_t reset_flags;
	enum fxos8700_mode mode;
	enum fxos8700_power_mode power_mode;
	uint8_t range;
	uint8_t start_addr;
	uint8_t start_channel;
	uint8_t num_channels;
#ifdef CONFIG_FXOS8700_PULSE
	uint8_t pulse_cfg;
	uint8_t pulse_ths[3];
	uint8_t pulse_tmlt;
	uint8_t pulse_ltcy;
	uint8_t pulse_wind;
#endif
#ifdef CONFIG_FXOS8700_MAG_VECM
	uint8_t mag_vecm_cfg;
	uint8_t mag_vecm_ths[2];
#endif
};

struct fxos8700_data {
	const struct device *i2c;
	struct k_sem sem;
#ifdef CONFIG_FXOS8700_TRIGGER
	const struct device *dev;
	const struct device *gpio;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#endif
#ifdef CONFIG_FXOS8700_PULSE
	sensor_trigger_handler_t tap_handler;
	sensor_trigger_handler_t double_tap_handler;
#endif
#ifdef CONFIG_FXOS8700_MOTION
	sensor_trigger_handler_t motion_handler;
#endif
#ifdef CONFIG_FXOS8700_MAG_VECM
	sensor_trigger_handler_t m_vecm_handler;
#endif
#ifdef CONFIG_FXOS8700_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_FXOS8700_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
	int16_t raw[FXOS8700_MAX_NUM_CHANNELS];
#ifdef CONFIG_FXOS8700_TEMP
	int8_t temp;
#endif
	uint8_t whoami;
};

int fxos8700_get_power(const struct device *dev, enum fxos8700_power *power);
int fxos8700_set_power(const struct device *dev, enum fxos8700_power power);

#if CONFIG_FXOS8700_TRIGGER
int fxos8700_trigger_init(const struct device *dev);
int fxos8700_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif
