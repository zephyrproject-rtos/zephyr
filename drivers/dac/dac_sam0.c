/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_dac

#include <errno.h>

#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dac_sam0, CONFIG_DAC_LOG_LEVEL);

/*
 * Maps between the DTS reference property names and register values.  Note that
 * the ASF uses the 09/2015 names which differ from the 03/2020 datasheet.
 *
 * TODO(#21273): replace once improved support for enum values lands.
 */
#define SAM0_DAC_REFSEL_0 DAC_CTRLB_REFSEL_INT1V_Val
#define SAM0_DAC_REFSEL_1 DAC_CTRLB_REFSEL_AVCC_Val
#define SAM0_DAC_REFSEL_2 DAC_CTRLB_REFSEL_VREFP_Val

struct dac_sam0_cfg {
	Dac *regs;
	const struct pinctrl_dev_config *pcfg;
	uint8_t pm_apbc_bit;
	uint8_t gclk_clkctrl_id;
	uint8_t refsel;
};

/* Write to the DAC. */
static int dac_sam0_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct dac_sam0_cfg *const cfg = dev->config;
	Dac *regs = cfg->regs;

	if (value >= BIT(12)) {
		LOG_ERR("value %d out of range", value);
		return -EINVAL;
	}

	regs->DATA.reg = (uint16_t)value;

	return 0;
}

/*
 * Setup the channel.  As the SAM0 has one fixed width channel, this validates
 * the input and does nothing else.
 */
static int dac_sam0_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id != 0) {
		return -EINVAL;
	}
	if (channel_cfg->resolution != 10) {
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		return -ENOSYS;
	}

	return 0;
}

/* Initialise and enable the DAC. */
static int dac_sam0_init(const struct device *dev)
{
	const struct dac_sam0_cfg *const cfg = dev->config;
	Dac *regs = cfg->regs;
	int retval;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Enable the clock in PM */
	PM->APBCMASK.reg |= 1 << cfg->pm_apbc_bit;

	/* Reset then configure the DAC */
	regs->CTRLA.bit.SWRST = 1;
	while (regs->STATUS.bit.SYNCBUSY) {
	}

	regs->CTRLB.bit.REFSEL = cfg->refsel;
	regs->CTRLB.bit.EOEN = 1;

	/* Enable */
	regs->CTRLA.bit.ENABLE = 1;
	while (regs->STATUS.bit.SYNCBUSY) {
	}

	return 0;
}

static DEVICE_API(dac, api_sam0_driver_api) = {
	.channel_setup = dac_sam0_channel_setup,
	.write_value = dac_sam0_write_value
};

#define SAM0_DAC_REFSEL(n)						       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reference),		       \
		    (DT_INST_ENUM_IDX(n, reference)), (0))

#define SAM0_DAC_INIT(n)						       \
	PINCTRL_DT_INST_DEFINE(n);					       \
	static const struct dac_sam0_cfg dac_sam0_cfg_##n = {		       \
		.regs = (Dac *)DT_INST_REG_ADDR(n),			       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		       \
		.pm_apbc_bit = DT_INST_CLOCKS_CELL_BY_NAME(n, pm, bit),	       \
		.gclk_clkctrl_id =					       \
			DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, clkctrl_id),      \
		.refsel = UTIL_CAT(SAM0_DAC_REFSEL_, SAM0_DAC_REFSEL(n)),      \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n, &dac_sam0_init, NULL, NULL,		       \
			    &dac_sam0_cfg_##n, POST_KERNEL,		       \
			    CONFIG_DAC_INIT_PRIORITY,			       \
			    &api_sam0_driver_api)

DT_INST_FOREACH_STATUS_OKAY(SAM0_DAC_INIT);
