/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/socket.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_link_format.h>
#include <zephyr/net/coap_mgmt.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/zvfs/eventfd.h>
#include <zephyr/random/random.h>

#if defined(CONFIG_COAP_OSCORE)
#include "coap_oscore.h"
#endif

#if defined(CONFIG_COAP_EDHOC)
#include "coap_edhoc.h"
#endif

#if defined(CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC)
#include <zephyr/net/coap/coap_edhoc_transport.h>

/* Forward declaration for EDHOC transport handler */
extern int coap_edhoc_transport_handle_request(const struct coap_service *service,
					       const struct coap_packet *request,
					       const struct net_sockaddr *client_addr,
					       net_socklen_t client_addr_len);
#endif

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
#include "coap_oscore_option.h"
#include "coap_edhoc_session.h"
#include "coap_oscore_ctx_cache.h"

/* Forward declarations for weak wrappers */
extern int coap_edhoc_msg3_process_wrapper(const uint8_t *edhoc_msg3,
					   size_t edhoc_msg3_len,
					   void *resp_ctx,
					   void *runtime_ctx,
					   void *cred_i_array,
					   uint8_t *prk_out,
					   size_t *prk_out_len,
					   uint8_t *initiator_pk,
					   size_t *initiator_pk_len,
					   uint8_t *c_i,
					   size_t *c_i_len);
extern int coap_edhoc_exporter_wrapper(const uint8_t *prk_out,
				       size_t prk_out_len,
				       int app_hash_alg,
				       uint8_t label,
				       uint8_t *output,
				       size_t *output_len);
extern int coap_oscore_context_init_wrapper(void *ctx,
					     const uint8_t *master_secret,
					     size_t master_secret_len,
					     const uint8_t *master_salt,
					     size_t master_salt_len,
					     const uint8_t *sender_id,
					     size_t sender_id_len,
					     const uint8_t *recipient_id,
					     size_t recipient_id_len,
					     int aead_alg,
					     int hkdf_alg);
#endif

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

#define ADDRLEN(sock) \
	(((struct net_sockaddr *)sock)->sa_family == NET_AF_INET ? \
		sizeof(struct net_sockaddr_in) : sizeof(struct net_sockaddr_in6))

/* Shortened defines */
#define MAX_OPTIONS    CONFIG_COAP_SERVER_MESSAGE_OPTIONS
#define MAX_PENDINGS   CONFIG_COAP_SERVICE_PENDING_MESSAGES
#define MAX_OBSERVERS  CONFIG_COAP_SERVICE_OBSERVERS
#define MAX_POLL_FD    CONFIG_ZVFS_POLL_MAX

BUILD_ASSERT(CONFIG_ZVFS_POLL_MAX > 0, "CONFIG_ZVFS_POLL_MAX can't be 0");

static K_MUTEX_DEFINE(lock);
static int control_sock;

#if defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC)
K_MEM_SLAB_DEFINE_STATIC(pending_data, CONFIG_COAP_SERVER_MESSAGE_SIZE,
			 CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC_BLOCKS, 4);
#endif

static inline void *coap_server_alloc(size_t len)
{
#if defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC)
	void *ptr;
	int ret;

	if (len > CONFIG_COAP_SERVER_MESSAGE_SIZE) {
		return NULL;
	}

	ret = k_mem_slab_alloc(&pending_data, &ptr, K_NO_WAIT);
	if (ret < 0) {
		return NULL;
	}

	return ptr;
#elif defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_SYSTEM_HEAP)
	return k_malloc(len);
#else
	ARG_UNUSED(len);

	return NULL;
#endif
}

