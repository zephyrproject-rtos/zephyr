/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LORAWAN_EMUL_H_
#define ZEPHYR_INCLUDE_LORAWAN_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/lorawan/lorawan.h>

/**
 * @brief Defines the emulator uplink callback handler function signature.
 *
 * @param port LoRaWAN port
 * @param len Payload data length
 * @param data Pointer to the payload data
 */
typedef void (*lorawan_uplink_cb_t)(uint8_t port, uint8_t len, const uint8_t *data);

/**
 * @brief Emulate LoRaWAN downlink message
 *
 * @param port Port message was sent on
 * @param data_pending Network server has more downlink packets pending
 * @param rssi Received signal strength in dBm
 * @param snr Signal to Noise ratio in dBm
 * @param len Length of data received, will be 0 for ACKs
 * @param data Data received, will be NULL for ACKs
 */
void lorawan_emul_send_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
				uint8_t len, const uint8_t *data);

/**
 * @brief Register callback for emulated uplink messages
 *
 * @param cb Pointer to the uplink callback handler function
 */
void lorawan_emul_register_uplink_callback(lorawan_uplink_cb_t cb);

#endif /* ZEPHYR_INCLUDE_LORAWAN_EMUL_H_ */
