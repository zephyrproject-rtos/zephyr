/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_RING_BUFFER_SIZE 64U

struct stream_ring_buffer {
	int16_t data[STREAM_RING_BUFFER_SIZE];
	size_t head;
	size_t count;
};

void stream_ring_buffer_reset(struct stream_ring_buffer *ring);
void stream_ring_buffer_push(struct stream_ring_buffer *ring, int16_t sample);
void stream_ring_buffer_push_many(struct stream_ring_buffer *ring,
				  const int16_t *samples, size_t len);
void stream_ring_buffer_copy_window(const struct stream_ring_buffer *ring,
				    int16_t *window, size_t len);

#ifdef __cplusplus
}
#endif
