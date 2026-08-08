/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Publishes the captured host console on a websocket so that the web UI can
 * attach a terminal to it. Unlike the TCP bridge, a websocket client is sent
 * the whole retained log when it connects.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/console.h>
#include <zephyr/mgmt/bmc/http.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/websocket.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/* No benefit in going beyond the host console UART TX chunk size. */
#define WS_RX_BUF_SIZE 32

/*
 * Buffer the HTTP server uses while upgrading the connection. The server
 * documentation suggests 1024 bytes, 256 is enough for the short control
 * messages the terminal sends.
 */
#define WS_UPGRADE_BUF_SIZE 256

#define WS_SEND_BUF_SIZE 64

struct host_websocket {
	/** Sockets polled by the websocket socket service. */
	struct zsock_pollfd fds[1];
	bool new_client;
};

static struct host_websocket host_websocket = {
	.fds = {{.fd = -1}},
};

static void ws_server_cb(struct net_socket_service_event *evt);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(host_console_ws_server, ws_server_cb, 1);

static void ws_end_client_connection(struct host_websocket *ws)
{
	if (ws->fds[0].fd < 0) {
		return;
	}

	LOG_INF("Closing host console websocket #%d", ws->fds[0].fd);

	(void)net_socket_service_unregister(&host_console_ws_server);
	(void)websocket_unregister(ws->fds[0].fd);

	ws->fds[0].fd = -1;
}

static ssize_t ws_send(struct host_websocket *ws, const void *buf, size_t size)
{
	size_t copied = 0;

	if (ws->fds[0].fd < 0) {
		return -ENOTCONN;
	}

	while (copied < size) {
		ssize_t ret;

		/*
		 * Binary frames are required: the host output is arbitrary
		 * bytes, and a text frame that is not valid UTF-8 makes the
		 * peer close the connection.
		 */
		ret = websocket_send_msg(ws->fds[0].fd, (const uint8_t *)buf + copied,
					 size - copied, WEBSOCKET_OPCODE_DATA_BINARY, false, true,
					 SYS_FOREVER_MS);
		if (ret < 0) {
			LOG_ERR("Host console websocket send failed (err=%zd)", ret);
			ws_end_client_connection(ws);
			return copied ? (ssize_t)copied : ret;
		}

		copied += ret;
	}

	return copied;
}

static ssize_t ws_recv(struct host_websocket *ws, void *buf, size_t size)
{
	uint32_t message_type;
	uint64_t remaining;
	ssize_t ret;

	ret = websocket_recv_msg(ws->fds[0].fd, buf, size, &message_type, &remaining, 0);
	if (ret == -EAGAIN) {
		return -EAGAIN;
	}

	if (ret < 0) {
		LOG_DBG("Host console websocket receive failed (err=%zd)", ret);
		ws_end_client_connection(ws);
		return ret;
	}

	if (ret == 0) {
		LOG_DBG("Host console websocket client closed the connection");
		ws_end_client_connection(ws);
	}

	return ret;
}

static K_THREAD_STACK_DEFINE(send_stack, CONFIG_BMC_CONSOLE_BRIDGE_WS_STACK_SIZE);
static struct k_thread send_thread_data;

static void send_thread(void *a, void *b, void *c)
{
	struct host_websocket *ws = &host_websocket;
	uint8_t buf[WS_SEND_BUF_SIZE];
	uint64_t pos = 0;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		ssize_t nread;

		if (ws->fds[0].fd < 0) {
			(void)k_event_wait_safe(&bmc_console_events, BMC_CONSOLE_EVENT_WS_CLIENT,
						false, K_FOREVER);
			continue;
		}

		if (ws->new_client) {
			ws->new_client = false;
			/* Replay the retained log to the new client. */
			pos = 0;
		}

		nread = bmc_console_read(buf, sizeof(buf), &pos);
		if (nread < 0) {
			continue;
		}

		if (nread == 0) {
			(void)k_event_wait_safe(&bmc_console_events,
						BMC_CONSOLE_EVENT_WS_CLIENT |
							BMC_CONSOLE_EVENT_DATA,
						false, K_FOREVER);
			continue;
		}

		if (ws_send(ws, buf, nread) < 0) {
			continue;
		}
	}
}

static uint8_t ws_recv_buf[WS_RX_BUF_SIZE];

static void ws_server_cb(struct net_socket_service_event *evt)
{
	struct host_websocket *ws = evt->user_data;
	net_socklen_t optlen = sizeof(int);
	int sock_error;

	if ((evt->event.revents & (ZSOCK_POLLERR | ZSOCK_POLLNVAL)) != 0) {
		(void)zsock_getsockopt(evt->event.fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_ERROR,
				       &sock_error, &optlen);
		LOG_ERR("Host console websocket %d error (err=%d)", evt->event.fd, sock_error);

		if (evt->event.fd == ws->fds[0].fd) {
			ws_end_client_connection(ws);
		}

		return;
	}

	if ((evt->event.revents & ZSOCK_POLLIN) == 0) {
		return;
	}

	if (evt->event.fd == ws->fds[0].fd) {
		ssize_t ret;

		ret = ws_recv(ws, ws_recv_buf, sizeof(ws_recv_buf));
		if (ret <= 0) {
			return;
		}

		ret = bmc_console_write(ws_recv_buf, ret);
		if (ret < 0) {
			LOG_WRN("Could not forward input to the host console (err=%zd)", ret);
		}
	}
}

static int console_ws_http_cb(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	struct host_websocket *ws = user_data;
	int ret;

	if (ws_socket < 0) {
		LOG_ERR("Invalid websocket socket %d", ws_socket);
		return -EBADF;
	}

	ret = bmc_http_ws_auth(ws_socket, request_ctx, user_data);
	if (ret < 0) {
		return ret;
	}

	/* Only one terminal at a time, so kick out any previous client. */
	ws_end_client_connection(ws);

	ws->fds[0].fd = ws_socket;
	ws->fds[0].events = ZSOCK_POLLIN;
	ws->new_client = true;

	ret = net_socket_service_register(&host_console_ws_server, ws->fds, ARRAY_SIZE(ws->fds),
					  ws);
	if (ret < 0) {
		LOG_ERR("Could not register the websocket socket service (err=%d)", ret);
		(void)zsock_close(ws_socket);
		ws->fds[0].fd = -1;
		return ret;
	}

	LOG_INF("Host console websocket client connected");
	k_event_post(&bmc_console_events, BMC_CONSOLE_EVENT_WS_CLIENT);

	return 0;
}

static uint8_t ws_upgrade_buf[WS_UPGRADE_BUF_SIZE];

static struct http_resource_detail_websocket ws_console_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_WEBSOCKET,
		/* HTTP/1.1 GET is what the upgrade handshake uses. */
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = console_ws_http_cb,
	.data_buffer = ws_upgrade_buf,
	.data_buffer_len = sizeof(ws_upgrade_buf),
	.user_data = &host_websocket,
};

BMC_HTTP_RESOURCE_DEFINE(bmc_host_console_ws, "/console/host", &ws_console_resource_detail);

static int console_bridge_ws_init(void)
{
	k_thread_create(&send_thread_data, send_stack, K_THREAD_STACK_SIZEOF(send_stack),
			send_thread, NULL, NULL, NULL, CONFIG_BMC_CONSOLE_BRIDGE_WS_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&send_thread_data, "bmc_con_ws");

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_console_bridge_ws, BMC_INIT_PHASE_SERVICE, console_bridge_ws_init, true);
