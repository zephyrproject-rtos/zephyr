/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/counter.h>
#include <hal/nrf_timer.h>
#include <zephyr/sys/atomic.h>

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#define LOG_MODULE_NAME counter_timer
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define TIMER_CLOCK(timer_instance) NRF_TIMER_BASE_FREQUENCY_GET(timer_instance)

#define CC_TO_ID(cc_num) (cc_num - 2)

#define ID_TO_CC(idx) (nrf_timer_cc_channel_t)(idx + 2)

#define TOP_CH NRF_TIMER_CC_CHANNEL0
#define COUNTER_TOP_EVT NRF_TIMER_EVENT_COMPARE0
#define COUNTER_TOP_INT_MASK NRF_TIMER_INT_COMPARE0_MASK

#define COUNTER_OVERFLOW_SHORT NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK
#define COUNTER_READ_CC NRF_TIMER_CC_CHANNEL1

struct counter_nrfx_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	atomic_t cc_int_pending;
};

struct counter_nrfx_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_nrfx_config {
	struct counter_config_info info;
	struct counter_nrfx_ch_data *ch_data;
	NRF_TIMER_Type *timer;
	LOG_INSTANCE_PTR_DECLARE(log);
};

struct counter_timer_config {
	nrf_timer_bit_width_t bit_width;
	nrf_timer_mode_t mode;
	uint32_t prescaler;
};

static int start(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;

	nrf_timer_task_trigger(config->timer, NRF_TIMER_TASK_START);

	return 0;
}

static int stop(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;

	nrf_timer_task_trigger(config->timer, NRF_TIMER_TASK_SHUTDOWN);

	return 0;
}

static uint32_t get_top_value(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;

	return nrf_timer_cc_get(config->timer, TOP_CH);
}

static uint32_t read(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;
	NRF_TIMER_Type *timer = config->timer;

	nrf_timer_task_trigger(timer,
			       nrf_timer_capture_task_get(COUNTER_READ_CC));

	return nrf_timer_cc_get(timer, COUNTER_READ_CC);
}

static int get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = read(dev);
	return 0;
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
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	atomic_or(&data->cc_int_pending, BIT(chan));
	NRFX_IRQ_PENDING_SET(NRFX_IRQ_NUMBER_GET(config->timer));
}

static int set_cc(const struct device *dev, uint8_t id, uint32_t val,
		  uint32_t flags)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	__ASSERT_NO_MSG(data->guard_period < get_top_value(dev));
	bool absolute = flags & COUNTER_ALARM_CFG_ABSOLUTE;
	bool irq_on_late;
	NRF_TIMER_Type  *reg = config->timer;
	uint8_t chan = ID_TO_CC(id);
	nrf_timer_event_t evt = nrf_timer_compare_event_get(chan);
	uint32_t top = get_top_value(dev);
	int err = 0;
	uint32_t prev_val;
	uint32_t now;
	uint32_t diff;
	uint32_t max_rel_val;

	__ASSERT(nrf_timer_int_enable_check(reg,
				nrf_timer_compare_int_get(chan)) == 0,
				"Expected that CC interrupt is disabled.");

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future (now tick is the furthest
	 * future).
	 */
	now = read(dev);
	prev_val = nrf_timer_cc_get(reg, chan);
	nrf_timer_cc_set(reg, chan, now);
	nrf_timer_event_clear(reg, evt);

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

	nrf_timer_cc_set(reg, chan, val);

	/* decrement value to detect also case when val == read(dev). Otherwise,
	 * condition would need to include comparing diff against 0.
	 */
	diff = ticks_sub(val - 1, read(dev), top);
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
			config->ch_data[id].callback = NULL;
		}
	} else {
		nrf_timer_int_enable(reg, nrf_timer_compare_int_get(chan));
	}

	return err;
}

static int set_alarm(const struct device *dev, uint8_t chan,
			const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_nrfx_config *nrfx_config = dev->config;
	struct counter_nrfx_ch_data *chdata = &nrfx_config->ch_data[chan];

	if (alarm_cfg->ticks >  get_top_value(dev)) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;

	return set_cc(dev, chan, alarm_cfg->ticks, alarm_cfg->flags);
}

static int cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_nrfx_config *config = dev->config;
	uint32_t int_mask =  nrf_timer_compare_int_get(ID_TO_CC(chan_id));

	nrf_timer_int_disable(config->timer, int_mask);
	config->ch_data[chan_id].callback = NULL;

	return 0;
}

