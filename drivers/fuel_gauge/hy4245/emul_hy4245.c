/*
 * Copyright (c) 2026, Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the HYCON HY4245 fuel gauge.
 *
 * Models the standard commands with fixed readings, the Control()
 * subcommands, the seal and calibration mode state and a small data flash
 * with the BlockData() checksum handling of the device.
 */

#define DT_DRV_COMPAT hycon_hy4245

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/fuel_gauge/hy4245.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "hy4245.h"

LOG_MODULE_REGISTER(EMUL_HY4245);

/* Fixed readings of the emulated gauge */
#define EMUL_HY4245_TEMPERATURE_DK      2981
#define EMUL_HY4245_VOLTAGE_MV          3700
#define EMUL_HY4245_CURRENT_MA          -1234
#define EMUL_HY4245_AVG_CURRENT_MA      -250
#define EMUL_HY4245_REMAINING_MAH       1200
#define EMUL_HY4245_FULL_CHARGE_MAH     2400
#define EMUL_HY4245_TIME_TO_EMPTY_MIN   300
#define EMUL_HY4245_TIME_TO_FULL_MIN    120
#define EMUL_HY4245_STATE_OF_CHARGE_PCT 55
#define EMUL_HY4245_STATE_OF_HEALTH_PCT 99
#define EMUL_HY4245_CHARGE_VOLTAGE_MV   4200
#define EMUL_HY4245_CHARGE_CURRENT_MA   1500
#define EMUL_HY4245_DESIGN_CAPACITY_MAH 2500
#define EMUL_HY4245_FW_VERSION          0x2006
#define EMUL_HY4245_DF_VERSION          0x0003
#define EMUL_HY4245_SAFETY_STATUS       0x0000
#define EMUL_HY4245_OVER_TEMP_MIN       42

/* Discharging, battery detected, capacity learned */
#define EMUL_HY4245_FLAGS_INIT (HY4245_FLAGS_DSG | HY4245_FLAGS_BAT_DET | HY4245_FLAGS_LRND)

/* OperationCfgA() defaults of the data sheet, UPD_EN cleared */
#define EMUL_HY4245_OPERATION_CFG_A_INIT                                                           \
	(HY4245_OPERATION_CONFIG_A_TEMPS | HY4245_OPERATION_CONFIG_A_SE_EN |                       \
	 HY4245_OPERATION_CONFIG_A_RMFCC | HY4245_OPERATION_CONFIG_A_SLEEP |                       \
	 HY4245_OPERATION_CONFIG_A_FACDC_EN)

struct emul_hy4245_flash_block {
	uint8_t subclass;
	uint8_t block;
	uint8_t data[HY4245_DATA_FLASH_BLOCK_SIZE];
};

struct emul_hy4245_data {
	uint16_t ctrl_status;
	/* Value returned when Control() is read after a subcommand */
	uint16_t ctrl_result;
	uint16_t flags;
	uint16_t operation_cfg_a;
	bool unsealed;
	/* First unseal key received, waiting for the second one */
	bool unseal_key_pending;
	uint8_t block_data_control;
	uint8_t subclass;
	uint8_t block;
	/* BlockData() window, committed to the flash by BlockDataChecksum() */
	uint8_t block_data[HY4245_DATA_FLASH_BLOCK_SIZE];
	struct emul_hy4245_flash_block flash[7];
};

struct emul_hy4245_cfg {
	uint16_t addr;
};

static struct emul_hy4245_flash_block *emul_hy4245_find_block(struct emul_hy4245_data *data,
							      uint8_t subclass, uint8_t block)
{
	for (size_t i = 0; i < ARRAY_SIZE(data->flash); i++) {
		if (data->flash[i].subclass == subclass && data->flash[i].block == block) {
			return &data->flash[i];
		}
	}

	return NULL;
}

/* BlockDataChecksum(): complement of the least significant byte of the sum */
static uint8_t emul_hy4245_block_checksum(const uint8_t *buf, size_t len)
{
	uint8_t sum = 0;

	for (size_t i = 0; i < len; i++) {
		sum += buf[i];
	}

	return 0xff - sum;
}

static uint16_t emul_hy4245_df_checksum(const struct emul_hy4245_data *data)
{
	uint16_t sum = 0;

	for (size_t i = 0; i < ARRAY_SIZE(data->flash); i++) {
		for (size_t j = 0; j < HY4245_DATA_FLASH_BLOCK_SIZE; j++) {
			sum += data->flash[i].data[j];
		}
	}

	return sum;
}

