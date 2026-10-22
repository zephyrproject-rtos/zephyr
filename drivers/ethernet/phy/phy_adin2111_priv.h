/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PHY_ADIN2111_PRIV_H__
#define PHY_ADIN2111_PRIV_H__

#include <zephyr/device.h>
#include <zephyr/net/phy.h>

/**
 * @brief Handles PHY interrupt.
 *
 * @note Used internally by the ADIN offloaded ISR handler.
 *       The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev PHY device.
 * @param[out] state Output of the link state.
 *
 * @retval 0 Successful and link state changed.
 * @retval -EAGAIN Successful but link state didn't change.
 * @retval <0 MDIO error.
 */
int phy_adin2111_handle_phy_irq(const struct device *dev,
				struct phy_link_state *state);

#endif /* PHY_ADIN2111_PRIV_H__ */
