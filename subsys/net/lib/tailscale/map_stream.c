/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "map_stream.h"

int tailscale_map_stream_init(struct tailscale_map_stream *stream, uint8_t *buffer,
			      size_t capacity, uint32_t generation,
			      tailscale_map_message_fn dispatch, void *user_data)
{
	if (stream == NULL || buffer == NULL || capacity == 0U || dispatch == NULL) {
		return -EINVAL;
	}

	*stream = (struct tailscale_map_stream){
		.buffer = buffer,
		.capacity = capacity,
		.dispatch = dispatch,
		.user_data = user_data,
		.generation = generation,
	};
	return 0;
}

int tailscale_map_stream_reset(struct tailscale_map_stream *stream, uint32_t generation)
{
	if (stream->generation == generation) {
		return -EINVAL;
	}

	stream->generation = generation;
	stream->payload_size = 0U;
	stream->received = 0U;
	stream->error = 0;
	stream->finished = false;
	return 0;
}

int tailscale_map_stream_feed(struct tailscale_map_stream *stream, uint32_t generation,
			      const uint8_t *data, size_t len)
{
	if (stream->generation != generation) {
		return -ESTALE;
	}
	if (stream->error != 0) {
		return stream->error;
	}
	if (stream->finished) {
		return -ESHUTDOWN;
	}
	if (data == NULL && len != 0U) {
		return -EINVAL;
	}

	while (len > 0U) {
		bool reading_header = stream->payload_size == 0U;
		uint8_t *target = reading_header ? stream->header : stream->buffer;
		size_t size = reading_header ? sizeof(stream->header) : stream->payload_size;
		size_t copy_len = MIN(len, size - stream->received);

		memcpy(target + stream->received, data, copy_len);
		stream->received += copy_len;
		data += copy_len;
		len -= copy_len;
		if (stream->received < size) {
			continue;
		}

		stream->received = 0U;
		if (reading_header) {
			uint32_t payload_size = sys_get_le32(stream->header);

			if (payload_size == 0U) {
				stream->error = -EBADMSG;
				return stream->error;
			}
			if (payload_size > stream->capacity) {
				stream->error = -EMSGSIZE;
				return stream->error;
			}
			stream->payload_size = payload_size;
		} else {
			stream->error = stream->dispatch(stream->buffer, size, stream->user_data);
			if (stream->error != 0) {
				return stream->error;
			}
			stream->payload_size = 0U;
		}
	}

	return 0;
}

int tailscale_map_stream_finish(struct tailscale_map_stream *stream, uint32_t generation)
{
	if (stream->generation != generation) {
		return -ESTALE;
	}
	if (stream->error != 0) {
		return stream->error;
	}
	if (stream->received != 0U || stream->payload_size != 0U) {
		stream->error = -EBADMSG;
		return stream->error;
	}

	stream->finished = true;
	return 0;
}
