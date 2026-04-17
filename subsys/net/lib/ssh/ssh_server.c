/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/zvfs/eventfd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include <zephyr/net/ssh/server.h>

#include "ssh_connection.h"
#include "ssh_transport.h"

static void ssh_server_thread_entry(void *p1, void *p2, void *p3);
static int ssh_server_thread_run(struct ssh_server *ssh_server);

static struct ssh_server ssh_server_instances[CONFIG_SSH_SERVER_MAX_SERVERS];

void ssh_server_foreach(ssh_service_server_cb_t cb, void *user_data)
{
	ARRAY_FOR_EACH(ssh_server_instances, i) {
		struct ssh_server *sshd = &ssh_server_instances[i];

		if (sshd->running) {
			cb(sshd, i, user_data);
		}
	}
}

struct ssh_server *ssh_server_instance(int instance)
{
	if (instance < ARRAY_SIZE(ssh_server_instances)) {
		return &ssh_server_instances[instance];
	}

	return NULL;
}

int ssh_server_start(struct ssh_server *sshd,
		     const struct net_sockaddr *bind_addr,
		     int host_key_index,
		     const char *username,
		     const char *password,
		     const int *authorized_keys,
		     size_t authorized_keys_len,
		     ssh_server_event_callback_t server_callback,
		     ssh_transport_event_callback_t transport_callback,
		     void *user_data)
{
	if (password == NULL) {
		password = "";
	}

	if (sshd == NULL || bind_addr == NULL || host_key_index < 0 ||
	    server_callback == NULL || transport_callback == NULL ||
	    strlen(password) > sizeof(sshd->password) - 1 ||
	    authorized_keys_len > ARRAY_SIZE(sshd->authorized_keys)) {
		return -EINVAL;
	}

	if (sshd->running) {
		return -EALREADY;
	}

	*sshd = (struct ssh_server) {
		.host_key_index = host_key_index,
		.server_callback = server_callback,
		.transport_callback = transport_callback,
		.callback_user_data = user_data,
		.authorized_keys_len = authorized_keys_len,
	};

	memcpy(&sshd->bind_addr, bind_addr,
	       bind_addr->sa_family == NET_AF_INET6 ?
		       sizeof(struct net_sockaddr_in6) : sizeof(struct net_sockaddr_in));

	memset(sshd->username, 0, sizeof(sshd->username));

	if (username != NULL) {
		strncpy(sshd->username, username, sizeof(sshd->username) - 1);
	}

	strcpy(sshd->password, password);

	if (authorized_keys != NULL) {
		memcpy(sshd->authorized_keys, authorized_keys,
		       authorized_keys_len * sizeof(authorized_keys[0]));
	}

	k_thread_create(&sshd->thread, sshd->thread_stack,
			K_KERNEL_STACK_SIZEOF(sshd->thread_stack),
			ssh_server_thread_entry, sshd, NULL, NULL,
			5, 0, K_FOREVER);
	k_thread_name_set(&sshd->thread, "ssh_server");
	k_thread_start(&sshd->thread);

	return 0;
}

static void ssh_server_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct ssh_server *sshd = p1;
	struct ssh_server_event event;
	int ret;

	ret = ssh_server_thread_run(sshd);

	event.type = SSH_SERVER_EVENT_CLOSED;
	sshd->server_callback(sshd, &event, sshd->callback_user_data);
}

