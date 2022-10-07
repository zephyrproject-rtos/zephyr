/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <fcntl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock_tls, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/sys/fdtable.h>

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
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/error.h>
#include <mbedtls/platform.h>
#include <mbedtls/ssl_cache.h>
#endif /* CONFIG_MBEDTLS */

#include "sockets_internal.h"
#include "tls_internal.h"

#if defined(CONFIG_MBEDTLS_BUILTIN)
#include <zephyr_mbedtls_priv.h>
#endif

#if defined(CONFIG_NET_SOCKETS_TLS_MAX_APP_PROTOCOLS)
#define ALPN_MAX_PROTOCOLS (CONFIG_NET_SOCKETS_TLS_MAX_APP_PROTOCOLS + 1)
#else
#define ALPN_MAX_PROTOCOLS 0
#endif /* CONFIG_NET_SOCKETS_TLS_MAX_APP_PROTOCOLS */

static const struct socket_op_vtable tls_sock_fd_op_vtable;

#ifndef MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED
#define MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE
#endif

/** A list of secure tags that TLS context should use. */
struct sec_tag_list {
	/** An array of secure tags referencing TLS credentials. */
	sec_tag_t sec_tags[CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS];

	/** Number of configured secure tags. */
	int sec_tag_count;
};

/** Timer context for DTLS. */
struct dtls_timing_context {
	/** Current time, stored during timer set. */
	uint32_t snapshot;

	/** Intermediate delay value. For details, refer to mbedTLS API
	 *  documentation (mbedtls_ssl_set_timer_t).
	 */
	uint32_t int_ms;

	/** Final delay value. For details, refer to mbedTLS API documentation
	 *  (mbedtls_ssl_set_timer_t).
	 */
	uint32_t fin_ms;
};

/** TLS peer address/session ID mapping. */
struct tls_session_cache {
	/** Creation time. */
	int64_t timestamp;

	/** Peer address. */
	struct sockaddr peer_addr;

	/** Session buffer. */
	uint8_t *session;

	/** Session length. */
	size_t session_len;
};

/** TLS context information. */
__net_socket struct tls_context {
	/** Information whether TLS context is used. */
	bool is_used;

	/** Underlying TCP/UDP socket. */
	int sock;

	/** Socket type. */
	enum net_sock_type type;

	/** Secure protocol version running on TLS context. */
	enum net_ip_protocol_secure tls_version;

	/** Socket flags passed to a socket call. */
	int flags;

	/** Information whether TLS context was initialized. */
	bool is_initialized;

	/** Information whether underlying socket is listening. */
	bool is_listening;

	/** Information whether TLS handshake is currently in progress. */
	bool handshake_in_progress;

	/** Information whether TLS handshake is complete or not. */
	struct k_sem tls_established;

	/** TLS specific option values. */
	struct {
		/** Select which credentials to use with TLS. */
		struct sec_tag_list sec_tag_list;

		/** 0-terminated list of allowed ciphersuites (mbedTLS format).
		 */
		int ciphersuites[CONFIG_NET_SOCKETS_TLS_MAX_CIPHERSUITES + 1];

		/** Information if hostname was explicitly set on a socket. */
		bool is_hostname_set;

		/** Peer verification level. */
		int8_t verify_level;

		/** Indicating on whether DER certificates should not be copied
		 * to the heap.
		 */
		int8_t cert_nocopy;

		/** DTLS role, client by default. */
		int8_t role;

		/** NULL-terminated list of allowed application layer
		 * protocols.
		 */
		const char *alpn_list[ALPN_MAX_PROTOCOLS];

		/** Session cache enabled on a socket. */
		bool cache_enabled;

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
		/* DTLS handshake timeout */
		uint32_t dtls_handshake_timeout_min;
		uint32_t dtls_handshake_timeout_max;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */
	} options;

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	/** Context information for DTLS timing. */
	struct dtls_timing_context dtls_timing;

	/** mbedTLS cookie context for DTLS */
	mbedtls_ssl_cookie_ctx cookie;

	/** DTLS peer address. */
	struct sockaddr dtls_peer_addr;

	/** DTLS peer address length. */
	socklen_t dtls_peer_addrlen;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

#if defined(CONFIG_MBEDTLS)
	/** mbedTLS context. */
	mbedtls_ssl_context ssl;

	/** mbedTLS configuration. */
	mbedtls_ssl_config config;

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	/** mbedTLS structure for CA chain. */
	mbedtls_x509_crt ca_chain;

	/** mbedTLS structure for own certificate. */
	mbedtls_x509_crt own_cert;

	/** mbedTLS structure for own private key. */
	mbedtls_pk_context priv_key;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#endif /* CONFIG_MBEDTLS */
};


/* A global pool of TLS contexts. */
static struct tls_context tls_contexts[CONFIG_NET_SOCKETS_TLS_MAX_CONTEXTS];

static struct tls_session_cache client_cache[CONFIG_NET_SOCKETS_TLS_MAX_CLIENT_SESSION_COUNT];

#if defined(MBEDTLS_SSL_CACHE_C)
static mbedtls_ssl_cache_context server_cache;
#endif

/* A mutex for protecting TLS context allocation. */
static struct k_mutex context_lock;

static void tls_session_cache_reset(void)
{
	for (int i = 0; i < ARRAY_SIZE(client_cache); i++) {
		if (client_cache[i].session != NULL) {
			mbedtls_free(client_cache[i].session);
		}
	}

	(void)memset(client_cache, 0, sizeof(client_cache));
}

bool net_socket_is_tls(void *obj)
{
	return PART_OF_ARRAY(tls_contexts, (struct tls_context *)obj);
}

static int tls_ctr_drbg_random(void *ctx, unsigned char *buf, size_t len)
{
	ARG_UNUSED(ctx);

#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return sys_csrand_get(buf, len);
#else
	sys_rand_get(buf, len);

	return 0;
#endif
}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
/* mbedTLS-defined function for setting timer. */
static void dtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
	struct dtls_timing_context *ctx = data;

	ctx->int_ms = int_ms;
	ctx->fin_ms = fin_ms;

	if (fin_ms != 0U) {
		ctx->snapshot = k_uptime_get_32();
	}
}

/* mbedTLS-defined function for getting timer status.
 * The return values are specified by mbedTLS. The callback must return:
 *   -1 if cancelled (fin_ms == 0),
 *    0 if none of the delays have passed,
 *    1 if only the intermediate delay has passed,
 *    2 if the final delay has passed.
 */
static int dtls_timing_get_delay(void *data)
{
	struct dtls_timing_context *timing = data;
	unsigned long elapsed_ms;

	NET_ASSERT(timing);

	if (timing->fin_ms == 0U) {
		return -1;
	}

	elapsed_ms = k_uptime_get_32() - timing->snapshot;

	if (elapsed_ms >= timing->fin_ms) {
		return 2;
	}

	if (elapsed_ms >= timing->int_ms) {
		return 1;
	}

	return 0;
}
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

/* Initialize TLS internals. */
static int tls_init(const struct device *unused)
{
	ARG_UNUSED(unused);

#if !defined(CONFIG_ENTROPY_HAS_DRIVER)
	NET_WARN("No entropy device on the system, "
		 "TLS communication is insecure!");
#endif

	(void)memset(tls_contexts, 0, sizeof(tls_contexts));
	(void)memset(client_cache, 0, sizeof(client_cache));

	k_mutex_init(&context_lock);

#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_init(&server_cache);
#endif

	return 0;
}

SYS_INIT(tls_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static inline bool is_handshake_complete(struct tls_context *ctx)
{
	return k_sem_count_get(&ctx->tls_established) != 0;
}

/*
 * Copied from include/mbedtls/ssl_internal.h
 *
 * Maximum length we can advertise as our max content length for
 * RFC 6066 max_fragment_length extension negotiation purposes
 * (the lesser of both sizes, if they are unequal.)
 */
#define MBEDTLS_TLS_EXT_ADV_CONTENT_LEN (                            \
	(MBEDTLS_SSL_IN_CONTENT_LEN > MBEDTLS_SSL_OUT_CONTENT_LEN)   \
	? (MBEDTLS_SSL_OUT_CONTENT_LEN)				     \
	: (MBEDTLS_SSL_IN_CONTENT_LEN)				     \
	)

#if defined(CONFIG_NET_SOCKETS_TLS_SET_MAX_FRAGMENT_LENGTH) &&	\
	defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH) &&		\
	(MBEDTLS_TLS_EXT_ADV_CONTENT_LEN < 16384)

BUILD_ASSERT(MBEDTLS_TLS_EXT_ADV_CONTENT_LEN >= 512,
	     "Too small content length!");

static inline unsigned char tls_mfl_code_from_content_len(void)
{
	size_t len = MBEDTLS_TLS_EXT_ADV_CONTENT_LEN;

	if (len >= 4096) {
		return MBEDTLS_SSL_MAX_FRAG_LEN_4096;
	} else if (len >= 2048) {
		return MBEDTLS_SSL_MAX_FRAG_LEN_2048;
	} else if (len >= 1024) {
		return MBEDTLS_SSL_MAX_FRAG_LEN_1024;
	} else if (len >= 512) {
		return MBEDTLS_SSL_MAX_FRAG_LEN_512;
	} else {
		return MBEDTLS_SSL_MAX_FRAG_LEN_INVALID;
	}
}

static inline void tls_set_max_frag_len(mbedtls_ssl_config *config)
{
	unsigned char mfl_code = tls_mfl_code_from_content_len();

	mbedtls_ssl_conf_max_frag_len(config, mfl_code);
}
#else
static inline void tls_set_max_frag_len(mbedtls_ssl_config *config) {}
#endif

/* Allocate TLS context. */
static struct tls_context *tls_alloc(void)
{
	int i;
	struct tls_context *tls = NULL;

	k_mutex_lock(&context_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(tls_contexts); i++) {
		if (!tls_contexts[i].is_used) {
			tls = &tls_contexts[i];
			(void)memset(tls, 0, sizeof(*tls));
			tls->is_used = true;
			tls->options.verify_level = -1;
			tls->sock = -1;

			NET_DBG("Allocated TLS context, %p", tls);
			break;
		}
	}

