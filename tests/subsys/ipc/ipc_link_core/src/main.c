/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two protocol cores on a simulated wire, a millisecond at a time: each step times both sides,
 * then each side that has a frame sends it, with a fault on the way when the test says so.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

#include <ipc_link_core.h>

#define MTU       128
#define RX_COUNT  3
#define WINDOW    2
#define LOG_MAX   64
#define DATA_MAX  IPC_LINK_MSG_LEN_MAX(MTU)
#define STEPS_MAX 100000

#define RETX_MS  50
#define MAX_RETX 5

enum fault {
	FAULT_NONE,
	/* A bit flip in the header */
	FAULT_HDR,
	/* A bit flip in the payload, or the header if there is none */
	FAULT_PAYLOAD,
	/* The frame never arrived */
	FAULT_LOST,
	FAULT_COUNT,
};

struct side {
	struct ipc_link_core core;
	uint8_t tx[IPC_LINK_TX_BUFS(WINDOW) * MTU];
	uint8_t rx[RX_COUNT * MTU];
	enum fault fault;
	int connected;
	int disconnected;

	/* Keep delivered buffers instead of releasing them */
	bool hold;
	uint8_t held[RX_COUNT];
	int held_count;

	struct {
		uint8_t addr;
		uint16_t len;
		uint8_t data[DATA_MAX];
	} log[LOG_MAX];
	int log_count;

	/* Streams of numbered messages */
	uint32_t stream_sent;
	uint32_t stream_received;
};

static struct side a;
static struct side b;
static uint32_t rand_state;
static uint8_t big[DATA_MAX];
/* Milliseconds */
static uint32_t now;

static uint32_t rand_next(void)
{
	rand_state = rand_state * 1103515245U + 12345U;

	return rand_state >> 16;
}

static void side_init(struct side *s)
{
	struct ipc_link_core_cfg cfg = {
		.tx_bufs = s->tx,
		.rx_bufs = s->rx,
		.mtu = MTU,
		.rx_count = RX_COUNT,
		.window = WINDOW,
		.retx_timeout = RETX_MS,
		.max_retx = MAX_RETX,
	};

	memset(s, 0, sizeof(*s));
	ipc_link_core_init(&s->core, &cfg);
}

static void deliver(struct side *s, uint8_t idx)
{
	struct ipc_link_msg msg;
	size_t offset = 0;

	while (ipc_link_core_msg_next(&s->core, idx, &offset, &msg)) {
		zassert_equal(ipc_link_core_rx_idx(&s->core, msg.data), idx);

		if (msg.addr == 0xee) {
			zassert_equal(msg.len, sizeof(uint32_t));
			zassert_equal(sys_get_le32(msg.data), s->stream_received,
				      "stream message %u instead of %u", sys_get_le32(msg.data),
				      s->stream_received);
			s->stream_received++;
			continue;
		}

		zassert_true(s->log_count < LOG_MAX);
		s->log[s->log_count].addr = msg.addr;
		s->log[s->log_count].len = msg.len;
		memcpy(s->log[s->log_count].data, msg.data, msg.len);
		s->log_count++;
	}

	if (s->hold) {
		s->held[s->held_count++] = idx;
	} else {
		ipc_link_core_rx_release(&s->core, idx);
	}
}

static void events(struct side *s, uint32_t evt, uint8_t idx)
{
	if (evt & IPC_LINK_CORE_EVT_CONNECTED) {
		s->connected++;
	}

	if (evt & IPC_LINK_CORE_EVT_DISCONNECTED) {
		s->disconnected++;
	}

	if (evt & IPC_LINK_CORE_EVT_RX) {
		deliver(s, idx);
	}
}

/* Hand the frame in the receive buffer to a side, as a transport does */
static uint32_t receive(struct side *s, size_t len, bool ok, uint8_t *idx)
{
	uint8_t checked = ipc_link_core_rx_check(&s->core, len, ok);

	return ipc_link_core_rx(&s->core, checked, now, idx);
}

static void receive_events(struct side *s, size_t len, bool ok)
{
	uint32_t evt;
	uint8_t idx;

	evt = receive(s, len, ok, &idx);
	events(s, evt, idx);
}

