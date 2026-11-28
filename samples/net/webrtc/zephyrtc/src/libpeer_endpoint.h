/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_ZEPHYR_ENDPOINT_H_
#define ZEPHYRTC_ZEPHYR_ENDPOINT_H_

#include <peer_connection.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define ZEPHYRTC_ENDPOINT_AUDIO_BYTES       160U
#define ZEPHYRTC_ENDPOINT_AUDIO_FRAMES      25
#define ZEPHYRTC_ENDPOINT_TX_QUEUE_DEPTH    16U
#define ZEPHYRTC_ENDPOINT_VIDEO_BYTES       32768U
#define ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH 4U
#define ZEPHYRTC_ENDPOINT_STACK_SIZE        16384U
#define ZEPHYRTC_CONNECT_TIMEOUT_MS         15000U
#define ZEPHYRTC_MEDIA_TIMEOUT_MS           5000U
#define ZEPHYRTC_ENDPOINT_THREAD_PRIO       K_PRIO_PREEMPT(5)

/*
 * One libpeer PCMU PeerConnection plus its loop thread and received-audio
 * accounting. The connection is not thread safe: exactly one thread may call
 * into it at a time. The owner uses it until libpeer_endpoint_start(), after
 * which only the loop thread does.
 */
struct libpeer_audio_frame {
	uint8_t data[ZEPHYRTC_ENDPOINT_AUDIO_BYTES];
};

struct libpeer_video_frame {
	size_t size;
	uint8_t data[ZEPHYRTC_ENDPOINT_VIDEO_BYTES];
};

typedef void (*libpeer_endpoint_audio_sink_fn)(const uint8_t *data, size_t size, void *user_data);
typedef void (*libpeer_endpoint_video_sink_fn)(const uint8_t *data, size_t size, void *user_data);
typedef void (*libpeer_endpoint_keyframe_fn)(void *user_data);

struct libpeer_endpoint {
	PeerConnection *connection;
	const char *name;
	uint8_t send_byte;
	uint8_t expected_byte;
	bool validate_pattern;
	libpeer_endpoint_audio_sink_fn audio_sink;
	libpeer_endpoint_video_sink_fn video_sink;
	libpeer_endpoint_keyframe_fn keyframe_request;
	void *audio_sink_user_data;
	void *video_sink_user_data;
	bool video_enabled;
	atomic_t state;
	atomic_t stop;
	atomic_t received_frames;
	atomic_t received_bytes;
	atomic_t received_video_nalus;
	atomic_t received_video_bytes;
	atomic_t invalid_frames;
	atomic_t send_failures;
	atomic_t keyframe_requests;
	struct k_msgq tx_queue;
	char tx_queue_buffer[ZEPHYRTC_ENDPOINT_TX_QUEUE_DEPTH * sizeof(struct libpeer_audio_frame)]
		__aligned(4);
	struct k_msgq video_tx_queue;
	char video_tx_queue_buffer[ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH *
				   sizeof(struct libpeer_video_frame *)] __aligned(sizeof(void *));
	struct k_msgq video_free_queue;
	char video_free_queue_buffer[ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH *
				     sizeof(struct libpeer_video_frame *)]
		__aligned(sizeof(void *));
	struct libpeer_video_frame video_storage[ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH];
	struct k_thread thread;
	bool thread_started;
};

/* Initialize libpeer, libSRTP, and the DTLS-SRTP stack once per run. */
int libpeer_stack_init(void);
void libpeer_stack_deinit(void);

/* Create a PCMU-only connection. Pattern validation is optional for real peers.
 */
int libpeer_endpoint_init(struct libpeer_endpoint *endpoint, const char *name, uint8_t send_byte,
			  uint8_t expected_byte, bool validate_pattern, bool video_enabled);
int libpeer_endpoint_start(struct libpeer_endpoint *endpoint, k_thread_stack_t *stack,
			   size_t stack_size);
int libpeer_endpoint_set_audio_sink(struct libpeer_endpoint *endpoint,
				    libpeer_endpoint_audio_sink_fn sink, void *user_data);
int libpeer_endpoint_set_video_sink(struct libpeer_endpoint *endpoint,
				    libpeer_endpoint_video_sink_fn sink,
				    libpeer_endpoint_keyframe_fn keyframe_request, void *user_data);
int libpeer_endpoint_send_audio(struct libpeer_endpoint *endpoint, const uint8_t *data,
				size_t size);
int libpeer_endpoint_send_video(struct libpeer_endpoint *endpoint, const uint8_t *data,
				size_t size);
int libpeer_endpoint_request_keyframe(struct libpeer_endpoint *endpoint);
void libpeer_endpoint_stop(struct libpeer_endpoint *endpoint);
void libpeer_endpoint_destroy(struct libpeer_endpoint *endpoint);
bool libpeer_endpoint_failed(const struct libpeer_endpoint *endpoint);
int libpeer_endpoint_wait_connected(struct libpeer_endpoint *endpoint, uint32_t timeout_ms);

/*
 * Wait for both endpoints to connect, then exchange and validate
 * ZEPHYRTC_ENDPOINT_AUDIO_FRAMES.
 */
int libpeer_pair_wait_connected(struct libpeer_endpoint *first, struct libpeer_endpoint *second);
int libpeer_pair_exchange_audio(struct libpeer_endpoint *first, struct libpeer_endpoint *second);

#endif /* ZEPHYRTC_ZEPHYR_ENDPOINT_H_ */