	k_mutex_unlock(&context_lock);

	if (tls) {
		k_sem_init(&tls->tls_established, 0, 1);

		mbedtls_ssl_init(&tls->ssl);
		mbedtls_ssl_config_init(&tls->config);
		tls_set_max_frag_len(&tls->config);
#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
		mbedtls_ssl_cookie_init(&tls->cookie);
		tls->options.dtls_handshake_timeout_min =
			MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MIN;
		tls->options.dtls_handshake_timeout_max =
			MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MAX;
#endif
#if defined(MBEDTLS_X509_CRT_PARSE_C)
		mbedtls_x509_crt_init(&tls->ca_chain);
		mbedtls_x509_crt_init(&tls->own_cert);
		mbedtls_pk_init(&tls->priv_key);
#endif

#if defined(CONFIG_MBEDTLS_DEBUG)
		mbedtls_ssl_conf_dbg(&tls->config, zephyr_mbedtls_debug, NULL);
#endif
	} else {
		NET_WARN("Failed to allocate TLS context");
	}

	return tls;
}

/* Allocate new TLS context and copy the content from the source context. */
static struct tls_context *tls_clone(struct tls_context *source_tls)
{
	struct tls_context *target_tls;

	target_tls = tls_alloc();
	if (!target_tls) {
		return NULL;
	}

	target_tls->tls_version = source_tls->tls_version;
	target_tls->type = source_tls->type;

	memcpy(&target_tls->options, &source_tls->options,
	       sizeof(target_tls->options));

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (target_tls->options.is_hostname_set) {
		mbedtls_ssl_set_hostname(&target_tls->ssl,
					 source_tls->ssl.hostname);
	}
#endif

	return target_tls;
}

/* Release TLS context. */
static int tls_release(struct tls_context *tls)
{
	if (!PART_OF_ARRAY(tls_contexts, tls)) {
		NET_ERR("Invalid TLS context");
		return -EBADF;
	}

	if (!tls->is_used) {
		NET_ERR("Deallocating unused TLS context");
		return -EBADF;
	}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	mbedtls_ssl_cookie_free(&tls->cookie);
#endif
	mbedtls_ssl_config_free(&tls->config);
	mbedtls_ssl_free(&tls->ssl);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_free(&tls->ca_chain);
	mbedtls_x509_crt_free(&tls->own_cert);
	mbedtls_pk_free(&tls->priv_key);
#endif

	tls->is_used = false;

	return 0;
}

static bool peer_addr_cmp(const struct sockaddr *addr,
			  const struct sockaddr *peer_addr)
{
	if (addr->sa_family != peer_addr->sa_family) {
		return false;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && peer_addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *addr1 = net_sin6(peer_addr);
		struct sockaddr_in6 *addr2 = net_sin6(addr);

		return (addr1->sin6_port == addr2->sin6_port) &&
			net_ipv6_addr_cmp(&addr1->sin6_addr, &addr2->sin6_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && peer_addr->sa_family == AF_INET) {
		struct sockaddr_in *addr1 = net_sin(peer_addr);
		struct sockaddr_in *addr2 = net_sin(addr);

		return (addr1->sin_port == addr2->sin_port) &&
			net_ipv4_addr_cmp(&addr1->sin_addr, &addr2->sin_addr);
	}

	return false;
}

static int tls_session_save(const struct sockaddr *peer_addr,
			    mbedtls_ssl_session *session)
{
	struct tls_session_cache *entry = NULL;
	size_t session_len;
	int ret;

	for (int i = 0; i < ARRAY_SIZE(client_cache); i++) {
		if (client_cache[i].session == NULL) {
			/* New entry. */
			if (entry == NULL || entry->session != NULL) {
				entry = &client_cache[i];
			}
		} else {
			if (peer_addr_cmp(&client_cache[i].peer_addr, peer_addr)) {
				/* Reuse old entry for given address. */
				entry = &client_cache[i];
				break;
			}

			/* Remember the oldest entry and reuse if needed. */
			if (entry == NULL ||
			    (entry->session != NULL &&
			     entry->timestamp < client_cache[i].timestamp)) {
				entry = &client_cache[i];
			}
		}
	}

	/* Allocate session and save */

	if (entry->session != NULL) {
		mbedtls_free(entry->session);
		entry->session = NULL;
	}

	(void)mbedtls_ssl_session_save(session, NULL, 0, &session_len);

	entry->session = mbedtls_calloc(1, session_len);
	if (entry->session == NULL) {
		NET_ERR("Failed to allocate session buffer.");
		return -ENOMEM;
	}

	ret = mbedtls_ssl_session_save(session, entry->session, session_len,
				       &session_len);
	if (ret < 0) {
		NET_ERR("Failed to serialize session, err: 0x%x.", -ret);
		mbedtls_free(entry->session);
		entry->session = NULL;
		return -ENOMEM;
	}

	entry->session_len = session_len;
	entry->timestamp = k_uptime_get();
	memcpy(&entry->peer_addr, peer_addr, sizeof(*peer_addr));

	return 0;
}

static int tls_session_get(const struct sockaddr *peer_addr,
			   mbedtls_ssl_session *session)
{
	struct tls_session_cache *entry = NULL;
	int ret;

	for (int i = 0; i < ARRAY_SIZE(client_cache); i++) {
		if (client_cache[i].session != NULL &&
		    peer_addr_cmp(&client_cache[i].peer_addr, peer_addr)) {
			entry = &client_cache[i];
			break;
		}
	}

	if (entry == NULL) {
		return -ENOENT;
	}

	ret = mbedtls_ssl_session_load(session, entry->session,
				       entry->session_len);
	if (ret < 0) {
		/* Discard corrupted session data. */
		mbedtls_free(entry->session);
		entry->session = NULL;
		return -EIO;
	}

	return 0;
}

static void tls_session_store(struct tls_context *context,
			      const struct sockaddr *addr,
			      socklen_t addrlen)
{
	mbedtls_ssl_session session;
	struct sockaddr peer_addr = { 0 };
	int ret;

	if (!context->options.cache_enabled) {
		return;
	}

	memcpy(&peer_addr, addr, addrlen);
	mbedtls_ssl_session_init(&session);

	ret = mbedtls_ssl_get_session(&context->ssl, &session);
	if (ret < 0) {
		NET_ERR("Failed to obtain session for %p", context);
		goto exit;
	}

	ret = tls_session_save(&peer_addr, &session);
	if (ret < 0) {
		NET_ERR("Failed to save session for %p", context);
	}

exit:
	mbedtls_ssl_session_free(&session);
}

static void tls_session_restore(struct tls_context *context,
				const struct sockaddr *addr,
				socklen_t addrlen)
{
	mbedtls_ssl_session session;
	struct sockaddr peer_addr = { 0 };
	int ret;

	if (!context->options.cache_enabled) {
		return;
	}

	memcpy(&peer_addr, addr, addrlen);
	mbedtls_ssl_session_init(&session);

	ret = tls_session_get(&peer_addr, &session);
	if (ret < 0) {
		NET_DBG("Session not found for %p", context);
		goto exit;
	}

	ret = mbedtls_ssl_set_session(&context->ssl, &session);
	if (ret < 0) {
		NET_ERR("Failed to set session for %p", context);
	}

exit:
	mbedtls_ssl_session_free(&session);
}

static void tls_session_purge(void)
{
	tls_session_cache_reset();

#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_free(&server_cache);
	mbedtls_ssl_cache_init(&server_cache);
#endif
}

static inline int time_left(uint32_t start, uint32_t timeout)
{
	uint32_t elapsed = k_uptime_get_32() - start;

	return timeout - elapsed;
}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
static bool dtls_is_peer_addr_valid(struct tls_context *context,
				    const struct sockaddr *peer_addr,
				    socklen_t addrlen)
{
	if (context->dtls_peer_addrlen != addrlen) {
		return false;
	}

	return peer_addr_cmp(&context->dtls_peer_addr, peer_addr);
}

static void dtls_peer_address_set(struct tls_context *context,
				  const struct sockaddr *peer_addr,
				  socklen_t addrlen)
{
	if (addrlen <= sizeof(context->dtls_peer_addr)) {
		memcpy(&context->dtls_peer_addr, peer_addr, addrlen);
		context->dtls_peer_addrlen = addrlen;
	}
}

static void dtls_peer_address_get(struct tls_context *context,
				  struct sockaddr *peer_addr,
				  socklen_t *addrlen)
{
	socklen_t len = MIN(context->dtls_peer_addrlen, *addrlen);

	memcpy(peer_addr, &context->dtls_peer_addr, len);
	*addrlen = len;
}

static int dtls_tx(void *ctx, const unsigned char *buf, size_t len)
{
	struct tls_context *tls_ctx = ctx;
	ssize_t sent;

	sent = zsock_sendto(tls_ctx->sock, buf, len, tls_ctx->flags,
			    &tls_ctx->dtls_peer_addr,
			    tls_ctx->dtls_peer_addrlen);
	if (sent < 0) {
		if (errno == EAGAIN) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		}

		return MBEDTLS_ERR_NET_SEND_FAILED;
	}

	return sent;
}

