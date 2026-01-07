/* pcm.h - PCM implementation for Bluetooth Hands-Free Profile */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_PCM_H_
#define _ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_PCM_H_

#include <stdint.h>

typedef void (*pcm_rx_cb_t)(const uint8_t *data, uint32_t len);

int pcm_init(uint8_t air_mode);
int pcm_tx(const uint8_t *data, uint32_t len);
int pcm_rx_start(pcm_rx_cb_t cb);
int pcm_rx_stop(void);

#endif /* _ZEPHYR_SAMPLES_CLASSIC_HANDSFREE_PCM_H_ */
