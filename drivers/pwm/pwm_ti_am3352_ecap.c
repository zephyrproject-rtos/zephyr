/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_CLOCK_CONTROL_TISCI
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_MSPM0
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#endif

LOG_MODULE_REGISTER(ti_ecap);

#define DT_DRV_COMPAT ti_am3352_ecap

struct ti_ecap_regs {
	uint8_t RESERVED_1[0x10];        /**< Reserved, offset: 0x00 - 0x10 */
	volatile uint32_t CAP3;          /**< Capture-3 Register, offset: 0x10 */
	volatile uint32_t CAP4;          /**< Capture-4 Register, offset: 0x14 */
	uint8_t RESERVED_2[0xC];         /**< Reserved, offset: 0x18 - 0x24 */
	volatile uint32_t ECCTL0;        /**< Input mux select, AM13E-only, offset: 0x24 */
	volatile uint32_t ECCTL;         /**< ECAP Control Register, offset: 0x28 */
	volatile uint32_t ECINT_EN_FLG;  /**< ECAP Interrupt Enable & Flag Register, offset: 0x2C */
	volatile uint32_t ECINT_CLR_FRC; /**< ECAP Interrupt Clear & Force Register, offset: 0x30 */
};

/* ECAP Control Register */
#define TI_ECAP_ECCTL_APWMPOL        BIT(26)
#define TI_ECAP_ECCTL_CAP_APWM       BIT(25)
#define TI_ECAP_ECCTL_SYNCO_SEL      GENMASK(23, 22)
#define TI_ECAP_ECCTL_TSCNTSTP       BIT(20)
#define TI_ECAP_ECCTL_REARM_RESET    BIT(19)
#define TI_ECAP_ECCTL_STOPVALUE      GENMASK(18, 17)
#define TI_ECAP_ECCTL_STOPVALUE_EVT4 (0x3)
#define TI_ECAP_ECCTL_CONT_ONESHT    BIT(16)
#define TI_ECAP_ECCTL_CAPLDEN        BIT(8)
#define TI_ECAP_ECCTL_CTRRST4        BIT(7)
#define TI_ECAP_ECCTL_CAP4POL        BIT(6)
#define TI_ECAP_ECCTL_CTRRST3        BIT(5)
#define TI_ECAP_ECCTL_CAP3POL        BIT(4)
#define TI_ECAP_ECCTL_CTRRST2        BIT(3)
#define TI_ECAP_ECCTL_CAP2POL        BIT(2)
#define TI_ECAP_ECCTL_CTRRST1        BIT(1)
#define TI_ECAP_ECCTL_CAP1POL        BIT(0)

/* ECAP Interrupt Enable and Flag Register */
#define TI_ECAP_ECINT_EN_CNTOVF_FLG BIT(21)
#define TI_ECAP_ECINT_EN_CEVT4_FLG  BIT(20)
#define TI_ECAP_ECINT_EN_CNTOVF     BIT(5)
#define TI_ECAP_ECINT_EN_CEVT4      BIT(4)

/* ECAP Interrupt Clear and Force Register */
#define TI_ECAP_ECINT_CLR_CNTOVF BIT(5)
#define TI_ECAP_ECINT_CLR_CEVT4  BIT(4)
#define TI_ECAP_ECINT_CLR_INT    BIT(0)

/* ECAP Input Mux Select Register (ECCTL0, AM13E-only) */
#define TI_ECAP_ECCTL0_INPUTSEL_MASK GENMASK(6, 0)

#define DEV_CFG(dev)  ((const struct ti_ecap_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct ti_ecap_data *)(dev)->data)
#define DEV_REGS(dev) ((struct ti_ecap_regs *)DEVICE_MMIO_GET(dev))

struct ti_ecap_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	bool capture_period;
	bool capture_pulse;
	bool continuous;
};

/* One crossbar routing applied through the mux subsystem at init, decoded
 * from a "mux-states" phandle-array entry.
 */
struct ti_ecap_mux_entry {
	const struct device *dev;
	const struct mux_state *state;
};

struct ti_ecap_cfg {
	DEVICE_MMIO_ROM;
	void (*irq_config_func)();
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
	const struct ti_ecap_mux_entry *mux_entries;
	uint8_t mux_entries_count;
	bool has_input_xbar;
	uint8_t input_xbar_channel;
};

struct ti_ecap_data {
	DEVICE_MMIO_RAM;
	struct ti_ecap_capture_data cpt;
	bool busy;
};

