/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_PHY_OA_TC14_PLCA_H_
#define ZEPHYR_DRIVERS_ETHERNET_PHY_OA_TC14_PLCA_H_

/**
 * @brief      Write PHY PLCA configuration
 *
 * This routine provides a generic interface to configure PHY PLCA settings.
 *
 * @param[in]  dev       PHY device structure
 * @param[in]  plca_cfg  Pointer to plca configuration structure
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 */
int genphy_get_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg);

/**
 * @brief      Read PHY PLCA configuration
 *
 * This routine provides a generic interface to get PHY PLCA settings.
 *
 * @param[in]  dev       PHY device structure
 * @param      plca_cfg  Pointer to plca configuration structure
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 */
int genphy_set_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg);

/**
 * @brief      Read PHY PLCA status
 *
 * This routine provides a generic interface to get PHY PLCA status.
 *
 * @param[in]  dev          PHY device structure
 * @param      plca_status  Pointer to plca status
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 */
int genphy_get_plca_sts(const struct device *dev, bool *plca_status);

#endif /* ZEPHYR_DRIVERS_ETHERNET_PHY_OA_TC14_PLCA_H_ */
