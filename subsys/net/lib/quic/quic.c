/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_quic, CONFIG_QUIC_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/quic.h>

/* TODO: Remove all direct access to private fields.
 * According with Mbed TLS migration guide:
 *
 * Direct access to fields of structures
 * (`struct` types) declared in public headers is no longer
 * supported. In Mbed TLS 3, the layout of structures is not
 * considered part of the stable API, and minor versions (3.1, 3.2,
 * etc.) may add, remove, rename, reorder or change the type of
 * structure fields.
 */
#if !defined(MBEDTLS_ALLOW_PRIVATE_ACCESS)
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif

#if defined(CONFIG_MBEDTLS)
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <mbedtls/platform.h>
#include <mbedtls/ssl_cache.h>

#include <psa/crypto.h>
#endif /* CONFIG_MBEDTLS */

#if defined(CONFIG_MBEDTLS_DEBUG)
#include <zephyr_mbedtls_priv.h>
#endif

#include "net_private.h"
#include "sockets_internal.h"
#include "tls_internal.h"
#include "quic_internal.h"
#include "quic_stats.h"

BUILD_ASSERT(CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL <= CONFIG_QUIC_STREAM_RX_BUFFER_SIZE,
	     "Flow control window must not exceed RX buffer size");

#define SLAB_ALLOC_TIMEOUT K_MSEC(500)

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
struct quic_endpoint *endpoint_alloc_from_slab_debug(struct k_mem_slab *slab,
						     k_timeout_t timeout,
						     const char *caller, int line);
#define endpoint_alloc_from_slab(_slab, _timeout)			\
	endpoint_alloc_from_slab_debug(_slab, _timeout, __func__, __LINE__)
#endif /* CONFIG_QUIC_LOG_LEVEL_DBG */

static struct quic_context contexts[CONFIG_QUIC_MAX_CONTEXTS];
static struct k_mutex contexts_lock;

static const struct socket_op_vtable quic_ctx_fd_op_vtable;
static const struct socket_op_vtable quic_stream_fd_op_vtable;

static enum quic_stream_states quic_stream_get_state(struct quic_stream *stream);
static const struct smf_state quic_stream_bidirectional_states[];

K_MEM_SLAB_DEFINE_STATIC(endpoints_slab, sizeof(struct quic_endpoint),
			 CONFIG_QUIC_MAX_ENDPOINTS, sizeof(intptr_t));
static struct quic_endpoint *endpoints[CONFIG_QUIC_MAX_ENDPOINTS];
static struct k_mutex endpoints_lock;

static struct quic_stream streams[CONFIG_QUIC_MAX_STREAMS_BIDI +
				  CONFIG_QUIC_MAX_STREAMS_UNI];
static struct k_mutex streams_lock;
ZTESTABLE_STATIC struct quic_context *quic_get_context(int sock);

static int connection_ids;

static void quic_svc_handler(struct net_socket_service_event *pev);

#if defined(CONFIG_QUIC_TLS_DEBUG_KEYLOG)
static void quic_log_tls_secret(const char *label,
				const uint8_t *client_random,
				const uint8_t *secret,
				size_t secret_len);
#else
static inline void quic_log_tls_secret(const char *label,
				       const uint8_t *client_random,
				       const uint8_t *secret,
				       size_t secret_len)
{
	ARG_UNUSED(label);
	ARG_UNUSED(client_random);
	ARG_UNUSED(secret);
	ARG_UNUSED(secret_len);
}
#endif /* CONFIG_QUIC_TLS_DEBUG_KEYLOG */

#if defined(CONFIG_NET_IPV4)
#define QUIC_IPV4_SVC_POLL_COUNT CONFIG_QUIC_MAX_ENDPOINTS
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc4, quic_svc_handler,
				      QUIC_IPV4_SVC_POLL_COUNT);
static struct zsock_pollfd quic_ipv4_pollfds[QUIC_IPV4_SVC_POLL_COUNT];
#else
#define QUIC_IPV4_SVC_POLL_COUNT 0
#endif

#if defined(CONFIG_NET_IPV6)
#define QUIC_IPV6_SVC_POLL_COUNT CONFIG_QUIC_MAX_ENDPOINTS
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc6, quic_svc_handler,
				      QUIC_IPV6_SVC_POLL_COUNT);
static struct zsock_pollfd quic_ipv6_pollfds[QUIC_IPV6_SVC_POLL_COUNT];
#else
#define QUIC_IPV6_SVC_POLL_COUNT 0
#endif

#if defined(CONFIG_NET_STATISTICS_QUIC)
static struct net_stats_quic_global quic_stats_vars;
struct net_stats_quic_global *quic_stats = &quic_stats_vars;
#endif /* CONFIG_NET_STATISTICS_QUIC */

static K_FIFO_DEFINE(quic_queue);

enum quic_header_type {
	QUIC_HEADER_TYPE_INVALID = 0,
	QUIC_HEADER_TYPE_LONG,
	QUIC_HEADER_TYPE_SHORT,
};

struct quic_pkt {
	/**
	 * The fifo is used to pass data from socket service handler
	 * to QUIC handler thread. The quic_pkt is queued via fifo
	 * to the QUIC processing thread.
	 */
	intptr_t fifo;

	/** Slab pointer from where it belongs to */
	struct k_mem_slab *slab;

	/** Endpoint information */
	struct quic_endpoint *ep;

	/** Original endpoint information (in case of server ep which
	 * accepts a new connection).
	 */
	struct quic_endpoint *old_ep;

	/** Reference counter */
	atomic_t atomic_ref;

	/** Header type */
	enum quic_header_type htype;

	/** Packet type */
	enum quic_packet_type ptype;

	/** Packet number */
	uint64_t pkt_num;

	/** Token */
	uint64_t token;

	/** Packet number offset */
	size_t pn_offset;

	/** Packet data payload offset */
	size_t pos;

	/** Packet type specific bits */
	int type_bits;

	/** Packet payload length */
	size_t len;

	/** Packet total length */
	size_t total_len;

	/** Pending datagram we have received on this endpoint but not yet
	 * processed.
	 */
	uint8_t data[CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN];
};

#define QUIC_SLAB_DEFINE(name, count) \
	K_MEM_SLAB_DEFINE_STATIC(name, sizeof(struct quic_pkt), count, sizeof(intptr_t));

QUIC_SLAB_DEFINE(quic_pkts, CONFIG_QUIC_PKT_COUNT);

/* Declaration of possible states */
enum quic_stream_states {
	STATE_READY,
	STATE_SEND,
	STATE_DATA_SENT,
	STATE_DATA_RECVD,
	STATE_RESET_SENT,
	STATE_RESET_RECVD,
	STATE_RECV,
	STATE_SIZE_KNOWN,
	STATE_DATA_READ,
	STATE_RESET_READ,
};

static int quic_put_varint(uint8_t *buf, size_t buf_len, uint64_t val);
static struct quic_context *quic_find_context(struct quic_endpoint *ep);
static struct quic_stream *quic_find_stream(struct quic_context *ctx,
					    struct quic_endpoint *ep,
					    uint64_t stream_id);
static struct quic_stream *quic_create_stream_from_peer(struct quic_context *ctx,
							struct quic_endpoint *ep,
							uint64_t stream_id);
static int quic_stream_receive_data(struct quic_stream *stream,
				    uint64_t offset,
				    const uint8_t *data,
				    size_t len,
				    bool is_fin);
static struct quic_stream *quic_find_stream_by_id(struct quic_endpoint *ep,
						  uint64_t stream_id);
static void quic_tls_cleanup(struct quic_tls_context *tls);
static struct quic_tls_context *tls_init(struct quic_endpoint *ep);
static int quic_handshake_complete(struct quic_endpoint *ep);
static void quic_stream_flush_queue(struct quic_stream *stream);
static int quic_endpoint_send_connection_close(struct quic_endpoint *ep,
					       uint64_t error_code,
					       const char *reason);
static int derive_application_secrets(struct quic_tls_context *ctx);
static int quic_send_max_data(struct quic_endpoint *ep);
static int quic_send_max_stream_data(struct quic_endpoint *ep,
				     struct quic_stream *stream);
static int quic_send_max_streams(struct quic_endpoint *ep, bool bidi);
static int quic_send_data_blocked(struct quic_endpoint *ep);
static int quic_send_stream_data_blocked(struct quic_endpoint *ep,
					 struct quic_stream *stream);
static void quic_pto_work_handler(struct k_work *work);
static void quic_reset_pto_timer(struct quic_endpoint *ep);
static int quic_send_packet_from_txbuf(struct quic_endpoint *ep,
				       enum quic_secret_level level,
				       size_t payload_len);
static int quic_send_packet(struct quic_endpoint *ep,
			    enum quic_secret_level level,
			    const uint8_t *payload,
			    size_t payload_len);
static int quic_send_stop_sending(struct quic_endpoint *ep,
				  uint64_t stream_id,
				  uint64_t error_code);

static int quic_get_by_ep(struct quic_endpoint *ep)
{
	return ep->slab_index;
}

static int quic_get_by_conn(struct quic_context *ctx)
{
	if (!IS_ARRAY_ELEMENT(contexts, ctx)) {
		return -1;
	}

	return ARRAY_INDEX(contexts, ctx);
}

static int quic_get_by_stream(struct quic_stream *stream)
{
	if (!IS_ARRAY_ELEMENT(streams, stream)) {
		return -1;
	}

	return ARRAY_INDEX(streams, stream);
}

/* Helper to zero out sensitive data in a way that prevents compiler
 * optimizations from removing the clearing.
 */
#define crypto_zero(dest, len) mbedtls_platform_zeroize(dest, len)

static void quic_pkt_unref(struct quic_pkt *pkt)
{
	atomic_val_t ref;

	if (pkt == NULL) {
		return;
	}

	do {
		ref = atomic_get(&pkt->atomic_ref);
		if (ref == 0) {
			return;
		}
	} while (!atomic_cas(&pkt->atomic_ref, ref, ref - 1));

	if (ref > 1) {
		return;
	}

	k_mem_slab_free(pkt->slab, (void *)pkt);
}

static struct quic_pkt *quic_pkt_alloc(struct k_mem_slab *slab, k_timeout_t timeout)
{
	struct quic_pkt *pkt;
	int ret;

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	ret = k_mem_slab_alloc(slab, (void **)&pkt, timeout);
	if (ret != 0) {
		return NULL;
	}

	memset(pkt, 0, sizeof(struct quic_pkt));

	pkt->atomic_ref = ATOMIC_INIT(1);
	pkt->slab = slab;

	return pkt;
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_endpoint_unref_debug(struct quic_endpoint *ep,
				     const char *caller, int line);
#define quic_endpoint_unref(ep) quic_endpoint_unref_debug(ep, __func__, __LINE__)
#else
static int quic_endpoint_unref(struct quic_endpoint *ep);
#endif

static void quic_tls_cleanup(struct quic_tls_context *tls);

static int quic_decode_len(uint8_t byte)
{
	return (int)(1 << (byte >> 6));
}

static int quic_encode_len(uint8_t len)
{
	return (int)((LOG2(len) & 0x3) << 6);
}

ZTESTABLE_STATIC int quic_get_len(const uint8_t *buf, size_t buf_len, uint64_t *len)
{
	uint32_t first_byte;

	NET_ASSERT(buf_len > 0);

	first_byte = buf[0] & 0x3f;

	switch (quic_decode_len(buf[0])) {
	case 1:
		*len = first_byte;
		return 1;

	case 2:
		if (buf_len < 2) {
			return -EINVAL;
		}

		*len = (first_byte << 8) | buf[1];
		return 2;

	case 4:
		if (buf_len < 4) {
			return -EINVAL;
		}

		*len = (first_byte << 24) | (buf[1] << 16) |
		       (buf[2] << 8) | buf[3];
		return 4;

	case 8:
		if (buf_len < 8) {
			return -EINVAL;
		}

		*len = ((uint64_t)first_byte << 56) | ((uint64_t)buf[1] << 48) |
		       ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32) |
		       ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) |
		       ((uint64_t)buf[6] << 8) | (uint64_t)buf[7];
		return 8;

	default:
		break;
	}

	return -EINVAL;
}

static int quic_get_varint_size(uint64_t val)
{
	if (val < 0x40) {
		return 1;
	} else if (val < 0x4000) {
		return 2;
	} else if (val < 0x40000000) {
		return 4;
	}

	return 8;
}

ZTESTABLE_STATIC int quic_put_len(uint8_t *buf, size_t buf_len, uint64_t len)
{
	switch (quic_get_varint_size(len)) {
	case 1:
		if (buf_len < 1) {
			return -EINVAL;
		}

		buf[0] = (uint8_t)(len & 0x3f);
		return 0;

	case 2:
		if (buf_len < 2) {
			return -EINVAL;
		}

		buf[0] = (uint8_t)(((len >> 8) & 0x3f) | quic_encode_len(2));
		buf[1] = (uint8_t)(len & 0xff);
		return 0;

	case 4:
		if (buf_len < 4) {
			return -EINVAL;
		}

		buf[0] = (uint8_t)(((len >> 24) & 0x3f) | quic_encode_len(4));
		buf[1] = (uint8_t)((len >> 16) & 0xff);
		buf[2] = (uint8_t)((len >> 8) & 0xff);
		buf[3] = (uint8_t)(len & 0xff);
		return 0;

	case 8:
		if (buf_len < 8) {
			return -EINVAL;
		}

		buf[0] = (uint8_t)(((len >> 56) & 0x3f) | quic_encode_len(8));
		buf[1] = (uint8_t)((len >> 48) & 0xff);
		buf[2] = (uint8_t)((len >> 40) & 0xff);
		buf[3] = (uint8_t)((len >> 32) & 0xff);
		buf[4] = (uint8_t)((len >> 24) & 0xff);
		buf[5] = (uint8_t)((len >> 16) & 0xff);
		buf[6] = (uint8_t)((len >> 8) & 0xff);
		buf[7] = (uint8_t)(len & 0xff);
		return 0;

	default:
		break;
	}

	return -EINVAL;
}

static int pollfd_try_add(net_sa_family_t family, int sock)
{
#if defined(CONFIG_NET_IPV4)
	if (family == NET_AF_INET) {
		ARRAY_FOR_EACH_PTR(quic_ipv4_pollfds, sockfd) {
			if (sockfd->fd == -1) {
				sockfd->fd = sock;
				sockfd->events = ZSOCK_POLLIN;

				return 0;
			}
		}
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (family == NET_AF_INET6) {
		ARRAY_FOR_EACH_PTR(quic_ipv6_pollfds, sockfd) {
			if (sockfd->fd == -1) {
				sockfd->fd = sock;
				sockfd->events = ZSOCK_POLLIN;

				return 0;
			}
		}
	}
#endif

	return -ENOMEM;
}

/*
 * Cleanup helper to call when destroying context or switching keys.
 * Destroys all PSA keys and clears sensitive data.
 */
ZTESTABLE_STATIC void quic_crypto_context_destroy(struct quic_crypto_context *ctx)
{
	if (ctx == NULL || !ctx->initialized) {
		return;
	}

	/* Destroy TX keys */
	if (ctx->tx.hp.initialized) {
		psa_destroy_key(ctx->tx.hp.key_id);
		ctx->tx.hp.initialized = false;
	}

	if (ctx->tx.pp.initialized) {
		psa_destroy_key(ctx->tx.pp.key_id);
		/* Clear IV */
		memset(ctx->tx.pp.iv, 0, sizeof(ctx->tx.pp.iv));
		ctx->tx.pp.initialized = false;
	}

	/* Destroy RX keys */
	if (ctx->rx.hp.initialized) {
		psa_destroy_key(ctx->rx.hp.key_id);
		ctx->rx.hp.initialized = false;
	}

	if (ctx->rx.pp.initialized) {
		psa_destroy_key(ctx->rx.pp.key_id);
		memset(ctx->rx.pp.iv, 0, sizeof(ctx->rx.pp.iv));
		ctx->rx.pp.initialized = false;
	}

	ctx->initialized = false;
}

/*
 * HKDF-Extract (RFC 5869 Section 2.2)
 * PRK = HMAC-Hash(salt, IKM)
 */
static int quic_hkdf_extract_ex(psa_algorithm_t hash_alg,
				const uint8_t *salt, size_t salt_len,
				const uint8_t *ikm, size_t ikm_len,
				uint8_t *prk, size_t prk_len)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = 0;
	psa_status_t status;
	size_t output_length;
	uint8_t zero_salt[QUIC_HASH_MAX_LEN] = {0};

	/* If salt is NULL/empty, use zeros of hash length */
	if (salt == NULL || salt_len == 0) {
		salt = zero_salt;
		salt_len = PSA_HASH_LENGTH(hash_alg);
	}

	/* Import salt as HMAC key */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(hash_alg));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);

	status = psa_import_key(&attributes, salt, salt_len, &key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	/* Compute HMAC: PRK = HMAC-Hash(salt, IKM) */
	status = psa_mac_compute(key_id, PSA_ALG_HMAC(hash_alg),
				 ikm, ikm_len,
				 prk, prk_len, &output_length);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS || output_length != prk_len) {
		return -EIO;
	}

	return 0;
}

/*
 * HKDF-Extract (RFC 5869 Section 2.2)
 * PRK = HMAC-Hash(salt, IKM)
 *
 * Used during initial secret derivation.
 */
ZTESTABLE_STATIC int quic_hkdf_extract(const uint8_t *salt, size_t salt_length,
				       const uint8_t *ikm, size_t ikm_length,
				       uint8_t prk[QUIC_HASH_SHA2_256_LEN])
{
	return quic_hkdf_extract_ex(PSA_ALG_SHA_256,
				    salt, salt_length,
				    ikm, ikm_length,
				    prk, QUIC_HASH_SHA2_256_LEN);
}

/* RFC 8446 Section 7.1 specifies this label length to be
 * 2 + 1 + sizeof("tls13 ") - 1 + 255 + 1 + 255
 * but for QUIC we use much shorter one since labels and context
 * are limited in size.
 */
#define MAX_LABEL_LEN 9   /* longest label is "server in" */
#define MAX_CONTEXT_LEN 0 /* QUIC always uses empty context */
#define MAX_HKDF_LABEL_LEN (2 + 1 + sizeof("tls13 ") - 1 + MAX_LABEL_LEN + 1 + MAX_CONTEXT_LEN)

/* The quic_hkdf_expand_label_ex() supports arbitrary label and context lengths,
 * but we need to define maximums to allocate the HkdfLabel buffer.
 */
#define TLS_MAX_LABEL_LEN 255
#define TLS_MAX_CONTEXT_LEN 255
#define TLS_MAX_HKDF_LABEL_LEN (2 + 1 + sizeof("tls13 ") - 1 + \
				TLS_MAX_LABEL_LEN + 1 +	       \
				TLS_MAX_CONTEXT_LEN)

/*
 * HKDF-Expand-Label with context support (RFC 8446 Section 7.1)
 *
 * HKDF-Expand-Label(Secret, Label, Context, Length) =
 *     HKDF-Expand(Secret, HkdfLabel, Length)
 */
static int quic_hkdf_expand_label_ex(psa_algorithm_t hash_alg,
				     const uint8_t *secret, size_t secret_len,
				     const uint8_t *label, size_t label_len,
				     const uint8_t *context, size_t context_len,
				     uint8_t *okm, size_t okm_len)
{
	static const char tls13_prefix[] = "tls13 ";
	const size_t prefix_len = sizeof(tls13_prefix) - 1;
	uint8_t hkdf_label[TLS_MAX_HKDF_LABEL_LEN];
	size_t hkdf_label_len = 0;
	size_t full_label_len;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = 0;
	psa_status_t status;
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

	if (label_len > TLS_MAX_LABEL_LEN || context_len > TLS_MAX_CONTEXT_LEN) {
		return -ERANGE;
	}

	/* Build HkdfLabel structure */
	/* Length (2 bytes, big-endian) */
	hkdf_label[hkdf_label_len++] = (uint8_t)((okm_len >> 8) & 0xFF);
	hkdf_label[hkdf_label_len++] = (uint8_t)(okm_len & 0xFF);

	/* Label: length byte + "tls13 " + label */
	full_label_len = prefix_len + label_len;
	hkdf_label[hkdf_label_len++] = (uint8_t)full_label_len;
	memcpy(&hkdf_label[hkdf_label_len], tls13_prefix, prefix_len);
	hkdf_label_len += prefix_len;
	memcpy(&hkdf_label[hkdf_label_len], label, label_len);
	hkdf_label_len += label_len;

	/* Context: length byte + context */
	hkdf_label[hkdf_label_len++] = (uint8_t)context_len;
	if (context_len > 0) {
		memcpy(&hkdf_label[hkdf_label_len], context, context_len);
		hkdf_label_len += context_len;
	}

	/* Import secret for key derivation */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, PSA_ALG_HKDF_EXPAND(hash_alg));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&attributes, secret_len * 8);

	status = psa_import_key(&attributes, secret, secret_len, &key_id);
	if (status != PSA_SUCCESS) {
		NET_DBG("psa_import_key failed: status=%d, hash_alg=0x%x, secret_len=%zu",
			status, hash_alg, secret_len);
		return -EIO;
	}

	/* Setup HKDF-Expand operation */
	status = psa_key_derivation_setup(&op, PSA_ALG_HKDF_EXPAND(hash_alg));
	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	/* Input the PRK (secret) */
	status = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, key_id);
	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	/* Input the info (HkdfLabel) */
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO,
						hkdf_label, hkdf_label_len);
	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	/* Output the derived key material */
	status = psa_key_derivation_output_bytes(&op, okm, okm_len);

cleanup:
	psa_key_derivation_abort(&op);
	psa_destroy_key(key_id);
	crypto_zero(hkdf_label, sizeof(hkdf_label));

	return (status == PSA_SUCCESS) ? 0 : -EINVAL;
}

/*
 * HKDF-Expand-Label (RFC 8446 Section 7.1, RFC 9001 Section 5.1)
 *
 * HKDF-Expand-Label(Secret, Label, Context, Length) =
 *     HKDF-Expand(Secret, HkdfLabel, Length)
 *
 * struct {
 *     uint16 length = Length;
 *     opaque label<7..255> = "tls13 " + Label;
 *     opaque context<0..255> = Context;
 * } HkdfLabel;
 *
 * Note: QUIC always uses empty context.
 */
ZTESTABLE_STATIC int quic_hkdf_expand_label(const uint8_t *secret, size_t secret_len,
					    const uint8_t *label, size_t label_length,
					    uint8_t *okm, size_t okm_len)
{
	return quic_hkdf_expand_label_ex(PSA_ALG_SHA_256,
					 secret, secret_len,
					 label, label_length,
					 NULL, 0,
					 okm, okm_len);
}

/*
 * Derive initial secrets for client and server (RFC 9001 Section 5.2)
 *
 * initial_secret = HKDF-Extract(initial_salt, client_dst_connection_id)
 * client_initial_secret = HKDF-Expand-Label(initial_secret, "client in", "", 32)
 * server_initial_secret = HKDF-Expand-Label(initial_secret, "server in", "", 32)
 */
ZTESTABLE_STATIC bool quic_setup_initial_secrets(struct quic_endpoint *ep,
				       const uint8_t *cid, size_t cid_len,
				       uint8_t client_initial_secret[QUIC_HASH_SHA2_256_LEN],
				       uint8_t server_initial_secret[QUIC_HASH_SHA2_256_LEN])
{
	/* QUIC v1 initial salt from RFC 9001 Section 5.2 */
	static const uint8_t quic_v1_initial_salt[] = {
		0x38, 0x76, 0x2c, 0xf7, 0xf5, 0x59, 0x34, 0xb3,
		0x4d, 0x17, 0x9a, 0xe6, 0xa4, 0xc8, 0x0c, 0xad,
		0xcc, 0xbb, 0x7f, 0x0a
	};

	uint8_t initial_secret[QUIC_HASH_SHA2_256_LEN];
	bool success = false;

	ARG_UNUSED(ep);

	/* Step 1: Extract initial secret from connection ID */
	if (quic_hkdf_extract(quic_v1_initial_salt, sizeof(quic_v1_initial_salt),
			      cid, cid_len, initial_secret) != 0) {
		goto cleanup;
	}

	/* Step 2: Derive client initial secret */
	if (quic_hkdf_expand_label(initial_secret, QUIC_HASH_SHA2_256_LEN,
				   (const uint8_t *)"client in",
				   sizeof("client in") - 1,
				   client_initial_secret,
				   QUIC_HASH_SHA2_256_LEN) != 0) {
		goto cleanup;
	}

	/* Step 3: Derive server initial secret */
	if (quic_hkdf_expand_label(initial_secret, QUIC_HASH_SHA2_256_LEN,
				   (const uint8_t *)"server in",
				   sizeof("server in") - 1,
				   server_initial_secret,
				   QUIC_HASH_SHA2_256_LEN) != 0) {
		goto cleanup;
	}

	success = true;

cleanup:
	/* Clear intermediate secret */
	crypto_zero(initial_secret, sizeof(initial_secret));
	return success;
}

/*
 * Setup header protection cipher (RFC 9001 Section 5.4)
 *
 * Derives hp_key = HKDF-Expand-Label(secret, "quic hp", "", key_len)
 * and imports it into PSA for AES-ECB or ChaCha20 operations.
 */