static int dtls_rx(void *ctx, unsigned char *buf, size_t len,
		   uint32_t dtls_timeout)
{
	struct tls_context *tls_ctx = ctx;
	int sock_flags = zsock_fcntl(tls_ctx->sock, F_GETFL, 0);
	bool is_block;
	int timeout = (dtls_timeout == 0U) ? -1 : dtls_timeout;
	uint32_t entry_time = k_uptime_get_32();
	socklen_t addrlen = sizeof(struct sockaddr);
	struct sockaddr addr;
	int err;
	ssize_t received;
	bool retry;
	struct zsock_pollfd fds;
	int flags = tls_ctx->flags & ~ZSOCK_MSG_TRUNC;

	if (sock_flags == -1) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	is_block = !((tls_ctx->flags & ZSOCK_MSG_DONTWAIT) ||
		     (sock_flags & O_NONBLOCK));

	do {
		retry = false;

		/* mbedtLS does not allow blocking rx for DTLS, therefore use
		 * k_poll for timeout functionality.
		 */
		if (is_block) {
			fds.fd = tls_ctx->sock;
			fds.events = ZSOCK_POLLIN;

			if (zsock_poll(&fds, 1, timeout) == 0) {
				return MBEDTLS_ERR_SSL_TIMEOUT;
			}
		}

		received = zsock_recvfrom(tls_ctx->sock, buf, len, flags,
					  &addr, &addrlen);
		if (received < 0) {
			if (errno == EAGAIN) {
				return MBEDTLS_ERR_SSL_WANT_READ;
			}

			return MBEDTLS_ERR_NET_RECV_FAILED;
		}

		if (tls_ctx->dtls_peer_addrlen == 0) {
			/* Only allow to store peer address for DTLS servers. */
			if (tls_ctx->options.role == MBEDTLS_SSL_IS_SERVER) {
				dtls_peer_address_set(tls_ctx, &addr, addrlen);

				err = mbedtls_ssl_set_client_transport_id(
					&tls_ctx->ssl,
					(const unsigned char *)&addr, addrlen);
				if (err < 0) {
					return err;
				}
			} else {
				/* For clients it's incorrect to receive when
				 * no peer has been set up.
				 */
				return MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED;
			}
		} else if (!dtls_is_peer_addr_valid(tls_ctx, &addr, addrlen)) {
			/* Received data from different peer, ignore it. */
			retry = true;

			if (timeout != -1) {
				/* Recalculate the timeout value. */
				timeout = time_left(entry_time, dtls_timeout);

				if (timeout <= 0) {
					return MBEDTLS_ERR_SSL_TIMEOUT;
				}
			}
		}
	} while (retry);

	return received;
}
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

static int tls_tx(void *ctx, const unsigned char *buf, size_t len)
{
	struct tls_context *tls_ctx = ctx;
	ssize_t sent;

	sent = zsock_sendto(tls_ctx->sock, buf, len,
			    tls_ctx->flags, NULL, 0);
	if (sent < 0) {
		if (errno == EAGAIN) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		}

		return MBEDTLS_ERR_NET_SEND_FAILED;
	}

	return sent;
}

static int tls_rx(void *ctx, unsigned char *buf, size_t len)
{
	struct tls_context *tls_ctx = ctx;
	int flags = tls_ctx->flags & ~ZSOCK_MSG_WAITALL;
	ssize_t received;

	received = zsock_recvfrom(tls_ctx->sock, buf, len, flags, NULL, 0);
	if (received < 0) {
		if (errno == EAGAIN) {
			return MBEDTLS_ERR_SSL_WANT_READ;
		}

		return MBEDTLS_ERR_NET_RECV_FAILED;
	}

	return received;
}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
static bool crt_is_pem(const unsigned char *buf, size_t buflen)
{
	return (buflen != 0 && buf[buflen - 1] == '\0' &&
		strstr((const char *)buf, "-----BEGIN CERTIFICATE-----") != NULL);
}
#endif

static int tls_add_ca_certificate(struct tls_context *tls,
				  struct tls_credential *ca_cert)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err;

	if (tls->options.cert_nocopy == TLS_CERT_NOCOPY_NONE ||
	    crt_is_pem(ca_cert->buf, ca_cert->len)) {
		err = mbedtls_x509_crt_parse(&tls->ca_chain, ca_cert->buf,
					     ca_cert->len);
	} else {
		err = mbedtls_x509_crt_parse_der_nocopy(&tls->ca_chain,
							ca_cert->buf,
							ca_cert->len);
	}

	if (err != 0) {
		return -EINVAL;
	}

	return 0;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return -ENOTSUP;
}

static void tls_set_ca_chain(struct tls_context *tls)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_ssl_conf_ca_chain(&tls->config, &tls->ca_chain, NULL);
	mbedtls_ssl_conf_cert_profile(&tls->config,
				      &mbedtls_x509_crt_profile_default);
#endif /* MBEDTLS_X509_CRT_PARSE_C */
}

static int tls_add_own_cert(struct tls_context *tls,
			    struct tls_credential *own_cert)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err;

	if (tls->options.cert_nocopy == TLS_CERT_NOCOPY_NONE ||
	    crt_is_pem(own_cert->buf, own_cert->len)) {
		err = mbedtls_x509_crt_parse(&tls->own_cert,
					     own_cert->buf, own_cert->len);
	} else {
		err = mbedtls_x509_crt_parse_der_nocopy(&tls->own_cert,
							own_cert->buf,
							own_cert->len);
	}

	if (err != 0) {
		return -EINVAL;
	}

	return 0;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return -ENOTSUP;
}

static int tls_set_own_cert(struct tls_context *tls)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err = mbedtls_ssl_conf_own_cert(&tls->config, &tls->own_cert,
					    &tls->priv_key);
	if (err != 0) {
		err = -ENOMEM;
	}

	return err;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return -ENOTSUP;
}

static int tls_set_private_key(struct tls_context *tls,
			       struct tls_credential *priv_key)
{
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	int err;

	err = mbedtls_pk_parse_key(&tls->priv_key, priv_key->buf,
				   priv_key->len, NULL, 0,
				   tls_ctr_drbg_random, NULL);
	if (err != 0) {
		return -EINVAL;
	}

	return 0;
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return -ENOTSUP;
}

static int tls_set_psk(struct tls_context *tls,
		       struct tls_credential *psk,
		       struct tls_credential *psk_id)
{
#if defined(MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED)
	int err = mbedtls_ssl_conf_psk(&tls->config,
				       psk->buf, psk->len,
				       (const unsigned char *)psk_id->buf,
				       psk_id->len);
	if (err != 0) {
		return -EINVAL;
	}

	return 0;
#endif

	return -ENOTSUP;
}

static int tls_set_credential(struct tls_context *tls,
			      struct tls_credential *cred)
{
	switch (cred->type) {
	case TLS_CREDENTIAL_CA_CERTIFICATE:
		return tls_add_ca_certificate(tls, cred);

	case TLS_CREDENTIAL_SERVER_CERTIFICATE:
		return tls_add_own_cert(tls, cred);

	case TLS_CREDENTIAL_PRIVATE_KEY:
		return tls_set_private_key(tls, cred);
	break;

	case TLS_CREDENTIAL_PSK:
	{
		struct tls_credential *psk_id =
			credential_get(cred->tag, TLS_CREDENTIAL_PSK_ID);
		if (!psk_id) {
			return -ENOENT;
		}

		return tls_set_psk(tls, cred, psk_id);
	}

	case TLS_CREDENTIAL_PSK_ID:
		/* Ignore PSK ID - it will be used together
		 * with PSK
		 */
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int tls_mbedtls_set_credentials(struct tls_context *tls)
{
	struct tls_credential *cred;
	sec_tag_t tag;
	int i, err = 0;
	bool tag_found, ca_cert_present = false, own_cert_present = false;

	credentials_lock();

	for (i = 0; i < tls->options.sec_tag_list.sec_tag_count; i++) {
		tag = tls->options.sec_tag_list.sec_tags[i];
		cred = NULL;
		tag_found = false;

		while ((cred = credential_next_get(tag, cred)) != NULL) {
			tag_found = true;

			err = tls_set_credential(tls, cred);
			if (err != 0) {
				goto exit;
			}

			if (cred->type == TLS_CREDENTIAL_CA_CERTIFICATE) {
				ca_cert_present = true;
			} else if (cred->type == TLS_CREDENTIAL_SERVER_CERTIFICATE) {
				own_cert_present = true;
			}
		}

		if (!tag_found) {
			err = -ENOENT;
			goto exit;
		}
	}

exit:
	credentials_unlock();

	if (err == 0) {
		if (ca_cert_present) {
			tls_set_ca_chain(tls);
		}
		if (own_cert_present) {
			err = tls_set_own_cert(tls);
		}
	}

	return err;
}

static int tls_mbedtls_reset(struct tls_context *context)
{
	int ret;

	ret = mbedtls_ssl_session_reset(&context->ssl);
	if (ret != 0) {
		return ret;
	}

	k_sem_reset(&context->tls_established);

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	/* Server role: reset the address so that a new
	 *              client can connect w/o a need to reopen a socket
	 * Client role: keep peer addr so socket can continue to be used
	 *              even on handshake timeout
	 */
	if (context->options.role == MBEDTLS_SSL_IS_SERVER) {
		(void)memset(&context->dtls_peer_addr, 0,
			     sizeof(context->dtls_peer_addr));
		context->dtls_peer_addrlen = 0;
	}
#endif

	return 0;
}

static int tls_mbedtls_handshake(struct tls_context *context, bool block)
{
	int ret;
	int sock_flags;

	sock_flags = zsock_fcntl(context->sock, F_GETFL, 0);
	if (sock_flags < 0) {
		return -EIO;
	}

	if (block && sock_flags & O_NONBLOCK) {
		/* Clear the O_NONBLOCK flag for the handshake to prevent busy
		 * looping in the handshake thread.
		 */
		(void)zsock_fcntl(context->sock, F_SETFL,
				  sock_flags & ~O_NONBLOCK);
	}

	context->handshake_in_progress = true;

	while ((ret = mbedtls_ssl_handshake(&context->ssl)) != 0) {
		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
		    ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
		    ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
			if (block) {
				continue;
			}

			ret = -EAGAIN;
			break;
		} else if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
			ret = tls_mbedtls_reset(context);
			if (ret == 0) {
				if (block) {
					continue;
				}

				ret = -EAGAIN;
				break;
			}
		} else if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
			/* MbedTLS API documentation requires session to
			 * be reset in this case
			 */
			ret = tls_mbedtls_reset(context);
			if (ret == 0) {
				NET_ERR("TLS handshake timeout");
				ret = -ETIMEDOUT;
				break;
			}
		} else {
			/* MbedTLS API documentation requires session to
			 * be reset in other error cases
			 */
			NET_ERR("TLS handshake error: -%x", -ret);
			ret = tls_mbedtls_reset(context);
			if (ret == 0) {
				ret = -ECONNABORTED;
				break;
			}
		}

