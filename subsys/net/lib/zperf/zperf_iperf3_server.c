/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* iperf3 server.
 *
 * One test runs at a time. A test is a TCP control connection plus one data
 * stream, which for TCP is a second connection to the same port and for UDP a
 * socket opened for the test. Everything runs on the socket service thread
 * and nothing in here blocks it: every read is non-blocking and resumes where
 * it left off on the next event.
 *
 * zperf_iperf3_tcp_download() and zperf_iperf3_udp_download() share the listening sockets.
 * Each enables its own protocol, and a client asking for one that is not
 * enabled is refused.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/zperf.h>
#include <zephyr/sys/byteorder.h>

#include "zperf_internal.h"
#include "zperf_iperf3.h"

enum {
	SLOT_LISTEN4,
	SLOT_LISTEN6,
	SLOT_CTRL,
	SLOT_DATA,
	SLOT_MAX,
};

enum transport {
	TRANSPORT_TCP,
	TRANSPORT_UDP,
	TRANSPORT_MAX,
};

enum srv_state {
	/* No test; the next connection becomes the control connection */
	ST_IDLE,
	ST_COOKIE,
	ST_PARAMS_LEN,
	ST_PARAMS,
	/* Waiting for the data stream */
	ST_CREATE_STREAMS,
	ST_RUNNING,
	ST_RESULTS_LEN,
	ST_RESULTS,
	/* Results sent, waiting for the client to finish */
	ST_DONE,
};

/* Reads per data event before other sockets get a turn */
#define DATA_READ_BUDGET 8

/* How often an active test is checked for inactivity */
#define IDLE_CHECK_PERIOD K_SECONDS(1)

#define JSON_BUF_SIZE (ZPERF_IPERF3_JSON_LEN_SIZE + CONFIG_NET_ZPERF_IPERF3_JSON_MAX_LEN + 1)

struct iperf3_test {
	enum srv_state state;
	enum transport transport;
	bool counters_64bit;
	bool started;
	char cookie[ZPERF_IPERF3_COOKIE_SIZE];

	/* Control channel read in progress */
	uint8_t *rx_buf;
	size_t rx_want;
	size_t rx_have;
	uint8_t len_buf[ZPERF_IPERF3_JSON_LEN_SIZE];
	uint8_t state_byte;

	/* TCP data stream: its cookie, then payload */
	char data_cookie[ZPERF_IPERF3_COOKIE_SIZE];
	size_t data_cookie_have;

	/* Socket of the data stream once the test is over, or -1: no longer
	 * read, but kept open until the client has closed its end.
	 */
	int held_data_sock;

	int64_t start_ticks;
	int64_t end_ticks;
	uint64_t tcp_bytes;
	struct zperf_iperf3_udp_stats udp;
};

static struct {
	bool enabled[TRANSPORT_MAX];
	zperf_callback cb[TRANSPORT_MAX];
	void *user_data[TRANSPORT_MAX];
	uint16_t port;
	/* Bind address and interface the listeners were opened with */
	struct net_sockaddr_storage addr;
	char if_name[NET_IFNAMSIZ];
	struct zsock_pollfd fds[SLOT_MAX];
	int64_t last_activity;
	struct iperf3_test test;
} srv = {
	.fds = { [0 ... SLOT_MAX - 1] = { .fd = -1 } },
	.test = { .held_data_sock = -1 },
};

static uint8_t json_buf[JSON_BUF_SIZE];
static uint8_t data_buf[CONFIG_NET_ZPERF_TCP_RECEIVER_BUF_SIZE];

static void srv_handler(struct net_socket_service_event *pev);
static void idle_work_handler(struct k_work *work);

static K_MUTEX_DEFINE(srv_lock);
static K_WORK_DELAYABLE_DEFINE(idle_work, idle_work_handler);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc_iperf3, srv_handler, SLOT_MAX);

static bool srv_listening(void)
{
	return srv.fds[SLOT_LISTEN4].fd >= 0 || srv.fds[SLOT_LISTEN6].fd >= 0;
}

static void srv_register(void)
{
	int ret;

	/* Every registration replaces the whole set, so always pass all the
	 * slots, free ones included.
	 */
	ret = net_socket_service_register(&svc_iperf3, srv.fds, ARRAY_SIZE(srv.fds), NULL);
	if (ret < 0) {
		NET_ERR("Cannot register socket service handler (%d)", ret);
	}
}

