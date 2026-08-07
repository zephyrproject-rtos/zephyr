/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
/* ambiq-sdk includes */
#include <soc.h>

LOG_MODULE_REGISTER(ambiq_counter, CONFIG_COUNTER_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
#define SOC_TIMER_BASE CTIMER_BASE
#elif defined(CONFIG_SOC_SERIES_APOLLO4X) || defined(CONFIG_SOC_SERIES_APOLLO5X)
#define SOC_TIMER_BASE TIMER_BASE
#endif
static void counter_ambiq_isr(void *arg);

struct counter_ambiq_config {
	struct counter_config_info counter_info;
	uint32_t instance;
	uint32_t clk_src;
	void (*irq_config_func)(void);
};

struct counter_ambiq_data {
	counter_alarm_callback_t callback;
	uint32_t freq;
	void *user_data;
};

static struct k_spinlock lock;

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
static void counter_irq_config_func(void)
{
	/* Apollo3 counters share the same irq number, connect to counter0 once when init and handle
	 * different banks in counter_ambiq_isr
	 */
	static bool global_irq_init = true;

	if (!global_irq_init) {
		return;
	}

	global_irq_init = false;

	/* Shared irq config default to ctimer0. */
	NVIC_ClearPendingIRQ(CTIMER_IRQn);
	IRQ_CONNECT(CTIMER_IRQn, DT_IRQ(DT_INST_PARENT(0), priority), counter_ambiq_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(CTIMER_IRQn);
};
#endif

static uint32_t get_clock_cycles(uint32_t clock_sel)
{
	uint32_t ret = 0;

	switch (clock_sel) {
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	case 1:
		ret = 12000000;
		break;
	case 2:
		ret = 3000000;
		break;
	case 3:
		ret = 187500;
		break;
	case 4:
		ret = 47000;
		break;
	case 5:
		ret = 12000;
		break;
	case 6:
		ret = 32768;
		break;
	case 7:
		ret = 16384;
		break;
	case 8:
		ret = 2048;
		break;
	case 9:
		ret = 256;
		break;
	case 10:
		ret = 512;
		break;
	case 11:
		ret = 32;
		break;
	case 12:
		ret = 1000;
		break;
	case 13:
		ret = 116;
		break;
	case 14:
		ret = 100;
		break;
	case 15: /* todo: check on value */
		ret = 0;
		break;
	case 16:
		ret = 8192;
		break;
	case 17:
		ret = 4096;
		break;
	case 18:
		ret = 1024;
		break;
#elif defined(CONFIG_SOC_SERIES_APOLLO4X) || defined(CONFIG_SOC_SERIES_APOLLO5X)
	case 0:
		ret = 24000000;
		break;
	case 1:
		ret = 6000000;
		break;
	case 2:
		ret = 1500000;
		break;
	case 3:
		ret = 375000;
		break;
	case 4:
		ret = 93750;
		break;
	case 5:
		ret = 1000;
		break;
	case 6:
		ret = 500;
		break;
	case 7:
		ret = 31;
		break;
	case 8:
		ret = 1;
		break;
	case 9:
		ret = 256;
		break;
	case 10:
		ret = 32768;
		break;
	case 11:
		ret = 16384;
		break;
	case 12:
		ret = 8192;
		break;
	case 13:
		ret = 4096;
		break;
	case 14:
		ret = 2048;
		break;
	case 15:
		ret = 1024;
		break;
	case 16:
		ret = 256;
		break;
	case 17:
		ret = 100;
		break;
#endif
	default:
		ret = 32768;
		break;
	}

	return ret;
}

static int counter_ambiq_init(const struct device *dev)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	const struct counter_ambiq_config *cfg = dev->config;
	struct counter_ambiq_data *data = dev->data;

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	/* Timer configuration */
	am_hal_ctimer_config_t sContTimer;
	/* Create 32-bit timer */
	sContTimer.ui32Link = 1;
	/* Set up TimerA. */
	sContTimer.ui32TimerAConfig = (AM_HAL_CTIMER_FN_REPEAT | AM_HAL_CTIMER_INT_ENABLE |
				       (cfg->clk_src << CTIMER_CTRL0_TMRA0CLK_Pos));
	/* Set up TimerB. */
	sContTimer.ui32TimerBConfig = 0;

	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

	data->freq = get_clock_cycles(cfg->clk_src);

	am_hal_ctimer_clear(cfg->instance, AM_HAL_CTIMER_BOTH);
	am_hal_ctimer_config(cfg->instance, &sContTimer);
	counter_irq_config_func();
#else
	am_hal_timer_config_t tc;

	am_hal_timer_default_config_set(&tc);
	tc.eInputClock = cfg->clk_src;
	tc.eFunction = AM_HAL_TIMER_FN_UPCOUNT;
	tc.ui32PatternLimit = 0;

	data->freq = get_clock_cycles(cfg->clk_src);

	am_hal_timer_config(cfg->instance, &tc);
	cfg->irq_config_func();
#endif

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_start(const struct device *dev)
{
	const struct counter_ambiq_config *cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&lock);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	am_hal_ctimer_start(cfg->instance, AM_HAL_CTIMER_TIMERA);
#else
	am_hal_timer_start(cfg->instance);
#endif

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_stop(const struct device *dev)
{
	const struct counter_ambiq_config *cfg = dev->config;

	k_spinlock_key_t key = k_spin_lock(&lock);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	am_hal_ctimer_stop(cfg->instance, AM_HAL_CTIMER_BOTH);
#else
	am_hal_timer_stop(cfg->instance);
#endif

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_ambiq_config *cfg = dev->config;

	k_spinlock_key_t key = k_spin_lock(&lock);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	*ticks = (am_hal_ctimer_read(cfg->instance, AM_HAL_CTIMER_TIMERA)) |
		 (am_hal_ctimer_read(cfg->instance, AM_HAL_CTIMER_TIMERB) << 16);
#else
	*ticks = am_hal_timer_read(cfg->instance);
#endif

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_ambiq_data *data = dev->data;
	const struct counter_ambiq_config *cfg = dev->config;
	uint32_t now;

	counter_ambiq_get_value(dev, &now);

	k_spinlock_key_t key = k_spin_lock(&lock);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	am_hal_ctimer_int_clear(AM_HAL_CTIMER_INT_TIMERA0C0);
	am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA0C0);

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		am_hal_ctimer_compare_set(cfg->instance, AM_HAL_CTIMER_BOTH, 0,
					  now + alarm_cfg->ticks);
	} else {
		am_hal_ctimer_compare_set(cfg->instance, AM_HAL_CTIMER_BOTH, 0, alarm_cfg->ticks);
	}
#else
	/* Enable interrupt, due to counter_ambiq_cancel_alarm() disables it*/
	am_hal_timer_interrupt_clear(AM_HAL_TIMER_MASK(cfg->instance, AM_HAL_TIMER_COMPARE1));
	am_hal_timer_interrupt_enable(AM_HAL_TIMER_MASK(cfg->instance, AM_HAL_TIMER_COMPARE1));

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		am_hal_timer_compare1_set(cfg->instance, now + alarm_cfg->ticks);
	} else {
		am_hal_timer_compare1_set(cfg->instance, alarm_cfg->ticks);
	}
