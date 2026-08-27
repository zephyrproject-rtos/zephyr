/*
 * Copyright (c) 2025 Daikin Comfort Technologies North America, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_samd5x_dac

#include <errno.h>

#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dac_samd5x, CONFIG_DAC_LOG_LEVEL);

#define DAC_CHANNEL_NO 2
#define DAC_RESOLUTION 12

#define SAMD5X_DAC_REFSEL_0 DAC_CTRLB_REFSEL_VREFPU
#define SAMD5X_DAC_REFSEL_1 DAC_CTRLB_REFSEL_VDDANA
#define SAMD5X_DAC_REFSEL_2 DAC_CTRLB_REFSEL_VREFPB
#define SAMD5X_DAC_REFSEL_3 DAC_CTRLB_REFSEL_INTREF

struct dac_samd5x_channel_cfg {
	uint8_t oversampling;    /* Oversampling ratio */
	uint8_t refresh_period;  /* Refresh period */
	bool run_in_standby;     /* Run in standby mode */
	uint8_t current_control; /* Current control */
};

struct dac_samd5x_cfg {
	Dac *regs;
	const struct pinctrl_dev_config *pcfg;
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint32_t gclk_gen;
	uint16_t gclk_id;
	uint8_t refsel;
	bool diff_mode;
	struct dac_samd5x_channel_cfg channel_cfg[DAC_CHANNEL_NO];
};

struct dac_samd5x_data {
	uint8_t resolution[DAC_CHANNEL_NO];
};

/* Write to the DAC. */
static int dac_samd5x_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_samd5x_cfg *const cfg = dev->config;
	struct dac_samd5x_data *data = dev->data;
	Dac *regs = cfg->regs;

	if (channel >= DAC_CHANNEL_NO) {
		return -EINVAL;
	}

	if (data->resolution[channel] > 12) {
		if (value >= BIT(16)) {
			LOG_ERR("value %d out of range", value);
			return -EINVAL;
		}
	} else {
		if (value >= BIT(12)) {
			LOG_ERR("value %d out of range", value);
			return -EINVAL;
		}
	}

	regs->DATA[channel].reg = (uint16_t)value;

	if (channel == 0) {
		while ((regs->SYNCBUSY.reg & DAC_SYNCBUSY_DATA0) == DAC_SYNCBUSY_DATA0) {
			/* Wait for synchronization */
		}
	} else {
		while ((regs->SYNCBUSY.reg & DAC_SYNCBUSY_DATA1) == DAC_SYNCBUSY_DATA1) {
			/* Wait for synchronization */
		}
	}

	return 0;
}

/*
 * Setup the channel. Validates the input id and resolution to match within
 * the samd5x/e5x parameters.
 */
static int dac_samd5x_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_samd5x_cfg *const cfg = dev->config;
	struct dac_samd5x_data *data = dev->data;
	Dac *regs = cfg->regs;

	if (channel_cfg->channel_id >= DAC_CHANNEL_NO) {
		return -EINVAL;
	}
	if ((channel_cfg->resolution != 12) && (channel_cfg->resolution != 16)) {
		return -ENOTSUP;
	}
	if (channel_cfg->internal) {
		return -ENOSYS;
	}

	/* Disable DAC */
	regs->CTRLA.reg = DAC_CTRLA_RESETVALUE;
	while (((regs->SYNCBUSY.reg & DAC_SYNCBUSY_ENABLE) == DAC_SYNCBUSY_ENABLE)) {
		/* Wait for synchronization */
	}

	/* Enable dithering for 16-bit resolution */
	if (channel_cfg->resolution == 16) {
		regs->DACCTRL[channel_cfg->channel_id].reg |= DAC_DACCTRL_DITHER;
		data->resolution[channel_cfg->channel_id] = 16;
	}

	/* Enable channel */
	regs->DACCTRL[channel_cfg->channel_id].reg |= DAC_DACCTRL_ENABLE;

	/* Enable DAC */
	regs->CTRLA.reg = DAC_CTRLA_ENABLE;
	while (((regs->SYNCBUSY.reg & DAC_SYNCBUSY_ENABLE) == DAC_SYNCBUSY_ENABLE)) {
		/* Wait for synchronization */
	}

	return 0;
}

