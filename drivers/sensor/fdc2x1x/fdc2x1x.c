/* fdc2x1x.c - Driver for the Texas Instruments FDC2X1X */

/*
 * Copyright (c) 2020 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_fdc2x1x

#include <device.h>
#include <sys/util.h>
#include <logging/log.h>
#include <math.h>

#include "fdc2x1x.h"
#include "drivers/sensor/fdc2x1x.h"

LOG_MODULE_REGISTER(FDC2X1X, CONFIG_SENSOR_LOG_LEVEL);

static int fdc2x1x_init_config(const struct device *dev);

/**
 * Convert raw data to frequency (MHz).
 * @param dev - The device structure.
 * @param ch - Channel to convert the data from.
 * @param freq - Calculated frequency value .
 */
static void fdc2x1x_raw_to_freq(const struct device *dev,
				uint8_t ch, double *freq)
{
	struct fdc2x1x_data *data = dev->data;
	const struct fdc2x1x_config *cfg = dev->config;

	if (data->fdc221x) {
		*freq = (cfg->ch_cfg->fin_sel * (cfg->fref / 1000.0) *
			 data->channel_buf[ch]) / pow(2, 28);
	} else {
		*freq = cfg->ch_cfg->fin_sel * (cfg->fref / 1000.0) *
			((data->channel_buf[ch] / pow(2, 12 + cfg->output_gain)) +
			 (cfg->ch_cfg[ch].offset / pow(2, 16)));
	}
}

/**
 * Convert raw data to capacitance in picofarad (pF).
 * Requires the previous conversion from raw to frequency.
 * @param dev - The device structure.
 * @param ch - Channel to convert the data from .
 * @param freq - Frequency value
 * @param capacitance - Calculated capacitance value
 */
static void fdc2x1x_raw_to_capacitance(const struct device *dev,
				       uint8_t ch, double freq, double *capacitance)
{
	const struct fdc2x1x_config *cfg = dev->config;

	*capacitance = 1 / ((cfg->ch_cfg->inductance / 1000000.0) *
			    pow((2 * PI * freq), 2));
}

