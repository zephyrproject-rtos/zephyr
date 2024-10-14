/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

int zbus_chan_add_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		      k_timeout_t timeout)
{
	int err;
	struct zbus_channel_observation *observation;
	sys_snode_t *prev = NULL;

	_ZBUS_ASSERT(!k_is_in_isr(), "ISR blocked");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	/* Add link back to parent structure */
	obs->data->obs = obs;

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
	if (sys_slist_find(&chan->data->observers, &obs->data->node, &prev)) {
		k_sem_give(&chan->data->sem);
		return -EALREADY;
	}

	sys_slist_append(&chan->data->observers, &obs->data->node);

	k_sem_give(&chan->data->sem);

	return 0;
}

int zbus_chan_rm_obs(const struct zbus_channel *chan, const struct zbus_observer *obs,
		     k_timeout_t timeout)
{
	bool removed;
	int err;

	_ZBUS_ASSERT(!k_is_in_isr(), "ISR blocked");
	_ZBUS_ASSERT(chan != NULL, "chan is required");
	_ZBUS_ASSERT(obs != NULL, "obs is required");

	err = k_sem_take(&chan->data->sem, timeout);
	if (err) {
		return err;
	}

	removed = sys_slist_find_and_remove(&chan->data->observers, &obs->data->node);

	k_sem_give(&chan->data->sem);

	return removed ? 0 : -ENODATA;
}
