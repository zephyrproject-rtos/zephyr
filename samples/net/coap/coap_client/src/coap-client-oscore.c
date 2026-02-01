/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_coap_client_oscore_sample, LOG_LEVEL_DBG);

#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>

#if defined(CONFIG_COAP_OSCORE)
#include <oscore.h>
#include <oscore/security_context.h>
#include <psa/crypto.h>
#endif

#include "net_private.h"

#define PEER_PORT 5683
#define MAX_COAP_MSG_LEN 256

#if defined(CONFIG_COAP_OSCORE)
/* OSCORE context - matches libcoap configuration */
static struct context oscore_ctx;

/* Master secret (shared with server) */
static const uint8_t master_secret[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
};

/* Master salt (optional, shared with server) */
static const uint8_t master_salt[] = {
	0x9e, 0x7c, 0xa9, 0x22, 0x23, 0x78, 0x63, 0x40
};

/* Sender ID (this device is the client) */
static const uint8_t sender_id[] = "client";

/* Recipient ID (remote peer is the server) */
static const uint8_t recipient_id[] = "server";
#endif

/* CoAP client instance */
static struct coap_client client;
static struct sockaddr_in6 server_addr;

/* Response handlers */
static void test_response_cb(int16_t code, size_t offset, const uint8_t *payload,
			     size_t len, bool last_block, void *user_data)
{
	LOG_INF("Response received: code=%d, len=%zu, last_block=%d",
		code, len, last_block);

	if (payload && len > 0) {
		LOG_HEXDUMP_INF(payload, len, "Payload:");
	}
}

static int setup_oscore(void)
{
#if defined(CONFIG_COAP_OSCORE)
	psa_status_t psa_status;
	
	/* Initialize PSA Crypto */
	LOG_DBG("Initializing PSA Crypto");
	psa_status = psa_crypto_init();
	if (psa_status != PSA_SUCCESS) {
		LOG_ERR("PSA Crypto initialization failed: %d", psa_status);
		return -ENODEV;
	}
	LOG_DBG("PSA Crypto initialized");

	struct oscore_init_params params = {
		.master_secret.ptr = (uint8_t *)master_secret,
		.master_secret.len = sizeof(master_secret),
		.sender_id.ptr = (uint8_t *)sender_id,
		.sender_id.len = sizeof(sender_id) - 1, /* Exclude null terminator */
		.recipient_id.ptr = (uint8_t *)recipient_id,
		.recipient_id.len = sizeof(recipient_id) - 1, /* Exclude null terminator */
		.master_salt.ptr = (uint8_t *)master_salt,
		.master_salt.len = sizeof(master_salt),
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
		.fresh_master_secret_salt = false,
	};

	LOG_DBG("Initializing OSCORE context");
	LOG_DBG("  master_secret: %p, len: %d", (void *)params.master_secret.ptr, params.master_secret.len);
	LOG_DBG("  sender_id: %p, len: %d", (void *)params.sender_id.ptr, params.sender_id.len);
	LOG_DBG("  recipient_id: %p, len: %d", (void *)params.recipient_id.ptr, params.recipient_id.len);
	LOG_DBG("  master_salt: %p, len: %d", (void *)params.master_salt.ptr, params.master_salt.len);

	int ret = oscore_context_init(&params, &oscore_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to initialize OSCORE context: %d (%s)", ret, 
			ret == -ENOMEM ? "Out of memory" :
			ret == -EINVAL ? "Invalid parameter" :
			ret == -ENOTSUP ? "Not supported" :
			ret == -ENOENT ? "Not found" :
			ret == -ENOSPC ? "No space (check heap/PSA key slots)" :
			"Unknown error");
		return ret;
	}

	LOG_INF("OSCORE enabled (client/server, AES-CCM-16-64-128)");
	LOG_DBG("  Sender ID: %s", sender_id);
	LOG_DBG("  Recipient ID: %s", recipient_id);
	LOG_DBG("  Master secret: %d bytes, Master salt: %d bytes", 
		sizeof(master_secret), sizeof(master_salt));
#endif /* CONFIG_COAP_OSCORE */
	return 0;
}

static int init_coap_client(void)
{
	int ret;

	/* Setup server address */
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(PEER_PORT);
	server_addr.sin6_scope_id = 0U;

	ret = inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			&server_addr.sin6_addr);
	if (ret < 0) {
		LOG_ERR("Failed to parse server address: %d", errno);
		return -errno;
	}

	/* Initialize CoAP client */
	ret = coap_client_init(&client, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CoAP client: %d", ret);
		return ret;
	}

#if defined(CONFIG_COAP_OSCORE)
	/* Attach OSCORE context to client */
	client.oscore_ctx = &oscore_ctx;
	LOG_INF("OSCORE context attached to CoAP client");
#endif

	LOG_INF("CoAP client initialized");
	return 0;
}

static int send_get_request(const char *path)
{
	struct coap_client_request request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = test_response_cb,
		.user_data = NULL,
	};

	LOG_INF("Sending GET request to: %s", path);

	int ret = coap_client_req(&client, 0, (struct sockaddr *)&server_addr,
				  &request, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request: %d", ret);
		return ret;
	}

	return 0;
}

static int send_post_request(const char *path, const uint8_t *payload, size_t len)
{
	struct coap_client_request request = {
		.method = COAP_METHOD_POST,
		.confirmable = true,
		.path = path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.payload = payload,
		.len = len,
		.cb = test_response_cb,
		.user_data = NULL,
	};

	LOG_INF("Sending POST request to: %s", path);

	int ret = coap_client_req(&client, 0, (struct sockaddr *)&server_addr,
				  &request, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request: %d", ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	int ret;
	const uint8_t test_payload[] = "OSCORE test payload";

	LOG_INF("Starting CoAP client with OSCORE support");

	/* Wait for network to be ready */
	k_sleep(K_SECONDS(2));

	ret = setup_oscore();
	if (ret < 0) {
		LOG_ERR("Failed to setup OSCORE: %d", ret);
		return ret;
	}

	ret = init_coap_client();
	if (ret < 0) {
		LOG_ERR("Failed to initialize CoAP client: %d", ret);
		return ret;
	}

	/* Test GET request */
	printk("\n=== CoAP GET /test ===\n");
	ret = send_get_request("test");
	if (ret < 0) {
		goto cleanup;
	}

	k_sleep(K_SECONDS(2));

	/* Test POST request */
	printk("\n=== CoAP POST /test ===\n");
	ret = send_post_request("test", test_payload, sizeof(test_payload) - 1);
	if (ret < 0) {
		goto cleanup;
	}

	k_sleep(K_SECONDS(2));

	LOG_INF("CoAP client tests completed");

cleanup:
	/* Cleanup */
	return ret;
}
