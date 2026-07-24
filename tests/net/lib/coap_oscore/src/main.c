/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_DBG);

#include <errno.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/net/socket.h>

#include <zephyr/ztest.h>

#include "net_private.h"

#include "coap_oscore_internal.h"

#define COAP_BUF_SIZE      128
#define SHORT_TIMEOUT      500
#define LONG_TIMEOUT       2000
#define COAP_OSCORE_OPTION 9

#if defined(CONFIG_COAP_OSCORE)
/*
 * Shared OSCORE test infrastructure.
 *
 * The key material below is shared by every end-to-end test; each test derives a
 * fresh client and server context from it so sender sequence numbers start from
 * zero. A second, non-OSCORE-required ("mixed") service exercises the code paths
 * that differ between required and mixed services.
 */

static const uint8_t oscore_master_secret[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
};
static const uint8_t oscore_master_salt[] = {
	0x9E, 0x7C, 0xA9, 0x22, 0x23, 0x78, 0x63, 0x40,
};
static const uint8_t oscore_client_id[] = {0x01};
static const uint8_t oscore_server_id[] = {0x02};

static const uint8_t oscore_notify_payload[] = "notify-payload";
static const uint8_t oscore_deferred_payload[] = "deferred-response";
static const uint8_t oscore_mixed_payload[] = "mixed-response";
static const uint8_t oscore_secure_payload[] = "encrypted-response";

static int oscore_make_ctx(const uint8_t *sender_id, size_t sender_id_len,
			   const uint8_t *recipient_id, size_t recipient_id_len,
			   struct coap_oscore_context **ctx)
{
	struct coap_oscore_init_params params = {
		.master_secret = oscore_master_secret,
		.master_secret_len = sizeof(oscore_master_secret),
		.sender_id = sender_id,
		.sender_id_len = sender_id_len,
		.recipient_id = recipient_id,
		.recipient_id_len = recipient_id_len,
		.master_salt = oscore_master_salt,
		.master_salt_len = sizeof(oscore_master_salt),
		.aead_alg = COAP_OSCORE_AEAD_AES_CCM_16_64_128,
		.hkdf = COAP_OSCORE_HKDF_SHA_256,
		.fresh_master_secret_salt = true,
	};

	return coap_oscore_context_init(&params, ctx);
}

static net_socklen_t oscore_addr_len(const struct net_sockaddr *addr)
{
	return addr->sa_family == NET_AF_INET ? sizeof(struct net_sockaddr_in)
					      : sizeof(struct net_sockaddr_in6);
}

static struct net_sockaddr_in6 oscore_loopback_dst(uint16_t port)
{
	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01}}},
		.sin6_port = net_htons(port),
	};

	return dst;
}

static int oscore_client_socket(void)
{
	return zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
}