static bool quic_hp_setup(struct quic_hp_cipher *hp_cipher, int hash_algo,
			  psa_key_type_t key_type, uint8_t *secret,
			  size_t secret_len)
{
	uint8_t hp_key[QUIC_HASH_SHA2_256_LEN];
	size_t hp_key_len;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_algorithm_t alg;

	ARG_UNUSED(hash_algo); /* currently not used */

	/* Determine key length and algorithm based on cipher type */
	switch (key_type) {
	case PSA_KEY_TYPE_AES:
		/* AES-128 for Initial/Handshake, could be AES-256 for some suites */
		hp_key_len = QUIC_HASH_SHA2_128_LEN;
		alg = PSA_ALG_ECB_NO_PADDING;
		hp_cipher->cipher_algo = QUIC_CIPHER_AES_128_GCM;
		break;
	case PSA_KEY_TYPE_CHACHA20:
		hp_key_len = QUIC_HASH_CHACHA20_LEN;
		alg = PSA_ALG_STREAM_CIPHER;
		hp_cipher->cipher_algo = QUIC_CIPHER_CHACHA20_POLY1305;
		break;
	default:
		return false;
	}

	/* Derive HP key: HKDF-Expand-Label(secret, "quic hp", "", key_len) */
	if (quic_hkdf_expand_label(secret, secret_len,
				   (const uint8_t *)"quic hp",
				   sizeof("quic hp") - 1,
				   hp_key, hp_key_len) != 0) {
		return false;
	}

	/* Import HP key into PSA Crypto */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, alg);
	psa_set_key_type(&attributes, key_type);
	psa_set_key_bits(&attributes, hp_key_len * 8);

	status = psa_import_key(&attributes, hp_key, hp_key_len, &hp_cipher->key_id);

	/* Clear key material from stack */
	crypto_zero(hp_key, sizeof(hp_key));

	if (status != PSA_SUCCESS) {
		return false;
	}

	hp_cipher->initialized = true;
	return true;
}

/*
 * Setup packet protection cipher (RFC 9001 Section 5.3)
 *
 * Derives:
 *   key = HKDF-Expand-Label(secret, "quic key", "", key_len)
 *   iv  = HKDF-Expand-Label(secret, "quic iv", "", 12)
 */
static bool quic_pp_setup(struct quic_pp_cipher *pp_cipher, int hash_algo,
			  int cipher_algo, int cipher_mode,
			  uint8_t *secret, size_t secret_len)
{
	uint8_t pp_key[QUIC_HASH_SHA2_256_LEN];
	size_t pp_key_len;
	const size_t pp_iv_len = TLS13_AEAD_NONCE_LENGTH;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_algorithm_t alg;
	psa_key_type_t key_type;

	ARG_UNUSED(hash_algo);   /* currently not used */
	ARG_UNUSED(cipher_mode); /* currently not used */

	/* Determine parameters based on cipher algorithm */
	switch (cipher_algo) {
	case QUIC_CIPHER_AES_128_GCM:
		pp_key_len = QUIC_HASH_SHA2_128_LEN;
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_GCM;
		break;
	case QUIC_CIPHER_AES_256_GCM:
		pp_key_len = QUIC_HASH_SHA2_256_LEN;
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_GCM;
		break;
	case QUIC_CIPHER_AES_128_CCM:
		pp_key_len = QUIC_HASH_SHA2_128_LEN;
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_CCM;
		break;
	case QUIC_CIPHER_CHACHA20_POLY1305:
		pp_key_len = QUIC_HASH_CHACHA20_LEN;
		key_type = PSA_KEY_TYPE_CHACHA20;
		alg = PSA_ALG_CHACHA20_POLY1305;
		break;
	default:
		return false;
	}

	/* Derive packet protection key */
	if (quic_hkdf_expand_label(secret, secret_len,
				   (const uint8_t *)"quic key",
				   sizeof("quic key") - 1,
				   pp_key, pp_key_len) != 0) {
		return false;
	}

	/* Derive packet protection IV */
	if (quic_hkdf_expand_label(secret, secret_len,
				   (const uint8_t *)"quic iv",
				   sizeof("quic iv") - 1,
				   pp_cipher->iv, pp_iv_len) != 0) {
		crypto_zero(pp_key, sizeof(pp_key));
		return false;
	}

	pp_cipher->iv_len = pp_iv_len;

	/* Import PP key into PSA Crypto */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, alg);
	psa_set_key_type(&attributes, key_type);
	psa_set_key_bits(&attributes, pp_key_len * 8);

	status = psa_import_key(&attributes, pp_key, pp_key_len, &pp_cipher->key_id);

	/* Clear key material from stack */
	crypto_zero(pp_key, sizeof(pp_key));

	if (status != PSA_SUCCESS) {
		memset(pp_cipher->iv, 0, sizeof(pp_cipher->iv));
		return false;
	}

	pp_cipher->cipher_algo = cipher_algo;
	pp_cipher->cipher_mode = cipher_mode;
	pp_cipher->initialized = true;

	return true;
}

/*
 * Setup both HP and PP ciphers for one direction.
 */
static bool quic_setup_ciphers(struct quic_ciphers *ciphers, int hash_algo,
			       int cipher_algo, int cipher_mode,
			       uint8_t *secret, size_t secret_len)
{
	psa_key_type_t hp_key_type;

	/* Determine HP key type based on cipher */
	if (cipher_algo == QUIC_CIPHER_CHACHA20_POLY1305) {
		hp_key_type = PSA_KEY_TYPE_CHACHA20;
	} else {
		hp_key_type = PSA_KEY_TYPE_AES;
	}

	/* Setup header protection */
	if (!quic_hp_setup(&ciphers->hp, hash_algo, hp_key_type, secret, secret_len)) {
		NET_ERR("Failed to setup %s cipher", "HP");
		return false;
	}

	/* Setup packet protection */
	if (!quic_pp_setup(&ciphers->pp, hash_algo, cipher_algo, cipher_mode,
			   secret, secret_len)) {
		/* Cleanup HP on failure */
		psa_destroy_key(ciphers->hp.key_id);
		ciphers->hp.initialized = false;
		NET_ERR("Failed to setup %s cipher", "PP");
		return false;
	}

	return true;
}

/*
 * Setup header protection cipher with explicit hash algorithm (RFC 9001 Section 5.4)
 *
 * Derives hp_key = HKDF-Expand-Label(secret, "quic hp", "", key_len)
 * and imports it into PSA for AES-ECB or ChaCha20 operations.
 */
static bool quic_hp_setup_ex(struct quic_hp_cipher *hp_cipher,
			     psa_algorithm_t hash_alg,
			     int cipher_algo,
			     const uint8_t *secret,
			     size_t secret_len)
{
	uint8_t hp_key[QUIC_HASH_SHA2_256_LEN];
	size_t hp_key_len;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_algorithm_t alg;
	psa_key_type_t key_type;

	/* Determine key length, algorithm and key type based on cipher */
	switch (cipher_algo) {
	case QUIC_CIPHER_AES_128_GCM:
	case QUIC_CIPHER_AES_128_CCM:
		hp_key_len = 16;  /* AES-128 key */
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_ECB_NO_PADDING;
		break;
	case QUIC_CIPHER_AES_256_GCM:
		hp_key_len = 32;  /* AES-256 key */
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_ECB_NO_PADDING;
		break;
	case QUIC_CIPHER_CHACHA20_POLY1305:
		hp_key_len = 32;  /* ChaCha20 key */
		key_type = PSA_KEY_TYPE_CHACHA20;
		alg = PSA_ALG_STREAM_CIPHER;
		break;
	default:
		NET_ERR("Unsupported cipher algorithm for HP:  %d", cipher_algo);
		return false;
	}

	/* Derive HP key using the correct hash algorithm */
	if (quic_hkdf_expand_label_ex(hash_alg,
				      secret, secret_len,
				      (const uint8_t *)"quic hp",
				      sizeof("quic hp") - 1,
				      NULL, 0,
				      hp_key, hp_key_len) != 0) {
		NET_ERR("Failed to derive HP key");
		return false;
	}

	/* Import HP key into PSA Crypto */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, alg);
	psa_set_key_type(&attributes, key_type);
	psa_set_key_bits(&attributes, hp_key_len * 8);

	status = psa_import_key(&attributes, hp_key, hp_key_len, &hp_cipher->key_id);

	/* Clear key material from stack */
	crypto_zero(hp_key, sizeof(hp_key));

	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import HP key:  %d", status);
		return false;
	}

	hp_cipher->cipher_algo = cipher_algo;
	hp_cipher->initialized = true;

	return true;
}

/*
 * Setup packet protection cipher with explicit hash algorithm (RFC 9001 Section 5.3)
 */
static bool quic_pp_setup_ex(struct quic_pp_cipher *pp_cipher,
			     psa_algorithm_t hash_alg,
			     int cipher_algo,
			     const uint8_t *secret,
			     size_t secret_len)
{
	uint8_t pp_key[QUIC_HASH_SHA2_256_LEN];
	size_t pp_key_len;
	const size_t pp_iv_len = TLS13_AEAD_NONCE_LENGTH;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_algorithm_t alg;
	psa_key_type_t key_type;

	/* Determine parameters based on cipher algorithm */
	switch (cipher_algo) {
	case QUIC_CIPHER_AES_128_GCM:
		pp_key_len = 16;
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_GCM;
		break;
	case QUIC_CIPHER_AES_256_GCM:
		pp_key_len = 32;
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_GCM;
		break;
	case QUIC_CIPHER_AES_128_CCM:
		pp_key_len = 16;
		key_type = PSA_KEY_TYPE_AES;
		alg = PSA_ALG_CCM;
		break;
	case QUIC_CIPHER_CHACHA20_POLY1305:
		pp_key_len = 32;
		key_type = PSA_KEY_TYPE_CHACHA20;
		alg = PSA_ALG_CHACHA20_POLY1305;
		break;
	default:
		NET_ERR("Unsupported cipher algorithm for PP: %d", cipher_algo);
		return false;
	}

	/* Derive packet protection key using correct hash algorithm */
	if (quic_hkdf_expand_label_ex(hash_alg,
				      secret, secret_len,
				      (const uint8_t *)"quic key",
				      sizeof("quic key") - 1,
				      NULL, 0,
				      pp_key, pp_key_len) != 0) {
		NET_ERR("Failed to derive PP key");
		crypto_zero(pp_key, sizeof(pp_key));
		return false;
	}

	/* Derive packet protection IV */
	if (quic_hkdf_expand_label_ex(hash_alg,
				      secret, secret_len,
				      (const uint8_t *)"quic iv",
				      sizeof("quic iv") - 1,
				      NULL, 0,
				      pp_cipher->iv, pp_iv_len) != 0) {
		NET_ERR("Failed to derive PP IV");
		crypto_zero(pp_key, sizeof(pp_key));
		return false;
	}

	pp_cipher->iv_len = pp_iv_len;

	/* Import PP key into PSA Crypto */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, alg);
	psa_set_key_type(&attributes, key_type);
	psa_set_key_bits(&attributes, pp_key_len * 8);

	status = psa_import_key(&attributes, pp_key, pp_key_len, &pp_cipher->key_id);

	/* Clear key material from stack */
	crypto_zero(pp_key, sizeof(pp_key));

	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import PP key:  %d", status);
		memset(pp_cipher->iv, 0, sizeof(pp_cipher->iv));
		return false;
	}

	pp_cipher->cipher_algo = cipher_algo;
	pp_cipher->initialized = true;

	return true;
}

/*
 * Initialize connection with Initial encryption level.
 *
 * Called when a connection is established, using the Destination Connection ID
 * from the client's first Initial packet.
 */
ZTESTABLE_STATIC bool quic_conn_init_setup(struct quic_endpoint *ep,
					   const uint8_t *cid, size_t cid_len)
{
	uint8_t client_initial_secret[QUIC_HASH_SHA2_256_LEN];
	uint8_t server_initial_secret[QUIC_HASH_SHA2_256_LEN];
	uint8_t *tx_secret;
	uint8_t *rx_secret;
	bool success = false;

	/* Derive initial secrets */
	if (!quic_setup_initial_secrets(ep, cid, cid_len,
					client_initial_secret,
					server_initial_secret)) {
		goto cleanup;
	}

	/* Determine TX/RX secrets based on role */
	if (ep->is_server) {
		tx_secret = server_initial_secret;
		rx_secret = client_initial_secret;
	} else {
		tx_secret = client_initial_secret;
		rx_secret = server_initial_secret;
	}

	/* Setup TX ciphers (Initial uses AES-128-GCM) */
	if (!quic_setup_ciphers(&ep->crypto.initial.tx,
				QUIC_HASH_SHA256,
				QUIC_CIPHER_AES_128_GCM,
				0,
				tx_secret,
				QUIC_HASH_SHA2_256_LEN)) {
		goto cleanup;
	}

	/* Setup RX ciphers */
	if (!quic_setup_ciphers(&ep->crypto.initial.rx,
				QUIC_HASH_SHA256,
				QUIC_CIPHER_AES_128_GCM,
				0,
				rx_secret,
				QUIC_HASH_SHA2_256_LEN)) {
		/* Cleanup TX ciphers on failure */
		psa_destroy_key(ep->crypto.initial.tx.hp.key_id);
		psa_destroy_key(ep->crypto.initial.tx.pp.key_id);
		ep->crypto.initial.tx.hp.initialized = false;
		ep->crypto.initial.tx.pp.initialized = false;
		goto cleanup;
	}

	ep->crypto.initial.initialized = true;
	success = true;

cleanup:
	/* Clear secrets from memory */
	crypto_zero(client_initial_secret, sizeof(client_initial_secret));
	crypto_zero(server_initial_secret, sizeof(server_initial_secret));

	return success;
}

/*
 * Generate header protection mask (RFC 9001 Section 5.4.3, 5.4.4)
 *
 * For AES-based ciphers:
 *   mask = AES-ECB(hp_key, sample)[0..4]
 *
 * For ChaCha20:
 *   counter = sample[0..3]
 *   nonce = sample[4..15]
 *   mask = ChaCha20(hp_key, counter, nonce, {0,0,0,0,0})[0..4]
 */
ZTESTABLE_STATIC int quic_hp_mask(psa_key_id_t hp_key_id,
				  int cipher_algo,
				  const uint8_t *sample,
				  uint8_t *mask)
{
	psa_status_t status;
	size_t output_length;

	if (cipher_algo == QUIC_CIPHER_CHACHA20_POLY1305) {
		/*
		 * ChaCha20 header protection (RFC 9001 Section 5.4.4):
		 * The first 4 bytes of sample are the block counter.
		 * The remaining 12 bytes are the nonce.
		 * Encrypt 5 zero bytes to produce the mask.
		 */
		uint8_t counter_nonce[16];
		uint8_t plaintext[5] = {0, 0, 0, 0, 0};
		uint8_t output[5 + 16]; /* May need extra space for some PSA implementations */

		/* sample[0..3] = counter (little-endian), sample[4..15] = nonce */
		memcpy(counter_nonce, sample, 16);

		status = psa_cipher_encrypt(hp_key_id,
					    PSA_ALG_STREAM_CIPHER,
					    plaintext, sizeof(plaintext),
					    output, sizeof(output),
					    &output_length);

		if (status == PSA_SUCCESS && output_length >= QUIC_HP_MASK_LEN) {
			memcpy(mask, output, QUIC_HP_MASK_LEN);
		}
	} else {
		/*
		 * AES header protection (RFC 9001 Section 5.4.3):
		 * mask = AES-ECB(hp_key, sample)
		 * Use first 5 bytes of the 16-byte output.
		 */
		uint8_t output[16 + 16]; /* PSA may add padding info */

		status = psa_cipher_encrypt(hp_key_id,
					    PSA_ALG_ECB_NO_PADDING,
					    sample, QUIC_HP_SAMPLE_LEN,
					    output, sizeof(output),
					    &output_length);

		if (status == PSA_SUCCESS && output_length >= QUIC_HP_MASK_LEN) {
			memcpy(mask, output, QUIC_HP_MASK_LEN);
		}
	}

	if (status != PSA_SUCCESS) {
		NET_DBG("Failed to encrypt (%d)", status);
	}

	return (status == PSA_SUCCESS) ? 0 : -1;
}

/*
 * Decrypt (remove) header protection from a received packet (RFC 9001 Section 5.4.2)
 *
 * Steps:
 * 1. Locate sample at pn_offset + 4
 * 2. Generate mask using HP key
 * 3. Unmask first byte to get packet number length
 * 4. Unmask packet number bytes
 *
 * Returns: 0 on success, -1 on failure
 *
 * Note: This function does NOT modify the packet buffer. The caller should
 * apply the returned values when constructing the decrypted header.
 */
ZTESTABLE_STATIC int quic_decrypt_header(const uint8_t *packet,
					 size_t packet_len,
					 size_t pn_offset,
					 psa_key_id_t hp_key_id,
					 int cipher_algo,
					 uint8_t *first_byte_out,
					 uint32_t *packet_number_out,
					 size_t *pn_length_out)
{
	uint8_t mask[QUIC_HP_MASK_LEN] = { 0 };
	const uint8_t *sample;
	size_t sample_offset;
	size_t pn_length;
	uint8_t first_byte;
	uint32_t pn;
	int ret;

	/*
	 * Sample starts at pn_offset + 4 (RFC 9001 Section 5.4.2)
	 * This assumes a 4-byte packet number for sampling purposes.
	 */
	sample_offset = pn_offset + QUIC_HP_MAX_PN_LEN;

	/* Verify packet is long enough for sample */
	if (sample_offset + QUIC_HP_SAMPLE_LEN > packet_len) {
		NET_DBG("Packet too short for sample extraction (%zu < %zu)",
			packet_len, sample_offset + QUIC_HP_SAMPLE_LEN);
		return -ERANGE;
	}

	sample = &packet[sample_offset];

	/* Generate mask */
	ret = quic_hp_mask(hp_key_id, cipher_algo, sample, mask);
	if (ret != 0) {
		NET_DBG("Failed to generate HP mask (%d)", ret);
		return -EINVAL;
	}

	/* Unmask first byte */
	first_byte = packet[0];
	if ((first_byte & 0x80) != 0) {
		/* Long header: unmask lower 4 bits */
		first_byte ^= (mask[0] & 0x0F);
	} else {
		/* Short header: unmask lower 5 bits */
		first_byte ^= (mask[0] & 0x1F);
	}

	*first_byte_out = first_byte;

	/* Extract packet number length from bits 0-1 of unmasked first byte */
	pn_length = (first_byte & 0x03) + 1;
	*pn_length_out = pn_length;

	/* Validate packet number fits in packet */
	if (pn_offset + pn_length > packet_len) {
		NET_DBG("Packet too short for PN extraction (%zu + %zu > %zu)",
			pn_offset, pn_length, packet_len);
		return -ERANGE;
	}

	/* Unmask and decode packet number (big-endian) */
	pn = 0;
	for (size_t i = 0; i < pn_length; i++) {
		uint8_t pn_byte = packet[pn_offset + i] ^ mask[1 + i];

		pn = (pn << 8) | pn_byte;
	}

	*packet_number_out = pn;

	return 0;
}

#if defined(CONFIG_NET_TEST)
/*
 * Encrypt (apply) header protection to an outgoing packet (RFC 9001 Section 5.4.1)
 *
 * This must be called after the payload has been encrypted,
 * because the sample is taken from the encrypted payload.
 *
 * Steps:
 * 1. Locate sample at pn_offset + 4
 * 2. Generate mask using HP key
 * 3. Mask first byte
 * 4. Mask packet number bytes
 *
 * The packet buffer is modified in-place.
 */
ZTESTABLE_STATIC int quic_encrypt_header(uint8_t *packet, size_t packet_len,
					 size_t pn_offset,
					 size_t pn_length,
					 psa_key_id_t hp_key_id,
					 int cipher_algo)
{
	uint8_t mask[QUIC_HP_MASK_LEN] = { 0 };
	const uint8_t *sample;
	size_t sample_offset;

	/* Validate packet number length */
	if (pn_length < 1 || pn_length > QUIC_HP_MAX_PN_LEN) {
		return -1;
	}

	/* Sample starts at pn_offset + 4 */
	sample_offset = pn_offset + QUIC_HP_MAX_PN_LEN;

	/* Verify packet is long enough for sample */
	if (sample_offset + QUIC_HP_SAMPLE_LEN > packet_len) {
		return -1;
	}

	sample = &packet[sample_offset];

	/* Generate mask */
	if (quic_hp_mask(hp_key_id, cipher_algo, sample, mask) != 0) {
		return -1;
	}

	/* Mask first byte */
	if ((packet[0] & 0x80) != 0) {
		/* Long header: mask lower 4 bits */
		packet[0] ^= (mask[0] & 0x0F);
	} else {
		/* Short header: mask lower 5 bits */
		packet[0] ^= (mask[0] & 0x1F);
	}

	/* Mask packet number bytes */
	for (size_t i = 0; i < pn_length; i++) {
		packet[pn_offset + i] ^= mask[1 + i];
	}

	return 0;
}
#endif

/*
 * Construct AEAD nonce by XORing IV with packet number.
 * RFC 9001 Section 5.3
 *
 * The 62-bit packet number is left-padded with zeros to the size of the IV
 * and then XORed with the IV.
 */
ZTESTABLE_STATIC void quic_construct_nonce(const uint8_t *iv, size_t iv_len,
					   uint64_t packet_number, uint8_t *nonce)
{
	memcpy(nonce, iv, iv_len);

	/* XOR packet number into rightmost 8 bytes (big-endian) */
	for (int i = 0; i < 8; i++) {
		nonce[iv_len - 1 - i] ^= (uint8_t)(packet_number >> (i * 8));
	}
}

/*
 * Reconstruct full packet number from truncated value.
 * RFC 9000 Appendix A
 */
ZTESTABLE_STATIC uint64_t quic_reconstruct_pn(uint32_t truncated_pn,
					      size_t pn_nbits,
					      uint64_t largest_pn)
{
	uint64_t expected_pn = largest_pn + 1;
	uint64_t pn_win = 1ULL << pn_nbits;
	uint64_t pn_hwin = pn_win / 2;
	uint64_t pn_mask = pn_win - 1;
	uint64_t candidate_pn = (expected_pn & ~pn_mask) | truncated_pn;

	if (candidate_pn + pn_hwin <= expected_pn &&
	    candidate_pn + pn_win < (1ULL << 62)) {
		return candidate_pn + pn_win;
	}

	if (candidate_pn > expected_pn + pn_hwin &&
	    candidate_pn >= pn_win) {
		return candidate_pn - pn_win;
	}

	return candidate_pn;
}

/*
 * Get PSA algorithm for the cipher.
 */
static psa_algorithm_t quic_get_aead_alg(int cipher_algo)
{
	switch (cipher_algo) {
	case QUIC_CIPHER_AES_128_GCM:
	case QUIC_CIPHER_AES_256_GCM:
		return PSA_ALG_GCM;
	case QUIC_CIPHER_CHACHA20_POLY1305:
		return PSA_ALG_CHACHA20_POLY1305;
	case QUIC_CIPHER_AES_128_CCM:
		return PSA_ALG_CCM;
	default:
		return PSA_ALG_GCM;
	}
}

/**
 * Decrypt packet payload using AEAD.
 * RFC 9001 Section 5.3
 *
 * @param pp Packet protection cipher context
 * @param packet_number Full reconstructed packet number
 * @param header Unprotected header (used as AAD)
 * @param header_len Length of header
 * @param ciphertext Encrypted payload (including auth tag)
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer for decrypted payload
 * @param plaintext_size Size of plaintext buffer
 * @param[out] plaintext_len Actual length of decrypted payload
 *
 * @return 0 on success, <0 on failure
 */
ZTESTABLE_STATIC int quic_decrypt_payload(struct quic_pp_cipher *pp,
					  uint64_t packet_number,
					  const uint8_t *header,
					  size_t header_len,
					  const uint8_t *ciphertext,
					  size_t ciphertext_len,
					  uint8_t *plaintext,
					  size_t plaintext_size,
					  size_t *plaintext_len)
{
	uint8_t nonce[TLS13_AEAD_NONCE_LENGTH];
	size_t expected_plaintext_len;
	psa_algorithm_t alg;
	psa_status_t status;

	if (!pp->initialized) {
		NET_DBG("Packet protection cipher not initialized");
		return -EINVAL;
	}

	/* Verify ciphertext has room for auth tag */
	if (ciphertext_len < QUIC_AEAD_TAG_LEN) {
		NET_DBG("Ciphertext too short for auth tag (%zu < %d)",
			ciphertext_len, QUIC_AEAD_TAG_LEN);
		return -EINVAL;
	}

	expected_plaintext_len = ciphertext_len - QUIC_AEAD_TAG_LEN;
	if (plaintext_size < expected_plaintext_len) {
		NET_DBG("Plaintext buffer too small (%zu < %zu)",
			plaintext_size, expected_plaintext_len);
		return -ENOBUFS;
	}

	/* Construct nonce: IV XOR packet_number */
	quic_construct_nonce(pp->iv, pp->iv_len, packet_number, nonce);

	alg = quic_get_aead_alg(pp->cipher_algo);

	status = psa_aead_decrypt(pp->key_id,
				  alg,
				  nonce, sizeof(nonce),
				  header, header_len,
				  ciphertext, ciphertext_len,
				  plaintext, plaintext_size,
				  plaintext_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("AEAD decryption failed (status %d)", status);
		return -EBADMSG;
	}

	return 0;
}

/**
 * Encrypt packet payload using AEAD.
 * RFC 9001 Section 5.3
 *
 * This must be called before quic_encrypt_header()
 *
 * @param pp Packet protection cipher context
 * @param packet_number Packet number to use for nonce
 * @param header Unprotected header (used as AAD)
 * @param header_len Length of header
 * @param plaintext Payload to encrypt
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer for encrypted payload + tag
 * @param ciphertext_size Size of ciphertext buffer
 * @param[out] ciphertext_len Actual length of ciphertext (including tag)
 *
 * @return 0 on success, <0 on failure
 */
