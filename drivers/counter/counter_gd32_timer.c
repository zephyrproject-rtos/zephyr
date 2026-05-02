/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_timer

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <gd32_timer.h>

LOG_MODULE_REGISTER(counter_gd32_timer);

#define TIMER_INT_CH(ch)  (TIMER_INT_CH0 << ch)
#define TIMER_FLAG_CH(ch) (TIMER_FLAG_CH0 << ch)
#define TIMER_INT_ALL	  (0xFFu)

struct counter_gd32_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_gd32_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	atomic_t cc_int_pending;
	uint32_t freq;
	struct counter_gd32_ch_data alarm[];
};

struct counter_gd32_config {
	struct counter_config_info counter_info;
	uint32_t reg;
	uint16_t clkid;
	struct reset_dt_spec reset;
	uint16_t prescaler;
	void (*irq_config)(const struct device *dev);
	void (*set_irq_pending)(void);
	uint32_t (*get_irq_pending)(void);
};

static uint32_t get_autoreload_value(const struct device *dev)
{
	const struct counter_gd32_config *config = dev->config;

	return TIMER_CAR(config->reg);
}

static void set_autoreload_value(const struct device *dev, uint32_t value)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_CAR(config->reg) = value;
}

static uint32_t get_counter(const struct device *dev)
{
	const struct counter_gd32_config *config = dev->config;

	return TIMER_CNT(config->reg);
}

static void set_counter(const struct device *dev, uint32_t value)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_CNT(config->reg) = value;
}

static void set_software_event_gen(const struct device *dev, uint8_t evt)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_SWEVG(config->reg) |= evt;
}

static void set_prescaler(const struct device *dev, uint16_t prescaler)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_PSC(config->reg) = prescaler;
}

static void set_compare_value(const struct device *dev, uint16_t chan,
			      uint32_t compare_value)
{
	const struct counter_gd32_config *config = dev->config;

	switch (chan) {
	case 0:
		TIMER_CH0CV(config->reg) = compare_value;
		break;
	case 1:
		TIMER_CH1CV(config->reg) = compare_value;
		break;
	case 2:
		TIMER_CH2CV(config->reg) = compare_value;
		break;
	case 3:
		TIMER_CH3CV(config->reg) = compare_value;
		break;
	}
}

static void interrupt_enable(const struct device *dev, uint32_t interrupt)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_DMAINTEN(config->reg) |= interrupt;
}

static void interrupt_disable(const struct device *dev, uint32_t interrupt)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_DMAINTEN(config->reg) &= ~interrupt;
}

static uint32_t interrupt_flag_get(const struct device *dev, uint32_t interrupt)
{
	const struct counter_gd32_config *config = dev->config;

	return (TIMER_DMAINTEN(config->reg) & TIMER_INTF(config->reg) &
		interrupt);
}

static void interrupt_flag_clear(const struct device *dev, uint32_t interrupt)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_INTF(config->reg) &= ~interrupt;
}

static int counter_gd32_timer_start(const struct device *dev)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_CTL0(config->reg) |= (uint32_t)TIMER_CTL0_CEN;

	return 0;
}

static int counter_gd32_timer_stop(const struct device *dev)
{
	const struct counter_gd32_config *config = dev->config;

	TIMER_CTL0(config->reg) &= ~(uint32_t)TIMER_CTL0_CEN;

	return 0;
}

static int counter_gd32_timer_get_value(const struct device *dev,
					uint32_t *ticks)
{
	*ticks = get_counter(dev);

	return 0;
}

static uint32_t counter_gd32_timer_get_top_value(const struct device *dev)
{
	return get_autoreload_value(dev);
}

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

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1 - old;
}

static void set_cc_int_pending(const struct device *dev, uint8_t chan)
{
	const struct counter_gd32_config *config = dev->config;
	struct counter_gd32_data *data = dev->data;

	atomic_or(&data->cc_int_pending, TIMER_INT_CH(chan));
	config->set_irq_pending();
}

static int set_cc(const struct device *dev, uint8_t chan, uint32_t val,
		  uint32_t flags)
{
	const struct counter_gd32_config *config = dev->config;
	struct counter_gd32_data *data = dev->data;

	__ASSERT_NO_MSG(data->guard_period <
			counter_gd32_timer_get_top_value(dev));
	bool absolute = flags & COUNTER_ALARM_CFG_ABSOLUTE;
	uint32_t top = counter_gd32_timer_get_top_value(dev);
	uint32_t now, diff, max_rel_val;
	bool irq_on_late;
	int err = 0;

	ARG_UNUSED(config);
	__ASSERT(!(TIMER_DMAINTEN(config->reg) & TIMER_INT_CH(chan)),
		 "Expected that CC interrupt is disabled.");

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future (now tick is the furthest
	 * future).
	 */
	now = get_counter(dev);
	set_compare_value(dev, chan, now);
	interrupt_flag_clear(dev, TIMER_FLAG_CH(chan));

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
		irq_on_late = val < (top / 2);
		/* limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? top / 2 : top;
		val = ticks_add(now, val, top);
	}

	set_compare_value(dev, chan, val);

	/* decrement value to detect also case when val == get_counter(dev).
	 * Otherwise, condition would need to include comparing diff against 0.
	 */
	diff = ticks_sub(val - 1, get_counter(dev), top);
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			set_cc_int_pending(dev, chan);
		} else {
			data->alarm[chan].callback = NULL;
		}
	} else {
		interrupt_enable(dev, TIMER_INT_CH(chan));
	}

	return err;
}

