/*
 * Copyright (c) 2020 Google LLC.
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
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

/* clang-format off */

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
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint32_t gclk_gen;
	uint16_t gclk_id;
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

	*cfg->mclk |= cfg->mclk_mask;

#ifdef MCLK
	GCLK->PCHCTRL[cfg->gclk_id].reg = GCLK_PCHCTRL_CHEN
					| GCLK_PCHCTRL_GEN(cfg->gclk_gen);
#else
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN
			  | GCLK_CLKCTRL_GEN(cfg->gclk_gen)
			  | GCLK_CLKCTRL_ID(cfg->gclk_id);
#endif

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

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

#define ASSIGNED_CLOCKS_CELL_BY_NAME						\
	ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME

#define SAM0_DAC_REFSEL(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, reference),			\
		    (DT_INST_ENUM_IDX(n, reference)), (0))

#define SAM0_DAC_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	static const struct dac_sam0_cfg dac_sam0_cfg_##n = {			\
		.regs = (Dac *)DT_INST_REG_ADDR(n),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.gclk_gen = ASSIGNED_CLOCKS_CELL_BY_NAME(n, gclk, gen),		\
		.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, id),		\
		.mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n),		\
		.mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, bit),	\
		.refsel = UTIL_CAT(SAM0_DAC_REFSEL_, SAM0_DAC_REFSEL(n)),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &dac_sam0_init, NULL, NULL,			\
			    &dac_sam0_cfg_##n, POST_KERNEL,			\
			    CONFIG_DAC_INIT_PRIORITY,				\
			    &api_sam0_driver_api)

DT_INST_FOREACH_STATUS_OKAY(SAM0_DAC_INIT);

/* clang-format on */