/* Build a plaintext CoAP request with an optional URI-Path and Observe option. */
static int oscore_build_request(struct coap_packet *req, uint8_t *buf, size_t buf_len, uint8_t type,
				uint8_t code, const uint8_t *token, uint8_t tkl, uint16_t id,
				const char *uri, int observe)
{
	int ret;

	ret = coap_packet_init(req, buf, buf_len, COAP_VERSION_1, type, tkl, token, code, id);
	if (ret < 0) {
		return ret;
	}

	if (observe >= 0) {
		ret = coap_append_option_int(req, COAP_OPTION_OBSERVE, observe);
		if (ret < 0) {
			return ret;
		}
	}

	if (uri != NULL) {
		ret = coap_packet_append_option(req, COAP_OPTION_URI_PATH, (const uint8_t *)uri,
						strlen(uri));
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int oscore_send_protected(int sock, struct coap_oscore_context *ctx,
				 const struct net_sockaddr_in6 *dst, const struct coap_packet *req)
{
	uint8_t protected[COAP_BUF_SIZE];
	uint32_t protected_len = sizeof(protected);
	int ret;

	ret = coap_oscore_protect(req->data, req->offset, protected, &protected_len, ctx);
	if (ret < 0) {
		return ret;
	}

	ret = zsock_sendto(sock, protected, protected_len, 0, (const struct net_sockaddr *)dst,
			   sizeof(*dst));
	return ret < 0 ? -errno : 0;
}

static int oscore_send_plain(int sock, const struct net_sockaddr_in6 *dst,
			     const struct coap_packet *req)
{
	int ret = zsock_sendto(sock, req->data, req->offset, 0, (const struct net_sockaddr *)dst,
			       sizeof(*dst));

	return ret < 0 ? -errno : 0;
}

static int oscore_recv(int sock, int timeout_ms, uint8_t *buf, size_t buf_len)
{
	struct zsock_pollfd pfd = {.fd = sock, .events = ZSOCK_POLLIN};
	int ret;

	ret = zsock_poll(&pfd, 1, timeout_ms);
	if (ret <= 0) {
		return -ETIMEDOUT;
	}

	ret = zsock_recvfrom(sock, buf, buf_len, 0, NULL, NULL);
	return ret < 0 ? -errno : ret;
}

/* Receive, parse and OSCORE-verify a response, returning the decrypted inner packet. */
static int oscore_recv_verify(int sock, int timeout_ms, struct coap_oscore_context *ctx,
			      uint8_t *inner_buf, uint32_t *inner_len, struct coap_packet *inner)
{
	uint8_t recv_buf[COAP_BUF_SIZE];
	struct coap_packet outer;
	uint8_t oscore_error = 0;
	int len;
	int ret;

	len = oscore_recv(sock, timeout_ms, recv_buf, sizeof(recv_buf));
	if (len < 0) {
		return len;
	}

	ret = coap_packet_parse(&outer, recv_buf, len, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	if (!coap_oscore_msg_has_oscore(&outer)) {
		return -ENOMSG;
	}

	ret = coap_oscore_verify(recv_buf, outer.offset, inner_buf, inner_len, ctx, &oscore_error);
	if (ret < 0) {
		return ret;
	}

	return coap_packet_parse(inner, inner_buf, *inner_len, NULL, 0);
}

/* Secure (OSCORE-required) service*/
static uint16_t oscore_secure_service_port = 56839;
static struct coap_oscore_context *oscore_secure_service_ctx;

/* Provider invoked by the CoAP server when OSCORE processing is needed. */
static struct coap_oscore_context *oscore_secure_service_provider(void)
{
	return oscore_secure_service_ctx;
}

static int oscore_secure_get(struct coap_resource *resource, struct coap_packet *request,
			     struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet response;
	uint8_t tkl;
	int ret;

	ARG_UNUSED(resource);

	tkl = coap_header_get_token(request, token);

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_ACK, tkl,
			       token, COAP_RESPONSE_CODE_CONTENT, coap_header_get_id(request));
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload(&response, oscore_secure_payload,
					 sizeof(oscore_secure_payload) - 1);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_resource_send(resource, &response, addr, addr_len, NULL);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	return 0;
}

/* Mixed (non-OSCORE-required) service. */
static uint16_t oscore_mixed_service_port = 56840;
static struct coap_oscore_context *oscore_mixed_service_ctx;

static struct coap_oscore_context *oscore_mixed_service_provider(void)
{
	return oscore_mixed_service_ctx;
}

/* State captured by the separate-response handler for a later (deferred) reply. */
struct oscore_saved_request {
	struct net_sockaddr_storage addr;
	net_socklen_t addr_len;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;
	uint16_t id;
	bool valid;
};

static struct oscore_saved_request oscore_deferred;

/* Observe: register/deregister and answer with an initial notification. */
static int oscore_observe_get(struct coap_resource *resource, struct coap_packet *request,
			      struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet response;
	uint8_t tkl;
	int observe;
	int ret;

	observe = coap_get_option_int(request, COAP_OPTION_OBSERVE);
	if (observe == 0 || observe == 1) {
		(void)coap_resource_parse_observe(resource, request, addr);
	}

	tkl = coap_header_get_token(request, token);

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_ACK, tkl,
			       token, COAP_RESPONSE_CODE_CONTENT, coap_header_get_id(request));
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	if (observe == 0) {
		ret = coap_append_option_int(&response, COAP_OPTION_OBSERVE, resource->age);
		if (ret < 0) {
			return COAP_RESPONSE_CODE_INTERNAL_ERROR;
		}
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload(&response, oscore_notify_payload,
					 sizeof(oscore_notify_payload) - 1);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_resource_send(resource, &response, addr, addr_len, NULL);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	return 0;
}

static void oscore_observe_notify(struct coap_resource *resource, struct coap_observer *observer)
{
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet response;
	int ret;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_NON_CON,
			       observer->tkl, observer->token, COAP_RESPONSE_CODE_CONTENT,
			       coap_next_id());
	if (ret < 0) {
		return;
	}

	ret = coap_append_option_int(&response, COAP_OPTION_OBSERVE, resource->age);
	if (ret < 0) {
		return;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return;
	}

	ret = coap_packet_append_payload(&response, oscore_notify_payload,
					 sizeof(oscore_notify_payload) - 1);
	if (ret < 0) {
		return;
	}

	(void)coap_resource_send(resource, &response, net_sad(&observer->addr),
				 oscore_addr_len(net_sad(&observer->addr)), NULL);
}

/* Separate response: acknowledge now (empty ACK), answer later. */
static int oscore_separate_get(struct coap_resource *resource, struct coap_packet *request,
			       struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet ack;
	int ret;

	oscore_deferred.tkl = coap_header_get_token(request, oscore_deferred.token);
	oscore_deferred.id = coap_header_get_id(request);
	memcpy(&oscore_deferred.addr, addr, addr_len);
	oscore_deferred.addr_len = addr_len;
	oscore_deferred.valid = true;

	/* RFC 7252 Section 5.2.2: empty ACK now, separate response later. */
	ret = coap_ack_init(&ack, request, data, sizeof(data), 0);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_resource_send(resource, &ack, addr, addr_len, NULL);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	return 0;
}

/* Send the deferred (separate) response from outside the request handler. */
static int oscore_send_deferred_response(struct coap_resource *resource)
{
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet response;
	int ret;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_NON_CON,
			       oscore_deferred.tkl, oscore_deferred.token,
			       COAP_RESPONSE_CODE_CONTENT, coap_next_id());
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_payload(&response, oscore_deferred_payload,
					 sizeof(oscore_deferred_payload) - 1);
	if (ret < 0) {
		return ret;
	}

	return coap_resource_send(resource, &response, net_sad(&oscore_deferred.addr),
				  oscore_deferred.addr_len, NULL);
}

/* Synchronous (piggybacked) response used by the mixed-service tests. */
static int oscore_mixed_get(struct coap_resource *resource, struct coap_packet *request,
			    struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet response;
	uint8_t tkl;
	int ret;

	tkl = coap_header_get_token(request, token);

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_ACK, tkl,
			       token, COAP_RESPONSE_CODE_CONTENT, coap_header_get_id(request));
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload(&response, oscore_mixed_payload,
					 sizeof(oscore_mixed_payload) - 1);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_resource_send(resource, &response, addr, addr_len, NULL);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	return 0;
}

COAP_SERVICE_DEFINE_OSCORE(oscore_secure_service, NULL, &oscore_secure_service_port, 0,
			   oscore_secure_service_provider, true);

COAP_SERVICE_DEFINE_OSCORE(oscore_mixed_service, NULL, &oscore_mixed_service_port, 0,
			   oscore_mixed_service_provider, false);

static const char *const oscore_observe_path[] = {"observe", NULL};
static const char *const oscore_separate_path[] = {"separate", NULL};
static const char *const oscore_e2e_path[] = {"e2e", NULL};
static const char *const oscore_mixed_get_path[] = {"mixed", NULL};
static const char *const oscore_mixed_observe_path[] = {"mobserve", NULL};
static const char *const oscore_separate_mixed_path[] = {"separatem", NULL};

/* clang-format off */
COAP_RESOURCE_DEFINE(oscore_e2e_resource, oscore_secure_service,
			{
				.path = oscore_e2e_path,
				.get = oscore_secure_get,
			}
);

COAP_RESOURCE_DEFINE(oscore_observe_resource, oscore_secure_service,
			{
				.path = oscore_observe_path,
				.get = oscore_observe_get,
				.notify = oscore_observe_notify,
			}
);

COAP_RESOURCE_DEFINE(oscore_separate_resource, oscore_secure_service,
			{
				.path = oscore_separate_path,
				.get = oscore_separate_get,
			}
);

COAP_RESOURCE_DEFINE(oscore_mixed_resource, oscore_mixed_service,
			{
				.path = oscore_mixed_get_path,
				.get = oscore_mixed_get,
			}
);

COAP_RESOURCE_DEFINE(oscore_mixed_observe_resource, oscore_mixed_service,
			{
				.path = oscore_mixed_observe_path,
				.get = oscore_observe_get,
				.notify = oscore_observe_notify,
			}
);