ZTESTABLE_STATIC int quic_encrypt_payload(struct quic_pp_cipher *pp,
					  uint64_t packet_number,
					  const uint8_t *header,
					  size_t header_len,
					  const uint8_t *plaintext,
					  size_t plaintext_len,
					  uint8_t *ciphertext,
					  size_t ciphertext_size,
					  size_t *ciphertext_len)
{
	uint8_t nonce[TLS13_AEAD_NONCE_LENGTH];
	size_t expected_ciphertext_len;
	psa_algorithm_t alg;
	psa_status_t status;

	if (!pp->initialized) {
		NET_DBG("Packet protection cipher not initialized");
		return -EINVAL;
	}

	expected_ciphertext_len = plaintext_len + QUIC_AEAD_TAG_LEN;
	if (ciphertext_size < expected_ciphertext_len) {
		NET_DBG("Ciphertext buffer too small (%zu < %zu)",
			ciphertext_size, expected_ciphertext_len);
		return -ENOBUFS;
	}

	quic_construct_nonce(pp->iv, pp->iv_len, packet_number, nonce);

	alg = quic_get_aead_alg(pp->cipher_algo);

	status = psa_aead_encrypt(pp->key_id,
				  alg,
				  nonce, sizeof(nonce),
				  header, header_len,
				  plaintext, plaintext_len,
				  ciphertext, ciphertext_size,
				  ciphertext_len);
	if (status != PSA_SUCCESS) {
		NET_DBG("AEAD encryption failed (status %d)", status);
		return -EIO;
	}

	return 0;
}

/*
 * Get largest packet number for the given encryption level.
 */
static uint64_t *quic_get_largest_pn(struct quic_endpoint *ep,
				     enum quic_packet_type ptype)
{
	switch (ptype) {
	case QUIC_PACKET_TYPE_INITIAL:
		return &ep->rx_pn.initial;
	case QUIC_PACKET_TYPE_HANDSHAKE:
		return &ep->rx_pn.handshake;
	case QUIC_PACKET_TYPE_0RTT:
		/* 0-RTT uses application PN space */
		return &ep->rx_pn.application;
	default:
		return &ep->rx_pn.application;
	}
}

/*
 * Get crypto context for the given packet type.
 */
static struct quic_crypto_context *quic_get_crypto_context(struct quic_endpoint *ep,
							   enum quic_packet_type ptype)
{
	switch (ptype) {
	case QUIC_PACKET_TYPE_INITIAL:
		return ep->crypto.initial.initialized ? &ep->crypto.initial : NULL;
	case QUIC_PACKET_TYPE_HANDSHAKE:
		return ep->crypto.handshake.initialized ? &ep->crypto.handshake : NULL;
	case QUIC_PACKET_TYPE_0RTT:
		/* 0-RTT not implemented */
		return NULL;
	default:
		break;
	}

	return ep->crypto.application.initialized ? &ep->crypto.application : NULL;
}

/*
 * Result structure for fully decrypted packet.
 */
struct quic_decrypted_packet {
	uint8_t *payload;           /* Decrypted frames */
	size_t payload_len;         /* Length of decrypted payload */
	uint64_t packet_number;     /* Full reconstructed packet number */
	enum quic_packet_type type; /* Packet type */
};

/**
 * Decrypt a complete QUIC packet (header protection + AEAD).
 *
 * 1. Remove header protection to get the packet number
 * 2. Reconstruct the full packet number
 * 3. Build the AAD from the unprotected header
 * 4. Decrypt the payload using AEAD
 *
 * @param ep QUIC endpoint
 * @param packet Complete protected packet
 * @param packet_len Length of packet
 * @param pn_offset Offset where packet number starts
 * @param ptype Packet type (Initial, Handshake, etc.)
 * @param plaintext Output buffer for decrypted frames
 * @param plaintext_size Size of plaintext buffer
 * @param result Output structure with decryption results
 *
 * @return 0 on success, <0 on failure
 */
static int quic_decrypt_packet(struct quic_endpoint *ep,
			       const uint8_t *packet,
			       size_t packet_len,
			       size_t pn_offset,
			       enum quic_packet_type ptype,
			       uint8_t *plaintext,
			       size_t plaintext_size,
			       struct quic_decrypted_packet *result)
{
	struct quic_crypto_context *crypto;
	uint8_t first_byte;
	uint32_t truncated_pn;
	size_t pn_length;
	uint64_t full_pn;
	uint64_t *largest_pn;
	uint8_t header_aad[128];
	size_t header_len;
	const uint8_t *ciphertext;
	size_t ciphertext_len;
	int ret;

	crypto = quic_get_crypto_context(ep, ptype);
	if (crypto == NULL) {
		NET_DBG("No crypto context for packet type %d", ptype);
		return -ENOENT;
	}

	/* Step 1: Remove header protection */
	ret = quic_decrypt_header(packet, packet_len, pn_offset,
				  crypto->rx.hp.key_id,
				  crypto->rx.hp.cipher_algo,
				  &first_byte,
				  &truncated_pn,
				  &pn_length);
	if (ret != 0) {
		NET_DBG("Header protection removal failed (%d)", ret);
		return ret;
	}

	/* Step 2: Reconstruct full packet number */
	largest_pn = quic_get_largest_pn(ep, ptype);
	full_pn = quic_reconstruct_pn(truncated_pn,
				      pn_length * 8, /* bits */
				      *largest_pn);

	NET_DBG("Packet number: truncated=%u, full=%" PRIu64 " (largest=%" PRIu64 ")",
		truncated_pn, full_pn, *largest_pn);

	/* Step 3: Build unprotected header for AAD */
	header_len = pn_offset + pn_length;

	if (header_len > sizeof(header_aad)) {
		NET_DBG("Header too large for AAD buffer");
		return -ENOBUFS;
	}

	if (header_len > packet_len) {
		NET_DBG("Header length exceeds packet length");
		return -EINVAL;
	}

	/* Copy original header bytes */
	memcpy(header_aad, packet, header_len);

	/* Replace first byte with unmasked value */
	header_aad[0] = first_byte;

	/* Replace packet number bytes with unmasked values (big-endian) */
	for (size_t i = 0; i < pn_length; i++) {
		header_aad[pn_offset + i] =
			(uint8_t)(truncated_pn >> (8 * (pn_length - 1 - i)));
	}

	/* Step 4: Locate ciphertext */
	ciphertext = &packet[header_len];
	ciphertext_len = packet_len - header_len;

	NET_DBG("Header len: %zu, ciphertext len: %zu (packet_len=%zu)",
		header_len, ciphertext_len, packet_len);

	if (ciphertext_len < QUIC_AEAD_TAG_LEN) {
		NET_DBG("Packet too short for AEAD tag (%zu < %d)",
			ciphertext_len, QUIC_AEAD_TAG_LEN);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(ciphertext, MIN(ciphertext_len, 48), "ciphertext (first 48):");
	}

	/* Step 5: Decrypt payload */
	ret = quic_decrypt_payload(&crypto->rx.pp,
				   full_pn,
				   header_aad, header_len,
				   ciphertext, ciphertext_len,
				   plaintext, plaintext_size,
				   &result->payload_len);
	if (ret != 0) {
		NET_DBG("Payload decryption failed (%d)", ret);
		return ret;
	}

	/* Step 6: Update largest packet number */
	if (full_pn > *largest_pn) {
		*largest_pn = full_pn;
	}

	/* Fill result */
	result->payload = plaintext;
	result->packet_number = full_pn;
	result->type = ptype;

	return 0;
}

