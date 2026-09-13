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

LOG_MODULE_REGISTER(mspm0_counter, CONFIG_COUNTER_LOG_LEVEL);

/* GPTIMER register map — TRM Chapter 34 */

struct mspm0_gptimer_gprcm {
	volatile uint32_t pwren;
	volatile uint32_t rstctl;
	uint32_t reserved[3];
	volatile uint32_t stat;
};

struct mspm0_gptimer_int {
	volatile uint32_t iidx;
	uint32_t reserved0;
	volatile uint32_t imask;
	uint32_t reserved1;
	volatile uint32_t ris;
	uint32_t reserved2;
	volatile uint32_t mis;
	uint32_t reserved3;
	volatile uint32_t iset;
	uint32_t reserved4;
	volatile uint32_t iclr;
};

struct mspm0_gptimer_common {
	volatile uint32_t ccpd;
	volatile uint32_t odis;
	volatile uint32_t cclkctl;
	volatile uint32_t cps;
	volatile uint32_t cpsv;
	volatile uint32_t cttrigctl;
	uint32_t reserved0;
	volatile uint32_t cttrig;
	volatile uint32_t fsctl;
	volatile uint32_t gctl;
};

struct mspm0_gptimer_counter_regs {
	volatile uint32_t ctr;
	volatile uint32_t ctrctl;
	volatile uint32_t load;
	uint32_t reserved0;
	volatile uint32_t cc_01[2];
	volatile uint32_t cc_23[2];
};

struct mspm0_gptimer_regs {
	uint32_t reserved0[256];                       /* 0x000-0x3FC */
	volatile uint32_t fsub_0;                      /* 0x400 */
	volatile uint32_t fsub_1;                      /* 0x404 */
	uint32_t reserved1[15];                        /* 0x408-0x440 */
	volatile uint32_t fpub_0;                      /* 0x444 */
	volatile uint32_t fpub_1;                      /* 0x448 */
	uint32_t reserved2[237];                       /* 0x44C-0x7FC */
	struct mspm0_gptimer_gprcm gprcm;              /* 0x800 */
	uint32_t reserved3[506];                       /* 0x818-0xFFF */
	volatile uint32_t clkdiv;                      /* 0x1000 */
	uint32_t reserved4;                            /* 0x1004 */
	volatile uint32_t clksel;                      /* 0x1008 */
	uint32_t reserved5[3];                         /* 0x100C-0x1014 */
	volatile uint32_t pdbgctl;                     /* 0x1018 */
	uint32_t reserved6;                            /* 0x101C */
	struct mspm0_gptimer_int cpu_int;              /* 0x1020 */
	uint32_t reserved7;                            /* 0x104C */
	struct mspm0_gptimer_int gen_event0;           /* 0x1050 */
	uint32_t reserved8;                            /* 0x107C */
	struct mspm0_gptimer_int gen_event1;           /* 0x1080 */
	uint32_t reserved9[13];                        /* 0x10AC-0x10DC */
	volatile uint32_t evt_mode;                    /* 0x10E0 */
	uint32_t reserved10[6];                        /* 0x10E4-0x10F8 */
	volatile uint32_t desc;                        /* 0x10FC */
	struct mspm0_gptimer_common commonregs;        /* 0x1100 */
	uint32_t reserved11[438];                      /* 0x1128-0x17FC */
	struct mspm0_gptimer_counter_regs counterregs; /* 0x1800 */
};

BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, gprcm) == 0x0800U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, clkdiv) == 0x1000U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, cpu_int) == 0x1020U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, commonregs) == 0x1100U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, counterregs) == 0x1800U);

/*
 * GPTIMER bit-field constants — TRM Chapter 34.
 * hw_gptimer.h (pulled in via DriverLib when CONFIG_HAS_MSPM0_SDK is set)
 * defines all of these; skip the block entirely in that case.
 */
#ifndef CONFIG_HAS_MSPM0_SDK

/* GPRCM.PWREN */
#define GPTIMER_PWREN_KEY_UNLOCK_W 0x26000000U
#define GPTIMER_PWREN_ENABLE_ENABLE 0x00000001U

