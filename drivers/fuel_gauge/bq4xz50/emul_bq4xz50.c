/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the bq4xz50 family fuel gauges
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(EMUL_BQ4XZ50, CONFIG_FUEL_GAUGE_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "bq4xz50.h"

/* PRES | DSG | SEC=2 (sealed); XCHG clear, so charging is not cut off. */
#define BQ4XZ50_EMUL_OPERATION_STATUS 0x00000203

/* ManufacturerAccess subcommand whose result is returned through ManufacturerData. */
#define BQ4XZ50_EMUL_MAC_SUBCMD_OPERATION_STATUS 0x0054

/*
 * Fixed readings for a discharging 4S pack. Values are deliberately distinct from one another so
 * that a driver reading the wrong register is caught rather than matching by coincidence.
 */
#define BQ4XZ50_EMUL_TEMPERATURE_DK    2980
#define BQ4XZ50_EMUL_VOLTAGE_MV        16800
#define BQ4XZ50_EMUL_CURRENT_MA        (-500)
#define BQ4XZ50_EMUL_AVG_CURRENT_MA    (-450)
#define BQ4XZ50_EMUL_REMAINING_CAP_MAH 3500
#define BQ4XZ50_EMUL_FULL_CHARGE_MAH   4000
#define BQ4XZ50_EMUL_DESIGN_CAP_MAH    4000
#define BQ4XZ50_EMUL_DESIGN_VOLT_MV    14400
#define BQ4XZ50_EMUL_CHARGING_CURR_MA  2000
#define BQ4XZ50_EMUL_CHARGING_VOLT_MV  16800
#define BQ4XZ50_EMUL_BATTERY_STATUS    0x00C0
#define BQ4XZ50_EMUL_CYCLE_COUNT       67
#define BQ4XZ50_EMUL_REL_CHARGE_PCT    87
#define BQ4XZ50_EMUL_ABS_CHARGE_PCT    85
#define BQ4XZ50_EMUL_STATE_OF_HEALTH   95
#define BQ4XZ50_EMUL_RUNTIME_EMPTY_MIN 420
#define BQ4XZ50_EMUL_AVG_EMPTY_MIN     430
#define BQ4XZ50_EMUL_BTP_DISCHARGE_MAH 150
#define BQ4XZ50_EMUL_BTP_CHARGE_MAH    175
/* AtRate defaults to 0, so the at-rate and charge-time estimates report "unknown". */
#define BQ4XZ50_EMUL_TIME_UNKNOWN      0xFFFF

/* Power-on values of the host-writable registers. */
#define BQ4XZ50_EMUL_DFLT_MFR_ACCESS   1
#define BQ4XZ50_EMUL_DFLT_CAP_ALARM    300
#define BQ4XZ50_EMUL_DFLT_TIME_ALARM   10
#define BQ4XZ50_EMUL_DFLT_BATTERY_MODE 0
#define BQ4XZ50_EMUL_DFLT_AT_RATE      0

#define BQ4XZ50_EMUL_MAC_CMD_LEN 2

struct bq4xz50_emul_cfg {
	uint16_t i2c_addr;
	/* Reported through DeviceName, so a test can tell the two parts apart. */
	const char *device_name;
};

/** Registers the host is allowed to write, so reads return what was written. */
struct bq4xz50_emul_data {
	uint16_t mfr_access;
	uint16_t remaining_capacity_alarm;
	uint16_t remaining_time_alarm;
	uint16_t battery_mode;
	int16_t at_rate;
};

static int emul_bq4xz50_buffer_read(const struct emul *target, int reg, uint8_t *buf, size_t len)
{
	const struct bq4xz50_emul_cfg *cfg = target->cfg;
	static const char manufacturer_name[] = "Texas Inst.";
	static const char device_chemistry[] = "LION";
	const char *str;

	switch (reg) {
	case BQ4XZ50_MANUFACTURERNAME:
		str = manufacturer_name;
		break;
	case BQ4XZ50_DEVICENAME:
		str = cfg->device_name;
		break;
	case BQ4XZ50_DEVICECHEMISTRY:
		str = device_chemistry;
		break;
	default:
		LOG_ERR("Buffer read for reg 0x%x is not supported", reg);
		return -EIO;
	}

	/* SMBus block read: a length byte followed by the payload. */
	if (len < strlen(str) + 1) {
		return -EIO;
	}

	buf[0] = strlen(str);
	memcpy(&buf[1], str, strlen(str));

	return 0;
}

static int emul_bq4xz50_reg_read(const struct emul *target, int reg, uint32_t *val)
{
	const struct bq4xz50_emul_data *data = target->data;

	switch (reg) {
	case BQ4XZ50_MANUFACTURERACCESS:
		*val = data->mfr_access;
		break;
	case BQ4XZ50_ATRATE:
		*val = (uint16_t)data->at_rate;
		break;
	case BQ4XZ50_ATRATETIMETOEMPTY:
	case BQ4XZ50_ATRATETIMETOFULL:
	case BQ4XZ50_AVERAGETIMETOFULL:
		*val = BQ4XZ50_EMUL_TIME_UNKNOWN;
		break;
	case BQ4XZ50_TEMPERATURE:
		*val = BQ4XZ50_EMUL_TEMPERATURE_DK;
		break;
	case BQ4XZ50_VOLTAGE:
		*val = BQ4XZ50_EMUL_VOLTAGE_MV;
		break;
	case BQ4XZ50_BATTERYSTATUS:
		*val = BQ4XZ50_EMUL_BATTERY_STATUS;
		break;
	case BQ4XZ50_CURRENT:
		*val = (uint16_t)BQ4XZ50_EMUL_CURRENT_MA;
		break;
	case BQ4XZ50_REMAININGCAPACITY:
		*val = BQ4XZ50_EMUL_REMAINING_CAP_MAH;
		break;
	case BQ4XZ50_FULLCHARGECAPACITY:
		*val = BQ4XZ50_EMUL_FULL_CHARGE_MAH;
		break;
	case BQ4XZ50_AVERAGECURRENT:
		*val = (uint16_t)BQ4XZ50_EMUL_AVG_CURRENT_MA;
		break;
	case BQ4XZ50_AVERAGETIMETOEMPTY:
		*val = BQ4XZ50_EMUL_AVG_EMPTY_MIN;
		break;
	case BQ4XZ50_BTPDISCHARGE:
		*val = BQ4XZ50_EMUL_BTP_DISCHARGE_MAH;
		break;
	case BQ4XZ50_BTPCHARGE:
		*val = BQ4XZ50_EMUL_BTP_CHARGE_MAH;
		break;
	case BQ4XZ50_CYCLECOUNT:
		*val = BQ4XZ50_EMUL_CYCLE_COUNT;
		break;
	case BQ4XZ50_RELATIVESTATEOFCHARGE:
		*val = BQ4XZ50_EMUL_REL_CHARGE_PCT;
		break;
	case BQ4XZ50_ABSOLUTESTATEOFCHARGE:
		*val = BQ4XZ50_EMUL_ABS_CHARGE_PCT;
		break;
	case BQ4XZ50_STATEOFHEALTH:
		*val = BQ4XZ50_EMUL_STATE_OF_HEALTH;
		break;
	case BQ4XZ50_CHARGINGVOLTAGE:
		*val = BQ4XZ50_EMUL_CHARGING_VOLT_MV;
		break;
	case BQ4XZ50_CHARGINGCURRENT:
		*val = BQ4XZ50_EMUL_CHARGING_CURR_MA;
		break;
	case BQ4XZ50_DESIGNCAPACITY:
		*val = BQ4XZ50_EMUL_DESIGN_CAP_MAH;
		break;
	case BQ4XZ50_DESIGNVOLTAGE:
		*val = BQ4XZ50_EMUL_DESIGN_VOLT_MV;
		break;
	case BQ4XZ50_RUNTIMETOEMPTY:
		*val = BQ4XZ50_EMUL_RUNTIME_EMPTY_MIN;
		break;
	case BQ4XZ50_BATTERYMODE:
		*val = data->battery_mode;
		break;
	case BQ4XZ50_ATRATEOK:
		*val = 0;
		break;
	case BQ4XZ50_REMAININGCAPACITYALARM:
		*val = data->remaining_capacity_alarm;
		break;
	case BQ4XZ50_REMAININGTIMEALARM:
		*val = data->remaining_time_alarm;
		break;
	case BQ4XZ50_OPERATIONSTATUS:
		*val = BQ4XZ50_EMUL_OPERATION_STATUS;
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}

	return 0;
}

static int emul_bq4xz50_manufacturer_data_read(const struct emul *target, uint8_t *buf, size_t len)
{
	const struct bq4xz50_emul_data *data = target->data;

	if (data->mfr_access != BQ4XZ50_EMUL_MAC_SUBCMD_OPERATION_STATUS) {
		LOG_ERR("No result staged for subcommand 0x%x", data->mfr_access);
		return -EIO;
	}
	if (len != sizeof(BQ4XZ50_EMUL_OPERATION_STATUS) + 1U) {
		LOG_ERR("Unexpected ManufacturerData read length %zu", len);
		return -EIO;
	}

	buf[0] = (uint8_t)sizeof(BQ4XZ50_EMUL_OPERATION_STATUS);
	sys_put_le32(BQ4XZ50_EMUL_OPERATION_STATUS, &buf[1]);

	return 0;
}

static int emul_bq4xz50_reg_write(const struct emul *target, int reg, uint16_t val)
{
	struct bq4xz50_emul_data *data = target->data;

	switch (reg) {
	case BQ4XZ50_MANUFACTURERACCESS:
		data->mfr_access = val;
		break;
	case BQ4XZ50_REMAININGCAPACITYALARM:
		data->remaining_capacity_alarm = val;
		break;
	case BQ4XZ50_REMAININGTIMEALARM:
		data->remaining_time_alarm = val;
		break;
	case BQ4XZ50_BATTERYMODE:
		data->battery_mode = val;
		break;
	case BQ4XZ50_ATRATE:
		data->at_rate = (int16_t)val;
		break;
	default:
		LOG_ERR("Register 0x%x is not writable", reg);
		return -EIO;
	}

	return 0;
}

static int emul_bq4xz50_read(const struct emul *target, int reg, uint8_t *buf, size_t len)
{
	uint32_t val;
	int rc;

	switch (reg) {
	case BQ4XZ50_MANUFACTURERNAME:
	case BQ4XZ50_DEVICENAME:
	case BQ4XZ50_DEVICECHEMISTRY:
		return emul_bq4xz50_buffer_read(target, reg, buf, len);
	case BQ4XZ50_MANUFACTURERDATA:
		return emul_bq4xz50_manufacturer_data_read(target, buf, len);
	default:
		break;
	}

	if (len > sizeof(val)) {
		LOG_ERR("Read of %zu bytes from reg 0x%x is too wide", len, reg);
		return -EIO;
	}

	rc = emul_bq4xz50_reg_read(target, reg, &val);
	if (rc) {
		return rc;
	}

	/* Return only as many bytes as the host asked for, little endian. */
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(val >> (8U * i));
	}

	return 0;
}

