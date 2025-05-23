/** @file
 * @brief HTTP compression handling functions
 *
 * Helper functions to handle compression formats
 */

/*
 * Copyright (c) 2025 Endress+Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include <zephyr/net/http/server.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "headers/server_internal.h"

static int http_compression_match(enum http_compression *compression, const char *text,
				  enum http_compression expected);

void http_compression_parse_accept_encoding(const char *accept_encoding, size_t len,
					    uint8_t *supported_compression)
{
	enum http_compression detected_compression;
	char strbuf[HTTP_COMPRESSION_MAX_STRING_LEN + 1] = {0};
	const char *start = accept_encoding;
	const char *end = NULL;
	bool priority_string = false;

	*supported_compression = HTTP_NONE;
	for (size_t i = 0; i < len; i++) {
		if (accept_encoding[i] == 0) {
			break;
		} else if (accept_encoding[i] == ' ') {
			start = &accept_encoding[i + 1];
			continue;
		} else if (accept_encoding[i] == ';') {
			end = &accept_encoding[i];
			priority_string = true;
		} else if (accept_encoding[i] == ',') {
			if (!priority_string) {
				end = &accept_encoding[i];
			}
			priority_string = false;
		} else if (i + 1 == len) {
			end = &accept_encoding[i + 1];
		}

		if (end == NULL) {
			continue;
		}

		if (end - start > HTTP_COMPRESSION_MAX_STRING_LEN) {
			end = NULL;
			start = end + 1;
			continue;
		}

		memcpy(strbuf, start, end - start);
		strbuf[end - start] = 0;

		if (http_compression_from_text(&detected_compression, strbuf) == 0) {
			WRITE_BIT(*supported_compression, detected_compression, true);
		}

		end = NULL;
		start = end + 1;
	}
}

const char *http_compression_text(enum http_compression compression)
{
	switch (compression) {
	case HTTP_NONE:
		return "";
	case HTTP_GZIP:
		return "gzip";
	case HTTP_COMPRESS:
		return "compress";
	case HTTP_DEFLATE:
		return "deflate";
	case HTTP_BR:
		return "br";
	case HTTP_ZSTD:
		return "zstd";
	}
	return "";
}

int http_compression_from_text(enum http_compression *compression, const char *text)
{
	__ASSERT_NO_MSG(compression);
	__ASSERT_NO_MSG(text);

	for (enum http_compression i = 0; compression_value_is_valid(i); ++i) {
		if (http_compression_match(compression, text, i) == 0) {
			return 0;
		}
	}
	return 1;
}

bool compression_value_is_valid(enum http_compression compression)
{
	switch (compression) {
	case HTTP_NONE:
	case HTTP_GZIP:
	case HTTP_COMPRESS:
	case HTTP_DEFLATE:
	case HTTP_BR:
	case HTTP_ZSTD:
		return true;
	}
	return false;
}

static int http_compression_match(enum http_compression *compression, const char *text,
				  enum http_compression expected)
{
	__ASSERT_NO_MSG(compression);
	__ASSERT_NO_MSG(text);

	if (strcasecmp(http_compression_text(expected), text) == 0) {
		*compression = expected;
		return 0;
	} else {
		return 1;
	}
}
