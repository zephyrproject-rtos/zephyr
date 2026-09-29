/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for ARM Generic Interrupt Controller V3 Interrupt Translation Service
 *
 * The Generic Interrupt Controller (GIC) Interrupt Translation Service translates an input
 * EventID from a device, identified by its DeviceID, determines a corresponding INTID for
 * this input and the target Redistributor and, through this, the target PE for that INTID.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_

/**
 * @brief Allocate an LPI INTID.
 *
 * @param dev ITS device.
 * @param[out] intid Where to store the allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -EINVAL @p intid is NULL.
 * @retval -ENOSPC No LPI INTIDs remain in the provisioned range.
 */
typedef int (*its_api_alloc_intid_t)(const struct device *dev, unsigned int *intid);

/**
 * @brief Allocate and map consecutive LPI INTIDs to EventIDs starting at zero.
 *
 * Call its_setup_deviceid() before mapping the EventIDs.
 *
 * When rollback cannot be confirmed, potentially mapped INTIDs and EventIDs
 * remain reserved. Retained INTIDs reduce later allocation capacity, and
 * retrying the same EventIDs returns -EBUSY.
 *
 * @param dev ITS device.
 * @param device_id DeviceID whose EventIDs are mapped.
 * @param count Number of INTIDs to allocate and map.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -EINVAL @p intid is NULL, @p count is zero or exceeds
 *                 @c CONFIG_GIC_V3_ITS_MAX_VECTORS, the DeviceID has not been set up,
 *                 or a DeviceID or EventID is out of range.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 * @retval -EBUSY One of the EventIDs is reserved or already mapped.
 * @retval -ETIMEDOUT The ITS did not complete a command.
 * @retval -EIO The allocated INTIDs could not be released after a failure.
 *
 * Other negative values from the ITS command and device setup paths are propagated.
 */
typedef int (*its_api_alloc_map_intids_t)(const struct device *dev, uint32_t device_id,
					  unsigned int count, unsigned int *intid);

/**
 * @brief Allocate consecutive LPI INTIDs.
 *
 * @param dev ITS device.
 * @param count Number of INTIDs to allocate.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -EINVAL @p intid is NULL or @p count is zero.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 */
typedef int (*its_api_alloc_intids_t)(const struct device *dev, unsigned int count,
				      unsigned int *intid);

/**
 * @brief Callback type for setting up a device ID.
 *
 * ITT storage is reserved statically for @c CONFIG_GIC_V3_ITS_MAX_VECTORS entries
 * per device. Published indirect Device table pages remain reserved after setup
 * failures. If cleanup cannot be confirmed, the device slot also remains reserved.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param nites Minimum number of usable EventIDs, starting at zero.
 *
 * @retval 0 Device ID set up.
 * @retval -EINVAL The DeviceID is out of range or @p nites is zero.
 * @retval -ENODEV The ITS device is not ready.
 * @retval -ENOTSUP The hardware cannot represent the requested EventID count.
 * @retval -ENOMEM Static device, indirect-page or vector-table capacity is insufficient.
 * @retval -EBUSY Device setup is in progress or the ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 */
typedef int (*its_api_setup_deviceid_t)(const struct device *dev, uint32_t device_id,
					unsigned int nites);

/**
 * @brief Callback type for mapping an EventID to an LPI INTID.
 *
 * Mapping an EventID that is already mapped for the same DeviceID replaces its
 * translation with @p intid and keeps the reservation made by the previous
 * mapping. A failed remap leaves that reservation in place.
 *
 * An EventID that belongs to a range returned by its_alloc_map_intids() must
 * not be remapped on its own, because the range mapping promises consecutive
 * INTIDs.
 *
 * @param dev ITS device.
 * @param device_id DeviceID that raises the EventID.
 * @param event_id EventID to map.
 * @param intid LPI INTID that receives the interrupt.
 * @retval 0 The EventID is mapped to @p intid.
 * @retval -EINVAL The DeviceID has not been set up, or the DeviceID, EventID or INTID
 *                 is out of range.
 * @retval -EBUSY Device setup, mapping or rollback is in progress, or the command queue is full.
 *
 * Other negative error codes are propagated from the ITS command path.
 */
typedef int (*its_api_map_intid_t)(const struct device *dev, uint32_t device_id,
				   uint32_t event_id, unsigned int intid);

/**
 * @brief Callback type for sending an interrupt.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param event_id Event ID to trigger.
 *
 * @retval 0 Interrupt command completed.
 * @retval -EBUSY ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 */
typedef int (*its_api_send_int_t)(const struct device *dev, uint32_t device_id, uint32_t event_id);

/**
 * @brief Callback type for retrieving the physical MSI target address.
 *
 * @param dev ITS device instance.
 *
 * @return The physical MSI target address.
 */
typedef uint64_t (*its_api_get_msi_addr_t)(const struct device *dev);

/**
 * @brief ITS driver API.
 */
__subsystem struct its_driver_api {
	/** @brief Allocate an INTID. */
	its_api_alloc_intid_t alloc_intid;
	/** @brief Set up a device ID. */
	its_api_setup_deviceid_t setup_deviceid;
	/** @brief Map an EventID to an INTID. */
	its_api_map_intid_t map_intid;
	/** @brief Send an interrupt. */
	its_api_send_int_t send_int;
	/** @brief Get the physical MSI target address. */
	its_api_get_msi_addr_t get_msi_addr;
	/** @brief Allocate consecutive LPI INTIDs. */
	its_api_alloc_intids_t alloc_intids;
	/** @brief Allocate consecutive LPI INTIDs and map them to EventIDs. */
	its_api_alloc_map_intids_t alloc_map_intids;
};