static inline void coap_server_free(void *ptr)
{
#if defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC)
	k_mem_slab_free(&pending_data, ptr);
#elif defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_SYSTEM_HEAP)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}

#if defined(CONFIG_COAP_OSCORE) || defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
/**
 * @brief Send a simple CoAP error response
 *
 * Helper function to reduce code duplication when sending error responses.
 *
 * @param service CoAP service
 * @param request Original request packet
 * @param code CoAP response code
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @param add_max_age_zero If true, add Max-Age: 0 option (for OSCORE errors per RFC 8613)
 * @return 0 on success, negative errno on error
 */
static int send_error_response_internal(const struct coap_service *service,
					const struct coap_packet *request,
					uint8_t code,
					const struct net_sockaddr *client_addr,
					net_socklen_t client_addr_len,
					bool add_max_age_zero)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	int ret;

	ret = coap_packet_init(&response, buf, sizeof(buf),
			       COAP_VERSION_1, type, tkl, token, code, id);
	if (ret < 0) {
		return ret;
	}

	/* RFC 8613 Section 8.2/7.4: OSCORE error responses MAY include Max-Age: 0 */
	if (add_max_age_zero) {
		ret = coap_append_option_int(&response, COAP_OPTION_MAX_AGE, 0);
		if (ret < 0) {
			LOG_WRN("Failed to add Max-Age option to OSCORE error response (%d)", ret);
			/* Continue anyway - Max-Age is optional */
		}
	}

	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

/**
 * @brief Send a simple CoAP error response (without Max-Age)
 *
 * Wrapper for send_error_response_internal for non-OSCORE errors.
 */
static int send_error_response(const struct coap_service *service,
			       const struct coap_packet *request,
			       uint8_t code,
			       const struct net_sockaddr *client_addr,
			       net_socklen_t client_addr_len)
{
	return send_error_response_internal(service, request, code,
					    client_addr, client_addr_len, false);
}

#if defined(CONFIG_COAP_OSCORE)
/**
 * @brief Send an OSCORE error response with Max-Age: 0
 *
 * Per RFC 8613 Section 8.2/8.3/7.4, OSCORE error responses are sent as
 * simple CoAP responses (without OSCORE processing) and MAY include
 * Max-Age: 0 to prevent caching.
 */
static int send_oscore_error_response(const struct coap_service *service,
				      const struct coap_packet *request,
				      uint8_t code,
				      const struct net_sockaddr *client_addr,
				      net_socklen_t client_addr_len)
{
	return send_error_response_internal(service, request, code,
					    client_addr, client_addr_len, true);
}
#endif /* CONFIG_COAP_OSCORE */
#endif /* CONFIG_COAP_OSCORE || CONFIG_COAP_EDHOC_COMBINED_REQUEST */

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
/**
 * @brief Build an EDHOC error response (unprotected)
 *
 * Per RFC 9668 Section 3.3.1 and RFC 9528 Section 6/Appendix A.2.3,
 * when EDHOC processing fails in a combined request, the server responds
 * with an unprotected CoAP error response containing an EDHOC error message.
 *
 * The response:
 * - Uses response code 4.00 (Bad Request) for client errors
 * - Contains Content-Format option set to application/edhoc+cbor-seq (64)
 * - Has payload with EDHOC error CBOR sequence: (ERR_CODE, ERR_INFO)
 * - MUST NOT be protected with OSCORE
 *
 * @param response Output CoAP packet
 * @param request Original request packet
 * @param code CoAP response code (typically 4.00 or 5.00)
 * @param err_code EDHOC error code (typically 1 for Unspecified Error)
 * @param diag_msg Diagnostic message (ERR_INFO as tstr)
 * @param buf Buffer for response
 * @param buf_len Buffer length
 * @return 0 on success, negative errno on error
 */
static int build_edhoc_error_response(struct coap_packet *response,
				       const struct coap_packet *request,
				       uint8_t code,
				       int err_code,
				       const char *diag_msg,
				       uint8_t *buf, size_t buf_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	int ret;

	/* Encode EDHOC error payload */
	uint8_t error_payload[256];
	size_t error_len = sizeof(error_payload);

	ret = coap_edhoc_encode_error(err_code, diag_msg, error_payload, &error_len);
	if (ret < 0) {
		LOG_ERR("Failed to encode EDHOC error (%d)", ret);
		return ret;
	}

	/* Build CoAP response */
	ret = coap_packet_init(response, buf, buf_len,
			       COAP_VERSION_1, type, tkl, token, code, id);
	if (ret < 0) {
		LOG_ERR("Failed to init EDHOC error response (%d)", ret);
		return ret;
	}

	/* Add Content-Format option: application/edhoc+cbor-seq (64) */
	ret = coap_append_option_int(response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_APP_EDHOC_CBOR_SEQ);
	if (ret < 0) {
		LOG_ERR("Failed to add Content-Format option (%d)", ret);
		return ret;
	}

	/* Add payload */
	ret = coap_packet_append_payload_marker(response);
	if (ret < 0) {
		LOG_ERR("Failed to add payload marker (%d)", ret);
		return ret;
	}

	ret = coap_packet_append_payload(response, error_payload, error_len);
	if (ret < 0) {
		LOG_ERR("Failed to add EDHOC error payload (%d)", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Send an EDHOC error response (unprotected)
 *
 * @param service CoAP service
 * @param request Original request packet
 * @param code CoAP response code (typically 4.00 or 5.00)
 * @param err_code EDHOC error code (typically 1 for Unspecified Error)
 * @param diag_msg Diagnostic message (ERR_INFO as tstr)
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @return 0 on success, negative errno on error
 */
static int send_edhoc_error_response(const struct coap_service *service,
				      const struct coap_packet *request,
				      uint8_t code,
				      int err_code,
				      const char *diag_msg,
				      const struct net_sockaddr *client_addr,
				      net_socklen_t client_addr_len)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	int ret;

	ret = build_edhoc_error_response(&response, request, code, err_code, diag_msg,
					 buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Sending EDHOC error response: code=%d, err_code=%d, diag=\"%s\"",
		code, err_code, diag_msg);

	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

#if defined(CONFIG_ZTEST)
/**
 * @brief Build an EDHOC error response for testing
 *
 * Test wrapper that exposes the internal build function for unit tests.
 */
int coap_edhoc_build_error_response(struct coap_packet *response,
				     const struct coap_packet *request,
				     uint8_t code,
				     int err_code,
				     const char *diag_msg,
				     uint8_t *buf, size_t buf_len);

int coap_edhoc_build_error_response(struct coap_packet *response,
				     const struct coap_packet *request,
				     uint8_t code,
				     int err_code,
				     const char *diag_msg,
				     uint8_t *buf, size_t buf_len)
{
	return build_edhoc_error_response(response, request, code, err_code, diag_msg,
					  buf, buf_len);
}
#endif /* CONFIG_ZTEST */
#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

#if defined(CONFIG_COAP_SERVER_ECHO)

/* Compare two socket addresses for equality */
static bool sockaddr_equal(const struct net_sockaddr *a, net_socklen_t a_len,
			   const struct net_sockaddr *b, net_socklen_t b_len)
{
	if (a_len != b_len || a->sa_family != b->sa_family) {
		return false;
	}

	if (a->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *a4 = (const struct net_sockaddr_in *)a;
		const struct net_sockaddr_in *b4 = (const struct net_sockaddr_in *)b;

		return a4->sin_port == b4->sin_port &&
		       net_ipv4_addr_cmp(&a4->sin_addr, &b4->sin_addr);
	} else if (a->sa_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *a6 = (const struct net_sockaddr_in6 *)a;
		const struct net_sockaddr_in6 *b6 = (const struct net_sockaddr_in6 *)b;

		return a6->sin6_port == b6->sin6_port &&
		       net_ipv6_addr_cmp(&a6->sin6_addr, &b6->sin6_addr);
	}

	return false;
}

/* Find Echo cache entry for a given address */
#if defined(CONFIG_ZTEST)
struct coap_echo_entry *coap_echo_cache_find(struct coap_echo_entry *cache,
					     const struct net_sockaddr *addr,
					     net_socklen_t addr_len);
#endif

static struct coap_echo_entry *echo_cache_find(struct coap_echo_entry *cache,
					       const struct net_sockaddr *addr,
					       net_socklen_t addr_len)
{
	for (int i = 0; i < CONFIG_COAP_SERVER_ECHO_CACHE_SIZE; i++) {
		if (cache[i].addr_len > 0 &&
		    sockaddr_equal(&cache[i].addr, cache[i].addr_len, addr, addr_len)) {
			return &cache[i];
		}
	}

	return NULL;
}

#if defined(CONFIG_ZTEST)
struct coap_echo_entry *coap_echo_cache_find(struct coap_echo_entry *cache,
					     const struct net_sockaddr *addr,
					     net_socklen_t addr_len)
{
	return echo_cache_find(cache, addr, addr_len);
}
#endif

/* Find or allocate Echo cache entry (LRU eviction) */
static struct coap_echo_entry *echo_cache_get_entry(struct coap_echo_entry *cache,
						    const struct net_sockaddr *addr,
						    net_socklen_t addr_len)
{
	struct coap_echo_entry *entry;
	struct coap_echo_entry *oldest = NULL;
	int64_t oldest_time = INT64_MAX;

	/* Try to find existing entry */
	entry = echo_cache_find(cache, addr, addr_len);
	if (entry != NULL) {
		return entry;
	}

	/* Find empty or oldest entry */
	for (int i = 0; i < CONFIG_COAP_SERVER_ECHO_CACHE_SIZE; i++) {
		if (cache[i].addr_len == 0) {
			return &cache[i];
		}
		if (cache[i].timestamp < oldest_time) {
			oldest_time = cache[i].timestamp;
			oldest = &cache[i];
		}
	}

	/* Evict oldest entry */
	if (oldest != NULL) {
		memset(oldest, 0, sizeof(*oldest));
	}

	return oldest;
}

/* Generate a new Echo option value */
static int echo_generate_value(uint8_t *buf, size_t len)
{
	if (len > CONFIG_COAP_SERVER_ECHO_MAX_LEN) {
		return -EINVAL;
	}

	/* Generate random bytes for the Echo value */
	return sys_csrand_get(buf, len);
}

/* Create and store a new Echo challenge for a client */
#if defined(CONFIG_ZTEST)
int coap_echo_create_challenge(struct coap_echo_entry *cache,
			       const struct net_sockaddr *addr,
			       net_socklen_t addr_len,
			       uint8_t *echo_value, size_t *echo_len);
#endif

static int echo_create_challenge(struct coap_echo_entry *cache,
				 const struct net_sockaddr *addr,
				 net_socklen_t addr_len,
				 uint8_t *echo_value, size_t *echo_len)
{
	struct coap_echo_entry *entry;
	int ret;

	entry = echo_cache_get_entry(cache, addr, addr_len);
	if (entry == NULL) {
		return -ENOMEM;
	}

	/* Generate new Echo value */
	ret = echo_generate_value(entry->echo_value, CONFIG_COAP_SERVER_ECHO_MAX_LEN);
	if (ret < 0) {
		return ret;
	}

	entry->echo_len = CONFIG_COAP_SERVER_ECHO_MAX_LEN;
	entry->timestamp = k_uptime_get();
	entry->verified_until = 0; /* Not verified yet */

	/* Copy address */
	memcpy(&entry->addr, addr, addr_len);
	entry->addr_len = addr_len;

	/* Return the Echo value to caller */
	memcpy(echo_value, entry->echo_value, entry->echo_len);
	*echo_len = entry->echo_len;

	return 0;
}

#if defined(CONFIG_ZTEST)
int coap_echo_create_challenge(struct coap_echo_entry *cache,
			       const struct net_sockaddr *addr,
			       net_socklen_t addr_len,
			       uint8_t *echo_value, size_t *echo_len)
{
	return echo_create_challenge(cache, addr, addr_len, echo_value, echo_len);
}
#endif

/* Verify an Echo option value from a request */
#if defined(CONFIG_ZTEST)
int coap_echo_verify_value(struct coap_echo_entry *cache,
			   const struct net_sockaddr *addr,
			   net_socklen_t addr_len,
			   const uint8_t *echo_value, size_t echo_len);
#endif

static int echo_verify_value(struct coap_echo_entry *cache,
			     const struct net_sockaddr *addr,
			     net_socklen_t addr_len,
			     const uint8_t *echo_value, size_t echo_len)
{
	struct coap_echo_entry *entry;
	int64_t now = k_uptime_get();

	/* RFC 9175 Section 2.2.1: Echo length must be 1-40 bytes */
	if (echo_len == 0 || echo_len > 40) {
		return -EINVAL;
	}

	entry = echo_cache_find(cache, addr, addr_len);
	if (entry == NULL) {
		/* No cached Echo value for this client */
		return -ENOENT;
	}

	/* Check if Echo value has expired */
	if ((now - entry->timestamp) > CONFIG_COAP_SERVER_ECHO_LIFETIME_MS) {
		return -ETIMEDOUT;
	}

	/* Verify Echo value matches */
	if (entry->echo_len != echo_len ||
	    memcmp(entry->echo_value, echo_value, echo_len) != 0) {
		return -EINVAL;
	}

	/* Mark address as verified for amplification mitigation */
	entry->verified_until = now + CONFIG_COAP_SERVER_ECHO_LIFETIME_MS;

	return 0;
}

#if defined(CONFIG_ZTEST)
int coap_echo_verify_value(struct coap_echo_entry *cache,
			   const struct net_sockaddr *addr,
			   net_socklen_t addr_len,
			   const uint8_t *echo_value, size_t echo_len)
{
	return echo_verify_value(cache, addr, addr_len, echo_value, echo_len);
}
#endif

/* Check if a client address is verified for amplification mitigation */
#if defined(CONFIG_ZTEST)
bool coap_echo_is_address_verified(struct coap_echo_entry *cache,
				   const struct net_sockaddr *addr,
				   net_socklen_t addr_len);
#endif

static bool echo_is_address_verified(struct coap_echo_entry *cache,
				     const struct net_sockaddr *addr,
				     net_socklen_t addr_len)
{
	struct coap_echo_entry *entry;
	int64_t now = k_uptime_get();

	entry = echo_cache_find(cache, addr, addr_len);
	if (entry == NULL) {
		return false;
	}

	return entry->verified_until > now;
}

#if defined(CONFIG_ZTEST)
bool coap_echo_is_address_verified(struct coap_echo_entry *cache,
				   const struct net_sockaddr *addr,
				   net_socklen_t addr_len)
{
	return echo_is_address_verified(cache, addr, addr_len);
}
#endif

/* Build a 4.01 Unauthorized response with Echo option */
#if defined(CONFIG_ZTEST)
int coap_echo_build_challenge_response(struct coap_packet *response,
				       const struct coap_packet *request,
				       const uint8_t *echo_value,
				       size_t echo_len,
				       uint8_t *buf, size_t buf_len);
#endif

static int echo_build_challenge_response(struct coap_packet *response,
					 const struct coap_packet *request,
					 const uint8_t *echo_value,
					 size_t echo_len,
					 uint8_t *buf, size_t buf_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = coap_header_get_type(request);
	int ret;

	/* RFC 9175 Section 2.4 item 3: must be piggybacked or NON, never separate */
	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
		id = coap_next_id();
	}

	ret = coap_packet_init(response, buf, buf_len, COAP_VERSION_1, type,
			       tkl, token, COAP_RESPONSE_CODE_UNAUTHORIZED, id);
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_option(response, COAP_OPTION_ECHO,
					echo_value, echo_len);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#if defined(CONFIG_ZTEST)
int coap_echo_build_challenge_response(struct coap_packet *response,
				       const struct coap_packet *request,
				       const uint8_t *echo_value,
				       size_t echo_len,
				       uint8_t *buf, size_t buf_len)
{
	return echo_build_challenge_response(response, request, echo_value,
					    echo_len, buf, buf_len);
}
#endif

/* Check if method is unsafe (requires freshness) */
#if defined(CONFIG_ZTEST)
bool coap_is_unsafe_method(uint8_t code);
#endif

static bool is_unsafe_method(uint8_t code)
{
	return code == COAP_METHOD_POST ||
	       code == COAP_METHOD_PUT ||
	       code == COAP_METHOD_DELETE ||
	       code == COAP_METHOD_PATCH ||
	       code == COAP_METHOD_IPATCH;
}

#if defined(CONFIG_ZTEST)
bool coap_is_unsafe_method(uint8_t code)
{
	return is_unsafe_method(code);
}
#endif

/* Extract Echo option from request */
#if defined(CONFIG_ZTEST)
int coap_echo_extract_from_request(const struct coap_packet *request,
				   uint8_t *echo_value, size_t *echo_len);
#endif

static int echo_extract_from_request(const struct coap_packet *request,
				     uint8_t *echo_value, size_t *echo_len)
{
	struct coap_option option;
	int ret;

	ret = coap_find_options(request, COAP_OPTION_ECHO, &option, 1);
	if (ret < 0) {
		return ret;
	}
	if (ret == 0) {
		return -ENOENT;
	}

	if (option.len > 40 || option.len == 0) {
		/* Invalid Echo length per RFC 9175 Section 2.2.1 */
		return -EINVAL;
	}

	memcpy(echo_value, option.value, option.len);
	*echo_len = option.len;

	return 0;
}

#if defined(CONFIG_ZTEST)
int coap_echo_extract_from_request(const struct coap_packet *request,
				   uint8_t *echo_value, size_t *echo_len)
{
	return echo_extract_from_request(request, echo_value, echo_len);
}
#endif

#endif /* CONFIG_COAP_SERVER_ECHO */

#if defined(CONFIG_COAP_OSCORE)

/* Find OSCORE exchange entry for a given address and token */
#if defined(CONFIG_COAP_TEST_API_ENABLE)
struct coap_oscore_exchange *oscore_exchange_find(
#else
static struct coap_oscore_exchange *oscore_exchange_find(
#endif
	struct coap_oscore_exchange *cache,
	const struct net_sockaddr *addr,
	net_socklen_t addr_len,
	const uint8_t *token,
	uint8_t tkl)
{
	int64_t now = k_uptime_get();

	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		if (cache[i].addr_len == 0) {
			continue;
		}

		/* Check if entry has expired */
		if (!cache[i].is_observe &&
		    (now - cache[i].timestamp) > CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS) {
			/* Entry expired, clear it */
			memset(&cache[i], 0, sizeof(cache[i]));
			continue;
		}

		/* Check if address and token match */
		if (cache[i].tkl == tkl &&
		    sockaddr_equal(&cache[i].addr, cache[i].addr_len, addr, addr_len) &&
		    memcmp(cache[i].token, token, tkl) == 0) {
			return &cache[i];
		}
	}

	return NULL;
}

/* Add or update OSCORE exchange entry */
#if defined(CONFIG_COAP_TEST_API_ENABLE)
int oscore_exchange_add(
#else
static int oscore_exchange_add(
#endif
			       struct coap_oscore_exchange *cache,
			       const struct net_sockaddr *addr,
			       net_socklen_t addr_len,
			       const uint8_t *token,
			       uint8_t tkl,
			       bool is_observe,
			       struct context *oscore_ctx)
{
	struct coap_oscore_exchange *entry;
	struct coap_oscore_exchange *oldest = NULL;
	int64_t oldest_time = INT64_MAX;
	int64_t now = k_uptime_get();

	if (tkl > COAP_TOKEN_MAX_LEN) {
		return -EINVAL;
	}

	/* Check if entry already exists */
	entry = oscore_exchange_find(cache, addr, addr_len, token, tkl);
	if (entry != NULL) {
		/* Update existing entry */
		entry->timestamp = now;
		entry->is_observe = is_observe;
		return 0;
	}

	/* Find empty or oldest entry */
	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		if (cache[i].addr_len == 0) {
			entry = &cache[i];
			break;
		}
		if (cache[i].timestamp < oldest_time) {
			oldest_time = cache[i].timestamp;
			oldest = &cache[i];
		}
	}

	/* Use empty entry or evict oldest */
	if (entry == NULL) {
		if (oldest != NULL) {
			entry = oldest;
			memset(entry, 0, sizeof(*entry));
		} else {
			return -ENOMEM;
		}
	}

	/* Populate entry */
	memcpy(&entry->addr, addr, addr_len);
	entry->addr_len = addr_len;
	memcpy(entry->token, token, tkl);
	entry->tkl = tkl;
	entry->timestamp = now;
	entry->is_observe = is_observe;
	entry->oscore_ctx = oscore_ctx;

	return 0;
}

/* Remove OSCORE exchange entry */
#if defined(CONFIG_COAP_TEST_API_ENABLE)
void oscore_exchange_remove(
#else
static void oscore_exchange_remove(
#endif
				   struct coap_oscore_exchange *cache,
				   const struct net_sockaddr *addr,
				   net_socklen_t addr_len,
				   const uint8_t *token,
				   uint8_t tkl)
{
	struct coap_oscore_exchange *entry;

	entry = oscore_exchange_find(cache, addr, addr_len, token, tkl);
	if (entry != NULL) {
		memset(entry, 0, sizeof(*entry));
	}
}

#endif /* CONFIG_COAP_OSCORE */

static int coap_service_remove_observer(const struct coap_service *service,
					struct coap_resource *resource,
					const struct net_sockaddr *addr,
					const uint8_t *token, uint8_t tkl)
{
	struct coap_observer *obs;

	if (tkl > 0 && addr != NULL) {
		/* Prefer addr+token to find the observer */
		obs = coap_find_observer(service->data->observers, MAX_OBSERVERS, addr, token, tkl);
	} else if (tkl > 0) {
		/* Then try to find the observer by token */
		obs = coap_find_observer_by_token(service->data->observers, MAX_OBSERVERS, token,
						  tkl);
	} else if (addr != NULL) {
		obs = coap_find_observer_by_addr(service->data->observers, MAX_OBSERVERS, addr);
	} else {
		/* Either a token or an address is required */
		return -EINVAL;
	}

	if (obs == NULL) {
		return 0;
	}

	if (resource == NULL) {
		COAP_SERVICE_FOREACH_RESOURCE(service, it) {
			if (coap_remove_observer(it, obs)) {
#if defined(CONFIG_COAP_OSCORE)
				/* RFC 8613 Section 8.3/8.4: Remove OSCORE exchange when observer removed */
				if (service->data->oscore_ctx != NULL) {
					net_socklen_t obs_addr_len = ADDRLEN(&obs->addr);
					oscore_exchange_remove(service->data->oscore_exchange_cache,
							       &obs->addr, obs_addr_len,
							       obs->token, obs->tkl);
				}
#endif
				memset(obs, 0, sizeof(*obs));
				return 1;
			}
		}
	} else if (coap_remove_observer(resource, obs)) {
#if defined(CONFIG_COAP_OSCORE)
		/* RFC 8613 Section 8.3/8.4: Remove OSCORE exchange when observer removed */
		if (service->data->oscore_ctx != NULL) {
			net_socklen_t obs_addr_len = ADDRLEN(&obs->addr);
			oscore_exchange_remove(service->data->oscore_exchange_cache,
					       &obs->addr, obs_addr_len,
					       obs->token, obs->tkl);
		}
#endif
		memset(obs, 0, sizeof(*obs));
		return 1;
	}

	return 0;
}

static int coap_server_process(int sock_fd)
{
	static uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];

	struct net_sockaddr client_addr;
	net_socklen_t client_addr_len = sizeof(client_addr);
	struct coap_service *service = NULL;
	struct coap_packet request;
	struct coap_pending *pending;
	struct coap_option options[MAX_OPTIONS] = { 0 };
	uint8_t opt_num = MAX_OPTIONS;
	uint8_t type;
	ssize_t received;
	int ret;
	int flags = ZSOCK_MSG_DONTWAIT;

	if (IS_ENABLED(CONFIG_COAP_SERVER_TRUNCATE_MSGS)) {
		flags |= ZSOCK_MSG_TRUNC;
	}

	received = zsock_recvfrom(sock_fd, buf, sizeof(buf), flags, &client_addr, &client_addr_len);

	if (received < 0) {
		if (errno == EWOULDBLOCK) {
			return 0;
		}

		LOG_ERR("Failed to process client request (%d)", -errno);
		return -errno;
	}

	ret = coap_packet_parse(&request, buf, MIN(received, sizeof(buf)), options, opt_num);
	if (ret < 0) {
		LOG_ERR("Failed To parse coap message (%d)", ret);
		return ret;
	}

	/* RFC 7252 Section 5.4.1: Check for unsupported critical options before processing.
	 * This must happen before any OSCORE-specific logic to ensure fail-closed behavior.
	 */
	uint16_t unsupported_opt;

	ret = coap_check_unsupported_critical_options(&request, &unsupported_opt);
	if (ret == -ENOTSUP) {
		/* RFC 7252 Section 5.4.1: Handle unrecognized critical option */
		uint8_t msg_type = coap_header_get_type(&request);

		LOG_WRN("Unsupported critical option %u in message", unsupported_opt);

		if (coap_packet_is_request(&request)) {
			if (msg_type == COAP_TYPE_CON) {
				/* RFC 7252 Section 5.4.1: CON request with unrecognized critical
				 * option MUST return 4.02 (Bad Option) response.
				 */
				struct coap_packet response;
				uint8_t response_buf[COAP_TOKEN_MAX_LEN + 4U];

				ret = coap_ack_init(&response, &request, response_buf,
						    sizeof(response_buf),
						    COAP_RESPONSE_CODE_BAD_OPTION);
				if (ret < 0) {
					LOG_ERR("Failed to init Bad Option response (%d)", ret);
					return ret;
				}

				ret = zsock_sendto(sock_fd, response.data, response.offset, 0,
						   &client_addr, client_addr_len);
				if (ret < 0) {
					LOG_ERR("Failed to send Bad Option response (%d)", -errno);
					return -errno;
				}

				LOG_DBG("Sent 4.02 Bad Option for unsupported critical option %u",
					unsupported_opt);
				return 0;
			} else {
				/* RFC 7252 Section 5.4.1: NON request with unrecognized critical
				 * option MUST be rejected (silently dropped, optionally send RST).
				 * We choose to silently drop as per RFC 7252 Section 4.3.
				 */
				LOG_DBG("Rejected NON request with unsupported critical option %u",
					unsupported_opt);
				return 0;
			}
		} else {
			/* RFC 7252 Section 5.4.1: Response with unrecognized critical option
			 * MUST be rejected. Since this is the server, we shouldn't normally
			 * receive responses, but handle it defensively.
			 */
			if (msg_type == COAP_TYPE_CON) {
				/* Send RST for CON response */
				struct coap_packet rst;
				uint8_t rst_buf[COAP_FIXED_HEADER_SIZE + COAP_TOKEN_MAX_LEN];

				ret = coap_rst_init(&rst, &request, rst_buf, sizeof(rst_buf));
				if (ret < 0) {
					LOG_ERR("Failed to init RST (%d)", ret);
					return ret;
				}

				ret = zsock_sendto(sock_fd, rst.data, rst.offset, 0,
						   &client_addr, client_addr_len);
				if (ret < 0) {
					LOG_ERR("Failed to send RST (%d)", -errno);
					return -errno;
				}

				LOG_DBG("Sent RST for response with unsupported critical option %u",
					unsupported_opt);
			}
			/* For NON/ACK responses, silently drop */
			return 0;
		}
	}

	/* RFC 8768 Section 3: Validate Hop-Limit option if present.
	 * This validation must happen on the outer message before OSCORE/EDHOC processing.
	 * Requests with invalid Hop-Limit MUST be rejected with 4.00 (Bad Request).
	 */
	{
		uint8_t hop_limit;
		int hop_ret = coap_get_hop_limit(&request, &hop_limit);

		if (hop_ret < 0 && hop_ret != -ENOENT) {
			/* RFC 8768 Section 3: Invalid Hop-Limit (wrong length or value 0) */
			uint8_t msg_type = coap_header_get_type(&request);

			LOG_ERR("Invalid Hop-Limit option in request");

			if (coap_packet_is_request(&request) && msg_type == COAP_TYPE_CON) {
				struct coap_packet response;
				uint8_t response_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
				uint8_t token[COAP_TOKEN_MAX_LEN];
				uint8_t tkl = coap_header_get_token(&request, token);
				uint16_t id = coap_header_get_id(&request);

				ret = coap_packet_init(&response, response_buf, sizeof(response_buf),
						       COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
						       COAP_RESPONSE_CODE_BAD_REQUEST, id);
				if (ret == 0) {
					(void)zsock_sendto(sock_fd, response.data, response.offset, 0,
							   &client_addr, client_addr_len);
				}
			}
			return -EINVAL;
		}
		/* If hop_ret == -ENOENT, Hop-Limit is absent, which is valid for endpoints */
	}

	(void)k_mutex_lock(&lock, K_FOREVER);
	/* Find the active service */
	COAP_SERVICE_FOREACH(svc) {
		if (svc->data->sock_fd == sock_fd) {
			service = svc;
			break;
		}
	}
	if (service == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

#if defined(CONFIG_COAP_OSCORE)
	/* RFC 8613 Section 2: Validate OSCORE message format */
	ret = coap_oscore_validate_msg(&request);
	if (ret < 0) {
		/* Malformed OSCORE message - reject per RFC 8613 Section 2 */
		LOG_ERR("Malformed OSCORE message");
		ret = -EBADMSG;
		goto unlock;
	}

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
	/* RFC 9668 Section 3.3.1: Handle EDHOC+OSCORE combined requests */
	if (coap_edhoc_msg_has_edhoc(&request)) {
		/* RFC 9668 Section 3.3.1 Step 1: EDHOC option requires OSCORE option */
		if (!coap_oscore_msg_has_oscore(&request)) {
			LOG_ERR("EDHOC option present without OSCORE option");
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			ret = -EINVAL;
			goto unlock;
		}

		/* RFC 9668 Section 3.3.1 Steps 2-3: Split combined payload */
		uint16_t payload_len;
		const uint8_t *payload = coap_packet_get_payload(&request, &payload_len);

		if (payload == NULL || payload_len == 0) {
			LOG_ERR("EDHOC+OSCORE request missing combined payload");
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			ret = -EINVAL;
			goto unlock;
		}

		/* Check combined payload size limit */
		if (payload_len > CONFIG_COAP_EDHOC_MAX_COMBINED_PAYLOAD_LEN) {
			LOG_ERR("EDHOC+OSCORE combined payload too large (%u > %d)",
				payload_len, CONFIG_COAP_EDHOC_MAX_COMBINED_PAYLOAD_LEN);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			ret = -EINVAL;
			goto unlock;
		}

		struct coap_edhoc_span edhoc_msg3, oscore_payload;

		ret = coap_edhoc_split_comb_payload(payload, payload_len,
						    &edhoc_msg3, &oscore_payload);
		if (ret < 0) {
			LOG_ERR("Failed to split EDHOC+OSCORE combined payload (%d)", ret);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		LOG_DBG("EDHOC+OSCORE combined request: EDHOC_MSG_3=%zu bytes, "
			"OSCORE_PAYLOAD=%zu bytes", edhoc_msg3.len, oscore_payload.len);

		/* RFC 9668 Section 3.3.1 Steps 4-9: Process EDHOC+OSCORE combined request */

		/* Step 3: Extract C_R from OSCORE option 'kid' field */
		uint8_t c_r[16];
		size_t c_r_len = sizeof(c_r);

		ret = coap_oscore_option_extract_kid(&request, c_r, &c_r_len);
		if (ret < 0) {
			LOG_ERR("Failed to extract C_R from OSCORE kid (%d)", ret);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		LOG_DBG("Extracted C_R from OSCORE kid: %zu bytes", c_r_len);

		/* Step 4: Retrieve EDHOC session by C_R */
		struct coap_edhoc_session *edhoc_session;

		edhoc_session = coap_edhoc_session_find(
			service->data->edhoc_session_cache,
			CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
			c_r, c_r_len);

		if (edhoc_session == NULL) {
			LOG_ERR("No EDHOC session found for C_R");
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			ret = -ENOENT;
			goto unlock;
		}

		/* RFC 9668 Section 3.3.1 Step 4: Check if message_4 is required */
		if (edhoc_session->message_4_required) {
			LOG_ERR("EDHOC session requires message_4, cannot use combined request");
			/* RFC 9668 Section 3.3.1: Abort EDHOC and send EDHOC error message */
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_edhoc_error_response(service, &request,
							COAP_RESPONSE_CODE_BAD_REQUEST,
							1, "EDHOC error",
							&client_addr, client_addr_len);
			ret = -EINVAL;
			goto unlock;
		}

		/* RFC 9668 Section 3.3.1 Step 4: Process EDHOC message_3 per RFC 9528 Section 5.4.3
		 * This derives PRK_out and extracts C_I from the EDHOC handshake.
		 */
		uint8_t prk_out[64]; /* Max hash size */
		size_t prk_out_len = sizeof(prk_out);
		uint8_t initiator_pk[64]; /* Max public key size */
		size_t initiator_pk_len = sizeof(initiator_pk);
		uint8_t c_i[16]; /* Connection identifier C_I */
		size_t c_i_len = sizeof(c_i);

		/* Note: For testing without CONFIG_UEDHOC, the wrapper will return -ENOTSUP.
		 * Tests can override the wrapper to inject test behavior.
		 */
		ret = coap_edhoc_msg3_process_wrapper(
			edhoc_msg3.ptr, edhoc_msg3.len,
			edhoc_session->resp_ctx,
			edhoc_session->runtime_ctx,
			NULL, /* cred_i_array - application provides trust anchors */
			prk_out, &prk_out_len,
			initiator_pk, &initiator_pk_len,
			c_i, &c_i_len);

		if (ret < 0) {
			LOG_ERR("EDHOC message_3 processing failed (%d)", ret);
			/* RFC 9668 Section 3.3.1: If Step 4 (EDHOC processing) fails,
			 * abort EDHOC and send EDHOC error message (not OSCORE-protected).
			 * Per RFC 9528 Section 6.2 and Appendix A.2.3:
			 * - Response code: 4.00 (Bad Request) for client error
			 * - Content-Format: application/edhoc+cbor-seq (64)
			 * - Payload: CBOR Sequence with ERR_CODE=1, ERR_INFO=tstr
			 */
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_edhoc_error_response(service, &request,
							COAP_RESPONSE_CODE_BAD_REQUEST,
							1, "EDHOC error",
							&client_addr, client_addr_len);
			goto unlock;
		}

		LOG_DBG("EDHOC message_3 processed successfully, C_I extracted (%zu bytes)", c_i_len);

		/* RFC 9668 Section 3.3.1 Step 5: Derive OSCORE Security Context per RFC 9528 Appendix A.1
		 * "The EDHOC Exporter Labels for deriving the OSCORE Master Secret and OSCORE Master Salt
		 * are the uints 0 and 1, respectively." (RFC 9528 Appendix A.1)
		 * "The context parameter is h'' (0x40), the empty CBOR byte string." (RFC 9528 Appendix A.1)
		 */
		uint8_t master_secret[32];
		uint8_t master_salt[16];
		size_t master_secret_len = 16; /* Default OSCORE key length */
		size_t master_salt_len = 8;    /* Default OSCORE salt length (RFC 9528 Appendix A.1) */

		/* Get application hash algorithm from EDHOC suite for exporter */
		int app_hash_alg = -16; /* SHA-256 (default) */
#if defined(CONFIG_UEDHOC)
		if (edhoc_session->runtime_ctx != NULL) {
			struct runtime_context *rc = (struct runtime_context *)edhoc_session->runtime_ctx;
			app_hash_alg = rc->suite.app_hash;
		}
#endif

		/* RFC 9528 Appendix A.1: Derive master secret using EDHOC exporter with label 0 */
		ret = coap_edhoc_exporter_wrapper(prk_out, prk_out_len, app_hash_alg, 0,
						  master_secret, &master_secret_len);
		if (ret < 0) {
			LOG_ERR("Failed to derive OSCORE master secret (%d)", ret);
			/* Zeroize secrets */
			memset(prk_out, 0, sizeof(prk_out));
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_INTERNAL_ERROR,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		/* RFC 9528 Appendix A.1: Derive master salt using EDHOC exporter with label 1 */
		ret = coap_edhoc_exporter_wrapper(prk_out, prk_out_len, app_hash_alg, 1,
						  master_salt, &master_salt_len);

		/* Zeroize PRK_out after deriving keying material */
		memset(prk_out, 0, sizeof(prk_out));

		if (ret < 0) {
			LOG_ERR("Failed to derive OSCORE master salt (%d)", ret);
			/* Zeroize master secret */
			memset(master_secret, 0, sizeof(master_secret));
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_INTERNAL_ERROR,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		/* Allocate OSCORE context cache entry (keyed by C_R for lookup) */
		struct coap_oscore_ctx_cache_entry *ctx_entry;

		ctx_entry = coap_oscore_ctx_cache_insert(
			service->data->oscore_ctx_cache,
			CONFIG_COAP_OSCORE_CTX_CACHE_SIZE,
			c_r, c_r_len);

		if (ctx_entry == NULL) {
			LOG_ERR("Failed to allocate OSCORE context cache entry");
			/* Zeroize keying material */
			memset(master_secret, 0, sizeof(master_secret));
			memset(master_salt, 0, sizeof(master_salt));
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_INTERNAL_ERROR,
						  &client_addr, client_addr_len);
			ret = -ENOMEM;
			goto unlock;
		}

		/* Allocate OSCORE context from internal pool */
#if defined(CONFIG_UOSCORE)
		if (ctx_entry->oscore_ctx == NULL) {
			ctx_entry->oscore_ctx = coap_oscore_ctx_alloc();
			if (ctx_entry->oscore_ctx == NULL) {
				LOG_ERR("Failed to allocate OSCORE context from pool");
				/* Zeroize keying material */
				memset(master_secret, 0, sizeof(master_secret));
				memset(master_salt, 0, sizeof(master_salt));
				coap_edhoc_session_remove(service->data->edhoc_session_cache,
							  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
							  c_r, c_r_len);
				(void)send_error_response(service, &request,
							  COAP_RESPONSE_CODE_INTERNAL_ERROR,
							  &client_addr, client_addr_len);
				ret = -ENOMEM;
				goto unlock;
			}
		}
#else
		/* When CONFIG_UOSCORE=n, tests must provide a mock context */
		if (ctx_entry->oscore_ctx == NULL) {
			LOG_ERR("OSCORE context not allocated (tests must provide)");
			/* Zeroize keying material */
			memset(master_secret, 0, sizeof(master_secret));
			memset(master_salt, 0, sizeof(master_salt));
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_INTERNAL_ERROR,
						  &client_addr, client_addr_len);
			ret = -ENOMEM;
			goto unlock;
		}
#endif

		/* RFC 9528 Appendix A.1 Table 14: Initialize OSCORE context with correct ID mapping
		 * "EDHOC Responder: OSCORE Sender ID = C_I; OSCORE Recipient ID = C_R"
		 * This is the FIX for the non-compliance issue!
		 */
		int aead_alg = 10;  /* AES-CCM-16-64-128 (default) */
		int hkdf_alg = 5;   /* HKDF-SHA-256 (default) */
#if defined(CONFIG_UEDHOC)
		if (edhoc_session->runtime_ctx != NULL) {
			struct runtime_context *rc = (struct runtime_context *)edhoc_session->runtime_ctx;
			aead_alg = rc->suite.app_aead;
			/* RFC 9528 Appendix A.1: HKDF based on application hash algorithm
			 * Map COSE hash algorithm to HKDF algorithm:
			 * -16 (SHA-256) -> 5 (HKDF-SHA-256)
			 * -43 (SHA-384) -> 6 (HKDF-SHA-384)
			 * -44 (SHA-512) -> 7 (HKDF-SHA-512)
			 */
			switch (rc->suite.app_hash) {
			case -16:
				hkdf_alg = 5; /* HKDF-SHA-256 */
				break;
			case -43:
				hkdf_alg = 6; /* HKDF-SHA-384 */
				break;
			case -44:
				hkdf_alg = 7; /* HKDF-SHA-512 */
				break;
			default:
				hkdf_alg = 5; /* Default to HKDF-SHA-256 */
				break;
			}
		}
#endif

		ret = coap_oscore_context_init_wrapper(
			ctx_entry->oscore_ctx,
			master_secret, master_secret_len,
			master_salt, master_salt_len,
			c_i, c_i_len,  /* Sender ID = C_I (RFC 9528 Table 14) */
			c_r, c_r_len,  /* Recipient ID = C_R (RFC 9528 Table 14) */
			aead_alg,
			hkdf_alg);

		/* Zeroize keying material after initialization */
		memset(master_secret, 0, sizeof(master_secret));
		memset(master_salt, 0, sizeof(master_salt));

		if (ret < 0) {
			LOG_ERR("Failed to initialize OSCORE context (%d)", ret);
			coap_edhoc_session_remove(service->data->edhoc_session_cache,
						  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
						  c_r, c_r_len);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_INTERNAL_ERROR,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		LOG_DBG("OSCORE context derived and cached (Sender ID=C_I, Recipient ID=C_R)");

		/* EDHOC session complete, remove it */
		coap_edhoc_session_remove(service->data->edhoc_session_cache,
					  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
					  c_r, c_r_len);

		/* Steps 6-7: Rebuild OSCORE-protected request without EDHOC option */
		static uint8_t rebuilt_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		struct coap_packet rebuilt_request;

		/* Build new packet with OSCORE_PAYLOAD replacing combined payload */
		size_t rebuilt_len = request.offset - payload_len + oscore_payload.len;

		if (rebuilt_len > sizeof(rebuilt_buf)) {
			LOG_ERR("Rebuilt request too large (%zu > %zu)",
				rebuilt_len, sizeof(rebuilt_buf));
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_REQUEST_TOO_LARGE,
						  &client_addr, client_addr_len);
			ret = -ENOMEM;
			goto unlock;
		}

		/* Copy header and options (up to payload marker) */
		size_t header_len = request.offset - payload_len - 1; /* -1 for 0xFF marker */

		memcpy(rebuilt_buf, buf, header_len);
		/* Add payload marker */
		rebuilt_buf[header_len] = 0xFF;
		/* Copy OSCORE_PAYLOAD */
		memcpy(rebuilt_buf + header_len + 1, oscore_payload.ptr, oscore_payload.len);

		/* Re-parse the rebuilt packet */
		ret = coap_packet_parse(&rebuilt_request, rebuilt_buf, rebuilt_len,
					options, opt_num);
		if (ret < 0) {
			LOG_ERR("Failed to parse rebuilt request (%d)", ret);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		/* Remove EDHOC option per RFC 9668 Section 3.3.1 Step 7 */
		ret = coap_edhoc_remove_option(&rebuilt_request);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Failed to remove EDHOC option (%d)", ret);
			(void)send_error_response(service, &request,
						  COAP_RESPONSE_CODE_BAD_REQUEST,
						  &client_addr, client_addr_len);
			goto unlock;
		}

		/* Get the updated buffer after EDHOC option removal */
		rebuilt_len = rebuilt_request.offset;

		/* Step 8: Verify and decrypt OSCORE using derived context */
		static uint8_t decrypted_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		uint32_t decrypted_len = sizeof(decrypted_buf);
		uint8_t error_code = COAP_RESPONSE_CODE_BAD_REQUEST;

		ret = coap_oscore_verify_wrapper(rebuilt_buf, rebuilt_len,
						 decrypted_buf, &decrypted_len,
						 ctx_entry->oscore_ctx, &error_code);
		if (ret < 0) {
			LOG_ERR("OSCORE verification failed (%d), sending error %d",
				ret, error_code);
			(void)send_oscore_error_response(service, &request, error_code,
							 &client_addr, client_addr_len);
			ret = -EACCES;
			goto unlock;
		}

		/* Re-parse the decrypted CoAP message */
		ret = coap_packet_parse(&request, decrypted_buf, decrypted_len,
					options, opt_num);
		if (ret < 0) {
			LOG_ERR("Failed to parse decrypted CoAP message (%d)", ret);
			goto unlock;
		}

		/* Copy decrypted message back to buf for further processing */
		memcpy(buf, decrypted_buf, decrypted_len);
		received = decrypted_len;

		LOG_DBG("EDHOC+OSCORE combined request processed successfully");

		/* RFC 9668 Section 3.3.1: Track OSCORE exchange for response protection
		 * RFC 8613 Section 8.3: Response MUST be OSCORE-protected using derived context
		 */
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl = coap_header_get_token(&request, token);
		bool is_observe = coap_request_is_observe(&request);

		ret = oscore_exchange_add(service->data->oscore_exchange_cache,
					  &client_addr, client_addr_len,
					  token, tkl, is_observe,
					  ctx_entry->oscore_ctx);
		if (ret < 0) {
			LOG_WRN("Failed to add OSCORE exchange entry (%d)", ret);
			/* Continue processing - this is not a fatal error */
		}

		/* Step 9: Continue with normal request processing
		 * Skip the normal OSCORE processing block since we already decrypted
		 */
		goto dispatch_request;
	}
#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

	/* RFC 8613 Section 8.2: Verify and decrypt OSCORE-protected requests */
	if (coap_oscore_msg_has_oscore(&request)) {
		static uint8_t decrypted_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		uint32_t decrypted_len = sizeof(decrypted_buf);
		uint8_t error_code = COAP_RESPONSE_CODE_BAD_REQUEST;
		struct context *oscore_ctx = service->data->oscore_ctx;

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
		/* RFC 9668: For EDHOC-derived contexts, look up in cache by kid */
		if (oscore_ctx == NULL) {
			uint8_t kid[16];
			size_t kid_len = sizeof(kid);

			ret = coap_oscore_option_extract_kid(&request, kid, &kid_len);
			if (ret == 0) {
				struct coap_oscore_ctx_cache_entry *ctx_entry;

				ctx_entry = coap_oscore_ctx_cache_find(
					service->data->oscore_ctx_cache,
					CONFIG_COAP_OSCORE_CTX_CACHE_SIZE,
					kid, kid_len);

				if (ctx_entry != NULL) {
					oscore_ctx = ctx_entry->oscore_ctx;
					LOG_DBG("Using cached OSCORE context for kid");
				}
			}
		}
#endif

		if (oscore_ctx == NULL) {
			/* RFC 8613 Section 8.2 step 2 bullet 2: Security context not found => 4.01 */
			LOG_WRN("OSCORE message received but no context configured or cached");
			(void)send_oscore_error_response(service, &request,
							 COAP_RESPONSE_CODE_UNAUTHORIZED,
							 &client_addr, client_addr_len);
			ret = -ENOTSUP;
			goto unlock;
		}

		/* Decrypt the OSCORE message */
		ret = coap_oscore_verify_wrapper(buf, received, decrypted_buf, &decrypted_len,
						 oscore_ctx, &error_code);
		if (ret < 0) {
			/* RFC 8613 Section 8.2: OSCORE errors are sent as simple CoAP
			 * responses without OSCORE processing
			 */
			LOG_ERR("OSCORE verification failed (%d), sending error %d", ret,
				error_code);
			(void)send_oscore_error_response(service, &request, error_code,
							 &client_addr, client_addr_len);
			ret = -EACCES;
			goto unlock;
		}

		/* Re-parse the decrypted CoAP message */
		ret = coap_packet_parse(&request, decrypted_buf, decrypted_len, options, opt_num);
		if (ret < 0) {
			LOG_ERR("Failed to parse decrypted CoAP message (%d)", ret);
			goto unlock;
		}

		/* Copy decrypted message back to buf for further processing */
		memcpy(buf, decrypted_buf, decrypted_len);
		received = decrypted_len;

		LOG_DBG("OSCORE request verified and decrypted");

		/* RFC 8613 Section 8.3: Track OSCORE exchanges to protect responses */
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl = coap_header_get_token(&request, token);
		bool is_observe = coap_request_is_observe(&request);

		ret = oscore_exchange_add(service->data->oscore_exchange_cache,
					  &client_addr, client_addr_len,
					  token, tkl, is_observe,
					  oscore_ctx);
		if (ret < 0) {
			LOG_WRN("Failed to add OSCORE exchange entry (%d)", ret);
			/* Continue processing - this is not a fatal error */
		}
	} else if (service->data->require_oscore && !coap_oscore_msg_has_oscore(&request)) {
		/* Service requires OSCORE but request is not protected */
		LOG_WRN("Service requires OSCORE but request is not protected");
		(void)send_error_response(service, &request, COAP_RESPONSE_CODE_UNAUTHORIZED,
					  &client_addr, client_addr_len);
		ret = -EACCES;
		goto unlock;
	}
#endif /* CONFIG_COAP_OSCORE */

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
dispatch_request:
#endif

	type = coap_header_get_type(&request);

	if (received > sizeof(buf)) {
		/* The message was truncated and can't be processed further */
		struct coap_packet response;
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl = coap_header_get_token(&request, token);
		uint16_t id = coap_header_get_id(&request);
		bool suppress = false;

		/* Check if response should be suppressed per RFC 7967 */
		ret = coap_no_response_check(&request, COAP_RESPONSE_CODE_REQUEST_TOO_LARGE,
					     &suppress);
		if (ret < 0 && ret != -ENOENT) {
			/* Invalid No-Response option - send 4.02 Bad Option */
			LOG_WRN("Invalid No-Response option in truncated request");
			suppress = false;
		}

		if (suppress) {
			/* Response suppressed, but send empty ACK for CON requests */
			if (type == COAP_TYPE_CON) {
				ret = coap_packet_init(&response, buf, sizeof(buf),
						       COAP_VERSION_1, COAP_TYPE_ACK, tkl,
						       token, COAP_CODE_EMPTY, id);
				if (ret < 0) {
					LOG_ERR("Failed to init empty ACK (%d)", ret);
					goto unlock;
				}

				ret = coap_service_send(service, &response, &client_addr,
							client_addr_len, NULL);
				if (ret < 0) {
					LOG_ERR("Failed to send empty ACK (%d)", ret);
					goto unlock;
				}
			}
			/* For NON requests, send nothing */
			goto unlock;
		}

		/* Response not suppressed, send error response */
		if (type == COAP_TYPE_CON) {
			type = COAP_TYPE_ACK;
		} else {
			type = COAP_TYPE_NON_CON;
		}

		ret = coap_packet_init(&response, buf, sizeof(buf), COAP_VERSION_1, type, tkl,
				       token, COAP_RESPONSE_CODE_REQUEST_TOO_LARGE, id);
		if (ret < 0) {
			LOG_ERR("Failed to init response (%d)", ret);
			goto unlock;
		}

		ret = coap_append_option_int(&response, COAP_OPTION_SIZE1,
					     CONFIG_COAP_SERVER_MESSAGE_SIZE);
		if (ret < 0) {
			LOG_ERR("Failed to add SIZE1 option (%d)", ret);
			goto unlock;
		}

		ret = coap_service_send(service, &response, &client_addr, client_addr_len, NULL);
		if (ret < 0) {
			LOG_ERR("Failed to reply \"Request Entity Too Large\" (%d)", ret);
			goto unlock;
		}

		goto unlock;
	}

	pending = coap_pending_received(&request, service->data->pending, MAX_PENDINGS);
	if (pending) {
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl;

		switch (type) {
		case COAP_TYPE_RESET:
			tkl = coap_header_get_token(&request, token);
			coap_service_remove_observer(service, NULL, &client_addr, token, tkl);
			__fallthrough;
		case COAP_TYPE_ACK:
			coap_server_free(pending->data);
			coap_pending_clear(pending);
			break;
		default:
			LOG_WRN("Unexpected pending type %d", type);
			ret = -EINVAL;
			goto unlock;
		}

		goto unlock;
	} else if (type == COAP_TYPE_ACK || type == COAP_TYPE_RESET) {
		LOG_WRN("Unexpected type %d without pending packet", type);
		ret = -EINVAL;
		goto unlock;
	}

#if defined(CONFIG_COAP_SERVER_ECHO)
	/* Echo option processing per RFC 9175 */
	{
		uint8_t echo_value[40];
		size_t echo_len = 0;
		uint8_t code = coap_header_get_code(&request);
		bool needs_echo = false;
		bool echo_verified = false;
		int echo_ret;

		/* Try to extract and verify Echo option from request */
		echo_ret = echo_extract_from_request(&request, echo_value, &echo_len);
		if (echo_ret == 0) {
			/* Echo present - verify it */
			echo_ret = echo_verify_value(service->data->echo_cache,
						     &client_addr, client_addr_len,
						     echo_value, echo_len);
			if (echo_ret == 0) {
				echo_verified = true;
			} else {
				/* Echo verification failed - send new challenge */
				LOG_DBG("Echo verification failed (%d), sending new challenge",
					echo_ret);
				needs_echo = true;
			}
		} else if (echo_ret == -EINVAL) {
			/* Invalid Echo option - treat as unverifiable per RFC 9175 */
			LOG_DBG("Invalid Echo option, sending new challenge");
			needs_echo = true;
		}

		/* Check if we need Echo for unsafe methods */
		if (!needs_echo && IS_ENABLED(CONFIG_COAP_SERVER_ECHO_REQUIRE_FOR_UNSAFE) &&
		    is_unsafe_method(code) && !echo_verified) {
			/* RFC 9175 Section 2.3: MUST NOT process further */
			LOG_DBG("Unsafe method requires Echo, sending challenge");
			needs_echo = true;
		}

		/* Check amplification mitigation for well-known/core */
		if (!needs_echo &&
		    IS_ENABLED(CONFIG_COAP_SERVER_ECHO_AMPLIFICATION_MITIGATION) &&
		    IS_ENABLED(CONFIG_COAP_SERVER_WELL_KNOWN_CORE) &&
		    code == COAP_METHOD_GET &&
		    coap_uri_path_match(COAP_WELL_KNOWN_CORE_PATH, options, opt_num)) {
			/* Check if address is verified */
			if (!echo_is_address_verified(service->data->echo_cache,
						      &client_addr, client_addr_len)) {
				/* Estimate response size for well-known/core */
				size_t est_response_size = 0;

				COAP_SERVICE_FOREACH_RESOURCE(service, res) {
					/* Rough estimate: path + attributes */
					if (res->path != NULL) {
						for (const char * const *p = res->path; *p; p++) {
							est_response_size += strlen(*p) + 3;
						}
					}
				}

				/* Add CoAP header overhead */
				est_response_size += 20;

				/* Check if response exceeds threshold */
				if (est_response_size >
				    CONFIG_COAP_SERVER_ECHO_MAX_INITIAL_RESPONSE_BYTES) {
					LOG_DBG("Well-known/core response too large (%zu bytes), "
						"sending Echo challenge", est_response_size);
					needs_echo = true;
				}
			}
		}

		/* Send Echo challenge if needed */
		if (needs_echo) {
			uint8_t challenge_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
			struct coap_packet challenge_response;
			uint8_t new_echo_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];
			size_t new_echo_len;

			/* Create new Echo challenge */
			ret = echo_create_challenge(service->data->echo_cache,
						    &client_addr, client_addr_len,
						    new_echo_value, &new_echo_len);
			if (ret < 0) {
				LOG_ERR("Failed to create Echo challenge (%d)", ret);
				goto unlock;
			}

			/* Build 4.01 Unauthorized response with Echo */
			ret = echo_build_challenge_response(&challenge_response, &request,
							   new_echo_value, new_echo_len,
							   challenge_buf,
							   sizeof(challenge_buf));
			if (ret < 0) {
				LOG_ERR("Failed to build Echo challenge response (%d)", ret);
				goto unlock;
			}

			/* Send the challenge */
			ret = coap_service_send(service, &challenge_response,
					       &client_addr, client_addr_len, NULL);
			if (ret < 0) {
				LOG_ERR("Failed to send Echo challenge (%d)", ret);
			}
			goto unlock;
		}
	}
#endif /* CONFIG_COAP_SERVER_ECHO */

#if defined(CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC)
	/* RFC 9528 Appendix A.2: Handle EDHOC-over-CoAP requests to /.well-known/edhoc */
	if (coap_uri_path_match(COAP_WELL_KNOWN_EDHOC_PATH, options, opt_num)) {
		ret = coap_edhoc_transport_handle_request(service, &request,
							  &client_addr, client_addr_len);
		goto unlock;
	}
#endif /* CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC */

	if (IS_ENABLED(CONFIG_COAP_SERVER_WELL_KNOWN_CORE) &&
	    coap_header_get_code(&request) == COAP_METHOD_GET &&
	    coap_uri_path_match(COAP_WELL_KNOWN_CORE_PATH, options, opt_num)) {
		uint8_t well_known_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		struct coap_packet response;
		bool suppress = false;

		/* Check if response should be suppressed per RFC 7967 */
		ret = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
		if (ret < 0 && ret != -ENOENT) {
			/* Invalid No-Response option - send 4.02 Bad Option */
			LOG_WRN("Invalid No-Response option in well-known/core request");
			suppress = false;
		}

		if (suppress) {
			/* Response suppressed, but send empty ACK for CON requests */
			if (type == COAP_TYPE_CON) {
				uint8_t token[COAP_TOKEN_MAX_LEN];
				uint8_t tkl = coap_header_get_token(&request, token);
				uint16_t id = coap_header_get_id(&request);

				ret = coap_packet_init(&response, well_known_buf,
						       sizeof(well_known_buf),
						       COAP_VERSION_1, COAP_TYPE_ACK, tkl,
						       token, COAP_CODE_EMPTY, id);
				if (ret < 0) {
					LOG_ERR("Failed to init empty ACK (%d)", ret);
					goto unlock;
				}

				ret = coap_service_send(service, &response, &client_addr,
							client_addr_len, NULL);
				if (ret < 0) {
					LOG_ERR("Failed to send empty ACK (%d)", ret);
					goto unlock;
				}
			}
			/* For NON requests, send nothing */
			ret = 0;
			goto unlock;
		}

		/* Response not suppressed, build and send well-known/core response */
		ret = coap_well_known_core_get_len(service->res_begin,
						   COAP_SERVICE_RESOURCE_COUNT(service),
						   &request, &response,
						   well_known_buf, sizeof(well_known_buf));
		if (ret < 0) {
			LOG_ERR("Failed to build well known core for %s (%d)", service->name, ret);
			goto unlock;
		}

		ret = coap_service_send(service, &response, &client_addr, client_addr_len, NULL);
	} else {
		ret = coap_handle_request_len(&request, service->res_begin,
					      COAP_SERVICE_RESOURCE_COUNT(service),
					      options, opt_num, &client_addr, client_addr_len);

		/* Translate errors to response codes */
		switch (ret) {
		case -ENOENT:
			ret = COAP_RESPONSE_CODE_NOT_FOUND;
			break;
		case -ENOTSUP:
			ret = COAP_RESPONSE_CODE_BAD_REQUEST;
			break;
		case -EPERM:
			ret = COAP_RESPONSE_CODE_NOT_ALLOWED;
			break;
		}

		/* Shortcut for replying a code without a body */
		if (ret > 0) {
			uint8_t response_code = (uint8_t)ret;
			bool suppress = false;
			int check_ret;

			/* Check if response should be suppressed per RFC 7967 */
			check_ret = coap_no_response_check(&request, response_code, &suppress);
			if (check_ret < 0 && check_ret != -ENOENT) {
				/* Invalid No-Response option - do not suppress,
				 * send 4.02 Bad Option instead
				 */
				LOG_WRN("Invalid No-Response option, sending Bad Option");
				response_code = COAP_RESPONSE_CODE_BAD_OPTION;
				suppress = false;
			}

			if (suppress) {
				/* Response suppressed, but send empty ACK for CON requests */
				if (type == COAP_TYPE_CON) {
					uint8_t ack_buf[COAP_TOKEN_MAX_LEN + 4U];
					struct coap_packet ack;

					ret = coap_ack_init(&ack, &request, ack_buf,
							    sizeof(ack_buf), COAP_CODE_EMPTY);
					if (ret < 0) {
						LOG_ERR("Failed to init empty ACK (%d)", ret);
						goto unlock;
					}

					ret = coap_service_send(service, &ack, &client_addr,
								client_addr_len, NULL);
				}
				/* For NON requests, send nothing */
			} else {
				/* Response not suppressed, send response */
				if (type == COAP_TYPE_CON) {
					/* Send ACK with response code */
					uint8_t ack_buf[COAP_TOKEN_MAX_LEN + 4U];
					struct coap_packet ack;

					ret = coap_ack_init(&ack, &request, ack_buf,
							    sizeof(ack_buf), response_code);
					if (ret < 0) {
						LOG_ERR("Failed to init ACK (%d)", ret);
						goto unlock;
					}

					ret = coap_service_send(service, &ack, &client_addr,
								client_addr_len, NULL);
				} else {
					/* Send NON response for NON requests per RFC 7967 */
					uint8_t response_buf[COAP_TOKEN_MAX_LEN + 4U];
					struct coap_packet response;
					uint8_t token[COAP_TOKEN_MAX_LEN];
					uint8_t tkl = coap_header_get_token(&request, token);
					uint16_t id = coap_next_id();

					ret = coap_packet_init(&response, response_buf,
							       sizeof(response_buf),
							       COAP_VERSION_1, COAP_TYPE_NON_CON,
							       tkl, token, response_code, id);
					if (ret < 0) {
						LOG_ERR("Failed to init NON response (%d)", ret);
						goto unlock;
					}

					ret = coap_service_send(service, &response, &client_addr,
								client_addr_len, NULL);
				}
			}
		}
	}

unlock:
	(void)k_mutex_unlock(&lock);

	return ret;
}

static void coap_server_retransmit(void)
{
	struct coap_pending *pending;
	int64_t remaining;
	int64_t now = k_uptime_get();
	int ret;

	(void)k_mutex_lock(&lock, K_FOREVER);

	COAP_SERVICE_FOREACH(service) {
		if (service->data->sock_fd < 0) {
			continue;
		}

		pending = coap_pending_next_to_expire(service->data->pending, MAX_PENDINGS);
		if (pending == NULL) {
			/* No work to be done */
			continue;
		}

		/* Check if the pending request has expired */
		remaining = pending->t0 + pending->timeout - now;
		if (remaining > 0) {
			continue;
		}

		if (coap_pending_cycle(pending)) {
			ret = zsock_sendto(service->data->sock_fd, pending->data, pending->len, 0,
					   &pending->addr, ADDRLEN(&pending->addr));
			if (ret < 0) {
				LOG_ERR("Failed to send pending retransmission for %s (%d)",
					service->name, ret);
			}
			__ASSERT_NO_MSG(ret == pending->len);
		} else {
			LOG_WRN("Packet retransmission failed for %s", service->name);

			coap_service_remove_observer(service, NULL, &pending->addr, NULL, 0U);
			coap_server_free(pending->data);
			coap_pending_clear(pending);
		}
	}

	(void)k_mutex_unlock(&lock);
}

static int coap_server_poll_timeout(void)
{
	struct coap_pending *pending;
	int64_t result = INT64_MAX;
	int64_t remaining;
	int64_t now = k_uptime_get();

	COAP_SERVICE_FOREACH(svc) {
		if (svc->data->sock_fd < -1) {
			continue;
		}

		pending = coap_pending_next_to_expire(svc->data->pending, MAX_PENDINGS);
		if (pending == NULL) {
			continue;
		}

		remaining = pending->t0 + pending->timeout - now;
		if (result > remaining) {
			result = remaining;
		}
	}

	if (result == INT64_MAX) {
		return -1;
	}

	return MAX(result, 0);
}

static void coap_server_update_services(void)
{
	if (zvfs_eventfd_write(control_sock, 1)) {
		LOG_ERR("Failed to notify server thread (%d)", errno);
	}
}

static inline bool coap_service_in_section(const struct coap_service *service)
{
	STRUCT_SECTION_START_EXTERN(coap_service);
	STRUCT_SECTION_END_EXTERN(coap_service);

	return STRUCT_SECTION_START(coap_service) <= service &&
	       STRUCT_SECTION_END(coap_service) > service;
}

static inline void coap_service_raise_event(const struct coap_service *service, uint64_t mgmt_event)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	const struct net_event_coap_service net_event = {
		.service = service,
	};

	net_mgmt_event_notify_with_info(mgmt_event, NULL, (void *)&net_event, sizeof(net_event));
#else
	ARG_UNUSED(service);

	net_mgmt_event_notify(mgmt_event, NULL);
#endif
}

int coap_service_start(const struct coap_service *service)
{
	int ret;

	uint8_t af;
	net_socklen_t len;
	struct net_sockaddr_storage addr_storage;
	union {
		struct net_sockaddr *addr;
		struct net_sockaddr_in *addr4;
		struct net_sockaddr_in6 *addr6;
	} addr_ptrs = {
		.addr = (struct net_sockaddr *)&addr_storage,
	};
	int proto = NET_IPPROTO_UDP;

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (service->data->sock_fd >= 0) {
		ret = -EALREADY;
		goto end;
	}

	/* set the default address (in6addr_any / NET_INADDR_ANY are all 0) */
	addr_storage = (struct net_sockaddr_storage){0};
	if (IS_ENABLED(CONFIG_NET_IPV6) && service->host != NULL &&
	    zsock_inet_pton(NET_AF_INET6, service->host, &addr_ptrs.addr6->sin6_addr) == 1) {
		/* if a literal IPv6 address is provided as the host, use IPv6 */
		af = NET_AF_INET6;
		len = sizeof(struct net_sockaddr_in6);

		addr_ptrs.addr6->sin6_family = NET_AF_INET6;
		addr_ptrs.addr6->sin6_port = net_htons(*service->port);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && service->host != NULL &&
		   zsock_inet_pton(NET_AF_INET, service->host, &addr_ptrs.addr4->sin_addr) == 1) {
		/* if a literal IPv4 address is provided as the host, use IPv4 */
		af = NET_AF_INET;
		len = sizeof(struct net_sockaddr_in);

		addr_ptrs.addr4->sin_family = NET_AF_INET;
		addr_ptrs.addr4->sin_port = net_htons(*service->port);
	} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
		/* prefer IPv6 if both IPv6 and IPv4 are supported */
		af = NET_AF_INET6;
		len = sizeof(struct net_sockaddr_in6);

		addr_ptrs.addr6->sin6_family = NET_AF_INET6;
		addr_ptrs.addr6->sin6_port = net_htons(*service->port);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		af = NET_AF_INET;
		len = sizeof(struct net_sockaddr_in);

		addr_ptrs.addr4->sin_family = NET_AF_INET;
		addr_ptrs.addr4->sin_port = net_htons(*service->port);
	} else {
		ret = -ENOTSUP;
		goto end;
	}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	if (service->sec_tag_list != NULL) {
		proto = NET_IPPROTO_DTLS_1_2;
	}
#endif

	service->data->sock_fd = zsock_socket(af, NET_SOCK_DGRAM, proto);
	if (service->data->sock_fd < 0) {
		ret = -errno;
		goto end;
	}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	if (service->sec_tag_list != NULL) {
		int role = ZSOCK_TLS_DTLS_ROLE_SERVER;

		ret = zsock_setsockopt(service->data->sock_fd, ZSOCK_SOL_TLS,
				       ZSOCK_TLS_SEC_TAG_LIST,
				       service->sec_tag_list, service->sec_tag_list_size);
		if (ret < 0) {
			ret = -errno;
			goto close;
		}

		ret = zsock_setsockopt(service->data->sock_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_ROLE,
				       &role, sizeof(role));
		if (ret < 0) {
			ret = -errno;
			goto close;
		}
	}
#endif

	ret = zsock_fcntl(service->data->sock_fd, ZVFS_F_SETFL, ZVFS_O_NONBLOCK);
	if (ret < 0) {
		ret = -errno;
		goto close;
	}

	ret = zsock_bind(service->data->sock_fd, addr_ptrs.addr, len);
	if (ret < 0) {
		ret = -errno;
		goto close;
	}

	if (*service->port == 0) {
		/* ephemeral port - read back the port number */
		len = sizeof(addr_storage);
		ret = zsock_getsockname(service->data->sock_fd, addr_ptrs.addr, &len);
		if (ret < 0) {
			goto close;
		}

		if (af == NET_AF_INET6) {
			*service->port = addr_ptrs.addr6->sin6_port;
		} else {
			*service->port = addr_ptrs.addr4->sin_port;
		}
	}

end:
	k_mutex_unlock(&lock);

	coap_server_update_services();

	coap_service_raise_event(service, NET_EVENT_COAP_SERVICE_STARTED);

	return ret;

close:
	(void)zsock_close(service->data->sock_fd);
	service->data->sock_fd = -1;

	k_mutex_unlock(&lock);

	return ret;
}

int coap_service_stop(const struct coap_service *service)
{
	int ret;

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (service->data->sock_fd < 0) {
		k_mutex_unlock(&lock);
		return -EALREADY;
	}

	/* Closing a socket will trigger a poll event */
	ret = zsock_close(service->data->sock_fd);
	service->data->sock_fd = -1;

	k_mutex_unlock(&lock);

	coap_service_raise_event(service, NET_EVENT_COAP_SERVICE_STOPPED);

	return ret;
}

int coap_service_is_running(const struct coap_service *service)
{
	int ret;

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ret = (service->data->sock_fd < 0) ? 0 : 1;

	k_mutex_unlock(&lock);

	return ret;
}

int coap_service_send(const struct coap_service *service, const struct coap_packet *cpkt,
		      const struct net_sockaddr *addr, net_socklen_t addr_len,
		      const struct coap_transmission_parameters *params)
{
	int ret;
	const uint8_t *send_data = cpkt->data;
	size_t send_len = cpkt->offset;

#if defined(CONFIG_COAP_OSCORE)
	/* Buffer for OSCORE-protected message (worst-case overhead).
	 * Static to avoid stack overflow for large message sizes.
	 * Safe because function is protected by mutex.
	 */
	static uint8_t oscore_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE + 128];
	uint32_t oscore_len = sizeof(oscore_buf);
	struct coap_oscore_exchange *exchange = NULL;
#endif

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);

	if (service->data->sock_fd < 0) {
		(void)k_mutex_unlock(&lock);
		return -EBADF;
	}

#if defined(CONFIG_COAP_OSCORE)
	/* RFC 8613 Section 8.3: Protect responses for OSCORE exchanges
	 * RFC 9668 Section 3.3.1: Use per-exchange OSCORE context for derived contexts
	 */
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(cpkt, token);

	/* Look up exchange to see if this response needs OSCORE protection */
	exchange = oscore_exchange_find(service->data->oscore_exchange_cache,
					addr, addr_len, token, tkl);
	if (exchange != NULL) {
		struct context *oscore_ctx = exchange->oscore_ctx;

		/* If exchange doesn't have a context, fall back to service context */
		if (oscore_ctx == NULL) {
			oscore_ctx = service->data->oscore_ctx;
		}

		if (oscore_ctx == NULL) {
			/* RFC 8613: Fail closed - do not send plaintext response */
			LOG_ERR("OSCORE exchange found but no context available");
			(void)k_mutex_unlock(&lock);
			return -ENOTSUP;
		}

		/* This response must be OSCORE-protected */
		ret = coap_oscore_protect(cpkt->data, cpkt->offset,
					  oscore_buf, &oscore_len,
					  oscore_ctx);
		if (ret < 0) {
			/* RFC 8613: Fail closed - do not send plaintext */
			LOG_ERR("OSCORE protection failed (%d), not sending response", ret);
			(void)k_mutex_unlock(&lock);
			return ret;
		}

		/* Use protected message for sending */
		send_data = oscore_buf;
		send_len = oscore_len;

		LOG_DBG("OSCORE protected response: %zu -> %zu bytes", cpkt->offset, send_len);
	}
#endif

	/*
	 * Check if we should start with retransmits, if creating a pending message fails we still
	 * try to send.
	 */
	if (coap_header_get_type(cpkt) == COAP_TYPE_CON) {
		struct coap_pending *pending = coap_pending_next_unused(service->data->pending,
									MAX_PENDINGS);

		if (pending == NULL) {
			LOG_WRN("No pending message available for %s", service->name);
			goto send;
		}

		ret = coap_pending_init(pending, cpkt, addr, params);
		if (ret < 0) {
			LOG_WRN("Failed to init pending message for %s (%d)", service->name, ret);
			goto send;
		}

		/* Replace tracked data with our allocated copy */
		pending->data = coap_server_alloc(send_len);
		if (pending->data == NULL) {
			LOG_WRN("Failed to allocate pending message data for %s", service->name);
			coap_pending_clear(pending);
			goto send;
		}
		/* Store the actual bytes to send (OSCORE-protected if applicable) */
		memcpy(pending->data, send_data, send_len);
		pending->len = send_len;

		coap_pending_cycle(pending);

		/* Trigger event in receive loop to schedule retransmit */
		coap_server_update_services();
	}

send:
#if defined(CONFIG_COAP_OSCORE)
	/* For non-Observe exchanges, remove entry after sending response */
	if (exchange != NULL && !exchange->is_observe) {
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl = coap_header_get_token(cpkt, token);

		/* Non-Observe exchange - remove after sending response */
		oscore_exchange_remove(service->data->oscore_exchange_cache,
				       addr, addr_len, token, tkl);
	}
#endif

	(void)k_mutex_unlock(&lock);

	ret = zsock_sendto(service->data->sock_fd, send_data, send_len, 0, addr, addr_len);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP message (%d)", ret);
		return ret;
	}
	__ASSERT_NO_MSG(ret == (int)send_len);

	return 0;
}

int coap_resource_send(const struct coap_resource *resource, const struct coap_packet *cpkt,
		       const struct net_sockaddr *addr, net_socklen_t addr_len,
		       const struct coap_transmission_parameters *params)
{
	/* Find owning service */
	COAP_SERVICE_FOREACH(svc) {
		if (COAP_SERVICE_HAS_RESOURCE(svc, resource)) {
			return coap_service_send(svc, cpkt, addr, addr_len, params);
		}
	}

	return -ENOENT;
}

int coap_resource_parse_observe(struct coap_resource *resource, const struct coap_packet *request,
				const struct net_sockaddr *addr)
{
	const struct coap_service *service = NULL;
	int ret;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;

	if (!coap_packet_is_request(request)) {
		return -EINVAL;
	}

	ret = coap_get_option_int(request, COAP_OPTION_OBSERVE);
	if (ret < 0) {
		return ret;
	}

	/* Find owning service */
	COAP_SERVICE_FOREACH(svc) {
		if (COAP_SERVICE_HAS_RESOURCE(svc, resource)) {
			service = svc;
			break;
		}
	}

	if (service == NULL) {
		return -ENOENT;
	}

	tkl = coap_header_get_token(request, token);
	if (tkl == 0) {
		return -EINVAL;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);

	if (ret == 0) {
		struct coap_observer *observer;

		/* RFC7641 section 4.1 - Check if the current observer already exists */
		observer = coap_find_observer(service->data->observers, MAX_OBSERVERS, addr, token,
					      tkl);
		if (observer != NULL) {
			/* Client refresh */
			goto unlock;
		}

		/* New client */
		observer = coap_observer_next_unused(service->data->observers, MAX_OBSERVERS);
		if (observer == NULL) {
			ret = -ENOMEM;
			goto unlock;
		}

		coap_observer_init(observer, request, addr);
		coap_register_observer(resource, observer);
	} else if (ret == 1) {
		ret = coap_service_remove_observer(service, resource, addr, token, tkl);
		if (ret < 0) {
			LOG_WRN("Failed to remove observer (%d)", ret);
			goto unlock;
		}

		if (ret == 0) {
			/* Observer not found */
			ret = -ENOENT;
		}
	}

unlock:
	(void)k_mutex_unlock(&lock);

	return ret;
}

static int coap_resource_remove_observer(struct coap_resource *resource,
					 const struct net_sockaddr *addr,
					 const uint8_t *token, uint8_t token_len)
{
	const struct coap_service *service = NULL;
	int ret;

	/* Find owning service */
	COAP_SERVICE_FOREACH(svc) {
		if (COAP_SERVICE_HAS_RESOURCE(svc, resource)) {
			service = svc;
			break;
		}
	}

	if (service == NULL) {
		return -ENOENT;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);
	ret = coap_service_remove_observer(service, resource, addr, token, token_len);
	(void)k_mutex_unlock(&lock);

	if (ret == 1) {
		/* An observer was found and removed */
		return 0;
	} else if (ret == 0) {
		/* No matching observer found */
		return -ENOENT;
	}

	/* An error occurred */
	return ret;
}

int coap_resource_remove_observer_by_addr(struct coap_resource *resource,
					  const struct net_sockaddr *addr)
{
	return coap_resource_remove_observer(resource, addr, NULL, 0);
}

int coap_resource_remove_observer_by_token(struct coap_resource *resource,
					   const uint8_t *token, uint8_t token_len)
{
	return coap_resource_remove_observer(resource, NULL, token, token_len);
}

static void coap_server_thread(void *p1, void *p2, void *p3)
{
	struct zsock_pollfd sock_fds[MAX_POLL_FD];
	int sock_nfds;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	control_sock = zvfs_eventfd(0, ZVFS_EFD_NONBLOCK);
	if (control_sock < 0) {
		LOG_ERR("Failed to create event fd (%d)", -errno);
		return;
	}

	COAP_SERVICE_FOREACH(svc) {
		if (svc->flags & COAP_SERVICE_AUTOSTART) {
			ret = coap_service_start(svc);
			if (ret < 0) {
				LOG_ERR("Failed to autostart service %s (%d)", svc->name, ret);
			}
		}
	}

	while (true) {
		sock_nfds = 0;
		COAP_SERVICE_FOREACH(svc) {
			if (svc->data->sock_fd < 0) {
				continue;
			}
			if (sock_nfds >= MAX_POLL_FD) {
				LOG_ERR("Maximum active CoAP services reached (%d), "
					"increase CONFIG_ZVFS_POLL_MAX to support more.",
					MAX_POLL_FD);
				break;
			}

			sock_fds[sock_nfds].fd = svc->data->sock_fd;
			sock_fds[sock_nfds].events = ZSOCK_POLLIN;
			sock_fds[sock_nfds].revents = 0;
			sock_nfds++;
		}

		/* Add event FD to allow wake up */
		if (sock_nfds < MAX_POLL_FD) {
			sock_fds[sock_nfds].fd = control_sock;
			sock_fds[sock_nfds].events = ZSOCK_POLLIN;
			sock_fds[sock_nfds].revents = 0;
			sock_nfds++;
		}

		__ASSERT_NO_MSG(sock_nfds > 0);

		ret = zsock_poll(sock_fds, sock_nfds, coap_server_poll_timeout());
		if (ret < 0) {
			LOG_ERR("Poll error (%d)", -errno);
			k_msleep(10);
		}

		for (int i = 0; i < sock_nfds; ++i) {
			/* Check the wake up event */
			if (sock_fds[i].fd == control_sock &&
			    sock_fds[i].revents & ZSOCK_POLLIN) {
				zvfs_eventfd_t tmp;

				zvfs_eventfd_read(sock_fds[i].fd, &tmp);
				continue;
			}

			/* Check if socket can receive/was closed first */
			if (sock_fds[i].revents & ZSOCK_POLLIN) {
				coap_server_process(sock_fds[i].fd);
				continue;
			}

			if (sock_fds[i].revents & ZSOCK_POLLERR) {
				LOG_ERR("Poll error on %d", sock_fds[i].fd);
			}
			if (sock_fds[i].revents & ZSOCK_POLLHUP) {
				LOG_DBG("Poll hup on %d", sock_fds[i].fd);
			}
			if (sock_fds[i].revents & ZSOCK_POLLNVAL) {
				LOG_ERR("Poll invalid on %d", sock_fds[i].fd);
			}
		}

		/* Process retransmits */
		coap_server_retransmit();
	}
}

K_THREAD_DEFINE(coap_server_id, CONFIG_COAP_SERVER_STACK_SIZE,
		coap_server_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);
