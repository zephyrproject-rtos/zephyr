/*
 * Copyright (c) 2021, Sateesh Kotapati
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_stimer

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <em_cmu.h>
#include <sl_atomic.h>
#include <sl_sleeptimer.h>
#include <sli_sleeptimer_hal.h>

LOG_MODULE_REGISTER(counter_gecko, CONFIG_COUNTER_LOG_LEVEL);

#if SL_SLEEPTIMER_PERIPHERAL == SL_SLEEPTIMER_PERIPHERAL_RTCC
#define STIMER_IRQ_HANDLER RTCC_IRQHandler
#define STIMER_MAX_VALUE _RTCC_CNT_MASK
#elif SL_SLEEPTIMER_PERIPHERAL == SL_SLEEPTIMER_PERIPHERAL_SYSRTC
#define STIMER_IRQ_HANDLER SYSRTC_APP_IRQHandler
#define STIMER_MAX_VALUE _SYSRTC_CNT_MASK
#else
#error "Unsupported sleep timer peripheral"
#endif

#define STIMER_ALARM_NUM 2

struct counter_gecko_config {
	struct counter_config_info info;
	void (*irq_config)(void);
	uint32_t prescaler;
};

struct counter_gecko_alarm_data {
	counter_alarm_callback_t callback;
	uint8_t chan_id;
	uint32_t ticks;
	struct device *dev;
	void *user_data;
};

struct counter_gecko_top_data {
	counter_top_callback_t callback;
	uint32_t ticks;
	struct device *dev;
	void *user_data;
};

struct counter_gecko_data {
	struct counter_gecko_alarm_data alarm[STIMER_ALARM_NUM];
	struct counter_gecko_top_data top_data;
};

static sl_sleeptimer_timer_handle_t alarm_timer[STIMER_ALARM_NUM];
static sl_sleeptimer_timer_handle_t top_timer;

#ifdef CONFIG_SOC_GECKO_HAS_ERRATA_RTCC_E201
#define ERRATA_RTCC_E201_MESSAGE                                                                   \
	"Errata RTCC_E201: In case RTCC prescaler != 1 the module does not "                       \
	"reset the counter value on CCV1 compare."
#endif

static void alarm_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	struct counter_gecko_alarm_data *alarm_data = (struct counter_gecko_alarm_data *)data;
	uint32_t count =
		((sl_sleeptimer_get_tick_count()) %
		 (((struct counter_gecko_data *const)(alarm_data->dev)->data)->top_data.ticks));

	if (alarm_data->callback != NULL) {
		alarm_data->callback(
			alarm_data->dev, alarm_data->chan_id, count,
			((struct counter_alarm_cfg *)(alarm_data->user_data))->user_data);
	}
}

static void top_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	struct counter_gecko_top_data *top_data = (struct counter_gecko_top_data *)data;

	if (top_data->callback != NULL) {
		top_data->callback(top_data->dev,
				   ((struct counter_top_cfg *)(top_data->user_data))->user_data);
	}
}

static int counter_gecko_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;

	*ticks = ((sl_sleeptimer_get_tick_count()) % (dev_data->top_data.ticks));

	return 0;
}

static int counter_gecko_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	sl_status_t error_code;
	bool is_top_timer_running = false;

	error_code = sl_sleeptimer_is_timer_running(&top_timer, &is_top_timer_running);
	if ((error_code == SL_STATUS_OK) && (is_top_timer_running == true)) {
		return 0;
	}
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;

	error_code = sl_sleeptimer_start_timer(&top_timer, dev_data->top_data.ticks, top_callback,
					       (void *)&dev_data->top_data, 0, 0);
	return error_code;
}

static int counter_gecko_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	sl_status_t error_code;
	bool is_top_timer_running = false;

	error_code = sl_sleeptimer_is_timer_running(&top_timer, &is_top_timer_running);
	if ((error_code == SL_STATUS_OK) && (is_top_timer_running == true)) {
		sl_sleeptimer_stop_timer(&top_timer);
	}
	return error_code;
}

static int counter_gecko_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;
	sl_status_t error_code;
	bool is_top_timer_running = false;

#ifdef CONFIG_SOC_GECKO_HAS_ERRATA_RTCC_E201
	const struct counter_gecko_config *const dev_cfg =
		(const struct counter_gecko_config *const)(dev)->config;

	if (dev_cfg->prescaler != 1) {
		LOG_ERR(ERRATA_RTCC_E201_MESSAGE);
		return -EINVAL;
	}
#endif

	error_code = sl_sleeptimer_is_timer_running(&top_timer, &is_top_timer_running);
	if ((error_code == SL_STATUS_OK) && (is_top_timer_running == true)) {
		sl_sleeptimer_stop_timer(&top_timer);
	}

	dev_data->top_data.callback = cfg->callback;
	dev_data->top_data.ticks = cfg->ticks;
	dev_data->top_data.dev = (struct device *)dev;
	dev_data->top_data.user_data = (struct counter_top_cfg *)cfg;

	error_code = sl_sleeptimer_start_periodic_timer(&top_timer, cfg->ticks, top_callback,
							(void *)&dev_data->top_data, 0, cfg->flags);

	return error_code;
}

static uint32_t counter_gecko_get_top_value(const struct device *dev)
{
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;

	return dev_data->top_data.ticks;
}

static int counter_gecko_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	bool is_alarm_timer_running = false;
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;
	sl_status_t error_code;
	uint32_t now_ticks = 0;
	uint32_t top_val = counter_gecko_get_top_value(dev);

	if ((top_val != 0) && (alarm_cfg->ticks > top_val)) {
		return -EINVAL;
	}

	if (chan_id >= STIMER_ALARM_NUM) {
		printk("Alarm timer count exceeded\n");
		return -EINVAL;
	}

	error_code = sl_sleeptimer_is_timer_running(&alarm_timer[chan_id], &is_alarm_timer_running);
	if ((error_code == SL_STATUS_OK) && (is_alarm_timer_running == true)) {
		sl_sleeptimer_stop_timer(&alarm_timer[chan_id]);
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		/* Absolute */
		error_code = counter_gecko_get_value(dev, &now_ticks);
		if (now_ticks < alarm_cfg->ticks) {
			dev_data->alarm[chan_id].ticks = top_val + (alarm_cfg->ticks - now_ticks);
		} else {
			dev_data->alarm[chan_id].ticks =
				(top_val - (now_ticks - alarm_cfg->ticks)) % top_val;
		}

	} else {
		/* Relative */
		dev_data->alarm[chan_id].ticks = alarm_cfg->ticks;
	}

	dev_data->alarm[chan_id].callback = alarm_cfg->callback;
	dev_data->alarm[chan_id].chan_id = chan_id;
	dev_data->alarm[chan_id].dev = (struct device *)dev;
	dev_data->alarm[chan_id].user_data = (struct counter_alarm_cfg *)alarm_cfg;

	error_code =
		sl_sleeptimer_start_timer(&alarm_timer[chan_id], dev_data->alarm[chan_id].ticks,
					  alarm_callback, (void *)&dev_data->alarm[chan_id], 0, 0);

	return 0;
}

