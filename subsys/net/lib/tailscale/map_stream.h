/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_TAILSCALE_MAP_STREAM_H_
#define ZEPHYR_SUBSYS_NET_LIB_TAILSCALE_MAP_STREAM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Return zero on acceptance or a negative errno to stop the stream. */
typedef int (*tailscale_map_message_fn)(const uint8_t *message, size_t len, void *user_data);

struct tailscale_map_stream {
	uint8_t *buffer;
	size_t capacity;
	tailscale_map_message_fn dispatch;
	void *user_data;
	uint32_t generation;
	uint8_t header[sizeof(uint32_t)];
	size_t payload_size;
	size_t received;
	int error;
	bool finished;
};

/*
 * The owner supplies exclusive storage and serializes calls for one HTTP body.
 * Dispatch runs synchronously on complete messages and must not reenter the
 * decoder. Its buffer view is valid only for the duration of the call.
 * Return -EINVAL for a null stream, buffer, or callback, or zero capacity.
 */
int tailscale_map_stream_init(struct tailscale_map_stream *stream, uint8_t *buffer,
			      size_t capacity, uint32_t generation,
			      tailscale_map_message_fn dispatch, void *user_data);

/*
 * Discard partial bytes and terminal status on reconnect. The owner uses a
 * different generation for each response while work from earlier responses
 * can still arrive. Return -EINVAL if the generation is already active.
 */
int tailscale_map_stream_reset(struct tailscale_map_stream *stream, uint32_t generation);

/*
 * Feed uncompressed HTTP body bytes after the owner has validated the response.
 * Each message starts with a little-endian uint32_t payload size. Data must not
 * overlap the decoder's storage. Zero bytes make no progress.
 *
 * Return -ESTALE for an obsolete generation or -ESHUTDOWN after clean EOF.
 * A null data pointer with nonzero length returns -EINVAL.
 * An empty frame (-EBADMSG), a frame above capacity (-EMSGSIZE), and a dispatch
 * error are terminal until reset. The remainder of a failed chunk is discarded.
 */
int tailscale_map_stream_feed(struct tailscale_map_stream *stream, uint32_t generation,
			      const uint8_t *data, size_t len);

/*
 * Signal body EOF. Return zero at a message boundary or -ESTALE for an
 * obsolete generation. EOF within a header or payload latches -EBADMSG.
 * Previously latched errors are returned unchanged.
 */
int tailscale_map_stream_finish(struct tailscale_map_stream *stream, uint32_t generation);

#endif /* ZEPHYR_SUBSYS_NET_LIB_TAILSCALE_MAP_STREAM_H_ */
