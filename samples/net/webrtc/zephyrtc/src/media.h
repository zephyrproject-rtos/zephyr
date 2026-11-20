/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_DEMO_MEDIA_H
#define ZEPHYRTC_DEMO_MEDIA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum {
	MEDIA_AUDIO,
	MEDIA_VIDEO
} MediaKind;
/*
 * Borrowed encoded bytes: PCMU/8000 mono, 20 ms; H264 Annex B AU. No
 * GstBuffer, RTP, transport, or OS handle crosses this boundary.
 */
typedef struct {
	const uint8_t *data;
	size_t size;
	uint64_t timestamp_ns;
	uint64_t duration_ns;
} MediaFrame;
typedef struct Media Media;
typedef struct {
	bool test_sources;
	bool fake_sinks;
	bool audio_only;    /* Disable video capture, encode, receive and decode. */
	uint32_t video_fps; /* 1..30; zero selects 30 fps. */
} MediaConfig;
Media *media_create(const MediaConfig *config);
bool media_enabled(const Media *media, MediaKind kind);
int media_start(Media *media);
int media_poll(Media *media); /* Surface asynchronous codec/source errors. */
bool media_pull(Media *media, MediaKind kind, MediaFrame *frame);
void media_release(Media *media, MediaKind kind);
int media_receive(Media *media, MediaKind kind, const MediaFrame *frame);
void media_request_keyframe(Media *media);
bool media_take_keyframe_request(Media *media);
uint64_t media_decoded(Media *media, MediaKind kind);
int media_destroy(Media *media);
#endif
