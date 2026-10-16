/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hex encode/decode helpers for the dual_board_loopback sync protocol.
 *
 * The protocol carries binary payloads as lowercase hex with no
 * separator, so both boards need the same conversion.  Header-only and
 * dependency-free, matching sync_proto.h.
 */

#ifndef I3C_DUAL_BOARD_LOOPBACK_COMMON_HEX_UTIL_H_
#define I3C_DUAL_BOARD_LOOPBACK_COMMON_HEX_UTIL_H_

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int hex_nibble_val(char c)
{
	if ((c >= '0') && (c <= '9')) {
		return c - '0';
	}
	if ((c >= 'a') && (c <= 'f')) {
		return 10 + (c - 'a');
	}
	if ((c >= 'A') && (c <= 'F')) {
		return 10 + (c - 'A');
	}
	return -1;
}

/**
 * @brief Decode a hex string into bytes.
 *
 * @retval n       Number of bytes decoded.
 * @retval -EINVAL Odd length, or a non-hex character.
 */
static inline int hex_to_bytes(const char *s, uint8_t *out, size_t out_max)
{
	const size_t len = strlen(s);
	size_t n = 0;

	if ((len % 2U) != 0U) {
		return -EINVAL;
	}
	for (size_t i = 0; (i < len) && (n < out_max); i += 2U) {
		int hi = hex_nibble_val(s[i]);
		int lo = hex_nibble_val(s[i + 1U]);

		if ((hi < 0) || (lo < 0)) {
			return -EINVAL;
		}
		out[n] = (uint8_t)((hi << 4) | lo);
		n++;
	}
	return (int)n;
}

/**
 * @brief Encode bytes as a NUL-terminated lowercase hex string.
 *
 * @retval 0       Success.
 * @retval -ENOSPC @p out_max is too small for @p len bytes plus the NUL.
 */
static inline int bytes_to_hex(const uint8_t *buf, size_t len, char *out, size_t out_max)
{
	static const char digits[] = "0123456789abcdef";

	if (out_max < ((len * 2U) + 1U)) {
		return -ENOSPC;
	}
	for (size_t i = 0; i < len; i++) {
		out[i * 2U] = digits[buf[i] >> 4];
		out[(i * 2U) + 1U] = digits[buf[i] & 0x0FU];
	}
	out[len * 2U] = '\0';
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* I3C_DUAL_BOARD_LOOPBACK_COMMON_HEX_UTIL_H_ */
