/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_
#define ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/sys/slist.h>

/**
 * @brief Generic function to insert a callback to a callback list
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL)
 * @param callback A pointer of the callback to insert to the list
 *
 * @return 0 on success, negative errno otherwise.
 */
static inline int smbus_callback_set(sys_slist_t *callbacks,
				     struct smbus_callback *callback)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		sys_slist_find_and_remove(callbacks, &callback->node);
	}

	sys_slist_prepend(callbacks, &callback->node);

	return 0;
}

/**
 * @brief Generic function to remove a callback from a callback list
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL)
 * @param callback A pointer of the callback to remove from the list
 *
 * @return 0 on success, negative errno otherwise.
 */
static inline int smbus_callback_remove(sys_slist_t *callbacks,
					struct smbus_callback *callback)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (sys_slist_is_empty(callbacks) ||
	    !sys_slist_find_and_remove(callbacks, &callback->node)) {
		return -ENOENT;
	}

	return 0;
}

/**
 * @brief Generic function to go through and fire callback from a callback list
 *
 * @param list A pointer on the SMBus callback list
 * @param dev A pointer on the SMBus device instance
 * @param addr A SMBus peripheral device address.
 */
static inline void smbus_fire_callbacks(sys_slist_t *list,
					const struct device *dev,
					uint8_t addr)
{
	struct smbus_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->addr == addr) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(dev, cb, addr);
		}
	}
}

/**
 * @brief Helper to initialize a struct smbus_callback properly
 *
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param addr A SMBus peripheral device address.
 */
static inline void smbus_init_callback(struct smbus_callback *callback,
				       smbus_callback_handler_t handler,
				       uint8_t addr)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->addr = addr;
}

/**
 * @brief Helper for handling an SMB alert
 *
 * This loops through all devices which triggered the SMB alert and
 * fires the callbacks.
 *
 * @param dev SMBus device
 * @param callbacks list of SMB alert callbacks
 */
void smbus_loop_alert_devices(const struct device *dev, sys_slist_t *callbacks);

#endif /*  ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_ */
