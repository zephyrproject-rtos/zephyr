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

#ifndef ZEPHYR_INCLUDE_DRIVERS_GICV3_ITS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GICV3_ITS_H_

typedef unsigned int (*its_api_alloc_intid_t)(const struct device *dev);
typedef int (*its_api_setup_deviceid_t)(const struct device *dev, uint32_t device_id,
					unsigned int nites);
typedef int (*its_api_map_intid_t)(const struct device *dev, uint32_t device_id,
				   uint32_t event_id, unsigned int intid);
typedef int (*its_api_send_int_t)(const struct device *dev, uint32_t device_id, uint32_t event_id);
typedef uint32_t (*its_api_get_msi_addr_t)(const struct device *dev);

__subsystem struct its_driver_api {
	its_api_alloc_intid_t alloc_intid;
	its_api_setup_deviceid_t setup_deviceid;
	its_api_map_intid_t map_intid;
	its_api_send_int_t send_int;
	its_api_get_msi_addr_t get_msi_addr;
};

static inline int its_alloc_intid(const struct device *dev)
{
	const struct its_driver_api *api =
		(const struct its_driver_api *)dev->api;

	return api->alloc_intid(dev);
}

static inline int its_setup_deviceid(const struct device *dev, uint32_t device_id,
				     unsigned int nites)
{
	const struct its_driver_api *api =
		(const struct its_driver_api *)dev->api;

	return api->setup_deviceid(dev, device_id, nites);
}

static inline int its_map_intid(const struct device *dev, uint32_t device_id,
				uint32_t event_id, unsigned int intid)
{
	const struct its_driver_api *api =
		(const struct its_driver_api *)dev->api;

	return api->map_intid(dev, device_id, event_id, intid);
}

static inline int its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	const struct its_driver_api *api =
		(const struct its_driver_api *)dev->api;

	return api->send_int(dev, device_id, event_id);
}

static inline uint32_t its_get_msi_addr(const struct device *dev)
{
	const struct its_driver_api *api =
		(const struct its_driver_api *)dev->api;

	return api->get_msi_addr(dev);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_GICV3_ITS_H_ */
