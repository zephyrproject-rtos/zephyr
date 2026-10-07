/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * The Binding transaction (RFC 8489 §6): the request, the retransmission
 * schedule, which datagrams are believed, and how the server's answer ends it.
 * No sockets and no clock: the test plays both.
 */
#include <errno.h>

#include "stun_test.h"

/* Types no RFC assigns, and what real servers add to their responses. */
#define ATTR_UNKNOWN_REQ     0x7F00U
#define ATTR_RESPONSE_ORIGIN 0x802BU
#define ATTR_OTHER_ADDRESS   0x802CU

/* When the tests start their transactions, on the caller's clock. */
#define T0 1000U

static const uint8_t kV6Addr[16] __maybe_unused = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
						   0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02};
static const uint8_t kV6Server[16] __maybe_unused = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00,
						     0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x34, 0x78};

/* MAPPED-ADDRESS 192.0.2.1 port 32853, for the attributes servers add. */
static const uint8_t kPlainAddr[] = {0x00, 0x01, 0x80, 0x55, 0xc0, 0x00, 0x02, 0x01};

struct fixture {
	struct stun_binding b;
	struct net_sockaddr server;
	uint8_t txid[STUN_TXID_SIZE];
	struct stun_txn_step step;
};

/* A transaction in flight to 203.0.113.1:3478, started at T0. */
static void start_v4(struct fixture *f, uint8_t txid0)
{
	memset(f, 0, sizeof(*f));
	sa_v4(&f->server, 203, 0, 113, 1, 3478);
	fill_txid(f->txid, txid0);
	zassert_ok(stun_binding_start(&f->b, &f->server, f->txid, NULL, NULL, T0, &f->step));
	zassert_equal(f->b.state, STUN_BINDING_IN_FLIGHT);
}

/* A response of `type` to the fixture's transaction. */
static void response(struct raw_msg *r, const struct fixture *f, uint16_t type)
{
	raw_init(r, type, 0);
	memcpy(r->buf + 8, f->txid, STUN_TXID_SIZE);
}

/* XOR-MAPPED-ADDRESS of an IPv4 address, as the server would encode it. */
static void add_mapped_v4(struct raw_msg *r, uint8_t a, uint8_t b, uint8_t c, uint8_t d,
			  uint16_t port)
{
	const uint8_t v[8] = {0x00,
			      0x01,
			      (uint8_t)((port >> 8) ^ 0x21),
			      (uint8_t)(port ^ 0x12),
			      (uint8_t)(a ^ 0x21),
			      (uint8_t)(b ^ 0x12),
			      (uint8_t)(c ^ 0xA4),
			      (uint8_t)(d ^ 0x42)};

	raw_attr(r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(v), v, sizeof(v));
}

/* The same for IPv6: the address is XORed with the cookie and the transaction id. */
static void add_mapped_v6(struct raw_msg *r, const uint8_t ip[16], uint16_t port)
{
	uint8_t v[20] = {0x00, 0x02, (uint8_t)((port >> 8) ^ 0x21), (uint8_t)(port ^ 0x12)};

	for (int i = 0; i < 16; i++) {
		v[4 + i] = ip[i] ^ r->buf[4 + i];
	}
	raw_attr(r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(v), v, sizeof(v));
}

static void add_error(struct raw_msg *r, uint16_t code)
{
	const uint8_t v[4] = {0x00, 0x00, (uint8_t)(code / 100U), (uint8_t)(code % 100U)};

	raw_attr(r, STUN_ATTR_ERROR_CODE, sizeof(v), v, sizeof(v));
}

/* An answer that ends the transaction: on_datagram takes it, whatever the outcome. */
static void deliver(struct fixture *f, const struct raw_msg *r)
{
	zassert_true(stun_binding_owns(&f->b, r->buf, r->len));
	zassert_ok(stun_binding_on_datagram(&f->b, &f->server, r->buf, r->len));
}

/* ============================ the request ============================ */