static int ti_ecap_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			      uint32_t pulse_cycles, pwm_flags_t flags)
{
	ARG_UNUSED(channel);
	struct ti_ecap_regs *regs = DEV_REGS(dev);
	uint32_t ecctl;

	ecctl = regs->ECCTL;

	/* enable apwm and counter */
	ecctl |= (TI_ECAP_ECCTL_TSCNTSTP | TI_ECAP_ECCTL_CAP_APWM);

	/* polarity */
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		ecctl &= ~TI_ECAP_ECCTL_APWMPOL;
	} else {
		ecctl |= TI_ECAP_ECCTL_APWMPOL;
	}

	/* update shadow period and shadow cmp registers in apwm mode */
	regs->CAP3 = period_cycles;
	regs->CAP4 = pulse_cycles;

	/* update ecctl */
	regs->ECCTL = ecctl;

	return 0;
}

static int ti_ecap_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(channel);
	const struct ti_ecap_cfg *cfg = DEV_CFG(dev);

	*cycles = 0;
	return clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, (uint32_t *)cycles);
}

#ifdef CONFIG_PWM_CAPTURE
static int ti_ecap_enable_capture(const struct device *dev, uint32_t channel)
{
	ARG_UNUSED(channel);
	struct ti_ecap_data *data = DEV_DATA(dev);
	struct ti_ecap_regs *regs = DEV_REGS(dev);

	if (data->busy) {
		return -EBUSY;
	}

	/* enable interrupt for 3rd event */
	regs->ECINT_EN_FLG |= (TI_ECAP_ECINT_EN_CEVT4 | TI_ECAP_ECINT_EN_CNTOVF);

	/* start counter and enable loading capture registers */
	regs->ECCTL |= (TI_ECAP_ECCTL_TSCNTSTP | TI_ECAP_ECCTL_CAPLDEN);

	if (!data->cpt.continuous) {
		/* rearms one shot */
		regs->ECCTL |= TI_ECAP_ECCTL_REARM_RESET;
	}

	data->busy = true;

	return 0;
}

static int ti_ecap_disable_capture(const struct device *dev, uint32_t channel)
{
	ARG_UNUSED(channel);
	struct ti_ecap_data *data = DEV_DATA(dev);
	struct ti_ecap_regs *regs = DEV_REGS(dev);

	/* disable interrupts */
	regs->ECINT_EN_FLG &= ~(TI_ECAP_ECINT_EN_CEVT4 | TI_ECAP_ECINT_EN_CNTOVF);

	/* stop counter and disable loading capture registers */
	regs->ECCTL &= ~(TI_ECAP_ECCTL_TSCNTSTP | TI_ECAP_ECCTL_CAPLDEN);

	data->busy = false;

	return 0;
}

