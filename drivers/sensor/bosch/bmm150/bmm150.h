/* sensor_bmm150.h - header file for BMM150 Geomagnetic sensor driver */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMM150_BMM150_H_
#define ZEPHYR_DRIVERS_SENSOR_BMM150_BMM150_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT bosch_bmm150

#define BMM150_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define BMM150_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union bmm150_bus {
#if BMM150_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if BMM150_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*bmm150_bus_check_fn)(const union bmm150_bus *bus);
typedef int (*bmm150_reg_read_fn)(const union bmm150_bus *bus,
				  uint8_t start, uint8_t *buf, int size);
typedef int (*bmm150_reg_write_fn)(const union bmm150_bus *bus,
				   uint8_t reg, uint8_t val);

struct bmm150_bus_io {
	bmm150_bus_check_fn check;
	bmm150_reg_read_fn read;
	bmm150_reg_write_fn write;
};

#if BMM150_BUS_SPI
#define BMM150_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB |	\
			      SPI_MODE_CPOL | SPI_MODE_CPHA)
extern const struct bmm150_bus_io bmm150_bus_io_spi;
#endif

#if BMM150_BUS_I2C
extern const struct bmm150_bus_io bmm150_bus_io_i2c;
#endif

#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/gpio.h>

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
#define BMM150_MASK_SOFT_RESET     (BIT(7) | BIT(1))
#define BMM150_SOFT_RESET          BMM150_MASK_SOFT_RESET

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
#define BMM150_REPZ_TO_REGVAL(rep)         BMM150_REPXY_TO_REGVAL(rep)

#define BMM150_REG_INT                     0x4D

#define BMM150_REG_INT_DRDY                0x4E
#define BMM150_MASK_DRDY_EN                BIT(7)
#define BMM150_SHIFT_DRDY_EN               7
#define BMM150_DRDY_INT3                   BIT(6)
#define BMM150_MASK_DRDY_Z_EN              BIT(5)
#define BMM150_MASK_DRDY_Y_EN              BIT(4)
#define BMM150_MASK_DRDY_X_EN              BIT(3)
#define BMM150_MASK_DRDY_DR_POLARITY       BIT(2)
#define BMM150_SHIFT_DRDY_DR_POLARITY      2
#define BMM150_MASK_DRDY_LATCHING          BIT(1)
#define BMM150_MASK_DRDY_INT3_POLARITY     BIT(0)

#if defined(CONFIG_BMM150_SAMPLING_REP_XY) || \
	defined(CONFIG_BMM150_SAMPLING_REP_Z)
	#define BMM150_SET_ATTR_REP
#endif

#if defined(CONFIG_BMM150_SAMPLING_RATE_RUNTIME) || \
	defined(BMM150_MAGN_SET_ATTR_REP)
	#define BMM150_MAGN_SET_ATTR
#endif

struct bmm150_trim_regs {
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

struct bmm150_config {
	union bmm150_bus bus;
	const struct bmm150_bus_io *bus_io;

#ifdef CONFIG_BMM150_TRIGGER
	struct gpio_dt_spec drdy_int;
#endif
};

struct bmm150_data {
	struct bmm150_trim_regs tregs;
	int rep_xy, rep_z, odr, max_odr;
	int sample_x, sample_y, sample_z;

#if defined(CONFIG_BMM150_TRIGGER)
	struct gpio_callback gpio_cb;
#endif

#ifdef CONFIG_BMM150_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_BMM150_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif

#if defined(CONFIG_BMM150_TRIGGER_GLOBAL_THREAD) || \
	defined(CONFIG_BMM150_TRIGGER_DIRECT)
	const struct device *dev;
#endif

#ifdef CONFIG_BMM150_TRIGGER
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t drdy_handler;
#endif /* CONFIG_BMM150_TRIGGER */
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
	#define BMM150_DEFAULT_PRESET BMM150_LOW_POWER_PRESET
#elif defined(CONFIG_BMM150_PRESET_REGULAR)
	#define BMM150_DEFAULT_PRESET BMM150_REGULAR_PRESET
#elif defined(CONFIG_BMM150_PRESET_ENHANCED_REGULAR)
	#define BMM150_DEFAULT_PRESET BMM150_ENHANCED_REGULAR_PRESET
#elif defined(CONFIG_BMM150_PRESET_HIGH_ACCURACY)
	#define BMM150_DEFAULT_PRESET BMM150_HIGH_ACCURACY_PRESET
#endif

/* Power On Reset time - from OFF to Suspend (Max) */
#define BMM150_POR_TIME K_MSEC(1)

/* Start-Up Time - from suspend to sleep (Max) */
#define BMM150_START_UP_TIME K_MSEC(3)

int bmm150_trigger_mode_init(const struct device *dev);

int bmm150_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int bmm150_reg_update_byte(const struct device *dev, uint8_t reg,
			   uint8_t mask, uint8_t value);

#endif /* __SENSOR_BMM150_H__ */