static void slot_close(int slot)
{
	if (srv.fds[slot].fd >= 0) {
		zsock_close(srv.fds[slot].fd);
		srv.fds[slot].fd = -1;
		srv.fds[slot].events = 0;
	}
}

static int slot_open(int slot, int sock)
{
	int ret;

	ret = zsock_fcntl(sock, ZVFS_F_SETFL, ZVFS_O_NONBLOCK);
	if (ret < 0) {
		ret = -errno;
		zsock_close(sock);
		return ret;
	}

	srv.fds[slot].fd = sock;
	srv.fds[slot].events = ZSOCK_POLLIN;

	return 0;
}

static void notify(enum transport transport, enum zperf_status status,
		   struct zperf_results *results)
{
	zperf_callback cb = srv.cb[transport];

	if (cb != NULL) {
		cb(status, results, srv.user_data[transport]);
	}
}

/* Forget the test, closing the data stream if it was still held open */
static void test_clear(void)
{
	if (srv.test.held_data_sock >= 0) {
		zsock_close(srv.test.held_data_sock);
	}

	memset(&srv.test, 0, sizeof(srv.test));
	srv.test.state = ST_IDLE;
	srv.test.held_data_sock = -1;
}

static void test_reset(void)
{
	slot_close(SLOT_CTRL);
	slot_close(SLOT_DATA);
	test_clear();
	srv_register();
}

static void test_abort(void)
{
	enum transport transport = srv.test.transport;
	bool started = srv.test.started;

	test_reset();

	if (started) {
		notify(transport, ZPERF_SESSION_ERROR, NULL);
	}
}

static void ctrl_expect(void *buf, size_t len)
{
	srv.test.rx_buf = buf;
	srv.test.rx_want = len;
	srv.test.rx_have = 0;
}

static int ctrl_send_state(int8_t state)
{
	return zperf_iperf3_send_state(srv.fds[SLOT_CTRL].fd, state, ZSOCK_MSG_DONTWAIT);
}

static void ctrl_server_error(int32_t error)
{
	uint8_t msg[ZPERF_IPERF3_SERVER_ERROR_SIZE];

	msg[0] = (uint8_t)ZPERF_IPERF3_SERVER_ERROR;
	sys_put_be32((uint32_t)error, &msg[1]);
	sys_put_be32(0U, &msg[5]);

	NET_WARN("Refusing iperf3 test: %s", zperf_iperf3_strerror(error));

	(void)zperf_iperf3_send_all(srv.fds[SLOT_CTRL].fd, msg, sizeof(msg), ZSOCK_MSG_DONTWAIT);
	test_abort();
}

static void deny(int sock)
{
	int8_t state = ZPERF_IPERF3_ACCESS_DENIED;

	(void)zsock_send(sock, &state, sizeof(state), ZSOCK_MSG_DONTWAIT);
	zsock_close(sock);
}

static void start_test(void)
{
	if (ctrl_send_state(ZPERF_IPERF3_TEST_START) < 0 ||
	    ctrl_send_state(ZPERF_IPERF3_TEST_RUNNING) < 0) {
		test_abort();
		return;
	}

	srv.test.state = ST_RUNNING;
	srv.test.start_ticks = k_uptime_ticks();
	srv.test.started = true;

	notify(srv.test.transport, ZPERF_SESSION_STARTED, NULL);
}

/* Returns the protocol error to refuse the test with, or 0 */
static int32_t check_params(const struct zperf_iperf3_params *params)
{
	if (params->reverse || params->bidirectional || params->omit > 0 || params->sctp) {
		return ZPERF_IPERF3_IEUNIMP;
	}

	if (params->parallel > 1) {
		return ZPERF_IPERF3_IEUNIMP;
	}

	if (params->udp ? !srv.enabled[TRANSPORT_UDP] :
			  (!params->tcp || !srv.enabled[TRANSPORT_TCP])) {
		return ZPERF_IPERF3_IEUNIMP;
	}

	return 0;
}

