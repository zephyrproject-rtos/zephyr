/* sensor_bmc150_magn.h - header file for BMC150 magnetometer sensor driver */

/*
* Copyright (c) 2016 Intel Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ZEPHYR_DRIVERS_SENSOR_BMC150_MAGN_BMC150_MAGN_H_
#define ZEPHYR_DRIVERS_SENSOR_BMC150_MAGN_BMC150_MAGN_H_

#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#define BMC150_MAGN_REG_CHIP_ID		0x40
#define BMC150_MAGN_CHIP_ID_VAL		0x32

#define BMC150_MAGN_REG_X_L		0x42
#define BMC150_MAGN_REG_X_M		0x43
#define BMC150_MAGN_REG_Y_L		0x44
#define BMC150_MAGN_REG_Y_M		0x45
#define BMC150_MAGN_SHIFT_XY_L		3
#define BMC150_MAGN_REG_Z_L		0x46
#define BMC150_MAGN_REG_Z_M		0x47
#define BMC150_MAGN_SHIFT_Z_L		1
#define BMC150_MAGN_REG_RHALL_L		0x48
#define BMC150_MAGN_REG_RHALL_M		0x49
#define BMC150_MAGN_SHIFT_RHALL_L	2

#define BMC150_MAGN_REG_INT_STATUS	0x4A

#define BMC150_MAGN_REG_POWER		0x4B
#define BMC150_MAGN_MASK_POWER_CTL	BIT(0)

#define BMC150_MAGN_REG_OPMODE_ODR	0x4C
#define BMC150_MAGN_MASK_OPMODE		(BIT(2) | BIT(1))
#define BMC150_MAGN_SHIFT_OPMODE	1
#define BMC150_MAGN_MODE_NORMAL		0x00
#define BMC150_MAGN_MODE_FORCED		0x01
#define BMC150_MAGN_MODE_SLEEP		0x03
#define BMC150_MAGN_MASK_ODR		(BIT(5) | BIT(4) | BIT(3))
#define BMC150_MAGN_SHIFT_ODR		3

#define BMC150_MAGN_REG_LOW_THRESH	0x4F
#define BMC150_MAGN_REG_HIGH_THRESH	0x50
#define BMC150_MAGN_REG_REP_XY		0x51
#define BMC150_MAGN_REG_REP_Z		0x52
#define BMC150_MAGN_REG_REP_DATAMASK	0xFF

#define BMC150_MAGN_REG_TRIM_START	0x5D
#define BMC150_MAGN_REG_TRIM_END	0x71

#define BMC150_MAGN_XY_OVERFLOW_VAL	-4096
#define BMC150_MAGN_Z_OVERFLOW_VAL	-16384

#define BMC150_MAGN_REGVAL_TO_REPXY(regval)	(((regval) * 2) + 1)
#define BMC150_MAGN_REGVAL_TO_REPZ(regval)	((regval) + 1)
#define BMC150_MAGN_REPXY_TO_REGVAL(rep)	(((rep) - 1) / 2)
#define BMC150_MAGN_REPZ_TO_REGVAL(rep)		((rep) - 1)

#define BMC150_MAGN_REG_INT                     0x4D

#define BMC150_MAGN_REG_INT_DRDY                0x4E
#define BMC150_MAGN_MASK_DRDY_EN                BIT(7)
#define BMC150_MAGN_SHIFT_DRDY_EN               7
#define BMC150_MAGN_MASK_DRDY_INT3              BIT(6)
#define BMC150_MAGN_MASK_DRDY_Z_EN              BIT(5)
#define BMC150_MAGN_MASK_DRDY_Y_EN              BIT(4)
#define BMC150_MAGN_MASK_DRDY_X_EN              BIT(3)
#define BMC150_MAGN_MASK_DRDY_DR_POLARITY       BIT(2)
#define BMC150_MAGN_SHIFT_DRDY_DR_POLARITY      2
#define BMC150_MAGN_MASK_DRDY_LATCHING          BIT(1)
#define BMC150_MAGN_MASK_DRDY_INT3_POLARITY     BIT(0)

#if defined(CONFIG_BMC150_MAGN_SAMPLING_REP_XY) || \
	defined(CONFIG_BMC150_MAGN_SAMPLING_REP_Z)
	#define BMC150_MAGN_SET_ATTR_REP
#endif

#if defined(CONFIG_BMC150_MAGN_SAMPLING_RATE_RUNTIME) || \
	defined(BMC150_MAGN_SET_ATTR_REP)
	#define BMC150_MAGN_SET_ATTR
#endif

struct bmc150_magn_config {
	struct i2c_dt_spec i2c;
#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	struct gpio_dt_spec int_gpio;
#endif
};

struct bmc150_magn_trim_regs {
	int8_t x1;
	int8_t y1;
	uint16_t reserved1;
	uint8_t reserved2;
	int16_t z4;
	int8_t x2;
	int8_t y2;
	uint16_t reserved3;
	int16_t z2;
	uint16_t z1;
	uint16_t xyz1;
	int16_t z3;
	int8_t xy2;
	uint8_t xy1;
} __packed;

struct bmc150_magn_data {
	struct k_sem sem;

#if defined(CONFIG_BMC150_MAGN_TRIGGER)
	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_BMC150_MAGN_TRIGGER_THREAD_STACK);
	struct k_thread thread;
#endif

#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	const struct device *gpio_drdy;
	const struct device *dev;
	struct gpio_callback gpio_cb;
	const struct sensor_trigger *trigger_drdy;
	sensor_trigger_handler_t handler_drdy;
#endif

	struct bmc150_magn_trim_regs tregs;
	int rep_xy, rep_z, odr, max_odr;
	int sample_x, sample_y, sample_z;
};

enum bmc150_magn_power_modes {
	BMC150_MAGN_POWER_MODE_SUSPEND,
	BMC150_MAGN_POWER_MODE_SLEEP,
	BMC150_MAGN_POWER_MODE_NORMAL,
};

enum bmc150_magn_presets {
	LOW_POWER_PRESET,
	REGULAR_PRESET,
	ENHANCED_REGULAR_PRESET,
	HIGH_ACCURACY_PRESET
};

#if defined(CONFIG_BMC150_MAGN_PRESET_LOW_POWER)
	#define BMC150_MAGN_DEFAULT_PRESET LOW_POWER_PRESET
#elif defined(CONFIG_BMC150_MAGN_PRESET_REGULAR)
	#define BMC150_MAGN_DEFAULT_PRESET REGULAR_PRESET
#elif defined(CONFIG_BMC150_MAGN_PRESET_ENHANCED_REGULAR)
	#define BMC150_MAGN_DEFAULT_PRESET ENHANCED_REGULAR_PRESET
#elif defined(CONFIG_BMC150_MAGN_PRESET_HIGH_ACCURACY)
	#define BMC150_MAGN_DEFAULT_PRESET HIGH_ACCURACY_PRESET
#endif

enum bmc150_magn_axis {
	BMC150_MAGN_AXIS_X,
	BMC150_MAGN_AXIS_Y,
	BMC150_MAGN_AXIS_Z,
	BMC150_MAGN_RHALL,
	BMC150_MAGN_AXIS_XYZ_MAX = BMC150_MAGN_RHALL,
	BMC150_MAGN_AXIS_XYZR_MAX,
};

#if defined(CONFIG_BMC150_MAGN_TRIGGER)
int bmc150_magn_trigger_set(const struct device *dev,
			    const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler);

int bmc150_magn_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMC150_MAGN_BMC150_MAGN_H_ */