/* Put a frame on the wire to a side, with the fault set on that side */
static void wire(struct side *to, const uint8_t *buf, size_t len)
{
	uint8_t *rx = ipc_link_core_rx_buf(&to->core);
	enum fault fault = to->fault;

	to->fault = FAULT_NONE;

	if (fault == FAULT_LOST) {
		return;
	}

	memcpy(rx, buf, len);

	switch (fault) {
	case FAULT_HDR:
		rx[rand_next() % IPC_LINK_HDR_SIZE] ^= BIT(rand_next() % 8);
		break;
	case FAULT_PAYLOAD:
		rx[rand_next() % len] ^= BIT(rand_next() % 8);
		break;
	default:
		break;
	}

	receive_events(to, len, true);
}

/* One millisecond: each side is timed, then sends a frame if it has one, with one in
 * fault_rate faulted
 */
static void step(uint32_t fault_rate)
{
	struct side *sides[] = {&a, &b};

	for (int i = 0; i < 2; i++) {
		struct side *s = sides[i];
		struct side *peer = sides[1 - i];

		events(s, ipc_link_core_check(&s->core, now), 0);

		if (ipc_link_core_pending(&s->core, now)) {
			const uint8_t *tx;
			size_t len;

			tx = ipc_link_core_tx_prepare(&s->core, now, &len);
			if (fault_rate > 0U && rand_next() % fault_rate == 0U) {
				peer->fault = rand_next() % FAULT_COUNT;
			}
			wire(peer, tx, len);
		}
	}

	now++;
}

/* Whether the side waits for the peer: for a boot acknowledgment, an acknowledgment or credits */
static bool waiting(struct side *s)
{
	return ipc_link_core_timeout(&s->core, now) != UINT32_MAX;
}

static bool idle(struct side *s)
{
	return !ipc_link_core_pending(&s->core, now) && !waiting(s);
}

/* Steps until nothing is left to send or to wait for, on either side */
static int run(void)
{
	int count = 0;

	while (!idle(&a) || !idle(&b)) {
		zassert_true(count < STEPS_MAX, "the link never went idle");
		step(0U);
		count++;
	}

	return count;
}

/* Steps for a while, for a side that waits for credits a peer holds */
static void run_for(uint32_t ms)
{
	for (uint32_t i = 0; i < ms; i++) {
		step(0U);
	}
}

static void connect(void)
{
	run();
	zexpect_equal(a.connected, 1);
	zexpect_equal(b.connected, 1);
}

static int stream_send(struct side *s)
{
	uint8_t data[sizeof(uint32_t)];
	int ret;

	sys_put_le32(s->stream_sent, data);
	ret = ipc_link_core_send(&s->core, 0xee, data, sizeof(data));
	if (ret == 0) {
		s->stream_sent++;
	}

	return ret;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	side_init(&a);
	side_init(&b);
	rand_state = 12345U;
	now = 0U;

	for (size_t i = 0; i < sizeof(big); i++) {
		big[i] = (uint8_t)i;
	}
}

ZTEST_SUITE(ipc_link_core, NULL, NULL, before, NULL, NULL);

ZTEST(ipc_link_core, test_handshake)
{
	zassert_true(ipc_link_core_pending(&a.core, now));
	zassert_false(ipc_link_core_connected(&a.core));
	zassert_equal(ipc_link_core_send(&a.core, 1, "x", 1), -ENOTCONN);

	connect();

	zassert_true(ipc_link_core_connected(&a.core));
	zassert_true(ipc_link_core_connected(&b.core));
	zassert_true(idle(&a));
	zassert_equal(a.core.session, 1);
	zassert_equal(b.core.session, 1);
}

ZTEST(ipc_link_core, test_handshake_one_side_first)
{
	size_t len;

	/* A announces its boot a few times into the void */
	for (int i = 0; i < 3; i++) {
		zassert_true(ipc_link_core_pending(&a.core, now));
		(void)ipc_link_core_tx_prepare(&a.core, now, &len);
		zassert_equal(len, IPC_LINK_HDR_SIZE);
		zassert_false(ipc_link_core_pending(&a.core, now));
		zassert_equal(ipc_link_core_timeout(&a.core, now), RETX_MS);
		now += RETX_MS;
	}

	connect();
}

ZTEST(ipc_link_core, test_boot_throttled)
{
	/* Nobody answers: one announcement per retransmission timeout */
	for (int i = 0; i < 10 * RETX_MS; i++) {
		if (ipc_link_core_pending(&a.core, now)) {
			size_t len;

			(void)ipc_link_core_tx_prepare(&a.core, now, &len);
			zassert_equal(len, IPC_LINK_HDR_SIZE);
			a.log_count++;
		}
		now++;
	}

	zexpect_equal(a.log_count, 10);
}