static int ti_ecap_configure_capture(const struct device *dev, uint32_t channel, pwm_flags_t flags,
				     pwm_capture_callback_handler_t cb, void *user_data)
{
	ARG_UNUSED(channel);
	struct ti_ecap_data *data = DEV_DATA(dev);
	struct ti_ecap_capture_data *cpt = &data->cpt;
	struct ti_ecap_regs *regs = DEV_REGS(dev);
	uint32_t ecctl;

	if (data->busy) {
		return -EBUSY;
	}

	cpt->callback = cb;
	cpt->user_data = user_data;
	cpt->capture_period = !!(flags & PWM_CAPTURE_TYPE_PERIOD);
	cpt->capture_pulse = !!(flags & PWM_CAPTURE_TYPE_PULSE);
	cpt->continuous = !!(flags & PWM_CAPTURE_MODE_CONTINUOUS);

	/* disable interrupts */
	regs->ECINT_EN_FLG &= ~(TI_ECAP_ECINT_EN_CEVT4 | TI_ECAP_ECINT_EN_CNTOVF);

	/* clear event flags */
	regs->ECINT_CLR_FRC |=
		(TI_ECAP_ECINT_CLR_CNTOVF | TI_ECAP_ECINT_CLR_CEVT4 | TI_ECAP_ECINT_CLR_INT);

	ecctl = regs->ECCTL;

	if (cpt->continuous) {
		/* continuous */
		ecctl &= ~TI_ECAP_ECCTL_CONT_ONESHT;
	} else {
		/* single shot */
		ecctl |= TI_ECAP_ECCTL_CONT_ONESHT;
	}

	/* we only care about first 3 events */
	ecctl &= ~TI_ECAP_ECCTL_STOPVALUE;
	ecctl |= FIELD_PREP(TI_ECAP_ECCTL_STOPVALUE, TI_ECAP_ECCTL_STOPVALUE_EVT4);

	/* configure capture timestamp to reset after edge to capture delta */
	ecctl |= (TI_ECAP_ECCTL_CTRRST1 | TI_ECAP_ECCTL_CTRRST2 | TI_ECAP_ECCTL_CTRRST3 |
		  TI_ECAP_ECCTL_CTRRST4);

	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		/* active high */
		ecctl &= ~TI_ECAP_ECCTL_CAP1POL; /* cap 1 - rising edge */
		ecctl |= TI_ECAP_ECCTL_CAP2POL;  /* cap 2 - falling edge */
		ecctl &= ~TI_ECAP_ECCTL_CAP3POL; /* cap 3 - rising edge */
		ecctl |= TI_ECAP_ECCTL_CAP4POL;  /* cap 4 - falling edge */
	} else {
		ecctl |= TI_ECAP_ECCTL_CAP1POL;  /* cap 1 - falling edge */
		ecctl &= ~TI_ECAP_ECCTL_CAP2POL; /* cap 2 - rising edge */
		ecctl |= TI_ECAP_ECCTL_CAP3POL;  /* cap 3 - falling edge */
		ecctl &= ~TI_ECAP_ECCTL_CAP4POL; /* cap 4 - rising edge */
	}

	/* stop counter and disable loading capture registers */
	ecctl &= ~(TI_ECAP_ECCTL_TSCNTSTP | TI_ECAP_ECCTL_CAPLDEN);

	/* enable capture mode and disable APWM */
	ecctl &= ~TI_ECAP_ECCTL_CAP_APWM;

	regs->ECCTL = ecctl;

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static int ti_ecap_init(const struct device *dev)
{
	const struct ti_ecap_cfg *cfg = DEV_CFG(dev);
	struct ti_ecap_regs *regs;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to configure pinctrl\n");
		return ret;
	}

	for (uint8_t i = 0; i < cfg->mux_entries_count; i++) {
		const struct ti_ecap_mux_entry *entry = &cfg->mux_entries[i];

		if (!device_is_ready(entry->dev)) {
			LOG_ERR("xbar device not ready");
			return -ENODEV;
		}

		ret = mux_state_apply(entry->dev, entry->state);
		if (ret < 0) {
			LOG_ERR("failed to apply mux state %d: %d", i, ret);
			return ret;
		}
	}

	if (cfg->has_input_xbar) {
		regs = DEV_REGS(dev);
		regs->ECCTL0 = (regs->ECCTL0 & ~TI_ECAP_ECCTL0_INPUTSEL_MASK) |
			       FIELD_PREP(TI_ECAP_ECCTL0_INPUTSEL_MASK, cfg->input_xbar_channel);
	}

	cfg->irq_config_func();

	return 0;
}

static void ti_ecap_isr(const struct device *dev)
{
	struct ti_ecap_capture_data *cpt = &DEV_DATA(dev)->cpt;
	struct ti_ecap_regs *regs = DEV_REGS(dev);
	uint32_t period = 0;
	uint32_t pulse = 0;
	uint32_t ecint_en_flg = regs->ECINT_EN_FLG;

	if (ecint_en_flg & TI_ECAP_ECINT_EN_CNTOVF_FLG) {

		if (cpt->callback != NULL) {
			cpt->callback(dev, 0, period, pulse, -ERANGE, cpt->user_data);
		}

		/* clear event */
		regs->ECINT_CLR_FRC |= TI_ECAP_ECINT_CLR_CNTOVF;
	} else if (ecint_en_flg & TI_ECAP_ECINT_EN_CEVT4_FLG) {
		uint32_t cap3 = regs->CAP3;
		uint32_t cap4 = regs->CAP4;

		if (cpt->capture_period) {
			period = cap3 + cap4;
		}

		if (cpt->capture_pulse) {
			pulse = cap4;
		}

		if (cpt->callback != NULL) {
			cpt->callback(dev, 0, period, pulse, 0, cpt->user_data);
		}

		/* clear event */
		regs->ECINT_CLR_FRC |= TI_ECAP_ECINT_CLR_CEVT4;
	}

	/* clear global interrupt */
	regs->ECINT_CLR_FRC |= TI_ECAP_ECINT_CLR_INT;
}

