/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "libpeer_loopback.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <peer_connection.h>

#include "libpeer_endpoint.h"
#include "platform.h"

#define LOOPBACK_SDP_BYTES 8096U

static struct libpeer_endpoint offerer;
static struct libpeer_endpoint answerer;
static char sdp_buffer[LOOPBACK_SDP_BYTES];

K_THREAD_STACK_DEFINE(offerer_stack, ZEPHYRTC_ENDPOINT_STACK_SIZE);
K_THREAD_STACK_DEFINE(answerer_stack, ZEPHYRTC_ENDPOINT_STACK_SIZE);

static int copy_sdp(const char *source)
{
	size_t length = 0U;

	if (source == NULL) {
		return -EINVAL;
	}
	while (length < sizeof(sdp_buffer) && source[length] != '\0') {
		length++;
	}
	if (length == sizeof(sdp_buffer)) {
		return -E2BIG;
	}
	memcpy(sdp_buffer, source, length + 1U);
	return 0;
}

int libpeer_loopback_run(void)
{
	const char *description;
	bool peer_initialized = false;
	int result = -EIO;

	memset(&offerer, 0, sizeof(offerer));
	memset(&answerer, 0, sizeof(answerer));

	if (libpeer_stack_init() != 0) {
		goto cleanup;
	}
	peer_initialized = true;

	/* The offerer sends 0x31 and expects 0xa7. The answerer does the opposite. */
	result = libpeer_endpoint_init(&offerer, "offerer", 0x31, 0xa7, true, false);
	if (result == 0) {
		result = libpeer_endpoint_init(&answerer, "answerer", 0xa7, 0x31, true, false);
	}
	platform_log("info", "loopback_create", "offerer=%s answerer=%s",
		     offerer.connection != NULL ? "ok" : "failed",
		     answerer.connection != NULL ? "ok" : "failed");
	if (result != 0) {
		goto cleanup;
	}
	result = -EIO;

	description = peer_connection_create_offer(offerer.connection);
	if (copy_sdp(description) != 0) {
		goto cleanup;
	}
	peer_connection_set_remote_description(answerer.connection, sdp_buffer, SDP_TYPE_OFFER);
	description = peer_connection_create_answer(answerer.connection);
	if (copy_sdp(description) != 0) {
		goto cleanup;
	}
	peer_connection_set_remote_description(offerer.connection, sdp_buffer, SDP_TYPE_ANSWER);
	if (libpeer_endpoint_failed(&offerer) || libpeer_endpoint_failed(&answerer)) {
		result = -EPROTO;
		goto cleanup;
	}

	(void)libpeer_endpoint_start(&offerer, offerer_stack, K_THREAD_STACK_SIZEOF(offerer_stack));
	(void)libpeer_endpoint_start(&answerer, answerer_stack,
				     K_THREAD_STACK_SIZEOF(answerer_stack));

	result = libpeer_pair_wait_connected(&offerer, &answerer);
	if (result == 0) {
		result = libpeer_pair_exchange_audio(&offerer, &answerer);
	}
	if (atomic_get(&offerer.invalid_frames) != 0 || atomic_get(&answerer.invalid_frames) != 0) {
		result = -EBADMSG;
	}

cleanup:
	libpeer_endpoint_destroy(&offerer);
	libpeer_endpoint_destroy(&answerer);
	if (peer_initialized) {
		libpeer_stack_deinit();
	}
	platform_log(result == 0 ? "info" : "error", "loopback_result",
		     "status=%s result=%d offerer_rx=%ld answerer_rx=%ld invalid=%ld/%ld",
		     result == 0 ? "pass" : "fail", result,
		     (long)atomic_get(&offerer.received_frames),
		     (long)atomic_get(&answerer.received_frames),
		     (long)atomic_get(&offerer.invalid_frames),
		     (long)atomic_get(&answerer.invalid_frames));
	return result;
}
