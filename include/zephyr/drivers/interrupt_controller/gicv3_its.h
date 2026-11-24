/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for ARM Generic Interrupt Controller V3 Interrupt Translation Service
 * @ingroup gicv3_its_interface
 *
 * The Generic Interrupt Controller (GIC) Interrupt Translation Service translates an input
 * EventID from a device, identified by its DeviceID, determines a corresponding INTID for
 * this input and the target Redistributor and, through this, the target PE for that INTID.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_

/**
 * @defgroup gicv3_its_interface GICv3 Interrupt Translation Service
 * @ingroup io_interfaces
 * @since 3.0
 * @version 0.1.0
 * @{
 */

/**
 * @def_driverbackendgroup{GICv3 ITS,gicv3_its_interface}
 * @{
 */

/**
 * @brief Callback type for allocating an LPI INTID.
 *
 * @param dev ITS device instance.
 *
 * @return The allocated LPI INTID (at least 8192).
 * @retval 0 No LPI INTIDs remain in the provisioned range or the ITS device is not ready.
 */
typedef unsigned int (*its_api_alloc_intid_t)(const struct device *dev);

/**
 * @brief Allocate and map consecutive LPI INTIDs to EventIDs starting at zero.
 *
 * Call its_setup_deviceid() before mapping the EventIDs.
 *
 * When rollback cannot be confirmed, potentially mapped INTIDs and EventIDs
 * remain reserved. Retained INTIDs reduce later allocation capacity, and
 * retrying the same EventIDs returns -EBUSY. If a rollback command fails, the
 * original mapping error is returned unless releasing unused INTIDs fails.
 *
 * @param dev ITS device.
 * @param device_id DeviceID whose EventIDs are mapped.
 * @param count Number of INTIDs to allocate and map.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -ENODEV
 *     - The ITS device is not ready.
 *     - LPIs are unavailable on the calling CPU.
 * @retval -ENOTSUP Cache maintenance is unavailable for non-coherent ITS memory.
 * @retval -EINVAL @p intid is NULL, @p count is zero or exceeds
 *                 @c CONFIG_GIC_V3_ITS_MAX_VECTORS, the DeviceID has not been set up,
 *                 or a DeviceID or EventID is out of range.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 * @retval -EBUSY
 *     - An EventID is reserved or already mapped.
 *     - Device setup or Collection mapping is in progress.
 *     - The ITS command queue is full.
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
 * @retval -ENODEV The ITS device is not ready.
 * @retval -EINVAL @p intid is NULL or @p count is zero.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 */
typedef int (*its_api_alloc_intids_t)(const struct device *dev, unsigned int count,
				      unsigned int *intid);

/**
 * @brief Callback type for setting up a device ID.
 * @copydetails its_setup_deviceid
 */
typedef int (*its_api_setup_deviceid_t)(const struct device *dev, uint32_t device_id,
					unsigned int nites);

/**
 * @brief Callback type for mapping an EventID to an LPI INTID.
 *
 * An EventID can be mapped once for each DeviceID. If cleanup of a failed
 * mapping cannot be confirmed, the EventID stays reserved and later mapping
 * requests return -EBUSY.
 *
 * @param dev ITS device.
 * @param device_id DeviceID that raises the EventID.
 * @param event_id EventID to map.
 * @param intid LPI INTID that receives the interrupt.
 * @retval 0 The EventID is mapped to @p intid.
 * @retval -ENODEV
 *     - The ITS device is not ready.
 *     - LPIs are unavailable on the calling CPU.
 * @retval -ENOTSUP Cache maintenance is unavailable for non-coherent ITS memory.
 * @retval -EINVAL The DeviceID has not been set up, or the DeviceID, EventID or INTID
 *                 is out of range.
 * @retval -EBUSY
 *     - The EventID is reserved or already mapped.
 *     - Device setup or Collection mapping is in progress.
 *     - The ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 *
 * A failed DeviceID setup can return its saved error.
 * Other negative error codes are propagated from the ITS command path.
 */
typedef int (*its_api_map_intid_t)(const struct device *dev, uint32_t device_id,
				   uint32_t event_id, unsigned int intid);

/**
 * @brief Callback type for sending an interrupt.
 * @copydetails its_send_int
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
 * @driver_ops{GICv3 ITS}
 */