ZTEST(ipc_link_core, test_message_both_ways)
{
	connect();

	zassert_ok(ipc_link_core_send(&a.core, 1, "hello", 5));
	zassert_ok(ipc_link_core_send(&b.core, 2, "world!", 6));
	zassert_ok(ipc_link_core_send(&a.core, 3, "", 0));
	run();

	zexpect_equal(b.log_count, 2);
	zassert_equal(b.log[0].addr, 1);
	zassert_equal(b.log[0].len, 5);
	zassert_mem_equal(b.log[0].data, "hello", 5);
	zassert_equal(b.log[1].addr, 3);
	zassert_equal(b.log[1].len, 0);

	zexpect_equal(a.log_count, 1);
	zassert_equal(a.log[0].addr, 2);
	zassert_equal(a.log[0].len, 6);
	zassert_mem_equal(a.log[0].data, "world!", 6);

	zexpect_equal(a.core.stats.tx_payloads, 1);
	zexpect_equal(b.core.stats.rx_payloads, 1);
	zexpect_equal(a.core.stats.retransmits, 0);
}

ZTEST(ipc_link_core, test_message_sizes)
{
	connect();

	zassert_equal(ipc_link_core_max_msg_len(&a.core), DATA_MAX);
	zassert_equal(ipc_link_core_send(&a.core, 1, big, DATA_MAX + 1), -EMSGSIZE);
	zassert_true(ipc_link_core_has_room(&a.core, DATA_MAX));
	zassert_false(ipc_link_core_has_room(&a.core, DATA_MAX + 1));

	zassert_ok(ipc_link_core_send(&a.core, 1, big, DATA_MAX));
	/* Doesn't fit in the same frame, so a second frame */
	zassert_ok(ipc_link_core_send(&a.core, 2, big, 1));
	run();

	zexpect_equal(b.log_count, 2);
	zassert_equal(b.log[0].len, DATA_MAX);
	zassert_mem_equal(b.log[0].data, big, DATA_MAX);
	zassert_equal(b.log[1].len, 1);
	zexpect_equal(a.core.stats.tx_payloads, 2);
}

ZTEST(ipc_link_core, test_window_full)
{
	connect();

	/* The window and the frame being filled */
	for (int i = 0; i < WINDOW + 1; i++) {
		zassert_ok(ipc_link_core_send(&a.core, 1, big, DATA_MAX), "message %d", i);
	}

	zassert_false(ipc_link_core_has_room(&a.core, DATA_MAX));
	zassert_equal(ipc_link_core_send(&a.core, 1, big, DATA_MAX), -EAGAIN);

	run();

	zexpect_equal(b.log_count, WINDOW + 1);
	zassert_true(ipc_link_core_has_room(&a.core, DATA_MAX));
}

ZTEST(ipc_link_core, test_full_duplex)
{
	connect();

	/* Keep both sides full: every step carries a payload each way */
	for (int i = 0; i < 200; i++) {
		while (stream_send(&a) == 0) {
		}
		while (stream_send(&b) == 0) {
		}
		step(0U);
	}

	run();

	zexpect_equal(a.stream_received, b.stream_sent);
	zexpect_equal(b.stream_received, a.stream_sent);
	zassert_true(a.stream_sent > 200 * (DATA_MAX / 8), "%u messages", a.stream_sent);
	zexpect_equal(a.core.stats.retransmits, 0);
	zexpect_equal(a.core.stats.rx_dropped, 0);
	zexpect_equal(b.core.stats.rx_dropped, 0);
}

ZTEST(ipc_link_core, test_faults)
{
	connect();

	for (int i = 0; i < 20000; i++) {
		if (rand_next() % 8 == 0) {
			(void)stream_send(&a);
		}
		if (rand_next() % 8 == 0) {
			(void)stream_send(&b);
		}

		step(8U);
	}

	run();

	zexpect_equal(a.stream_received, b.stream_sent);
	zexpect_equal(b.stream_received, a.stream_sent);
	zexpect_true(a.core.stats.retransmits > 0);
	zexpect_true(b.core.stats.header_errors > 0);
	zexpect_true(b.core.stats.payload_errors > 0);
	zexpect_equal(a.connected, 1);
	zexpect_equal(b.connected, 1);
	zexpect_equal(a.disconnected, 0);
}

