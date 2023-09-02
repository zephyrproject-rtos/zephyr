/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/buf.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_REGISTER(zbus, CONFIG_ZBUS_LOG_LEVEL);

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER)

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_DYNAMIC)

NET_BUF_POOL_HEAP_DEFINE(_zbus_msg_subscribers_pool, CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE,
			 sizeof(struct zbus_channel *), NULL);
BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE > 0, "MSG_SUBSCRIBER feature requires heap memory pool.");

static inline struct net_buf *_zbus_create_net_buf(struct net_buf_pool *pool, size_t size,
						   k_timeout_t timeout)
{
	return net_buf_alloc_len(&_zbus_msg_subscribers_pool, size, timeout);
}

#else

NET_BUF_POOL_FIXED_DEFINE(_zbus_msg_subscribers_pool,
			  (CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE),
			  (CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE),
			  sizeof(struct zbus_channel *), NULL);

static inline struct net_buf *_zbus_create_net_buf(struct net_buf_pool *pool, size_t size,
						   k_timeout_t timeout)
{
	__ASSERT(size <= CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE,
		 "CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE must be greater or equal to "
		 "%d",
		 (int)size);
	return net_buf_alloc(&_zbus_msg_subscribers_pool, timeout);
}
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_DYNAMIC */

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

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
					const struct zbus_observer *obs, k_timepoint_t end_time,
					struct net_buf *buf)
{
	switch (obs->type) {
	case ZBUS_OBSERVER_LISTENER_TYPE: {
		obs->callback(chan);
		break;
	}
	case ZBUS_OBSERVER_SUBSCRIBER_TYPE: {
		return k_msgq_put(obs->queue, &chan, sys_timepoint_timeout(end_time));
	}
#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER)
	case ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE: {
		struct net_buf *cloned_buf = net_buf_clone(buf, sys_timepoint_timeout(end_time));

		if (cloned_buf == NULL) {
			return -ENOMEM;
		}
		memcpy(net_buf_user_data(cloned_buf), &chan, sizeof(struct zbus_channel *));

		net_buf_put(obs->message_fifo, cloned_buf);

		break;
	}
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

	default:
		_ZBUS_ASSERT(false, "Unreachable");
	}
	return 0;
}

static inline int _zbus_vded_exec(const struct zbus_channel *chan, k_timepoint_t end_time)
{
	int err = 0;
	int last_error = 0;
	struct net_buf *buf = NULL;

	/* Static observer event dispatcher logic */
	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER)
	buf = _zbus_create_net_buf(&_zbus_msg_subscribers_pool, zbus_chan_msg_size(chan),
				   sys_timepoint_timeout(end_time));

	_ZBUS_ASSERT(buf != NULL, "net_buf zbus_msg_subscribers_pool is "
				  "unavailable or heap is full");

	net_buf_add_mem(buf, zbus_chan_msg(chan), zbus_chan_msg_size(chan));
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

	LOG_DBG("Notifing %s's observers. Starting VDED:", _ZBUS_CHAN_NAME(chan));

	int __maybe_unused index = 0;

	for (int16_t i = chan->data->observers_start_idx, limit = chan->data->observers_end_idx;
	     i < limit; ++i) {
		STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
		STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

		_ZBUS_ASSERT(observation != NULL, "observation must be not NULL");

		const struct zbus_observer *obs = observation->obs;

		if (!obs->enabled || observation_mask->enabled) {
			continue;
		}

		err = _zbus_notify_observer(chan, obs, end_time, buf);

		if (err) {
			last_error = err;
			LOG_ERR("could not deliver notification to observer %s. Error code %d",
				_ZBUS_OBS_NAME(obs), err);
			if (err == -ENOMEM) {
				if (IS_ENABLED(CONFIG_ZBUS_MSG_SUBSCRIBER)) {
					net_buf_unref(buf);
				}
				return err;
			}
		}

		LOG_DBG(" %d -> %s", index++, _ZBUS_OBS_NAME(obs));
	}

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS)
	/* Dynamic observer event dispatcher logic */
	struct zbus_observer_node *obs_nd, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&chan->data->observers, obs_nd, tmp, node) {

		const struct zbus_observer *obs = obs_nd->obs;

		if (!obs->enabled) {
			continue;
		}

		err = _zbus_notify_observer(chan, obs, end_time, buf);

		if (err) {
			last_error = err;
		}
	}
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */

	IF_ENABLED(CONFIG_ZBUS_MSG_SUBSCRIBER, (net_buf_unref(buf);))

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
	_ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_SUBSCRIBER_TYPE, "sub must be a SUBSCRIBER");
	_ZBUS_ASSERT(sub->queue != NULL, "sub queue is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	return k_msgq_get(sub->queue, chan, timeout);
}

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER)

int zbus_sub_wait_msg(const struct zbus_observer *sub, const struct zbus_channel **chan, void *msg,
		      k_timeout_t timeout)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus subscribers cannot be used inside ISRs");
	_ZBUS_ASSERT(sub != NULL, "sub is required");
	_ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,
		     "sub must be a MSG_SUBSCRIBER");
	_ZBUS_ASSERT(sub->message_fifo != NULL, "sub message_fifo is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");

	struct net_buf *buf = net_buf_get(sub->message_fifo, timeout);

	if (buf == NULL) {
		return -ENOMSG;
	}

	*chan = *((struct zbus_channel **)net_buf_user_data(buf));

	memcpy(msg, net_buf_remove_mem(buf, zbus_chan_msg_size(*chan)), zbus_chan_msg_size(*chan));

	net_buf_unref(buf);

	return 0;
}

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

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