static int pollfd_remove(net_sa_family_t family, int sock)
{
#if defined(CONFIG_NET_IPV4)
	if (family == NET_AF_INET) {
		ARRAY_FOR_EACH_PTR(quic_ipv4_pollfds, sockfd) {
			if (sockfd->fd == sock) {
				sockfd->fd = -1;
				return 0;
			}
		}
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (family == NET_AF_INET6) {
		ARRAY_FOR_EACH_PTR(quic_ipv6_pollfds, sockfd) {
			if (sockfd->fd == sock) {
				sockfd->fd = -1;
				return 0;
			}
		}
	}
#endif
	return -ENOENT;
}

static bool quic_context_is_used(struct quic_context *ctx)
{
	return !!atomic_get(&ctx->refcount);
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_context_ref_debug(struct quic_context *ctx, const char *caller, int line)
#define quic_context_ref(ctx) quic_context_ref_debug(ctx, __func__, __LINE__)
#else
static int quic_context_ref(struct quic_context *ctx)
#endif
{
	atomic_val_t ref;

	do {
		ref = atomic_get(&ctx->refcount);
	} while (!atomic_cas(&ctx->refcount, ref, ref + 1));

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	NET_DBG("[CO:%p/%d] Context ref %ld (%s():%d)", ctx, quic_get_by_conn(ctx),
		ref + 1, caller, line);
#endif

	return ref + 1;
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_context_unref_debug(struct quic_context *ctx, const char *caller, int line)
#define quic_context_unref(ctx) quic_context_unref_debug(ctx, __func__, __LINE__)
#else
static int quic_context_unref(struct quic_context *ctx)
#endif
{
	struct quic_endpoint *ep, *tmp;
	atomic_val_t ref;

	do {
		ref = atomic_get(&ctx->refcount);
		if (ref == 0) {
			NET_DBG("[CO:%p/%d] Context freed already", ctx, quic_get_by_conn(ctx));
			return 0;
		}
	} while (!atomic_cas(&ctx->refcount, ref, ref - 1));

	if (ref > 1) {
#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
		NET_DBG("[CO:%p/%d] Context sock %d ref %d by %s():%d", ctx, quic_get_by_conn(ctx),
			ctx->sock, (int)(ref - 1), caller, line);
#endif
		return ref - 1;
	}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	NET_DBG("[CO:%p/%d] Context released (was sock=%d) by %s():%d", ctx,
		quic_get_by_conn(ctx), ctx->sock, caller, line);
#endif

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->endpoints, ep, tmp, node) {
		sys_slist_find_and_remove(&ctx->endpoints, &ep->node);
		quic_endpoint_send_connection_close(ep, 0, NULL);
		quic_endpoint_unref(ep);

		if (ctx->listen == ep) {
			ctx->listen = NULL;
		}
	}

	if (ctx->listen != NULL) {
		quic_endpoint_unref(ctx->listen);
		ctx->listen = NULL;
	}

	k_mutex_unlock(&endpoints_lock);

	if (ctx->sock >= 0) {
		zsock_close(ctx->sock);
		ctx->sock = -1;
	}

	return 0;
}

static bool quic_stream_is_used(struct quic_stream *stream)
{
	return !!atomic_get(&stream->refcount);
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_stream_ref_debug(struct quic_stream *stream, const char *caller, int line)
#define quic_stream_ref(stream) quic_stream_ref_debug(stream, __func__, __LINE__)
#else
static int quic_stream_ref(struct quic_stream *stream)
#endif
{
	atomic_val_t ref;

	do {
		ref = atomic_get(&stream->refcount);
	} while (!atomic_cas(&stream->refcount, ref, ref + 1));

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	NET_DBG("[ST:%p/%d] Stream ref %d (%s():%d)", stream, quic_get_by_stream(stream),
		(int)(ref + 1), caller, line);
#endif

	return ref + 1;
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_stream_unref_debug(struct quic_stream *stream, const char *caller, int line)
#define quic_stream_unref(stream) quic_stream_unref_debug(stream, __func__, __LINE__)
#else
static int quic_stream_unref(struct quic_stream *stream)
#endif
{
	atomic_val_t ref;

	do {
		ref = atomic_get(&stream->refcount);
		if (ref == 0) {
			NET_DBG("[ST:%p/%d] Stream %" PRIu64 " freed already",
				stream, quic_get_by_stream(stream), stream->id);
			return 0;
		}
	} while (!atomic_cas(&stream->refcount, ref, ref - 1));

	if (ref > 1) {
#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
		NET_DBG("[ST:%p/%d] Stream %" PRIu64 " sock %d ref %d by %s():%d",
			stream, quic_get_by_stream(stream), stream->id,
			stream->sock, (int)(ref - 1), caller, line);
#endif
		return ref - 1;
	}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	NET_DBG("[ST:%p/%d] Stream %" PRIu64 " released (was sock=%d) by %s():%d",
		stream, quic_get_by_stream(stream), stream->id, stream->sock, caller, line);
#endif

	quic_stream_flush_queue(stream);

	/*
	 * If this was a peer-initiated stream (RFC 9000 ch. 4.6) freeing its slot
	 * back to the pool, advance our advertised stream limit by 1 so the peer
	 * can open another stream in place of the one that just closed.
	 *
	 * Peer-initiated streams on a server endpoint have bit 0 == 0 (client bit).
	 * Peer-initiated streams on a client endpoint have bit 0 == 1 (server bit).
	 * Bidi streams have bit 1 == 0; uni streams have bit 1 == 1.
	 */
	if (stream->ep != NULL && stream->id != QUIC_STREAM_ID_UNASSIGNED) {
		bool is_server = stream->ep->is_server;
		bool peer_init = ((stream->id & 1u) == 0u) == is_server;
		bool is_bidi = (stream->id & 2u) == 0u;

		if (peer_init) {
			if (is_bidi && stream->ep->rx_sl.open_bidi > 0) {
				stream->ep->rx_sl.open_bidi--;

				/*
				 * Slide the cumulative limit up by 1: the freed slot
				 * is now available for a future stream.  Notify the
				 * peer so it can unblock if it was waiting.
				 */
				stream->ep->rx_sl.max_bidi++;
				(void)quic_send_max_streams(stream->ep, true);

				NET_DBG("[EP:%p/%d] Stream %" PRIu64 " closed, "
					"open_bidi=%" PRIu64 " max_bidi=%" PRIu64,
					stream->ep, quic_get_by_ep(stream->ep),
					stream->id,
					stream->ep->rx_sl.open_bidi,
					stream->ep->rx_sl.max_bidi);

			} else if (!is_bidi && stream->ep->rx_sl.open_uni > 0) {
				stream->ep->rx_sl.open_uni--;
				stream->ep->rx_sl.max_uni++;
				(void)quic_send_max_streams(stream->ep, false);
			}
		}
	}

	if (stream->conn != NULL) {
		k_mutex_lock(&streams_lock, K_FOREVER);
		sys_slist_find_and_remove(&stream->conn->streams, &stream->node);
		k_mutex_unlock(&streams_lock);

		quic_context_unref(stream->conn);
		stream->conn = NULL;
	}

	if (stream->sock >= 0) {
		zsock_close(stream->sock);
		stream->sock = -1;
	}

	/*
	 * The stream fd must be closed by the caller (HTTP layer) via
	 * zsock_close() BEFORE quic_stream_unref() is called.
	 * quic_stream_close_vmeth() clears stream->sock = -1 before
	 * calling us, so this should never trigger. If it does, it
	 * indicates a missing zsock_close() somewhere.
	 */
	if (stream->sock >= 0) {
		NET_WARN("[ST:%p/%d] Stream %" PRIu64 " freed with open fd %d, "
			 "missing zsock_close() in caller",
			 stream, quic_get_by_stream(stream),
			 stream->id, stream->sock);
		stream->sock = -1;
	}

	return 0;
}

static bool quic_endpoint_is_used(struct quic_endpoint *ep)
{
	return !!atomic_get(&ep->refcount);
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_endpoint_ref_debug(struct quic_endpoint *ep, const char *caller, int line)
#define quic_endpoint_ref(ep) quic_endpoint_ref_debug(ep, __func__, __LINE__)
#else
static int quic_endpoint_ref(struct quic_endpoint *ep)
#endif
{
	atomic_val_t ref;

	do {
		ref = atomic_get(&ep->refcount);
	} while (!atomic_cas(&ep->refcount, ref, ref + 1));

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	NET_DBG("[EP:%p/%d] ref %d (%s():%d)", ep, quic_get_by_ep(ep),
		(int)(ref + 1), caller, line);
#endif

	return ref + 1;
}

/* Notify all streams associated with this endpoint that the connection is closed */
static void quic_endpoint_notify_streams_closed(struct quic_endpoint *ep)
{
	int i;

	k_mutex_lock(&contexts_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		struct quic_context *ctx = &contexts[i];
		struct quic_stream *stream, *tmp;

		if (!quic_context_is_used(ctx)) {
			continue;
		}

		ctx->error_code = -ECONNRESET;

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->streams, stream, tmp, node) {
			if (stream->ep == ep) {
				NET_DBG("[EP:%p/%d] Notifying stream %" PRIu64
					" of connection close",
					ep, quic_get_by_ep(ep), stream->id);

				/* Mark stream as closed */
				stream->ep = NULL;

				/* Wake up any waiting readers */
				k_condvar_signal(&stream->cond.recv);
				k_poll_signal_raise(&stream->recv.signal, 0);

				sys_slist_find_and_remove(&ctx->streams, &stream->node);

				ep->is_closing_notified = true;
			}
		}

		/* This will release any accepts waiting on this stream */
		k_sem_give(&ctx->incoming.stream_sem);
	}

	k_mutex_unlock(&contexts_lock);
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static int quic_endpoint_unref_debug(struct quic_endpoint *ep, const char *caller, int line)
#else
static int quic_endpoint_unref(struct quic_endpoint *ep)
#endif
{
	atomic_val_t ref;
	int ret;

	do {
		ref = atomic_get(&ep->refcount);
		if (ref == 0) {
			NET_DBG("[EP:%p/%d] freed already", ep, quic_get_by_ep(ep));
			return 0;
		}
	} while (!atomic_cas(&ep->refcount, ref, ref - 1));

	if (ref > 1) {
#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
		NET_DBG("[EP:%p/%d] sock %d ref %d by %s():%d", ep,
			quic_get_by_ep(ep), ep->sock,
			(int)(ref - 1), caller, line);
#endif
		return ref - 1;
	}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	NET_DBG("[EP:%p/%d] Endpoint released (was sock=%d) by %s():%d", ep,
		quic_get_by_ep(ep), ep->sock, caller, line);
#endif

	/* Only close socket if we own it (parent endpoint with valid socket) */
	if (ep->sock >= 0 && ep->parent == NULL) {
		const struct net_socket_service_desc *svc;

		NET_DBG("[EP:%p/%d] Closing endpoint socket %d", ep,
			quic_get_by_ep(ep), ep->sock);

		ret = pollfd_remove(ep->local_addr.ss_family, ep->sock);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] Cannot remove socket %d from pollfds (%d)",
				ep, quic_get_by_ep(ep), ep->sock, ret);
		}

		if (ep->local_addr.ss_family == NET_AF_INET) {
			NET_DBG("[EP:%p/%d] %s socket service handler for IPv%d",
				ep, quic_get_by_ep(ep), "Unregistering", 4);
			svc = COND_CODE_1(CONFIG_NET_IPV4, (&svc4), (NULL));

			ret = net_socket_service_register(svc,
					COND_CODE_1(CONFIG_NET_IPV4,
						    (quic_ipv4_pollfds), (NULL)),
					COND_CODE_1(CONFIG_NET_IPV4,
						    (ARRAY_SIZE(quic_ipv4_pollfds)), (0)),
					ep);
		} else if (ep->local_addr.ss_family == NET_AF_INET6) {
			NET_DBG("[EP:%p/%d] %s socket service handler for IPv%d",
				ep, quic_get_by_ep(ep), "Unregistering", 6);
			svc = COND_CODE_1(CONFIG_NET_IPV6, (&svc6), (NULL));

			ret = net_socket_service_register(svc,
					COND_CODE_1(CONFIG_NET_IPV6,
						    (quic_ipv6_pollfds), (NULL)),
					COND_CODE_1(CONFIG_NET_IPV6,
						    (ARRAY_SIZE(quic_ipv6_pollfds)), (0)),
					ep);
		}

		if (ret < 0) {
			NET_DBG("Cannot %sregister socket service handler (%d)", "un", ret);
		}

		zsock_close(ep->sock);
	}

	ret = k_work_cancel_delayable(&ep->recovery.pto_work);
	if (ret != 0) {
		NET_DBG("[EP:%p/%d] PTO work cancel issue (%d)",
			ep, quic_get_by_ep(ep), ret);
	}

	if (ep->is_closing_notified) {
		NET_DBG("[EP:%p/%d] Already notified streams closing", ep,
			quic_get_by_ep(ep));
	} else {
		NET_DBG("[EP:%p/%d] Notifying streams of connection close", ep,
			quic_get_by_ep(ep));
		quic_endpoint_notify_streams_closed(ep);
	}

	/* Remove this endpoint from its owning context's endpoint list.
	 * Without this, the freed slot's node.next is zeroed by memset in
	 * alloc_endpoint, corrupting any context slist that still references
	 * this node.
	 */
	k_mutex_lock(&contexts_lock, K_FOREVER);
	k_mutex_lock(&endpoints_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (!quic_context_is_used(&contexts[i])) {
			continue;
		}

		if (sys_slist_find_and_remove(&contexts[i].endpoints, &ep->node)) {
			break;
		}
	}

	/* Always clean up endpoint state */
	quic_tls_cleanup(&ep->crypto.tls);

	quic_crypto_context_destroy(&ep->crypto.initial);
	quic_crypto_context_destroy(&ep->crypto.handshake);
	quic_crypto_context_destroy(&ep->crypto.application);

	if (ep->parent != NULL) {
		/* The ref was taken in process_long_header() when we assigned
		 * the parent, so unref it here
		 */
		quic_endpoint_unref(ep->parent);
		ep->parent = NULL;
	}

	ep->sock = -1;

	__ASSERT(ep->slab_index >= 0 && ep->slab_index < ARRAY_SIZE(endpoints),
		 "Invalid slab index %d for endpoint %p", ep->slab_index, ep);

	NET_DBG("[EP:%p/%d] Freeing endpoint from slab %p", ep, quic_get_by_ep(ep), ep->slab);

	ep->slab_index = -1;

	k_mem_slab_free(ep->slab, (void *)ep);

	k_mutex_unlock(&endpoints_lock);
	k_mutex_unlock(&contexts_lock);

	return 0;
}

static struct quic_endpoint *find_endpoint(const struct net_sockaddr *remote_addr,
					   const struct net_sockaddr *local_addr,
					   uint8_t *peer_cid, uint8_t peer_cid_len,
					   uint8_t *my_cid, uint8_t my_cid_len)
{
	struct quic_endpoint *ep = NULL;

	ARRAY_FOR_EACH(endpoints, i) {
		if (endpoints[i] == NULL || atomic_get(&endpoints[i]->refcount) == 0) {
			continue;
		}

		if (endpoints[i]->sock < 0) {
			continue;
		}

		/* For client endpoints, the server's SCID in the response won't match
		 * the client's stored peer_cid (the DCID we sent in ClientHello).
		 * So we skip peer_cid matching for client endpoints and rely on
		 * my_cid and address matching.
		 */
		if (peer_cid != NULL && endpoints[i]->is_server &&
		    (peer_cid_len != endpoints[i]->peer_cid_len ||
		     memcmp(peer_cid, endpoints[i]->peer_cid, peer_cid_len) != 0)) {
			continue;
		}

		if (my_cid != NULL &&
		    (my_cid_len != endpoints[i]->my_cid_len ||
		     memcmp(my_cid, endpoints[i]->my_cid, my_cid_len) != 0)) {
			/* For server endpoints, if the incoming DCID (my_cid) doesn't match
			 * our new generated my_cid, check if it matches the original DCID
			 * sent by the client. This handles retransmitted Initial packets.
			 */
			if (!endpoints[i]->is_server ||
			    endpoints[i]->peer_orig_dcid_len == 0 ||
			    my_cid_len != endpoints[i]->peer_orig_dcid_len ||
			    memcmp(my_cid, endpoints[i]->peer_orig_dcid, my_cid_len) != 0) {
				continue;
			}
		}

		if (IS_ENABLED(CONFIG_NET_IPV4) && remote_addr->sa_family == NET_AF_INET) {
			struct net_sockaddr_in *raddr_in =
				net_sin(net_sad(&endpoints[i]->remote_addr));
			struct net_sockaddr_in *laddr_in =
				net_sin(net_sad(&endpoints[i]->local_addr));

			if (raddr_in->sin_family != NET_AF_INET) {
				goto check_next;
			}

			if (net_ipv4_addr_cmp(&raddr_in->sin_addr,
					      &net_sin(remote_addr)->sin_addr) &&
			    raddr_in->sin_port == net_sin(remote_addr)->sin_port) {
				/* If we do not care what the local address is,
				 * return the first match.
				 */
				if (local_addr == NULL) {
					ep = endpoints[i];
					break;
				}

				if (net_ipv4_addr_cmp(&laddr_in->sin_addr,
						      &net_sin(local_addr)->sin_addr) &&
				    laddr_in->sin_port == net_sin(local_addr)->sin_port) {
					ep = endpoints[i];
					break;
				}

				continue;
			}
		}

check_next:
		if (IS_ENABLED(CONFIG_NET_IPV6) && remote_addr->sa_family == NET_AF_INET6) {
			struct net_sockaddr_in6 *raddr_in =
				net_sin6(net_sad(&endpoints[i]->remote_addr));
			struct net_sockaddr_in6 *laddr_in =
				net_sin6(net_sad(&endpoints[i]->local_addr));

			if (raddr_in->sin6_family != NET_AF_INET6) {
				continue;
			}

			if (net_ipv6_addr_cmp(&raddr_in->sin6_addr,
					      &net_sin6(remote_addr)->sin6_addr) &&
			    raddr_in->sin6_port == net_sin6(remote_addr)->sin6_port) {
				/* If we do not care what the local address is,
				 * return the first match.
				 */
				if (local_addr == NULL) {
					ep = endpoints[i];
					break;
				}

				if (net_ipv6_addr_cmp(&laddr_in->sin6_addr,
						      &net_sin6(local_addr)->sin6_addr) &&
				    laddr_in->sin6_port == net_sin6(local_addr)->sin6_port) {
					ep = endpoints[i];
					break;
				}

				continue;
			}
		}
	}

	return ep;
}

static struct quic_endpoint *find_endpoint_lock(const struct net_sockaddr *remote_addr,
						const struct net_sockaddr *local_addr,
						uint8_t *peer_cid, uint8_t peer_cid_len,
						uint8_t *my_cid, uint8_t my_cid_len)
{
	struct quic_endpoint *ep;

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	ep = find_endpoint(remote_addr, local_addr, peer_cid, peer_cid_len,
			  my_cid, my_cid_len);

	k_mutex_unlock(&endpoints_lock);

	return ep;
}

static struct quic_endpoint *quic_endpoint_lookup(const struct net_sockaddr *remote_addr,
						  const struct net_sockaddr *local_addr,
						  uint8_t *peer_cid, uint8_t peer_cid_len,
						  uint8_t *my_cid, uint8_t my_cid_len)
{
	return find_endpoint_lock(remote_addr, local_addr,
				  peer_cid, peer_cid_len,
				  my_cid, my_cid_len);
}

static struct quic_endpoint *find_local_endpoint(const struct net_sockaddr *local_addr)
{
	struct quic_endpoint *ep = NULL;

	if (local_addr == NULL) {
		return NULL;
	}

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	ARRAY_FOR_EACH(endpoints, i) {
		if (endpoints[i] == NULL || atomic_get(&endpoints[i]->refcount) == 0) {
			continue;
		}

		if (IS_ENABLED(CONFIG_NET_IPV4) && local_addr->sa_family == NET_AF_INET) {
			struct net_sockaddr_in *laddr_in =
				net_sin(net_sad(&endpoints[i]->local_addr));

			if (laddr_in->sin_family != NET_AF_INET) {
				goto check_next;
			}

			if (net_ipv4_addr_cmp(&laddr_in->sin_addr,
					      &net_sin(local_addr)->sin_addr) &&
			    (net_sin(local_addr)->sin_port == 0 ||
			     laddr_in->sin_port == net_sin(local_addr)->sin_port)) {
				ep = endpoints[i];
				quic_endpoint_ref(ep);
				break;
			}
		}

check_next:
		if (IS_ENABLED(CONFIG_NET_IPV6) && local_addr->sa_family == NET_AF_INET6) {
			struct net_sockaddr_in6 *laddr_in =
				net_sin6(net_sad(&endpoints[i]->local_addr));

			if (laddr_in->sin6_family != NET_AF_INET6) {
				continue;
			}

			if (net_ipv6_addr_cmp(&laddr_in->sin6_addr,
					      &net_sin6(local_addr)->sin6_addr) &&
			    (net_sin6(local_addr)->sin6_port == 0 ||
			     laddr_in->sin6_port == net_sin6(local_addr)->sin6_port)) {
				ep = endpoints[i];
				quic_endpoint_ref(ep);
				break;
			}
		}
	}

	k_mutex_unlock(&endpoints_lock);

	return ep;
}

/* Find endpoint by socket fd, returns endpoint that owns the socket */
static struct quic_endpoint *find_endpoint_by_sock(int sock)
{
	struct quic_endpoint *ep = NULL;

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	ARRAY_FOR_EACH(endpoints, i) {
		if (endpoints[i] == NULL || atomic_get(&endpoints[i]->refcount) == 0) {
			continue;
		}

		if (endpoints[i]->sock == sock) {
			ep = endpoints[i];
			break;
		}
	}

	k_mutex_unlock(&endpoints_lock);

	return ep;
}

/* Get the next idle timeout checker interval in milliseconds */
static void quic_check_idle_timeouts(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	int64_t now = k_uptime_get();
	int64_t shortest = 0;

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	for (int i = 0; i < CONFIG_QUIC_MAX_ENDPOINTS; i++) {
		struct quic_endpoint *ep = endpoints[i];
		int64_t idle_time, remaining;

		if (ep == NULL || !quic_endpoint_is_used(ep)) {
			continue;
		}

		/* Skip handshake timeout for listening endpoints (server parent).
		 * Listening endpoints don't perform handshakes, they just wait for
		 * incoming connections. Only child endpoints (with parent != NULL)
		 * or client endpoints should have handshake timeout.
		 */
		if (ep->is_server && ep->parent == NULL) {
			/* This is a listening endpoint, skip handshake/idle checks */
			continue;
		}

		/* Check handshake timeout */
		if (!ep->handshake.completed) {
			int64_t handshake_time = now - ep->handshake.start_time;

			if (handshake_time >= (int64_t)ep->handshake.timeout_ms) {
				NET_DBG("[EP:%p/%d] Handshake timeout (%" PRId64 " ms), closing",
					ep, quic_get_by_ep(ep), handshake_time);

				quic_endpoint_send_connection_close(ep,
								    QUIC_ERROR_PROTOCOL_VIOLATION,
								    "Handshake timeout");

				quic_endpoint_unref(ep);
				continue;
			}
		}

		if (ep->idle.idle_timeout_disabled) {
			continue; /* Idle timeout disabled for this endpoint */
		}

		if (ep->idle.negotiated_timeout_ms == 0 ||
		    ep->idle.last_rx_time == 0) {
			continue; /* Idle timeout not negotiated yet */
		}


		idle_time = now - ep->idle.last_rx_time;

		if (idle_time >= (int64_t)ep->idle.negotiated_timeout_ms) {
			NET_INFO("[EP:%p/%d] Idle timeout (%" PRId64 " ms), closing",
				 ep, quic_get_by_ep(ep), idle_time);

			/* Per RFC 9000: No CONNECTION_CLOSE is sent for idle timeout */
			quic_endpoint_unref(ep);
			continue;
		}

		/* Figure out how much time until this endpoint hits idle timeout */
		remaining = (int64_t)ep->idle.negotiated_timeout_ms - idle_time;
		if (shortest > remaining || shortest == 0) {
			shortest = remaining;
		}
	}

	k_mutex_unlock(&endpoints_lock);

	if (shortest > 0) {
		k_work_reschedule(dwork, K_MSEC(shortest));
	}
}

static K_WORK_DELAYABLE_DEFINE(idle_timeout_work, quic_check_idle_timeouts);

/* Start the idle timeout checker */
static void quic_start_idle_timeout_checker(void)
{
	k_work_reschedule(&idle_timeout_work, K_NO_WAIT);
}

static void quic_endpoint_negotiate_idle_timeout(struct quic_endpoint *ep)
{
	uint64_t peer_timeout_ms = ep->peer_params.max_idle_timeout;
	uint64_t local_timeout_ms = ep->idle.local_max_idle_timeout_ms;

	if (peer_timeout_ms == 0 && local_timeout_ms == 0) {
		/* Both side disabled idle timeout */
		ep->idle.negotiated_timeout_ms = 0;
		ep->idle.idle_timeout_disabled = true;
		NET_INFO("Idle timeout disabled for endpoint");
		return;
	}

	/* Use minimum of both values. We do allow peer to disable idle
	 * timeout (unless we also disabled it) so that peer cannot consume
	 * our resources forever.
	 */
	if (peer_timeout_ms == 0) {
		peer_timeout_ms = UINT64_MAX;
	}

	ep->idle.negotiated_timeout_ms = MIN(peer_timeout_ms, local_timeout_ms);
	ep->idle.idle_timeout_disabled = false;

	NET_DBG("[EP:%p/%d] Negotiated idle timeout: %" PRIu64 " ms",
		ep, quic_get_by_ep(ep), ep->idle.negotiated_timeout_ms);
}

/**
 * Convert encryption level to packet number space index (0, 1, or 2)
 */
static inline int level_to_pn_space(enum quic_secret_level level)
{
	switch (level) {
	case QUIC_SECRET_LEVEL_INITIAL:
		return 0;
	case QUIC_SECRET_LEVEL_HANDSHAKE:
		return 1;
	case QUIC_SECRET_LEVEL_APPLICATION:
	default:
		return 2;
	}
}

/* RFC 9002 Section 6.4: discard all in-flight packet state for a PN space.
 * Must be called when Initial or Handshake keys are dropped.
 */
static void quic_recovery_discard_pn_space(struct quic_endpoint *ep, int pn_space)
{
	uint64_t discarded = 0;

	for (int i = 0; i < CONFIG_QUIC_SENT_PKT_HISTORY_SIZE; i++) {
		struct quic_sent_pkt_info *info;

		info = &ep->recovery.sent_pkts[pn_space][i];
		if (info->in_flight) {
			discarded += info->sent_bytes;
			ep->recovery.bytes_in_flight -= info->sent_bytes;
			info->in_flight = false;
		}

		/* Clear the slot entirely so the ring buffer is clean for any reuse */
		info->has_stream_frame = false;
	}

	ep->recovery.sent_pkts_idx[pn_space] = 0;
	ep->recovery.largest_acked[pn_space] = 0;

	NET_DBG("[EP:%p/%d] Discarded pn_space %d, cleared %" PRIu64
		" bytes, bytes_in_flight=%" PRIu64,
		ep, quic_get_by_ep(ep), pn_space, discarded,
		ep->recovery.bytes_in_flight);

	/* Immediately cancel or reschedule the PTO timer to reflect the
	 * updated bytes_in_flight, the timer was armed before the discard
	 * and would otherwise fire based on stale state.
	 */
	quic_reset_pto_timer(ep);
}

static void quic_recovery_init(struct quic_endpoint *ep)
{
	/* RFC 9002 Section 6.2.2: Initialize RTT with initial estimate */
	ep->recovery.smoothed_rtt = QUIC_INITIAL_RTT_US;
	ep->recovery.rtt_var = QUIC_INITIAL_RTT_US / 2;
	ep->recovery.min_rtt = UINT64_MAX;
	ep->recovery.latest_rtt = 0;
	ep->recovery.rtt_initialized = false;
	ep->recovery.bytes_in_flight = 0;
	ep->recovery.pto_count = 0;

	/* Clear sent packet history */
	for (int pn_space = 0; pn_space < 3; pn_space++) {
		ep->recovery.sent_pkts_idx[pn_space] = 0;
		ep->recovery.largest_acked[pn_space] = 0;

		for (int i = 0; i < CONFIG_QUIC_SENT_PKT_HISTORY_SIZE; i++) {
			ep->recovery.sent_pkts[pn_space][i].in_flight = false;
			ep->recovery.sent_pkts[pn_space][i].has_stream_frame = false;
		}
	}

	k_work_init_delayable(&ep->recovery.pto_work, quic_pto_work_handler);

	NET_DBG("[EP:%p/%d] Recovery state initialized, initial SRTT=%" PRIu64 " us",
		ep, quic_get_by_ep(ep), ep->recovery.smoothed_rtt);
}

static void quic_recovery_on_packet_sent(struct quic_endpoint *ep,
					 enum quic_secret_level level,
					 uint64_t pkt_num,
					 size_t sent_bytes,
					 bool ack_eliciting)
{
	int pn_space = level_to_pn_space(level);
	uint16_t idx = ep->recovery.sent_pkts_idx[pn_space];
	struct quic_sent_pkt_info *info = &ep->recovery.sent_pkts[pn_space][idx];

	/* If we're overwriting a packet still in flight, decrement bytes_in_flight */
	if (info->in_flight) {
		ep->recovery.bytes_in_flight -= info->sent_bytes;
	}

	/* Record this packet */
	info->pkt_num = pkt_num;
	info->sent_time = k_uptime_get();
	info->sent_bytes = (uint16_t)MIN(sent_bytes, UINT16_MAX);
	info->ack_eliciting = ack_eliciting;
	info->in_flight = ack_eliciting; /* Only ack-eliciting packets count */
	info->has_stream_frame  = false;

	/* Update bytes in flight */
	if (ack_eliciting) {
		ep->recovery.bytes_in_flight += info->sent_bytes;
		quic_reset_pto_timer(ep);
	}

	/* Advance ring buffer index */
	ep->recovery.sent_pkts_idx[pn_space] =
		(idx + 1) % CONFIG_QUIC_SENT_PKT_HISTORY_SIZE;

	NET_DBG("[EP:%p/%d] Sent pkt pn=%" PRIu64 ", %zu bytes, ack_eliciting=%d, "
		"bytes_in_flight=%" PRIu64,
		ep, quic_get_by_ep(ep), pkt_num, sent_bytes, ack_eliciting,
		ep->recovery.bytes_in_flight);
}

static void quic_stream_advance_tx_acked(struct quic_endpoint *ep,
					 uint64_t stream_id,
					 uint64_t acked_end)
{
	struct quic_stream *stream = quic_find_stream_by_id(ep, stream_id);
	struct quic_stream_tx_buffer *tx;
	uint64_t new_base;
	size_t advance;

	if (stream == NULL) {
		return;
	}

	/* Only advance if this ACK extends the contiguous frontier.
	 * Out-of-order ACKs (acked_end <= bytes_acked) are ignored;
	 * the data they cover stays in the buffer until the gap is filled.
	 */
	if (acked_end <= stream->bytes_acked) {
		return;
	}

	stream->bytes_acked = acked_end;

	tx = &stream->tx_buf;
	new_base = stream->bytes_acked;

	if (new_base <= tx->base_offset) {
		return; /* nothing new to release */
	}

	advance = (size_t)(new_base - tx->base_offset);
	if (advance > tx->len) {
		advance = tx->len; /* clamp, shouldn't happen */
	}

	memmove(tx->data, tx->data + advance, tx->len - advance);
	tx->len -= advance;
	tx->base_offset += advance;

	/* Signal that the stream is now writable (TX buffer has space) */
	k_poll_signal_raise(&stream->send.signal, 0);
}

/*
 * Scatter-gather variant of quic_send_packet().
 * Accepts a small frame header and a separate data buffer,
 * assembling them into ep->crypto.tx_buffer internally.
 * Avoids requiring a large frame[] on the caller's stack.
 */
static int quic_send_packet_sg(struct quic_endpoint *ep,
			       enum quic_secret_level level,
			       const uint8_t *hdr, size_t hdr_len,
			       const uint8_t *data, size_t data_len)
{
	size_t plaintext_len = hdr_len + data_len;

	if (plaintext_len > sizeof(ep->crypto.tx_buffer)) {
		return -ENOBUFS;
	}

	/* Assemble plaintext directly into the encryption buffer.
	 * This is safe: nothing else touches tx_buffer at this point.
	 */
	memcpy(ep->crypto.tx_buffer, hdr, hdr_len);
	if (data_len > 0) {
		memcpy(ep->crypto.tx_buffer + hdr_len, data, data_len);
	}

	/* Delegate to the internal _from_txbuf variant that skips the
	 * redundant payload copy inside quic_send_packet().
	 */
	return quic_send_packet_from_txbuf(ep, level, plaintext_len);
}

static void quic_annotate_last_sent_stream(struct quic_endpoint *ep,
					   enum quic_secret_level level,
					   uint64_t stream_id,
					   uint64_t stream_offset,
					   uint16_t stream_data_len,
					   bool stream_fin)
{
	int pn_space = level_to_pn_space(level);
	uint16_t last_idx = (ep->recovery.sent_pkts_idx[pn_space] +
			     CONFIG_QUIC_SENT_PKT_HISTORY_SIZE - 1) %
		CONFIG_QUIC_SENT_PKT_HISTORY_SIZE;
	struct quic_sent_pkt_info *info =
		&ep->recovery.sent_pkts[pn_space][last_idx];

	info->has_stream_frame  = true;
	info->stream_id         = stream_id;
	info->stream_offset     = stream_offset;
	info->stream_data_len   = stream_data_len;
	info->stream_fin        = stream_fin;
}

static void quic_retransmit_stream_frame(struct quic_endpoint *ep,
					 const struct quic_sent_pkt_info *lost)
{
	/* Small fixed header, 1 + 8 + 8 + 8 bytes max */
	uint8_t hdr[32];
	size_t hdr_len = 0;
	struct quic_stream_tx_buffer *tx;
	struct quic_stream *stream;
	uint8_t frame_type;
	size_t buf_off;
	int ret;

	stream = quic_find_stream_by_id(ep, lost->stream_id);
	if (stream == NULL) {
		return;
	}

	tx = &stream->tx_buf;

	if (lost->stream_offset < tx->base_offset) {
		return; /* already ACKed */
	}

	buf_off = (size_t)(lost->stream_offset - tx->base_offset);
	if (buf_off + lost->stream_data_len > tx->len) {
		NET_WARN("[EP:%p/%d] Lost frame not in TX buffer",
			 ep, quic_get_by_ep(ep));
		return;
	}

	/* Build STREAM frame header into the small stack buffer */
	frame_type = QUIC_FRAME_TYPE_STREAM_BASE | 0x04 | 0x02;
	if (lost->stream_fin) {
		frame_type |= 0x01;
	}
	hdr[hdr_len++] = frame_type;

	ret = quic_put_len(&hdr[hdr_len], sizeof(hdr) - hdr_len, lost->stream_id);
	if (ret != 0) {
		return;
	}
	hdr_len += quic_get_varint_size(lost->stream_id);

	ret = quic_put_len(&hdr[hdr_len], sizeof(hdr) - hdr_len, lost->stream_offset);
	if (ret != 0) {
		return;
	}
	hdr_len += quic_get_varint_size(lost->stream_offset);

	ret = quic_put_len(&hdr[hdr_len], sizeof(hdr) - hdr_len, lost->stream_data_len);
	if (ret != 0) {
		return;
	}
	hdr_len += quic_get_varint_size(lost->stream_data_len);

	/* Send: header from stack, payload directly from tx_buf */
	ret = quic_send_packet_sg(ep, QUIC_SECRET_LEVEL_APPLICATION,
				  hdr, hdr_len,
				  &tx->data[buf_off], lost->stream_data_len);
	if (ret == 0) {
		quic_annotate_last_sent_stream(ep, QUIC_SECRET_LEVEL_APPLICATION,
					       lost->stream_id, lost->stream_offset,
					       lost->stream_data_len, lost->stream_fin);
	}
}

/* Maximum number of ACK ranges we track from a single ACK frame.
 * The first range is always present; additional ranges come from
 * gap+range pairs in the ACK frame (RFC 9000 Section 19.3).
 */
#define QUIC_MAX_ACK_RANGES 8

struct quic_ack_range {
	uint64_t start;  /* lowest acked pkt_num in this range (inclusive) */
	uint64_t end;    /* highest acked pkt_num in this range (inclusive) */
};

static bool pkt_num_in_ack_ranges(uint64_t pkt_num,
				  const struct quic_ack_range *ranges,
				  int count)
{
	for (int i = 0; i < count; i++) {
		if (pkt_num >= ranges[i].start && pkt_num <= ranges[i].end) {
			return true;
		}
	}

	return false;
}

static void quic_detect_lost_packets(struct quic_endpoint *ep,
				     int pn_space,
				     uint64_t largest_ack)
{
	int64_t  now_ms = k_uptime_get();

	/* RFC 9002 Section 6.1.2: time threshold in milliseconds.
	 * loss_delay = max(K_TIME_THRESHOLD * SRTT, GRANULARITY)
	 * QUIC_TIME_THRESHOLD = 1125 (represents 9/8 scaled by 1000)
	 */
	uint64_t srtt_ms    = ep->recovery.smoothed_rtt / 1000U;
	uint64_t loss_delay = (srtt_ms * QUIC_TIME_THRESHOLD) / 1000U;
	bool pkt_threshold;
	bool time_threshold;

	if (loss_delay < (QUIC_GRANULARITY_US / 1000U)) {
		loss_delay = QUIC_GRANULARITY_US / 1000U;
	}

	for (int i = 0; i < CONFIG_QUIC_SENT_PKT_HISTORY_SIZE; i++) {
		struct quic_sent_pkt_info *info =
			&ep->recovery.sent_pkts[pn_space][i];

		if (!info->in_flight) {
			continue;
		}

		/* Only consider packets older than largest_ack */
		if (info->pkt_num >= largest_ack) {
			continue;
		}

		pkt_threshold  = (largest_ack >= info->pkt_num + QUIC_PACKET_THRESHOLD);
		time_threshold = ((now_ms - info->sent_time) >= (int64_t)loss_delay);

		if (!pkt_threshold && !time_threshold) {
			continue;
		}

		NET_DBG("[EP:%p/%d] Packet %" PRIu64 " declared lost "
			"(pkt_threshold=%d time_threshold=%d age=%" PRId64
			"ms delay=%" PRIu64 "ms)",
			ep, quic_get_by_ep(ep), info->pkt_num, pkt_threshold,
			time_threshold,
			(long long)(now_ms - info->sent_time),
			(unsigned long long)loss_delay);

		ep->recovery.bytes_in_flight -= info->sent_bytes;
		info->in_flight = false;

		if (info->has_stream_frame) {
			quic_retransmit_stream_frame(ep, info);
		}
	}
}

static void quic_recovery_on_ack_received(struct quic_endpoint *ep,
					  enum quic_secret_level level,
					  const struct quic_ack_range *ranges,
					  int range_count,
					  uint64_t ack_delay_us)
{
	int pn_space = level_to_pn_space(level);
	int64_t now = k_uptime_get();
	bool found_largest = false;
	int64_t largest_sent_time = 0;
	uint64_t largest_ack;

	if (range_count <= 0) {
		return;
	}

	/* The first range always contains the largest acknowledged PN */
	largest_ack = ranges[0].end;

	if (largest_ack > ep->recovery.largest_acked[pn_space]) {
		ep->recovery.largest_acked[pn_space] = largest_ack;
	}

	/* Find and mark acked packets, calculate RTT from largest_ack */
	for (int i = 0; i < CONFIG_QUIC_SENT_PKT_HISTORY_SIZE; i++) {
		struct quic_sent_pkt_info *info = &ep->recovery.sent_pkts[pn_space][i];

		if (!info->in_flight) {
			continue;
		}

		/* Only mark packets that fall within one of the ACK ranges */
		if (!pkt_num_in_ack_ranges(info->pkt_num, ranges, range_count)) {
			continue;
		}

		/* Decrement bytes in flight */
		ep->recovery.bytes_in_flight -= info->sent_bytes;
		info->in_flight = false;

		/* Release ACKed data from TX buffer */
		if (info->has_stream_frame) {
			quic_stream_advance_tx_acked(
				ep, info->stream_id,
				info->stream_offset + info->stream_data_len);
		}

		/* Track the largest acked packet's send time for RTT */
		if (info->pkt_num == largest_ack) {
			found_largest = true;
			largest_sent_time = info->sent_time;
		}
	}

	/* Update RTT if we found the largest acked packet */
	if (found_largest && largest_sent_time > 0) {
		/* Calculate latest_rtt in microseconds */
		int64_t rtt_ms = now - largest_sent_time;
		uint64_t latest_rtt_us = (uint64_t)rtt_ms * 1000;

		ep->recovery.latest_rtt = latest_rtt_us;

		/* Update min_rtt */
		if (latest_rtt_us < ep->recovery.min_rtt) {
			ep->recovery.min_rtt = latest_rtt_us;
		}

		/* RFC 9002 Section 5.3: RTT estimation */
		if (!ep->recovery.rtt_initialized) {
			/* First RTT sample */
			ep->recovery.smoothed_rtt = latest_rtt_us;
			ep->recovery.rtt_var = latest_rtt_us / 2;
			ep->recovery.rtt_initialized = true;
		} else {
			/* Adjust ack_delay, cannot exceed max_ack_delay after handshake */
			uint64_t adjusted_rtt = latest_rtt_us;
			uint64_t rtt_diff;

			if (latest_rtt_us > ep->recovery.min_rtt + ack_delay_us) {
				adjusted_rtt = latest_rtt_us - ack_delay_us;
			}

			/* RFC 9002 equations:
			 * rtt_var = 3/4 * rtt_var + 1/4 * |smoothed_rtt - adjusted_rtt|
			 * smoothed_rtt = 7/8 * smoothed_rtt + 1/8 * adjusted_rtt
			 */

			if (ep->recovery.smoothed_rtt > adjusted_rtt) {
				rtt_diff = ep->recovery.smoothed_rtt - adjusted_rtt;
			} else {
				rtt_diff = adjusted_rtt - ep->recovery.smoothed_rtt;
			}

			ep->recovery.rtt_var = (3 * ep->recovery.rtt_var + rtt_diff) / 4;
			ep->recovery.smoothed_rtt =
				(7 * ep->recovery.smoothed_rtt + adjusted_rtt) / 8;
		}

		NET_DBG("[EP:%p/%d] RTT update: level %d, latest=%" PRIu64 " us, "
			"smoothed=%" PRIu64 " us, var=%" PRIu64	" us, "
			"min=%" PRIu64 " us, bytes_in_flight=%" PRIu64,
			ep, quic_get_by_ep(ep), level,
			ep->recovery.latest_rtt,
			ep->recovery.smoothed_rtt,
			ep->recovery.rtt_var,
			ep->recovery.min_rtt,
			ep->recovery.bytes_in_flight);
	} else {
		NET_DBG("[EP:%p/%d] ACK received: largest=%" PRIu64
			", bytes_in_flight=%" PRIu64,
			ep, quic_get_by_ep(ep), largest_ack, ep->recovery.bytes_in_flight);
	}

	/* Run loss detection after processing the ACK */
	quic_detect_lost_packets(ep, pn_space, largest_ack);

	/* Reset PTO timer aspackets are moving */
	ep->recovery.pto_count = 0;
	quic_reset_pto_timer(ep);
}

/* RFC 9002 specifies minimum PTO count of 3 */
#define QUIC_MIN_PTO_COUNT 3

static uint8_t quic_compute_max_pto_count(uint64_t pto_base_ms)
{
	uint8_t n;

	/* Prevent division by zero */
	if (pto_base_ms == 0ULL) {
		return 0;
	}

	/* If base PTO already exceeds max, allow zero backoff */
	if (pto_base_ms >= CONFIG_QUIC_MAX_PTO_TIMEOUT_MS) {
		return 0;
	}

	/* Compute the maximum PTO count such that:
	 *   n = floor(log2(max_timeout_ms / pto_base_ms))
	 */
	n = LOG2(CONFIG_QUIC_MAX_PTO_TIMEOUT_MS / pto_base_ms);

	return MAX(QUIC_MIN_PTO_COUNT, n);
}

/* RFC 9002 Section 6.2.1 */
static uint64_t quic_compute_pto_ms(struct quic_endpoint *ep)
{
	uint64_t srtt_ms = ep->recovery.smoothed_rtt / 1000U;
	uint64_t rttvar_ms = ep->recovery.rtt_var / 1000U;
	uint64_t pto;

	/* TODO: do not add ACK_DELAY in handshake phase */
	pto = srtt_ms + MAX(4U * rttvar_ms, 1U) + QUIC_MAX_ACK_DELAY_MS;
	ep->recovery.max_pto_count = quic_compute_max_pto_count(pto);

	/* Exponential backoff */
	pto <<= ep->recovery.pto_count;

	/* Cap to a sensible maximum (e.g., 10 s) */
	return MIN(pto, (uint64_t)CONFIG_QUIC_MAX_PTO_TIMEOUT_MS);
}

static void quic_reset_pto_timer(struct quic_endpoint *ep)
{
	if (ep->recovery.bytes_in_flight == 0) {
		k_work_cancel_delayable(&ep->recovery.pto_work);
		return;
	}

	k_work_reschedule(&ep->recovery.pto_work,
			  K_MSEC(quic_compute_pto_ms(ep)));
}

/* Send a PING to elicit an ACK, or retransmit the oldest unACKed
 * stream frame if one is available. RFC 9002 Section 6.2.4.
 */
static void quic_pto_probe(struct quic_endpoint *ep)
{
	int pn_space = level_to_pn_space(QUIC_SECRET_LEVEL_APPLICATION);

	/* Find the oldest in-flight stream frame */
	struct quic_sent_pkt_info *oldest = NULL;

	for (int i = 0; i < CONFIG_QUIC_SENT_PKT_HISTORY_SIZE; i++) {
		struct quic_sent_pkt_info *info =
			&ep->recovery.sent_pkts[pn_space][i];

		if (!info->in_flight || !info->has_stream_frame) {
			continue;
		}

		if (oldest == NULL || info->sent_time < oldest->sent_time) {
			oldest = info;
		}
	}

	if (oldest != NULL) {
		NET_DBG("[EP:%p/%d] PTO: retransmitting oldest stream frame pn=%" PRIu64,
			ep, quic_get_by_ep(ep), oldest->pkt_num);
		quic_retransmit_stream_frame(ep, oldest);
	} else {
		/* No stream frame to retransmit, send a PING to keep the
		 * connection alive and elicit an ACK.
		 */
		uint8_t ping = QUIC_FRAME_TYPE_PING;

		NET_DBG("[EP:%p/%d] PTO: sending PING probe", ep, quic_get_by_ep(ep));
		quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, &ping, 1);
	}
}

static void quic_pto_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct quic_endpoint *ep = CONTAINER_OF(dwork, struct quic_endpoint,
						recovery.pto_work);

	NET_DBG("[EP:%p/%d] PTO fired (count=%u)", ep, quic_get_by_ep(ep),
		ep->recovery.pto_count);

	ep->recovery.pto_count++;

	/* Drop connection if PTO count exceeds a threshold (e.g. 10 retries) */
	if (ep->recovery.pto_count > ep->recovery.max_pto_count) {
		NET_DBG("[EP:%p/%d] Connection dropped due to excessive PTO "
			"timeouts (was %d, max %d)", ep, quic_get_by_ep(ep),
			(int)ep->recovery.pto_count,
			(int)ep->recovery.max_pto_count);
		quic_endpoint_unref(ep);
		return;
	}

	quic_pto_probe(ep);

	/* Reschedule with backoff for the next probe */
	quic_reset_pto_timer(ep);
}

