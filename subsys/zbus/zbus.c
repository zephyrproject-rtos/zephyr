/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_REGISTER(zbus, CONFIG_ZBUS_LOG_LEVEL);

int _zbus_init(void)
{
	const struct zbus_channel *curr = NULL;
	const struct zbus_channel *prev = NULL;

	STRUCT_SECTION_FOREACH(zbus_channel_observation, observation) {
		curr = observation->chan;

		if (prev != curr) {
			if (prev == NULL) {
				curr->data->observers_start_idx = 0;
				curr->data->observers_end_idx = 0;
			} else {
				curr->data->observers_start_idx = prev->data->observers_end_idx;
				curr->data->observers_end_idx = prev->data->observers_end_idx;
			}
			prev = curr;
		}

		++(curr->data->observers_end_idx);
	}
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		k_mutex_init(&chan->data->mutex);

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS)
		sys_slist_init(&chan->data->observers);
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */
	}
	return 0;
}
SYS_INIT(_zbus_init, APPLICATION, CONFIG_ZBUS_CHANNELS_SYS_INIT_PRIORITY);

static inline int _zbus_notify_observer(const struct zbus_channel *chan,
					const struct zbus_observer *obs, k_timepoint_t end_time)
{
	int err = 0;

	if (obs->type == ZBUS_OBSERVER_LISTENER_TYPE) {
		obs->callback(chan);

	} else if (obs->type == ZBUS_OBSERVER_SUBSCRIBER_TYPE) {
		err = k_msgq_put(obs->queue, &chan, sys_timepoint_timeout(end_time));
	} else {
		CODE_UNREACHABLE;
	}
	return err;
}

static inline int _zbus_vded_exec(const struct zbus_channel *chan, k_timepoint_t end_time)
{
	int err = 0;
	int last_error = 0;

	_ZBUS_ASSERT(chan != NULL, "chan is required");

	/* Static observer event dispatcher logic */
	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

	for (int16_t i = chan->data->observers_start_idx, limit = chan->data->observers_end_idx;
	     i < limit; ++i) {
		STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
		STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

		_ZBUS_ASSERT(observation != NULL, "observation must be not NULL");

		const struct zbus_observer *obs = observation->obs;

		if (!obs->enabled || observation_mask->enabled) {
			continue;
		}

		err = _zbus_notify_observer(chan, obs, end_time);

		_ZBUS_ASSERT(err == 0,
			     "could not deliver notification to observer %s. Error code %d",
			     _ZBUS_OBS_NAME(obs), err);

		if (err) {
			last_error = err;
		}
	}

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS)
	/* Dynamic observer event dispatcher logic */
	struct zbus_observer_node *obs_nd, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&chan->data->observers, obs_nd, tmp, node) {

		_ZBUS_ASSERT(obs_nd != NULL, "observer node is NULL");

		const struct zbus_observer *obs = obs_nd->obs;

		if (!obs->enabled) {
			continue;
		}

		err = _zbus_notify_observer(chan, obs, end_time);

		if (err) {
			last_error = err;
		}
	}
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */

	return last_error;
}

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg, k_timeout_t timeout)
{
	int err;

	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");

	k_timepoint_t end_time = sys_timepoint_calc(timeout);

	if (chan->validator != NULL && !chan->validator(msg, chan->message_size)) {
		return -ENOMSG;
	}

	err = k_mutex_lock(&chan->data->mutex, timeout);
	if (err) {
		return err;
	}

	memcpy(chan->message, msg, chan->message_size);

	err = _zbus_vded_exec(chan, end_time);

	k_mutex_unlock(&chan->data->mutex);

	return err;
}

int zbus_chan_read(const struct zbus_channel *chan, void *msg, k_timeout_t timeout)
{
	int err;

	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");

	err = k_mutex_lock(&chan->data->mutex, timeout);
	if (err) {
		return err;
	}

	memcpy(msg, chan->message, chan->message_size);

	return k_mutex_unlock(&chan->data->mutex);
}

int zbus_chan_notify(const struct zbus_channel *chan, k_timeout_t timeout)
{
	int err;

	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	k_timepoint_t end_time = sys_timepoint_calc(timeout);

	err = k_mutex_lock(&chan->data->mutex, timeout);
	if (err) {
		return err;
	}

	err = _zbus_vded_exec(chan, end_time);

	k_mutex_unlock(&chan->data->mutex);

	return err;
}

int zbus_chan_claim(const struct zbus_channel *chan, k_timeout_t timeout)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	int err = k_mutex_lock(&chan->data->mutex, timeout);

	if (err) {
		return err;
	}

	return 0;
}

int zbus_chan_finish(const struct zbus_channel *chan)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	int err = k_mutex_unlock(&chan->data->mutex);

	return err;
}

int zbus_sub_wait(const struct zbus_observer *sub, const struct zbus_channel **chan,
		  k_timeout_t timeout)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(sub != NULL, "sub is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	if (sub->queue == NULL) {
		return -EINVAL;
	}

	return k_msgq_get(sub->queue, chan, timeout);
}

int zbus_obs_set_chan_notification_mask(const struct zbus_observer *obs,
					const struct zbus_channel *chan, bool masked)
{
	_ZBUS_ASSERT(obs != NULL, "obs is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

	for (int16_t i = chan->data->observers_start_idx, limit = chan->data->observers_end_idx;
	     i < limit; ++i) {
		STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
		STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

		_ZBUS_ASSERT(observation != NULL, "observation must be not NULL");

		if (observation->obs == obs) {
			observation_mask->enabled = masked;
			return 0;
		}
	}
	return -ESRCH;
}

int zbus_obs_is_chan_notification_masked(const struct zbus_observer *obs,
					 const struct zbus_channel *chan, bool *masked)
{
	_ZBUS_ASSERT(obs != NULL, "obs is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

	for (int16_t i = chan->data->observers_start_idx, limit = chan->data->observers_end_idx;
	     i < limit; ++i) {
		STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
		STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

		_ZBUS_ASSERT(observation != NULL, "observation must be not NULL");

		if (observation->obs == obs) {
			*masked = observation_mask->enabled;
			return 0;
		}
	}
	return -ESRCH;
}
