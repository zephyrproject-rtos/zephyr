/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header with definitions for eSPI drivers callback functions
 */

#ifndef ZEPHYR_DRIVERS_ESPI_UTILS_H_
#define ZEPHYR_DRIVERS_ESPI_UTILS_H_

/**
 * @brief Generic function to insert or remove a callback from a callback list.
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL).
 * @param callback A pointer of the callback to insert or remove from the list.
 * @param set A boolean indicating insertion or removal of the callback.
 *
 * @return 0 on success, negative errno otherwise.
 */
static int espi_manage_callback(sys_slist_t *callbacks,
				struct espi_callback *callback, bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}

/**
 * @brief Generic function to go through and fire callback from a callback list.
 *
 * @param list A pointer on the espi callback list.
 * @param device A pointer on the espi driver instance.
 * @param pins The details on the event that triggered the callback.
 */
static inline void espi_send_callbacks(sys_slist_t *list,
				       const struct device *device,
				       struct espi_event evt)
{
	struct espi_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->evt_type & evt.evt_type) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(device, cb, evt);
		}
	}
}

#endif /* ZEPHYR_DRIVERS_ESPI_UTILS_H_ */
