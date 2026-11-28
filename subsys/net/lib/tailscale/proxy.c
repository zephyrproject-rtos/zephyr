/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <mbedtls/ssl.h>
#include <zephyr/kernel.h>
#include "tailscale_internal.h"

#include "proxy.h"

#define PROXY_HOST_MAX 253U
#define PROXY_HEADER_MAX 4096U

struct proxy_tls {
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt ca;
};

static int open_proxy(struct net_if *iface, uint16_t port)
{
	struct net_sockaddr_in target = {.sin_family = NET_AF_INET, .sin_port = net_htons(port)};
	struct net_ifreq name = {0};

	if (net_addr_pton(NET_AF_INET, CONFIG_TAILSCALE_PROXY_IPV4, &target.sin_addr) != 0 ||
	    net_ipv4_is_addr_unspecified(&target.sin_addr)) {
		return -EINVAL;
	}
	int rc = net_if_get_name(iface, name.ifr_name, sizeof(name.ifr_name));

	if (rc <= 0) {
		return rc < 0 ? rc : -ENODEV;
	}
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);

	if (fd < 0) {
		return -errno;
	}
	if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &name, sizeof(name)) <
		    0 ||
	    zsock_connect(fd, (const struct net_sockaddr *)&target, sizeof(target)) < 0) {
		rc = -errno;
		zsock_close(fd);
		return rc;
	}
	return fd;
}

static int transfer(struct control_io *io, void *buffer, size_t size, bool write)
{
	uint8_t *next = buffer;

	while (size != 0U) {
		int rc = control_io_socket_timeout(io, write ? ZSOCK_SO_SNDTIMEO :
							       ZSOCK_SO_RCVTIMEO);
		if (rc != 0) {
			return rc;
		}
		ssize_t n = write ? zsock_send(io->fd, next, size, 0)
				  : zsock_recv(io->fd, next, size, 0);

		if (n < 0 && (errno == EAGAIN || errno == ETIMEDOUT)) {
			continue;
		}
		if (n <= 0) {
			return n < 0 ? -errno : -ECONNRESET;
		}
		next += n;
		size -= n;
	}
	return 0;
}