ZTEST(ipc_link_core, test_credits)
{
	int sent = 0;

	connect();

	b.hold = true;

	while (ipc_link_core_send(&a.core, 1, big, DATA_MAX) == 0) {
		sent++;
		run_for(3 * RETX_MS);
	}

	/* B holds all but the armed buffer, A sent only as many as B had credits and queued the
	 * rest: the window and the frame being filled
	 */
	zexpect_equal(b.held_count, RX_COUNT - 1);
	zexpect_equal(b.log_count, RX_COUNT - 1);
	zassert_equal(sent, RX_COUNT - 1 + WINDOW + 1);
	zexpect_equal(b.core.stats.rx_dropped, 0);
	zassert_false(ipc_link_core_has_room(&a.core, DATA_MAX));

	b.hold = false;
	while (b.held_count > 0) {
		ipc_link_core_rx_release(&b.core, b.held[--b.held_count]);
	}

	run();
	zexpect_equal(b.log_count, sent);
	zexpect_equal(b.core.stats.rx_dropped, 0);
}

ZTEST(ipc_link_core, test_credits_update_lost)
{
	uint32_t start;

	connect();

	b.hold = true;
	for (int i = 0; i < RX_COUNT - 1; i++) {
		zassert_ok(ipc_link_core_send(&a.core, 1, big, DATA_MAX));
	}
	run();
	zexpect_equal(b.held_count, RX_COUNT - 1);
	zassert_equal(a.core.peer_credits, 0);

	/* A waits for credits, and the one header giving them back is lost: A asks again */
	zassert_ok(ipc_link_core_send(&a.core, 1, "more", 4));
	zassert_true(waiting(&a));
	b.hold = false;
	ipc_link_core_rx_release(&b.core, b.held[--b.held_count]);
	a.fault = FAULT_LOST;
	start = now;
	run();

	zexpect_equal(b.log_count, RX_COUNT, "%d messages", b.log_count);
	zassert_true(now - start >= RETX_MS, "%u ms", now - start);
	zassert_true(now - start < 3 * RETX_MS, "%u ms", now - start);
}

ZTEST(ipc_link_core, test_poll_unanswered)
{
	size_t len;

	connect();

	/* B holds every buffer, then falls silent while A waits for credits: the polls for them
	 * go unanswered, and after as many as retransmissions the session ends
	 */
	b.hold = true;
	for (int i = 0; i < RX_COUNT - 1; i++) {
		zassert_ok(ipc_link_core_send(&a.core, 1, big, DATA_MAX));
	}
	run();
	zassert_equal(a.core.peer_credits, 0);
	zassert_ok(ipc_link_core_send(&a.core, 1, "more", 4));

	for (int i = 1; i <= MAX_RETX; i++) {
		now += RETX_MS;
		events(&a, ipc_link_core_check(&a.core, now), 0);
		zassert_true(ipc_link_core_pending(&a.core, now), "poll %d", i);
		(void)ipc_link_core_tx_prepare(&a.core, now, &len);
		zassert_equal(a.core.retx_count, i);
		zassert_equal(a.disconnected, 0);
	}

	now += RETX_MS;
	events(&a, ipc_link_core_check(&a.core, now), 0);
	zexpect_equal(a.disconnected, 1);
	zassert_true(ipc_link_core_booting(&a.core));
}

ZTEST(ipc_link_core, test_poll_answered)
{
	connect();

	/* B holds every buffer for a long time, but answers the polls: the session goes on */
	b.hold = true;
	for (int i = 0; i < RX_COUNT - 1; i++) {
		zassert_ok(ipc_link_core_send(&a.core, 1, big, DATA_MAX));
	}
	run();
	zassert_ok(ipc_link_core_send(&a.core, 1, "more", 4));
	run_for(3 * (MAX_RETX + 1) * RETX_MS);

	zexpect_equal(a.disconnected, 0);
	zassert_true(ipc_link_core_connected(&a.core));
	zexpect_equal(b.log_count, RX_COUNT - 1);

	b.hold = false;
	while (b.held_count > 0) {
		ipc_link_core_rx_release(&b.core, b.held[--b.held_count]);
	}
	run();
	zexpect_equal(b.log_count, RX_COUNT);
}

