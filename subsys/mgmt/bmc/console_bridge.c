/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(console_bridge, LOG_LEVEL_INF);

#include <zephyr/posix/fcntl.h>
#include <zephyr/net/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#include "console_logger.h"
#include "synch.h"

#define CONSOLE_BRIDGE_PORT 22
#define CONSOLE_BRIDGE_STACK_SIZE K_THREAD_STACK_SIZEOF(console_bridge_stack_area)
#define CONSOLE_BRIDGE_PRIORITY   CONFIG_CONSOLE_BRIDGE_PRIORITY

K_THREAD_STACK_DEFINE(console_bridge_stack_area, CONFIG_CONSOLE_BRIDGE_STACK_SIZE);
static struct k_thread console_bridge_thread_data;

K_THREAD_STACK_DEFINE(socket_send_thread_stack_area, CONFIG_CONSOLE_BRIDGE_STACK_SIZE);
static struct k_thread socket_send_thread_data;

static volatile int active_client_fd = -1;
static volatile bool new_client;

static void socket_send_thread(void *a, void *b, void *c)
{
	uint64_t pos = 0;
	uint8_t buf[64];

	while (1) {
		ssize_t ret;

		if (active_client_fd == -1) {
			k_event_wait_safe(&events, EVENT_TELNET_CLIENT, false, K_FOREVER);
			continue;
		}

		if (new_client) {
			/*
			 * Whereas websocket clients get all history, telnet
			 * clients only send subsequent console output.
			 */
			host_console_seek_end(&pos);
			new_client = false;
		}

		ret = host_console_read(buf, sizeof(buf), &pos);
		if (ret < 0)
			continue;
		if (ret == 0) {
			k_event_wait_safe(&events,
					EVENT_TELNET_CLIENT |
					EVENT_CONSOLE_LOG_DATA,
					false, K_FOREVER);
			continue;
		}

		if (active_client_fd == -1)
			continue;

		ret = send(active_client_fd, buf, ret, 0);
		if (ret <= 0) {
			LOG_WRN("Socket send error: %d len: %zd", errno, ret);
			shutdown(active_client_fd, 2);
			active_client_fd = -1;
		}
	}
}

static void handle_client(int client_fd)
{
	/*
	 * Improve performance by enabling TCP_NODELAY (ie disabling Nagle)
	 */
	int opt = 1;
	if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		LOG_WRN("Failed to set TCP_NODELAY: %d", errno);
	}

	LOG_INF("Client connected to console bridge (host console UART -> TCP port %d)",
		CONSOLE_BRIDGE_PORT);

	/* Set active client */
	active_client_fd = client_fd;
	new_client = true;
	k_event_post(&events, EVENT_TELNET_CLIENT);

	uint8_t socket_buf[64];

	while (active_client_fd != -1) {
		ssize_t rc = recv(active_client_fd, socket_buf, sizeof(socket_buf), 0);
		if (rc <= 0) {
			if (rc == 0) {
				LOG_INF("Client disconnected gracefully");
			} else {
				LOG_WRN("Socket recv() error: %d", errno);
			}
			shutdown(active_client_fd, 2);
			active_client_fd = -1;
			break;
		}

		/* XXX: error handling */
		host_console_write(socket_buf, rc);
	}

	LOG_INF("Console bridge client disconnected");
}

static void console_bridge_daemon_thread(void *a, void *b, void *c)
{
	int server_fd;
	struct sockaddr_in server_addr;
	int opt;

	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0) {
		LOG_ERR("socket() failed: %d", errno);
		return;
	}

	/* Set SO_REUSEADDR to allow rebinding */
	opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		LOG_WRN("Failed to set SO_REUSEADDR: %d", errno);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONSOLE_BRIDGE_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		LOG_ERR("bind() failed: %d", errno);
		close(server_fd);
		return;
	}

	if (listen(server_fd, 1) < 0) {
		LOG_ERR("listen() failed: %d", errno);
		close(server_fd);
		return;
	}

	LOG_INF("Console bridge daemon listening on port %d...", CONSOLE_BRIDGE_PORT);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd;

		client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
				   &client_addr_len);
		if (client_fd < 0) {
			LOG_ERR("accept() failed: %d", errno);
			continue;
		}

		handle_client(client_fd);

		close(client_fd);
		LOG_INF("Client disconnected. Waiting for new connection...");
	}

	/* Unreachable */
	close(server_fd);
}

int console_bridge_init(void)
{
	LOG_INF("Starting console bridge daemon (host console UART -> TCP port %d)...", CONSOLE_BRIDGE_PORT);

	k_thread_create(&console_bridge_thread_data, console_bridge_stack_area,
			CONSOLE_BRIDGE_STACK_SIZE,
			console_bridge_daemon_thread,
			NULL, NULL, NULL,
			CONSOLE_BRIDGE_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&console_bridge_thread_data, "console_bridge");

	k_thread_create(&socket_send_thread_data, socket_send_thread_stack_area,
			CONFIG_CONSOLE_BRIDGE_STACK_SIZE,
			socket_send_thread,
			NULL, NULL, NULL,
			CONSOLE_BRIDGE_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&socket_send_thread_data, "socket_send");

	return 0;
}
