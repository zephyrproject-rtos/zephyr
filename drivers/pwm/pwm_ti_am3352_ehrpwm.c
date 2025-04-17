/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ti_ehrpwm);

#define DT_DRV_COMPAT ti_am3352_ehrpwm

#define TI_EHRPWM_PERIOD_CYCLES_MAX (0xFFFF)
#define TI_EHRPWM_NUM_CHANNELS      (2)

struct ti_ehrpwm_regs {
	volatile uint16_t TBCTL;
	uint8_t RESERVED_1[0x8];
	volatile uint16_t TBPRD;
	uint8_t RESERVED_2[0x6];
	volatile uint16_t CMPA;
	volatile uint16_t CMPB;
	volatile uint16_t AQCTLA;
	volatile uint16_t AQCTLB;
	volatile uint16_t AQSFRC;
	volatile uint16_t AQCSFRC;
};

/* Time Based Control Register */
#define TI_EHRPWM_TBCTL_CLKDIV          GENMASK(12, 10)
#define TI_EHRPWM_TBCTL_CLKDIV_MAX      (7)
#define TI_EHRPWM_TBCTL_HSPCLKDIV       GENMASK(9, 7)
#define TI_EHRPWM_TBCTL_HSPCLKDIV_MAX   (7)
#define TI_EHRPWM_TBCTL_PRDLD           BIT(3)
#define TI_EHRPWM_TBCTL_CTRMODE         GENMASK(1, 0)
#define TI_EHRPWM_TBCTL_CTRMODE_UP_ONLY (0)
#define TI_EHRPWM_TBCTL_CTRMODE_UP_DOWN (2)

/* Action Qualifier Control Register */
#define TI_EHRPWM_AQCTL_CBD GENMASK(11, 10)
#define TI_EHRPWM_AQCTL_CBU GENMASK(9, 8)
#define TI_EHRPWM_AQCTL_CAD GENMASK(7, 6)
#define TI_EHRPWM_AQCTL_CAU GENMASK(5, 4)
#define TI_EHRPWM_AQCTL_PRD GENMASK(3, 2)
#define TI_EHRPWM_AQCTL_ZRO GENMASK(1, 0)

/* Action Qualifier Software Force Register */
#define TI_EHRPWM_AQSFRC_RLDCSF GENMASK(7, 6)

/* Action Qualifier Continuous Software Force Register */
#define TI_EHRPWM_AQCSFRC_CSFB    GENMASK(3, 2)
#define TI_EHRPWM_AQCSFRC_CSFA    GENMASK(1, 0)
#define TI_EHRPWM_AQCSFRC_CSF_LOW (1)

/* Action Qualifier Control Register */
#define TI_EHRPWM_AQCTL_ZRO     GENMASK(1, 0)
#define TI_EHRPWM_AQCTL_PRD     GENMASK(3, 2)
#define TI_EHRPWM_AQCTL_CAU     GENMASK(5, 4)
#define TI_EHRPWM_AQCTL_CBU     GENMASK(9, 8)
#define TI_EHRPWM_AQCTL_FLD_CLR (1)
#define TI_EHRPWM_AQCTL_FLD_SET (2)

#define DEV_CFG(dev)  ((const struct ti_ehrpwm_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct ti_ehrpwm_data *)(dev)->data)
#define DEV_REGS(dev) ((struct ti_ehrpwm_regs *)DEVICE_MMIO_GET(dev))

struct ti_ehrpwm_cfg {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct device *tbclk;
	clock_control_subsys_t tbclk_subsys;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
};

struct ti_ehrpwm_data {
	DEVICE_MMIO_RAM;
	uint32_t period_cycles[TI_EHRPWM_NUM_CHANNELS];
	uint32_t prescale_div[TI_EHRPWM_NUM_CHANNELS];
	bool symmetric;
	bool enabled;
};

static int ti_ehrpwm_configure_tbctl(const struct device *dev, uint32_t channel,
				     uint32_t period_cycles)
{
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	uint16_t prescale_div;
	uint16_t tbctl;

	/* already configured */
	if (data->period_cycles[channel] == period_cycles) {
		return 0;
	}

	tbctl = regs->TBCTL;

	/* configure shadow loading on period register (=0h) */
	tbctl &= ~TI_EHRPWM_TBCTL_PRDLD;

	/* configure counter mode */
	tbctl &= ~TI_EHRPWM_TBCTL_CTRMODE;
	if (data->symmetric) {
		tbctl |= FIELD_PREP(TI_EHRPWM_TBCTL_CTRMODE, TI_EHRPWM_TBCTL_CTRMODE_UP_DOWN);
	} else {
		tbctl |= FIELD_PREP(TI_EHRPWM_TBCTL_CTRMODE, TI_EHRPWM_TBCTL_CTRMODE_UP_ONLY);
	}

	/* find the minimum prescaler that will allow configuring period_cycles */
	for (uint16_t clkdiv = 0; clkdiv <= TI_EHRPWM_TBCTL_CLKDIV_MAX; clkdiv++) {
		for (uint16_t hspclkdiv = 0; hspclkdiv <= TI_EHRPWM_TBCTL_HSPCLKDIV_MAX;
		     hspclkdiv++) {

			prescale_div = (1 << clkdiv) * (hspclkdiv ? (hspclkdiv * 2) : 1);

			if (prescale_div > period_cycles / TI_EHRPWM_PERIOD_CYCLES_MAX) {
				tbctl &= ~(TI_EHRPWM_TBCTL_CLKDIV | TI_EHRPWM_TBCTL_HSPCLKDIV);
				tbctl |= FIELD_PREP(TI_EHRPWM_TBCTL_HSPCLKDIV, hspclkdiv) |
					 FIELD_PREP(TI_EHRPWM_TBCTL_CLKDIV, clkdiv);

				/* write tbctl */
				regs->TBCTL = tbctl;

				/* save period_cycles */
				data->period_cycles[channel] = period_cycles;
				data->prescale_div[channel] = prescale_div;

				/* early return */
				return 0;
			}
		}
	}

	/* period is too long for configuration */
	return -1;
}

