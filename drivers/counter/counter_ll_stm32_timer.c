/*
 * Copyright (c) 2021 Kent Hall.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>

#include <stm32_ll_tim.h>
#include <stm32_ll_rcc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_timer_stm32, CONFIG_COUNTER_LOG_LEVEL);

/* L0 series MCUs only have 16-bit timers and don't have below macro defined */
#ifndef IS_TIM_32B_COUNTER_INSTANCE
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) (0)
#endif

/** Maximum number of timer channels. */
#define TIMER_MAX_CH 4U

/** Number of channels for timer by index. */
#define NUM_CH(timx)					    \
	(IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_4) ? 4U :    \
	 (IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_3) ? 3U :   \
	  (IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_2) ? 2U :  \
	   (IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_1) ? 1U : \
	    0))))

/** Channel to compare set function mapping. */
static void(*const set_timer_compare[TIMER_MAX_CH])(TIM_TypeDef *,
						    uint32_t) = {
	LL_TIM_OC_SetCompareCH1, LL_TIM_OC_SetCompareCH2,
	LL_TIM_OC_SetCompareCH3, LL_TIM_OC_SetCompareCH4,
};

/** Channel to compare get function mapping. */
#if !defined(CONFIG_SOC_SERIES_STM32MP1X)
static uint32_t(*const get_timer_compare[TIMER_MAX_CH])(const TIM_TypeDef *) = {
	LL_TIM_OC_GetCompareCH1, LL_TIM_OC_GetCompareCH2,
	LL_TIM_OC_GetCompareCH3, LL_TIM_OC_GetCompareCH4,
};
#else
static uint32_t(*const get_timer_compare[TIMER_MAX_CH])(TIM_TypeDef *) = {
	LL_TIM_OC_GetCompareCH1, LL_TIM_OC_GetCompareCH2,
	LL_TIM_OC_GetCompareCH3, LL_TIM_OC_GetCompareCH4,
};
#endif
/** Channel to interrupt enable function mapping. */
static void(*const enable_it[TIMER_MAX_CH])(TIM_TypeDef *) = {
	LL_TIM_EnableIT_CC1, LL_TIM_EnableIT_CC2,
	LL_TIM_EnableIT_CC3, LL_TIM_EnableIT_CC4,
};

/** Channel to interrupt enable function mapping. */
static void(*const disable_it[TIMER_MAX_CH])(TIM_TypeDef *) = {
	LL_TIM_DisableIT_CC1, LL_TIM_DisableIT_CC2,
	LL_TIM_DisableIT_CC3, LL_TIM_DisableIT_CC4,
};

#ifdef CONFIG_ASSERT
/** Channel to interrupt enable check function mapping. */
#if !defined(CONFIG_SOC_SERIES_STM32MP1X)
static uint32_t(*const check_it_enabled[TIMER_MAX_CH])(const TIM_TypeDef *) = {
	LL_TIM_IsEnabledIT_CC1, LL_TIM_IsEnabledIT_CC2,
	LL_TIM_IsEnabledIT_CC3, LL_TIM_IsEnabledIT_CC4,
};
#else
static uint32_t(*const check_it_enabled[TIMER_MAX_CH])(TIM_TypeDef *) = {
	LL_TIM_IsEnabledIT_CC1, LL_TIM_IsEnabledIT_CC2,
	LL_TIM_IsEnabledIT_CC3, LL_TIM_IsEnabledIT_CC4,
};
#endif
#endif

/** Channel to interrupt flag clear function mapping. */
static void(*const clear_it_flag[TIMER_MAX_CH])(TIM_TypeDef *) = {
	LL_TIM_ClearFlag_CC1, LL_TIM_ClearFlag_CC2,
	LL_TIM_ClearFlag_CC3, LL_TIM_ClearFlag_CC4,
};

struct counter_stm32_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	atomic_t cc_int_pending;
	uint32_t freq;
};

