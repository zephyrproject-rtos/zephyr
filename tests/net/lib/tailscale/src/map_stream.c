/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "map_stream.h"

#define STREAM_CAPACITY 260U
#define FIRST_GENERATION 1U
#define NEXT_GENERATION  2U

struct tailscale_map_stream_test_fixture {
	struct tailscale_map_stream stream;
	uint8_t storage[STREAM_CAPACITY];
	const char *const *expected;
	size_t expected_count;
	size_t attempts;
	size_t delivered;
	size_t reject_attempt;
};

static int receive_message(const uint8_t *message, size_t len, void *user_data)
{
	struct tailscale_map_stream_test_fixture *fixture = user_data;
	size_t index = fixture->attempts++;

	zassert_true(index < fixture->expected_count);
	zassert_equal(len, strlen(fixture->expected[index]));
	zassert_mem_equal(message, fixture->expected[index], len);
	if (fixture->attempts == fixture->reject_attempt) {
		return -EACCES;
	}

	fixture->delivered++;
	return 0;
}

static void *suite_setup(void)
{
	static struct tailscale_map_stream_test_fixture fixture;

	return &fixture;
}

static void stream_setup(void *data)
{
	struct tailscale_map_stream_test_fixture *fixture = data;

	fixture->expected = NULL;
	fixture->expected_count = 0U;
	fixture->attempts = 0U;
	fixture->delivered = 0U;
	fixture->reject_attempt = 0U;
	zassert_ok(tailscale_map_stream_init(&fixture->stream, fixture->storage,
					     sizeof(fixture->storage), FIRST_GENERATION,
					     receive_message, fixture));
}

static size_t write_frame(uint8_t *wire, const char *message)
{
	size_t len = strlen(message);

	sys_put_le32((uint32_t)len, wire);
	memcpy(wire + sizeof(uint32_t), message, len);
	return sizeof(uint32_t) + len;
}

static void transfer_chunk(struct tailscale_map_stream_test_fixture *fixture, const int pair[2],
			   const uint8_t *wire, size_t len)
{
	uint8_t received[128];

	zassert_true(len <= sizeof(received));
	zassert_equal(zsock_send(pair[0], wire, len, 0), (ssize_t)len);
	zassert_equal(zsock_recv(pair[1], received, len, 0), (ssize_t)len);
	zassert_ok(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, received, len));
}

ZTEST_F(tailscale_map_stream_test, test_socket_producer_fragments_and_coalesces_frames)
{
	const char *const messages[] = {
		"{\"Node\":{\"ID\":7}}",
		"{\"KeepAlive\":true}",
		"{\"PeersRemoved\":[7]}",
	};
	uint8_t wire[128];
	uint8_t eof;
	int pair[2];
	size_t len = 0U;

	fixture->expected = messages;
	fixture->expected_count = ARRAY_SIZE(messages);
	for (size_t i = 0U; i < ARRAY_SIZE(messages); i++) {
		len += write_frame(wire + len, messages[i]);
	}

	zassert_ok(zsock_socketpair(NET_AF_UNIX, NET_SOCK_STREAM, 0, pair));
	transfer_chunk(fixture, pair, wire, 1U);
	zassert_equal(fixture->delivered, 0U);
	transfer_chunk(fixture, pair, wire + 1U, 4U);
	zassert_equal(fixture->delivered, 0U);
	transfer_chunk(fixture, pair, wire + 5U, len - 5U);
	zassert_equal(fixture->delivered, ARRAY_SIZE(messages));

	zassert_ok(zsock_close(pair[0]));
	zassert_equal(zsock_recv(pair[1], &eof, sizeof(eof), 0), 0);
	zassert_ok(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION));
	zassert_ok(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION));
	zassert_equal(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, wire, len),
		      -ESHUTDOWN);
	zassert_equal(fixture->delivered, ARRAY_SIZE(messages));
	zassert_ok(zsock_close(pair[1]));
}

ZTEST_F(tailscale_map_stream_test, test_maximum_frame_uses_full_little_endian_length)
{
	const uint8_t header[] = {0x04, 0x01, 0x00, 0x00};
	char message[STREAM_CAPACITY + 1U];
	const char *const messages[] = {message};

	memset(message, ' ', STREAM_CAPACITY);
	message[0] = '{';
	message[1] = '}';
	message[STREAM_CAPACITY] = '\0';
	fixture->expected = messages;
	fixture->expected_count = ARRAY_SIZE(messages);

	zassert_ok(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, header,
					     sizeof(header)));
	zassert_equal(fixture->delivered, 0U);
	zassert_ok(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION,
					     (const uint8_t *)message, STREAM_CAPACITY));
	zassert_equal(fixture->delivered, 1U);
	zassert_ok(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION));
}

