/* @file
 * @brief Internal APIs for Audio Endpoint handling

 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

#include "ascs_internal.h"
#include "bap_stream.h"

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
#define UNICAST_GROUP_CNT	 CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_COUNT
#define UNICAST_GROUP_STREAM_CNT CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT
#else /* !CONFIG_BT_BAP_UNICAST_CLIENT */
#define UNICAST_GROUP_CNT 0
#define UNICAST_GROUP_STREAM_CNT 0
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
#define BROADCAST_STREAM_CNT CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT
#else /* !CONFIG_BT_BAP_BROADCAST_SOURCE */
#define BROADCAST_STREAM_CNT 0
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

/* Temp struct declarations to handle circular dependencies */
struct bt_bap_unicast_group;
struct bt_bap_broadcast_source;
struct bt_bap_broadcast_sink;

struct bt_bap_ep {
	uint8_t  dir;
	uint8_t  cig_id;
	uint8_t  cis_id;
	struct bt_ascs_ase_status status;
	struct bt_bap_stream *stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
	struct bt_audio_codec_qos_pref qos_pref;
	struct bt_bap_iso *iso;

	/* unicast stopped reason */
	uint8_t reason;

	/* Used by the unicast server and client */
	bool receiver_ready;

	/* TODO: Create a union to reduce memory usage */
	struct bt_bap_unicast_group *unicast_group;
	struct bt_bap_broadcast_source *broadcast_source;
	struct bt_bap_broadcast_sink *broadcast_sink;
};

struct bt_bap_unicast_group {
	uint8_t index;
	bool allocated;
	/* QoS used to create the CIG */
	const struct bt_audio_codec_qos *qos;
	struct bt_iso_cig *cig;
	/* The ISO API for CIG creation requires an array of pointers to ISO channels */
	struct bt_iso_chan *cis[UNICAST_GROUP_STREAM_CNT];
	sys_slist_t streams;
};

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
struct bt_audio_broadcast_stream_data {
	/** Codec Specific Data len */
	size_t data_len;
	/** Codec Specific Data */
	uint8_t data[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE];
};
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

struct bt_bap_broadcast_source {
	uint8_t stream_count;
	uint8_t packing;
	bool encryption;
	uint32_t broadcast_id; /* 24 bit */

	struct bt_iso_big *big;
	struct bt_audio_codec_qos *qos;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	/* Stored advanced parameters */
	uint8_t irc;
	uint8_t pto;
	uint16_t iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	/* The codec specific configured data for each stream in the subgroup */
	struct bt_audio_broadcast_stream_data stream_data[BROADCAST_STREAM_CNT];
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];

	/* The complete codec specific configured data for each stream in the subgroup.
	 * This contains both the subgroup and the BIS-specific data for each stream.
	 */
	struct bt_audio_codec_cfg codec_cfg[BROADCAST_STREAM_CNT];

	/* The subgroups containing the streams used to create the broadcast source */
	sys_slist_t subgroups;
};

enum bt_bap_broadcast_sink_flag {
	/** Sink has been initialized */
	BT_BAP_BROADCAST_SINK_FLAG_INITIALIZED,

	/** BIGInfo has been received */
	BT_BAP_BROADCAST_SINK_FLAG_BIGINFO_RECEIVED,

	/** Periodic Advertising is syncing */
	BT_BAP_BROADCAST_SINK_FLAG_SYNCING,

	/** Broadcast sink instance is scanning */
	BT_BAP_BROADCAST_SINK_FLAG_SCANNING,

	/** The BIG is encrypted */
	BT_BAP_BROADCAST_SINK_FLAG_BIG_ENCRYPTED,

	/** The Scan Delegator Source ID is valid */
	BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID,

	/** Total number of flags */
	BT_BAP_BROADCAST_SINK_FLAG_NUM_FLAGS,
};

struct bt_bap_broadcast_sink_subgroup {
	uint32_t bis_indexes;
};

struct bt_bap_broadcast_sink_bis {
	uint8_t index;
	struct bt_iso_chan *chan;
	struct bt_audio_codec_cfg codec_cfg;
};

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
struct bt_bap_broadcast_sink {
	uint8_t index; /* index of broadcast_snks array */
	uint8_t stream_count;
	uint8_t bass_src_id;
	uint8_t subgroup_count;
	uint16_t iso_interval;
	uint16_t biginfo_num_bis;
	uint32_t broadcast_id; /* 24 bit */
	uint32_t indexes_bitfield;
	uint32_t valid_indexes_bitfield; /* based on codec support */
	struct bt_audio_codec_qos codec_qos;
	struct bt_le_per_adv_sync *pa_sync;
	struct bt_iso_big *big;
	struct bt_bap_broadcast_sink_bis bis[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
	struct bt_bap_broadcast_sink_subgroup subgroups[CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT];
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	/* The streams used to create the broadcast sink */
	sys_slist_t streams;

	/** Flags */
	ATOMIC_DEFINE(flags, BT_BAP_BROADCAST_SINK_FLAG_NUM_FLAGS);
};
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */

static inline const char *bt_bap_ep_state_str(uint8_t state)
{
	switch (state) {
	case BT_BAP_EP_STATE_IDLE:
		return "idle";
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		return "codec-configured";
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		return "qos-configured";
	case BT_BAP_EP_STATE_ENABLING:
		return "enabling";
	case BT_BAP_EP_STATE_STREAMING:
		return "streaming";
	case BT_BAP_EP_STATE_DISABLING:
		return "disabling";
	case BT_BAP_EP_STATE_RELEASING:
		return "releasing";
	default:
		return "unknown";
	}
}

bool bt_bap_ep_is_broadcast_snk(const struct bt_bap_ep *ep);
bool bt_bap_ep_is_broadcast_src(const struct bt_bap_ep *ep);
bool bt_bap_ep_is_unicast_client(const struct bt_bap_ep *ep);
