/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_dac_g1
LOG_MODULE_REGISTER(dac_mchp_g1, CONFIG_DAC_LOG_LEVEL);

#define DAC_MAX_CHANNELS   DT_PROP(DT_NODELABEL(dac), max_channels)
#define DAC_CC100K         100
#define DAC_CC1M           500
#define DAC_CC12M          1000
#define DAC_REFRESH_PERIOD 30
#define DAC_RESOLUTION     12
#define DAC_DATA_MSB_MASK  (0x0FFFU)
#define DAC_DATA_LSB_MASK  (0xFFF0U)
#define DAC_DATA_RIGHT_ADJ 0
#define DAC_DATA_LEFT_ADJ  1
#define DAC_CHANNELS_ALL   0xFF
#define DAC_OSR_RATIO_1    1
#define DAC_OSR_RATIO_2    2
#define DAC_OSR_RATIO_4    4
#define DAC_OSR_RATIO_8    8
#define DAC_OSR_RATIO_16   16
#define DAC_OSR_RATIO_32   32
#define TIMEOUT_VALUE_US   1000
#define DELAY_US           2

struct dac_mchp_channel {
	uint8_t channel;
	int rate;
	bool ext_filter;
	uint8_t data_adj;
	bool dither;
	int sampling_ratio;
	int refresh;
};

struct dac_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct dac_mchp_dev_config {
	dac_registers_t *regs;
	uint8_t refsel;
	const struct pinctrl_dev_config *pcfg;
	struct dac_mchp_clock dac_clock;
	struct dac_mchp_channel channels[DAC_MAX_CHANNELS];
};

struct dac_mchp_dev_data {
	bool is_configured[DAC_MAX_CHANNELS];
};

