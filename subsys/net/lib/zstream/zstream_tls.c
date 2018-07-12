/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#include "mbedtls/platform.h"

#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>

#ifdef __ZEPHYR__
#include <net/socket.h>
#else /* __ZEPHYR__ */
#include <unistd.h>
#include <sys/socket.h>
#define NET_ERR(msg, ...) printf(msg "\n", ##__VA_ARGS__)

#endif /* __ZEPHYR__ */

#include <net/zstream_tls.h>

static ssize_t zstream_tls_read(struct zstream *stream, void *buf, size_t size)
{
	struct zstream_tls *self = (struct zstream_tls *)stream;
	int ret = mbedtls_ssl_read(&self->ssl, buf, size);

	if (ret >= 0) {
		return ret;
	}

	if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		/* End of stream */
		return 0;
	}

	if (ret == MBEDTLS_ERR_SSL_CLIENT_RECONNECT) {
		/* We don't support TLS reconnects over the same socket,
		 * treat as EOF.
		 */
		return 0;
	}

	if (ret == MBEDTLS_ERR_SSL_WANT_READ
	    || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		errno = EAGAIN;
		return -1;
	}

	NET_ERR("mbedtls_ssl_read: -%x\n", -ret);
	/* TODO: More error code conversion? */
	errno = EINVAL;
	return -1;
}

static ssize_t zstream_tls_write(struct zstream *stream, const void *buf,
				 size_t size)
{
	struct zstream_tls *self = (struct zstream_tls *)stream;
	int ret = mbedtls_ssl_write(&self->ssl, buf, size);

	if (ret >= 0) {
		return ret;
	}

	if (ret == MBEDTLS_ERR_SSL_WANT_READ
	    || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		errno = EAGAIN;
		return -1;
	}

	/* TODO: More error code conversion? */
	errno = EINVAL;
	return -1;
}

static int zstream_tls_flush(struct zstream *stream)
{
	/* mbedTLS doesn't buffer output data, even for a single
	 * byte written it will create a TLS record and send at once.
	 */
	return 0;
}

int zstream_tls_close(struct zstream *stream)
{
	struct zstream_tls *self = (struct zstream_tls *)stream;
	int tls_res = mbedtls_ssl_close_notify(&self->ssl);
	/* We need to close underlying stream regardless of TLS close
	 * notify status, or the stream will be leaked.
	 */
	int stream_res = zstream_close(self->sock);

	mbedtls_ssl_free(&self->ssl);

	if (tls_res == 0 && stream_res == 0) {
		return 0;
	}

	if (stream_res == 0) {
		/* TODO: Check which mbedTLS errors are possible and improve
		 * mapping to POSIX codes.
		 */
		errno = EINVAL;
	}

	return -1;
}

static const struct zstream_api zstream_tls_api = {
	.read = zstream_tls_read,
	.write = zstream_tls_write,
	.flush = zstream_tls_flush,
	.close = zstream_tls_close,
};

static int zstream_mbedtls_ssl_send(void *ctx, const unsigned char *buf,
				    size_t len)
{
	struct zstream *sock = ctx;
	ssize_t outlen = zstream_write(sock, buf, len);

	if (outlen != -1) {
		return outlen;
	}

	if (errno == EAGAIN) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}

	/* Generic error */
	return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int zstream_mbedtls_ssl_recv(void *ctx, unsigned char *buf,
				    size_t len)
{
	struct zstream *sock = ctx;
	ssize_t outlen = zstream_read(sock, buf, len);

	if (outlen != -1) {
		return outlen;
	}

	if (errno == EAGAIN) {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}

	/* Generic error */
	return MBEDTLS_ERR_NET_RECV_FAILED;
}

int zstream_tls_init(struct zstream_tls *self, struct zstream *sock,
		     mbedtls_ssl_config *conf, const char *hostname)
{
	int ret;

	self->api = &zstream_tls_api;
	self->sock = sock;

	mbedtls_ssl_init(&self->ssl);

	ret = mbedtls_ssl_setup(&self->ssl, conf);
	if (ret != 0) {
		goto error;
	}

	if (hostname) {
		/* Set server hostname for SNI */
		ret = mbedtls_ssl_set_hostname(&self->ssl, hostname);
		if (ret != 0) {
			NET_ERR("mbedtls_ssl_set_hostname: -0x%x", -ret);
			goto error;
		}
	}

	mbedtls_ssl_set_bio(&self->ssl, sock, zstream_mbedtls_ssl_send,
			    zstream_mbedtls_ssl_recv, NULL);

	while ((ret = mbedtls_ssl_handshake(&self->ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ
		    && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			NET_ERR("mbedtls_ssl_handshake: -%x\n", -ret);
			goto error;
		}
	}

	return 0;

error:
	return -1;
}
