/* codec.h - CODEC implementation for Bluetooth Hands-Free Profile */

/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_CODEC_H_
#define ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_CODEC_H_

#include <stdint.h>

typedef void (*codec_rx_cb_t)(const uint8_t *data, uint32_t len);

int codec_init(uint8_t air_mode);
int codec_deinit(void);
int codec_tx(const uint8_t *data, uint32_t len);
int codec_rx_start(codec_rx_cb_t cb);
int codec_rx_stop(void);

#endif /* _ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_CODEC_H_ */
