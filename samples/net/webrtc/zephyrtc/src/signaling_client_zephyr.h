/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_ZEPHYR_SIGNALING_CLIENT_H_
#define ZEPHYRTC_ZEPHYR_SIGNALING_CLIENT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Largest SDP body, in bytes, accepted in either direction. */
#define SIGNALING_SDP_MAX_BYTES       8095U
/* Largest HTTP response header block, including the terminating blank line. */
#define SIGNALING_HEADER_MAX_BYTES    4096U
/* Smallest answer buffer: body, one probe byte for trailing data, and a NUL. */
#define SIGNALING_ANSWER_BUFFER_BYTES (SIGNALING_SDP_MAX_BYTES + 2U)

enum signaling_result {
	SIGNALING_OK = 0,
	SIGNALING_BAD_ARGUMENT,
	SIGNALING_BAD_URL,
	SIGNALING_BAD_OFFER,
	SIGNALING_UNREACHABLE,
	SIGNALING_TIMEOUT,
	SIGNALING_CANCELLED,
	SIGNALING_IO_ERROR,
	SIGNALING_BAD_STATUS,
	SIGNALING_BAD_HEADER,
	SIGNALING_BAD_CONTENT_TYPE,
	SIGNALING_BAD_CONTENT_LENGTH,
	SIGNALING_OVERSIZE,
	SIGNALING_TRUNCATED,
	SIGNALING_BAD_BODY,
};

struct signaling_request {
	/* http://IPv4[:port]/path or http://localhost[:port]/path. No DNS, TLS, or redirects. */
	const char *url;
	/* Complete SDP offer, NUL terminated. */
	const char *offer;
	/* Total deadline for connect, request, and response. Zero selects the Kconfig default. */
	uint32_t timeout_ms;
	/* Optional. Polled at least every 20 ms from the calling thread. */
	bool (*cancelled)(void *user_data);
	void *user_data;
};

/*
 * POST the offer and receive one bounded, complete SDP answer.
 *
 * The caller's thread performs all socket I/O. On success the answer is NUL terminated in
 * @p answer and its length is stored in @p answer_length. @p capacity must be at least
 * SIGNALING_ANSWER_BUFFER_BYTES. The answer buffer content is unspecified on failure.
 */
enum signaling_result signaling_exchange(const struct signaling_request *request, char *answer,
					 size_t capacity, size_t *answer_length);

/* True for a complete CRLF-terminated SDP with a version line and a media section. */
bool signaling_sdp_is_complete(const char *sdp, size_t length);

const char *signaling_result_str(enum signaling_result result);

#endif /* ZEPHYRTC_ZEPHYR_SIGNALING_CLIENT_H_ */