struct counter_stm32_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_stm32_config {
	struct counter_config_info info;
	struct counter_stm32_ch_data *ch_data;
	TIM_TypeDef *timer;
	uint32_t prescaler;
	struct stm32_pclken pclken;
	void (*irq_config_func)(const struct device *dev);
	uint32_t irqn;
	/* Reset controller device configuration */
	const struct reset_dt_spec reset;

	LOG_INSTANCE_PTR_DECLARE(log);
};

static int counter_stm32_start(const struct device *dev)
{
	const struct counter_stm32_config *config = dev->config;
	TIM_TypeDef *timer = config->timer;

	/* enable counter */
	LL_TIM_EnableCounter(timer);

	return 0;
}

static int counter_stm32_stop(const struct device *dev)
{
	const struct counter_stm32_config *config = dev->config;
	TIM_TypeDef *timer = config->timer;

	/* disable counter */
	LL_TIM_DisableCounter(timer);

	return 0;
}

static uint32_t counter_stm32_get_top_value(const struct device *dev)
{
	const struct counter_stm32_config *config = dev->config;

	return LL_TIM_GetAutoReload(config->timer);
}

static uint32_t counter_stm32_read(const struct device *dev)
{
	const struct counter_stm32_config *config = dev->config;

	return LL_TIM_GetCounter(config->timer);
}

static int counter_stm32_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = counter_stm32_read(dev);
	return 0;
}

static uint32_t counter_stm32_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;

	if (likely(IS_BIT_MASK(top))) {
		return (val1 + val2) & top;
	}

	to_top = top - val1;

	return (val2 <= to_top) ? val1 + val2 : val2 - to_top - 1U;
}

static uint32_t counter_stm32_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1U - old;
}

static void counter_stm32_counter_stm32_set_cc_int_pending(const struct device *dev, uint8_t chan)
{
	const struct counter_stm32_config *config = dev->config;
	struct counter_stm32_data *data = dev->data;

	atomic_or(&data->cc_int_pending, BIT(chan));
	NVIC_SetPendingIRQ(config->irqn);
}

static int counter_stm32_set_cc(const struct device *dev, uint8_t id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_stm32_config *config = dev->config;
	struct counter_stm32_data *data = dev->data;

	__ASSERT_NO_MSG(data->guard_period < counter_stm32_get_top_value(dev));
	uint32_t val = alarm_cfg->ticks;
	uint32_t flags = alarm_cfg->flags;
	bool absolute = flags & COUNTER_ALARM_CFG_ABSOLUTE;
	bool irq_on_late;
	TIM_TypeDef *timer = config->timer;
	uint32_t top = counter_stm32_get_top_value(dev);
	int err = 0;
	uint32_t prev_val;
	uint32_t now;
	uint32_t diff;
	uint32_t max_rel_val;

	__ASSERT(!check_it_enabled[id](timer),
		 "Expected that CC interrupt is disabled.");

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future (now tick is the furthest
	 * future).
	 */
	now = counter_stm32_read(dev);
	prev_val = get_timer_compare[id](timer);
	set_timer_compare[id](timer, now);
	clear_it_flag[id](timer);

	if (absolute) {
		max_rel_val = top - data->guard_period;
		irq_on_late = flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		/* If relative value is smaller than half of the counter range
		 * it is assumed that there is a risk of setting value too late
		 * and late detection algorithm must be applied. When late
		 * setting is detected, interrupt shall be triggered for
		 * immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CC and counter.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = val < (top / 2U);
		/* limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? top / 2U : top;
		val = counter_stm32_ticks_add(now, val, top);
	}

	set_timer_compare[id](timer, val);

	/* decrement value to detect also case when val == counter_stm32_read(dev). Otherwise,
	 * condition would need to include comparing diff against 0.
	 */
	diff = counter_stm32_ticks_sub(val - 1U, counter_stm32_read(dev), top);
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			counter_stm32_counter_stm32_set_cc_int_pending(dev, id);
		} else {
			config->ch_data[id].callback = NULL;
		}
	} else {
		enable_it[id](timer);
	}

	return err;
}

