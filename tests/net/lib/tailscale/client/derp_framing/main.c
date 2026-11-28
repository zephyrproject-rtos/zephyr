/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include "derp_conn.h"

static uint8_t wire[5 + DERP_FRAME_MAX];
static uint8_t payload[DERP_FRAME_MAX];
static size_t available;
static size_t offset;
static struct derp_conn conn;

int control_io_read(struct control_io *io, void *dst, size_t len)
{
	if (len > available - offset) {
		return -ECONNRESET;
	}
	memcpy(dst, wire + offset, len);
	offset += len;
	return 0;
}

int control_io_write(struct control_io *io, const void *src, size_t len)
{
	if (len > sizeof(wire) - offset) {
		return -ENOSPC;
	}
	memcpy(wire + offset, src, len);
	offset += len;
	return 0;
}

static void reset(void *fixture)
{
	memset(&conn, 0, sizeof(conn));
	memset(wire, 0, sizeof(wire));
	available = 0;
	offset = 0;
}

ZTEST(derp_framing, test_complete_frame_at_limit)
{
	uint8_t type;
	size_t len;

	wire[0] = DERP_RECV;
	sys_put_be32(DERP_FRAME_MAX, wire + 1);
	memset(wire + 5, 0xad, DERP_FRAME_MAX);
	available = sizeof(wire);
	zassert_ok(derp_conn_read(&conn, &type, payload, &len));
	zassert_equal(type, DERP_RECV);
	zassert_equal(len, DERP_FRAME_MAX);
	zassert_mem_equal(payload, wire + 5, len);
}

ZTEST(derp_framing, test_reject_oversize_and_partial_frames)
{
	uint8_t type;
	size_t len;

	sys_put_be32(DERP_FRAME_MAX + 1, wire + 1);
	available = 5;
	zassert_equal(derp_conn_read(&conn, &type, payload, &len), -EMSGSIZE);
	zassert_equal(offset, 5);
	offset = 0;
	sys_put_be32(1, wire + 1);
	zassert_equal(derp_conn_read(&conn, &type, payload, &len), -ECONNRESET);
	offset = 0;
	available = 4;
	zassert_equal(derp_conn_read(&conn, &type, payload, &len), -ECONNRESET);
}

ZTEST(derp_framing, test_credit_and_size_checked_before_any_write)
{
	/* Five header bytes and three body bytes exhaust one full bucket. */
	conn.burst = 8;
	conn.rate = 1;
	conn.credit_time = k_uptime_get();
	conn.credit = 8000;
	zassert_equal(derp_conn_write(&conn, DERP_SEND, payload, DERP_FRAME_MAX + 1), -EMSGSIZE);
	zassert_equal(offset, 0);
	zassert_ok(derp_conn_write(&conn, DERP_SEND, payload, 3));
	zassert_equal(offset, 8);
	zassert_equal(wire[0], DERP_SEND);
	zassert_equal(sys_get_be32(wire + 1), 3);
	zassert_equal(derp_conn_write(&conn, DERP_SEND, payload, 1), -EAGAIN);
	zassert_equal(offset, 8);
	/* Control traffic can still progress while packet credit is exhausted. */
	zassert_ok(derp_conn_write(&conn, DERP_PONG, payload, 8));
	conn.credit_time -= 10000;
	zassert_ok(derp_conn_write(&conn, DERP_SEND, payload, 3));
	zassert_equal(derp_conn_write(&conn, DERP_SEND, payload, 1), -EAGAIN);
}

ZTEST(derp_framing, test_zero_burst_preserves_control)
{
	conn.rate = 1000;
	conn.burst = 0;
	conn.credit_time = k_uptime_get() - 1000;
	zassert_equal(derp_conn_write(&conn, DERP_SEND, payload, 1), -EAGAIN);
	zassert_equal(offset, 0);
	zassert_ok(derp_conn_write(&conn, DERP_PONG, payload, 8));
}

ZTEST_SUITE(derp_framing, NULL, NULL, reset, NULL, NULL);
