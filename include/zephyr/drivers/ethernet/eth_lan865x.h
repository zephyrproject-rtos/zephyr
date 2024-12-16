/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_LAN865X_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_LAN865X_H__

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

/**
 * @brief      Read C22 registers using LAN865X MDIO Bus
 *
 * This routine provides an interface to perform a C22 register read on the
 * LAN865X MDIO bus.
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
int eth_lan865x_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data);

/**
 * @brief      Write C22 registers using LAN865X MDIO Bus
 *
 * This routine provides an interface to perform a C22 register write on the
 * LAN865X MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param[in]  data        Write data
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if read is not supported
 */
int eth_lan865x_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data);

/**
 * @brief      Read C45 registers using LAN865X MDIO Bus
 *
 * This routine provides an interface to perform a C45 register read on the
 * LAN865X MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  devad       MMD device address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if read is not supported
 */
int eth_lan865x_mdio_c45_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			      uint16_t regad, uint16_t *data);

/**
 * @brief      Write C45 registers using LAN865X MDIO Bus
 *
 * This routine provides an interface to perform a C45 register write on the
 * LAN865X MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  devad       MMD device address
 * @param[in]  regad       Register address
 * @param[in]  data        Write data
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus
 * @retval -ENOSYS if read is not supported
 */
int eth_lan865x_mdio_c45_write(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t data);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_LAN865X_H__ */