		/* Avoid constant loop if tls_mbedtls_reset fails */
		NET_ERR("TLS reset error: -%x", -ret);
		ret = -ECONNABORTED;
		break;
	}

	if (block && sock_flags & O_NONBLOCK) {
		(void)zsock_fcntl(context->sock, F_SETFL, sock_flags);
	}

	if (ret == 0) {
		k_sem_give(&context->tls_established);
	}

	context->handshake_in_progress = false;

	return ret;
}

static int tls_mbedtls_init(struct tls_context *context, bool is_server)
{
	int role, type, ret;

	role = is_server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT;

	type = (context->type == SOCK_STREAM) ?
		MBEDTLS_SSL_TRANSPORT_STREAM :
		MBEDTLS_SSL_TRANSPORT_DATAGRAM;

	if (type == MBEDTLS_SSL_TRANSPORT_STREAM) {
		mbedtls_ssl_set_bio(&context->ssl, context,
				    tls_tx, tls_rx, NULL);
	} else {
#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
		mbedtls_ssl_set_bio(&context->ssl, context,
				    dtls_tx, NULL, dtls_rx);
#else
		return -ENOTSUP;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */
	}

	ret = mbedtls_ssl_config_defaults(&context->config, role, type,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		/* According to mbedTLS API documentation,
		 * mbedtls_ssl_config_defaults can fail due to memory
		 * allocation failure
		 */
		return -ENOMEM;
	}

#if defined(MBEDTLS_SSL_RENEGOTIATION)
	mbedtls_ssl_conf_legacy_renegotiation(&context->config,
					   MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE);
	mbedtls_ssl_conf_renegotiation(&context->config,
				       MBEDTLS_SSL_RENEGOTIATION_ENABLED);
#endif

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	if (type == MBEDTLS_SSL_TRANSPORT_DATAGRAM) {
		/* DTLS requires timer callbacks to operate */
		mbedtls_ssl_set_timer_cb(&context->ssl,
					 &context->dtls_timing,
					 dtls_timing_set_delay,
					 dtls_timing_get_delay);
		mbedtls_ssl_conf_handshake_timeout(&context->config,
				context->options.dtls_handshake_timeout_min,
				context->options.dtls_handshake_timeout_max);

		/* Configure cookie for DTLS server */
		if (role == MBEDTLS_SSL_IS_SERVER) {
			ret = mbedtls_ssl_cookie_setup(&context->cookie,
						       tls_ctr_drbg_random,
						       NULL);
			if (ret != 0) {
				return -ENOMEM;
			}

			mbedtls_ssl_conf_dtls_cookies(&context->config,
						      mbedtls_ssl_cookie_write,
						      mbedtls_ssl_cookie_check,
						      &context->cookie);

			mbedtls_ssl_conf_read_timeout(
					&context->config,
					CONFIG_NET_SOCKETS_DTLS_TIMEOUT);
		}
	}
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	/* For TLS clients, set hostname to empty string to enforce it's
	 * verification - only if hostname option was not set. Otherwise
	 * depend on user configuration.
	 */
	if (!is_server && !context->options.is_hostname_set) {
		mbedtls_ssl_set_hostname(&context->ssl, "");
	}
#endif

	/* If verification level was specified explicitly, set it. Otherwise,
	 * use mbedTLS default values (required for client, none for server)
	 */
	if (context->options.verify_level != -1) {
		mbedtls_ssl_conf_authmode(&context->config,
					  context->options.verify_level);
	}

	mbedtls_ssl_conf_rng(&context->config,
			     tls_ctr_drbg_random,
			     NULL);

	ret = tls_mbedtls_set_credentials(context);
	if (ret != 0) {
		return ret;
	}

	if (context->options.ciphersuites[0] != 0) {
		/* Specific ciphersuites configured, so use them */
		NET_DBG("Using user-specified ciphersuites");
		mbedtls_ssl_conf_ciphersuites(&context->config,
					      context->options.ciphersuites);
	}

#if defined(CONFIG_MBEDTLS_SSL_ALPN)
	if (ALPN_MAX_PROTOCOLS && context->options.alpn_list[0] != NULL) {
		ret = mbedtls_ssl_conf_alpn_protocols(&context->config,
				context->options.alpn_list);
		if (ret != 0) {
			return -EINVAL;
		}
	}
#endif /* CONFIG_MBEDTLS_SSL_ALPN */

#if defined(MBEDTLS_SSL_CACHE_C)
	if (is_server && context->options.cache_enabled) {
		mbedtls_ssl_conf_session_cache(&context->config, &server_cache,
					       mbedtls_ssl_cache_get,
					       mbedtls_ssl_cache_set);
	}
#endif

	ret = mbedtls_ssl_setup(&context->ssl,
				&context->config);
	if (ret != 0) {
		/* According to mbedTLS API documentation,
		 * mbedtls_ssl_setup can fail due to memory allocation failure
		 */
		return -ENOMEM;
	}

	context->is_initialized = true;

	return 0;
}

static int tls_opt_sec_tag_list_set(struct tls_context *context,
				    const void *optval, socklen_t optlen)
{
	int sec_tag_cnt;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen % sizeof(sec_tag_t) != 0) {
		return -EINVAL;
	}

	sec_tag_cnt = optlen / sizeof(sec_tag_t);
	if (sec_tag_cnt >
		ARRAY_SIZE(context->options.sec_tag_list.sec_tags)) {
		return -EINVAL;
	}

	memcpy(context->options.sec_tag_list.sec_tags, optval, optlen);
	context->options.sec_tag_list.sec_tag_count = sec_tag_cnt;

	return 0;
}

static int sock_opt_protocol_get(struct tls_context *context,
				 void *optval, socklen_t *optlen)
{
	int protocol = (int)context->tls_version;

	if (*optlen != sizeof(protocol)) {
		return -EINVAL;
	}

	*(int *)optval = protocol;

	return 0;
}

static int tls_opt_sec_tag_list_get(struct tls_context *context,
				    void *optval, socklen_t *optlen)
{
	int len;

	if (*optlen % sizeof(sec_tag_t) != 0 || *optlen == 0) {
		return -EINVAL;
	}

	len = MIN(context->options.sec_tag_list.sec_tag_count *
		  sizeof(sec_tag_t), *optlen);

	memcpy(optval, context->options.sec_tag_list.sec_tags, len);
	*optlen = len;

	return 0;
}

static int tls_opt_hostname_set(struct tls_context *context,
				const void *optval, socklen_t optlen)
{
	ARG_UNUSED(optlen);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (mbedtls_ssl_set_hostname(&context->ssl, optval) != 0) {
		return -EINVAL;
	}
#else
	return -ENOPROTOOPT;
#endif

	context->options.is_hostname_set = true;

	return 0;
}

static int tls_opt_ciphersuite_list_set(struct tls_context *context,
					const void *optval, socklen_t optlen)
{
	int cipher_cnt;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen % sizeof(int) != 0) {
		return -EINVAL;
	}

	cipher_cnt = optlen / sizeof(int);

	/* + 1 for 0-termination. */
	if (cipher_cnt + 1 > ARRAY_SIZE(context->options.ciphersuites)) {
		return -EINVAL;
	}

	memcpy(context->options.ciphersuites, optval, optlen);
	context->options.ciphersuites[cipher_cnt] = 0;

	mbedtls_ssl_conf_ciphersuites(&context->config,
				      context->options.ciphersuites);
	return 0;
}

static int tls_opt_ciphersuite_list_get(struct tls_context *context,
					void *optval, socklen_t *optlen)
{
	const int *selected_ciphers;
	int cipher_cnt, i = 0;
	int *ciphers = optval;

	if (*optlen % sizeof(int) != 0 || *optlen == 0) {
		return -EINVAL;
	}

	if (context->options.ciphersuites[0] == 0) {
		/* No specific ciphersuites configured, return all available. */
		selected_ciphers = mbedtls_ssl_list_ciphersuites();
	} else {
		selected_ciphers = context->options.ciphersuites;
	}

	cipher_cnt = *optlen / sizeof(int);
	while (selected_ciphers[i] != 0) {
		ciphers[i] = selected_ciphers[i];

		if (++i == cipher_cnt) {
			break;
		}
	}

	*optlen = i * sizeof(int);

	return 0;
}

static int tls_opt_ciphersuite_used_get(struct tls_context *context,
					void *optval, socklen_t *optlen)
{
	const char *ciph;

	if (*optlen != sizeof(int)) {
		return -EINVAL;
	}

	ciph = mbedtls_ssl_get_ciphersuite(&context->ssl);
	if (ciph == NULL) {
		return -ENOTCONN;
	}

	*(int *)optval = mbedtls_ssl_get_ciphersuite_id(ciph);

	return 0;
}

static int tls_opt_alpn_list_set(struct tls_context *context,
				 const void *optval, socklen_t optlen)
{
	int alpn_cnt;

	if (!ALPN_MAX_PROTOCOLS) {
		return -EINVAL;
	}

	if (!optval) {
		return -EINVAL;
	}

	if (optlen % sizeof(const char *) != 0) {
		return -EINVAL;
	}

	alpn_cnt = optlen / sizeof(const char *);
	/* + 1 for NULL-termination. */
	if (alpn_cnt + 1 > ARRAY_SIZE(context->options.alpn_list)) {
		return -EINVAL;
	}

	memcpy(context->options.alpn_list, optval, optlen);
	context->options.alpn_list[alpn_cnt] = NULL;

	return 0;
}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
static int tls_opt_dtls_handshake_timeout_get(struct tls_context *context,
					      void *optval, socklen_t *optlen,
					      bool is_max)
{
	uint32_t *val = (uint32_t *)optval;

	if (sizeof(uint32_t) != *optlen) {
		return -EINVAL;
	}

	if (is_max) {
		*val = context->options.dtls_handshake_timeout_max;
	} else {
		*val = context->options.dtls_handshake_timeout_min;
	}

	return 0;
}

