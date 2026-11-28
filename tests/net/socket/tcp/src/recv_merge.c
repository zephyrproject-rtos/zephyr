/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "../../socket_helpers.h"

int wait_for_n_tcp_contexts(int exp_num_contexts, k_timeout_t timeout);

/* Merging of small segments in the socket receive queue */

#define MERGE_V4_ADDR "127.0.0.1"
#define MERGE_V6_ADDR "::1"
#define MERGE_SERVER_PORT 4242
#define MERGE_TEARDOWN_TIMEOUT K_SECONDS(3)

#define MERGE_SEG_LEN 10
#define MERGE_SEG_COUNT 100
#define MERGE_DATA_LEN 2048
#define MERGE_SETTLE_MS 100
/* Size of a fixed size buffer; a stand-in to size the segments otherwise */
#define MERGE_BUF_SIZE                                                                     \
	COND_CODE_1(CONFIG_NET_BUF_FIXED_DATA_SIZE, (CONFIG_NET_BUF_DATA_SIZE), (128))
/* IPv4 and TCP headers, which stay in the first buffer of a packet */
#define MERGE_V4_HDR_LEN 40
/* Buffers a queued packet takes before the next segment starts a new one */
#define MERGE_BUF_COUNT DIV_ROUND_UP(CONFIG_NET_BUF_RX_COUNT, CONFIG_NET_PKT_RX_COUNT)

BUILD_ASSERT(MERGE_SEG_COUNT > CONFIG_NET_PKT_RX_COUNT);
BUILD_ASSERT(MERGE_BUF_COUNT > 1);

struct merge_test {
	int c_sock;
	int s_sock;
	int a_sock;
	int base_pkts;
	int base_bufs;
	size_t tx_off;
	size_t rx_off;
	struct k_work_delayable tx_work;
	size_t tx_work_len;
};

static uint8_t merge_tx[MERGE_DATA_LEN];
static uint8_t merge_rx[MERGE_DATA_LEN];

static int rx_pkts_used(void)
{
	struct k_mem_slab *rx;

	net_pkt_get_info(&rx, NULL, NULL, NULL);

	return k_mem_slab_num_used_get(rx);
}

static int rx_bufs_used(void)
{
	struct net_buf_pool *rx_data;

	net_pkt_get_info(NULL, NULL, &rx_data, NULL);

	return rx_data->buf_count - atomic_get(&rx_data->avail_count);
}

static void merge_close(int sock)
{
	zassert_ok(zsock_close(sock), "close failed (%d)", errno);
}

static void merge_connect(struct merge_test *t, struct net_sockaddr *addr, net_socklen_t addrlen)
{
	zassert_ok(zsock_bind(t->s_sock, addr, addrlen), "bind failed (%d)", errno);
	zassert_ok(zsock_listen(t->s_sock, 1), "listen failed (%d)", errno);
	zassert_ok(zsock_connect(t->c_sock, addr, addrlen), "connect failed (%d)", errno);
}

static void merge_accept(struct merge_test *t)
{
	t->a_sock = zsock_accept(t->s_sock, NULL, NULL);
	zassert_true(t->a_sock >= 0, "accept failed (%d)", errno);
}

