/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* iperf3 client.
 *
 * An upload is a whole iperf3 test: the control connection and its
 * handshake, one data stream sending for the requested duration, and the
 * exchange of results, from which the server side of the report is filled.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>
#include <zephyr/sys/byteorder.h>

#include "zperf_internal.h"
#include "zperf_iperf3.h"
#include "zperf_udp_pacing.h"

/* How long to wait for the server on the control connection */
#define CTRL_TIMEOUT_MS 10000

/* How long to wait for the answer to a UDP stream set-up datagram, and how
 * often to send it
 */
#define UDP_CONNECT_TIMEOUT_MS 1000
#define UDP_CONNECT_TRIES      3

#define JSON_BUF_SIZE (ZPERF_IPERF3_JSON_LEN_SIZE + CONFIG_NET_ZPERF_IPERF3_JSON_MAX_LEN + 1)

/* What the data stream carried */
struct send_stats {
	uint64_t bytes;
	uint32_t packets;
	uint32_t errors;
	uint32_t packet_size;
	uint64_t duration_us;
};

/* Where periodic results go, for a TCP upload with a report interval */
struct periodic_report {
	zperf_callback callback;
	void *user_data;
	uint32_t interval_ms;
};

static uint8_t sample_packet[PACKET_SIZE_MAX];
static uint8_t json_buf[JSON_BUF_SIZE];

/* One upload at a time: the buffers above are shared */
static K_MUTEX_DEFINE(upload_lock);

static struct zperf_async_upload_context udp_async_ctx;
static struct zperf_async_upload_context tcp_async_ctx;

static int set_rcvtimeo(int sock, uint32_t timeout_ms)
{
	struct zsock_timeval tv = {
		.tv_sec = timeout_ms / MSEC_PER_SEC,
		.tv_usec = (timeout_ms % MSEC_PER_SEC) * USEC_PER_MSEC,
	};

	if (zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		return -errno;
	}

	return 0;
}

static int map_recv_error(int ret)
{
	/* A receive timeout surfaces as EAGAIN */
	return (ret == -EAGAIN || ret == -EWOULDBLOCK) ? -ETIMEDOUT : ret;
}

/* Wait for the server to move the test to the expected state */
static int ctrl_expect_state(int ctrl, int8_t expected)
{
	uint8_t reply[ZPERF_IPERF3_SERVER_ERROR_SIZE - 1];
	int8_t state;
	int32_t error;
	int ret;

	ret = zperf_iperf3_recv_all(ctrl, &state, sizeof(state));
	if (ret < 0) {
		NET_ERR("No reply from the iperf3 server (%d)", ret);
		return map_recv_error(ret);
	}

	if (state == expected) {
		return 0;
	}

	switch (state) {
	case ZPERF_IPERF3_ACCESS_DENIED:
		NET_ERR("The iperf3 server is busy running a test");
		return -EBUSY;

	case ZPERF_IPERF3_SERVER_ERROR:
		ret = zperf_iperf3_recv_all(ctrl, reply, sizeof(reply));
		if (ret < 0) {
			return map_recv_error(ret);
		}

		error = (int32_t)sys_get_be32(reply);
		NET_ERR("The iperf3 server refused the test: %s (%d)",
			zperf_iperf3_strerror(error), error);
		return (error == ZPERF_IPERF3_IEUNIMP) ? -ENOTSUP : -EPROTO;

	case ZPERF_IPERF3_SERVER_TERMINATE:
		NET_ERR("The iperf3 server ended the test");
		return -ECONNABORTED;

	default:
		NET_ERR("Unexpected iperf3 state %d, expected %d", state, expected);
		return -EPROTO;
	}
}

/* The size of the blocks sent, which is also what the server is told */
static uint32_t block_size(const struct zperf_upload_params *param, bool udp)
{
	uint32_t min_size = udp ? ZPERF_IPERF3_UDP_HDR_SIZE : 1U;
	uint32_t size = CLAMP(param->packet_size, min_size, PACKET_SIZE_MAX);

	if (size != param->packet_size) {
		NET_WARN("Packet size %u out of range, using %u", param->packet_size, size);
	}

	return size;
}