/* GPRCM.RSTCTL */
#define GPTIMER_RSTCTL_KEY_UNLOCK_W 0xB1000000U
#define GPTIMER_RSTCTL_RESETSTKYCLR_CLR 0x00000002U
#define GPTIMER_RSTCTL_RESETASSERT_ASSERT 0x00000001U

/* COUNTERREGS.CTRCTL */
#define GPTIMER_CTRCTL_EN_ENABLED 0x00000001U
#define GPTIMER_CTRCTL_EN_MASK 0x00000001U
#define GPTIMER_CTRCTL_REPEAT_REPEAT_1 0x00000002U
#define GPTIMER_CTRCTL_CM_UP 0x00000020U
#define GPTIMER_CTRCTL_CVAE_ZEROVAL 0x20000000U

/* COMMONREGS.CCLKCTL */
#define GPTIMER_CCLKCTL_CLKEN_ENABLED 0x00000001U

/* CPU_INT interrupt bits (IMASK / RIS / ICLR) */
#define GPTIMER_CPU_INT_IMASK_L_SET 0x00000002U

#endif /* !CONFIG_HAS_MSPM0_SDK */
/* CCU0..CCU3: computed, no single TRM constant covers all channels */
#define GPTIMER_CPU_INT_CCU_MASK(ch) BIT(8U + (ch))

static inline void mspm0_timer_write_cc(struct mspm0_gptimer_regs *base, uint8_t chan, uint32_t val)
{
	if (chan < 2U) {
		base->counterregs.cc_01[chan] = val;
	} else {
		base->counterregs.cc_23[chan - 2U] = val;
	}
}

/* Tick arithmetic helpers for alarm scheduling */

static uint32_t mspm0_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;

	if (likely(IS_BIT_MASK(top))) {
		return (val1 + val2) & top;
	}
	to_top = top - val1;
	return (val2 <= to_top) ? val1 + val2 : val2 - to_top - 1U;
}

static uint32_t mspm0_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}
	return (val >= old) ? (val - old) : val + top + 1U - old;
}

struct counter_mspm0_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_mspm0_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	atomic_t cc_int_pending;
	uint32_t freq;
	/* 6 CC channels in 3 identical pairs (CC0/1, CC2/3, CC4/5); CC0-CC3 used */
	struct counter_mspm0_ch_data ch[4];
};

struct counter_mspm0_config {
	struct counter_config_info counter_info;
	struct mspm0_gptimer_regs *base;
	const struct device *clock_dev;
	const struct mspm0_sys_clock clock_subsys;
	uint32_t clk_sel;
	uint32_t clk_div_reg; /* CLKDIV register value: 0 = div-by-1, 1 = div-by-2, ... */
	uint32_t prescaler;
	unsigned int irqn;
	void (*irq_config_func)(void);
};

/* Software-pending interrupt for late alarm detection */
static void mspm0_set_cc_int_pending(const struct device *dev, uint8_t chan)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;

	atomic_or(&data->cc_int_pending, BIT(chan));
	NVIC_SetPendingIRQ((IRQn_Type)config->irqn);
}

static int counter_mspm0_start(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	config->base->counterregs.ctrctl |= GPTIMER_CTRCTL_EN_ENABLED;

	return 0;
}

static int counter_mspm0_stop(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	config->base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;

	return 0;
}

static int counter_mspm0_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_mspm0_config *config = dev->config;

	if (ticks == NULL) {
		return -EINVAL;
	}

	*ticks = config->base->counterregs.ctr;

	return 0;
}

static int counter_mspm0_reset(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	config->base->counterregs.ctr = 0U;

	return 0;
}

static int counter_mspm0_set_value(const struct device *dev, uint32_t ticks)
{
	const struct counter_mspm0_config *config = dev->config;

	if (ticks > config->counter_info.max_top_value) {
		return -EINVAL;
	}
	config->base->counterregs.ctr = ticks;

	return 0;
}