COAP_RESOURCE_DEFINE(oscore_separate_mixed_resource, oscore_mixed_service,
			{
				.path = oscore_separate_mixed_path,
				.get = oscore_separate_get,
			}
);
/* clang-format on */

/* Test OSCORE option number is correctly defined */
ZTEST(coap_oscore, test_oscore_option_number)
{
	/* RFC 8613 Section 2: OSCORE option number is 9 */
	zassert_equal(COAP_OPTION_OSCORE, 9, "OSCORE option number must be 9");
}

/* Test OSCORE malformed message validation (RFC 8613 Section 2) */
ZTEST(coap_oscore, test_oscore_malformed_validation)
{
	struct coap_packet cpkt;
	uint8_t buf[COAP_BUF_SIZE];
	int ret;

	/* RFC 8613 Section 2: OSCORE option without payload is malformed */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			       COAP_METHOD_GET, coap_next_id());
	zassert_ok(ret, "Should init packet");

	/* Add OSCORE option (empty value is valid for the option itself) */
	ret = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE, NULL, 0);
	zassert_ok(ret, "Should append OSCORE option");

	/* Validate - should fail because no payload */
	ret = coap_oscore_validate_msg(&cpkt);
	zassert_equal(ret, -EBADMSG, "Should reject OSCORE without payload, got %d", ret);

	/* Now add a payload marker and payload */
	ret = coap_packet_append_payload_marker(&cpkt);
	zassert_ok(ret, "Should append payload marker");

	const uint8_t payload[] = "test";

	ret = coap_packet_append_payload(&cpkt, payload, sizeof(payload) - 1);
	zassert_ok(ret, "Should append payload");

	/* Now validation should pass */
	ret = coap_oscore_validate_msg(&cpkt);
	zassert_ok(ret, "Should accept OSCORE with payload, got %d", ret);
}

/* Test OSCORE message detection */
ZTEST(coap_oscore, test_oscore_message_detection)
{
	struct coap_packet cpkt;
	uint8_t buf[COAP_BUF_SIZE];
	int ret;
	bool has_oscore;

	/* Create message without OSCORE option */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			       COAP_METHOD_GET, coap_next_id());
	zassert_ok(ret, "Should init packet");

	has_oscore = coap_oscore_msg_has_oscore(&cpkt);
	zassert_false(has_oscore, "Should not detect OSCORE option");

	/* Create message with OSCORE option */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			       COAP_METHOD_GET, coap_next_id());
	zassert_ok(ret, "Should init packet");

	ret = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE, NULL, 0);
	zassert_ok(ret, "Should append OSCORE option");

	has_oscore = coap_oscore_msg_has_oscore(&cpkt);
	zassert_true(has_oscore, "Should detect OSCORE option");
}

ZTEST(coap_oscore, test_oscore_exchange_cache)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr1 = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1}}},
		.sin6_port = net_htons(5683),
	};
	struct net_sockaddr_in6 addr2 = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2}}},
		.sin6_port = net_htons(5683),
	};
	uint8_t token1[] = {0x01, 0x02, 0x03, 0x04};
	uint8_t token2[] = {0x05, 0x06, 0x07, 0x08};

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Test: Add entry to cache */
	int ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&addr1, sizeof(addr1), token1,
				      sizeof(token1));
	zassert_ok(ret, "Should add exchange entry");

	/* Test: Find the entry */
	struct coap_oscore_exchange *entry;

	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr1, sizeof(addr1), token1,
				     sizeof(token1));
	zassert_not_null(entry, "Should find exchange entry");
	zassert_equal(entry->tkl, sizeof(token1), "Token length should match");
	zassert_mem_equal(entry->token, token1, sizeof(token1), "Token should match");

	/* Test: Add another entry with different address */
	ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&addr2, sizeof(addr2), token2,
				  sizeof(token2));
	zassert_ok(ret, "Should add second exchange entry");

	/* Test: Find second entry */
	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr2, sizeof(addr2), token2,
				     sizeof(token2));
	zassert_not_null(entry, "Should find second exchange entry");

	/* Test: Update existing entry */
	ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&addr1, sizeof(addr1), token1,
				  sizeof(token1));
	zassert_ok(ret, "Should update exchange entry");

	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr1, sizeof(addr1), token1,
				     sizeof(token1));
	zassert_not_null(entry, "Should still find exchange entry");

	/* Test: Remove entry */
	coap_oscore_exchange_remove(cache, (struct net_sockaddr *)&addr1, sizeof(addr1), token1,
			       sizeof(token1));

	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr1, sizeof(addr1), token1,
				     sizeof(token1));
	zassert_is_null(entry, "Should not find removed entry");

	/* Test: Second entry should still exist */
	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr2, sizeof(addr2), token2,
				     sizeof(token2));
	zassert_not_null(entry, "Second entry should still exist");
}

ZTEST(coap_oscore, test_oscore_exchange_expiry)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1}}},
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	int ret;

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Add non-Observe exchange */
	ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&addr, sizeof(addr), token,
				  sizeof(token));
	zassert_ok(ret, "Should add exchange");

	/* Entry should be found initially */
	struct coap_oscore_exchange *entry;

	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr, sizeof(addr), token,
				     sizeof(token));
	zassert_not_null(entry, "Should find fresh entry");

	/* Set timestamp to expired value */
	entry->timestamp = k_uptime_get() - CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS - 1000;

	/* Next find should detect expiry and clear the entry */
	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr, sizeof(addr), token,
				     sizeof(token));
	zassert_is_null(entry, "Expired entry should be cleared");
}

