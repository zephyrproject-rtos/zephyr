/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for coredump pseudo-device driver
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_COREDUMP_H_
#define INCLUDE_ZEPHYR_DRIVERS_COREDUMP_H_

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Coredump pseudo-device driver APIs
 * @defgroup coredump_device_interface Coredump pseudo-device driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Structure describing a region in memory that may be
 * stored in core dump at the time it is generated
 *
 * Instances of this are passed to the coredump_device_register_memory() and
 * coredump_device_unregister_memory() functions to indicate addition and removal
 * of memory regions to be captured
 */
struct coredump_mem_region_node {
	/** Node of single-linked list, do not modify */
	sys_snode_t node;

	/** Address of start of memory region */
	uintptr_t start;

	/** Size of memory region */
	size_t size;
};

/**
 * @brief Callback that occurs at dump time, data copied into dump_area will
 * be included in the dump that is generated
 *
 * @param dump_area      Pointer to area to copy data into for inclusion in dump
 * @param dump_area_size Size of available memory at dump_area
 */
typedef void (*coredump_dump_callback_t)(uintptr_t dump_area, size_t dump_area_size);

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/*
 * Type definition of coredump API function for adding specified
 * data into coredump
 */
typedef void (*coredump_device_dump_t)(const struct device *dev);

/*
 * Type definition of coredump API function for registering a memory
 * region
 */
typedef bool (*coredump_device_register_memory_t)(const struct device *dev,
	struct coredump_mem_region_node *region);

/*
 * Type definition of coredump API function for unregistering a memory
 * region
 */
typedef bool (*coredump_device_unregister_memory_t)(const struct device *dev,
	struct coredump_mem_region_node *region);

/*
 * Type definition of coredump API function for registering a dump
 * callback
 */
typedef bool (*coredump_device_register_callback_t)(const struct device *dev,
	coredump_dump_callback_t callback);

/*
 * API which a coredump pseudo-device driver should expose
 */
__subsystem struct coredump_driver_api {
	coredump_device_dump_t              dump;
	coredump_device_register_memory_t   register_memory;
	coredump_device_unregister_memory_t unregister_memory;
	coredump_device_register_callback_t register_callback;
};

/**
 * @endcond
 */

/**
 * @brief Register a region of memory to be stored in core dump at the
 * time it is generated
 *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param region Struct describing memory to be collected
 *
 * @return true if registration succeeded
 * @return false if registration failed
 */
static inline bool coredump_device_register_memory(const struct device *dev,
	struct coredump_mem_region_node *region)
{
	const struct coredump_driver_api *api =
		(const struct coredump_driver_api *)dev->api;

	return api->register_memory(dev, region);
}

/**
 * @brief Unregister a region of memory to be stored in core dump at the
 * time it is generated
 *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param region Struct describing memory to be collected
 *
 * @return true if unregistration succeeded
 * @return false if unregistration failed
 */
static inline bool coredump_device_unregister_memory(const struct device *dev,
	struct coredump_mem_region_node *region)
{
	const struct coredump_driver_api *api =
		(const struct coredump_driver_api *)dev->api;

	return api->unregister_memory(dev, region);
}

/**
 * @brief Register a callback to be invoked at dump time
 *
 * @param dev      Pointer to the device structure for the driver instance.
 * @param callback Callback to be invoked at dump time
 *
 * @return true if registration succeeded
 * @return false if registration failed
 */
static inline bool coredump_device_register_callback(const struct device *dev,
	coredump_dump_callback_t callback)
{
	const struct coredump_driver_api *api =
		(const struct coredump_driver_api *)dev->api;

	return api->register_callback(dev, callback);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DRIVERS_COREDUMP_H_ */
