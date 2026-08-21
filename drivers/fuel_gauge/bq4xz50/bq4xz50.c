/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implementation shared by the bq40z50 and bq41z50 fuel gauges. Instances are created by the
 * per-compatible translation units.
 */

#include "bq4xz50.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(BQ4XZ50, CONFIG_FUEL_GAUGE_LOG_LEVEL);

/* ManufacturerBlockAccess (0x44) subcommands */
#define BQ4XZ50_MAC_CMD_DEVICE_TYPE  0x0001
#define BQ4XZ50_MAC_CMD_FIRMWARE_VER 0x0002
#define BQ4XZ50_MAC_CMD_SHUTDOWNMODE 0x0010
#define BQ4XZ50_MAC_CMD_SLEEPMODE    0x0011
#define BQ4XZ50_MAC_CMD_GAUGING      0x0021

#define BQ4XZ50_MAC_BLOCK_MAX_LEN 32U

/* ManufacturerAccess (0x00) subcommand; its result is read back from ManufacturerData (0x23). */
#define BQ4XZ50_MAC_SUBCMD_OPERATION_STATUS 0x0054

/* BatteryMode (0x03) CAPACITY_MODE: set means capacity registers report 10 mWh instead of mAh */
#define BQ4XZ50_BATTERYMODE_CAPM_BIT 15

/* OperationStatus XCHG: set means charging is disabled */
#define BQ4XZ50_OPERATIONSTATUS_XCHG_BIT 14

static int bq4xz50_i2c_read(const struct device *dev, uint8_t reg_addr, uint8_t *value, size_t len)
{
	const struct bq4xz50_config *cfg = dev->config;
	int ret = i2c_burst_read_dt(&cfg->i2c, reg_addr, value, len);

	if (ret) {
		LOG_ERR("i2c_burst_read_dt failed for address 0x%02x: %d", reg_addr, ret);
	}
	return ret;
}

static int bq4xz50_i2c_write(const struct device *dev, uint8_t reg_addr, uint8_t *value, size_t len)
{
	const struct bq4xz50_config *cfg = dev->config;
	int ret = i2c_burst_write_dt(&cfg->i2c, reg_addr, value, len);

	if (ret) {
		LOG_ERR("i2c_burst_write_dt failed for address 0x%02x: %d", reg_addr, ret);
	}
	return ret;
}

static int bq4xz50_read_u8(const struct device *dev, uint8_t reg_addr, uint8_t *value)
{
	return bq4xz50_i2c_read(dev, reg_addr, value, sizeof(*value));
}

static int bq4xz50_read_u16(const struct device *dev, uint8_t reg_addr, uint16_t *value)
{
	uint8_t buf[sizeof(uint16_t)];
	int ret = bq4xz50_i2c_read(dev, reg_addr, buf, sizeof(buf));

	if (ret == 0) {
		*value = sys_get_le16(buf);
	}
	return ret;
}

static int bq4xz50_write_u16(const struct device *dev, uint8_t reg_addr, uint16_t value)
{
	uint8_t buf[sizeof(uint16_t)];

	sys_put_le16(value, buf);
	return bq4xz50_i2c_write(dev, reg_addr, buf, sizeof(buf));
}

/*
 * Capacity is reported in either mAh or 10 mWh depending on BatteryMode CAPACITY_MODE, but the API
 * expresses RemainingCapacity and FullChargeCapacity in uAh. CAPACITY_MODE can change behind the
 * driver's back, so re-read it every time to ensure we have the correct state.
 */
static int bq4xz50_read_capacity_uah(const struct device *dev, uint8_t reg_addr, uint32_t *value)
{
	uint16_t battery_mode;
	uint16_t tmp_val;
	int ret;

	ret = bq4xz50_read_u16(dev, BQ4XZ50_BATTERYMODE, &battery_mode);
	if (ret) {
		return ret;
	}

	/* Reporting a 10 mWh reading as uAh would be silently wrong, so refuse it instead. */
	if (IS_BIT_SET(battery_mode, BQ4XZ50_BATTERYMODE_CAPM_BIT)) {
		return -ENOTSUP;
	}

	ret = bq4xz50_read_u16(dev, reg_addr, &tmp_val);
	if (ret == 0) {
		/* convert mAh to uAh */
		*value = tmp_val * 1000U;
	}
	return ret;
}