static int counter_gecko_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;

	if (chan_id >= STIMER_ALARM_NUM) {
		LOG_DBG("Alarm timer count exceeded\n");
		return -EINVAL;
	}

	sl_sleeptimer_stop_timer(&alarm_timer[chan_id]);

	dev_data->alarm[chan_id].callback = NULL;
	dev_data->alarm[chan_id].user_data = NULL;

	LOG_DBG("cancel alarm: channel %u", chan_id);

	return 0;
}

static uint32_t counter_gecko_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int counter_gecko_init(const struct device *dev)
{
	const struct counter_gecko_config *const dev_cfg =
		(const struct counter_gecko_config *const)(dev)->config;
	struct counter_gecko_data *const dev_data = (struct counter_gecko_data *const)(dev)->data;

	sl_sleeptimer_init();
	dev_data->top_data.ticks = STIMER_MAX_VALUE;

	/* Configure & enable module interrupts */
	dev_cfg->irq_config();

	LOG_INF("Device %s initialized", (dev)->name);

	return 0;
}

static const struct counter_driver_api counter_gecko_driver_api = {
	.start = counter_gecko_start,
	.stop = counter_gecko_stop,
	.get_value = counter_gecko_get_value,
	.set_alarm = counter_gecko_set_alarm,
	.cancel_alarm = counter_gecko_cancel_alarm,
	.set_top_value = counter_gecko_set_top_value,
	.get_pending_int = counter_gecko_get_pending_int,
	.get_top_value = counter_gecko_get_top_value,
};

BUILD_ASSERT((DT_INST_PROP(0, prescaler) > 0U) && (DT_INST_PROP(0, prescaler) <= 32768U));

static void counter_gecko_0_irq_config(void)
{
	IRQ_DIRECT_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), STIMER_IRQ_HANDLER, 0);
	irq_enable(DT_INST_IRQN(0));
}

static const struct counter_gecko_config counter_gecko_0_config = {
	.info = {
			.max_top_value = STIMER_MAX_VALUE,
			.freq = DT_INST_PROP(0, clock_frequency) / DT_INST_PROP(0, prescaler),
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,
			.channels = STIMER_ALARM_NUM,
		},
	.irq_config = counter_gecko_0_irq_config,
	.prescaler = DT_INST_PROP(0, prescaler),
};

static struct counter_gecko_data counter_gecko_0_data;

DEVICE_DT_INST_DEFINE(0, counter_gecko_init, NULL, &counter_gecko_0_data, &counter_gecko_0_config,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &counter_gecko_driver_api);
