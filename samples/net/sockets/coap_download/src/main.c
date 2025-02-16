/*
 * Copyright (c) 2024 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/coap_client.h>
#include <inttypes.h>
#include <errno.h>

LOG_MODULE_REGISTER(coap_download, LOG_LEVEL_INF);

static K_SEM_DEFINE(net_connected_sem, 0, 1);
static K_SEM_DEFINE(coap_done_sem, 0, 1);
static int64_t start_time;

/* This struct contains (potentially large) TX and RX buffers, so allocate statically */
static struct coap_client client = {0};

static void net_event_handler(uint32_t mgmt_event, struct net_if *iface, void *info,
			      size_t info_length, void *user_data)
{
	if (NET_EVENT_L4_CONNECTED == mgmt_event) {
		k_sem_give(&net_connected_sem);
	}
}

NET_MGMT_REGISTER_EVENT_HANDLER(l4_conn_handler, NET_EVENT_L4_CONNECTED, net_event_handler, NULL);

static void on_coap_response(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
			     bool last_block, void *user_data)
{
	LOG_INF("CoAP response, result_code=%d, offset=%u, len=%u", result_code, offset, len);

	if ((COAP_RESPONSE_CODE_CONTENT == result_code) && last_block) {
		int64_t elapsed_time = k_uptime_delta(&start_time);
		size_t total_size = offset + len;

		LOG_INF("CoAP download done, got %u bytes in %" PRId64 " ms", total_size,
			elapsed_time);
		k_sem_give(&coap_done_sem);
	} else if (COAP_RESPONSE_CODE_CONTENT != result_code) {
		LOG_ERR("Error during CoAP download, result_code=%d", result_code);
		k_sem_give(&coap_done_sem);
	}
}

static void do_coap_download(struct sockaddr *sa)
{
	int ret;
	int sockfd;
	struct coap_client_request request = {.method = COAP_METHOD_GET,
					      .confirmable = true,
					      .path = CONFIG_NET_SAMPLE_COAP_RESOURCE_PATH,
					      .payload = NULL,
					      .len = 0,
					      .cb = on_coap_response,
					      .options = NULL,
					      .num_options = 0,
					      .user_data = NULL};

	LOG_INF("Starting CoAP download using %s", (AF_INET == sa->sa_family) ? "IPv4" : "IPv6");

	sockfd = zsock_socket(sa->sa_family, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		return;
	}

	start_time = k_uptime_get();

	ret = coap_client_req(&client, sockfd, sa, &request, NULL);
	if (ret) {
		LOG_ERR("Failed to send CoAP request, err %d", ret);
		return;
	}

	/* Wait for CoAP request to complete */
	k_sem_take(&coap_done_sem, K_FOREVER);

	coap_client_cancel_requests(&client);

	zsock_close(sockfd);
}

int main(void)
{
	int ret;

	ret = coap_client_init(&client, NULL);
	if (ret) {
		LOG_ERR("Failed to init coap client, err %d", ret);
		return ret;
	}

	k_sem_take(&net_connected_sem, K_FOREVER);
	LOG_INF("Network L4 is connected");

	struct sockaddr sa;

#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in *addr4 = (struct sockaddr_in *)&sa;

	addr4->sin_family = AF_INET;
	addr4->sin_port = htons(CONFIG_NET_SAMPLE_COAP_SERVER_PORT);
	zsock_inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &addr4->sin_addr);

	do_coap_download(&sa);
#endif

#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&sa;

	addr6->sin6_family = AF_INET6;
	addr6->sin6_port = htons(CONFIG_NET_SAMPLE_COAP_SERVER_PORT);
	zsock_inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, &addr6->sin6_addr);

	do_coap_download(&sa);
#endif

	return 0;
}