static void merge_setup(struct merge_test *t, net_sa_family_t family, bool accept)
{
	struct net_sockaddr_in6 c_saddr6;
	struct net_sockaddr_in6 s_saddr6;
	struct net_sockaddr_in c_saddr;
	struct net_sockaddr_in s_saddr;
	int nodelay = 1;
	int rv;

	memset(t, 0, sizeof(*t));
	t->a_sock = -1;

	for (size_t i = 0; i < sizeof(merge_tx); i++) {
		merge_tx[i] = (uint8_t)(i ^ (i >> 8));
	}

	/* Packets of an earlier test may still be on their way */
	k_msleep(MERGE_SETTLE_MS);
	t->base_pkts = rx_pkts_used();
	t->base_bufs = rx_bufs_used();

	if (family == NET_AF_INET) {
		prepare_sock_tcp_v4(MERGE_V4_ADDR, 0, &t->c_sock, &c_saddr);
		prepare_sock_tcp_v4(MERGE_V4_ADDR, MERGE_SERVER_PORT, &t->s_sock, &s_saddr);
		merge_connect(t, (struct net_sockaddr *)&s_saddr, sizeof(s_saddr));
	} else {
		prepare_sock_tcp_v6(MERGE_V6_ADDR, 0, &t->c_sock, &c_saddr6);
		prepare_sock_tcp_v6(MERGE_V6_ADDR, MERGE_SERVER_PORT, &t->s_sock, &s_saddr6);
		merge_connect(t, (struct net_sockaddr *)&s_saddr6, sizeof(s_saddr6));
	}

	rv = zsock_setsockopt(t->c_sock, NET_IPPROTO_TCP, ZSOCK_TCP_NODELAY, &nodelay,
			      sizeof(nodelay));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	if (accept) {
		merge_accept(t);
	}

	/* Let the handshake packets go */
	k_msleep(MERGE_SETTLE_MS);
}

/* Close what is still open and check that no packet or buffer is left behind */
static void merge_teardown(struct merge_test *t)
{
	if (t->c_sock >= 0) {
		merge_close(t->c_sock);
	}

	if (t->a_sock >= 0) {
		merge_close(t->a_sock);
	}

	merge_close(t->s_sock);

	zassert_ok(wait_for_n_tcp_contexts(0, MERGE_TEARDOWN_TIMEOUT),
		   "Not all TCP contexts properly cleaned up");

	zassert_equal(rx_pkts_used(), t->base_pkts, "RX packets leaked");
	zassert_equal(rx_bufs_used(), t->base_bufs, "RX buffers leaked");
}

/* Send the next len bytes of the test data as a segment of their own */
static void merge_send(struct merge_test *t, size_t len)
{
	zassert_true(t->tx_off + len <= sizeof(merge_tx), "test data too short");

	zassert_equal(zsock_send(t->c_sock, &merge_tx[t->tx_off], len, 0), len,
		      "send failed (%d)", errno);
	t->tx_off += len;

	/* Let the segment through before the next one */
	k_msleep(1);
}

static void merge_send_segments(struct merge_test *t, size_t count, size_t len)
{
	for (size_t i = 0; i < count; i++) {
		merge_send(t, len);
	}
}

/* Read len bytes, at most chunk bytes at a time, and compare them to what was sent */
static void merge_recv(struct merge_test *t, size_t len, size_t chunk)
{
	size_t end = t->rx_off + len;
	ssize_t rv;

	zassert_true(end <= t->tx_off, "more than was sent");

	while (t->rx_off < end) {
		rv = zsock_recv(t->a_sock, &merge_rx[t->rx_off], MIN(chunk, end - t->rx_off),
				ZSOCK_MSG_DONTWAIT);
		zassert_true(rv > 0, "recv failed after %zu bytes (%d)", t->rx_off, errno);
		t->rx_off += rv;
	}

	zassert_mem_equal(merge_rx, merge_tx, t->rx_off, "wrong data");
}

static void merge_recv_all(struct merge_test *t)
{
	char c;

	merge_recv(t, t->tx_off - t->rx_off, sizeof(merge_rx));

	zassert_equal(zsock_recv(t->a_sock, &c, sizeof(c), ZSOCK_MSG_DONTWAIT), -1,
		      "more data than was sent");
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);
}

/* Check the packets and buffers the unread data takes. Without fixed size
 * buffers there is no room to copy into, so only the packets are bounded.
 */
