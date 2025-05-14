/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>

#include "common.h"

LOG_MODULE_REGISTER(bap_stream_rx, LOG_LEVEL_INF);

#define LOG_INTERVAL 100

static void log_stream_rx(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			  struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	LOG_INF("[%zu|%zu]: Incoming audio on stream %p len %u, flags 0x%02X, seq_num %u and ts %u",
		test_stream->valid_rx_cnt, test_stream->rx_cnt, stream, buf->len, info->flags,
		info->seq_num, info->ts);
}

static void log_stream_err(struct audio_test_stream *test_stream,
			   const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	if (!test_stream->last_rx_failed || (test_stream->rx_cnt % LOG_INTERVAL) == 0U) {
		log_stream_rx(&test_stream->stream.bap_stream, info, buf);
	}

	test_stream->last_rx_failed = true;
}

void bap_stream_rx_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);
	static bool last_failed;

	test_stream->rx_cnt++;
	if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
		if (memcmp(buf->data, mock_iso_data, buf->len) == 0) {
			test_stream->valid_rx_cnt++;
			last_failed = false;

			if (test_stream->valid_rx_cnt >= MIN_SEND_COUNT) {
				SET_FLAG(test_stream->flag_audio_received);
			}
		} else {
			log_stream_err(test_stream, info, buf);
			FAIL("Unexpected data received\n");
		}
	}

	if ((test_stream->rx_cnt % LOG_INTERVAL) == 0U) {
		log_stream_rx(stream, info, buf);
	}

	if (test_stream->rx_cnt >= LOG_INTERVAL && test_stream->valid_rx_cnt == 0U) {
		log_stream_err(test_stream, info, buf);

		FAIL("No valid RX received: %zu/%zu\n", test_stream->valid_rx_cnt,
		     test_stream->rx_cnt);
	}

	if (info->ts == test_stream->last_info.ts) {
		log_stream_err(test_stream, info, buf);

		if (test_stream->valid_rx_cnt > 1U) {
			FAIL("Duplicated timestamp received: %u\n", test_stream->last_info.ts);
		}
	}

	if (info->seq_num == test_stream->last_info.seq_num) {
		log_stream_err(test_stream, info, buf);

		if (test_stream->valid_rx_cnt > 1U) {
			FAIL("Duplicated PSN received: %u\n", test_stream->last_info.seq_num);
		}
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		/* Fail the test if we have not received what we expected */
		log_stream_err(test_stream, info, buf);

		if (test_stream->valid_rx_cnt > 1U &&
		    !TEST_FLAG(test_stream->flag_audio_received)) {
			FAIL("ISO receive error\n");
		}
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		log_stream_err(test_stream, info, buf);

		if (test_stream->valid_rx_cnt > 1U) {
			FAIL("ISO receive lost\n");
		}
	}
}

bool bap_stream_rx_can_recv(const struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info info;
	int err;

	if (stream == NULL || stream->ep == NULL) {
		return false;
	}

	err = bt_bap_ep_get_info(stream->ep, &info);
	if (err != 0) {
		return false;
	}

	return info.can_recv;
}
