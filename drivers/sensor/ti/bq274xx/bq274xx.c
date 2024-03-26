/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Relevant documents:
 * - BQ27441
 *   Datasheet: https://www.ti.com/lit/gpn/bq27441-g1
 *   Technical reference manual: https://www.ti.com/lit/pdf/sluuac9
 * - BQ27421
 *   Datasheet: https://www.ti.com/lit/gpn/bq27421-g1
 *   Technical reference manual: https://www.ti.com/lit/pdf/sluuac5
 * - BQ27427
 *   Datasheet: https://www.ti.com/lit/gpn/bq27427
 *   Technical reference manual: https://www.ti.com/lit/pdf/sluucd5
 */

#define DT_DRV_COMPAT ti_bq274xx

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "bq274xx.h"

LOG_MODULE_REGISTER(bq274xx, CONFIG_SENSOR_LOG_LEVEL);

/* subclass 64 & 82 needs 5ms delay */
#define BQ274XX_SUBCLASS_DELAY K_MSEC(5)

/* Time to wait for CFGUP bit to be set, up to 1 second according to the
 * technical reference manual, keep some headroom like the Linux driver.
 */
#define BQ274XX_CFGUP_DELAY K_MSEC(25)
#define BQ274XX_CFGUP_MAX_TRIES 100

/* Time to set pin in order to exit shutdown mode */
#define PIN_DELAY_TIME K_MSEC(1)

/* Delay from power up or shutdown exit to chip entering active state, this is
 * defined as 250ms typical in the datasheet (Power-up communication delay).
 */
#define POWER_UP_DELAY_MS 300

/* Data memory size */
#define BQ27XXX_DM_SZ 32

/* Config update mode flag */
#define BQ27XXX_FLAG_CFGUP BIT(4)

/* BQ27427 CC Gain */
#define BQ27427_CC_GAIN BQ274XX_EXT_BLKDAT(5)
#define BQ27427_CC_GAIN_SIGN_BIT BIT(7)

/* Subclasses */
#define BQ274XX_SUBCLASS_82 82
#define BQ274XX_SUBCLASS_105 105

/* For temperature conversion */
#define KELVIN_OFFSET 273.15

static const struct bq274xx_regs bq27421_regs = {
	.dm_design_capacity = 10,
	.dm_design_energy = 12,
	.dm_terminate_voltage = 16,
	.dm_taper_rate = 27,
};

static const struct bq274xx_regs bq27427_regs = {
	.dm_design_capacity = 6,
	.dm_design_energy = 8,
	.dm_terminate_voltage = 10,
	.dm_taper_rate = 21,
};

static int bq274xx_cmd_reg_read(const struct device *dev, uint8_t reg_addr,
				int16_t *val)
{
	const struct bq274xx_config *config = dev->config;
	uint8_t i2c_data[2];
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, reg_addr, i2c_data, sizeof(i2c_data));
	if (ret < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	*val = sys_get_le16(i2c_data);

	return 0;
}

static int bq274xx_ctrl_reg_write(const struct device *dev, uint16_t subcommand)
{
	const struct bq274xx_config *config = dev->config;
	int ret;

	uint8_t tx_buf[3];

	tx_buf[0] = BQ274XX_CMD_CONTROL;
	sys_put_le16(subcommand, &tx_buf[1]);

	ret = i2c_write_dt(&config->i2c, tx_buf, sizeof(tx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write into control register");
		return -EIO;
	}

	return 0;
}

static int bq274xx_get_device_type(const struct device *dev, uint16_t *val)
{
	int ret;

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_DEVICE_TYPE);
	if (ret < 0) {
		LOG_ERR("Unable to write control register");
		return -EIO;
	}

	ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_CONTROL, val);
	if (ret < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	return 0;
}

static int bq274xx_read_block(const struct device *dev,
			      uint8_t subclass,
			      uint8_t *block, uint8_t num_bytes)
{
	const struct bq274xx_config *const config = dev->config;
	int ret;

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_DATA_CLASS, subclass);
	if (ret < 0) {
		LOG_ERR("Failed to update state subclass");
		return -EIO;
	}

	/* DataBlock(), 0 for the first 32 bytes. */
	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_DATA_BLOCK, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to update block offset");
		return -EIO;
	}

	k_sleep(BQ274XX_SUBCLASS_DELAY);

	ret = i2c_burst_read_dt(&config->i2c, BQ274XX_EXT_BLKDAT_START, block, num_bytes);
	if (ret < 0) {
		LOG_ERR("Unable to read block data");
		return -EIO;
	}

	return 0;
}

