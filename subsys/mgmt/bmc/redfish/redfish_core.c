/*
 * Request dispatch, authentication and JSON buffering shared by every Redfish
 * resource.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/auth.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/mgmt/bmc/redfish.h>
#include <zephyr/sys/base64.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/* Basic authentication, so the credentials arrive in the Authorization header. */
HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_authorization, "authorization");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_x_auth_code, "x-auth-code");

#define CREDENTIALS_MAX_LEN 64
#define BASIC_AUTH_PREFIX   "Basic "

const struct json_obj_descr redfish_link_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_link, "@odata.id", odata_id, JSON_TOK_STRING),
};

static K_HEAP_DEFINE(redfish_buffers, CONFIG_BMC_REDFISH_BUFFER_HEAP_SIZE);

static const char *find_header(struct http_client_ctx *client, const char *name)
{
	const struct http_header *headers = client->header_capture_ctx.headers;
	size_t count = client->header_capture_ctx.count;

	for (size_t i = 0; i < count; i++) {
		if (strcasecmp(headers[i].name, name) == 0) {
			return headers[i].value;
		}
	}

	return NULL;
}

static int validate_auth(struct http_client_ctx *client)
{
	char decoded[CREDENTIALS_MAX_LEN];
	const char *auth_header;
	const char *b64_token;
	size_t decoded_len = 0;
	int ret;

	auth_header = find_header(client, "authorization");
	if (auth_header == NULL) {
		LOG_DBG("No authorization header");
		return -EACCES;
	}

	if (strncmp(auth_header, BASIC_AUTH_PREFIX, sizeof(BASIC_AUTH_PREFIX) - 1) != 0) {
		LOG_DBG("Authorization header is not basic authentication");
		return -EACCES;
	}

	b64_token = auth_header + sizeof(BASIC_AUTH_PREFIX) - 1;

	ret = base64_decode(decoded, sizeof(decoded) - 1, &decoded_len, b64_token,
			    strlen(b64_token));
	if (ret < 0) {
		LOG_DBG("Could not decode the credentials (err=%d)", ret);
		return -EACCES;
	}

	decoded[decoded_len] = '\0';

	ret = bmc_auth_check_pair(decoded, ':');
	if (ret < 0) {
		LOG_WRN("Authentication failed");
		return -EACCES;
	}

	return 0;
}

static int validate_and_set_headers(struct http_client_ctx *client,
				    struct http_response_ctx *ctx, uint32_t supported_methods)
{
	if ((BIT(client->method) & supported_methods) == 0) {
		static const struct http_header allow_get[] = {
			{.name = "allow", .value = "GET"},
		};
		static const struct http_header allow_post[] = {
			{.name = "allow", .value = "POST"},
		};
		static const struct http_header allow_get_patch[] = {
			{.name = "allow", .value = "GET, PATCH"},
		};

		ctx->status = HTTP_405_METHOD_NOT_ALLOWED;

		if (supported_methods == BIT(HTTP_GET)) {
			ctx->headers = allow_get;
			ctx->header_count = ARRAY_SIZE(allow_get);
		} else if (supported_methods == BIT(HTTP_POST)) {
			ctx->headers = allow_post;
			ctx->header_count = ARRAY_SIZE(allow_post);
		} else if (supported_methods == (BIT(HTTP_GET) | BIT(HTTP_PATCH))) {
			ctx->headers = allow_get_patch;
			ctx->header_count = ARRAY_SIZE(allow_get_patch);
		} else {
			LOG_ERR("No allow header for method mask 0x%08x", supported_methods);
		}

		ctx->final_chunk = true;
		ctx->body = NULL;
		ctx->body_len = 0;

		return -ENOTSUP;
	}

	if (client->method == HTTP_GET) {
		static const struct http_header json_headers[] = {
			{.name = "content-type", .value = "application/json"},
			{.name = "cache-control", .value = "no-cache"},
		};

		ctx->headers = json_headers;
		ctx->header_count = ARRAY_SIZE(json_headers);
	}

	return 0;
}