/*
 * ManufacturerBlockAccess writes arrive as an SMBus block write: the 0x44 command, a length byte,
 * the subcommand and any payload. Only the framing is checked, since none of the subcommands the
 * driver issues have an observable effect on the emulated register file.
 */
static int emul_bq4xz50_mac_write(const struct emul *target, struct i2c_msg *msgs, int num_msgs)
{
	size_t payload_len = (num_msgs == 4) ? msgs[3].len : 0U;
	uint8_t block_len;

	if (msgs[1].len != 1 || msgs[2].len != BQ4XZ50_EMUL_MAC_CMD_LEN) {
		LOG_ERR("Malformed manufacturer block access write");
		return -EIO;
	}

	block_len = msgs[1].buf[0];
	if (block_len != BQ4XZ50_EMUL_MAC_CMD_LEN + payload_len) {
		LOG_ERR("Unexpected block length %u", block_len);
		return -EIO;
	}

	LOG_DBG("Manufacturer block access command 0x%04x", sys_get_le16(msgs[2].buf));

	return 0;
}

static int bq4xz50_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	const struct bq4xz50_emul_cfg *cfg = target->cfg;
	int reg;

	__ASSERT_NO_MSG(msgs && num_msgs);

	if (addr != cfg->i2c_addr) {
		LOG_ERR("I2C address (0x%2x) is not supported.", addr);
		return -EIO;
	}

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("Transfer must start with a register address write");
		return -EIO;
	}
	if (msgs[0].len != 1) {
		LOG_ERR("Unexpected addr length %d", msgs[0].len);
		return -EIO;
	}
	reg = msgs[0].buf[0];

	switch (num_msgs) {
	case 2:
		if (msgs[1].flags & I2C_MSG_READ) {
			return emul_bq4xz50_read(target, reg, msgs[1].buf, msgs[1].len);
		}
		if (msgs[1].len != sizeof(uint16_t)) {
			LOG_ERR("Unexpected write length %d for reg 0x%x", msgs[1].len, reg);
			return -EIO;
		}
		return emul_bq4xz50_reg_write(target, reg, sys_get_le16(msgs[1].buf));
	case 3:
	case 4:
		if (reg != BQ4XZ50_MANUFACTURERBLOCKACCESS) {
			LOG_ERR("Unexpected %d-message transfer to reg 0x%x", num_msgs, reg);
			return -EIO;
		}
		return emul_bq4xz50_mac_write(target, msgs, num_msgs);
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
}