static int send_params(int ctrl, const struct zperf_upload_params *param, bool udp,
		       uint32_t packet_size)
{
	static const char client_version[] = "zperf-" ZPERF_VERSION;
	struct zperf_iperf3_params params = {
		.tcp = !udp,
		.udp = udp,
		/* The client ends the test itself; this only tells the server
		 * roughly how long to expect it to take.
		 */
		.time = DIV_ROUND_UP(param->duration_ms, MSEC_PER_SEC),
		.parallel = 1,
		.len = packet_size,
		.bandwidth = udp ? (uint64_t)param->rate_kbps * 1000U : 0U,
		.pacing_timer = 1000,
		.nodelay = (param->options.tcp_nodelay != 0),
		.tos = param->options.tos,
		.client_version = {
			/* Only read when encoding */
			.start = (char *)client_version,
			.length = sizeof(client_version) - 1,
		},
	};
	int len;

	len = zperf_iperf3_params_encode(&params, json_buf, sizeof(json_buf));
	if (len < 0) {
		return len;
	}

	return zperf_iperf3_send_all(ctrl, json_buf, len, 0);
}

static bool udp_connect_reply_ok(const uint8_t *reply)
{
	/* Servers from before the text form answer with 987654321, written
	 * in their own byte order.
	 */
	static const uint8_t legacy_le[] = { 0xb1, 0x68, 0xde, 0x3a };
	static const uint8_t legacy_be[] = { 0x3a, 0xde, 0x68, 0xb1 };

	return memcmp(reply, zperf_iperf3_udp_connect_reply, ZPERF_IPERF3_UDP_CONNECT_SIZE) == 0 ||
	       memcmp(reply, legacy_le, sizeof(legacy_le)) == 0 ||
	       memcmp(reply, legacy_be, sizeof(legacy_be)) == 0;
}

static int open_stream(const struct zperf_upload_params *param, bool udp, const char *cookie)
{
	const struct net_sockaddr *peer = net_sad(&param->peer_addr_storage);
	uint8_t reply[ZPERF_IPERF3_UDP_CONNECT_SIZE];
	int sock;
	int ret;

	sock = zperf_prepare_upload_sock(peer, param->options.tos, param->options.priority,
					 udp ? 0 : param->options.tcp_nodelay,
					 udp ? NET_IPPROTO_UDP : NET_IPPROTO_TCP, param->if_name);
	if (sock < 0) {
		return sock;
	}

	if (!udp) {
		/* A TCP stream identifies its test with the cookie */
		ret = zperf_iperf3_send_all(sock, cookie, ZPERF_IPERF3_COOKIE_SIZE, 0);
		if (ret < 0) {
			goto error;
		}

		return sock;
	}

	ret = set_rcvtimeo(sock, UDP_CONNECT_TIMEOUT_MS);
	if (ret < 0) {
		goto error;
	}

	/* A UDP stream is set up with one datagram each way, repeated in
	 * case one is lost.
	 */
	for (int tries = 0; tries < UDP_CONNECT_TRIES; tries++) {
		if (zsock_send(sock, zperf_iperf3_udp_connect_msg, ZPERF_IPERF3_UDP_CONNECT_SIZE,
			       0) < 0) {
			ret = -errno;
			goto error;
		}

		ret = zsock_recv(sock, reply, sizeof(reply), 0);
		if (ret == sizeof(reply) && udp_connect_reply_ok(reply)) {
			return sock;
		}
	}

	NET_ERR("No answer to the iperf3 UDP stream set-up");
	ret = -ETIMEDOUT;

error:
	zsock_close(sock);
	return ret;
}

static int load_payload(const struct zperf_upload_params *param, uint64_t *offset,
			uint8_t *data, uint32_t len)
{
	int ret;

	if (param->data_loader == NULL || len == 0U) {
		return 0;
	}

	ret = param->data_loader(param->data_loader_ctx, *offset, data, len);
	if (ret < 0) {
		NET_ERR("Failed to load data for offset %llu", *offset);
		return ret;
	}

	*offset += len;

	return 0;
}

static int send_udp(int sock, const struct zperf_upload_params *param, uint32_t packet_size,
		    struct send_stats *stats)
{
	/* A rate of 0 means as fast as possible */
	uint32_t packet_duration_us = (param->rate_kbps != 0U) ?
		zperf_packet_duration(packet_size, param->rate_kbps) : 0U;
	struct zperf_udp_pacer pacer;
	uint64_t data_offset = 0U;
	int64_t start_time;
	int64_t end_time;
	int ret;

	start_time = k_uptime_ticks();
	end_time = start_time + k_ms_to_ticks_ceil64(param->duration_ms);
	zperf_udp_pacer_init(&pacer, packet_duration_us, start_time);

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		int64_t loop_time = k_uptime_ticks();
		int64_t remaining;
		int32_t adjust;
		int delay;

		delay = zperf_udp_pacer_next(&pacer, loop_time, &adjust);

		zperf_iperf3_udp_hdr_put(sample_packet,
					 param->unix_offset_us +
					 k_ticks_to_us_floor64(loop_time - start_time),
					 stats->packets + 1U, false);

