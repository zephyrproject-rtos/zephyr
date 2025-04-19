/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_DYNAMIC)
static inline int _zbus_runtime_observer_node_alloc(struct zbus_observer_node **node,
						    k_timeout_t timeout)
{
	ARG_UNUSED(timeout);

	*node = k_malloc(sizeof(struct zbus_observer_node));

	_ZBUS_ASSERT(*node != NULL, "could not allocate observer node the heap is full!");

	if (*node == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static inline void _zbus_runtime_observer_node_free(struct zbus_observer_node *node)
{
	k_free(node);
}

#elif defined(CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_STATIC)

K_MEM_SLAB_DEFINE_STATIC(_zbus_runtime_observers_slab, sizeof(struct zbus_observer_node),
			 CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_POOL_SIZE, 8);

static inline int _zbus_runtime_observer_node_alloc(struct zbus_observer_node **node,
						    k_timeout_t timeout)
{
	int err = k_mem_slab_alloc(&_zbus_runtime_observers_slab, (void **)node, timeout);

	_ZBUS_ASSERT(*node != NULL, "not enough runtime observer nodes in the pool. Increase the "
				    "ZBUS_RUNTIME_OBSERVERS_NODE_POOL_SIZE");

	return err;
}

static inline void _zbus_runtime_observer_node_free(struct zbus_observer_node *node)
{
	k_mem_slab_free(&_zbus_runtime_observers_slab, (void *)node);
}

#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_DYNAMIC */

static inline int _zbus_runtime_take_chan_sem_and_obs_check(const struct zbus_channel *chan,
							    const struct zbus_observer *obs,
							    k_timeout_t timeout)
{
	int err;

	struct zbus_observer_node *obs_nd, *tmp;
	struct zbus_channel_observation *observation;

	_ZBUS_ASSERT(!k_is_in_isr(), "ISR blocked");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	err = k_sem_take(&chan->data->sem, timeout);
	if (err) {
		return err;
	}

	for (int16_t i = chan->data->observers_start_idx, limit = chan->data->observers_end_idx;
	     i < limit; ++i) {
		STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

		__ASSERT(observation != NULL, "observation must be not NULL");

		if (observation->obs == obs) {
			k_sem_give(&chan->data->sem);

			return -EEXIST;
		}
	}

	/* Check if the observer is already a runtime observer */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&chan->data->observers, obs_nd, tmp, node) {
		if (obs_nd->obs == obs) {
			k_sem_give(&chan->data->sem);

			return -EALREADY;
		}
	}

	return 0;
}

#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE)
int zbus_chan_add_obs_with_node(const struct zbus_channel *chan, const struct zbus_observer *obs,
				struct zbus_observer_node *node, k_timeout_t timeout)
{
	int err;

	/* On success the channel semaphore has been taken */
	err = _zbus_runtime_take_chan_sem_and_obs_check(chan, obs, timeout);
	if (err) {
		return err;
	}

	if (node->chan != NULL) {
		k_sem_give(&chan->data->sem);

		return -EBUSY;
	}

	node->obs = obs;
	node->chan = chan;

	sys_slist_append(&chan->data->observers, &node->node);

	k_sem_give(&chan->data->sem);

	return 0;
}

#else

int zbus_chan_add_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		      k_timeout_t timeout)
{
	int err;
	k_timepoint_t end_time = sys_timepoint_calc(timeout);

	/* On success the channel semaphore has been taken */
	err = _zbus_runtime_take_chan_sem_and_obs_check(chan, obs, timeout);
	if (err) {
		return err;
	}

	struct zbus_observer_node *new_obs_nd = NULL;

	err = _zbus_runtime_observer_node_alloc(&new_obs_nd, sys_timepoint_timeout(end_time));
	if (err) {
		k_sem_give(&chan->data->sem);

		return err;
	}

	new_obs_nd->obs = obs;

	sys_slist_append(&chan->data->observers, &new_obs_nd->node);

	k_sem_give(&chan->data->sem);

	return 0;
}
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE */

int zbus_chan_rm_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		     k_timeout_t timeout)
{
	int err;
	struct zbus_observer_node *obs_nd, *tmp;
	struct zbus_observer_node *prev_obs_nd = NULL;

	_ZBUS_ASSERT(!k_is_in_isr(), "ISR blocked");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	err = k_sem_take(&chan->data->sem, timeout);
	if (err) {
		return err;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&chan->data->observers, obs_nd, tmp, node) {
		if (obs_nd->obs == obs) {
			sys_slist_remove(&chan->data->observers, &prev_obs_nd->node, &obs_nd->node);
#if defined(CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE)
			obs_nd->chan = NULL;
#else
			_zbus_runtime_observer_node_free(obs_nd);
#endif

			k_sem_give(&chan->data->sem);

			return 0;
		}

		prev_obs_nd = obs_nd;
	}

	k_sem_give(&chan->data->sem);

	return -ENODATA;
}
