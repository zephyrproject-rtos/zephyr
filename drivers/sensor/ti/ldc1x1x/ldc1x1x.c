/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ldc1x1x

#include <zephyr/device.h>
#include <zephyr/drivers/sensor/ldc1x1x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ldc1x1x.h"

LOG_MODULE_REGISTER(ldc1x1x, CONFIG_SENSOR_LOG_LEVEL);

#define LDC1X1X_T_WAKEUP_US 2000U

static int ldc1x1x_init_config(const struct device *dev);

static void ldc1x1x_wait_wakeup(void)
{
	k_usleep(LDC1X1X_T_WAKEUP_US);
}

static int ldc1x1x_reg_read(const struct device *dev, uint8_t reg_addr, uint16_t *reg_data)
{
	const struct ldc1x1x_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c, reg_addr, buf, sizeof(buf));
	if (ret == 0) {
		*reg_data = sys_get_be16(buf);
	}

	return ret;
}

static int ldc1x1x_reg_write(const struct device *dev, uint8_t reg_addr, uint16_t reg_data)
{
	const struct ldc1x1x_config *cfg = dev->config;
	uint8_t buf[3];

	buf[0] = reg_addr;
	sys_put_be16(reg_data, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

int ldc1x1x_reg_write_mask(const struct device *dev, uint8_t reg_addr, uint16_t mask, uint16_t data)
{
	uint16_t value;
	int ret;

	ret = ldc1x1x_reg_read(dev, reg_addr, &value);
	if (ret != 0) {
		return ret;
	}

	value = (value & ~mask) | data;

	return ldc1x1x_reg_write(dev, reg_addr, value);
}

static int ldc1x1x_set_fin_divider(const struct device *dev, uint8_t channel, uint8_t fin_divider)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CLOCK_DIVIDERS0 + channel,
				      LDC1X1X_CLK_DIV_FIN_DIVIDER_MASK,
				      LDC1X1X_CLK_DIV_FIN_DIVIDER_SET(fin_divider));
}

static int ldc1x1x_set_fref_divider(const struct device *dev, uint8_t channel,
				    uint16_t fref_divider)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CLOCK_DIVIDERS0 + channel,
				      LDC1X1X_CLK_DIV_FREF_DIVIDER_MASK,
				      LDC1X1X_CLK_DIV_FREF_DIVIDER_SET(fref_divider));
}

static int ldc1x1x_set_idrive(const struct device *dev, uint8_t channel, uint8_t idrive)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_DRIVE_CURRENT0 + channel,
				      LDC1X1X_DRIVE_CURRENT_IDRIVE_MASK,
				      LDC1X1X_DRIVE_CURRENT_IDRIVE_SET(idrive));
}

static int ldc1x1x_set_settlecount(const struct device *dev, uint8_t channel, uint16_t settlecount)
{
	return ldc1x1x_reg_write(dev, LDC1X1X_SETTLECOUNT0 + channel, settlecount);
}

static int ldc1x1x_set_rcount(const struct device *dev, uint8_t channel, uint16_t rcount)
{
	return ldc1x1x_reg_write(dev, LDC1X1X_RCOUNT0 + channel, rcount);
}

static int ldc1x1x_set_offset(const struct device *dev, uint8_t channel, uint16_t offset)
{
	return ldc1x1x_reg_write(dev, LDC1X1X_OFFSET0 + channel, offset);
}

static int ldc1x1x_set_autoscan(const struct device *dev, bool enable)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_MUX_CONFIG, LDC1X1X_MUX_CONFIG_AUTOSCAN_EN_MASK,
				      LDC1X1X_MUX_CONFIG_AUTOSCAN_EN_SET(enable));
}

static int ldc1x1x_set_rr_sequence(const struct device *dev, uint8_t rr_sequence)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_MUX_CONFIG, LDC1X1X_MUX_CONFIG_RR_SEQUENCE_MASK,
				      LDC1X1X_MUX_CONFIG_RR_SEQUENCE_SET(rr_sequence));
}

static int ldc1x1x_set_deglitch(const struct device *dev, uint8_t deglitch)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_MUX_CONFIG, LDC1X1X_MUX_CONFIG_DEGLITCH_MASK,
				      LDC1X1X_MUX_CONFIG_DEGLITCH_SET(deglitch));
}

