/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq40z50

#include "bq40z50.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>

LOG_MODULE_REGISTER(BQ40Z50);

static int bq40z50_i2c_read(const struct device *dev, uint8_t reg_addr, uint8_t *value, size_t len)
{
	const struct bq40z50_config *cfg = dev->config;
	int ret = i2c_burst_read_dt(&cfg->i2c, reg_addr, value, len);
	if (ret) {
		LOG_ERR("i2c_burst_read_dt failed for address %d: %d", reg_addr, ret);
	}
	return ret;
}

static int bq40z50_i2c_write(const struct device *dev, uint8_t reg_addr, uint8_t *value, size_t len)
{
	const struct bq40z50_config *cfg = dev->config;
	int ret = i2c_burst_write_dt(&cfg->i2c, reg_addr, value, len);
	if (ret) {
		LOG_ERR("i2c_burst_write_dt failed for address %d: %d", reg_addr, ret);
	}
	return ret;
}

static int bq40z50_i2c_write_mfr_acces(const struct device *dev, uint8_t mac_cmd, uint16_t cmd,
				       uint8_t *value, size_t len)
{
	const struct bq40z50_config *cfg = dev->config;
	struct i2c_msg msg[4];
	uint8_t total_len = sizeof(cmd) + len;

	msg[0].buf = &mac_cmd;
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = &total_len;
	msg[1].len = 1U;
	msg[1].flags = I2C_MSG_WRITE;

	msg[2].buf = (uint8_t *)&cmd;
	msg[2].len = sizeof(cmd);
	msg[2].flags = I2C_MSG_WRITE;

	msg[3].buf = value;
	msg[3].len = len;
	msg[3].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	// For battery cutoff, we only need to send the command.
	int ret = i2c_transfer_dt(&cfg->i2c, msg, ((value == NULL) ? 3 : 4));
	if (ret) {
		LOG_ERR("i2c_burst_write_dt returned %d", ret);
	}
	return ret;
}

static int bq40z50_i2c_read_mfr_blk_acces(const struct device *dev, uint16_t addr, uint8_t *value,
					  size_t len)
{
	const struct bq40z50_config *cfg = dev->config;
	uint8_t manufacturer_blk_access = BQ40Z50_MANUFACTURERBLOCKACCESS;
	uint8_t rd_len = len + BQ40Z50_MAC_META_DATA_LEN;
	uint8_t wr_len = sizeof(addr);
	uint8_t recv_msg[rd_len];
	struct i2c_msg msg[3];
	msg[0].buf = &manufacturer_blk_access;
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)&wr_len;
	msg[1].len = 1;
	msg[1].flags = I2C_MSG_WRITE;

	msg[2].buf = (uint8_t *)&addr;
	msg[2].len = sizeof(addr);
	msg[2].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	int ret = i2c_transfer_dt(&cfg->i2c, msg, 3);
	if (ret) {
		LOG_ERR("i2c_burst_write_dt returned: %d", ret);
	}

	ret = i2c_burst_read_dt(&cfg->i2c, BQ40Z50_MANUFACTURERBLOCKACCESS, recv_msg, rd_len);
	if (ret) {
		LOG_ERR("i2c_burst_read_dt returned: %d", ret);
	}

	uint16_t recv_addr = sys_get_le16(&recv_msg[1]);

	if (recv_addr != addr || (recv_msg[0] - sizeof(addr)) != len) {
		LOG_ERR("manufacture block access read failed: send cmd: %d\t recv cmd: %d\t "
			"recv_len: %d\t requested_len: %d",
			addr, recv_addr, recv_msg[0], len);
	}

	memcpy(value, &recv_msg[3], len);

	return ret;
}

static int bq40z50_battery_cutoff(const struct device *dev)
{
	int ret = 0;
	ret = bq40z50_i2c_write_mfr_acces(dev, BQ40Z50_MANUFACTURERACCESS,
					  (uint16_t)BQ40Z50_MAC_CMD_SHUTDOWNMODE, NULL, 0);

	ret |= bq40z50_i2c_write_mfr_acces(dev, BQ40Z50_MANUFACTURERACCESS,
					   (uint16_t)BQ40Z50_MAC_CMD_SHUTDOWNMODE, NULL, 0);
	return ret;
}

