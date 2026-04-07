/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ptp_time.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "transport.h"

struct fake_recv_action {
	int ret;
	int err;
	bool with_timestamp;
};

static int64_t fake_now_ms;
static int recvmsg_call_count;
static int recvfrom_call_count;
static struct fake_recv_action recvmsg_script[4];
static size_t recvmsg_script_len;
static size_t recvmsg_script_pos;
static struct fake_recv_action recvfrom_script[4];
static size_t recvfrom_script_len;
static size_t recvfrom_script_pos;
static const struct net_ptp_time fake_rx_timestamp = {
	.second = 42,
	.nanosecond = 123456789,
};

static int64_t fake_k_uptime_get(void)
{
	return fake_now_ms;
}

static const struct device *fake_net_eth_get_ptp_clock(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NULL;
}

static int fake_ptp_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tm);

	return -ENODEV;
}

static struct fake_recv_action next_action(struct fake_recv_action *script, size_t script_len,
					   size_t *script_pos, const char *name)
{
	zassert_true(*script_pos < script_len, "No scripted action left for %s", name);

	return script[(*script_pos)++];
}

static void fill_payload(void *buf, size_t len, int ret, uint8_t seed)
{
	if (ret > 0) {
		memset(buf, seed, MIN(len, (size_t)ret));
	}
}

static ssize_t fake_zsock_recvmsg(int sock, struct net_msghdr *msg, int flags)
{
	struct fake_recv_action action =
		next_action(recvmsg_script, recvmsg_script_len, &recvmsg_script_pos, "recvmsg");
	struct net_cmsghdr *cmsg;

	ARG_UNUSED(sock);
	ARG_UNUSED(flags);

	recvmsg_call_count++;

	if (action.ret < 0) {
		errno = action.err;
		return -1;
	}

	fill_payload(msg->msg_iov[0].iov_base, msg->msg_iov[0].iov_len, action.ret, 0xA5);

	if (!action.with_timestamp) {
		return action.ret;
	}

	cmsg = NET_CMSG_FIRSTHDR(msg);
	zassert_not_null(cmsg, "Missing control buffer for timestamp");

	cmsg->cmsg_level = ZSOCK_SOL_SOCKET;
	cmsg->cmsg_type = ZSOCK_SO_TIMESTAMPING;
	cmsg->cmsg_len = NET_CMSG_LEN(sizeof(fake_rx_timestamp));
	memcpy(NET_CMSG_DATA(cmsg), &fake_rx_timestamp, sizeof(fake_rx_timestamp));

	return action.ret;
}

static ssize_t fake_zsock_recvfrom(int sock, void *buf, size_t max_len, int flags,
				   struct net_sockaddr *src_addr, net_socklen_t *addrlen)
{
	struct fake_recv_action action =
		next_action(recvfrom_script, recvfrom_script_len, &recvfrom_script_pos, "recvfrom");

	ARG_UNUSED(sock);
	ARG_UNUSED(flags);
	ARG_UNUSED(src_addr);
	ARG_UNUSED(addrlen);

	recvfrom_call_count++;

	if (action.ret < 0) {
		errno = action.err;
		return -1;
	}

	fill_payload(buf, max_len, action.ret, 0x5A);

	return action.ret;
}

#define k_uptime_get fake_k_uptime_get
#define net_eth_get_ptp_clock fake_net_eth_get_ptp_clock
#define ptp_clock_get fake_ptp_clock_get
#define zsock_recvmsg fake_zsock_recvmsg
#define zsock_recvfrom fake_zsock_recvfrom
#include "../../../../../subsys/net/lib/ptp/transport.c"
#undef zsock_recvfrom
#undef zsock_recvmsg
#undef ptp_clock_get
#undef net_eth_get_ptp_clock
#undef k_uptime_get

static void reset_fakes(void)
{
	fake_now_ms = 0;
	recvmsg_call_count = 0;
	recvfrom_call_count = 0;
	recvmsg_script_len = 0;
	recvmsg_script_pos = 0;
	recvfrom_script_len = 0;
	recvfrom_script_pos = 0;
	memset(recvmsg_script, 0, sizeof(recvmsg_script));
	memset(recvfrom_script, 0, sizeof(recvfrom_script));
}

static void transport_retry_before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_fakes();
}

ZTEST(ptp_transport_retry, test_recvmsg_failure_temporarily_falls_back_then_retries)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	port.socket[PTP_SOCKET_EVENT] = 7;
	port.l2_try_recvmsg = true;

	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = -1,
		.err = EIO,
	};
	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 24,
		.with_timestamp = true,
	};

	recvfrom_script[recvfrom_script_len++] = (struct fake_recv_action){
		.ret = 16,
	};
	recvfrom_script[recvfrom_script_len++] = (struct fake_recv_action){
		.ret = 20,
	};

	fake_now_ms = 100;
	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 16, "unexpected recv return after recvmsg failure");
	zassert_false(port.l2_try_recvmsg, "recvmsg should be disabled until retry");
	zassert_equal(port.l2_recvmsg_retry_at, 100 + PTP_L2_RECVMSG_RETRY_MS,
		      "retry deadline not scheduled");
	zassert_equal(recvmsg_call_count, 1, "recvmsg should have been tried once");
	zassert_equal(recvfrom_call_count, 1, "recvfrom fallback should have been used");
	zassert_false(msg.rx_timestamp_valid, "fallback path should not mark RX timestamp valid");

	memset(&msg, 0, sizeof(msg));
	fake_now_ms = 100 + PTP_L2_RECVMSG_RETRY_MS - 1;
	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 20, "unexpected recv return before retry window");
	zassert_false(port.l2_try_recvmsg, "recvmsg should remain disabled before retry time");
	zassert_equal(recvmsg_call_count, 1, "recvmsg should not be retried early");
	zassert_equal(recvfrom_call_count, 2, "recvfrom should handle packets during backoff");

	memset(&msg, 0, sizeof(msg));
	fake_now_ms = 100 + PTP_L2_RECVMSG_RETRY_MS;
	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 24, "unexpected recv return after retry");
	zassert_true(port.l2_try_recvmsg, "recvmsg should be re-enabled after retry");
	zassert_equal(port.l2_recvmsg_retry_at, 0, "retry deadline should be cleared on success");
	zassert_equal(recvmsg_call_count, 2, "recvmsg should be retried after timeout");
	zassert_equal(recvfrom_call_count, 2, "recvfrom should not run after recvmsg recovery");
	zassert_true(msg.rx_timestamp_valid, "recvmsg timestamp should be accepted after retry");
	zassert_equal(msg.timestamp.host.second, fake_rx_timestamp.second,
		      "timestamp seconds mismatch");
	zassert_equal(msg.timestamp.host.nanosecond, fake_rx_timestamp.nanosecond,
		      "timestamp nanoseconds mismatch");
}

ZTEST_SUITE(ptp_transport_retry, NULL, NULL, transport_retry_before, NULL, NULL);