/**
 * Read/Write from device.
 * @param dev - The device structure.
 * @param reg - The register address. Use FDC2X1X_REG_READ(x) or
 *				FDC2X1X_REG_WRITE(x).
 * @param data - The register data.
 * @param length - Number of bytes being read
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_bus_access(const struct device *dev, uint8_t reg,
			      uint8_t *data, size_t length)
{
	const struct fdc2x1x_config *cfg = dev->config;

	if (reg & FDC2X1X_READ) {
		return i2c_burst_read(cfg->bus,
				      cfg->i2c_addr,
				      FDC2X1X_TO_I2C_REG(reg),
				      data, length);
	} else {
		if (length != 2) {
			return -EINVAL;
		}

		uint8_t buf[3];

		buf[0] = FDC2X1X_TO_I2C_REG(reg);
		memcpy(buf + 1, data, sizeof(uint16_t));

		return i2c_write(cfg->bus, buf,
				 sizeof(buf), cfg->i2c_addr);
	}
}

/**
 * Read (16 Bit) from device.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_reg_read(const struct device *dev,
			    uint8_t reg_addr,
			    uint16_t *reg_data)
{
	uint8_t buf[2];
	int ret;

	ret = fdc2x1x_bus_access(dev, FDC2X1X_REG_READ(reg_addr), buf, 2);
	*reg_data = ((uint16_t)buf[0] << 8) | buf[1];

	return ret;
}

/**
 * Write (16 Bit) to device.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_reg_write(const struct device *dev,
			     uint8_t reg_addr,
			     uint16_t reg_data)
{
	LOG_DBG("[0x%x] = 0x%x", reg_addr, reg_data);

	uint8_t buf[2];

	buf[0] = (uint8_t)(reg_data >> 8);
	buf[1] = (uint8_t)reg_data;

	return fdc2x1x_bus_access(dev, FDC2X1X_REG_WRITE(reg_addr), buf, 2);
}

/**
 * I2C write (16 Bit) to device using a mask.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param mask - The mask.
 * @param data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
int fdc2x1x_reg_write_mask(const struct device *dev,
			   uint8_t reg_addr,
			   uint16_t mask,
			   uint16_t data)
{
	int ret;
	uint16_t tmp;

	ret = fdc2x1x_reg_read(dev, reg_addr, &tmp);
	if (ret) {
		return ret;
	}
	LOG_DBG("read [0x%x] = 0x%x", reg_addr, tmp);
	LOG_DBG("mask: 0x%x", mask);

	tmp &= ~mask;
	tmp |= data;

	return fdc2x1x_reg_write(dev, reg_addr, tmp);
}

/**
 * Set the Frequency Selection value of a specific channel.
 * @param dev - The device structure.
 * @param chx - Channel number.
 * @param fin_sel - Frequency selection value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_fin_sel(const struct device *dev, uint8_t chx,
			       uint8_t fin_sel)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CLOCK_DIVIDERS_CH0 + chx,
				      FDC2X1X_CLK_DIV_CHX_FIN_SEL_MSK,
				      FDC2X1X_CLK_DIV_CHX_FIN_SEL_SET(fin_sel));
}

/**
 * Set the Reference Divider value of a specific channel.
 * @param dev - The device structure.
 * @param chx - Channel number.
 * @param fref_div - Reference divider value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_fref_divider(const struct device *dev, uint8_t chx,
				    uint16_t fref_div)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CLOCK_DIVIDERS_CH0 + chx,
				      FDC2X1X_CLK_DIV_CHX_FREF_DIV_MSK,
				      FDC2X1X_CLK_DIV_CHX_FREF_DIV_SET(fref_div));
}

/**
 * Set the Drive Current value of a specific channel.
 * @param dev - The device structure.
 * @param chx - Channel number.
 * @param idrv - Sensor driver current.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_idrive(const struct device *dev, uint8_t chx,
			      uint8_t idrv)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_DRIVE_CURRENT_CH0 + chx,
				      FDC2X1X_DRV_CURRENT_CHX_IDRIVE_MSK,
				      FDC2X1X_DRV_CURRENT_CHX_IDRIVE_SET(idrv));
}

/**
 * Set the Conversion Settling value of a specific channel.
 * @param dev - The device structure.
 * @param chx - Channel number.
 * @param settle_count - Settling time value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_settle_count(const struct device *dev, uint8_t chx,
				    uint16_t settle_count)
{
	return fdc2x1x_reg_write(dev,
				 FDC2X1X_SETTLECOUNT_CH0 + chx, settle_count);
}

/**
 * Set the Reference Count value of a specific channel.
 * @param dev - The device structure.
 * @param chx - Channel number.
 * @param rcount - Reference count value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_rcount(const struct device *dev,
			      uint8_t chx, uint16_t rcount)
{
	return fdc2x1x_reg_write(dev, FDC2X1X_RCOUNT_CH0 + chx, rcount);
}


/**
 * Set the Offset value of a specific channel.
 * @param dev - The device structure.
 * @param chx - Channel number.
 * @param offset - Offset value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_offset(const struct device *dev,
			      uint8_t chx, uint16_t offset)
{
	return fdc2x1x_reg_write(dev, FDC2X1X_OFFSET_CH0 + chx, offset);
}

/**
 * Set the Auto-Scan Mode.
 * @param dev - The device structure.
 * @param en - Enable/disable auto-acan mode.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_autoscan_mode(const struct device *dev, bool en)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_MUX_CONFIG,
				      FDC2X1X_MUX_CFG_AUTOSCAN_EN_MSK,
				      FDC2X1X_MUX_CFG_AUTOSCAN_EN_SET(en));
}

/**
 * Set the Auto-Scan Sequence Configuration.
 * @param dev - The device structure.
 * @param rr_seq - Auto-Scan sequence value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_rr_sequence(const struct device *dev, uint8_t rr_seq)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_MUX_CONFIG,
				      FDC2X1X_MUX_CFG_RR_SEQUENCE_MSK,
				      FDC2X1X_MUX_CFG_RR_SEQUENCE_SET(rr_seq));
}

/**
 * Set the Input deglitch filter bandwidth.
 * @param dev - The device structure.
 * @param deglitch - Deglitch selection.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_deglitch(const struct device *dev, uint8_t deglitch)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_MUX_CONFIG,
				      FDC2X1X_MUX_CFG_DEGLITCH_MSK,
				      FDC2X1X_MUX_CFG_DEGLITCH_SET(deglitch));
}

/**
 * Set the Output gain control.
 * @param dev - The device structure.
 * @param gain - Output gain.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_output_gain(const struct device *dev, uint8_t gain)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_RESET_DEV,
				      FDC2X1X_RESET_DEV_OUTPUT_GAIN_MSK,
				      FDC2X1X_RESET_DEV_OUTPUT_GAIN_SET(gain));
}

/**
 * Set the Active Channel for single channel
 * conversion if Auto-Scan Mode is disabled.
 * @param dev - The device structure.
 * @param ch - Active channel.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_active_channel(const struct device *dev, uint8_t ch)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CONFIG,
				      FDC2X1X_CFG_ACTIVE_CHAN_MSK,
				      FDC2X1X_CFG_ACTIVE_CHAN_SET(ch));
}

/**
 * Set the Sensor Activation Mode Selection.
 * @param dev - The device structure.
 * @param act_sel - Sensor Activation Mode Selection.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_sensor_activate_sel(const struct device *dev,
					   uint8_t act_sel)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CONFIG,
				      FDC2X1X_CFG_SENSOR_ACTIVATE_SEL_MSK,
				      FDC2X1X_CFG_SENSOR_ACTIVATE_SEL_SET(act_sel));
}

/**
 * Set the Reference Frequency Source.
 * @param dev - The device structure.
 * @param clk_src - Clock source.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_ref_clk_src(const struct device *dev, uint8_t clk_src)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CONFIG,
				      FDC2X1X_CFG_REF_CLK_SRC_MSK,
				      FDC2X1X_CFG_REF_CLK_SRC_SET(clk_src));
}

/**
 * Set the Current Sensor Drive.
 * @param dev - The device structure.
 * @param cur_drv - Current Sensor Drive.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_current_drv(const struct device *dev, uint8_t cur_drv)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CONFIG,
				      FDC2X1X_CFG_HIGH_CURRENT_DRV_MSK,
				      FDC2X1X_CFG_HIGH_CURRENT_DRV_SET(cur_drv));
}

/**
 * Enable/disable the INTB-Pin interrupt assertion.
 * @param dev - The device structure.
 * @param enable - True = enable int assertion, false = disable int assertion.
 * @return 0 in case of success, negative error code otherwise.
 */