static int counter_mspm0_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	int err = 0;

	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	/* Top can only be changed when all alarms are disabled; a lower top
	 * would leave any active alarm's CC value unreachable, so it never fires.
	 */
	for (int i = 0; i < config->counter_info.channels; i++) {
		if (data->ch[i].callback != NULL) {
			return -EBUSY;
		}
	}

	base->cpu_int.imask &= ~GPTIMER_CPU_INT_IMASK_L_SET;

	bool do_reset = !(cfg->flags & COUNTER_TOP_CFG_DONT_RESET);

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) && base->counterregs.ctr >= cfg->ticks) {
		err = -ETIME;
		do_reset = !!(cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE);
	}

	if (do_reset) {
		base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;
		base->counterregs.ctr = 0U;
		base->counterregs.ctrctl |= GPTIMER_CTRCTL_EN_ENABLED;
	}

	base->counterregs.load = cfg->ticks;
	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (cfg->callback) {
		base->cpu_int.iclr = GPTIMER_CPU_INT_IMASK_L_SET;
		base->cpu_int.imask |= GPTIMER_CPU_INT_IMASK_L_SET;
	}

	return err;
}

static uint32_t counter_mspm0_get_top_value(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	return config->base->counterregs.load;
}

static int counter_mspm0_set_alarm(const struct device *dev,
				   uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t top = counter_mspm0_get_top_value(dev);
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

	now = base->counterregs.ctr;

	if (absolute) {
		max_rel_val = top - data->guard_period;
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		irq_on_late = val < (top / 2U);
		max_rel_val = irq_on_late ? top / 2U : top;
		val = mspm0_ticks_add(now, val, top);
	}

	/* Lock interrupts: CC write, ICLR clear, late-check, and IMASK enable
	 * must be atomic to prevent a missed alarm if the counter reaches val
	 * between the CC write and the IMASK enable.
	 */
	uint32_t key = irq_lock();

	mspm0_timer_write_cc(base, chan_id, val);
	base->cpu_int.iclr = GPTIMER_CPU_INT_CCU_MASK(chan_id);

	diff = mspm0_ticks_sub(val - 1U, base->counterregs.ctr, top);
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}
		if (irq_on_late) {
			mspm0_set_cc_int_pending(dev, chan_id);
		} else {
			data->ch[chan_id].callback = NULL;
		}
	} else {
		base->cpu_int.imask |= GPTIMER_CPU_INT_CCU_MASK(chan_id);
	}

	irq_unlock(key);

	return err;
}

static int counter_mspm0_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;

	if (chan_id >= config->counter_info.channels) {
		return -EINVAL;
	}

	config->base->cpu_int.imask &= ~GPTIMER_CPU_INT_CCU_MASK(chan_id);
	atomic_and(&data->cc_int_pending, ~BIT(chan_id));
	data->ch[chan_id].callback = NULL;

	return 0;
}

static uint32_t counter_mspm0_get_pending_int(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	uint32_t mask = GPTIMER_CPU_INT_IMASK_L_SET;

	for (int i = 0; i < config->counter_info.channels; i++) {
		mask |= GPTIMER_CPU_INT_CCU_MASK(i);
	}
	return !!(config->base->cpu_int.ris & mask);
}

static uint32_t counter_mspm0_get_freq(const struct device *dev)
{
	const struct counter_mspm0_data *data = dev->data;

	return data->freq;
}

static int counter_mspm0_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	struct counter_mspm0_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(guard < counter_mspm0_get_top_value(dev));
	data->guard_period = guard;

	return 0;
}

static uint32_t counter_mspm0_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(flags);
	return ((const struct counter_mspm0_data *)dev->data)->guard_period;
}

static int counter_mspm0_init(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t clock_rate;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)(uintptr_t)&config->clock_subsys,
				     &clock_rate);
	if (ret != 0) {
		LOG_ERR("clk get rate err %d", ret);
		return ret;
	}

	/* Assert reset and clear sticky bit, then enable power — follows TI SDK
	 * DL_Timer_reset()/DL_Timer_enablePower() sequence. RESETASSERT is
	 * self-clearing; PWREN latches power on.
	 */
	base->gprcm.rstctl = GPTIMER_RSTCTL_KEY_UNLOCK_W | GPTIMER_RSTCTL_RESETSTKYCLR_CLR |
			     GPTIMER_RSTCTL_RESETASSERT_ASSERT;
	base->gprcm.pwren = GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE;
	k_busy_wait(10U); /* wait for peripheral power-up */

	base->clksel = config->clk_sel;
	base->clkdiv = config->clk_div_reg;
	base->commonregs.cps = config->prescaler;
	base->commonregs.cclkctl = GPTIMER_CCLKCTL_CLKEN_ENABLED;

	data->freq = clock_rate / ((config->clk_div_reg + 1U) * (config->prescaler + 1U));

	base->counterregs.ctrctl =
		GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_REPEAT_REPEAT_1 | GPTIMER_CTRCTL_CVAE_ZEROVAL;
	base->counterregs.load = config->counter_info.max_top_value;

	base->cpu_int.imask = 0U;
	base->cpu_int.iclr = 0xFFFFFFFFU;

	config->irq_config_func();

	return 0;
}