static void merge_expect_held(struct merge_test *t, int pkts, int bufs)
{
	int unread;

	k_msleep(MERGE_SETTLE_MS);

	if (IS_ENABLED(CONFIG_NET_BUF_FIXED_DATA_SIZE)) {
		zassert_equal(rx_pkts_used() - t->base_pkts, pkts, "%d RX packets held",
			      rx_pkts_used() - t->base_pkts);
		zassert_equal(rx_bufs_used() - t->base_bufs, bufs, "%d RX buffers held",
			      rx_bufs_used() - t->base_bufs);
	} else {
		zassert_true(rx_pkts_used() - t->base_pkts <= pkts, "%d RX packets held",
			     rx_pkts_used() - t->base_pkts);
	}

	if (t->a_sock >= 0) {
		zassert_ok(zsock_ioctl(t->a_sock, ZFD_IOCTL_FIONREAD, &unread));
		zassert_equal(unread, t->tx_off - t->rx_off, "%d bytes unread", unread);
	}
}

/* Most RX packets small segments may take: a packet collects them until it has
 * MERGE_BUF_COUNT buffers, the first of which also carries the headers.
 */
static int merge_pkts_limit(size_t unread)
{
	return DIV_ROUND_UP(unread, (MERGE_BUF_COUNT - 1) * MERGE_BUF_SIZE);
}

/* Small segments queued on a socket nobody reads must not take a packet each */
static void test_merge_unread_common(net_sa_family_t family)
{
	struct merge_test t;
	int held;

	if (!IS_ENABLED(CONFIG_NET_BUF_FIXED_DATA_SIZE)) {
		ztest_test_skip();
	}

	merge_setup(&t, family, false);

	/* The connection is not accepted yet */
	merge_send_segments(&t, MERGE_SEG_COUNT, MERGE_SEG_LEN);
	k_msleep(MERGE_SETTLE_MS);

	held = rx_pkts_used() - t.base_pkts;
	zassert_true(held <= merge_pkts_limit(t.tx_off), "%d RX packets held", held);

	merge_accept(&t);

	/* Leave the first packet partly read while more segments get merged */
	merge_recv(&t, MERGE_SEG_LEN / 2, sizeof(merge_rx));

	merge_send_segments(&t, MERGE_SEG_COUNT, MERGE_SEG_LEN);
	k_msleep(MERGE_SETTLE_MS);

	held = rx_pkts_used() - t.base_pkts;
	zassert_true(held <= merge_pkts_limit(t.tx_off), "%d RX packets held", held);

	merge_recv_all(&t);

	merge_teardown(&t);
}

ZTEST(net_socket_tcp, test_v4_merge_unread)
{
	test_merge_unread_common(NET_AF_INET);
}

ZTEST(net_socket_tcp, test_v6_merge_unread)
{
	test_merge_unread_common(NET_AF_INET6);
}

/* Segments that fit in the free room of the last buffer are copied there */
ZTEST(net_socket_tcp, test_v4_merge_copy)
{
	const size_t room = MERGE_BUF_SIZE - MERGE_V4_HDR_LEN;
	struct merge_test t;

	merge_setup(&t, NET_AF_INET, true);

	merge_send(&t, room / 2);
	merge_expect_held(&t, 1, 1);

	/* Fills the buffer to the last byte */
	merge_send(&t, room - room / 2);
	merge_expect_held(&t, 1, 1);

	/* No room left, the buffer of the segment is taken over */
	merge_send(&t, 1);
	merge_expect_held(&t, 1, 2);

	/* And is the one the next segments are copied to */
	merge_send_segments(&t, 3, MERGE_SEG_LEN);
	merge_expect_held(&t, 1, 2);

	merge_recv_all(&t);
	merge_expect_held(&t, 0, 0);

	merge_teardown(&t);
}