static int bq274xx_write_block(const struct device *dev,
			       uint8_t *block, uint8_t num_bytes)
{
	const struct bq274xx_config *const config = dev->config;
	uint8_t checksum = 0;
	int ret;
	uint8_t buf[1 + BQ27XXX_DM_SZ];

	__ASSERT_NO_MSG(num_bytes <= BQ27XXX_DM_SZ);

	buf[0] = BQ274XX_EXT_BLKDAT_START;
	memcpy(&buf[1], block, num_bytes);

	ret = i2c_write_dt(&config->i2c, buf, 1 + num_bytes);
	if (ret < 0) {
		LOG_ERR("Unable to write block data");
		return -EIO;
	}

	for (uint8_t i = 0; i < num_bytes; i++) {
		checksum += block[i];
	}
	checksum = 0xff - checksum;

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_CHECKSUM, checksum);
	if (ret < 0) {
		LOG_ERR("Failed to update block checksum");
		return -EIO;
	}

	k_sleep(BQ274XX_SUBCLASS_DELAY);

	return 0;
}

static void bq274xx_update_block(uint8_t *block,
				 uint8_t offset, uint16_t val,
				 bool *block_modified)
{
	uint16_t old_val;

	old_val = sys_get_be16(&block[offset]);

	LOG_DBG("update block: off=%d old=%d new=%d\n", offset, old_val, val);

	if (val == old_val) {
		return;
	}

	sys_put_be16(val, &block[offset]);

	*block_modified = true;
}

static int bq274xx_mode_cfgupdate(const struct device *dev, bool enabled)
{
	uint16_t flags;
	uint8_t try;
	int ret;
	uint16_t val = enabled ? BQ274XX_CTRL_SET_CFGUPDATE : BQ274XX_CTRL_SOFT_RESET;
	bool enabled_flag;

	ret = bq274xx_ctrl_reg_write(dev, val);
	if (ret < 0) {
		LOG_ERR("Unable to set device mode to %02x", val);
		return -EIO;
	}

	for (try = 0; try < BQ274XX_CFGUP_MAX_TRIES; try++) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_FLAGS, &flags);
		if (ret < 0) {
			LOG_ERR("Unable to read flags");
			return -EIO;
		}

		enabled_flag =  !!(flags & BQ27XXX_FLAG_CFGUP);
		if (enabled_flag == enabled) {
			LOG_DBG("CFGUP ready, try %u", try);
			break;
		}

		k_sleep(BQ274XX_CFGUP_DELAY);
	}

	if (try >= BQ274XX_CFGUP_MAX_TRIES) {
		LOG_ERR("Config mode change timeout");
		return -EIO;
	}

	return 0;
}

/*
 * BQ27427 needs the CC Gain polarity swapped from the ROM value.
 * The details are currently only documented in the TI E2E support forum:
 * https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1215460/bq27427evm-misbehaving-stateofcharge
 */
static int bq27427_ccgain_quirk(const struct device *dev)
{
	const struct bq274xx_config *const config = dev->config;
	int ret;
	uint8_t val, checksum;

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_DATA_CLASS,
				    BQ274XX_SUBCLASS_105);
	if (ret < 0) {
		LOG_ERR("Failed to update state subclass");
		return -EIO;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_DATA_BLOCK, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to update block offset");
		return -EIO;
	}

	k_sleep(BQ274XX_SUBCLASS_DELAY);

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ27427_CC_GAIN, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read ccgain");
		return -EIO;
	}

	if (!(val & BQ27427_CC_GAIN_SIGN_BIT)) {
		LOG_DBG("bq27427 quirk already applied");
		return 0;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ274XX_EXT_CHECKSUM, &checksum);
	if (ret < 0) {
		LOG_ERR("Failed to read block checksum");
		return -EIO;
	}

	/* Flip the sign bit on both value and checksum. */
	val ^= BQ27427_CC_GAIN_SIGN_BIT;
	checksum ^= BQ27427_CC_GAIN_SIGN_BIT;

	LOG_DBG("bq27427: val=%02x checksum=%02x", val, checksum);

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ27427_CC_GAIN, val);
	if (ret < 0) {
		LOG_ERR("Failed to update ccgain");
		return -EIO;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_CHECKSUM, checksum);
	if (ret < 0) {
		LOG_ERR("Failed to update block checksum");
		return -EIO;
	}

	k_sleep(BQ274XX_SUBCLASS_DELAY);

	return 0;
}