static int bq4xz50_write_mac(const struct device *dev, uint16_t cmd, uint8_t *data, size_t data_len)
{
	const struct bq4xz50_config *cfg = dev->config;
	/* Manufacturer Block Access (0x44) is standard for the bq4xz50 family. */
	uint8_t mac_cmd = BQ4XZ50_MANUFACTURERBLOCKACCESS;
	uint8_t cmd_le[sizeof(cmd)];
	struct i2c_msg msg[4];
	uint8_t block_len = sizeof(cmd) + data_len;
	int ret;

	/*
	 * Data pointer must not be NULL if data_len is greater than 0, data_len must fit one SMBus
	 * block.
	 */
	if ((data == NULL && data_len != 0U) || (data != NULL && data_len == 0U) ||
	    data_len > BQ4XZ50_MAC_BLOCK_MAX_LEN - sizeof(cmd)) {
		return -EINVAL;
	}

	sys_put_le16(cmd, cmd_le);

	/* As per Datasheet, use SMBus block protocol to write/read using
	 * Manufacturer Block Access (0x44).
	 * SMBus block write requires writing command followed by number of
	 * bytes that will follow and then actual bytes to write.
	 */
	msg[0].buf = &mac_cmd;
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = &block_len;
	msg[1].len = 1U;
	msg[1].flags = I2C_MSG_WRITE;

	msg[2].buf = cmd_le;
	msg[2].len = sizeof(cmd_le);
	msg[2].flags = I2C_MSG_WRITE;

	/* Commands without a payload, such as battery cutoff, end after the subcommand. */
	if (data == NULL) {
		msg[2].flags |= I2C_MSG_STOP;
	} else {
		msg[3].buf = data;
		msg[3].len = data_len;
		msg[3].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
	}

	ret = i2c_transfer_dt(&cfg->i2c, msg, ((data == NULL) ? 3 : 4));
	if (ret) {
		LOG_ERR("i2c_transfer_dt returned %d", ret);
	}
	return ret;
}

static int bq4xz50_read_mac(const struct device *dev, uint16_t cmd, uint8_t *data, size_t data_len)
{
	uint8_t buf[BQ4XZ50_MAC_BLOCK_MAX_LEN + 1U];
	int ret;

	if (data == NULL || data_len == 0U || data_len > BQ4XZ50_MAC_BLOCK_MAX_LEN) {
		return -EINVAL;
	}

	ret = bq4xz50_write_u16(dev, BQ4XZ50_MANUFACTURERACCESS, cmd);
	if (ret == 0) {
		ret = bq4xz50_i2c_read(dev, BQ4XZ50_MANUFACTURERDATA, buf, data_len + 1U);
	}
	if (ret == 0 && buf[0] != data_len) {
		LOG_ERR("Unexpected MAC 0x%04x byte count: %u (expected %zu)", cmd, buf[0],
			data_len);
		ret = -EIO;
	}
	if (ret == 0) {
		memcpy(data, &buf[1], data_len);
	}

	return ret;
}

static int bq4xz50_read_operation_status(const struct device *dev, uint32_t *status)
{
	uint8_t data[sizeof(*status)];
	int ret;

	ret = bq4xz50_read_mac(dev, BQ4XZ50_MAC_SUBCMD_OPERATION_STATUS, data, sizeof(data));
	if (ret == 0) {
		*status = sys_get_le32(data);
	}

	return ret;
}

static int bq4xz50_battery_cutoff(const struct device *dev)
{
	int ret;

	/*
	 * As per TRM, in order to enter shutdown mode we need to send
	 * BQ4XZ50_MAC_CMD_SHUTDOWNMODE twice irrespective of access mode. The first command arms
	 * the shutdown sequence and the second confirms it.
	 */
	ret = bq4xz50_write_mac(dev, BQ4XZ50_MAC_CMD_SHUTDOWNMODE, NULL, 0U);
	if (ret) {
		return ret;
	}

	return bq4xz50_write_mac(dev, BQ4XZ50_MAC_CMD_SHUTDOWNMODE, NULL, 0U);
}