/* A packet takes the buffers of following segments up to a limit */
ZTEST(net_socket_tcp, test_v4_merge_chain_limit)
{
	/* Fills its buffer, so that nothing can be copied */
	const size_t len = MERGE_BUF_SIZE - MERGE_V4_HDR_LEN;
	struct merge_test t;

	merge_setup(&t, NET_AF_INET, true);

	merge_send_segments(&t, MERGE_BUF_COUNT, len);
	merge_expect_held(&t, 1, MERGE_BUF_COUNT);

	merge_send(&t, len);
	merge_expect_held(&t, 2, MERGE_BUF_COUNT + 1);

	merge_send_segments(&t, MERGE_BUF_COUNT - 1, len);
	merge_expect_held(&t, 2, 2 * MERGE_BUF_COUNT);

	/* Read across the buffer and packet boundaries */
	merge_recv(&t, t.tx_off - 1, 37);
	merge_recv_all(&t);

	merge_teardown(&t);
}

/* Segments of more buffers than a packet collects are queued as they are, and
 * take small ones after them
 */
ZTEST(net_socket_tcp, test_v4_merge_large_segment)
{
	const size_t large = MERGE_BUF_COUNT * MERGE_BUF_SIZE + MERGE_BUF_SIZE / 2 -
			     MERGE_V4_HDR_LEN;
	struct merge_test t;

	merge_setup(&t, NET_AF_INET, true);

	merge_send(&t, MERGE_SEG_LEN);
	merge_expect_held(&t, 1, 1);

	merge_send(&t, large);
	merge_expect_held(&t, 2, MERGE_BUF_COUNT + 2);

	merge_send(&t, MERGE_SEG_LEN);
	merge_expect_held(&t, 2, MERGE_BUF_COUNT + 2);

	merge_send(&t, large);
	merge_send(&t, MERGE_SEG_LEN);
	merge_send(&t, 3 * MERGE_BUF_SIZE);
	merge_send(&t, 1);
	merge_send(&t, MERGE_SEG_LEN);

	merge_recv(&t, t.tx_off - 1, 100);
	merge_recv_all(&t);

	merge_teardown(&t);
}

/* Segments of two buffers are copied or have their buffers taken as well */
ZTEST(net_socket_tcp, test_v4_merge_two_buffers)
{
	/* Leaves room for one more such segment in its second buffer */
	const size_t len = MERGE_BUF_SIZE - MERGE_V4_HDR_LEN + MERGE_BUF_SIZE / 10;
	struct merge_test t;

	BUILD_ASSERT(MERGE_BUF_COUNT == 4, "the buffer counts below assume this");

	merge_setup(&t, NET_AF_INET, true);

	merge_send(&t, len);
	merge_expect_held(&t, 1, 2);

	/* Fits behind the first one */
	merge_send(&t, len);
	merge_expect_held(&t, 1, 2);

	/* Does not fit, both buffers are taken */
	merge_send(&t, len);
	merge_expect_held(&t, 1, 4);

	merge_send(&t, len);
	merge_expect_held(&t, 1, 4);

	/* The packet has its buffers, the next segment starts a new one */
	merge_send(&t, len);
	merge_expect_held(&t, 2, 6);

	merge_recv(&t, t.tx_off - 1, 33);
	merge_recv_all(&t);
	merge_expect_held(&t, 0, 0);

	merge_teardown(&t);
}

/* Buffers of a packet are given back as the application reads through them */
ZTEST(net_socket_tcp, test_v4_merge_read_releases_buffers)
{
	const size_t bufs = MERGE_BUF_COUNT + 2;
	const size_t len = bufs * MERGE_BUF_SIZE - MERGE_V4_HDR_LEN;
	struct merge_test t;

	merge_setup(&t, NET_AF_INET, true);

	merge_send(&t, len);
	merge_expect_held(&t, 1, bufs);

	/* Still in the first buffer */
	merge_recv(&t, MERGE_BUF_SIZE - MERGE_V4_HDR_LEN - 1, sizeof(merge_rx));
	merge_expect_held(&t, 1, bufs);

	/* Peeking further does not release anything */
	zassert_equal(zsock_recv(t.a_sock, merge_rx + t.rx_off, 2 * MERGE_BUF_SIZE,
				 ZSOCK_MSG_PEEK | ZSOCK_MSG_DONTWAIT),
		      2 * MERGE_BUF_SIZE, "peek failed (%d)", errno);
	merge_expect_held(&t, 1, bufs);

	/* The last byte of the first buffer */
	merge_recv(&t, 1, sizeof(merge_rx));
	merge_expect_held(&t, 1, bufs - 1);

	merge_recv(&t, 2 * MERGE_BUF_SIZE + 1, 50);
	merge_expect_held(&t, 1, bufs - 3);

	merge_recv_all(&t);
	merge_expect_held(&t, 0, 0);

	merge_teardown(&t);
}

