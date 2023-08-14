/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>

#include <zephyr/net/http/hpack.h>

static inline bool http_hpack_key_is_static(uint32_t key)
{
	return key > HTTP_SERVER_HPACK_INVALID && key <= HTTP_SERVER_HPACK_WWW_AUTHENTICATE;
}

static inline bool http_hpack_key_is_dynamic(uint32_t key)
{
	return key > HTTP_SERVER_HPACK_WWW_AUTHENTICATE;
}

static const void *http_hpack_table_static[] = {
	[HTTP_SERVER_HPACK_AUTHORITY] = (void *)":authority",
	[HTTP_SERVER_HPACK_METHOD_GET] = (void *)":method GET",
	[HTTP_SERVER_HPACK_METHOD_POST] = (void *)":method POST",
	[HTTP_SERVER_HPACK_PATH_ROOT] = (void *)":path /",
	[HTTP_SERVER_HPACK_PATH_INDEX] = (void *)":path /index.html",
	[HTTP_SERVER_HPACK_SCHEME_HTTP] = (void *)":scheme http",
	[HTTP_SERVER_HPACK_SCHEME_HTTPS] = (void *)":scheme https",
	[HTTP_SERVER_HPACK_STATUS_200] = (void *)":status 200",
	[HTTP_SERVER_HPACK_STATUS_204] = (void *)":status 204",
	[HTTP_SERVER_HPACK_STATUS_206] = (void *)":status 206",
	[HTTP_SERVER_HPACK_STATUS_304] = (void *)":status 304",
	[HTTP_SERVER_HPACK_STATUS_400] = (void *)":status 400",
	[HTTP_SERVER_HPACK_STATUS_404] = (void *)":status 404",
	[HTTP_SERVER_HPACK_STATUS_500] = (void *)":status 500",
	[HTTP_SERVER_HPACK_ACCEPT_CHARSET] = (void *)"accept-charset",
	[HTTP_SERVER_HPACK_ACCEPT_ENCODING] = (void *)"accept-encoding gzip, deflate",
	[HTTP_SERVER_HPACK_ACCEPT_LANGUAGE] = (void *)"accept-language",
	[HTTP_SERVER_HPACK_ACCEPT_RANGES] = (void *)"accept-ranges",
	[HTTP_SERVER_HPACK_ACCEPT] = (void *)"accept",
	[HTTP_SERVER_HPACK_ACCESS_CONTROL_ALLOW_ORIGIN] = (void *)"access-control-allow-origin",
	[HTTP_SERVER_HPACK_AGE] = (void *)"age",
	[HTTP_SERVER_HPACK_ALLOW] = (void *)"allow",
	[HTTP_SERVER_HPACK_AUTHORIZATION] = (void *)"authorization",
	[HTTP_SERVER_HPACK_CACHE_CONTROL] = (void *)"cache-control",
	[HTTP_SERVER_HPACK_CONTENT_DISPOSITION] = (void *)"content-disposition",
	[HTTP_SERVER_HPACK_CONTENT_ENCODING] = (void *)"content-encoding",
	[HTTP_SERVER_HPACK_CONTENT_LANGUAGE] = (void *)"content-language",
	[HTTP_SERVER_HPACK_CONTENT_LENGTH] = (void *)"content-length",
	[HTTP_SERVER_HPACK_CONTENT_LOCATION] = (void *)"content-location",
	[HTTP_SERVER_HPACK_CONTENT_RANGE] = (void *)"content-range",
	[HTTP_SERVER_HPACK_CONTENT_TYPE] = (void *)"content-type",
	[HTTP_SERVER_HPACK_COOKIE] = (void *)"cookie",
	[HTTP_SERVER_HPACK_DATE] = (void *)"date",
	[HTTP_SERVER_HPACK_ETAG] = (void *)"etag",
	[HTTP_SERVER_HPACK_EXPECT] = (void *)"expect",
	[HTTP_SERVER_HPACK_EXPIRES] = (void *)"expires",
	[HTTP_SERVER_HPACK_FROM] = (void *)"from",
	[HTTP_SERVER_HPACK_HOST] = (void *)"host",
	[HTTP_SERVER_HPACK_IF_MATCH] = (void *)"if-match",
	[HTTP_SERVER_HPACK_IF_MODIFIED_SINCE] = (void *)"if-modified-since",
	[HTTP_SERVER_HPACK_IF_NONE_MATCH] = (void *)"if-none-match",
	[HTTP_SERVER_HPACK_IF_RANGE] = (void *)"if-range",
	[HTTP_SERVER_HPACK_IF_UNMODIFIED_SINCE] = (void *)"if-unmodified-since",
	[HTTP_SERVER_HPACK_LAST_MODIFIED] = (void *)"last-modified",
	[HTTP_SERVER_HPACK_LINK] = (void *)"link",
	[HTTP_SERVER_HPACK_LOCATION] = (void *)"location",
	[HTTP_SERVER_HPACK_MAX_FORWARDS] = (void *)"max-forwards",
	[HTTP_SERVER_HPACK_PROXY_AUTHENTICATE] = (void *)"proxy-authenticate",
	[HTTP_SERVER_HPACK_PROXY_AUTHORIZATION] = (void *)"proxy-authorization",
	[HTTP_SERVER_HPACK_RANGE] = (void *)"range",
	[HTTP_SERVER_HPACK_REFERER] = (void *)"referer",
	[HTTP_SERVER_HPACK_REFRESH] = (void *)"refresh",
	[HTTP_SERVER_HPACK_RETRY_AFTER] = (void *)"retry-after",
	[HTTP_SERVER_HPACK_SERVER] = (void *)"server",
	[HTTP_SERVER_HPACK_SET_COOKIE] = (void *)"set-cookie",
	[HTTP_SERVER_HPACK_STRICT_TRANSPORT_SECURITY] = (void *)"strict-transport-security",
	[HTTP_SERVER_HPACK_TRANSFER_ENCODING] = (void *)"transfer-encoding",
	[HTTP_SERVER_HPACK_USER_AGENT] = (void *)"user-agent",
	[HTTP_SERVER_HPACK_VARY] = (void *)"vary",
	[HTTP_SERVER_HPACK_VIA] = (void *)"via",
	[HTTP_SERVER_HPACK_WWW_AUTHENTICATE] = (void *)"www-authenticate",
};