ZTEST(stun, test_binding_success_ipv4)
{
	struct fixture f;
	struct raw_msg r;
	struct stun_msg m;

	start_v4(&f, 0x10);

	/* The first step is the request: a plain Binding request with our
	 * transaction id and FINGERPRINT, nothing that needs a credential.
	 */
	zassert_equal(f.step.action, STUN_TXN_SEND);
	zassert_ok(stun_parse(f.step.tx, f.step.tx_len, &m));
	zassert_equal(m.type, STUN_BINDING_REQUEST);
	zassert_mem_equal(m.txid, f.txid, STUN_TXID_SIZE);
	zassert_true(m.has_fingerprint);
	zassert_ok(stun_verify_fingerprint(f.step.tx, f.step.tx_len, &m));
	zassert_false(m.has_integrity);
	zassert_is_null(m.username);
	zassert_equal(f.step.tx_len, STUN_HEADER_SIZE + 8U);

	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	deliver(&f, &r);
	zassert_equal(f.b.state, STUN_BINDING_DONE);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_NONE);
	zassert_true(sa_is_v4(&f.b.mapped, 198, 51, 100, 7, 40000));
	zassert_equal(f.b.rx_rejected, 0);

	/* Done stays done. */
	zassert_ok(stun_binding_advance(&f.b, T0 + 100000U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_DONE);
	zassert_ok(stun_binding_advance(&f.b, T0 + 200000U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_DONE);
}

ZTEST(stun, test_binding_ipv6)
{
	struct fixture f;
	struct raw_msg r;

#if defined(CONFIG_NET_IPV6)
	/* An IPv6 server telling an IPv6 address. */
	memset(&f, 0, sizeof(f));
	sa_v6(&f.server, kV6Server, 3478);
	fill_txid(f.txid, 0x11);
	zassert_ok(stun_binding_start(&f.b, &f.server, f.txid, NULL, NULL, T0, &f.step));
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v6(&r, kV6Addr, 40001);
	deliver(&f, &r);
	zassert_equal(f.b.state, STUN_BINDING_DONE);
	zassert_true(sa_is_v6(&f.b.mapped, kV6Addr, 40001));

	/* RFC 8489 §6.3.3: an unexpected but supported family may be used. */
	start_v4(&f, 0x12);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v6(&r, kV6Addr, 40002);
	deliver(&f, &r);
	zassert_equal(f.b.state, STUN_BINDING_DONE);
	zassert_true(sa_is_v6(&f.b.mapped, kV6Addr, 40002));
#else
	struct net_sockaddr_in6 server6 = {.sin6_family = NET_AF_INET6,
					   .sin6_port = net_htons(3478)};

	/* Without the IPv6 stack there is no IPv6 server to talk to... */
	memset(&f, 0, sizeof(f));
	fill_txid(f.txid, 0x11);
	zassert_equal(stun_binding_start(&f.b, (struct net_sockaddr *)&server6, f.txid, NULL, NULL,
					 T0, &f.step),
		      -EAFNOSUPPORT);
	zassert_equal(f.b.state, STUN_BINDING_IDLE);

	/* ...and an IPv6 address in the answer is one this build cannot hold. */
	start_v4(&f, 0x12);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v6(&r, kV6Addr, 40002);
	deliver(&f, &r);
	zassert_equal(f.b.state, STUN_BINDING_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_BAD_RESPONSE);
#endif
}

ZTEST(stun, test_binding_software)
{
	static char long_sw[100U + 1U];
	static char many_chars[STUN_TEXT_MAX_CHARS + 2U];
	struct fixture f;
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;

	/* RFC 8489 §6.1: a request should carry SOFTWARE. */
	memset(&f, 0, sizeof(f));
	sa_v4(&f.server, 203, 0, 113, 1, 3478);
	fill_txid(f.txid, 0x13);
	zassert_ok(stun_binding_start(&f.b, &f.server, f.txid, "zephyr test", NULL, T0, &f.step));
	zassert_ok(stun_parse(f.step.tx, f.step.tx_len, &m));
	zassert_ok(stun_find_attr(f.step.tx, f.step.tx_len, &m, STUN_ATTR_SOFTWARE, &val, &vlen));
	zassert_equal(vlen, strlen("zephyr test"));
	zassert_mem_equal(val, "zephyr test", vlen);
	zassert_true(m.has_fingerprint);
	zassert_ok(stun_verify_fingerprint(f.step.tx, f.step.tx_len, &m));

	/* None when not asked for. */
	start_v4(&f, 0x14);
	zassert_ok(stun_parse(f.step.tx, f.step.tx_len, &m));
	zassert_equal(stun_find_attr(f.step.tx, f.step.tx_len, &m, STUN_ATTR_SOFTWARE, &val, &vlen),
		      -ENOENT);

	/* One the request buffer has no room for, and one no SOFTWARE attribute
	 * may carry: refused, and the transaction is not started.
	 */
	memset(&f, 0, sizeof(f));
	sa_v4(&f.server, 203, 0, 113, 1, 3478);
	fill_txid(f.txid, 0x15);
	memset(long_sw, 's', sizeof(long_sw) - 1U);
	zassert_equal(stun_binding_start(&f.b, &f.server, f.txid, long_sw, NULL, T0, &f.step),
		      -EMSGSIZE);
	zassert_equal(f.b.state, STUN_BINDING_IDLE);
	zassert_equal(f.step.action, STUN_TXN_NONE);

	memset(many_chars, 's', sizeof(many_chars) - 1U);
	zassert_equal(stun_binding_start(&f.b, &f.server, f.txid, many_chars, NULL, T0, &f.step),
		      -EINVAL);
	zassert_equal(f.b.state, STUN_BINDING_IDLE);
}

/* ====================== the retransmission schedule ====================== */

/* Advance at `at` and expect a retransmission of the very same request. */
static void expect_send(struct fixture *f, uint32_t at, const uint8_t *first, size_t first_len)
{
	zassert_ok(stun_binding_advance(&f->b, at, &f->step));
	zassert_equal(f->step.action, STUN_TXN_SEND, "a transmission is due at +%u", at - T0);
	zassert_equal_ptr(f->step.tx, first);
	zassert_equal(f->step.tx_len, first_len);
}

static void expect_wait(struct fixture *f, uint32_t at, uint32_t until)
{
	zassert_ok(stun_binding_advance(&f->b, at, &f->step));
	zassert_equal(f->step.action, STUN_TXN_WAIT, "nothing is due at +%u", at - T0);
	zassert_equal(f->step.wait_until_ms, until);
}

ZTEST(stun, test_binding_default_schedule)
{
	/* RFC 8489 §6.2.1 with RTO 500 ms, Rc 7, Rm 16: requests at these times,
	 * and the transaction fails 39500 ms after the first.
	 */
	static const uint32_t kSendAt[] = {500, 1500, 3500, 7500, 15500, 31500};
	const uint8_t *first;
	size_t first_len;
	struct fixture f;

	start_v4(&f, 0x20);
	first = f.step.tx;
	first_len = f.step.tx_len;

	for (size_t i = 0; i < ARRAY_SIZE(kSendAt); i++) {
		expect_wait(&f, T0 + kSendAt[i] - 1U, T0 + kSendAt[i]);
		expect_send(&f, T0 + kSendAt[i], first, first_len);
	}

	expect_wait(&f, T0 + 31500U, T0 + 39500U);
	expect_wait(&f, T0 + 39499U, T0 + 39500U);
	zassert_equal(f.b.state, STUN_BINDING_IN_FLIGHT);

	zassert_ok(stun_binding_advance(&f.b, T0 + 39500U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);
	zassert_equal(f.b.state, STUN_BINDING_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_TIMEOUT);

	/* Failed stays failed. */
	zassert_ok(stun_binding_advance(&f.b, T0 + 50000U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_TIMEOUT);
}

ZTEST(stun, test_binding_policy)
{
	static const struct stun_txn_policy kFast = {
		.rto_ms = 100, .max_transmits = 3, .final_wait_rto = 4};
	static const struct stun_txn_policy kBad[] = {
		{.rto_ms = 0, .max_transmits = 7, .final_wait_rto = 16},
		{.rto_ms = 60001, .max_transmits = 7, .final_wait_rto = 16},
		{.rto_ms = 500, .max_transmits = 0, .final_wait_rto = 16},
		{.rto_ms = 500, .max_transmits = 17, .final_wait_rto = 16},
		{.rto_ms = 500, .max_transmits = 7, .final_wait_rto = 0},
	};
	const uint8_t *first;
	size_t first_len;
	struct fixture f;

	/* Requests at 0, 100 and 300 ms; failure 4 RTO after the last. */
	memset(&f, 0, sizeof(f));
	sa_v4(&f.server, 203, 0, 113, 1, 3478);
	fill_txid(f.txid, 0x21);
	zassert_ok(stun_binding_start(&f.b, &f.server, f.txid, NULL, &kFast, T0, &f.step));
	first = f.step.tx;
	first_len = f.step.tx_len;
	expect_wait(&f, T0 + 99U, T0 + 100U);
	expect_send(&f, T0 + 100U, first, first_len);
	expect_send(&f, T0 + 300U, first, first_len);
	expect_wait(&f, T0 + 699U, T0 + 700U);
	zassert_ok(stun_binding_advance(&f.b, T0 + 700U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_TIMEOUT);

	/* A single transmission is a legal policy. */
	memset(&f, 0, sizeof(f));
	sa_v4(&f.server, 203, 0, 113, 1, 3478);
	fill_txid(f.txid, 0x22);
	zassert_ok(stun_binding_start(
		&f.b, &f.server, f.txid, NULL,
		&(struct stun_txn_policy){.rto_ms = 200, .max_transmits = 1, .final_wait_rto = 2},
		T0, &f.step));
	expect_wait(&f, T0 + 399U, T0 + 400U);
	zassert_ok(stun_binding_advance(&f.b, T0 + 400U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);

	for (size_t i = 0; i < ARRAY_SIZE(kBad); i++) {
		memset(&f, 0, sizeof(f));
		sa_v4(&f.server, 203, 0, 113, 1, 3478);
		fill_txid(f.txid, 0x23);
		zassert_equal(
			stun_binding_start(&f.b, &f.server, f.txid, NULL, &kBad[i], T0, &f.step),
			-EINVAL, "policy #%zu", i);
		zassert_equal(f.b.state, STUN_BINDING_IDLE, "policy #%zu", i);
	}
}

ZTEST(stun, test_binding_clock)
{
	const uint32_t start = 0xFFFFFF00U;
	const uint8_t *first;
	size_t first_len;
	struct fixture f;

	/* The clock wraps between the request and its first retransmission. */
	memset(&f, 0, sizeof(f));
	sa_v4(&f.server, 203, 0, 113, 1, 3478);
	fill_txid(f.txid, 0x24);
	zassert_ok(stun_binding_start(&f.b, &f.server, f.txid, NULL, NULL, start, &f.step));
	first = f.step.tx;
	first_len = f.step.tx_len;
	zassert_ok(stun_binding_advance(&f.b, start + 499U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_WAIT);
	zassert_equal(f.step.wait_until_ms, start + 500U, "a time after the wrap");
	zassert_true(f.step.wait_until_ms < start);
	zassert_ok(stun_binding_advance(&f.b, start + 500U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_SEND);

	/* A caller that comes late moves the schedule with it: the next
	 * interval is counted from the transmission it made.
	 */
	start_v4(&f, 0x25);
	first = f.step.tx;
	first_len = f.step.tx_len;
	expect_send(&f, T0 + 700U, first, first_len);
	expect_wait(&f, T0 + 1699U, T0 + 1700U);
	expect_send(&f, T0 + 1700U, first, first_len);
}

/* RFC 8489 section 6.2.1: the client retransmits "starting with an interval
 * of RTO, doubling after each retransmission", and after the last request it
 * waits Rm * RTO. The intervals are between transmissions. A caller that
 * comes late therefore moves the rest of the schedule: it does not get to
 * send twice in a row to catch up, and the final wait follows the last
 * transmission that was actually made.
 */
ZTEST(stun, test_binding_late_caller_keeps_the_intervals)
{
	const uint8_t *first;
	size_t first_len;
	struct fixture f;

	start_v4(&f, 0x26);
	first = f.step.tx;
	first_len = f.step.tx_len;

	/* 1.1 s late for the first retransmission (due at +500). */
	expect_send(&f, T0 + 1600U, first, first_len);
	/* The next interval is 2 * RTO after that transmission, not after the
	 * time it was due: nothing to send right now.
	 */
	expect_wait(&f, T0 + 1600U, T0 + 2600U);
	expect_wait(&f, T0 + 2599U, T0 + 2600U);
	expect_send(&f, T0 + 2600U, first, first_len);
	expect_wait(&f, T0 + 2600U, T0 + 4600U);
}

ZTEST(stun, test_binding_stall_does_not_end_the_transaction_at_once)
{
	const uint8_t *first;
	size_t first_len;
	struct fixture f;

	/* The caller was away longer than the whole RFC schedule (39.5 s). On
	 * its return the transaction sends once and waits; six transmissions
	 * in a row and an immediate failure would give the server no chance to
	 * answer the request that was just sent.
	 */
	start_v4(&f, 0x27);
	first = f.step.tx;
	first_len = f.step.tx_len;

	expect_send(&f, T0 + 40000U, first, first_len); /* transmission 2 */
	expect_wait(&f, T0 + 40000U, T0 + 41000U);
	expect_send(&f, T0 + 41000U, first, first_len); /* 3 */
	expect_wait(&f, T0 + 41000U, T0 + 43000U);
	expect_send(&f, T0 + 43000U, first, first_len); /* 4 */
	expect_send(&f, T0 + 47000U, first, first_len); /* 5 */
	expect_send(&f, T0 + 55000U, first, first_len); /* 6 */
	expect_send(&f, T0 + 71000U, first, first_len); /* 7, the last */
	/* Rm * RTO = 8 s after the last transmission, not before. */
	expect_wait(&f, T0 + 71000U, T0 + 79000U);
	expect_wait(&f, T0 + 78999U, T0 + 79000U);
	zassert_ok(stun_binding_advance(&f.b, T0 + 79000U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_TIMEOUT);
}

/* ==================== which datagrams are believed ==================== */

ZTEST(stun, test_binding_rejects_what_is_not_the_answer)
{
	static const uint8_t kGarbage[24] = {0x80, 0x60, 0x12, 0x34};
	struct net_sockaddr other;
	struct fixture f;
	struct raw_msg r;
	uint32_t rejected = 0;

	start_v4(&f, 0x30);

	/* Another transaction's response. */
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	r.buf[8] ^= 0x01;
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	zassert_false(stun_binding_owns(&f.b, r.buf, r.len));
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -ENOENT);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* The right response from another address, and from another port: an
	 * off-path sender must not be able to plant an address.
	 */
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	sa_v4(&other, 203, 0, 113, 2, 3478);
	zassert_equal(stun_binding_on_datagram(&f.b, &other, r.buf, r.len), -EACCES);
	zassert_equal(f.b.rx_rejected, ++rejected);
	sa_v4(&other, 203, 0, 113, 1, 3479);
	zassert_equal(stun_binding_on_datagram(&f.b, &other, r.buf, r.len), -EACCES);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* Not STUN at all. */
	zassert_false(stun_binding_owns(&f.b, kGarbage, sizeof(kGarbage)));
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, kGarbage, sizeof(kGarbage)),
		      -ENOENT);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* A request with our transaction id is not a response to it. */
	response(&r, &f, STUN_BINDING_REQUEST);
	zassert_false(stun_binding_owns(&f.b, r.buf, r.len));
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -ENOENT);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* A response of another method. */
	response(&r, &f, stun_msg_type(0x003, STUN_CLASS_SUCCESS));
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	zassert_false(stun_binding_owns(&f.b, r.buf, r.len));
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -ENOENT);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* Ours, from the server, but damaged on the way: FINGERPRINT fails. */
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	raw_fingerprint(&r);
	r.buf[STUN_HEADER_SIZE + 4U + 7U] ^= 0x01;
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -EBADMSG);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* Ours, from the server, but malformed. */
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	raw_u32(&r, STUN_ATTR_PRIORITY, 3, 0);
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -EBADMSG);
	zassert_equal(f.b.rx_rejected, ++rejected);

	/* None of that ended the transaction or left an address behind. */
	zassert_equal(f.b.state, STUN_BINDING_IN_FLIGHT);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_NONE);
	zassert_ok(stun_binding_advance(&f.b, T0 + 1U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_WAIT);

	/* The real answer still gets through. */
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	raw_fingerprint(&r);
	deliver(&f, &r);
	zassert_equal(f.b.state, STUN_BINDING_DONE);
	zassert_true(sa_is_v4(&f.b.mapped, 198, 51, 100, 7, 40000));
	zassert_equal(f.b.rx_rejected, rejected, "the answer is not a rejection");
}

/* ================== how the server's answer ends it ================== */

static void expect_failed(const struct fixture *f, enum stun_binding_fail why, uint16_t code)
{
	zassert_equal(f->b.state, STUN_BINDING_FAILED);
	zassert_equal(f->b.fail, why);
	zassert_equal(f->b.error_code, code);
	zassert_equal(f->b.rx_rejected, 0, "an answer that fails the transaction is not rejected");
}

ZTEST(stun, test_binding_unusable_response)
{
	struct fixture f;
	struct raw_msg r;

	/* RFC 8489 §6.3.3: an unknown comprehension-required attribute in a
	 * success response — discarded, and the transaction has failed.
	 */
	start_v4(&f, 0x40);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	raw_attr(&r, ATTR_UNKNOWN_REQ, 4, "abcd", 4);
	deliver(&f, &r);
	expect_failed(&f, STUN_BINDING_FAIL_BAD_RESPONSE, 0);
	zassert_ok(stun_binding_advance(&f.b, T0 + 1U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);

	/* §6.3.4: the same in an error response. */
	start_v4(&f, 0x41);
	response(&r, &f, STUN_BINDING_ERROR_RESPONSE);
	add_error(&r, 400);
	raw_attr(&r, ATTR_UNKNOWN_REQ, 4, "abcd", 4);
	deliver(&f, &r);
	expect_failed(&f, STUN_BINDING_FAIL_BAD_RESPONSE, 0);

	/* §6.3.3: a success response without XOR-MAPPED-ADDRESS. */
	start_v4(&f, 0x42);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	raw_attr(&r, STUN_ATTR_SOFTWARE, 4, "test", 4);
	deliver(&f, &r);
	expect_failed(&f, STUN_BINDING_FAIL_BAD_RESPONSE, 0);

	/* The legacy MAPPED-ADDRESS alone is not it. */
	start_v4(&f, 0x43);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	raw_attr(&r, STUN_ATTR_MAPPED_ADDRESS, 8, kPlainAddr, 8);
	deliver(&f, &r);
	expect_failed(&f, STUN_BINDING_FAIL_BAD_RESPONSE, 0);
}

ZTEST(stun, test_binding_error_response)
{
	static const uint16_t kCodes[] = {300, 401, 420, 500, 699};
	struct fixture f;
	struct raw_msg r;

	for (size_t i = 0; i < ARRAY_SIZE(kCodes); i++) {
		start_v4(&f, (uint8_t)(0x50 + i));
		response(&r, &f, STUN_BINDING_ERROR_RESPONSE);
		add_error(&r, kCodes[i]);
		deliver(&f, &r);
		expect_failed(&f, STUN_BINDING_FAIL_ERROR_RESPONSE, kCodes[i]);
		zassert_ok(stun_binding_advance(&f.b, T0 + 1U, &f.step));
		zassert_equal(f.step.action, STUN_TXN_FAILED);
	}

	/* RFC 8489 §6.3.4: an error response without ERROR-CODE. */
	start_v4(&f, 0x58);
	response(&r, &f, STUN_BINDING_ERROR_RESPONSE);
	deliver(&f, &r);
	expect_failed(&f, STUN_BINDING_FAIL_ERROR_RESPONSE, 0);
}

ZTEST(stun, test_binding_responses_of_live_servers)
{
	/* The attribute sets of parse.c's test_responses_of_live_servers, as
	 * answers to a transaction.
	 */
	static const char kSoftware[] = "example STUN server";
	struct fixture f;
	struct raw_msg r;

	for (int shape = 0; shape < 5; shape++) {
		start_v4(&f, (uint8_t)(0x60 + shape));
		response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
		add_mapped_v4(&r, 192, 0, 2, 1, 32853);
		if (shape >= 1) {
			raw_attr(&r, STUN_ATTR_MAPPED_ADDRESS, 8, kPlainAddr, 8);
		}
		if (shape >= 2) {
			raw_attr(&r, ATTR_RESPONSE_ORIGIN, 8, kPlainAddr, 8);
		}
		if (shape == 2 || shape == 3) {
			raw_attr(&r, ATTR_OTHER_ADDRESS, 8, kPlainAddr, 8);
		}
		if (shape == 1 || shape >= 3) {
			raw_attr(&r, STUN_ATTR_SOFTWARE, sizeof(kSoftware) - 1U, kSoftware,
				 sizeof(kSoftware) - 1U);
		}
		if (shape == 4) {
			raw_fingerprint(&r);
		}
		deliver(&f, &r);
		zassert_equal(f.b.state, STUN_BINDING_DONE, "shape %d", shape);
		zassert_true(sa_is_v4(&f.b.mapped, 192, 0, 2, 1, 32853), "shape %d", shape);
	}
}

/* ====================== cancel, restart, misuse ====================== */

ZTEST(stun, test_binding_cancel)
{
	struct fixture f;
	struct raw_msg r;

	start_v4(&f, 0x70);
	stun_binding_cancel(&f.b);
	zassert_equal(f.b.state, STUN_BINDING_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_CANCELLED);
	zassert_ok(stun_binding_advance(&f.b, T0 + 1U, &f.step));
	zassert_equal(f.step.action, STUN_TXN_FAILED);

	/* An answer that comes too late changes nothing. */
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -EALREADY);
	zassert_equal(f.b.state, STUN_BINDING_FAILED);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_CANCELLED);

	/* Cancelling again, or what is not in flight, does nothing. */
	stun_binding_cancel(&f.b);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_CANCELLED);

	start_v4(&f, 0x71);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	deliver(&f, &r);
	stun_binding_cancel(&f.b);
	zassert_equal(f.b.state, STUN_BINDING_DONE, "a finished transaction stays finished");

	memset(&f, 0, sizeof(f));
	stun_binding_cancel(&f.b);
	zassert_equal(f.b.state, STUN_BINDING_IDLE);
	stun_binding_cancel(NULL);
}

