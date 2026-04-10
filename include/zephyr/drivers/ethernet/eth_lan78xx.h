/*
 * Copyright (c) 2026 Narek Aydinyan
 * Copyright (c) 2026 Arayik Gharibyan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETHERNET_ETH_LAN78XX_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETHERNET_ETH_LAN78XX_H__

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read a LAN78xx PHY C22 register through the adapter MDIO interface.
 *
 * @param dev Ethernet device pointer.
 * @param prtad PHY address.
 * @param regad Register address.
 * @param data Pointer receiving read value.
 *
 * @retval 0 On success.
 * @retval -EINVAL On invalid argument.
 * @retval -ETIMEDOUT On MDIO timeout.
 * @retval Other negative errno from the control path.
 */
int eth_lan78xx_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data);

/**
 * @brief Write a LAN78xx PHY C22 register through the adapter MDIO interface.
 *
 * @param dev Ethernet device pointer.
 * @param prtad PHY address.
 * @param regad Register address.
 * @param data Register value to write.
 *
 * @retval 0 On success.
 * @retval -EINVAL On invalid argument.
 * @retval -ETIMEDOUT On MDIO timeout.
 * @retval Other negative errno from the control path.
 */
int eth_lan78xx_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETHERNET_ETH_LAN78XX_H__ */