static int set_top_value(const struct device *dev,
			 const struct counter_top_cfg *cfg)
{
	const struct counter_nrfx_config *nrfx_config = dev->config;
	NRF_TIMER_Type *timer = nrfx_config->timer;
	struct counter_nrfx_data *data = dev->data;
	int err = 0;
	for (int i = 0; i < counter_get_num_of_channels(dev); i++) {
		/* Overflow can be changed only when all alarms are
		 * disables.
		 */
		if (nrfx_config->ch_data[i].callback) {
			return -EBUSY;
		}
	}

	nrf_timer_int_disable(timer, COUNTER_TOP_INT_MASK);
	nrf_timer_cc_set(timer, TOP_CH, cfg->ticks);
	nrf_timer_shorts_enable(timer, COUNTER_OVERFLOW_SHORT);

	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		nrf_timer_task_trigger(timer, NRF_TIMER_TASK_CLEAR);
	} else if (read(dev) >= cfg->ticks) {
		err = -ETIME;
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			nrf_timer_task_trigger(timer, NRF_TIMER_TASK_CLEAR);
		}
	}

	if (cfg->callback) {
		nrf_timer_int_enable(timer, COUNTER_TOP_INT_MASK);
	}

	return err;
}

static uint32_t get_pending_int(const struct device *dev)
{
	return 0;
}

static int init_timer(const struct device *dev,
		      const struct counter_timer_config *config)
{
	const struct counter_nrfx_config *nrfx_config = dev->config;
	NRF_TIMER_Type *reg = nrfx_config->timer;

	nrf_timer_bit_width_set(reg, config->bit_width);
	nrf_timer_mode_set(reg, config->mode);
	nrf_timer_prescaler_set(reg, config->prescaler);

	nrf_timer_cc_set(reg, TOP_CH, counter_get_max_top_value(dev));

	NRFX_IRQ_ENABLE(NRFX_IRQ_NUMBER_GET(reg));

	return 0;
}

static uint32_t get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_nrfx_data *data = dev->data;

	return data->guard_period;
}

static int set_guard_period(const struct device *dev, uint32_t guard,
			    uint32_t flags)
{
	struct counter_nrfx_data *data = dev->data;

	__ASSERT_NO_MSG(guard < get_top_value(dev));

	data->guard_period = guard;
	return 0;
}

static void top_irq_handle(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	NRF_TIMER_Type *reg = config->timer;
	counter_top_callback_t cb = data->top_cb;

	if (nrf_timer_event_check(reg, COUNTER_TOP_EVT) &&
		nrf_timer_int_enable_check(reg, COUNTER_TOP_INT_MASK)) {
		nrf_timer_event_clear(reg, COUNTER_TOP_EVT);
		__ASSERT(cb != NULL, "top event enabled - expecting callback");
		cb(dev, data->top_user_data);
	}
}

static void alarm_irq_handle(const struct device *dev, uint32_t id)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	uint32_t cc = ID_TO_CC(id);
	NRF_TIMER_Type *reg = config->timer;
	uint32_t int_mask = nrf_timer_compare_int_get(cc);
	nrf_timer_event_t evt = nrf_timer_compare_event_get(cc);
	bool hw_irq_pending = nrf_timer_event_check(reg, evt) &&
			      nrf_timer_int_enable_check(reg, int_mask);
	bool sw_irq_pending = data->cc_int_pending & BIT(cc);

	if (hw_irq_pending || sw_irq_pending) {
		struct counter_nrfx_ch_data *chdata;
		counter_alarm_callback_t cb;

		nrf_timer_event_clear(reg, evt);
		atomic_and(&data->cc_int_pending, ~BIT(cc));
		nrf_timer_int_disable(reg, int_mask);

		chdata = &config->ch_data[id];
		cb = chdata->callback;
		chdata->callback = NULL;

		if (cb) {
			uint32_t cc_val = nrf_timer_cc_get(reg, cc);

			cb(dev, id, cc_val, chdata->user_data);
		}
	}
}

static void irq_handler(const struct device *dev)
{
	top_irq_handle(dev);

	for (uint32_t i = 0; i < counter_get_num_of_channels(dev); i++) {
		alarm_irq_handle(dev, i);
	}
}