ZTEST(stun, test_binding_restart)
{
	uint8_t first_txid[STUN_TXID_SIZE];
	uint8_t other_txid[STUN_TXID_SIZE];
	struct stun_txn_step step;
	struct fixture f;
	struct raw_msg r;
	struct stun_msg m;

	/* Not while one is in flight: the transaction and its request stay. */
	start_v4(&f, 0x72);
	memcpy(first_txid, f.txid, sizeof(first_txid));
	fill_txid(other_txid, 0x73);
	zassert_equal(stun_binding_start(&f.b, &f.server, other_txid, NULL, NULL, T0 + 10U, &step),
		      -EALREADY);
	zassert_equal(step.action, STUN_TXN_NONE);
	zassert_equal(f.b.state, STUN_BINDING_IN_FLIGHT);
	zassert_ok(stun_parse(f.step.tx, f.step.tx_len, &m));
	zassert_mem_equal(m.txid, first_txid, STUN_TXID_SIZE);

	/* After it has ended, a new one starts clean. */
	response(&r, &f, STUN_BINDING_ERROR_RESPONSE);
	add_error(&r, 500);
	deliver(&f, &r);
	zassert_equal(f.b.state, STUN_BINDING_FAILED);

	memcpy(f.txid, other_txid, sizeof(f.txid));
	zassert_ok(stun_binding_start(&f.b, &f.server, f.txid, NULL, NULL, T0 + 5000U, &f.step));
	zassert_equal(f.b.state, STUN_BINDING_IN_FLIGHT);
	zassert_equal(f.b.fail, STUN_BINDING_FAIL_NONE);
	zassert_equal(f.b.error_code, 0);
	zassert_ok(stun_parse(f.step.tx, f.step.tx_len, &m));
	zassert_mem_equal(m.txid, other_txid, STUN_TXID_SIZE);
	expect_wait(&f, T0 + 5499U, T0 + 5500U);

	/* The answer to the first request is no longer ours. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0);
	memcpy(r.buf + 8, first_txid, STUN_TXID_SIZE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	zassert_equal(stun_binding_on_datagram(&f.b, &f.server, r.buf, r.len), -ENOENT);
}

ZTEST(stun, test_binding_refused_restart_changes_nothing)
{
	static char many_chars[STUN_TEXT_MAX_CHARS + 2U];
	uint8_t other_txid[STUN_TXID_SIZE];
	struct stun_txn_step step;
	struct fixture f;
	struct raw_msg r;

	/* A finished transaction, then a restart that is refused: the result
	 * stays, and so does the request it belongs to.
	 */
	start_v4(&f, 0x76);
	response(&r, &f, STUN_BINDING_SUCCESS_RESPONSE);
	add_mapped_v4(&r, 198, 51, 100, 7, 40000);
	deliver(&f, &r);

	fill_txid(other_txid, 0x77);
	memset(many_chars, 's', sizeof(many_chars) - 1U);
	zassert_equal(stun_binding_start(&f.b, &f.server, other_txid, many_chars, NULL, T0, &step),
		      -EINVAL);
	zassert_equal(f.b.state, STUN_BINDING_DONE);
	zassert_true(sa_is_v4(&f.b.mapped, 198, 51, 100, 7, 40000));
	zassert_true(stun_binding_owns(&f.b, r.buf, r.len), "still the first request's answer");

	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0);
	memcpy(r.buf + 8, other_txid, STUN_TXID_SIZE);
	zassert_false(stun_binding_owns(&f.b, r.buf, r.len), "the refused request never existed");
}

