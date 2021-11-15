/* @file
 * @brief Internal APIs for BAP

 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bap_config(struct bt_audio_stream *stream,
	       struct bt_codec *codec);

int bap_enable(struct bt_audio_stream *stream, uint8_t meta_count,
	       struct bt_codec_data *meta);

int bap_metadata(struct bt_audio_stream *stream,
		 uint8_t meta_count, struct bt_codec_data *meta);

int bap_disable(struct bt_audio_stream *stream);

int bap_start(struct bt_audio_stream *stream);

int bap_stop(struct bt_audio_stream *stream);
