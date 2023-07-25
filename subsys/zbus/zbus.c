/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_REGISTER(zbus, CONFIG_ZBUS_LOG_LEVEL);

k_timeout_t _zbus_timeout_remainder(uint64_t end_ticks)
{
	int64_t now_ticks = sys_clock_tick_get();

	return K_TICKS((k_ticks_t)MAX(end_ticks - now_ticks, 0));
}

#if (CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE > 0)
static inline void _zbus_notify_runtime_listeners(const struct zbus_channel *chan)
{
	__ASSERT(chan != NULL, "chan is required");

	struct zbus_observer_node *obs_nd, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(chan->runtime_observers, obs_nd, tmp, node) {

		__ASSERT(obs_nd != NULL, "observer node is NULL");

		if (obs_nd->obs->enabled && (obs_nd->obs->callback != NULL)) {
			obs_nd->obs->callback(chan);
		}
	}
}

static inline int _zbus_notify_runtime_subscribers(const struct zbus_channel *chan,
						   uint64_t end_ticks)
{
	__ASSERT(chan != NULL, "chan is required");

	int last_error = 0, err;
	struct zbus_observer_node *obs_nd, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(chan->runtime_observers, obs_nd, tmp, node) {

		__ASSERT(obs_nd != NULL, "observer node is NULL");

		if (obs_nd->obs->enabled && (obs_nd->obs->queue != NULL)) {
			err = k_msgq_put(obs_nd->obs->queue, &chan,
					 _zbus_timeout_remainder(end_ticks));

			_ZBUS_ASSERT(err == 0,
				     "could not deliver notification to observer %s. Error code %d",
				     _ZBUS_OBS_NAME(obs_nd->obs), err);

			if (err) {
				last_error = err;
			}
		}
	}

	return last_error;
}
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE */

static int _zbus_notify_observers(const struct zbus_channel *chan, uint64_t end_ticks)
{
	int last_error = 0, err;
	/* Notify static listeners */
	for (const struct zbus_observer *const *obs = chan->observers; *obs != NULL; ++obs) {
		if ((*obs)->enabled && ((*obs)->callback != NULL)) {
			(*obs)->callback(chan);
		}
	}

#if CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE > 0
	_zbus_notify_runtime_listeners(chan);
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE */

	/* Notify static subscribers */
	for (const struct zbus_observer *const *obs = chan->observers; *obs != NULL; ++obs) {
		if ((*obs)->enabled && ((*obs)->queue != NULL)) {
			err = k_msgq_put((*obs)->queue, &chan, _zbus_timeout_remainder(end_ticks));
			_ZBUS_ASSERT(err == 0, "could not deliver notification to observer %s.",
				     _ZBUS_OBS_NAME(*obs));
			if (err) {
				LOG_ERR("Observer %s at %p could not be notified. Error code %d",
					_ZBUS_OBS_NAME(*obs), *obs, err);
				last_error = err;
			}
		}
	}

#if CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE > 0
	err = _zbus_notify_runtime_subscribers(chan, end_ticks);
	if (err) {
		last_error = err;
	}
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE */
	return last_error;
}

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg, k_timeout_t timeout)
{
	int err;
	uint64_t end_ticks = sys_clock_timeout_end_calc(timeout);

	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");

	if (chan->validator != NULL && !chan->validator(msg, chan->message_size)) {
		return -ENOMSG;
	}

	err = k_mutex_lock(chan->mutex, timeout);
	if (err) {
		return err;
	}

	memcpy(chan->message, msg, chan->message_size);

	err = _zbus_notify_observers(chan, end_ticks);

	k_mutex_unlock(chan->mutex);

	return err;
}

int zbus_chan_read(const struct zbus_channel *chan, void *msg, k_timeout_t timeout)
{
	int err;

	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(msg != NULL, "msg is required");

	err = k_mutex_lock(chan->mutex, timeout);
	if (err) {
		return err;
	}

	memcpy(msg, chan->message, chan->message_size);

	return k_mutex_unlock(chan->mutex);
}

int zbus_chan_notify(const struct zbus_channel *chan, k_timeout_t timeout)
{
	int err;
	uint64_t end_ticks = sys_clock_timeout_end_calc(timeout);

	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	err = k_mutex_lock(chan->mutex, timeout);
	if (err) {
		return err;
	}

	err = _zbus_notify_observers(chan, end_ticks);

	k_mutex_unlock(chan->mutex);

	return err;
}

int zbus_chan_claim(const struct zbus_channel *chan, k_timeout_t timeout)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	int err = k_mutex_lock(chan->mutex, timeout);

	if (err) {
		return err;
	}

	return 0;
}

int zbus_chan_finish(const struct zbus_channel *chan)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "zbus cannot be used inside ISRs");
	_ZBUS_ASSERT(chan != NULL, "chan is required");

	int err = k_mutex_unlock(chan->mutex);

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