ZTEST(ipc_link_core, test_idle_in_flight)
{
	size_t len;

	connect();

	/* A payload waits for its acknowledgment without another frame being due */
	zassert_ok(ipc_link_core_send(&a.core, 1, "wait", 4));
	zassert_true(ipc_link_core_pending(&a.core, now));
	(void)ipc_link_core_tx_prepare(&a.core, now, &len);
	zassert_equal(len, IPC_LINK_HDR_SIZE + 8 + IPC_LINK_PCRC_SIZE);
	zassert_true(waiting(&a));

	for (int i = 0; i < RETX_MS - 1; i++) {
		now++;
		zassert_equal(ipc_link_core_check(&a.core, now), 0);
		zassert_false(ipc_link_core_pending(&a.core, now), "frame due at %d ms", i);
	}
}

ZTEST(ipc_link_core, test_retransmit)
{
	uint32_t start;

	connect();

	/* The payload is lost, sent again after the timeout */
	b.fault = FAULT_LOST;
	zassert_ok(ipc_link_core_send(&a.core, 1, "again", 5));
	start = now;
	run();

	zexpect_equal(b.log_count, 1);
	zexpect_equal(a.core.stats.retransmits, 1);
	zassert_true(now - start >= RETX_MS, "%u ms", now - start);

	/* The acknowledgment is lost, the payload is repeated and dropped as a duplicate */
	a.fault = FAULT_LOST;
	zassert_ok(ipc_link_core_send(&a.core, 1, "dup", 3));
	run();

	zexpect_equal(b.log_count, 2);
	zexpect_equal(a.core.stats.retransmits, 2);
	zexpect_equal(b.core.stats.rx_dropped, 1);
	zexpect_equal(a.connected, 1);
}

ZTEST(ipc_link_core, test_retransmit_unsent)
{
	size_t len;

	connect();

	/* A sends a payload once and then can't send at all, as on a bus whose peer never lets
	 * a transaction through: every timeout without the acknowledgment counts, and the
	 * session ends all the same
	 */
	zassert_ok(ipc_link_core_send(&a.core, 1, "stuck", 5));
	(void)ipc_link_core_tx_prepare(&a.core, now, &len);

	for (int i = 1; i <= MAX_RETX; i++) {
		now += RETX_MS;
		events(&a, ipc_link_core_check(&a.core, now), 0);
		zassert_equal(a.core.retx_count, i, "timeout %d", i);
		zassert_true(ipc_link_core_pending(&a.core, now));
		zassert_true(waiting(&a));
		zassert_equal(a.disconnected, 0);
	}

	now += RETX_MS;
	events(&a, ipc_link_core_check(&a.core, now), 0);

	zexpect_equal(a.disconnected, 1);
	zexpect_equal(a.core.stats.retransmits, 1);
	zassert_false(ipc_link_core_connected(&a.core));
	zassert_true(ipc_link_core_booting(&a.core));
}

ZTEST(ipc_link_core, test_never_sent)
{
	connect();

	/* A queues a payload that the transport never gets to send, as on a bus whose peer never
	 * lets a transaction through: every timeout counts from when the wait was first timed
	 */
	zassert_ok(ipc_link_core_send(&a.core, 1, "stuck", 5));
	zassert_true(ipc_link_core_pending(&a.core, now));
	zassert_equal(ipc_link_core_timeout(&a.core, now), RETX_MS);

	for (int i = 1; i <= MAX_RETX; i++) {
		now += RETX_MS;
		events(&a, ipc_link_core_check(&a.core, now), 0);
		zassert_equal(a.core.retx_count, i, "timeout %d", i);
		zassert_equal(a.disconnected, 0);
	}

	now += RETX_MS;
	events(&a, ipc_link_core_check(&a.core, now), 0);

	zexpect_equal(a.disconnected, 1);
	zexpect_equal(a.core.stats.retransmits, 0);
	zassert_true(ipc_link_core_booting(&a.core));

	/* Sent at last, the wait ends: B is back */
	run();
	zexpect_equal(a.connected, 2);
}

