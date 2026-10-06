/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modem computations shared by the native Semtech SX12xx drivers
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX12XX_LORA_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX12XX_LORA_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/lora.h>

/**
 * @brief Time on air of a LoRa packet with an explicit header
 *
 * From the Semtech LoRa modem designer's guide.
 *
 * @param bw_hz        Bandwidth in Hz
 * @param sf           Spreading factor
 * @param cr           Coding rate, 1 to 4 for 4/5 to 4/8
 * @param preamble_len Preamble length in symbols
 * @param ldro         Low data rate optimization on
 * @param crc          Payload CRC on
 * @param payload_len  Payload length in bytes
 *
 * @return Time on air in milliseconds, rounded to the nearest
 */
uint32_t sx12xx_lora_airtime_ms(uint32_t bw_hz, uint8_t sf, uint8_t cr, uint32_t preamble_len,
				bool ldro, bool crc, uint32_t payload_len);

/**
 * @brief Time on air of a GFSK packet
 *
 * Preamble, sync word, the length header unless the length is fixed, the
 * payload and the CRC, all at the configured bit rate.
 *
 * @param cfg         GFSK configuration
 * @param data_len    Payload length in bytes, when the length is not fixed
 * @param header_bits Length of the header of a variable-length packet
 *
 * @return Time on air in milliseconds, rounded up
 */
uint32_t sx12xx_gfsk_airtime_ms(const struct lora_modem_config_gfsk *cfg, uint32_t data_len,
				uint32_t header_bits);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX12XX_LORA_H_ */
