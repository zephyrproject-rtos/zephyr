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

	client->transport.tcp.sock = socket(broker->sa_family, SOCK_STREAM,
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

	ret = connect(client->transport.tcp.sock, client->broker,
		      peer_addr_size);
	if (ret < 0) {
		(void)close(client->transport.tcp.sock);
		return -errno;
	}

	MQTT_TRC("Connect completed");
	return 0;
}

int mqtt_client_tcp_write(struct mqtt_client *client, const u8_t *data,
			  u32_t datalen)
{
	u32_t offset = 0U;
	int ret;

	while (offset < datalen) {
		ret = send(client->transport.tcp.sock, data + offset,
			   datalen - offset, 0);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
	}

	return 0;
}

int mqtt_client_tcp_read(struct mqtt_client *client, u8_t *data, u32_t buflen,
			 bool shall_block)
{
	int flags = 0;
	int ret;

	if (!shall_block) {
		flags |= MSG_DONTWAIT;
	}

	ret = recv(client->transport.tcp.sock, data, buflen, flags);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

int mqtt_client_tcp_disconnect(struct mqtt_client *client)
{
	int ret;

	MQTT_TRC("Closing socket %d", client->transport.tcp.sock);

	ret = close(client->transport.tcp.sock);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}
