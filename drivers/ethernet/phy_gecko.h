/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_PHY_GECKO_H_
#define ZEPHYR_DRIVERS_ETHERNET_PHY_GECKO_H_

#include <zephyr/types.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct phy_gecko_dev {
	ETH_TypeDef *regs;
	uint8_t address;
};

/**
 * @brief Initialize Ethernet PHY device.
 *
 * @param phy PHY instance
 * @return 0 on success or a negative error value on failure
 */
int phy_gecko_init(const struct phy_gecko_dev *phy);

/**
 * @brief Auto-negotiate and configure link parameters.
 *
 * @param phy PHY instance
 * @param status link parameters common to remote and local PHY
 * @return 0 on success or a negative error value on failure
 */
int phy_gecko_auto_negotiate(const struct phy_gecko_dev *phy,
				uint32_t *status);
/**
 * @brief Get PHY ID value.
 *
 * @param phy PHY instance
 * @return PHY ID value or 0xFFFFFFFF on failure
 */
uint32_t phy_gecko_id_get(const struct phy_gecko_dev *phy);

/**
 * @brief Get PHY linked status.
 *
 * @param phy PHY instance
 * @return PHY linked status
 */
bool phy_gecko_is_linked(const struct phy_gecko_dev *phy);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ETHERNET_PHY_GECKO_H_ */