static int tls_opt_dtls_handshake_timeout_set(struct tls_context *context,
					      const void *optval,
					      socklen_t optlen, bool is_max)
{
	uint32_t *val = (uint32_t *)optval;

	if (!optval) {
		return -EINVAL;
	}

	if (sizeof(uint32_t) != optlen) {
		return -EINVAL;
	}

	/* If mbedTLS context not inited, it will
	 * use these values upon init.
	 */
	if (is_max) {
		context->options.dtls_handshake_timeout_max = *val;
	} else {
		context->options.dtls_handshake_timeout_min = *val;
	}

	/* If mbedTLS context already inited, we need to
	 * update mbedTLS config for it to take effect
	 */
	mbedtls_ssl_conf_handshake_timeout(&context->config,
			context->options.dtls_handshake_timeout_min,
			context->options.dtls_handshake_timeout_max);

	return 0;
}
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

static int tls_opt_alpn_list_get(struct tls_context *context,
				 void *optval, socklen_t *optlen)
{
	const char **alpn_list = context->options.alpn_list;
	int alpn_cnt, i = 0;
	const char **ret_list = optval;

	if (!ALPN_MAX_PROTOCOLS) {
		return -EINVAL;
	}

	if (*optlen % sizeof(const char *) != 0 || *optlen == 0) {
		return -EINVAL;
	}

	alpn_cnt = *optlen / sizeof(const char *);
	while (alpn_list[i] != NULL) {
		ret_list[i] = alpn_list[i];

		if (++i == alpn_cnt) {
			break;
		}
	}

	*optlen = i * sizeof(const char *);

	return 0;
}

static int tls_opt_session_cache_set(struct tls_context *context,
				     const void *optval, socklen_t optlen)
{
	int *val = (int *)optval;

	if (!optval) {
		return -EINVAL;
	}

	if (sizeof(int) != optlen) {
		return -EINVAL;
	}

	context->options.cache_enabled = (*val == TLS_SESSION_CACHE_ENABLED);

	return 0;
}

static int tls_opt_session_cache_get(struct tls_context *context,
				     void *optval, socklen_t *optlen)
{
	int cache_enabled = context->options.cache_enabled ?
			    TLS_SESSION_CACHE_ENABLED :
			    TLS_SESSION_CACHE_DISABLED;

	if (*optlen != sizeof(cache_enabled)) {
		return -EINVAL;
	}

	*(int *)optval = cache_enabled;

	return 0;
}

static int tls_opt_session_cache_purge_set(struct tls_context *context,
					   const void *optval, socklen_t optlen)
{
	ARG_UNUSED(context);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	tls_session_purge();

	return 0;
}

static int tls_opt_peer_verify_set(struct tls_context *context,
				   const void *optval, socklen_t optlen)
{
	int *peer_verify;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen != sizeof(int)) {
		return -EINVAL;
	}

	peer_verify = (int *)optval;

	if (*peer_verify != MBEDTLS_SSL_VERIFY_NONE &&
	    *peer_verify != MBEDTLS_SSL_VERIFY_OPTIONAL &&
	    *peer_verify != MBEDTLS_SSL_VERIFY_REQUIRED) {
		return -EINVAL;
	}

	context->options.verify_level = *peer_verify;

	return 0;
}

static int tls_opt_cert_nocopy_set(struct tls_context *context,
				   const void *optval, socklen_t optlen)
{
	int *cert_nocopy;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen != sizeof(int)) {
		return -EINVAL;
	}

	cert_nocopy = (int *)optval;

	if (*cert_nocopy != TLS_CERT_NOCOPY_NONE &&
	    *cert_nocopy != TLS_CERT_NOCOPY_OPTIONAL) {
		return -EINVAL;
	}

	context->options.cert_nocopy = *cert_nocopy;

	return 0;
}

static int tls_opt_dtls_role_set(struct tls_context *context,
				 const void *optval, socklen_t optlen)
{
	int *role;

	if (!optval) {
		return -EINVAL;
	}

	if (optlen != sizeof(int)) {
		return -EINVAL;
	}

	role = (int *)optval;
	if (*role != MBEDTLS_SSL_IS_CLIENT &&
	    *role != MBEDTLS_SSL_IS_SERVER) {
		return -EINVAL;
	}

	context->options.role = *role;

	return 0;
}

static int protocol_check(int family, int type, int *proto)
{
	if (family != AF_INET && family != AF_INET6) {
		return -EAFNOSUPPORT;
	}

	if (*proto >= IPPROTO_TLS_1_0 && *proto <= IPPROTO_TLS_1_2) {
		if (type != SOCK_STREAM) {
			return -EPROTOTYPE;
		}

		*proto = IPPROTO_TCP;
	} else if (*proto >= IPPROTO_DTLS_1_0 && *proto <= IPPROTO_DTLS_1_2) {
		if (!IS_ENABLED(CONFIG_NET_SOCKETS_ENABLE_DTLS)) {
			return -EPROTONOSUPPORT;
		}

		if (type != SOCK_DGRAM) {
			return -EPROTOTYPE;
		}

		*proto = IPPROTO_UDP;
	} else {
		return -EPROTONOSUPPORT;
	}

	return 0;
}

static int ztls_socket(int family, int type, int proto)
{
	enum net_ip_protocol_secure tls_proto = proto;
	int fd = z_reserve_fd();
	int sock = -1;
	int ret;
	struct tls_context *ctx;

	if (fd < 0) {
		return -1;
	}

	ret = protocol_check(family, type, &proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	ctx = tls_alloc();
	if (ctx == NULL) {
		errno = ENOMEM;
		goto free_fd;
	}

	sock = zsock_socket(family, type, proto);
	if (sock < 0) {
		goto release_tls;
	}

	ctx->tls_version = tls_proto;
	ctx->type = (proto == IPPROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM;
	ctx->sock = sock;

	z_finalize_fd(
		fd, ctx, (const struct fd_op_vtable *)&tls_sock_fd_op_vtable);

	return fd;

release_tls:
	(void)tls_release(ctx);

free_fd:
	z_free_fd(fd);

	return -1;
}

int ztls_close_ctx(struct tls_context *ctx)
{
	int ret, err = 0;

	/* Try to send close notification. */
	ctx->flags = 0;

	(void)mbedtls_ssl_close_notify(&ctx->ssl);

	err = tls_release(ctx);
	ret = zsock_close(ctx->sock);

	/* In case close fails, we propagate errno value set by close.
	 * In case close succeeds, but tls_release fails, set errno
	 * according to tls_release return value.
	 */
	if (ret == 0 && err < 0) {
		errno = -err;
		ret = -1;
	}

	return ret;
}

int ztls_connect_ctx(struct tls_context *ctx, const struct sockaddr *addr,
		     socklen_t addrlen)
{
	int ret;

	ret = zsock_connect(ctx->sock, addr, addrlen);
	if (ret < 0) {
		return ret;
	}

	if (ctx->type == SOCK_STREAM) {
		/* Do the handshake for TLS, not DTLS. */
		ret = tls_mbedtls_init(ctx, false);
		if (ret < 0) {
			goto error;
		}

		/* Do not use any socket flags during the handshake. */
		ctx->flags = 0;

		tls_session_restore(ctx, addr, addrlen);

		/* TODO For simplicity, TLS handshake blocks the socket
		 * even for non-blocking socket.
		 */
		ret = tls_mbedtls_handshake(ctx, true);
		if (ret < 0) {
			goto error;
		}

		tls_session_store(ctx, addr, addrlen);
	} else {
#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
		/* Just store the address. */
		dtls_peer_address_set(ctx, addr, addrlen);
#else
		ret = -ENOTSUP;
		goto error;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */
	}

	return 0;

error:
	errno = -ret;
	return -1;
}

int ztls_accept_ctx(struct tls_context *parent, struct sockaddr *addr,
		    socklen_t *addrlen)
{
	struct tls_context *child = NULL;
	int ret, err, fd, sock;

	fd = z_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	sock = zsock_accept(parent->sock, addr, addrlen);
	if (sock < 0) {
		ret = -errno;
		goto error;
	}

	child = tls_clone(parent);
	if (child == NULL) {
		ret = -ENOMEM;
		goto error;
	}

	z_finalize_fd(
		fd, child, (const struct fd_op_vtable *)&tls_sock_fd_op_vtable);

	child->sock = sock;

	ret = tls_mbedtls_init(child, true);
	if (ret < 0) {
		goto error;
	}

	/* Do not use any socket flags during the handshake. */
	child->flags = 0;

	/* TODO For simplicity, TLS handshake blocks the socket even for
	 * non-blocking socket.
	 */
	ret = tls_mbedtls_handshake(child, true);
	if (ret < 0) {
		goto error;
	}

	return fd;

error:
	if (child != NULL) {
		err = tls_release(child);
		__ASSERT(err == 0, "TLS context release failed");
	}

	if (sock >= 0) {
		err = zsock_close(sock);
		__ASSERT(err == 0, "Child socket close failed");
	}

	z_free_fd(fd);

	errno = -ret;
	return -1;
}

static ssize_t send_tls(struct tls_context *ctx, const void *buf,
			size_t len, int flags)
{
	int ret;

	ret = mbedtls_ssl_write(&ctx->ssl, buf, len);
	if (ret >= 0) {
		return ret;
	}

	if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
	    ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
	    ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
	    ret ==  MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
		errno = EAGAIN;
	} else {
		(void)tls_mbedtls_reset(ctx);
		errno = EIO;
	}

	return -1;
}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
static ssize_t sendto_dtls_client(struct tls_context *ctx, const void *buf,
				  size_t len, int flags,
				  const struct sockaddr *dest_addr,
				  socklen_t addrlen)
{
	int ret;

	if (!dest_addr) {
		/* No address provided, check if we have stored one,
		 * otherwise return error.
		 */
		if (ctx->dtls_peer_addrlen == 0) {
			ret = -EDESTADDRREQ;
			goto error;
		}
	} else if (ctx->dtls_peer_addrlen == 0) {
		/* Address provided and no peer address stored. */
		dtls_peer_address_set(ctx, dest_addr, addrlen);
	} else if (!dtls_is_peer_addr_valid(ctx, dest_addr, addrlen) != 0) {
		/* Address provided but it does not match stored one */
		ret = -EISCONN;
		goto error;
	}

	if (!ctx->is_initialized) {
		ret = tls_mbedtls_init(ctx, false);
		if (ret < 0) {
			goto error;
		}
	}

	if (!is_handshake_complete(ctx)) {
		tls_session_restore(ctx, &ctx->dtls_peer_addr,
				    ctx->dtls_peer_addrlen);

		/* TODO For simplicity, TLS handshake blocks the socket even for
		 * non-blocking socket.
		 */
		ret = tls_mbedtls_handshake(ctx, true);
		if (ret < 0) {
			goto error;
		}

		tls_session_store(ctx, &ctx->dtls_peer_addr,
				  ctx->dtls_peer_addrlen);
	}

	return send_tls(ctx, buf, len, flags);

error:
	errno = -ret;
	return -1;
}

static ssize_t sendto_dtls_server(struct tls_context *ctx, const void *buf,
				  size_t len, int flags,
				  const struct sockaddr *dest_addr,
				  socklen_t addrlen)
{
	/* For DTLS server, require to have established DTLS connection
	 * in order to send data.
	 */
	if (!is_handshake_complete(ctx)) {
		errno = ENOTCONN;
		return -1;
	}

	/* Verify we are sending to a peer that we have connection with. */
	if (dest_addr &&
	    !dtls_is_peer_addr_valid(ctx, dest_addr, addrlen) != 0) {
		errno = EISCONN;
		return -1;
	}

	return send_tls(ctx, buf, len, flags);
}
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

ssize_t ztls_sendto_ctx(struct tls_context *ctx, const void *buf, size_t len,
			int flags, const struct sockaddr *dest_addr,
			socklen_t addrlen)
{
	ctx->flags = flags;

	/* TLS */
	if (ctx->type == SOCK_STREAM) {
		return send_tls(ctx, buf, len, flags);
	}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	/* DTLS */
	if (ctx->options.role == MBEDTLS_SSL_IS_SERVER) {
		return sendto_dtls_server(ctx, buf, len, flags,
					  dest_addr, addrlen);
	}

	return sendto_dtls_client(ctx, buf, len, flags, dest_addr, addrlen);
#else
	errno = ENOTSUP;
	return -1;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */
}

ssize_t ztls_sendmsg_ctx(struct tls_context *ctx, const struct msghdr *msg,
			 int flags)
{
	ssize_t len;
	ssize_t ret;
	int i;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_ENABLE_DTLS) &&
	    ctx->type == SOCK_DGRAM) {
		/*
		 * Current mbedTLS API (i.e. mbedtls_ssl_write()) allows only to send a single
		 * contiguous buffer. This means that gather write using sendmsg() can only be
		 * handled correctly if there is a single non-empty buffer in msg->msg_iov.
		 */
		if (msghdr_non_empty_iov_count(msg) > 1) {
			errno = EMSGSIZE;
			return -1;
		}
	}

	len = 0;
	if (msg) {
		for (i = 0; i < msg->msg_iovlen; i++) {
			struct iovec *vec = msg->msg_iov + i;
			size_t sent = 0;

			if (vec->iov_len == 0) {
				continue;
			}

			while (sent < vec->iov_len) {
				uint8_t *ptr = (uint8_t *)vec->iov_base + sent;

				ret = ztls_sendto_ctx(ctx, ptr,
					    vec->iov_len - sent, flags,
					    msg->msg_name, msg->msg_namelen);
				if (ret < 0) {
					return ret;
				}
				sent += ret;
			}
			len += sent;
		}
	}

	return len;
}

