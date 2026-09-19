/*
 * Copyright (c) 2026 Muhammed Asif P
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Microchip Event System g1 driver API.
 * @ingroup mchp_evsys_interface
 */

#ifndef ZEPHYR_DRIVERS_MISC_MCHP_EVSYS_MCHP_EVSYS_G1_H_
#define ZEPHYR_DRIVERS_MISC_MCHP_EVSYS_MCHP_EVSYS_G1_H_

/**
 * @brief Interfaces for the Microchip Event System.
 * @defgroup mchp_evsys_interface Microchip Event System
 * @ingroup misc_interfaces
 * @{
 */

#include <zephyr/device.h>

#ifdef CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X
#include <zephyr/dt-bindings/misc/mchp_evsys_g1_sam_d5x_e5x.h>
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X */

/**
 * @brief Connect an event system channel to an event generator.
 *
 * Configures the specified EVSYS channel to receive events from the given
 * event generator using an asynchronous path. The channel is marked as
 * occupied after successful configuration.
 *
 * @param[in] dev         Pointer to the EVSYS device instance.
 * @param[in] channel_num EVSYS channel number to configure.
 * @param[in] ev_gen      Event generator to connect to the channel.
 *
 * @return 0              If the channel is successfully configured.
 * @return -EINVAL        If the specified channel number is invalid.
 * @return -EBUSY         If the specified channel is already in use.
 */

int evsys_mchp_connect_channel_to_evgen(const struct device *dev, uint8_t channel_num, int ev_gen);

/**
 * @brief Connect an EVSYS user to an event system channel.
 *
 * Configures the specified EVSYS user to receive events from the given
 * EVSYS channel. The user register is selected using the provided user
 * register offset.
 *
 * @param[in] dev         Pointer to the EVSYS device instance.
 * @param[in] channel_num EVSYS channel number to connect to the user.
 * @param[in] user_offset Offset of the EVSYS user register corresponding
 *                        to the user to be configured.
 *
 * @return 0       If the user is successfully connected to the channel.
 * @return -EINVAL If the specified channel number or user ID is invalid.
 */
int evsys_mchp_connect_user_to_channel(const struct device *dev, uint8_t channel_num,
				       int user_offset);

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_MCHP_EVSYS_MCHP_EVSYS_G1_H_ */