ZTEST_F(tailscale_map_stream_test, test_rejects_invalid_lengths_without_dispatch)
{
	const uint8_t oversized[] = {0x05, 0x01, 0x00, 0x00};
	const uint8_t largest[] = {0xff, 0xff, 0xff, 0xff};
	const uint8_t empty[] = {0x00, 0x00, 0x00, 0x00};

	zassert_equal(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, oversized,
					       sizeof(oversized)), -EMSGSIZE);
	zassert_equal(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION), -EMSGSIZE);
	zassert_ok(tailscale_map_stream_reset(&fixture->stream, NEXT_GENERATION));
	zassert_equal(tailscale_map_stream_feed(&fixture->stream, NEXT_GENERATION, largest,
					       sizeof(largest)), -EMSGSIZE);
	zassert_ok(tailscale_map_stream_reset(&fixture->stream, NEXT_GENERATION + 1U));
	zassert_equal(tailscale_map_stream_feed(&fixture->stream, NEXT_GENERATION + 1U, empty,
					       sizeof(empty)), -EBADMSG);
	zassert_equal(fixture->attempts, 0U);
}

ZTEST_F(tailscale_map_stream_test, test_incomplete_header_or_payload_at_eof)
{
	const uint8_t bytes[] = {0x02, 0x00, 0x00, 0x00, '{', '}'};

	for (size_t len = 1U; len < sizeof(bytes); len++) {
		uint32_t generation = NEXT_GENERATION + (uint32_t)len;

		zassert_ok(tailscale_map_stream_reset(&fixture->stream, generation));
		zassert_ok(tailscale_map_stream_feed(&fixture->stream, generation, bytes, len));
		zassert_equal(tailscale_map_stream_finish(&fixture->stream, generation), -EBADMSG);
		zassert_equal(tailscale_map_stream_feed(&fixture->stream, generation, bytes,
						       sizeof(bytes)), -EBADMSG);
	}
	zassert_equal(fixture->attempts, 0U);
}

ZTEST_F(tailscale_map_stream_test, test_dispatch_failure_stops_at_failing_frame)
{
	const char *const messages[] = {"{}", "{\"KeepAlive\":true}", "{}"};
	uint8_t wire[64];
	size_t len = 0U;

	fixture->expected = messages;
	fixture->expected_count = ARRAY_SIZE(messages);
	fixture->reject_attempt = 2U;
	for (size_t i = 0U; i < ARRAY_SIZE(messages); i++) {
		len += write_frame(wire + len, messages[i]);
	}

	zassert_equal(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, wire, len),
		      -EACCES);
	zassert_equal(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, wire, len),
		      -EACCES);
	zassert_equal(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION), -EACCES);
	zassert_equal(fixture->attempts, 2U);
	zassert_equal(fixture->delivered, 1U);
}

ZTEST_F(tailscale_map_stream_test, test_generation_reset_discards_incomplete_message)
{
	const uint8_t wire[] = {0x02, 0x00, 0x00, 0x00, '{', '}'};
	const char *const messages[] = {"{}"};

	fixture->expected = messages;
	fixture->expected_count = ARRAY_SIZE(messages);
	zassert_ok(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, wire, 5U));
	zassert_equal(tailscale_map_stream_reset(&fixture->stream, FIRST_GENERATION), -EINVAL);
	zassert_ok(tailscale_map_stream_reset(&fixture->stream, NEXT_GENERATION));
	zassert_equal(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, wire + 5U, 1U),
		      -ESTALE);
	zassert_equal(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION), -ESTALE);
	zassert_equal(fixture->attempts, 0U);
	zassert_ok(tailscale_map_stream_feed(&fixture->stream, NEXT_GENERATION, wire,
					     sizeof(wire)));
	zassert_ok(tailscale_map_stream_finish(&fixture->stream, NEXT_GENERATION));
	zassert_equal(fixture->delivered, 1U);
}

ZTEST_F(tailscale_map_stream_test, test_empty_body_ends_cleanly)
{
	zassert_ok(tailscale_map_stream_feed(&fixture->stream, FIRST_GENERATION, NULL, 0U));
	zassert_ok(tailscale_map_stream_finish(&fixture->stream, FIRST_GENERATION));
	zassert_equal(fixture->attempts, 0U);
}

ZTEST_SUITE(tailscale_map_stream_test, NULL, suite_setup, stream_setup, NULL, NULL);
