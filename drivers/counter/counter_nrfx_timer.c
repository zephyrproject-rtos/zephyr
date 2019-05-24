/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <counter.h>
#include <nrfx_timer.h>

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#define LOG_MODULE_NAME counter_timer
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define TIMER_CLOCK 16000000

#define CC_TO_ID(cc_num) (cc_num - 2)

#define ID_TO_CC(idx) (nrf_timer_cc_channel_t)(idx + 2)

#define COUNTER_EVENT_TO_ID(evt) \
	(evt - NRF_TIMER_EVENT_COMPARE2)/sizeof(u32_t)

#define TOP_CH NRF_TIMER_CC_CHANNEL0
#define COUNTER_TOP_INT NRF_TIMER_EVENT_COMPARE0
#define COUNTER_OVERFLOW_SHORT NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK
#define COUNTER_READ_CC NRF_TIMER_CC_CHANNEL1

struct counter_nrfx_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
};

struct counter_nrfx_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_nrfx_config {
	struct counter_config_info info;
	struct counter_nrfx_ch_data *ch_data;
	nrfx_timer_t timer;

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

static int counter_nrfx_start(struct device *dev)
{
	nrfx_timer_enable(&get_nrfx_config(dev)->timer);

	return 0;
}

static int counter_nrfx_stop(struct device *dev)
{
	nrfx_timer_disable(&get_nrfx_config(dev)->timer);

	return 0;
}

static u32_t counter_nrfx_get_top_value(struct device *dev)
{
	return nrfx_timer_capture_get(&get_nrfx_config(dev)->timer, TOP_CH);
}

static u32_t counter_nrfx_get_max_relative_alarm(struct device *dev)
{
	return nrfx_timer_capture_get(&get_nrfx_config(dev)->timer, TOP_CH);
}

static u32_t counter_nrfx_read(struct device *dev)
{
	return nrfx_timer_capture(&get_nrfx_config(dev)->timer,
				  COUNTER_READ_CC);
}

/** @brief Calculate compare value.
 *
 * If ticks are relative then compare value must take into consideration
 * counter wrapping.
 *
 * @return Compare value to be used in TIMER channel.
 */
static inline u32_t counter_nrfx_get_cc_value(struct device *dev,
				const struct counter_alarm_cfg *alarm_cfg)
{
	u32_t remainder;
	u32_t cc_val;
	u32_t ticks = alarm_cfg->ticks;

	if (alarm_cfg->absolute) {
		return ticks;
	}

	cc_val = counter_nrfx_read(dev);
	remainder = counter_nrfx_get_top_value(dev) - cc_val;

	if (remainder > ticks) {
		cc_val += ticks;
	} else {
		cc_val = ticks - remainder;
	}

	return cc_val;
}

static int counter_nrfx_set_alarm(struct device *dev, u8_t chan_id,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_timer_t *timer = &nrfx_config->timer;
	u32_t cc_val;

	if (alarm_cfg->ticks > nrfx_timer_capture_get(timer, TOP_CH)) {
		return -EINVAL;
	}

	if (nrfx_config->ch_data[chan_id].callback) {
		return -EBUSY;
	}

	cc_val = counter_nrfx_get_cc_value(dev, alarm_cfg);

	nrfx_config->ch_data[chan_id].callback = alarm_cfg->callback;
	nrfx_config->ch_data[chan_id].user_data = alarm_cfg->user_data;

	nrfx_timer_compare(timer, ID_TO_CC(chan_id), cc_val, true);

	return 0;
}

static void disable(struct device *dev, u8_t id)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);

	nrfx_timer_compare_int_disable(&config->timer, ID_TO_CC(id));
	config->ch_data[id].callback = NULL;
}

static int counter_nrfx_cancel_alarm(struct device *dev, u8_t chan_id)
{
	disable(dev, chan_id);

	return 0;
}


static int counter_nrfx_set_top_value(struct device *dev, u32_t ticks,
				      counter_top_callback_t callback,
				      void *user_data)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_timer_t *timer = &nrfx_config->timer;
	struct counter_nrfx_data *data = get_dev_data(dev);

	for (int i = 0; i < counter_get_num_of_channels(dev); i++) {
		/* Overflow can be changed only when all alarms are
		 * disables.
		 */
		if (nrfx_config->ch_data[i].callback) {
			return -EBUSY;
		}
	}

	nrfx_timer_compare_int_disable(timer, TOP_CH);
	nrfx_timer_clear(timer);

	data->top_cb = callback;
	data->top_user_data = user_data;
	nrfx_timer_extended_compare(timer, TOP_CH,
				    ticks, COUNTER_OVERFLOW_SHORT,
				    callback ? true : false);

	return 0;
}