static int bq274xx_ensure_chemistry(const struct device *dev)
{
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_config *const config = dev->config;
	uint16_t chem_id = config->chemistry_id;

	if (chem_id == 0) {
		/* No chemistry ID set, rely on the default of the device.*/
		return 0;
	}
	int ret;
	uint16_t val;

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_CHEM_ID);
	if (ret < 0) {
		LOG_ERR("Unable to write control register");
		return -EIO;
	}

	ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_CONTROL, &val);
	if (ret < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	LOG_DBG("Chem ID: %04x", val);

	if (val != chem_id) {
		/* Only the bq27427 has a configurable Chemistry ID. On bq27421, it depends on the
		 * variant of the chip, so just error out if the chemistry ID is wrong.
		 */
		if (data->regs != &bq27427_regs) {
			LOG_ERR("Unable to confirm chemistry ID 0x%04x. Device reported 0x%04x",
				chem_id, val);
			return -EIO;
		}

		uint16_t cmd;

		switch (val) {
		case BQ27427_CHEM_ID_A:
			cmd = BQ27427_CTRL_CHEM_A;
			break;
		case BQ27427_CHEM_ID_B:
			cmd = BQ27427_CTRL_CHEM_B;
			break;
		case BQ27427_CHEM_ID_C:
			cmd = BQ27427_CTRL_CHEM_C;
			break;
		default:
			LOG_ERR("Unsupported chemistry ID 0x%04x", val);
			return -EINVAL;
		}

		ret = bq274xx_ctrl_reg_write(dev, cmd);
		if (ret < 0) {
			LOG_ERR("Unable to configure chemistry");
			return -EIO;
		}
	}
	return 0;
}

static int bq274xx_gauge_configure(const struct device *dev)
{
	const struct bq274xx_config *const config = dev->config;
	struct bq274xx_data *data = dev->data;
	const struct bq274xx_regs *regs = data->regs;
	int ret;
	uint16_t designenergy_mwh, taperrate;
	uint8_t block[BQ27XXX_DM_SZ];
	bool block_modified = false;

	designenergy_mwh = (uint32_t)config->design_capacity * 37 / 10; /* x3.7 */
	taperrate = config->design_capacity * 10 / config->taper_current;

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_UNSEAL_KEY_A);
	if (ret < 0) {
		LOG_ERR("Unable to unseal the battery");
		return -EIO;
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_UNSEAL_KEY_B);
	if (ret < 0) {
		LOG_ERR("Unable to unseal the battery");
		return -EIO;
	}

	ret = bq274xx_mode_cfgupdate(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, BQ274XX_EXT_DATA_CONTROL, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to enable block data memory");
		return -EIO;
	}

	ret = bq274xx_read_block(dev, BQ274XX_SUBCLASS_82, block, sizeof(block));
	if (ret < 0) {
		return ret;
	}

	bq274xx_update_block(block,
			     regs->dm_design_capacity, config->design_capacity,
			     &block_modified);
	bq274xx_update_block(block,
			     regs->dm_design_energy, designenergy_mwh,
			     &block_modified);
	bq274xx_update_block(block,
			     regs->dm_terminate_voltage, config->terminate_voltage,
			     &block_modified);
	bq274xx_update_block(block,
			     regs->dm_taper_rate, taperrate,
			     &block_modified);

	if (block_modified) {
		LOG_INF("bq274xx: updating fuel gauge parameters");

		ret = bq274xx_write_block(dev, block, sizeof(block));
		if (ret < 0) {
			return ret;
		}

		if (data->regs == &bq27427_regs) {
			ret = bq27427_ccgain_quirk(dev);
			if (ret < 0) {
				return ret;
			}
		}

		ret = bq274xx_ensure_chemistry(dev);
		if (ret < 0) {
			return ret;
		}

		ret = bq274xx_mode_cfgupdate(dev, false);
		if (ret < 0) {
			return ret;
		}
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_SEALED);
	if (ret < 0) {
		LOG_ERR("Failed to seal the gauge");
		return -EIO;
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_BAT_INSERT);
	if (ret < 0) {
		LOG_ERR("Unable to configure BAT Detect");
		return -EIO;
	}

	data->configured = true;

	return 0;
}

