/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport_socket_tls.h
 *
 * @brief Internal functions to handle transport over TLS socket.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_sock_tls, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <net/socket.h>
#include <net/mqtt.h>

#include "mqtt_os.h"

int mqtt_client_tls_connect(struct mqtt_client *client)
{
	const struct sockaddr *broker = client->broker;
	struct mqtt_sec_config *tls_config = &client->transport.tls.config;
	int ret;

	client->transport.tls.sock = socket(broker->sa_family,
					    SOCK_STREAM, IPPROTO_TLS_1_2);
	if (client->transport.tls.sock < 0) {
		return -errno;
	}

	MQTT_TRC("Created socket %d", client->transport.tls.sock);

#if defined(CONFIG_SOCKS)
	if (client->transport.proxy.addrlen != 0) {
		ret = setsockopt(client->transport.tls.sock,
				 SOL_SOCKET, SO_SOCKS5,
				 &client->transport.proxy.addr,
				 client->transport.proxy.addrlen);
		if (ret < 0) {
			return -errno;
		}
	}
#endif
	/* Set secure socket options. */
	ret = setsockopt(client->transport.tls.sock, SOL_TLS, TLS_PEER_VERIFY,
			 &tls_config->peer_verify,
			 sizeof(tls_config->peer_verify));
	if (ret < 0) {
		goto error;
	}

	if (tls_config->cipher_list != NULL && tls_config->cipher_count > 0) {
		ret = setsockopt(client->transport.tls.sock, SOL_TLS,
				 TLS_CIPHERSUITE_LIST, tls_config->cipher_list,
				 sizeof(int) * tls_config->cipher_count);
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->sec_tag_list != NULL && tls_config->sec_tag_count > 0) {
		ret = setsockopt(client->transport.tls.sock, SOL_TLS,
				 TLS_SEC_TAG_LIST, tls_config->sec_tag_list,
				 sizeof(sec_tag_t) * tls_config->sec_tag_count);
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->hostname) {
		ret = setsockopt(client->transport.tls.sock, SOL_TLS,
				 TLS_HOSTNAME, tls_config->hostname,
				 strlen(tls_config->hostname));
		if (ret < 0) {
			goto error;
		}
	}

	size_t peer_addr_size = sizeof(struct sockaddr_in6);

	if (broker->sa_family == AF_INET) {
		peer_addr_size = sizeof(struct sockaddr_in);
	}

	ret = connect(client->transport.tls.sock, client->broker,
		      peer_addr_size);
	if (ret < 0) {
		goto error;
	}

	MQTT_TRC("Connect completed");
	return 0;

error:
	(void)close(client->transport.tls.sock);
	return -errno;
}

int mqtt_client_tls_write(struct mqtt_client *client, const u8_t *data,
			  u32_t datalen)
{
	u32_t offset = 0U;
	int ret;

	while (offset < datalen) {
		ret = send(client->transport.tls.sock, data + offset,
			   datalen - offset, 0);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
	}

	return 0;
}

int mqtt_client_tls_write_msg(struct mqtt_client *client,
			      const struct msghdr *message)
{
	int ret;

	ret = sendmsg(client->transport.tls.sock, message, 0);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}

int mqtt_client_tls_read(struct mqtt_client *client, u8_t *data, u32_t buflen,
			 bool shall_block)
{
	int flags = 0;
	int ret;

	if (!shall_block) {
		flags |= MSG_DONTWAIT;
	}

	ret = recv(client->transport.tls.sock, data, buflen, flags);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

int mqtt_client_tls_disconnect(struct mqtt_client *client)
{
	int ret;

	MQTT_TRC("Closing socket %d", client->transport.tls.sock);
	ret = close(client->transport.tls.sock);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}