/* Data is merged into a packet the application has read a part of */
ZTEST(net_socket_tcp, test_v4_merge_partial_read)
{
	struct merge_test t;

	merge_setup(&t, NET_AF_INET, true);

	merge_send(&t, 2 * MERGE_SEG_LEN);
	merge_recv(&t, MERGE_SEG_LEN / 2, sizeof(merge_rx));

	/* Copied behind the data that is left */
	merge_send(&t, 2 * MERGE_SEG_LEN);
	merge_expect_held(&t, 1, 1);
	merge_recv(&t, MERGE_SEG_LEN, sizeof(merge_rx));

	/* Does not fit, its buffer is taken */
	merge_send(&t, MERGE_BUF_SIZE - MERGE_V4_HDR_LEN);
	merge_expect_held(&t, 1, 2);
	merge_recv(&t, 2 * MERGE_SEG_LEN, 1);

	/* Read up to the last byte of the first buffer, then merge again */
	merge_recv(&t, MERGE_SEG_LEN / 2 - 1, sizeof(merge_rx));
	merge_send(&t, MERGE_SEG_LEN);
	merge_recv(&t, 1 + MERGE_SEG_LEN, sizeof(merge_rx));
	merge_send(&t, MERGE_SEG_LEN);

	merge_recv_all(&t);
	merge_expect_held(&t, 0, 0);

	merge_teardown(&t);
}

/* Peeking sees merged data, and leaves it in place */
ZTEST(net_socket_tcp, test_v4_merge_peek)
{
	const size_t len = MERGE_BUF_SIZE - MERGE_V4_HDR_LEN;
	static uint8_t peek_buf[MERGE_DATA_LEN];
	struct merge_test t;
	ssize_t rv;

	merge_setup(&t, NET_AF_INET, true);

	/* Some copied, some with a buffer of their own */
	merge_send_segments(&t, 4, MERGE_SEG_LEN);
	merge_send_segments(&t, 2, len);
	merge_expect_held(&t, 1, 3);

	for (int i = 0; i < 2; i++) {
		rv = zsock_recv(t.a_sock, peek_buf, sizeof(peek_buf),
				ZSOCK_MSG_PEEK | ZSOCK_MSG_DONTWAIT);
		zassert_equal(rv, t.tx_off, "peek failed (%d)", errno);
		zassert_mem_equal(peek_buf, merge_tx, t.tx_off, "wrong data");
	}

	merge_recv(&t, 3 * MERGE_SEG_LEN, sizeof(merge_rx));

	rv = zsock_recv(t.a_sock, peek_buf, sizeof(peek_buf), ZSOCK_MSG_PEEK | ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, t.tx_off - t.rx_off, "peek failed (%d)", errno);
	zassert_mem_equal(peek_buf, &merge_tx[t.rx_off], rv, "wrong data");

	merge_recv_all(&t);

	merge_teardown(&t);
}

static void merge_tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct merge_test *t = CONTAINER_OF(dwork, struct merge_test, tx_work);

	(void)zsock_send(t->c_sock, &merge_tx[t->tx_off], t->tx_work_len, 0);
	t->tx_off += t->tx_work_len;
}