static int counter_stm32_set_alarm(const struct device *dev, uint8_t chan,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_stm32_config *config = dev->config;
	struct counter_stm32_ch_data *chdata = &config->ch_data[chan];

	if (alarm_cfg->ticks >  counter_stm32_get_top_value(dev)) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;

	return counter_stm32_set_cc(dev, chan, alarm_cfg);
}

static int counter_stm32_cancel_alarm(const struct device *dev, uint8_t chan)
{
	const struct counter_stm32_config *config = dev->config;

	disable_it[chan](config->timer);
	config->ch_data[chan].callback = NULL;

	return 0;
}

static int counter_stm32_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_stm32_config *config = dev->config;
	TIM_TypeDef *timer = config->timer;
	struct counter_stm32_data *data = dev->data;
	int err = 0;

	for (int i = 0; i < counter_get_num_of_channels(dev); i++) {
		/* Overflow can be changed only when all alarms are
		 * disabled.
		 */
		if (config->ch_data[i].callback) {
			return -EBUSY;
		}
	}

	LL_TIM_DisableIT_UPDATE(timer);
	LL_TIM_SetAutoReload(timer, cfg->ticks);
	LL_TIM_ClearFlag_UPDATE(timer);

	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		LL_TIM_SetCounter(timer, 0);
	} else if (counter_stm32_read(dev) >= cfg->ticks) {
		err = -ETIME;
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			LL_TIM_SetCounter(timer, 0);
		}
	}

	if (cfg->callback) {
		LL_TIM_EnableIT_UPDATE(timer);
	}

	return err;
}

static uint32_t counter_stm32_get_pending_int(const struct device *dev)
{
	const struct counter_stm32_config *cfg = dev->config;
	uint32_t pending = 0;

	switch (counter_get_num_of_channels(dev)) {
	case 4U:
		pending |= LL_TIM_IsActiveFlag_CC4(cfg->timer);
		__fallthrough;
	case 3U:
		pending |= LL_TIM_IsActiveFlag_CC3(cfg->timer);
		__fallthrough;
	case 2U:
		pending |= LL_TIM_IsActiveFlag_CC2(cfg->timer);
		__fallthrough;
	case 1U:
		pending |= LL_TIM_IsActiveFlag_CC1(cfg->timer);
	}

	return !!pending;
}

/**
 * Obtain timer clock speed.
 *
 * @param pclken  Timer clock control subsystem.
 * @param tim_clk Where computed timer clock will be stored.
 *
 * @return 0 on success, error code otherwise.
 *
 * This function is ripped from the PWM driver; TODO handle code duplication.
 */
static int counter_stm32_get_tim_clk(const struct stm32_pclken *pclken, uint32_t *tim_clk)
{
	int r;
	const struct device *clk;
	uint32_t bus_clk, apb_psc;

	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	r = clock_control_get_rate(clk, (clock_control_subsys_t)pclken,
				   &bus_clk);
	if (r < 0) {
		return r;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = STM32_D2PPRE1;
	} else {
		apb_psc = STM32_D2PPRE2;
	}
#else
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
#if defined(CONFIG_SOC_SERIES_STM32MP1X)
		apb_psc = (uint32_t)(READ_BIT(RCC->APB1DIVR, RCC_APB1DIVR_APB1DIV));
#else
		apb_psc = STM32_APB1_PRESCALER;
#endif
	}
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
#if defined(CONFIG_SOC_SERIES_STM32MP1X)
		apb_psc = (uint32_t)(READ_BIT(RCC->APB2DIVR, RCC_APB2DIVR_APB2DIV));
#else
		apb_psc = STM32_APB2_PRESCALER;
#endif
	}
#endif
#endif

#if defined(RCC_DCKCFGR_TIMPRE) || defined(RCC_DCKCFGR1_TIMPRE) || \
	defined(RCC_CFGR_TIMPRE)
	/*
	 * There are certain series (some F4, F7 and H7) that have the TIMPRE
	 * bit to control the clock frequency of all the timers connected to
	 * APB1 and APB2 domains.
	 *
	 * Up to a certain threshold value of APB{1,2} prescaler, timer clock
	 * equals to HCLK. This threshold value depends on TIMPRE setting
	 * (2 if TIMPRE=0, 4 if TIMPRE=1). Above threshold, timer clock is set
	 * to a multiple of the APB domain clock PCLK{1,2} (2 if TIMPRE=0, 4 if
	 * TIMPRE=1).
	 */

	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
		/* TIMPRE = 0 */
		if (apb_psc <= 2u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 2u;
		}
	} else {
		/* TIMPRE = 1 */
		if (apb_psc <= 4u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 4u;
		}
	}
