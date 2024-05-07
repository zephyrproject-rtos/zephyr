/** @file
 *  @brief Bluetooth Common Audio Profile (CAP) Acceptor unicast.
 *
 *  Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *  Copyright (c) 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "cap_acceptor.h"

LOG_MODULE_REGISTER(cap_acceptor_unicast, LOG_LEVEL_INF);

#define PREF_PHY           BT_GAP_LE_PHY_2M
#define MIN_PD             20000U
#define MAX_PD             40000U
#define UNFRAMED_SUPPORTED true
#define LATENCY            20U
#define RTN                2U

static const struct bt_audio_codec_qos_pref qos_pref = BT_AUDIO_CODEC_QOS_PREF(
	UNFRAMED_SUPPORTED, PREF_PHY, RTN, LATENCY, MIN_PD, MAX_PD, MIN_PD, MAX_PD);
uint64_t total_unicast_rx_iso_packet_count; /* This value is exposed to test code */
uint64_t total_unicast_tx_iso_packet_count; /* This value is exposed to test code */

static bool log_codec_cfg_cb(struct bt_data *data, void *user_data)
{
	const char *str = (const char *)user_data;

	LOG_DBG("\t%s: type 0x%02x value_len %u", str, data->type, data->data_len);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "\t\tdata");

	return true;
}

static void log_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	LOG_INF("codec_cfg 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cfg->id, codec_cfg->cid,
		codec_cfg->vid, codec_cfg->data_len);

	if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		enum bt_audio_location chan_allocation;
		int ret;

		/* LC3 uses the generic LTV format - other codecs might do as well */

		bt_audio_data_parse(codec_cfg->data, codec_cfg->data_len, log_codec_cfg_cb, "data");

		ret = bt_audio_codec_cfg_get_freq(codec_cfg);
		if (ret > 0) {
			LOG_INF("\tFrequency: %d Hz", bt_audio_codec_cfg_freq_to_freq_hz(ret));
		}

		ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
		if (ret > 0) {
			LOG_INF("\tFrame Duration: %d us",
				bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret));
		}

		if (bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation, true) ==
		    0) {
			LOG_INF("\tChannel allocation: 0x%08X", chan_allocation);
		}

		ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
		if (ret > 0) {
			LOG_INF("\tOctets per frame: %d", ret);
		}

		LOG_INF("\tFrames per SDU: %d",
			bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true));
	} else {
		LOG_HEXDUMP_DBG(codec_cfg->data, codec_cfg->data_len, "data");
	}

	bt_audio_data_parse(codec_cfg->meta, codec_cfg->meta_len, log_codec_cfg_cb, "meta");
}

static void log_qos(const struct bt_audio_codec_qos *qos)
{
	LOG_INF("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u latency %u pd %u",
		qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency, qos->pd);
}

static int unicast_server_config_cb(struct bt_conn *conn, const struct bt_bap_ep *ep,
				    enum bt_audio_dir dir,
				    const struct bt_audio_codec_cfg *codec_cfg,
				    struct bt_bap_stream **bap_stream,
				    struct bt_audio_codec_qos_pref *const pref,
				    struct bt_bap_ascs_rsp *rsp)
{
	struct bt_cap_stream *cap_stream;

	LOG_INF("ASE Codec Config: conn %p ep %p dir %u", (void *)conn, (void *)ep, dir);

	log_codec_cfg(codec_cfg);

	cap_stream = stream_alloc(dir);
	if (cap_stream == NULL) {
		LOG_WRN("No streams available");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		return -ENOMEM;
	}

	*bap_stream = &cap_stream->bap_stream;

	LOG_INF("ASE Codec Config bap_stream %p", *bap_stream);

	*pref = qos_pref;

	return 0;
}

static int unicast_server_reconfig_cb(struct bt_bap_stream *bap_stream, enum bt_audio_dir dir,
				      const struct bt_audio_codec_cfg *codec_cfg,
				      struct bt_audio_codec_qos_pref *const pref,
				      struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("ASE Codec Reconfig: bap_stream %p", bap_stream);
	log_codec_cfg(codec_cfg);
	*pref = qos_pref;

	return 0;
}

static int unicast_server_qos_cb(struct bt_bap_stream *bap_stream,
				 const struct bt_audio_codec_qos *qos, struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("QoS: bap_stream %p qos %p", bap_stream, qos);

	log_qos(qos);

	return 0;
}