static int bq40z50_get_buffer_prop(const struct device *dev, fuel_gauge_prop_t prop_type, void *dst,
				   size_t dst_len)
{
	int ret = 0;
	(void)memset(dst, 0, dst_len);
	switch (prop_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		ret = bq40z50_i2c_read(dev, BQ40Z50_MANUFACTURERNAME, dst, dst_len);
		struct sbs_gauge_manufacturer_name *mfgname =
			(struct sbs_gauge_manufacturer_name *)dst;
		mfgname->manufacturer_name[mfgname->manufacturer_name_length] = '\0';
		break;

	case FUEL_GAUGE_DEVICE_NAME:
		ret = bq40z50_i2c_read(dev, BQ40Z50_DEVICENAME, dst, dst_len);
		struct sbs_gauge_device_name *devname = (struct sbs_gauge_device_name *)dst;
		devname->device_name[devname->device_name_length] = '\0';
		break;

	case FUEL_GAUGE_DEVICE_CHEMISTRY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_DEVICECHEMISTRY, dst, dst_len);
		struct sbs_gauge_device_chemistry *devchem =
			(struct sbs_gauge_device_chemistry *)dst;
		devchem->device_chemistry[devchem->device_chemistry_length] = '\0';
		break;

	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

static int bq40z50_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	int ret = 0;
	switch (prop) {
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		ret = bq40z50_i2c_write(dev, BQ40Z50_REMAININGCAPACITYALARM,
					(uint8_t *)&val.sbs_remaining_capacity_alarm,
					sizeof(val.sbs_remaining_capacity_alarm));
		break;

	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		ret = bq40z50_i2c_write(dev, BQ40Z50_REMAININGTIMEALARM,
					(uint8_t *)&val.sbs_remaining_time_alarm,
					sizeof(val.sbs_remaining_time_alarm));
		break;

	case FUEL_GAUGE_SBS_MODE:
		ret = bq40z50_i2c_write(dev, BQ40Z50_BATTERYMODE, (uint8_t *)&val.sbs_mode,
					sizeof(val.sbs_mode));
		break;

	case FUEL_GAUGE_SBS_ATRATE:
		ret = bq40z50_i2c_write(dev, BQ40Z50_ATRATE, (uint8_t *)&val.sbs_at_rate,
					sizeof(val.sbs_at_rate));
		break;

	case FUEL_GAUGE_SBS_MFR_ACCESS:
		ret = bq40z50_i2c_write(dev, BQ40Z50_MANUFACTURERACCESS,
					(uint8_t *)&val.sbs_mfr_access_word,
					sizeof(val.sbs_mfr_access_word));
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int bq40z50_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	int ret = 0;
	uint16_t tmp_val = 0;
	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT:
		ret = bq40z50_i2c_read(dev, BQ40Z50_AVERAGECURRENT, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// convert mA to uA
		val->avg_current = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CURRENT:
		ret = bq40z50_i2c_read(dev, BQ40Z50_CURRENT, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// convert mA to uA
		val->current = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CHARGE_CUTOFF:
		ret = bq40z50_i2c_read(dev, BQ40Z50_MANUFACTURERACCESS, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->cutoff = IS_BIT_SET(tmp_val, BQ40Z50_OPERATION_STATUS_XCHG_BIT);
		break;

	case FUEL_GAUGE_CYCLE_COUNT:
		ret = bq40z50_i2c_read(dev, BQ40Z50_CYCLECOUNT, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->cycle_count = tmp_val;
		break;

	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_FULLCHARGECAPACITY, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// convert mAh to uAh
		val->full_charge_capacity = tmp_val * 1000;
		break;

	case FUEL_GAUGE_REMAINING_CAPACITY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_REMAININGCAPACITY, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// convert mAh to uAh
		val->remaining_capacity = tmp_val * 1000;
		break;

	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_RUNTIMETOEMPTY, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->runtime_to_empty = tmp_val;
		break;

	case FUEL_GAUGE_SBS_MFR_ACCESS:
		ret = bq40z50_i2c_read(dev, BQ40Z50_MANUFACTURERACCESS, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_mfr_access_word = tmp_val;
		break;

	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_ABSOLUTESTATEOFCHARGE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_BYTE);
		val->absolute_state_of_charge = tmp_val;
		break;

	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_RELATIVESTATEOFCHARGE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_BYTE);
		val->relative_state_of_charge = tmp_val;
		break;

	case FUEL_GAUGE_TEMPERATURE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_TEMPERATURE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->temperature = tmp_val;
		break;

	case FUEL_GAUGE_VOLTAGE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_VOLTAGE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// change mV to uV
		val->voltage = tmp_val * 1000;
		break;

	case FUEL_GAUGE_SBS_MODE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_BATTERYMODE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_mode = tmp_val;
		break;

	case FUEL_GAUGE_CHARGE_CURRENT:
		ret = bq40z50_i2c_read(dev, BQ40Z50_CHARGINGCURRENT, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// change mA to uA
		val->chg_current = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CHARGE_VOLTAGE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_CHARGINGVOLTAGE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// change mV to uV
		val->chg_voltage = tmp_val * 1000;
		break;

	case FUEL_GAUGE_STATUS:
		ret = bq40z50_i2c_read(dev, BQ40Z50_BATTERYSTATUS, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->fg_status = tmp_val;
		break;

	case FUEL_GAUGE_DESIGN_CAPACITY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_DESIGNCAPACITY, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		// return in mAh unit
		val->design_cap = tmp_val;
		break;

	case FUEL_GAUGE_DESIGN_VOLTAGE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_DESIGNVOLTAGE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->design_volt = tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_ATRATE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_at_rate = (int16_t)tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL:
		ret = bq40z50_i2c_read(dev, BQ40Z50_ATRATETIMETOFULL, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_at_rate_time_to_full = tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_ATRATETIMETOEMPTY, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_at_rate_time_to_empty = tmp_val;
		break;

	case FUEL_GAUGE_SBS_ATRATE_OK:
		ret = bq40z50_i2c_read(dev, BQ40Z50_ATRATEOK, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_at_rate_ok = tmp_val;
		break;

	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		ret = bq40z50_i2c_read(dev, BQ40Z50_REMAININGCAPACITYALARM, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_remaining_capacity_alarm = tmp_val;
		break;

	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		ret = bq40z50_i2c_read(dev, BQ40Z50_REMAININGTIMEALARM, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		val->sbs_remaining_time_alarm = tmp_val;
		break;

	case FUEL_GAUGE_CONNECT_STATE:
	case FUEL_GAUGE_PRESENT_STATE:
	case FUEL_GAUGE_RUNTIME_TO_FULL:
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static __maybe_unused int bq40z50_get_metadata(const struct device *dev)
{
	struct bq40z50_data *data = (struct bq40z50_data *)(dev->data);
	uint8_t firmware_ver[BQ40Z50_FIRMWARE_VERSION_LEN];
	int ret = bq40z50_i2c_read_mfr_blk_acces(dev, BQ40Z50_MAC_CMD_FIRMWARE_VER,
						 (uint8_t *)firmware_ver,
						 BQ40Z50_FIRMWARE_VERSION_LEN);

	data->major_version = firmware_ver[2];
	data->minor_version = firmware_ver[3];
	return ret;
}

static int bq40z50_init(const struct device *dev)
{
	const struct bq40z50_config *cfg = (const struct bq40z50_config *)dev->config;
	int ret = 0;
	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	// do not get firmware version in emulator.
#if !(defined(CONFIG_EMUL_BQ40Z50))
	ret = bq40z50_get_metadata(dev);
#endif

	return ret;
}

static DEVICE_API(fuel_gauge,
		  bq40z50_driver_api) = {.get_property = &bq40z50_get_prop,
					 .get_buffer_property = &bq40z50_get_buffer_prop,
					 .set_property = &bq40z50_set_prop,
					 .battery_cutoff = &bq40z50_battery_cutoff};

#define BQ40Z50_INIT(inst)                                                                         \
	struct bq40z50_config bq40z50_config_##inst = {.i2c = I2C_DT_SPEC_INST_GET(inst)};         \
	struct bq40z50_data bq40z50_data_##inst = {.major_version = 0x00, .minor_version = 0x00};  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &bq40z50_init, NULL, &bq40z50_data_##inst,                     \
			      &bq40z50_config_##inst, POST_KERNEL,                                 \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &bq40z50_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ40Z50_INIT)
