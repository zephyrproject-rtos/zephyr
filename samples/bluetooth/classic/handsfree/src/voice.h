/* voice.h - VOICE implementation for Bluetooth Hands-Free Profile */

/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_VOICE_H_
#define ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_VOICE_H_

#include <stdint.h>

typedef void (*voice_rx_cb_t)(const uint8_t *data, uint32_t len);

int voice_init(struct bt_conn *sco_conn, uint8_t air_mode, uint8_t codec_id);
int voice_deinit(void);
int voice_tx(const uint8_t *data, uint32_t len);
int voice_rx_start(voice_rx_cb_t cb);
int voice_rx_stop(void);

#endif /* _ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_VOICE_H_ */