static int http_line(struct control_io *io, size_t *budget, char prefix[13])
{
	size_t length = 0;
	bool colon = false;
	bool cr = false;

	while (*budget != 0U) {
		uint8_t byte;
		int rc = transfer(io, &byte, 1U, false);

		if (rc != 0) {
			return rc;
		}
		--*budget;
		if (cr) {
			return byte == '\n' && (prefix != NULL || length == 0U || colon) ?
				       (int)length : -EPROTO;
		}
		if (byte == '\r') {
			cr = true;
			continue;
		}
		if ((byte < ' ' && byte != '\t') || byte == 0x7f) {
			return -EPROTO;
		}
		if (prefix == NULL && !colon) {
			if (byte == ':' && length != 0U) {
				colon = true;
			} else if (!((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
				     (byte >= '0' && byte <= '9') ||
				     strchr("!#$%&'*+-.^_`|~", byte) != NULL)) {
				return -EPROTO;
			}
		}
		if (prefix != NULL && length < 13U) {
			prefix[length] = byte;
		}
		++length;
	}
	return -EMSGSIZE;
}

static int http_connect(struct control_io *io, const char *host, uint16_t port)
{
	if (host[0] == '\0' || strpbrk(host, " \t\r\n") != NULL) {
		return -EINVAL;
	}

	char authority[PROXY_HOST_MAX + sizeof("[]:65535")];
	char request[2U * sizeof(authority) + sizeof("CONNECT  HTTP/1.1\r\nHost: \r\n\r\n")];
	bool ipv6 = strchr(host, ':') != NULL;
	int length = snprintf(authority, sizeof(authority), "%s%s%s:%u", ipv6 ? "[" : "", host,
			      ipv6 ? "]" : "", (unsigned int)port);

	if (length < 0 || (size_t)length >= sizeof(authority)) {
		return -EINVAL;
	}
	length = snprintf(request, sizeof(request), "CONNECT %s HTTP/1.1\r\nHost: %s\r\n\r\n",
			  authority, authority);
	if (length < 0 || (size_t)length >= sizeof(request)) {
		return -EINVAL;
	}
	int rc = transfer(io, request, length, true);
	size_t budget = PROXY_HEADER_MAX;

	while (rc == 0) {
		char status[13];

		length = http_line(io, &budget, status);
		if (length < 0) {
			return length;
		}
		if (length < (int)sizeof(status) || memcmp(status, "HTTP/1.", 7) != 0 ||
		    (status[7] != '0' && status[7] != '1') || status[8] != ' ' ||
		    status[9] < '1' || status[9] > '5' || status[10] < '0' || status[10] > '9' ||
		    status[11] < '0' || status[11] > '9' || status[12] != ' ') {
			return -EPROTO;
		}
		do {
			rc = http_line(io, &budget, NULL);
		} while (rc > 0);
		if (rc != 0) {
			return rc;
		}
		if (status[9] == '1' && memcmp(status + 9, "101", 3) != 0) {
			continue;
		}
		return status[9] == '2' ? 0 :
		       memcmp(status + 9, "407", 3) == 0 ? -EACCES : -ECONNREFUSED;
	}
	return rc;
}

static int bio_send(void *context, const unsigned char *data, size_t size)
{
	struct control_io *io = context;
	int n = zsock_send(io->fd, data, size, 0);

	return n >= 0                                    ? n
	       : (errno == EAGAIN || errno == ETIMEDOUT) ? MBEDTLS_ERR_SSL_WANT_WRITE
							 : MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int bio_recv(void *context, unsigned char *data, size_t size)
{
	struct control_io *io = context;
	int n = zsock_recv(io->fd, data, size, 0);

	return n >= 0                                    ? n
	       : (errno == EAGAIN || errno == ETIMEDOUT) ? MBEDTLS_ERR_SSL_WANT_READ
							 : MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int load_authority(struct proxy_tls *tls, sec_tag_t tag)
{
	size_t len = 0;
	int rc = tls_credential_get(tag, TLS_CREDENTIAL_CA_CERTIFICATE, NULL, &len);

	if (rc != -EFBIG || len == 0 || len > 16384) {
		return -EACCES;
	}
	uint8_t *der = k_malloc(len);

	if (der == NULL) {
		return -ENOMEM;
	}
	rc = tls_credential_get(tag, TLS_CREDENTIAL_CA_CERTIFICATE, der, &len);
	if (rc == 0 && mbedtls_x509_crt_parse_der(&tls->ca, der, len) != 0) {
		rc = -EBADMSG;
	}
	k_free(der);
	return rc;
}

void tailscale_proxy_close(struct control_io *io)
{
	struct proxy_tls *tls = io->tls;

	if (tls != NULL) {
		mbedtls_ssl_free(&tls->ssl);
		mbedtls_ssl_config_free(&tls->conf);
		mbedtls_x509_crt_free(&tls->ca);
		k_free(tls);
		io->tls = NULL;
	}
	if (io->fd >= 0) {
		zsock_close(io->fd);
		io->fd = -1;
	}
}

int tailscale_proxy_tls(struct control_io *io, const char *host, uint16_t port,
			const char *tls_name, sec_tag_t ca, struct net_if *iface,
			int (*verify)(void *verify_ctx, mbedtls_x509_crt *cert, int depth,
				      uint32_t *flags),
			void *ctx)
{
	const int64_t deadline = io->deadline;
	int rc = open_proxy(iface, CONFIG_TAILSCALE_HTTP_PROXY_PORT);

	if (rc < 0) {
		return rc;
	}
	io->fd = rc;
	io->deadline = MIN(deadline, k_uptime_get() + TAILSCALE_PROXY_SETUP_MS);
	io->rx_pos = io->rx_len = 0;
	rc = http_connect(io, host, port);
	if (rc < 0) {
		goto fail;
	}
	struct proxy_tls *tls = k_calloc(1, sizeof(*tls));

	if (tls == NULL) {
		rc = -ENOMEM;
		goto fail;
	}
	io->tls = tls;
	mbedtls_ssl_init(&tls->ssl);
	mbedtls_ssl_config_init(&tls->conf);
	mbedtls_x509_crt_init(&tls->ca);
	rc = load_authority(tls, ca);
	if (rc != 0) {
		goto fail;
	}
	rc = mbedtls_ssl_config_defaults(&tls->conf, MBEDTLS_SSL_IS_CLIENT,
					 MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if (rc == 0) {
		mbedtls_ssl_conf_ca_chain(&tls->conf, &tls->ca, NULL);
		mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		if (verify) {
			mbedtls_ssl_conf_verify(&tls->conf, verify, ctx);
		}
		rc = mbedtls_ssl_setup(&tls->ssl, &tls->conf);
	}
	if (rc == 0) {
		rc = mbedtls_ssl_set_hostname(&tls->ssl, tls_name);
	}
	if (rc == 0) {
		mbedtls_ssl_set_bio(&tls->ssl, io, bio_send, bio_recv, NULL);
		do {
			rc = control_io_socket_timeout(io, ZSOCK_SO_SNDTIMEO);
			if (rc == 0) {
				rc = control_io_socket_timeout(io, ZSOCK_SO_RCVTIMEO);
			}
			if (rc != 0) {
				goto fail;
			}
			rc = mbedtls_ssl_handshake(&tls->ssl);
		} while (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE);
	}
	if (rc == 0 && mbedtls_ssl_get_verify_result(&tls->ssl) == 0) {
		io->deadline = deadline;
		return 0;
	}
	rc = -EACCES;
fail:
	io->deadline = deadline;
	tailscale_proxy_close(io);
	return rc;
}

static ssize_t tls_result(int n)
{
	if (n == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		return 0;
	}
	if (n < 0) {
		errno = n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE ? EAGAIN
											  : EIO;
		return -1;
	}
	return n;
}

ssize_t tailscale_proxy_read(struct control_io *io, void *buf, size_t size)
{
	return tls_result(mbedtls_ssl_read(&io->tls->ssl, buf, size));
}

ssize_t tailscale_proxy_write(struct control_io *io, const void *buf, size_t size)
{
	return tls_result(mbedtls_ssl_write(&io->tls->ssl, buf, size));
}

bool tailscale_proxy_pending(struct control_io *io)
{
	return io->tls != NULL && mbedtls_ssl_check_pending(&io->tls->ssl);
}

#ifdef CONFIG_TAILSCALE_SOCKS_UDP_PORT
int tailscale_proxy_associate(struct net_if *iface, struct net_sockaddr_in *relay)
{
	/* The wildcard endpoint requests an association for this TCP client. */
	uint8_t request[] = {5, 3, 0, 1, 0, 0, 0, 0, 0, 0};
	uint8_t method[] = {5, 1, 0};
	uint8_t reply[10];
	struct control_io io = {0};
	int rc = open_proxy(iface, CONFIG_TAILSCALE_SOCKS_UDP_PORT);

	if (rc < 0) {
		return rc;
	}
	io.fd = rc;
	io.deadline = k_uptime_get() + TAILSCALE_PROXY_SETUP_MS;
	rc = transfer(&io, method, sizeof(method), true);
	if (rc == 0) {
		rc = transfer(&io, reply, 2, false);
	}
	if (rc == 0 && memcmp(reply, (uint8_t[]){5, 0}, 2) != 0) {
		rc = -EACCES;
	}
	if (rc == 0) {
		rc = transfer(&io, request, sizeof(request), true);
	}
	if (rc == 0) {
		rc = transfer(&io, reply, 4, false);
	}
	if (rc == 0 && memcmp(reply, (uint8_t[]){5, 0, 0, 1}, 4) != 0) {
		rc = -EPROTO;
	}
	if (rc == 0) {
		rc = transfer(&io, reply + 4, 6, false);
	}
	if (rc == 0) {
		*relay = (struct net_sockaddr_in){.sin_family = NET_AF_INET};
		memcpy(&relay->sin_addr, reply + 4, 4);
		memcpy(&relay->sin_port, reply + 8, 2);
		struct net_in_addr proxy;

		net_addr_pton(NET_AF_INET, CONFIG_TAILSCALE_PROXY_IPV4, &proxy);
		if (net_ipv4_is_addr_unspecified(&relay->sin_addr)) {
			relay->sin_addr = proxy;
		}
		if (relay->sin_port == 0 || !net_ipv4_addr_cmp(&proxy, &relay->sin_addr)) {
			rc = -EACCES;
		}
	}
	if (rc != 0) {
		zsock_close(io.fd);
		return rc;
	}
	return io.fd;
}
#endif
