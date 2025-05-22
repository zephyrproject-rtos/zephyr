/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

int zbus_chan_add_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		      struct zbus_observer_node *node, k_timeout_t timeout)
{
	int err;
	struct zbus_observer_node *obs_nd, *tmp;
	struct zbus_channel_observation *observation;

	_ZBUS_ASSERT(!k_is_in_isr(), "ISR blocked");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(obs != NULL, "obs is required");
	_ZBUS_ASSERT(node != NULL, "node is required");

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

	node->obs = obs;

	sys_slist_append(&chan->data->observers, &node->node);

	k_sem_give(&chan->data->sem);

	return 0;
}

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

			k_sem_give(&chan->data->sem);

			return 0;
		}

		prev_obs_nd = obs_nd;
	}

	k_sem_give(&chan->data->sem);

	return -ENODATA;
}
