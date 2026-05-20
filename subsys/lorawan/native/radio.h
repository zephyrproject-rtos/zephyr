/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_RADIO_H_
#define SUBSYS_LORAWAN_NATIVE_RADIO_H_

#include <zephyr/device.h>

#include "region/region.h"

#ifdef __cplusplus
extern "C" {
#endif

int radio_init(const struct device *lora_dev);

/* TX: IQ normal, CRC on; uses lora_send_async + k_poll for precise timing */
int radio_tx(const uint8_t *data, size_t len,
	     uint32_t freq, const struct lwan_dr_params *dr,
	     int8_t power);

/* RX: IQ inverted, CRC off; returns byte count, 0 on timeout, -EIO on error */
int radio_rx(uint32_t freq, const struct lwan_dr_params *dr,
	     uint32_t timeout_ms,
	     uint8_t *buf, uint8_t buf_size,
	     int16_t *rssi, int8_t *snr);

/* Airtime in ms; radio must already be configured for TX */
uint32_t radio_airtime(uint32_t data_len);

/* Airtime in ms from explicit SF/BW/len (no prior radio config needed) */
uint32_t radio_airtime_params(uint8_t sf, uint16_t bw_khz, uint8_t frame_len);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_RADIO_H_ */
