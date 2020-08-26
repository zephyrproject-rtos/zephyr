/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq274xx

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <string.h>
#include <sys/byteorder.h>

#include "bq274xx.h"

#define BQ274XX_SUBCLASS_DELAY 5 /* subclass 64 & 82 needs 5ms delay */

static int bq274xx_command_reg_read(struct bq274xx_data *bq274xx, uint8_t reg_addr,
				    int16_t *val)
{
	uint8_t i2c_data[2];
	int status;

	status = i2c_burst_read(bq274xx->i2c, DT_INST_REG_ADDR(0), reg_addr,
				i2c_data, 2);
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	*val = (i2c_data[1] << 8) | i2c_data[0];

	return 0;
}

static int bq274xx_control_reg_write(struct bq274xx_data *bq274xx,
				     uint16_t subcommand)
{
	uint8_t i2c_data, reg_addr;
	int status = 0;

	reg_addr = BQ274XX_COMMAND_CONTROL_LOW;
	i2c_data = (uint8_t)((subcommand)&0x00FF);

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0), reg_addr,
				    i2c_data);
	if (status < 0) {
		LOG_ERR("Failed to write into control low register");
		return -EIO;
	}

	k_msleep(BQ274XX_SUBCLASS_DELAY);

	reg_addr = BQ274XX_COMMAND_CONTROL_HIGH;
	i2c_data = (uint8_t)((subcommand >> 8) & 0x00FF);

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0), reg_addr,
				    i2c_data);
	if (status < 0) {
		LOG_ERR("Failed to write into control high register");
		return -EIO;
	}

	return 0;
}

static int bq274xx_command_reg_write(struct bq274xx_data *bq274xx, uint8_t command,
				     uint8_t data)
{
	uint8_t i2c_data, reg_addr;
	int status = 0;

	reg_addr = command;
	i2c_data = data;

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0), reg_addr,
				    i2c_data);
	if (status < 0) {
		LOG_ERR("Failed to write into control register");
		return -EIO;
	}

	return 0;
}

static int bq274xx_read_data_block(struct bq274xx_data *bq274xx, uint8_t offset,
				   uint8_t *data, uint8_t bytes)
{
	uint8_t i2c_data;
	int status = 0;

	i2c_data = BQ274XX_EXTENDED_BLOCKDATA_START + offset;

	status = i2c_burst_read(bq274xx->i2c, DT_INST_REG_ADDR(0), i2c_data,
				data, bytes);
	if (status < 0) {
		LOG_ERR("Failed to read block");
		return -EIO;
	}

	k_msleep(BQ274XX_SUBCLASS_DELAY);

	return 0;
}

