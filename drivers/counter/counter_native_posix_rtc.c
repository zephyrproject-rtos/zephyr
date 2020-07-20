/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <time.h>
#include <device.h>
#include <drivers/counter.h>
#include <native_rtc.h>
#include <logging/log.h>

#define ERR(...) LOG_INST_ERR(printf, __VA_ARGS__)
#define WRN(...) LOG_INST_WRN(printf, __VA_ARGS__)
#define INF(...) LOG_INST_INF(printf, __VA_ARGS__)
#define DBG(...) LOG_INST_DBG(printf, __VA_ARGS__)

#define DT_RTC_NODE_LABEL rtc0

#define COUNTER_NATIVE_POSIX_RTC_FLAGS (COUNTER_CONFIG_INFO_COUNT_UP)
#define COUNTER_NATIVE_POSIX_RTC_CHANNELS_NR 1
#define RTC_MODE (RTC_CLOCK_REALTIME)

#define SIMU_USEC_PER_USER_TICKS (100)
#define COUNTER_NATIVE_POSIX_RTC_FREQ                                         \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / SIMU_USEC_PER_USER_TICKS)
#define SIMU_USEC_TO_USER_TICKS(simu_usec)                                    \
	(simu_usec / SIMU_USEC_PER_USER_TICKS)
#define USER_TICKS_TO_SIMU_USEC(user_ticks)                                   \
	(user_ticks * SIMU_USEC_PER_USER_TICKS)
/* Needed to avoid cpu overload, when waiting for alarm */
#define SLEEP_PERIOD_NSEC (100)

#define LOG_MODULE_NAME counter_posix_rtc
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

struct native_posix_counter_data {
	pthread_t alarm_thr;

	pthread_mutex_t alarm_lock;
	pthread_cond_t alarm_pending_cond;
	pthread_mutex_t pending_cond_lock;
	bool is_alarm_pending;

	struct counter_alarm_cfg next_alarm;
	bool is_ctr_running;
	uint64_t start_offset_us;
	struct device *dev;
};

struct native_posix_counter {
	struct native_posix_counter_data data;
	struct counter_config_info config;
};

static struct native_posix_counter native_posix_ctr = {
	.config = {
			.max_top_value = UINT_MAX,
			.flags = COUNTER_NATIVE_POSIX_RTC_FLAGS,
			.channels = COUNTER_NATIVE_POSIX_RTC_CHANNELS_NR,
			.freq = COUNTER_NATIVE_POSIX_RTC_FREQ,
		},
	.data = {
			.is_ctr_running = false,
			.is_alarm_pending = false,
			.dev = NULL,
		},
};

static inline struct native_posix_counter_data *
get_counter_data(struct device *dev)
{
	return (struct native_posix_counter_data *)dev->driver_data;
}

static struct counter_config_info *get_counter_config(struct device *dev)
{
	struct native_posix_counter_data *data;
	struct native_posix_counter *ctr;

	data = get_counter_data(dev);
	ctr = CONTAINER_OF(data, struct native_posix_counter, data);

	return &ctr->config;
}

static void *alarms_executor(void *pnative_posix_counter_data)
{
	struct native_posix_counter_data *ctr_data =
		(struct native_posix_counter_data *)pnative_posix_counter_data;
	struct timespec req, rem;

	while (true) {
		uint64_t current_simu_usec;
		bool is_alarm_pending;
		struct counter_alarm_cfg alarm_cfg;

		/* Wait for pending alarm */
		pthread_mutex_lock(&ctr_data->pending_cond_lock);
		while (!ctr_data->is_alarm_pending) {
			pthread_cond_wait(&ctr_data->alarm_pending_cond,
					  &ctr_data->pending_cond_lock);
		}
		pthread_mutex_unlock(&ctr_data->pending_cond_lock);

		do {
#ifdef SLEEP_PERIOD_NSEC
			req.tv_nsec = SLEEP_PERIOD_NSEC;
			req.tv_sec = 0;
			nanosleep(&req, &rem);
#endif
			pthread_mutex_lock(&ctr_data->alarm_lock);
			current_simu_usec = native_rtc_gettime_us(RTC_MODE) -
					    ctr_data->start_offset_us;
			if ((ctr_data->next_alarm.ticks <= current_simu_usec)
			    && ctr_data->is_alarm_pending) {
				alarm_cfg = ctr_data->next_alarm;
				ctr_data->is_alarm_pending = false;
				pthread_mutex_unlock(&ctr_data->alarm_lock);
				alarm_cfg.callback(ctr_data->dev, 0,
						   SIMU_USEC_TO_USER_TICKS(
							   current_simu_usec),
						   alarm_cfg.user_data);
			}
			is_alarm_pending = ctr_data->is_alarm_pending;
			pthread_mutex_unlock(&ctr_data->alarm_lock);
		} while (is_alarm_pending);
	}

	/* Shouldn't get here */
	pthread_exit(NULL);
}

