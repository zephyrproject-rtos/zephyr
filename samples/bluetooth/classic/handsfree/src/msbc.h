/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MSBC_H_
#define MSBC_H_

#include <stddef.h>
#include <stdint.h>

/** mSBC SCO packet size: 2-byte H2 + 57-byte SBC frame + 1 pad */
#define MSBC_SCO_PKT_LEN 60U

/** PCM samples produced/consumed per mSBC frame */
#define MSBC_PCM_SAMPLES 120U

/** mSBC frame duration in microseconds (120 samples @ 16 kHz) */
#define MSBC_FRAME_DURATION_US 7500U

/** mSBC stream rate in bytes per second (one 60-byte frame every 7.5 ms) */
#define MSBC_BYTE_RATE 8000U

int msbc_init(void);

/** @brief Locate the start of an mSBC frame in a byte stream.
 *
 *  @param data Stream bytes to scan.
 *  @param len  Number of bytes in @p data.
 *
 *  @return Offset of the H2 header, or a negative value if no frame starts in
 *          @p data.
 */
int msbc_frame_sync(const uint8_t *data, uint16_t len);

int msbc_decode(const uint8_t *pkt, uint16_t len, int16_t *pcm_out, size_t pcm_count);
int msbc_encode(const int16_t *pcm_in, size_t pcm_count, uint8_t *pkt_out, size_t pkt_size);

#endif /* MSBC_H_ */
