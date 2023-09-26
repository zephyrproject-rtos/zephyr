/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/counter.h>
#include <hal/nrf_rtc.h>
#ifdef CONFIG_CLOCK_CONTROL_NRF
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif
#include <zephyr/sys/atomic.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#define LOG_MODULE_NAME counter_rtc
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

#define ERR(...) LOG_INST_ERR( \
	((const struct counter_nrfx_config *)dev->config)->log, __VA_ARGS__)
#define WRN(...) LOG_INST_WRN( \
	((const struct counter_nrfx_config *)dev->config)->log, __VA_ARGS__)
#define INF(...) LOG_INST_INF( \
	((const struct counter_nrfx_config *)dev->config)->log, __VA_ARGS__)
#define DBG(...) LOG_INST_DBG( \
	((const struct counter_nrfx_config *)dev->config)->log, __VA_ARGS__)

#define DT_DRV_COMPAT nordic_nrf_rtc

#define COUNTER_GET_TOP_CH(dev) counter_get_num_of_channels(dev)

#define IS_FIXED_TOP(dev) COND_CODE_1(CONFIG_COUNTER_RTC_CUSTOM_TOP_SUPPORT, \
		(((const struct counter_nrfx_config *)dev->config)->fixed_top), (true))

#define IS_PPI_WRAP(dev) COND_CODE_1(CONFIG_COUNTER_RTC_WITH_PPI_WRAP, \
		(((const struct counter_nrfx_config *)dev->config)->use_ppi), (false))

#define CC_ADJUSTED_OFFSET 16
#define CC_ADJ_MASK(chan) (BIT(chan + CC_ADJUSTED_OFFSET))

struct counter_nrfx_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t top;
	uint32_t guard_period;
	/* Store channel interrupt pending and CC adjusted flags. */
	atomic_t ipend_adj;
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	uint8_t ppi_ch;
#endif
};

struct counter_nrfx_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_nrfx_config {
	struct counter_config_info info;
	struct counter_nrfx_ch_data *ch_data;
	NRF_RTC_Type *rtc;
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	bool use_ppi;
#endif
#if CONFIG_COUNTER_RTC_CUSTOM_TOP_SUPPORT
	bool fixed_top;
#endif
	LOG_INSTANCE_PTR_DECLARE(log);
};

static int start(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;

	nrf_rtc_task_trigger(config->rtc, NRF_RTC_TASK_START);

	return 0;
}

static int stop(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;

	nrf_rtc_task_trigger(config->rtc, NRF_RTC_TASK_STOP);

	return 0;
}

static uint32_t read(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;

	return nrf_rtc_counter_get(config->rtc);
}

static int get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = read(dev);
	return 0;
}

/* Function calculates distance between to values assuming that one first
 * argument is in front and that values wrap.
 */
static uint32_t ticks_sub(const struct device *dev, uint32_t val,
			  uint32_t old, uint32_t top)
{
	if (IS_FIXED_TOP(dev)) {
		return (val - old) & NRF_RTC_COUNTER_MAX;
	} else if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1 - old;

}

static uint32_t skip_zero_on_custom_top(uint32_t val, uint32_t top)
{
	/* From Product Specification: If a CC register value is 0 when
	 * a CLEAR task is set, this will not trigger a COMPARE event.
	 */
	if (unlikely(val == 0) && (top != NRF_RTC_COUNTER_MAX)) {
		val++;
	}

	return val;
}

static uint32_t ticks_add(const struct device *dev, uint32_t val1,
			  uint32_t val2, uint32_t top)
{
	uint32_t sum = val1 + val2;

	if (IS_FIXED_TOP(dev)) {
		ARG_UNUSED(top);
		return sum & NRF_RTC_COUNTER_MAX;
	}
	if (likely(IS_BIT_MASK(top))) {
		sum = sum & top;
	} else {
		sum = sum > top ? sum - (top + 1) : sum;
	}

	return skip_zero_on_custom_top(sum, top);
}

static void set_cc_int_pending(const struct device *dev, uint8_t chan)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	atomic_or(&data->ipend_adj, BIT(chan));
	NRFX_IRQ_PENDING_SET(NRFX_IRQ_NUMBER_GET(config->rtc));
}

/** @brief Handle case when CC value equals COUNTER+1.
 *
 * RTC will not generate event if CC value equals COUNTER+1. If such CC is
 * about to be set then special algorithm is applied. Since counter must not
 * expire before expected value, CC is set to COUNTER+2. If COUNTER progressed
 * during that time it means that target value is reached and interrupt is
 * manually triggered. If not then interrupt is enabled since it is expected
 * that CC value will generate event.
 *
 * Additionally, an information about CC adjustment is stored. This information
 * is used in the callback to return original CC value which was requested by
 * the user.
 */