ZTEST(coap_oscore, test_oscore_exchange_cache_eviction)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr_base = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	int ret;

	zassert(CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE >= 2,
		"Cache size must be at least 2 for eviction test");
	zassert(CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE < 0xFF,
		"Cache size must be less than 0xFF for eviction test");

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Fill the cache */
	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		struct net_sockaddr_in6 addr = addr_base;

		addr.sin6_addr.s6_addr[15] = i + 1;
		token[0] = i + 1;

		ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&addr, sizeof(addr), token,
					  sizeof(token));
		zassert_ok(ret, "Should add entry %d", i);

		/* Small delay to ensure different timestamps */
		k_msleep(1);
	}

	/* Verify cache is full */
	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		struct net_sockaddr_in6 addr = addr_base;

		addr.sin6_addr.s6_addr[15] = i + 1;
		token[0] = i + 1;

		struct coap_oscore_exchange *entry;

		entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr, sizeof(addr),
					     token, sizeof(token));
		zassert_not_null(entry, "Should find entry %d", i);
	}

	/* Add one more entry - should fail because the cache is full (no eviction). */
	struct net_sockaddr_in6 new_addr = addr_base;

	new_addr.sin6_addr.s6_addr[15] = 0xFF;
	token[0] = 0xFF;

	ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&new_addr, sizeof(new_addr), token,
				  sizeof(token));
	zassert_equal(ret, -ENOMEM, "Should prevent adding new entry when cache is full");

	/* Verify new entry does not exist */
	struct coap_oscore_exchange *entry;

	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&new_addr, sizeof(new_addr),
				     token, sizeof(token));
	zassert_is_null(entry, "New entry should not exist");

	/* Verify no entry was evicted */
	struct net_sockaddr_in6 first_addr = addr_base;

	first_addr.sin6_addr.s6_addr[15] = 1;
	token[0] = 1;

	/* Verify the cache */
	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		struct net_sockaddr_in6 addr = addr_base;

		addr.sin6_addr.s6_addr[15] = i + 1;
		token[0] = i + 1;

		entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr, sizeof(addr),
					     token, sizeof(token));
		zassert_not_null(entry, "No entry should have been evicted");
	}
}

/* A secure service returns an OSCORE protected response. */
ZTEST(coap_oscore, test_oscore_e2e_response_encrypted)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t innerbuf[COAP_BUF_SIZE];
	uint32_t inner_len = sizeof(innerbuf);
	uint8_t token[] = {0x11, 0x22, 0x33, 0x44};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_secure_service_port);
	struct coap_oscore_context *client_ctx;
	struct coap_packet req;
	struct coap_packet inner;
	const uint8_t *payload;
	uint16_t payload_len;
	int sock;
	int ret;

	ret = oscore_make_ctx(oscore_client_id, sizeof(oscore_client_id), oscore_server_id,
			      sizeof(oscore_server_id), &client_ctx);
	zassert_ok(ret, "Client context init failed (%d)", ret);
	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_secure_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_secure_service);
	zassert_ok(ret, "Failed to start service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x2001, "e2e", -1);
	zassert_ok(ret, "Failed to build request (%d)", ret);

	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send request (%d)", ret);

	ret = oscore_recv_verify(sock, LONG_TIMEOUT, client_ctx, innerbuf, &inner_len, &inner);
	zassert_equal(ret, 0, "OSCORE response verify/parse failed (%d)", ret);
	zassert_equal(coap_header_get_code(&inner), COAP_RESPONSE_CODE_CONTENT,
		      "Unexpected inner response code");

	payload = coap_packet_get_payload(&inner, &payload_len);
	zassert_not_null(payload, "Missing decrypted payload");
	zassert_equal(payload_len, sizeof(oscore_secure_payload) - 1, "Unexpected payload length");
	zassert_mem_equal(payload, oscore_secure_payload, payload_len, "Unexpected payload");

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_secure_service), "Failed to stop service");
	coap_oscore_context_free(client_ctx);
	coap_oscore_context_free(oscore_secure_service_ctx);
	oscore_secure_service_ctx = NULL;
}

/* A required service must reject an unprotected request with a plaintext 4.01. */
ZTEST(coap_oscore, test_oscore_required_rejects_plaintext)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t recvbuf[COAP_BUF_SIZE];
	uint8_t token[] = {0x11, 0x22, 0x33, 0x44};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_secure_service_port);
	struct coap_packet req;
	struct coap_packet rsp;
	int sock;
	int ret;

	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_secure_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_secure_service);
	zassert_ok(ret, "Failed to start service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x2001, "e2e", -1);
	zassert_ok(ret, "Failed to build request (%d)", ret);

	ret = oscore_send_plain(sock, &dst, &req);
	zassert_ok(ret, "Failed to send request (%d)", ret);

	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for response (%d)", ret);

	ret = coap_packet_parse(&rsp, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse response (%d)", ret);
	zassert_false(coap_oscore_msg_has_oscore(&rsp), "Error response must be plaintext");
	zassert_equal(coap_header_get_code(&rsp), COAP_RESPONSE_CODE_UNAUTHORIZED,
		      "Expected 4.01 Unauthorized, got %u", coap_header_get_code(&rsp));

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_secure_service), "Failed to stop service");
	coap_oscore_context_free(oscore_secure_service_ctx);
	oscore_secure_service_ctx = NULL;
}

/*
 * On a mixed service an OSCORE request gets an OSCORE-protected synchronous
 * response (decided from the request, not the exchange cache), while a plaintext
 * request to the same resource gets a plaintext response.
 */
ZTEST(coap_oscore, test_oscore_mixed_sync_and_plaintext)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t recvbuf[COAP_BUF_SIZE];
	uint8_t innerbuf[COAP_BUF_SIZE];
	uint32_t inner_len = sizeof(innerbuf);
	uint8_t token[] = {0xA1, 0xA2, 0xA3, 0xA4};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_mixed_service_port);
	struct coap_oscore_context *client_ctx;
	struct coap_packet req;
	struct coap_packet inner;
	struct coap_packet rsp;
	const uint8_t *payload;
	uint16_t payload_len;
	int sock;
	int ret;

	ret = oscore_make_ctx(oscore_client_id, sizeof(oscore_client_id), oscore_server_id,
			      sizeof(oscore_server_id), &client_ctx);
	zassert_ok(ret, "Client context init failed (%d)", ret);
	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_mixed_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_mixed_service);
	zassert_ok(ret, "Failed to start mixed service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	/* Phase 1: OSCORE request -> OSCORE-protected response. */
	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x3001, "mixed", -1);
	zassert_ok(ret, "Failed to build OSCORE request (%d)", ret);

	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send OSCORE request (%d)", ret);

	ret = oscore_recv_verify(sock, LONG_TIMEOUT, client_ctx, innerbuf, &inner_len, &inner);
	zassert_ok(ret, "OSCORE response verify/parse failed (%d)", ret);
	zassert_equal(coap_header_get_code(&inner), COAP_RESPONSE_CODE_CONTENT,
		      "Unexpected inner response code");
	payload = coap_packet_get_payload(&inner, &payload_len);
	zassert_not_null(payload, "Missing decrypted payload");
	zassert_equal(payload_len, sizeof(oscore_mixed_payload) - 1, "Unexpected payload length");
	zassert_mem_equal(payload, oscore_mixed_payload, payload_len, "Unexpected payload");

	/* Phase 2: plaintext request -> plaintext response. */
	token[0] = 0xB1;
	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x3002, "mixed", -1);
	zassert_ok(ret, "Failed to build plaintext request (%d)", ret);

	ret = oscore_send_plain(sock, &dst, &req);
	zassert_ok(ret, "Failed to send plaintext request (%d)", ret);

	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for plaintext response (%d)", ret);

	ret = coap_packet_parse(&rsp, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse plaintext response (%d)", ret);
	zassert_false(coap_oscore_msg_has_oscore(&rsp),
		      "Plaintext request must get a plaintext response");
	zassert_equal(coap_header_get_code(&rsp), COAP_RESPONSE_CODE_CONTENT,
		      "Unexpected plaintext response code");

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_mixed_service), "Failed to stop mixed service");
	coap_oscore_context_free(client_ctx);
	coap_oscore_context_free(oscore_mixed_service_ctx);
	oscore_mixed_service_ctx = NULL;
}

