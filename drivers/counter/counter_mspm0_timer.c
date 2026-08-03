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

struct counter_mspm0_data {
	void *user_data_top;
	void *user_data;
	counter_top_callback_t top_cb;
	counter_alarm_callback_t alarm_cb;
	uint32_t freq;
};

struct counter_mspm0_config {
	struct counter_config_info counter_info;
	struct mspm0_gptimer_regs *base;
	const struct device *clock_dev;
	const struct mspm0_sys_clock clock_subsys;
	uint32_t clk_sel;
	uint32_t clk_div_reg; /* CLKDIV register value: 0 = div-by-1, 1 = div-by-2, ... */
	uint8_t prescaler;
	void (*irq_config_func)(void);
};

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

static int counter_mspm0_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;

	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		config->base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;
		config->base->counterregs.ctrctl |= GPTIMER_CTRCTL_EN_ENABLED;
	} else if (config->base->counterregs.ctr >= cfg->ticks) {
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			config->base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;
			config->base->counterregs.ctrctl |= GPTIMER_CTRCTL_EN_ENABLED;
		}

		return -ETIME;
	}

	config->base->counterregs.load = cfg->ticks;

	data->top_cb = cfg->callback;
	data->user_data_top = cfg->user_data;
	if (cfg->callback) {
		config->base->cpu_int.iclr = GPTIMER_CPU_INT_IMASK_L_SET;
		config->base->cpu_int.imask |= GPTIMER_CPU_INT_IMASK_L_SET;
	}

	return 0;
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
	uint32_t top = counter_mspm0_get_top_value(dev);
	uint32_t ticks = alarm_cfg->ticks;

	ARG_UNUSED(chan_id);

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	if (data->alarm_cb != NULL) {
		LOG_DBG("Alarm busy\n");
		return -EBUSY;
	}

	if ((COUNTER_ALARM_CFG_ABSOLUTE & alarm_cfg->flags) == 0) {
		ticks += config->base->counterregs.ctr;
		if (ticks > top) {
			ticks %= top;
		}
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	mspm0_timer_write_cc(config->base, 0, ticks);
	config->base->cpu_int.iclr = GPTIMER_CPU_INT_CCU_MASK(0);
	config->base->cpu_int.imask |= GPTIMER_CPU_INT_CCU_MASK(0);

	return 0;
}

static int counter_mspm0_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;

	ARG_UNUSED(chan_id);

	config->base->cpu_int.imask &= ~GPTIMER_CPU_INT_CCU_MASK(0);
	data->alarm_cb = NULL;

	return 0;
}

static uint32_t counter_mspm0_get_pending_int(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	return !!(config->base->cpu_int.ris &
		  (GPTIMER_CPU_INT_IMASK_L_SET | GPTIMER_CPU_INT_CCU_MASK(0)));
}

static uint32_t counter_mspm0_get_freq(const struct device *dev)
{
	const struct counter_mspm0_data *data = dev->data;

	return data->freq;
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

	data->freq = clock_rate / ((config->clk_div_reg + 1U) * ((uint32_t)config->prescaler + 1U));

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
	.set_top_value = counter_mspm0_set_top_value,
	.get_pending_int = counter_mspm0_get_pending_int,
	.get_top_value = counter_mspm0_get_top_value,
	.get_freq = counter_mspm0_get_freq,
	.cancel_alarm = counter_mspm0_cancel_alarm,
	.set_alarm = counter_mspm0_set_alarm,
};

static void counter_mspm0_isr(void *arg)
{
	const struct device *dev = arg;
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t ris;

	ris = base->cpu_int.ris;
	base->cpu_int.iclr = ris;

	if ((ris & GPTIMER_CPU_INT_CCU_MASK(0)) && data->alarm_cb) {
		uint32_t now = base->counterregs.ctr;
		counter_alarm_callback_t alarm_cb = data->alarm_cb;

		data->alarm_cb = NULL;
		alarm_cb(dev, 0, now, data->user_data);
	} else if ((ris & GPTIMER_CPU_INT_IMASK_L_SET) && data->top_cb) {
		data->top_cb(dev, data->user_data_top);
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
		.counter_info = {.max_top_value = (DT_INST_PROP(n, resolution) == 32)	\
							? UINT32_MAX : UINT16_MAX,	\
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
				 .channels = 1},					\
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
