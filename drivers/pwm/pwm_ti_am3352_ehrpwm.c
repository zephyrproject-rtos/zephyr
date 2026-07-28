/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PWM_EVENT
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/pwm/pwm_utils.h>
#endif /* CONFIG_PWM_EVENT */

#ifdef CONFIG_CLOCK_CONTROL_TISCI
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#endif

LOG_MODULE_REGISTER(ti_ehrpwm);

#define DT_DRV_COMPAT ti_am3352_ehrpwm

#define TI_EHRPWM_PERIOD_CYCLES_MAX (0xFFFF)
#define TI_EHRPWM_NUM_CHANNELS      (2)

struct ti_ehrpwm_regs {
	volatile uint16_t TBCTL;   /**< Time-Base Control Register, offset: 0x00 */
	uint8_t RESERVED_1[0x8];   /**< Reserved, offset: 0x04 - 0x0A */
	volatile uint16_t TBPRD;   /**< Time-Base Period Register, offest: 0x0A */
	uint8_t RESERVED_2[0x6];   /**< Reserved, offset: 0x0E - 0x12 */
	volatile uint16_t CMPA;    /**< Counter-Compare A Register, offest: 0x12 */
	volatile uint16_t CMPB;    /**< Counter-Compare B Register, offest: 0x14 */
	volatile uint16_t AQCTLA;  /**< AQ Control Register for Output A, offset: 0x16 */
	volatile uint16_t AQCTLB;  /**< AQ Control Register for Output B, offset: 0x18 */
	volatile uint16_t AQSFRC;  /**< AQ Software Force Register, offset: 0x1A */
	volatile uint16_t AQCSFRC; /**< AQ Software Continuous Force Register, offset: 0x1C */
	uint8_t RESERVED_3[0x14];  /**< Reserved, offset: 0x1E - 0x32 */
	volatile uint16_t ETSEL;   /**< Event Trigger Selection Register, offset: 0x32 */
	volatile uint16_t ETPS;    /**< Event Trigger Pre-Scale Register, offset: 0x34 */
	volatile uint16_t ETFLG;   /**< Event Trigger Flag Register, offset: 0x36 */
	volatile uint16_t ETCLR;   /**< Event Trigger Clear Register, offset: 0x38 */
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

/* Event Trigger Selection Register */
#define TI_EHRPWM_ETSEL_INTEN            BIT(3)
#define TI_EHRPWM_ETSEL_INTSEL           GENMASK(2, 0)
#define TI_EHRPWM_ETSEL_INTSEL_PERIOD    (1)
#define TI_EHRPWM_ETSEL_INTSEL_CMPA_UP   (4)
#define TI_EHRPWM_ETSEL_INTSEL_CMPA_DOWN (5)
#define TI_EHRPWM_ETSEL_INTSEL_CMPB_UP   (6)
#define TI_EHRPWM_ETSEL_INTSEL_CMPB_DOWN (7)

/* Event Trigger Pre-Scale Register */
#define TI_EHRPWM_ETPS_INTPRD       GENMASK(1, 0)
#define TI_EHRPWM_ETPS_INTPRD_FIRST (1)

/* Event Trigger Flag Register */
#define TI_EHRPWM_ETFLG_INT BIT(0)

/* Event Trigger Clear Register */
#define TI_EHRPWM_ETCLR_INT BIT(0)

#define DEV_CFG(dev)  ((const struct ti_ehrpwm_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct ti_ehrpwm_data *)(dev)->data)
#define DEV_REGS(dev) ((struct ti_ehrpwm_regs *)DEVICE_MMIO_GET(dev))

struct ti_ehrpwm_cfg {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct device *tbclk_syscon;
	uint32_t tbclk_offset;
	uint8_t tbclk_bit;
	clock_control_subsys_t tbclk_subsys;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_PWM_EVENT
	void (*irq_config)(void);
#endif /* CONFIG_PWM_EVENT */
};

struct ti_ehrpwm_data {
	DEVICE_MMIO_RAM;
	uint32_t period_cycles[TI_EHRPWM_NUM_CHANNELS];
	uint32_t prescale_div[TI_EHRPWM_NUM_CHANNELS];
	bool symmetric;
	bool enabled;
#ifdef CONFIG_PWM_EVENT
	uint16_t event;
	sys_slist_t event_callbacks;
	struct k_spinlock lock;
#endif /* CONFIG_PWM_EVENT */
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
	uint32_t tbclk_reg;
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
	err = syscon_read_reg(cfg->tbclk_syscon, cfg->tbclk_offset, &tbclk_reg);
	if (err != 0) {
		LOG_ERR("failed to read timebase clock register");
		return err;
	}
	tbclk_reg |= BIT(cfg->tbclk_bit);
	err = syscon_write_reg(cfg->tbclk_syscon, cfg->tbclk_offset, tbclk_reg);
	if (err != 0) {
		LOG_ERR("failed to write timebase clock register");
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
	uint32_t tbclk_reg;
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
	err = syscon_read_reg(cfg->tbclk_syscon, cfg->tbclk_offset, &tbclk_reg);
	if (err != 0) {
		LOG_ERR("failed to read timebase clock register");
		return err;
	}
	tbclk_reg &= ~BIT(cfg->tbclk_bit);
	err = syscon_write_reg(cfg->tbclk_syscon, cfg->tbclk_offset, tbclk_reg);
	if (err != 0) {
		LOG_ERR("failed to write timebase clock register");
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

	return clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, (uint32_t *)cycles);
}

#ifdef CONFIG_PWM_EVENT
static void ti_ehrpwm_isr(const struct device *dev)
{
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);