/*
 * The SBS string registers use the block read protocol: a count byte followed by the payload. The
 * API structures are laid out the same way and are length-delimited, so the payload is never NULL
 * terminated and callers must honour the length field.
 */
static int bq4xz50_read_block_str(const struct device *dev, uint8_t reg_addr, void *dst,
				  size_t dst_len)
{
	uint8_t *buf = dst;
	int ret = bq4xz50_i2c_read(dev, reg_addr, buf, dst_len);

	if (ret == 0 && buf[0] > dst_len - 1U) {
		LOG_ERR("Block length %u at 0x%02x exceeds %zu bytes", buf[0], reg_addr,
			dst_len - 1U);
		ret = -EIO;
	}

	return ret;
}

static int bq4xz50_get_buffer_prop(const struct device *dev, fuel_gauge_prop_t prop_type, void *dst,
				   size_t dst_len)
{
	int ret;

	if (dst == NULL) {
		return -EINVAL;
	}

	switch (prop_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		if (dst_len != sizeof(struct sbs_gauge_manufacturer_name)) {
			return -EINVAL;
		}
		ret = bq4xz50_read_block_str(dev, BQ4XZ50_MANUFACTURERNAME, dst, dst_len);
		break;

	case FUEL_GAUGE_DEVICE_NAME:
		if (dst_len != sizeof(struct sbs_gauge_device_name)) {
			return -EINVAL;
		}
		ret = bq4xz50_read_block_str(dev, BQ4XZ50_DEVICENAME, dst, dst_len);
		break;

	case FUEL_GAUGE_DEVICE_CHEMISTRY:
		if (dst_len != sizeof(struct sbs_gauge_device_chemistry)) {
			return -EINVAL;
		}
		ret = bq4xz50_read_block_str(dev, BQ4XZ50_DEVICECHEMISTRY, dst, dst_len);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int bq4xz50_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	int ret;

	switch (prop) {
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		ret = bq4xz50_write_u16(dev, BQ4XZ50_REMAININGCAPACITYALARM,
					val.sbs_remaining_capacity_alarm);
		break;

	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS:
		ret = bq4xz50_write_u16(dev, BQ4XZ50_REMAININGTIMEALARM,
					val.sbs_remaining_time_alarm_mins);
		break;

	case FUEL_GAUGE_SBS_MODE:
		ret = bq4xz50_write_u16(dev, BQ4XZ50_BATTERYMODE, val.sbs_mode);
		break;

	case FUEL_GAUGE_SBS_ATRATE:
		ret = bq4xz50_write_u16(dev, BQ4XZ50_ATRATE, (uint16_t)val.sbs_at_rate);
		break;

	case FUEL_GAUGE_SBS_MFR_ACCESS:
		ret = bq4xz50_write_u16(dev, BQ4XZ50_MANUFACTURERACCESS, val.sbs_mfr_access_word);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int bq4xz50_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	uint32_t tmp_u32 = 0;
	uint16_t tmp_val = 0;
	uint8_t tmp_u8 = 0;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT_UA:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_AVERAGECURRENT, &tmp_val);
		/* convert mA to uA */
		val->avg_current_ua = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CURRENT_UA:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_CURRENT, &tmp_val);
		/* convert mA to uA */
		val->current_ua = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CHARGE_CUTOFF:
		ret = bq4xz50_read_operation_status(dev, &tmp_u32);
		val->cutoff = IS_BIT_SET(tmp_u32, BQ4XZ50_OPERATIONSTATUS_XCHG_BIT);
		break;

	case FUEL_GAUGE_CYCLE_COUNT:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_CYCLECOUNT, &tmp_val);
		val->cycle_count = tmp_val;
		break;

	case FUEL_GAUGE_FLAGS:
		ret = bq4xz50_read_operation_status(dev, &val->flags);
		break;

	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		ret = bq4xz50_read_capacity_uah(dev, BQ4XZ50_FULLCHARGECAPACITY,
						&val->full_charge_capacity_uah);
		break;

	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
		ret = bq4xz50_read_capacity_uah(dev, BQ4XZ50_REMAININGCAPACITY,
						&val->remaining_capacity_uah);
		break;

	case FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_RUNTIMETOEMPTY, &tmp_val);
		val->runtime_to_empty_mins = tmp_val;
		break;

	case FUEL_GAUGE_RUNTIME_TO_FULL_MINS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_AVERAGETIMETOFULL, &tmp_val);
		val->runtime_to_full_mins = tmp_val;
		break;

	case FUEL_GAUGE_SBS_MFR_ACCESS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_MANUFACTURERACCESS, &tmp_val);
		val->sbs_mfr_access_word = tmp_val;
		break;

	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT:
		ret = bq4xz50_read_u8(dev, BQ4XZ50_ABSOLUTESTATEOFCHARGE, &tmp_u8);
		val->absolute_state_of_charge_pct = tmp_u8;
		break;

	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		ret = bq4xz50_read_u8(dev, BQ4XZ50_RELATIVESTATEOFCHARGE, &tmp_u8);
		val->relative_state_of_charge_pct = tmp_u8;
		break;

	case FUEL_GAUGE_TEMPERATURE_DK:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_TEMPERATURE, &tmp_val);
		val->temperature_dk = tmp_val;
		break;

	case FUEL_GAUGE_VOLTAGE_UV:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_VOLTAGE, &tmp_val);
		/* convert mV to uV */
		val->voltage_uv = tmp_val * 1000;
		break;

	case FUEL_GAUGE_SBS_MODE:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_BATTERYMODE, &tmp_val);
		val->sbs_mode = tmp_val;
		break;

	case FUEL_GAUGE_CHARGE_CURRENT_UA:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_CHARGINGCURRENT, &tmp_val);
		/* convert mA to uA */
		val->chg_current_ua = tmp_val * 1000;
		break;

	case FUEL_GAUGE_CHARGE_VOLTAGE_UV:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_CHARGINGVOLTAGE, &tmp_val);
		/* convert mV to uV */
		val->chg_voltage_uv = tmp_val * 1000;
		break;

	case FUEL_GAUGE_STATUS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_BATTERYSTATUS, &tmp_val);
		val->fg_status = tmp_val;
		break;

	case FUEL_GAUGE_DESIGN_CAPACITY:
		/* API expresses this as mAh or 10 mWh, matching CAPACITY_MODE, so pass it on. */
		ret = bq4xz50_read_u16(dev, BQ4XZ50_DESIGNCAPACITY, &tmp_val);
		val->design_cap = tmp_val;
		break;

	case FUEL_GAUGE_DESIGN_VOLTAGE_MV:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_DESIGNVOLTAGE, &tmp_val);
		val->design_volt_mv = tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_ATRATE, &tmp_val);
		val->sbs_at_rate = (int16_t)tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL_MINS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_ATRATETIMETOFULL, &tmp_val);
		val->sbs_at_rate_time_to_full_mins = tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY_MINS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_ATRATETIMETOEMPTY, &tmp_val);
		val->sbs_at_rate_time_to_empty_mins = tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE_OK:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_ATRATEOK, &tmp_val);
		val->sbs_at_rate_ok = tmp_val;
		break;

	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_REMAININGCAPACITYALARM, &tmp_val);
		val->sbs_remaining_capacity_alarm = tmp_val;
		break;

	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS:
		ret = bq4xz50_read_u16(dev, BQ4XZ50_REMAININGTIMEALARM, &tmp_val);
		val->sbs_remaining_time_alarm_mins = tmp_val;
		break;

	case FUEL_GAUGE_STATE_OF_HEALTH:
		ret = bq4xz50_read_u8(dev, BQ4XZ50_STATEOFHEALTH, &tmp_u8);
		val->state_of_health = tmp_u8;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

int bq4xz50_init(const struct device *dev)
{
	const struct bq4xz50_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

DEVICE_API(fuel_gauge, bq4xz50_driver_api) = {.get_property = &bq4xz50_get_prop,
					      .get_buffer_property = &bq4xz50_get_buffer_prop,
					      .set_property = &bq4xz50_set_prop,
					      .battery_cutoff = &bq4xz50_battery_cutoff};