#endif

	data->user_data = alarm_cfg->user_data;
	data->callback = alarm_cfg->callback;

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	const struct counter_ambiq_config *cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&lock);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	am_hal_ctimer_int_disable(AM_HAL_CTIMER_INT_TIMERA0C0);
	/* Reset the compare register */
	am_hal_ctimer_compare_set(cfg->instance, AM_HAL_CTIMER_BOTH, 0, 0);
#else
	am_hal_timer_interrupt_disable(AM_HAL_TIMER_MASK(cfg->instance, AM_HAL_TIMER_COMPARE1));
	/* Reset the compare register */
	am_hal_timer_compare1_set(cfg->instance, 0);
#endif

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_ambiq_config *config = dev->config;

	if (cfg->ticks != config->counter_info.max_top_value) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static uint32_t counter_ambiq_get_pending_int(const struct device *dev)
{
	return 0;
}

static uint32_t counter_ambiq_get_top_value(const struct device *dev)
{
	const struct counter_ambiq_config *config = dev->config;

	return config->counter_info.max_top_value;
}

static uint32_t counter_ambiq_get_freq(const struct device *dev)
{
	const struct counter_ambiq_data *data = dev->data;

	return data->freq;
}

static DEVICE_API(counter, counter_api) = {
	.start = counter_ambiq_start,
	.stop = counter_ambiq_stop,
	.get_value = counter_ambiq_get_value,
	.set_alarm = counter_ambiq_set_alarm,
	.cancel_alarm = counter_ambiq_cancel_alarm,
	.set_top_value = counter_ambiq_set_top_value,
	.get_pending_int = counter_ambiq_get_pending_int,
	.get_top_value = counter_ambiq_get_top_value,
	.get_freq = counter_ambiq_get_freq,
};