static void emul_hy4245_load_window(struct emul_hy4245_data *data)
{
	const struct emul_hy4245_flash_block *blk =
		emul_hy4245_find_block(data, data->subclass, data->block);

	if (blk != NULL) {
		memcpy(data->block_data, blk->data, sizeof(data->block_data));
	} else {
		memset(data->block_data, 0, sizeof(data->block_data));
	}
}

static void emul_hy4245_reset(struct emul_hy4245_data *data)
{
	data->unsealed = false;
	data->unseal_key_pending = false;
	data->ctrl_status =
		HY4245_CONTROL_STATUS_SS | HY4245_CONTROL_STATUS_CSV | HY4245_CONTROL_STATUS_VOK;
	data->ctrl_result = 0;
	data->block_data_control = 0;
	data->subclass = 0;
	data->block = 0;
	memset(data->block_data, 0, sizeof(data->block_data));
}

static int emul_hy4245_control(struct emul_hy4245_data *data, uint16_t subcmd)
{
	switch (subcmd) {
	case HY4245_SUBCMD_CTRL_STATUS:
		data->ctrl_result = data->ctrl_status;
		break;
	case HY4245_SUBCMD_CTRL_DF_CHECKSUM:
		/* The device only computes the checksum while unsealed */
		if (data->unsealed) {
			data->ctrl_result = emul_hy4245_df_checksum(data);
		}
		break;
	case HY4245_SUBCMD_CTRL_DF_VERSION:
		data->ctrl_result = EMUL_HY4245_DF_VERSION;
		break;
	case HY4245_SUBCMD_CTRL_SET_UPD_EN:
		data->operation_cfg_a |= HY4245_OPERATION_CONFIG_A_UPD_EN;
		break;
	case HY4245_SUBCMD_CTRL_CALIB_MODE:
		/* Only an unsealed gauge enters calibration mode */
		if (data->unsealed) {
			data->ctrl_status |= HY4245_CONTROL_STATUS_BCA;
		}
		break;
	case HY4245_SUBCMD_CTRL_RESET:
		emul_hy4245_reset(data);
		break;
	case HY4245_SUBCMD_CTRL_QUICK_START:
		break;
	case HY4245_SUBCMD_CTRL_CLEAR_LEARNED:
		data->flags &= ~HY4245_FLAGS_LRND;
		break;
	case HY4245_SUBCMD_CTRL_CHIPID:
		data->ctrl_result = HY4245_CHIPID;
		break;
	case HY4245_SUBCMD_CTRL_FW_VERSION:
		data->ctrl_result = EMUL_HY4245_FW_VERSION;
		break;
	case HY4245_SUBCMD_CTRL_OPERATION_CFG_A:
		data->ctrl_result = data->operation_cfg_a;
		break;
	case HY4245_SUBCMD_CTRL_SAFETY_STATUS:
		data->ctrl_result = EMUL_HY4245_SAFETY_STATUS;
		break;
	case HY4245_SUBCMD_CTRL_LIFETIME_OVER_TEMP:
		data->ctrl_result = EMUL_HY4245_OVER_TEMP_MIN;
		break;
	default:
		LOG_ERR("Unknown Control() subcommand 0x%04x", subcmd);
		return -EIO;
	}

	return 0;
}

static void emul_hy4245_unseal_key(struct emul_hy4245_data *data, const uint8_t *key)
{
	static const uint8_t key0[] = {HY4245_UNSEAL_KEY_0};
	static const uint8_t key1[] = {HY4245_UNSEAL_KEY_1};

	if (memcmp(key, key0, sizeof(key0)) == 0) {
		data->unseal_key_pending = true;
		return;
	}

	if (data->unseal_key_pending && memcmp(key, key1, sizeof(key1)) == 0) {
		data->unsealed = true;
		data->ctrl_status &= ~HY4245_CONTROL_STATUS_SS;
	}

	data->unseal_key_pending = false;
}