static int open_udp_stream(void)
{
	struct net_sockaddr_storage local = { 0 };
	net_socklen_t len = sizeof(local);
	int sock;
	int ret;

	/* Listen on the address and port the control connection arrived at:
	 * the client sends the stream set-up datagram to that same address.
	 */
	ret = zsock_getsockname(srv.fds[SLOT_CTRL].fd, net_sad(&local), &len);
	if (ret < 0) {
		return -errno;
	}

	len = (local.ss_family == NET_AF_INET) ? sizeof(struct net_sockaddr_in)
					       : sizeof(struct net_sockaddr_in6);

	sock = zsock_socket(local.ss_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		return -errno;
	}

	ret = zsock_bind(sock, net_sad(&local), len);
	if (ret < 0) {
		ret = -errno;
		zsock_close(sock);
		return ret;
	}

	return slot_open(SLOT_DATA, sock);
}

static void on_params(size_t len)
{
	struct zperf_iperf3_params params;
	int32_t error;
	int ret;

	ret = zperf_iperf3_params_decode((char *)json_buf, len, &params);
	if (ret < 0) {
		NET_ERR("Cannot decode iperf3 parameters (%d)", ret);
		ctrl_server_error(ZPERF_IPERF3_IERECVPARAMS);
		return;
	}

	error = check_params(&params);
	if (error != 0) {
		ctrl_server_error(error);
		return;
	}

	srv.test.transport = params.udp ? TRANSPORT_UDP : TRANSPORT_TCP;
	srv.test.counters_64bit = (params.udp_counters_64bit != 0);

	NET_INFO("iperf3 %s test, %d s, client %.*s", params.udp ? "UDP" : "TCP", params.time,
		 (int)params.client_version.length,
		 params.client_version.start != NULL ? params.client_version.start : "");

	if (srv.test.transport == TRANSPORT_UDP) {
		ret = open_udp_stream();
		if (ret < 0) {
			NET_ERR("Cannot open the UDP stream socket (%d)", ret);
			test_abort();
			return;
		}

		srv_register();
	}

	srv.test.state = ST_CREATE_STREAMS;

	if (ctrl_send_state(ZPERF_IPERF3_CREATE_STREAMS) < 0) {
		test_abort();
	}
}

static void drain_data(void);
static bool data_waiting(int slot);

static void on_test_end(void)
{
	srv.test.end_ticks = k_uptime_ticks();

	/* Take what is already queued, then stop reading the stream. It is
	 * left for the client to close first, as it does once it has the
	 * results: closing a connection on the listening port from this end
	 * would keep that port busy while the connection winds down.
	 */
	drain_data();
	if (srv.fds[SLOT_DATA].fd >= 0) {
		srv.test.held_data_sock = srv.fds[SLOT_DATA].fd;
		srv.fds[SLOT_DATA].fd = -1;
		srv.fds[SLOT_DATA].events = 0;
	}
	srv_register();

	srv.test.state = ST_RESULTS_LEN;
	ctrl_expect(srv.test.len_buf, sizeof(srv.test.len_buf));

	if (ctrl_send_state(ZPERF_IPERF3_EXCHANGE_RESULTS) < 0) {
		test_abort();
	}
}

static void on_client_results(size_t len)
{
	struct zperf_iperf3_stream_stats client = { 0 };
	struct zperf_iperf3_stream_stats stats = { 0 };
	struct zperf_results results = { 0 };
	uint64_t duration_us;
	int ret;

	/* Only kept for the report; a client that sends something odd here
	 * still gets the server's results.
	 */
	ret = zperf_iperf3_results_decode((char *)json_buf, len, &client);
	if (ret < 0) {
		NET_WARN("Cannot decode the client's iperf3 results (%d)", ret);
	}

	duration_us = k_ticks_to_us_floor64(srv.test.end_ticks - srv.test.start_ticks);

	stats.duration_us = duration_us;
	results.time_in_us = duration_us;
	results.client_time_in_us = client.duration_us;
	results.nb_packets_sent = (uint32_t)client.packets;

	if (srv.test.transport == TRANSPORT_UDP) {
		stats.bytes = srv.test.udp.bytes;
		stats.packets = srv.test.udp.packet_count;
		stats.errors = srv.test.udp.errors;
		stats.jitter_us = zperf_iperf3_udp_jitter_us(&srv.test.udp);

		results.total_len = srv.test.udp.bytes;
		results.nb_packets_rcvd = (uint32_t)srv.test.udp.datagrams;
		results.nb_packets_lost = (uint32_t)srv.test.udp.errors;
		results.nb_packets_outorder = (uint32_t)srv.test.udp.outoforder;
		results.jitter_in_us = (uint32_t)stats.jitter_us;
		results.packet_size = (srv.test.udp.datagrams != 0U) ?
			(uint32_t)(srv.test.udp.bytes / srv.test.udp.datagrams) : 0U;
	} else {
		stats.bytes = srv.test.tcp_bytes;
		results.total_len = srv.test.tcp_bytes;
	}

	ret = zperf_iperf3_results_encode(&stats, false, json_buf, sizeof(json_buf));
	if (ret < 0) {
		NET_ERR("Cannot encode the iperf3 results (%d)", ret);
		test_abort();
		return;
	}

	if (zperf_iperf3_send_all(srv.fds[SLOT_CTRL].fd, json_buf, ret, ZSOCK_MSG_DONTWAIT) < 0 ||
	    ctrl_send_state(ZPERF_IPERF3_DISPLAY_RESULTS) < 0) {
		test_abort();
		return;
	}

	srv.test.state = ST_DONE;
	srv.test.started = false;

	notify(srv.test.transport, ZPERF_SESSION_FINISHED, &results);
}