		ret = load_payload(param, &data_offset, sample_packet + ZPERF_IPERF3_UDP_HDR_SIZE,
				   packet_size - ZPERF_IPERF3_UDP_HDR_SIZE);
		if (ret < 0) {
			return ret;
		}

		if (zsock_send(sock, sample_packet, packet_size, 0) < 0) {
			NET_ERR("Failed to send the packet (%d)", errno);
			return -errno;
		}

		stats->packets++;
		stats->bytes += packet_size;

		/* At a coarse tick rate the rate control sends in bursts and
		 * makes up for them with long waits. Do not let one run past
		 * the end of the test.
		 */
		remaining = end_time - k_uptime_ticks();
		delay = CLAMP(remaining, 0, delay);
		zperf_udp_pacer_wait(delay);
	} while (pacer.last_loop_time < end_time);

	stats->packet_size = packet_size;
	stats->duration_us = k_ticks_to_us_ceil64(k_uptime_ticks() - start_time);

	return 0;
}

/* Send a whole block, reporting how much of it went out if that fails */
static int send_block(int sock, const uint8_t *buf, size_t len, size_t *sent)
{
	*sent = 0;

	while (*sent < len) {
		ssize_t ret = zsock_send(sock, buf + *sent, len - *sent, 0);

		if (ret < 0) {
			return -errno;
		}

		*sent += (size_t)ret;
	}

	return 0;
}

static int send_tcp(int sock, const struct zperf_upload_params *param, uint32_t packet_size,
		    struct send_stats *stats, const struct periodic_report *periodic)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(param->duration_ms));
	struct zperf_results report = { 0 };
	uint64_t data_offset = 0U;
	uint32_t alloc_errors = 0U;
	int64_t report_start;
	int64_t start_time;
	int64_t now;
	size_t sent;
	int ret = 0;

	start_time = k_uptime_ticks();
	report_start = start_time;

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		ret = load_payload(param, &data_offset, sample_packet, packet_size);
		if (ret < 0) {
			return ret;
		}

		ret = send_block(sock, sample_packet, packet_size, &sent);

		/* The server counts every byte, those of a failed block too */
		stats->bytes += sent;
		report.total_len += sent;

		if (ret == -ENOMEM) {
			/* Running out of buffers is expected when their
			 * counts are not tuned for the test
			 */
			stats->errors++;
			report.nb_packets_errors++;
			alloc_errors++;
		} else if (ret < 0) {
			NET_ERR("Failed to send the packet (%d)", ret);
			stats->errors++;
			return ret;
		}

		if (ret == 0) {
			stats->packets++;
			report.nb_packets_sent++;
		}

		now = k_uptime_ticks();

		if (periodic != NULL &&
		    k_ticks_to_ms_floor64(now - report_start) >= periodic->interval_ms) {
			report.client_time_in_us = k_ticks_to_us_ceil64(now - report_start);
			report.packet_size = packet_size;
			periodic->callback(ZPERF_SESSION_PERIODIC_RESULT, &report,
					   periodic->user_data);

			memset(&report, 0, sizeof(report));
			report_start = now;
		}

#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(100 * USEC_PER_MSEC);
#else
		k_yield();
#endif
	} while (!sys_timepoint_expired(end));

	/* Report the last interval too, which is shorter than the others
	 * unless the duration is a multiple of the interval.
	 */
	if (periodic != NULL && (report.nb_packets_sent > 0U || report.nb_packets_errors > 0U)) {
		now = k_uptime_ticks();
		report.client_time_in_us = k_ticks_to_us_ceil64(now - report_start);
		report.packet_size = packet_size;
		periodic->callback(ZPERF_SESSION_PERIODIC_RESULT, &report, periodic->user_data);
	}

	stats->packet_size = packet_size;
	stats->duration_us = k_ticks_to_us_ceil64(k_uptime_ticks() - start_time);

	if (alloc_errors > 0U) {
		NET_WARN("There were %u network buffer allocation errors during send. "
			 "Consider increasing CONFIG_NET_BUF_TX_COUNT and CONFIG_NET_PKT_TX_COUNT.",
			 alloc_errors);
	}

	return 0;
}

static int exchange_results(int ctrl, const struct send_stats *sent,
			    struct zperf_iperf3_stream_stats *server)
{
	struct zperf_iperf3_stream_stats ours = {
		.bytes = sent->bytes,
		.packets = sent->packets,
		.duration_us = sent->duration_us,
	};
	uint32_t len;
	int ret;

