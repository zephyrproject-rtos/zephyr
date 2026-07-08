/*
 * Copyright (c) 2025, Linumiz GmbH
 * Copyright (c) 2026, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_timer_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_mspm0_timer, CONFIG_COUNTER_LOG_LEVEL);

/* GPTIMER register map — TRM Chapter 34 */

struct mspm_gptimer_gprcm {
	volatile uint32_t PWREN;
	volatile uint32_t RSTCTL;
	uint32_t RESERVED[3];
	volatile uint32_t STAT;
};

struct mspm_gptimer_int {
	volatile uint32_t IIDX;
	uint32_t RESERVED0;
	volatile uint32_t IMASK;
	uint32_t RESERVED1;
	volatile uint32_t RIS;
	uint32_t RESERVED2;
	volatile uint32_t MIS;
	uint32_t RESERVED3;
	volatile uint32_t ISET;
	uint32_t RESERVED4;
	volatile uint32_t ICLR;
};

struct mspm_gptimer_common {
	volatile uint32_t CCPD;
	volatile uint32_t ODIS;
	volatile uint32_t CCLKCTL;
	volatile uint32_t CPS;
	volatile uint32_t CPSV;
	volatile uint32_t CTTRIGCTL;
	uint32_t RESERVED0;
	volatile uint32_t CTTRIG;
	volatile uint32_t FSCTL;
	volatile uint32_t GCTL;
};

struct mspm_gptimer_counter_regs {
	volatile uint32_t CTR;
	volatile uint32_t CTRCTL;
	volatile uint32_t LOAD;
	uint32_t RESERVED0;
	volatile uint32_t CC_01[2];
	volatile uint32_t CC_23[2];
};

struct mspm_gptimer_regs {
	uint32_t RESERVED0[256];                      /* 0x000-0x3FC */
	volatile uint32_t FSUB_0;                     /* 0x400 */
	volatile uint32_t FSUB_1;                     /* 0x404 */
	uint32_t RESERVED1[15];                       /* 0x408-0x440 */
	volatile uint32_t FPUB_0;                     /* 0x444 */
	volatile uint32_t FPUB_1;                     /* 0x448 */
	uint32_t RESERVED2[237];                      /* 0x44C-0x7FC */
	struct mspm_gptimer_gprcm GPRCM;              /* 0x800 */
	uint32_t RESERVED3[506];                      /* 0x818-0xFFC */
	volatile uint32_t CLKDIV;                     /* 0x1000 */
	uint32_t RESERVED4;                           /* 0x1004 */
	volatile uint32_t CLKSEL;                     /* 0x1008 */
	uint32_t RESERVED5[3];                        /* 0x100C-0x1014 */
	volatile uint32_t PDBGCTL;                    /* 0x1018 */
	uint32_t RESERVED6;                           /* 0x101C */
	struct mspm_gptimer_int CPU_INT;              /* 0x1020 */
	uint32_t RESERVED7;                           /* 0x104C */
	struct mspm_gptimer_int GEN_EVENT0;           /* 0x1050 */
	uint32_t RESERVED8;                           /* 0x107C */
	struct mspm_gptimer_int GEN_EVENT1;           /* 0x1080 */
	uint32_t RESERVED9[13];                       /* 0x10AC-0x10DC */
	volatile uint32_t EVT_MODE;                   /* 0x10E0 */
	uint32_t RESERVED10[6];                       /* 0x10E4-0x10F8 */
	volatile uint32_t DESC;                       /* 0x10FC */
	struct mspm_gptimer_common COMMONREGS;        /* 0x1100 */
	uint32_t RESERVED11[438];                     /* 0x1128-0x17FC */
	struct mspm_gptimer_counter_regs COUNTERREGS; /* 0x1800 */
};

BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, GPRCM) == 0x0800U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, CLKDIV) == 0x1000U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, CPU_INT) == 0x1020U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, COMMONREGS) == 0x1100U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, COUNTERREGS) == 0x1800U);

/*
 * GPTIMER bit-field constants — TRM Chapter 34.
 * Guarded against redefinition if hw_gptimer.h is pulled in transitively
 * by another subsystem (e.g. clock control or GPIO drivers).
 */

/* GPRCM.PWREN */
#ifndef GPTIMER_PWREN_KEY_UNLOCK_W
#define GPTIMER_PWREN_KEY_UNLOCK_W 0x26000000U
#endif
#ifndef GPTIMER_PWREN_ENABLE_ENABLE
#define GPTIMER_PWREN_ENABLE_ENABLE 0x00000001U
#endif

