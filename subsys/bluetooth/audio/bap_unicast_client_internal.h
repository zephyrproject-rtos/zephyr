/* @file
 * @brief Internal APIs for BAP

 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_bap_unicast_client_config(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_cfg *codec_cfg);

int bt_bap_unicast_client_qos(struct bt_conn *conn, struct bt_bap_unicast_group *group);

int bt_bap_unicast_client_enable(struct bt_bap_stream *stream, const uint8_t meta[],
				 size_t meta_len);

int bt_bap_unicast_client_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len);

int bt_bap_unicast_client_disable(struct bt_bap_stream *stream);

int bt_bap_unicast_client_start(struct bt_bap_stream *stream);

int bt_bap_unicast_client_stop(struct bt_bap_stream *stream);

int bt_bap_unicast_client_release(struct bt_bap_stream *stream);

struct net_buf_simple *bt_bap_unicast_client_ep_create_pdu(struct bt_conn *conn, uint8_t op);

int bt_bap_unicast_client_ep_qos(struct bt_bap_ep *ep, struct net_buf_simple *buf,
				 struct bt_audio_codec_qos *qos);

int bt_bap_unicast_client_ep_send(struct bt_conn *conn, struct bt_bap_ep *ep,
				  struct net_buf_simple *buf);

struct bt_bap_iso *bt_bap_unicast_client_new_audio_iso(void);
