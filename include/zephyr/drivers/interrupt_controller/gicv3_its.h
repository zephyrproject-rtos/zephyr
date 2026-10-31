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
 * @brief Callback type for allocating an INTID.
 *
 * @param dev ITS device instance.
 *
 * @return The allocated INTID.
 */
typedef unsigned int (*its_api_alloc_intid_t)(const struct device *dev);

/**
 * @brief Callback type for setting up a device ID.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param nites Number of ITEs to allocate for the device.
 *
 * @return 0 on success, negative error code on failure.
 */
typedef int (*its_api_setup_deviceid_t)(const struct device *dev, uint32_t device_id,
					unsigned int nites);

/**
 * @brief Callback type for mapping an EventID to an INTID.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param event_id Event ID to map.
 * @param intid INTID the EventID is mapped to.
 *
 * @return 0 on success, negative error code on failure.
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
 * @return 0 on success, negative error code on failure.
 */
typedef int (*its_api_send_int_t)(const struct device *dev, uint32_t device_id, uint32_t event_id);

/**
 * @brief Callback type for retrieving the 32-bit MSI target address.
 *
 * @param dev ITS device instance.
 *
 * @return The 32-bit MSI target address.
 */
typedef uint32_t (*its_api_get_msi_addr_t)(const struct device *dev);

/**
 * @brief Callback type for retrieving the 64-bit MSI target address.
 *
 * @param dev ITS device instance.
 *
 * @return The 64-bit MSI target address.
 */
typedef uint64_t (*its_api_get_msi_addr64_t)(const struct device *dev);

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
	/** @brief Get the 32-bit MSI target address. */
	its_api_get_msi_addr_t get_msi_addr;
	/**
	 * @brief Optional callback returning the 64-bit MSI target address.
	 *
	 * When not implemented, the 32-bit get_msi_addr() callback is used and
	 * its result is zero-extended.
	 */
	its_api_get_msi_addr64_t get_msi_addr64;
};

/**
 * @brief Allocate an INTID for an ITS device.
 *
 * @param dev ITS device instance.
 *
 * @return The allocated INTID.
 */
static inline int its_alloc_intid(const struct device *dev)
{
	return DEVICE_API_GET(its, dev)->alloc_intid(dev);
}

/**
 * @brief Set up a device ID for an ITS device.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param nites Number of ITEs to allocate for the device.
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int its_setup_deviceid(const struct device *dev, uint32_t device_id,
				     unsigned int nites)
{
	return DEVICE_API_GET(its, dev)->setup_deviceid(dev, device_id, nites);
}

/**
 * @brief Map an EventID to an INTID.
 *
 * @param dev ITS device instance.
 * @param device_id Device ID of the endpoint.
 * @param event_id Event ID to map.
 * @param intid INTID the EventID is mapped to.
 *
 * @return 0 on success, negative error code on failure.
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
 * @return 0 on success, negative error code on failure.
 */
static inline int its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	return DEVICE_API_GET(its, dev)->send_int(dev, device_id, event_id);
}

/**
 * @brief Get the 32-bit MSI target address for an ITS device.
 *
 * Requires the get_msi_addr callback. Providers that implement only
 * get_msi_addr64() must use its_get_msi_addr64().
 *
 * @param dev ITS device instance.
 *
 * @return The 32-bit MSI target address.
 */
static inline uint32_t its_get_msi_addr(const struct device *dev)
{
	return DEVICE_API_GET(its, dev)->get_msi_addr(dev);
}

/**
 * @brief Get the 64-bit MSI target address for an ITS device.
 *
 * Uses the optional 64-bit callback when implemented; otherwise the 32-bit
 * address returned by its_get_msi_addr() is zero-extended.
 *
 * @param dev ITS device instance.
 *
 * @return The 64-bit MSI target address.
 */
static inline uint64_t its_get_msi_addr64(const struct device *dev)
{
	const struct its_driver_api *api = DEVICE_API_GET(its, dev);

	if (api->get_msi_addr64 != NULL) {
		return api->get_msi_addr64(dev);
	}

	return (uint64_t)api->get_msi_addr(dev);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_ */
