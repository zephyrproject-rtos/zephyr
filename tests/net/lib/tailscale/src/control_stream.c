/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "tailscale_internal.h"
#include <zephyr/ztest.h>

struct control_fixture {
	struct tailscale_control_stream stream;
	uint32_t attempts;
	uint32_t reads;
	uint32_t closed;
	uint32_t applied;
	int reason[2];
};

static int open_response(void *data)
{
	struct control_fixture *fixture = data;

	fixture->attempts++;
	fixture->reads = 0;
	return 0;
}

static int next_chunk(int64_t deadline, const uint8_t **data, size_t *len, void *user_data)
{
	static const uint8_t partial[] = {8, 0, 0, 0, '{'};
	static const uint8_t next_map[] = {2, 0, 0, 0, '{', '}'};
	struct control_fixture *fixture = user_data;

	fixture->reads++;
	if (fixture->attempts == 1U) {
		if (fixture->reads == 1U) {
			*data = partial;
			*len = sizeof(partial);
			return 0;
		}

		k_sleep(K_TIMEOUT_ABS_MS(deadline + 1));
	}
	*data = next_map;
	*len = sizeof(next_map);
	return 0;
}

static void close_response(void *data)
{
	struct control_fixture *fixture = data;

	fixture->closed++;
}

static int apply_map(uint32_t generation, uint8_t *json, size_t len, void *data)
{
	struct control_fixture *fixture = data;

	zassert_equal(generation, 2U);
	zassert_equal(len, 2U);
	zassert_mem_equal(json, "{}", len);
	fixture->applied++;
	tailscale_control_stream_cancel(&fixture->stream);
	return 0;
}

static void receive_event(uint32_t generation, enum tailscale_control_stream_event event,
			  int reason, void *user_data)
{
	struct control_fixture *fixture = user_data;

	if (event == TAILSCALE_CONTROL_STREAM_CLOSED) {
		zassert_equal(generation, fixture->closed);
		zassert_between_inclusive(generation, 1U, ARRAY_SIZE(fixture->reason));
		fixture->reason[generation - 1U] = reason;
	}
}

ZTEST(tailscale_control_stream, test_timeout_discards_partial_map_then_reconnects)
{
	static const struct tailscale_control_stream_ops ops = {
		.open = open_response,
		.read = next_chunk,
		.close = close_response,
		.map = apply_map,
		.event = receive_event,
	};
	struct control_fixture fixture = {0};
	uint8_t frame[8];
	const struct tailscale_control_stream_config config = {
		.frame = frame,
		.capacity = sizeof(frame),
		.idle_ms = 10U,
		.retry_min_ms = 1U,
		.retry_max_ms = 4U,
	};

	tailscale_control_stream_init(&fixture.stream);
	zassert_ok(tailscale_control_stream_run(&fixture.stream, &config, &ops, &fixture));
	zexpect_equal(fixture.attempts, 2U);
	zexpect_equal(fixture.closed, 2U);
	zexpect_equal(fixture.applied, 1U);
	zexpect_equal(fixture.reason[0], -ETIMEDOUT);
	zexpect_equal(fixture.reason[1], -ECANCELED);
}

ZTEST_SUITE(tailscale_control_stream, NULL, NULL, NULL, NULL, NULL);
