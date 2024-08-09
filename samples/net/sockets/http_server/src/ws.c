/*
 * Copyright (c) 2024, Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_http_server_sample, LOG_LEVEL_DBG);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) || defined(CONFIG_COVERAGE_GCOV)
#define STACK_SIZE 4096
#else
#define STACK_SIZE 2048
#endif

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#if defined(CONFIG_USERSPACE)
#include <zephyr/app_memory/app_memdomain.h>
extern struct k_mem_partition app_partition;
extern struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

#define MAX_CLIENT_QUEUE CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS
#define RECV_BUFFER_SIZE 1280

struct ws_netstats_ctx {
	int sock;
	struct k_work_delayable work;
};

K_THREAD_STACK_ARRAY_DEFINE(ws_handler_stack,
			    CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS,
			    STACK_SIZE);
static struct k_thread ws_handler_thread[CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS];
static APP_BMEM bool ws_handler_in_use[CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS];

static struct ws_netstats_ctx netstats_ctx[CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS];

static struct data {
	int sock;
	uint32_t counter;
	uint32_t bytes_received;
	struct pollfd fds[1];
	char recv_buffer[RECV_BUFFER_SIZE];
} config[CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS] = {
	[0 ... (CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS - 1)] = {
		.sock = -1,
		.fds[0].fd = -1,
	}
};

static int get_free_echo_slot(struct data *cfg)
{
	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		if (cfg[i].sock < 0) {
			return i;
		}
	}

	return -1;
}

static int get_free_netstats_slot(void)
{
	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		if (netstats_ctx[i].sock < 0) {
			return i;
		}
	}

	return -1;
}

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

static void ws_echo_handler(void *ptr1, void *ptr2, void *ptr3)
{
	int slot = POINTER_TO_INT(ptr1);
	struct data *cfg = ptr2;
	bool *in_use = ptr3;
	int offset = 0;
	int received;
	int client;
	int ret;

	client = cfg->sock;

	cfg->fds[0].fd = client;
	cfg->fds[0].events = POLLIN;

	/* In this example, we start to receive data from the websocket
	 * and send it back to the client. Note that we could either use
	 * the BSD socket interface if we do not care about Websocket
	 * specific packets, or we could use the websocket_{send/recv}_msg()
	 * function to send websocket specific data.
	 */
	while (true) {
		if (poll(cfg->fds, 1, -1) < 0) {
			LOG_ERR("Error in poll:%d", errno);
			continue;
		}

		if (cfg->fds[0].fd < 0) {
			continue;
		}

		if (cfg->fds[0].revents & ZSOCK_POLLHUP) {
			LOG_DBG("Client #%d has disconnected", client);
			break;
		}

		received = recv(client,
				cfg->recv_buffer + offset,
				sizeof(cfg->recv_buffer) - offset,
				0);

		if (received == 0) {
			/* Connection closed */
			LOG_INF("[%d] Connection closed", slot);
			break;
		} else if (received < 0) {
			/* Socket error */
			LOG_ERR("[%d] Connection error %d", slot, errno);
			break;
		}

		cfg->bytes_received += received;
		offset += received;

#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		/* To prevent fragmentation of the response, reply only if
		 * buffer is full or there is no more data to read
		 */
		if (offset == sizeof(cfg->recv_buffer) ||
		    (recv(client, cfg->recv_buffer + offset,
			  sizeof(cfg->recv_buffer) - offset,
			  MSG_PEEK | MSG_DONTWAIT) < 0 &&
		     (errno == EAGAIN || errno == EWOULDBLOCK))) {
#endif
			ret = sendall(client, cfg->recv_buffer, offset);
			if (ret < 0) {
				LOG_ERR("[%d] Failed to send data, closing socket",
					slot);
				break;
			}

			LOG_DBG("[%d] Received and replied with %d bytes",
				slot, offset);

			if (++cfg->counter % 1000 == 0U) {
				LOG_INF("[%d] Sent %u packets", slot, cfg->counter);
			}

			offset = 0;
#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		}
#endif
	}

	*in_use = false;

	(void)websocket_unregister(client);

	cfg->sock = -1;
}