static void handle_next_tick_case(const struct device *dev, uint8_t chan,
				  uint32_t now, uint32_t val)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	val = ticks_add(dev, val, 1, data->top);
	nrf_rtc_cc_set(config->rtc, chan, val);
	atomic_or(&data->ipend_adj, CC_ADJ_MASK(chan));
	if (nrf_rtc_counter_get(config->rtc) != now) {
		set_cc_int_pending(dev, chan);
	} else {
		nrf_rtc_int_enable(config->rtc, NRF_RTC_CHANNEL_INT_MASK(chan));
	}
}

/*
 * @brief Set COMPARE value with optional too late setting detection.
 *
 * Setting CC algorithm takes into account:
 * - Current COMPARE value written to the register may be close to the current
 *   COUNTER value thus COMPARE event may be generated at any moment
 * - Next COMPARE value may be soon in the future. Taking into account potential
 *   preemption COMPARE value may be set too late.
 * - RTC registers are clocked with LF clock (32kHz) and sampled between two
 *   LF ticks.
 * - Setting COMPARE register to COUNTER+1 does not generate COMPARE event if
 *   done half tick before tick boundary.
 *
 * Algorithm assumes that:
 * - COMPARE interrupt is disabled
 * - absolute value is taking into account guard period. It means that
 *   it won't be further in future than <top> - <guard_period> from now.
 *
 * @param dev	Device.
 * @param chan	COMPARE channel.
 * @param val	Value (absolute or relative).
 * @param flags	Alarm flags.
 *
 * @retval 0 if COMPARE value was set on time and COMPARE interrupt is expected.
 * @retval -ETIME if absolute alarm was set too late and error reporting is
 *		  enabled.
 *
 */
static int set_cc(const struct device *dev, uint8_t chan, uint32_t val,
		  uint32_t flags)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	__ASSERT_NO_MSG(data->guard_period < data->top);
	NRF_RTC_Type  *rtc = config->rtc;
	nrf_rtc_event_t evt;
	uint32_t prev_val;
	uint32_t top;
	uint32_t now;
	uint32_t diff;
	uint32_t int_mask = NRF_RTC_CHANNEL_INT_MASK(chan);
	int err = 0;
	uint32_t max_rel_val;
	bool absolute = flags & COUNTER_ALARM_CFG_ABSOLUTE;
	bool irq_on_late;

	__ASSERT(nrf_rtc_int_enable_check(rtc, int_mask) == 0,
			"Expected that CC interrupt is disabled.");

	evt = NRF_RTC_CHANNEL_EVENT_ADDR(chan);
	top =  data->top;
	now = nrf_rtc_counter_get(rtc);

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future (now tick is the furthest
	 * future). If CC was set to next tick we need to wait for up to 15us
	 * (half of 32k tick) and clean potential event. After that time there
	 * is no risk of unwanted event.
	 */
	prev_val = nrf_rtc_cc_get(rtc, chan);
	nrf_rtc_event_clear(rtc, evt);
	nrf_rtc_cc_set(rtc, chan, now);
	nrf_rtc_event_enable(rtc, int_mask);

	if (ticks_sub(dev, prev_val, now, top) == 1) {
		NRFX_DELAY_US(15);
		nrf_rtc_event_clear(rtc, evt);
	}

	now = nrf_rtc_counter_get(rtc);

	if (absolute) {
		val = skip_zero_on_custom_top(val, top);
		irq_on_late = flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
		max_rel_val = top - data->guard_period;
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
		val = ticks_add(dev, now, val, top);
	}

	diff = ticks_sub(dev, val, now, top);
	if (diff == 1) {
		/* CC cannot be set to COUNTER+1 because that will not
		 * generate an event. In that case, special handling is
		 * performed (attempt to set CC to COUNTER+2).
		 */
		handle_next_tick_case(dev, chan, now, val);
	} else {
		nrf_rtc_cc_set(rtc, chan, val);
		now = nrf_rtc_counter_get(rtc);

		/* decrement value to detect also case when val == read(dev).
		 * Otherwise, condition would need to include comparing diff
		 * against 0.
		 */
		diff = ticks_sub(dev, val - 1, now, top);
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
				config->ch_data[chan].callback = NULL;
			}
		} else if (diff == 0) {
			/* It is possible that setting CC was interrupted and
			 * CC might be set to COUNTER+1 value which will not
			 * generate an event. In that case, special handling
			 * is performed (attempt to set CC to COUNTER+2).
			 */
			handle_next_tick_case(dev, chan, now, val);
		} else {
			nrf_rtc_int_enable(rtc, int_mask);
		}
	}

	return err;
}