static int ldc1x1x_set_output_gain(const struct device *dev, uint8_t output_gain)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_RESET_DEV, LDC1X1X_RESET_DEV_OUTPUT_GAIN_MASK,
				      LDC1X1X_RESET_DEV_OUTPUT_GAIN_SET(output_gain));
}

static int ldc1x1x_set_active_channel(const struct device *dev, uint8_t active_channel)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_ACTIVE_CHAN_MASK,
				      LDC1X1X_CONFIG_ACTIVE_CHAN_SET(active_channel));
}

static int ldc1x1x_set_sensor_activate_sel(const struct device *dev, uint8_t sensor_activate_sel)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_SENSOR_ACTIVATE_SEL_MASK,
				      LDC1X1X_CONFIG_SENSOR_ACTIVATE_SEL_SET(sensor_activate_sel));
}

static int ldc1x1x_set_ref_clk_src(const struct device *dev, uint8_t ref_clk_src)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_REF_CLK_SRC_MASK,
				      LDC1X1X_CONFIG_REF_CLK_SRC_SET(ref_clk_src));
}

static int ldc1x1x_set_current_drive(const struct device *dev, uint8_t current_drive)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_HIGH_CURRENT_DRV_MASK,
				      LDC1X1X_CONFIG_HIGH_CURRENT_DRV_SET(current_drive));
}

static int ldc1x1x_set_rp_override(const struct device *dev, bool enable)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_RP_OVERRIDE_EN_MASK,
				      LDC1X1X_CONFIG_RP_OVERRIDE_EN_SET(enable));
}

static int ldc1x1x_set_auto_amplitude_dis(const struct device *dev, bool disable)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_AUTO_AMP_DIS_MASK,
				      LDC1X1X_CONFIG_AUTO_AMP_DIS_SET(disable));
}

int ldc1x1x_set_interrupt_pin(const struct device *dev, bool enable)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_INTB_DIS_MASK,
				      LDC1X1X_CONFIG_INTB_DIS_SET(!enable));
}

static int ldc1x1x_set_op_mode(const struct device *dev, enum ldc1x1x_op_mode op_mode)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_CONFIG, LDC1X1X_CONFIG_SLEEP_SET_EN_MASK,
				      LDC1X1X_CONFIG_SLEEP_SET_EN_SET(op_mode));
}

int ldc1x1x_get_status(const struct device *dev, uint16_t *status)
{
	return ldc1x1x_reg_read(dev, LDC1X1X_STATUS, status);
}

static int ldc1x1x_reset(const struct device *dev)
{
	return ldc1x1x_reg_write_mask(dev, LDC1X1X_RESET_DEV, LDC1X1X_RESET_DEV_MASK,
				      LDC1X1X_RESET_DEV_SET(1));
}

#ifdef CONFIG_PM_DEVICE
static int ldc1x1x_restart(const struct device *dev)
{
	int ret;

	k_msleep(100);

	ret = ldc1x1x_init_config(dev);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_LDC1X1X_TRIGGER
	struct ldc1x1x_data *data = dev->data;

	if (data->int_config != 0) {
		ret = ldc1x1x_reg_write_mask(dev, LDC1X1X_ERROR_CONFIG,
					     LDC1X1X_ERROR_CONFIG_DRDY_2INT_MASK, data->int_config);
		if (ret != 0) {
			return ret;
		}
	}
#endif

	return 0;
}

static int ldc1x1x_set_shutdown(const struct device *dev, bool enable)
{
	const struct ldc1x1x_config *cfg = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&cfg->sd_gpio, enable);
	if (ret != 0 || enable) {
		return ret;
	}

	ldc1x1x_wait_wakeup();

	return ldc1x1x_restart(dev);
}

static int ldc1x1x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ldc1x1x_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return ldc1x1x_set_op_mode(dev, LDC1X1X_ACTIVE_MODE);
	case PM_DEVICE_ACTION_SUSPEND:
		return ldc1x1x_set_op_mode(dev, LDC1X1X_SLEEP_MODE);
	case PM_DEVICE_ACTION_TURN_OFF:
		if (cfg->sd_gpio.port == NULL) {
			return -ENOTSUP;
		}

		return ldc1x1x_set_shutdown(dev, true);
	case PM_DEVICE_ACTION_TURN_ON:
		if (cfg->sd_gpio.port == NULL) {
			return -ENOTSUP;
		}

		return ldc1x1x_set_shutdown(dev, false);
	default:
		return -ENOTSUP;
	}
}
#endif

