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

static int bq27z746_get_prop(const struct device *dev, struct fuel_gauge_property *prop)
{
	int rc = 0;
	uint16_t val = 0;

	/*
	 * Possibly negative values must be cast from uint16 to int16 first to
	 * then correctly end up in the wider datatypes of `prop`.
	 */

	switch (prop->property_type) {
	case FUEL_GAUGE_AVG_CURRENT:
		rc = bq27z746_read16(dev, BQ27Z746_AVERAGECURRENT, &val);
		prop->value.avg_current = (int16_t)val * 1000;
		break;
	case FUEL_GAUGE_CYCLE_COUNT:
		rc = bq27z746_read16(dev, BQ27Z746_CYCLECOUNT, &val);
		prop->value.cycle_count = val * 100;
		break;
	case FUEL_GAUGE_CURRENT:
		rc = bq27z746_read16(dev, BQ27Z746_CURRENT, &val);
		prop->value.current = (int16_t)val * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		rc = bq27z746_read16(dev, BQ27Z746_FULLCHARGECAPACITY, &val);
		prop->value.full_charge_capacity = val * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		rc = bq27z746_read16(dev, BQ27Z746_REMAININGCAPACITY, &val);
		prop->value.remaining_capacity = val * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		rc = bq27z746_read16(dev, BQ27Z746_AVERAGETIMETOEMPTY, &val);
		prop->value.runtime_to_empty = val;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		rc = bq27z746_read16(dev, BQ27Z746_AVERAGETIMETOFULL, &val);
		prop->value.runtime_to_full = val;
		break;
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = bq27z746_read16(dev, BQ27Z746_MANUFACTURERACCESS, &val);
		prop->value.sbs_mfr_access_word = val;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = bq27z746_read16(dev, BQ27Z746_RELATIVESTATEOFCHARGE, &val);
		prop->value.relative_state_of_charge = val;
		break;
	case FUEL_GAUGE_TEMPERATURE:
		rc = bq27z746_read16(dev, BQ27Z746_TEMPERATURE, &val);
		prop->value.temperature = val;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = bq27z746_read16(dev, BQ27Z746_VOLTAGE, &val);
		prop->value.voltage = val * 1000;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = bq27z746_read16(dev, BQ27Z746_ATRATE, &val);
		prop->value.sbs_at_rate = (int16_t)val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
		rc = bq27z746_read16(dev, BQ27Z746_ATRATETIMETOEMPTY, &val);
		prop->value.sbs_at_rate_time_to_empty = val;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE:
		rc = bq27z746_read16(dev, BQ27Z746_CHARGINGVOLTAGE, &val);
		prop->value.chg_voltage = val;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT:
		rc = bq27z746_read16(dev, BQ27Z746_CHARGINGCURRENT, &val);
		prop->value.chg_current = val;
		break;
	case FUEL_GAUGE_STATUS:
		rc = bq27z746_read16(dev, BQ27Z746_BATTERYSTATUS, &val);
		prop->value.fg_status = val;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		rc = bq27z746_read16(dev, BQ27Z746_DESIGNCAPACITY, &val);
		prop->value.design_cap = val;
		break;
	default:
		rc = -ENOTSUP;
	}

	prop->status = rc;

	return rc;
}

static int bq27z746_get_buffer_prop(const struct device *dev,
				    struct fuel_gauge_buffer_property *prop, void *dst,
				    size_t dst_len)
{
	int rc = 0;

	switch (prop->property_type) {
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

	prop->status = rc;
	return rc;
}

static int bq27z746_set_prop(const struct device *dev, struct fuel_gauge_property *prop)
{
	int rc = 0;
	uint16_t val = 0;

	switch (prop->property_type) {
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = bq27z746_write16(dev, BQ27Z746_MANUFACTURERACCESS,
				      prop->value.sbs_mfr_access_word);
		prop->value.sbs_mfr_access_word = val;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = bq27z746_write16(dev, BQ27Z746_ATRATE, prop->value.sbs_at_rate);
		prop->value.sbs_at_rate = val;
		break;
	default:
		rc = -ENOTSUP;
	}

	prop->status = rc;

	return rc;
}

static int bq27z746_set_props(const struct device *dev, struct fuel_gauge_property *props,
			      size_t len)
{
	int err_count = 0;

	for (int i = 0; i < len; i++) {
		int ret = bq27z746_set_prop(dev, props + i);

		err_count += ret ? 1 : 0;
	}

	err_count = (err_count == len) ? -1 : err_count;

	return err_count;
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

static const struct fuel_gauge_driver_api bq27z746_driver_api = {
	.get_property = &bq27z746_get_prop,
	.set_property = &bq27z746_set_props,
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