static int set_channel_alarm(const struct device *dev, uint8_t chan,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_nrfx_config *nrfx_config = dev->config;
	struct counter_nrfx_data *data = dev->data;
	struct counter_nrfx_ch_data *chdata = &nrfx_config->ch_data[chan];

	if (alarm_cfg->ticks > data->top) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;
	atomic_and(&data->ipend_adj, ~CC_ADJ_MASK(chan));

	return set_cc(dev, chan, alarm_cfg->ticks, alarm_cfg->flags);
}

static void disable(const struct device *dev, uint8_t chan)
{
	const struct counter_nrfx_config *config = dev->config;
	NRF_RTC_Type *rtc = config->rtc;
	nrf_rtc_event_t evt = NRF_RTC_CHANNEL_EVENT_ADDR(chan);

	nrf_rtc_int_disable(rtc, NRF_RTC_CHANNEL_INT_MASK(chan));
	nrf_rtc_event_disable(rtc, NRF_RTC_CHANNEL_INT_MASK(chan));
	nrf_rtc_event_clear(rtc, evt);
	config->ch_data[chan].callback = NULL;
}

static int cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	disable(dev, chan_id);

	return 0;
}

static int ppi_setup(const struct device *dev, uint8_t chan)
{
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	const struct counter_nrfx_config *nrfx_config = dev->config;
	struct counter_nrfx_data *data = dev->data;
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	nrf_rtc_event_t evt = NRF_RTC_CHANNEL_EVENT_ADDR(chan);
	nrfx_err_t result;

	if (!nrfx_config->use_ppi) {
		return 0;
	}

	nrf_rtc_event_enable(rtc, NRF_RTC_CHANNEL_INT_MASK(chan));
#ifdef DPPI_PRESENT
	result = nrfx_dppi_channel_alloc(&data->ppi_ch);
	if (result != NRFX_SUCCESS) {
		ERR("Failed to allocate PPI channel.");
		return -ENODEV;
	}

	nrf_rtc_subscribe_set(rtc, NRF_RTC_TASK_CLEAR, data->ppi_ch);
	nrf_rtc_publish_set(rtc, evt, data->ppi_ch);
	(void)nrfx_dppi_channel_enable(data->ppi_ch);
#else /* DPPI_PRESENT */
	uint32_t evt_addr;
	uint32_t task_addr;

	evt_addr = nrf_rtc_event_address_get(rtc, evt);
	task_addr = nrf_rtc_task_address_get(rtc, NRF_RTC_TASK_CLEAR);

	result = nrfx_ppi_channel_alloc(&data->ppi_ch);
	if (result != NRFX_SUCCESS) {
		ERR("Failed to allocate PPI channel.");
		return -ENODEV;
	}
	(void)nrfx_ppi_channel_assign(data->ppi_ch, evt_addr, task_addr);
	(void)nrfx_ppi_channel_enable(data->ppi_ch);
#endif
#endif /* CONFIG_COUNTER_RTC_WITH_PPI_WRAP */
	return 0;
}

static void ppi_free(const struct device *dev, uint8_t chan)
{
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	const struct counter_nrfx_config *nrfx_config = dev->config;
	struct counter_nrfx_data *data = dev->data;
	uint8_t ppi_ch = data->ppi_ch;
	NRF_RTC_Type *rtc = nrfx_config->rtc;

	if (!nrfx_config->use_ppi) {
		return;
	}
	nrf_rtc_event_disable(rtc, NRF_RTC_CHANNEL_INT_MASK(chan));
#ifdef DPPI_PRESENT
	nrf_rtc_event_t evt = NRF_RTC_CHANNEL_EVENT_ADDR(chan);

	(void)nrfx_dppi_channel_disable(ppi_ch);
	nrf_rtc_subscribe_clear(rtc, NRF_RTC_TASK_CLEAR);
	nrf_rtc_publish_clear(rtc, evt);
	(void)nrfx_dppi_channel_free(ppi_ch);
#else /* DPPI_PRESENT */
	(void)nrfx_ppi_channel_disable(ppi_ch);
	(void)nrfx_ppi_channel_free(ppi_ch);
#endif
#endif
}

/* Return true if counter must be cleared by the CPU. It is cleared
 * automatically in case of max top value or PPI usage.
 */
static bool sw_wrap_required(const struct device *dev)
{
	struct counter_nrfx_data *data = dev->data;

	return (data->top != NRF_RTC_COUNTER_MAX) && !IS_PPI_WRAP(dev);
}