static bool ldc1x1x_channel_enabled(const struct ldc1x1x_config *cfg, uint8_t channel)
{
	return (cfg->channel_mask & BIT(channel)) != 0U;
}

static uint16_t ldc1x1x_ready_mask(const struct ldc1x1x_config *cfg)
{
	uint16_t mask = 0U;

	for (uint8_t channel = 0; channel < LDC1X1X_MAX_CHANNELS; channel++) {
		if (ldc1x1x_channel_enabled(cfg, channel)) {
			mask |= LDC1X1X_STATUS_UNREADCONV(channel);
		}
	}

	return mask;
}

static int64_t ldc1x1x_timeout_ms(const struct ldc1x1x_config *cfg)
{
	uint64_t total_us = 0U;

	for (uint8_t channel = 0; channel < LDC1X1X_MAX_CHANNELS; channel++) {
		const struct ldc1x1x_channel_config *ch_cfg = &cfg->ch_cfg[channel];
		uint64_t counts;

		if (!ldc1x1x_channel_enabled(cfg, channel)) {
			continue;
		}

		counts = (uint64_t)ch_cfg->rcount + ch_cfg->settle_count;
		total_us += DIV_ROUND_UP(counts * 16U * ch_cfg->fref_divider * 1000U, cfg->fref);
	}

	total_us = MAX(total_us * 2U, 1000U);

	return (int64_t)(DIV_ROUND_UP(total_us, 1000U) + 10U);
}

static int ldc1x1x_wait_for_data(const struct device *dev)
{
	const struct ldc1x1x_config *cfg = dev->config;
	uint16_t ready_mask = ldc1x1x_ready_mask(cfg);
	int64_t deadline = k_uptime_get() + ldc1x1x_timeout_ms(cfg);
	uint16_t status = 0U;

	while (k_uptime_get() <= deadline) {
		int ret = ldc1x1x_get_status(dev, &status);

		if (ret != 0) {
			return ret;
		}

		if ((status & LDC1X1X_STATUS_DRDY) != 0U || (status & ready_mask) != 0U) {
			return 0;
		}

		k_msleep(1);
	}

	LOG_ERR("data ready timeout, STATUS 0x%04x, ready_mask 0x%04x", status, ready_mask);

	return -ETIMEDOUT;
}

static int ldc1x1x_read_channel(const struct device *dev, uint8_t channel, uint32_t *sample)
{
	struct ldc1x1x_data *data = dev->data;
	uint8_t reg_addr = LDC1X1X_DATA0 + (channel * 2U);
	uint32_t full_scale;
	uint16_t msw;
	int ret;

	ret = ldc1x1x_reg_read(dev, reg_addr, &msw);
	if (ret != 0) {
		return ret;
	}

	if ((msw & LDC1X1X_DATA_ERR_MASK) != 0U) {
		LOG_ERR("conversion error 0x%04x on channel %u", msw, channel);
		return -EIO;
	}

	if (data->ldc161x) {
		uint16_t lsw;

		ret = ldc1x1x_reg_read(dev, reg_addr + 1U, &lsw);
		if (ret != 0) {
			return ret;
		}

		*sample = ((uint32_t)(msw & GENMASK(11, 0)) << 16) | lsw;
		full_scale = LDC1X1X_DATA_FULL_SCALE_161X;
	} else {
		*sample = msw & GENMASK(11, 0);
		full_scale = LDC1X1X_DATA_FULL_SCALE_131X;
	}

	/*
	 * The sensor only raises the conversion error flags for some of the
	 * ways a channel can fail, so an open LC tank can still come back
	 * flagless with the counter run all the way out. That is not a
	 * measurement, and reporting it as one hides a disconnected sensor.
	 */
	if (*sample >= full_scale) {
		LOG_ERR("channel %u result saturated, check the sensor connection", channel);
		return -EIO;
	}

	return 0;
}

