/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FLAT_BUFFER_UTIL_H_
#define ZEPHYR_INCLUDE_FLAT_BUFFER_UTIL_H_

#include <zephyr/types.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fbuf_ctx {
	u8_t *buf;
	u16_t buf_size;
	u16_t buf_len;
};

/* init */
static inline int fbuf_init(struct fbuf_ctx *ctx,
			    u8_t *buf, u16_t buf_size, u16_t buf_len)
{
	if (!ctx || !buf) {
		return -EINVAL;
	}

	ctx->buf = buf;
	ctx->buf_size = buf_size;
	ctx->buf_len = buf_len;
	return 0;
}

/* append */
static inline int fbuf_append(struct fbuf_ctx *ctx, u8_t *src, u16_t len)
{
	if (!ctx || !ctx->buf || !src) {
		return -EINVAL;
	}

	if (ctx->buf_len + len > ctx->buf_size) {
		return -ENOMEM;
	}

	memcpy(ctx->buf + ctx->buf_len, src, len);
	ctx->buf_len += len;
	return 0;
}

/* insert */
static inline int fbuf_insert(struct fbuf_ctx *ctx, u16_t offset,
			      u8_t *src, u16_t len)
{
	if (!ctx || !ctx->buf || !src) {
		return -EINVAL;
	}

	if (ctx->buf_len + len > ctx->buf_size) {
		return -ENOMEM;
	}

	/* shift everything in fbuf after offset by len */
	memmove(ctx->buf + offset + len, ctx->buf + offset,
		ctx->buf_len - offset);

	/* copy src into fbuf at offset */
	memcpy(ctx->buf + offset, src, len);
	ctx->buf_len += len;
	return 0;
}

/* read */
static inline int fbuf_read(struct fbuf_ctx *ctx, u16_t *offset,
			    u8_t *dst, u16_t len)
{
	if (!ctx || !ctx->buf) {
		return -EINVAL;
	}

	if (*offset + len > ctx->buf_len) {
		return -ENOMEM;
	}

	if (dst) {
		/* copy fbuf at offset into dst */
		memcpy(dst, ctx->buf + *offset, len);
	}

	*offset += len;
	return 0;
}

static inline int fbuf_skip(struct fbuf_ctx *ctx, u16_t *offset, u16_t len)
{
	return fbuf_read(ctx, offset, NULL, len);
}

static inline int fbuf_read_u8(struct fbuf_ctx *ctx, u16_t *offset, u8_t *value)
{
	return fbuf_read(ctx, offset, value, sizeof(u8_t));
}

static inline int fbuf_read_u16(struct fbuf_ctx *ctx, u16_t *offset,
				u16_t *value)
{
	return fbuf_read(ctx, offset, (u8_t *)value, sizeof(u16_t));
}

static inline int fbuf_read_be16(struct fbuf_ctx *ctx, u16_t *offset,
				 u16_t *value)
{
	int ret;
	u8_t v16[2];

	ret = fbuf_read(ctx, offset, (u8_t *)v16, sizeof(u16_t));
	*value = v16[0] << 8 | v16[1];

	return ret;
}

static inline int fbuf_read_be32(struct fbuf_ctx *ctx, u16_t *offset,
				 u32_t *value)
{
	int ret;
	u8_t v32[4];

	ret = fbuf_read(ctx, offset, (u8_t *)v32, sizeof(u32_t));
	*value = v32[0] << 24 | v32[1] << 16 | v32[2] << 8 | v32[3];

	return ret;
}

static inline int fbuf_read_u32(struct fbuf_ctx *ctx, u16_t *offset,
				u32_t *value)
{
	return fbuf_read(ctx, offset, (u8_t *)value, sizeof(u32_t));
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FLAT_BUFFER_UTIL_H_ */
