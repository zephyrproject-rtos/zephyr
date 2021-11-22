/* @file
 * @brief Internal APIs for BAP

 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_bap_config(struct bt_audio_stream *stream,
		  struct bt_codec *codec);

int bt_bap_enable(struct bt_audio_stream *stream, uint8_t meta_count,
		  struct bt_codec_data *meta);

int bt_bap_metadata(struct bt_audio_stream *stream,
		    uint8_t meta_count, struct bt_codec_data *meta);

int bt_bap_disable(struct bt_audio_stream *stream);

int bt_bap_start(struct bt_audio_stream *stream);

int bt_bap_stop(struct bt_audio_stream *stream);

int bt_bap_release(struct bt_audio_stream *stream);