/*
 * An OSCORE observation must get OSCORE-protected notifications, both the initial
 * response and asynchronous ones triggered by coap_resource_notify(). After
 * deregistration no further notifications are produced.
 */
ZTEST(coap_oscore, test_oscore_observe_notifications_encrypted)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t innerbuf[COAP_BUF_SIZE];
	uint32_t inner_len;
	uint8_t token[] = {0xC1, 0xC2, 0xC3, 0xC4};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_secure_service_port);
	struct coap_oscore_context *client_ctx;
	struct coap_packet req;
	struct coap_packet inner;
	int sock;
	int ret;

	ret = oscore_make_ctx(oscore_client_id, sizeof(oscore_client_id), oscore_server_id,
			      sizeof(oscore_server_id), &client_ctx);
	zassert_ok(ret, "Client context init failed (%d)", ret);
	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_secure_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_secure_service);
	zassert_ok(ret, "Failed to start service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	/* Register the observation. */
	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x4001, "observe", 0);
	zassert_ok(ret, "Failed to build observe request (%d)", ret);
	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send observe request (%d)", ret);

	/* Initial notification must be OSCORE-protected. */
	inner_len = sizeof(innerbuf);
	ret = oscore_recv_verify(sock, LONG_TIMEOUT, client_ctx, innerbuf, &inner_len, &inner);
	zassert_ok(ret, "Initial notification verify/parse failed (%d)", ret);
	zassert_true(coap_get_option_int(&inner, COAP_OPTION_OBSERVE) >= 0,
		     "Initial notification must carry the Observe option");

	/* Asynchronous notification must be OSCORE-protected. */
	ret = coap_resource_notify(&oscore_observe_resource);
	zassert_ok(ret, "coap_resource_notify failed (%d)", ret);

	inner_len = sizeof(innerbuf);
	ret = oscore_recv_verify(sock, LONG_TIMEOUT, client_ctx, innerbuf, &inner_len, &inner);
	zassert_ok(ret, "Async notification verify/parse failed (%d)", ret);
	zassert_true(coap_get_option_int(&inner, COAP_OPTION_OBSERVE) >= 0,
		     "Async notification must carry the Observe option");

	/* Deregister and drain the final response. */
	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x4002, "observe", 1);
	zassert_ok(ret, "Failed to build deregister request (%d)", ret);
	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send deregister request (%d)", ret);

	inner_len = sizeof(innerbuf);
	ret = oscore_recv_verify(sock, LONG_TIMEOUT, client_ctx, innerbuf, &inner_len, &inner);
	zassert_ok(ret, "Deregister response verify/parse failed (%d)", ret);

	/* No further notifications once the observation is gone. */
	ret = coap_resource_notify(&oscore_observe_resource);
	zassert_ok(ret, "coap_resource_notify after deregister failed (%d)", ret);
	ret = oscore_recv(sock, SHORT_TIMEOUT, innerbuf, sizeof(innerbuf));
	zassert_equal(ret, -ETIMEDOUT, "No notification expected after deregister (%d)", ret);

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_secure_service), "Failed to stop service");
	coap_oscore_context_free(client_ctx);
	coap_oscore_context_free(oscore_secure_service_ctx);
	oscore_secure_service_ctx = NULL;
}

/*
 * A plaintext observer registered on a mixed service must get plaintext
 * notifications: observer->is_oscore gates the protection.
 */
ZTEST(coap_oscore, test_oscore_observe_plaintext_not_encrypted)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t recvbuf[COAP_BUF_SIZE];
	uint8_t token[] = {0xD1, 0xD2, 0xD3, 0xD4};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_mixed_service_port);
	struct coap_packet req;
	struct coap_packet rsp;
	int sock;
	int ret;

	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_mixed_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_mixed_service);
	zassert_ok(ret, "Failed to start mixed service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	/* Register a plaintext observation. */
	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x5001, "mobserve", 0);
	zassert_ok(ret, "Failed to build observe request (%d)", ret);
	ret = oscore_send_plain(sock, &dst, &req);
	zassert_ok(ret, "Failed to send observe request (%d)", ret);

	/* Initial notification must be plaintext. */
	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for initial notification (%d)", ret);
	ret = coap_packet_parse(&rsp, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse initial notification (%d)", ret);
	zassert_false(coap_oscore_msg_has_oscore(&rsp),
		      "Plaintext observer must get a plaintext initial notification");

	/* Asynchronous notification must also be plaintext. */
	ret = coap_resource_notify(&oscore_mixed_observe_resource);
	zassert_ok(ret, "coap_resource_notify failed (%d)", ret);
	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for async notification (%d)", ret);
	ret = coap_packet_parse(&rsp, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse async notification (%d)", ret);
	zassert_false(coap_oscore_msg_has_oscore(&rsp),
		      "Plaintext observer must get plaintext notifications");

	/* Deregister and drain the final response. */
	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x5002, "mobserve", 1);
	zassert_ok(ret, "Failed to build deregister request (%d)", ret);
	ret = oscore_send_plain(sock, &dst, &req);
	zassert_ok(ret, "Failed to send deregister request (%d)", ret);

	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for deregister response (%d)", ret);
	ret = coap_packet_parse(&rsp, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse deregister response (%d)", ret);

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_mixed_service), "Failed to stop mixed service");
	coap_oscore_context_free(oscore_mixed_service_ctx);
	oscore_mixed_service_ctx = NULL;
}

/*
 * Separate-response flow: the empty ACK is sent as plaintext (messaging layer)
 * and does not clear the exchange, and the later deferred response is
 * OSCORE-protected via the surviving exchange entry.
 */
