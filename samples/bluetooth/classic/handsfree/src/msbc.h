/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MSBC_H_
#define MSBC_H_

#include <stddef.h>
#include <stdint.h>

/** mSBC SCO packet size (H2 header + SBC frame) */
#define MSBC_SCO_PKT_LEN 60U

/** PCM samples produced/consumed per mSBC frame */
#define MSBC_PCM_SAMPLES 120U

/** Recommended SCO transfer interval for mSBC (ms) */
#define MSBC_XFER_INTERVAL_MS 7U

int msbc_init(void);
int msbc_decode(const uint8_t *pkt, uint16_t len, int16_t *pcm_out, size_t pcm_count);
int msbc_encode(const int16_t *pcm_in, size_t pcm_count, uint8_t *pkt_out, size_t pkt_size);

#endif /* MSBC_H_ */