ZTEST(ipc_link_core, test_late_ack)
{
	const uint8_t *tx;
	size_t len;

	connect();

	/* B holds a buffer, so it offers one credit at a time */
	b.hold = true;
	zassert_ok(ipc_link_core_send(&a.core, 1, "one", 3));
	run();
	zassert_equal(b.held_count, 1);
	b.hold = false;

	/* A sends a payload that B receives but acknowledges late: A goes back and sends it
	 * again, then the acknowledgment arrives. It covers more than A has in flight, and must
	 * be taken all the same, or A keeps sending the payload and never catches up
	 */
	zassert_ok(ipc_link_core_send(&a.core, 1, "two", 3));
	tx = ipc_link_core_tx_prepare(&a.core, now, &len);
	wire(&b, tx, len);
	zexpect_equal(b.log_count, 2);

	now += RETX_MS;
	events(&a, ipc_link_core_check(&a.core, now), 0);
	zexpect_equal(a.core.stats.retransmits, 1);

	tx = ipc_link_core_tx_prepare(&b.core, now, &len);
	wire(&a, tx, len);
	zassert_equal(a.core.base, a.core.head, "the late acknowledgment was ignored");

	zassert_ok(ipc_link_core_send(&a.core, 1, "three", 5));
	run();
	zexpect_equal(b.log_count, 3);
	zexpect_equal(a.core.stats.retransmits, 1);
	zexpect_equal(a.disconnected, 0);
}

ZTEST(ipc_link_core, test_peer_reboot)
{
	connect();

	zassert_ok(ipc_link_core_send(&a.core, 1, "before", 6));
	run();
	zexpect_equal(b.log_count, 1);

	/* B reboots: A sees a new session and the messages after it */
	side_init(&b);
	zassert_ok(ipc_link_core_send(&a.core, 1, "lost", 4));
	run();

	zexpect_equal(a.connected, 2);
	zexpect_equal(b.connected, 1);
	zassert_equal(a.core.session, 2);
	zexpect_equal(a.core.stats.peer_resets, 1);
	zexpect_equal(b.log_count, 0);

	zassert_ok(ipc_link_core_send(&a.core, 1, "after", 5));
	zassert_ok(ipc_link_core_send(&b.core, 2, "back", 4));
	run();

	zexpect_equal(b.log_count, 1);
	zassert_mem_equal(b.log[0].data, "after", 5);
	zexpect_equal(a.log_count, 1);
	zassert_mem_equal(a.log[0].data, "back", 4);
}

ZTEST(ipc_link_core, test_peer_reboot_in_flight)
{
	connect();

	for (int i = 0; i < WINDOW; i++) {
		zassert_ok(ipc_link_core_send(&a.core, 1, big, DATA_MAX));
	}
	step(0U);

	side_init(&b);
	run();

	zexpect_equal(a.connected, 2);
	zexpect_equal(b.log_count, 0);
	zassert_true(ipc_link_core_has_room(&a.core, DATA_MAX));
}

ZTEST(ipc_link_core, test_reboot_drops_old_session)
{
	connect();

	/* B has a message on its way when A reboots: it belongs to the old session */
	zassert_ok(ipc_link_core_send(&b.core, 1, "stale", 5));
	side_init(&a);
	run();

	zexpect_equal(a.connected, 1);
	zexpect_equal(b.connected, 2);
	zexpect_equal(a.log_count, 0);

	zassert_ok(ipc_link_core_send(&b.core, 1, "fresh", 5));
	run();
	zexpect_equal(a.log_count, 1);
	zassert_mem_equal(a.log[0].data, "fresh", 5);
}

ZTEST(ipc_link_core, test_mtu_mismatch)
{
	struct ipc_link_core_cfg cfg = b.core.cfg;
	const uint8_t *a_tx;
	size_t a_len;
	uint32_t evt;
	uint8_t idx;

	cfg.mtu = MTU / 2;
	ipc_link_core_init(&b.core, &cfg);

	a_tx = ipc_link_core_tx_prepare(&a.core, now, &a_len);
	memcpy(ipc_link_core_rx_buf(&b.core), a_tx, IPC_LINK_HDR_SIZE);
	evt = receive(&b, IPC_LINK_HDR_SIZE, true, &idx);

	zassert_equal(evt, IPC_LINK_CORE_EVT_PEER_MISMATCH);
	zassert_false(ipc_link_core_connected(&b.core));
	zexpect_equal(b.core.stats.peer_mismatches, 1);
}