static DEVICE_API(pwm, ti_ecap_api) = {
	.set_cycles = ti_ecap_set_cycles,
	.get_cycles_per_sec = ti_ecap_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.enable_capture = ti_ecap_enable_capture,
	.disable_capture = ti_ecap_disable_capture,
	.configure_capture = ti_ecap_configure_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

#define TI_ECAP_DEFINE_CLK_SUBSYS(n)                                                               \
	COND_CODE_1(CONFIG_CLOCK_CONTROL_TISCI, (                                                  \
		static struct tisci_clock_config tisci_fclk_##n =                                  \
			TISCI_GET_CLOCK_DETAILS_BY_INST(n);                                        \
		static const clock_control_subsys_t ti_ecap_clk_subsys_##n =                     \
			&tisci_fclk_##n;                                                           \
	), (COND_CODE_1(CONFIG_CLOCK_CONTROL_ARM_SCMI,                                             \
		(static const clock_control_subsys_t ti_ecap_clk_subsys_##n =                    \
			(clock_control_subsys_t)DT_INST_PHA(n, clocks, name);                      \
	), (COND_CODE_1(CONFIG_CLOCK_CONTROL_MSPM0,                                                \
		(static const struct mspm0_sys_clock ti_ecap_mspm0_sys_clock_##n =                \
			MSPM0_CLOCK_SUBSYS_FN(n);                                                  \
		static const clock_control_subsys_t ti_ecap_clk_subsys_##n =                     \
			(clock_control_subsys_t)&ti_ecap_mspm0_sys_clock_##n;                     \
	), (BUILD_ASSERT(0, "Unsupported clock controller");))))))

/* One entry per "mux-states" phandle-array element: applies the crossbar
 * routing (Input or Output XBAR) named by that entry through the mux
 * subsystem.
 */
#define TI_ECAP_MUX_ENTRY(node_id, prop, idx)                                                      \
	{                                                                                          \
		.dev = MUX_STATE_DT_DEV_GET_BY_IDX(node_id, idx),                                  \
		.state = MUX_STATE_DT_GET_BY_IDX(node_id, idx),                                    \
	}

#define TI_ECAP_MUX_ENTRIES_DEFINE(n)                                                              \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, mux_states),                                          \
		   (MUX_STATE_DT_INST_SPEC_DEFINE_ALL(n);                                         \
		    static const struct ti_ecap_mux_entry ti_ecap_mux_entries_##n[] = {          \
			    DT_INST_FOREACH_PROP_ELEM_SEP(n, mux_states, TI_ECAP_MUX_ENTRY,      \
							   (,))};))

/* If a "mux-states" entry is named "input", its control cell is the Input
 * XBAR channel that feeds this eCAP's own ECCTL0.INPUTSEL field - a
 * separate mux internal to the eCAP, indexing into its own 128-entry table
 * (INPUTXBAR1-16 occupy indices 0-15), not the crossbar's own addressing.
 */
#define TI_ECAP_MUX_ENTRIES_INIT(n)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, mux_states),                                         \
		    (.mux_entries = ti_ecap_mux_entries_##n,                                      \
		     .mux_entries_count = ARRAY_SIZE(ti_ecap_mux_entries_##n),), ()) \
	COND_CODE_1(DT_INST_PROP_HAS_NAME(n, mux_states, input),                                  \
		    (.has_input_xbar = true,                                                      \
		     .input_xbar_channel = DT_INST_PHA_BY_NAME(n, mux_states, input, channel),), ())

#define TI_ECAP_INIT(n)                                                                            \
	TI_ECAP_DEFINE_CLK_SUBSYS(n);                                                              \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	TI_ECAP_MUX_ENTRIES_DEFINE(n);                                                             \
	static void ti_ecap_irq_config_func_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(                                                                       \
			DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ti_ecap_isr,                    \
			DEVICE_DT_INST_GET(n),                                                     \
			COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, flags),                            \
					(DT_INST_IRQ(n, flags)), (0)));    \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct ti_ecap_cfg ti_ecap_config_##n = {                                           \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = ti_ecap_clk_subsys_##n,                                            \
		.irq_config_func = ti_ecap_irq_config_func_##n,                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		TI_ECAP_MUX_ENTRIES_INIT(n)};                                                      \
                                                                                                   \
	static struct ti_ecap_data ti_ecap_data_##n;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ti_ecap_init, NULL, &ti_ecap_data_##n, &ti_ecap_config_##n,       \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &ti_ecap_api);

DT_INST_FOREACH_STATUS_OKAY(TI_ECAP_INIT)