static inline void dac_wait_sync(dac_registers_t *dac_reg, uint32_t sync_flag)
{
	if (WAIT_FOR(((dac_reg->DAC_SYNCBUSY & sync_flag) == 0U), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for DAC_SYNCBUSY bits to clear");
	}
}

static void dac_wait_ready(dac_registers_t *dac_reg, uint8_t channel_id)
{
	uint32_t mask;

	/* Return early if DAC is not enabled */
	if ((dac_reg->DAC_CTRLA & DAC_CTRLA_ENABLE_Msk) == 0U) {
		return;
	}

	if (channel_id == DAC_CHANNELS_ALL) {
		mask = DAC_STATUS_READY0_Msk | DAC_STATUS_READY1_Msk;
	} else {
		mask = (channel_id == 1U) ? DAC_STATUS_READY1_Msk : DAC_STATUS_READY0_Msk;
	}

	if (WAIT_FOR(((dac_reg->DAC_STATUS & mask) == mask), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for DAC_STATUS_READY (mask=0x%x)", mask);
	}
}

static inline void dac_enable_controller(dac_registers_t *dac_reg)
{
	dac_reg->DAC_CTRLA |= DAC_CTRLA_ENABLE_Msk;
	dac_wait_sync(dac_reg, DAC_SYNCBUSY_ENABLE_Msk);
}

static inline void dac_disable_controller(dac_registers_t *dac_reg)
{
	dac_reg->DAC_CTRLA &= ~DAC_CTRLA_ENABLE_Msk;
	dac_wait_sync(dac_reg, DAC_SYNCBUSY_ENABLE_Msk);
}

static inline void dac_reset(dac_registers_t *dac_reg)
{
	dac_reg->DAC_CTRLA = DAC_CTRLA_SWRST_Msk;
	dac_wait_sync(dac_reg, DAC_SYNCBUSY_SWRST_Msk);
}

static inline void dac_set_diff_output(dac_registers_t *dac_reg)
{
#if defined(CONFIG_DAC_MCHP_G1_DIFFERENTIAL)

	dac_reg->DAC_CTRLB = DAC_CTRLB_DIFF_Msk;

#endif /* CONFIG_DAC_MCHP_G1_DIFFERENTIAL */
}

static inline void dac_ref_selection(dac_registers_t *dac_reg, uint8_t refsel)
{
	dac_reg->DAC_CTRLB =
		(dac_reg->DAC_CTRLB & ~DAC_CTRLB_REFSEL_Msk) | DAC_CTRLB_REFSEL(refsel);
}

static inline void dac_channel_enable(dac_registers_t *dac_reg, uint8_t channel_id)
{
	dac_reg->DAC_DACCTRL[channel_id] |= DAC_DACCTRL_ENABLE_Msk;
}

static void dac_conversion_speed(dac_registers_t *dac_reg, int rate, uint8_t channel_id)
{
	uint32_t cctrl_val;

	switch (rate) {
	case DAC_CC100K:
		cctrl_val = DAC_DACCTRL_CCTRL_CC100K;
		break;
	case DAC_CC1M:
		cctrl_val = DAC_DACCTRL_CCTRL_CC1M;
		break;
	case DAC_CC12M:
		cctrl_val = DAC_DACCTRL_CCTRL_CC12M;
		break;
	default:
		LOG_WRN("Invalid DAC conversion rate (%d), defaulting to DAC_CC100K", rate);
		cctrl_val = DAC_DACCTRL_CCTRL_CC100K;
		break;
	}

	dac_reg->DAC_DACCTRL[channel_id] =
		(dac_reg->DAC_DACCTRL[channel_id] & ~DAC_DACCTRL_CCTRL_Msk) | cctrl_val;
}

static inline void dac_external_filter(dac_registers_t *dac_reg, bool ext_filter,
				       uint8_t channel_id)
{
	dac_reg->DAC_DACCTRL[channel_id] =
		(dac_reg->DAC_DACCTRL[channel_id] & ~DAC_DACCTRL_FEXT_Msk) |
		DAC_DACCTRL_FEXT(ext_filter);
}

static inline void dac_data_adj(dac_registers_t *dac_reg, uint8_t data_adj, uint8_t channel_id)
{
	dac_reg->DAC_DACCTRL[channel_id] =
		(dac_reg->DAC_DACCTRL[channel_id] & ~DAC_DACCTRL_LEFTADJ_Msk) |
		DAC_DACCTRL_LEFTADJ(data_adj);
}

static inline void dac_dither(dac_registers_t *dac_reg, uint8_t dither, uint8_t channel_id)
{
	dac_reg->DAC_DACCTRL[channel_id] =
		(dac_reg->DAC_DACCTRL[channel_id] & ~DAC_DACCTRL_DITHER_Msk) |
		DAC_DACCTRL_DITHER(dither);
}

static inline void dac_refresh(dac_registers_t *dac_reg, uint8_t refresh, uint8_t channel_id)
{
	if (refresh != 0) {
		refresh = refresh / DAC_REFRESH_PERIOD;
	}
	dac_reg->DAC_DACCTRL[channel_id] =
		(dac_reg->DAC_DACCTRL[channel_id] & ~DAC_DACCTRL_REFRESH_Msk) |
		DAC_DACCTRL_REFRESH(refresh);
}

static void dac_sampling_ratio(dac_registers_t *dac_reg, uint8_t sampling_ratio, uint8_t channel_id)
{
	uint8_t osr;

	switch (sampling_ratio) {
	case DAC_OSR_RATIO_2:
		osr = DAC_DACCTRL_OSR_OSR_2_Val;
		break;
	case DAC_OSR_RATIO_4:
		osr = DAC_DACCTRL_OSR_OSR_4_Val;
		break;
	case DAC_OSR_RATIO_8:
		osr = DAC_DACCTRL_OSR_OSR_8_Val;
		break;
	case DAC_OSR_RATIO_16:
		osr = DAC_DACCTRL_OSR_OSR_16_Val;
		break;
	case DAC_OSR_RATIO_32:
		osr = DAC_DACCTRL_OSR_OSR_32_Val;
		break;
	default:
		osr = DAC_DACCTRL_OSR_OSR_1_Val;
		break;
	}

	dac_reg->DAC_DACCTRL[channel_id] =
		(dac_reg->DAC_DACCTRL[channel_id] & ~DAC_DACCTRL_OSR_Msk) | DAC_DACCTRL_OSR(osr);
}

static void dac_write_channel(dac_registers_t *dac_reg, const struct dac_mchp_channel *ch_cfg,
			      uint8_t channel_id, uint32_t value)
{
	uint32_t data;

	if (ch_cfg->data_adj == DAC_DATA_LEFT_ADJ) {
		data = DAC_DATA_LSB_MASK & DAC_DATA_DATA(value);
	} else {
		data = DAC_DATA_MSB_MASK & DAC_DATA_DATA(value);
	}

	dac_reg->DAC_DATA[channel_id] = data;

	dac_wait_sync(dac_reg,
		      (channel_id == 0U) ? DAC_SYNCBUSY_DATA0_Msk : DAC_SYNCBUSY_DATA1_Msk);
}

static void dac_write_data(const struct device *dev, uint8_t channel_id, uint32_t value)
{
	const struct dac_mchp_dev_config *dev_cfg = dev->config;

	if (channel_id == DAC_CHANNELS_ALL) {
		dac_write_channel(dev_cfg->regs, &dev_cfg->channels[0], 0U, value);

		dac_write_channel(dev_cfg->regs, &dev_cfg->channels[1], 1U, value);
	} else {
		dac_write_channel(dev_cfg->regs, &dev_cfg->channels[channel_id], channel_id, value);
	}
}

static int dac_configure(const struct device *dev, uint8_t channel_id)
{
	const struct dac_mchp_dev_config *dev_cfg = dev->config;
	uint8_t i = 0, start = 0, end = 0;

#if defined(CONFIG_DAC_MCHP_G1_DIFFERENTIAL)
	/* If differential is selected, we can use only channel 0 */
	if (channel_cfg->channel_id != 0) {
		return -EINVAL;
	}
#endif /* CONFIG_DAC_MCHP_G1_DIFFERENTIAL */
	/*
	 * Determine the range of channels to configure.
	 * If channel_id is 0xFF, configure all DAC channels by iterating
	 * from channel 0 to DAC_MAX_CHANNELS - 1.
	 * Otherwise, configure only the specified channel.
	 */
	if (channel_id == DAC_CHANNELS_ALL) {
		start = 0;
		end = DAC_MAX_CHANNELS;
	} else {
		start = channel_id;
		end = channel_id + 1;
	}

	for (i = start; i < end; i++) {
		/* Enable the DAC for channels */
		dac_channel_enable(dev_cfg->regs, i);

		/* Set the DATA Adjustment */
		dac_data_adj(dev_cfg->regs, dev_cfg->channels[i].data_adj, i);

		/* Set the Dither */
		dac_dither(dev_cfg->regs, dev_cfg->channels[i].dither, i);

		/* Set the refresh period */
		if (dev_cfg->channels[i].sampling_ratio != 1) {
			dac_refresh(dev_cfg->regs, 0, i);
		} else {
			dac_refresh(dev_cfg->regs, dev_cfg->channels[i].refresh, i);
		}
		/* Set the conversion speed */
		dac_conversion_speed(dev_cfg->regs, dev_cfg->channels[i].rate, i);

		/* Set the External filter */
		dac_external_filter(dev_cfg->regs, dev_cfg->channels[i].ext_filter, i);

		/* Set the Oversampling Ratio */
		dac_sampling_ratio(dev_cfg->regs, dev_cfg->channels[i].sampling_ratio, i);
	}

	return 0;
}

static int dac_mchp_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_mchp_dev_config *dev_cfg = dev->config;
	struct dac_mchp_dev_data *const data = dev->data;
	int ret;
	int8_t i;

	/* Disable the Controller */
	dac_disable_controller(dev_cfg->regs);

	if (channel_cfg->resolution != DAC_RESOLUTION || (channel_cfg->internal == 1) ||
	    (channel_cfg->buffered == 1)) {
		LOG_ERR("Unsupported configuration!");
		return -ENOTSUP;
	}

	/* Channel ID validity */
	if ((channel_cfg->channel_id >= DAC_MAX_CHANNELS) &&
	    (channel_cfg->channel_id != DAC_CHANNELS_ALL)) {
		LOG_ERR("Invalid Channel!");
		return -EINVAL;
	}

	/* Configure the DAC channel(s) */
	ret = dac_configure(dev, channel_cfg->channel_id);
	if (ret != 0) {
		return ret;
	}

	/* Enable the DAC */
	dac_enable_controller(dev_cfg->regs);

	/* Wait for ready state */
	dac_wait_ready(dev_cfg->regs, channel_cfg->channel_id);

	/* Mark configuration status */
	if (channel_cfg->channel_id == DAC_CHANNELS_ALL) {
		for (i = 0; i < DAC_MAX_CHANNELS; i++) {
			data->is_configured[i] = true;
		}
	} else {
		data->is_configured[channel_cfg->channel_id] = true;
	}

	return 0;
}

static int dac_mchp_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dac_mchp_dev_data *const data = dev->data;

	/* Validate ALL channels */
	if (channel == DAC_CHANNELS_ALL) {
		for (int i = 0; i < DAC_MAX_CHANNELS; i++) {
			if (data->is_configured[i] == false) {
				LOG_ERR("DAC write failed: channel %d not configured", i);
				return -EINVAL;
			}
		}
	}
	/* Validate SINGLE channel */
	else {
		if (channel >= DAC_MAX_CHANNELS) {
			LOG_ERR("DAC write failed: invalid channel %u", channel);
			return -EINVAL;
		}

		if (data->is_configured[channel] == false) {
			LOG_ERR("DAC write failed: channel %u not configured", channel);
			return -EINVAL;
		}
	}

	dac_write_data(dev, channel, value);

	return 0;
}