/* A control message of len bytes has been read in full */
static void on_ctrl_message(size_t len)
{
	uint32_t json_len;

	switch (srv.test.state) {
	case ST_COOKIE:
		srv.test.cookie[ZPERF_IPERF3_COOKIE_SIZE - 1] = '\0';
		srv.test.state = ST_PARAMS_LEN;
		ctrl_expect(srv.test.len_buf, sizeof(srv.test.len_buf));
		if (ctrl_send_state(ZPERF_IPERF3_PARAM_EXCHANGE) < 0) {
			test_abort();
		}
		break;

	case ST_PARAMS_LEN:
	case ST_RESULTS_LEN:
		json_len = sys_get_be32(srv.test.len_buf);
		if (json_len == 0U || json_len > CONFIG_NET_ZPERF_IPERF3_JSON_MAX_LEN) {
			NET_ERR("iperf3 message of %u bytes does not fit", json_len);
			if (srv.test.state == ST_PARAMS_LEN) {
				ctrl_server_error(ZPERF_IPERF3_IERECVPARAMS);
			} else {
				test_abort();
			}
			break;
		}

		srv.test.state = (srv.test.state == ST_PARAMS_LEN) ? ST_PARAMS : ST_RESULTS;
		ctrl_expect(json_buf, json_len);
		break;

	case ST_PARAMS:
		on_params(len);
		break;

	case ST_RESULTS:
		on_client_results(len);
		break;

	case ST_CREATE_STREAMS:
	case ST_RUNNING:
	case ST_DONE:
		/* A single state byte */
		if (srv.test.state == ST_RUNNING && srv.test.state_byte == ZPERF_IPERF3_TEST_END) {
			on_test_end();
		} else if (srv.test.state == ST_DONE &&
			   srv.test.state_byte == (uint8_t)ZPERF_IPERF3_IPERF_DONE) {
			/* The client closes next; the end of file finishes the
			 * test, for the same reason the data stream is held.
			 */
		} else {
			if (srv.test.state_byte != (uint8_t)ZPERF_IPERF3_CLIENT_TERMINATE) {
				NET_WARN("Unexpected iperf3 state %d", (int8_t)srv.test.state_byte);
			}
			test_abort();
		}
		break;

	default:
		test_abort();
		break;
	}
}

static void on_ctrl(void)
{
	/* Read until the socket runs dry; several messages may be queued */
	while (srv.test.state != ST_IDLE) {
		size_t len;
		ssize_t ret;

		if (srv.test.rx_want == 0U) {
			/* Between messages the client only ever sends a state */
			ctrl_expect(&srv.test.state_byte, sizeof(srv.test.state_byte));
		}

		ret = zsock_recv(srv.fds[SLOT_CTRL].fd, srv.test.rx_buf + srv.test.rx_have,
				 srv.test.rx_want - srv.test.rx_have, ZSOCK_MSG_DONTWAIT);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}

			test_abort();
			break;
		}

		if (ret == 0) {
			/* A client that has its results may simply go */
			if (srv.test.state == ST_DONE) {
				test_reset();
			} else {
				NET_WARN("iperf3 client went away");
				test_abort();
			}
			break;
		}

		srv.test.rx_have += (size_t)ret;
		if (srv.test.rx_have < srv.test.rx_want) {
			continue;
		}

		len = srv.test.rx_want;
		srv.test.rx_want = 0U;
		on_ctrl_message(len);
	}
}