/* A reader waiting for more data is woken up by data that gets merged */
ZTEST(net_socket_tcp, test_v4_merge_wakeup)
{
	struct zsock_timeval timeo = { .tv_sec = 2 };
	struct k_work_sync sync;
	struct merge_test t;
	ssize_t rv;

	merge_setup(&t, NET_AF_INET, true);

	rv = zsock_setsockopt(t.a_sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeo,
			      sizeof(timeo));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	merge_send(&t, MERGE_SEG_LEN);

	t.tx_work_len = MERGE_SEG_LEN;
	k_work_init_delayable(&t.tx_work, merge_tx_work_handler);
	k_work_reschedule(&t.tx_work, K_MSEC(200));

	rv = zsock_recv(t.a_sock, merge_rx, 2 * MERGE_SEG_LEN, ZSOCK_MSG_WAITALL);
	zassert_equal(rv, 2 * MERGE_SEG_LEN, "recv failed (%d)", errno);
	zassert_mem_equal(merge_rx, merge_tx, rv, "wrong data");
	t.rx_off += rv;

	/* The work uses the socket and the test data until its send returns */
	k_work_flush_delayable(&t.tx_work, &sync);

	merge_teardown(&t);
}

/* The end of the stream is reported after the merged data */
ZTEST(net_socket_tcp, test_v4_merge_eof)
{
	struct merge_test t;
	char c;

	merge_setup(&t, NET_AF_INET, true);

	merge_send_segments(&t, 5, MERGE_SEG_LEN);
	merge_expect_held(&t, 1, 1);

	merge_close(t.c_sock);
	t.c_sock = -1;
	k_msleep(MERGE_SETTLE_MS);

	merge_recv(&t, t.tx_off, sizeof(merge_rx));
	zassert_equal(zsock_recv(t.a_sock, &c, sizeof(c), 0), 0, "no EOF");

	merge_teardown(&t);
}

/* The receive window follows the merged data as the application reads it */
ZTEST(net_socket_tcp, test_v4_merge_recv_window)
{
	const int count = 10;
	int rcvbuf = count * MERGE_SEG_LEN;
	struct merge_test t;
	char c = 0;
	ssize_t rv;

	merge_setup(&t, NET_AF_INET, true);

	rv = zsock_setsockopt(t.a_sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVBUF, &rcvbuf,
			      sizeof(rcvbuf));
	zassert_equal(rv, 0, "setsockopt failed (%d)", errno);

	merge_send_segments(&t, count, MERGE_SEG_LEN);

	/* Wait for the ACK that closes the window */
	k_msleep(150);
	merge_expect_held(&t, 1, 2);

	rv = zsock_send(t.c_sock, &c, sizeof(c), ZSOCK_MSG_DONTWAIT);
	zassert_equal(rv, -1, "Unexpected return code %zd", rv);
	zassert_equal(errno, EAGAIN, "Unexpected errno value: %d", errno);

	merge_recv_all(&t);

	/* And for the one that opens it again */
	k_msleep(150);

	merge_send_segments(&t, count, MERGE_SEG_LEN);
	k_msleep(MERGE_SETTLE_MS);
	merge_recv_all(&t);

	merge_teardown(&t);
}

/* Merged data that is never read is released with the socket */
static void test_merge_close_unread_common(bool accept)
{
	struct merge_test t;

	merge_setup(&t, NET_AF_INET, accept);

	merge_send_segments(&t, 2 * CONFIG_NET_PKT_RX_COUNT / 3, MERGE_SEG_LEN);
	merge_send(&t, 3 * MERGE_BUF_SIZE);
	merge_send_segments(&t, 3, MERGE_BUF_SIZE - MERGE_V4_HDR_LEN);
	k_msleep(MERGE_SETTLE_MS);

	merge_teardown(&t);
}

ZTEST(net_socket_tcp, test_v4_merge_close_unread)
{
	test_merge_close_unread_common(true);
}

ZTEST(net_socket_tcp, test_v4_merge_close_unaccepted)
{
	test_merge_close_unread_common(false);
}