/* Initialize and enable DAC and channels properties. */
static int dac_samd5x_init(const struct device *dev)
{
	const struct dac_samd5x_cfg *const cfg = dev->config;
	Dac *regs = cfg->regs;
	int retval;

	*cfg->mclk |= cfg->mclk_mask;

#ifdef MCLK
	GCLK->PCHCTRL[cfg->gclk_id].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(cfg->gclk_gen);
#else
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(cfg->gclk_gen) |
			    GCLK_CLKCTRL_ID(cfg->gclk_id);
#endif

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Reset then configure the DAC */
	regs->CTRLA.reg = DAC_CTRLA_SWRST;
	while (((regs->CTRLA.reg & DAC_CTRLA_SWRST) == DAC_CTRLA_SWRST) &&
	       ((regs->SYNCBUSY.reg & DAC_SYNCBUSY_SWRST) == DAC_SYNCBUSY_SWRST)) {
		/* Wait for synchronization */
	}

	regs->CTRLB.reg = cfg->refsel;
	if (cfg->diff_mode) {
		regs->CTRLB.reg |= DAC_CTRLB_DIFF;
	}

	/* Configure each channel */
	for (int i = 0; i < DAC_CHANNEL_NO; i++) {
		regs->DACCTRL[i].reg = (
			DAC_DACCTRL_OSR(cfg->channel_cfg[i].oversampling) |
			DAC_DACCTRL_REFRESH(cfg->channel_cfg[i].refresh_period) |
			(cfg->channel_cfg[i].run_in_standby ? DAC_DACCTRL_RUNSTDBY : 0) |
			DAC_DACCTRL_CCTRL(cfg->channel_cfg[i].current_control));
	}

	/* Enable DAC */
	regs->CTRLA.reg = DAC_CTRLA_ENABLE;
	while (((regs->SYNCBUSY.reg & DAC_SYNCBUSY_ENABLE) == DAC_SYNCBUSY_ENABLE)) {
		/* Wait for synchronization */
	}

	return 0;
}

static DEVICE_API(dac, dac_samd5x_driver_api) = {
	.channel_setup = dac_samd5x_channel_setup,
	.write_value = dac_samd5x_write_value
};

/* Helper macros for device tree properties */
#define SAMD5X_DAC_REFSEL(n) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reference), \
				(DT_INST_ENUM_IDX(n, reference)), (0))

#define SAMD5X_DAC_DIFF_MODE(n) DT_INST_PROP_OR(n, differential - mode, 0)

/* Channel configuration macros */
#define CHANNEL_CFG_DEF(n) \
	{.oversampling = DT_INST_ENUM_IDX_OR(n, oversampling, 0), \
	 .refresh_period = DT_PROP_OR(n, refresh_period, 0), \
	 .run_in_standby = DT_PROP_OR(n, run_in_standby, 0), \
	 .current_control = DT_INST_ENUM_IDX_OR(n, current_control, 0)}

/* Device initialization macro */
#define SAMD5X_DAC_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct dac_samd5x_data dac_samd5x_data_##n; \
	static const struct dac_samd5x_cfg dac_samd5x_cfg_##n = { \
		.regs = (Dac *)DT_INST_REG_ADDR(n), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.gclk_gen = ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME(n, gclk, gen), \
		.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, id), \
		.mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n), \
		.mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, bit), \
		.refsel = UTIL_CAT(SAMD5X_DAC_REFSEL_, SAMD5X_DAC_REFSEL(n)), \
		.diff_mode = SAMD5X_DAC_DIFF_MODE(n), \
		.channel_cfg = {DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, CHANNEL_CFG_DEF, (,))}, \
	}; \
	DEVICE_DT_INST_DEFINE(n, &dac_samd5x_init, NULL, &dac_samd5x_data_##n, \
			      &dac_samd5x_cfg_##n, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY, \
			      &dac_samd5x_driver_api)

/* Create all instances */
DT_INST_FOREACH_STATUS_OKAY(SAMD5X_DAC_INIT);
