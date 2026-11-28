/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <zephyr/net/http/parser_url.h>
#include <zephyr/net/url.h>
#include <zephyr/sys/util.h>

static struct net_url_field get_field(const struct http_parser_url *parsed,
				      enum http_parser_url_fields id)
{
	struct net_url_field field = {.off = 0, .len = 0};

	if ((parsed->field_set & BIT(id)) != 0U) {
		field.off = parsed->field_data[id].off;
		field.len = parsed->field_data[id].len;
	}

	return field;
}

int net_url_parse(const char *url, struct net_url *out)
{
	struct http_parser_url parsed;

	if (url == NULL || out == NULL) {
		return -EINVAL;
	}

	http_parser_url_init(&parsed);
	if (http_parser_parse_url(url, strlen(url), 0, &parsed) != 0) {
		return -EINVAL;
	}

	out->base = url;
	out->scheme = get_field(&parsed, UF_SCHEMA);
	out->userinfo = get_field(&parsed, UF_USERINFO);
	out->host = get_field(&parsed, UF_HOST);
	out->path = get_field(&parsed, UF_PATH);
	out->query = get_field(&parsed, UF_QUERY);
	out->fragment = get_field(&parsed, UF_FRAGMENT);
	out->port = ((parsed.field_set & BIT(UF_PORT)) != 0U) ? parsed.port : 0U;

	return 0;
}

bool net_url_scheme_is(const struct net_url *url, const char *scheme)
{
	const char *found = url->base + url->scheme.off;

	if (url->scheme.len != strlen(scheme)) {
		return false;
	}

	for (size_t i = 0; i < url->scheme.len; i++) {
		int a = tolower((unsigned char)found[i]);
		int b = tolower((unsigned char)scheme[i]);

		if (a != b) {
			return false;
		}
	}

	return true;
}

int net_url_field_copy(const struct net_url *url, const struct net_url_field *field, char *dst,
		       size_t dst_len)
{
	if (field->len >= dst_len) {
		return -ENOBUFS;
	}

	memcpy(dst, url->base + field->off, field->len);
	dst[field->len] = '\0';

	return 0;
}

int net_url_target_copy(const struct net_url *url, char *dst, size_t dst_len)
{
	const char *path = "/";
	size_t path_len = 1U;
	size_t query_len = 0U;
	char *pos = dst;

	if (url->path.len > 0U) {
		path = url->base + url->path.off;
		path_len = url->path.len;
	}

	if (url->query.len > 0U) {
		query_len = 1U + url->query.len; /* "?" and the query */
	}

	if (path_len + query_len >= dst_len) {
		return -ENOBUFS;
	}

	memcpy(pos, path, path_len);
	pos += path_len;

	if (query_len > 0U) {
		*pos = '?';
		pos++;
		memcpy(pos, url->base + url->query.off, url->query.len);
		pos += url->query.len;
	}
	*pos = '\0';

	return 0;
}