static void quic_endpoint_init_idle_timeout(struct quic_endpoint *ep,
					    uint64_t local_timeout_ms)
{
	ep->idle.last_rx_time = k_uptime_get();
	ep->idle.local_max_idle_timeout_ms = local_timeout_ms;
	ep->idle.negotiated_timeout_ms = 0; /* Will be set after handshake */
	ep->idle.idle_timeout_disabled = (local_timeout_ms == 0);
}

static void quic_endpoint_init_handshake_timeout(struct quic_endpoint *ep,
						 uint64_t timeout_ms)
{
	ep->handshake.start_time = k_uptime_get();
	ep->handshake.timeout_ms = timeout_ms;
	ep->handshake.completed = false;
}

static void quic_endpoint_handshake_complete(struct quic_endpoint *ep)
{
	int64_t duration = k_uptime_get() - ep->handshake.start_time;

	ep->handshake.completed = true;

	/* Signal any waiters that handshake is complete */
	k_sem_give(&ep->handshake.sem);

	NET_DBG("[EP:%p/%d] Handshake completed in %" PRId64 " ms", ep,
		quic_get_by_ep(ep), duration);
}

#define QUIC_DEFAULT_IDLE_TIMEOUT_MS (30 * MSEC_PER_SEC)  /* Default 30s idle timeout */
#define QUIC_DEFAULT_HANDSHAKE_TIMEOUT_MS (30 * MSEC_PER_SEC)  /* Default 30s handshake timeout */