static int bq274xx_get_device_type(struct bq274xx_data *bq274xx, uint16_t *val)
{
	int status;

	status =
		bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_DEVICE_TYPE);
	if (status < 0) {
		LOG_ERR("Unable to write control register");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_CONTROL_LOW,
					  val);

	if (status < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	return 0;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int bq274xx_channel_get(struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq274xx_data *bq274xx = dev->data;
	float int_temp;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = ((bq274xx->voltage / 1000));
		val->val2 = ((bq274xx->voltage % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = ((bq274xx->avg_current / 1000));
		val->val2 = ((bq274xx->avg_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_STDBY_CURRENT:
		val->val1 = ((bq274xx->stdby_current / 1000));
		val->val2 = ((bq274xx->stdby_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
		val->val1 = ((bq274xx->max_load_current / 1000));
		val->val2 = ((bq274xx->max_load_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		int_temp = (bq274xx->internal_temperature * 0.1);
		int_temp = int_temp - 273.15;
		val->val1 = (int32_t)int_temp;
		val->val2 = (int_temp - (int32_t)int_temp) * 1000000;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = bq274xx->state_of_charge;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		val->val1 = bq274xx->state_of_health;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		val->val1 = (bq274xx->full_charge_capacity / 1000);
		val->val2 = ((bq274xx->full_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = (bq274xx->remaining_charge_capacity / 1000);
		val->val2 =
			((bq274xx->remaining_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		val->val1 = (bq274xx->nom_avail_capacity / 1000);
		val->val2 = ((bq274xx->nom_avail_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		val->val1 = (bq274xx->full_avail_capacity / 1000);
		val->val2 = ((bq274xx->full_avail_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_POWER:
		val->val1 = (bq274xx->avg_power / 1000);
		val->val2 = ((bq274xx->avg_power % 1000) * 1000U);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bq274xx_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bq274xx_data *bq274xx = dev->data;
	int status = 0;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		status = bq274xx_command_reg_read(
			bq274xx, BQ274XX_COMMAND_VOLTAGE, &bq274xx->voltage);
		if (status < 0) {
			LOG_ERR("Failed to read voltage");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		status = bq274xx_command_reg_read(bq274xx,
						  BQ274XX_COMMAND_AVG_CURRENT,
						  &bq274xx->avg_current);
		if (status < 0) {
			LOG_ERR("Failed to read average current ");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		status = bq274xx_command_reg_read(
			bq274xx, BQ274XX_COMMAND_INT_TEMP,
			&bq274xx->internal_temperature);
		if (status < 0) {
			LOG_ERR("Failed to read internal temperature");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_STDBY_CURRENT:
		status = bq274xx_command_reg_read(bq274xx,
						  BQ274XX_COMMAND_STDBY_CURRENT,
						  &bq274xx->stdby_current);
		if (status < 0) {
			LOG_ERR("Failed to read standby current");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
		status = bq274xx_command_reg_read(bq274xx,
						  BQ274XX_COMMAND_MAX_CURRENT,
						  &bq274xx->max_load_current);
		if (status < 0) {
			LOG_ERR("Failed to read maximum current");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_SOC,
						  &bq274xx->state_of_charge);
		if (status < 0) {
			LOG_ERR("Failed to read state of charge");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		status = bq274xx_command_reg_read(
			bq274xx, BQ274XX_COMMAND_FULL_CAPACITY,
			&bq274xx->full_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full charge capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		status = bq274xx_command_reg_read(
			bq274xx, BQ274XX_COMMAND_REM_CAPACITY,
			&bq274xx->remaining_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read remaining charge capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		status = bq274xx_command_reg_read(bq274xx,
						  BQ274XX_COMMAND_NOM_CAPACITY,
						  &bq274xx->nom_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read nominal available capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		status =
			bq274xx_command_reg_read(bq274xx,
						 BQ274XX_COMMAND_AVAIL_CAPACITY,
						 &bq274xx->full_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full available capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_POWER:
		status = bq274xx_command_reg_read(bq274xx,
						  BQ274XX_COMMAND_AVG_POWER,
						  &bq274xx->avg_power);
		if (status < 0) {
			LOG_ERR("Failed to read battery average power");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_SOH,
						  &bq274xx->state_of_health);

		bq274xx->state_of_health = (bq274xx->state_of_health) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read state of health");
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief initialise the fuel gauge
 *
 * @return 0 for success
 */
static int bq274xx_gauge_init(struct device *dev)
{
	struct bq274xx_data *bq274xx = dev->data;
	const struct bq274xx_config *const config = dev->config;
	int status = 0;
	uint8_t tmp_checksum = 0, checksum_old = 0, checksum_new = 0;
	uint16_t flags = 0, designenergy_mwh = 0, taperrate = 0, id;
	uint8_t designcap_msb, designcap_lsb, designenergy_msb, designenergy_lsb,
		terminatevolt_msb, terminatevolt_lsb, taperrate_msb,
		taperrate_lsb;
	uint8_t block[32];

	designenergy_mwh = (uint16_t)3.7 * config->design_capacity;
	taperrate =
		(uint16_t)config->design_capacity / (0.1 * config->taper_current);

	bq274xx->i2c = device_get_binding(config->bus_name);
	if (bq274xx->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			config->bus_name);
		return -EINVAL;
	}

	status = bq274xx_get_device_type(bq274xx, &id);
	if (status < 0) {
		LOG_ERR("Unable to get device ID");
		return -EIO;
	}

	if (id != BQ274XX_DEVICE_ID) {
		LOG_ERR("Invalid Device");
		return -EINVAL;
	}

	/** Unseal the battery control register **/
	status = bq274xx_control_reg_write(bq274xx, BQ274XX_UNSEAL_KEY);
	if (status < 0) {
		LOG_ERR("Unable to unseal the battery");
		return -EIO;
	}

	status = bq274xx_control_reg_write(bq274xx, BQ274XX_UNSEAL_KEY);
	if (status < 0) {
		LOG_ERR("Unable to unseal the battery");
		return -EIO;
	}

	/* Send CFG_UPDATE */
	status = bq274xx_control_reg_write(bq274xx,
					   BQ274XX_CONTROL_SET_CFGUPDATE);
	if (status < 0) {
		LOG_ERR("Unable to set CFGUpdate");
		return -EIO;
	}

	/** Step to place the Gauge into CONFIG UPDATE Mode **/
	do {
		status = bq274xx_command_reg_read(
			bq274xx, BQ274XX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			LOG_ERR("Unable to read flags");
			return -EIO;
		}

		if (!(flags & 0x0010)) {
			k_msleep(BQ274XX_SUBCLASS_DELAY * 10);
		}

	} while (!(flags & 0x0010));

	status = bq274xx_command_reg_write(bq274xx,
					   BQ274XX_EXTENDED_DATA_CONTROL, 0x00);
	if (status < 0) {
		LOG_ERR("Failed to enable block data memory");
		return -EIO;
	}

	/* Access State subclass */
	status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_DATA_CLASS,
					   0x52);
	if (status < 0) {
		LOG_ERR("Failed to update state subclass");
		return -EIO;
	}

	/* Write the block offset */
	status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_DATA_BLOCK,
					   0x00);
	if (status < 0) {
		LOG_ERR("Failed to update block offset");
		return -EIO;
	}

	for (uint8_t i = 0; i < 32; i++) {
		block[i] = 0;
	}

	status = bq274xx_read_data_block(bq274xx, 0x00, block, 32);
	if (status < 0) {
		LOG_ERR("Unable to read block data");
		return -EIO;
	}

	tmp_checksum = 0;
	for (uint8_t i = 0; i < 32; i++) {
		tmp_checksum += block[i];
	}
	tmp_checksum = 255 - tmp_checksum;

	/* Read the block checksum */
	status = i2c_reg_read_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				   BQ274XX_EXTENDED_CHECKSUM, &checksum_old);
	if (status < 0) {
		LOG_ERR("Unable to read block checksum");
		return -EIO;
	}

	designcap_msb = config->design_capacity >> 8;
	designcap_lsb = config->design_capacity & 0x00FF;
	designenergy_msb = designenergy_mwh >> 8;
	designenergy_lsb = designenergy_mwh & 0x00FF;
	terminatevolt_msb = config->terminate_voltage >> 8;
	terminatevolt_lsb = config->terminate_voltage & 0x00FF;
	taperrate_msb = taperrate >> 8;
	taperrate_lsb = taperrate & 0x00FF;

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				    BQ274XX_EXTENDED_BLOCKDATA_DESIGN_CAP_HIGH,
				    designcap_msb);
	if (status < 0) {
		LOG_ERR("Failed to write designCAP MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				    BQ274XX_EXTENDED_BLOCKDATA_DESIGN_CAP_LOW,
				    designcap_lsb);
	if (status < 0) {
		LOG_ERR("Failed to erite designCAP LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				    BQ274XX_EXTENDED_BLOCKDATA_DESIGN_ENR_HIGH,
				    designenergy_msb);
	if (status < 0) {
		LOG_ERR("Failed to write designEnergy MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				    BQ274XX_EXTENDED_BLOCKDATA_DESIGN_ENR_LOW,
				    designenergy_lsb);
	if (status < 0) {
		LOG_ERR("Failed to erite designEnergy LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(
		bq274xx->i2c, DT_INST_REG_ADDR(0),
		BQ274XX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_HIGH,
		terminatevolt_msb);
	if (status < 0) {
		LOG_ERR("Failed to write terminateVolt MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(
		bq274xx->i2c, DT_INST_REG_ADDR(0),
		BQ274XX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_LOW,
		terminatevolt_lsb);
	if (status < 0) {
		LOG_ERR("Failed to write terminateVolt LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				    BQ274XX_EXTENDED_BLOCKDATA_TAPERRATE_HIGH,
				    taperrate_msb);
	if (status < 0) {
		LOG_ERR("Failed to write taperRate MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				    BQ274XX_EXTENDED_BLOCKDATA_TAPERRATE_LOW,
				    taperrate_lsb);
	if (status < 0) {
		LOG_ERR("Failed to erite taperRate LSB");
		return -EIO;
	}

	for (uint8_t i = 0; i < 32; i++) {
		block[i] = 0;
	}

	status = bq274xx_read_data_block(bq274xx, 0x00, block, 32);
	if (status < 0) {
		LOG_ERR("Unable to read block data");
		return -EIO;
	}

	checksum_new = 0;
	for (uint8_t i = 0; i < 32; i++) {
		checksum_new += block[i];
	}
	checksum_new = 255 - checksum_new;

	status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_CHECKSUM,
					   checksum_new);
	if (status < 0) {
		LOG_ERR("Failed to update new checksum");
		return -EIO;
	}

	tmp_checksum = 0;
	status = i2c_reg_read_byte(bq274xx->i2c, DT_INST_REG_ADDR(0),
				   BQ274XX_EXTENDED_CHECKSUM, &tmp_checksum);
	if (status < 0) {
		LOG_ERR("Failed to read checksum");
		return -EIO;
	}

	status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_BAT_INSERT);
	if (status < 0) {
		LOG_ERR("Unable to configure BAT Detect");
		return -EIO;
	}

	status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_SOFT_RESET);
	if (status < 0) {
		LOG_ERR("Failed to soft reset the gauge");
		return -EIO;
	}

	flags = 0;
	/* Poll Flags   */
	do {
		status = bq274xx_command_reg_read(
			bq274xx, BQ274XX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			LOG_ERR("Unable to read flags");
			return -EIO;
		}

		if (!(flags & 0x0010)) {
			k_msleep(BQ274XX_SUBCLASS_DELAY * 10);
		}
	} while ((flags & 0x0010));

	/* Seal the gauge */
	status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_SEALED);
	if (status < 0) {
		LOG_ERR("Failed to seal the gauge");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api bq274xx_battery_driver_api = {
	.sample_fetch = bq274xx_sample_fetch,
	.channel_get = bq274xx_channel_get,
};

#define BQ274XX_INIT(index)                                                    \
	static struct bq274xx_data bq274xx_driver_##index;                     \
									       \
	static const struct bq274xx_config bq274xx_config_##index = {          \
		.bus_name = DT_INST_BUS_LABEL(index),                          \
		.design_voltage = DT_INST_PROP(index, design_voltage),         \
		.design_capacity = DT_INST_PROP(index, design_capacity),       \
		.taper_current = DT_INST_PROP(index, taper_current),           \
		.terminate_voltage = DT_INST_PROP(index, terminate_voltage),   \
	};                                                                     \
									       \
	DEVICE_AND_API_INIT(bq274xx_##index, DT_INST_LABEL(index),             \
			    &bq274xx_gauge_init, &bq274xx_driver_##index,      \
			    &bq274xx_config_##index, POST_KERNEL,              \
			    CONFIG_SENSOR_INIT_PRIORITY,                       \
			    &bq274xx_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ274XX_INIT)
