/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include <zephyr/zvfs/eventfd.h>

#include <zephyr/net/ssh/client.h>

#include "ssh_auth.h"
#include "ssh_connection.h"
#include "ssh_transport.h"

static void ssh_client_thread_entry(void *p1, void *p2, void *p3);
static int ssh_client_thread_run(struct ssh_client *ssh_client);

static struct ssh_client ssh_client_instances[CONFIG_SSH_CLIENT_MAX_CLIENTS];

struct ssh_client *ssh_client_instance(int instance)
{
	if (instance < ARRAY_SIZE(ssh_client_instances)) {
		return &ssh_client_instances[instance];
	}

	return NULL;
}

int ssh_client_start(struct ssh_client *sshc, const char *user_name,
		     const struct net_sockaddr *addr, int host_key_index,
		     ssh_transport_event_callback_t callback, void *user_data)
{
	size_t user_name_len;

	if (sshc == NULL || user_name == NULL || addr == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (sshc->running) {
		return -EALREADY;
	}

	*sshc = (struct ssh_client) {
		.host_key_index = host_key_index,
		.callback = callback,
		.callback_user_data = user_data
	};

	memcpy(&sshc->addr, addr, net_family2size(addr->sa_family));

	user_name_len = strlen(user_name);
	if (user_name_len + 1 >= ARRAY_SIZE(sshc->user_name)) {
		return -EINVAL;
	}

	memcpy(sshc->user_name, user_name, user_name_len + 1);

	k_thread_create(&sshc->thread, sshc->thread_stack,
			K_KERNEL_STACK_SIZEOF(sshc->thread_stack),
			ssh_client_thread_entry, sshc, NULL, NULL,
			5, 0, K_FOREVER);
	k_thread_name_set(&sshc->thread, "ssh_client");
	k_thread_start(&sshc->thread);

	return 0;
}

static void ssh_client_thread_entry(void *p1, void *p2, void *p3)
{
	struct ssh_client *ssh_client = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)ssh_client_thread_run(ssh_client);
}

static int ssh_client_thread_run(struct ssh_client *sshc)
{
	const struct net_sockaddr *addr = net_sad(&sshc->addr);
	net_socklen_t addr_len = net_family2size(addr->sa_family);
	struct ssh_transport *transport = &sshc->transport;
	char str[NET_INET6_ADDRSTRLEN];
	char *addr_str;
	void *addr_ptr;
	struct zsock_pollfd fds[2] = {
		{
			.fd = -1,
			.events = ZSOCK_POLLIN
		},
		{
			.fd = -1,
			.events = ZSOCK_POLLIN
		}
	};

	const int timeout = 10000; /* Wake up every 10 seconds to check timeouts */
	int ret;

	NET_DBG("Starting SSH Client");

	sshc->eventfd = zvfs_eventfd(0, 0);
	if (sshc->eventfd < 0) {
		NET_ERR("Failed to create eventfd: %d", errno);
		return -1;
	}

	sshc->sock = zsock_socket(addr->sa_family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (sshc->sock < 0) {
		NET_ERR("Failed to create TCP socket: %d", errno);
		zsock_close(sshc->eventfd);
		return -1;
	}

	ret = zsock_connect(sshc->sock, addr, addr_len);
	if (ret < 0) {
		NET_DBG("Failed to connect TCP socket: %d", errno);

		zsock_close(sshc->sock);
		zsock_close(sshc->eventfd);

		return -1;
	}

	addr_ptr = addr->sa_family == NET_AF_INET ?
		(void *)&net_sin(addr)->sin_addr : (void *)&net_sin6(addr)->sin6_addr;
	addr_str = zsock_inet_ntop(addr->sa_family, addr_ptr, str, sizeof(str));
	if (addr_str == NULL) {
		NET_ERR("Failed to convert %s address to string", "server");
		strncpy(str, "<invalid>", sizeof(str));
	}

	ret = ssh_transport_start(transport, false, sshc, sshc->sock, addr, -1,
				  sshc->callback, sshc->callback_user_data);
	if (ret == 0) {
		NET_DBG("Connection %s %s%s%s:%d", "to",
			addr->sa_family == NET_AF_INET6 ? "[" : "",
			str,
			addr->sa_family == NET_AF_INET6 ? "]" : "",
			net_ntohs(net_sin(addr)->sin_port));
	} else {
		NET_DBG("Failed to init connection %s%s%s:%d",
			addr->sa_family == NET_AF_INET6 ? "[" : "",
			str,
			addr->sa_family == NET_AF_INET6 ? "]" : "",
			net_ntohs(net_sin(addr)->sin_port));

		ssh_transport_close(transport);
		zsock_close(sshc->sock);
		zsock_close(sshc->eventfd);

		return -1;
	}

	fds[0].fd = sshc->sock;
	fds[1].fd = sshc->eventfd;

	sshc->running = true;

	while (true) {
		ret = zsock_poll(fds, ARRAY_SIZE(fds), timeout);
		if (ret < 0) {
			NET_ERR("Poll error (%d)", errno);
			ret = -errno;
			break;
		}

		if (fds[0].revents) {
			ret = ssh_transport_input(transport);
			if (ret < 0) {
				break;
			}
		}

		if (fds[1].revents) {
			zvfs_eventfd_t value;

			zvfs_eventfd_read(sshc->eventfd, &value);
			if (sshc->stopping) {
				/* Requested stop */
				ret = 0;
				break;
			}
		}

		/* Update the transport */
		ret = ssh_transport_update(transport);
		if (ret != 0) {
			break;
		}
	}

	ssh_transport_close(transport);
	zsock_close(sshc->sock);
	zsock_close(sshc->eventfd);

	sshc->running = false;

	return ret;
}

int ssh_client_stop(struct ssh_client *sshc)
{
	zvfs_eventfd_t value = 1;
	int ret;

	if (!sshc->running) {
		return -EALREADY;
	}

	sshc->stopping = true;

	/* Wake up the thread */
	ret = zvfs_eventfd_write(sshc->eventfd, value);
	if (ret == 0) {
		(void)k_thread_join(&sshc->thread, K_FOREVER);
	}

	return ret;
}