#else
	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1u) {
		*tim_clk = bus_clk;
	} else {
		*tim_clk = bus_clk * 2u;
	}
#endif

	return 0;
}

static int counter_stm32_init_timer(const struct device *dev)
{
	const struct counter_stm32_config *cfg = dev->config;
	struct counter_stm32_data *data = dev->data;
	TIM_TypeDef *timer = cfg->timer;
	LL_TIM_InitTypeDef init;
	uint32_t tim_clk;
	int r;

	/* initialize clock and check its speed  */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)&cfg->pclken);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}
	r = counter_stm32_get_tim_clk(&cfg->pclken, &tim_clk);
	if (r < 0) {
		LOG_ERR("Could not obtain timer clock (%d)", r);
		return r;
	}
	data->freq = tim_clk / (cfg->prescaler + 1U);

	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	/* Reset timer to default state using RCC */
	(void)reset_line_toggle_dt(&cfg->reset);

	/* config/enable IRQ */
	cfg->irq_config_func(dev);

	/* initialize timer */
	LL_TIM_StructInit(&init);

	init.Prescaler = cfg->prescaler;
	init.CounterMode = LL_TIM_COUNTERMODE_UP;
	init.Autoreload = counter_get_max_top_value(dev);
	init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;

	if (LL_TIM_Init(timer, &init) != SUCCESS) {
		LOG_ERR("Could not initialize timer");
		return -EIO;
	}

	return 0;
}

static uint32_t counter_stm32_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_stm32_data *data = dev->data;

	ARG_UNUSED(flags);
	return data->guard_period;
}

static int counter_stm32_set_guard_period(const struct device *dev, uint32_t guard,
					  uint32_t flags)
{
	struct counter_stm32_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(guard < counter_stm32_get_top_value(dev));

	data->guard_period = guard;
	return 0;
}

static uint32_t counter_stm32_get_freq(const struct device *dev)
{
	struct counter_stm32_data *data = dev->data;

	return data->freq;
}

static void counter_stm32_top_irq_handle(const struct device *dev)
{
	struct counter_stm32_data *data = dev->data;

	counter_top_callback_t cb = data->top_cb;

	__ASSERT(cb != NULL, "top event enabled - expecting callback");
	cb(dev, data->top_user_data);
}

static void counter_stm32_alarm_irq_handle(const struct device *dev, uint32_t id)
{
	const struct counter_stm32_config *config = dev->config;
	struct counter_stm32_data *data = dev->data;
	TIM_TypeDef *timer = config->timer;

	struct counter_stm32_ch_data *chdata;
	counter_alarm_callback_t cb;

	atomic_and(&data->cc_int_pending, ~BIT(id));
	disable_it[id](timer);

	chdata = &config->ch_data[id];
	cb = chdata->callback;
	chdata->callback = NULL;

	if (cb) {
		uint32_t cc_val = get_timer_compare[id](timer);

		cb(dev, id, cc_val, chdata->user_data);
	}
}

static const struct counter_driver_api counter_stm32_driver_api = {
	.start = counter_stm32_start,
	.stop = counter_stm32_stop,
	.get_value = counter_stm32_get_value,
	.set_alarm = counter_stm32_set_alarm,
	.cancel_alarm = counter_stm32_cancel_alarm,
	.set_top_value = counter_stm32_set_top_value,
	.get_pending_int = counter_stm32_get_pending_int,
	.get_top_value = counter_stm32_get_top_value,
	.get_guard_period = counter_stm32_get_guard_period,
	.set_guard_period = counter_stm32_set_guard_period,
	.get_freq = counter_stm32_get_freq,
};

