/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mdio_interface
 * @brief Main header file for MDIO (Management Data Input/Output) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MDIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_MDIO_H_

/**
 * @brief Interfaces for Management Data Input/Output (MDIO) controllers.
 * @defgroup mdio_interface MDIO
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
 * @def_driverbackendgroup{MDIO,mdio_interface}
 * @{
 */

/**
 * @brief Read data from MDIO bus.
 * See mdio_read() for argument description.
 */
typedef int (*mdio_api_read_t)(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t *data);

/**
 * @brief Write data to MDIO bus.
 * See mdio_write() for argument description.
 */
typedef int (*mdio_api_write_t)(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t data);

/**
 * @brief Read data from MDIO bus using Clause 45 access.
 * See mdio_read_c45() for argument description.
 */
typedef int (*mdio_api_read_c45_t)(const struct device *dev, uint8_t prtad, uint8_t devad,
				   uint16_t regad, uint16_t *data);

/**
 * @brief Write data to MDIO bus using Clause 45 access.
 * See mdio_write_c45() for argument description.
 */
typedef int (*mdio_api_write_c45_t)(const struct device *dev, uint8_t prtad, uint8_t devad,
				    uint16_t regad, uint16_t data);

/**
 * @driver_ops{MDIO}
 */
__subsystem struct mdio_driver_api {
	/** @driver_ops_optional @copybrief mdio_read */
	mdio_api_read_t read;

	/** @driver_ops_optional @copybrief mdio_write */
	mdio_api_write_t write;

	/** @driver_ops_optional @copybrief mdio_read_c45 */
	mdio_api_read_c45_t read_c45;

	/** @driver_ops_optional @copybrief mdio_write_c45 */
	mdio_api_write_c45_t write_c45;
};

/**
 * @}
 */

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
	if (DEVICE_API_GET(mdio, dev)->read == NULL) {
		return -ENOSYS;
	}

	return DEVICE_API_GET(mdio, dev)->read(dev, prtad, regad, data);
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
	if (DEVICE_API_GET(mdio, dev)->write == NULL) {
		return -ENOSYS;
	}

	return DEVICE_API_GET(mdio, dev)->write(dev, prtad, regad, data);
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
	if (DEVICE_API_GET(mdio, dev)->read_c45 == NULL) {
		return -ENOSYS;
	}

	return DEVICE_API_GET(mdio, dev)->read_c45(dev, prtad, devad, regad, data);
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
	if (DEVICE_API_GET(mdio, dev)->write_c45 == NULL) {
		return -ENOSYS;
	}

	return DEVICE_API_GET(mdio, dev)->write_c45(dev, prtad, devad, regad, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/mdio.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MDIO_H_ */