int fdc2x1x_set_interrupt_pin(const struct device *dev, bool enable)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CONFIG,
				      FDC2X1X_CFG_INTB_DIS_MSK,
				      FDC2X1X_CFG_INTB_DIS_SET(!enable));
}

/**
 * Set the Operation Mode
 * @param dev - The device structure.
 * @param op_mode - Operation mode
 * @return 0 in case of success, negative error code otherwise.
 */
int fdc2x1x_set_op_mode(const struct device *dev,
			enum fdc2x1x_op_mode op_mode)
{
	return fdc2x1x_reg_write_mask(dev,
				      FDC2X1X_CONFIG,
				      FDC2X1X_CFG_SLEEP_SET_EN_MSK,
				      FDC2X1X_CFG_SLEEP_SET_EN_SET(op_mode));
}

/**
 * Get the STATUS register data
 * @param dev - The device structure.
 * @param status - Data stored in the STATUS register
 * @return 0 in case of success, negative error code otherwise.
 */
int fdc2x1x_get_status(const struct device *dev, uint16_t *status)
{
	return fdc2x1x_reg_read(dev, FDC2X1X_STATUS, status);
}

/**
 * Reset the device.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_reset(const struct device *dev)
{
	int ret;

	ret = fdc2x1x_reg_write_mask(dev,
				     FDC2X1X_RESET_DEV,
				     FDC2X1X_RESET_DEV_MSK,
				     FDC2X1X_RESET_DEV_SET(1));

	/* device defaults to sleep mode */
