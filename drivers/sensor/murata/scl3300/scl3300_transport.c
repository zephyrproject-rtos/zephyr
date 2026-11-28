/*
 * Copyright (c) 2026 Sacra Systems Private Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "scl3300_transport.h"

int scl3300_transport_read(const scl3300_ctx_t *ctx, uint32_t command, uint8_t *rx_buf)
{
	return scl3300_read_command(ctx, command, rx_buf);
}
int scl3300_transport_write(const scl3300_ctx_t *ctx, uint32_t command)
{
	return scl3300_write_command(ctx, command);
}

int32_t __weak scl3300_write_command(const scl3300_ctx_t *ctx, uint32_t command)
{
	int32_t ret;

	if (ctx == NULL || ctx->write == NULL) {
		return -EINVAL;
	}

	ret = ctx->write(ctx->handle, command);

	return ret;
}

int32_t __weak scl3300_read_command(const scl3300_ctx_t *ctx, uint32_t command, uint8_t *rx_buf)
{
	int32_t ret;

	if (ctx == NULL || ctx->read == NULL) {
		return -EINVAL;
	}

	ret = ctx->read(ctx->handle, command, rx_buf);

	return ret;
}