static ssize_t recv_tls(struct tls_context *ctx, void *buf,
			size_t max_len, int flags)
{
	size_t recv_len = 0;
	const bool waitall = flags & ZSOCK_MSG_WAITALL;
	int ret;

	do {
		size_t read_len = max_len - recv_len;

		ret = mbedtls_ssl_read(&ctx->ssl, (uint8_t *)buf + recv_len,
				       read_len);
		if (ret < 0) {
			if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
				/* Peer notified that it's closing the
				 * connection.
				 */
				break;
			}

			if (ret == MBEDTLS_ERR_SSL_CLIENT_RECONNECT) {
				/* Client reconnect on the same socket is not
				 * supported. See mbedtls_ssl_read API
				 * documentation.
				 */
				break;
			}

			if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
			    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
				if (recv_len > 0) {
					break;
				}

				ret = -EAGAIN;
			} else {
				ret = -EIO;
			}

			errno = -ret;
			return -1;
		}

		if (ret == 0) {
			break;
		}

		recv_len += ret;
	} while (waitall && (recv_len < max_len));

	return recv_len;
}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
static ssize_t recvfrom_dtls_common(struct tls_context *ctx, void *buf,
				    size_t max_len, int flags,
				    struct sockaddr *src_addr,
				    socklen_t *addrlen)
{
	int ret;

	ret = mbedtls_ssl_read(&ctx->ssl, buf, max_len);
	if (ret >= 0) {
		size_t remaining;

		if (src_addr && addrlen) {
			dtls_peer_address_get(ctx, src_addr, addrlen);
		}

		/* mbedtls_ssl_get_bytes_avail() indicate the data length
		 * remaining in the current datagram.
		 */
		remaining = mbedtls_ssl_get_bytes_avail(&ctx->ssl);

		/* No more data in the datagram, or dummy read. */
		if ((remaining == 0) || (max_len == 0)) {
			return ret;
		}

		if (flags & ZSOCK_MSG_TRUNC) {
			ret += remaining;
		}

		for (int i = 0; i < remaining; i++) {
			uint8_t byte;
			int err;

			err = mbedtls_ssl_read(&ctx->ssl, &byte, sizeof(byte));
			if (err <= 0) {
				NET_ERR("Error while flushing the rest of the"
					" datagram, err %d", err);
				ret = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
				break;
			}
		}
	}


	return ret;
}

static ssize_t recvfrom_dtls_client(struct tls_context *ctx, void *buf,
				    size_t max_len, int flags,
				    struct sockaddr *src_addr,
				    socklen_t *addrlen)
{
	int ret;

	if (!is_handshake_complete(ctx)) {
		ret = -ENOTCONN;
		goto error;
	}

	ret = recvfrom_dtls_common(ctx, buf, max_len, flags, src_addr, addrlen);
	if (ret >= 0) {
		return ret;
	}

	switch (ret) {
	case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
		/* Peer notified that it's closing the connection. */
		ret = tls_mbedtls_reset(ctx);
		if (ret == 0) {
			ret = -ENOTCONN;
		} else {
			ret = -ENOMEM;
		}
		break;

	case MBEDTLS_ERR_SSL_TIMEOUT:
		(void)mbedtls_ssl_close_notify(&ctx->ssl);
		ret = -ETIMEDOUT;
		break;

	case MBEDTLS_ERR_SSL_WANT_READ:
	case MBEDTLS_ERR_SSL_WANT_WRITE:
		ret = -EAGAIN;
		break;

	default:
		ret = -EIO;
		break;
	}

error:
	errno = -ret;
	return -1;
}

static ssize_t recvfrom_dtls_server(struct tls_context *ctx, void *buf,
				    size_t max_len, int flags,
				    struct sockaddr *src_addr,
				    socklen_t *addrlen)
{
	int ret;
	bool repeat;
	int sock_flags = zsock_fcntl(ctx->sock, F_GETFL, 0);
	bool is_block;

	if (sock_flags == -1) {
		ret = -errno;
		goto error;
	}

	if (!ctx->is_initialized) {
		ret = tls_mbedtls_init(ctx, true);
		if (ret < 0) {
			goto error;
		}
	}

	is_block = !((flags & ZSOCK_MSG_DONTWAIT) || (sock_flags & O_NONBLOCK));

	/* Loop to enable DTLS reconnection for servers without closing
	 * a socket.
	 */
	do {
		repeat = false;

		if (!is_handshake_complete(ctx)) {
			ret = tls_mbedtls_handshake(ctx, is_block);
			if (ret < 0) {
				/* In case of EAGAIN, just exit. */
				if (ret == -EAGAIN) {
					break;
				}

				ret = tls_mbedtls_reset(ctx);
				if (ret == 0) {
					repeat = true;
				} else {
					ret = -ENOMEM;
				}

				continue;
			}
		}

		ret = recvfrom_dtls_common(ctx, buf, max_len, flags,
					   src_addr, addrlen);
		if (ret >= 0) {
			return ret;
		}

		switch (ret) {
		case MBEDTLS_ERR_SSL_TIMEOUT:
			(void)mbedtls_ssl_close_notify(&ctx->ssl);
			__fallthrough;
			/* fallthrough */

		case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
		case MBEDTLS_ERR_SSL_CLIENT_RECONNECT:
			ret = tls_mbedtls_reset(ctx);
			if (ret == 0) {
				repeat = true;
			} else {
				ret = -ENOMEM;
			}
			break;

		case MBEDTLS_ERR_SSL_WANT_READ:
		case MBEDTLS_ERR_SSL_WANT_WRITE:
			ret = -EAGAIN;
			break;

		default:
			ret = -EIO;
			break;
		}
	} while (repeat);

error:
	errno = -ret;
	return -1;
}
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

ssize_t ztls_recvfrom_ctx(struct tls_context *ctx, void *buf, size_t max_len,
			  int flags, struct sockaddr *src_addr,
			  socklen_t *addrlen)
{
	if (flags & ZSOCK_MSG_PEEK) {
		/* TODO mbedTLS does not support 'peeking' This could be
		 * bypassed by having intermediate buffer for peeking
		 */
		errno = ENOTSUP;
		return -1;
	}

	ctx->flags = flags;

	/* TLS */
	if (ctx->type == SOCK_STREAM) {
		return recv_tls(ctx, buf, max_len, flags);
	}

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	/* DTLS */
	if (ctx->options.role == MBEDTLS_SSL_IS_SERVER) {
		return recvfrom_dtls_server(ctx, buf, max_len, flags,
					    src_addr, addrlen);
	}

	return recvfrom_dtls_client(ctx, buf, max_len, flags,
				    src_addr, addrlen);
#else
	errno = ENOTSUP;
	return -1;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */
}

