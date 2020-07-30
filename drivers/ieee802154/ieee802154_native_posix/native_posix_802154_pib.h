/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Module that implements the storage of PIB attributes in the
 *        nRF 802.15.4 radio driver.
 *
 */

#ifndef NATIVE_POSIX_802154_PIB_H_
#define NATIVE_POSIX_802154_PIB_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes this module.
 */
void nrf_802154_pib_init(void);

/**
 * @brief Checks if the promiscuous mode is enabled.
 *
 * @retval  true   The promiscuous mode is enabled.
 * @retval  false  The promiscuous mode is disabled.
 */
bool nrf_802154_pib_promiscuous_get(void);

/**
 * @brief Enables or disables the promiscuous mode.
 *
 * @param[in]  enabled  If the promiscuous mode is to be enabled.
 */
void nrf_802154_pib_promiscuous_set(bool enabled);

/**
 * @brief Checks if the auto ACK procedure is enabled.
 *
 * @retval  true   The auto ACK procedure is enabled.
 * @retval  false  The auto ACK procedure is disabled.
 */
bool nrf_802154_pib_auto_ack_get(void);

/**
 * @brief Enables or disables the auto ACK procedure.
 *
 * @param[in]  enabled  If the auto ACK procedure is to be enabled.
 */
void nrf_802154_pib_auto_ack_set(bool enabled);

/**
 * @brief Checks if the radio is configured as the PAN coordinator.
 *
 * @retval  true   The radio is configured as the PAN coordinator.
 * @retval  false  The radio is not configured as the PAN coordinator.
 */
bool nrf_802154_pib_pan_coord_get(void);

/**
 * @brief Configures the device as the PAN coordinator.
 *
 * @param[in]  enabled  If the radio is configured as the PAN coordinator.
 */
void nrf_802154_pib_pan_coord_set(bool enabled);

/**
 * @brief Gets the currently used channel.
 *
 * @returns  Channel number used by the driver.
 */
uint8_t nrf_802154_pib_channel_get(void);

/**
 * @brief Sets the channel that will be used by the driver.
 *
 * @param[in]  channel  Number of the channel used by the driver.
 */
void nrf_802154_pib_channel_set(uint8_t channel);

/**
 * @brief Sets the transmit power used for ACK frames.
 *
 * @param[in]  dbm  Transmit power in dBm.
 */
void nrf_802154_pib_tx_power_set(int8_t dbm);

/**
 * @brief Gets the PAN ID used by this device.
 *
 * @returns Pointer to the buffer containing the PAN ID value
 *          (2 bytes, little-endian).
 */
const uint8_t *nrf_802154_pib_pan_id_get(void);

/**
 * @brief Sets the PAN ID used by this device.
 *
 * @param[in]  p_pan_id  Pointer to the PAN ID (2 bytes, little-endian).
 *
 * This function makes a copy of the PAN ID.
 */
void nrf_802154_pib_pan_id_set(const uint8_t *p_pan_id);

/**
 * @brief Gets the extended address of this device.
 *
 * @returns Pointer to the buffer containing the extended address
 *          (8 bytes, little-endian).
 */
const uint8_t *nrf_802154_pib_extended_address_get(void);

/**
 * @brief Sets the extended address of this device.
 *
 * @param[in]  p_extended_address  Pointer to extended address
 *                                 (8 bytes, little-endian).
 *
 * This function makes a copy of the address.
 */
void nrf_802154_pib_extended_address_set(const uint8_t *p_extended_address);

/**
 * @brief Gets the short address of this device.
 *
 * @returns Pointer to the buffer containing the short address
 *          (2 bytes, little-endian).
 */
const uint8_t *nrf_802154_pib_short_address_get(void);

/**
 * @brief Sets the short address of this device.
 *
 * @param[in]  p_short_address  Pointer to the short address
 *                              (2 bytes, little-endian).
 *
 * This function makes a copy of the address.
 */
void nrf_802154_pib_short_address_set(const uint8_t *p_short_address);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_PIB_H_ */
