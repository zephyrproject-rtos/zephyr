/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/counter.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_rtc.h>
#include <sys/atomic.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#define LOG_MODULE_NAME counter_rtc
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

#define ERR(...) LOG_INST_ERR(get_nrfx_config(dev)->log, __VA_ARGS__)
#define WRN(...) LOG_INST_WRN(get_nrfx_config(dev)->log, __VA_ARGS__)
#define INF(...) LOG_INST_INF(get_nrfx_config(dev)->log, __VA_ARGS__)
#define DBG(...) LOG_INST_DBG(get_nrfx_config(dev)->log, __VA_ARGS__)

#define COUNTER_MAX_TOP_VALUE RTC_COUNTER_COUNTER_Msk

#define COUNTER_GET_TOP_CH(dev) counter_get_num_of_channels(dev)

#define IS_FIXED_TOP(dev) COND_CODE_1(CONFIG_COUNTER_RTC_CUSTOM_TOP_SUPPORT, \
		(get_nrfx_config(dev)->fixed_top), (true))

#define IS_PPI_WRAP(dev) COND_CODE_1(CONFIG_COUNTER_RTC_WITH_PPI_WRAP, \
		(get_nrfx_config(dev)->use_ppi), (false))

#define CC_ADJUSTED_OFFSET 16
#define CC_ADJ_MASK(chan) (BIT(chan + CC_ADJUSTED_OFFSET))

struct counter_nrfx_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	u32_t top;
	u32_t guard_period;
	/* Store channel interrupt pending and CC adjusted flags. */
	atomic_t ipend_adj;
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	u8_t ppi_ch;
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

static inline struct counter_nrfx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct counter_nrfx_config *get_nrfx_config(
							struct device *dev)
{
	return CONTAINER_OF(dev->config->config_info,
				struct counter_nrfx_config, info);
}

static int start(struct device *dev)
{
	nrf_rtc_task_trigger(get_nrfx_config(dev)->rtc, NRF_RTC_TASK_START);

	return 0;
}

static int stop(struct device *dev)
{
	nrf_rtc_task_trigger(get_nrfx_config(dev)->rtc, NRF_RTC_TASK_STOP);

	return 0;
}

static u32_t read(struct device *dev)
{
	return nrf_rtc_counter_get(get_nrfx_config(dev)->rtc);
}

/* Return true if value equals 2^n - 1 */
static inline bool is_bit_mask(u32_t val)
{
	return !(val & (val + 1));
}

/* Function calculates distance between to values assuming that one first
 * argument is in front and that values wrap.
 */
