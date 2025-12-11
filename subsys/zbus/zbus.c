/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_REGISTER(zbus, CONFIG_ZBUS_LOG_LEVEL);

#if defined(CONFIG_ZBUS_PRIORITY_BOOST)
/* Available only when the priority boost is enabled */
static struct k_spinlock _zbus_chan_slock;
#endif /* CONFIG_ZBUS_PRIORITY_BOOST */

static struct k_spinlock obs_slock;

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER)

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC)

NET_BUF_POOL_HEAP_DEFINE(_zbus_msg_subscribers_pool, CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE,
			 sizeof(struct zbus_channel *), NULL);

static inline struct net_buf *_zbus_create_net_buf(struct net_buf_pool *pool, size_t size,
						   k_timeout_t timeout)
{
	return net_buf_alloc_len(pool, size, timeout);
}

#else

NET_BUF_POOL_FIXED_DEFINE(_zbus_msg_subscribers_pool,
			  (CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE),
			  (CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE),
			  sizeof(struct zbus_channel *), NULL);

static inline struct net_buf *_zbus_create_net_buf(struct net_buf_pool *pool, size_t size,
						   k_timeout_t timeout)
{
	__ASSERT(size <= CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE,
		 "CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE must be greater or equal to "
		 "%d",
		 (int)size);
	return net_buf_alloc(pool, timeout);
}
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC */

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

#if defined(CONFIG_ZBUS_CHANNEL_ID)
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		/* Check for duplicate channel IDs */
		if (chan->id == ZBUS_CHAN_ID_INVALID) {
			continue;
		}
		/* Iterate over all previous channels */
		STRUCT_SECTION_FOREACH(zbus_channel, chan_prev) {
			if (chan_prev == chan) {
				break;
			}
			if (chan->id == chan_prev->id) {
#if defined(CONFIG_ZBUS_CHANNEL_NAME)
				LOG_WRN("Channels %s and %s have matching IDs (%d)", chan->name,
					chan_prev->name, chan->id);
#else
				LOG_WRN("Channels %p and %p have matching IDs (%d)", chan,
					chan_prev, chan->id);
#endif /* CONFIG_ZBUS_CHANNEL_NAME */
			}
		}
	}
#endif /* CONFIG_ZBUS_CHANNEL_ID */

	return 0;
}
SYS_INIT(_zbus_init, APPLICATION, CONFIG_ZBUS_CHANNELS_SYS_INIT_PRIORITY);

#if defined(CONFIG_ZBUS_ASYNC_LISTENER)
void async_listener_work_handler(struct k_work *item)
{
	struct zbus_async_listener_work *async_listener =
		CONTAINER_OF(item, struct zbus_async_listener_work, work);

	__ASSERT(async_listener != NULL, "async_listener required");
	__ASSERT(async_listener->callback != NULL, "callback required");
	__ASSERT(async_listener->queue != NULL, "queue required");
	__ASSERT(async_listener->message_fifo != NULL, "async listener message_fifo is required");

	while (k_fifo_is_empty(async_listener->message_fifo) == 0) {

		struct net_buf *buf = k_fifo_get(async_listener->message_fifo,
						 K_MSEC(CONFIG_ZBUS_ASYNC_LISTENER_EXEC_TIMEOUT));
		__ASSERT(buf != NULL, "buf element required");

		if (buf == NULL) {
			LOG_ERR("Could not retrieve message from async listener fifo");
			return;
		}

		struct zbus_channel **chan = ((struct zbus_channel **)net_buf_user_data(buf));

		__ASSERT_NO_MSG(*chan != NULL);

		async_listener->callback(*chan, net_buf_remove_mem(buf, zbus_chan_msg_size(*chan)));

		net_buf_unref(buf);
	}
}
#endif /* CONFIG_ZBUS_ASYNC_LISTENER */

#if defined(CONFIG_ZBUS_CHANNEL_ID)

const struct zbus_channel *zbus_chan_from_id(uint32_t channel_id)
{
	if (channel_id == ZBUS_CHAN_ID_INVALID) {
		return NULL;
	}
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		if (chan->id == channel_id) {
			/* Found matching channel */
			return chan;
		}
	}
	/* No matching channel exists */
	return NULL;
}

#endif /* CONFIG_ZBUS_CHANNEL_ID */

#if defined(CONFIG_ZBUS_CHANNEL_NAME)

const struct zbus_channel *zbus_chan_from_name(const char *name)
{
	CHECKIF(name == NULL) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		if (strcmp(chan->name, name) == 0) {
			/* Found matching channel */
			return chan;
		}
	}
	/* No matching channel exists */
	return NULL;
}

#endif /* CONFIG_ZBUS_CHANNEL_NAME */

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

		k_fifo_put(obs->message_fifo, cloned_buf);

		break;
	}
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

