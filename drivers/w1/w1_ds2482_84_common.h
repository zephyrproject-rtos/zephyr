/*
 * Copyright (c) 2023 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_W1_W1_DS2482_84_H_
#define ZEPHYR_DRIVERS_W1_W1_DS2482_84_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define CMD_1WT	 0x78
#define CMD_1WSB 0x87
#define CMD_1WRB 0x96
#define CMD_1WWB 0xa5
#define CMD_1WRS 0xb4
#define CMD_CHSL 0xc3 /* DS2482-800 only */
#define CMD_ADJP 0xc3 /* DS2484 only */
#define CMD_WCFG 0xd2
#define CMD_SRP	 0xe1
#define CMD_DRST 0xf0

#define REG_NONE    0x00 /* special value */
#define REG_CONFIG  0xc3
#define REG_DATA    0xe1
#define REG_STATUS  0xf0
#define REG_CHANNEL 0xd2 /* DS2482-800 only */
#define REG_PORT    0xb4 /* DS2484 only */

/*
 * Device Configuration Register
 */
#define DEVICE_APU_pos 0
#define DEVICE_APU_msk BIT(DEVICE_APU_pos)
#define DEVICE_PDN_pos 1		   /* DS2484 only */
#define DEVICE_PDN_msk BIT(DEVICE_PDN_pos) /* DS2484 only */
#define DEVICE_SPU_pos 2
#define DEVICE_SPU_msk BIT(DEVICE_SPU_pos)
#define DEVICE_1WS_pos 3
#define DEVICE_1WS_msk BIT(DEVICE_1WS_pos)

/*
 * Status Register
 */
#define STATUS_1WB_pos 0
#define STATUS_1WB_msk BIT(STATUS_1WB_pos)
#define STATUS_PPD_pos 1
#define STATUS_PPD_msk BIT(STATUS_PPD_pos)
#define STATUS_SD_pos  2
#define STATUS_SD_msk  BIT(STATUS_SD_pos)
#define STATUS_LL_pos  3
#define STATUS_LL_msk  BIT(STATUS_LL_pos)
#define STATUS_RST_pos 4
#define STATUS_RST_msk BIT(STATUS_RST_pos)
#define STATUS_SBR_pos 5
#define STATUS_SBR_msk BIT(STATUS_SBR_pos)
#define STATUS_TSB_pos 6
#define STATUS_TSB_msk BIT(STATUS_TSB_pos)
#define STATUS_DIR_pos 7
#define STATUS_DIR_msk BIT(STATUS_DIR_pos)

/*
 * Channel Selection Codes, DS2482-800 only
 */
#define CHSL_IO0 0xf0
#define CHSL_IO1 0xe1
#define CHSL_IO2 0xd2
#define CHSL_IO3 0xc3
#define CHSL_IO4 0xb4
#define CHSL_IO5 0xa5
#define CHSL_IO6 0x96
#define CHSL_IO7 0x87

/*
 * Channel Selection Codes (read back values), DS2482-800 only
 */
#define CHSL_RB_IO0 0xb8
#define CHSL_RB_IO1 0xb1
#define CHSL_RB_IO2 0xaa
#define CHSL_RB_IO3 0xa3
#define CHSL_RB_IO4 0x9c
#define CHSL_RB_IO5 0x95
#define CHSL_RB_IO6 0x8e
#define CHSL_RB_IO7 0x87

/*
 * Port Configuration Register, DS2484 only
 */
#define PORT_VAL0_pos 0
#define PORT_VAL0_msk BIT(PORT_VAL0_pos)
#define PORT_VAL1_pos 1
#define PORT_VAL1_msk BIT(PORT_VAL1_pos)
#define PORT_VAL2_pos 2
#define PORT_VAL2_msk BIT(PORT_VAL2_pos)
#define PORT_VAL3_pos 3
#define PORT_VAL3_msk BIT(PORT_VAL3_pos)

/*
 * Bit Byte
 */
#define BIT_CLR_msk 0
#define BIT_SET_msk BIT(7)

