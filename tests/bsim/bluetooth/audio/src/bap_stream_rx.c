/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>

#include "common.h"
#include <string.h>

LOG_MODULE_REGISTER(bap_stream_rx, LOG_LEVEL_INF);

static void log_stream_rx(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			  struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	LOG_INF("[%zu]: Incoming audio on stream %p len %u, flags 0x%02X, seq_num %u and ts %u",
		test_stream->rx_cnt, stream, buf->len, info->flags, info->seq_num, info->ts);
}

void bap_stream_rx_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	if ((test_stream->rx_cnt % 50U) == 0U) {
		log_stream_rx(stream, info, buf);
	}

	if (test_stream->rx_cnt > 0U && info->ts == test_stream->last_info.ts) {
		log_stream_rx(stream, info, buf);
		FAIL("Duplicated timestamp received: %u\n", test_stream->last_info.ts);
		return;
	}

	if (test_stream->rx_cnt > 0U && info->seq_num == test_stream->last_info.seq_num) {
		log_stream_rx(stream, info, buf);
		FAIL("Duplicated PSN received: %u\n", test_stream->last_info.seq_num);
		return;
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		/* Fail the test if we have not received what we expected */
		if (!TEST_FLAG(flag_audio_received)) {
			log_stream_rx(stream, info, buf);
			FAIL("ISO receive error\n");
		}

		return;
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		log_stream_rx(stream, info, buf);
		FAIL("ISO receive lost\n");
		return;
	}

	if (memcmp(buf->data, mock_iso_data, buf->len) == 0) {
		test_stream->rx_cnt++;

		if (test_stream->rx_cnt >= MIN_SEND_COUNT) {
			/* We set the flag is just one stream has received the expected */
			SET_FLAG(flag_audio_received);
		}
	} else {
		log_stream_rx(stream, info, buf);
		FAIL("Unexpected data received\n");
	}
}
