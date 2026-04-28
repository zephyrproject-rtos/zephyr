/*
 * Copyright (c) 2026 Narek Aydinyan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the LAN78xx USB Ethernet driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETHERNET_ETH_LAN78XX_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETHERNET_ETH_LAN78XX_H__

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @brief Read C22 registers using the LAN78xx MDIO bus.
 *
 * This routine provides an interface to perform a C22 register read on the
 * LAN78xx MDIO bus.
 *
 * @param[in]  dev     Pointer to the LAN78xx Ethernet device.
 * @param[in]  prtad   PHY address.
 * @param[in]  regad   Register address.
 * @param[out] data    Pointer receiving read data.
 *
 * @retval 0           If successful.
 * @retval -EINVAL     If one or more arguments are invalid.
 * @retval -EIO        If register access fails.
 * @retval -ETIMEDOUT  If the MDIO transaction times out.
 */
int eth_lan78xx_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data);

/**
 * @brief Write C22 registers using the LAN78xx MDIO bus.
 *
 * This routine provides an interface to perform a C22 register write on the
 * LAN78xx MDIO bus.
 *
 * @param[in] dev      Pointer to the LAN78xx Ethernet device.
 * @param[in] prtad    PHY address.
 * @param[in] regad    Register address.
 * @param[in] data     Register value to write.
 *
 * @retval 0           If successful.
 * @retval -EINVAL     If one or more arguments are invalid.
 * @retval -EIO        If register access fails.
 * @retval -ETIMEDOUT  If the MDIO transaction times out.
 */
int eth_lan78xx_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETHERNET_ETH_LAN78XX_H__ */
