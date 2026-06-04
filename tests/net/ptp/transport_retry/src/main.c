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

#include "msg.h"
#include "transport.h"

struct fake_recv_action {
	int ret;
	int err;
	size_t timestamp_len;
	const struct net_ptp_time *timestamp;
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
static int sendto_call_count;
static int last_send_sock;
static int fake_sendto_ret;
static net_socklen_t last_send_addrlen;
static struct net_sockaddr_storage last_send_addr;
static const struct net_ptp_time fake_rx_timestamp = {
	.second = 42,
	.nanosecond = 123456789,
};
static const struct net_ptp_time fake_invalid_rx_timestamp = {
	.second = UINT64_MAX,
	.nanosecond = UINT32_MAX,
};

static int64_t fake_k_uptime_get(void)
{
	return fake_now_ms;
}

static int fake_net_if_get_by_iface(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return 4;
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
	memcpy(NET_CMSG_DATA(cmsg),
	       action.timestamp != NULL ? action.timestamp : &fake_rx_timestamp, timestamp_len);

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

static ssize_t fake_zsock_sendto(int sock, const void *buf, size_t len, int flags,
				 const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(flags);

	sendto_call_count++;
	last_send_sock = sock;
	last_send_addrlen = addrlen;
	memset(&last_send_addr, 0, sizeof(last_send_addr));
	memcpy(&last_send_addr, dest_addr, MIN(addrlen, sizeof(last_send_addr)));

	return fake_sendto_ret != 0 ? fake_sendto_ret : (ssize_t)len;
}

#define k_uptime_get        fake_k_uptime_get
#define net_if_get_by_iface fake_net_if_get_by_iface
#define zsock_sendto        fake_zsock_sendto
#define zsock_recvmsg       fake_zsock_recvmsg
#define zsock_recvfrom      fake_zsock_recvfrom
#include "../../../../../subsys/net/lib/ptp/transport.c"
#undef zsock_recvfrom
#undef zsock_recvmsg
#undef zsock_sendto
#undef net_if_get_by_iface
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
	sendto_call_count = 0;
	last_send_sock = -1;
	fake_sendto_ret = 0;
	last_send_addrlen = 0;
	memset(&last_send_addr, 0, sizeof(last_send_addr));
	memset(recvmsg_script, 0, sizeof(recvmsg_script));
	memset(recvfrom_script, 0, sizeof(recvfrom_script));
}

static void init_send_msg(struct ptp_msg *msg, enum ptp_msg_type type)
{
	static const uint16_t msg_size[] = {
		[PTP_MSG_SYNC] = sizeof(struct ptp_sync_msg),
		[PTP_MSG_PDELAY_REQ] = sizeof(struct ptp_pdelay_req_msg),
		[PTP_MSG_PDELAY_RESP] = sizeof(struct ptp_pdelay_resp_msg),
		[PTP_MSG_PDELAY_RESP_FOLLOW_UP] = sizeof(struct ptp_pdelay_resp_follow_up_msg),
	};

	memset(msg, 0, sizeof(*msg));
	msg->header.type_major_sdo_id = type;
	msg->header.msg_length = net_htons(msg_size[type]);
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
		.ret = 32,
		.with_timestamp = false,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 32, "unexpected recv return");
	zassert_false(msg.rx_timestamp_valid, "missing timestamp should not be valid");
	zassert_equal(msg.timestamp.host.second, 12, "uptime fallback seconds mismatch");
	zassert_equal(msg.timestamp.host.nanosecond, 345000000,
		      "uptime fallback nanoseconds mismatch");
}

ZTEST(ptp_transport_retry, test_l2_recvmsg_invalid_timestamp_uses_uptime_fallback)
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
		.with_timestamp = true,
		.timestamp = &fake_invalid_rx_timestamp,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 28, "unexpected recv return");
	zassert_false(msg.rx_timestamp_valid, "invalid timestamp should not be valid");
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

ZTEST(ptp_transport_retry, test_udp_recvmsg_invalid_timestamp_is_not_valid)
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
		.with_timestamp = true,
		.timestamp = &fake_invalid_rx_timestamp,
	};

	ret = ptp_transport_recv(&port, &msg, PTP_SOCKET_EVENT);
	zassert_equal(ret, 40, "unexpected UDP recv return");
	zassert_false(msg.rx_timestamp_valid, "invalid UDP timestamp should not be valid");
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

