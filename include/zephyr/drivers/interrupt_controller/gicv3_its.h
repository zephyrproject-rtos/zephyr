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
 * @defgroup gicv3_its GICv3 Interrupt Translation Service
 * @ingroup io_interfaces
 */

/**
 * @brief First INTID in the LPI range.
 * @ingroup gicv3_its
 */
#define GIC_LPI_INT_BASE 8192U

/**
 * @brief No LPI INTID is available for allocation.
 * @ingroup gicv3_its
 */
#define ITS_INTID_INVALID 0U

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

/**
 * @brief Allocate an LPI INTID.
 * @ingroup gicv3_its
 *
 * INTIDs are shared by all ITS instances and cannot be freed. The range is
 * limited by the GIC, the LPI tables and @kconfig{CONFIG_NUM_IRQS}.
 *
 * @param dev ITS device instance. Must not be @c NULL.
 *
 * @return Allocated LPI INTID on success.
 * @retval ITS_INTID_INVALID No LPI INTIDs remain.
 */
static inline int its_alloc_intid(const struct device *dev)
{
	return DEVICE_API_GET(its, dev)->alloc_intid(dev);
}

static inline int its_setup_deviceid(const struct device *dev, uint32_t device_id,
				     unsigned int nites)
{
	return DEVICE_API_GET(its, dev)->setup_deviceid(dev, device_id, nites);
}

static inline int its_map_intid(const struct device *dev, uint32_t device_id,
				uint32_t event_id, unsigned int intid)
{
	return DEVICE_API_GET(its, dev)->map_intid(dev, device_id, event_id, intid);
}

static inline int its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	return DEVICE_API_GET(its, dev)->send_int(dev, device_id, event_id);
}

static inline uint32_t its_get_msi_addr(const struct device *dev)
{
	return DEVICE_API_GET(its, dev)->get_msi_addr(dev);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GICV3_ITS_H_ */
