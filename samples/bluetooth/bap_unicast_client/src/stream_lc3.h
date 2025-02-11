/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STREAM_LC3_H
#define STREAM_LC3_H

#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys_clock.h>

/* Since the lc3.h header file is not available when CONFIG_LIBLC3=n, we need to guard the include
 * and use of it
 */
#if defined(CONFIG_LIBLC3)
/* Header file for the liblc3 */
#include <lc3.h>

struct stream_lc3_tx {
	uint32_t freq_hz;
	uint32_t frame_duration_us;
	uint16_t octets_per_frame;
	uint8_t frame_blocks_per_sdu;
	uint8_t chan_cnt;
	enum bt_audio_location chan_allocation;
	lc3_encoder_t encoder;
	lc3_encoder_mem_48k_t encoder_mem;
};
#endif /* CONFIG_LIBLC3 */

/* Opaque definition to avoid including stream_tx.h */
struct tx_stream;

/*
 * @brief Initialize LC3 encoder for a stream
 *
 * This will initialize the encoder for the provided TX stream
 */
int stream_lc3_init(struct tx_stream *stream);

/**
 * Add LC3 encoded data to the provided buffer from the provided stream
 *
 * @param stream The TX stream to add data from
 * @param buf The buffer to store the encoded audio data in
 */
void stream_lc3_add_data(struct tx_stream *stream, struct net_buf *buf);

#endif /* STREAM_LC3_H */