static int ztls_poll_prepare_pollin(struct tls_context *ctx)
{
	/* If there already is mbedTLS data to read, there is no
	 * need to set the k_poll_event object. Return EALREADY
	 * so we won't block in the k_poll.
	 */
	if (!ctx->is_listening) {
		if (mbedtls_ssl_get_bytes_avail(&ctx->ssl) > 0) {
			return -EALREADY;
		}
	}

	return 0;
}

static int ztls_poll_prepare_ctx(struct tls_context *ctx,
				 struct zsock_pollfd *pfd,
				 struct k_poll_event **pev,
				 struct k_poll_event *pev_end)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;
	void *obj;
	int ret;
	short events = pfd->events;

	/* DTLS client should wait for the handshake to complete before
	 * it actually starts to poll for data.
	 */
	if ((pfd->events & ZSOCK_POLLIN) && (ctx->type == SOCK_DGRAM) &&
	    (ctx->options.role == MBEDTLS_SSL_IS_CLIENT) &&
	    !is_handshake_complete(ctx)) {
		(*pev)->obj = &ctx->tls_established;
		(*pev)->type = K_POLL_TYPE_SEM_AVAILABLE;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;

		/* Since k_poll_event is configured by the TLS layer in this
		 * case, do not forward ZSOCK_POLLIN to the underlying socket.
		 */
		pfd->events &= ~ZSOCK_POLLIN;
	}

	obj = z_get_fd_obj_and_vtable(
		ctx->sock, (const struct fd_op_vtable **)&vtable, &lock);
	if (obj == NULL) {
		ret = -EBADF;
		goto exit;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	ret = z_fdtable_call_ioctl(vtable, obj, ZFD_IOCTL_POLL_PREPARE,
				   pfd, pev, pev_end);
	if (ret != 0) {
		goto exit;
	}

	if (pfd->events & ZSOCK_POLLIN) {
		ret = ztls_poll_prepare_pollin(ctx);
	}

exit:
	/* Restore original events. */
	pfd->events = events;

	k_mutex_unlock(lock);

	return ret;
}

static int ztls_socket_data_check(struct tls_context *ctx)
{
	int ret;

	ctx->flags = ZSOCK_MSG_DONTWAIT;

	ret = mbedtls_ssl_read(&ctx->ssl, NULL, 0);
	if (ret < 0) {
		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			/* Don't reset the context for STREAM socket - the
			 * application needs to reopen the socket anyway, and
			 * resetting the context would result in an error instead
			 * of 0 in a consecutive recv() call.
			 */
			if (ctx->type == SOCK_DGRAM) {
				ret = tls_mbedtls_reset(ctx);
				if (ret != 0) {
					return -ENOMEM;
				}
			}

			return -ENOTCONN;
		}

		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			return 0;
		}

		/* Treat any other error as fatal. */
		return -EIO;
	}

	return mbedtls_ssl_get_bytes_avail(&ctx->ssl);
}

static int ztls_poll_update_pollin(int fd, struct tls_context *ctx,
				   struct zsock_pollfd *pfd)
{
	int ret;

	if (!ctx->is_listening) {
		/* Already had TLS data to read on socket. */
		if (mbedtls_ssl_get_bytes_avail(&ctx->ssl) > 0) {
			pfd->revents |= ZSOCK_POLLIN;
			goto next;
		}
	}

	if (!(pfd->revents & ZSOCK_POLLIN)) {
		/* No new data on a socket. */
		goto next;
	}

	if (ctx->is_listening) {
		goto next;
	}

	ret = ztls_socket_data_check(ctx);
	if (ret == -ENOTCONN || (pfd->revents & ZSOCK_POLLHUP)) {
		/* Datagram does not return 0 on consecutive recv, but an error
		 * code, hence clear POLLIN.
		 */
		if (ctx->type == SOCK_DGRAM) {
			pfd->revents &= ~ZSOCK_POLLIN;
		}
		pfd->revents |= ZSOCK_POLLHUP;
		goto next;
	} else if (ret < 0) {
		pfd->revents |= ZSOCK_POLLERR;
		goto next;
	} else if (ret == 0) {
		goto again;
	}

next:
	return 0;

again:
	/* Received encrypted data, but still not enough
	 * to decrypt it and return data through socket,
	 * ask for retry if no other events are set.
	 */
	pfd->revents &= ~ZSOCK_POLLIN;

	return -EAGAIN;
}

static int ztls_poll_update_ctx(struct tls_context *ctx,
				struct zsock_pollfd *pfd,
				struct k_poll_event **pev)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;
	void *obj;
	int ret;
	short events = pfd->events;

	obj = z_get_fd_obj_and_vtable(
		ctx->sock, (const struct fd_op_vtable **)&vtable, &lock);
	if (obj == NULL) {
		return -EBADF;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	/* Check if the socket was waiting for the handshake to complete. */
	if ((pfd->events & ZSOCK_POLLIN) &&
	    ((*pev)->obj == &ctx->tls_established)) {
		/* In case handshake is complete, reconfigure the k_poll_event
		 * to monitor the underlying socket now.
		 */
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			ret = z_fdtable_call_ioctl(vtable, obj,
						   ZFD_IOCTL_POLL_PREPARE,
						   pfd, pev, *pev + 1);
			if (ret != 0 && ret != -EALREADY) {
				goto out;
			}

			/* Return -EAGAIN to signal to poll() that it should
			 * make another iteration with the event reconfigured
			 * above (if needed).
			 */
			ret = -EAGAIN;
			goto out;
		}

		/* Handshake still not ready - skip ZSOCK_POLLIN verification
		 * for the underlying socket.
		 */
		(*pev)++;
		pfd->events &= ~ZSOCK_POLLIN;
	}

	ret = z_fdtable_call_ioctl(vtable, obj, ZFD_IOCTL_POLL_UPDATE,
				   pfd, pev);
	if (ret != 0) {
		goto exit;
	}

	if (pfd->events & ZSOCK_POLLIN) {
		ret = ztls_poll_update_pollin(pfd->fd, ctx, pfd);
		if (ret == -EAGAIN && pfd->revents != 0) {
			(*pev - 1)->state = K_POLL_STATE_NOT_READY;
			goto exit;
		}
	}
exit:
	/* Restore original events. */
	pfd->events = events;

out:
	k_mutex_unlock(lock);

	return ret;
}

/* Return true if needed to retry rightoff or false otherwise. */
static bool poll_offload_dtls_client_retry(struct tls_context *ctx,
					   struct zsock_pollfd *pfd)
{
	/* DTLS client should wait for the handshake to complete before it
	 * reports that data is ready.
	 */
	if ((ctx->type != SOCK_DGRAM) ||
	    (ctx->options.role != MBEDTLS_SSL_IS_CLIENT)) {
		return false;
	}

	if (ctx->handshake_in_progress) {
		/* Add some sleep to allow lower priority threads to proceed
		 * with handshake.
		 */
		k_msleep(10);

		pfd->revents &= ~ZSOCK_POLLIN;
		return true;
	} else if (!is_handshake_complete(ctx)) {
		uint8_t byte;
		int ret;

		/* Handshake didn't start yet - just drop the incoming data -
		 * it's the client who should initiate the handshake.
		 */
		ret = zsock_recv(ctx->sock, &byte, sizeof(byte),
				 ZSOCK_MSG_DONTWAIT);
		if (ret < 0) {
			pfd->revents |= ZSOCK_POLLERR;
		}

		pfd->revents &= ~ZSOCK_POLLIN;
		return true;
	}

	/* Handshake complete, just proceed. */
	return false;
}

static int ztls_poll_offload(struct zsock_pollfd *fds, int nfds, int timeout)
{
	int fd_backup[CONFIG_NET_SOCKETS_POLL_MAX];
	const struct fd_op_vtable *vtable;
	void *ctx;
	int ret = 0;
	int result;
	int i;
	bool retry;
	int remaining;
	uint32_t entry = k_uptime_get_32();

	/* Overwrite TLS file descriptors with underlying ones. */
	for (i = 0; i < nfds; i++) {
		fd_backup[i] = fds[i].fd;

		ctx = z_get_fd_obj(fds[i].fd,
				   (const struct fd_op_vtable *)
						     &tls_sock_fd_op_vtable,
				   0);
		if (ctx == NULL) {
			continue;
		}

		if (fds[i].events & ZSOCK_POLLIN) {
			ret = ztls_poll_prepare_pollin(ctx);
			/* In case data is already available in mbedtls,
			 * do not wait in poll.
			 */
			if (ret == -EALREADY) {
				timeout = 0;
			}
		}

		fds[i].fd = ((struct tls_context *)ctx)->sock;
	}

	/* Get offloaded sockets vtable. */
	ctx = z_get_fd_obj_and_vtable(fds[0].fd,
				      (const struct fd_op_vtable **)&vtable,
				      NULL);
	if (ctx == NULL) {
		errno = EINVAL;
		goto exit;
	}

	remaining = timeout;

	do {
		for (i = 0; i < nfds; i++) {
			fds[i].revents = 0;
		}

		ret = z_fdtable_call_ioctl(vtable, ctx, ZFD_IOCTL_POLL_OFFLOAD,
					   fds, nfds, remaining);
		if (ret < 0) {
			goto exit;
		}

		retry = false;
		ret = 0;

		for (i = 0; i < nfds; i++) {
			ctx = z_get_fd_obj(fd_backup[i],
					   (const struct fd_op_vtable *)
							&tls_sock_fd_op_vtable,
					   0);
			if (ctx != NULL) {
				if (fds[i].events & ZSOCK_POLLIN) {
					if (poll_offload_dtls_client_retry(
							ctx, &fds[i])) {
						retry = true;
						continue;
					}

					result = ztls_poll_update_pollin(
						    fd_backup[i], ctx, &fds[i]);
					if (result == -EAGAIN) {
						retry = true;
					}
				}
			}

			if (fds[i].revents != 0) {
				ret++;
			}
		}

		if (retry) {
			if (ret > 0 || timeout == 0) {
				goto exit;
			}

			if (timeout > 0) {
				remaining = time_left(entry, timeout);
				if (remaining <= 0) {
					goto exit;
				}
			}
		}
	} while (retry);

exit:
	/* Restore original fds. */
	for (i = 0; i < nfds; i++) {
		fds[i].fd = fd_backup[i];
	}

	return ret;
}

