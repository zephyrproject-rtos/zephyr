/*
 * Copyright (c) 2026 Ellenby Technologies Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief CoAP over TCP Client Sample
 *
 * This sample demonstrates the CoAP over TCP client API (RFC 8323).
 * It connects to an external CoAP TCP server and exercises:
 * - CSM (Capabilities and Settings Message) exchange
 * - Sending GET requests over TCP
 * - Using Ping/Pong signals for connectivity checks
 * - Graceful session release
 * - Event callback for server signals (Release, Abort)
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client_tcp.h>
#include <zephyr/logging/log.h>
#include <arpa/inet.h>

LOG_MODULE_REGISTER(coap_tcp_sample, LOG_LEVEL_DBG);

#define PEER_PORT 5683
#define RESOURCE_PATH "test"

static struct coap_client_tcp client;
static K_SEM_DEFINE(response_sem, 0, 1);
static K_SEM_DEFINE(csm_sem, 0, 1);
static K_SEM_DEFINE(pong_sem, 0, 1);
static bool response_received;

static void event_callback(struct coap_client_tcp *cli,
			   enum coap_client_tcp_event event,
			   const union coap_client_tcp_event_data *data,
			   void *user_data)
{
	switch (event) {
	case COAP_CLIENT_TCP_EVENT_CSM_UPDATED:
		LOG_INF("CSM capabilities updated");
		k_sem_give(&csm_sem);
		break;
	case COAP_CLIENT_TCP_EVENT_PONG_RECEIVED:
		LOG_INF("Pong received - connection is alive");
		k_sem_give(&pong_sem);
		break;
	case COAP_CLIENT_TCP_EVENT_RELEASE:
		LOG_WRN("Server requested session release");
		break;
	case COAP_CLIENT_TCP_EVENT_ABORT:
		LOG_ERR("Connection aborted by server");
		break;
	}
}

static void response_callback(const struct coap_client_response_data *data,
			      void *user_data)
{
	if (data->result_code < 0) {
		LOG_ERR("Request failed with error: %d", data->result_code);
		response_received = true;
		k_sem_give(&response_sem);
		return;
	}

	LOG_INF("Response received: %d.%02d",
		(data->result_code >> 5), (data->result_code & 0x1F));

	if (data->payload != NULL && data->payload_len > 0) {
		LOG_INF("Payload (%zu bytes): %.*s",
			data->payload_len,
			(int)data->payload_len, data->payload);
	}

	if (data->last_block) {
		LOG_INF("Transfer complete");
		response_received = true;
		k_sem_give(&response_sem);
	}
}

static int send_get_request(const char *path)
{
	struct coap_client_tcp_request req = {0};
	int ret;

	LOG_INF("Sending GET /%s", path);

	req.method = COAP_METHOD_GET;
	strncpy(req.path, path, sizeof(req.path) - 1);
	req.cb = response_callback;

	response_received = false;

	ret = coap_client_tcp_req(&client, &req);
	if (ret < 0) {
		LOG_ERR("Failed to send request: %d", ret);
		return ret;
	}

	ret = k_sem_take(&response_sem, K_SECONDS(30));
	if (ret < 0) {
		LOG_ERR("Timeout waiting for response");
		return -ETIMEDOUT;
	}

	return 0;
}

int main(void)
{
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
	};
	int ret;

	LOG_INF("CoAP over TCP Client Sample");
	LOG_INF("============================");

	inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
		  &server_addr.sin_addr);

	/* Initialize CoAP TCP client */
	ret = coap_client_tcp_init(&client, "coap_tcp");
	if (ret < 0) {
		LOG_ERR("Failed to init client: %d", ret);
		return ret;
	}

	coap_client_tcp_set_event_cb(&client, event_callback, NULL);

	LOG_INF("Connecting to %s:%d...", CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
		PEER_PORT);

	ret = coap_client_tcp_connect(&client,
				      (struct sockaddr *)&server_addr,
				      sizeof(server_addr),
				      IPPROTO_TCP);
	if (ret < 0) {
		LOG_ERR("Failed to connect: %d", ret);
		return ret;
	}

	LOG_INF("Connected");

	/* Wait for CSM exchange to complete via event callback */
	ret = k_sem_take(&csm_sem, K_SECONDS(5));
	if (ret < 0) {
		LOG_ERR("Timeout waiting for CSM exchange");
		return ret;
	}

	/* Test Ping/Pong */
	LOG_INF("Sending Ping...");
	ret = coap_client_tcp_ping(&client);
	if (ret < 0) {
		LOG_WRN("Ping failed: %d", ret);
	}

	ret = k_sem_take(&pong_sem, K_SECONDS(5));
	if (ret < 0) {
		LOG_WRN("Timeout waiting for Pong");
	}

	/* Send GET request */
	ret = send_get_request(RESOURCE_PATH);
	if (ret < 0) {
		LOG_ERR("GET failed: %d", ret);
	}

	/* Graceful release */
	LOG_INF("Sending Release...");
	ret = coap_client_tcp_release(&client, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Release failed: %d", ret);
	}

	ret = coap_client_tcp_close(&client);
	if (ret < 0) {
		LOG_ERR("Close failed: %d", ret);
	}

	LOG_INF("Sample complete");

	return 0;
}
