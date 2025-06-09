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

	LOG_INF("[%zu|%zu|%zu]: Incoming audio on stream %p len %u, flags 0x%02X, seq_num %u and"
		" ts %u",
		test_stream->valid_rx_cnt, test_stream->err_rx_cnt, test_stream->rx_cnt, stream,
		buf->len, info->flags, info->seq_num, info->ts);
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
	bool do_log_info = false;
	bool do_log_err = false;
	bool is_err = false;

	test_stream->rx_cnt++;
	if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
		if (memcmp(buf->data, mock_iso_data, buf->len) == 0) {
			test_stream->valid_rx_cnt++;
			last_failed = false;

			if (test_stream->valid_rx_cnt >= MIN_SEND_COUNT) {
				SET_FLAG(test_stream->flag_audio_received);
			}
		} else {
			LOG_ERR("Unexpected data received.");
			is_err = true;
		}
	}

	if ((test_stream->rx_cnt % LOG_INTERVAL) == 0U) {
		do_log_info = true;
	}

	if (test_stream->rx_cnt >= LOG_INTERVAL && test_stream->valid_rx_cnt == 0U) {
		is_err = true;

		LOG_ERR("No valid RX received: %zu/%zu.", test_stream->valid_rx_cnt,
			test_stream->rx_cnt);
	}

	if (info->ts == test_stream->last_info.ts) {
		do_log_err = true;

		if (test_stream->valid_rx_cnt > 1U) {
			is_err = true;

			LOG_ERR("Duplicated timestamp received: %u.", test_stream->last_info.ts);
		}
	}

	if (info->seq_num == test_stream->last_info.seq_num) {
		do_log_err = true;

		if (test_stream->valid_rx_cnt > 1U) {
			is_err = true;

			LOG_ERR("Duplicated PSN received: %u.", test_stream->last_info.seq_num);
		}
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		do_log_err = true;

		/* Fail the test if we have not received what we expected */
		if (test_stream->valid_rx_cnt > 1U &&
		    !TEST_FLAG(test_stream->flag_audio_received)) {
			test_stream->err_rx_cnt++;

			LOG_ERR("ISO receive error.");
		}
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		do_log_err = true;

		if (test_stream->valid_rx_cnt > 1U) {
			LOG_ERR("ISO receive lost.");
			is_err = true;
		}
	}

	if (test_stream->err_rx_cnt > MAX_FAIL_COUNT) {
		is_err = true;
	}

	if (do_log_err || is_err) {
		log_stream_err(test_stream, info, buf);

		if (is_err) {
			FAIL("ISO Receive Failure.\n");
		}
	} else if (do_log_info) {
		log_stream_rx(stream, info, buf);
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
