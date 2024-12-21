/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MMC56X3_MMC56X3_H_
#define ZEPHYR_DRIVERS_SENSOR_MMC56X3_MMC56X3_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/sensor/mmc56x3.h>

#define DT_DRV_COMPAT memsic_mmc56x3

#define MMC56X3_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union mmc56x3_bus {
	struct i2c_dt_spec i2c;
};

typedef int (*mmc56x3_bus_check_fn)(const union mmc56x3_bus *bus);
typedef int (*mmc56x3_reg_read_fn)(const union mmc56x3_bus *bus, uint8_t start, uint8_t *buf,
				   int size);
typedef int (*mmc56x3_reg_write_fn)(const union mmc56x3_bus *bus, uint8_t reg, uint8_t val);
typedef int (*mmc56x3_raw_read_fn)(const union mmc56x3_bus *bus, uint8_t *buf, size_t size);
typedef int (*mmc56x3_raw_write_fn)(const union mmc56x3_bus *bus, uint8_t *buf, size_t size);

struct mmc56x3_bus_io {
	mmc56x3_bus_check_fn check;
	mmc56x3_reg_read_fn read;
	mmc56x3_reg_write_fn write;
	mmc56x3_raw_read_fn raw_read;
	mmc56x3_raw_write_fn raw_write;
};

extern const struct mmc56x3_bus_io mmc56x3_bus_io_i2c;

#define MMC56X3_REG_TEMP            0x09
#define MMC56X3_CHIP_ID             0x10
#define MMC56X3_REG_STATUS          0x18
#define MMC56X3_REG_INTERNAL_ODR    0x1a
#define MMC56X3_REG_INTERNAL_CTRL_0 0x1b
#define MMC56X3_REG_INTERNAL_CTRL_1 0x1c
#define MMC56X3_REG_INTERNAL_CTRL_2 0x1d
#define MMC56X3_REG_ID              0x39

#define MMC56X3_CMD_RESET              0x10
#define MMC56X3_CMD_SET                0x08
#define MMC56X3_CMD_SW_RESET           0x80
#define MMC56X3_CMD_TAKE_MEAS_M        0x01
#define MMC56X3_CMD_TAKE_MEAS_T        0x02
#define MMC56X3_CMD_AUTO_SELF_RESET_EN 0x20
#define MMC56X3_CMD_CMM_FREQ_EN        0x80
#define MMC56X3_CMD_CMM_EN             0x10
#define MMC56X3_CMD_HPOWER             0x80

#define MMC56X3_STATUS_MEAS_M_DONE 0x80
#define MMC56X3_STATUS_MEAS_T_DONE 0x40

#define MMC56X3_REG_MAGN_X_OUT_0     0x00
/* Range is -30 to 30, sensitivity of raw 20-bit reading is
 * 16384 = 1 Gauss. To convert raw reading to
 * Q5.26 with range -32 to 32,
 * reading * (1/16384) * pow(2, 31)/32
 * = reading * 4096
 */
#define MMC56X3_MAGN_CONV_Q5_26_20B  4096
/* 1/16384 */
#define MMC56X3_MAGN_GAUSS_RES       0.000061035
/* To convert reading to Q7.24 with range -128, 128,
 * (BASE + reading * RES) * pow(2, 31)/128
 * = BASE * pow(2, 31)/128 + reading * RES * pow(2, 31)/128
 * CONV_BASE = BASE * pow(2, 31)/128
 * CONV_RES = RES * pow(2, 31)/128
 * = CONV_BASE + reading * CONV_RES
 */
#define MMC56X3_TEMP_BASE            -75
#define MMC56X3_TEMP_RES             0.8
#define MMC56X3_TEMP_CONV_Q7_24_BASE -1258291200
#define MMC56X3_TEMP_CONV_Q7_24_RES  13421773

#define MMC56X3_MAGN_SHIFT 5
#define MMC56X3_TEMP_SHIFT 7

#ifdef __cplusplus
extern "C" {
#endif

struct mmc56x3_config {
	uint16_t magn_odr;
	bool bw0;
	bool bw1;
	bool auto_sr;
};

struct mmc56x3_data {
	struct mmc56x3_config config;

	uint8_t ctrl0_cache;
	uint8_t ctrl1_cache;
	uint8_t ctrl2_cache;

	uint32_t temp;
	int32_t magn_x;
	int32_t magn_y;
	int32_t magn_z;
};

struct mmc56x3_dev_config {
	union mmc56x3_bus bus;
	const struct mmc56x3_bus_io *bus_io;
};

struct mmc56x3_decoder_header {
	uint64_t timestamp;
} __attribute__((__packed__));

struct mmc56x3_encoded_data {
	struct mmc56x3_decoder_header header;
	struct {
		/** Set if `temp` has data */
		uint8_t has_temp: 1;
		/** Set if `magn_x` has data */
		uint8_t has_magn_x: 1;
		/** Set if `magn_y` has data */
		uint8_t has_magn_y: 1;
		/** Set if `magn_z` has data */
		uint8_t has_magn_z: 1;
	} __attribute__((__packed__));
	struct mmc56x3_data data;
};

int mmc56x3_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

void mmc56x3_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

int mmc56x3_sample_fetch(const struct device *dev, enum sensor_channel chan);

int mmc56x3_sample_fetch_helper(const struct device *dev, enum sensor_channel chan,
				struct mmc56x3_data *data);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_MMC56X3_MMC56X3_H_ */