static void on_tcp_data(void)
{
	int sock = srv.fds[SLOT_DATA].fd;

	if (srv.test.state == ST_CREATE_STREAMS) {
		/* Read exactly the cookie; payload may follow it */
		size_t want = ZPERF_IPERF3_COOKIE_SIZE - srv.test.data_cookie_have;
		ssize_t ret;

		ret = zsock_recv(sock, srv.test.data_cookie + srv.test.data_cookie_have, want,
				 ZSOCK_MSG_DONTWAIT);
		if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return;
		}

		if (ret <= 0) {
			slot_close(SLOT_DATA);
			srv_register();
			return;
		}

		srv.test.data_cookie_have += (size_t)ret;
		if (srv.test.data_cookie_have < ZPERF_IPERF3_COOKIE_SIZE) {
			return;
		}

		if (memcmp(srv.test.data_cookie, srv.test.cookie,
			   ZPERF_IPERF3_COOKIE_SIZE - 1) != 0) {
			NET_WARN("iperf3 data connection with a foreign cookie");
			srv.fds[SLOT_DATA].fd = -1;
			deny(sock);
			srv.test.data_cookie_have = 0;
			srv_register();
			return;
		}

		start_test();
		return;
	}

	for (int i = 0; i < DATA_READ_BUDGET; i++) {
		ssize_t ret = zsock_recv(sock, data_buf, sizeof(data_buf), ZSOCK_MSG_DONTWAIT);

		if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return;
		}

		if (ret <= 0) {
			/* The stream ended ahead of TEST_END: keep waiting for
			 * the control connection, but stop polling this one.
			 */
			slot_close(SLOT_DATA);
			srv_register();
			return;
		}

		srv.test.tcp_bytes += (size_t)ret;
	}
}

static void on_udp_data(void)
{
	int sock = srv.fds[SLOT_DATA].fd;
	size_t hdr_size = srv.test.counters_64bit ? ZPERF_IPERF3_UDP_HDR_SIZE_64 :
						 ZPERF_IPERF3_UDP_HDR_SIZE;
	uint8_t hdr[ZPERF_IPERF3_UDP_HDR_SIZE_64];

	if (srv.test.state == ST_CREATE_STREAMS) {
		struct net_sockaddr_storage peer = { 0 };
		net_socklen_t peer_len = sizeof(peer);
		ssize_t ret;

		/* Any first datagram sets the stream up; clients differ in
		 * what they put in it.
		 */
		ret = zsock_recvfrom(sock, hdr, sizeof(hdr), ZSOCK_MSG_DONTWAIT, net_sad(&peer),
				     &peer_len);
		if (ret < 0) {
			return;
		}

		if (zsock_connect(sock, net_sad(&peer), peer_len) < 0 ||
		    zsock_send(sock, zperf_iperf3_udp_connect_reply, ZPERF_IPERF3_UDP_CONNECT_SIZE,
			       0) < 0) {
			NET_ERR("Cannot answer the iperf3 UDP stream (%d)", errno);
			test_abort();
			return;
		}

		start_test();
		return;
	}

	for (int i = 0; i < DATA_READ_BUDGET; i++) {
		uint64_t sent_us;
		uint64_t now_us;
		uint64_t seq;
		ssize_t ret;

		/* Only the header is needed. MSG_TRUNC still reports the
		 * whole datagram length.
		 */
		ret = zsock_recv(sock, hdr, hdr_size, ZSOCK_MSG_DONTWAIT | ZSOCK_MSG_TRUNC);
		if (ret < 0) {
			return;
		}

		/* A repeated set-up datagram is shorter than any data */
		if (zperf_iperf3_udp_hdr_get(hdr, (size_t)ret, srv.test.counters_64bit, &sent_us,
					     &seq) < 0) {
			continue;
		}

		now_us = k_ticks_to_us_floor64(k_uptime_ticks());
		zperf_iperf3_udp_account(&srv.test.udp, seq, (int64_t)(now_us - sent_us),
					 (size_t)ret);
	}
}

static void on_data(void)
{
	if (srv.test.transport == TRANSPORT_UDP) {
		on_udp_data();
	} else {
		on_tcp_data();
	}
}

/* Upper bound on reads when draining the stream at the end of a test */
#define DRAIN_READ_LIMIT 1024

