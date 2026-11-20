/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/util.h>

#include "media.h"
#include "media_stats_zephyr.h"
#include "platform.h"
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
#include "video_codec_zephyr.h"
#endif

#define AUDIO_FRAME_BYTES          160U
#define AUDIO_FRAME_MS             20U
#define AUDIO_DURATION_NS          UINT64_C(20000000)
#define BEEP_PERIOD_FRAMES         100U
#define BEEP_ACTIVE_FRAMES         5U
#define DDS_PHASE_INCREMENT_440_HZ UINT32_C(236223201)
#define VIDEO_FRAME_MS             200U
#define VIDEO_DURATION_NS          UINT64_C(200000000)

struct Media {
	uint8_t audio[AUDIO_FRAME_BYTES];
	uint32_t phase;
	uint64_t next_frame_ms;
	uint64_t frame_index;
	uint64_t generated_audio;
	uint64_t late_audio;
	uint64_t decoded_audio;
	uint64_t next_video_ms;
	uint64_t video_index;
	uint64_t generated_video;
	uint64_t decoded_video;
	bool video_enabled;
	bool failed;
	bool started;
	bool borrowed[2];
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	uint8_t video_i420[VIDEO_CODEC_I420_BYTES] __aligned(64);
#endif
};

static struct Media synthetic_media;

static const int16_t sine_32[] = {
	0,     799,   1567,  2276,  2896,  3406,  3784,  4017,  4095,  4017,  3784,
	3406,  2896,  2276,  1567,  799,   0,     -799,  -1567, -2276, -2896, -3406,
	-3784, -4017, -4095, -4017, -3784, -3406, -2896, -2276, -1567, -799,
};

static uint8_t linear_to_mulaw(int16_t input)
{
	static const int16_t segment_end[] = {0x00ff, 0x01ff, 0x03ff, 0x07ff,
					      0x0fff, 0x1fff, 0x3fff, 0x7fff};
	int32_t sample = input;
	int32_t mask;
	int32_t segment;
	uint8_t value;

	if (sample < 0) {
		sample = 0x84 - sample;
		mask = 0x7f;
	} else {
		sample += 0x84;
		mask = 0xff;
	}
	if (sample > 32635) {
		sample = 32635;
	}
	for (segment = 0; segment < 8; ++segment) {
		if (sample <= segment_end[segment]) {
			break;
		}
	}
	if (segment >= 8) {
		return (uint8_t)(0x7f ^ mask);
	}
	value = (uint8_t)((segment << 4) | ((sample >> (segment + 3)) & 0x0f));
	return (uint8_t)(value ^ mask);
}

static void generate_audio(struct Media *media)
{
	const uint64_t frame_in_period = media->frame_index % BEEP_PERIOD_FRAMES;
	const bool active = frame_in_period < BEEP_ACTIVE_FRAMES;

	if (frame_in_period == 0U) {
		media->phase = 0U;
	}
	for (size_t i = 0; i < sizeof(media->audio); ++i) {
		int16_t sample = 0;

		if (active) {
			sample = sine_32[media->phase >> 27];
			media->phase += DDS_PHASE_INCREMENT_440_HZ;
		}
		media->audio[i] = linear_to_mulaw(sample);
	}
}

Media *media_create(const MediaConfig *config)
{
	if (config == NULL || !config->test_sources || !config->fake_sinks ||
	    (!config->audio_only && !IS_ENABLED(CONFIG_ZEPHYRTC_VIDEO_H264))) {
		return NULL;
	}
	memset(&synthetic_media, 0, sizeof(synthetic_media));
	synthetic_media.video_enabled = !config->audio_only;
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (synthetic_media.video_enabled && video_codec_init() != 0) {
		return NULL;
	}
#endif
	return &synthetic_media;
}

bool media_enabled(const Media *media, MediaKind kind)
{
	return media != NULL &&
	       (kind == MEDIA_AUDIO || (kind == MEDIA_VIDEO && media->video_enabled));
}

int media_start(Media *media)
{
	if (media == NULL || media->started) {
		return -EINVAL;
	}
	media->next_frame_ms = platform_now_ms();
	media->next_video_ms = media->next_frame_ms;
	media->started = true;
	return 0;
}

int media_poll(Media *media)
{
	return media == NULL ? -EINVAL : media->failed ? -EIO : 0;
}