	if ((regs->ETFLG & TI_EHRPWM_ETFLG_INT) == 0) {
		return;
	}

	/* Clear interrupt flags */
	regs->ETCLR = TI_EHRPWM_ETCLR_INT;

	for (uint32_t ch = 0; ch < TI_EHRPWM_NUM_CHANNELS; ch++) {
		pwm_fire_event_callbacks(&data->event_callbacks, dev, ch, data->event);
	}
}

static void ti_ehrpwm_update_interrupts(const struct device *dev)
{
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	struct ti_ehrpwm_regs *regs = DEV_REGS(dev);
	struct pwm_event_callback *cb, *tmp;
	uint16_t etsel = 0;

	/* Check if any callbacks need interrupts */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->event_callbacks, cb, tmp, node) {
		if (cb->event_mask == PWM_EVENT_TYPE_PERIOD) {
			data->event = PWM_EVENT_TYPE_PERIOD;
			etsel = TI_EHRPWM_ETSEL_INTSEL_PERIOD;
			break;
		}

		if (cb->event_mask == PWM_EVENT_TYPE_COMPARE_CAPTURE) {
			data->event = PWM_EVENT_TYPE_COMPARE_CAPTURE;
			if (cb->channel == 0) {
				etsel = TI_EHRPWM_ETSEL_INTSEL_CMPA_UP;
			} else if (cb->channel == 1) {
				etsel = TI_EHRPWM_ETSEL_INTSEL_CMPB_UP;
			}

			break;
		}
	}

	if (etsel == 0) {
		data->event = 0;
		regs->ETSEL = 0;
		return;
	}

	/* generate interrupt on every event */
	regs->ETPS &= ~TI_EHRPWM_ETPS_INTPRD;
	regs->ETPS |= TI_EHRPWM_ETPS_INTPRD_FIRST;

	/* select interrupt type and enable */
	regs->ETSEL = etsel | TI_EHRPWM_ETSEL_INTEN;
}

static int ti_ehrpwm_validate_event_callback(const struct device *dev,
					     struct pwm_event_callback *callback)
{
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	uint8_t num_events = sys_count_bits(&callback->event_mask, sizeof(callback->event_mask));

	if ((callback->event_mask & ~(PWM_EVENT_TYPE_PERIOD | PWM_EVENT_TYPE_COMPARE_CAPTURE)) !=
	    0) {
		LOG_ERR("Unsupported event type: 0x%x", callback->event_mask);
		return -ENOTSUP;
	}
	if (num_events > 1) {
		LOG_ERR("Cannot configure multiple events simultaneously, num_events: %u",
			num_events);
		return -EINVAL;
	}