static void quic_endpoint_init(struct quic_endpoint *ep)
{
	ep->sock = -1;
	ep->parent = NULL;
	ep->peer_cid_len = 0;
	ep->my_cid_len = 0;
	memset(ep->peer_cid, 0, sizeof(ep->peer_cid));
	memset(ep->my_cid, 0, sizeof(ep->my_cid));
	memset(ep->peer_orig_dcid, 0, sizeof(ep->peer_orig_dcid));

	/* Default max UDP payload size per RFC 9000 when PMTUD is not performed */
	ep->max_tx_payload_size = 1200;

	/* Initialize flow control with default values.
	 * These will be updated when peer transport params are parsed.
	 */
	ep->tx_fc.max_data = 0;
	ep->tx_fc.bytes_sent = 0;
	ep->rx_fc.max_data = CONFIG_QUIC_INITIAL_MAX_DATA;
	ep->rx_fc.bytes_received = 0;
	ep->rx_fc.max_data_sent = CONFIG_QUIC_INITIAL_MAX_DATA;
	ep->rx_fc.need_window_update = false;
	ep->rx_sl.max_bidi = CONFIG_QUIC_INITIAL_MAX_STREAMS_BIDI;
	ep->rx_sl.max_uni = CONFIG_QUIC_INITIAL_MAX_STREAMS_UNI;
	ep->rx_sl.open_bidi = 0;
	ep->rx_sl.open_uni = 0;
	ep->peer_params.parsed = false;

	quic_endpoint_init_idle_timeout(ep, QUIC_DEFAULT_IDLE_TIMEOUT_MS);
	quic_endpoint_init_handshake_timeout(ep, QUIC_DEFAULT_HANDSHAKE_TIMEOUT_MS);
	quic_recovery_init(ep);

	k_mutex_init(&ep->pending.lock);
	k_sem_init(&ep->handshake.sem, 0, 1);

	tls_init(ep);
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
static struct quic_endpoint *endpoint_alloc(struct k_mem_slab *slab, k_timeout_t timeout,
					    const char *caller, int line)
#else
static struct quic_endpoint *endpoint_alloc(struct k_mem_slab *slab, k_timeout_t timeout)
#endif
{
	struct quic_endpoint *ep = NULL;
	int ret;

	ret = k_mem_slab_alloc(slab, (void **)&ep, timeout);
	if (ret < 0) {
		return NULL;
	}

	memset(ep, 0, sizeof(struct quic_endpoint));

	ep->refcount = ATOMIC_INIT(1);
	ep->slab = slab;
	ep->slab_index = -1;

	/* Populate slab index array for debugging/logging purposes */
	ARRAY_FOR_EACH(endpoints, i) {
		if (endpoints[i] == ep) {
			ep->slab_index = i;
			break;
		}
	}

	if (ep->slab_index == -1) {
		ARRAY_FOR_EACH(endpoints, i) {
			if (endpoints[i] == NULL) {
				endpoints[i] = ep;
				ep->slab_index = i;
				break;
			}
		}
	}

	/* This should never happen since the slab size matches the array size */
	if (ep->slab_index == -1) {
		NET_ERR("[EP:%p/%d] Failed to allocate endpoint, no free slot found",
			ep, quic_get_by_ep(ep));

		k_mem_slab_free(slab, (void **)&ep);

		memset(ep, 0, sizeof(struct quic_endpoint));
		return NULL;
	}

	NET_DBG("[EP:%p/%d] Allocated endpoint from slab %p", ep, quic_get_by_ep(ep), ep->slab);

	return ep;
}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
struct quic_endpoint *endpoint_alloc_from_slab_debug(struct k_mem_slab *slab,
						     k_timeout_t timeout,
						     const char *caller, int line)
#else
struct quic_endpoint *endpoint_alloc_from_slab(struct k_mem_slab *slab,
					       k_timeout_t timeout)
#endif
{
	if (slab == NULL) {
		return NULL;
	}

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
	return endpoint_alloc(slab, timeout, caller, line);
#else
	return endpoint_alloc(slab, timeout);
#endif
}

static struct quic_endpoint *alloc_endpoint(struct quic_context *ctx,
					    const struct net_sockaddr *remote_addr,
					    const struct net_sockaddr *local_addr)
{
	struct quic_endpoint *ep;

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	ep = endpoint_alloc_from_slab(&endpoints_slab, SLAB_ALLOC_TIMEOUT);
	if (ep == NULL) {
		NET_ERR("No free endpoint available");
		k_mutex_unlock(&endpoints_lock);
		return NULL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && remote_addr->sa_family == NET_AF_INET) {
		struct net_sockaddr_in *raddr_in =
			net_sin(net_sad(&ep->remote_addr));
		struct net_sockaddr_in *laddr_in =
			net_sin(net_sad(&ep->local_addr));

		memcpy(raddr_in, remote_addr, sizeof(struct net_sockaddr_in));

		if (local_addr != NULL) {
			memcpy(laddr_in, local_addr, sizeof(struct net_sockaddr_in));
		} else {
			laddr_in->sin_family = NET_AF_INET;
			laddr_in->sin_port = 0;
			laddr_in->sin_addr.s_addr = NET_INADDR_ANY;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && remote_addr->sa_family == NET_AF_INET6) {
		struct net_sockaddr_in6 *raddr_in =
			net_sin6(net_sad(&ep->remote_addr));
		struct net_sockaddr_in6 *laddr_in =
			net_sin6(net_sad(&ep->local_addr));

		memcpy(raddr_in, remote_addr, sizeof(struct net_sockaddr_in6));

		if (local_addr != NULL) {
			memcpy(laddr_in, local_addr, sizeof(struct net_sockaddr_in6));
		} else {
			laddr_in->sin6_family = NET_AF_INET6;
			laddr_in->sin6_port = 0;
			net_ipaddr_copy(&laddr_in->sin6_addr, &net_in6addr_any);
		}
	}

	if (ctx != NULL) {
		sys_slist_find_and_remove(&ctx->endpoints, &ep->node);
		sys_slist_prepend(&ctx->endpoints, &ep->node);
	} else {
		NET_DBG("[EP:%p/%d] No context provided for new endpoint",
			ep, quic_get_by_ep(ep));
	}

	quic_endpoint_init(ep);

	k_mutex_unlock(&endpoints_lock);

	return ep;
}

static struct quic_endpoint *quic_endpoint_create(struct quic_endpoint *ep,
						  const struct net_sockaddr *remote_addr,
						  net_socklen_t addrlen,
						  uint8_t *peer_cid,
						  uint8_t peer_cid_len,
						  uint8_t *my_cid,
						  uint8_t my_cid_len)
{
	struct quic_endpoint *new_ep;
	struct quic_context *ctx;

	ctx = quic_find_context(ep);

	new_ep = alloc_endpoint(ctx, remote_addr, net_sad(&ep->local_addr));
	if (new_ep != NULL) {
		if (peer_cid != NULL && peer_cid_len > 0 &&
		    peer_cid_len <= MAX_CONN_ID_LEN) {
			new_ep->peer_cid_len = peer_cid_len;
			memcpy(new_ep->peer_cid, peer_cid, peer_cid_len);

			/* Also store in peer CID pool as sequence 0 */
			new_ep->peer_cid_pool[0].seq_num = 0;
			new_ep->peer_cid_pool[0].cid_len = peer_cid_len;
			memcpy(new_ep->peer_cid_pool[0].cid, peer_cid, peer_cid_len);
			/* Initial CID has no stateless reset token */
			memset(new_ep->peer_cid_pool[0].stateless_reset_token, 0,
			       QUIC_RESET_TOKEN_LEN);
			new_ep->peer_cid_pool[0].active = true;
		}

		if (my_cid != NULL && my_cid_len > 0 &&
		    my_cid_len <= MAX_CONN_ID_LEN) {
			new_ep->my_cid_len = my_cid_len;
			memcpy(new_ep->my_cid, my_cid, my_cid_len);
		}

		new_ep->peer_params.initial_max_data = ep->peer_params.initial_max_data;
		new_ep->tx_fc.max_data = ep->tx_fc.max_data;
		new_ep->peer_params.initial_max_stream_data_bidi_local =
			ep->peer_params.initial_max_stream_data_bidi_local;
		new_ep->peer_params.initial_max_stream_data_bidi_remote =
			ep->peer_params.initial_max_stream_data_bidi_remote;
		new_ep->peer_params.initial_max_stream_data_uni =
			ep->peer_params.initial_max_stream_data_uni;
		new_ep->peer_params.initial_max_streams_bidi =
			ep->peer_params.initial_max_streams_bidi;
		new_ep->peer_params.initial_max_streams_uni =
			ep->peer_params.initial_max_streams_uni;
		new_ep->peer_params.max_idle_timeout = ep->peer_params.max_idle_timeout;
		new_ep->max_tx_payload_size = ep->max_tx_payload_size;
		new_ep->rx_sl.max_bidi = ep->rx_sl.max_bidi;
		new_ep->rx_sl.max_uni = ep->rx_sl.max_uni;

		/* Child starts with no open streams (it is a fresh connection) */
		new_ep->rx_sl.open_bidi = 0;
		new_ep->rx_sl.open_uni = 0;

		new_ep->sock = ep->sock;

		NET_DBG("[EP:%p/%d] Child endpoint inherits socket %d from parent %d [%p]",
			new_ep, quic_get_by_ep(new_ep), new_ep->sock, quic_get_by_ep(ep), ep);
	} else {
		NET_DBG("[EP:%p/%d] Cannot allocate new endpoint", ep, quic_get_by_ep(ep));
	}

	return new_ep;
}

static struct quic_endpoint *alloc_local_endpoint(struct quic_context *ctx,
						  const struct net_sockaddr *local_addr)
{
	struct quic_endpoint *ep;

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	ep = endpoint_alloc_from_slab(&endpoints_slab, SLAB_ALLOC_TIMEOUT);
	if (ep == NULL) {
		NET_ERR("No free endpoint available");
		k_mutex_unlock(&endpoints_lock);
		return NULL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && local_addr->sa_family == NET_AF_INET) {
		struct net_sockaddr_in *raddr_in =
			net_sin(net_sad(&ep->remote_addr));
		struct net_sockaddr_in *laddr_in =
			net_sin(net_sad(&ep->local_addr));

		memcpy(laddr_in, local_addr, sizeof(struct net_sockaddr_in));

		raddr_in->sin_family = NET_AF_INET;
		raddr_in->sin_port = 0;
		raddr_in->sin_addr.s_addr = NET_INADDR_ANY;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && local_addr->sa_family == NET_AF_INET6) {
		struct net_sockaddr_in6 *raddr_in =
			net_sin6(net_sad(&ep->remote_addr));
		struct net_sockaddr_in6 *laddr_in =
			net_sin6(net_sad(&ep->local_addr));

		memcpy(laddr_in, local_addr, sizeof(struct net_sockaddr_in6));

		raddr_in->sin6_family = NET_AF_INET6;
		raddr_in->sin6_port = 0;
		net_ipaddr_copy(&raddr_in->sin6_addr, &net_in6addr_any);
	}

	sys_slist_find_and_remove(&ctx->endpoints, &ep->node);
	sys_slist_prepend(&ctx->endpoints, &ep->node);

	quic_endpoint_init(ep);

	k_mutex_unlock(&endpoints_lock);

	return ep;
}

static int endpoint_socket_create(struct quic_endpoint *ep)
{
	const struct net_socket_service_desc *svc = NULL;
	int sock;
	int ret;
	size_t addr_len;

	sock = zsock_socket(ep->remote_addr.ss_family,
			    NET_SOCK_DGRAM,
			    NET_IPPROTO_UDP);
	if (sock < 0) {
		ret = -errno;
		goto fail;
	}

	ep->sock = sock;

	ret = pollfd_try_add(ep->remote_addr.ss_family, sock);
	if (ret < 0) {
		goto close_fail;
	}

	if (ep->local_addr.ss_family == NET_AF_INET) {
		addr_len = sizeof(struct net_sockaddr_in);
	} else {
		addr_len = sizeof(struct net_sockaddr_in6);
	}

	if (IS_ENABLED(CONFIG_QUIC_ENDPOINT_USE_IPV4_MAPPING_TO_IPV6)) {
		net_socklen_t optlen = sizeof(int);
		int opt;

		ret = zsock_getsockopt(sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY,
				       &opt, &optlen);
		if (ret == 0 && opt != 0) {
			NET_DBG("[EP:%p/%d] IPV6_V6ONLY option is on for "
				"socket %d, turning it off", ep, quic_get_by_ep(ep),
				sock);

			opt = 0;
			ret = zsock_setsockopt(sock, NET_IPPROTO_IPV6,
					       ZSOCK_IPV6_V6ONLY,
					       &opt, optlen);
			if (ret < 0) {
				NET_WARN("[EP:%p/%d] Cannot turn off IPV6_V6ONLY "
					 "option for socket %d (%d)",
					 ep, quic_get_by_ep(ep), sock, -errno);
			} else {
				NET_DBG("[EP:%p/%d] Sharing same socket %d between "
					"IPv6 and IPv4",
					ep, quic_get_by_ep(ep), sock);
			}
		} else {
			NET_ERR("[EP:%p/%d] Cannot get IPV6_V6ONLY option for "
				"socket %d (%d)",
				ep, quic_get_by_ep(ep), sock, -errno);
		}
	}

	ret = zsock_bind(ep->sock, net_sad(&ep->local_addr), addr_len);
	if (ret < 0) {
		ret = -errno;
		NET_DBG("[EP:%p/%d] Cannot %s socket %d (%d)", ep, quic_get_by_ep(ep),
			"bind", ep->sock, ret);
		goto close_fail;
	}

	ret = zsock_connect(ep->sock, net_sad(&ep->remote_addr), addr_len);
	if (ret < 0) {
		ret = -errno;
		NET_DBG("[EP:%p/%d] Cannot %s socket %d (%d)", ep, quic_get_by_ep(ep),
			"connect", ep->sock, ret);
		goto close_fail;
	}

	if (ep->local_addr.ss_family == NET_AF_INET) {
		NET_DBG("[EP:%p/%d] %s socket service handler for IPv%d",
			ep, quic_get_by_ep(ep), "Registering", 4);
		svc = COND_CODE_1(CONFIG_NET_IPV4, (&svc4), (NULL));
	} else if (ep->local_addr.ss_family == NET_AF_INET6) {
		NET_DBG("[EP:%p/%d] %s socket service handler for IPv%d",
			ep, quic_get_by_ep(ep), "Registering", 6);
		svc = COND_CODE_1(CONFIG_NET_IPV6, (&svc6), (NULL));
	}

	if (svc == NULL) {
		NET_ERR("[EP:%p/%d] Socket service not available for address family %d",
			ep, quic_get_by_ep(ep), ep->local_addr.ss_family);
		ret = -EAFNOSUPPORT;
		goto close_fail;
	}

	if (ep->local_addr.ss_family == NET_AF_INET && IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_socket_service_register(svc,
				COND_CODE_1(CONFIG_NET_IPV4,
					    (quic_ipv4_pollfds), (NULL)),
				COND_CODE_1(CONFIG_NET_IPV4,
					    (ARRAY_SIZE(quic_ipv4_pollfds)), (0)),
				ep);
	} else if (ep->local_addr.ss_family == NET_AF_INET6 && IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = net_socket_service_register(svc,
				COND_CODE_1(CONFIG_NET_IPV6,
					    (quic_ipv6_pollfds), (NULL)),
				COND_CODE_1(CONFIG_NET_IPV6,
					    (ARRAY_SIZE(quic_ipv6_pollfds)), (0)),
				ep);
	} else {
		ret = -EAFNOSUPPORT;
	}

	if (ret < 0) {
		NET_DBG("Cannot %sregister socket service handler (%d)", "", ret);
		goto close_fail;
	}

	return ret;

close_fail:
	zsock_close(ep->sock);
	ep->sock = -1;

fail:
	return ret;
}

static struct quic_context *quic_context_init(struct quic_context *ctx)
{
	k_fifo_init(&ctx->pending.accept_q);
	k_sem_init(&ctx->pending.accept_sem, 0, 1);

	k_fifo_init(&ctx->incoming.stream_q);
	k_sem_init(&ctx->incoming.stream_sem, 0, 1);

	sys_slist_init(&ctx->endpoints);
	sys_slist_init(&ctx->streams);

	ctx->error_code = 0;

	return ctx;
}

static struct quic_context *quic_get(void)
{
	struct quic_context *ctx = NULL;

	k_mutex_lock(&contexts_lock, K_FOREVER);

	ARRAY_FOR_EACH(contexts, i) {
		if (quic_context_is_used(&contexts[i])) {
			continue;
		}

		quic_context_ref(&contexts[i]);
		ctx = quic_context_init(&contexts[i]);
		break;
	}

	k_mutex_unlock(&contexts_lock);

	return ctx;
}

static struct quic_stream *quic_stream_init(struct quic_stream *stream)
{
	/* Condition variable is used to avoid keeping lock for a long time
	 * when waiting data to be received
	 */
	k_condvar_init(&stream->cond.recv);
	k_mutex_init(&stream->cond.data_available);

	/* The event is used when waiting for data to be received with timeout */
	k_poll_signal_init(&stream->recv.signal);
	k_poll_event_init(&stream->recv.event, K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &stream->recv.signal);

	/* Signal for TX buffer ready (writable) */
	k_poll_signal_init(&stream->send.signal);

	memset(&stream->rx_buf, 0, sizeof(stream->rx_buf));
	stream->rx_buf.size = sizeof(stream->rx_buf.data);
	stream->rx_buf.fin_received = false;

	memset(&stream->tx_buf, 0, sizeof(stream->tx_buf));
	stream->tx_buf.base_offset = 0;

	/* Initialize flow control with default values.
	 * This should be updated when peer transport parameters are received.
	 */
	stream->remote_max_data = 16384;
	stream->bytes_sent = 0;
	stream->bytes_acked = 0;

	/* RX flow control. Initialize to what we advertise in transport params */
	stream->local_max_data = MIN(CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
				     sizeof(stream->rx_buf.data));
	stream->local_max_data_sent = CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL;
	stream->bytes_received = 0;
	stream->read_closed = false;
	stream->stop_sending_error_code = QUIC_ERROR_NO_ERROR;
	stream->tx_reset = false;

	return stream;
}

static struct quic_stream *quic_get_stream(struct quic_context *conn)
{
	struct quic_stream *stream = NULL;

	k_mutex_lock(&streams_lock, K_FOREVER);

	ARRAY_FOR_EACH(streams, i) {
		if (quic_stream_is_used(&streams[i])) {
			continue;
		}

		quic_stream_ref(&streams[i]);
		stream = quic_stream_init(&streams[i]);

		sys_slist_find_and_remove(&conn->streams, &stream->node);
		sys_slist_prepend(&conn->streams, &stream->node);

		stream->conn = conn;
		quic_context_ref(conn);
		break;
	}

	k_mutex_unlock(&streams_lock);

	return stream;
}

static void entry_ready(void *obj)
{
	NET_DBG("Entered %s state", "READY");
}

static enum smf_state_result run_ready(void *obj)
{
	NET_DBG("Run %s state", "READY");

	return SMF_EVENT_HANDLED;
}

static void entry_send(void *obj)
{
	NET_DBG("Entered %s state", "SEND");
}

static enum smf_state_result run_send(void *obj)
{
	NET_DBG("Run %s state", "SEND");

	return SMF_EVENT_HANDLED;
}

static void entry_data_sent(void *obj)
{
	NET_DBG("Entered %s state", "DATA_SENT");
}

static enum smf_state_result run_data_sent(void *obj)
{
	NET_DBG("Run %s state", "DATA_SENT");

	return SMF_EVENT_HANDLED;
}

static void entry_data_recvd(void *obj)
{
	NET_DBG("Entered %s state", "DATA_RECVD");
}

static enum smf_state_result run_data_recvd(void *obj)
{
	NET_DBG("Run %s state", "DATA_RECVD");

	return SMF_EVENT_HANDLED;
}

static void entry_reset_sent(void *obj)
{
	NET_DBG("Entered %s state", "RESET_SENT");
}

static enum smf_state_result run_reset_sent(void *obj)
{
	NET_DBG("Run %s state", "RESET_SENT");

	return SMF_EVENT_HANDLED;
}

static void entry_reset_recvd(void *obj)
{
	NET_DBG("Entered %s state", "RESET_RECVD");
}

static enum smf_state_result run_reset_recvd(void *obj)
{
	NET_DBG("Run %s state", "RESET_RECVD");

	return SMF_EVENT_HANDLED;
}

static void entry_recv(void *obj)
{
	NET_DBG("Entered %s state", "RECV");
}

static enum smf_state_result run_recv(void *obj)
{
	NET_DBG("Run %s state", "RECV");

	return SMF_EVENT_HANDLED;
}

static void entry_size_known(void *obj)
{
	NET_DBG("Entered %s state", "SIZE_KNOWN");
}

static enum smf_state_result run_size_known(void *obj)
{
	NET_DBG("Run %s state", "SIZE_KNOWN");

	return SMF_EVENT_HANDLED;
}

static void entry_data_read(void *obj)
{
	NET_DBG("Entered %s state", "DATA_READ");
}

static enum smf_state_result run_data_read(void *obj)
{
	NET_DBG("Run %s state", "DATA_READ");

	return SMF_EVENT_HANDLED;
}

static void entry_reset_read(void *obj)
{
	NET_DBG("Entered %s state", "RESET_READ");
}

static enum smf_state_result run_reset_read(void *obj)
{
	NET_DBG("Run %s state", "RESET_READ");

	return SMF_EVENT_HANDLED;
}

static const struct smf_state quic_stream_bidirectional_states[] = {
	[STATE_READY] = SMF_CREATE_STATE(entry_ready, run_ready, NULL, NULL, NULL),
	[STATE_SEND] = SMF_CREATE_STATE(entry_send, run_send, NULL, NULL, NULL),
	[STATE_DATA_SENT] = SMF_CREATE_STATE(entry_data_sent, run_data_sent, NULL, NULL, NULL),
	[STATE_DATA_RECVD] = SMF_CREATE_STATE(entry_data_recvd, run_data_recvd, NULL, NULL, NULL),
	[STATE_RESET_SENT] = SMF_CREATE_STATE(entry_reset_sent, run_reset_sent, NULL, NULL, NULL),
	[STATE_RESET_RECVD] = SMF_CREATE_STATE(entry_reset_recvd, run_reset_recvd,
					       NULL, NULL, NULL),
	[STATE_RECV] = SMF_CREATE_STATE(entry_recv, run_recv, NULL, NULL, NULL),
	[STATE_SIZE_KNOWN] = SMF_CREATE_STATE(entry_size_known, run_size_known, NULL, NULL, NULL),
	[STATE_DATA_READ] = SMF_CREATE_STATE(entry_data_read, run_data_read, NULL, NULL, NULL),
	[STATE_RESET_READ] = SMF_CREATE_STATE(entry_reset_read, run_reset_read, NULL, NULL, NULL),
};

static enum quic_stream_states quic_stream_get_state(struct quic_stream *stream)
{
	const struct smf_state *state = SMF_CTX(&stream->state.ctx)->current;

	if (!IS_ARRAY_ELEMENT(quic_stream_bidirectional_states, state)) {
		NET_DBG("Stream %p in invalid state %p", stream, state);
		return -1;
	}

	return ARRAY_INDEX(quic_stream_bidirectional_states, state);
}

/*
 * Map TLS cipher suite to QUIC cipher algorithm
 */
static int tls_suite_to_quic_cipher(uint16_t cipher_suite)
{
	switch (cipher_suite) {
	case TLS_AES_128_GCM_SHA256:
		return QUIC_CIPHER_AES_128_GCM;
	case TLS_AES_256_GCM_SHA384:
		return QUIC_CIPHER_AES_256_GCM;
	case TLS_CHACHA20_POLY1305_SHA256:
		return QUIC_CIPHER_CHACHA20_POLY1305;
	default:
		return QUIC_CIPHER_AES_128_GCM;  /* Default fallback */
	}
}

/*
 * Setup ciphers with explicit hash algorithm for handshake/application levels
 */
static bool quic_setup_ciphers_ex(struct quic_ciphers *ciphers,
				  psa_algorithm_t hash_alg,
				  int cipher_algo,
				  const uint8_t *secret,
				  size_t secret_len)
{
	if (!quic_hp_setup_ex(&ciphers->hp, hash_alg, cipher_algo,
			      secret, secret_len)) {
		NET_ERR("Failed to setup HP cipher");
		return false;
	}

	if (!quic_pp_setup_ex(&ciphers->pp, hash_alg, cipher_algo,
			      secret, secret_len)) {
		psa_destroy_key(ciphers->hp.key_id);
		ciphers->hp.initialized = false;
		NET_ERR("Failed to setup PP cipher");
		return false;
	}

	return true;
}

/* Callback to deliver secrets to QUIC crypto layer */
static int quic_tls_secret_callback(void *user_data,
				    enum quic_secret_level level,
				    const uint8_t *rx_secret,
				    const uint8_t *tx_secret,
				    size_t secret_len)
{
	struct quic_endpoint *ep = user_data;
	struct quic_crypto_context *crypto_ctx;
	struct quic_tls_context *tls = &ep->crypto.tls;
	psa_algorithm_t hash_alg;
	int cipher_algo;

	if (IS_ENABLED(CONFIG_QUIC_TLS_DEBUG_KEYLOG)) {
		/* Log secrets for Wireshark */
		if (level == QUIC_SECRET_LEVEL_HANDSHAKE) {
			if (ep->is_server) {
				quic_log_tls_secret("CLIENT_HANDSHAKE_TRAFFIC_SECRET",
						    tls->client_random, rx_secret, secret_len);
				quic_log_tls_secret("SERVER_HANDSHAKE_TRAFFIC_SECRET",
						    tls->client_random, tx_secret, secret_len);
			} else {
				quic_log_tls_secret("SERVER_HANDSHAKE_TRAFFIC_SECRET",
						    tls->client_random, rx_secret, secret_len);
				quic_log_tls_secret("CLIENT_HANDSHAKE_TRAFFIC_SECRET",
						    tls->client_random, tx_secret, secret_len);
			}
		} else if (level == QUIC_SECRET_LEVEL_APPLICATION) {
			if (ep->is_server) {
				quic_log_tls_secret("CLIENT_TRAFFIC_SECRET_0",
						    tls->client_random, rx_secret, secret_len);
				quic_log_tls_secret("SERVER_TRAFFIC_SECRET_0",
						    tls->client_random, tx_secret, secret_len);
			} else {
				quic_log_tls_secret("SERVER_TRAFFIC_SECRET_0",
						    tls->client_random, rx_secret, secret_len);
				quic_log_tls_secret("CLIENT_TRAFFIC_SECRET_0",
						    tls->client_random, tx_secret, secret_len);
			}
		}
	}

	NET_DBG("TLS secret callback, level %d, secret_len=%zu", level, secret_len);

	if (tls == NULL) {
		NET_ERR("TLS context not available");
		return -EINVAL;
	}

	/* Determine hash algorithm and cipher from TLS context */
	if (tls->ks.initialized) {
		/* Get cipher parameters from the negotiated cipher suite */
		hash_alg = tls->ks.hash_alg;
		cipher_algo = tls_suite_to_quic_cipher(tls->ks.cipher_suite);
	} else {
		/* Default for Initial level */
		hash_alg = PSA_ALG_SHA_256;
		cipher_algo = QUIC_CIPHER_AES_128_GCM;
	}

	NET_DBG("Using cipher_suite=0x%04x, hash_alg=0x%08x, cipher_algo=%d",
		tls->ks.cipher_suite, hash_alg, cipher_algo);

	switch (level) {
	case QUIC_SECRET_LEVEL_INITIAL:
		crypto_ctx = &ep->crypto.initial;
		break;
	case QUIC_SECRET_LEVEL_HANDSHAKE:
		crypto_ctx = &ep->crypto.handshake;
		break;
	case QUIC_SECRET_LEVEL_APPLICATION:
		crypto_ctx = &ep->crypto.application;
		break;
	default:
		NET_ERR("Invalid secret level: %d", level);
		return -EINVAL;
	}

	/* Check if already initialized, avoid double initialization */
	if (crypto_ctx->initialized) {
		NET_DBG("Crypto context for level %d already initialized", level);
		return 0;
	}

	/* Setup RX ciphers from rx_secret */
	if (!quic_setup_ciphers_ex(&crypto_ctx->rx,
				   hash_alg,
				   cipher_algo,
				   rx_secret,
				   secret_len)) {
		NET_ERR("Failed to setup RX ciphers for level %d", level);
		return -EIO;
	}

	/* Setup TX ciphers from tx_secret */
	if (!quic_setup_ciphers_ex(&crypto_ctx->tx,
				   hash_alg,
				   cipher_algo,
				   tx_secret,
				   secret_len)) {
		NET_ERR("Failed to setup TX ciphers for level %d", level);

		/* Clean up RX on failure */
		psa_destroy_key(crypto_ctx->rx.hp.key_id);
		psa_destroy_key(crypto_ctx->rx.pp.key_id);
		crypto_ctx->rx.hp.initialized = false;
		crypto_ctx->rx.pp.initialized = false;

		return -EIO;
	}

	crypto_ctx->initialized = true;

	NET_DBG("Crypto context for level %d initialized successfully", level);

	return 0;
}

#include "quic_tls.c"
#include "quic_socket.c"
#include "quic_packet.c"

/*
 * Wrapper for Initial packets
 */
static int handle_initial_packet(struct quic_endpoint *ep,
				 const uint8_t *payload,
				 size_t payload_len,
				 size_t packet_len)
{
	return handle_crypto_level_packet(ep, QUIC_SECRET_LEVEL_INITIAL,
					  payload, payload_len, packet_len, NULL);
}

/*
 * Wrapper for Handshake packets
 */
static int handle_handshake_packet(struct quic_endpoint *ep,
				   const uint8_t *payload,
				   size_t payload_len,
				   size_t packet_len)
{
	return handle_crypto_level_packet(ep, QUIC_SECRET_LEVEL_HANDSHAKE,
					  payload, payload_len, packet_len, NULL);
}

/*
 * Derive application traffic secrets after handshake completes
 */
static int derive_application_secrets(struct quic_tls_context *ctx)
{
	uint8_t zero_ikm[QUIC_HASH_MAX_LEN] = {0};
	uint8_t derived[QUIC_HASH_MAX_LEN];
	uint8_t empty_hash[QUIC_HASH_MAX_LEN];
	size_t empty_hash_len;
	psa_status_t status;
	int ret;

	/* Get hash of empty string */
	status = psa_hash_compute(ctx->ks.hash_alg, NULL, 0,
				  empty_hash, sizeof(empty_hash), &empty_hash_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	/* derived_secret = Derive-Secret(handshake_secret, "derived", "") */
	ret = quic_hkdf_expand_label_ex(ctx->ks.hash_alg,
					ctx->ks.handshake_secret, ctx->ks.hash_len,
					(const uint8_t *)TLS13_LABEL_DERIVED,
					strlen(TLS13_LABEL_DERIVED),
					empty_hash, empty_hash_len,
					derived, ctx->ks.hash_len);
	if (ret != 0) {
		return ret;
	}

	/* Master Secret = HKDF-Extract(derived_secret, 0) */
	ret = quic_hkdf_extract_ex(ctx->ks.hash_alg,
				   derived, ctx->ks.hash_len,
				   zero_ikm, ctx->ks.hash_len,
				   ctx->ks.master_secret, ctx->ks.hash_len);
	if (ret != 0) {
		return ret;
	}

	/* Derive client/server application traffic secrets
	 * Per RFC 8446 Section 7.1, application traffic secrets use the
	 * transcript hash up to and including Server Finished (not Client Finished).
	 * The server_finished_transcript was saved after sending Server Finished.
	 */
	if (ctx->ks.server_finished_transcript_len == 0) {
		NET_ERR("Server Finished transcript not saved");
		return -EINVAL;
	}

	ret = quic_derive_secret_ex(ctx, ctx->ks.master_secret,
				    TLS13_LABEL_C_AP_TRAFFIC,
				    ctx->ks.server_finished_transcript,
				    ctx->ks.server_finished_transcript_len,
				    ctx->ks.client_ap_traffic_secret);
	if (ret != 0) {
		return ret;
	}

	ret = quic_derive_secret_ex(ctx, ctx->ks.master_secret,
				    TLS13_LABEL_S_AP_TRAFFIC,
				    ctx->ks.server_finished_transcript,
				    ctx->ks.server_finished_transcript_len,
				    ctx->ks.server_ap_traffic_secret);
	if (ret != 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(ctx->ks.client_ap_traffic_secret, ctx->ks.hash_len,
				"Client app traffic secret:");
		NET_HEXDUMP_DBG(ctx->ks.server_ap_traffic_secret, ctx->ks.hash_len,
				"Server app traffic secret:");
	}

	/* Notify QUIC layer of new secrets */
	if (ctx->secret_cb != NULL) {
		if (ctx->ep->is_server) {
			ctx->secret_cb(ctx->user_data, QUIC_SECRET_LEVEL_APPLICATION,
				       ctx->ks.client_ap_traffic_secret,
				       ctx->ks.server_ap_traffic_secret,
				       ctx->ks.hash_len);
		} else {
			ctx->secret_cb(ctx->user_data, QUIC_SECRET_LEVEL_APPLICATION,
				       ctx->ks.server_ap_traffic_secret,
				       ctx->ks.client_ap_traffic_secret,
				       ctx->ks.hash_len);
		}
	}

	return 0;
}

static int quic_send_handshake_done(struct quic_endpoint *ep)
{
	uint8_t frame[1];

	/* HANDSHAKE_DONE frame is just the frame type, no payload */
	frame[0] = QUIC_FRAME_TYPE_HANDSHAKE_DONE;

	/* Send at Application level (1-RTT / short header) */
	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, 1);
}

/**
 * Send CONNECTION_CLOSE frame to peer
 *
 * Per RFC 9000, CONNECTION_CLOSE frame format:
 * - Frame Type (0x1c for transport error, 0x1d for application error)
 * - Error Code (varint)
 * - Frame Type (varint, only for 0x1c, the frame that triggered the error)
 * - Reason Phrase Length (varint)
 * - Reason Phrase (bytes)
 *
 * For TLS alerts, use error code 0x100 + TLS_alert_code (RFC 9001 Section 4.8)
 */
static int quic_endpoint_send_connection_close(struct quic_endpoint *ep,
					       uint64_t error_code,
					       const char *reason)
{
	uint8_t frame[128];
	size_t pos = 0;
	size_t reason_len = reason != NULL ? strlen(reason) : 0;
	enum quic_secret_level level;

	/* Use transport error CONNECTION_CLOSE (0x1c) */
	frame[pos++] = QUIC_FRAME_TYPE_CONNECTION_CLOSE_TRANSPORT;

	/* Error Code */
	pos += quic_put_varint(&frame[pos], sizeof(frame) - pos, error_code);

	/* Frame Type that triggered the error (0 = unknown/not applicable) */
	frame[pos++] = 0;

	/* Reason Phrase Length */
	pos += quic_put_varint(&frame[pos], sizeof(frame) - pos, reason_len);

	/* Reason Phrase */
	if (reason_len > 0 && pos + reason_len <= sizeof(frame)) {
		memcpy(&frame[pos], reason, reason_len);
		pos += reason_len;
	}

	NET_DBG("[EP:%p/%d] Sending CONNECTION_CLOSE: error=0x%" PRIx64 ", reason=%s",
		ep, quic_get_by_ep(ep), error_code, reason != NULL ? reason : "(none)");

	/* Send at the highest available encryption level */
	if (ep->crypto.application.initialized) {
		level = QUIC_SECRET_LEVEL_APPLICATION;
	} else if (ep->crypto.handshake.initialized) {
		level = QUIC_SECRET_LEVEL_HANDSHAKE;
	} else {
		level = QUIC_SECRET_LEVEL_INITIAL;
	}

	/* Cancel any PTO work to avoid unnecessary retransmissions after
	 * connection close.
	 */
	k_work_cancel_delayable(&ep->recovery.pto_work);

	return quic_send_packet(ep, level, frame, pos);
}

/* Parse peer's transport parameters and initialize flow control */
static int parse_peer_transport_params(struct quic_endpoint *ep)
{
	struct quic_tls_context *tls = &ep->crypto.tls;
	uint8_t params[256];
	size_t params_len = sizeof(params);
	size_t pos = 0;
	int ret;

	if (!tls->is_initialized) {
		return -EINVAL;
	}

	ret = quic_tls_get_peer_transport_params(tls, params, &params_len);
	if (ret != 0) {
		NET_DBG("[EP:%p/%d] No peer transport params available",
			ep, quic_get_by_ep(ep));
		return ret;
	}

	NET_DBG("[EP:%p/%d] Parsing peer transport params, len=%zu",
		ep, quic_get_by_ep(ep), params_len);

	/* Parse transport parameters (varint ID, varint len, value) */
	while (pos < params_len) {
		uint64_t param_id, param_len, value;
		int id_len, len_len, val_len;

		id_len = quic_get_len(&params[pos], params_len - pos, &param_id);
		if (id_len <= 0) {
			break;
		}
		pos += id_len;

		len_len = quic_get_len(&params[pos], params_len - pos, &param_len);
		if (len_len <= 0) {
			break;
		}
		pos += len_len;

		if (pos + param_len > params_len) {
			NET_ERR("[EP:%p/%d] Transport param extends beyond buffer",
				ep, quic_get_by_ep(ep));
			break;
		}

		/* Parse value as varint for most parameters */
		if (param_len > 0) {
			val_len = quic_get_len(&params[pos], param_len, &value);
			if (val_len <= 0) {
				value = 0;
			}
		} else {
			value = 0;
		}

		switch (param_id) {
		case QUIC_INITIAL_MAX_DATA:
			ep->peer_params.initial_max_data = value;
			ep->tx_fc.max_data = value;
			NET_DBG("  initial_max_data: %" PRIu64, value);
			break;
		case QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL:
			ep->peer_params.initial_max_stream_data_bidi_local = value;
			NET_DBG("  initial_max_stream_data_bidi_local: %" PRIu64, value);
			break;
		case QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE:
			ep->peer_params.initial_max_stream_data_bidi_remote = value;
			NET_DBG("  initial_max_stream_data_bidi_remote: %" PRIu64, value);
			break;
		case QUIC_INITIAL_MAX_STREAM_DATA_UNI:
			ep->peer_params.initial_max_stream_data_uni = value;
			NET_DBG("  initial_max_stream_data_uni: %" PRIu64, value);
			break;
		case QUIC_INITIAL_MAX_STREAMS_BIDI:
			ep->peer_params.initial_max_streams_bidi = value;
			NET_DBG("  initial_max_streams_bidi: %" PRIu64, value);
			break;
		case QUIC_INITIAL_MAX_STREAMS_UNI:
			ep->peer_params.initial_max_streams_uni = value;
			NET_DBG("  initial_max_streams_uni: %" PRIu64, value);
			break;
		case QUIC_MAX_IDLE_TIMEOUT:
			ep->peer_params.max_idle_timeout = value;
			NET_DBG("  max_idle_timeout: %" PRIu64 " ms", value);
			break;
		case QUIC_MAX_UDP_PAYLOAD_SIZE:
			if (value >= 1200) {
				/* Cap at our local estimated MTU limits (e.g. 1452)
				 * or 1200 if PMTUD not supported
				 */
				ep->max_tx_payload_size = 1200;
			}
			NET_DBG("  max_udp_payload_size: %" PRIu64 " (eff TX %u)",
				value, ep->max_tx_payload_size);
			break;
		default:
			NET_DBG("  param 0x%02" PRIx64 ": %" PRIu64 " (len=%" PRIu64 ")",
				param_id, value, param_len);
			break;
		}

		pos += param_len;
	}

	ep->peer_params.parsed = true;

	return 0;
}

/**
 * Called server-side when a child endpoint completes the handshake.
 * Creates a new quic_context for the connection, migrates the endpoint
 * into it, and enqueues it in the listening context's pending.accept_q.
 */
static void quic_connection_accept_enqueue(struct quic_endpoint *child_ep)
{
	struct quic_context *listen_ctx;
	struct quic_context *child_ctx;
	struct quic_stream *stream, *tmp;

	/* Only server child endpoints should be enqueued */
	if (!child_ep->is_server || child_ep->parent == NULL) {
		return;
	}

	/* Find the listening context via the parent (listening) endpoint */
	listen_ctx = quic_find_context(child_ep->parent);
	if (listen_ctx == NULL) {
		NET_ERR("[EP:%p/%d] Cannot find listening context for parent ep %d [%p]",
			child_ep, quic_get_by_ep(child_ep),
			quic_get_by_ep(child_ep->parent), child_ep->parent);
		return;
	}

	/* Allocate a fresh context that represents this single connection */
	child_ctx = quic_get();
	if (child_ctx == NULL) {
		NET_ERR("[EP:%p/%d] Cannot allocate context for accepted connection",
			child_ep, quic_get_by_ep(child_ep));
		return;
	}

	/* Migrate the child endpoint from the listening context to the new one */
	k_mutex_lock(&contexts_lock, K_FOREVER);
	k_mutex_lock(&endpoints_lock, K_FOREVER);

	sys_slist_find_and_remove(&listen_ctx->endpoints, &child_ep->node);
	sys_slist_prepend(&child_ctx->endpoints, &child_ep->node);

	/* Migrate any pending streams to the new context as well. This
	 * can happen if the connection accept queue has not been processed
	 * fully yet and the streams were allocated to wrong connection context.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&listen_ctx->streams, stream, tmp, node) {
		if (stream->conn != listen_ctx) {
			continue;
		}

		NET_DBG("[EP:%p/%d] Migrating stream %p/%d from CO:%p/%d to new CO:%p/%d",
			child_ep, quic_get_by_ep(child_ep),
			stream, quic_get_by_stream(stream),
			listen_ctx, quic_get_by_conn(listen_ctx),
			child_ctx, quic_get_by_conn(child_ctx));

		sys_slist_find_and_remove(&listen_ctx->streams, &stream->node);
		sys_slist_prepend(&child_ctx->streams, &stream->node);

		stream->conn = child_ctx;
		stream->ep = child_ep;

		/* Remove stream from listen_ctx pending accept queue if it's there,
		 * since it should now be delivered to the new child_ctx accept queue.
		 */
		do {
			struct quic_stream *st = k_fifo_get(&listen_ctx->incoming.stream_q,
							    K_NO_WAIT);

			if (st == stream) {
				/* Found the stream, don't put it back */
				break;
			} else if (st == NULL) {
				/* No more streams in the queue */
				break;
			}

			/* Not the stream we're looking for, put it back and keep looking */
			k_fifo_put(&listen_ctx->incoming.stream_q, st);
		} while (true);

		/* Set the stream's socket as it was assigned to wrong context before */
		zvfs_free_fd(stream->sock);
		(void)sock_obj_core_dealloc(stream->sock);

		stream->sock = zvfs_reserve_fd();
		if (stream->sock < 0) {
			NET_ERR("[EP:%p/%d] Failed to reserve fd for stream %p/%d: %d",
				child_ep, quic_get_by_ep(child_ep),
				stream, quic_get_by_stream(stream), stream->sock);
			quic_stream_unref(stream);
			break;
		}

		zvfs_finalize_typed_fd(stream->sock, stream,
				       (const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
				       ZVFS_MODE_IFSOCK);

		(void)sock_obj_core_alloc_find(child_ctx->sock, stream->sock,
					       NET_SOCK_STREAM);

		/* Queue for accept() call */
		k_fifo_put(&child_ctx->incoming.stream_q, stream);
		k_sem_give(&child_ctx->incoming.stream_sem);
	}

	k_mutex_unlock(&endpoints_lock);
	k_mutex_unlock(&contexts_lock);

	/* Inherit the listening endpoint reference so stream getsockopt works */
	child_ctx->listen = NULL;
	child_ctx->stream_id_counter = 0ULL;
	child_ctx->id = connection_ids++;
	child_ctx->is_listening = false;

	NET_DBG("[EP:%p/%d] Enqueueing accept CO:%p/%d to CO:%p/%d; parent EP:%p/%d",
		child_ep, quic_get_by_ep(child_ep),
		child_ctx, quic_get_by_conn(child_ctx),
		listen_ctx, quic_get_by_conn(listen_ctx),
		child_ep->parent, quic_get_by_ep(child_ep->parent));

	/* Make the child context reachable to accept() */
	k_fifo_put(&listen_ctx->pending.accept_q, child_ctx);
	k_sem_give(&listen_ctx->pending.accept_sem);

	k_yield();
}

static int quic_handshake_complete(struct quic_endpoint *ep)
{
	struct quic_tls_context *tls = &ep->crypto.tls;
	int ret;

	if (!tls->is_initialized) {
		return -EINVAL;
	}

	/* Parse peer's transport parameters for flow control */
	ret = parse_peer_transport_params(ep);
	if (ret != 0) {
		NET_WARN("Failed to parse peer transport params: %d", ret);
		/* Continue anyway, use defaults */
	}

	quic_endpoint_negotiate_idle_timeout(ep);

	/* Derive application traffic secrets */
	ret = derive_application_secrets(tls);
	if (ret != 0) {
		NET_ERR("Failed to derive application secrets");
		return ret;
	}

	/* Send HANDSHAKE_DONE frame (only server sends this) */
	if (ep->is_server) {
		ret = quic_send_handshake_done(ep);
		if (ret != 0) {
			NET_ERR("Failed to send HANDSHAKE_DONE");
			return ret;
		}
	}

	quic_recovery_discard_pn_space(ep, level_to_pn_space(QUIC_SECRET_LEVEL_INITIAL));
	quic_recovery_discard_pn_space(ep, level_to_pn_space(QUIC_SECRET_LEVEL_HANDSHAKE));

	quic_endpoint_handshake_complete(ep);

	NET_DBG("[EP:%p/%d] QUIC handshake complete", ep, quic_get_by_ep(ep));

	/* For server, this will enqueue the new connection context in the
	 * listening context's accept queue.
	 * For client, this is no-op.
	 */
	quic_connection_accept_enqueue(ep);

	return 0;
}

/* Helper to encode varint in-place */
static int quic_put_varint(uint8_t *buf, size_t buf_len, uint64_t val)
{
	int size = quic_get_varint_size(val);

	if (buf_len < size) {
		return -EINVAL;
	}

	if (quic_put_len(buf, buf_len, val) != 0) {
		return 0;
	}

	return size;
}

/*
 * Find an existing stream by ID
 */
static struct quic_context *quic_find_context(struct quic_endpoint *ep)
{
	struct quic_context *ctx = NULL;

	/* Look up stream in connection's stream list */
	k_mutex_lock(&contexts_lock, K_FOREVER);

	/* Find the connection context for this endpoint */
	ARRAY_FOR_EACH(contexts, i) {
		struct quic_endpoint *endp, *tmp;

		if (!quic_context_is_used(&contexts[i])) {
			continue;
		}

		k_mutex_lock(&endpoints_lock, K_FOREVER);

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&contexts[i].endpoints,
						  endp, tmp, node) {
			if (endp == ep) {
				ctx = &contexts[i];
				break;
			}
		}

		k_mutex_unlock(&endpoints_lock);

		if (ctx != NULL) {
			break;
		}
	}

	k_mutex_unlock(&contexts_lock);

	return ctx;
}

void quic_context_foreach(quic_context_cb_t cb, void *user_data)
{
	k_mutex_lock(&contexts_lock, K_FOREVER);

	/* Find the connection context for this endpoint */
	ARRAY_FOR_EACH(contexts, i) {
		struct quic_context *ctx = &contexts[i];

		if (!quic_context_is_used(ctx)) {
			continue;
		}

		k_mutex_unlock(&contexts_lock);

		cb(ctx, user_data);

		k_mutex_lock(&contexts_lock, K_FOREVER);
	}

	k_mutex_unlock(&contexts_lock);
}

void quic_context_endpoint_foreach(struct quic_context *ctx,
				   quic_endpoint_cb_t cb, void *user_data)
{
	struct quic_endpoint *endp, *tmp;

	k_mutex_lock(&contexts_lock, K_FOREVER);
	k_mutex_lock(&endpoints_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->endpoints, endp, tmp, node) {
		if (!quic_endpoint_is_used(endp)) {
			continue;
		}

		cb(endp, user_data);
	}

	k_mutex_unlock(&endpoints_lock);
	k_mutex_unlock(&contexts_lock);
}

void quic_context_stream_foreach(struct quic_context *ctx,
				 quic_stream_cb_t cb, void *user_data)
{
	struct quic_stream *stream, *tmp;

	k_mutex_lock(&contexts_lock, K_FOREVER);
	k_mutex_lock(&streams_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->streams, stream, tmp, node) {
		if (!quic_stream_is_used(stream)) {
			continue;
		}

		cb(stream, user_data);
	}

	k_mutex_unlock(&streams_lock);
	k_mutex_unlock(&contexts_lock);
}

void quic_endpoint_foreach(quic_endpoint_cb_t cb, void *user_data)
{
	k_mutex_lock(&endpoints_lock, K_FOREVER);

	ARRAY_FOR_EACH(endpoints, i) {
		if (endpoints[i] == NULL || atomic_get(&endpoints[i]->refcount) == 0) {
			continue;
		}

		cb(endpoints[i], user_data);
	}

	k_mutex_unlock(&endpoints_lock);
}

void quic_stream_foreach(quic_stream_cb_t cb, void *user_data)
{
	k_mutex_lock(&streams_lock, K_FOREVER);

	ARRAY_FOR_EACH(streams, i) {
		if (!quic_stream_is_used(&streams[i])) {
			continue;
		}

		cb(&streams[i], user_data);
	}

	k_mutex_unlock(&streams_lock);
}

/*
 * Deliver received data to a stream
 */
static int quic_stream_receive_data(struct quic_stream *stream,
				    uint64_t offset,
				    const uint8_t *data,
				    size_t len,
				    bool is_fin)
{
	struct quic_stream_rx_buffer *buf = &stream->rx_buf;
	struct quic_endpoint *ep = stream->ep;
	bool progress = true;
	size_t available;
	int ret = 0;

	/*
	 * If the application shut down the read half (SHUT_RD), discard
	 * incoming data silently. We have already sent STOP_SENDING so the
	 * peer should stop soon; any data in flight before that arrives is
	 * simply dropped here.
	 */
	if (stream->read_closed) {
		NET_DBG("[ST:%p/%d] stream %" PRIu64 ": discarding %zu bytes "
			"(read half closed)", stream, quic_get_by_stream(stream),
			stream->id, len);
		return 0;
	}

	k_mutex_lock(&stream->cond.data_available, K_FOREVER);

	/* For now, just log and store if this is in-order data */
	if (offset != buf->read_offset + (buf->tail - buf->head)) {
		uint64_t expected = buf->read_offset + (buf->tail - buf->head);

		if (offset < expected) {
			/* Duplicate, already have this data, silently ignore */
			NET_DBG("[ST:%p/%d] Duplicate data at offset %" PRIu64 ", ignoring",
				stream, quic_get_by_stream(stream), offset);
			goto unlock;
		}

		/* offset > expected: out-of-order, stash it if we have room */
		if (buf->ooo_count < ARRAY_SIZE(buf->ooo) &&
		    len <= sizeof(buf->ooo[0].data)) {
			/* Check not already stored */
			bool already_stored = false;

			for (int i = 0; i < buf->ooo_count; i++) {
				if (buf->ooo[i].offset == offset) {
					already_stored = true;
					break;
				}
			}

			if (!already_stored) {
				struct quic_ooo_segment *seg = &buf->ooo[buf->ooo_count++];

				seg->offset = offset;
				seg->len = (uint16_t)len;
				memcpy(seg->data, data, len);

				NET_DBG("[ST:%p/%d] Stored OOO segment: offset=%" PRIu64
					" len=%zu slots_used=%u",
					stream, quic_get_by_stream(stream), offset, len,
					buf->ooo_count);
			}
		} else {
			NET_DBG("[ST:%p/%d] OOO queue full, dropping offset=%" PRIu64,
				stream, quic_get_by_stream(stream), offset);
		}

		ret = -EAGAIN;
		goto unlock;
	}

	NET_DBG("[ST:%p/%d] Stream %" PRIu64 ": received %zu bytes at offset %" PRIu64 "%s",
		stream, quic_get_by_stream(stream), stream->id, len, offset,
		is_fin ? " (FIN)" : "");

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(data, MIN(len, 64), "Stream data (first 64 bytes):");
	}

	/* Check buffer space */
	available = buf->size - buf->tail;
	if (len > available) {
		/* Attempt to compact the buffer to reclaim space from already-read data */
		if (buf->head > 0) {
			size_t unread = buf->tail - buf->head;

			memmove(buf->data, &buf->data[buf->head], unread);
			buf->tail = unread;
			buf->head = 0;

			/* Recalculate available space */
			available = buf->size - buf->tail;
		}

		/* If it's still full, the peer exceeded our flow control
		 * window which is a protocol violation (RFC 9000, Section 4.1).
		 * We can close the connection now.
		 */
		if (len > available) {
			NET_ERR("[ST:%p/%d] Stream buffer overflow: need %zu, "
				"have %zu (flow control violated)",
				stream, quic_get_by_stream(stream),
				len, available);

			k_mutex_unlock(&stream->cond.data_available);

			if (ep != NULL) {
				quic_endpoint_send_connection_close(
					ep,
					QUIC_ERROR_FLOW_CONTROL_ERROR,
					"stream data exceeds rx buffer");
			}

			return -EPROTO;
		}
	}

	/* Copy data to buffer */
	memcpy(&buf->data[buf->tail], data, len);
	buf->tail += len;

	/* Update flow control tracking */
	stream->bytes_received += len;
	if (ep != NULL) {
		ep->rx_fc.bytes_received += len;
	}

	/* Flow control window updates are sent when the application consumes
	 * data via recv(). This allows proper backpressure, if the app can't
	 * keep up, we don't tell the peer to send more data.
	 */

	/* Replay: keep delivering OOO segments that are now in order */
	while (progress && buf->ooo_count > 0) {
		uint64_t next;
		size_t avail;
		size_t copy;

		progress = false;
		next = buf->read_offset + (buf->tail - buf->head);

		for (int i = 0; i < buf->ooo_count; i++) {
			struct quic_ooo_segment *seg = &buf->ooo[i];

			if (seg->offset != next) {
				continue;
			}

			/* This segment is now in order, so deliver it */
			avail = buf->size - buf->tail;
			copy  = MIN((size_t)seg->len, avail);

			memcpy(&buf->data[buf->tail], seg->data, copy);
			buf->tail += copy;

			/* Remove from queue by replacing with last entry */
			buf->ooo[i] = buf->ooo[buf->ooo_count - 1];
			buf->ooo_count--;

			progress = true;
			break;
		}
	}

	if (is_fin) {
		buf->fin_received = true;

		/* Mark stream as having received all data */
		smf_set_state(SMF_CTX(&stream->state.ctx),
			      &quic_stream_bidirectional_states[STATE_SIZE_KNOWN]);
	}

	/* Signal waiting readers */
	k_poll_signal_raise(&stream->recv.signal, 0);
	k_condvar_signal(&stream->cond.recv);

unlock:
	k_mutex_unlock(&stream->cond.data_available);

	return ret;
}