/* GPRCM.RSTCTL */
#ifndef GPTIMER_RSTCTL_KEY_UNLOCK_W
#define GPTIMER_RSTCTL_KEY_UNLOCK_W 0xB1000000U
#endif
#ifndef GPTIMER_RSTCTL_RESETSTKYCLR_CLR
#define GPTIMER_RSTCTL_RESETSTKYCLR_CLR 0x00000002U
#endif
#ifndef GPTIMER_RSTCTL_RESETASSERT_ASSERT
#define GPTIMER_RSTCTL_RESETASSERT_ASSERT 0x00000001U
#endif

/* COUNTERREGS.CTRCTL */
#ifndef GPTIMER_CTRCTL_EN_ENABLED
#define GPTIMER_CTRCTL_EN_ENABLED 0x00000001U
#endif
#ifndef GPTIMER_CTRCTL_EN_MASK
#define GPTIMER_CTRCTL_EN_MASK 0x00000001U
#endif
#ifndef GPTIMER_CTRCTL_REPEAT_REPEAT_1
#define GPTIMER_CTRCTL_REPEAT_REPEAT_1 0x00000002U
#endif
#ifndef GPTIMER_CTRCTL_CM_UP
#define GPTIMER_CTRCTL_CM_UP 0x00000020U
#endif
#ifndef GPTIMER_CTRCTL_CVAE_ZEROVAL
#define GPTIMER_CTRCTL_CVAE_ZEROVAL 0x20000000U
#endif

/* COMMONREGS.CCLKCTL */
#ifndef GPTIMER_CCLKCTL_CLKEN_ENABLED
#define GPTIMER_CCLKCTL_CLKEN_ENABLED 0x00000001U
#endif

/* CPU_INT interrupt bits (IMASK / RIS / ICLR) */
#ifndef GPTIMER_CPU_INT_IMASK_L_SET
#define GPTIMER_CPU_INT_IMASK_L_SET 0x00000002U
#endif
/* CCU0..CCU3: computed, no single TRM constant covers all channels */
#define GPTIMER_CPU_INT_CCU_MASK(ch) BIT(8U + (ch))

/* Register accessor helpers */

static inline uint32_t mspm_timer_read_ctr(struct mspm_gptimer_regs *base)
{
	return base->COUNTERREGS.CTR;
}

static inline void mspm_timer_write_ctr(struct mspm_gptimer_regs *base, uint32_t val)
{
	base->COUNTERREGS.CTR = val;
}

static inline uint32_t mspm_timer_read_load(struct mspm_gptimer_regs *base)
{
	return base->COUNTERREGS.LOAD;
}

static inline void mspm_timer_write_load(struct mspm_gptimer_regs *base, uint32_t val)
{
	base->COUNTERREGS.LOAD = val;
}

static inline void mspm_timer_write_cc(struct mspm_gptimer_regs *base, uint8_t chan, uint32_t val)
{
	if (chan < 2U) {
		base->COUNTERREGS.CC_01[chan] = val;
	} else {
		base->COUNTERREGS.CC_23[chan - 2U] = val;
	}
}

static inline void mspm_timer_imask_set(struct mspm_gptimer_regs *base, uint32_t mask)
{
	base->CPU_INT.IMASK |= mask;
}

static inline void mspm_timer_imask_clr(struct mspm_gptimer_regs *base, uint32_t mask)
{
	base->CPU_INT.IMASK &= ~mask;
}

static inline uint32_t mspm_timer_ris_read(struct mspm_gptimer_regs *base)
{
	return base->CPU_INT.RIS;
}

static inline void mspm_timer_iclr_write(struct mspm_gptimer_regs *base, uint32_t mask)
{
	base->CPU_INT.ICLR = mask;
}

/* Tick arithmetic helpers for alarm scheduling */

static uint32_t ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;

	if (likely(IS_BIT_MASK(top))) {
		return (val1 + val2) & top;
	}
	to_top = top - val1;
	return (val2 <= to_top) ? val1 + val2 : val2 - to_top;
}

static uint32_t ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}
	return (val >= old) ? (val - old) : val + top + 1U - old;
}

struct counter_mspm_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_mspm_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	atomic_t cc_int_pending;
	uint32_t freq;
	struct counter_mspm_ch_data ch[4]; /* hardware maximum: 4 CC channels */
};

struct counter_mspm_config {
	struct counter_config_info counter_info;
	struct mspm_gptimer_regs *base;
	const struct device *clock_dev;
	const struct mspm0_sys_clock clock_subsys;
	uint32_t clk_sel;
	uint32_t clk_div_reg; /* CLKDIV register value: 0 = div-by-1, 1 = div-by-2, ... */
	uint8_t prescaler;
	uint32_t irqn;
	void (*irq_config_func)(void);
};

/* Software-pending interrupt for late alarm detection */
static void mspm_set_cc_int_pending(const struct device *dev, uint8_t chan)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;

	atomic_or(&data->cc_int_pending, BIT(chan));
	NVIC_SetPendingIRQ(config->irqn);
}