#define TIM_IRQ_HANDLE_CC(timx, cc)						\
	do {									\
		bool hw_irq = LL_TIM_IsActiveFlag_CC##cc(timer) &&		\
			      LL_TIM_IsEnabledIT_CC##cc(timer);			\
		if (hw_irq || (data->cc_int_pending & BIT(cc - 1U))) {		\
			if (hw_irq) {						\
				LL_TIM_ClearFlag_CC##cc(timer);			\
			}							\
			counter_stm32_alarm_irq_handle(dev, cc - 1U);		\
		}								\
	} while (0)

void counter_stm32_irq_handler(const struct device *dev)
{
	const struct counter_stm32_config *config = dev->config;
	struct counter_stm32_data *data = dev->data;
	TIM_TypeDef *timer = config->timer;

	/* Capture compare events */
	switch (counter_get_num_of_channels(dev)) {
	case 4U:
		TIM_IRQ_HANDLE_CC(timer, 4);
		__fallthrough;
	case 3U:
		TIM_IRQ_HANDLE_CC(timer, 3);
		__fallthrough;
	case 2U:
		TIM_IRQ_HANDLE_CC(timer, 2);
		__fallthrough;
	case 1U:
		TIM_IRQ_HANDLE_CC(timer, 1);
	}

	/* TIM Update event */
	if (LL_TIM_IsActiveFlag_UPDATE(timer) && LL_TIM_IsEnabledIT_UPDATE(timer)) {
		LL_TIM_ClearFlag_UPDATE(timer);
		counter_stm32_top_irq_handle(dev);
	}
}

#define TIMER(idx)              DT_INST_PARENT(idx)

/** TIMx instance from DT */
#define TIM(idx) ((TIM_TypeDef *)DT_REG_ADDR(TIMER(idx)))

#define COUNTER_DEVICE_INIT(idx)						  \
	BUILD_ASSERT(DT_PROP(TIMER(idx), st_prescaler) <= 0xFFFF,		  \
		     "TIMER prescaler out of range");				  \
	BUILD_ASSERT(NUM_CH(TIM(idx)) <= TIMER_MAX_CH,				  \
		     "TIMER too many channels");				  \
										  \
	static struct counter_stm32_data counter##idx##_data;			  \
	static struct counter_stm32_ch_data counter##idx##_ch_data[TIMER_MAX_CH]; \
										  \
	static void counter_##idx##_stm32_irq_config(const struct device *dev)	  \
	{									  \
		IRQ_CONNECT(DT_IRQN(TIMER(idx)),				  \
			    DT_IRQ(TIMER(idx), priority),			  \
			    counter_stm32_irq_handler,				  \
			    DEVICE_DT_INST_GET(idx),				  \
			    0);							  \
		irq_enable(DT_IRQN(TIMER(idx)));				  \
	}									  \
										  \
	static const struct counter_stm32_config counter##idx##_config = {	  \
		.info = {							  \
			.max_top_value =					  \
				IS_TIM_32B_COUNTER_INSTANCE(TIM(idx)) ?		  \
				0xFFFFFFFF : 0x0000FFFF,			  \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			  \
			.channels = NUM_CH(TIM(idx)),				  \
		},								  \
		.ch_data = counter##idx##_ch_data,				  \
		.timer = TIM(idx),						  \
		.prescaler = DT_PROP(TIMER(idx), st_prescaler),			  \
		.pclken = {							  \
			.bus = DT_CLOCKS_CELL(TIMER(idx), bus),			  \
			.enr = DT_CLOCKS_CELL(TIMER(idx), bits)			  \
		},								  \
		.irq_config_func = counter_##idx##_stm32_irq_config,		  \
		.irqn = DT_IRQN(TIMER(idx)),					  \
		.reset = RESET_DT_SPEC_GET(TIMER(idx)),				  \
	};									  \
										  \
	DEVICE_DT_INST_DEFINE(idx,						  \
			      counter_stm32_init_timer,				  \
			      NULL,						  \
			      &counter##idx##_data,				  \
			      &counter##idx##_config,				  \
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,	  \
			      &counter_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT)
