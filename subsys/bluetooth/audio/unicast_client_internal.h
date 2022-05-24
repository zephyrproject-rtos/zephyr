/* @file
 * @brief Internal APIs for BAP

 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_unicast_client_config(struct bt_audio_stream *stream,
		  struct bt_codec *codec);

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


void bt_unicast_client_ep_set_state(struct bt_audio_ep *ep, uint8_t state);

struct net_buf_simple *bt_unicast_client_ep_create_pdu(uint8_t op);

int bt_unicast_client_ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		  struct bt_codec_qos *qos);

int bt_unicast_client_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
		   struct net_buf_simple *buf);

void bt_unicast_client_ep_attach(struct bt_audio_ep *ep, struct bt_audio_stream *stream);

void bt_unicast_client_ep_detach(struct bt_audio_ep *ep, struct bt_audio_stream *stream);

/**
 * @brief Get a new a audio_iso that is a match for the provided stream
 *
 * This will first attempt to find a audio_iso where the stream's direction
 * is not yet used, and where the opposing direction is for the same stream
 * group and ACL connection.
 *
 * If no such one is available, it will use the first completely free audio_iso
 * available, or return NULL if not such one is found.
 *
 * @param stream Pointer to a stream used to find a corresponding audio_iso
 *
 * @return An audio_iso struct fit for the stream, or NULL.
 */
struct bt_audio_iso *bt_unicast_client_new_audio_iso(
	const struct bt_audio_unicast_group *unicast_group,
	const struct bt_audio_stream *stream,
	enum bt_audio_dir dir);

struct bt_audio_iso *bt_unicast_client_audio_iso_by_stream(const struct bt_audio_stream *stream);

void bt_unicast_client_stream_bind_audio_iso(struct bt_audio_stream *stream,
					     struct bt_audio_iso *audio_iso,
					     enum bt_audio_dir dir);
void bt_unicast_client_stream_unbind_audio_iso(struct bt_audio_stream *stream);

void bt_unicast_client_ep_bind_audio_iso(struct bt_audio_ep *ep,
					 struct bt_audio_iso *audio_iso);
void bt_unicast_client_ep_unbind_audio_iso(struct bt_audio_ep *ep);