static int ldc1x1x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ldc1x1x_config *cfg = dev->config;
	struct ldc1x1x_data *data = dev->data;
	uint8_t converted = 0U;
	int ret;

	ARG_UNUSED(chan);

	ret = ldc1x1x_wait_for_data(dev);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t channel = 0; channel < LDC1X1X_MAX_CHANNELS; channel++) {
		if (!ldc1x1x_channel_enabled(cfg, channel)) {
			continue;
		}

		ret = ldc1x1x_read_channel(dev, channel, &data->sample_buf[channel]);
		if (ret == -EIO) {
			/* One channel failing to convert says nothing about the
			 * others, so keep going and let ldc1x1x_channel_get()
			 * report it.
			 */
			data->ch_err |= BIT(channel);
			continue;
		}

		if (ret != 0) {
			return ret;
		}

		data->ch_err &= ~BIT(channel);
		converted++;
	}

	return converted > 0 ? 0 : -EIO;
}

static double ldc1x1x_frequency_hz(const struct device *dev, uint8_t channel)
{
	const struct ldc1x1x_config *cfg = dev->config;
	const struct ldc1x1x_channel_config *ch_cfg = &cfg->ch_cfg[channel];
	struct ldc1x1x_data *data = dev->data;
	double fref_hz = ((double)cfg->fref * 1000.0) / ch_cfg->fref_divider;
	double offset = (double)ch_cfg->offset / 65536.0;
	double full_scale;
	double ratio;

	if (data->ldc161x) {
		full_scale = (double)BIT64(28);
	} else {
		/* Gain 1, 4, 8 and 16 resolve 12, 14, 15 and 16 bits. */
		static const uint8_t gain_bits[] = {12U, 14U, 15U, 16U};

		full_scale = (double)BIT(gain_bits[cfg->output_gain]);
	}

	ratio = ((double)data->sample_buf[channel] / full_scale) + offset;

	return ch_cfg->fin_divider * fref_hz * ratio;
}

static int ldc1x1x_channel_to_index(const struct ldc1x1x_config *cfg, enum sensor_channel chan)
{
	switch ((int)chan) {
	case SENSOR_CHAN_LDC1X1X_FREQ_CH0:
		return 0;
	case SENSOR_CHAN_LDC1X1X_FREQ_CH1:
		return 1;
	case SENSOR_CHAN_LDC1X1X_FREQ_CH2:
		return 2;
	case SENSOR_CHAN_LDC1X1X_FREQ_CH3:
		return 3;
	case SENSOR_CHAN_FREQUENCY:
		if (cfg->autoscan) {
			return -ENOTSUP;
		}

		return cfg->active_channel;
	default:
		return -ENOTSUP;
	}
}

static int ldc1x1x_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ldc1x1x_config *cfg = dev->config;
	struct ldc1x1x_data *data = dev->data;
	int channel = ldc1x1x_channel_to_index(cfg, chan);

	if (channel < 0) {
		return channel;
	}

	if (!ldc1x1x_channel_enabled(cfg, (uint8_t)channel)) {
		return -ENOTSUP;
	}

	if ((data->ch_err & BIT(channel)) != 0U) {
		return -EIO;
	}

	return sensor_value_from_double(val, ldc1x1x_frequency_hz(dev, (uint8_t)channel));
}

static DEVICE_API(sensor, ldc1x1x_api) = {
	.sample_fetch = ldc1x1x_sample_fetch,
	.channel_get = ldc1x1x_channel_get,
#ifdef CONFIG_LDC1X1X_TRIGGER
	.trigger_set = ldc1x1x_trigger_set,
#endif
};

