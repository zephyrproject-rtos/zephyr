/**
 * @file
 *
 * @brief Public APIs for MDIO drivers.
 */

/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MDIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_MDIO_H_

/**
 * @brief MDIO Interface
 * @defgroup mdio_interface MDIO Interface
 * @ingroup io_interfaces
 * @{
 */
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
__subsystem struct mdio_driver_api {
	/** Enable the MDIO bus device */
	void (*bus_enable)(const struct device *dev);

	/** Disable the MDIO bus device */
	void (*bus_disable)(const struct device *dev);

	/** Read data from MDIO bus */
	int (*read)(const struct device *dev, uint8_t prtad, uint8_t regad,
		    uint16_t *data);

	/** Write data to MDIO bus */
	int (*write)(const struct device *dev, uint8_t prtad, uint8_t regad,
		     uint16_t data);

	/** Read data from MDIO bus using Clause 45 access */
	int (*read_c45)(const struct device *dev, uint8_t prtad, uint8_t devad,
			uint16_t regad, uint16_t *data);

	/** Write data to MDIO bus using Clause 45 access */
	int (*write_c45)(const struct device *dev, uint8_t prtad, uint8_t devad,
			 uint16_t regad, uint16_t data);
};
/**
 * @endcond
 */

/**
 * @brief      Enable MDIO bus
 *
 * @param[in]  dev   Pointer to the device structure for the controller
 *
 */
__syscall void mdio_bus_enable(const struct device *dev);

static inline void z_impl_mdio_bus_enable(const struct device *dev)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	return api->bus_enable(dev);
}

/**
 * @brief      Disable MDIO bus and tri-state drivers
 *
 * @param[in]  dev   Pointer to the device structure for the controller
 *
 */
__syscall void mdio_bus_disable(const struct device *dev);

static inline void z_impl_mdio_bus_disable(const struct device *dev)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	return api->bus_disable(dev);
}

/**
 * @brief      Read from MDIO Bus
 *
 * This routine provides a generic interface to perform a read on the
 * MDIO bus.
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
__syscall int mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			uint16_t *data);

static inline int z_impl_mdio_read(const struct device *dev, uint8_t prtad,
				   uint8_t regad, uint16_t *data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->read == NULL) {
		return -ENOSYS;
	}

	return api->read(dev, prtad, regad, data);
}


/**
 * @brief      Write to MDIO bus
 *
 * This routine provides a generic interface to perform a write on the
 * MDIO bus.
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
__syscall int mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			 uint16_t data);

static inline int z_impl_mdio_write(const struct device *dev, uint8_t prtad,
				    uint8_t regad, uint16_t data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->write == NULL) {
		return -ENOSYS;
	}

	return api->write(dev, prtad, regad, data);
}

/**
 * @brief      Read from MDIO Bus using Clause 45 access
 *
 * This routine provides an interface to perform a read on the MDIO bus using
 * IEEE 802.3 Clause 45 access.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  devad       Device address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if write using Clause 45 access is not supported
 */
__syscall int mdio_read_c45(const struct device *dev, uint8_t prtad,
			    uint8_t devad, uint16_t regad, uint16_t *data);

static inline int z_impl_mdio_read_c45(const struct device *dev, uint8_t prtad,
				       uint8_t devad, uint16_t regad,
				       uint16_t *data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->read_c45 == NULL) {
		return -ENOSYS;
	}

	return api->read_c45(dev, prtad, devad, regad, data);
}

/**
 * @brief      Write to MDIO bus using Clause 45 access
 *
 * This routine provides an interface to perform a write on the MDIO bus using
 * IEEE 802.3 Clause 45 access.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  devad       Device address
 * @param[in]  regad       Register address
 * @param[in]  data        Data to write
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if write using Clause 45 access is not supported
 */
__syscall int mdio_write_c45(const struct device *dev, uint8_t prtad,
			     uint8_t devad, uint16_t regad, uint16_t data);

static inline int z_impl_mdio_write_c45(const struct device *dev, uint8_t prtad,
					uint8_t devad, uint16_t regad,
					uint16_t data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->write_c45 == NULL) {
		return -ENOSYS;
	}

	return api->write_c45(dev, prtad, devad, regad, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/mdio.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MDIO_H_ */