static int set_fixed_top_value(const struct device *dev,
				const struct counter_top_cfg *cfg)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	NRF_RTC_Type *rtc = config->rtc;

	if (cfg->ticks != NRF_RTC_COUNTER_MAX) {
		return -EINVAL;
	}

	nrf_rtc_int_disable(rtc, NRF_RTC_INT_OVERFLOW_MASK);
	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
	}

	if (cfg->callback) {
		nrf_rtc_int_enable(rtc, NRF_RTC_INT_OVERFLOW_MASK);
	}

	return 0;
}

static int set_top_value(const struct device *dev,
			 const struct counter_top_cfg *cfg)
{
	const struct counter_nrfx_config *nrfx_config = dev->config;
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	struct counter_nrfx_data *dev_data = dev->data;
	uint32_t top_ch = COUNTER_GET_TOP_CH(dev);
	int err = 0;

	if (IS_FIXED_TOP(dev)) {
		return set_fixed_top_value(dev, cfg);
	}

	for (int i = 0; i < counter_get_num_of_channels(dev); i++) {
		/* Overflow can be changed only when all alarms are
		 * disables.
		 */
		if (nrfx_config->ch_data[i].callback) {
			return -EBUSY;
		}
	}

	nrf_rtc_int_disable(rtc, NRF_RTC_CHANNEL_INT_MASK(top_ch));

	if (IS_PPI_WRAP(dev)) {
		if ((dev_data->top == NRF_RTC_COUNTER_MAX) &&
				cfg->ticks != NRF_RTC_COUNTER_MAX) {
			err = ppi_setup(dev, top_ch);
		} else if (((dev_data->top != NRF_RTC_COUNTER_MAX) &&
				cfg->ticks == NRF_RTC_COUNTER_MAX)) {
			ppi_free(dev, top_ch);
		}
	}

	dev_data->top_cb = cfg->callback;
	dev_data->top_user_data = cfg->user_data;
	dev_data->top = cfg->ticks;
	nrf_rtc_cc_set(rtc, top_ch, cfg->ticks);

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
	} else if (read(dev) >= cfg->ticks) {
		err = -ETIME;
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
		}
	}

	if (cfg->callback || sw_wrap_required(dev)) {
		nrf_rtc_int_enable(rtc, NRF_RTC_CHANNEL_INT_MASK(top_ch));
	}

	return err;
}

static uint32_t get_pending_int(const struct device *dev)
{
	return 0;
}

static int init_rtc(const struct device *dev, uint32_t prescaler)
{
	const struct counter_nrfx_config *nrfx_config = dev->config;
	struct counter_nrfx_data *data = dev->data;
	struct counter_top_cfg top_cfg = {
		.ticks = NRF_RTC_COUNTER_MAX
	};
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	int err;

#ifdef CONFIG_CLOCK_CONTROL_NRF
	z_nrf_clock_control_lf_on(CLOCK_CONTROL_NRF_LF_START_NOWAIT);
#endif

	nrf_rtc_prescaler_set(rtc, prescaler);

	NRFX_IRQ_ENABLE(NRFX_IRQ_NUMBER_GET(rtc));

	data->top = NRF_RTC_COUNTER_MAX;
	err = set_top_value(dev, &top_cfg);
	DBG("Initialized");

	return err;
}

static uint32_t get_top_value(const struct device *dev)
{
	struct counter_nrfx_data *data = dev->data;

	return data->top;
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

	data->guard_period = guard;
	return 0;
}

static void top_irq_handle(const struct device *dev)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	NRF_RTC_Type *rtc = config->rtc;
	counter_top_callback_t cb = data->top_cb;
	nrf_rtc_event_t top_evt;

	top_evt = IS_FIXED_TOP(dev) ?
		  NRF_RTC_EVENT_OVERFLOW :
		  NRF_RTC_CHANNEL_EVENT_ADDR(counter_get_num_of_channels(dev));

	if (nrf_rtc_event_check(rtc, top_evt)) {
		nrf_rtc_event_clear(rtc, top_evt);

		/* Perform manual clear if custom top value is used and PPI
		 * clearing is not used.
		 */
		if (!IS_FIXED_TOP(dev) && !IS_PPI_WRAP(dev)) {
			nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
		}

		if (cb) {
			cb(dev, data->top_user_data);
		}
	}
}

