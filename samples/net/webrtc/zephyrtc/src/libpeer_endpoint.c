/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "libpeer_endpoint.h"

#include <errno.h>
#include <string.h>

#include "platform.h"

int peer_init(void);
void peer_deinit(void);

int libpeer_stack_init(void)
{
	return peer_init();
}

void libpeer_stack_deinit(void)
{
	peer_deinit();
}

static void state_changed(PeerConnectionState state, void *user_data)
{
	struct libpeer_endpoint *endpoint = user_data;

	atomic_set(&endpoint->state, (atomic_val_t)state);
	platform_log("info", "endpoint_state", "peer=%s state=%s", endpoint->name,
		     peer_connection_state_to_string(state));
}

static void audio_received(uint8_t *data, size_t size, void *user_data)
{
	struct libpeer_endpoint *endpoint = user_data;
	bool valid = size == ZEPHYRTC_ENDPOINT_AUDIO_BYTES;

	for (size_t index = 0U; endpoint->validate_pattern && valid && index < size; index++) {
		if (data[index] != endpoint->expected_byte) {
			valid = false;
		}
	}
	if (!valid) {
		atomic_inc(&endpoint->invalid_frames);
		return;
	}
	atomic_inc(&endpoint->received_frames);
	atomic_add(&endpoint->received_bytes, (atomic_val_t)size);
	if (endpoint->audio_sink != NULL) {
		endpoint->audio_sink(data, size, endpoint->audio_sink_user_data);
	}
}

static void video_received(uint8_t *data, size_t size, void *user_data)
{
	struct libpeer_endpoint *endpoint = user_data;

	if (!endpoint->video_enabled || data == NULL || size == 0U ||
	    size > ZEPHYRTC_ENDPOINT_VIDEO_BYTES) {
		atomic_inc(&endpoint->invalid_frames);
		return;
	}
	atomic_inc(&endpoint->received_video_nalus);
	atomic_add(&endpoint->received_video_bytes, (atomic_val_t)size);
	if (endpoint->video_sink != NULL) {
		endpoint->video_sink(data, size, endpoint->video_sink_user_data);
	}
}

static void keyframe_requested(void *user_data)
{
	struct libpeer_endpoint *endpoint = user_data;

	if (endpoint->keyframe_request != NULL) {
		endpoint->keyframe_request(endpoint->video_sink_user_data);
	}
}

static void endpoint_loop(void *endpoint_arg, void *unused_1, void *unused_2)
{
	struct libpeer_endpoint *endpoint = endpoint_arg;

	ARG_UNUSED(unused_1);
	ARG_UNUSED(unused_2);
	while (atomic_get(&endpoint->stop) == 0) {
		struct libpeer_audio_frame frame;
		struct libpeer_video_frame *video;

		if (k_msgq_get(&endpoint->tx_queue, &frame, K_NO_WAIT) == 0 &&
		    peer_connection_send_audio(endpoint->connection, frame.data,
					       sizeof(frame.data)) != 0) {
			atomic_inc(&endpoint->send_failures);
		}
		if (endpoint->video_enabled && atomic_cas(&endpoint->keyframe_requests, 1, 0) &&
		    peer_connection_request_keyframe(endpoint->connection) != 0) {
			atomic_inc(&endpoint->send_failures);
		}
		if (endpoint->video_enabled &&
		    k_msgq_get(&endpoint->video_tx_queue, &video, K_NO_WAIT) == 0) {
			if (peer_connection_send_video(endpoint->connection, video->data,
						       video->size) != 0) {
				atomic_inc(&endpoint->send_failures);
			}
			(void)k_msgq_put(&endpoint->video_free_queue, &video, K_FOREVER);
		}
		(void)peer_connection_loop(endpoint->connection);
		k_sleep(K_MSEC(1));
	}
}

static void reset_video_queues(struct libpeer_endpoint *endpoint)
{
	k_msgq_purge(&endpoint->video_tx_queue);
	k_msgq_purge(&endpoint->video_free_queue);
	for (size_t index = 0U; index < ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH; index++) {
		struct libpeer_video_frame *frame = &endpoint->video_storage[index];

		(void)k_msgq_put(&endpoint->video_free_queue, &frame, K_NO_WAIT);
	}
}