static void drain_data(void)
{
	/* Everything the client sent before TEST_END belongs to the test,
	 * however much of it is still queued.
	 */
	for (int i = 0; i < DRAIN_READ_LIMIT / DATA_READ_BUDGET; i++) {
		if (srv.test.state != ST_RUNNING || !data_waiting(SLOT_DATA)) {
			break;
		}

		on_data();
	}
}

static void on_accept(int listen_fd)
{
	bool changed = false;

	for (;;) {
		struct net_sockaddr_storage peer = { 0 };
		net_socklen_t peer_len = sizeof(peer);
		int sock;

		sock = zsock_accept(listen_fd, net_sad(&peer), &peer_len);
		if (sock < 0) {
			break;
		}

		/* A test whose results are out is over, even if its client's
		 * last message is still queued behind this connection.
		 */
		if (srv.test.state == ST_DONE) {
			test_reset();
		}

		if (srv.test.state == ST_IDLE && srv.fds[SLOT_CTRL].fd < 0) {
			if (slot_open(SLOT_CTRL, sock) < 0) {
				continue;
			}

			test_clear();
			srv.test.state = ST_COOKIE;
			srv.last_activity = k_uptime_get();
			ctrl_expect(srv.test.cookie, ZPERF_IPERF3_COOKIE_SIZE);
			(void)k_work_reschedule(&idle_work, IDLE_CHECK_PERIOD);
			changed = true;
		} else if (srv.test.state == ST_CREATE_STREAMS &&
			   srv.test.transport == TRANSPORT_TCP && srv.fds[SLOT_DATA].fd < 0) {
			if (slot_open(SLOT_DATA, sock) < 0) {
				continue;
			}

			srv.test.data_cookie_have = 0;
			changed = true;
		} else {
			/* Busy, as iperf3 answers a second client */
			deny(sock);
		}
	}

	if (changed) {
		srv_register();
	}
}

static void srv_handler(struct net_socket_service_event *pev)
{
	int fd = pev->event.fd;
	short revents = pev->event.revents;

	k_mutex_lock(&srv_lock, K_FOREVER);

	if (!srv_listening() || fd < 0) {
		goto out;
	}

	if (fd == srv.fds[SLOT_LISTEN4].fd || fd == srv.fds[SLOT_LISTEN6].fd) {
		if ((revents & (ZSOCK_POLLERR | ZSOCK_POLLNVAL)) != 0) {
			/* Stop polling it, or the error is reported for ever */
			NET_ERR("iperf3 listening socket failed");
			slot_close(fd == srv.fds[SLOT_LISTEN4].fd ? SLOT_LISTEN4 : SLOT_LISTEN6);
			srv_register();
			if (!srv_listening()) {
				test_abort();
			}
			goto out;
		}

		on_accept(fd);
		goto out;
	}

	/* Only the connections of the test count as its client being alive */
	srv.last_activity = k_uptime_get();

	if (fd == srv.fds[SLOT_CTRL].fd) {
		if ((revents & (ZSOCK_POLLERR | ZSOCK_POLLNVAL)) != 0) {
			test_abort();
		} else {
			/* A hang-up is seen by reading end of file */
			on_ctrl();
		}
	} else if (fd == srv.fds[SLOT_DATA].fd) {
		if ((revents & (ZSOCK_POLLERR | ZSOCK_POLLNVAL)) != 0) {
			slot_close(SLOT_DATA);
			srv_register();
		} else {
			on_data();
		}
	}

	/* Anything else is an event for a socket already closed */

out:
	k_mutex_unlock(&srv_lock);
}

static bool data_waiting(int slot)
{
	uint8_t byte;

	return srv.fds[slot].fd >= 0 &&
	       zsock_recv(srv.fds[slot].fd, &byte, sizeof(byte),
			  ZSOCK_MSG_DONTWAIT | ZSOCK_MSG_PEEK) > 0;
}

static void idle_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_mutex_lock(&srv_lock, K_FOREVER);

	if (srv.test.state != ST_IDLE) {
		/* This thread may run while the socket service thread, which
		 * has the lowest priority, is kept from reading. Data waiting
		 * is not the client's silence the timeout is for.
		 */
		if (data_waiting(SLOT_CTRL) || data_waiting(SLOT_DATA)) {
			srv.last_activity = k_uptime_get();
		}

		if (k_uptime_get() - srv.last_activity >
		    CONFIG_NET_ZPERF_IPERF3_IDLE_TIMEOUT * MSEC_PER_SEC) {
			NET_WARN("iperf3 test idle, dropping it");
			test_abort();
		} else {
			(void)k_work_reschedule(&idle_work, IDLE_CHECK_PERIOD);
		}
	}

	k_mutex_unlock(&srv_lock);
}