#if defined(CONFIG_ZBUS_ASYNC_LISTENER)
	case ZBUS_OBSERVER_ASYNC_LISTENER_TYPE: {
		struct net_buf *cloned_buf = net_buf_clone(buf, sys_timepoint_timeout(end_time));

		if (cloned_buf == NULL) {
			return -ENOMEM;
		}

		struct zbus_async_listener_work *async_listener =
			CONTAINER_OF(obs->work, struct zbus_async_listener_work, work);

		k_fifo_put(async_listener->message_fifo, cloned_buf);

		int ret;

		ret = k_work_submit_to_queue(async_listener->queue, obs->work);
		if (ret < 0) {
			return ret;
		}

		break;
	}
#endif /* CONFIG_ZBUS_ASYNC_LISTENER */
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
	struct net_buf_pool *pool =
		COND_CODE_1(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION,
			    (chan->data->msg_subscriber_pool), (&_zbus_msg_subscribers_pool));

	buf = _zbus_create_net_buf(pool, zbus_chan_msg_size(chan), sys_timepoint_timeout(end_time));

	_ZBUS_ASSERT(buf != NULL, "net_buf zbus_msg_subscribers_pool is "
				  "unavailable or heap is full");

	memcpy(net_buf_user_data(buf), &chan, sizeof(struct zbus_channel *));

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

		if (!obs->data->enabled || observation_mask->enabled) {
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

		if (!obs->data->enabled) {
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

#if defined(CONFIG_ZBUS_PRIORITY_BOOST)

static inline void chan_update_hop(const struct zbus_channel *chan)
{
	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

	int chan_highest_observer_priority = ZBUS_MIN_THREAD_PRIORITY;

	K_SPINLOCK(&_zbus_chan_slock) {
		const int limit = chan->data->observers_end_idx;

		for (int16_t i = chan->data->observers_start_idx; i < limit; ++i) {
			STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
			STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

			__ASSERT(observation != NULL, "observation must be not NULL");

			const struct zbus_observer *obs = observation->obs;

			if (!obs->data->enabled || observation_mask->enabled) {
				continue;
			}

			if (chan_highest_observer_priority > obs->data->priority) {
				chan_highest_observer_priority = obs->data->priority;
			}
		}
		chan->data->highest_observer_priority = chan_highest_observer_priority;
	}
}

static inline void update_all_channels_hop(const struct zbus_observer *obs)
{
	STRUCT_SECTION_FOREACH(zbus_channel_observation, observation) {
		if (obs != observation->obs) {
			continue;
		}

		chan_update_hop(observation->chan);
	}
}

int zbus_obs_attach_to_thread(const struct zbus_observer *obs)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "cannot attach to an ISR");
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	int current_thread_priority = k_thread_priority_get(k_current_get());

	K_SPINLOCK(&obs_slock) {
		if (obs->data->priority != current_thread_priority) {
			obs->data->priority = current_thread_priority;

			update_all_channels_hop(obs);
		}
	}

	return 0;
}

int zbus_obs_detach_from_thread(const struct zbus_observer *obs)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "cannot detach from an ISR");
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	K_SPINLOCK(&obs_slock) {
		obs->data->priority = ZBUS_MIN_THREAD_PRIORITY;

		update_all_channels_hop(obs);
	}

	return 0;
}

#else

static inline void update_all_channels_hop(const struct zbus_observer *obs)
{
}

#endif /* CONFIG_ZBUS_PRIORITY_BOOST */

static inline int chan_lock(const struct zbus_channel *chan, k_timeout_t timeout, int *prio)
{
	bool boosting = false;

#if defined(CONFIG_ZBUS_PRIORITY_BOOST)
	if (!k_is_in_isr()) {
		*prio = k_thread_priority_get(k_current_get());

		K_SPINLOCK(&_zbus_chan_slock) {
			if (*prio > chan->data->highest_observer_priority) {
				int new_prio = chan->data->highest_observer_priority - 1;

				new_prio = MAX(new_prio, 0);

				/* Elevating priority since the highest_observer_priority is
				 * greater than the current thread
				 */
				k_thread_priority_set(k_current_get(), new_prio);

				boosting = true;
			}
		}
	}
#endif /* CONFIG_ZBUS_PRIORITY_BOOST */

	int err = k_sem_take(&chan->data->sem, timeout);

	if (err) {
		/* When the priority boost is disabled, this IF will be optimized out. */
		if (boosting) {
			/* Restoring thread priority since the semaphore is not available */
			k_thread_priority_set(k_current_get(), *prio);
		}

		return err;
	}

	return 0;
}