static int bq274xx_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq274xx_data *data = dev->data;
	int32_t int_temp;

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
		/* Convert units from 0.1K to 0.01K */
		int_temp = data->internal_temperature * 10;
		/* Convert to 0.01C */
		int_temp -= (int32_t)(100.0 * KELVIN_OFFSET);
		val->val1 = int_temp / 100;
		val->val2 = (int_temp % 100) * 10000;
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
		val->val1 = data->full_charge_capacity;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = data->remaining_charge_capacity;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		val->val1 = data->nom_avail_capacity;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		val->val1 = data->full_avail_capacity;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_AVG_POWER:
		val->val1 = data->avg_power;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bq274xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bq274xx_data *data = dev->data;
	int ret = -ENOTSUP;

	if (!data->configured) {
		ret = bq274xx_gauge_configure(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_VOLTAGE) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_VOLTAGE,
					   &data->voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read voltage");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_AVG_CURRENT) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_AVG_CURRENT,
					   &data->avg_current);
		if (ret < 0) {
			LOG_ERR("Failed to read average current ");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_TEMP) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_INT_TEMP,
					   &data->internal_temperature);
		if (ret < 0) {
			LOG_ERR("Failed to read internal temperature");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_STDBY_CURRENT) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_STDBY_CURRENT,
					   &data->stdby_current);
		if (ret < 0) {
			LOG_ERR("Failed to read standby current");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_MAX_CURRENT,
					   &data->max_load_current);
		if (ret < 0) {
			LOG_ERR("Failed to read maximum current");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_STATE_OF_CHARGE) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_SOC,
					   &data->state_of_charge);
		if (ret < 0) {
			LOG_ERR("Failed to read state of charge");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_FULL_CAPACITY,
				   &data->full_charge_capacity);
		if (ret < 0) {
			LOG_ERR("Failed to read full charge capacity");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_REM_CAPACITY,
				   &data->remaining_charge_capacity);
		if (ret < 0) {
			LOG_ERR("Failed to read remaining charge capacity");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_NOM_CAPACITY,
				   &data->nom_avail_capacity);
		if (ret < 0) {
			LOG_ERR("Failed to read nominal available capacity");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_AVAIL_CAPACITY,
				   &data->full_avail_capacity);
		if (ret < 0) {
			LOG_ERR("Failed to read full available capacity");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_AVG_POWER) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_AVG_POWER,
					   &data->avg_power);
		if (ret < 0) {
			LOG_ERR("Failed to read battery average power");
			return -EIO;
		}
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GAUGE_STATE_OF_HEALTH) {
		ret = bq274xx_cmd_reg_read(dev, BQ274XX_CMD_SOH,
					   &data->state_of_health);

		data->state_of_health = (data->state_of_health) & 0x00FF;

		if (ret < 0) {
			LOG_ERR("Failed to read state of health");
			return -EIO;
		}
	}

	return ret;
}

/**
 * @brief initialise the fuel gauge
 *
 * @return 0 for success
 */
static int bq274xx_gauge_init(const struct device *dev)
{
	const struct bq274xx_config *const config = dev->config;
	struct bq274xx_data *data = dev->data;
	int ret;
	uint16_t id;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

#if defined(CONFIG_BQ274XX_PM) || defined(CONFIG_BQ274XX_TRIGGER)
	if (!gpio_is_ready_dt(&config->int_gpios)) {
		LOG_ERR("GPIO device pointer is not ready to be used");
		return -ENODEV;
	}
#endif

	k_sleep(K_TIMEOUT_ABS_MS(POWER_UP_DELAY_MS));

	ret = bq274xx_get_device_type(dev, &id);
	if (ret < 0) {
		LOG_ERR("Unable to get device ID");
		return -EIO;
	}

	if (id == BQ27421_DEVICE_ID) {
		data->regs = &bq27421_regs;
	} else if (id == BQ27427_DEVICE_ID) {
		data->regs = &bq27427_regs;
	} else {
		LOG_ERR("Unsupported device ID: 0x%04x", id);
		return -ENOTSUP;
	}

#ifdef CONFIG_BQ274XX_TRIGGER
	ret = bq274xx_trigger_mode_init(dev);
	if (ret < 0) {
		LOG_ERR("Unable set up trigger mode.");
		return ret;
	}
#endif

	if (!config->lazy_loading) {
		ret = bq274xx_gauge_configure(dev);
	}

	return ret;
}