static inline int ds2482_84_write(const struct i2c_dt_spec *spec, uint8_t cmd, const uint8_t *data)
{
	int ret;

	const uint8_t buf[] = {cmd, data ? *data : 0};

	ret = i2c_write_dt(spec, buf, data ? 2 : 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static inline int ds2482_84_read(const struct i2c_dt_spec *spec, uint8_t rp, uint8_t *reg)
{
	int ret;

	switch (rp) {
	case REG_NONE:
		/*
		 * Special value: Don't change read pointer
		 */
		break;
	case REG_PORT:
		__fallthrough;
	case REG_CONFIG:
		__fallthrough;
	case REG_CHANNEL:
		__fallthrough;
	case REG_DATA:
		__fallthrough;
	case REG_STATUS:
		ret = ds2482_84_write(spec, CMD_SRP, &rp);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	ret = i2c_read_dt(spec, reg, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static inline int ds2482_84_reset_bus(const struct i2c_dt_spec *spec)
{
	int ret;

	uint8_t reg;

	ret = ds2482_84_write(spec, CMD_1WRS, NULL);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = ds2482_84_read(spec, REG_NONE, &reg);
		if (ret < 0) {
			return ret;
		}
	} while (reg & STATUS_1WB_msk);

	return reg & STATUS_PPD_msk ? 1 : 0;
}

static inline int ds2482_84_reset_device(const struct i2c_dt_spec *spec)
{
	int ret;

	uint8_t reg;

	ret = ds2482_84_write(spec, CMD_DRST, NULL);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = ds2482_84_read(spec, REG_NONE, &reg);
		if (ret < 0) {
			return ret;
		}
	} while (!(reg & STATUS_RST_msk));

	return 0;
}

static inline int ds2482_84_single_bit(const struct i2c_dt_spec *spec, uint8_t bit_msk)
{
	int ret;

	uint8_t reg;

	ret = ds2482_84_write(spec, CMD_1WSB, &bit_msk);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = ds2482_84_read(spec, REG_NONE, &reg);
		if (ret < 0) {
			return ret;
		}
	} while (reg & STATUS_1WB_msk);

	return reg & STATUS_SBR_msk ? 1 : 0;
}

static inline int ds2482_84_read_bit(const struct i2c_dt_spec *spec)
{
	return ds2482_84_single_bit(spec, BIT_SET_msk);
}

static inline int ds2482_84_write_bit(const struct i2c_dt_spec *spec, bool bit)
{
	return ds2482_84_single_bit(spec, bit ? BIT_SET_msk : BIT_CLR_msk);
}

static inline int ds2482_84_read_byte(const struct i2c_dt_spec *spec)
{
	int ret;

	uint8_t reg;

	ret = ds2482_84_write(spec, CMD_1WRB, NULL);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = ds2482_84_read(spec, REG_NONE, &reg);
		if (ret < 0) {
			return ret;
		}
	} while (reg & STATUS_1WB_msk);

	ret = ds2482_84_read(spec, REG_DATA, &reg);
	if (ret < 0) {
		return ret;
	}

	return reg;
}

static inline int ds2482_84_write_byte(const struct i2c_dt_spec *spec, uint8_t byte)
{
	int ret;

	uint8_t reg;

	ret = ds2482_84_write(spec, CMD_1WWB, &byte);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = ds2482_84_read(spec, REG_NONE, &reg);
		if (ret < 0) {
			return ret;
		}
	} while (reg & STATUS_1WB_msk);

	return 0;
}

static inline int ds2482_84_write_config(const struct i2c_dt_spec *spec, uint8_t cfg)
{
	int ret;

	uint8_t reg = cfg | ~cfg << 4;

	if (cfg & ~(DEVICE_APU_msk | DEVICE_PDN_msk | DEVICE_SPU_msk | DEVICE_1WS_msk)) {
		return -EINVAL;
	}

	ret = ds2482_84_write(spec, CMD_WCFG, &reg);
	if (ret < 0) {
		return ret;
	}

	ret = ds2482_84_read(spec, REG_NONE, &reg);
	if (ret < 0) {
		return ret;
	}

	return (reg == cfg) ? 0 : -EIO;
}

#endif /* ZEPHYR_DRIVERS_W1_W1_DS2482_84_H_ */