static inline void chan_unlock(const struct zbus_channel *chan, int prio)
{
	k_sem_give(&chan->data->sem);

#if defined(CONFIG_ZBUS_PRIORITY_BOOST)
	/* During the unlock phase, with the priority boost enabled, the priority must be
	 * restored to the original value in case it was elevated
	 */
	if (prio < ZBUS_MIN_THREAD_PRIORITY) {
		k_thread_priority_set(k_current_get(), prio);
	}
#endif /* CONFIG_ZBUS_PRIORITY_BOOST */
}

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg, k_timeout_t timeout)
{
	int err;

	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");
	_ZBUS_ASSERT(k_is_in_isr() ? K_TIMEOUT_EQ(timeout, K_NO_WAIT) : true,
		     "inside an ISR, the timeout must be K_NO_WAIT");

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	k_timepoint_t end_time = sys_timepoint_calc(timeout);

	if (chan->validator != NULL && !chan->validator(msg, chan->message_size)) {
		return -ENOMSG;
	}

	int context_priority = ZBUS_MIN_THREAD_PRIORITY;

	err = chan_lock(chan, timeout, &context_priority);
	if (err) {
		return err;
	}

#if defined(CONFIG_ZBUS_CHANNEL_PUBLISH_STATS)
	chan->data->publish_timestamp = k_uptime_ticks();
	chan->data->publish_count += 1;
#endif /* CONFIG_ZBUS_CHANNEL_PUBLISH_STATS */

	memcpy(chan->message, msg, chan->message_size);

	err = _zbus_vded_exec(chan, end_time);

	chan_unlock(chan, context_priority);

	return err;
}

int zbus_chan_read(const struct zbus_channel *chan, void *msg, k_timeout_t timeout)
{
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");
	_ZBUS_ASSERT(k_is_in_isr() ? K_TIMEOUT_EQ(timeout, K_NO_WAIT) : true,
		     "inside an ISR, the timeout must be K_NO_WAIT");

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	int err = k_sem_take(&chan->data->sem, timeout);
	if (err) {
		return err;
	}

	memcpy(msg, chan->message, chan->message_size);

	k_sem_give(&chan->data->sem);

	return 0;
}

int zbus_chan_notify(const struct zbus_channel *chan, k_timeout_t timeout)
{
	int err;

	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(k_is_in_isr() ? K_TIMEOUT_EQ(timeout, K_NO_WAIT) : true,
		     "inside an ISR, the timeout must be K_NO_WAIT");

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	k_timepoint_t end_time = sys_timepoint_calc(timeout);

	int context_priority = ZBUS_MIN_THREAD_PRIORITY;

	err = chan_lock(chan, timeout, &context_priority);
	if (err) {
		return err;
	}

	err = _zbus_vded_exec(chan, end_time);

	chan_unlock(chan, context_priority);

	return err;
}

int zbus_chan_claim(const struct zbus_channel *chan, k_timeout_t timeout)
{
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(k_is_in_isr() ? K_TIMEOUT_EQ(timeout, K_NO_WAIT) : true,
		     "inside an ISR, the timeout must be K_NO_WAIT");

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	int err = k_sem_take(&chan->data->sem, timeout);

	if (err) {
		return err;
	}

	return 0;
}

int zbus_chan_finish(const struct zbus_channel *chan)
{
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	k_sem_give(&chan->data->sem);

	return 0;
}

int zbus_sub_wait(const struct zbus_observer *sub, const struct zbus_channel **chan,
		  k_timeout_t timeout)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus_sub_wait cannot be used inside ISRs");
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
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus_sub_wait_msg cannot be used inside ISRs");
	_ZBUS_ASSERT(sub != NULL, "sub is required");
	_ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,
		     "sub must be a MSG_SUBSCRIBER");
	_ZBUS_ASSERT(sub->message_fifo != NULL, "sub message_fifo is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");

	struct net_buf *buf = k_fifo_get(sub->message_fifo, timeout);

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

	int err = -ESRCH;

	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

	K_SPINLOCK(&obs_slock) {
		for (int16_t i = chan->data->observers_start_idx,
			     limit = chan->data->observers_end_idx;
		     i < limit; ++i) {
			STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
			STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

			__ASSERT(observation != NULL, "observation must be not NULL");

			if (observation->obs == obs) {
				if (observation_mask->enabled != masked) {
					observation_mask->enabled = masked;

					update_all_channels_hop(obs);
				}

				err = 0;

				K_SPINLOCK_BREAK;
			}
		}
	}

	return err;
}

int zbus_obs_is_chan_notification_masked(const struct zbus_observer *obs,
					 const struct zbus_channel *chan, bool *masked)
{
	_ZBUS_ASSERT(obs != NULL, "obs is required");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	int err = -ESRCH;

	struct zbus_channel_observation *observation;
	struct zbus_channel_observation_mask *observation_mask;

	K_SPINLOCK(&obs_slock) {
		const int limit = chan->data->observers_end_idx;

		for (int16_t i = chan->data->observers_start_idx; i < limit; ++i) {
			STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);
			STRUCT_SECTION_GET(zbus_channel_observation_mask, i, &observation_mask);

			__ASSERT(observation != NULL, "observation must be not NULL");

			if (observation->obs == obs) {
				*masked = observation_mask->enabled;

				err = 0;

				K_SPINLOCK_BREAK;
			}
		}
	}

	return err;
}

int zbus_obs_set_enable(const struct zbus_observer *obs, bool enabled)
{
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	K_SPINLOCK(&obs_slock) {
		if (obs->data->enabled != enabled) {
			obs->data->enabled = enabled;

			update_all_channels_hop(obs);
		}
	}

	return 0;
}
