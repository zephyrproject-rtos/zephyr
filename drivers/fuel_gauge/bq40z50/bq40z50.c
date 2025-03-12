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

/* MAC commands */
#define BQ40Z50_MAC_CMD_DEVICE_TYPE  0X0001
#define BQ40Z50_MAC_CMD_FIRMWARE_VER 0X0002
#define BQ40Z50_MAC_CMD_SHUTDOWNMODE 0X0010
#define BQ40Z50_MAC_CMD_SLEEPMODE    0X0011
#define BQ40Z50_MAC_CMD_GAUGING      0X0021

/* first byte is the length of the data received from the block access and the next two bytes are
 * the cmd.
 */
#define BQ40Z50_MAC_META_DATA_LEN    3
#define BQ40Z50_FIRMWARE_VERSION_LEN 11

#define BQ40Z50_LEN_BYTE      1
#define BQ40Z50_LEN_HALF_WORD 2
#define BQ40Z50_LEN_WORD      4

/* Bit 14 of Operational status (0x54) is XCHG (charging disabled) */
#define BQ40Z50_OPERATION_STATUS_XCHG_BIT 14

struct bq40z50_config {
	struct i2c_dt_spec i2c;
};

struct bq40z50_data {
	uint8_t major_version;
	uint8_t minor_version;
};

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

static int bq40z50_i2c_write_mfr_blk_access(const struct device *dev, uint16_t cmd, uint8_t *value,
					    size_t len)
{
	const struct bq40z50_config *cfg = dev->config;
	/* Manufacturer Block Access (0x44) is standard for bq40zxy family.*/
	uint8_t mac_cmd = BQ40Z50_MANUFACTURERBLOCKACCESS;
	struct i2c_msg msg[4];
	uint8_t total_len = sizeof(cmd) + len;

	/* As per Datasheet, use SMBus block protocol to write/read using
	 * Manufacturer Block Access (0x44).
	 * SMBus block write requires writing command followed by number of
	 * bytes that will follow and then actual bytes to write.
	 */
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

	/* For battery cutoff, we only need to send the command. */
	int ret = i2c_transfer_dt(&cfg->i2c, msg, ((value == NULL) ? 3 : 4));

	if (ret) {
		LOG_ERR("i2c_burst_write_dt returned %d", ret);
	}
	return ret;
}

static int bq40z50_battery_cutoff(const struct device *dev)
{
	int ret = 0;

	/*
	 * As per TRM (RevB.) (section 14.1.10), in order to enter shutdown mode
	 * we need to send BQ40Z50_MAC_CMD_SHUTDOWNMODE command twice irrespective of access mode.
	 * The first command will arm the shutdown sequence and second command will confirm it.
	 */

	ret = bq40z50_i2c_write_mfr_blk_access(dev, (uint16_t)BQ40Z50_MAC_CMD_SHUTDOWNMODE, NULL,
					       0);

	ret |= bq40z50_i2c_write_mfr_blk_access(dev, (uint16_t)BQ40Z50_MAC_CMD_SHUTDOWNMODE, NULL,
						0);
	return ret;
}

static int bq40z50_get_buffer_prop(const struct device *dev, fuel_gauge_prop_t prop_type, void *dst,
				   size_t dst_len)
{
	int ret = 0;

	if (dst == NULL) {
		return -EINVAL;
	}

	switch (prop_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		if (dst_len == sizeof(struct sbs_gauge_manufacturer_name)) {
			ret = bq40z50_i2c_read(dev, BQ40Z50_MANUFACTURERNAME, dst, dst_len);
			struct sbs_gauge_manufacturer_name *mfgname =
				(struct sbs_gauge_manufacturer_name *)dst;

			mfgname->manufacturer_name[mfgname->manufacturer_name_length] = '\0';
		} else {
			ret = -EINVAL;
		}
		break;

	case FUEL_GAUGE_DEVICE_NAME:
		if (dst_len == sizeof(struct sbs_gauge_device_name)) {
			ret = bq40z50_i2c_read(dev, BQ40Z50_DEVICENAME, dst, dst_len);
			struct sbs_gauge_device_name *devname = (struct sbs_gauge_device_name *)dst;

			devname->device_name[devname->device_name_length] = '\0';
		} else {
			ret = -EINVAL;
		}
		break;

	case FUEL_GAUGE_DEVICE_CHEMISTRY:
		if (dst_len == sizeof(struct sbs_gauge_device_chemistry)) {
			ret = bq40z50_i2c_read(dev, BQ40Z50_DEVICECHEMISTRY, dst, dst_len);
			struct sbs_gauge_device_chemistry *devchem =
				(struct sbs_gauge_device_chemistry *)dst;

			devchem->device_chemistry[devchem->device_chemistry_length] = '\0';
		} else {
			ret = -EINVAL;
		}
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
		/* convert mA to uA */
		val->avg_current = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CURRENT:
		ret = bq40z50_i2c_read(dev, BQ40Z50_CURRENT, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		/* convert mA to uA */
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
		/* convert mAh to uAh */
		val->full_charge_capacity = tmp_val * 1000;
		break;

	case FUEL_GAUGE_REMAINING_CAPACITY:
		ret = bq40z50_i2c_read(dev, BQ40Z50_REMAININGCAPACITY, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		/* convert mAh to uAh */
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
		/* change mV to uV */
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
		/* change mA to uA */
		val->chg_current = (int16_t)tmp_val * 1000;
		break;

	case FUEL_GAUGE_CHARGE_VOLTAGE:
		ret = bq40z50_i2c_read(dev, BQ40Z50_CHARGINGVOLTAGE, (uint8_t *)&tmp_val,
				       BQ40Z50_LEN_HALF_WORD);
		/* change mV to uV */
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
		/* return in mAh unit */
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

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int bq40z50_init(const struct device *dev)
{
	const struct bq40z50_config *cfg = (const struct bq40z50_config *)dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
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