ZTEST(ptp_transport_retry, test_l2_pdelay_uses_peer_delay_multicast)
{
	const uint8_t default_addr[NET_ETH_ADDR_LEN] = {0x01, 0x1B, 0x19, 0x00, 0x00, 0x00};
	const uint8_t pdelay_addr[NET_ETH_ADDR_LEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};
	struct net_sockaddr_ll *addr = (struct net_sockaddr_ll *)&last_send_addr;
	struct ptp_port port = {0};
	struct ptp_msg msg;

	if (!IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 7;

	init_send_msg(&msg, PTP_MSG_PDELAY_REQ);
	zassert_true(ptp_transport_send(&port, &msg, PTP_SOCKET_EVENT) > 0,
		     "Pdelay L2 send failed");
	zassert_equal(sendto_call_count, 1, "Pdelay L2 sendto not called");
	zassert_mem_equal(addr->sll_addr, pdelay_addr, sizeof(pdelay_addr),
			  "Pdelay L2 multicast address mismatch");

	init_send_msg(&msg, PTP_MSG_SYNC);
	zassert_true(ptp_transport_send(&port, &msg, PTP_SOCKET_EVENT) > 0, "Sync L2 send failed");
	zassert_equal(sendto_call_count, 2, "Sync L2 sendto not called");
	zassert_mem_equal(addr->sll_addr, default_addr, sizeof(default_addr),
			  "default L2 multicast address mismatch");
}

ZTEST(ptp_transport_retry, test_udp4_pdelay_uses_peer_delay_multicast)
{
	struct net_sockaddr_in *addr = net_sin((struct net_sockaddr *)&last_send_addr);
	struct ptp_port port = {0};
	struct ptp_msg msg;

	if (!IS_ENABLED(CONFIG_PTP_UDP_IPV4_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_EVENT] = 7;

	init_send_msg(&msg, PTP_MSG_PDELAY_RESP);
	zassert_true(ptp_transport_send(&port, &msg, PTP_SOCKET_EVENT) > 0,
		     "Pdelay IPv4 send failed");
	zassert_equal(sendto_call_count, 1, "Pdelay IPv4 sendto not called");
	zassert_mem_equal(&addr->sin_addr, &pdelay_mcast_addr_ipv4, sizeof(pdelay_mcast_addr_ipv4),
			  "Pdelay IPv4 multicast address mismatch");

	init_send_msg(&msg, PTP_MSG_SYNC);
	zassert_true(ptp_transport_send(&port, &msg, PTP_SOCKET_EVENT) > 0,
		     "Sync IPv4 send failed");
	zassert_equal(sendto_call_count, 2, "Sync IPv4 sendto not called");
	zassert_mem_equal(&addr->sin_addr, &mcast_addr_ipv4, sizeof(mcast_addr_ipv4),
			  "default IPv4 multicast address mismatch");
}

ZTEST(ptp_transport_retry, test_udp6_pdelay_uses_peer_delay_multicast)
{
	struct net_sockaddr_in6 *addr = net_sin6((struct net_sockaddr *)&last_send_addr);
	struct ptp_port port = {0};
	struct ptp_msg msg;

	if (!IS_ENABLED(CONFIG_PTP_UDP_IPV6_PROTOCOL)) {
		ztest_test_skip();
	}

	port.socket[PTP_SOCKET_GENERAL] = 9;

	init_send_msg(&msg, PTP_MSG_PDELAY_RESP_FOLLOW_UP);
	zassert_true(ptp_transport_send(&port, &msg, PTP_SOCKET_GENERAL) > 0,
		     "Pdelay IPv6 send failed");
	zassert_equal(sendto_call_count, 1, "Pdelay IPv6 sendto not called");
	zassert_mem_equal(&addr->sin6_addr, &pdelay_mcast_addr_ipv6, sizeof(pdelay_mcast_addr_ipv6),
			  "Pdelay IPv6 multicast address mismatch");

	init_send_msg(&msg, PTP_MSG_SYNC);
	zassert_true(ptp_transport_send(&port, &msg, PTP_SOCKET_GENERAL) > 0,
		     "Sync IPv6 send failed");
	zassert_equal(sendto_call_count, 2, "Sync IPv6 sendto not called");
	zassert_mem_equal(&addr->sin6_addr, &mcast_addr_ipv6, sizeof(mcast_addr_ipv6),
			  "default IPv6 multicast address mismatch");
}

ZTEST_SUITE(ptp_transport_retry, NULL, NULL, transport_retry_before, NULL, NULL);
