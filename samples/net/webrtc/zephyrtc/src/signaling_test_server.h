/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_ZEPHYR_SIGNALING_TEST_SERVER_H_
#define ZEPHYRTC_ZEPHYR_SIGNALING_TEST_SERVER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Test-only in-process HTTP answerer on the Zephyr loopback interface. It accepts one connection,
 * validates the offer request, then sends either a scripted raw response or a 200 response around
 * an answer produced by a callback. It exists to exercise the real client over real Zephyr TCP
 * sockets without external infrastructure.
 */
struct signaling_test_script {
	/* When set, the request body must equal this offer exactly. */
	const char *expected_offer;
	/* Raw bytes sent verbatim after a valid request. Ignored when answer is set. */
	const char *wire;
	size_t wire_length;
	/* Send the wire in fragments of this size. Zero sends it in one write. */
	size_t chunk_bytes;
	uint32_t chunk_delay_ms;
	/* Keep the connection open after the response until the client closes or hold_ms. */
	bool hold_open;
	uint32_t hold_ms;
	/*
	 * Produce the SDP answer for the received offer. Returns the answer length or a negative
	 * errno, which becomes a 500 response. The server wraps the answer in a 200 response.
	 */
	int (*answer)(const char *offer, char *answer, size_t capacity, void *user_data);
	void *user_data;
};

/* Listen on 127.0.0.1 and an ephemeral port, and start the server thread. */
int signaling_test_server_start(const struct signaling_test_script *script, uint16_t *port);

/*
 * Join the server thread. Returns 0 only when it received a valid offer request and, for
 * callback servers, produced an answer. A client that hangs up early is not a server failure.
 */
int signaling_test_server_finish(void);

/* Reserve an ephemeral loopback port that has no listener, for connection-refused tests. */
int signaling_test_closed_port(uint16_t *port);

#endif /* ZEPHYRTC_ZEPHYR_SIGNALING_TEST_SERVER_H_ */
