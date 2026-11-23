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
#include <zephyr/sys/byteorder.h>

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

/* The dynamic table pool. The server advertises a table size of 0, so a
 * client needs a table only until it has acknowledged that, and a few tables
 * serve all clients.
 */
#if HTTP_HPACK_DYNAMIC_TABLE_COUNT > 0
static struct http_hpack_dynamic_table dynamic_tables[HTTP_HPACK_DYNAMIC_TABLE_COUNT];
#endif

/* Bytes in front of each entry's strings: name length and value length. */
#define HPACK_DYNAMIC_ENTRY_HEADER_LEN 4

struct http_hpack_dynamic_table *http_hpack_dynamic_table_alloc(void)
{
#if HTTP_HPACK_DYNAMIC_TABLE_COUNT > 0
	ARRAY_FOR_EACH_PTR(dynamic_tables, table) {
		if (table->in_use) {
			continue;
		}

		table->data_len = 0;
		table->count = 0;
		table->size = 0;
		table->max_size = HTTP_HPACK_DYNAMIC_TABLE_SIZE;
		table->in_use = true;

		return table;
	}
#endif

	return NULL;
}

void http_hpack_dynamic_table_free(struct http_hpack_dynamic_table *table)
{
	if (table != NULL) {
		table->in_use = false;
	}
}

/* Entry at the given position counted from the oldest one. */
static uint8_t *hpack_dynamic_entry(struct http_hpack_dynamic_table *table, uint32_t pos,
				    uint16_t *name_len, uint16_t *value_len)
{
	uint8_t *entry = table->data;

	NET_ASSERT(pos < table->count);

	while (true) {
		*name_len = sys_get_le16(entry);
		*value_len = sys_get_le16(entry + sizeof(uint16_t));

		if (pos == 0) {
			return entry + HPACK_DYNAMIC_ENTRY_HEADER_LEN;
		}

		entry += HPACK_DYNAMIC_ENTRY_HEADER_LEN + *name_len + *value_len;
		pos--;
	}
}

static void hpack_dynamic_table_evict_to(struct http_hpack_dynamic_table *table, uint32_t max_size)
{
	while (table->size > max_size) {
		uint16_t name_len, value_len;
		uint16_t entry_len;

		(void)hpack_dynamic_entry(table, 0, &name_len, &value_len);
		entry_len = (uint16_t)(HPACK_DYNAMIC_ENTRY_HEADER_LEN + name_len + value_len);

		memmove(table->data, table->data + entry_len, table->data_len - entry_len);
		table->data_len -= entry_len;
		table->count--;
		table->size -= name_len + value_len + HTTP_HPACK_DYNAMIC_TABLE_ENTRY_OVERHEAD;
	}
}

/* Adds the field as the newest entry and points the header at the copy
 * in the table. An entry larger than the table empties it (RFC 7541,
 * Section 4.4). name_in_table tells that the name is a dynamic table
 * reference.
 */
static void hpack_dynamic_table_add(struct http_hpack_dynamic_table *table,
				    struct http_hpack_header_buf *header, bool name_in_table)
{
	uint32_t entry_size = (uint32_t)(header->name_len + header->value_len +
					 HTTP_HPACK_DYNAMIC_TABLE_ENTRY_OVERHEAD);
	uint8_t *entry;

	/* A name taken from the dynamic table does not survive the eviction
	 * below, which moves the entries, so it goes to the decode buffer
	 * like a Huffman string, with the same size limit.
	 */
	if (name_in_table) {
		if (header->name_len > sizeof(header->buf) - header->datalen) {
			header->oversized = true;
			return;
		}

		memcpy(header->buf + header->datalen, header->name, header->name_len);
		header->name = header->buf + header->datalen;
		header->datalen += header->name_len;
	}

	if (entry_size > table->max_size) {
		hpack_dynamic_table_evict_to(table, 0);
		return;
	}

	hpack_dynamic_table_evict_to(table, table->max_size - entry_size);

	/* The size accounting leaves room: every entry counts 32 bytes on
	 * top of its strings and is stored with 4.
	 */
	entry = table->data + table->data_len;
	sys_put_le16((uint16_t)header->name_len, entry);
	sys_put_le16((uint16_t)header->value_len, entry + sizeof(uint16_t));
	entry += HPACK_DYNAMIC_ENTRY_HEADER_LEN;
	memcpy(entry, header->name, header->name_len);
	memcpy(entry + header->name_len, header->value, header->value_len);

