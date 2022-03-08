/* @file
 * @brief Internal APIs for BAP

 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_unicast_client_config(struct bt_audio_stream *stream,
		  struct bt_codec *codec);

int bt_unicast_client_enable(struct bt_audio_stream *stream, uint8_t meta_count,
		  struct bt_codec_data *meta);

int bt_unicast_client_metadata(struct bt_audio_stream *stream,
		    uint8_t meta_count, struct bt_codec_data *meta);

int bt_unicast_client_disable(struct bt_audio_stream *stream);

int bt_unicast_client_start(struct bt_audio_stream *stream);

int bt_unicast_client_stop(struct bt_audio_stream *stream);

int bt_unicast_client_release(struct bt_audio_stream *stream);

bool bt_unicast_client_ep_is_src(const struct bt_audio_ep *ep);

void bt_unicast_client_ep_set_state(struct bt_audio_ep *ep, uint8_t state);

struct net_buf_simple *bt_unicast_client_ep_create_pdu(uint8_t op);

int bt_unicast_client_ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		  struct bt_codec_qos *qos);

int bt_unicast_client_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
		   struct net_buf_simple *buf);

void bt_unicast_client_ep_attach(struct bt_audio_ep *ep, struct bt_audio_stream *stream);

void bt_unicast_client_ep_detach(struct bt_audio_ep *ep, struct bt_audio_stream *stream);