static int bind_addr(int family, const struct net_sockaddr_storage *given,
		     struct net_sockaddr_storage *addr, net_socklen_t *len)
{
	memset(addr, 0, sizeof(*addr));
	addr->ss_family = family;

	if (family == NET_AF_INET) {
		struct net_sockaddr_in *in = net_sin(net_sad(addr));

		*len = sizeof(*in);
		in->sin_port = net_htons(srv.port);

		if (given->ss_family == NET_AF_INET) {
			in->sin_addr = net_sin(net_sad(given))->sin_addr;
		} else if (strlen(MY_IP4ADDR ? MY_IP4ADDR : "") > 0 &&
			   zperf_get_ipv4_addr(MY_IP4ADDR, &in->sin_addr) == 0) {
			/* The address the application was configured with */
		} else {
			in->sin_addr.s_addr = NET_INADDR_ANY;
		}

		if (net_ipv4_is_addr_mcast(&in->sin_addr)) {
			return -ENOTSUP;
		}
	} else {
		struct net_sockaddr_in6 *in6 = net_sin6(net_sad(addr));

		*len = sizeof(*in6);
		in6->sin6_port = net_htons(srv.port);

		if (given->ss_family == NET_AF_INET6) {
			in6->sin6_addr = net_sin6(net_sad(given))->sin6_addr;
		} else if (strlen(MY_IP6ADDR ? MY_IP6ADDR : "") > 0 &&
			   zperf_get_ipv6_addr(MY_IP6ADDR, MY_PREFIX_LEN_STR,
					       &in6->sin6_addr) == 0) {
			/* The address the application was configured with */
		} else {
			memcpy(&in6->sin6_addr, net_ipv6_unspecified_address(),
			       sizeof(struct net_in6_addr));
		}

		if (net_ipv6_is_addr_mcast(&in6->sin6_addr)) {
			return -ENOTSUP;
		}
	}

	return 0;
}

