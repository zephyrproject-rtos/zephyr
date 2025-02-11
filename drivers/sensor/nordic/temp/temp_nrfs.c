/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_temp_nrfs

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <nrfs_temp.h>

LOG_MODULE_REGISTER(temp_nrfs, CONFIG_SENSOR_LOG_LEVEL);

struct temp_nrfs_data {
	struct k_sem measure_sem;
	struct k_mutex mutex;
	int32_t raw_temp;

#ifdef CONFIG_TEMP_NRFS_TRIGGER
	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;
	const struct device *dev;
	struct sensor_value sampling_freq;
	struct sensor_value up_threshold;
	struct sensor_value low_threshold;
#endif
#if defined(CONFIG_TEMP_NRFS_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TEMP_NRFS_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem event_sem;
#elif defined(CONFIG_TEMP_NRFS_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
};

#ifdef CONFIG_TEMP_NRFS_TRIGGER

#define DEFAULT_SAMPLING_FREQ {  1, 0 }
#define DEFAULT_UP_THRESHOLD  { 25, 0 }
#define DEFAULT_LOW_THRESHOLD {  0, 0 }

static void temp_nrfs_handle_event(const struct device *dev)
{
	struct temp_nrfs_data *data = dev->data;
	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;

	k_mutex_lock(&data->mutex, K_FOREVER);
	trigger = data->trigger;
	handler = data->handler;
	k_mutex_unlock(&data->mutex);

	if (handler) {
		handler(dev, &trigger);
	}
}

#if defined(CONFIG_TEMP_NRFS_TRIGGER_OWN_THREAD)
static void temp_nrfs_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct temp_nrfs_data *data = p1;

	while (1) {
		k_sem_take(&data->event_sem, K_FOREVER);
		temp_nrfs_handle_event(data->dev);
	}
}
#elif defined(CONFIG_TEMP_NRFS_TRIGGER_GLOBAL_THREAD)
static void temp_nrfs_work_handler(struct k_work *work)
{
	struct temp_nrfs_data *data =
		CONTAINER_OF(work, struct temp_nrfs_data, work);

	temp_nrfs_handle_event(data->dev);
}
#endif

static uint16_t to_measure_rate_ms(const struct sensor_value *freq_val)
{
	uint32_t measure_rate_ms = (MSEC_PER_SEC * 1000) /
		(uint32_t)sensor_value_to_milli(freq_val);

	return (uint16_t)MIN(measure_rate_ms, UINT16_MAX);
}

static int32_t to_raw_temp(const struct sensor_value *temp_val)
{
	int32_t temp_mul_100 = (int32_t)(sensor_value_to_milli(temp_val) / 10);

	return nrfs_temp_to_raw(temp_mul_100);
}

