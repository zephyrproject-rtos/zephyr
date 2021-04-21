/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport_socket_tcp.h
 *
 * @brief Internal functions to handle transport over TCP socket.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_sock_tcp, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <net/socket.h>
#include <net/mqtt.h>

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
			return -errno;
		}
	}
#endif

	MQTT_TRC("Created socket %d", client->transport.tcp.sock);

	size_t peer_addr_size = sizeof(struct sockaddr_in6);

	if (broker->sa_family == AF_INET) {
		peer_addr_size = sizeof(struct sockaddr_in);
	}

	ret = zsock_connect(client->transport.tcp.sock, client->broker,
			    peer_addr_size);
	if (ret < 0) {
		(void) zsock_close(client->transport.tcp.sock);
		return -errno;
	}

	MQTT_TRC("Connect completed");
	return 0;
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
	int ret;

	ret = zsock_sendmsg(client->transport.tcp.sock, message, 0);
	if (ret < 0) {
		return -errno;
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

	MQTT_TRC("Closing socket %d", client->transport.tcp.sock);

	ret = zsock_close(client->transport.tcp.sock);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}