static int unicast_server_enable_cb(struct bt_bap_stream *bap_stream, const uint8_t meta[],
				    size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("Enable: bap_stream %p meta_len %zu", bap_stream, meta_len);

	return 0;
}

static int unicast_server_start_cb(struct bt_bap_stream *bap_stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("Start: bap_stream %p", bap_stream);

	return 0;
}

struct data_func_param {
	struct bt_bap_ascs_rsp *rsp;
	bool stream_context_present;
};

static bool data_func_cb(struct bt_data *data, void *user_data)
{
	struct data_func_param *func_param = (struct data_func_param *)user_data;

	if (data->type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
		func_param->stream_context_present = true;
	}

	return true;
}

static int unicast_server_metadata_cb(struct bt_bap_stream *bap_stream, const uint8_t meta[],
				      size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	struct data_func_param func_param = {
		.rsp = rsp,
		.stream_context_present = false,
	};
	int err;

	LOG_INF("Metadata: bap_stream %p meta_len %zu", bap_stream, meta_len);

	err = bt_audio_data_parse(meta, meta_len, data_func_cb, &func_param);
	if (err != 0) {
		return err;
	}

	if (!func_param.stream_context_present) {
		LOG_ERR("Stream audio context not present");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
				       BT_BAP_ASCS_REASON_NONE);

		return -EINVAL;
	}

	return 0;
}

static int unicast_server_disable_cb(struct bt_bap_stream *bap_stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("Disable: bap_stream %p", bap_stream);

	return 0;
}

static int unicast_server_stop_cb(struct bt_bap_stream *bap_stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("Stop: bap_stream %p", bap_stream);

	return 0;
}

static int unicast_server_release_cb(struct bt_bap_stream *bap_stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_INF("Release: bap_stream %p", bap_stream);

	return 0;
}

static const struct bt_bap_unicast_server_cb unicast_server_cb = {
	.config = unicast_server_config_cb,
	.reconfig = unicast_server_reconfig_cb,
	.qos = unicast_server_qos_cb,
	.enable = unicast_server_enable_cb,
	.start = unicast_server_start_cb,
	.metadata = unicast_server_metadata_cb,
	.disable = unicast_server_disable_cb,
	.stop = unicast_server_stop_cb,
	.release = unicast_server_release_cb,
};

static void unicast_stream_configured_cb(struct bt_bap_stream *bap_stream,
					 const struct bt_audio_codec_qos_pref *pref)
{
	LOG_INF("Configured bap_stream %p", bap_stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */

	LOG_INF("Local preferences: unframed %s, phy %u, rtn %u, latency %u, pd_min %u, pd_max "
		"%u, pref_pd_min %u, pref_pd_max %u",
		pref->unframed_supported ? "supported" : "not supported", pref->phy, pref->rtn,
		pref->latency, pref->pd_min, pref->pd_max, pref->pref_pd_min, pref->pref_pd_max);
}

static void unicast_stream_qos_set_cb(struct bt_bap_stream *bap_stream)
{
	LOG_INF("QoS set bap_stream %p", bap_stream);
}

static void unicast_stream_enabled_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	LOG_INF("Enabled bap_stream %p", bap_stream);

	err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
	if (err != 0) {
		LOG_ERR("Failed to get ep info: %d", err);

		return;
	}

	if (ep_info.dir == BT_AUDIO_DIR_SINK) {
		/* Automatically do the receiver start ready operation */
		err = bt_bap_stream_start(bap_stream);
		if (err != 0) {
			LOG_ERR("Failed to start: %d", err);

			return;
		}
	}
}

static void unicast_stream_started_cb(struct bt_bap_stream *bap_stream)
{
	LOG_INF("Started bap_stream %p", bap_stream);
	total_unicast_rx_iso_packet_count = 0U;
}

static void unicast_stream_metadata_updated_cb(struct bt_bap_stream *bap_stream)
{
	LOG_INF("Metadata updated bap_stream %p", bap_stream);
}

static void unicast_stream_disabled_cb(struct bt_bap_stream *bap_stream)
{
	LOG_INF("Disabled bap_stream %p", bap_stream);
}

static void unicast_stream_stopped_cb(struct bt_bap_stream *bap_stream, uint8_t reason)
{
	LOG_INF("Stopped bap_stream %p with reason 0x%02X", bap_stream, reason);
}