#ifdef CONFIG_PM_DEVICE
	struct fdc2x1x_data *data = dev->data;

	data->pm_state = PM_DEVICE_STATE_LOW_POWER;
#endif

	return ret;
}

#ifdef CONFIG_PM_DEVICE
/**
 * Reinitialize device after exiting shutdown mode
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_restart(const struct device *dev)
{
	int ret;

	k_sleep(K_MSEC(100));

	ret = fdc2x1x_init_config(dev);
	if (ret) {
		LOG_ERR("Reinitializing failed");
		return ret;
	}

#ifdef CONFIG_FDC2X1X_TRIGGER
	struct fdc2x1x_data *data = dev->data;

	ret = fdc2x1x_reg_write_mask(dev, FDC2X1X_ERROR_CONFIG,
				     data->int_config, data->int_config);
	if (ret) {
		LOG_ERR("Reinitializing trigger failed");
		return ret;
	}
#endif

	return 0;
}

/**
 * Enable/disable Shutdown Mode
 * @param dev - The device structure.
 * @param enable - True = enable shutdown, false = disable shutdown
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_shutdown(const struct device *dev, bool enable)
{
	const struct fdc2x1x_config *cfg = dev->config;
	int ret = 0;

	gpio_pin_set(cfg->sd_gpio, cfg->sd_pin, enable);

	if (!enable) {
		ret = fdc2x1x_restart(dev);
	}

	return ret;
}

/**
 * Set the Device Power Management State.
 * @param dev - The device structure.
 * @param pm_state - power management state
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_set_pm_state(const struct device *dev,
				enum pm_device_state pm_state)
{
	int ret;
	struct fdc2x1x_data *data = dev->data;
	const struct fdc2x1x_config *cfg = dev->config;

	switch (pm_state) {
	case PM_DEVICE_STATE_ACTIVE:
		if (data->pm_state == PM_DEVICE_STATE_OFF) {
			ret = fdc2x1x_set_shutdown(dev, false);
			if (ret) {
				return ret;
			}
		}

		ret = fdc2x1x_set_op_mode(dev, FDC2X1X_ACTIVE_MODE);
		if (ret) {
			return ret;
		}
		data->pm_state = PM_DEVICE_STATE_ACTIVE;

		break;
	case PM_DEVICE_STATE_LOW_POWER:
		if (data->pm_state == PM_DEVICE_STATE_OFF) {
			ret = fdc2x1x_set_shutdown(dev, false);
			if (ret) {
				return ret;
			}
		}
		ret = fdc2x1x_set_op_mode(dev, FDC2X1X_SLEEP_MODE);
		if (ret) {
			return ret;
		}
		data->pm_state = PM_DEVICE_STATE_LOW_POWER;

		break;
	case PM_DEVICE_STATE_OFF:
		if (cfg->sd_gpio->name) {
			ret = fdc2x1x_set_shutdown(dev, true);
			data->pm_state = PM_DEVICE_STATE_OFF;
		} else {
			LOG_ERR("SD pin not defined");
			ret = -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int fdc2x1x_device_pm_ctrl(const struct device *dev,
				  uint32_t ctrl_command,
				  enum pm_device_state *state)
{
	struct fdc2x1x_data *data = dev->data;
	int ret = 0;

	if (ctrl_command == PM_DEVICE_STATE_SET) {
		if (*state != data->pm_state) {
			switch (*state) {
			case PM_DEVICE_STATE_ACTIVE:
			case PM_DEVICE_STATE_LOW_POWER:
			case PM_DEVICE_STATE_OFF:
				ret = fdc2x1x_set_pm_state(dev, *state);
				break;
			default:
				LOG_ERR("PM state not supported");
				ret = -EINVAL;
			}
		}
	} else if (ctrl_command == PM_DEVICE_STATE_GET) {
		*state = data->pm_state;
	}

	return ret;
}
#endif

/**
 * Set attributes for the device.
 * @param dev - The device structure.
 * @param chan - The sensor channel type.
 * @param attr - The sensor attribute.
 * @param value - The sensor attribute value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	return -ENOTSUP;
}

/**
 * Read sensor data from the device.
 * @param dev - The device structure.
 * @param cap_data - The sensor value data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_get_cap_data(const struct device *dev)
{
	uint8_t increment_steps;
	int i;

	const struct fdc2x1x_config *cfg = dev->config;
	struct fdc2x1x_data *data = dev->data;
	uint8_t reg_addr = FDC2X1X_DATA_CH0;
	uint8_t buf_size = cfg->num_channels;

	if (data->fdc221x) {
		buf_size *= 2;
		increment_steps = 1;
	} else {
		increment_steps = 2;
	}

	uint16_t buf[buf_size];

#ifdef CONFIG_FDC2X1X_TRIGGER_NONE
	uint16_t status;

	do {
		fdc2x1x_get_status(dev, &status);
	} while (!(FDC2X1X_STATUS_DRDY(status)));
#endif

	for (i = 0; i < buf_size; i++) {
		if (fdc2x1x_reg_read(dev, reg_addr, &buf[i]) < 0) {
			LOG_ERR("Failed to read reg 0x%x", reg_addr);
			return -EIO;
		}
		reg_addr += increment_steps;
	}

	for (i = 0; i < cfg->num_channels; i++) {
		if (data->fdc221x) {
			data->channel_buf[i] = buf[i * 2] << 16 | buf[i * 2 + 1];
		} else {
			data->channel_buf[i] = buf[i];
		}
	}

	return 0;
}

/**
 * Fetch sensor data from the device.
 * @param dev - The device structure.
 * @param chan - The sensor channel type.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
#ifdef CONFIG_PM_DEVICE
	struct fdc2x1x_data *data = dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE) {
		LOG_ERR("Sample fetch failed, device is not in active mode");
		return -ENXIO;
	}
#endif

	return fdc2x1x_get_cap_data(dev);
}

/**
 * Get sensor channel value from the device.
 * @param dev - The device structure.
 * @param chan - The sensor channel type.
 * @param val - The sensor channel value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct fdc2x1x_config *cfg = dev->config;
	double ch_data;

	switch ((int16_t)chan) {
	case SENSOR_CHAN_FDC2X1X_FREQ_CH0:
		fdc2x1x_raw_to_freq(dev, 0, &ch_data);
		val->val1 = (uint32_t)ch_data;
		val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		break;
	case SENSOR_CHAN_FDC2X1X_FREQ_CH1:
		if (cfg->num_channels >= 2) {
			fdc2x1x_raw_to_freq(dev, 1, &ch_data);
			val->val1 = (uint32_t)ch_data;
			val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		} else {
			LOG_ERR("CH1 not defined.");
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_FDC2X1X_FREQ_CH2:
		if (cfg->num_channels >= 3) {
			fdc2x1x_raw_to_freq(dev, 2, &ch_data);
			val->val1 = (uint32_t)ch_data;
			val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		} else {
			LOG_ERR("CH2 not selected or not supported by device.");
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_FDC2X1X_FREQ_CH3:
		if (cfg->num_channels == 4) {
			fdc2x1x_raw_to_freq(dev, 3, &ch_data);
			val->val1 = (uint32_t)ch_data;
			val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		} else {
			LOG_ERR("CH3 not selected or not supported by device.");
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH0:
		fdc2x1x_raw_to_freq(dev, 0, &ch_data);
		fdc2x1x_raw_to_capacitance(dev, 0, ch_data, &ch_data);
		val->val1 = (uint32_t)ch_data;
		val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		break;
	case SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH1:
		if (cfg->num_channels >= 2) {
			fdc2x1x_raw_to_freq(dev, 1, &ch_data);
			fdc2x1x_raw_to_capacitance(dev, 1, ch_data, &ch_data);
			val->val1 = (uint32_t)ch_data;
			val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		} else {
			LOG_ERR("CH1 not selected or not supported by device.");
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH2:
		if (cfg->num_channels >= 3) {
			fdc2x1x_raw_to_freq(dev, 2, &ch_data);
			fdc2x1x_raw_to_capacitance(dev, 2, ch_data, &ch_data);
			val->val1 = (uint32_t)ch_data;
			val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		} else {
			LOG_ERR("CH3 not selected or not supported by device.");
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH3:
		if (cfg->num_channels >= 4) {
			fdc2x1x_raw_to_freq(dev, 3, &ch_data);
			fdc2x1x_raw_to_capacitance(dev, 3, ch_data, &ch_data);
			val->val1 = (uint32_t)ch_data;
			val->val2 = ((uint32_t)(ch_data * 1000000)) % 1000000;
		} else {
			LOG_ERR("CH3 not selected or not supported by device.");
			return -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Channel type not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api fdc2x1x_api_funcs = {
	.attr_set = fdc2x1x_attr_set,
	.sample_fetch = fdc2x1x_sample_fetch,
	.channel_get = fdc2x1x_channel_get,
#ifdef CONFIG_FDC2X1X_TRIGGER
	.trigger_set = fdc2x1x_trigger_set,
#endif
};

static int fdc2x1x_init_config(const struct device *dev)
{
	int ret;
	int ch;
	const struct fdc2x1x_config *cfg = dev->config;
	struct fdc2x1x_data *data = dev->data;

	/* Channel specific settings */
	for (ch = 0; ch < cfg->num_channels; ch++) {
		ret = fdc2x1x_set_fin_sel(dev, ch, cfg->ch_cfg[ch].fin_sel);
		if (ret) {
			return ret;
		}

		ret = fdc2x1x_set_fref_divider(dev, ch,
					       cfg->ch_cfg[ch].fref_divider);
		if (ret) {
			return ret;
		}

		ret = fdc2x1x_set_idrive(dev, ch, cfg->ch_cfg[ch].idrive);
		if (ret) {
			return ret;
		}

		ret = fdc2x1x_set_settle_count(dev, ch,
					       cfg->ch_cfg[ch].settle_count);
		if (ret) {
			return ret;
		}

		ret = fdc2x1x_set_rcount(dev, ch, cfg->ch_cfg[ch].rcount);
		if (ret) {
			return ret;
		}

		if (!data->fdc221x) {
			ret = fdc2x1x_set_offset(dev, ch,
						 cfg->ch_cfg[ch].offset);
			if (ret) {
				return ret;
			}
		}
	}

	ret = fdc2x1x_set_autoscan_mode(dev, cfg->autoscan_en);
	if (ret) {
		return ret;
	}

	ret = fdc2x1x_set_rr_sequence(dev, cfg->rr_sequence);
	if (ret) {
		return ret;
	}

	ret = fdc2x1x_set_deglitch(dev, cfg->deglitch);
	if (ret) {
		return ret;
	}

	if (!data->fdc221x) {
		ret = fdc2x1x_set_output_gain(dev, cfg->output_gain);
		if (ret) {
			return ret;
		}
	}

	ret = fdc2x1x_set_active_channel(dev, cfg->active_channel);
	if (ret) {
		return ret;
	}

	ret = fdc2x1x_set_sensor_activate_sel(dev, cfg->sensor_activate_sel);
	if (ret) {
		return ret;
	}

	ret = fdc2x1x_set_ref_clk_src(dev, cfg->clk_src);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_FDC2X1X_TRIGGER_NONE
	/* Enable Data Ready Flag to poll for new measurement */
	ret = fdc2x1x_reg_write_mask(dev, FDC2X1X_ERROR_CONFIG,
				     FDC2X1X_ERROR_CONFIG_DRDY_2INT_MSK,
				     FDC2X1X_ERROR_CONFIG_DRDY_2INT_SET(1));
	if (ret) {
		return ret;
	}

	/* INTB asserts by default, so disable it here */
	ret = fdc2x1x_set_interrupt_pin(dev, false);
	if (ret) {
		return ret;
	}
