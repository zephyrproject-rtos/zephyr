/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_NET_TAILSCALE_PROXY_H_
#define ZEPHYR_NET_TAILSCALE_PROXY_H_

#include <mbedtls/x509_crt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include "control_internal.h"

#define TAILSCALE_PROXY_SETUP_MS 8000U

int tailscale_proxy_tls(struct control_io *io, const char *host, uint16_t port,
			const char *tls_name, sec_tag_t ca, struct net_if *iface,
			int (*verify)(void *verify_ctx, mbedtls_x509_crt *cert, int depth,
				      uint32_t *flags),
			void *ctx);
ssize_t tailscale_proxy_read(struct control_io *io, void *buf, size_t size);
ssize_t tailscale_proxy_write(struct control_io *io, const void *buf, size_t size);
bool tailscale_proxy_pending(struct control_io *io);
void tailscale_proxy_close(struct control_io *io);
int tailscale_proxy_associate(struct net_if *iface, struct net_sockaddr_in *relay);
#endif
