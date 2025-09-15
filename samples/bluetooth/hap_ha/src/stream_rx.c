/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#if defined(CONFIG_LIBLC3)
#include <lc3.h>
#endif /* defined(CONFIG_LIBLC3) */

#include "stream_rx.h"
#include "lc3.h"

struct stream_rx rx_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
uint64_t total_rx_iso_packet_count; /* This value is exposed to test code */

LOG_MODULE_REGISTER(stream_rx, CONFIG_LOG_DEFAULT_LEVEL);

#if CONFIG_INFO_REPORTING_INTERVAL > 0
static void log_stream_rx(const struct stream_rx *stream, const struct bt_iso_recv_info *info,
			  const struct net_buf *buf)
{
	LOG_INF("[%zu]: Incoming audio on stream %p len %u, flags 0x%02X, seq_num %u and ts %u: "
		"Valid %zu | Error %zu | Loss %zu | Dup TS %zu | Dup PSN %zu",
		stream->reporting_info.recv_cnt, &stream->stream, buf->len, info->flags,
		info->seq_num, info->ts, stream->reporting_info.valid_cnt,
		stream->reporting_info.error_cnt, stream->reporting_info.loss_cnt,
		stream->reporting_info.dup_ts_cnt, stream->reporting_info.dup_psn_cnt);
}
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */

void stream_rx_recv(struct bt_bap_stream *bap_stream, const struct bt_iso_recv_info *info,
		    struct net_buf *buf)
{
	struct stream_rx *stream = CONTAINER_OF(bap_stream, struct stream_rx, stream);

#if CONFIG_INFO_REPORTING_INTERVAL > 0
	bool do_log = false;

	stream->reporting_info.recv_cnt++;
	if (stream->reporting_info.recv_cnt == 1U) {
		/* Log first reception */
		do_log = true;

	} else if ((stream->reporting_info.recv_cnt % CONFIG_INFO_REPORTING_INTERVAL) == 0U) {
		/* Log once for every CONFIG_INFO_REPORTING_INTERVAL reception */
		do_log = true;
	}

	if (stream->reporting_info.recv_cnt > 1U && info->ts == stream->reporting_info.last_ts) {
		stream->reporting_info.dup_ts_cnt++;
		do_log = true;
		LOG_WRN("Duplicated timestamp received: %u", stream->reporting_info.last_ts);
	}

	if (stream->reporting_info.recv_cnt > 1U &&
	    info->seq_num == stream->reporting_info.last_seq_num) {
		stream->reporting_info.dup_psn_cnt++;
		do_log = true;
		LOG_WRN("Duplicated PSN received: %u", stream->reporting_info.last_seq_num);
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		stream->reporting_info.error_cnt++;
		do_log = true;
		LOG_DBG("ISO receive error");
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		stream->reporting_info.loss_cnt++;
		do_log = true;
		LOG_DBG("ISO receive lost");
	}

	if (info->flags & BT_ISO_FLAGS_VALID) {
		if (buf->len == 0U) {
			stream->reporting_info.empty_sdu_cnt++;
		} else {
			stream->reporting_info.valid_cnt++;
		}
	}

	if (do_log) {
		log_stream_rx(stream, info, buf);
	}

	stream->reporting_info.last_seq_num = info->seq_num;
	stream->reporting_info.last_ts = info->ts;
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */

	total_rx_iso_packet_count++;

	if (IS_ENABLED(CONFIG_LIBLC3)) {
		/* Invalid SDUs will trigger PLC */
		lc3_enqueue_for_decoding(stream, info, buf);
	}
}

int stream_rx_started(struct bt_bap_stream *bap_stream)
{
	struct stream_rx *stream = CONTAINER_OF(bap_stream, struct stream_rx, stream);

	if (stream == NULL) {
		return -EINVAL;
	}

#if CONFIG_INFO_REPORTING_INTERVAL > 0
	memset(&stream->reporting_info, 0, sizeof((stream->reporting_info)));
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */

	if (IS_ENABLED(CONFIG_LIBLC3) && bap_stream->codec_cfg != NULL &&
	    bap_stream->codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		int err;

		err = lc3_enable(stream);
		if (err < 0) {
			LOG_ERR("Error: cannot enable LC3 codec: %d", err);
			return err;
		}
	}

	return 0;
}

int stream_rx_stopped(struct bt_bap_stream *bap_stream)
{
	struct stream_rx *stream = CONTAINER_OF(bap_stream, struct stream_rx, stream);

	if (bap_stream == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_LIBLC3) && bap_stream->codec_cfg != NULL &&
	    bap_stream->codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		int err;

		err = lc3_disable(stream);
		if (err < 0) {
			LOG_ERR("Error: cannot disable LC3 codec: %d", err);
			return err;
		}
	}

	return 0;
}

void stream_rx_get_streams(
	struct bt_bap_stream *bap_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT])
{
	for (size_t i = 0U; i < ARRAY_SIZE(rx_streams); i++) {
		bap_streams[i] = &rx_streams[i].stream;
	}
}
