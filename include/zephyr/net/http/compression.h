/** @file
 * @brief HTTP compressions
 */

/*
 * Copyright (c) 2025 Endress+Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_COMPRESSION_H_
#define ZEPHYR_INCLUDE_NET_HTTP_COMPRESSION_H_

/**
 * @brief HTTP compressions
 * @defgroup http_compressions HTTP compressions
 * @since 4.2
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define HTTP_COMPRESSION_MAX_STRING_LEN 8

/** @brief HTTP compressions */
enum http_compression {
	HTTP_GZIP = 0,     /**< GZIP */
	HTTP_COMPRESS = 1, /**< COMPRESS */
	HTTP_DEFLATE = 2,  /**< DEFLATE */
	HTTP_BR = 3,       /**< BR */
	HTTP_ZSTD = 4      /**< ZSTD */
};

void http_compression_parse_accept_encoding(const char *accept_encoding, size_t len,
					    uint8_t *supported_compression);
const char *http_compression_text(enum http_compression compression);
int http_compression_from_text(enum http_compression compression[static 1], const char *text);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_HTTP_COMPRESSION_H_ */
