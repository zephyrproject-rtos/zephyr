/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_
#define ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_

/**
 * @brief Generic function to insert or remove a callback from a callback list
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL)
 * @param callback A pointer of the callback to insert or remove from the list
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @return 0 on success, negative errno otherwise.
 */
static inline int smbus_manage_smbus_callback(sys_slist_t *callbacks,
					      struct smbus_callback *callback,
					      bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -ENOENT;
			}
		}
	} else {
		if (!set) {
			return -ENOENT;
		}
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
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


#endif /*  ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_ */
