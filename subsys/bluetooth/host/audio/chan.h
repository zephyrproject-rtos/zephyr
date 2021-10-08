/* @file
 * @brief Internal APIs for Audio Channel handling

 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP)
#define SNK_SIZE CONFIG_BT_BAP_ASE_SNK_COUNT
#define SRC_SIZE CONFIG_BT_BAP_ASE_SRC_COUNT
#else
#define SNK_SIZE 0
#define SRC_SIZE 0
#endif

/** Life-span states of ASE. Used only by internal APIs
 *  dealing with setting ASE to proper state depending on operational
 *  context.
 *
 * The values are defined by the ASCS spec and shall not be changed.
 */
enum bt_audio_state {
	/** Audio Stream Endpoint Idle state */
	BT_AUDIO_EP_STATE_IDLE =             0x00,
	/** Audio Stream Endpoint Codec Configured state */
	BT_AUDIO_EP_STATE_CODEC_CONFIGURED = 0x01,
	/** Audio Stream Endpoint QoS Configured state */
	BT_AUDIO_EP_STATE_QOS_CONFIGURED =   0x02,
	/** Audio Stream Endpoint Enabling state */
	BT_AUDIO_EP_STATE_ENABLING =         0x03,
	/** Audio Stream Endpoint Streaming state */
	BT_AUDIO_EP_STATE_STREAMING =        0x04,
	/** Audio Stream Endpoint Disabling state */
	BT_AUDIO_EP_STATE_DISABLING =        0x05,
	/** Audio Stream Endpoint Streaming state */
	BT_AUDIO_EP_STATE_RELEASING =        0x06,
};

/* Bind ISO channel */
struct bt_conn_iso *bt_audio_cig_create(struct bt_audio_chan *chan,
					struct bt_codec_qos *qos);

/* Unbind ISO channel */
int bt_audio_cig_terminate(struct bt_audio_chan *chan);

/* Connect ISO channel */
int bt_audio_chan_connect(struct bt_audio_chan *chan);

/* Disconnect ISO channel */
int bt_audio_chan_disconnect(struct bt_audio_chan *chan);

void bt_audio_chan_reset(struct bt_audio_chan *chan);

void bt_audio_chan_attach(struct bt_conn *conn, struct bt_audio_chan *chan,
			  struct bt_audio_ep *ep,
			  struct bt_audio_capability *cap,
			  struct bt_codec *codec);

int bt_audio_chan_codec_qos_to_iso_qos(struct bt_iso_chan_qos *qos,
				       struct bt_codec_qos *codec);

void bt_audio_chan_detach(struct bt_audio_chan *chan);
