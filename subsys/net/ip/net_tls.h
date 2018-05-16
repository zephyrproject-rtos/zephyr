/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_TLS_H
#define __NET_TLS_H

#include <net/net_context.h>

#if defined(CONFIG_NET_TLS) || defined(CONFIG_NET_DTLS)

#if defined(CONFIG_MBEDTLS)

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_time_t       time_t
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#include <mbedtls/ssl_cookie.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

#endif /* CONFIG_MBEDTLS */

int net_tls_enable(struct net_context *context, bool enabled);
int net_tls_connect(struct net_context *context, bool listening);
int net_tls_send(struct net_pkt *pkt);
int net_tls_recv(struct net_context *context, net_context_recv_cb_t cb,
		 void *user_data);

#else
static inline int net_tls_enable(struct net_context *context, bool enabled)
{
	ARG_UNUSED(context);
	ARG_UNUSED(enabled);

	return 0;
}

static inline int net_tls_connect(struct net_context *context, bool listening)
{
	ARG_UNUSED(context);
	ARG_UNUSED(listening);

	return 0;
}

static inline int net_tls_send(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline int net_tls_recv(struct net_context *context,
			       net_context_recv_cb_t cb, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return 0;
}
#endif /* CONFIG_NET_TLS || CONFIG_NET_DTLS */

#endif
