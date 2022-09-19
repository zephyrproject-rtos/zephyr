/* @file
 * @brief Internal APIs for Audio Stream handling

 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

/* Connect ISO channel */
int bt_audio_stream_connect(struct bt_audio_stream *stream);

/* Disconnect ISO channel */
int bt_audio_stream_disconnect(struct bt_audio_stream *stream);

void bt_audio_stream_reset(struct bt_audio_stream *stream);

void bt_audio_stream_attach(struct bt_conn *conn, struct bt_audio_stream *stream,
			    struct bt_audio_ep *ep,
			    struct bt_codec *codec);

void bt_audio_codec_to_iso_path(struct bt_iso_chan_path *path,
				const struct bt_codec *codec);
void bt_audio_codec_qos_to_iso_qos(struct bt_iso_chan_io_qos *io,
				   const struct bt_codec_qos *codec_qos);

void bt_audio_stream_detach(struct bt_audio_stream *stream);

bool bt_audio_valid_qos(const struct bt_codec_qos *qos);

bool bt_audio_valid_stream_qos(const struct bt_audio_stream *stream,
			     const struct bt_codec_qos *qos);

int bt_audio_stream_iso_listen(struct bt_audio_stream *stream);
