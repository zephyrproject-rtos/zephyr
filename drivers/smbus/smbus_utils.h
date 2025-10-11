/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_
#define ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
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

/**
 * @brief Compute the number of messages required for an SMBus transaction
 *
 * If @p flags indicates that the transaction requires packet error checking (PEC),
 * the number of messages is equal to @p num_msgs, otherwise the number of messages
 * required is @p num_msgs - 1, since a PEC byte is not required.
 *
 * Callers are expected to allocated an array of @ref i2c_msg objects to hold a number of
 * i2c messages, including one message dedicated to the a PEC byte, whether or not PEC is
 * being used.
 *
 * @note This function is intended for SMBus drivers that do not have hardware PEC support and
 * therefore must rely on software PEC calculation.
 *
 * @param flags SMBus flags.
 * @param num_msgs Number of allocated messages.
 * @return The number of required messages.
 */
uint8_t smbus_pec_num_msgs(uint32_t flags, uint8_t num_msgs);

/**
 * @brief Compute the packet error checking (PEC) byte for an SMBus transaction
 *
 * @note This function is intended for SMBus drivers that do not have hardware PEC support and
 * therefore must rely on software PEC calculation.
 *
 * @note At this time, only 7-bit addresses are supported in @p addr.
 *
 * @param addr The address of the target device.
 * @param msgs Array of @ref i2c_msg that make up the transaction.
 * @param num_msgs Number of messages in the transaction.
 * @return the computed PEC byte.
 */
uint8_t smbus_pec(uint16_t addr, const struct i2c_msg *msgs, uint8_t num_msgs);

/**
 * @brief Prepare the packet error checking (PEC) byte for an SMBus write transaction
 *
 * If the @p flags bitmask contains @ref SMBUS_MODE_PEC (i.e. PEC is enabled), the number of
 * messages is equal to @p num_msgs, otherwise the number of messages required is @p num_msgs - 1,
 * since a PEC byte is not required.
 *
 * @note This function is intended for SMBus drivers that do not have hardware PEC support and
 * therefore must rely on software PEC calculation.
 *
 * @note At this time, only 7-bit addresses are supported in @p addr.
 *
 * @param flags SMBus flags.
 * @param addr The address of the target device.
 * @param messages Array of @ref i2c_msg objects that make up the transaction.
 * @param num_msgs Number of messages in the transaction.
 */
void smbus_write_prepare_pec(uint32_t flags, uint16_t addr, struct i2c_msg *msgs, uint8_t num_msgs);

/**
 * @brief Check the packet error checking (PEC) byte for an SMBus read transaction
 *
 * If the @p flags bitmask contains @ref SMBUS_MODE_PEC (i.e. PEC is enabled), the number of
 * messages is equal to @p num_msgs, otherwise the number of messages required is
 * @p num_msgs - 1, since a PEC byte is not required.
 *
 * When PEC is not enabled, this function returns 0.
 *
 * @note This function is intended for SMBus drivers that do not have hardware PEC support and
 * therefore must rely on software PEC calculation.
 *
 * @note At this time, only 7-bit addresses are supported in @p addr.
 *
 * @param flags SMBus flags.
 * @param addr The address of the target device.
 * @param messages Array of @ref i2c_msg objects that make up the transaction.
 * @param num_msgs Number of messages in the transaction.
 * @retval 0 on success.
 * @retval -EIO if the PEC byte does not match the computed value.
 */
int smbus_read_check_pec(uint32_t flags, uint16_t addr, const struct i2c_msg *msgs,
			 uint8_t num_msgs);

#endif /*  ZEPHYR_DRIVERS_SMBUS_SMBUS_UTILS_H_ */