static int netstats_collect(char *buf, size_t maxlen)
{
	int ret;
	struct net_stats data;
	uint32_t bytes_recv = 0;
	uint32_t bytes_sent = 0;
	uint32_t ipv6_recv = 0;
	uint32_t ipv6_sent = 0;
	uint32_t ipv4_recv = 0;
	uint32_t ipv4_sent = 0;
	uint32_t tcp_recv = 0;
	uint32_t tcp_sent = 0;

	net_mgmt(NET_REQUEST_STATS_GET_ALL, NULL, &data, sizeof(data));

	const char *net_stats_json_template = "{"
					      "\"bytes_recv\":%u,"
					      "\"bytes_sent\":%u,"
					      "\"ipv6_pkt_recv\":%u,"
					      "\"ipv6_pkt_sent\":%u,"
					      "\"ipv4_pkt_recv\":%u,"
					      "\"ipv4_pkt_sent\":%u,"
					      "\"tcp_bytes_recv\":%u,"
					      "\"tcp_bytes_sent\":%u"
					      "}";

	bytes_recv = data.bytes.received;
	bytes_sent = data.bytes.sent;
#if defined(CONFIG_NET_STATISTICS_IPV6)
	ipv6_recv = data.ipv6.recv;
	ipv6_sent = data.ipv6.sent;
#endif
#if defined(CONFIG_NET_STATISTICS_IPV4)
	ipv4_recv = data.ipv4.recv;
	ipv4_sent = data.ipv4.sent;
#endif
#if defined(CONFIG_NET_STATISTICS_TCP)
	tcp_recv = data.tcp.bytes.received;
	tcp_sent = data.tcp.bytes.sent;
#endif

	ret = snprintf(buf, maxlen, net_stats_json_template, bytes_recv, bytes_sent, ipv6_recv,
		       ipv6_sent, ipv4_recv, ipv4_sent, tcp_recv, tcp_sent);
	if (ret >= maxlen) {
		LOG_ERR("Net stats do not fit in buffer");
		return -ENOSPC;
	}

	return ret;
}

static void netstats_handler(struct k_work *work)
{
	int ret;
	static char tx_buf[256];
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ws_netstats_ctx *ctx = CONTAINER_OF(dwork, struct ws_netstats_ctx, work);

	ret = netstats_collect(tx_buf, sizeof(tx_buf));
	if (ret < 0) {
		LOG_ERR("Unable to collect network statistics, err %d", ret);
		goto unregister;
	}

	ret = websocket_send_msg(ctx->sock, tx_buf, ret, WEBSOCKET_OPCODE_DATA_TEXT, false, true,
				 SYS_FOREVER_MS);
	if (ret < 0) {
		LOG_INF("Couldn't send websocket msg (%d), closing connection", ret);
		goto unregister;
	}

	ret = k_work_reschedule(&ctx->work, K_MSEC(CONFIG_NET_SAMPLE_WEBSOCKET_STATS_INTERVAL));
	if (ret < 0) {
		LOG_ERR("Failed to schedule netstats work, err %d", ret);
		goto unregister;
	}

	return;

unregister:
	(void)websocket_unregister(ctx->sock);
	ctx->sock = -1;
}

int ws_netstats_init(void)
{
	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		netstats_ctx[i].sock = -1;
		k_work_init_delayable(&netstats_ctx[i].work, netstats_handler);
	}

	return 0;
}
SYS_INIT(ws_netstats_init, APPLICATION, 0);

int ws_echo_setup(int ws_socket, void *user_data)
{
	int slot;

	slot = get_free_echo_slot(config);
	if (slot < 0) {
		LOG_ERR("Cannot accept more connections");
		/* The caller will close the connection in this case */
		return -ENOENT;
	}

	config[slot].sock = ws_socket;

	LOG_INF("[%d] Accepted a Websocket connection", slot);

	k_thread_create(&ws_handler_thread[slot],
			ws_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(ws_handler_stack[slot]),
			ws_echo_handler,
			INT_TO_POINTER(slot), &config[slot], &ws_handler_in_use[slot],
			THREAD_PRIORITY,
			IS_ENABLED(CONFIG_USERSPACE) ? K_USER |
						       K_INHERIT_PERMS : 0,
			K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
#define MAX_NAME_LEN sizeof("ws[xx]")
		char name[MAX_NAME_LEN];

		snprintk(name, sizeof(name), "ws[%d]", slot);
		k_thread_name_set(&ws_handler_thread[slot], name);
	}

	return 0;
}

int ws_netstats_setup(int ws_socket, void *user_data)
{
	int ret;
	int slot;

	slot = get_free_netstats_slot();
	if (slot < 0) {
		LOG_ERR("Cannot accept more netstats websocket connections");
		return -ENOENT;
	}

	netstats_ctx[slot].sock = ws_socket;

	ret = k_work_reschedule(&netstats_ctx[slot].work, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to schedule netstats work, err %d", ret);
		return ret;
	}

	LOG_INF("Accepted websocket connection for net stats");
	return 0;
}