ZTEST(stun, test_binding_bad_arguments)
{
	struct net_sockaddr server;
	struct stun_txn_step step;
	struct stun_binding b;
	uint8_t txid[STUN_TXID_SIZE];
	uint8_t buf[STUN_HEADER_SIZE] = {0};

	memset(&b, 0, sizeof(b));
	sa_v4(&server, 203, 0, 113, 1, 3478);
	fill_txid(txid, 0x74);

	zassert_equal(stun_binding_start(NULL, &server, txid, NULL, NULL, T0, &step), -EINVAL);
	zassert_equal(stun_binding_start(&b, NULL, txid, NULL, NULL, T0, &step), -EINVAL);
	zassert_equal(stun_binding_start(&b, &server, NULL, NULL, NULL, T0, &step), -EINVAL);
	zassert_equal(stun_binding_start(&b, &server, txid, NULL, NULL, T0, NULL), -EINVAL);

	/* A server without a port, and one of no known family. */
	sa_v4(&server, 203, 0, 113, 1, 0);
	zassert_equal(stun_binding_start(&b, &server, txid, NULL, NULL, T0, &step), -EINVAL);
	sa_v4(&server, 203, 0, 113, 1, 3478);
	net_sin(&server)->sin_family = NET_AF_UNSPEC;
	zassert_equal(stun_binding_start(&b, &server, txid, NULL, NULL, T0, &step), -EAFNOSUPPORT);
	zassert_equal(b.state, STUN_BINDING_IDLE);

	/* Never started. */
	zassert_equal(stun_binding_advance(&b, T0, &step), -EPERM);
	zassert_equal(step.action, STUN_TXN_NONE);
	zassert_equal(stun_binding_on_datagram(&b, &server, buf, sizeof(buf)), -EALREADY);
	zassert_false(stun_binding_owns(&b, buf, sizeof(buf)));

	zassert_equal(stun_binding_advance(NULL, T0, &step), -EINVAL);
	zassert_equal(stun_binding_advance(&b, T0, NULL), -EINVAL);
	zassert_equal(stun_binding_on_datagram(NULL, &server, buf, sizeof(buf)), -EINVAL);
	zassert_equal(stun_binding_on_datagram(&b, NULL, buf, sizeof(buf)), -EINVAL);
	zassert_equal(stun_binding_on_datagram(&b, &server, NULL, sizeof(buf)), -EINVAL);
	zassert_false(stun_binding_owns(NULL, buf, sizeof(buf)));
	zassert_false(stun_binding_owns(&b, NULL, sizeof(buf)));
}