static DEVICE_API(counter, mspm0_counter_api) = {
	.start = counter_mspm0_start,
	.stop = counter_mspm0_stop,
	.get_value = counter_mspm0_get_value,
	.reset = counter_mspm0_reset,
	.set_value = counter_mspm0_set_value,
	.set_top_value = counter_mspm0_set_top_value,
	.get_pending_int = counter_mspm0_get_pending_int,
	.get_top_value = counter_mspm0_get_top_value,
	.get_freq = counter_mspm0_get_freq,
	.set_alarm = counter_mspm0_set_alarm,
	.cancel_alarm = counter_mspm0_cancel_alarm,
	.set_guard_period = counter_mspm0_set_guard_period,
	.get_guard_period = counter_mspm0_get_guard_period,
};

static void counter_mspm0_isr(void *arg)
{
	const struct device *dev = arg;
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	counter_alarm_callback_t cb;
	void *user_data;
	uint32_t ris;

	ris = base->cpu_int.ris;
	base->cpu_int.iclr = ris;

	if ((ris & GPTIMER_CPU_INT_IMASK_L_SET) && data->top_cb) {
		data->top_cb(dev, data->top_user_data);
	}

	for (int i = 0; i < config->counter_info.channels; i++) {
		bool hw = !!(ris & GPTIMER_CPU_INT_CCU_MASK(i));
		bool sw = !!(atomic_and(&data->cc_int_pending, ~BIT(i)) & BIT(i));

		if (!hw && !sw) {
			continue;
		}

		base->cpu_int.imask &= ~GPTIMER_CPU_INT_CCU_MASK(i);
		cb = data->ch[i].callback;
		user_data = data->ch[i].user_data;
		data->ch[i].callback = NULL;

		if (cb) {
			cb(dev, (uint8_t)i, base->counterregs.ctr, user_data);
		}
	}
}

#define MSPM0_COUNTER_IRQ_REGISTER(n)							\
	static void mspm0_ ## n ##_irq_register(void)					\
	{										\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)),					\
			    DT_IRQ(DT_INST_PARENT(n), priority),			\
			    counter_mspm0_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));					\
	}

#define COUNTER_DEVICE_INIT_MSPM0(n)							\
	BUILD_ASSERT(DT_INST_PROP(n, channels) <= 4,					\
		     "channels exceeds hardware maximum of 4");				\
	static struct counter_mspm0_data counter_mspm0_data_ ## n;			\
	MSPM0_COUNTER_IRQ_REGISTER(n)							\
											\
	static const struct counter_mspm0_config counter_mspm0_config_ ## n = {		\
		.base = (struct mspm0_gptimer_regs *)DT_REG_ADDR(DT_INST_PARENT(n)),	\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(			\
						DT_INST_PARENT(n), 0)),			\
		.clock_subsys = {							\
			.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),	\
			},								\
		.irq_config_func = (mspm0_ ## n ##_irq_register),			\
		.clk_sel = MSPM0_CLOCK_PERIPH_REG_MASK(				\
				DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk)),	\
		.clk_div_reg = DT_PROP(DT_INST_PARENT(n), ti_clk_div) - 1U,		\
		.prescaler = DT_PROP(DT_INST_PARENT(n), ti_clk_prescaler),		\
		.irqn = DT_IRQN(DT_INST_PARENT(n)),					\
		.counter_info = {.max_top_value = (DT_INST_PROP(n, resolution) == 32)	\
							? UINT32_MAX : UINT16_MAX,	\
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
				 .channels = DT_INST_PROP(n, channels)},			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			      counter_mspm0_init,					\
			      NULL,							\
			      &counter_mspm0_data_ ## n,				\
			      &counter_mspm0_config_ ## n,				\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			      &mspm0_counter_api);
DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT_MSPM0)