static int ldc1x1x_init_channels(const struct device *dev)
{
	const struct ldc1x1x_config *cfg = dev->config;

	for (uint8_t channel = 0; channel < LDC1X1X_MAX_CHANNELS; channel++) {
		const struct ldc1x1x_channel_config *ch_cfg = &cfg->ch_cfg[channel];
		int ret;

		if (!ldc1x1x_channel_enabled(cfg, channel)) {
			continue;
		}

		ret = ldc1x1x_set_fin_divider(dev, channel, ch_cfg->fin_divider);
		if (ret != 0) {
			return ret;
		}

		ret = ldc1x1x_set_fref_divider(dev, channel, ch_cfg->fref_divider);
		if (ret != 0) {
			return ret;
		}

		ret = ldc1x1x_set_idrive(dev, channel, ch_cfg->idrive);
		if (ret != 0) {
			return ret;
		}

		ret = ldc1x1x_set_settlecount(dev, channel, ch_cfg->settle_count);
		if (ret != 0) {
			return ret;
		}

		ret = ldc1x1x_set_rcount(dev, channel, ch_cfg->rcount);
		if (ret != 0) {
			return ret;
		}

		ret = ldc1x1x_set_offset(dev, channel, ch_cfg->offset);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int ldc1x1x_init_config(const struct device *dev)
{
	const struct ldc1x1x_config *cfg = dev->config;
	int ret;

	ret = ldc1x1x_init_channels(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_autoscan(dev, cfg->autoscan);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_rr_sequence(dev, cfg->rr_sequence);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_deglitch(dev, cfg->deglitch);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_active_channel(dev, cfg->active_channel);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_sensor_activate_sel(dev, cfg->sensor_activate_sel);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_rp_override(dev, cfg->rp_override);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_auto_amplitude_dis(dev, cfg->auto_amplitude_dis);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_ref_clk_src(dev, cfg->ref_clk_src);
	if (ret != 0) {
		return ret;
	}

	if (!((struct ldc1x1x_data *)dev->data)->ldc161x) {
		ret = ldc1x1x_set_output_gain(dev, cfg->output_gain);
		if (ret != 0) {
			return ret;
		}
	}

	/*
	 * The conversion error flags in DATAx_MSB stay clear unless the
	 * matching ERR2OUT bit is set, so a channel whose sensor never
	 * oscillates would otherwise report a full scale result instead of an
	 * error.
	 */
	ret = ldc1x1x_reg_write_mask(dev, LDC1X1X_ERROR_CONFIG,
				     LDC1X1X_ERROR_CONFIG_ERR2OUT_MASK,
				     LDC1X1X_ERROR_CONFIG_REPORTED);
	if (ret != 0) {
		return ret;
	}

	return ldc1x1x_set_current_drive(dev, cfg->current_drive);
}

static int ldc1x1x_probe(const struct device *dev)
{
	struct ldc1x1x_data *data = dev->data;
	uint16_t manufacturer_id;
	uint16_t device_id;
	int ret;

	ret = ldc1x1x_reg_read(dev, LDC1X1X_MANUFACTURER_ID, &manufacturer_id);
	if (ret != 0) {
		return ret;
	}

	if (manufacturer_id != LDC1X1X_MANUFACTURER_ID_VAL) {
		return -ENODEV;
	}

	ret = ldc1x1x_reg_read(dev, LDC1X1X_DEVICE_ID, &device_id);
	if (ret != 0) {
		return ret;
	}

	if (device_id == LDC1X1X_DEVICE_ID_LDC161X) {
		data->ldc161x = true;
		return 0;
	}

	if (device_id == LDC1X1X_DEVICE_ID_LDC131X) {
		data->ldc161x = false;
		return 0;
	}

	return -ENODEV;
}

static int ldc1x1x_validate_config(const struct ldc1x1x_config *cfg)
{
	uint8_t max_channels = cfg->ldc1x14 ? 4U : 2U;
	uint8_t scanned;

	/*
	 * Auto-Scan converts channel 0 up to the one selected by rr-sequence,
	 * and single-channel mode converts only the active channel. Requiring
	 * a node for exactly those channels keeps the devicetree honest about
	 * what the sensor is programmed to do.
	 */
	if (cfg->autoscan) {
		scanned = BIT_MASK(cfg->rr_sequence + 2);
	} else {
		scanned = BIT(cfg->active_channel);
	}

	if ((scanned & ~BIT_MASK(max_channels)) != 0U) {
		LOG_ERR("channels 0x%x are not available on this part", scanned);
		return -EINVAL;
	}

	if (cfg->channel_mask != scanned) {
		LOG_ERR("channel nodes 0x%x do not match the scanned channels 0x%x",
			cfg->channel_mask, scanned);
		return -EINVAL;
	}

	if (cfg->current_drive != 0U && (cfg->autoscan || cfg->active_channel != 0U)) {
		LOG_ERR("high current drive requires single-channel mode on channel 0");
		return -EINVAL;
	}

	return 0;
}

static int ldc1x1x_init_sd_pin(const struct device *dev)
{
	const struct ldc1x1x_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->sd_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->sd_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->sd_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	ldc1x1x_wait_wakeup();

	return 0;
}

static int ldc1x1x_init(const struct device *dev)
{
	const struct ldc1x1x_config *cfg = dev->config;
	int ret;

	ret = ldc1x1x_validate_config(cfg);
	if (ret != 0) {
		return ret;
	}

	if (cfg->sd_gpio.port != NULL) {
		ret = ldc1x1x_init_sd_pin(dev);
		if (ret != 0) {
			return ret;
		}
	}

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = ldc1x1x_probe(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_reset(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_init_config(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ldc1x1x_set_op_mode(dev, LDC1X1X_ACTIVE_MODE);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_LDC1X1X_TRIGGER
	ret = ldc1x1x_init_interrupt(dev);
	if (ret != 0) {
		return ret;
	}
#endif

	return 0;
}

#define LDC1X1X_CH_BIT(node_id) | BIT(DT_REG_ADDR(node_id))

#define LDC1X1X_CH_CFG_INIT(node_id)                                                               \
	[DT_REG_ADDR(node_id)] = {                                                                 \
		.rcount = DT_PROP(node_id, rcount),                                                \
		.offset = DT_PROP(node_id, offset),                                                \
		.settle_count = DT_PROP(node_id, settlecount),                                     \
		.fref_divider = DT_PROP(node_id, fref_divider),                                    \
		.idrive = DT_PROP(node_id, idrive),                                                \
		.fin_divider = DT_PROP(node_id, fin_divider),                                      \
	},

#define LDC1X1X_INT_GPIO(inst)                                                                     \
	IF_ENABLED(CONFIG_LDC1X1X_TRIGGER,						       \
		   (.intb_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, intb_gpios, {0}),))

#define LDC1X1X_DEFINE(inst)                                                                       \
	static uint32_t ldc1x1x_sample_buf_##inst[LDC1X1X_MAX_CHANNELS];                           \
                                                                                                   \
	static struct ldc1x1x_data ldc1x1x_data_##inst = {                                         \
		.sample_buf = ldc1x1x_sample_buf_##inst,                                           \
	};                                                                                         \
                                                                                                   \
	static const struct ldc1x1x_channel_config                                                 \
		ldc1x1x_ch_cfg_##inst[LDC1X1X_MAX_CHANNELS] = {                                    \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, LDC1X1X_CH_CFG_INIT)};             \
                                                                                                   \
	static const struct ldc1x1x_config ldc1x1x_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.sd_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sd_gpios, {0}),                          \
		.ldc1x14 = DT_INST_PROP(inst, ldc1x14),                                            \
		.autoscan = DT_INST_PROP(inst, autoscan),                                          \
		.rr_sequence = DT_INST_PROP(inst, rr_sequence),                                    \
		.active_channel = DT_INST_PROP(inst, active_channel),                              \
		.output_gain = DT_INST_PROP(inst, output_gain),                                    \
		.deglitch = DT_INST_PROP(inst, deglitch),                                          \
		.rp_override = DT_INST_PROP(inst, rp_override),                                    \
		.auto_amplitude_dis = DT_INST_PROP(inst, auto_amplitude_correction_disable),       \
		.sensor_activate_sel = DT_INST_ENUM_IDX(inst, sensor_activate_sel),                \
		.ref_clk_src = DT_INST_ENUM_IDX(inst, ref_clk_src),                                \
		.current_drive = DT_INST_ENUM_IDX(inst, current_drive),                            \
		.channel_mask =                                                                    \
			(0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, LDC1X1X_CH_BIT)),               \
		.fref = DT_INST_PROP(inst, fref),                                                  \
		.ch_cfg = ldc1x1x_ch_cfg_##inst,                                                   \
		LDC1X1X_INT_GPIO(inst)};                                                           \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, ldc1x1x_pm_action);                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ldc1x1x_init, PM_DEVICE_DT_INST_GET(inst),              \
				     &ldc1x1x_data_##inst, &ldc1x1x_config_##inst, POST_KERNEL,    \
				     CONFIG_SENSOR_INIT_PRIORITY, &ldc1x1x_api);

DT_INST_FOREACH_STATUS_OKAY(LDC1X1X_DEFINE)
