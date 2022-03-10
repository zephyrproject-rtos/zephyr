/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_transport_socket_tls.h
 *
 * @brief Internal functions to handle transport over TLS socket.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_sock_tls, CONFIG_MQTT_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include "mqtt_os.h"

int mqtt_client_tls_connect(struct mqtt_client *client)
{
	const struct sockaddr *broker = client->broker;
	struct mqtt_sec_config *tls_config = &client->transport.tls.config;
	int type = SOCK_STREAM;
	int ret;

	if (tls_config->set_native_tls) {
		type |= SOCK_NATIVE_TLS;
	}

	client->transport.tls.sock = zsock_socket(broker->sa_family,
						  type, IPPROTO_TLS_1_2);
	if (client->transport.tls.sock < 0) {
		return -errno;
	}

	NET_DBG("Created socket %d", client->transport.tls.sock);

#if defined(CONFIG_SOCKS)
	if (client->transport.proxy.addrlen != 0) {
		ret = setsockopt(client->transport.tls.sock,
				 SOL_SOCKET, SO_SOCKS5,
				 &client->transport.proxy.addr,
				 client->transport.proxy.addrlen);
		if (ret < 0) {
			goto error;
		}
	}
#endif
	/* Set secure socket options. */
	ret = zsock_setsockopt(client->transport.tls.sock, SOL_TLS, TLS_PEER_VERIFY,
			       &tls_config->peer_verify,
			       sizeof(tls_config->peer_verify));
	if (ret < 0) {
		goto error;
	}

	if (tls_config->cipher_list != NULL && tls_config->cipher_count > 0) {
		ret = zsock_setsockopt(client->transport.tls.sock, SOL_TLS,
				       TLS_CIPHERSUITE_LIST, tls_config->cipher_list,
				       sizeof(int) * tls_config->cipher_count);
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->sec_tag_list != NULL && tls_config->sec_tag_count > 0) {
		ret = zsock_setsockopt(client->transport.tls.sock, SOL_TLS,
				       TLS_SEC_TAG_LIST, tls_config->sec_tag_list,
				       sizeof(sec_tag_t) * tls_config->sec_tag_count);
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->hostname) {
		ret = zsock_setsockopt(client->transport.tls.sock, SOL_TLS,
				       TLS_HOSTNAME, tls_config->hostname,
				       strlen(tls_config->hostname));
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->session_cache == TLS_SESSION_CACHE_ENABLED) {
		ret = zsock_setsockopt(client->transport.tls.sock, SOL_TLS,
				       TLS_SESSION_CACHE,
				       &tls_config->session_cache,
				       sizeof(tls_config->session_cache));
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->cert_nocopy != TLS_CERT_NOCOPY_NONE) {
		ret = zsock_setsockopt(client->transport.tls.sock, SOL_TLS,
				       TLS_CERT_NOCOPY, &tls_config->cert_nocopy,
				       sizeof(tls_config->cert_nocopy));
		if (ret < 0) {
			goto error;
		}
	}

	size_t peer_addr_size = sizeof(struct sockaddr_in6);

	if (broker->sa_family == AF_INET) {
		peer_addr_size = sizeof(struct sockaddr_in);
	}

	ret = zsock_connect(client->transport.tls.sock, client->broker,
			    peer_addr_size);
	if (ret < 0) {
		goto error;
	}

	NET_DBG("Connect completed");
	return 0;

error:
	(void) zsock_close(client->transport.tls.sock);
	return -errno;
}

int mqtt_client_tls_write(struct mqtt_client *client, const uint8_t *data,
			  uint32_t datalen)
{
	uint32_t offset = 0U;
	int ret;

	while (offset < datalen) {
		ret = zsock_send(client->transport.tls.sock, data + offset,
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
	int ret, i;
	size_t offset = 0;
	size_t total_len = 0;

	for (i = 0; i < message->msg_iovlen; i++) {
		total_len += message->msg_iov[i].iov_len;
	}

	while (offset < total_len) {
		ret = zsock_sendmsg(client->transport.tls.sock, message, 0);
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

int mqtt_client_tls_read(struct mqtt_client *client, uint8_t *data, uint32_t buflen,
			 bool shall_block)
{
	int flags = 0;
	int ret;

	if (!shall_block) {
		flags |= ZSOCK_MSG_DONTWAIT;
	}

	ret = zsock_recv(client->transport.tls.sock, data, buflen, flags);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

int mqtt_client_tls_disconnect(struct mqtt_client *client)
{
	int ret;

	NET_INFO("Closing socket %d", client->transport.tls.sock);
	ret = zsock_close(client->transport.tls.sock);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}
