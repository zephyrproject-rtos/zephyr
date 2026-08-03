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
	uint32_t RESERVED3[506];                      /* 0x818-0xFFF */
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

static inline void mspm_timer_write_cc(struct mspm_gptimer_regs *base, uint8_t chan, uint32_t val)
{
	if (chan < 2U) {
		base->COUNTERREGS.CC_01[chan] = val;
	} else {
		base->COUNTERREGS.CC_23[chan - 2U] = val;
	}
}

struct counter_mspm_data {
	void *user_data_top;
	void *user_data;
	counter_top_callback_t top_cb;
	counter_alarm_callback_t alarm_cb;
	uint32_t freq;
};

struct counter_mspm_config {
	struct counter_config_info counter_info;
	struct mspm_gptimer_regs *base;
	const struct device *clock_dev;
	const struct mspm0_sys_clock clock_subsys;
	uint32_t clk_sel;
	uint32_t clk_div_reg; /* CLKDIV register value: 0 = div-by-1, 1 = div-by-2, ... */
	uint8_t prescaler;
	void (*irq_config_func)(void);
};

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

	*ticks = config->base->COUNTERREGS.CTR;

	return 0;
}

static int counter_mspm_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;

	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		config->base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
		config->base->COUNTERREGS.CTRCTL |= GPTIMER_CTRCTL_EN_ENABLED;
	} else if (config->base->COUNTERREGS.CTR >= cfg->ticks) {
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			config->base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
			config->base->COUNTERREGS.CTRCTL |= GPTIMER_CTRCTL_EN_ENABLED;
		}

		return -ETIME;
	}

	config->base->COUNTERREGS.LOAD = cfg->ticks;

	data->top_cb = cfg->callback;
	data->user_data_top = cfg->user_data;
	if (cfg->callback) {
		config->base->CPU_INT.ICLR = GPTIMER_CPU_INT_IMASK_L_SET;
		config->base->CPU_INT.IMASK |= GPTIMER_CPU_INT_IMASK_L_SET;
	}

	return 0;
}

static uint32_t counter_mspm_get_top_value(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;

	return config->base->COUNTERREGS.LOAD;
}

static int counter_mspm_set_alarm(const struct device *dev,
				   uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;
	uint32_t top = counter_mspm_get_top_value(dev);
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
		ticks += config->base->COUNTERREGS.CTR;
		if (ticks > top) {
			ticks %= top;
		}
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	mspm_timer_write_cc(config->base, 0, ticks);
	config->base->CPU_INT.ICLR = GPTIMER_CPU_INT_CCU_MASK(0);
	config->base->CPU_INT.IMASK |= GPTIMER_CPU_INT_CCU_MASK(0);

	return 0;
}

static int counter_mspm_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;

	ARG_UNUSED(chan_id);

	config->base->CPU_INT.IMASK &= ~GPTIMER_CPU_INT_CCU_MASK(0);
	data->alarm_cb = NULL;

	return 0;
}

static uint32_t counter_mspm_get_pending_int(const struct device *dev)
{
	const struct counter_mspm_config *config = dev->config;

	return !!(config->base->CPU_INT.RIS &
		  (GPTIMER_CPU_INT_IMASK_L_SET | GPTIMER_CPU_INT_CCU_MASK(0)));
}

static uint32_t counter_mspm_get_freq(const struct device *dev)
{
	const struct counter_mspm_data *data = dev->data;

	return data->freq;
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
	base->GPRCM.RSTCTL = GPTIMER_RSTCTL_KEY_UNLOCK_W | GPTIMER_RSTCTL_RESETSTKYCLR_CLR |
			     GPTIMER_RSTCTL_RESETASSERT_ASSERT;
	base->GPRCM.PWREN = GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE;
	k_busy_wait(10U); /* wait for peripheral power-up */

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

static DEVICE_API(counter, mspm_counter_api) = {
	.start = counter_mspm_start,
	.stop = counter_mspm_stop,
	.get_value = counter_mspm_get_value,
	.set_top_value = counter_mspm_set_top_value,
	.get_pending_int = counter_mspm_get_pending_int,
	.get_top_value = counter_mspm_get_top_value,
	.get_freq = counter_mspm_get_freq,
	.cancel_alarm = counter_mspm_cancel_alarm,
	.set_alarm = counter_mspm_set_alarm,
};

static void counter_mspm_isr(void *arg)
{
	const struct device *dev = arg;
	const struct counter_mspm_config *config = dev->config;
	struct counter_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t ris;

	ris = base->CPU_INT.RIS;
	base->CPU_INT.ICLR = ris;

	if ((ris & GPTIMER_CPU_INT_CCU_MASK(0)) && data->alarm_cb) {
		uint32_t now = base->COUNTERREGS.CTR;
		counter_alarm_callback_t alarm_cb = data->alarm_cb;

		data->alarm_cb = NULL;
		alarm_cb(dev, 0, now, data->user_data);
	} else if ((ris & GPTIMER_CPU_INT_IMASK_L_SET) && data->top_cb) {
		data->top_cb(dev, data->user_data_top);
	}
}

#define MSPM_COUNTER_IRQ_REGISTER(n)							\
	static void mspm_ ## n ##_irq_register(void)					\
	{										\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)),					\
			    DT_IRQ(DT_INST_PARENT(n), priority),			\
			    counter_mspm_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));					\
	}

#define COUNTER_DEVICE_INIT_MSPM(n)							\
	static struct counter_mspm_data counter_mspm_data_ ## n;			\
	MSPM_COUNTER_IRQ_REGISTER(n)							\
											\
	static const struct counter_mspm_config counter_mspm_config_ ## n = {		\
		.base = (struct mspm_gptimer_regs *)DT_REG_ADDR(DT_INST_PARENT(n)),	\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(			\
						DT_INST_PARENT(n), 0)),			\
		.clock_subsys = {							\
			.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),	\
			},								\
		.irq_config_func = (mspm_ ## n ##_irq_register),			\
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
			      counter_mspm_init,					\
			      NULL,							\
			      &counter_mspm_data_ ## n,				\
			      &counter_mspm_config_ ## n,				\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			      &mspm_counter_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT_MSPM)
