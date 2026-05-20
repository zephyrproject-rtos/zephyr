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
	size_t timestamp_len;
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
static bool fake_phc_available;
static int fake_ptp_clock_get_ret;
static struct net_ptp_time fake_phc_timestamp;
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

	return fake_phc_available ? (const struct device *)0x1 : NULL;
}

static int fake_ptp_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	if (tm) {
		*tm = fake_phc_timestamp;
	}

	return fake_ptp_clock_get_ret;
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
	size_t timestamp_len;

	ARG_UNUSED(sock);
	ARG_UNUSED(flags);

	recvmsg_call_count++;

	if (action.ret < 0) {
		errno = action.err;
		return -1;
	}

	fill_payload(msg->msg_iov[0].iov_base, msg->msg_iov[0].iov_len, action.ret, 0xA5);

	if (!action.with_timestamp) {
		msg->msg_controllen = 0;
		return action.ret;
	}

	cmsg = NET_CMSG_FIRSTHDR(msg);
	zassert_not_null(cmsg, "Missing control buffer for timestamp");
	timestamp_len = action.timestamp_len ? action.timestamp_len : sizeof(fake_rx_timestamp);

	cmsg->cmsg_level = ZSOCK_SOL_SOCKET;
	cmsg->cmsg_type = ZSOCK_SO_TIMESTAMPING;
	cmsg->cmsg_len = NET_CMSG_LEN(timestamp_len);
	msg->msg_controllen = NET_CMSG_SPACE(timestamp_len);
	memcpy(NET_CMSG_DATA(cmsg), &fake_rx_timestamp, timestamp_len);

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

#define k_uptime_get          fake_k_uptime_get
#define net_eth_get_ptp_clock fake_net_eth_get_ptp_clock
#define ptp_clock_get         fake_ptp_clock_get
#define zsock_recvmsg         fake_zsock_recvmsg
#define zsock_recvfrom        fake_zsock_recvfrom
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
	fake_phc_available = false;
	fake_ptp_clock_get_ret = -ENODEV;
	memset(&fake_phc_timestamp, 0, sizeof(fake_phc_timestamp));
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

	if (!IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		ztest_test_skip();
	}

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

ZTEST(ptp_transport_retry, test_l2_recvmsg_eagain_does_not_disable_recvmsg)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 7;
	port.l2_try_recvmsg = true;

	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = -1,
		.err = EAGAIN,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 0, "EAGAIN should be reported as no packet");
	zassert_true(port.l2_try_recvmsg, "recvmsg should remain enabled after EAGAIN");
	zassert_equal(port.l2_recvmsg_retry_at, 0, "retry deadline should not be scheduled");
	zassert_equal(recvmsg_call_count, 1, "recvmsg should be called once");
	zassert_equal(recvfrom_call_count, 0, "recvfrom fallback should not run on EAGAIN");
}

ZTEST(ptp_transport_retry, test_l2_recvmsg_without_timestamp_uses_phc_fallback)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 7;
	port.l2_try_recvmsg = true;
	fake_phc_available = true;
	fake_ptp_clock_get_ret = 0;
	fake_phc_timestamp.second = 1000;
	fake_phc_timestamp.nanosecond = 2000;

	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 32,
		.with_timestamp = false,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 32, "unexpected recv return");
	zassert_true(msg.rx_timestamp_valid, "PHC fallback should mark timestamp valid");
	zassert_equal(msg.timestamp.host.second, fake_phc_timestamp.second,
		      "PHC fallback seconds mismatch");
	zassert_equal(msg.timestamp.host.nanosecond, fake_phc_timestamp.nanosecond,
		      "PHC fallback nanoseconds mismatch");
}

ZTEST(ptp_transport_retry, test_l2_recvmsg_without_timestamp_uses_uptime_fallback)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 7;
	port.l2_try_recvmsg = true;
	fake_now_ms = 12345;

	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 28,
		.with_timestamp = false,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 28, "unexpected recv return");
	zassert_false(msg.rx_timestamp_valid, "uptime fallback is not a valid RX timestamp");
	zassert_equal(msg.timestamp.host.second, 12, "uptime fallback seconds mismatch");
	zassert_equal(msg.timestamp.host.nanosecond, 345000000,
		      "uptime fallback nanoseconds mismatch");
}

ZTEST(ptp_transport_retry, test_l2_recvmsg_short_timestamp_returns_error)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 7;
	port.l2_try_recvmsg = true;
	fake_phc_available = true;
	fake_ptp_clock_get_ret = 0;
	fake_phc_timestamp.second = 1000;
	fake_phc_timestamp.nanosecond = 2000;

	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 28,
		.with_timestamp = true,
		.timestamp_len = sizeof(fake_rx_timestamp) - 1,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, -EINVAL, "short timestamp control message should fail");
	zassert_false(msg.rx_timestamp_valid, "malformed timestamp should not be valid");
	zassert_equal(msg.timestamp.host.second, 0,
		      "malformed timestamp should not use PHC fallback");
	zassert_equal(msg.timestamp.host.nanosecond, 0,
		      "malformed timestamp should not use PHC fallback");
}

ZTEST(ptp_transport_retry, test_udp_recvmsg_extracts_timestamp)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_UDP_IPV4_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_GENERAL] = 9;
	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 44,
		.with_timestamp = true,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_GENERAL);
	zassert_equal(ret, 44, "unexpected UDP recv return");
	zassert_equal(recvmsg_call_count, 1, "UDP should use recvmsg");
	zassert_equal(recvfrom_call_count, 0, "UDP should not use recvfrom");
	zassert_true(msg.rx_timestamp_valid, "UDP timestamp should be valid");
	zassert_equal(msg.timestamp.host.second, fake_rx_timestamp.second,
		      "UDP timestamp seconds mismatch");
	zassert_equal(msg.timestamp.host.nanosecond, fake_rx_timestamp.nanosecond,
		      "UDP timestamp nanoseconds mismatch");
}

ZTEST(ptp_transport_retry, test_udp_recvmsg_without_timestamp_is_not_valid)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_UDP_IPV4_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 8;
	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 40,
		.with_timestamp = false,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 40, "unexpected UDP recv return");
	zassert_false(msg.rx_timestamp_valid, "missing UDP timestamp should not be valid");
	zassert_equal(msg.timestamp.host.second, 0, "unexpected UDP fallback timestamp");
	zassert_equal(msg.timestamp.host.nanosecond, 0, "unexpected UDP fallback timestamp");
}

ZTEST(ptp_transport_retry, test_udp_recvmsg_short_timestamp_returns_error)
{
	struct ptp_port port = {0};
	struct ptp_msg msg = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_PTP_UDP_IPV4_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_GENERAL] = 9;
	recvmsg_script[recvmsg_script_len++] = (struct fake_recv_action){
		.ret = 44,
		.with_timestamp = true,
		.timestamp_len = sizeof(fake_rx_timestamp) - 1,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_GENERAL);
	zassert_equal(ret, -EINVAL, "short UDP timestamp control message should fail");
	zassert_false(msg.rx_timestamp_valid, "malformed UDP timestamp should not be valid");
}

ZTEST_SUITE(ptp_transport_retry, NULL, NULL, transport_retry_before, NULL, NULL);