static void set_unauth_response(struct http_client_ctx *client, struct http_response_ctx *ctx)
{
	static const struct http_header auth_headers[] = {
		{.name = "www-authenticate", .value = "Basic realm=\"BMC\""},
	};
	const char *auth_code = find_header(client, "x-auth-code");

	ctx->status = HTTP_401_UNAUTHORIZED;
	ctx->final_chunk = true;
	ctx->body = NULL;
	ctx->body_len = 0;

	/*
	 * A www-authenticate header makes the browser pop up its own login
	 * dialog, which would fight with the web UI login form, so the web UI
	 * marks its requests and gets a bare 401 back to render itself.
	 */
	if (auth_code == NULL || strcmp(auth_code, "webui") != 0) {
		ctx->headers = auth_headers;
		ctx->header_count = ARRAY_SIZE(auth_headers);
	}
}

static void ctx_alloc(struct bmc_redfish_ctx *ctx, size_t size)
{
	ctx->started = true;
	ctx->size = size;
	ctx->data_len = 0;
	ctx->data_buffer = k_heap_alloc(&redfish_buffers, size, K_FOREVER);
}

static void ctx_free(struct bmc_redfish_ctx *ctx)
{
	if (!ctx->started) {
		return;
	}

	k_heap_free(&redfish_buffers, ctx->data_buffer);

	ctx->started = false;
	ctx->size = 0;
	ctx->data_len = 0;
	ctx->data_buffer = NULL;
}

int bmc_redfish_reply_append(struct bmc_redfish_ctx *ctx, const void *data, size_t len)
{
	if (ctx->data_len + len > ctx->size) {
		LOG_WRN("Redfish buffer too small: %zu bytes needed, %zu available",
			ctx->data_len + len, ctx->size);
		return -ENOSPC;
	}

	memcpy(ctx->data_buffer + ctx->data_len, data, len);
	ctx->data_len += len;

	return 0;
}

static int json_append_cb(const char *bytes, size_t len, void *data)
{
	return bmc_redfish_reply_append(data, bytes, len);
}

int bmc_redfish_reply_encode(struct bmc_redfish_ctx *ctx, const struct json_obj_descr *descr,
			     size_t descr_len, const void *val)
{
	return json_obj_encode(descr, descr_len, val, json_append_cb, ctx);
}

int bmc_redfish_request_parse(struct bmc_redfish_ctx *ctx, const struct json_obj_descr *descr,
			      size_t descr_len, void *val)
{
	return json_obj_parse((char *)ctx->data_buffer, ctx->data_len, descr, descr_len, val);
}

int bmc_redfish_reply_add_member(struct bmc_redfish_ctx *ctx, const char *name,
				 const struct json_obj_descr *descr, size_t descr_len,
				 const void *val)
{
	int ret;

	ret = bmc_redfish_reply_append(ctx, ",\"", 2);
	if (ret < 0) {
		return ret;
	}

	ret = bmc_redfish_reply_append(ctx, name, strlen(name));
	if (ret < 0) {
		return ret;
	}

	ret = bmc_redfish_reply_append(ctx, "\":", 2);
	if (ret < 0) {
		return ret;
	}

	return bmc_redfish_reply_encode(ctx, descr, descr_len, val);
}

int redfish_encode_with_oem(struct bmc_redfish_ctx *ctx, const struct json_obj_descr *descr,
			    size_t descr_len, const void *val, uint8_t target)
{
	size_t closed_len;
	int ret;

	ret = bmc_redfish_reply_encode(ctx, descr, descr_len, val);
	if (ret < 0) {
		return ret;
	}

	if (ctx->data_len == 0 || ctx->data_buffer[ctx->data_len - 1] != '}') {
		return -EINVAL;
	}

	/* Reopen the object so that extensions can append further members. */
	closed_len = ctx->data_len;
	ctx->data_len--;

	STRUCT_SECTION_FOREACH(bmc_redfish_oem, oem) {
		if (oem->target != target) {
			continue;
		}

		ret = oem->encode(ctx);
		if (ret < 0) {
			LOG_ERR("Redfish OEM encoder failed (err=%d)", ret);
			ctx->data_len = closed_len;
			return ret;
		}
	}

	if (ctx->data_len == closed_len - 1) {
		/* Nothing was appended, restore the original closing brace. */
		ctx->data_len = closed_len;
		return 0;
	}

	return bmc_redfish_reply_append(ctx, "}", 1);
}

int redfish_collection_open(struct bmc_redfish_ctx *ctx, const char *odata_id,
			    const char *odata_type, const char *name, size_t count)
{
	char header[256];
	int len;

	len = snprintk(header, sizeof(header),
		       "{\"@odata.id\":\"%s\",\"@odata.type\":\"%s\",\"Name\":\"%s\","
		       "\"Members@odata.count\":%zu,\"Members\":[",
		       odata_id, odata_type, name, count);
	if (len < 0 || len >= (int)sizeof(header)) {
		return -ENOSPC;
	}

	return bmc_redfish_reply_append(ctx, header, len);
}

