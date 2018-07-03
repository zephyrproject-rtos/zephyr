/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_SOCKETS)
#define SYS_LOG_DOMAIN "net/tls"
#define NET_LOG_ENABLED 1
#endif

#include <stdbool.h>

#include <init.h>
#include <net/net_context.h>
#include <net/socket.h>

#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#endif /* CONFIG_MBEDTLS */

/** TLS context information. */
struct tls_context {
	/** Information whether TLS context is used. */
	bool is_used;

	/** Secure protocol version running on TLS context. */
	enum net_ip_protocol_secure tls_version;

#if defined(CONFIG_MBEDTLS)
	/** mbedTLS context. */
	mbedtls_ssl_context ssl;

	/** mbedTLS configuration. */
	mbedtls_ssl_config config;
#endif /* CONFIG_MBEDTLS */
};

static mbedtls_ctr_drbg_context tls_ctr_drbg;

/* A global pool of TLS contexts. */
static struct tls_context tls_contexts[CONFIG_NET_SOCKETS_TLS_MAX_CONTEXTS];

/* A mutex for protecting TLS context allocation. */
static struct k_mutex context_lock;

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_SOCKETS)
static void tls_debug(void *ctx, int level, const char *file,
		      int line, const char *str)
{
	const char *p, *basename;

	ARG_UNUSED(ctx);

	if (!file || !str) {
		return;
	}

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

	NET_DBG("%s:%04d: |%d| %s", basename, line, level, str);
}
#endif /* defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_TLS_DEBUG) */

#if defined(CONFIG_ENTROPY_HAS_DRIVER)
static int tls_entropy_func(void *ctx, unsigned char *buf, size_t len)
{
	return entropy_get_entropy(ctx, buf, len);
}
#else
static int tls_entropy_func(void *ctx, unsigned char *buf, size_t len)
{
	ARG_UNUSED(ctx);

	size_t i = len / 4;
	u32_t val;

	while (i--) {
		val = sys_rand32_get();
		UNALIGNED_PUT(val, (u32_t *)buf);
		buf += 4;
	}

	i = len & 0x3;
	val = sys_rand32_get();
	while (i--) {
		*buf++ = val;
		val >>= 8;
	}

	return 0;
}
#endif /* defined(CONFIG_ENTROPY_HAS_DRIVER) */

/* Initialize TLS internals. */
static int tls_init(struct device *unused)
{
	ARG_UNUSED(unused);

	int ret;
	static const unsigned char drbg_seed[] = "zephyr";
	struct device *dev = NULL;

#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	dev = device_get_binding(CONFIG_ENTROPY_NAME);

	if (!dev) {
		NET_ERR("Failed to obtain entropy device");
		return -ENODEV;
	}
#else
	NET_WARN("No entropy device on the system, "
		 "TLS communication may be insecure!");
#endif /* defined(CONFIG_ENTROPY_HAS_DRIVER) */

	memset(tls_contexts, 0, sizeof(tls_contexts));

	k_mutex_init(&context_lock);

	mbedtls_ctr_drbg_init(&tls_ctr_drbg);

	ret = mbedtls_ctr_drbg_seed(&tls_ctr_drbg, tls_entropy_func, dev,
				    drbg_seed, sizeof(drbg_seed));
	if (ret != 0) {
		mbedtls_ctr_drbg_free(&tls_ctr_drbg);
		NET_ERR("TLS entropy source initialization failed");
		return -EFAULT;
	}

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_SOCKETS)
	mbedtls_debug_set_threshold(CONFIG_MBEDTLS_DEBUG_LEVEL);
#endif

	return 0;
}

SYS_INIT(tls_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Allocate TLS context. */
static struct tls_context *tls_alloc(void)
{
	int i;
	struct tls_context *tls = NULL;

	k_mutex_lock(&context_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(tls_contexts); i++) {
		if (!tls_contexts[i].is_used) {
			tls = &tls_contexts[i];
			memset(tls, 0, sizeof(*tls));
			tls->is_used = true;

			NET_DBG("Allocated TLS context, %p", tls);
			break;
		}
	}

	k_mutex_unlock(&context_lock);

	if (tls) {
		mbedtls_ssl_init(&tls->ssl);
		mbedtls_ssl_config_init(&tls->config);

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_SOCKETS)
		mbedtls_ssl_conf_dbg(&tls->config, tls_debug, NULL);
#endif
	} else {
		NET_WARN("Failed to allocate TLS context");
	}

	return tls;
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

	mbedtls_ssl_config_free(&tls->config);
	mbedtls_ssl_free(&tls->ssl);

	tls->is_used = false;

	return 0;
}

int ztls_socket(int family, int type, int proto)
{
	enum net_ip_protocol_secure tls_proto = 0;
	int sock, ret, err;

	if (proto  >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) {
		tls_proto = proto;
		proto = (type == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP;
	}

	sock = zsock_socket(family, type, proto);
	if (sock < 0) {
		/* errno will be propagated */
		return -1;
	}

	if (tls_proto != 0) {
		/* If TLS protocol is used, allocate TLS context */
		struct net_context *context = INT_TO_POINTER(sock);

		context->tls = tls_alloc();

		if (!context->tls) {
			ret = -ENOMEM;
			goto error;
		}

		context->tls->tls_version = tls_proto;
	}

	return sock;

error:
	err = zsock_close(sock);
	__ASSERT(err == 0, "Socket close failed");

	errno = -ret;
	return -1;
}

int ztls_close(int sock)
{
	struct net_context *context = INT_TO_POINTER(sock);
	int ret, err = 0;

	if (context->tls) {
		err = tls_release(context->tls);
	}

	ret = zsock_close(sock);

	/* In case zsock_close fails, we propagate errno value set by
	 * zsock_close.
	 * In case zsock_close succeeds, but tls_release fails, set errno
	 * according to tls_release return value.
	 */
	if (ret == 0 && err < 0) {
		errno = -err;
		ret = -1;
	}

	return ret;
}

int ztls_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_bind(sock, addr, addrlen);
}

int ztls_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

int ztls_listen(int sock, int backlog)
{
	return zsock_listen(sock, backlog);
}

int ztls_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_accept(sock, addr, addrlen);
}

ssize_t ztls_send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_send(sock, buf, len, flags);
}

ssize_t ztls_recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recv(sock, buf, max_len, flags);
}

ssize_t ztls_sendto(int sock, const void *buf, size_t len, int flags,
		    const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return zsock_sendto(sock, buf, len, flags, dest_addr, addrlen);
}

ssize_t ztls_recvfrom(int sock, void *buf, size_t max_len, int flags,
		      struct sockaddr *src_addr, socklen_t *addrlen)
{
	return zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
}

int ztls_fcntl(int sock, int cmd, int flags)
{
	return zsock_fcntl(sock, cmd, flags);
}

int ztls_poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	return zsock_poll(fds, nfds, timeout);
}
