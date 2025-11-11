/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/net_core.h>

LOG_MODULE_DECLARE(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

static inline bool http_hpack_key_is_static(uint32_t key)
{
	return key > HTTP_SERVER_HPACK_INVALID && key <= HTTP_SERVER_HPACK_WWW_AUTHENTICATE;
}

struct hpack_table_entry {
	const char *name;
	const char *value;
};

static const struct hpack_table_entry http_hpack_table_static[] = {
	[HTTP_SERVER_HPACK_AUTHORITY] = { ":authority", NULL },
	[HTTP_SERVER_HPACK_METHOD_GET] = { ":method", "GET" },
	[HTTP_SERVER_HPACK_METHOD_POST] = { ":method", "POST" },
	[HTTP_SERVER_HPACK_PATH_ROOT] = { ":path", "/" },
	[HTTP_SERVER_HPACK_PATH_INDEX] = { ":path", "/index.html" },
	[HTTP_SERVER_HPACK_SCHEME_HTTP] = { ":scheme", "http" },
	[HTTP_SERVER_HPACK_SCHEME_HTTPS] = { ":scheme", "https" },
	[HTTP_SERVER_HPACK_STATUS_200] = { ":status", "200" },
	[HTTP_SERVER_HPACK_STATUS_204] = { ":status", "204" },
	[HTTP_SERVER_HPACK_STATUS_206] = { ":status", "206" },
	[HTTP_SERVER_HPACK_STATUS_304] = { ":status", "304" },
	[HTTP_SERVER_HPACK_STATUS_400] = { ":status", "400" },
	[HTTP_SERVER_HPACK_STATUS_404] = { ":status", "404" },
	[HTTP_SERVER_HPACK_STATUS_500] = { ":status", "500" },
	[HTTP_SERVER_HPACK_ACCEPT_CHARSET] = { "accept-charset", NULL },
	[HTTP_SERVER_HPACK_ACCEPT_ENCODING] = { "accept-encoding", "gzip, deflate" },
	[HTTP_SERVER_HPACK_ACCEPT_LANGUAGE] = { "accept-language", NULL },
	[HTTP_SERVER_HPACK_ACCEPT_RANGES] = { "accept-ranges", NULL },
	[HTTP_SERVER_HPACK_ACCEPT] = { "accept", NULL },
	[HTTP_SERVER_HPACK_ACCESS_CONTROL_ALLOW_ORIGIN] = { "access-control-allow-origin", NULL },
	[HTTP_SERVER_HPACK_AGE] = { "age", NULL },
	[HTTP_SERVER_HPACK_ALLOW] = { "allow", NULL },
	[HTTP_SERVER_HPACK_AUTHORIZATION] = { "authorization", NULL },
	[HTTP_SERVER_HPACK_CACHE_CONTROL] = { "cache-control", NULL },
	[HTTP_SERVER_HPACK_CONTENT_DISPOSITION] = { "content-disposition", NULL },
	[HTTP_SERVER_HPACK_CONTENT_ENCODING] = { "content-encoding", NULL },
	[HTTP_SERVER_HPACK_CONTENT_LANGUAGE] = { "content-language", NULL },
	[HTTP_SERVER_HPACK_CONTENT_LENGTH] = { "content-length", NULL },
	[HTTP_SERVER_HPACK_CONTENT_LOCATION] = { "content-location", NULL },
	[HTTP_SERVER_HPACK_CONTENT_RANGE] = { "content-range", NULL },
	[HTTP_SERVER_HPACK_CONTENT_TYPE] = { "content-type", NULL },
	[HTTP_SERVER_HPACK_COOKIE] = { "cookie", NULL },
	[HTTP_SERVER_HPACK_DATE] = { "date", NULL },
	[HTTP_SERVER_HPACK_ETAG] = { "etag", NULL },
	[HTTP_SERVER_HPACK_EXPECT] = { "expect", NULL },
	[HTTP_SERVER_HPACK_EXPIRES] = { "expires", NULL },
	[HTTP_SERVER_HPACK_FROM] = { "from", NULL },
	[HTTP_SERVER_HPACK_HOST] = { "host", NULL },
	[HTTP_SERVER_HPACK_IF_MATCH] = { "if-match", NULL },
	[HTTP_SERVER_HPACK_IF_MODIFIED_SINCE] = { "if-modified-since", NULL },
	[HTTP_SERVER_HPACK_IF_NONE_MATCH] = { "if-none-match", NULL },
	[HTTP_SERVER_HPACK_IF_RANGE] = { "if-range", NULL },
	[HTTP_SERVER_HPACK_IF_UNMODIFIED_SINCE] = { "if-unmodified-since", NULL },
	[HTTP_SERVER_HPACK_LAST_MODIFIED] = { "last-modified", NULL },
	[HTTP_SERVER_HPACK_LINK] = { "link", NULL },
	[HTTP_SERVER_HPACK_LOCATION] = { "location", NULL },
	[HTTP_SERVER_HPACK_MAX_FORWARDS] = { "max-forwards", NULL },
	[HTTP_SERVER_HPACK_PROXY_AUTHENTICATE] = { "proxy-authenticate", NULL },
	[HTTP_SERVER_HPACK_PROXY_AUTHORIZATION] = { "proxy-authorization", NULL },
	[HTTP_SERVER_HPACK_RANGE] = { "range", NULL },
	[HTTP_SERVER_HPACK_REFERER] = { "referer", NULL },
	[HTTP_SERVER_HPACK_REFRESH] = { "refresh", NULL },
	[HTTP_SERVER_HPACK_RETRY_AFTER] = { "retry-after", NULL },
	[HTTP_SERVER_HPACK_SERVER] = { "server", NULL },
	[HTTP_SERVER_HPACK_SET_COOKIE] = { "set-cookie", NULL },
	[HTTP_SERVER_HPACK_STRICT_TRANSPORT_SECURITY] = { "strict-transport-security", NULL },
	[HTTP_SERVER_HPACK_TRANSFER_ENCODING] = { "transfer-encoding", NULL },
	[HTTP_SERVER_HPACK_USER_AGENT] = { "user-agent", NULL },
	[HTTP_SERVER_HPACK_VARY] = { "vary", NULL },
	[HTTP_SERVER_HPACK_VIA] = { "via", NULL },
	[HTTP_SERVER_HPACK_WWW_AUTHENTICATE] = { "www-authenticate", NULL },
};

const struct hpack_table_entry *http_hpack_table_get(uint32_t key)
{
	if (!http_hpack_key_is_static(key)) {
		return NULL;
	}

	return &http_hpack_table_static[key];
}

static int http_hpack_find_index(struct http_hpack_header_buf *header,
				 bool *name_only)
{
	const struct hpack_table_entry *entry;
	int candidate = -1;

	for (int i = HTTP_SERVER_HPACK_AUTHORITY;
	     i <= HTTP_SERVER_HPACK_WWW_AUTHENTICATE; i++) {
		entry = &http_hpack_table_static[i];

		if (entry->name != NULL &&
		    strlen(entry->name) == header->name_len &&
		    memcmp(entry->name, header->name, header->name_len) == 0) {
			if (entry->value != NULL &&
			    strlen(entry->value) == header->value_len &&
			    memcmp(entry->value, header->value, header->value_len) == 0) {
				/* Got exact match. */
				*name_only = false;
				return i;
			}

			if (candidate < 0) {
				candidate = i;
			}
		}
	}

	if (candidate > 0) {
		/* Matched name only. */
		*name_only = true;
		return candidate;
	}

	return -ENOENT;
}

#define HPACK_INTEGER_CONTINUATION_FLAG            0x80
#define HPACK_STRING_HUFFMAN_FLAG                  0x80
#define HPACK_STRING_PREFIX_LEN                    7

#define HPACK_PREFIX_INDEXED_MASK                  0x80
#define HPACK_PREFIX_INDEXED                       0x80
#define HPACK_PREFIX_LEN_INDEXED                   7

#define HPACK_PREFIX_LITERAL_INDEXING_MASK         0xC0
#define HPACK_PREFIX_LITERAL_INDEXING              0x40
#define HPACK_PREFIX_LEN_LITERAL_INDEXING          6

#define HPACK_PREFIX_LITERAL_NO_INDEXING_MASK      0xF0
#define HPACK_PREFIX_LITERAL_NO_INDEXING           0x00
#define HPACK_PREFIX_LEN_LITERAL_NO_INDEXING       4

#define HPACK_PREFIX_LITERAL_NEVER_INDEXED_MASK    0xF0
#define HPACK_PREFIX_LITERAL_NEVER_INDEXED         0x10
#define HPACK_PREFIX_LEN_LITERAL_NEVER_INDEXED     4

#define HPACK_PREFIX_DYNAMIC_TABLE_SIZE_MASK       0xE0
#define HPACK_PREFIX_DYNAMIC_TABLE_SIZE_UPDATE     0x20
#define HPACK_PREFIX_LEN_DYNAMIC_TABLE_SIZE_UPDATE 5

static int hpack_integer_decode(const uint8_t *buf, size_t datalen,
				uint8_t n, uint32_t *value)
{
	int len = 0;
	uint8_t m = 0;
	uint8_t value_mask = (1 << n) - 1;

	NET_ASSERT(n < 8);

	if (datalen == 0) {
		return -EAGAIN;
	}

	/* Based on RFC7541, ch 5.1. */
	len++;
	*value = *buf & value_mask;
	if (*value < value_mask) {
		return len;
	}

	do {
		buf++;
		len++;

		if (--datalen == 0) {
			return -EAGAIN;
		}

		if (m > sizeof(uint32_t) * 8) {
			/* Can't handle integer that large. */
			return -EBADMSG;
		}

		*value += (*buf & ~HPACK_INTEGER_CONTINUATION_FLAG) * (1 << m);
		m += 7;

	} while (*buf & HPACK_INTEGER_CONTINUATION_FLAG);

	return len;
}

enum hpack_string_type {
	HPACK_HEADER_NAME,
	HPACK_HEADER_VALUE,
};

static int hpack_huffman_decode(const uint8_t *encoded_buf, size_t encoded_len,
				enum hpack_string_type type,
				struct http_hpack_header_buf *header)
{
	uint8_t *buf = header->buf + header->datalen;
	size_t buflen = sizeof(header->buf) - header->datalen;
	int ret;

	NET_ASSERT(type == HPACK_HEADER_NAME || type == HPACK_HEADER_VALUE);

	ret = http_hpack_huffman_decode(encoded_buf, encoded_len, buf, buflen);
	if (ret < 0) {
		return ret;
	}

	if (type == HPACK_HEADER_NAME) {
		header->name = buf;
		header->name_len = ret;
	} else if (type == HPACK_HEADER_VALUE) {
		header->value = buf;
		header->value_len = ret;
	}

	header->datalen += ret;

	return 0;
}

static int hpack_string_decode(const uint8_t *buf, size_t datalen,
			       enum hpack_string_type type,
			       struct http_hpack_header_buf *header)
{
	uint32_t str_len;
	bool huffman;
	int len = 0;
	int ret;

	NET_ASSERT(type == HPACK_HEADER_NAME || type == HPACK_HEADER_VALUE);

	if (datalen == 0) {
		return -EAGAIN;
	}

	huffman = *buf & HPACK_STRING_HUFFMAN_FLAG;

	ret = hpack_integer_decode(buf, datalen, HPACK_STRING_PREFIX_LEN,
				   &str_len);
	if (ret < 0) {
		return ret;
	}

	len += ret;
	datalen -= ret;
	buf += ret;

	if (str_len > datalen) {
		return -EAGAIN;
	}

	if (huffman) {
		ret = hpack_huffman_decode(buf, str_len, type, header);
		if (ret < 0) {
			return ret;
		}
	} else {
		if (type == HPACK_HEADER_NAME) {
			header->name = buf;
			header->name_len = str_len;
		} else if (type == HPACK_HEADER_VALUE) {
			header->value = buf;
			header->value_len = str_len;
		}
	}

	len += str_len;

	return len;
}

static int hpack_handle_indexed(const uint8_t *buf, size_t datalen,
				struct http_hpack_header_buf *header)
{
	const struct hpack_table_entry *entry;
	uint32_t index;
	int ret;

	ret = hpack_integer_decode(buf, datalen, HPACK_PREFIX_LEN_INDEXED,
				   &index);
	if (ret < 0) {
		return ret;
	}

	if (index == 0) {
		return -EBADMSG;
	}

	entry = http_hpack_table_get(index);
	if (entry == NULL) {
		return -EBADMSG;
	}

	if (entry->name == NULL || entry->value == NULL) {
		return -EBADMSG;
	}

	header->name = entry->name;
	header->name_len = strlen(entry->name);
	header->value = entry->value;
	header->value_len = strlen(entry->value);

	return ret;
}

static int hpack_handle_literal(const uint8_t *buf, size_t datalen,
				struct http_hpack_header_buf *header,
				uint8_t prefix_len)
{
	uint32_t index;
	int ret, len;

	header->datalen = 0;

	ret = hpack_integer_decode(buf, datalen, prefix_len, &index);
	if (ret < 0) {
		return ret;
	}

	len = ret;
	buf += ret;
	datalen -= ret;

	if (index == 0) {
		/* Literal name. */
		ret = hpack_string_decode(buf, datalen, HPACK_HEADER_NAME,
					  header);
		if (ret < 0) {
			return ret;
		}

		len += ret;
		buf += ret;
		datalen -= ret;
	} else {
		/* Indexed name. */
		const struct hpack_table_entry *entry;

		entry = http_hpack_table_get(index);
		if (entry == NULL) {
			return -EBADMSG;
		}

		if (entry->name == NULL) {
			return -EBADMSG;
		}

		header->name = entry->name;
		header->name_len = strlen(entry->name);
	}

	ret = hpack_string_decode(buf, datalen, HPACK_HEADER_VALUE, header);
	if (ret < 0) {
		return ret;
	}

	len += ret;

	return len;
}

static int hpack_handle_literal_index(const uint8_t *buf, size_t datalen,
			       struct http_hpack_header_buf *header)
{
	/* TODO Add dynamic table support, if needed. */

	return hpack_handle_literal(buf, datalen, header,
				    HPACK_PREFIX_LEN_LITERAL_INDEXING);
}

static int hpack_handle_literal_no_index(const uint8_t *buf, size_t datalen,
				  struct http_hpack_header_buf *header)
{
	return hpack_handle_literal(buf, datalen, header,
				    HPACK_PREFIX_LEN_LITERAL_NO_INDEXING);
}

static int hpack_handle_dynamic_size_update(const uint8_t *buf, size_t datalen)
{
	uint32_t max_size;
	int ret;

	ret = hpack_integer_decode(
		buf, datalen, HPACK_PREFIX_LEN_DYNAMIC_TABLE_SIZE_UPDATE, &max_size);
	if (ret < 0) {
		return ret;
	}

	/* TODO Add dynamic table support, if needed. */

	return ret;
}

int http_hpack_decode_header(const uint8_t *buf, size_t datalen,
			     struct http_hpack_header_buf *header)
{
	uint8_t prefix;
	int ret;

	if (buf == NULL || header == NULL) {
		return -EINVAL;
	}

	if (datalen == 0) {
		return -EAGAIN;
	}

	prefix = *buf;

	if ((prefix & HPACK_PREFIX_INDEXED_MASK) == HPACK_PREFIX_INDEXED) {
		ret = hpack_handle_indexed(buf, datalen, header);
	} else if ((prefix & HPACK_PREFIX_LITERAL_INDEXING_MASK) ==
		   HPACK_PREFIX_LITERAL_INDEXING) {
		ret = hpack_handle_literal_index(buf, datalen, header);
	} else if (((prefix & HPACK_PREFIX_LITERAL_NO_INDEXING_MASK) ==
		    HPACK_PREFIX_LITERAL_NO_INDEXING) ||
		   ((prefix & HPACK_PREFIX_LITERAL_NEVER_INDEXED_MASK) ==
		    HPACK_PREFIX_LITERAL_NEVER_INDEXED)) {
		ret = hpack_handle_literal_no_index(buf, datalen, header);
	} else if ((prefix & HPACK_PREFIX_DYNAMIC_TABLE_SIZE_MASK) ==
		   HPACK_PREFIX_DYNAMIC_TABLE_SIZE_UPDATE) {
		ret = hpack_handle_dynamic_size_update(buf, datalen);
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static int hpack_integer_encode(uint8_t *buf, size_t buflen, int value,
				uint8_t prefix, uint8_t n)
{
	uint8_t limit = (1 << n) - 1;
	int len = 0;

	if (buflen == 0) {
		return -ENOBUFS;
	}

	/* Based on RFC7541, ch 5.1. */
	if (value  < limit) {
		*buf = prefix | (uint8_t)value;

		return 1;
	}

	*buf++ = prefix | limit;
	len++;
	value -= limit;

	while (value >= 128) {
		if (len >= buflen) {
			return -ENOBUFS;
		}

		*buf = (uint8_t)((value % 128) + 128);
		len++;
		value /= 128;
	}

	if (len >= buflen) {
		return -ENOBUFS;
	}

	*buf = (uint8_t)value;
	len++;

	return len;
}

static int hpack_string_encode(uint8_t *buf, size_t buflen,
			       enum hpack_string_type type,
			       struct http_hpack_header_buf *header)
{
	int ret, len = 0;
	const char *str;
	size_t str_len;
	uint8_t prefix = 0;

	if (type == HPACK_HEADER_NAME) {
		str = header->name;
		str_len = header->name_len;
	} else {
		str = header->value;
		str_len = header->value_len;
	}

	/* Try to encode string into intermediate buffer. */
	ret = http_hpack_huffman_encode(str, str_len, header->buf,
					sizeof(header->buf));
	if (ret > 0 && ret < str_len) {
		/* Use Huffman encoded string only if smaller than the original. */
		str = header->buf;
		str_len = ret;
		prefix = HPACK_STRING_HUFFMAN_FLAG;
	}

	/* Encode string length. */
	ret = hpack_integer_encode(buf, buflen, str_len, prefix,
				   HPACK_STRING_PREFIX_LEN);
	if (ret < 0) {
		return ret;
	}

	buf += ret;
	buflen -= ret;
	len += ret;

	/* Copy string. */
	if (str_len > buflen) {
		return -ENOBUFS;
	}

	memcpy(buf, str, str_len);
	len += str_len;

	return len;
}

static int hpack_encode_literal(uint8_t *buf, size_t buflen,
				struct http_hpack_header_buf *header)
{
	int ret, len = 0;

	ret = hpack_integer_encode(buf, buflen, 0,
				   HPACK_PREFIX_LITERAL_NEVER_INDEXED,
				   HPACK_PREFIX_LEN_LITERAL_NEVER_INDEXED);
	if (ret < 0) {
		return ret;
	}

	buf += ret;
	buflen -= ret;
	len += ret;

	ret = hpack_string_encode(buf, buflen, HPACK_HEADER_NAME, header);
	if (ret < 0) {
		return ret;
	}

	buf += ret;
	buflen -= ret;
	len += ret;

	ret = hpack_string_encode(buf, buflen, HPACK_HEADER_VALUE, header);
	if (ret < 0) {
		return ret;
	}

	len += ret;

	return len;
}

static int hpack_encode_literal_value(uint8_t *buf, size_t buflen, int index,
				      struct http_hpack_header_buf *header)
{
	int ret, len = 0;

	ret = hpack_integer_encode(buf, buflen, index,
				   HPACK_PREFIX_LITERAL_NEVER_INDEXED,
				   HPACK_PREFIX_LEN_LITERAL_NEVER_INDEXED);
	if (ret < 0) {
		return ret;
	}

	buf += ret;
	buflen -= ret;
	len += ret;

	ret = hpack_string_encode(buf, buflen, HPACK_HEADER_VALUE, header);
	if (ret < 0) {
		return ret;
	}

	len += ret;

	return len;
}

static int hpack_encode_indexed(uint8_t *buf, size_t buflen, int index)
{
	return hpack_integer_encode(buf, buflen, index, HPACK_PREFIX_INDEXED,
				    HPACK_PREFIX_LEN_INDEXED);
}

int http_hpack_encode_header(uint8_t *buf, size_t buflen,
			     struct http_hpack_header_buf *header)
{
	int ret, len = 0;
	bool name_only;

	if (buf == NULL || header == NULL ||
	    header->name == NULL || header->name_len == 0 ||
	    header->value == NULL || header->value_len == 0) {
		return -EINVAL;
	}

	if (buflen == 0) {
		return -ENOBUFS;
	}

	ret = http_hpack_find_index(header, &name_only);
	if (ret < 0) {
		/* All literal */
		len = hpack_encode_literal(buf, buflen, header);
	} else if (name_only) {
		/* Literal value */
		len = hpack_encode_literal_value(buf, buflen, ret, header);
	} else {
		/* Indexed */
		len = hpack_encode_indexed(buf, buflen, ret);
	}

	return len;
}