int libpeer_endpoint_init(struct libpeer_endpoint *endpoint, const char *name, uint8_t send_byte,
			  uint8_t expected_byte, bool validate_pattern, bool video_enabled)
{
	PeerConfiguration configuration = {0};

	memset(endpoint, 0, sizeof(*endpoint));
	k_msgq_init(&endpoint->tx_queue, endpoint->tx_queue_buffer,
		    sizeof(struct libpeer_audio_frame), ZEPHYRTC_ENDPOINT_TX_QUEUE_DEPTH);
	k_msgq_init(&endpoint->video_tx_queue, endpoint->video_tx_queue_buffer,
		    sizeof(struct libpeer_video_frame *), ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH);
	k_msgq_init(&endpoint->video_free_queue, endpoint->video_free_queue_buffer,
		    sizeof(struct libpeer_video_frame *), ZEPHYRTC_ENDPOINT_VIDEO_QUEUE_DEPTH);
	reset_video_queues(endpoint);
	endpoint->name = name;
	endpoint->send_byte = send_byte;
	endpoint->expected_byte = expected_byte;
	endpoint->validate_pattern = validate_pattern;
	endpoint->video_enabled = video_enabled;
	atomic_set(&endpoint->state, PEER_CONNECTION_NEW);

	configuration.audio_codec = CODEC_PCMU;
	configuration.video_codec = video_enabled ? CODEC_H264 : CODEC_NONE;
	configuration.datachannel = DATA_CHANNEL_NONE;
	configuration.onaudiotrack = audio_received;
	if (video_enabled) {
		configuration.onvideotrack = video_received;
		configuration.on_request_keyframe = keyframe_requested;
	}
	configuration.user_data = endpoint;
	endpoint->connection = peer_connection_create(&configuration);
	if (endpoint->connection == NULL) {
		return -ENOMEM;
	}
	peer_connection_oniceconnectionstatechange(endpoint->connection, state_changed);
	return 0;
}

int libpeer_endpoint_start(struct libpeer_endpoint *endpoint, k_thread_stack_t *stack,
			   size_t stack_size)
{
	k_msgq_purge(&endpoint->tx_queue);
	reset_video_queues(endpoint);
	atomic_set(&endpoint->send_failures, 0);
	atomic_set(&endpoint->keyframe_requests, 0);
	atomic_set(&endpoint->stop, 0);
	(void)k_thread_create(&endpoint->thread, stack, stack_size, endpoint_loop, endpoint, NULL,
			      NULL, ZEPHYRTC_ENDPOINT_THREAD_PRIO, 0, K_NO_WAIT);
	endpoint->thread_started = true;
	return 0;
}

int libpeer_endpoint_set_audio_sink(struct libpeer_endpoint *endpoint,
				    libpeer_endpoint_audio_sink_fn sink, void *user_data)
{
	if (endpoint == NULL || endpoint->thread_started) {
		return -EINVAL;
	}
	endpoint->audio_sink = sink;
	endpoint->audio_sink_user_data = user_data;
	return 0;
}

int libpeer_endpoint_set_video_sink(struct libpeer_endpoint *endpoint,
				    libpeer_endpoint_video_sink_fn sink,
				    libpeer_endpoint_keyframe_fn keyframe_request, void *user_data)
{
	if (endpoint == NULL || endpoint->thread_started || !endpoint->video_enabled) {
		return -EINVAL;
	}
	endpoint->video_sink = sink;
	endpoint->keyframe_request = keyframe_request;
	endpoint->video_sink_user_data = user_data;
	return 0;
}

int libpeer_endpoint_send_audio(struct libpeer_endpoint *endpoint, const uint8_t *data, size_t size)
{
	struct libpeer_audio_frame frame;

	if (endpoint == NULL || data == NULL || size != sizeof(frame.data) ||
	    !endpoint->thread_started) {
		return -EINVAL;
	}
	memcpy(frame.data, data, sizeof(frame.data));
	if (k_msgq_put(&endpoint->tx_queue, &frame, K_MSEC(20)) != 0) {
		return -ENOBUFS;
	}
	return 0;
}

int libpeer_endpoint_send_video(struct libpeer_endpoint *endpoint, const uint8_t *data, size_t size)
{
	struct libpeer_video_frame *frame;

	if (endpoint == NULL || data == NULL || size == 0U ||
	    size > ZEPHYRTC_ENDPOINT_VIDEO_BYTES || !endpoint->thread_started ||
	    !endpoint->video_enabled) {
		return -EINVAL;
	}
	if (k_msgq_get(&endpoint->video_free_queue, &frame, K_MSEC(50)) != 0) {
		return -ENOBUFS;
	}
	frame->size = size;
	memcpy(frame->data, data, size);
	if (k_msgq_put(&endpoint->video_tx_queue, &frame, K_NO_WAIT) != 0) {
		(void)k_msgq_put(&endpoint->video_free_queue, &frame, K_NO_WAIT);
		return -ENOBUFS;
	}
	return 0;
}