static int ssh_server_thread_run(struct ssh_server *sshd)
{
	const struct net_sockaddr *bind_addr = net_sad(&sshd->bind_addr);
	struct zsock_pollfd fds[2 + CONFIG_SSH_SERVER_MAX_CLIENTS];
	net_socklen_t addr_len = bind_addr->sa_family == NET_AF_INET ?
		sizeof(struct net_sockaddr_in) : sizeof(struct net_sockaddr_in6);
	int ret;
	/* Wake up every 10 seconds to check timeouts */
	const int timeout = 10000;

	NET_DBG("Starting SSH Server");

	sshd->eventfd = zvfs_eventfd(0, 0);
	if (sshd->eventfd < 0) {
		NET_ERR("Failed to create eventfd: %d", errno);
		return -1;
	}

	sshd->sock = zsock_socket(bind_addr->sa_family,
					NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (sshd->sock < 0) {
		NET_ERR("Failed to create TCP socket: %d", errno);
		zsock_close(sshd->eventfd);
		return -1;
	}

	/* If IPv4-to-IPv6 mapping is enabled, then turn off V6ONLY option
	 * so that IPv6 socket can serve IPv4 connections.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6) &&
	    bind_addr->sa_family == NET_AF_INET6) {
		int optval = 0;

		(void)zsock_setsockopt(sshd->sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY,
				       &optval, sizeof(optval));
	}

	ret = zsock_bind(sshd->sock, bind_addr, addr_len);
	if (ret < 0) {
		NET_ERR("Failed to bind TCP socket: %d", errno);
		zsock_close(sshd->sock);
		zsock_close(sshd->eventfd);
		return -1;
	}

	ret = zsock_listen(sshd->sock, 1);
	if (ret < 0) {
		NET_ERR("Failed to listen on TCP socket: %d", errno);
		zsock_close(sshd->sock);
		zsock_close(sshd->eventfd);
		return -1;
	}

	fds[0] = (struct zsock_pollfd){
		.fd = sshd->sock, .events = ZSOCK_POLLIN,
	};
	fds[1] = (struct zsock_pollfd){
		.fd = sshd->eventfd, .events = ZSOCK_POLLIN,
	};

	BUILD_ASSERT(CONFIG_NET_SOCKETS_POLL_MAX >= ARRAY_SIZE(fds));

	for (int i = 2; i < ARRAY_SIZE(fds); i++) {
		fds[i].fd = -1;
	}

	sshd->running = true;
	NET_DBG("Waiting for connection...");

	while (true) {
		void *addr_ptr;

		ret = zsock_poll(fds, ARRAY_SIZE(fds), timeout);
		if (ret < 0) {
			NET_ERR("Poll error (%d)", errno);
			ret = -errno;
			break;
		}

		if (fds[0].revents & ZSOCK_POLLIN) {
			struct net_sockaddr_storage client_addr;
			net_socklen_t client_addr_len = sizeof(client_addr);
			struct ssh_transport *transport = NULL;
			int client_sock;
			char str[NET_INET6_ADDRSTRLEN];
			char *addr_str;

			client_sock = zsock_accept(sshd->sock,
						   net_sad(&client_addr),
						   &client_addr_len);
			if (client_sock < 0) {
				NET_ERR("Accept error (%d)", errno);
				ret = -1;
				break;
			}

			for (int i = 2; i < ARRAY_SIZE(fds); i++) {
				if (fds[i].fd == -1) {
					fds[i] = (struct zsock_pollfd){
						.fd = client_sock,
						.events = ZSOCK_POLLIN
					};

					transport = &sshd->transport[i-2];
					break;
				}
			}

			addr_ptr = client_addr.ss_family == NET_AF_INET ?
				(void *)&net_sin(net_sad(&client_addr))->sin_addr :
				(void *)&net_sin6(net_sad(&client_addr))->sin6_addr;

			addr_str = zsock_inet_ntop(client_addr.ss_family, addr_ptr,
						   str, sizeof(str));
			if (addr_str == NULL) {
				NET_DBG("Failed to convert %s address to string", "client");
				strncpy(str, "<invalid>", sizeof(str));
			}

			if (transport != NULL) {
				ret = ssh_transport_start(
					transport, true, sshd,
					client_sock, net_sad(&client_addr),
					sshd->host_key_index,
					sshd->transport_callback,
					sshd->callback_user_data);
				if (ret == 0) {
					struct ssh_server_event event;

					NET_DBG("Connection %s %s%s%s:%d", "from",
					      client_addr.ss_family == NET_AF_INET6 ? "[" : "",
					      str,
					      client_addr.ss_family == NET_AF_INET6 ? "]" : "",
					      net_ntohs(net_sin(net_sad(&client_addr))->sin_port));

					event.type = SSH_SERVER_EVENT_CLIENT_CONNECTED;
					event.client_connected.transport = transport;
					sshd->server_callback(sshd, &event,
							      sshd->callback_user_data);
				} else {
					NET_DBG("Failed to init connection %s%s%s:%d",
					      client_addr.ss_family == NET_AF_INET6 ? "[" : "",
					      str,
					      client_addr.ss_family == NET_AF_INET6 ? "]" : "",
					      net_ntohs(net_sin(net_sad(&client_addr))->sin_port));
					ssh_transport_close(transport);
					zsock_close(transport->sock);
				}
			} else {
				NET_DBG("Too many connections, refusing %s%s%s:%d",
					client_addr.ss_family == NET_AF_INET6 ? "[" : "",
					str,
					client_addr.ss_family == NET_AF_INET6 ? "]" : "",
					net_ntohs(net_sin(net_sad(&client_addr))->sin_port));

				zsock_close(client_sock);
			}
		}
		if (fds[1].revents) {
			zvfs_eventfd_t value;

			zvfs_eventfd_read(sshd->eventfd, &value);

			if (sshd->stopping) {
				/* Requested stop */
				ret = 0;
				break;
			}
		}

		for (int i = 2; i < ARRAY_SIZE(fds); i++) {
			if (fds[i].fd >= 0 && fds[i].revents) {
				struct ssh_transport *transport = &sshd->transport[i-2];

				ret = ssh_transport_input(transport);
				if (ret < 0) {
					struct ssh_server_event event;

					event.type = SSH_SERVER_EVENT_CLIENT_DISCONNECTED;
					event.client_disconnected.transport = transport;
					sshd->server_callback(sshd, &event,
							      sshd->callback_user_data);

					zsock_close(transport->sock);
					ssh_transport_close(transport);
					fds[i].fd = -1;
				}
			}
		}

		/* Update the transport */
		for (int i = 0; i < ARRAY_SIZE(sshd->transport); i++) {
			struct ssh_transport *transport;

			transport = &sshd->transport[i];
			if (!transport->running) {
				continue;
			}

			ret = ssh_transport_update(transport);
			if (ret != 0) {
				struct ssh_server_event event;

				event.type = SSH_SERVER_EVENT_CLIENT_DISCONNECTED;
				event.client_disconnected.transport = transport;
				sshd->server_callback(sshd, &event,
						      sshd->callback_user_data);

				zsock_close(transport->sock);
				ssh_transport_close(transport);
				fds[i+2].fd = -1;
			}
		}
	}

	for (int i = 0; i < ARRAY_SIZE(sshd->transport); i++) {
		struct ssh_transport *transport;
		struct ssh_server_event event;

		transport = &sshd->transport[i];
		if (!transport->running) {
			continue;
		}

		event.type = SSH_SERVER_EVENT_CLIENT_DISCONNECTED;
		event.client_disconnected.transport = transport;
		sshd->server_callback(sshd, &event, sshd->callback_user_data);

		zsock_close(transport->sock);
		ssh_transport_close(transport);
	}

	zsock_close(sshd->sock);
	zsock_close(sshd->eventfd);

	sshd->running = false;

	return ret;
}

int ssh_server_stop(struct ssh_server *sshd)
{
	zvfs_eventfd_t value = 1;
	int ret;

	if (!sshd->running) {
		return -EALREADY;
	}

	sshd->stopping = true;

	/* Wake up the thread */
	ret = zvfs_eventfd_write(sshd->eventfd, value);
	if (ret == 0) {
		(void)k_thread_join(&sshd->thread, K_FOREVER);
	}

	return ret;
}
