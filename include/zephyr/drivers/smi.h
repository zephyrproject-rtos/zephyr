/**
 * @file
 *
 * @brief Public APIs for SMI drivers.
 */

/*
 * Copyright 2026 Bas van Loon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SMI_H_
#define ZEPHYR_INCLUDE_DRIVERS_SMI_H_

/**
 * @brief SMI Interface
 * @defgroup smi_interface SMI Interface
 * @ingroup io_interfaces
 * @{
 */
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
__subsystem struct smi_driver_api {
	/** Enable the SMI bus device */
	int (*bus_enable)(const struct device *dev);

	/** Disable the SMI bus device */
	int (*bus_disable)(const struct device *dev);

	/** Read data from SMI bus */
	int (*read)(const struct device *dev, uint8_t prtad, uint8_t regad,
		    uint16_t *data);

	/** Write data to SMI bus */
	int (*write)(const struct device *dev, uint8_t prtad, uint8_t regad,
		     uint16_t data);
};
/**
 * @endcond
 */

/**
 * @brief      Enable SMI bus
 *
 * @param[in]  dev   Pointer to the device structure for the controller
 *
 */
__syscall void smi_bus_enable(const struct device *dev);

static inline void z_impl_smi_bus_enable(const struct device *dev)
{
	const struct smi_driver_api *api =
		(const struct smi_driver_api *)dev->api;

	if (api->bus_enable != NULL) {
		api->bus_enable(dev);
	}
}

/**
 * @brief      Disable SMI bus and tri-state drivers
 *
 * @param[in]  dev   Pointer to the device structure for the controller
 *
 */
__syscall void smi_bus_disable(const struct device *dev);

static inline void z_impl_smi_bus_disable(const struct device *dev)
{
	const struct smi_driver_api *api =
		(const struct smi_driver_api *)dev->api;

	if (api->bus_disable != NULL) {
		api->bus_disable(dev);
	}
}

/**
 * @brief      Read from SMI Bus
 *
 * This routine provides a generic interface to perform a read on the
 * SMI bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if read is not supported
 */
__syscall int smi_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			uint16_t *data);

static inline int z_impl_smi_read(const struct device *dev, uint8_t prtad,
				   uint8_t regad, uint16_t *data)
{
	const struct smi_driver_api *api =
		(const struct smi_driver_api *)dev->api;

	if (api->read == NULL) {
		return -ENOSYS;
	}

	return api->read(dev, prtad, regad, data);
}


/**
 * @brief      Write to SMI bus
 *
 * This routine provides a generic interface to perform a write on the
 * SMI bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param[in]  data        Data to write
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if write is not supported
 */
__syscall int smi_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			 uint16_t data);

static inline int z_impl_smi_write(const struct device *dev, uint8_t prtad,
				    uint8_t regad, uint16_t data)
{
	const struct smi_driver_api *api =
		(const struct smi_driver_api *)dev->api;

	if (api->write == NULL) {
		return -ENOSYS;
	}

	return api->write(dev, prtad, regad, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/smi.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SMI_H_ */