int redfish_collection_add(struct bmc_redfish_ctx *ctx, bool first, const char *uri, ...)
{
	static const char member_open[] = "{\"@odata.id\":\"";
	static const char member_close[] = "\"}";
	char member[128];
	va_list args;
	int len;
	int ret;

	if (!first) {
		ret = bmc_redfish_reply_append(ctx, ",", 1);
		if (ret < 0) {
			return ret;
		}
	}

	va_start(args, uri);
	len = vsnprintk(member, sizeof(member), uri, args);
	va_end(args);

	if (len < 0 || len >= (int)sizeof(member)) {
		return -ENOSPC;
	}

	ret = bmc_redfish_reply_append(ctx, member_open, sizeof(member_open) - 1);
	if (ret < 0) {
		return ret;
	}

	ret = bmc_redfish_reply_append(ctx, member, len);
	if (ret < 0) {
		return ret;
	}

	return bmc_redfish_reply_append(ctx, member_close, sizeof(member_close) - 1);
}

int redfish_collection_close(struct bmc_redfish_ctx *ctx)
{
	return bmc_redfish_reply_append(ctx, "]}", 2);
}

const char *redfish_iso_time(void)
{
	/* Oversized so that the compiler cannot warn about truncation. */
	static char str[48];
	struct timespec ts;
	struct tm tm;

	if (sys_clock_gettime(SYS_CLOCK_REALTIME, &ts) != 0) {
		ts.tv_sec = 0;
	}

	gmtime_r(&ts.tv_sec, &tm);

	/* strftime() is not in the minimal libc, so format the fields here. */
	snprintk(str, sizeof(str), "%04d-%02d-%02dT%02d:%02d:%02dZ", tm.tm_year + 1900,
		 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	return str;
}

/*
 * Length of the request path, query string and any trailing slash removed, so
 * that "/redfish/v1/" and "/redfish/v1?foo=bar" address the same resource.
 */
static size_t request_path_len(const char *url)
{
	size_t len = strcspn(url, "?");

	if (len > 1 && url[len - 1] == '/') {
		len--;
	}

	return len;
}

static bool url_is_wildcard(const char *url)
{
	size_t len = strlen(url);

	return len >= 2 && url[len - 1] == '*' && url[len - 2] == '/';
}

static bool url_matches(const char *url, const char *path, size_t path_len)
{
	if (strncmp(url, path, path_len) != 0) {
		return false;
	}

	/* The registered URL may carry the trailing slash stripped from the path. */
	return url[path_len] == '\0' || (url[path_len] == '/' && url[path_len + 1] == '\0');
}

static bool url_matches_wildcard(const char *url, const char *path, size_t path_len)
{
	size_t prefix_len = strlen(url) - 1;

	if (path_len <= prefix_len || strncmp(url, path, prefix_len) != 0) {
		return false;
	}

	/* The wildcard stands for exactly one path segment. */
	return memchr(path + prefix_len, '/', path_len - prefix_len) == NULL;
}

static const struct bmc_redfish_resource *find_resource(const char *path, size_t path_len)
{
	STRUCT_SECTION_FOREACH(bmc_redfish_resource, resource) {
		if (!url_is_wildcard(resource->url) &&
		    url_matches(resource->url, path, path_len)) {
			return resource;
		}
	}

	STRUCT_SECTION_FOREACH(bmc_redfish_resource, resource) {
		if (url_is_wildcard(resource->url) &&
		    url_matches_wildcard(resource->url, path, path_len)) {
			return resource;
		}
	}

	return NULL;
}

/*
 * The whole Redfish tree is served by a single HTTP resource, so a transaction
 * context is claimed per client instead of per resource.
 */
static struct redfish_transaction {
	struct http_client_ctx *client;
	struct bmc_redfish_ctx ctx;
} redfish_transactions[CONFIG_HTTP_SERVER_MAX_CLIENTS];

static struct bmc_redfish_ctx *transaction_claim(struct http_client_ctx *client)
{
	struct redfish_transaction *unused = NULL;

	ARRAY_FOR_EACH_PTR(redfish_transactions, transaction) {
		if (transaction->client == client) {
			return &transaction->ctx;
		}

		if (unused == NULL && transaction->client == NULL) {
			unused = transaction;
		}
	}

	if (unused == NULL) {
		return NULL;
	}

	unused->client = client;

	return &unused->ctx;
}

static void transaction_release(struct http_client_ctx *client)
{
	ARRAY_FOR_EACH_PTR(redfish_transactions, transaction) {
		if (transaction->client != client) {
			continue;
		}

		ctx_free(&transaction->ctx);
		transaction->client = NULL;
		return;
	}
}

static int redfish_dispatch(struct http_client_ctx *client, enum http_transaction_status status,
			    const struct http_request_ctx *request_ctx,
			    struct http_response_ctx *response_ctx, void *user_data)
{
	const struct bmc_redfish_resource *resource;
	uint32_t allow_methods = 0;
	struct bmc_redfish_ctx *ctx;
	const char *url;
	int ret;

	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_TRANSACTION_COMPLETE ||
	    status == HTTP_SERVER_TRANSACTION_ABORTED) {
		transaction_release(client);
		return 0;
	}

	url = (const char *)client->url_buffer;

	resource = find_resource(url, request_path_len(url));
	if (resource == NULL) {
		response_ctx->status = HTTP_404_NOT_FOUND;
		response_ctx->final_chunk = true;
		return 0;
	}

	ctx = transaction_claim(client);
	if (ctx == NULL) {
		LOG_WRN("No Redfish transaction context left for %s", url);
		response_ctx->status = HTTP_503_SERVICE_UNAVAILABLE;
		response_ctx->final_chunk = true;
		return 0;
	}

	ctx->url = url;

	if (resource->get != NULL) {
		allow_methods |= BIT(HTTP_GET);
	}

	if (resource->patch != NULL) {
		allow_methods |= BIT(HTTP_PATCH);
	}

	if (resource->post != NULL) {
		allow_methods |= BIT(HTTP_POST);
	}

	if (validate_and_set_headers(client, response_ctx, allow_methods) < 0) {
		return 0;
	}

	if (resource->auth && validate_auth(client) < 0) {
		set_unauth_response(client, response_ctx);
		return 0;
	}

	if (client->method == HTTP_PATCH || client->method == HTTP_POST) {
		if (!ctx->started) {
			ctx_alloc(ctx, CONFIG_BMC_REDFISH_REQUEST_BUFFER_SIZE);
		}

		/* Accumulate the payload until the final chunk arrives. */
		if (request_ctx->data != NULL && request_ctx->data_len > 0) {
			if (bmc_redfish_reply_append(ctx, request_ctx->data,
						     request_ctx->data_len) < 0) {
				ctx_free(ctx);
				response_ctx->status = HTTP_413_PAYLOAD_TOO_LARGE;
				response_ctx->final_chunk = true;
				return 0;
			}
		}

		if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
			return 0;
		}

		ret = (client->method == HTTP_PATCH) ? resource->patch(ctx) : resource->post(ctx);
		ctx_free(ctx);

		response_ctx->status = (ret != 0) ? ret : HTTP_204_NO_CONTENT;
	} else if (client->method == HTTP_GET) {
		if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
			/* GET requests are small, there is nothing to accumulate. */
			response_ctx->status = HTTP_400_BAD_REQUEST;
			response_ctx->final_chunk = true;
			return 0;
		}

		ctx_free(ctx);
		ctx_alloc(ctx, CONFIG_BMC_REDFISH_RESPONSE_BUFFER_SIZE);

		ret = resource->get(ctx);
		if (ret != 0) {
			response_ctx->status = ret;
		} else {
			response_ctx->body = ctx->data_buffer;
			response_ctx->body_len = ctx->data_len;
			response_ctx->status = HTTP_200_OK;
		}
	} else {
		/* Unreachable, validate_and_set_headers() rejects other methods. */
		response_ctx->status = HTTP_500_INTERNAL_SERVER_ERROR;
	}

	response_ctx->final_chunk = true;

	return 0;
}

static struct http_resource_detail_dynamic redfish_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = -1U,
	},
	.cb = redfish_dispatch,
};

/*
 * The HTTP server matches this both exactly and as a leading directory, so the
 * router sees every request below /redfish that no other resource claims.
 */
BMC_HTTP_RESOURCE_DEFINE(bmc_redfish, "/redfish", &redfish_detail);
