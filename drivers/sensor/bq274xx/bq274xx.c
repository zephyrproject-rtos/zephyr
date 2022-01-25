/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <pm/device.h>
#include <sys/__assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>

#include "bq274xx.h"

/*** Time period definitions, all are in units of milliseconds. ***/
/* Time for transferring data between command registers and data memory. */
#define BQ274XX_SUBCLASS_DELAY 5
/* Time to set pin in order to exit shutdown mode */
#define PIN_DELAY_TIME 1U
/* Time it takes device to initialize before doing any configuration */
#define INIT_TIME 100U
/* Time it takes before an unseal is allowed after sealing with CFG update. */
#define UNSEAL_COOLDOWN 4000U
/* Time to wait when polling for changes that generally take a long time. */
#define SLOW_POLL_PERIOD 50U

static int bq274xx_gauge_configure(const struct device *dev);

static int bq274xx_command_reg_read16(const struct device *dev, uint8_t command, int16_t *value)
{
	uint8_t i2c_data[2];
	int status;
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_config *const config = dev->config;

	status = i2c_burst_read(data->i2c, config->reg_addr, command, i2c_data, 2);
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return status;
	}

	*value = (i2c_data[1] << 8) | i2c_data[0];

	return 0;
}

static int bq274xx_command_reg_write8(const struct device *dev, uint8_t command, uint8_t value)
{
	int status = 0;
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_config *const config = dev->config;

	status = i2c_reg_write_byte(data->i2c, config->reg_addr, command, value);
	if (status < 0) {
		LOG_ERR("Failed to write command register");
		return status;
	}

	return 0;
}

static int bq274xx_command_reg_write16(const struct device *dev, uint8_t command, uint16_t value)
{
	uint8_t i2c_data[3];
	int status = 0;
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_config *const config = dev->config;

	i2c_data[0] = command;
	i2c_data[1] = value & 0xFF;
	i2c_data[2] = value >> 8;

	status = i2c_write(data->i2c, i2c_data, sizeof(i2c_data), config->reg_addr);
	if (status < 0) {
		LOG_ERR("Failed to write command register");
		return status;
	}

	return 0;
}

static int bq274xx_control_reg_write(const struct device *dev, uint16_t subcommand)
{
	return bq274xx_command_reg_write16(dev, BQ274XX_COMMAND_CONTROL, subcommand);
}

static int bq274xx_read_data_block(const struct device *dev, uint8_t offset, uint8_t *buffer,
				   uint8_t num_bytes)
{
	uint8_t command;
	int status = 0;
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_config *const config = dev->config;

	command = BQ274XX_EXTENDED_BLOCKDATA_START + offset;

	status = i2c_burst_read(data->i2c, config->reg_addr, command, buffer, num_bytes);
	if (status < 0) {
		LOG_ERR("Failed to read block");
		return status;
	}

	return 0;
}

static int bq274xx_read_control_status(const struct device *dev, uint16_t *value)
{
	int status;

	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_CONTROL_STATUS);
	if (status < 0) {
		return status;
	}

	status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_CONTROL, value);

	if (status < 0) {
		return status;
	}

	return 0;
}

static int bq274xx_read_device_type(const struct device *dev, uint16_t *value)
{
	int status;

	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_DEVICE_TYPE);
	if (status < 0) {
		return status;
	}

	status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_CONTROL, value);

	if (status < 0) {
		return status;
	}

	return 0;
}

static int bq274xx_expected_device_type(enum bq274xx_part part, uint16_t *value)
{
	switch (part) {
	case BQ27411:
		*value = 0x0411;
		return 0;
	case BQ27421:
		*value = 0x0421;
		return 0;
	case BQ27425:
		*value = 0x0425;
		return 0;
	case BQ27426:
		*value = 0x0426;
		return 0;
	case BQ27441:
		*value = 0x0441;
		return 0;
	}

	return -EINVAL;
}

