/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_VIDEO_CODEC_ZEPHYR_H_
#define ZEPHYRTC_VIDEO_CODEC_ZEPHYR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VIDEO_CODEC_WIDTH         320U
#define VIDEO_CODEC_HEIGHT        240U
#define VIDEO_CODEC_I420_BYTES    (VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT * 3U / 2U)
#define VIDEO_CODEC_MAX_NAL_BYTES 32768U

struct video_codec_stats {
	uint64_t decode_errors;
	uint64_t decoder_resets;
	uint64_t awaiting_idr_drops;
	uint64_t recovered_idrs;
	uint64_t concealed_pictures;
	uint64_t parameter_set_replays;
};

int video_codec_init(void);
int video_codec_encode(const uint8_t *i420, const uint8_t **encoded, size_t *encoded_size);
int video_codec_decode(const uint8_t *annex_b_nal, size_t size, uint64_t sequence,
		       const uint8_t **i420, size_t *i420_size, bool *picture_ready);
bool video_codec_take_keyframe_request(void);
void video_codec_get_stats(struct video_codec_stats *stats);
void video_codec_deinit(void);

#endif /* ZEPHYRTC_VIDEO_CODEC_ZEPHYR_H_ */