ZTEST(ipc_link_core, test_mismatch_keeps_probing)
{
	struct ipc_link_core_cfg cfg = b.core.cfg;
	const uint8_t *tx;
	size_t len;
	uint8_t idx;

	cfg.mtu = MTU / 2;
	ipc_link_core_init(&b.core, &cfg);

	/* B announced its boot and gets a mismatched header back: the announcement is repeated
	 * at the usual interval, no sooner and no later
	 */
	(void)ipc_link_core_tx_prepare(&b.core, now, &len);
	tx = ipc_link_core_tx_prepare(&a.core, now, &len);
	memcpy(ipc_link_core_rx_buf(&b.core), tx, IPC_LINK_HDR_SIZE);
	(void)receive(&b, IPC_LINK_HDR_SIZE, true, &idx);

	zassert_false(ipc_link_core_pending(&b.core, now));
	now += RETX_MS;
	zassert_true(ipc_link_core_pending(&b.core, now));
	tx = ipc_link_core_tx_prepare(&b.core, now, &len);
	zassert_equal(len, IPC_LINK_HDR_SIZE);
	zassert_true((tx[1] & IPC_LINK_FLAG_BOOT) != 0U);
}

ZTEST(ipc_link_core, test_peer_answers_garbage)
{
	connect();

	/* B's answers no longer carry a valid header: A's payload is never acknowledged, and
	 * after the retransmissions the session ends, as with a peer that falls silent
	 */
	zassert_ok(ipc_link_core_send(&a.core, 1, "anyone", 6));
	for (int i = 0; i < 2 * (MAX_RETX + 1) * RETX_MS; i++) {
		a.fault = FAULT_HDR;
		step(0U);
	}

	zexpect_equal(a.disconnected, 1);
	zexpect_equal(a.core.stats.retransmits, MAX_RETX);
	zassert_false(ipc_link_core_connected(&a.core));
	zassert_true(ipc_link_core_booting(&a.core));

	/* B answers again: the session restarts */
	a.fault = FAULT_NONE;
	run();
	zexpect_equal(a.connected, 2);
}

ZTEST(ipc_link_core, test_peer_reboots_incompatible)
{
	struct ipc_link_core_cfg cfg = b.core.cfg;
	const uint8_t *tx;
	size_t len;

	connect();
	zassert_ok(ipc_link_core_send(&a.core, 1, "pending", 7));

	/* B comes back with another frame size: the session with it ends, what was queued for
	 * it is dropped, and A announces its boot again
	 */
	cfg.mtu = MTU / 2;
	ipc_link_core_init(&b.core, &cfg);
	for (int i = 0; i < 3; i++) {
		now += RETX_MS;
		(void)ipc_link_core_tx_prepare(&a.core, now, &len);
		tx = ipc_link_core_tx_prepare(&b.core, now, &len);
		memcpy(ipc_link_core_rx_buf(&a.core), tx, IPC_LINK_HDR_SIZE);
		receive_events(&a, IPC_LINK_HDR_SIZE, true);
	}

	zexpect_equal(a.disconnected, 1);
	zassert_false(ipc_link_core_connected(&a.core));
	zassert_true(ipc_link_core_booting(&a.core));
	zexpect_equal(a.core.stats.peer_mismatches, 3);

	/* Back with the right size, it connects again, without what was queued before */
	cfg.mtu = MTU;
	ipc_link_core_init(&b.core, &cfg);
	run();
	zexpect_equal(a.connected, 2);
	zexpect_equal(b.log_count, 0);
}

ZTEST(ipc_link_core, test_short_frame)
{
	uint32_t evt;
	uint8_t idx;

	memset(ipc_link_core_rx_buf(&b.core), 0, MTU);
	evt = receive(&b, IPC_LINK_HDR_SIZE - 1, true, &idx);
	zassert_equal(evt, IPC_LINK_CORE_EVT_HDR_ERR);

	evt = receive(&b, MTU + 1, true, &idx);
	zassert_equal(evt, IPC_LINK_CORE_EVT_HDR_ERR);
	zexpect_equal(b.core.stats.header_errors, 2);
}

