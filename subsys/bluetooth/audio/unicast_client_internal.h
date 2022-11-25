/* @file
 * @brief Internal APIs for BAP

 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_unicast_client_config(struct bt_audio_stream *stream,
			     const struct bt_codec *codec);

int bt_unicast_client_enable(struct bt_audio_stream *stream,
			     struct bt_codec_data *meta,
			     size_t meta_count);

int bt_unicast_client_metadata(struct bt_audio_stream *stream,
			       struct bt_codec_data *meta,
			       size_t meta_count);

int bt_unicast_client_disable(struct bt_audio_stream *stream);

int bt_unicast_client_start(struct bt_audio_stream *stream);

int bt_unicast_client_stop(struct bt_audio_stream *stream);

int bt_unicast_client_release(struct bt_audio_stream *stream);

struct net_buf_simple *bt_unicast_client_ep_create_pdu(uint8_t op);

int bt_unicast_client_ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		  struct bt_codec_qos *qos);

int bt_unicast_client_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
		   struct net_buf_simple *buf);

struct bt_audio_iso *bt_unicast_client_new_audio_iso(void);