static int dac_mchp_init(const struct device *dev)
{
	const struct dac_mchp_dev_config *dev_cfg = dev->config;
	int ret;

	/* Enable GCLK */
	ret = clock_control_on(dev_cfg->dac_clock.clock_dev, dev_cfg->dac_clock.gclk_sys);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable the GCLK for DAC: %d", ret);
		return 0;
	}

	/* Enable MCLK */
	ret = clock_control_on(dev_cfg->dac_clock.clock_dev, dev_cfg->dac_clock.mclk_sys);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable the MCLK for DAC: %d", ret);
		return 0;
	}

	ret = (ret == -EALREADY) ? 0 : ret;

	pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	dac_reset(dev_cfg->regs);
	dac_disable_controller(dev_cfg->regs);
	dac_set_diff_output(dev_cfg->regs);
	dac_ref_selection(dev_cfg->regs, dev_cfg->refsel);

	return 0;
}

static DEVICE_API(dac, dac_mchp_api) = {.channel_setup = dac_mchp_channel_setup,
					.write_value = dac_mchp_write_value};

#define DAC_MCHP_CHANNEL_DEFN(child)                                                               \
	[DT_REG_ADDR(child)] = {.channel = DT_REG_ADDR(child),                                     \
				.rate = DT_PROP(child, rate),                                      \
				.ext_filter = DT_PROP(child, ext_filter),                          \
				.data_adj = DT_ENUM_IDX(child, data_adj),                          \
				.dither = DT_PROP(child, dither_mode),                             \
				.sampling_ratio = DT_PROP(child, sampling_ratio),                  \
				.refresh = DT_PROP(child, refresh_period)}

/* clang-format off */
#define DAC_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct dac_mchp_dev_config dac_mchp_config_##n = {                            \
		.regs = (dac_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.refsel = DT_ENUM_IDX(DT_DRV_INST(n), refsel),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.channels = {DT_FOREACH_CHILD_SEP(DT_DRV_INST(n), DAC_MCHP_CHANNEL_DEFN, (,))},    \
		.dac_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                         \
		.dac_clock.mclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),     \
		.dac_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),   \
	}
/* clang-format on */

#define DAC_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	DAC_MCHP_CONFIG_DEFN(n);                                                                   \
	static struct dac_mchp_dev_data dac_mchp_data_##n = {};                                    \
	DEVICE_DT_INST_DEFINE(n, dac_mchp_init, NULL, &dac_mchp_data_##n, &dac_mchp_config_##n,    \
			      POST_KERNEL, CONFIG_DAC_INIT_PRIORITY, &dac_mchp_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MCHP_DEVICE_INIT)