__subsystem struct its_driver_api {
	/** @driver_ops_mandatory @copybrief its_alloc_intid */
	its_api_alloc_intid_t alloc_intid;
	/** @driver_ops_mandatory @copybrief its_setup_deviceid */
	its_api_setup_deviceid_t setup_deviceid;
	/** @driver_ops_mandatory @copybrief its_map_intid */
	its_api_map_intid_t map_intid;
	/** @driver_ops_mandatory @copybrief its_send_int */
	its_api_send_int_t send_int;
	/** @driver_ops_mandatory @copybrief its_get_msi_addr */
	its_api_get_msi_addr_t get_msi_addr;
	/** @driver_ops_mandatory @copybrief its_alloc_intids */
	its_api_alloc_intids_t alloc_intids;
	/** @driver_ops_mandatory @copybrief its_alloc_map_intids */
	its_api_alloc_map_intids_t alloc_map_intids;
};

/** @} */

/**
 * @brief Allocate an LPI INTID.
 *
 * @param dev ITS device.
 *
 * @return The allocated LPI INTID (at least 8192).
 * @retval 0 No LPI INTIDs remain in the provisioned range or the ITS device is not ready.
 */
static inline int its_alloc_intid(const struct device *dev)
{
	return DEVICE_API_GET(its, dev)->alloc_intid(dev);
}

/**
 * @brief Allocate consecutive LPI INTIDs.
 *
 * @param dev ITS device.
 * @param count Number of INTIDs to allocate.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -ENODEV The ITS device is not ready.
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
 * retrying the same EventIDs returns -EBUSY. If a rollback command fails, the
 * original mapping error is returned unless releasing unused INTIDs fails.
 *
 * @param dev ITS device.
 * @param device_id DeviceID whose EventIDs are mapped.
 * @param count Number of INTIDs to allocate and map.
 * @param[out] intid Where to store the first allocated LPI INTID (at least 8192).
 *                   Not modified on failure.
 * @retval 0 On success.
 * @retval -ENODEV
 *     - The ITS device is not ready.
 *     - LPIs are unavailable on the calling CPU.
 * @retval -ENOTSUP Cache maintenance is unavailable for non-coherent ITS memory.
 * @retval -EINVAL @p intid is NULL, @p count is zero or exceeds
 *                 @c CONFIG_GIC_V3_ITS_MAX_VECTORS, the DeviceID has not been set up,
 *                 or a DeviceID or EventID is out of range.
 * @retval -ENOSPC Not enough consecutive LPI INTIDs remain.
 * @retval -EBUSY
 *     - An EventID is reserved or already mapped.
 *     - Device setup or Collection mapping is in progress.
 *     - The ITS command queue is full.
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
 * failures. The device slot remains reserved if cleanup cannot be confirmed
 * or cache maintenance for the reclaimed ITT fails. Later calls for the same DeviceID
 * return the saved -ETIMEDOUT while the slot is reserved.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param nites Minimum number of usable EventIDs, starting at zero.
 *
 * @retval 0 Device ID set up.
 * @retval -EINVAL The DeviceID is out of range or @p nites is zero.
 * @retval -ENODEV The ITS device is not ready.
 * @retval -ENOTSUP
 *     - The hardware cannot represent the requested EventID count.
 *     - Cache maintenance is unavailable for non-coherent ITS memory.
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
 * An EventID can be mapped once for each DeviceID. If cleanup of a failed
 * mapping cannot be confirmed, the EventID stays reserved and later mapping
 * requests return -EBUSY.
 *
 * @param dev ITS device.
 * @param device_id DeviceID that raises the EventID.
 * @param event_id EventID to map.
 * @param intid LPI INTID that receives the interrupt.
 * @retval 0 The EventID is mapped to @p intid.
 * @retval -ENODEV
 *     - The ITS device is not ready.
 *     - LPIs are unavailable on the calling CPU.
 * @retval -ENOTSUP Cache maintenance is unavailable for non-coherent ITS memory.
 * @retval -EINVAL The DeviceID has not been set up, or the DeviceID, EventID or INTID
 *                 is out of range.
 * @retval -EBUSY
 *     - The EventID is reserved or already mapped.
 *     - Device setup or Collection mapping is in progress.
 *     - The ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 *
 * A failed DeviceID setup can return its saved error.
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
 * @retval -ENODEV The ITS device is not ready.
 * @retval -EINVAL
 *     - The DeviceID or EventID is out of range.
 *     - The DeviceID has not been set up.
 *     - The EventID is unmapped.
 * @retval -ENOTSUP Cache maintenance is unavailable for non-coherent ITS memory.
 * @retval -EBUSY
 *     - Device setup is in progress.
 *     - EventID mapping or rollback is unconfirmed.
 *     - The ITS command queue is full.
 * @retval -ETIMEDOUT ITS command processing timed out.
 *
 * A failed DeviceID setup can return its saved error.
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

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_ */
