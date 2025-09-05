/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq27z746

#include "bq27z746.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(BQ27Z746);

#define BQ27Z746_MAC_DATA_LEN     32
#define BQ27Z746_MAC_OVERHEAD_LEN 4 /* 2 cmd bytes, 1 length byte, 1 checksum byte */
#define BQ27Z746_MAC_COMPLETE_LEN (BQ27Z746_MAC_DATA_LEN + BQ27Z746_MAC_OVERHEAD_LEN)

struct bq27z746_config {
	struct i2c_dt_spec i2c;
};

static int bq27z746_read16(const struct device *dev, uint8_t reg, uint16_t *value)
{
	uint8_t i2c_data[2];
	const struct bq27z746_config *cfg = dev->config;
	const int status = i2c_burst_read_dt(&cfg->i2c, reg, i2c_data, sizeof(i2c_data));

	if (status < 0) {
		LOG_ERR("Unable to read register");
		return status;
	}
	*value = sys_get_le16(i2c_data);

	return 0;
}

static int bq27z746_write16(const struct device *dev, uint8_t reg, uint16_t value)
{
	uint8_t buf[3];
	const struct bq27z746_config *cfg = dev->config;

	buf[0] = reg;
	sys_put_le16(value, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int bq27z746_read_mac(const struct device *dev, uint16_t cmd, uint8_t *data, int len)
{
	if (len > BQ27Z746_MAC_DATA_LEN) {
		return -EINVAL;
	}

	uint8_t buf[BQ27Z746_MAC_COMPLETE_LEN];
	const struct bq27z746_config *cfg = dev->config;

	/* Instead of MAC, ALTMAC is used as reccommended in the datasheet */
	int ret = bq27z746_write16(dev, BQ27Z746_ALTMANUFACTURERACCESS, cmd);

	if (ret != 0) {
		return ret;
	}

	/*
	 * The data read from BQ27Z746_ALTMANUFACTURERACCESS is:
	 * 0..1: The command (for verification)
	 * 2..33: The data
	 * 34: Checksum calculated as (uint8_t)(0xFF - (sum of all command and data bytes))
	 * 35: Length including command, checksum and length (e.g. data length + 4)
	 */

	ret = i2c_burst_read_dt(&cfg->i2c, BQ27Z746_ALTMANUFACTURERACCESS, buf,
				BQ27Z746_MAC_COMPLETE_LEN);
	if (ret != 0) {
		return ret;
	}

	/* The first two bytes read is the command and is used for verification */
	const uint16_t read_cmd = sys_get_le16(buf);

	if (read_cmd != cmd) {
		LOG_ERR("Read command 0x%x != written command 0x%x", read_cmd, cmd);
		return -EIO;
	}

	const uint8_t checksum_actual = buf[34];
	uint8_t sum = 0; /* Intentionally 8 bit wide and overflowing */

	for (int i = 0; i < BQ27Z746_MAC_COMPLETE_LEN - 2; i++) {
		sum += buf[i];
	}

	const uint8_t checksum_expected = 0xFF - sum;

	if (checksum_expected != checksum_actual) {
		LOG_ERR("Checksum mismatch");
		return -EIO;
	}

	/* First byte of the user buffer is the length */
	data[0] = buf[35] - BQ27Z746_MAC_OVERHEAD_LEN;

	/* Copy only the data to the user buffer (= skipping the first two command bytes) */
	memcpy(&data[1], &buf[2], len);

	return ret;
}

static int bq27z746_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			     union fuel_gauge_prop_val *val)
{
	int rc = 0;
	uint16_t tmp_val = 0;

	/*
	 * Possibly negative values must be cast from uint16 to int16 first to
	 * then correctly end up in the wider datatypes of `prop`.
	 */

	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT:
		rc = bq27z746_read16(dev, BQ27Z746_AVERAGECURRENT, &tmp_val);
		val->avg_current = (int16_t)tmp_val * 1000;
		break;
	case FUEL_GAUGE_CYCLE_COUNT:
		rc = bq27z746_read16(dev, BQ27Z746_CYCLECOUNT, &tmp_val);
		val->cycle_count = tmp_val * 100;
		break;
	case FUEL_GAUGE_CURRENT:
		rc = bq27z746_read16(dev, BQ27Z746_CURRENT, &tmp_val);
		val->current = (int16_t)tmp_val * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		rc = bq27z746_read16(dev, BQ27Z746_FULLCHARGECAPACITY, &tmp_val);
		val->full_charge_capacity = tmp_val * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		rc = bq27z746_read16(dev, BQ27Z746_REMAININGCAPACITY, &tmp_val);
		val->remaining_capacity = tmp_val * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		rc = bq27z746_read16(dev, BQ27Z746_AVERAGETIMETOEMPTY, &tmp_val);
		val->runtime_to_empty = tmp_val;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		rc = bq27z746_read16(dev, BQ27Z746_AVERAGETIMETOFULL, &tmp_val);
		val->runtime_to_full = tmp_val;
		break;
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = bq27z746_read16(dev, BQ27Z746_MANUFACTURERACCESS, &tmp_val);
		val->sbs_mfr_access_word = tmp_val;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = bq27z746_read16(dev, BQ27Z746_RELATIVESTATEOFCHARGE, &tmp_val);
		val->relative_state_of_charge = tmp_val;
		break;
	case FUEL_GAUGE_TEMPERATURE:
		rc = bq27z746_read16(dev, BQ27Z746_TEMPERATURE, &tmp_val);
		val->temperature = tmp_val;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = bq27z746_read16(dev, BQ27Z746_VOLTAGE, &tmp_val);
		val->voltage = tmp_val * 1000;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = bq27z746_read16(dev, BQ27Z746_ATRATE, &tmp_val);
		val->sbs_at_rate = (int16_t)tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
		rc = bq27z746_read16(dev, BQ27Z746_ATRATETIMETOEMPTY, &tmp_val);
		val->sbs_at_rate_time_to_empty = tmp_val;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE:
		rc = bq27z746_read16(dev, BQ27Z746_CHARGINGVOLTAGE, &tmp_val);
		val->chg_voltage = tmp_val * 1000;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT:
		rc = bq27z746_read16(dev, BQ27Z746_CHARGINGCURRENT, &tmp_val);
		val->chg_current = tmp_val * 1000;
		break;
	case FUEL_GAUGE_STATUS:
		rc = bq27z746_read16(dev, BQ27Z746_BATTERYSTATUS, &tmp_val);
		val->fg_status = tmp_val;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		rc = bq27z746_read16(dev, BQ27Z746_DESIGNCAPACITY, &tmp_val);
		val->design_cap = tmp_val;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int bq27z746_get_buffer_prop(const struct device *dev, fuel_gauge_prop_t property_type,
				    void *dst, size_t dst_len)
{
	int rc = 0;

	switch (property_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		if (dst_len == sizeof(struct sbs_gauge_manufacturer_name)) {
			rc = bq27z746_read_mac(dev, BQ27Z746_MAC_CMD_MANUFACTURER_NAME,
					       (uint8_t *)dst, dst_len - 1);
		} else {
			rc = -EINVAL;
		}
		break;
	case FUEL_GAUGE_DEVICE_NAME:
		if (dst_len == sizeof(struct sbs_gauge_device_name)) {
			rc = bq27z746_read_mac(dev, BQ27Z746_MAC_CMD_DEVICE_NAME, (uint8_t *)dst,
					       dst_len - 1);
		} else {
			rc = -EINVAL;
		}
		break;
	case FUEL_GAUGE_DEVICE_CHEMISTRY:
		if (dst_len == sizeof(struct sbs_gauge_device_chemistry)) {
			rc = bq27z746_read_mac(dev, BQ27Z746_MAC_CMD_DEVICE_CHEM, (uint8_t *)dst,
					       dst_len - 1);
		} else {
			rc = -EINVAL;
		}
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int bq27z746_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			     union fuel_gauge_prop_val val)
{
	int rc = 0;
	uint16_t tmp_val = 0;

	switch (prop) {
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = bq27z746_write16(dev, BQ27Z746_MANUFACTURERACCESS, val.sbs_mfr_access_word);
		val.sbs_mfr_access_word = tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = bq27z746_write16(dev, BQ27Z746_ATRATE, val.sbs_at_rate);
		val.sbs_at_rate = tmp_val;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int bq27z746_init(const struct device *dev)
{
	const struct bq27z746_config *cfg;

	cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, bq27z746_driver_api) = {
	.get_property = &bq27z746_get_prop,
	.set_property = &bq27z746_set_prop,
	.get_buffer_property = &bq27z746_get_buffer_prop,
};

#define BQ27Z746_INIT(index)                                                                       \
                                                                                                   \
	static const struct bq27z746_config bq27z746_config_##index = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &bq27z746_init, NULL, NULL, &bq27z746_config_##index,         \
			      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY, &bq27z746_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ27Z746_INIT)