static u32_t counter_nrfx_get_pending_int(struct device *dev)
{
	return 0;
}

static void alarm_event_handler(struct device *dev, u32_t id)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);
	counter_alarm_callback_t clbk = config->ch_data[id].callback;
	u32_t cc_val;

	if (!clbk) {
		return;
	}

	cc_val = nrfx_timer_capture_get(&config->timer, ID_TO_CC(id));
	disable(dev, id);
	clbk(dev, id, cc_val, config->ch_data[id].user_data);
}

static void event_handler(nrf_timer_event_t event_type, void *p_context)
{
	struct device *dev = p_context;
	struct counter_nrfx_data *dev_data = get_dev_data(dev);

	if (event_type == COUNTER_TOP_INT) {
		if (dev_data->top_cb) {
			dev_data->top_cb(dev, dev_data->top_user_data);
		}
	} else if (event_type > NRF_TIMER_EVENT_COMPARE1) {
		alarm_event_handler(dev, COUNTER_EVENT_TO_ID(event_type));

	}
}

static int init_timer(struct device *dev, const nrfx_timer_config_t *config)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_timer_t *timer = &nrfx_config->timer;

	nrfx_err_t result = nrfx_timer_init(timer, config, event_handler);

	if (result != NRFX_SUCCESS) {
		LOG_INST_ERR(nrfx_config->log, "Failed to initialize device.");
		return -EBUSY;
	}

	nrfx_timer_compare(timer, TOP_CH, UINT32_MAX, false);

	LOG_INST_DBG(nrfx_config->log, "Initialized");

	return 0;
}

static const struct counter_driver_api counter_nrfx_driver_api = {
	.start = counter_nrfx_start,
	.stop = counter_nrfx_stop,
	.read = counter_nrfx_read,
	.set_alarm = counter_nrfx_set_alarm,
	.cancel_alarm = counter_nrfx_cancel_alarm,
	.set_top_value = counter_nrfx_set_top_value,
	.get_pending_int = counter_nrfx_get_pending_int,
	.get_top_value = counter_nrfx_get_top_value,
	.get_max_relative_alarm = counter_nrfx_get_max_relative_alarm,
};

#define COUNTER_NRFX_TIMER_DEVICE(idx)					       \
	BUILD_ASSERT_MSG(DT_NORDIC_NRF_TIMER_TIMER_##idx##_PRESCALER <=	       \
			TIMER_PRESCALER_PRESCALER_Msk,			       \
			"TIMER prescaler out of range");		       \
	static int counter_##idx##_init(struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_NORDIC_NRF_TIMER_TIMER_##idx##_IRQ,	       \
			    DT_NORDIC_NRF_TIMER_TIMER_##idx##_IRQ_PRIORITY,    \
			    nrfx_isr, nrfx_timer_##idx##_irq_handler, 0);      \
		const nrfx_timer_config_t config = {			       \
			.frequency =					       \
				DT_NORDIC_NRF_TIMER_TIMER_##idx##_PRESCALER,   \
			.mode      = NRF_TIMER_MODE_TIMER,		       \
			.bit_width = (TIMER##idx##_MAX_SIZE == 32) ?	       \
					NRF_TIMER_BIT_WIDTH_32 :	       \
					NRF_TIMER_BIT_WIDTH_16,		       \
			.p_context = dev				       \
		};							       \
		return init_timer(dev, &config);			       \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static struct counter_nrfx_ch_data				       \
		counter##idx##_ch_data[CC_TO_ID(TIMER##idx##_CC_NUM)];	       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##z_config = {\
		.info = {						       \
			.max_top_value = (TIMER##idx##_MAX_SIZE == 32) ?       \
					0xffffffff : 0x0000ffff,	       \
			.freq = TIMER_CLOCK /				       \
			   (1 << DT_NORDIC_NRF_TIMER_TIMER_##idx##_PRESCALER), \
			.count_up = true,				       \
			.channels = CC_TO_ID(TIMER##idx##_CC_NUM),	       \
		},							       \
		.ch_data = counter##idx##_ch_data,			       \
		.timer = NRFX_TIMER_INSTANCE(idx),			       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)	       \
	};								       \
	DEVICE_AND_API_INIT(timer_##idx,				       \
			    DT_NORDIC_NRF_TIMER_TIMER_##idx##_LABEL,	       \
			    counter_##idx##_init,			       \
			    &counter_##idx##_data,			       \
			    &nrfx_counter_##idx##z_config.info,		       \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
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