static const struct i2c_emul_api bq4xz50_emul_api_i2c = {
	.transfer = bq4xz50_emul_transfer_i2c,
};

/**
 * Set up a new emulator (I2C)
 *
 * @param target Emulation information
 * @param parent Device to emulate
 * @return 0 indicating success (always)
 */
static int emul_bq4xz50_init(const struct emul *target, const struct device *parent)
{
	struct bq4xz50_emul_data *data = target->data;

	ARG_UNUSED(parent);

	data->mfr_access = BQ4XZ50_EMUL_DFLT_MFR_ACCESS;
	data->remaining_capacity_alarm = BQ4XZ50_EMUL_DFLT_CAP_ALARM;
	data->remaining_time_alarm = BQ4XZ50_EMUL_DFLT_TIME_ALARM;
	data->battery_mode = BQ4XZ50_EMUL_DFLT_BATTERY_MODE;
	data->at_rate = BQ4XZ50_EMUL_DFLT_AT_RATE;

	return 0;
}

/*
 * Main instantiation macro. The part token keeps the per-instance symbols distinct when more
 * than one compatible is instantiated from this translation unit.
 */
#define BQ4XZ50_EMUL(n, part, dev_name)                                                            \
	static const struct bq4xz50_emul_cfg bq4xz50_emul_cfg_##part##n = {                        \
		.i2c_addr = DT_INST_REG_ADDR(n),                                                   \
		.device_name = dev_name,                                                           \
	};                                                                                         \
	static struct bq4xz50_emul_data bq4xz50_emul_data_##part##n;                               \
	EMUL_DT_INST_DEFINE(n, emul_bq4xz50_init, &bq4xz50_emul_data_##part##n,                    \
			    &bq4xz50_emul_cfg_##part##n, &bq4xz50_emul_api_i2c, NULL)

#define DT_DRV_COMPAT ti_bq40z50
DT_INST_FOREACH_STATUS_OKAY_VARGS(BQ4XZ50_EMUL, bq40z50, "bq40z50")
#undef DT_DRV_COMPAT