static u32_t ticks_sub(struct device *dev, u32_t val, u32_t old, u32_t top)
{
	if (IS_FIXED_TOP(dev)) {
		return (val - old) & COUNTER_MAX_TOP_VALUE;
	} else if (likely(is_bit_mask(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1 - old;

}

static u32_t skip_zero_on_custom_top(u32_t val, u32_t top)
{
	/* From Product Specification: If a CC register value is 0 when
	 * a CLEAR task is set, this will not trigger a COMPARE event.
	 */
	if (unlikely(val == 0) && (top != COUNTER_MAX_TOP_VALUE)) {
		val++;
	}

	return val;
}

static u32_t ticks_add(struct device *dev, u32_t val1, u32_t val2, u32_t top)
{
	u32_t sum = val1 + val2;

	if (IS_FIXED_TOP(dev)) {
		ARG_UNUSED(top);
		return sum & COUNTER_MAX_TOP_VALUE;
	}
	if (likely(is_bit_mask(top))) {
		sum = sum & top;
	} else {
		sum = sum > top ? sum - (top + 1) : sum;
	}

	return skip_zero_on_custom_top(sum, top);
}

static void set_cc_int_pending(struct device *dev, u8_t chan)
{
	atomic_or(&get_dev_data(dev)->ipend_adj, BIT(chan));
	NRFX_IRQ_PENDING_SET(NRFX_IRQ_NUMBER_GET(get_nrfx_config(dev)->rtc));
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
static void handle_next_tick_case(struct device *dev, u8_t chan,
				  u32_t now, u32_t val)
{
	val = ticks_add(dev, val, 1, get_dev_data(dev)->top);
	nrf_rtc_cc_set(get_nrfx_config(dev)->rtc, chan, val);
	atomic_or(&get_dev_data(dev)->ipend_adj, CC_ADJ_MASK(chan));
	if (nrf_rtc_counter_get(get_nrfx_config(dev)->rtc) != now) {
		set_cc_int_pending(dev, chan);
	} else {
		nrf_rtc_int_enable(get_nrfx_config(dev)->rtc,
				   RTC_CHANNEL_INT_MASK(chan));
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
static int set_cc(struct device *dev, u8_t chan, u32_t val, u32_t flags)
{
	__ASSERT_NO_MSG(get_dev_data(dev)->guard_period <
			get_dev_data(dev)->top);
	NRF_RTC_Type  *rtc = get_nrfx_config(dev)->rtc;
	nrf_rtc_event_t evt;
	u32_t prev_val;
	u32_t top;
	u32_t now;
	u32_t diff;
	u32_t int_mask = RTC_CHANNEL_INT_MASK(chan);
	int err = 0;
	u32_t max_rel_val;
	bool absolute = flags & COUNTER_ALARM_CFG_ABSOLUTE;
	bool irq_on_late;

	__ASSERT(nrf_rtc_int_is_enabled(rtc, int_mask) == 0,
			"Expected that CC interrupt is disabled.");

	evt = RTC_CHANNEL_EVENT_ADDR(chan);
	top =  get_dev_data(dev)->top;
	now = nrf_rtc_counter_get(rtc);

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future (now tick is the furtherest
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
		max_rel_val = top - get_dev_data(dev)->guard_period;
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
				get_nrfx_config(dev)->ch_data[chan].callback =
									NULL;
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

static int set_channel_alarm(struct device *dev, u8_t chan,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	struct counter_nrfx_ch_data *chdata = &nrfx_config->ch_data[chan];

	if (alarm_cfg->ticks > get_dev_data(dev)->top) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;
	atomic_and(&get_dev_data(dev)->ipend_adj, ~CC_ADJ_MASK(chan));

	return set_cc(dev, chan, alarm_cfg->ticks, alarm_cfg->flags);
}

static void disable(struct device *dev, u8_t chan)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);
	NRF_RTC_Type *rtc = config->rtc;
	nrf_rtc_event_t evt = RTC_CHANNEL_EVENT_ADDR(chan);

	nrf_rtc_int_disable(rtc, RTC_CHANNEL_INT_MASK(chan));
	nrf_rtc_event_disable(rtc, RTC_CHANNEL_INT_MASK(chan));
	nrf_rtc_event_clear(rtc, evt);
	config->ch_data[chan].callback = NULL;
}

static int cancel_alarm(struct device *dev, u8_t chan_id)
{
	disable(dev, chan_id);

	return 0;
}

static int ppi_setup(struct device *dev, u8_t chan)
{
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	struct counter_nrfx_data *data = get_dev_data(dev);
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	nrf_rtc_event_t evt = RTC_CHANNEL_EVENT_ADDR(chan);
	nrfx_err_t result;

	if (!nrfx_config->use_ppi) {
		return 0;
	}

	nrf_rtc_event_enable(rtc, RTC_CHANNEL_INT_MASK(chan));
#ifdef DPPI_PRESENT
	result = nrfx_dppi_channel_alloc(&data->ppi_ch);
	if (result != NRFX_SUCCESS) {
		ERR("Failed to allocate PPI channel.");
		return -ENODEV;
	}

	nrf_rtc_subscribe_set(rtc, NRF_RTC_TASK_CLEAR, data->ppi_ch);
	nrf_rtc_publish_set(rtc->p_reg, evt, data->ppi_ch);
	(void)nrfx_dppi_channel_enable(data->ppi_ch);
#else /* DPPI_PRESENT */
	u32_t evt_addr;
	u32_t task_addr;

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

static void ppi_free(struct device *dev, u8_t chan)
{
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	u8_t ppi_ch = get_dev_data(dev)->ppi_ch;
	NRF_RTC_Type *rtc = nrfx_config->rtc;

	if (!nrfx_config->use_ppi) {
		return;
	}
	nrf_rtc_event_disable(rtc, RTC_CHANNEL_INT_MASK(chan));
#ifdef DPPI_PRESENT
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	nrf_rtc_event_t evt = RTC_CHANNEL_EVENT_ADDR(chan);

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
static bool sw_wrap_required(struct device *dev)
{
	return (get_dev_data(dev)->top != COUNTER_MAX_TOP_VALUE)
			&& !IS_PPI_WRAP(dev);
}

static int set_fixed_top_value(struct device *dev,
				const struct counter_top_cfg *cfg)
{
	NRF_RTC_Type *rtc = get_nrfx_config(dev)->rtc;

	if (cfg->ticks != COUNTER_MAX_TOP_VALUE) {
		return -EINVAL;
	}

	nrf_rtc_int_disable(rtc, NRF_RTC_INT_OVERFLOW_MASK);
	get_dev_data(dev)->top_cb = cfg->callback;
	get_dev_data(dev)->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
	}

	if (cfg->callback) {
		nrf_rtc_int_enable(rtc, NRF_RTC_INT_OVERFLOW_MASK);
	}

	return 0;
}

static int set_top_value(struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	struct counter_nrfx_data *dev_data = get_dev_data(dev);
	u32_t top_ch = COUNTER_GET_TOP_CH(dev);
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

	nrf_rtc_int_disable(rtc, RTC_CHANNEL_INT_MASK(top_ch));

	if (IS_PPI_WRAP(dev)) {
		if ((dev_data->top == COUNTER_MAX_TOP_VALUE) &&
				cfg->ticks != COUNTER_MAX_TOP_VALUE) {
			err = ppi_setup(dev, top_ch);
		} else if (((dev_data->top != COUNTER_MAX_TOP_VALUE) &&
				cfg->ticks == COUNTER_MAX_TOP_VALUE)) {
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
		nrf_rtc_int_enable(rtc, RTC_CHANNEL_INT_MASK(top_ch));
	}

	return err;
}

static u32_t get_pending_int(struct device *dev)
{
	return 0;
}

static int init_rtc(struct device *dev, u8_t prescaler)
{
	struct device *clock;
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	struct counter_top_cfg top_cfg = {
		.ticks = COUNTER_MAX_TOP_VALUE
	};
	NRF_RTC_Type *rtc = nrfx_config->rtc;
	int err;

	clock = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_32K");
	if (!clock) {
		return -ENODEV;
	}

	clock_control_on(clock, NULL);

	nrf_rtc_prescaler_set(rtc, prescaler);

	NRFX_IRQ_ENABLE(NRFX_IRQ_NUMBER_GET(rtc));

	get_dev_data(dev)->top = COUNTER_MAX_TOP_VALUE;
	err = set_top_value(dev, &top_cfg);
	DBG("Initialized");

	return err;
}

static u32_t get_top_value(struct device *dev)
{
	return get_dev_data(dev)->top;
}

static u32_t get_max_relative_alarm(struct device *dev)
{
	return get_dev_data(dev)->top;
}

static u32_t get_guard_period(struct device *dev, u32_t flags)
{
	return get_dev_data(dev)->guard_period;
}

static int set_guard_period(struct device *dev, u32_t guard, u32_t flags)
{
	get_dev_data(dev)->guard_period = guard;
	return 0;
}

static void top_irq_handle(struct device *dev)
{
	NRF_RTC_Type *rtc = get_nrfx_config(dev)->rtc;
	counter_top_callback_t cb = get_dev_data(dev)->top_cb;
	nrf_rtc_event_t top_evt;

	top_evt = IS_FIXED_TOP(dev) ?
		  NRF_RTC_EVENT_OVERFLOW :
		  RTC_CHANNEL_EVENT_ADDR(counter_get_num_of_channels(dev));

	if (nrf_rtc_event_pending(rtc, top_evt)) {
		nrf_rtc_event_clear(rtc, top_evt);

		/* Perform manual clear if custom top value is used and PPI
		 * clearing is not used.
		 */
		if (!IS_FIXED_TOP(dev) && !IS_PPI_WRAP(dev)) {
			nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
		}

		if (cb) {
			cb(dev, get_dev_data(dev)->top_user_data);
		}
	}
}

static void alarm_irq_handle(struct device *dev, u32_t chan)
{
	NRF_RTC_Type *rtc = get_nrfx_config(dev)->rtc;
	nrf_rtc_event_t evt = RTC_CHANNEL_EVENT_ADDR(chan);
	u32_t int_mask = RTC_CHANNEL_INT_MASK(chan);
	bool hw_irq_pending = nrf_rtc_event_pending(rtc, evt) &&
			      nrf_rtc_int_is_enabled(rtc, int_mask);
	bool sw_irq_pending = get_dev_data(dev)->ipend_adj & BIT(chan);

	if (hw_irq_pending || sw_irq_pending) {
		struct counter_nrfx_ch_data *chdata;
		counter_alarm_callback_t cb;

		nrf_rtc_event_clear(rtc, evt);
		atomic_and(&get_dev_data(dev)->ipend_adj, ~BIT(chan));
		nrf_rtc_int_disable(rtc, int_mask);

		chdata = &get_nrfx_config(dev)->ch_data[chan];
		cb = chdata->callback;
		chdata->callback = NULL;

		if (cb) {
			u32_t cc = nrf_rtc_cc_get(rtc, chan);

			if (get_dev_data(dev)->ipend_adj & CC_ADJ_MASK(chan)) {
				cc = ticks_sub(dev, cc, 1,
						get_dev_data(dev)->top);
			}

			cb(dev, chan, cc, chdata->user_data);
		}
	}
}

static void irq_handler(struct device *dev)
{
	top_irq_handle(dev);

	for (u32_t i = 0; i < counter_get_num_of_channels(dev); i++) {
		alarm_irq_handle(dev, i);
	}
}

static const struct counter_driver_api counter_nrfx_driver_api = {
	.start = start,
	.stop = stop,
	.read = read,
	.set_alarm = set_channel_alarm,
	.cancel_alarm = cancel_alarm,
	.set_top_value = set_top_value,
	.get_pending_int = get_pending_int,
	.get_top_value = get_top_value,
	.get_max_relative_alarm = get_max_relative_alarm,
	.get_guard_period = get_guard_period,
	.set_guard_period = set_guard_period,
};

#define COUNTER_NRF_RTC_DEVICE(idx)					       \
	BUILD_ASSERT_MSG((DT_NORDIC_NRF_RTC_RTC_##idx##_PRESCALER - 1) <=      \
			RTC_PRESCALER_PRESCALER_Msk,			       \
			"RTC prescaler out of range");			       \
	DEVICE_DECLARE(rtc_##idx);					       \
	static int counter_##idx##_init(struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_NORDIC_NRF_RTC_RTC_##idx##_IRQ_0,	       \
			    DT_NORDIC_NRF_RTC_RTC_##idx##_IRQ_0_PRIORITY,      \
			    irq_handler, DEVICE_GET(rtc_##idx), 0);	       \
		return init_rtc(dev,					       \
				DT_NORDIC_NRF_RTC_RTC_##idx##_PRESCALER - 1);  \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static struct counter_nrfx_ch_data				       \
		counter##idx##_ch_data[RTC##idx##_CC_NUM];		       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##_config = {\
		.info = {						       \
			.max_top_value = COUNTER_MAX_TOP_VALUE,		       \
			.freq = DT_NORDIC_NRF_RTC_RTC_##idx##_CLOCK_FREQUENCY /\
				(DT_NORDIC_NRF_RTC_RTC_##idx##_PRESCALER),     \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		       \
			.channels = DT_NORDIC_NRF_RTC_RTC_##idx##_FIXED_TOP ?  \
				RTC##idx##_CC_NUM : RTC##idx##_CC_NUM - 1      \
		},							       \
		.ch_data = counter##idx##_ch_data,			       \
		.rtc = NRF_RTC##idx,					       \
		COND_CODE_1(DT_NORDIC_NRF_RTC_RTC_##idx##_PPI_WRAP,	       \
			    (.use_ppi = true,), ())			       \
		COND_CODE_1(CONFIG_COUNTER_RTC_CUSTOM_TOP_SUPPORT,	       \
		  (.fixed_top = DT_NORDIC_NRF_RTC_RTC_##idx##_FIXED_TOP,), ()) \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)	       \
	};								       \
	DEVICE_AND_API_INIT(rtc_##idx,					       \
			    DT_NORDIC_NRF_RTC_RTC_##idx##_LABEL,	       \
			    counter_##idx##_init,			       \
			    &counter_##idx##_data,			       \
			    &nrfx_counter_##idx##_config.info,		       \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &counter_nrfx_driver_api)

#ifdef CONFIG_COUNTER_RTC0
COUNTER_NRF_RTC_DEVICE(0);
#endif

#ifdef CONFIG_COUNTER_RTC1
COUNTER_NRF_RTC_DEVICE(1);
#endif

#ifdef CONFIG_COUNTER_RTC2
COUNTER_NRF_RTC_DEVICE(2);
#endif
