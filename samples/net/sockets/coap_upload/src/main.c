/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/coap_client.h>
#include <inttypes.h>
#include <errno.h>

#include "net_sample_common.h"

LOG_MODULE_REGISTER(coap_upload, LOG_LEVEL_INF);

#define SHORT_PAYLOAD "sample_payload"

static K_SEM_DEFINE(coap_done_sem, 0, 1);
static int64_t start_time;

static void response_cb(const struct coap_client_response_data *data,
			void *user_data)
{

	if (data->result_code < 0) {
		goto error;
	}

	if (data->last_block) {
		int64_t elapsed_time = k_uptime_get() - start_time;

		if (data->result_code != COAP_RESPONSE_CODE_CHANGED) {
			goto error;
		}

		LOG_INF("CoAP upload %s done in %" PRId64 " ms",
			(char *)user_data, elapsed_time);

		k_sem_give(&coap_done_sem);
	} else {
		if (data->result_code != COAP_RESPONSE_CODE_CONTINUE) {
			goto error;
		}

		LOG_INF("CoAP upload %s ongoing, sent %zu bytes so far",
			(char *)user_data, data->offset);
	}

	return;

error:
	LOG_ERR("Error during CoAP upload, result_code=%d", data->result_code);
	k_sem_give(&coap_done_sem);
}

static int short_payload_cb(size_t offset, const uint8_t **payload, size_t *len,
			    bool *last_block, void *user_data)
{
	if (*len < sizeof(SHORT_PAYLOAD) - 1) {
		return -ENOMEM;
	}

	*payload = SHORT_PAYLOAD;
	*len = sizeof(SHORT_PAYLOAD) - 1;
	*last_block = true;

	LOG_DBG("CoAP short payload callback, returning %zu bytes at offset %zu",
		*len, offset);

	return 0;
}

static int block_payload_cb(size_t offset, const uint8_t **payload, size_t *len,
			    bool *last_block, void *user_data)
{
	size_t data_left;

	if (offset > LOREM_IPSUM_SHORT_STRLEN) {
		return -EINVAL;
	}

	*payload = LOREM_IPSUM_SHORT + offset;

	data_left = LOREM_IPSUM_SHORT_STRLEN - offset;
	if (data_left <= *len) {
		*len = data_left;
		*last_block = true;
	} else {
		*last_block = false;
	}

	LOG_DBG("CoAP blockwise payload callback, returning %zu bytes at offset %zu",
		*len, offset);

	return 0;
}

static int coap_upload_single(struct coap_client *client, int sock,
			      struct coap_client_request *request)
{
	int ret;

	LOG_INF("");
	LOG_INF("** CoAP upload %s", (char *)request->user_data);

	start_time = k_uptime_get();

	ret = coap_client_req(client, sock, NULL, request, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request, err %d", ret);
		return ret;
	}

	/* Wait for CoAP request to complete */
	k_sem_take(&coap_done_sem, K_FOREVER);

	return ret;
}

static void coap_upload(struct coap_client *client, struct sockaddr *sa,
			socklen_t addrlen)
{
	struct coap_client_request requests[] = {
		{
			.method = COAP_METHOD_PUT,
			.confirmable = true,
			.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
			.path = "test",
			.payload = SHORT_PAYLOAD,
			.len = sizeof(SHORT_PAYLOAD) - 1,
			.cb = response_cb,
			.user_data = "short",
		},
		{
			.method = COAP_METHOD_PUT,
			.confirmable = true,
			.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
			.path = "test",
			.cb = response_cb,
			.payload_cb = short_payload_cb,
			.user_data = "short with callback",
		},
		{
			.method = COAP_METHOD_PUT,
			.confirmable = true,
			.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
			.path = "large-update",
			.payload = LOREM_IPSUM_SHORT,
			.len = LOREM_IPSUM_SHORT_STRLEN,
			.cb = response_cb,
			.user_data = "blockwise",
		},
		{
			.method = COAP_METHOD_PUT,
			.confirmable = true,
			.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
			.path = "large-update",
			.cb = response_cb,
			.payload_cb = block_payload_cb,
			.user_data = "blockwise with callback",
		}
	};
	int sock;
	int ret;

	LOG_INF("");
	LOG_INF("* Starting CoAP upload using %s", (AF_INET == sa->sa_family) ? "IPv4" : "IPv6");

	sock = zsock_socket(sa->sa_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		return;
	}

	ret = zsock_connect(sock, sa, addrlen);
	if (ret < 0) {
		LOG_ERR("Failed to connect socket, err %d", errno);
		goto out;

	}

	ARRAY_FOR_EACH(requests, i) {
		ret = coap_upload_single(client, sock, &requests[i]);
		if (ret < 0) {
			LOG_ERR("CoAP upload %s failed, err %d",
				(char *)requests[i].user_data, ret);
			goto out;
		}
	}

out:
	coap_client_cancel_requests(client);

	zsock_close(sock);
}

int main(void)
{
	static struct coap_client client;
	int ret;

	wait_for_network();

	ret = coap_client_init(&client, NULL);
	if (ret) {
		LOG_ERR("Failed to init CoAP client, err %d", ret);
		return ret;
	}

	struct sockaddr sa;

#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in *addr4 = (struct sockaddr_in *)&sa;

	addr4->sin_family = AF_INET;
	addr4->sin_port = htons(CONFIG_NET_SAMPLE_COAP_SERVER_PORT);
	zsock_inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &addr4->sin_addr);

	coap_upload(&client, &sa, sizeof(struct sockaddr_in));
#endif

#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&sa;

	addr6->sin6_family = AF_INET6;
	addr6->sin6_port = htons(CONFIG_NET_SAMPLE_COAP_SERVER_PORT);
	zsock_inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, &addr6->sin6_addr);

	coap_upload(&client, &sa, sizeof(struct sockaddr_in6));
#endif

	return 0;
}