ZTEST(coap_oscore, test_oscore_empty_ack_and_deferred)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t recvbuf[COAP_BUF_SIZE];
	uint8_t innerbuf[COAP_BUF_SIZE];
	uint32_t inner_len = sizeof(innerbuf);
	uint8_t token[] = {0xE1, 0xE2, 0xE3, 0xE4};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_secure_service_port);
	struct coap_oscore_context *client_ctx;
	struct coap_oscore_exchange *entry;
	struct coap_packet req;
	struct coap_packet ack;
	struct coap_packet inner;
	const uint8_t *payload;
	uint16_t payload_len;
	int sock;
	int ret;

	oscore_deferred.valid = false;

	ret = oscore_make_ctx(oscore_client_id, sizeof(oscore_client_id), oscore_server_id,
			      sizeof(oscore_server_id), &client_ctx);
	zassert_ok(ret, "Client context init failed (%d)", ret);
	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_secure_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_secure_service);
	zassert_ok(ret, "Failed to start service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x6001, "separate", -1);
	zassert_ok(ret, "Failed to build request (%d)", ret);
	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send request (%d)", ret);

	/* The empty ACK is a plaintext, code 0.00 messaging-layer message. */
	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for empty ACK (%d)", ret);
	ret = coap_packet_parse(&ack, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse empty ACK (%d)", ret);
	zassert_false(coap_oscore_msg_has_oscore(&ack), "Empty ACK must be plaintext");
	zassert_equal(coap_header_get_type(&ack), COAP_TYPE_ACK, "Empty ACK must be type ACK");
	zassert_equal(coap_header_get_code(&ack), COAP_CODE_EMPTY, "Empty ACK must be code 0.00");

	/* The exchange added for the request must survive the empty ACK. */
	zassert_true(oscore_deferred.valid, "Separate handler must have run");
	entry = coap_oscore_exchange_find(oscore_secure_service.data->oscore_exchange_cache,
				     net_sad(&oscore_deferred.addr), oscore_deferred.addr_len,
				     oscore_deferred.token, oscore_deferred.tkl);
	zassert_not_null(entry, "Exchange must survive the empty ACK");

	/* The deferred response must be OSCORE-protected via the exchange cache. */
	ret = oscore_send_deferred_response(&oscore_separate_resource);
	zassert_ok(ret, "Failed to send deferred response (%d)", ret);

	ret = oscore_recv_verify(sock, LONG_TIMEOUT, client_ctx, innerbuf, &inner_len, &inner);
	zassert_ok(ret, "Deferred response verify/parse failed (%d)", ret);
	zassert_equal(coap_header_get_code(&inner), COAP_RESPONSE_CODE_CONTENT,
		      "Unexpected deferred response code");
	payload = coap_packet_get_payload(&inner, &payload_len);
	zassert_not_null(payload, "Missing decrypted deferred payload");
	zassert_equal(payload_len, sizeof(oscore_deferred_payload) - 1,
		      "Unexpected payload length");
	zassert_mem_equal(payload, oscore_deferred_payload, payload_len, "Unexpected payload");

	/* The exchange entry must be removed after the deferred response is sent. */
	entry = coap_oscore_exchange_find(oscore_secure_service.data->oscore_exchange_cache,
				     net_sad(&oscore_deferred.addr), oscore_deferred.addr_len,
				     oscore_deferred.token, oscore_deferred.tkl);
	zassert_is_null(entry, "Exchange must be removed after the deferred response");

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_secure_service), "Failed to stop service");
	coap_oscore_context_free(client_ctx);
	coap_oscore_context_free(oscore_secure_service_ctx);
	oscore_secure_service_ctx = NULL;
}

/*
 * On a required service, a deferred response whose exchange entry has expired is
 * dropped (fail-closed), never downgraded to plaintext.
 */
ZTEST(coap_oscore, test_oscore_deferred_after_expiry_required_dropped)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t recvbuf[COAP_BUF_SIZE];
	uint8_t token[] = {0xF1, 0xF2, 0xF3, 0xF4};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_secure_service_port);
	struct coap_oscore_context *client_ctx;
	struct coap_oscore_exchange *entry;
	struct coap_packet req;
	int sock;
	int ret;

	oscore_deferred.valid = false;

	ret = oscore_make_ctx(oscore_client_id, sizeof(oscore_client_id), oscore_server_id,
			      sizeof(oscore_server_id), &client_ctx);
	zassert_ok(ret, "Client context init failed (%d)", ret);
	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_secure_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_secure_service);
	zassert_ok(ret, "Failed to start service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x7001, "separate", -1);
	zassert_ok(ret, "Failed to build request (%d)", ret);
	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send request (%d)", ret);

	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for empty ACK (%d)", ret);
	zassert_true(oscore_deferred.valid, "Separate handler must have run");

	/* Force the exchange entry to expire. */
	entry = coap_oscore_exchange_find(oscore_secure_service.data->oscore_exchange_cache,
				     net_sad(&oscore_deferred.addr), oscore_deferred.addr_len,
				     oscore_deferred.token, oscore_deferred.tkl);
	zassert_not_null(entry, "Exchange must exist before expiry");
	entry->timestamp = k_uptime_get() - CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS - 1000;

	/* Deferred response must be dropped, not downgraded. */
	ret = oscore_send_deferred_response(&oscore_separate_resource);
	zassert_equal(ret, -EACCES, "Expired deferred response must fail closed (%d)", ret);

	ret = oscore_recv(sock, SHORT_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_equal(ret, -ETIMEDOUT, "No plaintext downgrade expected (%d)", ret);

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_secure_service), "Failed to stop service");
	coap_oscore_context_free(client_ctx);
	coap_oscore_context_free(oscore_secure_service_ctx);
	oscore_secure_service_ctx = NULL;
}

/*
 * On a mixed service, a deferred response whose exchange entry has expired can no
 * longer be matched and is sent unprotected. This documents the known mixed-service
 * gap and guards against it changing silently.
 */
