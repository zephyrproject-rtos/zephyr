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

static void log_stream_rx(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			  struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	LOG_INF("[%zu|%zu|%zu]: Incoming audio on stream %p len %u, flags 0x%02X, seq_num %u and"
		" ts %u",
		test_stream->valid_rx_cnt, test_stream->err_rx_cnt, test_stream->rx_cnt, stream,
		buf->len, info->flags, info->seq_num, info->ts);
}

void bap_stream_rx_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);
	bool do_log = false;
	bool is_err = false;

	test_stream->rx_cnt++;
	if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
		if (memcmp(buf->data, mock_iso_data, buf->len) == 0) {
			test_stream->valid_rx_cnt++;

			if (test_stream->valid_rx_cnt >= MIN_SEND_COUNT) {
				/* We set the flag is just one stream has received the expected */
				SET_FLAG(flag_audio_received);
			}
		} else {
			LOG_ERR("Unexpected data received.");
			is_err = true;
		}
	}

	if ((test_stream->rx_cnt % 1000U) == 0U) {
		do_log = true;
	}

	if (info->ts == test_stream->last_info.ts && test_stream->valid_rx_cnt > 1U) {
		LOG_ERR("Duplicated timestamp received: %u", test_stream->last_info.ts);
		is_err = true;
	}

	if (info->seq_num == test_stream->last_info.seq_num && test_stream->valid_rx_cnt > 1U) {
		LOG_ERR("Duplicated PSN received: %u", test_stream->last_info.seq_num);
		is_err = true;
	}

	if (info->seq_num != (test_stream->last_info.seq_num + 1U) &&
	    test_stream->valid_rx_cnt > 1U) {
		LOG_ERR("Incorrect PSN received: %u", test_stream->last_info.seq_num);
		is_err = true;
	}

	if (info->flags & BT_ISO_FLAGS_ERROR && test_stream->valid_rx_cnt > 1U &&
	    !TEST_FLAG(flag_audio_received)) {
		test_stream->err_rx_cnt++;
		/* Fail the test if we have not received what we expected */
		LOG_ERR("ISO receive error.");
		is_err = true;
	}

	if (info->flags & BT_ISO_FLAGS_LOST && test_stream->valid_rx_cnt > 1U) {
		LOG_ERR("ISO receive lost.");
		is_err = true;
	}

	if (do_log || is_err) {
		log_stream_rx(stream, info, buf);
	}

	if (test_stream->err_rx_cnt > MAX_FAIL_COUNT) {
		FAIL("ISO Receive Failure.\n");
	}

	test_stream->last_info.ts = info->ts;
	test_stream->last_info.seq_num = info->seq_num;
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