/* Handle a write transfer: register followed by an optional payload */
static int emul_hy4245_write(struct emul_hy4245_data *data, const uint8_t *buf, size_t len)
{
	const uint8_t reg = buf[0];
	const uint8_t *payload = &buf[1];
	const size_t plen = len - 1;
	struct emul_hy4245_flash_block *blk;

	/* A write of the register alone selects it for the following read */
	if (plen == 0) {
		return 0;
	}

	switch (reg) {
	case HY4245_CMD_CTRL:
		if (plen == 2) {
			return emul_hy4245_control(data, sys_get_le16(payload));
		}
		if (plen == 4) {
			emul_hy4245_unseal_key(data, payload);
			return 0;
		}
		break;
	case HY4245_EXTCMD_SUBCLASS:
		if (plen == 1) {
			data->subclass = payload[0];
			emul_hy4245_load_window(data);
			return 0;
		}
		break;
	case HY4245_EXTCMD_BLOCK:
		if (plen == 1) {
			data->block = payload[0];
			emul_hy4245_load_window(data);
			return 0;
		}
		break;
	case HY4245_EXTCMD_BLKDATA:
		if (plen == HY4245_DATA_FLASH_BLOCK_SIZE) {
			if (!data->unsealed) {
				LOG_ERR("Data flash write while sealed");
				return -EIO;
			}
			memcpy(data->block_data, payload, plen);
			return 0;
		}
		break;
	case HY4245_EXTCMD_BLKDATA_CHECKSUM:
		if (plen == 1) {
			blk = emul_hy4245_find_block(data, data->subclass, data->block);
			if (!data->unsealed || blk == NULL) {
				return -EIO;
			}
			/* The block is only committed when the checksum matches */
			if (payload[0] == emul_hy4245_block_checksum(data->block_data,
								     sizeof(data->block_data))) {
				memcpy(blk->data, data->block_data, sizeof(blk->data));
			} else {
				LOG_WRN("Checksum mismatch, block 0x%02x/%u not written",
					data->subclass, data->block);
			}
			return 0;
		}
		break;
	case HY4245_EXTCMD_BLKDATA_CTRL:
		if (plen == 1) {
			data->block_data_control = payload[0];
			return 0;
		}
		break;
	default:
		break;
	}

	LOG_ERR("Unsupported write to register 0x%02x with %zu bytes", reg, plen);
	return -EIO;
}

static int emul_hy4245_read16(struct emul_hy4245_data *data, uint8_t reg, uint16_t *val)
{
	switch (reg) {
	case HY4245_CMD_CTRL:
		*val = data->ctrl_result;
		break;
	case HY4245_CMD_TEMPERATURE:
		*val = EMUL_HY4245_TEMPERATURE_DK;
		break;
	case HY4245_CMD_VOLTAGE:
		*val = EMUL_HY4245_VOLTAGE_MV;
		break;
	case HY4245_CMD_FLAGS:
		*val = data->flags;
		break;
	case HY4245_CMD_CURRENT:
		*val = (uint16_t)(int16_t)EMUL_HY4245_CURRENT_MA;
		break;
	case HY4245_CMD_CAPACITY_REM:
		*val = EMUL_HY4245_REMAINING_MAH;
		break;
	case HY4245_CMD_CAPACITY_FULL:
		*val = EMUL_HY4245_FULL_CHARGE_MAH;
		break;
	case HY4245_CMD_AVG_CURRENT:
		*val = (uint16_t)(int16_t)EMUL_HY4245_AVG_CURRENT_MA;
		break;
	case HY4245_CMD_TIME_TO_EMPTY:
		*val = EMUL_HY4245_TIME_TO_EMPTY_MIN;
		break;
	case HY4245_CMD_TIME_TO_FULL:
		*val = EMUL_HY4245_TIME_TO_FULL_MIN;
		break;
	case HY4245_CMD_RELATIVE_STATE_OF_CHRG:
		*val = EMUL_HY4245_STATE_OF_CHARGE_PCT;
		break;
	case HY4245_CMD_STATE_OF_HEALTH:
		*val = EMUL_HY4245_STATE_OF_HEALTH_PCT;
		break;
	case HY4245_CMD_CHRG_VOLTAGE:
		*val = EMUL_HY4245_CHARGE_VOLTAGE_MV;
		break;
	case HY4245_CMD_CHRG_CURRENT:
		*val = EMUL_HY4245_CHARGE_CURRENT_MA;
		break;
	case HY4245_CMD_CAPACITY_FULL_AVAIL:
		*val = EMUL_HY4245_DESIGN_CAPACITY_MAH;
		break;
	default:
		LOG_ERR("Unknown register 0x%02x read", reg);
		return -EIO;
	}

	return 0;
}