ZTEST(ipc_link_core, test_rx_check)
{
	uint8_t *rx = ipc_link_core_rx_buf(&b.core);
	const uint8_t *tx;
	size_t len;

	connect();

	/* A bare header */
	tx = ipc_link_core_tx_prepare(&a.core, now, &len);
	memcpy(rx, tx, len);
	zassert_equal(ipc_link_core_rx_check(&b.core, len, true), IPC_LINK_RX_HDR_OK);
	zassert_equal(ipc_link_core_rx_check(&b.core, len, false), 0);

	/* A payload, then one with a bad CRC and one whose messages don't fill it */
	zassert_ok(ipc_link_core_send(&a.core, 1, "check", 5));
	tx = ipc_link_core_tx_prepare(&a.core, now, &len);
	memcpy(rx, tx, len);
	zassert_equal(ipc_link_core_rx_check(&b.core, len, true),
		      IPC_LINK_RX_HDR_OK | IPC_LINK_RX_PAYLOAD_OK | IPC_LINK_RX_LAYOUT_OK);

	rx[IPC_LINK_HDR_SIZE + 4] ^= 0x01;
	zassert_equal(ipc_link_core_rx_check(&b.core, len, true), IPC_LINK_RX_HDR_OK);
	receive_events(&b, len, true);
	zexpect_equal(b.core.stats.payload_errors, 1);
	zassert_equal(b.core.expected, 0);
	rx[IPC_LINK_HDR_SIZE + 4] ^= 0x01;

	/* The message claims more than the payload holds. Acknowledged, as sending it again
	 * won't fix it, and dropped
	 */
	rx[IPC_LINK_HDR_SIZE + 2] = 9;
	sys_put_le16(crc16_ccitt(0xffff, &rx[IPC_LINK_HDR_SIZE], 12), &rx[IPC_LINK_HDR_SIZE + 12]);
	zassert_equal(ipc_link_core_rx_check(&b.core, len, true),
		      IPC_LINK_RX_HDR_OK | IPC_LINK_RX_PAYLOAD_OK);
	receive_events(&b, len, true);
	zexpect_equal(b.core.stats.payload_errors, 2);
	zassert_equal(b.core.expected, 1);
	zexpect_equal(b.log_count, 0);
}

ZTEST(ipc_link_core, test_disconnect_drops_frame)
{
	uint32_t session;
	const uint8_t *tx;
	size_t len;

	connect();
	session = a.core.session;

	/* B stops acknowledging; A retransmits up to the limit */
	zassert_ok(ipc_link_core_send(&a.core, 1, "lost", 4));
	while (a.core.retx_count < MAX_RETX) {
		if (ipc_link_core_pending(&a.core, now)) {
			(void)ipc_link_core_tx_prepare(&a.core, now, &len);
		}
		now += RETX_MS;
		events(&a, ipc_link_core_check(&a.core, now), 0);
	}
	zassert_equal(a.disconnected, 0);

	/* Sent once more and unacknowledged once more, past the limit: the session ends */
	zassert_true(ipc_link_core_pending(&a.core, now));
	(void)ipc_link_core_tx_prepare(&a.core, now, &len);
	now += RETX_MS;
	events(&a, ipc_link_core_check(&a.core, now), 0);
	zexpect_equal(a.disconnected, 1);
	zassert_equal(a.core.session, session + 1);

	/* B's next frame carries a payload with the sequence number the next session starts
	 * with: it belongs to the session that ended, and B has not acknowledged A's boot
	 */
	zassert_ok(ipc_link_core_send(&b.core, 1, "stale", 5));
	tx = ipc_link_core_tx_prepare(&b.core, now, &len);
	wire(&a, tx, len);

	zexpect_equal(a.log_count, 0);
	zassert_equal(a.core.expected, 0);
	zassert_true(ipc_link_core_booting(&a.core));

	/* The session restarts, and B's first payload comes through */
	side_init(&b);
	run();
	zassert_ok(ipc_link_core_send(&b.core, 1, "fresh", 5));
	run();
	zexpect_equal(a.log_count, 1);
	zassert_mem_equal(a.log[0].data, "fresh", 5);
}

ZTEST(ipc_link_core, test_peer_gone)
{
	size_t len;

	connect();

	/* B falls silent: after the retransmissions A ends the session and announces its boot */
	zassert_ok(ipc_link_core_send(&a.core, 1, "anyone", 6));

	for (int i = 0; i < 2 * (MAX_RETX + 1) * RETX_MS; i++) {
		events(&a, ipc_link_core_check(&a.core, now), 0);

		if (ipc_link_core_pending(&a.core, now)) {
			(void)ipc_link_core_tx_prepare(&a.core, now, &len);
		}

		now++;
	}

	zexpect_equal(a.disconnected, 1);
	zexpect_equal(a.core.stats.retransmits, MAX_RETX);
	zassert_false(ipc_link_core_connected(&a.core));
	zassert_true(ipc_link_core_booting(&a.core));

	/* B is back */
	side_init(&b);
	run();
	zexpect_equal(a.connected, 2);
	zexpect_equal(b.connected, 1);
}