static int native_posix_counter_init(struct native_posix_counter *ctr)
{
	int err;

	ctr->data.is_ctr_running = false;
	ctr->data.is_alarm_pending = false;

	err = pthread_mutex_init(&ctr->data.alarm_lock, NULL);
	if (err) {
		return err;
	}

	err = pthread_mutex_init(&ctr->data.pending_cond_lock, NULL);
	if (err) {
		pthread_mutex_destroy(&ctr->data.alarm_lock);
		return err;
	}

	err = pthread_cond_init(&ctr->data.alarm_pending_cond, NULL);
	if (err) {
		pthread_mutex_destroy(&ctr->data.alarm_lock);
		pthread_mutex_destroy(&ctr->data.pending_cond_lock);
		return err;
	}

	err = pthread_create(&ctr->data.alarm_thr, NULL, &alarms_executor,
			     &ctr->data);
	if (err) {
		pthread_mutex_destroy(&ctr->data.alarm_lock);
		pthread_mutex_destroy(&ctr->data.pending_cond_lock);
		pthread_cond_destroy(&ctr->data.alarm_pending_cond);
	}

	return err;
}

static int posix_counter_dev_init(struct device *dev)
{
	struct native_posix_counter_data *data;
	struct native_posix_counter *ctr;

	data = get_counter_data(dev);
	ctr = CONTAINER_OF(data, struct native_posix_counter, data);
	data->dev = dev;

	return native_posix_counter_init(ctr);
}

static int ctr_start(struct device *dev)
{
	struct native_posix_counter_data *data = get_counter_data(dev);

	if (!data->is_ctr_running) {
		data->start_offset_us = native_rtc_gettime_us(RTC_MODE);
		data->is_ctr_running = true;
	}

	return 0;
}

static int ctr_stop(struct device *dev)
{
	struct native_posix_counter_data *data = get_counter_data(dev);

	pthread_mutex_lock(&data->alarm_lock);
	data->is_alarm_pending = false;
	pthread_mutex_unlock(&data->alarm_lock);

	data->is_ctr_running = false;

	return 0;
}

static int ctr_get_value(struct device *dev, uint32_t *ticks)
{
	uint64_t us_elapsed = native_rtc_gettime_us(RTC_MODE) -
			      get_counter_data(dev)->start_offset_us;

	*ticks = SIMU_USEC_TO_USER_TICKS(us_elapsed);

	return 0;
}

static int ctr_set_alarm(struct device *dev, uint8_t chan_id,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct native_posix_counter_data *ctr_data = get_counter_data(dev);
	uint32_t current_time_us =
		native_rtc_gettime_us(RTC_MODE) - ctr_data->start_offset_us;

	if (!ctr_data->is_ctr_running) {
		ERR("%s set alarm failed - counter is not running", dev->name);
		return -EPERM;
	}

	if ((USER_TICKS_TO_SIMU_USEC(alarm_cfg->ticks) <= current_time_us) &&
	    (USER_TICKS_TO_SIMU_USEC(alarm_cfg->ticks) >=
	     get_counter_config(dev)->max_top_value)) {
		return -EINVAL;
	}

	pthread_mutex_lock(&ctr_data->alarm_lock);
	ctr_data->next_alarm = *alarm_cfg;
	ctr_data->next_alarm.ticks = USER_TICKS_TO_SIMU_USEC(alarm_cfg->ticks);

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		uint32_t ticks_now;

		ctr_get_value(dev, &ticks_now);
		ctr_data->next_alarm.ticks =
			USER_TICKS_TO_SIMU_USEC(ticks_now) +
			USER_TICKS_TO_SIMU_USEC(alarm_cfg->ticks);
	}
	pthread_mutex_unlock(&ctr_data->alarm_lock);

	pthread_mutex_lock(&ctr_data->pending_cond_lock);
	ctr_data->is_alarm_pending = true;
	pthread_mutex_unlock(&ctr_data->pending_cond_lock);
	pthread_cond_signal(&ctr_data->alarm_pending_cond);

	return 0;
}

static int ctr_cancel_alarm(struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	struct native_posix_counter_data *data = get_counter_data(dev);

	pthread_mutex_lock(&data->alarm_lock);
	data->is_alarm_pending = false;
	pthread_mutex_unlock(&data->alarm_lock);

	return 0;
}

static uint32_t ctr_get_pending_int(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static uint32_t ctr_get_top_value(struct device *dev)
{
	return get_counter_config(dev)->max_top_value;
}

static uint32_t ctr_get_max_relative_alarm(struct device *dev)
{
	return get_counter_config(dev)->max_top_value;
}

static int ctr_set_top_value(struct device *dev,
			     const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

static const struct counter_driver_api native_posix_ctr_api = {
	.start = ctr_start,
	.stop = ctr_stop,
	.get_value = ctr_get_value,
	.set_alarm = ctr_set_alarm,
	.cancel_alarm = ctr_cancel_alarm,
	.set_top_value = ctr_set_top_value,
	.get_pending_int = ctr_get_pending_int,
	.get_top_value = ctr_get_top_value,
	.get_max_relative_alarm = ctr_get_max_relative_alarm,
};

DEVICE_AND_API_INIT(posix_rtc0, DT_LABEL(DT_NODELABEL(DT_RTC_NODE_LABEL)),
		    &posix_counter_dev_init, &native_posix_ctr.data,
		    &native_posix_ctr.config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &native_posix_ctr_api);