int http_hpack_table_contains(struct http_client_ctx *ctx, uint32_t key)
{
	if (!ctx || !http_hpack_key_is_static(key)) {
		return -EINVAL;
	}

	if (http_hpack_table_static[key]) {
		return 0;
	} else {
		return -EINVAL;
	}
}

int http_hpack_table_get(struct http_client_ctx *ctx, uint32_t key, void **value)
{
	if (!ctx || !value || !http_hpack_key_is_static(key)) {
		return -EINVAL;
	}

	*value = (void *)http_hpack_table_static[key];
	if (*value) {
		return 0;
	} else {
		return -EINVAL;
	}
}

int http_hpack_table_add(struct http_client_ctx *ctx, uint32_t key, void *value)
{
	return -ENOSYS;
}

int http_hpack_table_remove(struct http_client_ctx *ctx, uint32_t key)
{
	return -ENOSYS;
}

char *http_hpack_parse_header(struct http_client_ctx *ctx, int requested_data)
{
	size_t i = 0;

	while (i < ctx->current_frame.length) {
		uint8_t prefix = ctx->current_frame.payload[i];
		uint32_t index;

		if (prefix & HTTP_SERVER_HPACK_INDEXED_HEADER_FIELD) {
			/* Indexed Header Field */
			index = prefix & 0x7F;
			void *value;

			if (http_hpack_table_get(ctx, index, &value) == 0) {

				if (requested_data == HTTP_SERVER_HPACK_METHOD &&
				    strncmp((char *)value, ":method ",
					    _http_server_hpack_method_strlen) == 0) {
					return (char *)value + _http_server_hpack_method_strlen;
				} else if (requested_data == HTTP_SERVER_HPACK_PATH &&
					   strncmp((char *)value, ":path ",
						   _http_server_hpack_path_strlen) == 0) {
					return (char *)value + _http_server_hpack_path_strlen;
				}
			}
			i++;
		} else if (prefix & HTTP_SERVER_HPACK_LITERAL_WITH_INDEXING) {
			/* Literal Header Field with Incremental Indexing */
			i++;
		} else if (prefix & HTTP_SERVER_HPACK_DYNAMIC_TABLE_SIZE_UPDATE) {
			/* Dynamic Table Size Update */
			i++;
		} else {
			/* Literal Header Field without Indexing */
			i++;
		}
	}

	return "";
}
