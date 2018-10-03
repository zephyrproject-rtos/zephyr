/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Ethernet PHY (GMAC) driver.
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_PHY_SAM_GMAC_H_
#define ZEPHYR_DRIVERS_ETHERNET_PHY_SAM_GMAC_H_

#include <zephyr/types.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PHY_DUPLEX_FULL  GMAC_NCFGR_FD
#define PHY_DUPLEX_HALF  0
#define PHY_SPEED_100M   GMAC_NCFGR_SPD
#define PHY_SPEED_10M    0

struct phy_sam_gmac_dev {
	Gmac *regs;
	u8_t address;
};

/**
 * @brief Initialize Ethernet PHY device.
 *
 * @param phy PHY instance
 * @return 0 on success or a negative error value on failure
 */
int phy_sam_gmac_init(const struct phy_sam_gmac_dev *phy);

/**
 * @brief Auto-negotiate and configure link parameters.
 *
 * @param phy PHY instance
 * @param status link parameters common to remote and local PHY
 * @return 0 on success or a negative error value on failure
 */
int phy_sam_gmac_auto_negotiate(const struct phy_sam_gmac_dev *phy,
				u32_t *status);

/**
 * @brief Get PHY ID value.
 *
 * @param phy PHY instance
 * @return PHY ID value or 0xFFFFFFFF on failure
 */
u32_t phy_sam_gmac_id_get(const struct phy_sam_gmac_dev *phy);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ETHERNET_PHY_SAM_GMAC_H_ */