#ifdef CONFIG_BQ274XX_PM
static int bq274xx_enter_shutdown_mode(const struct device *dev)
{
	int ret;

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_UNSEAL_KEY_A);
	if (ret < 0) {
		LOG_ERR("Unable to unseal the battery");
		return ret;
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_UNSEAL_KEY_B);
	if (ret < 0) {
		LOG_ERR("Unable to unseal the battery");
		return ret;
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_SHUTDOWN_ENABLE);
	if (ret < 0) {
		LOG_ERR("Unable to enable shutdown mode");
		return ret;
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_SHUTDOWN);
	if (ret < 0) {
		LOG_ERR("Unable to enter shutdown mode");
		return ret;
	}

	ret = bq274xx_ctrl_reg_write(dev, BQ274XX_CTRL_SEALED);
	if (ret < 0) {
		LOG_ERR("Failed to seal the gauge");
		return ret;
	}

	return 0;
}

static int bq274xx_exit_shutdown_mode(const struct device *dev)
{
	const struct bq274xx_config *const config = dev->config;
	int ret;

	ret = gpio_pin_configure_dt(&config->int_gpios, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
	if (ret < 0) {
		LOG_ERR("Unable to configure interrupt pin to output and open drain");
		return ret;
	}

	ret = gpio_pin_set_dt(&config->int_gpios, 0);
	if (ret < 0) {
		LOG_ERR("Unable to set interrupt pin to low");
		return ret;
	}

	k_sleep(PIN_DELAY_TIME);

	ret = gpio_pin_configure_dt(&config->int_gpios, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Unable to configure interrupt pin to input");
		return ret;
	}

	if (!config->lazy_loading) {
		k_sleep(K_MSEC(POWER_UP_DELAY_MS));

		ret = bq274xx_gauge_configure(dev);
		if (ret < 0) {
			LOG_ERR("Unable to configure bq274xx gauge");
			return ret;
		}
	}

	return 0;
}

static int bq274xx_pm_action(const struct device *dev,
			     enum pm_device_action action)
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
#endif /* CONFIG_BQ274XX_PM */

static const struct sensor_driver_api bq274xx_battery_driver_api = {
	.sample_fetch = bq274xx_sample_fetch,
	.channel_get = bq274xx_channel_get,
#ifdef CONFIG_BQ274XX_TRIGGER
	.trigger_set = bq274xx_trigger_set,
#endif
};

#if defined(CONFIG_BQ274XX_PM) || defined(CONFIG_BQ274XX_TRIGGER)
#define BQ274XX_INT_CFG(index)							\
	.int_gpios = GPIO_DT_SPEC_INST_GET(index, int_gpios),
#define PM_BQ274XX_DT_INST_DEFINE(index, bq274xx_pm_action)			\
	PM_DEVICE_DT_INST_DEFINE(index, bq274xx_pm_action)
#define PM_BQ274XX_DT_INST_GET(index) PM_DEVICE_DT_INST_GET(index)
#else
#define BQ274XX_INT_CFG(index)
#define PM_BQ274XX_DT_INST_DEFINE(index, bq274xx_pm_action)
#define PM_BQ274XX_DT_INST_GET(index) NULL
#endif

#define BQ274XX_INIT(index)							\
	static struct bq274xx_data bq274xx_driver_##index;			\
										\
	static const struct bq274xx_config bq274xx_config_##index = {		\
		.i2c = I2C_DT_SPEC_INST_GET(index),				\
		BQ274XX_INT_CFG(index)						\
		.design_voltage = DT_INST_PROP(index, design_voltage),		\
		.design_capacity = DT_INST_PROP(index, design_capacity),	\
		.taper_current = DT_INST_PROP(index, taper_current),		\
		.terminate_voltage = DT_INST_PROP(index, terminate_voltage),	\
		.chemistry_id = DT_INST_PROP_OR(index, chemistry_id, 0),			\
		.lazy_loading = DT_INST_PROP(index, zephyr_lazy_load),		\
	};									\
										\
	PM_BQ274XX_DT_INST_DEFINE(index, bq274xx_pm_action);			\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(index, &bq274xx_gauge_init,		\
			    PM_BQ274XX_DT_INST_GET(index),			\
			    &bq274xx_driver_##index,				\
			    &bq274xx_config_##index, POST_KERNEL,		\
			    CONFIG_SENSOR_INIT_PRIORITY,			\
			    &bq274xx_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ274XX_INIT)
