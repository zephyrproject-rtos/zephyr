/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DEVICE_STRUCTS_H_
#define ZEPHYR_INCLUDE_DEVICE_STRUCTS_H_

/**
 * @brief Device Model Structures
 * @defgroup device_model Device Model Structures
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Type used to represent a "handle" for a device.
 *
 * Every struct device has an associated handle. You can get a pointer
 * to a device structure from its handle and vice versa, but the
 * handle uses less space than a pointer. The device.h API mainly uses
 * handles to store lists of multiple devices in a compact way.
 *
 * The extreme values and zero have special significance. Negative
 * values identify functionality that does not correspond to a Zephyr
 * device, such as the system clock or a SYS_INIT() function.
 *
 * @see device_handle_get()
 * @see device_from_handle()
 */
typedef int16_t device_handle_t;

/**
 * @brief Runtime device dynamic structure (in RAM) per driver instance
 *
 * Fields in this are expected to be default-initialized to zero. The
 * kernel driver infrastructure and driver access functions are
 * responsible for ensuring that any non-zero initialization is done
 * before they are accessed.
 */
struct device_state {
	/** Non-negative result of initializing the device.
	 *
	 * The absolute value returned when the device initialization
	 * function was invoked, or `UINT8_MAX` if the value exceeds
	 * an 8-bit integer. If initialized is also set, a zero value
	 * indicates initialization succeeded.
	 */
	unsigned int init_res : 8;

	/** Indicates the device initialization function has been
	 * invoked.
	 */
	bool initialized : 1;
};

struct pm_device;

/**
 * @brief Runtime device structure (in ROM) per driver instance
 */
struct device {
	/** Name of the device instance */
	const char *name;
	/** Address of device instance config information */
	const void *config;
	/** Address of the API structure exposed by the device instance */
	const void *api;
	/** Address of the common device state */
	struct device_state * const state;
	/** Address of the device instance private data */
	void * const data;
	/** optional pointer to handles associated with the device.
	 *
	 * This encodes a sequence of sets of device handles that have
	 * some relationship to this node. The individual sets are
	 * extracted with dedicated API, such as
	 * device_required_handles_get().
	 */
	const device_handle_t *const handles;
#ifdef CONFIG_PM_DEVICE
	/** Reference to the device PM resources. */
	struct pm_device * const pm;
#endif
};

#endif /* ZEPHYR_INCLUDE_DEVICE_STRUCTS_H */