static void alarm_irq_handle(const struct device *dev, uint32_t chan)
{
	const struct counter_nrfx_config *config = dev->config;
	struct counter_nrfx_data *data = dev->data;

	NRF_RTC_Type *rtc = config->rtc;
	nrf_rtc_event_t evt = NRF_RTC_CHANNEL_EVENT_ADDR(chan);
	uint32_t int_mask = NRF_RTC_CHANNEL_INT_MASK(chan);
	bool hw_irq_pending = nrf_rtc_event_check(rtc, evt) &&
			      nrf_rtc_int_enable_check(rtc, int_mask);
	bool sw_irq_pending = data->ipend_adj & BIT(chan);

	if (hw_irq_pending || sw_irq_pending) {
		struct counter_nrfx_ch_data *chdata;
		counter_alarm_callback_t cb;

		nrf_rtc_event_clear(rtc, evt);
		atomic_and(&data->ipend_adj, ~BIT(chan));
		nrf_rtc_int_disable(rtc, int_mask);

		chdata = &config->ch_data[chan];
		cb = chdata->callback;
		chdata->callback = NULL;

		if (cb) {
			uint32_t cc = nrf_rtc_cc_get(rtc, chan);

			if (data->ipend_adj & CC_ADJ_MASK(chan)) {
				cc = ticks_sub(dev, cc, 1, data->top);
			}

			cb(dev, chan, cc, chdata->user_data);
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
	.set_alarm = set_channel_alarm,
	.cancel_alarm = cancel_alarm,
	.set_top_value = set_top_value,
	.get_pending_int = get_pending_int,
	.get_top_value = get_top_value,
	.get_guard_period = get_guard_period,
	.set_guard_period = set_guard_period,
};

/*
 * Devicetree access is done with node labels due to HAL API
 * requirements. In particular, RTCx_CC_NUM values from HALs
 * are indexed by peripheral number, so DT_INST APIs won't work.
 */

#define RTC_IRQ_CONNECT(idx)						       \
	COND_CODE_1(DT_INST_PROP(idx, zli),				       \
		(IRQ_DIRECT_CONNECT(DT_INST_IRQN(idx),			       \
				    DT_INST_IRQ(idx, priority),		       \
				    counter_rtc##idx##_isr_wrapper,	       \
				    IRQ_ZERO_LATENCY)),			       \
		(IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),    \
			    irq_handler, DEVICE_DT_INST_GET(idx), 0))	       \
	)

#define COUNTER_NRF_RTC_DEVICE(idx)					       \
	BUILD_ASSERT((DT_INST_PROP(idx, prescaler) - 1) <=		       \
		     RTC_PRESCALER_PRESCALER_Msk,			       \
		     "RTC prescaler out of range");			       \
	COND_CODE_1(DT_INST_PROP(idx, zli), (				       \
		ISR_DIRECT_DECLARE(counter_rtc##idx##_isr_wrapper)	       \
		{							       \
			irq_handler(DEVICE_DT_INST_GET(idx));		       \
			/* No rescheduling, it shall not access zephyr primitives. */ \
			return 0;					       \
		}), ())							       \
	static int counter_##idx##_init(const struct device *dev)	       \
	{								       \
		RTC_IRQ_CONNECT(idx);					       \
		return init_rtc(dev, DT_INST_PROP(idx, prescaler) - 1);	       \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static struct counter_nrfx_ch_data				       \
		counter##idx##_ch_data[DT_INST_PROP(idx, cc_num)];	       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##_config = {\
		.info = {						       \
			.max_top_value = NRF_RTC_COUNTER_MAX,		       \
			.freq = DT_INST_PROP(idx, clock_frequency) /	       \
				DT_INST_PROP(idx, prescaler),		       \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		       \
			.channels = DT_INST_PROP(idx, fixed_top)	       \
				  ? DT_INST_PROP(idx, cc_num)		       \
				  : DT_INST_PROP(idx, cc_num) - 1	       \
		},							       \
		.ch_data = counter##idx##_ch_data,			       \
		.rtc = (NRF_RTC_Type *)DT_INST_REG_ADDR(idx),		       \
		IF_ENABLED(CONFIG_COUNTER_RTC_WITH_PPI_WRAP,		       \
			   (.use_ppi = DT_INST_PROP(idx, ppi_wrap),))	       \
		IF_ENABLED(CONFIG_COUNTER_RTC_CUSTOM_TOP_SUPPORT,	       \
			   (.fixed_top = DT_INST_PROP(idx, fixed_top),))       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)	       \
	};								       \
	DEVICE_DT_INST_DEFINE(idx,					       \
			    counter_##idx##_init,			       \
			    NULL,					       \
			    &counter_##idx##_data,			       \
			    &nrfx_counter_##idx##_config.info,		       \
			    PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,	       \
			    &counter_nrfx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_NRF_RTC_DEVICE)