ZTEST(coap_oscore, test_oscore_deferred_after_expiry_mixed_plaintext)
{
	uint8_t reqbuf[COAP_BUF_SIZE];
	uint8_t recvbuf[COAP_BUF_SIZE];
	uint8_t token[] = {0x1A, 0x2B, 0x3C, 0x4D};
	struct net_sockaddr_in6 dst = oscore_loopback_dst(oscore_mixed_service_port);
	struct coap_oscore_context *client_ctx;
	struct coap_oscore_exchange *entry;
	struct coap_packet req;
	struct coap_packet rsp;
	int sock;
	int ret;

	oscore_deferred.valid = false;

	ret = oscore_make_ctx(oscore_client_id, sizeof(oscore_client_id), oscore_server_id,
			      sizeof(oscore_server_id), &client_ctx);
	zassert_ok(ret, "Client context init failed (%d)", ret);
	ret = oscore_make_ctx(oscore_server_id, sizeof(oscore_server_id), oscore_client_id,
			      sizeof(oscore_client_id), &oscore_mixed_service_ctx);
	zassert_ok(ret, "Server context init failed (%d)", ret);

	ret = coap_service_start(&oscore_mixed_service);
	zassert_ok(ret, "Failed to start mixed service (%d)", ret);

	sock = oscore_client_socket();
	zassert_true(sock >= 0, "Failed to create socket (%d)", -errno);

	ret = oscore_build_request(&req, reqbuf, sizeof(reqbuf), COAP_TYPE_CON, COAP_METHOD_GET,
				   token, sizeof(token), 0x8001, "separatem", -1);
	zassert_ok(ret, "Failed to build request (%d)", ret);
	ret = oscore_send_protected(sock, client_ctx, &dst, &req);
	zassert_ok(ret, "Failed to send request (%d)", ret);

	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for empty ACK (%d)", ret);
	zassert_true(oscore_deferred.valid, "Separate handler must have run");

	entry = coap_oscore_exchange_find(oscore_mixed_service.data->oscore_exchange_cache,
				     net_sad(&oscore_deferred.addr), oscore_deferred.addr_len,
				     oscore_deferred.token, oscore_deferred.tkl);
	zassert_not_null(entry, "Exchange must exist before expiry");
	entry->timestamp = k_uptime_get() - CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS - 1000;

	/* Mixed service: the deferred response goes out unprotected (documented gap). */
	ret = oscore_send_deferred_response(&oscore_separate_mixed_resource);
	zassert_ok(ret, "Mixed deferred response should still be sent (%d)", ret);

	ret = oscore_recv(sock, LONG_TIMEOUT, recvbuf, sizeof(recvbuf));
	zassert_true(ret > 0, "Timed out waiting for deferred response (%d)", ret);
	ret = coap_packet_parse(&rsp, recvbuf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse deferred response (%d)", ret);
	zassert_false(coap_oscore_msg_has_oscore(&rsp),
		      "Mixed deferred response after expiry is plaintext (documented gap)");

	zassert_ok(zsock_close(sock), "Failed to close socket");
	zassert_ok(coap_service_stop(&oscore_mixed_service), "Failed to stop mixed service");
	coap_oscore_context_free(client_ctx);
	coap_oscore_context_free(oscore_mixed_service_ctx);
	oscore_mixed_service_ctx = NULL;
}

/*
 * An empty ACK carries a zero-length token, so a tkl=0 removal must not clear the
 * real (tkl>0) exchange entry.
 */
ZTEST(coap_oscore, test_oscore_empty_ack_does_not_clear_exchange)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1}}},
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0xAA, 0xBB, 0xCC, 0xDD};
	struct coap_oscore_exchange *entry;
	int ret;

	memset(cache, 0, sizeof(cache));

	ret = coap_oscore_exchange_add(cache, (struct net_sockaddr *)&addr, sizeof(addr), token,
				  sizeof(token));
	zassert_ok(ret, "Should add exchange entry");

	/* Empty ACK removal uses a zero-length token; it must not match. */
	coap_oscore_exchange_remove(cache, (struct net_sockaddr *)&addr, sizeof(addr), token, 0);

	entry = coap_oscore_exchange_find(cache, (struct net_sockaddr *)&addr, sizeof(addr), token,
				     sizeof(token));
	zassert_not_null(entry, "tkl=0 removal must not clear the real (tkl>0) entry");
}
#else
static uint16_t coap_service_port = 56839;
static const uint8_t plain_payload[] = "encrypted-response";
static const char *const coap_service_path[] = {"plain", NULL};

static int coap_service_get(struct coap_resource *resource, struct coap_packet *request,
			    struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t data[COAP_BUF_SIZE];
	struct coap_packet response;
	uint8_t tkl;
	int ret;

	ARG_UNUSED(resource);

	tkl = coap_header_get_token(request, token);

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_ACK, tkl,
			       token, COAP_RESPONSE_CODE_CONTENT, coap_header_get_id(request));
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_packet_append_payload(&response, plain_payload, sizeof(plain_payload) - 1);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	ret = coap_resource_send(resource, &response, addr, addr_len, NULL);
	if (ret < 0) {
		return COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	return 0;
}

static int coap_client_socket(void)
{
	return zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
}

static int coap_server_send(int sock, enum coap_msgtype type, bool add_unsupported, uint16_t msg_id,
			    const uint8_t *token, uint8_t tkl)
{
	uint8_t req_buf[COAP_BUF_SIZE];
	struct coap_packet req;
	int ret;

	ret = coap_packet_init(&req, req_buf, sizeof(req_buf), COAP_VERSION_1, type, tkl, token,
			       COAP_METHOD_GET, msg_id);
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_option(&req, COAP_OPTION_URI_PATH,
					(const uint8_t *)coap_service_path[0],
					strlen(coap_service_path[0]));
	if (ret < 0) {
		return ret;
	}

	if (add_unsupported) {
		uint8_t critical_opt_value[] = {0x42};

		ret = coap_packet_append_option(&req, COAP_OSCORE_OPTION, critical_opt_value,
						sizeof(critical_opt_value));
		if (ret < 0) {
			return ret;
		}
	}

	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = {{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}},
		.sin6_port = net_htons(coap_service_port),
	};

	ret = zsock_sendto(sock, req.data, req.offset, 0, (struct net_sockaddr *)&dst, sizeof(dst));
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

