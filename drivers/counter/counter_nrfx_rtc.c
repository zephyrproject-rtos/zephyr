/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <counter.h>
#include <clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <nrfx_rtc.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#define LOG_MODULE_NAME counter_rtc
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define COUNTER_MAX_TOP_VALUE RTC_COUNTER_COUNTER_Msk

#define CC_TO_ID(cc) ((cc) - 1)
#define ID_TO_CC(id) ((id) + 1)

#define TOP_CH 0
#define COUNTER_TOP_INT NRFX_RTC_INT_COMPARE0

struct counter_nrfx_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	u32_t top;
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
	nrfx_rtc_t rtc;
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	bool use_ppi;
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

	if (alarm_cfg->absolute) {
		cc_val = alarm_cfg->ticks;
	} else {
		/* As RTC is 24 bit there is no risk of overflow. */
		cc_val = alarm_cfg->ticks + nrfx_rtc_counter_get(rtc);
		cc_val -= (cc_val > get_dev_data(dev)->top) ?
				get_dev_data(dev)->top : 0;
	}

	nrfx_config->ch_data[chan_id].callback = alarm_cfg->callback;
	nrfx_config->ch_data[chan_id].user_data = alarm_cfg->user_data;

	if ((cc_val == 0) &&
	    (get_dev_data(dev)->top != counter_get_max_top_value(dev))) {
		/* From Product Specification: If a CC register value is 0 when
		 * a CLEAR task is set, this will not trigger a COMPARE event.
		 */
		LOG_INST_INF(nrfx_config->log,
				"Attempt to set CC to 0, delayed to 1.");
		cc_val++;
	}
	nrfx_rtc_cc_set(rtc, ID_TO_CC(chan_id), cc_val, true);

	return 0;
}

static void disable(struct device *dev, u8_t id)
{
	const struct counter_nrfx_config *config = get_nrfx_config(dev);

	nrfx_rtc_cc_disable(&config->rtc, ID_TO_CC(id));
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

	disable(dev, id);
	clbk(dev, id, cc_val, config->ch_data[id].user_data);
}

static void event_handler(nrfx_rtc_int_type_t int_type, void *p_context)
{
	struct device *dev = p_context;
	struct counter_nrfx_data *data = get_dev_data(dev);

	if (int_type == COUNTER_TOP_INT) {
		/* Manually reset counter if top value is different than max. */
		if ((data->top != COUNTER_MAX_TOP_VALUE)
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
		    && !get_nrfx_config(dev)->use_ppi
#endif
		    ) {
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

static int ppi_setup(struct device *dev)
{
#if CONFIG_COUNTER_RTC_WITH_PPI_WRAP
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	struct counter_nrfx_data *data = get_dev_data(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;
	nrfx_err_t result;

	if (!nrfx_config->use_ppi) {
		return 0;
	}

#ifdef DPPI_PRESENT
	result = nrfx_dppi_channel_alloc(&data->ppi_ch);
	if (result != NRFX_SUCCESS) {
		LOG_INST_ERR(nrfx_config->log,
			     "Failed to allocate PPI channel.");
		return -ENODEV;
	}

	nrf_rtc_subscribe_set(rtc->p_reg, NRF_RTC_TASK_CLEAR, data->ppi_ch);
	nrf_rtc_publish_set(rtc->p_reg, NRF_RTC_EVENT_COMPARE_0, data->ppi_ch);
	(void)nrfx_dppi_channel_enable(data->ppi_ch);
#else /* DPPI_PRESENT */
	u32_t evt;
	u32_t task;

	evt = nrfx_rtc_event_address_get(rtc, NRF_RTC_EVENT_COMPARE_0);
	task = nrfx_rtc_task_address_get(rtc, NRF_RTC_TASK_CLEAR);

	result = nrfx_ppi_channel_alloc(&data->ppi_ch);
	if (result != NRFX_SUCCESS) {
		LOG_INST_ERR(nrfx_config->log,
			     "Failed to allocate PPI channel.");
		return -ENODEV;
	}

	(void)nrfx_ppi_channel_assign(data->ppi_ch, evt, task);
	(void)nrfx_ppi_channel_enable(data->ppi_ch);
#endif
#endif /* CONFIG_COUNTER_RTC_WITH_PPI_WRAP */
	return 0;
}

static int init_rtc(struct device *dev,
		    const nrfx_rtc_config_t *config,
		    nrfx_rtc_handler_t handler)
{
	struct device *clock;
	const struct counter_nrfx_config *nrfx_config = get_nrfx_config(dev);
	struct counter_nrfx_data *data = get_dev_data(dev);
	const nrfx_rtc_t *rtc = &nrfx_config->rtc;
	int err;

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

	err = ppi_setup(dev);
	if (err != 0) {
		return err;
	}

	data->top = COUNTER_MAX_TOP_VALUE;

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
	BUILD_ASSERT_MSG((DT_NORDIC_NRF_RTC_RTC_##idx##_PRESCALER - 1) <=      \
			RTC_PRESCALER_PRESCALER_Msk,			       \
			"RTC prescaler out of range");			       \
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
			.prescaler =					       \
				DT_NORDIC_NRF_RTC_RTC_##idx##_PRESCALER - 1,   \
		};							       \
		return init_rtc(dev, &config, rtc_##idx##_handler);	       \
	}								       \
	static struct counter_nrfx_data counter_##idx##_data;		       \
	static struct counter_nrfx_ch_data				       \
		counter##idx##_ch_data[CC_TO_ID(RTC##idx##_CC_NUM)];	       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_COUNTER_LOG_LEVEL); \
	static const struct counter_nrfx_config nrfx_counter_##idx##z_config = {\
		.info = {						       \
			.max_top_value = COUNTER_MAX_TOP_VALUE,		       \
			.freq = DT_NORDIC_NRF_RTC_RTC_##idx##_CLOCK_FREQUENCY /\
				(DT_NORDIC_NRF_RTC_RTC_##idx##_PRESCALER),     \
			.count_up = true,				       \
			.channels = CC_TO_ID(RTC##idx##_CC_NUM)		       \
		},							       \
		.ch_data = counter##idx##_ch_data,			       \
		.rtc = NRFX_RTC_INSTANCE(idx),				       \
		COND_CODE_1(DT_NORDIC_NRF_RTC_RTC_##idx##_PPI_WRAP,	       \
			    (.use_ppi = true,), ())			       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)	       \
	};								       \
	DEVICE_AND_API_INIT(rtc_##idx,					       \
			    DT_NORDIC_NRF_RTC_RTC_##idx##_LABEL,	       \
			    counter_##idx##_init,			       \
			    &counter_##idx##_data,			       \
			    &nrfx_counter_##idx##z_config.info,		       \
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
