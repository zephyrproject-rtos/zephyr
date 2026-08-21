/*
 * Copyright (c) 2024-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Public API for the ADIN1140 10BASE-T1S MAC-PHY ethernet driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN1140_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN1140_H__

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Write C22 registers using ADIN1140 MDIO Bus
 *
 * This routine provides an interface to perform a C22 register write on the
 * ADIN1140 MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param[in]  data        Write data
 *
 * @retval 0 If successful.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin1140_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t data);

/**
 * @brief      Read C22 registers using ADIN1140 MDIO Bus
 *
 * This routine provides an interface to perform a C22 register read on the
 * ADIN1140 MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @retval 0 If successful.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin1140_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t *data);

/**
 * @brief      Write C45 registers using ADIN1140 MDIO Bus
 *
 * This routine provides an interface to perform a C45 register write on the
 * ADIN1140 MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  devad       MMD device address
 * @param[in]  regad       Register address
 * @param[in]  data        Write data
 *
 * @retval 0 If successful.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin1140_mdio_c45_write(const struct device *dev, uint8_t prtad, uint8_t devad,
				uint16_t regad, uint16_t data);

/**
 * @brief      Read C45 registers using ADIN1140 MDIO Bus
 *
 * This routine provides an interface to perform a C45 register read on the
 * ADIN1140 MDIO bus.
 *
 * @param[in]  dev         Pointer to the device structure for the controller
 * @param[in]  prtad       Port address
 * @param[in]  devad       MMD device address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @retval 0 If successful.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin1140_mdio_c45_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN1140_H__ */
