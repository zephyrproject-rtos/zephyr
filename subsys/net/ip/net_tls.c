/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "net_tls.h"

static int tls_tx(void *ctx, const unsigned char *buf, size_t len)
{
	struct net_context *context = ctx;

	ARG_UNUSED(context);

	return -EOPNOTSUPP;
}

static int tls_rx(void *ctx, unsigned char *buf, size_t len)
{
	struct net_context *context = ctx;

	ARG_UNUSED(context);

	return -EOPNOTSUPP;
}

int net_tls_enable(struct net_context *context, bool enabled)
{
	int state;

	if (!context) {
		return -EINVAL;
	}

	if (context->options.tls == enabled) {
		return 0;
	}

	state = net_context_get_state(context);
	if (state != NET_CONTEXT_IDLE && state != NET_CONTEXT_UNCONNECTED) {
		return -EBUSY;
	}

	if (enabled) {
		k_fifo_init(&context->mbedtls.rx_fifo);
		mbedtls_ssl_init(&context->mbedtls.ssl);
		mbedtls_ssl_config_init(&context->mbedtls.config);
		mbedtls_ssl_set_bio(&context->mbedtls.ssl, context,
				    tls_tx, tls_rx, NULL);
	} else {
		mbedtls_ssl_set_bio(&context->mbedtls.ssl,
				    NULL, NULL, NULL, NULL);
		mbedtls_ssl_config_free(&context->mbedtls.config);
		mbedtls_ssl_free(&context->mbedtls.ssl);
	}

	context->options.tls = enabled;

	return 0;
}