static int
counter_gd32_timer_set_alarm(const struct device *dev, uint8_t chan,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_gd32_data *data = dev->data;
	struct counter_gd32_ch_data *chdata = &data->alarm[chan];

	if (alarm_cfg->ticks > counter_gd32_timer_get_top_value(dev)) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;

	return set_cc(dev, chan, alarm_cfg->ticks, alarm_cfg->flags);
}

static int counter_gd32_timer_cancel_alarm(const struct device *dev,
					   uint8_t chan)
{
	struct counter_gd32_data *data = dev->data;

	interrupt_disable(dev, TIMER_INT_CH(chan));
	data->alarm[chan].callback = NULL;

	return 0;
}

static int counter_gd32_timer_set_top_value(const struct device *dev,
					    const struct counter_top_cfg *cfg)
{
	const struct counter_gd32_config *config = dev->config;
	struct counter_gd32_data *data = dev->data;
	int err = 0;

	for (uint32_t i = 0; i < config->counter_info.channels; i++) {
		/* Overflow can be changed only when all alarms are
		 * disables.
		 */
		if (data->alarm[i].callback) {
			return -EBUSY;
		}
	}

	interrupt_disable(dev, TIMER_INT_UP);
	set_autoreload_value(dev, cfg->ticks);
	interrupt_flag_clear(dev, TIMER_INT_FLAG_UP);

	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		set_counter(dev, 0);
	} else if (get_counter(dev) >= cfg->ticks) {
		err = -ETIME;
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			set_counter(dev, 0);
		}
	}

	if (cfg->callback) {
		interrupt_enable(dev, TIMER_INT_UP);
	}

	return err;
}

static uint32_t counter_gd32_timer_get_pending_int(const struct device *dev)
{
	const struct counter_gd32_config *cfg = dev->config;

	return cfg->get_irq_pending();
}

static uint32_t counter_gd32_timer_get_freq(const struct device *dev)
{
	struct counter_gd32_data *data = dev->data;

	return data->freq;
}

static uint32_t counter_gd32_timer_get_guard_period(const struct device *dev,
						    uint32_t flags)
{
	struct counter_gd32_data *data = dev->data;

	return data->guard_period;
}

static int counter_gd32_timer_set_guard_period(const struct device *dev,
					       uint32_t guard, uint32_t flags)
{
	struct counter_gd32_data *data = dev->data;

	__ASSERT_NO_MSG(guard < counter_gd32_timer_get_top_value(dev));

	data->guard_period = guard;
	return 0;
}

static void top_irq_handle(const struct device *dev)
{
	struct counter_gd32_data *data = dev->data;
	counter_top_callback_t cb = data->top_cb;

	if (interrupt_flag_get(dev, TIMER_INT_FLAG_UP) != 0) {
		interrupt_flag_clear(dev, TIMER_INT_FLAG_UP);
		__ASSERT(cb != NULL, "top event enabled - expecting callback");
		cb(dev, data->top_user_data);
	}
}

static void alarm_irq_handle(const struct device *dev, uint32_t chan)
{
	struct counter_gd32_data *data = dev->data;
	struct counter_gd32_ch_data *alarm = &data->alarm[chan];
	counter_alarm_callback_t cb;
	bool hw_irq_pending = !!(interrupt_flag_get(dev, TIMER_FLAG_CH(chan)));
	bool sw_irq_pending = data->cc_int_pending & TIMER_INT_CH(chan);

	if (hw_irq_pending || sw_irq_pending) {
		atomic_and(&data->cc_int_pending, ~TIMER_INT_CH(chan));
		interrupt_disable(dev, TIMER_INT_CH(chan));
		interrupt_flag_clear(dev, TIMER_FLAG_CH(chan));

		cb = alarm->callback;
		alarm->callback = NULL;

		if (cb) {
			cb(dev, chan, get_counter(dev), alarm->user_data);
		}
	}
}

static void irq_handler(const struct device *dev)
{
	const struct counter_gd32_config *cfg = dev->config;

	top_irq_handle(dev);

	for (uint32_t i = 0; i < cfg->counter_info.channels; i++) {
		alarm_irq_handle(dev, i);
	}
}