static int counter_mspm_start(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;

	config->base->COUNTERREGS.CTRCTL |= GPTIMER_CTRCTL_EN_ENABLED;
	return 0;
}

static int counter_mspm_stop(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;

	config->base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
	return 0;
}

static int counter_mspm_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_mspm_config *config = dev->config;

	*ticks = mspm_timer_read_ctr(config->base);
	return 0;
}

static int counter_mspm_reset(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;

	mspm_timer_write_ctr(config->base, 0U);
	return 0;
}

static int counter_mspm_set_value(const struct device *dev, uint32_t ticks)
{
	const struct counter_mspm_config *config = dev->config;

	if (ticks > config->counter_info.max_top_value) {
		return -EINVAL;
	}
	mspm_timer_write_ctr(config->base, ticks);
	return 0;
}

static int counter_mspm_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	int err = 0;

	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	mspm_timer_imask_clr(base, GPTIMER_CPU_INT_IMASK_L_SET);

	bool do_reset = !(cfg->flags & COUNTER_TOP_CFG_DONT_RESET);

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) && mspm_timer_read_ctr(base) >= cfg->ticks) {
		err = -ETIME;
		do_reset = !!(cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE);
	}

	if (do_reset) {
		base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
		mspm_timer_write_ctr(base, 0U);
		base->COUNTERREGS.CTRCTL |= GPTIMER_CTRCTL_EN_ENABLED;
	}

	mspm_timer_write_load(base, cfg->ticks);
	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (cfg->callback) {
		mspm_timer_iclr_write(base, GPTIMER_CPU_INT_IMASK_L_SET);
		mspm_timer_imask_set(base, GPTIMER_CPU_INT_IMASK_L_SET);
	}

	return err;
}

static uint32_t counter_mspm_get_top_value(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;

	return mspm_timer_read_load(config->base);
}

static int counter_mspm_set_alarm(const struct device *dev, uint8_t chan_id,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t top = counter_mspm_get_top_value(dev);
	uint32_t val = alarm_cfg->ticks;
	bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	bool irq_on_late = false;
	uint32_t now, diff, max_rel_val;
	int err = 0;

	if (chan_id >= config->counter_info.channels) {
		return -EINVAL;
	}

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	if (data->ch[chan_id].callback) {
		return -EBUSY;
	}

	data->ch[chan_id].callback = alarm_cfg->callback;
	data->ch[chan_id].user_data = alarm_cfg->user_data;

	now = mspm_timer_read_ctr(base);
	/* Pre-arm CC to current value so no spurious match fires during setup */
	mspm_timer_write_cc(base, chan_id, now);
	mspm_timer_iclr_write(base, GPTIMER_CPU_INT_CCU_MASK(chan_id));

	if (absolute) {
		max_rel_val = top - data->guard_period;
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		irq_on_late = val < (top / 2U);
		max_rel_val = irq_on_late ? top / 2U : top;
		val = ticks_add(now, val, top);
	}

	mspm_timer_write_cc(base, chan_id, val);

	diff = ticks_sub(val - 1U, mspm_timer_read_ctr(base), top);
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}
		if (irq_on_late) {
			mspm_set_cc_int_pending(dev, chan_id);
		} else {
			data->ch[chan_id].callback = NULL;
		}
	} else {
		mspm_timer_imask_set(base, GPTIMER_CPU_INT_CCU_MASK(chan_id));
	}

	return err;
}

static int counter_mspm_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;

	if (chan_id >= config->counter_info.channels) {
		return -EINVAL;
	}

	mspm_timer_imask_clr(config->base, GPTIMER_CPU_INT_CCU_MASK(chan_id));
	data->ch[chan_id].callback = NULL;
	return 0;
}

static uint32_t counter_mspm_get_pending_int(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;
	uint32_t mask = GPTIMER_CPU_INT_IMASK_L_SET;

	for (int i = 0; i < config->counter_info.channels; i++) {
		mask |= GPTIMER_CPU_INT_CCU_MASK(i);
	}
	return !!(mspm_timer_ris_read(config->base) & mask);
}

static uint32_t counter_mspm_get_freq(const struct device *dev)
{
	return ((const struct counter_mspm_data *)dev->data)->freq;
}

static int counter_mspm_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	struct counter_mspm_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(guard < counter_mspm_get_top_value(dev));
	data->guard_period = guard;
	return 0;
}

static uint32_t counter_mspm_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(flags);
	return ((const struct counter_mspm_data *)dev->data)->guard_period;
}

