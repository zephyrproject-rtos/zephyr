/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BUF_UTIL_H_
#define ZEPHYR_INCLUDE_BUF_UTIL_H_

#include <zephyr/types.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* append */
static inline int buf_append(u8_t *dst, u16_t *dst_len, u16_t dst_size,
			     u8_t *src, u16_t src_len)
{
	if (!dst || !src) {
		return -EINVAL;
	}

	if (*dst_len + src_len > dst_size) {
		return -ENOMEM;
	}

	memcpy(dst + *dst_len, src, src_len);
	*dst_len += src_len;
	return 0;
}

/* insert */
static inline int buf_insert(u8_t *dst, u16_t *dst_len, u16_t dst_size,
			     u16_t offset, u8_t *src, u16_t src_len)
{
	if (!dst || !src) {
		return -EINVAL;
	}

	if (*dst_len + src_len > dst_size) {
		return -ENOMEM;
	}

	/* shift everything in fbuf after offset by len */
	memmove(dst + offset + src_len, dst + offset, *dst_len - offset);

	/* copy src into fbuf at offset */
	memcpy(dst + offset, src, src_len);
	*dst_len += src_len;
	return 0;
}

/* read */
static inline int buf_read(u8_t *dst, u16_t len, u8_t *src, u16_t src_len,
			   u16_t *offset)
{
	if (!src) {
		return -EINVAL;
	}

	if (*offset + len > src_len) {
		return -ENOMEM;
	}

	if (dst) {
		/* copy data at offset into dst */
		memcpy(dst, src + *offset, len);
	}

	*offset += len;
	return 0;
}

static inline int buf_skip(u16_t len, u8_t *src, u16_t src_len, u16_t *offset)
{
	return buf_read(NULL, len, src, src_len, offset);
}

static inline int buf_read_u8(u8_t *value, u8_t *src, u16_t src_len,
			      u16_t *offset)
{
	return buf_read(value, sizeof(u8_t), src, src_len, offset);
}

static inline int buf_read_u16(u16_t *value, u8_t *src, u16_t src_len,
			       u16_t *offset)
{
	return buf_read((u8_t *)value, sizeof(u16_t), src, src_len, offset);
}

static inline int buf_read_be16(u16_t *value, u8_t *src, u16_t src_len,
				u16_t *offset)
{
	int ret;
	u8_t v16[2];

	ret = buf_read(v16, sizeof(u16_t), src, src_len, offset);
	*value = v16[0] << 8 | v16[1];

	return ret;
}

static inline int buf_read_u32(u32_t *value, u8_t *src, u16_t src_len,
			       u16_t *offset)
{
	return buf_read((u8_t *)value, sizeof(u32_t), src, src_len, offset);
}

static inline int buf_read_be32(u32_t *value, u8_t *src, u16_t src_len,
				u16_t *offset)
{
	int ret;
	u8_t v32[4];

	ret = buf_read(v32, sizeof(u32_t), src, src_len, offset);
	*value = v32[0] << 24 | v32[1] << 16 | v32[2] << 8 | v32[3];

	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BUF_UTIL_H_ */
