/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "tailscale_internal.h"
#include <zephyr/sys/util.h>

#include "map_stream.h"

struct control_session {
	struct tailscale_control_stream *stream;
	const struct tailscale_control_stream_config *config;
	const struct tailscale_control_stream_ops *ops;
	void *user_data;
	int64_t deadline;
	bool received_frame;
};

void tailscale_control_stream_init(struct tailscale_control_stream *stream)
{
	atomic_clear(&stream->cancelled);
	k_sem_init(&stream->wake, 0, 1);
	stream->generation = 0U;
}

void tailscale_control_stream_prepare(struct tailscale_control_stream *stream)
{
	k_sem_reset(&stream->wake);
	atomic_clear(&stream->cancelled);
}

void tailscale_control_stream_cancel(struct tailscale_control_stream *stream)
{
	atomic_set(&stream->cancelled, 1);
	k_sem_give(&stream->wake);
}

bool tailscale_control_stream_cancelled(const struct tailscale_control_stream *stream)
{
	return atomic_get(&stream->cancelled) != 0;
}

static int receive_map(const uint8_t *message, size_t len, void *user_data)
{
	struct control_session *session = user_data;
	int rc;

	ARG_UNUSED(message);
	if (tailscale_control_stream_cancelled(session->stream)) {
		return -ECANCELED;
	}

	rc = session->ops->map(session->stream->generation, session->config->frame, len,
			       session->user_data);
	if (rc == 0) {
		session->received_frame = true;
		session->deadline = k_uptime_get() + session->config->idle_ms;
	}

	return rc;
}

static int receive_body(struct control_session *session)
{
	struct tailscale_map_stream decoder;
	uint32_t generation = session->stream->generation;
	int rc;

	rc = tailscale_map_stream_init(&decoder, session->config->frame, session->config->capacity,
				       generation, receive_map, session);
	if (rc != 0) {
		return rc;
	}

	session->deadline = k_uptime_get() + session->config->idle_ms;
	while (!tailscale_control_stream_cancelled(session->stream)) {
		const uint8_t *data;
		size_t len;

		if (k_uptime_get() >= session->deadline) {
			return -ETIMEDOUT;
		}
		rc = session->ops->read(session->deadline, &data, &len, session->user_data);
		if (rc != 0) {
			return rc;
		}
		if (tailscale_control_stream_cancelled(session->stream)) {
			return -ECANCELED;
		}
		if (k_uptime_get() >= session->deadline) {
			return -ETIMEDOUT;
		}
		if (len == 0U) {
			rc = tailscale_map_stream_finish(&decoder, generation);
			return rc == 0 ? -ECONNRESET : rc;
		}
		rc = tailscale_map_stream_feed(&decoder, generation, data, len);
		if (rc != 0) {
			return rc;
		}
	}

	return -ECANCELED;
}

static void notify(struct control_session *session, enum tailscale_control_stream_event event,
		   int reason)
{
	if (session->ops->event != NULL) {
		session->ops->event(session->stream->generation, event, reason, session->user_data);
	}
}

static bool permanent_open_error(int rc)
{
	return rc == -EACCES || rc == -EINVAL || rc == -EINPROGRESS || rc == -ENOTSUP;
}

int tailscale_control_stream_run(struct tailscale_control_stream *stream,
				 const struct tailscale_control_stream_config *config,
				 const struct tailscale_control_stream_ops *ops, void *user_data)
{
	struct control_session session = {
		.stream = stream, .config = config, .ops = ops, .user_data = user_data,
	};
	uint32_t delay;
	bool retry;

	if (stream == NULL || config == NULL || ops == NULL || ops->open == NULL ||
	    ops->read == NULL || ops->close == NULL || ops->map == NULL || config->frame == NULL ||
	    config->capacity == 0U || config->idle_ms == 0U || config->retry_min_ms == 0U ||
	    config->retry_max_ms < config->retry_min_ms) {
		return -EINVAL;
	}

	delay = config->retry_min_ms;
	while (!tailscale_control_stream_cancelled(stream)) {
		int rc;

		if (++stream->generation == 0U) {
			stream->generation++;
		}
		session.received_frame = false;
		notify(&session, TAILSCALE_CONTROL_STREAM_OPENING, 0);
		if (tailscale_control_stream_cancelled(stream)) {
			notify(&session, TAILSCALE_CONTROL_STREAM_CLOSED, -ECANCELED);
			break;
		}
		rc = ops->open(user_data);
		retry = !permanent_open_error(rc);
		if (rc == 0) {
			notify(&session, TAILSCALE_CONTROL_STREAM_RECEIVING, 0);
			rc = receive_body(&session);
		}
		ops->close(user_data);
		if (tailscale_control_stream_cancelled(stream)) {
			rc = -ECANCELED;
		}
		notify(&session, TAILSCALE_CONTROL_STREAM_CLOSED, rc);

		if (!retry && rc != -ECANCELED) {
			return rc;
		}
		if (tailscale_control_stream_cancelled(stream)) {
			break;
		}
		if (session.received_frame) {
			delay = config->retry_min_ms;
		}
		(void)k_sem_take(&stream->wake, K_MSEC(delay));
		delay = MIN((uint64_t)delay * 2U, (uint64_t)config->retry_max_ms);
	}

	return 0;
}
