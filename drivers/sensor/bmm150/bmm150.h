/* sensor_bmm150.h - header file for BMM150 Geomagnetic sensor driver */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMM150_BMM150_H_
#define ZEPHYR_DRIVERS_SENSOR_BMM150_BMM150_H_


#include <zephyr/types.h>
#include <drivers/i2c.h>
#include <stdint.h>
#include <sys/util.h>

#include <kernel.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <drivers/gpio.h>

#define BMM150_REG_CHIP_ID         0x40
#define BMM150_CHIP_ID_VAL         0x32

#define BMM150_REG_X_L             0x42
#define BMM150_REG_X_M             0x43
#define BMM150_REG_Y_L             0x44
#define BMM150_REG_Y_M             0x45
#define BMM150_SHIFT_XY_L          3
#define BMM150_REG_Z_L             0x46
#define BMM150_REG_Z_M             0x47
#define BMM150_SHIFT_Z_L           1
#define BMM150_REG_RHALL_L         0x48
#define BMM150_REG_RHALL_M         0x49
#define BMM150_SHIFT_RHALL_L       2

#define BMM150_REG_INT_STATUS      0x4A

#define BMM150_REG_POWER           0x4B
#define BMM150_MASK_POWER_CTL      BIT(0)

#define BMM150_REG_OPMODE_ODR      0x4C
#define BMM150_MASK_OPMODE         (BIT(2) | BIT(1))
#define BMM150_SHIFT_OPMODE        1
#define BMM150_MODE_NORMAL         0x00
#define BMM150_MODE_FORCED         0x01
#define BMM150_MODE_SLEEP          0x03
#define BMM150_MASK_ODR            (BIT(5) | BIT(4) | BIT(3))
#define BMM150_SHIFT_ODR           3

#define BMM150_REG_LOW_THRESH      0x4F
#define BMM150_REG_HIGH_THRESH     0x50
#define BMM150_REG_REP_XY          0x51
#define BMM150_REG_REP_Z           0x52
#define BMM150_REG_REP_DATAMASK    0xFF

#define BMM150_REG_TRIM_START      0x5D
#define BMM150_REG_TRIM_END        0x71

#define BMM150_XY_OVERFLOW_VAL     -4096
#define BMM150_Z_OVERFLOW_VAL      -16384

#define BMM150_REGVAL_TO_REPXY(regval)     (((regval) * 2) + 1)
#define BMM150_REGVAL_TO_REPZ(regval)      ((regval) + 1)
#define BMM150_REPXY_TO_REGVAL(rep)        (((rep) - 1) / 2)
#define BMM150_REPZ_TO_REGVAL(rep)         ((rep) - 1)

#define BMM150_REG_INT                     0x4D

#define BMM150_REG_INT_DRDY                0x4E
#define BMM150_MASK_DRDY_EN                BIT(7)
#define BMM150_SHIFT_DRDY_EN               7
#define BMM150_DRDY_INT3              BIT(6)
#define BMM150_MASK_DRDY_Z_EN              BIT(5)
#define BMM150_MASK_DRDY_Y_EN              BIT(4)
#define BMM150_MASK_DRDY_X_EN              BIT(3)
#define BMM150_MASK_DRDY_DR_POLARITY       BIT(2)
#define BMM150_SHIFT_DRDY_DR_POLARITY      2
#define BMM150_MASK_DRDY_LATCHING          BIT(1)
#define BMM150_MASK_DRDY_INT3_POLARITY     BIT(0)

#define BMM150_I2C_ADDR                    CONFIG_BMM150_I2C_ADDR

#if defined(CONFIG_BMM150_SAMPLING_REP_XY) || \
	defined(CONFIG_BMM150_SAMPLING_REP_Z)
	#define BMM150_SET_ATTR_REP
#endif

#if defined(CONFIG_BMM150_SAMPLING_RATE_RUNTIME) || \
	defined(BMM150_MAGN_SET_ATTR_REP)
	#define BMM150_MAGN_SET_ATTR
#endif


struct bmm150_config {
	char *i2c_master_dev_name;
	u16_t i2c_slave_addr;
};

struct bmm150_trim_regs {
	s8_t x1;
	s8_t y1;
	u16_t reserved1;
	u8_t reserved2;
	s16_t z4;
	s8_t x2;
	s8_t y2;
	u16_t reserved3;
	s16_t z2;
	u16_t z1;
	u16_t xyz1;
	s16_t z3;
	s8_t xy2;
	u8_t xy1;
} __packed;

struct bmm150_data {
	struct device *i2c;
	struct k_sem sem;
	struct bmm150_trim_regs tregs;
	int rep_xy, rep_z, odr, max_odr;
	int sample_x, sample_y, sample_z;
};

enum bmm150_power_modes {
	BMM150_POWER_MODE_SUSPEND,
	BMM150_POWER_MODE_SLEEP,
	BMM150_POWER_MODE_NORMAL
};

enum bmm150_axis {
	BMM150_AXIS_X,
	BMM150_AXIS_Y,
	BMM150_AXIS_Z,
	BMM150_RHALL,
	BMM150_AXIS_XYZ_MAX = BMM150_RHALL,
	BMM150_AXIS_XYZR_MAX
};

enum bmm150_presets {
	BMM150_LOW_POWER_PRESET,
	BMM150_REGULAR_PRESET,
	BMM150_ENHANCED_REGULAR_PRESET,
	BMM150_HIGH_ACCURACY_PRESET
};

#if defined(CONFIG_BMM150_PRESET_LOW_POWER)
	#define BMM150_DEFAULT_PRESET BMM150LOW_POWER_PRESET
#elif defined(CONFIG_BMM150_PRESET_REGULAR)
	#define BMM150_DEFAULT_PRESET BMM150_REGULAR_PRESET
#elif defined(CONFIG_BMM150_PRESET_ENHANCED_REGULAR)
	#define BMM150_DEFAULT_PRESET BMM150_ENHANCED_REGULAR_PRESET
#elif defined(CONFIG_BMM150_PRESET_HIGH_ACCURACY)
	#define BMM150_DEFAULT_PRESET BMM150_HIGH_ACCURACY_PRESET
#endif

#endif /* __SENSOR_BMM150_H__ */