/**
 * @brief Allocate an LPI INTID.
 *
 * @param dev ITS device.
 * @param[out] intid Where to store the allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -EINVAL @p intid is NULL.
 * @retval -ENOSPC No LPI INTIDs remain in the provisioned range.
 */
static inline int its_alloc_intid(const struct device *dev, unsigned int *intid)
{
	return DEVICE_API_GET(its, dev)->alloc_intid(dev, intid);
}

/**
 * @brief Allocate consecutive LPI INTIDs.
 *
 * @param dev ITS device.
 * @param count Number of INTIDs to allocate.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -EINVAL @p intid is NULL or @p count is zero.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 */
static inline int its_alloc_intids(const struct device *dev, unsigned int count,
				   unsigned int *intid)
{
	return DEVICE_API_GET(its, dev)->alloc_intids(dev, count, intid);
}

/**
 * @brief Allocate consecutive LPI INTIDs and map them to EventIDs.
 *
 * Call its_setup_deviceid() before mapping the EventIDs.
 *
 * When rollback cannot be confirmed, potentially mapped INTIDs and EventIDs
 * remain reserved. Retained INTIDs reduce later allocation capacity, and
 * retrying the same EventIDs returns -EBUSY.
 *
 * @param dev ITS device.
 * @param device_id DeviceID whose EventIDs are mapped.
 * @param count Number of INTIDs to allocate and map.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -EINVAL @p intid is NULL, @p count is zero or exceeds
 *                 @c CONFIG_GIC_V3_ITS_MAX_VECTORS, the DeviceID has not been set up,
 *                 or a DeviceID or EventID is out of range.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 * @retval -EBUSY One of the EventIDs is reserved or already mapped.
 * @retval -ETIMEDOUT The ITS did not complete a command.
 * @retval -EIO The allocated INTIDs could not be released after a failure.
 *
 * Other negative values from the ITS command and device setup paths are propagated.
 */
static inline int its_alloc_map_intids(const struct device *dev, uint32_t device_id,
				       unsigned int count, unsigned int *intid)
{
	return DEVICE_API_GET(its, dev)->alloc_map_intids(dev, device_id, count, intid);
}

/**
 * @brief Set up a device ID for an ITS device.
 *
 * ITT storage is reserved statically for @c CONFIG_GIC_V3_ITS_MAX_VECTORS entries
 * per device. Published indirect Device table pages remain reserved after setup
 * failures. If cleanup cannot be confirmed, the device slot also remains reserved.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param nites Minimum number of usable EventIDs, starting at zero.
 *
 * @retval 0 Device ID set up.
 * @retval -EINVAL The DeviceID is out of range or @p nites is zero.
 * @retval -ENODEV The ITS device is not ready.
 * @retval -ENOTSUP The hardware cannot represent the requested EventID count.
 * @retval -ENOMEM Static device, indirect-page or vector-table capacity is insufficient.
 * @retval -EBUSY Device setup is in progress or the ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 */
static inline int its_setup_deviceid(const struct device *dev, uint32_t device_id,
				     unsigned int nites)
{
	return DEVICE_API_GET(its, dev)->setup_deviceid(dev, device_id, nites);
}

/**
 * @brief Map an EventID to an LPI INTID.
 *
 * Mapping an EventID that is already mapped for the same DeviceID replaces its
 * translation with @p intid and keeps the reservation made by the previous
 * mapping. A failed remap leaves that reservation in place.
 *
 * An EventID that belongs to a range returned by its_alloc_map_intids() must
 * not be remapped on its own, because the range mapping promises consecutive
 * INTIDs.
 *
 * @param dev ITS device.
 * @param device_id DeviceID that raises the EventID.
 * @param event_id EventID to map.
 * @param intid LPI INTID that receives the interrupt.
 * @retval 0 The EventID is mapped to @p intid.
 * @retval -EINVAL The DeviceID has not been set up, or the DeviceID, EventID or INTID
 *                 is out of range.
 * @retval -EBUSY Device setup, mapping or rollback is in progress, or the command queue is full.
 *
 * Other negative error codes are propagated from the ITS command path.
 */
static inline int its_map_intid(const struct device *dev, uint32_t device_id,
				uint32_t event_id, unsigned int intid)
{
	return DEVICE_API_GET(its, dev)->map_intid(dev, device_id, event_id, intid);
}

/**
 * @brief Send an interrupt for an EventID.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param event_id Event ID to trigger.
 *
 * @retval 0 Interrupt command completed.
 * @retval -EBUSY ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 */
static inline int its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	return DEVICE_API_GET(its, dev)->send_int(dev, device_id, event_id);
}

/**
 * @brief Get the physical MSI target address for an ITS device.
 *
 * The return type is 64 bits wide on all targets. The address value may
 * still be limited by the provider's underlying address width.
 *
 * @param dev ITS device instance.
 *
 * @return The physical MSI target address.
 */
static inline uint64_t its_get_msi_addr(const struct device *dev)
{
	return DEVICE_API_GET(its, dev)->get_msi_addr(dev);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_ */
