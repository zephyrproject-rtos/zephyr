/*
 * Copyright (c) 2026 Simon Maurer <mail@maurer.systems>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup remoteproc_interface
 * @brief Header file for the RemoteProc (remote side) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_H_
#define ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_H_

/**
 * @brief Interfaces for RemoteProc resource table and VirtIO devices
 * @defgroup remoteproc_interface RemoteProc remote side API
 * @version 0.0.1
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for OpenAMP resource structures */
struct fw_rsc_carveout;
struct fw_rsc_vdev;

#ifdef CONFIG_OPENAMP_RPMSG_VDEV
struct rpmsg_device;
#endif

/**
 * @brief Get pointer to the OpenAMP resource table
 *
 * Returns a pointer to the resource table included in the firmware image.
 * The resource table describes shared memory regions, virtio devices,
 * and other resources used for inter-processor communication.
 *
 * @param size Pointer to a variable that will be filled with the size
 *             of the resource table in bytes.
 *
 * @return Pointer to the resource table, or NULL if not available.
 */
void *z_rproc_rsc_table(size_t *size);

/**
 * @brief Get carveout resource by name
 *
 * Searches the resource table for a carveout entry with the given name.
 *
 * @param name Null-terminated string identifying the carveout.
 *
 * @return Pointer to the carveout resource, or NULL if not found.
 */
struct fw_rsc_carveout *z_rproc_get_carveout_by_name(const char *name);

/**
 * @brief Get carveout resource by index
 *
 * Retrieves a carveout entry from the resource table by index.
 *
 * @param idx Index of the carveout entry.
 *
 * @return Pointer to the carveout resource, or NULL if the index
 *         is out of range.
 */
struct fw_rsc_carveout *z_rproc_get_carveout_by_index(unsigned int idx);

/**
 * @brief Get VirtIO device resource
 *
 * Retrieves a VirtIO device entry from the resource table.
 *
 * @param idx Index of the VirtIO device entry.
 *
 * @return Pointer to the vdev resource, or NULL if the index
 *         is out of range.
 */
struct fw_rsc_vdev *z_rproc_get_vdev(unsigned int idx);

#ifdef CONFIG_OPENAMP_RPMSG_VDEV

/**
 * @brief Get RPMsg device associated with a RemoteProc instance
 *
 * Returns the RPMsg device corresponding to the given Zephyr device.
 *
 * @param dev Pointer to the Zephyr device instance.
 *
 * @return Pointer to the RPMsg device, or NULL if not available.
 */
struct rpmsg_device *z_rproc_get_rpmsg_device(const struct device *dev);

#endif /* CONFIG_OPENAMP_RPMSG_VDEV */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_H_ */