static int api_sensor_trigger_set(const struct device *dev,
				  const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	struct temp_nrfs_data *data = dev->data;
	nrfs_err_t err;

	if (trig->chan != SENSOR_CHAN_ALL &&
	    trig->chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		k_mutex_lock(&data->mutex, K_FOREVER);
		data->trigger = *trig;
		data->handler = handler;
		k_mutex_unlock(&data->mutex);

		if (handler) {
			err = nrfs_temp_subscribe(
				to_measure_rate_ms(&data->sampling_freq),
				to_raw_temp(&data->low_threshold),
				to_raw_temp(&data->up_threshold),
				data);
		} else {
			err = nrfs_temp_unsubscribe();
		}

		switch (err) {
		case NRFS_SUCCESS:
			break;
		case NRFS_ERR_INVALID_STATE:
			return -EAGAIN;
		default:
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int api_sensor_attr_set(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	struct temp_nrfs_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (sensor_value_to_milli(val) <= 0) {
			return -EINVAL;
		}
		data->sampling_freq = *val;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		data->up_threshold = *val;
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		data->low_threshold = *val;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

#endif /* CONFIG_TEMP_NRFS_TRIGGER */

static void sensor_handler(nrfs_temp_evt_t const *p_evt, void *context)
{
	ARG_UNUSED(context);

	struct temp_nrfs_data *data = context;

	switch (p_evt->type) {
	case NRFS_TEMP_EVT_MEASURE_DONE:
		data->raw_temp = p_evt->raw_temp;
		k_sem_give(&data->measure_sem);
		break;

#ifdef CONFIG_TEMP_NRFS_TRIGGER
	case NRFS_TEMP_EVT_CHANGE:
		data->raw_temp = p_evt->raw_temp;
#if defined(CONFIG_TEMP_NRFS_TRIGGER_OWN_THREAD)
		k_sem_give(&data->event_sem);
#elif defined(CONFIG_TEMP_NRFS_TRIGGER_GLOBAL_THREAD)
		k_work_submit(&data->work);
#endif
		break;
#endif /* CONFIG_TEMP_NRFS_TRIGGER */

	default:
		LOG_DBG("Temperature handler - unsupported event: 0x%x",
			p_evt->type);
		break;
	}
}

static int api_sample_fetch(const struct device *dev,
			    enum sensor_channel chan)
{
	struct temp_nrfs_data *data = dev->data;
	int nrfs_rc;
	int rc = 0;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	nrfs_rc = nrfs_temp_measure_request(data);
	switch (nrfs_rc) {
	case NRFS_SUCCESS:
		k_sem_take(&data->measure_sem, K_FOREVER);
		LOG_DBG("Temperature sample: %d", data->raw_temp);
		break;
	case NRFS_ERR_INVALID_STATE:
		LOG_DBG("Backend is not ready, try again.");
		rc = -EAGAIN;
		break;
	default:
		LOG_DBG("Measure request failed: %d", nrfs_rc);
		rc = -EIO;
		break;
	}
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int api_channel_get(const struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct temp_nrfs_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	int32_t uval = nrfs_temp_from_raw(data->raw_temp);

	val->val1 = uval / 100;
	val->val2 = (abs(uval) % 100) * 10000;

	LOG_DBG("Temperature: %d.%02u[C]", uval / 100, abs(uval) % 100);

	return 0;
}

static int temp_nrfs_init(const struct device *dev)
{
	int rc;

	rc = nrfs_temp_init(sensor_handler);
	if (rc < 0) {
		return rc;
	}

#if defined(CONFIG_TEMP_NRFS_TRIGGER_OWN_THREAD)
	struct temp_nrfs_data *data = dev->data;

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_TEMP_NRFS_THREAD_STACK_SIZE,
			temp_nrfs_thread,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_TEMP_NRFS_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#endif

	return 0;
}

static const struct sensor_driver_api temp_nrfs_drv_api = {
#ifdef CONFIG_TEMP_NRFS_TRIGGER
	.attr_set = api_sensor_attr_set,
	.trigger_set = api_sensor_trigger_set,
#endif
	.sample_fetch = api_sample_fetch,
	.channel_get = api_channel_get
};

static struct temp_nrfs_data temp_nrfs_drv_data = {
	.mutex = Z_MUTEX_INITIALIZER(temp_nrfs_drv_data.mutex),
	.measure_sem = Z_SEM_INITIALIZER(temp_nrfs_drv_data.measure_sem, 0, 1),
#ifdef CONFIG_TEMP_NRFS_TRIGGER
	.dev = DEVICE_DT_INST_GET(0),
	.sampling_freq = DEFAULT_SAMPLING_FREQ,
	.up_threshold  = DEFAULT_UP_THRESHOLD,
	.low_threshold = DEFAULT_LOW_THRESHOLD,
#endif
#if defined(CONFIG_TEMP_NRFS_TRIGGER_OWN_THREAD)
	.event_sem = Z_SEM_INITIALIZER(temp_nrfs_drv_data.event_sem, 0, 1),
#elif defined(CONFIG_TEMP_NRFS_TRIGGER_GLOBAL_THREAD)
	.work = Z_WORK_INITIALIZER(temp_nrfs_work_handler),
#endif
};

DEVICE_DT_INST_DEFINE(0, temp_nrfs_init, NULL,
		      &temp_nrfs_drv_data, NULL,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		      &temp_nrfs_drv_api);
