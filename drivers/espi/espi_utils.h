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
static inline int espi_manage_callback(sys_slist_t *callbacks,
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
	} else if (!set) {
		return -EINVAL;
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
 * @param dev A pointer on the espi driver instance.
 * @param pins The details on the event that triggered the callback.
 */
static inline void espi_send_callbacks(sys_slist_t *list,
				       const struct device *dev,
				       struct espi_event evt)
{
	struct espi_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->evt_type & evt.evt_type) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(dev, cb, evt);
		}
	}
}

/**
 * @brief Convert a timeout into microseconds for a busy wait with WAIT_FOR().
 *
 * A relative timeout is rounded up to the next microsecond. An absolute timeout is converted
 * into the time left until it expires. The result is limited to the range of the 32-bit cycle
 * counter used by WAIT_FOR().
 *
 * @param timeout Timeout other than K_FOREVER.
 *
 * @return Time to wait in microseconds, 0 for K_NO_WAIT or an expired absolute timeout.
 */
static inline uint32_t espi_timeout_to_us(k_timeout_t timeout)
{
	uint64_t max_us = MIN(k_cyc_to_us_floor64(UINT32_MAX), UINT32_MAX);

	if (!Z_IS_TIMEOUT_RELATIVE(timeout)) {
		timeout = sys_timepoint_timeout(sys_timepoint_calc(timeout));
	}

	return (uint32_t)MIN(k_ticks_to_us_ceil64(timeout.ticks), max_us);
}

/**
 * @brief Propagate eSPI driver interrupt control changes to eSPI host-specific implementations
 *
 * @param espi_flags the bitmap to select generic eSPI interrupt events.
 * @param espi_vendor_flags the bitmap to select any vendor-specific eSPI interrupt events.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by eSPI target.
 */
int espi_host_interrupt_config(uint32_t espi_flags, uint32_t espi_vendor_flags);

#endif /* ZEPHYR_DRIVERS_ESPI_UTILS_H_ */