static int counter_gd32_timer_init(const struct device *dev)
{
	const struct counter_gd32_config *cfg = dev->config;
	struct counter_gd32_data *data = dev->data;
	uint32_t pclk;

	clock_control_on(GD32_CLOCK_CONTROLLER,
			 (clock_control_subsys_t)&cfg->clkid);
	clock_control_get_rate(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clkid, &pclk);

	data->freq = pclk / (cfg->prescaler + 1);

	interrupt_disable(dev, TIMER_INT_ALL);
	reset_line_toggle_dt(&cfg->reset);

	cfg->irq_config(dev);
	set_prescaler(dev, cfg->prescaler);
	set_autoreload_value(dev, cfg->counter_info.max_top_value);
	set_software_event_gen(dev, TIMER_SWEVG_UPG);

	return 0;
}

static DEVICE_API(counter, counter_api) = {
	.start = counter_gd32_timer_start,
	.stop = counter_gd32_timer_stop,
	.get_value = counter_gd32_timer_get_value,
	.set_alarm = counter_gd32_timer_set_alarm,
	.cancel_alarm = counter_gd32_timer_cancel_alarm,
	.set_top_value = counter_gd32_timer_set_top_value,
	.get_pending_int = counter_gd32_timer_get_pending_int,
	.get_top_value = counter_gd32_timer_get_top_value,
	.get_guard_period = counter_gd32_timer_get_guard_period,
	.set_guard_period = counter_gd32_timer_set_guard_period,
	.get_freq = counter_gd32_timer_get_freq,
};

#define TIMER_IRQ_CONFIG(n)                                                    \
	static void irq_config_##n(const struct device *dev)                   \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, global, irq),               \
			    DT_INST_IRQ_BY_NAME(n, global, priority),          \
			    irq_handler, DEVICE_DT_INST_GET(n), 0);            \
		irq_enable(DT_INST_IRQ_BY_NAME(n, global, irq));               \
	}                                                                      \
	static void set_irq_pending_##n(void)                                  \
	{                                                                      \
		(NVIC_SetPendingIRQ(DT_INST_IRQ_BY_NAME(n, global, irq)));     \
	}                                                                      \
	static uint32_t get_irq_pending_##n(void)                              \
	{                                                                      \
		return NVIC_GetPendingIRQ(                                     \
			DT_INST_IRQ_BY_NAME(n, global, irq));                  \
	}

#define TIMER_IRQ_CONFIG_ADVANCED(n)                                           \
	static void irq_config_##n(const struct device *dev)                   \
	{                                                                      \
		IRQ_CONNECT((DT_INST_IRQ_BY_NAME(n, up, irq)),                 \
			    (DT_INST_IRQ_BY_NAME(n, up, priority)),            \
			    irq_handler, (DEVICE_DT_INST_GET(n)), 0);          \
		irq_enable((DT_INST_IRQ_BY_NAME(n, up, irq)));                 \
		IRQ_CONNECT((DT_INST_IRQ_BY_NAME(n, cc, irq)),                 \
			    (DT_INST_IRQ_BY_NAME(n, cc, priority)),            \
			    irq_handler, (DEVICE_DT_INST_GET(n)), 0);          \
		irq_enable((DT_INST_IRQ_BY_NAME(n, cc, irq)));                 \
	}                                                                      \
	static void set_irq_pending_##n(void)                                  \
	{                                                                      \
		(NVIC_SetPendingIRQ(DT_INST_IRQ_BY_NAME(n, cc, irq)));         \
	}                                                                      \
	static uint32_t get_irq_pending_##n(void)                              \
	{                                                                      \
		return NVIC_GetPendingIRQ(DT_INST_IRQ_BY_NAME(n, cc, irq));    \
	}

#define GD32_TIMER_INIT(n)                                                     \
	COND_CODE_1(DT_INST_PROP(n, is_advanced),                              \
		    (TIMER_IRQ_CONFIG_ADVANCED(n)), (TIMER_IRQ_CONFIG(n)));    \
	static struct counter_gd32_data_##n {                                  \
		struct counter_gd32_data data;                                 \
		struct counter_gd32_ch_data alarm[DT_INST_PROP(n, channels)];  \
	} timer_data_##n = {0};                                                \
	static const struct counter_gd32_config timer_config_##n = {           \
		.counter_info = {.max_top_value = COND_CODE_1(                 \
					 DT_INST_PROP(n, is_32bit),            \
					 (UINT32_MAX), (UINT16_MAX)),          \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,        \
				 .freq = 0,                                    \
				 .channels = DT_INST_PROP(n, channels)},       \
		.reg = DT_INST_REG_ADDR(n),                                    \
		.clkid = DT_INST_CLOCKS_CELL(n, id),                           \
		.reset = RESET_DT_SPEC_INST_GET(n),                            \
		.prescaler = DT_INST_PROP(n, prescaler),                       \
		.irq_config = irq_config_##n,                                  \
		.set_irq_pending = set_irq_pending_##n,                        \
		.get_irq_pending = get_irq_pending_##n,                        \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, counter_gd32_timer_init, NULL,                \
			      &timer_data_##n, &timer_config_##n,              \
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,      \
			      &counter_api);

DT_INST_FOREACH_STATUS_OKAY(GD32_TIMER_INIT);