static int ti_ehrpwm_configure_aq(const struct device *dev, uint32_t channel, bool polarity)
{
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);
	uint16_t aqctl_upmask;
	uint16_t aqctl_downmask;
	uint16_t aqctl;

	if (channel == 0) {
		aqctl = regs->AQCTLA;
		aqctl_upmask = TI_EHRPWM_AQCTL_CAU;
		aqctl_downmask = TI_EHRPWM_AQCTL_CAD;
	} else {
		aqctl = regs->AQCTLB;
		aqctl_upmask = TI_EHRPWM_AQCTL_CBU;
		aqctl_downmask = TI_EHRPWM_AQCTL_CBD;
	}

	aqctl &= ~(TI_EHRPWM_AQCTL_ZRO | TI_EHRPWM_AQCTL_PRD | aqctl_upmask | aqctl_downmask);
	if (polarity == PWM_POLARITY_NORMAL) {
		/* active-high */
		aqctl |= FIELD_PREP(aqctl_upmask, TI_EHRPWM_AQCTL_FLD_CLR);

		aqctl &= ~TI_EHRPWM_AQCTL_PRD;

		if (data->symmetric) {
			aqctl &= ~TI_EHRPWM_AQCTL_ZRO;
			aqctl |= FIELD_PREP(aqctl_downmask, TI_EHRPWM_AQCTL_FLD_SET);
		} else {
			aqctl |= FIELD_PREP(TI_EHRPWM_AQCTL_ZRO, TI_EHRPWM_AQCTL_FLD_SET);
			aqctl &= ~aqctl_downmask;
		}
	} else {
		/* active-low */
		aqctl |= FIELD_PREP(aqctl_upmask, TI_EHRPWM_AQCTL_FLD_SET);

		aqctl &= ~TI_EHRPWM_AQCTL_ZRO;

		if (data->symmetric) {
			aqctl &= ~TI_EHRPWM_AQCTL_PRD;
			aqctl |= FIELD_PREP(aqctl_downmask, TI_EHRPWM_AQCTL_FLD_CLR);
		} else {
			aqctl |= FIELD_PREP(TI_EHRPWM_AQCTL_PRD, TI_EHRPWM_AQCTL_FLD_CLR);
			aqctl &= ~aqctl_downmask;
		}
	}

	if (channel == 0) {
		regs->AQCTLA = aqctl;
	} else {
		regs->AQCTLB = aqctl;
	}

	return 0;
}

static int ti_ehrpwm_enable(const struct device *dev, uint32_t channel)
{
	const struct ti_ehrpwm_cfg *cfg = DEV_CFG(dev);
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);
	uint16_t aqcsfrc;
	int err;

	/* already enabled */
	if (data->enabled == true) {
		return 0;
	}

	/* disable forced action qualifier */
	aqcsfrc = regs->AQCSFRC;
	if (channel == 0) {
		aqcsfrc &= ~TI_EHRPWM_AQCSFRC_CSFA;
	} else {
		aqcsfrc &= ~TI_EHRPWM_AQCSFRC_CSFB;
	}

	/* configure shadow register */
	regs->AQSFRC &= ~TI_EHRPWM_AQSFRC_RLDCSF;
	regs->AQCSFRC = aqcsfrc;

	/* enable TBCLK */
	err = clock_control_on(cfg->tbclk, cfg->tbclk_subsys);
	if (err != 0) {
		LOG_ERR("failed to enable tbclk");
		return err;
	}

	data->enabled = true;

	return 0;
}

