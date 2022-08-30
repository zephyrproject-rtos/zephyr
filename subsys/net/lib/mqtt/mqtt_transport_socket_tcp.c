/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport_socket_tcp.h
 *
 * @brief Internal functions to handle transport over TCP socket.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_sock_tcp, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include "mqtt_os.h"

int mqtt_client_tcp_connect(struct mqtt_client *client)
{
	const struct sockaddr *broker = client->broker;
	int ret;

	client->transport.tcp.sock = zsock_socket(broker->sa_family, SOCK_STREAM,
						  IPPROTO_TCP);
	if (client->transport.tcp.sock < 0) {
		return -errno;
	}

#if defined(CONFIG_SOCKS)
	if (client->transport.proxy.addrlen != 0) {
		ret = setsockopt(client->transport.tcp.sock,
				 SOL_SOCKET, SO_SOCKS5,
				 &client->transport.proxy.addr,
				 client->transport.proxy.addrlen);
		if (ret < 0) {
			goto error;
		}
	}
#endif

	NET_DBG("Created socket %d", client->transport.tcp.sock);

	size_t peer_addr_size = sizeof(struct sockaddr_in6);

	if (broker->sa_family == AF_INET) {
		peer_addr_size = sizeof(struct sockaddr_in);
	}

	ret = zsock_connect(client->transport.tcp.sock, client->broker,
			    peer_addr_size);
	if (ret < 0) {
		goto error;
	}

	NET_DBG("Connect completed");
	return 0;

error:
	(void)zsock_close(client->transport.tcp.sock);
	return -errno;
}

int mqtt_client_tcp_write(struct mqtt_client *client, const uint8_t *data,
			  uint32_t datalen)
{
	uint32_t offset = 0U;
	int ret;

	while (offset < datalen) {
		ret = zsock_send(client->transport.tcp.sock, data + offset,
				 datalen - offset, 0);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
	}

	return 0;
}

int mqtt_client_tcp_write_msg(struct mqtt_client *client,
			      const struct msghdr *message)

{
	int ret, i;
	size_t offset = 0;
	size_t total_len = 0;

	for (i = 0; i < message->msg_iovlen; i++) {
		total_len += message->msg_iov[i].iov_len;
	}

	while (offset < total_len) {
		ret = zsock_sendmsg(client->transport.tcp.sock, message, 0);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
		if (offset >= total_len) {
			break;
		}

		/* Update msghdr for the next iteration. */
		for (i = 0; i < message->msg_iovlen; i++) {
			if (ret < message->msg_iov[i].iov_len) {
				message->msg_iov[i].iov_len -= ret;
				message->msg_iov[i].iov_base =
					(uint8_t *)message->msg_iov[i].iov_base + ret;
				break;
			}

			ret -= message->msg_iov[i].iov_len;
			message->msg_iov[i].iov_len = 0;
		}
	}

	return 0;
}

int mqtt_client_tcp_read(struct mqtt_client *client, uint8_t *data, uint32_t buflen,
			 bool shall_block)
{
	int flags = 0;
	int ret;

	if (!shall_block) {
		flags |= ZSOCK_MSG_DONTWAIT;
	}

	ret = zsock_recv(client->transport.tcp.sock, data, buflen, flags);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

int mqtt_client_tcp_disconnect(struct mqtt_client *client)
{
	int ret;

	NET_INFO("Closing socket %d", client->transport.tcp.sock);

	ret = zsock_close(client->transport.tcp.sock);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}