int ztls_getsockopt_ctx(struct tls_context *ctx, int level, int optname,
			void *optval, socklen_t *optlen)
{
	int err;

	if (!optval || !optlen) {
		errno = EINVAL;
		return -1;
	}

	if ((level == SOL_SOCKET) && (optname == SO_PROTOCOL)) {
		/* Protocol type is overridden during socket creation. Its
		 * value is restored here to return current value.
		 */
		err = sock_opt_protocol_get(ctx, optval, optlen);
		if (err < 0) {
			errno = -err;
			return -1;
		}
		return err;
	} else if (level != SOL_TLS) {
		return zsock_getsockopt(ctx->sock, level, optname,
					optval, optlen);
	}

	switch (optname) {
	case TLS_SEC_TAG_LIST:
		err =  tls_opt_sec_tag_list_get(ctx, optval, optlen);
		break;

	case TLS_CIPHERSUITE_LIST:
		err = tls_opt_ciphersuite_list_get(ctx, optval, optlen);
		break;

	case TLS_CIPHERSUITE_USED:
		err = tls_opt_ciphersuite_used_get(ctx, optval, optlen);
		break;

	case TLS_ALPN_LIST:
		err = tls_opt_alpn_list_get(ctx, optval, optlen);
		break;

	case TLS_SESSION_CACHE:
		err = tls_opt_session_cache_get(ctx, optval, optlen);
		break;

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	case TLS_DTLS_HANDSHAKE_TIMEOUT_MIN:
		err = tls_opt_dtls_handshake_timeout_get(ctx, optval,
							 optlen, false);
		break;

	case TLS_DTLS_HANDSHAKE_TIMEOUT_MAX:
		err = tls_opt_dtls_handshake_timeout_get(ctx, optval,
							 optlen, true);
		break;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

	default:
		/* Unknown or write-only option. */
		err = -ENOPROTOOPT;
		break;
	}

	if (err < 0) {
		errno = -err;
		return -1;
	}

	return 0;
}

int ztls_setsockopt_ctx(struct tls_context *ctx, int level, int optname,
			const void *optval, socklen_t optlen)
{
	int err;

	if (level != SOL_TLS) {
		return zsock_setsockopt(ctx->sock, level, optname,
					optval, optlen);
	}

	switch (optname) {
	case TLS_SEC_TAG_LIST:
		err =  tls_opt_sec_tag_list_set(ctx, optval, optlen);
		break;

	case TLS_HOSTNAME:
		err = tls_opt_hostname_set(ctx, optval, optlen);
		break;

	case TLS_CIPHERSUITE_LIST:
		err = tls_opt_ciphersuite_list_set(ctx, optval, optlen);
		break;

	case TLS_PEER_VERIFY:
		err = tls_opt_peer_verify_set(ctx, optval, optlen);
		break;

	case TLS_CERT_NOCOPY:
		err = tls_opt_cert_nocopy_set(ctx, optval, optlen);
		break;

	case TLS_DTLS_ROLE:
		err = tls_opt_dtls_role_set(ctx, optval, optlen);
		break;

	case TLS_ALPN_LIST:
		err = tls_opt_alpn_list_set(ctx, optval, optlen);
		break;

	case TLS_SESSION_CACHE:
		err = tls_opt_session_cache_set(ctx, optval, optlen);
		break;

	case TLS_SESSION_CACHE_PURGE:
		err = tls_opt_session_cache_purge_set(ctx, optval, optlen);
		break;

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	case TLS_DTLS_HANDSHAKE_TIMEOUT_MIN:
		err = tls_opt_dtls_handshake_timeout_set(ctx, optval,
							 optlen, false);
		break;

	case TLS_DTLS_HANDSHAKE_TIMEOUT_MAX:
		err = tls_opt_dtls_handshake_timeout_set(ctx, optval,
							 optlen, true);
		break;
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

	case TLS_NATIVE:
		/* Option handled at the socket dispatcher level. */
		err = 0;
		break;

	default:
		/* Unknown or read-only option. */
		err = -ENOPROTOOPT;
		break;
	}

	if (err < 0) {
		errno = -err;
		return -1;
	}

	return 0;
}

static ssize_t tls_sock_read_vmeth(void *obj, void *buffer, size_t count)
{
	return ztls_recvfrom_ctx(obj, buffer, count, 0, NULL, 0);
}

static ssize_t tls_sock_write_vmeth(void *obj, const void *buffer,
				    size_t count)
{
	return ztls_sendto_ctx(obj, buffer, count, 0, NULL, 0);
}

static int tls_sock_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	struct tls_context *ctx = obj;

	switch (request) {
	/* fcntl() commands */
	case F_GETFL:
	case F_SETFL: {
		const struct fd_op_vtable *vtable;
		struct k_mutex *lock;
		void *obj;
		int ret;

		obj = z_get_fd_obj_and_vtable(ctx->sock,
				(const struct fd_op_vtable **)&vtable, &lock);
		if (obj == NULL) {
			errno = EBADF;
			return -1;
		}

		(void)k_mutex_lock(lock, K_FOREVER);

		/* Pass the call to the core socket implementation. */
		ret = vtable->ioctl(obj, request, args);

		k_mutex_unlock(lock);

		return ret;
	}

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return ztls_poll_prepare_ctx(obj, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return ztls_poll_update_ctx(obj, pfd, pev);
	}

	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds;
		int timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return ztls_poll_offload(fds, nfds, timeout);
	}

	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static int tls_sock_shutdown_vmeth(void *obj, int how)
{
	struct tls_context *ctx = obj;

	return zsock_shutdown(ctx->sock, how);
}

static int tls_sock_bind_vmeth(void *obj, const struct sockaddr *addr,
			       socklen_t addrlen)
{
	struct tls_context *ctx = obj;

	return zsock_bind(ctx->sock, addr, addrlen);
}

static int tls_sock_connect_vmeth(void *obj, const struct sockaddr *addr,
				  socklen_t addrlen)
{
	return ztls_connect_ctx(obj, addr, addrlen);
}

static int tls_sock_listen_vmeth(void *obj, int backlog)
{
	struct tls_context *ctx = obj;

	ctx->is_listening = true;

	return zsock_listen(ctx->sock, backlog);
}

static int tls_sock_accept_vmeth(void *obj, struct sockaddr *addr,
				 socklen_t *addrlen)
{
	return ztls_accept_ctx(obj, addr, addrlen);
}

static ssize_t tls_sock_sendto_vmeth(void *obj, const void *buf, size_t len,
				     int flags,
				     const struct sockaddr *dest_addr,
				     socklen_t addrlen)
{
	return ztls_sendto_ctx(obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t tls_sock_sendmsg_vmeth(void *obj, const struct msghdr *msg,
				      int flags)
{
	return ztls_sendmsg_ctx(obj, msg, flags);
}

static ssize_t tls_sock_recvfrom_vmeth(void *obj, void *buf, size_t max_len,
				       int flags, struct sockaddr *src_addr,
				       socklen_t *addrlen)
{
	return ztls_recvfrom_ctx(obj, buf, max_len, flags,
				 src_addr, addrlen);
}

static int tls_sock_getsockopt_vmeth(void *obj, int level, int optname,
				     void *optval, socklen_t *optlen)
{
	return ztls_getsockopt_ctx(obj, level, optname, optval, optlen);
}

static int tls_sock_setsockopt_vmeth(void *obj, int level, int optname,
				     const void *optval, socklen_t optlen)
{
	return ztls_setsockopt_ctx(obj, level, optname, optval, optlen);
}

static int tls_sock_close_vmeth(void *obj)
{
	return ztls_close_ctx(obj);
}

static int tls_sock_getpeername_vmeth(void *obj, struct sockaddr *addr,
				      socklen_t *addrlen)
{
	struct tls_context *ctx = obj;

	return zsock_getpeername(ctx->sock, addr, addrlen);
}

static int tls_sock_getsockname_vmeth(void *obj, struct sockaddr *addr,
				      socklen_t *addrlen)
{
	struct tls_context *ctx = obj;

	return zsock_getsockname(ctx->sock, addr, addrlen);
}

static const struct socket_op_vtable tls_sock_fd_op_vtable = {
	.fd_vtable = {
		.read = tls_sock_read_vmeth,
		.write = tls_sock_write_vmeth,
		.close = tls_sock_close_vmeth,
		.ioctl = tls_sock_ioctl_vmeth,
	},
	.shutdown = tls_sock_shutdown_vmeth,
	.bind = tls_sock_bind_vmeth,
	.connect = tls_sock_connect_vmeth,
	.listen = tls_sock_listen_vmeth,
	.accept = tls_sock_accept_vmeth,
	.sendto = tls_sock_sendto_vmeth,
	.sendmsg = tls_sock_sendmsg_vmeth,
	.recvfrom = tls_sock_recvfrom_vmeth,
	.getsockopt = tls_sock_getsockopt_vmeth,
	.setsockopt = tls_sock_setsockopt_vmeth,
	.getpeername = tls_sock_getpeername_vmeth,
	.getsockname = tls_sock_getsockname_vmeth,
};

static bool tls_is_supported(int family, int type, int proto)
{
	if (protocol_check(family, type, &proto) == 0) {
		return true;
	}

	return false;
}

/* Since both, TLS sockets and regular ones fall under the same address family,
 * it's required to process TLS first in order to capture socket calls which
 * create sockets for secure protocols. Every other call for AF_INET/AF_INET6
 * will be forwarded to regular socket implementation.
 */
BUILD_ASSERT(CONFIG_NET_SOCKETS_TLS_PRIORITY < CONFIG_NET_SOCKETS_PRIORITY_DEFAULT,
	     "CONFIG_NET_SOCKETS_TLS_PRIORITY have to be smaller than CONFIG_NET_SOCKETS_PRIORITY_DEFAULT");

NET_SOCKET_REGISTER(tls, CONFIG_NET_SOCKETS_TLS_PRIORITY, AF_UNSPEC,
		    tls_is_supported, ztls_socket);