#endif

	ret = fdc2x1x_set_current_drv(dev, cfg->current_drv);
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * Probe device (Check if it is the correct device).
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_probe(const struct device *dev)
{
	struct fdc2x1x_data *data = dev->data;
	uint16_t dev_id, man_id;

	if (fdc2x1x_reg_read(dev, FDC2X1X_DEVICE_ID, &dev_id) < 0) {
		LOG_ERR("Failed to read device id");
		return -EIO;
	}


	if (dev_id == FDC2X1X_DEVICE_ID_VAL_28BIT) {
		data->fdc221x = true;
	} else if (dev_id == FDC2X1X_DEVICE_ID_VAL) {
		data->fdc221x = false;
	} else {
		LOG_ERR("Wrong device id");
		return -ENODEV;
	}

	if (data->fdc221x) {
		printk("is 28bit\n");
	} else {
		printk("is 12bit\n");
	}

	if (fdc2x1x_reg_read(dev, FDC2X1X_MANUFACTURER_ID, &man_id) < 0) {
		LOG_ERR("Failed to read manufacturer id");
		return -EIO;
	}

	if (man_id != FDC2X1X_MANUFACTURER_ID_VAL) {
		LOG_ERR("Wrong manufaturer id");
		return -ENODEV;
	}

	return 0;
}