	if ((num_events != 0) && (data->event != 0) && (callback->event_mask != data->event)) {
		LOG_ERR("Can only use one event at a time: existing=0x%x, new=0x%x", data->event,
			callback->event_mask);
		return -EINVAL;
	}

	return 0;
}

static int ti_ehrpwm_manage_event_callback(const struct device *dev,
					   struct pwm_event_callback *callback, bool set)
{
	struct ti_ehrpwm_data *data = DEV_DATA(dev);
	int ret;

	if (set) {
		ret = ti_ehrpwm_validate_event_callback(dev, callback);
		if (ret < 0) {
			return ret;
		}
	}

	ret = pwm_manage_event_callback(&data->event_callbacks, callback, set);
	if (ret < 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		ti_ehrpwm_update_interrupts(dev);
	}

	return 0;
}
#endif /* CONFIG_PWM_EVENT */

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

#ifdef CONFIG_PWM_EVENT
	sys_slist_init(&DEV_DATA(dev)->event_callbacks);
	DEV_DATA(dev)->event = 0;
	cfg->irq_config();
#endif /* CONFIG_PWM_EVENT */

	return 0;
}

static DEVICE_API(pwm, ti_ehrpwm_api) = {
	.set_cycles = ti_ehrpwm_set_cycles,
	.get_cycles_per_sec = ti_ehrpwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_EVENT
	.manage_event_callback = ti_ehrpwm_manage_event_callback,
#endif /* CONFIG_PWM_EVENT */
};

#define TI_EHRPWM_DEFINE_CLK_SUBSYS(n)                                                             \
	COND_CODE_1(CONFIG_CLOCK_CONTROL_TISCI, (                                                  \
		static struct tisci_clock_config tisci_fclk_##n =                                  \
			TISCI_GET_CLOCK_DETAILS_BY_INST(n);                                        \
		static const clock_control_subsys_t ti_ehrpwm_clk_subsys_##n =                     \
			&tisci_fclk_##n;                                                           \
	), (COND_CODE_1(CONFIG_CLOCK_CONTROL_ARM_SCMI,                                             \
		(static const clock_control_subsys_t ti_ehrpwm_clk_subsys_##n =                    \
			(clock_control_subsys_t)DT_INST_PHA(n, clocks, name);                      \
	), (BUILD_ASSERT(0, "Unsupported clock controller");))))

#ifdef CONFIG_PWM_EVENT
#define TI_EHRPWM_IRQ_INIT(n)                                                                      \
	static void ti_ehrpwm_irq_config_##n(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ti_ehrpwm_isr,              \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#else
#define TI_EHRPWM_IRQ_INIT(n)
#endif /* CONFIG_PWM_EVENT */

#define TI_EHRPWM_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	TI_EHRPWM_DEFINE_CLK_SUBSYS(n);                                                            \
	TI_EHRPWM_IRQ_INIT(n);                                                                     \
	static struct ti_ehrpwm_cfg ti_ehrpwm_config_##n = {                                       \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = ti_ehrpwm_clk_subsys_##n,                                          \
		.tbclk_syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, tbclk)),                          \
		.tbclk_offset = DT_INST_PHA(n, tbclk, offset),                                     \
		.tbclk_bit = DT_INST_PHA(n, tbclk, bit),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		COND_CODE_1(CONFIG_PWM_EVENT, (.irq_config = ti_ehrpwm_irq_config_##n,), ()) };    \
                                                                                                   \
	static struct ti_ehrpwm_data ti_ehrpwm_data_##n = {                                        \
		.symmetric = DT_INST_PROP(n, symmetric),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ti_ehrpwm_init, NULL, &ti_ehrpwm_data_##n, &ti_ehrpwm_config_##n, \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &ti_ehrpwm_api);

DT_INST_FOREACH_STATUS_OKAY(TI_EHRPWM_INIT)