int libpeer_endpoint_request_keyframe(struct libpeer_endpoint *endpoint)
{
	if (endpoint == NULL || endpoint->connection == NULL || !endpoint->thread_started ||
	    !endpoint->video_enabled) {
		return -EINVAL;
	}
	atomic_set(&endpoint->keyframe_requests, 1);
	return 0;
}

void libpeer_endpoint_stop(struct libpeer_endpoint *endpoint)
{
	if (!endpoint->thread_started) {
		return;
	}
	atomic_set(&endpoint->stop, 1);
	if (k_thread_join(&endpoint->thread, K_SECONDS(5)) != 0) {
		k_thread_abort(&endpoint->thread);
	}
	endpoint->thread_started = false;
}

void libpeer_endpoint_destroy(struct libpeer_endpoint *endpoint)
{
	libpeer_endpoint_stop(endpoint);
	if (endpoint->connection != NULL) {
		peer_connection_close(endpoint->connection);
		peer_connection_destroy(endpoint->connection);
		endpoint->connection = NULL;
	}
}

bool libpeer_endpoint_failed(const struct libpeer_endpoint *endpoint)
{
	return atomic_get(&endpoint->state) == PEER_CONNECTION_FAILED ||
	       atomic_get(&endpoint->send_failures) != 0;
}

int libpeer_endpoint_wait_connected(struct libpeer_endpoint *endpoint, uint32_t timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < deadline) {
		if (atomic_get(&endpoint->state) == PEER_CONNECTION_CONNECTED) {
			return 0;
		}
		if (libpeer_endpoint_failed(endpoint)) {
			return -ECONNABORTED;
		}
		k_sleep(K_MSEC(10));
	}
	return -ETIMEDOUT;
}

static bool pair_failed(const struct libpeer_endpoint *first, const struct libpeer_endpoint *second)
{
	return libpeer_endpoint_failed(first) || libpeer_endpoint_failed(second);
}

int libpeer_pair_wait_connected(struct libpeer_endpoint *first, struct libpeer_endpoint *second)
{
	int64_t deadline = k_uptime_get() + ZEPHYRTC_CONNECT_TIMEOUT_MS;

	while (k_uptime_get() < deadline) {
		if (atomic_get(&first->state) == PEER_CONNECTION_CONNECTED &&
		    atomic_get(&second->state) == PEER_CONNECTION_CONNECTED) {
			return 0;
		}
		if (pair_failed(first, second)) {
			return -ECONNABORTED;
		}
		k_sleep(K_MSEC(10));
	}
	return -ETIMEDOUT;
}

int libpeer_pair_exchange_audio(struct libpeer_endpoint *first, struct libpeer_endpoint *second)
{
	uint8_t first_audio[ZEPHYRTC_ENDPOINT_AUDIO_BYTES];
	uint8_t second_audio[ZEPHYRTC_ENDPOINT_AUDIO_BYTES];
	int64_t deadline;

	memset(first_audio, first->send_byte, sizeof(first_audio));
	memset(second_audio, second->send_byte, sizeof(second_audio));
	for (int frame = 0; frame < ZEPHYRTC_ENDPOINT_AUDIO_FRAMES; frame++) {
		if (libpeer_endpoint_send_audio(first, first_audio, sizeof(first_audio)) != 0 ||
		    libpeer_endpoint_send_audio(second, second_audio, sizeof(second_audio)) != 0) {
			return -EIO;
		}
		k_sleep(K_MSEC(20));
	}

	deadline = k_uptime_get() + ZEPHYRTC_MEDIA_TIMEOUT_MS;
	while (k_uptime_get() < deadline) {
		if (atomic_get(&first->received_frames) >= ZEPHYRTC_ENDPOINT_AUDIO_FRAMES &&
		    atomic_get(&second->received_frames) >= ZEPHYRTC_ENDPOINT_AUDIO_FRAMES) {
			return 0;
		}
		if (pair_failed(first, second)) {
			return -ECONNABORTED;
		}
		k_sleep(K_MSEC(10));
	}
	return -ETIMEDOUT;
}