/*
 * Find an existing stream by ID
 */
static struct quic_stream *quic_find_stream(struct quic_context *ctx,
					    struct quic_endpoint *ep,
					    uint64_t stream_id)
{
	struct quic_stream *stream, *tmp, *found = NULL;

	/* Look up stream in connection's stream list */
	k_mutex_lock(&contexts_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->streams, stream, tmp, node) {
		/* Stream found, verify it belongs to this connection */
		if (stream->id == stream_id && stream->conn == ctx) {
			found = stream;
			break;
		}
	}

	k_mutex_unlock(&contexts_lock);

	return found;
}

/* Find stream by ID using only endpoint (searches all contexts) */
static struct quic_stream *quic_find_stream_by_id(struct quic_endpoint *ep,
						  uint64_t stream_id)
{
	struct quic_stream *stream, *tmp, *found = NULL;
	int i;

	k_mutex_lock(&contexts_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		struct quic_context *ctx = &contexts[i];

		if (!quic_context_is_used(ctx)) {
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->streams, stream, tmp, node) {
			if (stream->id == stream_id && stream->ep == ep) {
				found = stream;
				goto out;
			}
		}
	}

out:
	k_mutex_unlock(&contexts_lock);
	return found;
}

/*
 * Create a new stream initiated by the peer
 */
static struct quic_stream *quic_create_stream_from_peer(struct quic_context *ctx,
							struct quic_endpoint *ep,
							uint64_t stream_id)
{
	struct quic_stream *stream;
	bool is_bidi;

	/* Allocate a new stream */
	stream = quic_get_stream(ctx);
	if (stream == NULL) {
		NET_DBG("[CO:%p/%d] No available stream slots", ctx, quic_get_by_conn(ctx));
		quic_stats_update_stream_open_failed();
		return NULL;
	}

	stream->id = stream_id;
	stream->ep = ep;

	/* Determine stream type from ID (RFC 9000 Section 2.1):
	 * Bit 0: 0 = client-initiated, 1 = server-initiated
	 * Bit 1: 0 = bidirectional, 1 = unidirectional
	 */
	stream->type = stream_id & 0x03;
	is_bidi = (stream->type & 0x02) == 0;

	/* Initialize stream-level flow control from peer transport params */
	if (ep->peer_params.parsed) {
		if (is_bidi) {
			/* Peer-initiated bidi stream: use bidi_local for our TX limit */
			stream->remote_max_data =
				ep->peer_params.initial_max_stream_data_bidi_local;
		} else {
			/* Peer-initiated uni stream: peer sends, we receive */
			stream->remote_max_data = 0;
		}
	} else {
		/* Default if params not yet parsed */
		stream->remote_max_data = CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE;
	}

	/* Set local limits for receiving data on this stream */
	stream->local_max_data = MIN(CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
				     sizeof(stream->rx_buf.data));
	stream->bytes_received = 0;
	stream->local_max_data_sent = stream->local_max_data;

	stream->tx_buf.base_offset = 0;
	stream->tx_buf.len = 0;

	quic_stats_update_stream_opened();

	NET_DBG("[ST:%p/%d] Created stream %" PRIu64 " (type=%d) from peer, remote_max=%" PRIu64,
		stream, quic_get_by_stream(stream), stream_id, stream->type,
		stream->remote_max_data);

	smf_set_initial(SMF_CTX(&stream->state.ctx),
			&quic_stream_bidirectional_states[STATE_RECV]);

	return stream;
}

static void process_pkt(struct quic_pkt *pkt)
{
	int ret = 0;

	switch (pkt->ptype) {
	case QUIC_PACKET_TYPE_INITIAL:
		ret = handle_initial_packet(pkt->ep,
					    pkt->data,
					    pkt->len,
					    pkt->total_len);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] %s packet handling failure (%d)",
				pkt->ep, quic_get_by_ep(pkt->ep), "Initial", ret);
		} else if (ret == 1) {
			NET_DBG("[EP:%p/%d] Connection closing after %s packet",
				pkt->ep, quic_get_by_ep(pkt->ep), "Initial");
			quic_endpoint_unref(pkt->ep);
		}

		break;

	case QUIC_PACKET_TYPE_HANDSHAKE:
		ret = handle_handshake_packet(pkt->ep,
					      pkt->data,
					      pkt->len,
					      pkt->total_len);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] %s packet handling failure (%d)",
				pkt->ep, quic_get_by_ep(pkt->ep), "Handshake", ret);

			if (ret == -EACCES) {
				(void)quic_endpoint_send_connection_close(pkt->ep,
					QUIC_ERROR_CRYPTO_BASE +
						QUIC_CRYPTO_ERROR_CERTIFICATE_REQUIRED,
					"Certificate required");
			}

		} else if (ret == 1) {
			NET_DBG("[EP:%p/%d] Connection closing after %s packet",
				pkt->ep, quic_get_by_ep(pkt->ep), "Handshake");
			quic_endpoint_unref(pkt->ep);
		}

		break;

	case QUIC_PACKET_TYPE_0RTT:
		/* TODO: Handle 0-RTT */
		break;

	case QUIC_PACKET_TYPE_1RTT:
		/* Short header / 1-RTT packet */
		ret = handle_1rtt_packet(pkt);
		if (ret == -EACCES) {
			NET_DBG("[EP:%p/%d] %s packet handling failure (%d)",
				pkt->ep, quic_get_by_ep(pkt->ep), "1-RTT", ret);

			(void)quic_endpoint_send_connection_close(pkt->ep,
					QUIC_ERROR_CRYPTO_BASE +
						QUIC_CRYPTO_ERROR_CERTIFICATE_REQUIRED,
					"Certificate required");
		} else {
			ret = 0;
		}
		break;

	default:
		NET_WARN("[EP:%p/%d] Unknown packet type: %d", pkt->ep,
			 quic_get_by_ep(pkt->ep), pkt->ptype);
		break;
	}

	if (ret < 0) {
		quic_endpoint_unref(pkt->ep);
	}
}

/* This is called by Quic handler thread and it will decrypt the header and process
 * the packet further.
 */
static bool process_long_header_msg(struct quic_pkt *pkt)
{
	enum quic_packet_type ptype = pkt->ptype;
	struct quic_endpoint *ep = pkt->ep;
	int pos = 0;
	uint8_t *buf = &pkt->data[pos];
	struct quic_decrypted_packet decrypted;
	int ret;

	/* Plaintext buffer for decrypted frames */
	uint8_t plaintext[CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN];

	NET_DBG("[EP:%p/%d] %s long header packet, len %zu bytes",
		ep, quic_get_by_ep(ep),
		ptype == QUIC_PACKET_TYPE_INITIAL ? "Initial" :
		ptype == QUIC_PACKET_TYPE_HANDSHAKE ? "Handshake" :
		ptype == QUIC_PACKET_TYPE_0RTT ? "0-RTT" : "Unknown",
		pkt->total_len);

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(buf, pkt->total_len, "Msg:");
	}

	if (ptype == QUIC_PACKET_TYPE_INITIAL) {
		if (pkt->token > 0) {
			NET_DBG("[EP:%p/%d] Initial packet with token: %" PRIu64,
				ep, quic_get_by_ep(ep), pkt->token);
		}

		/* TODO: store the token somewhere */
	}

	if (ptype == QUIC_PACKET_TYPE_INITIAL) {
		if (ep->crypto.initial.rx.hp.key_id == 0) {
			if (!quic_conn_init_setup(ep,
						  ep->peer_orig_dcid,
						  ep->peer_orig_dcid_len)) {
				NET_DBG("[EP:%p/%d] Cannot setup initial connection ID",
					ep, quic_get_by_ep(ep));
				goto fail;
			}

			NET_DBG("[EP:%p/%d] Init connection setup done", ep,
				quic_get_by_ep(ep));
		}
	}

	NET_DBG("[EP:%p/%d] Payload length: %zu, pn_offset: %zu", ep,
		quic_get_by_ep(ep), pkt->len, pkt->pn_offset);

	/*
	 * The actual packet we need to decrypt:
	 * - Starts at buf[0]
	 * - Has total length of: pn_offset + payload_length
	 *
	 * NOT the entire 1200-byte UDP payload (which may have padding)
	 */

	if (pkt->total_len > pkt->len + pkt->pn_offset) {
		NET_DBG("[EP:%p/%d] Actual packet length %zu exceeds calculated length %zu",
			ep, quic_get_by_ep(ep), pkt->total_len, pkt->len + pkt->pn_offset);
		goto fail;
	}

	/*
	 * Decrypt the complete packet:
	 * 1. Remove header protection
	 * 2. Reconstruct packet number
	 * 3. Decrypt payload with AEAD
	 */
	ret = quic_decrypt_packet(ep,
				  buf,
				  pkt->total_len,
				  pkt->pn_offset,
				  ptype,
				  plaintext, sizeof(plaintext),
				  &decrypted);
	if (ret != 0) {
		NET_DBG("[EP:%p/%d] Packet decryption failed (%d)", ep,
			quic_get_by_ep(ep), ret);
		goto free_ep;
	}

	pkt->pkt_num = decrypted.packet_number;

	NET_DBG("[EP:%p/%d] Decrypted packet %" PRIu64 ", payload %zu bytes", ep,
		quic_get_by_ep(ep), decrypted.packet_number, decrypted.payload_len);

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		NET_HEXDUMP_DBG(decrypted.payload, decrypted.payload_len,
				"Decrypted payload:");
	}

	if (!ep->crypto.initial.initialized) {
		if (!quic_conn_init_setup(ep, ep->peer_orig_dcid,
					  ep->peer_orig_dcid_len)) {
			NET_ERR("[EP:%p/%d] Failed to setup Initial crypto for new endpoint",
				ep, quic_get_by_ep(ep));
			goto free_ep;
		}

		NET_DBG("[EP:%p/%d] Initialized Initial crypto for new endpoint", ep,
			quic_get_by_ep(ep));
	}

	/* Clone the TLS context from the old endpoint in order to get things set by
	 * setsockopt() to that existing TLS context.
	 */
	if (!ep->crypto.tls.is_initialized) {
		int clone_ret = quic_tls_clone(&ep->crypto.tls, &pkt->old_ep->crypto.tls);

		if (clone_ret != 0) {
			NET_ERR("[EP:%p/%d] Failed to clone TLS context for Initial", ep,
				quic_get_by_ep(ep));
			goto free_ep;
		}

		ep->crypto.tls.ep = ep;

		quic_tls_set_callbacks(&ep->crypto.tls,
				       quic_tls_secret_callback,
				       quic_tls_send_callback,
				       ep);

		NET_DBG("[EP:%p/%d] Created TLS context for Initial processing", ep,
			quic_get_by_ep(ep));
	}

	/*
	 * Store decrypted payload in endpoint's pending buffer and overwrite
	 * the original encrypted data.
	 */
	memcpy(&pkt->data[0], decrypted.payload, decrypted.payload_len);
	pkt->len = decrypted.payload_len;

	return true;

free_ep:
	quic_endpoint_unref(ep);
	return false;

fail:
	return false;
}

static bool process_short_header_msg(struct quic_pkt *pkt)
{
	struct quic_endpoint *ep = pkt->ep;
	struct quic_crypto_context *crypto_ctx;
	struct quic_decrypted_packet decrypted;
	uint8_t plaintext[CONFIG_QUIC_ENDPOINT_PENDING_DATA_LEN];
	int ret;

	NET_DBG("[EP:%p/%d] Short header packet, len %zu bytes", ep,
		quic_get_by_ep(ep), pkt->len);

	/* Get crypto context for APPLICATION level (1-RTT) */
	crypto_ctx = quic_get_crypto_context_by_level(ep, QUIC_SECRET_LEVEL_APPLICATION);
	if (crypto_ctx == NULL || !crypto_ctx->initialized) {
		NET_DBG("[%p] Application crypto context still not ready for endpoint %d",
			ep, quic_get_by_ep(ep));
		return false;
	}

	/* Decrypt the packet */
	ret = quic_decrypt_packet(ep, pkt->data, pkt->len, pkt->pn_offset,
				  QUIC_PACKET_TYPE_1RTT,
				  plaintext, sizeof(plaintext),
				  &decrypted);
	if (ret != 0) {
		NET_ERR("Failed to decrypt short header packet: %d", ret);
		return false;
	}

	NET_DBG("[EP:%p/%d] Decrypted short header packet %" PRIu64 ", payload %zu bytes",
		ep, quic_get_by_ep(ep), decrypted.packet_number, decrypted.payload_len);

	/* Update packet with decrypted data */
	memcpy(&pkt->data[0], decrypted.payload, decrypted.payload_len);
	pkt->len = decrypted.payload_len;
	pkt->pkt_num = decrypted.packet_number;

	return true;
}

static void process_one_msg(struct quic_pkt *pkt)
{
	bool ret;

	if (pkt->htype == QUIC_HEADER_TYPE_LONG) {
		ret = process_long_header_msg(pkt);
	} else {
		/* Short header packet, process directly */
		ret = process_short_header_msg(pkt);
	}

	if (ret) {
		process_pkt(pkt);
	}
}

static void quic_service_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct quic_pkt *pkt;

		pkt = k_fifo_get(&quic_queue, K_FOREVER);

		NET_DBG("Processing packet %p", pkt);

		quic_endpoint_unref(pkt->ep);

		if (!quic_endpoint_is_used(pkt->ep) || pkt->ep->sock < 0) {
			NET_DBG("Endpoint %p released while in queue",
				pkt->ep);
		} else {
			process_one_msg(pkt);
		}

		quic_pkt_unref(pkt);
	}
}

/* Both the long and short header processing functions make rudimentary
 * checks and then store the remaining packet data in the endpoint's
 * pending buffer for further processing done by QUIC handler thread.
 */