static int bq274xx_read_chem_id(const struct device *dev, uint16_t *value)
{
	int status;

	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_CHEM_ID);
	if (status < 0) {
		return status;
	}

	status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_CONTROL, value);
	if (status < 0) {
		return status;
	}

	return 0;
}

static int bq274xx_expected_chem_id(enum bq274xx_part part, enum bq274xx_chemistry chemistry,
				    uint16_t *value)
{
	if (part == BQ27426) {
		switch (chemistry) {
		case CHEM_A:
			*value = 0x3230;
			return 0;
		case CHEM_B:
			*value = 0x1202;
			return 0;
		case CHEM_C:
			*value = 0x3142;
			return 0;
		default:
			break;
		}
	}

	return -EINVAL;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int bq274xx_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq274xx_data *data = dev->data;
	float int_temp;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = ((data->voltage / 1000));
		val->val2 = ((data->voltage % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = ((data->avg_current / 1000));
		val->val2 = ((data->avg_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_STDBY_CURRENT:
		val->val1 = ((data->stdby_current / 1000));
		val->val2 = ((data->stdby_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
		val->val1 = ((data->max_load_current / 1000));
		val->val2 = ((data->max_load_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		int_temp = (data->internal_temperature * 0.1f);
		int_temp = int_temp - 273.15f;
		val->val1 = (int32_t)int_temp;
		val->val2 = (int_temp - (int32_t)int_temp) * 1000000;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = data->state_of_charge;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		val->val1 = data->state_of_health;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		val->val1 = (data->full_charge_capacity / 1000);
		val->val2 = ((data->full_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = (data->remaining_charge_capacity / 1000);
		val->val2 = ((data->remaining_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		val->val1 = (data->nom_avail_capacity / 1000);
		val->val2 = ((data->nom_avail_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		val->val1 = (data->full_avail_capacity / 1000);
		val->val2 = ((data->full_avail_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_POWER:
		val->val1 = (data->avg_power / 1000);
		val->val2 = ((data->avg_power % 1000) * 1000U);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bq274xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bq274xx_data *data = dev->data;
	int status = 0;

#ifdef CONFIG_BQ274XX_LAZY_CONFIGURE
	if (!data->lazy_loaded) {
		status = bq274xx_gauge_configure(dev);

		if (status < 0) {
			return status;
		}
		data->lazy_loaded = true;
	}
#endif

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_VOLTAGE, &data->voltage);
		if (status < 0) {
			LOG_ERR("Failed to read voltage");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_AVG_CURRENT,
						    &data->avg_current);
		if (status < 0) {
			LOG_ERR("Failed to read average current ");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_INT_TEMP,
						    &data->internal_temperature);
		if (status < 0) {
			LOG_ERR("Failed to read internal temperature");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_STDBY_CURRENT:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_STDBY_CURRENT,
						    &data->stdby_current);
		if (status < 0) {
			LOG_ERR("Failed to read standby current");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_MAX_CURRENT,
						    &data->max_load_current);
		if (status < 0) {
			LOG_ERR("Failed to read maximum current");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_SOC,
						    &data->state_of_charge);
		if (status < 0) {
			LOG_ERR("Failed to read state of charge");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_FULL_CAPACITY,
						    &data->full_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full charge capacity");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_REM_CAPACITY,
						    &data->remaining_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read remaining charge capacity");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_NOM_CAPACITY,
						    &data->nom_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read nominal available capacity");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_AVAIL_CAPACITY,
						    &data->full_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full available capacity");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_POWER:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_AVG_POWER,
						    &data->avg_power);
		if (status < 0) {
			LOG_ERR("Failed to read battery average power");
			return status;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_SOH,
						    &data->state_of_health);

		data->state_of_health = (data->state_of_health) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read state of health");
			return status;
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
static int bq274xx_gauge_init(const struct device *dev)
{
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_config *const config = dev->config;
	int status = 0;
	uint16_t id, expected_id;

#ifdef CONFIG_PM_DEVICE
	if (!device_is_ready(config->int_gpios.port)) {
		LOG_ERR("GPIO device pointer is not ready to be used");
		return -ENODEV;
	}
#endif

	data->i2c = device_get_binding(config->bus_name);
	if (data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.", config->bus_name);
		return -EINVAL;
	}

	status = bq274xx_read_device_type(dev, &id);
	if (status < 0) {
		LOG_ERR("Unable to read device ID");
		return status;
	}

	status = bq274xx_expected_device_type(config->part, &expected_id);
	if (status < 0) {
		LOG_ERR("Unable to determine expected device ID");
		return status;
	}

	if (id != expected_id) {
		LOG_ERR("Invalid Device: 0x%02x", id);
		return -EINVAL;
	}

#ifdef CONFIG_BQ274XX_LAZY_CONFIGURE
	data->lazy_loaded = false;
#else
	status = bq274xx_gauge_configure(dev);
#endif

	return status;
}

static int bq274xx_unseal(const struct device *dev)
{
	int status;
	uint16_t control_status;

	/* Unseal the battery control register by writing the key twice. */
	status = bq274xx_control_reg_write(dev, BQ274XX_UNSEAL_KEY);
	if (status < 0) {
		return status;
	}

	status = bq274xx_control_reg_write(dev, BQ274XX_UNSEAL_KEY);
	if (status < 0) {
		return status;
	}

	/* Check that it actually succeeded. */
	status = bq274xx_read_control_status(dev, &control_status);
	if (status < 0) {
		return status;
	}

	if (control_status & BIT(BQ274XX_CONTROL_STATUS_SS)) {
		return -EBUSY;
	}

	return 0;
}

static int bq274xx_unseal_with_retry(const struct device *dev)
{
	int status;

	status = bq274xx_unseal(dev);
	if (status == -EBUSY) {
		/* There is a 4 second cooldown after sealing the device before
		 * it can be unsealed. When writing the unseal register it will
		 * reset the cooldown regardless of success, so the full
		 * duration must elapse before we should try again.
		 */
		LOG_WRN("BQ274XX didn't unseal, trying again in 4s");
		k_msleep(UNSEAL_COOLDOWN);
		status = bq274xx_unseal(dev);
	}

	return status;
}

static int bq274xx_seal(const struct device *dev)
{
	int status;

	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_SEALED);
	if (status < 0) {
		return status;
	}

	return status;
}

static int bq274xx_enter_cfg_update(const struct device *dev)
{
	int status;
	uint16_t flags = 0;

	/* Send SET_CFGUPDATE to enter */
	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_SET_CFGUPDATE);
	if (status < 0) {
		return status;
	}

	/* Poll flags for completion. */
	do {
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			return status;
		}

		if (!(flags & BIT(BQ274XX_FLAGS_CFGUPMODE))) {
			k_msleep(SLOW_POLL_PERIOD);
		}

	} while (!(flags & BIT(BQ274XX_FLAGS_CFGUPMODE)));

	return status;
}

static int bq274xx_exit_cfg_update(const struct device *dev)
{
	int status;
	uint16_t flags = 0;

	/* Send SOFT_RESET to exit. */
	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_SOFT_RESET);
	if (status < 0) {
		return status;
	}

	/* Poll flags for completion. */
	do {
		status = bq274xx_command_reg_read16(dev, BQ274XX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			return status;
		}

		if (flags & BIT(BQ274XX_FLAGS_CFGUPMODE)) {
			k_msleep(SLOW_POLL_PERIOD);
		}
	} while (flags & BIT(BQ274XX_FLAGS_CFGUPMODE));

	return status;
}

static int bq274xx_load_data_memory(const struct device *dev, uint8_t subclass, uint8_t offset)
{
	int status;

	status = bq274xx_command_reg_write8(dev, BQ274XX_EXTENDED_DATA_CONTROL, 0x00);
	if (status < 0) {
		return status;
	}

	status = bq274xx_command_reg_write8(dev, BQ274XX_EXTENDED_DATA_CLASS, subclass);
	if (status < 0) {
		return status;
	}

	status = bq274xx_command_reg_write8(dev, BQ274XX_EXTENDED_DATA_BLOCK, offset);
	if (status < 0) {
		return status;
	}

	/* Wait for block to be copied from data RAM to block data registers */
	k_msleep(BQ274XX_SUBCLASS_DELAY);

	return status;
}

static int bq274xx_commit_data_memory(const struct device *dev)
{
	int status;
	uint8_t checksum;
	uint8_t buffer[BQ274XX_BLOCKDATA_SIZE];

	memset(buffer, 0, BQ274XX_BLOCKDATA_SIZE);

	status = bq274xx_read_data_block(dev, 0x00, buffer, BQ274XX_BLOCKDATA_SIZE);
	if (status < 0) {
		return status;
	}

	checksum = 0;
	for (uint8_t i = 0; i < BQ274XX_BLOCKDATA_SIZE; i++) {
		checksum += buffer[i];
	}
	checksum = 255 - checksum;

	status = bq274xx_command_reg_write8(dev, BQ274XX_EXTENDED_CHECKSUM, checksum);
	if (status < 0) {
		return status;
	}

	/* Wait for data to be committed to data RAM */
	k_msleep(BQ274XX_SUBCLASS_DELAY);

	return status;
}

static int bq274xx_set_chemistry(const struct device *dev, enum bq274xx_chemistry chemistry)
{
	uint16_t control_subcommand;

	switch (chemistry) {
	case CHEM_A:
		control_subcommand = BQ274XX_CONTROL_CHEM_A;
		break;
	case CHEM_B:
		control_subcommand = BQ274XX_CONTROL_CHEM_B;
		break;
	case CHEM_C:
		control_subcommand = BQ274XX_CONTROL_CHEM_C;
		break;
	default:
		return -EINVAL;
	}

	return bq274xx_control_reg_write(dev, control_subcommand);
}

static int bq274xx_configure_chemistry(const struct device *dev)
{
	int status;
	uint16_t control_status;
	uint16_t current_chem_id, desired_chem_id;
	const struct bq274xx_config *const config = dev->config;

	if (config->chemistry == CHEM_DEFAULT) {
		/* Don't need to do anything. */
		return 0;
	}

	status = bq274xx_read_chem_id(dev, &current_chem_id);
	if (status < 0) {
		LOG_ERR("Unable to read CHEM_ID");
		return status;
	}

	status = bq274xx_expected_chem_id(config->part, config->chemistry, &desired_chem_id);
	if (status < 0) {
		LOG_ERR("Unable to determine desired CHEM_ID");
		return status;
	}

	if (current_chem_id != desired_chem_id) {
		status = bq274xx_set_chemistry(dev, config->chemistry);
		if (status < 0) {
			LOG_ERR("Failed to update chemistry profile");
			return status;
		}

		while (1) {
			status = bq274xx_read_control_status(dev, &control_status);
			if (status < 0) {
				LOG_ERR("Unable to read CONTROL_STATUS register");
				return status;
			}

			/* Wait until active chemistry table is updated. */
			if ((control_status & BIT(BQ274XX_CONTROL_STATUS_CHEM_CHANGE)) == 0) {
				break;
			}

			/* This usually takes ~500ms, so sleep for a while. */
			k_msleep(SLOW_POLL_PERIOD);
		}

		status = bq274xx_read_chem_id(dev, &current_chem_id);
		if (status < 0) {
			LOG_ERR("Unable to read updated CHEM_ID");
			return status;
		}

		if (current_chem_id != desired_chem_id) {
			LOG_ERR("CHEM_ID failed to update");
			return -EIO;
		}
	}

	return 0;
}

struct bq274xx_blockdata_entry {
	struct bq274xx_blockdata_address address;
	uint16_t value;
};

static int bq274xx_compare_blockdata_addresses(struct bq274xx_blockdata_address arg1,
					       struct bq274xx_blockdata_address arg2)
{
	uint8_t block_index1 = arg1.offset / BQ274XX_BLOCKDATA_SIZE;
	uint8_t block_index2 = arg2.offset / BQ274XX_BLOCKDATA_SIZE;

	/* Sort by subclass first. */
	if (arg1.subclass < arg2.subclass) {
		return -1;
	}

	if (arg1.subclass > arg2.subclass) {
		return 1;
	}

	/* Then sort by block index. */
	if (block_index1 < block_index2) {
		return -1;
	}

	if (block_index1 > block_index2) {
		return 1;
	}

	return 0;
}

static int bq274xx_compare_blockdata_entries(const void *arg1, const void *arg2)
{
	/* Sort by address. */
	return bq274xx_compare_blockdata_addresses(
		((const struct bq274xx_blockdata_entry *)arg1)->address,
		((const struct bq274xx_blockdata_entry *)arg2)->address);
}

static void bq274xx_blockdata_entries_append(struct bq274xx_blockdata_entry *entries, int *size,
					     struct bq274xx_blockdata_address address,
					     uint16_t value)
{
	if (address.subclass != BQ274XX_SUBCLASS_INVALID) {
		entries[*size].address = address;
		entries[*size].value = value;
		*size += 1;
	}
}

static void bq274xx_blockdata_entries_initialize(const struct device *dev,
						 struct bq274xx_blockdata_entry *entries, int *size)
{
	uint16_t design_energy, taper_rate;
	const struct bq274xx_config *const config = dev->config;

	design_energy = (uint16_t)(config->design_voltage * config->design_capacity / 1000);
	taper_rate = (uint16_t)(config->design_capacity * 10 / config->taper_current);

	bq274xx_blockdata_entries_append(entries, size, config->blockdata_addresses->design_cap,
					 config->design_capacity);
	bq274xx_blockdata_entries_append(entries, size, config->blockdata_addresses->design_enr,
					 design_energy);
	bq274xx_blockdata_entries_append(entries, size,
					 config->blockdata_addresses->terminate_voltage,
					 config->terminate_voltage);
	bq274xx_blockdata_entries_append(entries, size, config->blockdata_addresses->taper_rate,
					 taper_rate);
	bq274xx_blockdata_entries_append(entries, size, config->blockdata_addresses->taper_current,
					 config->taper_current);

	qsort(entries, *size, sizeof(*entries), &bq274xx_compare_blockdata_entries);
}

static int bq274xx_write_blockdata_entry(const struct device *dev,
					 struct bq274xx_blockdata_entry entry)
{
	uint8_t command;
	uint16_t value;

	command = BQ274XX_EXTENDED_BLOCKDATA_START + entry.address.offset % BQ274XX_BLOCKDATA_SIZE;
	value = (entry.value >> 8) | (entry.value << 8);

	return bq274xx_command_reg_write16(dev, command, value);
}

static int bq274xx_configure_data_memory(const struct device *dev)
{
	int status;
	struct bq274xx_blockdata_entry blockdata_entries[MAX_BLOCKDATA_ENTRIES];
	int num_entries = 0, index = 0;
	struct bq274xx_blockdata_address current_block;

	bq274xx_blockdata_entries_initialize(dev, blockdata_entries, &num_entries);

	while (index < num_entries) {
		current_block = blockdata_entries[index].address;
		current_block.offset /= BQ274XX_BLOCKDATA_SIZE;
		current_block.offset *= BQ274XX_BLOCKDATA_SIZE;

		status =
			bq274xx_load_data_memory(dev, current_block.subclass, current_block.offset);
		if (status < 0) {
			LOG_ERR("Unable to load data memory");
			return status;
		}

		do {
			status = bq274xx_write_blockdata_entry(dev, blockdata_entries[index]);
			if (status < 0) {
				LOG_ERR("Unable to write to data memory");
				return status;
			}

			index++;
		} while (index < num_entries &&
			 bq274xx_compare_blockdata_addresses(blockdata_entries[index].address,
							     current_block) == 0);

		status = bq274xx_commit_data_memory(dev);
		if (status < 0) {
			LOG_ERR("Unable to commit data memory");
			return status;
		}
	}

	return 0;
}

static int bq274xx_gauge_configure(const struct device *dev)
{
	int status;

	status = bq274xx_unseal_with_retry(dev);
	if (status < 0) {
		LOG_ERR("Unable to unseal the gauge");
		return status;
	}

	status = bq274xx_enter_cfg_update(dev);
	if (status < 0) {
		LOG_ERR("Unable to enter CFG update");
		return status;
	}

	/* The chemistry change needs to be complete before data memory is
	 * updated, otherwise the firmware can overwrite some settings.
	 */
	status = bq274xx_configure_chemistry(dev);
	if (status < 0) {
		LOG_ERR("Unable to update chemistry");
		return status;
	}

	status = bq274xx_configure_data_memory(dev);
	if (status < 0) {
		LOG_ERR("Unable to update data memory");
		return status;
	}

	status = bq274xx_exit_cfg_update(dev);
	if (status < 0) {
		LOG_ERR("Unable to exit CFG update");
		return status;
	}

	/* Force the battery to be detected. */
	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_BAT_INSERT);
	if (status < 0) {
		LOG_ERR("Unable to configure BAT Detect");
		return status;
	}

	status = bq274xx_seal(dev);
	if (status < 0) {
		LOG_ERR("Unable to seal the gauge");
		return status;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int bq274xx_enter_shutdown_mode(const struct device *dev)
{
	int status;

	status = bq274xx_unseal_with_retry(dev);
	if (status < 0) {
		LOG_ERR("Unable to unseal the gauge");
		return status;
	}

	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_SHUTDOWN_ENABLE);
	if (status < 0) {
		LOG_ERR("Unable to enable shutdown mode");
		return status;
	}

	status = bq274xx_control_reg_write(dev, BQ274XX_CONTROL_SHUTDOWN);
	if (status < 0) {
		LOG_ERR("Unable to enter shutdown mode");
		return status;
	}

	status = bq274xx_seal(dev);
	if (status < 0) {
		LOG_ERR("Unable to seal the gauge");
		return status;
	}

	return 0;
}

static int bq274xx_exit_shutdown_mode(const struct device *dev)
{
	const struct bq274xx_config *const config = dev->config;
	int status = 0;

	status = gpio_pin_configure_dt(&config->int_gpios, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
	if (status < 0) {
		LOG_ERR("Unable to configure interrupt pin to output and open drain");
		return status;
	}

	status = gpio_pin_set_dt(&config->int_gpios, 0);
	if (status < 0) {
		LOG_ERR("Unable to set interrupt pin to low");
		return status;
	}

	k_msleep(PIN_DELAY_TIME);

	status = gpio_pin_configure_dt(&config->int_gpios, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Unable to configure interrupt pin to input");
		return status;
	}

	k_msleep(INIT_TIME);

	status = bq274xx_gauge_configure(dev);
	if (status < 0) {
		LOG_ERR("Unable to configure bq274xx gauge");
		return status;
	}

	return 0;
}

static int bq274xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = bq274xx_enter_shutdown_mode(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = bq274xx_exit_shutdown_mode(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct sensor_driver_api bq274xx_battery_driver_api = {
	.sample_fetch = bq274xx_sample_fetch,
	.channel_get = bq274xx_channel_get,
};

__unused static const struct bq274xx_blockdata_addresses bq27411_blockdata_addresses = {
	.design_cap = { BQ274XX_SUBCLASS_STATE, 10 },
	.design_enr = { BQ274XX_SUBCLASS_STATE, 12 },
	.terminate_voltage = { BQ274XX_SUBCLASS_STATE, 16 },
	.taper_rate = { BQ274XX_SUBCLASS_STATE, 27 },
};

__unused static const struct bq274xx_blockdata_addresses bq27421_blockdata_addresses = {
	.design_cap = { BQ274XX_SUBCLASS_STATE, 10 },
	.design_enr = { BQ274XX_SUBCLASS_STATE, 12 },
	.terminate_voltage = { BQ274XX_SUBCLASS_STATE, 16 },
	.taper_rate = { BQ274XX_SUBCLASS_STATE, 27 },
};

__unused static const struct bq274xx_blockdata_addresses bq27425_blockdata_addresses = {
	.design_cap = { BQ274XX_SUBCLASS_STATE, 12 },
	.design_enr = { BQ274XX_SUBCLASS_STATE, 14 },
	.terminate_voltage = { BQ274XX_SUBCLASS_STATE, 18 },
	.taper_current = { BQ274XX_SUBCLASS_STATE, 30 },
};

__unused static const struct bq274xx_blockdata_addresses bq27426_blockdata_addresses = {
	.design_cap = { BQ274XX_SUBCLASS_STATE, 6 },
	.design_enr = { BQ274XX_SUBCLASS_STATE, 8 },
	.terminate_voltage = { BQ274XX_SUBCLASS_STATE, 10 },
	.taper_rate = { BQ274XX_SUBCLASS_STATE, 21 },
};

__unused static const struct bq274xx_blockdata_addresses bq27441_blockdata_addresses = {
	.design_cap = { BQ274XX_SUBCLASS_STATE, 10 },
	.design_enr = { BQ274XX_SUBCLASS_STATE, 12 },
	.terminate_voltage = { BQ274XX_SUBCLASS_STATE, 16 },
	.taper_rate = { BQ274XX_SUBCLASS_STATE, 27 },
};

#ifdef CONFIG_PM_DEVICE
#define BQ274XX_INT_CFG(index) .int_gpios = GPIO_DT_SPEC_INST_GET(index, int_gpios),
#else
#define BQ274XX_INT_CFG(index)
#endif

#define BQ274XX_INIT(index, PART_TYPE, part_type)						   \
	static struct bq274xx_data part_type##_driver_##index;					   \
												   \
	static const struct bq274xx_config part_type##_config_##index = {			   \
		BQ274XX_INT_CFG(index)								   \
		.bus_name = DT_INST_BUS_LABEL(index),						   \
		.reg_addr = DT_INST_REG_ADDR(index),						   \
		.part = PART_TYPE,								   \
		.blockdata_addresses = &part_type##_blockdata_addresses,			   \
		.design_voltage = DT_INST_PROP(index, design_voltage),				   \
		.design_capacity = DT_INST_PROP(index, design_capacity),			   \
		.taper_current = DT_INST_PROP(index, taper_current),				   \
		.terminate_voltage = DT_INST_PROP(index, terminate_voltage),			   \
		.chemistry = DT_ENUM_IDX_OR(DT_DRV_INST(index), chemistry, CHEM_DEFAULT),	   \
	};											   \
												   \
	PM_DEVICE_DT_INST_DEFINE(index, bq274xx_pm_action);					   \
												   \
	DEVICE_DT_INST_DEFINE(index, &bq274xx_gauge_init, PM_DEVICE_DT_INST_GET(index),		   \
			      &part_type##_driver_##index, &part_type##_config_##index,		   \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				   \
			      &bq274xx_battery_driver_api);

#define BQ27411_INIT(index) BQ274XX_INIT(index, BQ27411, bq27411)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_bq27411
DT_INST_FOREACH_STATUS_OKAY(BQ27411_INIT)

#define BQ27421_INIT(index) BQ274XX_INIT(index, BQ27421, bq27421)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_bq27421
DT_INST_FOREACH_STATUS_OKAY(BQ27421_INIT)

#define BQ27425_INIT(index) BQ274XX_INIT(index, BQ27425, bq27425)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_bq27425
DT_INST_FOREACH_STATUS_OKAY(BQ27425_INIT)

#define BQ27426_INIT(index) BQ274XX_INIT(index, BQ27426, bq27426)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_bq27426
DT_INST_FOREACH_STATUS_OKAY(BQ27426_INIT)

#define BQ27441_INIT(index) BQ274XX_INIT(index, BQ27441, bq27441)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_bq27441
DT_INST_FOREACH_STATUS_OKAY(BQ27441_INIT)