/**
 * Initialize the SD-Pin.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_init_sd_pin(const struct device *dev)
{
	const struct fdc2x1x_config *cfg = dev->config;

	if (!device_is_ready(cfg->sd_gpio)) {
		LOG_ERR("%s: sd_gpio device not ready", cfg->sd_gpio->name);
		return -ENODEV;
	}

	gpio_pin_configure(cfg->sd_gpio, cfg->sd_pin,
			   GPIO_OUTPUT_INACTIVE | cfg->sd_flags);

	return 0;
}

/**
 * Initialization of the device.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int fdc2x1x_init(const struct device *dev)
{
	const struct fdc2x1x_config *cfg = dev->config;
	uint8_t ch_supported;

	if (cfg->fdc2x14) {
		ch_supported = 4;
	} else {
		ch_supported = 2;
	}

	if (cfg->num_channels == 0) {
		LOG_ERR("No channel nodes found");
		return -EINVAL;
	} else if (cfg->num_channels > ch_supported) {
		LOG_ERR("Amount of channels not supported by this device");
		return -EINVAL;
	}

	if (cfg->sd_gpio->name) {
		if (fdc2x1x_init_sd_pin(dev) < 0) {
			return -ENODEV;
		}
	}

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("%s: fdc2x1x device not ready", dev->name);
		return -ENODEV;
	}

	if (fdc2x1x_probe(dev) < 0) {
		return -ENODEV;
	}

	if (fdc2x1x_reset(dev) < 0) {
		return -EIO;
	}

	if (fdc2x1x_init_config(dev) < 0) {
		return -EIO;
	}

	if (fdc2x1x_set_op_mode(dev, FDC2X1X_ACTIVE_MODE) < 0) {
		return -EIO;
	}
#ifdef CONFIG_PM_DEVICE
	struct fdc2x1x_data *data = dev->data;

	data->pm_state = FDC2X1X_ACTIVE_MODE;
#endif

#ifdef CONFIG_FDC2X1X_TRIGGER
	if (fdc2x1x_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

#define FDC2X1X_SD_PROPS(n)						  \
	.sd_gpio = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(n), sd_gpios)), \
	.sd_pin = DT_INST_GPIO_PIN(n, sd_gpios),			  \
	.sd_flags = DT_INST_GPIO_FLAGS(n, sd_gpios),			  \

#define FDC2X1X_SD(n)				       \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, sd_gpios), \
		   (FDC2X1X_SD_PROPS(n)))

#define FDC2X1X_INTB_PROPS(n)						      \
	.intb_gpio = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(n), intb_gpios)), \
	.intb_pin = DT_INST_GPIO_PIN(n, intb_gpios),			      \
	.intb_flags = DT_INST_GPIO_FLAGS(n, intb_gpios),		      \

#define FDC2X1X_INTB(n)			   \
	IF_ENABLED(CONFIG_FDC2X1X_TRIGGER, \
		   (FDC2X1X_INTB_PROPS(n)))

#define FDC2X1X_CH_CFG_INIT(ch)				   \
	{						   \
		.rcount = DT_PROP(ch, rcount),		   \
		.offset = DT_PROP(ch, offset),		   \
		.settle_count = DT_PROP(ch, settlecount),  \
		.fref_divider = DT_PROP(ch, fref_divider), \
		.idrive = DT_PROP(ch, idrive),		   \
		.fin_sel = DT_PROP(ch, fin_sel),	   \
		.inductance = DT_PROP(ch, inductance),	   \
	},

#define FDC2X1X_CHANNEL_BUF_INIT(ch)          0,

#define FDC2X1X_INIT(n)							   \
	static uint32_t fdc2x1x_sample_buf_##n[] = {			   \
		DT_INST_FOREACH_CHILD(n, FDC2X1X_CHANNEL_BUF_INIT)	   \
	};								   \
									   \
	static struct fdc2x1x_data fdc2x1x_data_##n = {			   \
		.channel_buf = fdc2x1x_sample_buf_##n,			   \
	};								   \
									   \
	const struct fdc2x1x_chx_config ch_cfg_##n[] = {		   \
		DT_INST_FOREACH_CHILD(n, FDC2X1X_CH_CFG_INIT)		   \
	};								   \
									   \
	static const struct fdc2x1x_config fdc2x1x_config_##n = {	   \
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),			   \
		.i2c_addr = DT_INST_REG_ADDR(n),			   \
		.fdc2x14 = DT_INST_PROP(n, fdc2x14),			   \
		.autoscan_en = DT_INST_PROP(n, autoscan),		   \
		.rr_sequence = DT_INST_PROP(n, rr_sequence),		   \
		.active_channel = DT_INST_PROP(n, active_channel),	   \
		.deglitch = DT_INST_PROP(n, deglitch),			   \
		.sensor_activate_sel =					   \
			DT_ENUM_IDX(DT_DRV_INST(n), sensor_activate_sel),  \
		.clk_src = DT_ENUM_IDX(DT_DRV_INST(n), ref_clk_src),	   \
		.current_drv = DT_ENUM_IDX(DT_DRV_INST(n), current_drive), \
		.output_gain = DT_INST_PROP(n, output_gain),		   \
		.ch_cfg =       ch_cfg_##n,				   \
		.num_channels = ARRAY_SIZE(fdc2x1x_sample_buf_##n),	   \
		.fref = DT_INST_PROP(n, fref),				   \
		FDC2X1X_SD(n)						   \
		FDC2X1X_INTB(n)						   \
	};								   \
									   \
	DEVICE_DT_INST_DEFINE(n,					   \
			      fdc2x1x_init,				   \
			      fdc2x1x_device_pm_ctrl,			   \
			      &fdc2x1x_data_##n,			   \
			      &fdc2x1x_config_##n,			   \
			      POST_KERNEL,				   \
			      CONFIG_SENSOR_INIT_PRIORITY,		   \
			      &fdc2x1x_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(FDC2X1X_INIT)