#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
static void generate_video(struct Media *media)
{
	uint8_t *y = media->video_i420;
	uint8_t *u = y + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT;
	uint8_t *v = u + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT / 4U;

	for (uint32_t row = 0U; row < VIDEO_CODEC_HEIGHT; row++) {
		for (uint32_t column = 0U; column < VIDEO_CODEC_WIDTH; column++) {
			y[row * VIDEO_CODEC_WIDTH + column] =
				(uint8_t)((row + column + media->video_index * 5U) & 0xffU);
		}
	}
	memset(u, (int)(96U + media->video_index % 32U),
	       VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT / 4U);
	memset(v, (int)(160U - media->video_index % 32U),
	       VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT / 4U);
}

static bool pull_video(Media *media, MediaFrame *frame)
{
	const uint8_t *encoded;
	size_t encoded_size;
	uint64_t now;

	if (!media->video_enabled || media->borrowed[MEDIA_VIDEO]) {
		return false;
	}
	now = platform_now_ms();
	if (now < media->next_video_ms) {
		return false;
	}
	generate_video(media);
	if (video_codec_encode(media->video_i420, &encoded, &encoded_size) != 0) {
		media->failed = true;
		return false;
	}
	*frame = (MediaFrame){
		.data = encoded,
		.size = encoded_size,
		.timestamp_ns = media->video_index * VIDEO_DURATION_NS,
		.duration_ns = VIDEO_DURATION_NS,
	};
	media->video_index++;
	media->generated_video++;
	media->next_video_ms += VIDEO_FRAME_MS;
	media->borrowed[MEDIA_VIDEO] = true;
	return true;
}
#endif

bool media_pull(Media *media, MediaKind kind, MediaFrame *frame)
{
	uint64_t now;
	uint64_t missed;

	if (media == NULL || frame == NULL || !media->started) {
		return false;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (kind == MEDIA_VIDEO) {
		return pull_video(media, frame);
	}
#endif
	if (kind != MEDIA_AUDIO || media->borrowed[MEDIA_AUDIO]) {
		return false;
	}
	now = platform_now_ms();
	if (now < media->next_frame_ms) {
		return false;
	}
	missed = (now - media->next_frame_ms) / AUDIO_FRAME_MS;
	if (missed > 0U) {
		media->late_audio += missed;
		media->frame_index += missed;
		media->next_frame_ms += missed * AUDIO_FRAME_MS;
	}
	generate_audio(media);
	*frame = (MediaFrame){
		.data = media->audio,
		.size = sizeof(media->audio),
		.timestamp_ns = media->frame_index * AUDIO_DURATION_NS,
		.duration_ns = AUDIO_DURATION_NS,
	};
	media->frame_index++;
	media->generated_audio++;
	media->next_frame_ms += AUDIO_FRAME_MS;
	media->borrowed[MEDIA_AUDIO] = true;
	return true;
}

void media_release(Media *media, MediaKind kind)
{
	if (media != NULL && (kind == MEDIA_AUDIO || kind == MEDIA_VIDEO)) {
		media->borrowed[kind] = false;
	}
}

int media_receive(Media *media, MediaKind kind, const MediaFrame *frame)
{
	if (media == NULL || frame == NULL || frame->data == NULL || frame->size == 0U) {
		return -EINVAL;
	}
	if (kind == MEDIA_AUDIO) {
		media->decoded_audio++;
		return 0;
	}
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
	if (kind == MEDIA_VIDEO && media->video_enabled) {
		const uint8_t *i420;
		size_t i420_size;
		bool picture_ready;
		int result = video_codec_decode(frame->data, frame->size, 0U, &i420, &i420_size,
						&picture_ready);

		if (result != 0) {
			media->failed = true;
			return result;
		}
		if (picture_ready) {
			if (i420 == NULL || i420_size != VIDEO_CODEC_I420_BYTES) {
				media->failed = true;
				return -EBADMSG;
			}
			media->decoded_video++;
		}
		return 0;
	}
#endif
	return -EINVAL;
}

void media_request_keyframe(Media *media)
{
	(void)media;
}

bool media_take_keyframe_request(Media *media)
{
	(void)media;
	return false;
}

uint64_t media_decoded(Media *media, MediaKind kind)
{
	if (media == NULL) {
		return 0U;
	}
	return kind == MEDIA_AUDIO   ? media->decoded_audio
	       : kind == MEDIA_VIDEO ? media->decoded_video
				     : 0U;
}

uint64_t media_zephyr_generated_audio(void)
{
	return synthetic_media.generated_audio;
}

uint64_t media_zephyr_late_audio(void)
{
	return synthetic_media.late_audio;
}

int media_zephyr_quiesce(void)
{
	return 0;
}

int media_destroy(Media *media)
{
	if (media != NULL) {
#if defined(CONFIG_ZEPHYRTC_VIDEO_H264)
		if (media->video_enabled) {
			video_codec_deinit();
		}
#endif
		memset(media, 0, sizeof(*media));
	}
	return 0;
}