	ret = zperf_iperf3_results_encode(&ours, true, json_buf, sizeof(json_buf));
	if (ret < 0) {
		return ret;
	}

	ret = zperf_iperf3_send_all(ctrl, json_buf, ret, 0);
	if (ret < 0) {
		return ret;
	}

	ret = zperf_iperf3_recv_all(ctrl, json_buf, ZPERF_IPERF3_JSON_LEN_SIZE);
	if (ret < 0) {
		return map_recv_error(ret);
	}

	len = sys_get_be32(json_buf);
	if (len == 0U || len > CONFIG_NET_ZPERF_IPERF3_JSON_MAX_LEN) {
		NET_ERR("iperf3 results of %u bytes do not fit", len);
		return -EMSGSIZE;
	}

	ret = zperf_iperf3_recv_all(ctrl, json_buf, len);
	if (ret < 0) {
		return map_recv_error(ret);
	}

	ret = zperf_iperf3_results_decode((char *)json_buf, len, server);
	if (ret < 0) {
		NET_ERR("Cannot decode the server's iperf3 results (%d)", ret);
	}

	return ret;
}

static void fill_results(bool udp, const struct send_stats *sent,
			 const struct zperf_iperf3_stream_stats *server,
			 struct zperf_results *result)
{
	memset(result, 0, sizeof(*result));

	result->nb_packets_sent = sent->packets;
	result->nb_packets_errors = sent->errors;
	result->client_time_in_us = sent->duration_us;
	result->packet_size = sent->packet_size;
	result->time_in_us = server->duration_us;

	if (udp) {
		/* The server reports the highest sequence number it saw and
		 * how many before it went missing, so datagrams lost after the
		 * last one it received do not show up in its count. Count
		 * them as lost here, as the iperf2 report does. iperf3 does
		 * not report reordering.
		 */
		result->nb_packets_rcvd = (uint32_t)(server->packets - MIN(server->errors,
									   server->packets));
		result->nb_packets_lost = sent->packets - MIN(result->nb_packets_rcvd,
							       sent->packets);
		result->jitter_in_us = (uint32_t)server->jitter_us;
		result->total_len = server->bytes;
	} else {
		result->total_len = sent->bytes;
	}
}

static int iperf3_upload(const struct zperf_upload_params *param, bool udp,
			 struct zperf_results *result, const struct periodic_report *periodic)
{
	struct zperf_iperf3_stream_stats server = { 0 };
	struct send_stats sent = { 0 };
	char cookie[ZPERF_IPERF3_COOKIE_SIZE];
	const struct net_sockaddr *peer;
	uint32_t packet_size;
	bool mcast;
	int stream = -1;
	int ctrl;
	int ret;

	if (udp && !IS_ENABLED(CONFIG_NET_UDP)) {
		return -ENOTSUP;
	}

	peer = net_sad(&param->peer_addr_storage);

	if (peer->sa_family == NET_AF_INET) {
		mcast = net_ipv4_is_addr_mcast(&net_sin(peer)->sin_addr);
	} else if (peer->sa_family == NET_AF_INET6) {
		mcast = net_ipv6_is_addr_mcast(&net_sin6(peer)->sin6_addr);
	} else {
		return -EINVAL;
	}

	if (mcast) {
		NET_ERR("iperf3 does not support multicast");
		return -ENOTSUP;
	}

	if (k_mutex_lock(&upload_lock, K_NO_WAIT) != 0) {
		NET_ERR("An iperf3 upload is already running");
		return -EBUSY;
	}

	/* The control connection carries small messages that should not
	 * wait for more to coalesce with. It goes out of the same interface
	 * as the data stream.
	 */
	ctrl = zperf_prepare_upload_sock(peer, param->options.tos, param->options.priority, 1,
					 NET_IPPROTO_TCP, param->if_name);
	if (ctrl < 0) {
		k_mutex_unlock(&upload_lock);
		return ctrl;
	}

	ret = set_rcvtimeo(ctrl, CTRL_TIMEOUT_MS);
	if (ret < 0) {
		goto out;
	}

	zperf_iperf3_make_cookie(cookie);

	ret = zperf_iperf3_send_all(ctrl, cookie, sizeof(cookie), 0);
	if (ret < 0) {
		goto out;
	}

	ret = ctrl_expect_state(ctrl, ZPERF_IPERF3_PARAM_EXCHANGE);
	if (ret < 0) {
		goto out;
	}

	packet_size = block_size(param, udp);

	ret = send_params(ctrl, param, udp, packet_size);
	if (ret < 0) {
		goto out;
	}

