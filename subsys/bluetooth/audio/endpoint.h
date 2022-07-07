/* @file
 * @brief Internal APIs for Audio Endpoint handling

 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ascs_internal.h"
#include "stream.h"

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT) && defined(CONFIG_BT_AUDIO_UNICAST)
#define UNICAST_GROUP_CNT CONFIG_BT_AUDIO_UNICAST_CLIENT_GROUP_COUNT
#define UNICAST_GROUP_STREAM_CNT CONFIG_BT_AUDIO_UNICAST_CLIENT_GROUP_STREAM_COUNT
#else /* !(CONFIG_BT_AUDIO_UNICAST_CLIENT && CONFIG_BT_AUDIO_UNICAST) */
#define UNICAST_GROUP_CNT 0
#define UNICAST_GROUP_STREAM_CNT 0
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT && CONFIG_BT_AUDIO_UNICAST */
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
#define BROADCAST_STREAM_CNT CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT
#else /* !CONFIG_BT_AUDIO_BROADCAST_SOURCE */
#define BROADCAST_STREAM_CNT 0
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

/* Temp struct declarations to handle circular dependencies */
struct bt_audio_unicast_group;
struct bt_audio_broadcast_source;
struct bt_audio_broadcast_sink;
struct bt_audio_ep;

struct bt_audio_iso {
	struct bt_iso_chan iso_chan;
	struct bt_iso_chan_qos iso_qos;
	struct bt_audio_ep *sink_ep;
	struct bt_audio_ep *source_ep;
};

struct bt_audio_ep {
	uint8_t  dir;
	uint16_t handle;
	uint16_t cp_handle;
	uint8_t  cig_id;
	uint8_t  cis_id;
	/* ISO sequence number */
	uint32_t seq_num;
	struct bt_ascs_ase_status status;
	struct bt_audio_stream *stream;
	struct bt_codec codec;
	struct bt_codec_qos qos;
	struct bt_codec_qos_pref qos_pref;
	struct bt_audio_iso *iso;
	struct bt_iso_chan_io_qos iso_io_qos;
	struct bt_gatt_subscribe_params subscribe;
	struct bt_gatt_discover_params discover;

	/* TODO: Create a union to reduce memory usage */
	struct bt_audio_unicast_group *unicast_group;
	struct bt_audio_broadcast_source *broadcast_source;
	struct bt_audio_broadcast_sink *broadcast_sink;
};

struct bt_audio_unicast_group {
	bool allocated;
	/* QoS used to create the CIG */
	struct bt_codec_qos *qos;
	struct bt_iso_cig *cig;
	/* The ISO API for CIG creation requires an array of pointers to ISO channels */
	struct bt_iso_chan *cis[UNICAST_GROUP_STREAM_CNT];
	sys_slist_t streams;
};

struct bt_audio_broadcast_source {
	uint8_t stream_count;
	uint8_t subgroup_count;
	uint32_t pd; /** QoS Presentation Delay */
	uint32_t broadcast_id; /* 24 bit */

	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	struct bt_iso_chan *bis[BROADCAST_STREAM_CNT];
	struct bt_codec_qos *qos;
	/* The streams used to create the broadcast source */
	struct bt_audio_stream **streams;
};

struct bt_audio_broadcast_sink {
	uint8_t index; /* index of broadcast_snks array */
	uint8_t stream_count;
	uint8_t subgroup_count;
	uint16_t pa_interval;
	uint16_t iso_interval;
	uint16_t biginfo_num_bis;
	bool biginfo_received;
	bool syncing;
	bool big_encrypted;
	uint32_t broadcast_id; /* 24 bit */
	struct bt_le_per_adv_sync *pa_sync;
	struct bt_codec *codec;
	struct bt_iso_big *big;
	struct bt_iso_chan *bis[BROADCAST_SNK_STREAM_CNT];
	/* The streams used to create the broadcast sink */
	struct bt_audio_stream **streams;
};

static inline const char *bt_audio_ep_state_str(uint8_t state)
{
	switch (state) {
	case BT_AUDIO_EP_STATE_IDLE:
		return "idle";
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		return "codec-configured";
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		return "qos-configured";
	case BT_AUDIO_EP_STATE_ENABLING:
		return "enabling";
	case BT_AUDIO_EP_STATE_STREAMING:
		return "streaming";
	case BT_AUDIO_EP_STATE_DISABLING:
		return "disabling";
	case BT_AUDIO_EP_STATE_RELEASING:
		return "releasing";
	default:
		return "unknown";
	}
}

bool bt_audio_ep_is_broadcast_snk(const struct bt_audio_ep *ep);
bool bt_audio_ep_is_broadcast_src(const struct bt_audio_ep *ep);