	table->data_len +=
		(uint16_t)(HPACK_DYNAMIC_ENTRY_HEADER_LEN + header->name_len + header->value_len);
	table->count++;
	table->size += entry_size;

	header->name = entry;
	header->value = entry + header->name_len;
}

/* Looks an index up in the static table and the dynamic table. Returns
 * -EBADMSG for an index neither table has, -ENOENT for a dynamic table
 * index while there is no table.
 */
static int hpack_resolve_index(uint32_t index, struct http_hpack_dynamic_table *table,
			       struct http_hpack_header_buf *header)
{
	const struct hpack_table_entry *entry;
	uint16_t name_len, value_len;
	uint8_t *strings;
	uint32_t pos;

	if (index == 0) {
		return -EBADMSG;
	}

	entry = http_hpack_table_get(index);
	if (entry != NULL) {
		if (entry->name == NULL) {
			return -EBADMSG;
		}

		header->name = entry->name;
		header->name_len = strlen(entry->name);
		/* Static entries with an empty value store it as NULL. */
		header->value = (entry->value != NULL) ? entry->value : "";
		header->value_len = strlen(header->value);

		return 0;
	}

	if (table == NULL) {
		return -ENOENT;
	}

	/* The newest entry has the lowest dynamic index. */
	pos = index - HTTP_HPACK_STATIC_TABLE_LAST_INDEX - 1;
	if (pos >= table->count) {
		return -EBADMSG;
	}

	strings = hpack_dynamic_entry(table, table->count - 1 - pos, &name_len, &value_len);
	header->name = strings;
	header->name_len = name_len;
	header->value = strings + name_len;
	header->value_len = value_len;

	return 0;
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
/* Shift of the last 7-bit group that fits a 32-bit integer. */
#define HPACK_INTEGER_MAX_SHIFT                    28
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
		uint32_t group;

		buf++;
		len++;

		if (--datalen == 0) {
			return -EAGAIN;
		}

		group = *buf & ~HPACK_INTEGER_CONTINUATION_FLAG;

		/* Integers are limited to 32 bits (RFC 7541, Section 5.1). */
		if (m > HPACK_INTEGER_MAX_SHIFT || ((group << m) >> m) != group ||
		    *value > UINT32_MAX - (group << m)) {
			return -EBADMSG;
		}

		*value += group << m;
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
	if (ret == -ENOBUFS) {
		/* The string length is explicit, so the entry can still be
		 * consumed; the field is reported as oversized instead.
		 */
		header->oversized = true;
		ret = 0;
	} else if (ret < 0) {
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
				struct http_hpack_header_buf *header,
				struct http_hpack_dynamic_table *table)
{
	uint32_t index;
	int ret, len;

	ret = hpack_integer_decode(buf, datalen, HPACK_PREFIX_LEN_INDEXED,
				   &index);
	if (ret < 0) {
		return ret;
	}

	len = ret;

	ret = hpack_resolve_index(index, table, header);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	if (ret == -ENOENT) {
		header->unresolved = true;
		header->name_len = 0;
		header->value_len = 0;
	}

	return len;
}

static int hpack_handle_literal(const uint8_t *buf, size_t datalen,
				struct http_hpack_header_buf *header,
				struct http_hpack_dynamic_table *table, uint8_t prefix_len,
				bool add_to_table)
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
		ret = hpack_resolve_index(index, table, header);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}

		if (ret == -ENOENT) {
			header->unresolved = true;
			header->name_len = 0;
		}
	}

	/* The value is consumed even when the name is unresolved. */
	ret = hpack_string_decode(buf, datalen, HPACK_HEADER_VALUE, header);
	if (ret < 0) {
		return ret;
	}

	len += ret;

	if (header->unresolved) {
		header->value_len = 0;
	} else if (add_to_table && table != NULL && !header->oversized) {
		/* An oversized field is not stored: it ends the request, and
		 * its decoded length is unknown.
		 */
		hpack_dynamic_table_add(table, header, index > HTTP_HPACK_STATIC_TABLE_LAST_INDEX);
	} else {
		/* Not added to the table. */
	}

	return len;
}