static int process_long_header(struct quic_endpoint *ep,
			       struct net_sockaddr *addr,
			       net_socklen_t addrlen,
			       uint8_t *buf,
			       size_t payload_len,
			       uint64_t token,
			       size_t total_len,
			       size_t pn_offset,
			       size_t datagram_len)
{
	uint8_t dst_conn_id[MAX_CONN_ID_LEN];
	uint8_t src_conn_id[MAX_CONN_ID_LEN];
	uint8_t my_cid[MAX_CONN_ID_LEN];
	struct quic_endpoint *existing_ep;
	struct quic_pkt *pkt;
	enum quic_packet_type ptype;
	uint32_t version;
	int dst_conn_id_len;
	int src_conn_id_len;
	int my_cid_len = 0;
	int pos = 0;

	ptype = (buf[pos++] >> 4) & 0x03;

	NET_DBG("[EP:%p/%d] Packet type: %s, first byte: 0x%02x",
		ep, quic_get_by_ep(ep),
		ptype == QUIC_PACKET_TYPE_INITIAL ? "initial" :
		ptype == QUIC_PACKET_TYPE_0RTT ? "0-RTT" :
		ptype == QUIC_PACKET_TYPE_HANDSHAKE ? "handshake" :
		ptype == QUIC_PACKET_TYPE_RETRY ? "retry" : "unknown",
		buf[0]);

	version = sys_get_be32(&buf[pos]);
	if (version != QUIC_VERSION_1) {
		NET_DBG("[EP:%p/%d] Unsupported QUIC version: 0x%08x",
			ep, quic_get_by_ep(ep), version);
		return -EINVAL;
	}

	pos += sizeof(uint32_t);
	if ((size_t)pos >= total_len) {
		NET_DBG("[EP:%p/%d] Packet too short for connection IDs",
			ep, quic_get_by_ep(ep));
		return -EINVAL;
	}

	/* Destination id len */
	dst_conn_id_len = buf[pos++];
	if (dst_conn_id_len > MAX_CONN_ID_LEN) {
		NET_DBG("[EP:%p/%d] Invalid QUIC connection ID len %d",
			ep, quic_get_by_ep(ep), dst_conn_id_len);
		return -EINVAL;
	}

	memcpy(dst_conn_id, &buf[pos], dst_conn_id_len);
	pos += dst_conn_id_len;
	if ((size_t)pos >= total_len) {
		NET_DBG("[EP:%p/%d] Packet too short for %s connection ID",
			ep, quic_get_by_ep(ep), "destination");
		return -EINVAL;
	}

	/* Src id len */
	src_conn_id_len = buf[pos++];
	if (src_conn_id_len > MAX_CONN_ID_LEN) {
		NET_DBG("[EP:%p/%d] Invalid QUIC connection ID len %d",
			ep, quic_get_by_ep(ep), src_conn_id_len);
		return -EINVAL;
	}

	memcpy(src_conn_id, &buf[pos], src_conn_id_len);
	pos += src_conn_id_len;
	if ((size_t)pos >= total_len) {
		NET_DBG("[EP:%p/%d] Packet too short for %s connection ID",
			ep, quic_get_by_ep(ep), "source");
		return -EINVAL;
	}

	NET_HEXDUMP_DBG(dst_conn_id, dst_conn_id_len, "Destination Conn ID:");
	NET_HEXDUMP_DBG(src_conn_id, src_conn_id_len, "Source Conn ID:");

	/* All the crypto stuff is done in a separate thread.
	 * We next find the correct endpoint and pass the data to it.
	 */
	existing_ep = quic_endpoint_lookup(addr, net_sad(&ep->local_addr),
					   src_conn_id, src_conn_id_len,
					   dst_conn_id, dst_conn_id_len);
	if (existing_ep == NULL) {
		struct quic_endpoint *new_ep;

		/* Make sure that the initial packet's UDP datagram is at least
		 * 1200 bytes long (RFC 9000 Section 14.1) to prevent
		 * amplification attacks.
		 */
		if (ptype == QUIC_PACKET_TYPE_INITIAL && datagram_len < 1200) {
			NET_DBG("[EP:%p/%d] Initial datagram too short: %zu bytes",
				ep, quic_get_by_ep(ep), datagram_len);
			return -EINVAL;
		}

		/*
		 * Server generates its own CID for the client to use.
		 * This is different from the DCID the client used in Initial.
		 */
		my_cid_len = MAX_MY_CONN_ID_LEN;
		sys_rand_get(my_cid, my_cid_len);

		NET_HEXDUMP_DBG(my_cid, my_cid_len, "Recipient generated CID:");

		new_ep = quic_endpoint_create(ep, addr, addrlen,
					      src_conn_id, src_conn_id_len,
					      my_cid, my_cid_len);
		if (new_ep == NULL) {
			NET_DBG("[EP:%p/%d] Cannot create new endpoint",
				ep, quic_get_by_ep(ep));
			return -ENOMEM;
		}

		/* Save the original DCID as it is used during handshake process */
		if (new_ep->peer_orig_dcid_len == 0) {
			new_ep->peer_orig_dcid_len = dst_conn_id_len;
			memcpy(new_ep->peer_orig_dcid, dst_conn_id, dst_conn_id_len);
		}

		NET_DBG("[EP:%p/%d] New endpoint", new_ep, quic_get_by_ep(new_ep));

		NET_HEXDUMP_DBG(new_ep->peer_orig_dcid,
				new_ep->peer_orig_dcid_len,
				"Original DCID:");

		existing_ep = new_ep;

		new_ep->parent = ep;
		new_ep->is_server = true;

		/* Take a ref to parent endpoint to ensure it stays alive as long as this
		 * child endpoint exists.
		 */
		quic_endpoint_ref(ep);

		/* TODO: Recheck that the idle timeout checker is properly
		 * running in all cases (where is it restarted after timeout?).
		 */
		quic_start_idle_timeout_checker();
	} else {
		NET_DBG("[EP:%p/%d] Existing endpoint", existing_ep,
			quic_get_by_ep(existing_ep));

		/* For client endpoints receiving server's response, update peer_cid
		 * to the server's SCID from the packet. This happens on the first
		 * response when the client learns the server's actual connection ID.
		 */
		if (!existing_ep->is_server && src_conn_id_len > 0) {
			if (existing_ep->peer_cid_len == 0 ||
			    memcmp(existing_ep->peer_cid, src_conn_id, src_conn_id_len) != 0) {
				NET_DBG("[EP:%p/%d] Updating peer_cid to server's SCID",
					existing_ep, quic_get_by_ep(existing_ep));
				existing_ep->peer_cid_len = src_conn_id_len;
				memcpy(existing_ep->peer_cid, src_conn_id, src_conn_id_len);
			}
		}
	}

	if (total_len > sizeof(((struct quic_pkt *)0)->data)) {
		NET_DBG("Packet too large: %zu > %zu", total_len,
			sizeof(((struct quic_pkt *)0)->data));
		return -ENOBUFS;
	}

	/* Update idle timer on ANY received packet */
	if (!existing_ep->idle.idle_timeout_disabled) {
		existing_ep->idle.last_rx_time = k_uptime_get();
	}

	/*
	 * Queue packet for processing
	 */
	pkt = quic_pkt_alloc(&quic_pkts, K_MSEC(CONFIG_QUIC_PKT_ALLOC_TIMEOUT));
	if (pkt == NULL) {
		NET_DBG("Cannot allocate QUIC packet");
		return -ENOMEM;
	}

	pkt->ep = existing_ep;
	pkt->old_ep = ep;
	pkt->ptype = ptype;
	pkt->htype = QUIC_HEADER_TYPE_LONG;
	pkt->token = token;
	pkt->pn_offset = pn_offset;
	pkt->pos = 0;
	pkt->len = payload_len;
	pkt->total_len = total_len;
	memcpy(pkt->data, buf, total_len);

	quic_endpoint_ref(pkt->ep);

	/* Quic thread will process the packet further like decrypt it etc */

	NET_DBG("Queueing packet %p", pkt);

	k_fifo_put(&quic_queue, pkt);

	/* Let also the QUIC service thread run to process the packets */
	k_yield();

	return 0;
}

static int process_short_header(struct quic_endpoint *ep,
				struct net_sockaddr *addr,
				net_socklen_t addrlen,
				uint8_t *buf,
				size_t len,
				bool force_queue)
{
	/*
	 * Short header format (RFC 9000 Section 17.3):
	 * - First byte (1): Form(0) + Fixed(1) + Spin + Reserved + Key Phase + PN Length
	 * - DCID (known length from connection state)
	 * - Packet Number (1-4, encoded length in first byte)
	 * - [Encrypted Payload]
	 */
	struct quic_endpoint *target_ep;
	struct quic_crypto_context *crypto_ctx;
	struct quic_decrypted_packet decrypted;
	uint8_t first_byte;
	uint8_t *dcid;
	size_t dcid_len;
	size_t pn_offset;
	uint8_t plaintext[1500];
	bool ack_only = false;
	int ret;

	ARG_UNUSED(addrlen);

	if (len < 1) {
		return -EINVAL;
	}

	first_byte = buf[0];

	/* Verify it's a short header (bit 7 = 0) with fixed bit set (bit 6 = 1) */
	if ((first_byte & 0x80) != 0) {
		NET_ERR("Not a short header packet");
		return -EINVAL;
	}
	if ((first_byte & 0x40) == 0) {
		NET_ERR("Fixed bit not set in short header");
		return -EINVAL;
	}

	/*
	 * For short headers, DCID is our connection ID. We need to find the
	 * endpoint that has this as its my_cid. Use the parent endpoint's CID
	 * length as a hint, then search for the actual endpoint.
	 */
	dcid_len = ep->my_cid_len;
	if (dcid_len == 0) {
		/* Try a common default CID length */
		dcid_len = 8;
	}

	if (len < 1 + dcid_len + 1) {
		NET_ERR("Short header packet too small for CID");
		return -EINVAL;
	}

	dcid = &buf[1];

	/* Look up the endpoint by DCID (which is our my_cid) */
	target_ep = quic_endpoint_lookup(addr, NULL, NULL, 0, dcid, dcid_len);
	if (target_ep == NULL || target_ep->sock == -1) {
		NET_DBG("No endpoint found for short header DCID");
		return 0; /* Silently ignore, might be for unknown connection */
	}

	/* Update idle timer on ANY received packet */
	if (!ep->idle.idle_timeout_disabled) {
		ep->idle.last_rx_time = k_uptime_get();
	}

	pn_offset = 1 + target_ep->my_cid_len;

	/* Get crypto context for APPLICATION level (1-RTT) */
	crypto_ctx = quic_get_crypto_context_by_level(target_ep, QUIC_SECRET_LEVEL_APPLICATION);
	if (force_queue || crypto_ctx == NULL || !crypto_ctx->initialized) {
		struct quic_pkt *pkt;

		NET_DBG("[EP:%p/%d] Queueing short header packet for endpoint (%s)",
			target_ep, quic_get_by_ep(target_ep),
			force_queue ? "coalesced with long header" :
			"crypto context not ready");

		/* Queue packet for processing by quic_service thread.
		 * This is required when the packet arrives coalesced with
		 * a long header packet in the same datagram, because the
		 * long header needs to be processed first (e.g., to complete
		 * the handshake and derive application keys).
		 */
		pkt = quic_pkt_alloc(&quic_pkts, K_MSEC(CONFIG_QUIC_PKT_ALLOC_TIMEOUT));
		if (pkt == NULL) {
			NET_DBG("Cannot allocate QUIC packet for short header");
			quic_endpoint_unref(target_ep);
			return -ENOMEM;
		}

		pkt->ep = target_ep;
		pkt->old_ep = ep;
		pkt->ptype = QUIC_PACKET_TYPE_1RTT;
		pkt->htype = QUIC_HEADER_TYPE_SHORT;
		pkt->token = 0;
		pkt->pn_offset = pn_offset;
		pkt->pos = 0;
		pkt->len = len;
		pkt->total_len = len;
		memcpy(pkt->data, buf, len);

		quic_endpoint_ref(pkt->ep);

		k_fifo_put(&quic_queue, pkt);
		k_yield();

		/* We don't unref, the packet struct holds the ref now */
		return 0;
	}

	/* Decrypt the packet, use value > 3 to get application (1-RTT) crypto context */
	ret = quic_decrypt_packet(target_ep, buf, len, pn_offset,
				  QUIC_PACKET_TYPE_1RTT,
				  plaintext, sizeof(plaintext),
				  &decrypted);
	if (ret != 0) {
		NET_ERR("[EP:%p/%d] Failed to decrypt short header packet: %d",
			target_ep, quic_get_by_ep(target_ep), ret);
		quic_endpoint_unref(target_ep);
		return ret;
	}

	NET_DBG("[EP:%p/%d] Short header packet %" PRIu64 ", payload %zu bytes",
		target_ep, quic_get_by_ep(target_ep), decrypted.packet_number,
		decrypted.payload_len);

	/* Keep endpoint alive for the duration of crypto API call */
	quic_endpoint_ref(target_ep);

	/* Process frames at APPLICATION level */
	ret = handle_crypto_level_packet(target_ep, QUIC_SECRET_LEVEL_APPLICATION,
					 decrypted.payload, decrypted.payload_len, len,
					 &ack_only);
	/* EAGAIN indicates out-of-order data. Packet was valid, fall through to ACK */
	if (ret < 0 && ret != -EAGAIN) {
		NET_DBG("Short header packet handling failure (%d)", ret);
		quic_endpoint_unref(target_ep);
		goto out;
	} else if (ret > 0) {
		/* Close the connection */
		NET_DBG("[EP:%p/%d] Connection closing after short header packet",
			target_ep, quic_get_by_ep(target_ep));

		quic_send_ack(target_ep, QUIC_SECRET_LEVEL_APPLICATION,
			      decrypted.packet_number);
		quic_endpoint_notify_streams_closed(target_ep);
		quic_endpoint_unref(target_ep);
		ret = 0;
		goto out;
	}

	/* Send ACK for 1-RTT packets, but NOT for ACK-only packets.
	 * Per RFC 9000 Section 13.2.1: "A sender MUST NOT send an ACK frame
	 * in response to a packet containing only ACK frames."
	 */
	if (!ack_only) {
		NET_DBG("[EP:%p/%d] Sending ACK for short header packet %" PRIu64,
			target_ep, quic_get_by_ep(target_ep), decrypted.packet_number);

		quic_send_ack(target_ep, QUIC_SECRET_LEVEL_APPLICATION,
			      decrypted.packet_number);
	}

	/* If the refcount is 1, then it means that the endpoint is being
	 * closed and we are the last one holding a reference.
	 */
	if (atomic_get(&target_ep->refcount) == 1) {
		NET_DBG("[EP:%p/%d] Endpoint closing",
			target_ep, quic_get_by_ep(target_ep));
	}

	ret = 0;
out:
	/* Release our ref we took before the crypto call */
	quic_endpoint_unref(target_ep);

	return ret;
}

/*
 * Parse a long header to determine the total packet length.
 * Returns total packet size including header.
 */
static int get_long_header_packet_length(const uint8_t *data, size_t data_len,
					 size_t *packet_len, uint64_t *token,
					 size_t *pn_offset)
{
	enum quic_packet_type ptype;
	uint8_t dcid_len, scid_len;
	uint64_t payload_len;
	int varint_size;
	size_t pos;

	if (data_len < 7) {
		return -EINVAL;
	}

	/* First byte + Version (4 bytes) */
	pos = 5;

	ptype = quic_get_long_packet_type(data[0]);

	/* DCID length and DCID */
	dcid_len = data[pos++];
	if (dcid_len > MAX_CONN_ID_LEN) {
		return -EINVAL;
	}

	pos += dcid_len;
	if (pos > data_len) {
		return -EINVAL;
	}

	/* SCID length and SCID */
	scid_len = data[pos++];
	if (scid_len > MAX_CONN_ID_LEN) {
		return -EINVAL;
	}

	pos += scid_len;
	if (pos > data_len) {
		return -EINVAL;
	}

	/* Token (only for Initial packets) */
	if (ptype == QUIC_PACKET_TYPE_INITIAL) {
		uint64_t token_len = 0;

		varint_size = quic_get_len(&data[pos], data_len - pos, &token_len);
		if (varint_size < 0) {
			return -EINVAL;
		}

		pos += varint_size;

		if (pos + token_len > data_len) {
			return -EINVAL;
		}

		if (token != NULL) {
			*token = token_len;
		}

		pos += token_len;
	}

	/* Length field (varint), this is PN length + encrypted payload + tag */
	if (pos >= data_len) {
		return -EINVAL;
	}

	varint_size = quic_get_len(&data[pos], data_len - pos, &payload_len);
	if (varint_size < 0) {
		return -EINVAL;
	}

	pos += varint_size;

	/* pos now points to the packet number, this is pn_offset */
	*pn_offset = pos;

	/* packet_len is the Length field value */
	*packet_len = payload_len;

	/* Return total packet length = header up to length field + payload_len */
	return pos + payload_len;
}

/*
 * Handle a complete UDP datagram which may contain coalesced QUIC packets.
 *
 * RFC 9000 Section 12.2: Multiple QUIC packets can be coalesced into a
 * single UDP datagram. Each packet is processed independently.
 */
static int handle_datagram(struct quic_endpoint *ep,
			   uint8_t *data, size_t data_len,
			   struct net_sockaddr *src_addr,
			   net_socklen_t addr_len)
{
	size_t offset = 0;
	int packets_processed = 0;
	bool long_header_queued = false;
	int ret;

	while (offset < data_len) {
		uint8_t first_byte = data[offset];
		size_t packet_len, total_len, pn_offset;
		uint64_t token = 0;

		/* Skip padding bytes (0x00) between coalesced packets */
		if (first_byte == 0x00) {
			offset++;
			continue;
		}

		if (quic_is_long_header(first_byte)) {
			/* Long header packet, need to parse to find length */
			ret = get_long_header_packet_length(data + offset,
							    data_len - offset,
							    &packet_len, &token,
							    &pn_offset);
			if (ret < 0) {
				NET_DBG("Failed to get long header packet length:  %d", ret);
				break;
			}

			total_len = ret;
		} else {
			/* Short header packet, consumes rest of datagram */
			packet_len = data_len - offset;
			total_len = packet_len;
			pn_offset = 0;
		}

		if (offset + total_len > data_len) {
			NET_ERR("Total packet length %zu exceeds datagram at offset %zu",
				total_len, offset);
			break;
		}

		/* Process this single packet based on header type */
		if (quic_is_long_header(first_byte)) {
			ret = process_long_header(ep, src_addr, addr_len,
						  data + offset, packet_len,
						  token, total_len, pn_offset,
						  data_len);
			if (ret == 0) {
				long_header_queued = true;
			}
		} else {
			/* If a long header packet was queued earlier in this
			 * datagram, force-queue the short header packet too.
			 * The long header may complete the handshake and derive
			 * application keys that this packet needs for decryption.
			 * Processing order is preserved via the FIFO queue.
			 */
			ret = process_short_header(ep, src_addr, addr_len,
						   data + offset, packet_len,
						   long_header_queued);
		}
		if (ret < 0) {
			NET_DBG("Failed to process packet at offset %zu: %d", offset, ret);
			break;
		}

		offset += total_len;
		packets_processed++;
	}

	return packets_processed > 0 ? packets_processed :  -EINVAL;
}

static void receive_data(int sock, struct quic_endpoint *ep_hint)
{
	/* Socket service will call this one packet at a time so we can use
	 * the buffer in endpoint pending data.
	 */
	struct net_sockaddr_in6 addr;
	net_socklen_t addrlen = sizeof(addr);
	struct quic_endpoint *ep;
	ssize_t len;
	int packets;

	/* The ep_hint from socket service user_data may not be correct if multiple
	 * endpoints share the same socket service. Look up the endpoint by socket.
	 */
	ep = find_endpoint_by_sock(sock);
	if (ep == NULL) {
		/* Fallback to hint if lookup fails */
		ep = ep_hint;
		if (ep == NULL) {
			NET_DBG("No endpoint found for socket %d", sock);
			return;
		}

		NET_DBG("Fallback to EP %p/%d when using socket %d",
			ep, quic_get_by_ep(ep), sock);
	}

	ep->pending.len = 0;
	ep->pending.pos = 0;

	len = zsock_recvfrom(sock, ep->pending.data, sizeof(ep->pending.data), 0,
			     (struct net_sockaddr *)&addr, &addrlen);
	if (len <= 0) {
		if (len < 0) {
			NET_DBG("recv (%d)", -errno);
		}

		return;
	}

	if (IS_ENABLED(CONFIG_QUIC_LOG_LEVEL_DBG)) {
		char addr_str[NET_INET6_ADDRSTRLEN];

		zsock_inet_ntop(addr.sin6_family, &addr.sin6_addr,
				addr_str, sizeof(addr_str));

		NET_DBG("[EP:%p/%d] Connection from %s%s%s:%d (%d)",
			ep, quic_get_by_ep(ep),
			addr.sin6_family == NET_AF_INET6 ? "[" : "",
			addr_str,
			addr.sin6_family == NET_AF_INET6 ? "]" : "",
			net_ntohs(addr.sin6_port), sock);
	}

	if (ep->remote_addr.ss_family != addr.sin6_family) {
		NET_ERR("[EP:%p/%d] Address family mismatch: endpoint %d vs packet %d",
			ep, quic_get_by_ep(ep), ep->remote_addr.ss_family, addr.sin6_family);
		quic_endpoint_unref(ep);
		return;
	}

	ep->pending.len = len;
	ep->pending.pos = 0;

	if (IS_ENABLED(CONFIG_QUIC_TXRX_DEBUG)) {
		char addr_str[NET_INET6_ADDRSTRLEN];
		char *str;

		str = net_addr_ntop(addr.sin6_family, &addr.sin6_addr,
				    addr_str, sizeof(addr_str));
		if (str != NULL) {
			net_hexdump(str, ep->pending.data, ep->pending.len);
		}
	}

	sock_obj_core_update_recv_stats(sock, ep->pending.len);

	packets = handle_datagram(ep, ep->pending.data, ep->pending.len,
				  (struct net_sockaddr *)&addr, addrlen);
	if (packets < 0) {
		NET_DBG("[EP:%p/%d] Failed to handle QUIC datagram (%d)", ep,
			quic_get_by_ep(ep), packets);
	} else {
		NET_DBG("[EP:%p/%d] Read %d QUIC packet%sfrom datagram", ep,
			quic_get_by_ep(ep), packets,
			packets > 1 ? "s " : " ");

		quic_stats_update_packets_rx(packets);
	}
}

static void quic_svc_handler(struct net_socket_service_event *pev)
{
	struct zsock_pollfd *pfd = &pev->event;

	receive_data(pfd->fd, pev->user_data);
}

/* Initialize TLS subsystem. */
static void tls_subsystem_init(void)
{
#if !defined(CONFIG_ENTROPY_HAS_DRIVER)
	NET_WARN("No entropy device on the system, "
		 "TLS communication is insecure!");
#endif

#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_init(&server_cache);
#endif
}

static void init_quic_service(void)
{
	k_tid_t tid;
	static struct k_thread quic_thread;

	static K_THREAD_STACK_DEFINE(quic_thread_stack,
				     CONFIG_QUIC_SERVICE_STACK_SIZE);

	tid = k_thread_create(&quic_thread,
			      quic_thread_stack,
			      K_THREAD_STACK_SIZEOF(quic_thread_stack),
			      (k_thread_entry_t)quic_service_thread, NULL, NULL, NULL,
			      CLAMP(CONFIG_QUIC_SERVICE_THREAD_PRIO,
				    K_HIGHEST_APPLICATION_THREAD_PRIO,
				    K_LOWEST_APPLICATION_THREAD_PRIO), 0, K_NO_WAIT);

	k_thread_name_set(tid, "quic_service");
}

/**
 * @brief Internal initialization function for QUIC library.
 *
 * Called by the network layer init procedure.
 *
 * @note This function is not exposed in a public header, as it's for internal
 * use and should therefore not be exposed to applications.
 */
void net_quic_init(void)
{
	k_mutex_init(&contexts_lock);
	k_mutex_init(&endpoints_lock);
	k_mutex_init(&streams_lock);

#if defined(CONFIG_NET_IPV4)
	ARRAY_FOR_EACH_PTR(quic_ipv4_pollfds, sockfd) {
		sockfd->fd = -1;
	}
#endif

#if defined(CONFIG_NET_IPV6)
	ARRAY_FOR_EACH_PTR(quic_ipv6_pollfds, sockfd) {
		sockfd->fd = -1;
	}
#endif

	tls_subsystem_init();
	init_quic_service();
}

#if defined(CONFIG_QUIC_TLS_DEBUG_KEYLOG)

#define KEYLOG_PREFIX "QUIC_KEYLOG: "

/*
 * Log TLS secrets in NSS Key Log format for Wireshark decryption.
 * Format: <label> <client_random_hex> <secret_hex>
 *
 * Labels for TLS 1.3/QUIC:
 * - CLIENT_HANDSHAKE_TRAFFIC_SECRET
 * - SERVER_HANDSHAKE_TRAFFIC_SECRET
 * - CLIENT_TRAFFIC_SECRET_0
 * - SERVER_TRAFFIC_SECRET_0
 * - EXPORTER_SECRET
 */
static void quic_log_tls_secret(const char *label,
				const uint8_t *client_random,
				const uint8_t *secret,
				size_t secret_len)
{
	char line[256];
	char *p = line;

	p += snprintk(p, sizeof(line), "%s ", label);

	/* Client random (32 bytes as hex) */
	for (int i = 0; i < 32; i++) {
		p += snprintk(p, sizeof(line) - (p - line), "%02x", client_random[i]);
	}

	*p++ = ' ';

	/* Secret (as hex) */
	for (size_t i = 0; i < secret_len; i++) {
		p += snprintk(p, sizeof(line) - (p - line), "%02x", secret[i]);
	}

	*p++ = '\n';
	*p = '\0';

	printk("%s%s", KEYLOG_PREFIX, line);
}
#endif /* CONFIG_QUIC_TLS_DEBUG_KEYLOG */

ZTESTABLE_STATIC struct quic_context *quic_get_context(int sock)
{
	struct quic_context *ctx;

	ctx = zvfs_get_fd_obj(sock,
			      (const struct fd_op_vtable *)&quic_ctx_fd_op_vtable,
			      ENOENT);
	if (ctx == NULL) {
		errno = ENOENT;
		return NULL;
	}

	if (!PART_OF_ARRAY(contexts, ctx)) {
		errno = EINVAL;
		return NULL;
	}

	return ctx;
}