static int counter_mspm_init(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t clock_rate;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&config->clock_subsys, &clock_rate);
	if (ret != 0) {
		LOG_ERR("clk get rate err %d", ret);
		return ret;
	}

	base->GPRCM.RSTCTL = GPTIMER_RSTCTL_KEY_UNLOCK_W | GPTIMER_RSTCTL_RESETSTKYCLR_CLR |
			     GPTIMER_RSTCTL_RESETASSERT_ASSERT;
	base->GPRCM.PWREN = GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE;
	k_busy_wait(1U);

	base->CLKSEL = config->clk_sel;
	base->CLKDIV = config->clk_div_reg;
	base->COMMONREGS.CPS = config->prescaler;
	base->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;

	data->freq = clock_rate / ((config->clk_div_reg + 1U) * ((uint32_t)config->prescaler + 1U));

	base->COUNTERREGS.CTRCTL =
		GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_REPEAT_REPEAT_1 | GPTIMER_CTRCTL_CVAE_ZEROVAL;
	base->COUNTERREGS.LOAD = config->counter_info.max_top_value;

	base->CPU_INT.IMASK = 0U;
	base->CPU_INT.ICLR = 0xFFFFFFFFU;

	config->irq_config_func();
	return 0;
}

static void counter_mspm_isr(void *arg)
{
	const struct device *dev = arg;
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	counter_alarm_callback_t cb;
	void *user_data;
	uint32_t ris;

	ris = mspm_timer_ris_read(base);
	mspm_timer_iclr_write(base, ris);

	if ((ris & GPTIMER_CPU_INT_IMASK_L_SET) && data->top_cb) {
		data->top_cb(dev, data->top_user_data);
	}

	for (int i = 0; i < config->counter_info.channels; i++) {
		bool hw = !!(ris & GPTIMER_CPU_INT_CCU_MASK(i));
		bool sw = !!(atomic_and(&data->cc_int_pending, ~BIT(i)) & BIT(i));

		if (!hw && !sw) {
			continue;
		}

		mspm_timer_imask_clr(base, GPTIMER_CPU_INT_CCU_MASK(i));
		cb = data->ch[i].callback;
		user_data = data->ch[i].user_data;
		data->ch[i].callback = NULL;

		if (cb) {
			cb(dev, (uint8_t)i, mspm_timer_read_ctr(base), user_data);
		}
	}
}

static DEVICE_API(counter, mspm_counter_api) = {
	.start = counter_mspm_start,
	.stop = counter_mspm_stop,
	.get_value = counter_mspm_get_value,
	.reset = counter_mspm_reset,
	.set_value = counter_mspm_set_value,
	.set_top_value = counter_mspm_set_top_value,
	.get_pending_int = counter_mspm_get_pending_int,
	.get_top_value = counter_mspm_get_top_value,
	.get_freq = counter_mspm_get_freq,
	.set_alarm = counter_mspm_set_alarm,
	.cancel_alarm = counter_mspm_cancel_alarm,
	.set_guard_period = counter_mspm_set_guard_period,
	.get_guard_period = counter_mspm_get_guard_period,
};

#define MSPM_COUNTER_IRQ_REGISTER(n)                                                               \
	static void mspm_##n##_irq_register(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    counter_mspm_isr, DEVICE_DT_INST_GET(n), 0);                           \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}

#define COUNTER_DEVICE_INIT_MSPM(n)                                                                \
	BUILD_ASSERT(DT_INST_PROP(n, channels) <= 4, "channels exceeds hardware maximum of 4");    \
	static struct counter_mspm_data counter_mspm_data_##n;                                     \
	MSPM_COUNTER_IRQ_REGISTER(n)                                                               \
                                                                                                   \
	static const struct counter_mspm_config counter_mspm_config_##n = {                        \
		.base = (struct mspm_gptimer_regs *)DT_REG_ADDR(DT_INST_PARENT(n)),                \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(DT_INST_PARENT(n), 0)),           \
		.clock_subsys =                                                                    \
			{                                                                          \
				.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),           \
			},                                                                         \
		.clk_sel = MSPM0_CLOCK_PERIPH_REG_MASK(                                            \
			DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk)),                         \
		.clk_div_reg = DT_PROP(DT_INST_PARENT(n), ti_clk_div) - 1U,                        \
		.prescaler = DT_PROP(DT_INST_PARENT(n), ti_clk_prescaler),                         \
		.irqn = DT_IRQN(DT_INST_PARENT(n)),                                                \
		.irq_config_func = mspm_##n##_irq_register,                                        \
		.counter_info =                                                                    \
			{                                                                          \
				.max_top_value = (DT_INST_PROP(n, resolution) == 32) ? UINT32_MAX  \
										     : UINT16_MAX, \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = DT_INST_PROP(n, channels),                             \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_mspm_init, NULL, &counter_mspm_data_##n,                  \
			      &counter_mspm_config_##n, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, \
			      &mspm_counter_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT_MSPM)