static int hpack_handle_literal_index(const uint8_t *buf, size_t datalen,
				      struct http_hpack_header_buf *header,
				      struct http_hpack_dynamic_table *table)
{
	return hpack_handle_literal(buf, datalen, header, table, HPACK_PREFIX_LEN_LITERAL_INDEXING,
				    true);
}

static int hpack_handle_literal_no_index(const uint8_t *buf, size_t datalen,
					 struct http_hpack_header_buf *header,
					 struct http_hpack_dynamic_table *table)
{
	return hpack_handle_literal(buf, datalen, header, table,
				    HPACK_PREFIX_LEN_LITERAL_NO_INDEXING, false);
}

/* The size requested is applied to the table, if any, and reported to the
 * caller, who knows what it has advertised.
 */
static int hpack_handle_dynamic_size_update(const uint8_t *buf, size_t datalen,
					    struct http_hpack_header_buf *header,
					    struct http_hpack_dynamic_table *table)
{
	uint32_t max_size;
	int ret;

	ret = hpack_integer_decode(
		buf, datalen, HPACK_PREFIX_LEN_DYNAMIC_TABLE_SIZE_UPDATE, &max_size);
	if (ret < 0) {
		return ret;
	}

	if (table != NULL) {
		if (max_size > HTTP_HPACK_DYNAMIC_TABLE_SIZE) {
			return -EBADMSG;
		}

		table->max_size = max_size;
		hpack_dynamic_table_evict_to(table, max_size);
	}

	header->type = HTTP_HPACK_ENTRY_TABLE_SIZE_UPDATE;
	header->table_size = max_size;
	header->name_len = 0;
	header->value_len = 0;

	return ret;
}

bool http_hpack_entry_adds_to_table(const uint8_t *buf, size_t datalen)
{
	return datalen > 0 &&
	       (*buf & HPACK_PREFIX_LITERAL_INDEXING_MASK) == HPACK_PREFIX_LITERAL_INDEXING;
}

int http_hpack_decode_header(const uint8_t *buf, size_t datalen,
			     struct http_hpack_header_buf *header,
			     struct http_hpack_dynamic_table *table)
{
	uint8_t prefix;
	int ret;

	if (buf == NULL || header == NULL) {
		return -EINVAL;
	}

	if (datalen == 0) {
		return -EAGAIN;
	}

	header->type = HTTP_HPACK_ENTRY_FIELD;
	header->oversized = false;
	header->unresolved = false;
	prefix = *buf;

	if ((prefix & HPACK_PREFIX_INDEXED_MASK) == HPACK_PREFIX_INDEXED) {
		ret = hpack_handle_indexed(buf, datalen, header, table);
	} else if ((prefix & HPACK_PREFIX_LITERAL_INDEXING_MASK) ==
		   HPACK_PREFIX_LITERAL_INDEXING) {
		ret = hpack_handle_literal_index(buf, datalen, header, table);
	} else if (((prefix & HPACK_PREFIX_LITERAL_NO_INDEXING_MASK) ==
		    HPACK_PREFIX_LITERAL_NO_INDEXING) ||
		   ((prefix & HPACK_PREFIX_LITERAL_NEVER_INDEXED_MASK) ==
		    HPACK_PREFIX_LITERAL_NEVER_INDEXED)) {
		ret = hpack_handle_literal_no_index(buf, datalen, header, table);
	} else if ((prefix & HPACK_PREFIX_DYNAMIC_TABLE_SIZE_MASK) ==
		   HPACK_PREFIX_DYNAMIC_TABLE_SIZE_UPDATE) {
		ret = hpack_handle_dynamic_size_update(buf, datalen, header, table);
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

		*buf++ = (uint8_t)((value % 128) + 128);
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
	    (header->value == NULL && header->value_len > 0)) {
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

int http_hpack_encode_field(uint8_t *buf, size_t buflen, struct http_hpack_header_buf *header,
			    const char *name, const char *value)
{
	header->name = name;
	header->name_len = strlen(name);
	header->value = value;
	header->value_len = strlen(value);

	return http_hpack_encode_header(buf, buflen, header);
}
