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
LOG_MODULE_REGISTER();

#define RTC_CLOCK 32768
#define COUNTER_MAX_WRAP RTC_COUNTER_COUNTER_Msk

#define CC_TO_ID(cc) ((cc) - 1)
#define ID_TO_CC(id) ((id) + 1)

#define WRAP_CH 0
#define COUNTER_WRAP_INT NRFX_RTC_INT_COMPARE0

struct counter_nrfx_data {
	counter_wrap_callback_t wrap_cb;
	void *wrap_user_data;
	u32_t wrap;
};

struct counter_nrfx_config {
	struct counter_config_info info;
	const struct counter_alarm_cfg **alarm_cfgs;
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

static int counter_nrfx_set_alarm(struct device *dev,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;
	u32_t cc_val;

	if (alarm_cfg->ticks > get_dev_data(dev)->wrap) {
		return -EINVAL;
	}

	if (nrfx_config->alarm_cfgs[alarm_cfg->channel_id]) {
		return -EBUSY;
	}

	cc_val = alarm_cfg->ticks + (alarm_cfg->absolute ?
					0 : nrfx_rtc_counter_get(rtc));
	cc_val = (cc_val > get_dev_data(dev)->wrap) ?
			(cc_val - get_dev_data(dev)->wrap) : cc_val;
	nrfx_config->alarm_cfgs[alarm_cfg->channel_id] = alarm_cfg;

	nrfx_rtc_cc_set(rtc, ID_TO_CC(alarm_cfg->channel_id), cc_val, true);

	return 0;
}

static void _disable(struct device *dev, u8_t id)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);

	nrfx_rtc_cc_disable(&config->rtc, ID_TO_CC(id));
	config->alarm_cfgs[id] = NULL;
}

static int counter_nrfx_disable_alarm(struct device *dev,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	_disable(dev, alarm_cfg->channel_id);

	return 0;
}

static int counter_nrfx_set_wrap(struct device *dev, u32_t ticks,
				 counter_wrap_callback_t callback,
				 void *user_data)
{
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;
	struct counter_nrfx_data *dev_data = get_dev_data(dev);

	for (int i = 0; i < counter_get_num_of_channels(dev); i++) {
		/* Overflow can be changed only when all alarms are
		 * disables.
		 */
		if (nrfx_config->alarm_cfgs[i]) {
			return -EBUSY;
		}
	}

	nrfx_rtc_cc_disable(rtc, WRAP_CH);
	nrfx_rtc_counter_clear(rtc);

	dev_data->wrap_cb = callback;
	dev_data->wrap_user_data = user_data;
	dev_data->wrap = ticks;
	nrfx_rtc_cc_set(rtc, WRAP_CH, ticks, callback ? true : false);

	return 0;
}

static u32_t counter_nrfx_get_pending_int(struct device *dev)
{
	return 0;
}

static void alarm_event_handler(struct device *dev, u32_t id)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);
	const struct counter_alarm_cfg *alarm_cfg = config->alarm_cfgs[id];
	u32_t cc_val;

	cc_val = nrf_rtc_cc_get(config->rtc.p_reg, ID_TO_CC(id));

	_disable(dev, id);
	alarm_cfg->handler(dev, alarm_cfg, cc_val);
}

static void event_handler(nrfx_rtc_int_type_t int_type, void *p_context)
{
	struct device *dev = p_context;
	struct counter_nrfx_data *data = get_dev_data(dev);

	if (int_type == COUNTER_WRAP_INT) {
		/* Manually reset counter when wrap is different than max wrap*/
		if (data->wrap != COUNTER_MAX_WRAP) {
			nrfx_rtc_counter_clear(&get_nrfx_config(dev)->rtc);
			nrfx_rtc_cc_set(&get_nrfx_config(dev)->rtc,
					WRAP_CH, data->wrap, true);
		}

		if (data->wrap_cb) {
			data->wrap_cb(dev, data->wrap_user_data);
		}
	} else if (int_type > COUNTER_WRAP_INT) {
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

	clock = device_get_binding(CONFIG_CLOCK_CONTROL_NRF_K32SRC_DRV_NAME);
	if (!clock) {
		return -ENODEV;
	}

	clock_control_on(clock, (void *)CLOCK_CONTROL_NRF_K32SRC);

	nrfx_err_t result = nrfx_rtc_init(rtc, config, handler);

	if (result != NRFX_SUCCESS) {
		LOG_INST_ERR(nrfx_config->log, "Failed to initialize device.");
		return -EBUSY;
	}

	get_dev_data(dev)->wrap = COUNTER_MAX_WRAP;

	LOG_INST_DBG(nrfx_config->log, "Initialized");
	return 0;
}

static u32_t counter_nrfx_get_wrap(struct device *dev)
{
	return get_dev_data(dev)->wrap;
}

static u32_t counter_nrfx_get_max_relative_alarm(struct device *dev)
{
	/* Maybe decreased. */
	return get_dev_data(dev)->wrap;
}

static const struct counter_driver_api counter_nrfx_driver_api = {
	.start = counter_nrfx_start,
	.stop = counter_nrfx_stop,
	.read = counter_nrfx_read,
	.set_alarm = counter_nrfx_set_alarm,
	.disable_alarm = counter_nrfx_disable_alarm,
	.set_wrap = counter_nrfx_set_wrap,
	.get_pending_int = counter_nrfx_get_pending_int,
	.get_wrap = counter_nrfx_get_wrap,
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
		IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_RTC##idx),		       \
			    CONFIG_COUNTER_RTC##idx##_IRQ_PRI,		       \
			    nrfx_isr, nrfx_rtc_##idx##_irq_handler, 0);	       \
		const nrfx_rtc_config_t config = {			       \
			.prescaler = CONFIG_COUNTER_RTC##idx##_PRESCALER,      \
		};							       \
		return init_rtc(dev, &config, rtc_##idx##_handler);	       \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static const struct counter_alarm_cfg				       \
		*counter##idx##_alarm_cfgs[CC_TO_ID(RTC##idx##_CC_NUM)];       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##_config = {\
		.info = {						       \
			.max_wrap = COUNTER_MAX_WRAP,			       \
			.freq = RTC_CLOCK /				       \
				(CONFIG_COUNTER_RTC##idx##_PRESCALER + 1),     \
			.count_up = true,				       \
			.channels = CC_TO_ID(RTC##idx##_CC_NUM)		       \
		},							       \
		.alarm_cfgs = counter##idx##_alarm_cfgs,		       \
		.rtc = NRFX_RTC_INSTANCE(idx),				       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)	       \
	};								       \
	DEVICE_AND_API_INIT(rtc_##idx, CONFIG_COUNTER_RTC##idx##_NAME,	       \
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