static void unicast_stream_released_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream =
		CONTAINER_OF(bap_stream, struct bt_cap_stream, bap_stream);

	LOG_INF("Released bap_stream %p", bap_stream);

	stream_released(cap_stream);
}

static void unicast_stream_recv_cb(struct bt_bap_stream *bap_stream,
				   const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	/* Triggered every time we receive an HCI data packet from the controller.
	 * A call to this does not indicate valid data
	 * (see the `info->flags` for which flags to check),
	 */

	if ((total_unicast_rx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Received %llu HCI ISO data packets", total_unicast_rx_iso_packet_count);
	}

	total_unicast_rx_iso_packet_count++;
}

static void unicast_stream_sent_cb(struct bt_bap_stream *stream)
{
	/* Triggered every time we have sent an HCI data packet to the controller */

	if ((total_unicast_tx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Sent %llu HCI ISO data packets", total_unicast_tx_iso_packet_count);
	}

	total_unicast_tx_iso_packet_count++;
}

static void tx_thread_func(void *arg1, void *arg2, void *arg3)
{
	NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
	static uint8_t data[CONFIG_BT_ISO_TX_MTU];
	struct peer_config *peer = arg1;
	struct bt_cap_stream *cap_stream = &peer->source_stream;
	struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;

	for (size_t i = 0U; i < ARRAY_SIZE(data); i++) {
		data[i] = (uint8_t)i;
	}

	while (true) {
		/* No-op if stream is not configured */
		if (bap_stream->ep != NULL) {
			struct bt_bap_ep_info ep_info;
			int err;

			err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
			if (err == 0) {
				if (ep_info.state == BT_BAP_EP_STATE_STREAMING) {
					struct net_buf *buf;

					buf = net_buf_alloc(&tx_pool, K_FOREVER);
					net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

					net_buf_add_mem(buf, data, bap_stream->qos->sdu);

					err = bt_cap_stream_send(cap_stream, buf, peer->tx_seq_num);
					if (err == 0) {
						peer->tx_seq_num++;
						continue; /* Attempt to send again ASAP */
					} else {
						LOG_ERR("Unable to send: %d", err);
						net_buf_unref(buf);
					}
				}
			}
		}

		/* In case of any errors, retry with a delay */
		k_sleep(K_MSEC(100));
	}
}

int init_cap_acceptor_unicast(struct peer_config *peer)
{
	static struct bt_bap_stream_ops unicast_stream_ops = {
		.configured = unicast_stream_configured_cb,
		.qos_set = unicast_stream_qos_set_cb,
		.enabled = unicast_stream_enabled_cb,
		.started = unicast_stream_started_cb,
		.metadata_updated = unicast_stream_metadata_updated_cb,
		.disabled = unicast_stream_disabled_cb,
		.stopped = unicast_stream_stopped_cb,
		.released = unicast_stream_released_cb,
		.recv = unicast_stream_recv_cb,
		.sent = unicast_stream_sent_cb,
	};
	static bool cbs_registered;

	if (!cbs_registered) {
		int err;
		struct bt_bap_unicast_server_register_param param = {
			CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
			CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
		};

		err = bt_bap_unicast_server_register(&param);
		if (err != 0) {
			LOG_ERR("Failed to register BAP unicast server: %d", err);

			return -ENOEXEC;
		}

		err = bt_bap_unicast_server_register_cb(&unicast_server_cb);
		if (err != 0) {
			LOG_ERR("Failed to register BAP unicast server callbacks: %d", err);

			return -ENOEXEC;
		}

		cbs_registered = true;
	}

	bt_cap_stream_ops_register(&peer->source_stream, &unicast_stream_ops);
	bt_cap_stream_ops_register(&peer->sink_stream, &unicast_stream_ops);

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		static bool thread_started;

		if (!thread_started) {
			static K_KERNEL_STACK_DEFINE(tx_thread_stack, 1024);
			const int tx_thread_prio = K_PRIO_PREEMPT(5);
			static struct k_thread tx_thread;

			k_thread_create(&tx_thread, tx_thread_stack,
					K_KERNEL_STACK_SIZEOF(tx_thread_stack), tx_thread_func,
					peer, NULL, NULL, tx_thread_prio, 0, K_NO_WAIT);
			k_thread_name_set(&tx_thread, "TX thread");
			thread_started = true;
		}
	}

	return 0;
}