/* Handle the read part of a transfer for the previously selected register */
static int emul_hy4245_read(struct emul_hy4245_data *data, uint8_t reg, uint8_t *buf, size_t len)
{
	const struct emul_hy4245_flash_block *blk;
	uint16_t val;
	int ret;

	switch (reg) {
	case HY4245_EXTCMD_BLKDATA:
		if (len != HY4245_DATA_FLASH_BLOCK_SIZE || !data->unsealed) {
			LOG_ERR("Invalid data flash read, len %zu, unsealed %d", len,
				data->unsealed);
			return -EIO;
		}
		memcpy(buf, data->block_data, len);
		return 0;
	case HY4245_EXTCMD_BLKDATA_CHECKSUM:
		blk = emul_hy4245_find_block(data, data->subclass, data->block);
		if (len != 1 || blk == NULL) {
			return -EIO;
		}
		buf[0] = emul_hy4245_block_checksum(blk->data, sizeof(blk->data));
		return 0;
	case HY4245_EXTCMD_BLKDATA_CTRL:
		if (len != 1) {
			return -EIO;
		}
		buf[0] = data->block_data_control;
		return 0;
	default:
		if (len != 2) {
			LOG_ERR("Register 0x%02x read with %zu bytes", reg, len);
			return -EIO;
		}
		ret = emul_hy4245_read16(data, reg, &val);
		if (ret != 0) {
			return ret;
		}
		sys_put_le16(val, buf);
		return 0;
	}
}

static int emul_hy4245_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct emul_hy4245_data *data = target->data;
	const struct emul_hy4245_cfg *cfg = target->cfg;
	int ret;

	__ASSERT_NO_MSG(msgs && num_msgs);

	if (addr != cfg->addr) {
		LOG_ERR("Address mismatch, expected 0x%02x, got 0x%02x", cfg->addr, addr);
		return -EIO;
	}

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if ((msgs[0].flags & I2C_MSG_READ) || msgs[0].len == 0) {
		LOG_ERR("First message must be a write with the register");
		return -EIO;
	}

	ret = emul_hy4245_write(data, msgs[0].buf, msgs[0].len);
	if (ret != 0 || num_msgs == 1) {
		return ret;
	}

	if (num_msgs != 2 || !(msgs[1].flags & I2C_MSG_READ)) {
		LOG_ERR("Unsupported message sequence");
		return -EIO;
	}

	return emul_hy4245_read(data, msgs[0].buf[0], msgs[1].buf, msgs[1].len);
}

static const struct i2c_emul_api emul_hy4245_api_i2c = {
	.transfer = emul_hy4245_transfer_i2c,
};

static int emul_hy4245_init(const struct emul *target, const struct device *parent)
{
	struct emul_hy4245_data *data = target->data;
	static const struct {
		uint8_t subclass;
		uint8_t block;
	} blocks[] = {
		{0x02, 0},
		{0x02, 1},
		{0x03, 0},
		{0x03, 1},
		{HY4245_SUBCLASS_MANUFACTURER_INFO, 0},
		{HY4245_SUBCLASS_MANUFACTURER_INFO, 1},
		{HY4245_SUBCLASS_MANUFACTURER_INFO, 2},
	};

	ARG_UNUSED(parent);

	BUILD_ASSERT(ARRAY_SIZE(blocks) == ARRAY_SIZE(data->flash));

	for (size_t i = 0; i < ARRAY_SIZE(blocks); i++) {
		data->flash[i].subclass = blocks[i].subclass;
		data->flash[i].block = blocks[i].block;
		for (size_t j = 0; j < HY4245_DATA_FLASH_BLOCK_SIZE; j++) {
			data->flash[i].data[j] =
				(uint8_t)(blocks[i].subclass * 16 + blocks[i].block * 4 + j);
		}
	}

	data->flags = EMUL_HY4245_FLAGS_INIT;
	data->operation_cfg_a = EMUL_HY4245_OPERATION_CFG_A_INIT;
	emul_hy4245_reset(data);

	return 0;
}

#define EMUL_HY4245_DEFINE(n)                                                                      \
	static struct emul_hy4245_data emul_hy4245_data_##n;                                       \
	static const struct emul_hy4245_cfg emul_hy4245_cfg_##n = {                                \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_hy4245_init, &emul_hy4245_data_##n, &emul_hy4245_cfg_##n,      \
			    &emul_hy4245_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(EMUL_HY4245_DEFINE)