#define APOLLO3_HANDLE_SHARED_TIMER_IRQ(n)                                                         \
	static const struct device *const dev_##n = DEVICE_DT_INST_GET(n);                         \
	struct counter_ambiq_data *const data_##n = dev_##n->data;                                 \
	uint32_t status_##n = CTIMERn(n)->INTSTAT;                                                 \
	status_##n &= CTIMERn(n)->INTEN;                                                           \
	if (status_##n) {                                                                          \
		CTIMERn(n)->INTCLR = AM_HAL_CTIMER_INT_TIMERA0C0;                                  \
		counter_ambiq_get_value(dev_##n, &now);                                            \
		if (data_##n->callback) {                                                          \
			data_##n->callback(dev_##n, 0, now, data_##n->user_data);                  \
		}                                                                                  \
	}

static void counter_ambiq_isr(void *arg)
{
	uint32_t now = 0;

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	ARG_UNUSED(arg);

	DT_INST_FOREACH_STATUS_OKAY(APOLLO3_HANDLE_SHARED_TIMER_IRQ)
#else
	const struct device *dev = (const struct device *)arg;
	struct counter_ambiq_data *data = dev->data;
	const struct counter_ambiq_config *cfg = dev->config;

	am_hal_timer_interrupt_clear(AM_HAL_TIMER_MASK(cfg->instance, AM_HAL_TIMER_COMPARE1));
	counter_ambiq_get_value(dev, &now);

	if (data->callback) {
		data->callback(dev, 0, now, data->user_data);
	}
#endif
}

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
/* Apollo3 counters share the same irq number, connect irq here will cause build error, so we
 * leave this function blank here and do it in counter_irq_config_func
 */
#define AMBIQ_COUNTER_CONFIG_FUNC(idx)                                                             \
	static void counter_irq_config_func_##idx(void)                                            \
	{                                                                                          \
	}
#else
#define AMBIQ_COUNTER_CONFIG_FUNC(idx)                                                             \
	static void counter_irq_config_func_##idx(void)                                            \
	{                                                                                          \
		NVIC_ClearPendingIRQ(DT_IRQN(DT_INST_PARENT(idx)));                                \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(idx)), DT_IRQ(DT_INST_PARENT(idx), priority),   \
			    counter_ambiq_isr, DEVICE_DT_INST_GET(idx), 0);                        \
		irq_enable(DT_IRQN(DT_INST_PARENT(idx)));                                          \
	}
#endif

#define AMBIQ_COUNTER_INIT(idx)                                                                    \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_INST_PARENT(idx)) == 1,                           \
		     "Too many children for Timer!");                                              \
	static void counter_irq_config_func_##idx(void);                                           \
	static struct counter_ambiq_data counter_data_##idx;                                       \
	static const struct counter_ambiq_config counter_config_##idx = {                          \
		.instance = (DT_REG_ADDR(DT_INST_PARENT(idx)) - SOC_TIMER_BASE) /                  \
			    DT_REG_SIZE(DT_INST_PARENT(idx)),                                      \
		.clk_src = DT_ENUM_IDX(DT_INST_PARENT(idx), clk_source),                           \
		.counter_info = {.max_top_value = UINT32_MAX,                                      \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
		.irq_config_func = counter_irq_config_func_##idx,                                  \
	};                                                                                         \
	AMBIQ_COUNTER_CONFIG_FUNC(idx)                                                             \
	DEVICE_DT_INST_DEFINE(idx, counter_ambiq_init, NULL, &counter_data_##idx,                  \
			      &counter_config_##idx, PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,   \
			      &counter_api);
DT_INST_FOREACH_STATUS_OKAY(AMBIQ_COUNTER_INIT);