static int coap_server_wait_response_timeout(int sock, uint8_t *buf, size_t len, int timeout_ms)
{
	struct zsock_pollfd pfd = {
		.fd = sock,
		.events = ZSOCK_POLLIN,
		.revents = 0,
	};
	int ret;

	ret = zsock_poll(&pfd, 1, timeout_ms);
	if (ret <= 0) {
		return ret;
	}

	ret = zsock_recvfrom(sock, buf, len, 0, NULL, NULL);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

COAP_SERVICE_DEFINE(coap_service, NULL, &coap_service_port, 0);

/* clang-format off */
COAP_RESOURCE_DEFINE(coap_service_resource, coap_service,
			{
				.path = coap_service_path,
				.get = coap_service_get,
			}
);

ZTEST(coap_oscore, test_unsupported_critical_option_helper)
{
	struct coap_packet cpkt;
	uint8_t buffer[128];
	uint16_t unsupported_opt;
	int ret;

	/* Build a packet with OSCORE option (which is unsupported in this build) */
	ret = coap_packet_init(&cpkt, buffer, sizeof(buffer), COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0x1234);
	zassert_ok(ret, "Failed to init packet");

	/* Add OSCORE option with some dummy value */
	uint8_t oscore_value[] = {0x01, 0x02, 0x03};

	ret = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE, oscore_value,
				      sizeof(oscore_value));
	zassert_ok(ret, "Failed to append OSCORE option");

	/* Add a payload to make it a valid OSCORE message format */
	ret = coap_packet_append_payload_marker(&cpkt);
	zassert_ok(ret, "Failed to append payload marker");

	uint8_t payload[] = "test";

	ret = coap_packet_append_payload(&cpkt, payload, sizeof(payload));
	zassert_ok(ret, "Failed to append payload");

	/* Test: Check for unsupported critical options */
	ret = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_equal(ret, -ENOTSUP, "Should detect unsupported OSCORE option");
	zassert_equal(unsupported_opt, COAP_OPTION_OSCORE,
		      "Should report OSCORE as unsupported option");

	/* Test: Packet without OSCORE should pass */
	uint8_t buffer2[128];

	ret = coap_packet_init(&cpkt, buffer2, sizeof(buffer2), COAP_VERSION_1, COAP_TYPE_CON, 0,
			     NULL, COAP_METHOD_GET, 0x1235);
	zassert_ok(ret, "Failed to init packet");

	ret = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_ok(ret, "Should not detect unsupported options in normal packet");
}

ZTEST(coap_oscore, test_server_rejects_unsupported_critical_con_request)
{
	uint8_t recv_buf[COAP_BUF_SIZE];
	struct coap_packet response;
	uint8_t token[] = {0xAA, 0x55};
	uint8_t rsp_token[COAP_TOKEN_MAX_LEN] = {0};
	uint16_t msg_id = 0x3301;
	int sock;
	int ret;

	ret = coap_service_start(&coap_service);
	zassert_ok(ret, "Failed to start emulation CoAP service (%d)", ret);

	sock = coap_client_socket();
	zassert_true(sock >= 0, "Failed to open client socket (%d)", sock);

	ret = coap_server_send(sock, COAP_TYPE_CON, true, msg_id, token, sizeof(token));
	zassert_true(ret > 0, "Failed to send CON request with unsupported option (%d)", ret);

	ret = coap_server_wait_response_timeout(sock, recv_buf, sizeof(recv_buf),
						    SHORT_TIMEOUT);
	zassert_true(ret > 0, "Expected 4.02 response, got %d", ret);

	ret = coap_packet_parse(&response, recv_buf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse response (%d)", ret);
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_ACK,
		      "Expected ACK for unsupported CON request");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_BAD_OPTION,
		      "Expected 4.02 Bad Option response");
	zassert_equal(coap_header_get_id(&response), msg_id, "Response id mismatch");
	zassert_equal(coap_header_get_token(&response, rsp_token), sizeof(token),
		      "Response token length mismatch");
	zassert_mem_equal(rsp_token, token, sizeof(token), "Response token mismatch");

	zassert_ok(zsock_close(sock), "Failed to close client socket");
	zassert_ok(coap_service_stop(&coap_service),
		      "Failed to stop emulation CoAP service");
}

ZTEST(coap_oscore, test_server_drops_unsupported_critical_non_request)
{
	uint8_t recv_buf[COAP_BUF_SIZE];
	uint8_t token[] = {0xAA, 0x56};
	int sock;
	int ret;

	ret = coap_service_start(&coap_service);
	zassert_ok(ret, "Failed to start emulation CoAP service (%d)", ret);

	sock = coap_client_socket();
	zassert_true(sock >= 0, "Failed to open client socket (%d)", sock);

	ret = coap_server_send(sock, COAP_TYPE_NON_CON, true, 0x3302, token,
					   sizeof(token));
	zassert_true(ret > 0, "Failed to send NON request with unsupported option (%d)", ret);

	ret = coap_server_wait_response_timeout(sock, recv_buf, sizeof(recv_buf),
						    SHORT_TIMEOUT);
	zassert_ok(ret,
		      "Expected silent drop for NON request with unsupported option, got %d", ret);

	zassert_ok(zsock_close(sock), "Failed to close client socket");
	zassert_ok(coap_service_stop(&coap_service),
		      "Failed to stop emulation CoAP service");
}

ZTEST(coap_oscore, test_normal_messages_not_affected_by_unsupported_option)
{
	static const uint8_t expected_payload[] = "encrypted-response";
	uint8_t recv_buf[COAP_BUF_SIZE];
	struct coap_packet response;
	uint8_t token[] = {0xAA, 0x57};
	uint16_t payload_len;
	const uint8_t *payload;
	int sock;
	int ret;

	ret = coap_service_start(&coap_service);
	zassert_ok(ret, "Failed to start emulation CoAP service (%d)", ret);

	sock = coap_client_socket();
	zassert_true(sock >= 0, "Failed to open client socket (%d)", sock);
	ret = coap_server_send(sock, COAP_TYPE_CON, false, 0x3303, token,
					   sizeof(token));
	zassert_true(ret > 0, "Failed to send normal request (%d)", ret);
	ret = coap_server_wait_response_timeout(sock, recv_buf, sizeof(recv_buf),
						    SHORT_TIMEOUT);
	zassert_true(ret > 0, "Expected normal response, got %d", ret);

	ret = coap_packet_parse(&response, recv_buf, ret, NULL, 0);
	zassert_ok(ret, "Failed to parse response (%d)", ret);
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_ACK,
		      "Expected ACK for normal CON request");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_CONTENT,
		      "Expected 2.05 Content response");

			  payload = coap_packet_get_payload(&response, &payload_len);
	zassert_not_null(payload, "Expected payload in normal response");
	zassert_equal(payload_len, sizeof(expected_payload) - 1U, "Unexpected payload length");
	zassert_mem_equal(payload, expected_payload, sizeof(expected_payload) - 1U,
			  "Unexpected response payload");

	zassert_ok(zsock_close(sock), "Failed to close client socket");
	zassert_ok(coap_service_stop(&coap_service),
		      "Failed to stop emulation CoAP service");
}
#endif /* CONFIG_COAP_OSCORE */

ZTEST_SUITE(coap_oscore, NULL, NULL, NULL, NULL, NULL);