static int ti_ehrpwm_disable(const struct device *dev, uint32_t channel)
{
	const struct ti_ehrpwm_cfg *cfg = DEV_CFG(dev);
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);
	uint16_t aqcsfrc;
	int err;

	/* already disabled */
	if (data->enabled == false) {
		return 0;
	}

	/* force continuous low on aq submodule */
	aqcsfrc = regs->AQCSFRC;
	if (channel == 0) {
		aqcsfrc &= ~TI_EHRPWM_AQCSFRC_CSFA;
		aqcsfrc |= FIELD_PREP(TI_EHRPWM_AQCSFRC_CSFA, TI_EHRPWM_AQCSFRC_CSF_LOW);
	} else {
		aqcsfrc &= ~TI_EHRPWM_AQCSFRC_CSFB;
		aqcsfrc |= FIELD_PREP(TI_EHRPWM_AQCSFRC_CSFB, TI_EHRPWM_AQCSFRC_CSF_LOW);
	}

	/* configure shadow register */
	regs->AQSFRC &= ~TI_EHRPWM_AQSFRC_RLDCSF;
	regs->AQCSFRC = aqcsfrc;

	/* configure active register */
	regs->AQSFRC |= TI_EHRPWM_AQSFRC_RLDCSF;
	regs->AQCSFRC = aqcsfrc;

	/* disable TBCLK */
	err = clock_control_off(cfg->tbclk, cfg->tbclk_subsys);
	if (err != 0) {
		LOG_ERR("failed to disable tbclk");
		return err;
	}

	data->period_cycles[channel] = 0;
	data->enabled = false;

	return 0;
}

static int ti_ehrpwm_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
				uint32_t pulse_cycles, pwm_flags_t flags)
{
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	int err = 0;

	if (channel >= TI_EHRPWM_NUM_CHANNELS) {
		LOG_ERR("invalid channel number %u", channel);
		return -EINVAL;
	}

	/* there is a common period register, so the period should be same for all channels */
	for (uint32_t i = 0; i < TI_EHRPWM_NUM_CHANNELS; i++) {
		if (i != channel && data->period_cycles[i] != 0 &&
		    data->period_cycles[i] != period_cycles) {
			LOG_ERR("period value must be same as other channels");
			return -EINVAL;
		}
	}

	/* force constant low */
	if (period_cycles == 0) {
		err = ti_ehrpwm_disable(dev, channel);
		if (err != 0) {
			LOG_ERR("failed to disable ehrpwm module");
			return err;
		}

		/* early return after disable */
		return 0;
	}

	err = ti_ehrpwm_enable(dev, channel);
	if (err != 0) {
		return err;
	}

	/* configure action qualifier */
	err = ti_ehrpwm_configure_aq(dev, channel, flags & PWM_POLARITY_MASK);
	if (err != 0) {
		LOG_ERR("failed to configure action qualifier");
		return err;
	}

	/* configure tbctl and prescaler */
	err = ti_ehrpwm_configure_tbctl(dev, channel, period_cycles);
	if (err != 0) {
		LOG_ERR("failed to configure clock prescaler values");
		return err;
	}

	/* update cycles */
	period_cycles /= data->prescale_div[channel];
	pulse_cycles /= data->prescale_div[channel];

	if (data->symmetric) {
		period_cycles /= 2;
		pulse_cycles /= 2;
	}

	/* write period cycles */
	regs->TBPRD = period_cycles;

	/* write duty cycles */
	if (channel == 0) {
		regs->CMPA = pulse_cycles;
	} else {
		regs->CMPB = pulse_cycles;
	}

	return 0;
}

static int ti_ehrpwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					uint64_t *cycles)
{
	ARG_UNUSED(channel);
	const struct ti_ehrpwm_cfg *cfg = DEV_CFG(dev);

	if (cfg->clock_dev != NULL) {
		return clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys,
					      (uint32_t *)cycles);
	}

	if (cfg->clock_frequency != 0U) {
		*cycles = cfg->clock_frequency;
		return 0;
	}

	return -ENOTSUP;
}

static int ti_ehrpwm_init(const struct device *dev)
{
	const struct ti_ehrpwm_cfg *cfg = DEV_CFG(dev);
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to configure pinctrl\n");
		return ret;
	}

	return 0;
}

static DEVICE_API(pwm, ti_ehrpwm_api) = {
	.set_cycles = ti_ehrpwm_set_cycles,
	.get_cycles_per_sec = ti_ehrpwm_get_cycles_per_sec,
};

#define TI_EHRPWM_CLK_CONFIG(n)                                                                    \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, fck), (                                             \
			.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, fck)),           \
			.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(       \
				n, fck, clk_id),                                                   \
			.clock_frequency = 0                                                       \
		), (                                                                               \
			.clock_dev = NULL,                                                         \
			.clock_subsys = NULL,                                                      \
			.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 0)                  \
		)                                                                                  \
	)

#define TI_EHRPWM_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct ti_ehrpwm_cfg ti_ehrpwm_config_##n = {                                       \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		TI_EHRPWM_CLK_CONFIG(n),                                                           \
		.tbclk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, tbclk)),                     \
		.tbclk_subsys =                                                                    \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, tbclk, clk_id),     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	static struct ti_ehrpwm_data ti_ehrpwm_data_##n = {                                        \
		.symmetric = DT_INST_PROP(n, symmetric),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ti_ehrpwm_init, NULL, &ti_ehrpwm_data_##n, &ti_ehrpwm_config_##n, \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &ti_ehrpwm_api);

DT_INST_FOREACH_STATUS_OKAY(TI_EHRPWM_INIT)
