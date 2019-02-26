/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <counter.h>
#include <clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <nrfx_rtc.h>

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#define LOG_MODULE_NAME counter_rtc
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define RTC_CLOCK 32768
#define COUNTER_MAX_TOP_VALUE RTC_COUNTER_COUNTER_Msk

#define CC_TO_ID(cc) ((cc) - 1)
#define ID_TO_CC(id) ((id) + 1)

#define TOP_CH 0
#define COUNTER_TOP_INT NRFX_RTC_INT_COMPARE0

struct counter_nrfx_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	u32_t top;
};

struct counter_nrfx_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_nrfx_config {
	struct counter_config_info info;
	struct counter_nrfx_ch_data *ch_data;
	nrfx_rtc_t rtc;

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
	nrfx_rtc_enable(&get_nrfx_config(dev)->rtc);

	return 0;
}

static int counter_nrfx_stop(struct device *dev)
{
	nrfx_rtc_disable(&get_nrfx_config(dev)->rtc);

	return 0;
}

static u32_t counter_nrfx_read(struct device *dev)
{
	return nrfx_rtc_counter_get(&get_nrfx_config(dev)->rtc);
}

static int counter_nrfx_set_alarm(struct device *dev, u8_t chan_id,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;
	u32_t cc_val;

	if (alarm_cfg->ticks > get_dev_data(dev)->top) {
		return -EINVAL;
	}

	if (nrfx_config->ch_data[chan_id].callback) {
		return -EBUSY;
	}

	cc_val = alarm_cfg->ticks + (alarm_cfg->absolute ?
					0 : nrfx_rtc_counter_get(rtc));
	cc_val = (cc_val > get_dev_data(dev)->top) ?
			(cc_val - get_dev_data(dev)->top) : cc_val;
	nrfx_config->ch_data[chan_id].callback = alarm_cfg->callback;
	nrfx_config->ch_data[chan_id].user_data = alarm_cfg->user_data;

	nrfx_rtc_cc_set(rtc, ID_TO_CC(chan_id), cc_val, true);

	return 0;
}

static void _disable(struct device *dev, u8_t id)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);

	nrfx_rtc_cc_disable(&config->rtc, ID_TO_CC(id));
	config->ch_data[id].callback = NULL;
}

static int counter_nrfx_cancel_alarm(struct device *dev, u8_t chan_id)
{
	_disable(dev, chan_id);

	return 0;
}

static int counter_nrfx_set_top_value(struct device *dev, u32_t ticks,
				      counter_top_callback_t callback,
				      void *user_data)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;
	struct counter_nrfx_data *dev_data = get_dev_data(dev);

	for (int i = 0; i < counter_get_num_of_channels(dev); i++) {
		/* Overflow can be changed only when all alarms are
		 * disables.
		 */
		if (nrfx_config->ch_data[i].callback) {
			return -EBUSY;
		}
	}

	nrfx_rtc_cc_disable(rtc, TOP_CH);
	nrfx_rtc_counter_clear(rtc);

	dev_data->top_cb = callback;
	dev_data->top_user_data = user_data;
	dev_data->top = ticks;
	nrfx_rtc_cc_set(rtc, TOP_CH, ticks, callback ? true : false);

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

	cc_val = nrf_rtc_cc_get(config->rtc.p_reg, ID_TO_CC(id));

	_disable(dev, id);
	clbk(dev, id, cc_val, config->ch_data[id].user_data);
}

static void event_handler(nrfx_rtc_int_type_t int_type, void *p_context)
{
	struct device *dev = p_context;
	struct counter_nrfx_data *data = get_dev_data(dev);

	if (int_type == COUNTER_TOP_INT) {
		/* Manually reset counter if top value is different than max. */
		if (data->top != COUNTER_MAX_TOP_VALUE) {
			nrfx_rtc_counter_clear(&get_nrfx_config(dev)->rtc);
		}

		nrfx_rtc_cc_set(&get_nrfx_config(dev)->rtc,
				TOP_CH, data->top, true);

		if (data->top_cb) {
			data->top_cb(dev, data->top_user_data);
		}
	} else if (int_type > COUNTER_TOP_INT) {
		alarm_event_handler(dev, CC_TO_ID(int_type));

	}
}

static int init_rtc(struct device *dev,
		    const nrfx_rtc_config_t *config,
		    nrfx_rtc_handler_t handler)
{
	struct device *clock;
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;

	clock = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_32K");
	if (!clock) {
		return -ENODEV;
	}

	clock_control_on(clock, (void *)CLOCK_CONTROL_NRF_K32SRC);

	nrfx_err_t result = nrfx_rtc_init(rtc, config, handler);

	if (result != NRFX_SUCCESS) {
		LOG_INST_ERR(nrfx_config->log, "Failed to initialize device.");
		return -EBUSY;
	}

	get_dev_data(dev)->top = COUNTER_MAX_TOP_VALUE;

	LOG_INST_DBG(nrfx_config->log, "Initialized");
	return 0;
}

static u32_t counter_nrfx_get_top_value(struct device *dev)
{
	return get_dev_data(dev)->top;
}

static u32_t counter_nrfx_get_max_relative_alarm(struct device *dev)
{
	/* Maybe decreased. */
	return get_dev_data(dev)->top;
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

#define COUNTER_NRFX_RTC_DEVICE(idx)					       \
	DEVICE_DECLARE(rtc_##idx);					       \
	static void rtc_##idx##_handler(nrfx_rtc_int_type_t int_type)	       \
	{								       \
		event_handler(int_type, DEVICE_GET(rtc_##idx));		       \
	}								       \
	static int counter_##idx##_init(struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_NORDIC_NRF_RTC_RTC_##idx##_IRQ,		       \
			    DT_NORDIC_NRF_RTC_RTC_##idx##_IRQ_PRIORITY,	       \
			    nrfx_isr, nrfx_rtc_##idx##_irq_handler, 0);	       \
		const nrfx_rtc_config_t config = {			       \
			.prescaler = CONFIG_COUNTER_RTC##idx##_PRESCALER,      \
		};							       \
		return init_rtc(dev, &config, rtc_##idx##_handler);	       \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static struct counter_nrfx_ch_data				       \
		counter##idx##_ch_data[CC_TO_ID(RTC##idx##_CC_NUM)];	       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##_config = {\
		.info = {						       \
			.max_top_value = COUNTER_MAX_TOP_VALUE,		       \
			.freq = RTC_CLOCK /				       \
				(CONFIG_COUNTER_RTC##idx##_PRESCALER + 1),     \
			.count_up = true,				       \
			.channels = CC_TO_ID(RTC##idx##_CC_NUM)		       \
		},							       \
		.ch_data = counter##idx##_ch_data,			       \
		.rtc = NRFX_RTC_INSTANCE(idx),				       \
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
COUNTER_NRFX_RTC_DEVICE(0);
#endif

#ifdef CONFIG_COUNTER_RTC1
COUNTER_NRFX_RTC_DEVICE(1);
#endif

#ifdef CONFIG_COUNTER_RTC2
COUNTER_NRFX_RTC_DEVICE(2);
#endif
