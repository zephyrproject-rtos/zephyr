/*
 * Copyright (c) 2026 Junseo Pyun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TLV493D_TLV493D_H_
#define ZEPHYR_DRIVERS_SENSOR_TLV493D_TLV493D_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>

#define TLV493D_RD_REG_X     0x00
#define TLV493D_RD_REG_Y     0x01
#define TLV493D_RD_REG_Z     0x02
#define TLV493D_RD_REG_TEMP  0x03
#define TLV493D_RD_REG_X2    0x04
#define TLV493D_RD_REG_Z2    0x05
#define TLV493D_RD_REG_TEMP2 0x06
#define TLV493D_RD_REG_RES1  0x07
#define TLV493D_RD_REG_RES2  0x08
#define TLV493D_RD_REG_RES3  0x09
#define TLV493D_RD_REG_MAX   0x0a
#define TLV493D_WR_REG_MODE1 0x01
#define TLV493D_WR_REG_MODE2 0x03
#define TLV493D_WR_REG_MAX   0x04

#define TLV493D_X_MAG_X_AXIS_MSB    GENMASK(7, 0)
#define TLV493D_X2_MAG_X_AXIS_LSB   GENMASK(7, 4)
#define TLV493D_Y_MAG_Y_AXIS_MSB    GENMASK(7, 0)
#define TLV493D_X2_MAG_Y_AXIS_LSB   GENMASK(3, 0)
#define TLV493D_Z_MAG_Z_AXIS_MSB    GENMASK(7, 0)
#define TLV493D_Z2_MAG_Z_AXIS_LSB   GENMASK(3, 0)
#define TLV493D_TEMP_TEMP_MSB       GENMASK(7, 4)
#define TLV493D_TEMP2_TEMP_LSB      GENMASK(7, 0)
#define TLV493D_TEMP_CHANNEL        GENMASK(1, 0)
#define TLV493D_MODE1_MOD_LOWFAST   GENMASK(1, 0)
#define TLV493D_MODE2_LP_PERIOD     BIT(6)
#define TLV493D_RD_REG_RES1_WR_MASK GENMASK(6, 3)
#define TLV493D_RD_REG_RES2_WR_MASK GENMASK(7, 0)
#define TLV493D_RD_REG_RES3_WR_MASK GENMASK(4, 0)

#define TLV493D_CONVERSION_FACTOR 1000000
#define TLV493D_MAG_SCALE_NUM     98
#define TLV493D_MAG_SCALE_DEN     100
#define TLV493D_TEMP_SCALE_NUM    11
#define TLV493D_TEMP_SCALE_DEN    10
#define TLV493D_TEMP_OFFSET       340
#define TLV493D_TEMP_REF          25

enum tlv493d_op_mode {
	TLV493D_OP_MODE_POWERDOWN,
	TLV493D_OP_MODE_FAST,
	TLV493D_OP_MODE_LOWPOWER,
	TLV493D_OP_MODE_ULTRA_LOWPOWER,
	TLV493D_OP_MODE_MASTERCONTROLLED,
};

/* Driver runtime data */
struct tlv493d_data {
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t t;
	uint8_t rd_regs[10];
	uint8_t wr_regs[4];
	enum tlv493d_op_mode mode;
};

/* Driver configuration */
struct tlv493d_config {
	struct i2c_dt_spec i2c;
	enum tlv493d_op_mode mode;
	bool temp_disable;
};

int tlv493d_init(const struct device *dev);
int tlv493d_sample_fetch(const struct device *dev, enum sensor_channel chan);
int tlv493d_channel_get(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val);

#endif /* ZEPHYR_DRIVERS_SENSOR_TLV493D_H_ */