	ret = ctrl_expect_state(ctrl, ZPERF_IPERF3_CREATE_STREAMS);
	if (ret < 0) {
		goto out;
	}

	stream = open_stream(param, udp, cookie);
	if (stream < 0) {
		ret = stream;
		goto out;
	}

	ret = ctrl_expect_state(ctrl, ZPERF_IPERF3_TEST_START);
	if (ret < 0) {
		goto out;
	}

	ret = ctrl_expect_state(ctrl, ZPERF_IPERF3_TEST_RUNNING);
	if (ret < 0) {
		goto out;
	}

	/* The rate control is built only with UDP */
	if (IS_ENABLED(CONFIG_NET_UDP) && udp) {
		ret = send_udp(stream, param, packet_size, &sent);
	} else {
		ret = send_tcp(stream, param, packet_size, &sent, periodic);
	}
	if (ret < 0) {
		(void)zperf_iperf3_send_state(ctrl, ZPERF_IPERF3_CLIENT_TERMINATE, 0);
		goto out;
	}

	ret = zperf_iperf3_send_state(ctrl, ZPERF_IPERF3_TEST_END, 0);
	if (ret < 0) {
		goto out;
	}

	ret = ctrl_expect_state(ctrl, ZPERF_IPERF3_EXCHANGE_RESULTS);
	if (ret < 0) {
		goto out;
	}

	ret = exchange_results(ctrl, &sent, &server);
	if (ret < 0) {
		goto out;
	}

	ret = ctrl_expect_state(ctrl, ZPERF_IPERF3_DISPLAY_RESULTS);
	if (ret < 0) {
		goto out;
	}

	(void)zperf_iperf3_send_state(ctrl, ZPERF_IPERF3_IPERF_DONE, 0);

	fill_results(udp, &sent, &server, result);

out:
	if (stream >= 0) {
		zsock_close(stream);
	}

	zsock_close(ctrl);
	k_mutex_unlock(&upload_lock);

	return ret;
}

int zperf_iperf3_udp_upload(const struct zperf_upload_params *param, struct zperf_results *result)
{
	return iperf3_upload(param, true, result, NULL);
}

int zperf_iperf3_tcp_upload(const struct zperf_upload_params *param, struct zperf_results *result)
{
	return iperf3_upload(param, false, result, NULL);
}

static void upload_async_work(struct zperf_async_upload_context *ctx, bool udp)
{
	struct zperf_upload_params param = ctx->param;
	struct zperf_results result;
	struct periodic_report periodic = {
		.callback = ctx->callback,
		.user_data = ctx->user_data,
		.interval_ms = param.options.report_interval_ms,
	};
	int ret;

	ctx->callback(ZPERF_SESSION_STARTED, NULL, ctx->user_data);

	/* As for iperf2, periodic results are reported for TCP */
	ret = iperf3_upload(&param, udp, &result,
			    (!udp && periodic.interval_ms > 0U) ? &periodic : NULL);
	if (ret < 0) {
		ctx->callback(ZPERF_SESSION_ERROR, NULL, ctx->user_data);
		return;
	}

	ctx->callback(ZPERF_SESSION_FINISHED, &result, ctx->user_data);
}

static void udp_upload_async_work(struct k_work *work)
{
	ARG_UNUSED(work);

	upload_async_work(&udp_async_ctx, true);
}

static void tcp_upload_async_work(struct k_work *work)
{
	ARG_UNUSED(work);

	upload_async_work(&tcp_async_ctx, false);
}

static int upload_async(struct zperf_async_upload_context *ctx, enum session_proto proto,
			const struct zperf_upload_params *param, zperf_callback callback,
			void *user_data)
{
	if (k_work_is_pending(&ctx->work)) {
		return -EBUSY;
	}

	memcpy(&ctx->param, param, sizeof(*param));
	ctx->callback = callback;
	ctx->user_data = user_data;

	zperf_async_work_submit(proto, -1, &ctx->work);

	return 0;
}

int zperf_iperf3_udp_upload_async(const struct zperf_upload_params *param, zperf_callback callback,
			   void *user_data)
{
	return upload_async(&udp_async_ctx, SESSION_UDP, param, callback, user_data);
}

int zperf_iperf3_tcp_upload_async(const struct zperf_upload_params *param, zperf_callback callback,
			   void *user_data)
{
	return upload_async(&tcp_async_ctx, SESSION_TCP, param, callback, user_data);
}

void zperf_udp_uploader_init(void)
{
	k_work_init(&udp_async_ctx.work, udp_upload_async_work);
}

void zperf_tcp_uploader_init(void)
{
	k_work_init(&tcp_async_ctx.work, tcp_upload_async_work);
}
