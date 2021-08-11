/* @file
 * @brief Internal APIs for Audio Endpoint handling

 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ascs_internal.h"

#if defined(CONFIG_BT_BAP)
#define BROADCAST_SRC_CNT CONFIG_BT_BAP_BROADCAST_SRC_COUNT
#define BROADCAST_STREAM_CNT CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT
#define BROADCAST_SNK_CNT CONFIG_BT_BAP_BROADCAST_SNK_COUNT
#else
#define BROADCAST_SRC_CNT 0
#define BROADCAST_STREAM_CNT 0
#define BROADCAST_SNK_CNT 0
#endif

struct bt_audio_ep_cb {
	sys_snode_t node;
	void (*status_changed)(struct bt_audio_ep *ep, uint8_t old_state,
			       uint8_t state);
};

#define BT_AUDIO_EP_LOCAL	0x00
#define BT_AUDIO_EP_REMOTE	0x01

struct bt_audio_broadcaster {
	uint8_t bis_count;
	uint8_t subgroup_count;
	uint32_t pd; /** QoS Presentation Delay */
	uint32_t broadcast_id; /* 24 bit */
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	struct bt_iso_chan *bis[BROADCAST_STREAM_CNT];
};

struct bt_audio_broadcast_sink {
	uint8_t index; /* index of broadcast_snks array */
	uint8_t bis_count;
	uint8_t subgroup_count;
	uint16_t pa_interval;
	uint16_t iso_interval;
	uint16_t biginfo_num_bis;
	bool biginfo_received;
	bool syncing;
	bool big_encrypted;
	uint32_t broadcast_id; /* 24 bit */
	struct bt_le_per_adv_sync *pa_sync;
	struct bt_audio_capability *cap; /* Capability that accepted the PA sync */
	struct bt_iso_big *big;
	struct bt_iso_chan *bis[BROADCAST_SNK_STREAM_CNT];
};

struct bt_audio_ep {
	uint8_t  type;
	uint16_t handle;
	uint16_t cp_handle;
	uint8_t  cig;
	uint8_t  cis;
	struct bt_ascs_ase_status status;
	struct bt_audio_capability *cap;
	struct bt_audio_chan *chan;
	struct bt_codec codec;
	struct bt_codec_qos qos;
	struct bt_iso_chan iso;
	struct bt_iso_chan_qos iso_qos;
	struct bt_iso_chan_io_qos iso_tx;
	struct bt_iso_chan_io_qos iso_rx;
	sys_slist_t cbs;
	struct bt_gatt_subscribe_params subscribe;
	struct bt_gatt_discover_params discover;

	/* Broadcast fields */
	/* TODO: Create a union to reduce memory usage */
	struct bt_audio_broadcaster *broadcaster;
	struct bt_audio_broadcast_sink *broadcast_sink;
};

static inline const char *bt_audio_ep_state_str(uint8_t state)
{
	switch (state) {
	case BT_ASCS_ASE_STATE_IDLE:
		return "idle";
	case BT_ASCS_ASE_STATE_CONFIG:
		return "codec-configured";
	case BT_ASCS_ASE_STATE_QOS:
		return "qos-configured";
	case BT_ASCS_ASE_STATE_ENABLING:
		return "enabling";
	case BT_ASCS_ASE_STATE_STREAMING:
		return "streaming";
	case BT_ASCS_ASE_STATE_DISABLING:
		return "disabling";
	case BT_ASCS_ASE_STATE_RELEASING:
		return "releasing";
	default:
		return "unknown";
	}
}

void bt_audio_ep_init(struct bt_audio_ep *ep, uint8_t type, uint16_t handle,
		      uint8_t id);

struct bt_audio_ep *bt_audio_ep_find(struct bt_conn *conn, uint16_t handle);

struct bt_audio_ep *bt_audio_ep_find_by_chan(struct bt_audio_chan *chan);

struct bt_audio_ep *bt_audio_ep_alloc(struct bt_conn *conn, uint16_t handle);

struct bt_audio_ep *bt_audio_ep_get(struct bt_conn *conn, uint8_t dir,
				    uint16_t handle);

void bt_audio_ep_set_status(struct bt_audio_ep *ep, struct net_buf_simple *buf);

int bt_audio_ep_get_status(struct bt_audio_ep *ep, struct net_buf_simple *buf);

void bt_audio_ep_register_cb(struct bt_audio_ep *ep, struct bt_audio_ep_cb *cb);

void bt_audio_ep_set_state(struct bt_audio_ep *ep, uint8_t state);

int bt_audio_ep_subscribe(struct bt_conn *conn, struct bt_audio_ep *ep);

void bt_audio_ep_set_cp(struct bt_conn *conn, uint16_t handle);

void bt_audio_ep_reset(struct bt_conn *conn);

void bt_audio_ep_attach(struct bt_audio_ep *ep,
			      struct bt_audio_chan *chan);
void bt_audio_ep_detach(struct bt_audio_ep *ep,
			      struct bt_audio_chan *chan);

struct net_buf_simple *bt_audio_ep_create_pdu(uint8_t op);

int bt_audio_ep_set_codec(struct bt_audio_ep *ep, uint8_t id, uint16_t cid,
			  uint16_t vid, struct net_buf_simple *buf,
			  uint8_t len, struct bt_codec *codec);

int bt_audio_ep_set_metadata(struct bt_audio_ep *ep, struct net_buf_simple *buf,
			     uint8_t len, struct bt_codec *codec);

int bt_audio_ep_config(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		       struct bt_audio_capability *cap, struct bt_codec *codec);

int bt_audio_ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		    uint8_t cig, uint8_t cis, struct bt_codec_qos *qos);

int bt_audio_ep_enable(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		       uint8_t meta_count, struct bt_codec_data *meta);

int bt_audio_ep_metadata(struct bt_audio_ep *ep, struct net_buf_simple *buf,
			 uint8_t meta_count, struct bt_codec_data *meta);

int bt_audio_ep_start(struct bt_audio_ep *ep, struct net_buf_simple *buf);

int bt_audio_ep_disable(struct bt_audio_ep *ep, struct net_buf_simple *buf);

int bt_audio_ep_stop(struct bt_audio_ep *ep, struct net_buf_simple *buf);

int bt_audio_ep_release(struct bt_audio_ep *ep, struct net_buf_simple *buf);

int bt_audio_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
		     struct net_buf_simple *buf);

struct bt_audio_ep *bt_audio_ep_broadcaster_new(uint8_t index, uint8_t dir);
bool bt_audio_ep_is_broadcast_snk(const struct bt_audio_ep *ep);
bool bt_audio_ep_is_broadcast_src(const struct bt_audio_ep *ep);
bool bt_audio_ep_is_broadcast(const struct bt_audio_ep *ep);
