/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SAMPLE_BAP_BROADCAST_SINK_STREAM_RX_H
#define SAMPLE_BAP_BROADCAST_SINK_STREAM_RX_H

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#if defined(CONFIG_LIBLC3)
#include <lc3.h>
#endif /* defined(CONFIG_LIBLC3) */

struct stream_rx {
	/* A BAP stream object */
	struct bt_bap_stream stream;
#if CONFIG_INFO_REPORTING_INTERVAL > 0
	/** Struct containing information useful for logging purposes */
	struct {
		/** Total Number of SDUs received */
		size_t recv_cnt;
		/** Number of lost SDUs */
		size_t loss_cnt;
		/** Number of SDUs containing errors */
		size_t error_cnt;
		/** Number of valid SDUs received */
		size_t valid_cnt;
		/** Number of empty SDUs received */
		size_t empty_sdu_cnt;
		/** Number of SDUs with duplicated packet sequence number received */
		size_t dup_psn_cnt;
		/** Number of SDUs with duplicated timestamps received */
		size_t dup_ts_cnt;
		/** The last received timestamp to track dup_ts_cnt */
		uint32_t last_ts;
		/** The last received sequence number to track dup_psn_cnt */
		uint16_t last_seq_num;
#if CONFIG_LIBLC3 > 0
		/** Number of SDUs decoded  */
		size_t lc3_decoded_cnt;
#endif /* CONFIG_LIBLC3 > 0 */
	} reporting_info;
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */

#if defined(CONFIG_LIBLC3)
	/** Octets per frame - Used to validate that the incoming data is of correct size  */
	uint16_t lc3_octets_per_frame;
	/** Frame blocks per SDU - Used to split the SDU into frame blocks when decoding */
	uint8_t lc3_frame_blocks_per_sdu;

	/** Number of channels - Used to split the SDU into frame blocks when decoding */
	uint8_t lc3_chan_cnt;

	/**
	 * @brief The configured channels of the stream
	 *
	 * Used to determine whether to send data to USB and count number of channels
	 */
	enum bt_audio_location lc3_chan_allocation;

	/** Memory use for the LC3 decoder - Supports any configuration */
	lc3_decoder_mem_48k_t lc3_decoder_mem;
	/** Reference to the LC3 decoder */
	lc3_decoder_t lc3_decoder;
#endif /* defined(CONFIG_LIBLC3) */
};

/**
 * @brief Function to call for each SDU received
 *
 * Will decode with LC3 and send to USB if enabled
 */
void stream_rx_recv(struct bt_bap_stream *bap_stream, const struct bt_iso_recv_info *info,
		    struct net_buf *buf);

size_t stream_rx_get_streaming_cnt(void);
int stream_rx_started(struct bt_bap_stream *bap_stream);
int stream_rx_stopped(struct bt_bap_stream *bap_stream);
void stream_rx_get_streams(
	struct bt_bap_stream *bap_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT]);

#endif /* SAMPLE_BAP_BROADCAST_SINK_STREAM_RX_H */
