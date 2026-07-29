/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bridges the captured host console to a plain TCP port. Only output produced
 * after a client connects is forwarded, history is left to the websocket
 * transport.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/console.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/atomic.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#define CONSOLE_BRIDGE_PORT     CONFIG_BMC_CONSOLE_BRIDGE_PORT
#define CONSOLE_BRIDGE_PRIORITY CONFIG_BMC_CONSOLE_BRIDGE_PRIORITY
#define CONSOLE_BRIDGE_BUF_SIZE 64

static K_THREAD_STACK_DEFINE(listen_stack, CONFIG_BMC_CONSOLE_BRIDGE_STACK_SIZE);
static struct k_thread listen_thread_data;

static K_THREAD_STACK_DEFINE(send_stack, CONFIG_BMC_CONSOLE_BRIDGE_STACK_SIZE);
static struct k_thread send_thread_data;

/* Shared between the listening thread and the sending thread. */
static atomic_t active_client_fd = ATOMIC_INIT(-1);
static atomic_t new_client;

static void drop_client(int fd)
{
	if (atomic_cas(&active_client_fd, fd, -1)) {
		(void)zsock_shutdown(fd, ZSOCK_SHUT_RDWR);
	}
}

static void send_thread(void *a, void *b, void *c)
{
	uint8_t buf[CONSOLE_BRIDGE_BUF_SIZE];
	uint64_t pos = 0;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		int fd = (int)atomic_get(&active_client_fd);
		ssize_t ret;

		if (fd < 0) {
			(void)k_event_wait_safe(&bmc_console_events,
						BMC_CONSOLE_EVENT_TCP_CLIENT, false, K_FOREVER);
			continue;
		}

		if (atomic_cas(&new_client, 1, 0)) {
			/*
			 * Unlike websocket clients, TCP clients only get output
			 * produced after they connected.
			 */
			bmc_console_seek_end(&pos);
		}

		ret = bmc_console_read(buf, sizeof(buf), &pos);
		if (ret < 0) {
			continue;
		}

		if (ret == 0) {
			(void)k_event_wait_safe(&bmc_console_events,
						BMC_CONSOLE_EVENT_TCP_CLIENT |
							BMC_CONSOLE_EVENT_DATA,
						false, K_FOREVER);
			continue;
		}

		ret = zsock_send(fd, buf, ret, 0);
		if (ret <= 0) {
			LOG_WRN("Console bridge send error (err=%d)", errno);
			drop_client(fd);
		}
	}
}

static void handle_client(int client_fd)
{
	uint8_t buf[CONSOLE_BRIDGE_BUF_SIZE];
	int opt = 1;

	/* Disable Nagle so that keystrokes are not batched. */
	if (zsock_setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		LOG_WRN("Could not set TCP_NODELAY (err=%d)", errno);
	}

	LOG_INF("Console bridge client connected on port %d", CONSOLE_BRIDGE_PORT);

	atomic_set(&new_client, 1);
	atomic_set(&active_client_fd, client_fd);
	k_event_post(&bmc_console_events, BMC_CONSOLE_EVENT_TCP_CLIENT);

	while (atomic_get(&active_client_fd) == client_fd) {
		ssize_t ret = zsock_recv(client_fd, buf, sizeof(buf), 0);

		if (ret <= 0) {
			if (ret == 0) {
				LOG_INF("Console bridge client disconnected");
			} else {
				LOG_WRN("Console bridge recv error (err=%d)", errno);
			}

			drop_client(client_fd);
			break;
		}

		ret = bmc_console_write(buf, ret);
		if (ret < 0) {
			LOG_WRN("Could not forward input to the host console (err=%d)",
				(int)ret);
		}
	}
}

static void listen_thread(void *a, void *b, void *c)
{
	struct sockaddr_in server_addr;
	int server_fd;
	int opt = 1;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0) {
		LOG_ERR("Console bridge socket() failed (err=%d)", errno);
		return;
	}

	if (zsock_setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		LOG_WRN("Could not set SO_REUSEADDR (err=%d)", errno);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONSOLE_BRIDGE_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (zsock_bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		LOG_ERR("Console bridge bind() failed (err=%d)", errno);
		zsock_close(server_fd);
		return;
	}

	if (zsock_listen(server_fd, 1) < 0) {
		LOG_ERR("Console bridge listen() failed (err=%d)", errno);
		zsock_close(server_fd);
		return;
	}

	LOG_INF("Console bridge listening on port %d", CONSOLE_BRIDGE_PORT);

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd;

		client_fd = zsock_accept(server_fd, (struct sockaddr *)&client_addr,
					 &client_addr_len);
		if (client_fd < 0) {
			LOG_ERR("Console bridge accept() failed (err=%d)", errno);
			continue;
		}

		handle_client(client_fd);
		zsock_close(client_fd);
	}
}

static int console_bridge_init(void)
{
	k_thread_create(&listen_thread_data, listen_stack, K_THREAD_STACK_SIZEOF(listen_stack),
			listen_thread, NULL, NULL, NULL, CONSOLE_BRIDGE_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&listen_thread_data, "bmc_con_listen");

	k_thread_create(&send_thread_data, send_stack, K_THREAD_STACK_SIZEOF(send_stack),
			send_thread, NULL, NULL, NULL, CONSOLE_BRIDGE_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&send_thread_data, "bmc_con_send");

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_console_bridge, BMC_INIT_PHASE_SERVICE, console_bridge_init, true);