static const struct counter_driver_api counter_nrfx_driver_api = {
	.start = start,
	.stop = stop,
	.get_value = get_value,
	.set_alarm = set_alarm,
	.cancel_alarm = cancel_alarm,
	.set_top_value = set_top_value,
	.get_pending_int = get_pending_int,
	.get_top_value = get_top_value,
	.get_guard_period = get_guard_period,
	.set_guard_period = set_guard_period,
};

/*
 * Device instantiation is done with node labels due to HAL API
 * requirements. In particular, TIMERx_MAX_SIZE values from HALs
 * are indexed by peripheral number, so DT_INST APIs won't work.
 */

#define TIMER(idx)		DT_NODELABEL(timer##idx)
#define TIMER_PROP(idx, prop)	DT_PROP(TIMER(idx), prop)

#define TIMER_IRQ_CONNECT(idx)						       \
	COND_CODE_1(CONFIG_COUNTER_TIMER##idx##_ZLI,			       \
		(IRQ_DIRECT_CONNECT(DT_IRQN(TIMER(idx)),		       \
				    DT_IRQ(TIMER(idx), priority),	       \
				    counter_timer##idx##_isr_wrapper,	       \
				    IRQ_ZERO_LATENCY)),			       \
		(IRQ_CONNECT(DT_IRQN(TIMER(idx)), DT_IRQ(TIMER(idx), priority),\
			    irq_handler, DEVICE_DT_GET(TIMER(idx)), 0))	       \
	)

#define COUNTER_NRFX_TIMER_DEVICE(idx)					       \
	BUILD_ASSERT(TIMER_PROP(idx, prescaler) <=			       \
			TIMER_PRESCALER_PRESCALER_Msk,			       \
		     "TIMER prescaler out of range");			       \
	COND_CODE_1(CONFIG_COUNTER_TIMER##idx##_ZLI, (			       \
		ISR_DIRECT_DECLARE(counter_timer##idx##_isr_wrapper)	       \
		{							       \
			irq_handler(DEVICE_DT_GET(TIMER(idx)));		       \
			/* No rescheduling, it shall not access zephyr primitives. */ \
			return 0;					       \
		}), ())							       \
	static int counter_##idx##_init(const struct device *dev)	       \
	{								       \
		TIMER_IRQ_CONNECT(idx);					       \
		static const struct counter_timer_config config = {	       \
			.prescaler = TIMER_PROP(idx, prescaler),	       \
			.mode = NRF_TIMER_MODE_TIMER,			       \
			.bit_width = (TIMER##idx##_MAX_SIZE == 32) ?	       \
					NRF_TIMER_BIT_WIDTH_32 :	       \
					NRF_TIMER_BIT_WIDTH_16,		       \
		};							       \
		return init_timer(dev, &config);			       \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static struct counter_nrfx_ch_data				       \
		counter##idx##_ch_data[CC_TO_ID(TIMER##idx##_CC_NUM)];	       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##_config = {\
		.info = {						       \
			.max_top_value = (TIMER##idx##_MAX_SIZE == 32) ?       \
					0xffffffff : 0x0000ffff,	       \
			.freq = TIMER_CLOCK(NRF_TIMER##idx) /		       \
					(1 << TIMER_PROP(idx, prescaler)),     \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		       \
			.channels = CC_TO_ID(TIMER##idx##_CC_NUM),	       \
		},							       \
		.ch_data = counter##idx##_ch_data,			       \
		.timer = (NRF_TIMER_Type *)DT_REG_ADDR(TIMER(idx)),	       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)	       \
	};								       \
	DEVICE_DT_DEFINE(TIMER(idx),					       \
			    counter_##idx##_init,			       \
			    NULL,					       \
			    &counter_##idx##_data,			       \
			    &nrfx_counter_##idx##_config.info,		       \
			    PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,	       \
			    &counter_nrfx_driver_api)

#ifdef CONFIG_COUNTER_TIMER0
COUNTER_NRFX_TIMER_DEVICE(0);
#endif

#ifdef CONFIG_COUNTER_TIMER1
COUNTER_NRFX_TIMER_DEVICE(1);
#endif

#ifdef CONFIG_COUNTER_TIMER2
COUNTER_NRFX_TIMER_DEVICE(2);
#endif

#ifdef CONFIG_COUNTER_TIMER3
COUNTER_NRFX_TIMER_DEVICE(3);
#endif

#ifdef CONFIG_COUNTER_TIMER4
COUNTER_NRFX_TIMER_DEVICE(4);
#endif