static int open_listener(int slot, int family, const struct net_sockaddr_storage *given)
{
	struct net_sockaddr_storage addr = { 0 };
	net_socklen_t len;
	int on = 1;
	int sock;
	int ret;

	ret = bind_addr(family, given, &addr, &len);
	if (ret < 0) {
		return ret;
	}

	sock = zsock_socket(family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (sock < 0) {
		return -errno;
	}

	/* The connections of the previous test may still be closing when the
	 * server is started again.
	 */
	(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &on, sizeof(on));

	if (family == NET_AF_INET6) {
		/* IPv4 has a listener of its own. Where the socket layer is
		 * dual stack, as offloaded host sockets are, an IPv6 wildcard
		 * would otherwise claim the IPv4 port as well.
		 */
		(void)zsock_setsockopt(sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &on, sizeof(on));
	}

	/* Room for a control connection and its data stream */
	if (zsock_bind(sock, net_sad(&addr), len) < 0 || zsock_listen(sock, 2) < 0) {
		ret = -errno;
		NET_ERR("Cannot listen on IPv%d TCP port %u (%d)", family == NET_AF_INET ? 4 : 6,
			srv.port, ret);
		zsock_close(sock);
		return ret;
	}

	return slot_open(slot, sock);
}

static void srv_close(void)
{
	(void)k_work_cancel_delayable(&idle_work);
	(void)net_socket_service_unregister(&svc_iperf3);

	ARRAY_FOR_EACH(srv.fds, i) {
		slot_close(i);
	}

	test_clear();
}

static int srv_open(const struct zperf_download_params *param)
{
	const struct net_sockaddr_storage *given = &param->addr_storage;
	int family = given->ss_family;
	int ret;

	ARRAY_FOR_EACH(srv.fds, i) {
		srv.fds[i].fd = -1;
	}

	srv.port = param->port;
	srv.addr = *given;
	strncpy(srv.if_name, param->if_name, sizeof(srv.if_name) - 1U);
	srv.if_name[sizeof(srv.if_name) - 1U] = '\0';
	test_clear();

	if (IS_ENABLED(CONFIG_NET_IPV4) && (family == NET_AF_INET || family == NET_AF_UNSPEC)) {
		ret = open_listener(SLOT_LISTEN4, NET_AF_INET, given);
		if (ret < 0) {
			goto error;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && (family == NET_AF_INET6 || family == NET_AF_UNSPEC)) {
		ret = open_listener(SLOT_LISTEN6, NET_AF_INET6, given);
		if (ret < 0) {
			goto error;
		}
	}

	if (!srv_listening()) {
		ret = -EAFNOSUPPORT;
		goto error;
	}

	NET_INFO("iperf3 server listening on port %u", srv.port);
	srv_register();

	return 0;

error:
	srv_close();
	return ret;
}

/* Whether the listeners already open serve the request, which must name
 * the same port, bind address and interface.
 */
static bool srv_listener_matches(const struct zperf_download_params *param)
{
	const struct net_sockaddr_storage *given = &param->addr_storage;

	if (param->port != srv.port || given->ss_family != srv.addr.ss_family ||
	    strncmp(param->if_name, srv.if_name, sizeof(srv.if_name)) != 0) {
		return false;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && given->ss_family == NET_AF_INET) {
		return net_ipv4_addr_cmp(&net_sin(net_sad(given))->sin_addr,
					 &net_sin(net_sad(&srv.addr))->sin_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && given->ss_family == NET_AF_INET6) {
		return net_ipv6_addr_cmp(&net_sin6(net_sad(given))->sin6_addr,
					 &net_sin6(net_sad(&srv.addr))->sin6_addr);
	}

	/* NET_AF_UNSPEC: any address, nothing more to compare */
	return true;
}

static int srv_enable(enum transport transport, const struct zperf_download_params *param,
		      zperf_callback callback, void *user_data)
{
	int ret = 0;

	k_mutex_lock(&srv_lock, K_FOREVER);

	if (srv.enabled[transport]) {
		ret = -EALREADY;
		goto out;
	}

	if (srv_listening()) {
		/* The other protocol already has the listening sockets, which
		 * the request must be able to share.
		 */
		if (!srv_listener_matches(param)) {
			NET_ERR("iperf3 server already listening on port %u with another "
				"address or interface", srv.port);
			ret = -EBUSY;
			goto out;
		}
	} else {
		ret = srv_open(param);
		if (ret < 0) {
			goto out;
		}
	}

	srv.cb[transport] = callback;
	srv.user_data[transport] = user_data;
	srv.enabled[transport] = true;

out:
	k_mutex_unlock(&srv_lock);
	return ret;
}

static int srv_disable(enum transport transport)
{
	int ret = 0;

	k_mutex_lock(&srv_lock, K_FOREVER);

	if (!srv.enabled[transport]) {
		ret = -EALREADY;
		goto out;
	}

	/* End a test of this protocol, telling the client why unless it
	 * already has its results
	 */
	if (srv.test.state != ST_IDLE && srv.test.transport == transport) {
		if (srv.test.state != ST_DONE) {
			(void)ctrl_send_state(ZPERF_IPERF3_SERVER_TERMINATE);
		}
		test_abort();
	}

	srv.enabled[transport] = false;
	srv.cb[transport] = NULL;
	srv.user_data[transport] = NULL;

	if (!srv.enabled[TRANSPORT_TCP] && !srv.enabled[TRANSPORT_UDP]) {
		if (srv.test.state != ST_IDLE && srv.test.state != ST_DONE) {
			(void)ctrl_send_state(ZPERF_IPERF3_SERVER_TERMINATE);
		}

		test_reset();
		srv_close();
	}

out:
	k_mutex_unlock(&srv_lock);
	return ret;
}

int zperf_iperf3_tcp_download(const struct zperf_download_params *param, zperf_callback callback,
		       void *user_data)
{
	return srv_enable(TRANSPORT_TCP, param, callback, user_data);
}

int zperf_iperf3_udp_download(const struct zperf_download_params *param, zperf_callback callback,
		       void *user_data)
{
	return srv_enable(TRANSPORT_UDP, param, callback, user_data);
}

int zperf_iperf3_tcp_download_stop(void)
{
	return srv_disable(TRANSPORT_TCP);
}

int zperf_iperf3_udp_download_stop(void)
{
	return srv_disable(TRANSPORT_UDP);
}
