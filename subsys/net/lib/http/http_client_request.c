/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Request building, shared by the protocol layers: the request line and the
 * header fields of a request, validated and filtered for the wire.
 */

#include <ctype.h>
#include <errno.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client_multiprotocol.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>

#include "headers/client_internal.h"

LOG_MODULE_DECLARE(net_http_client_mp, CONFIG_NET_HTTP_LOG_LEVEL);

/* Room for the decimal form of the largest size_t. */
#define CONTENT_LENGTH_STR_SIZE sizeof("18446744073709551615")

static const char *find_header_value(const struct http_client_header *headers,
				     size_t headers_count, const char *name)
{
	if (headers == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < headers_count; i++) {
		if (headers[i].name != NULL && strcasecmp(headers[i].name, name) == 0) {
			return headers[i].value;
		}
	}

	return NULL;
}

bool http_client_field_is_connection_specific(const char *name, size_t name_len)
{
	static const char *const names[] = {
		"connection", "keep-alive", "proxy-connection", "transfer-encoding", "upgrade",
	};

	for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
		if (name_len == strlen(names[i]) && strncasecmp(name, names[i], name_len) == 0) {
			return true;
		}
	}

	return false;
}

/* Only token characters may make up a field name (RFC 9110, Section 5.6.2),
 * a pseudo-header name is the protocol's to give, and a name has to fit the
 * buffer it may be lowercased in.
 */
static bool field_name_is_valid(const char *name)
{
	if (name[0] == '\0' || name[0] == ':' ||
	    strlen(name) >= HTTP_CLIENT_REQUEST_FIELD_MAX_LEN) {
		return false;
	}

	for (; *name != '\0'; name++) {
		unsigned char c = (unsigned char)*name;

		if (isalnum(c) == 0 && strchr("!#$%&'*+-.^_`|~", c) == NULL) {
			return false;
		}
	}

	return true;
}

/* A field value carries no control characters other than tab, and neither
 * starts nor ends with whitespace (RFC 9113, Section 8.2.1).
 */
static bool field_value_is_valid(const char *value)
{
	size_t len = strlen(value);

	if (len > 0 && (value[0] == ' ' || value[0] == '\t' || value[len - 1] == ' ' ||
			value[len - 1] == '\t')) {
		return false;
	}

	for (size_t i = 0; i < len; i++) {
		unsigned char c = (unsigned char)value[i];

		if (c != '\t' && iscntrl(c) != 0) {
			return false;
		}
	}

	return true;
}

/* Whether the len characters at s are all host characters (RFC 3986,
 * Section 3.2.2), plus those in extra: no userinfo, path, brackets or
 * whitespace.
 */
static bool host_chars_valid(const char *s, size_t len, const char *extra)
{
	for (size_t i = 0; i < len; i++) {
		unsigned char c = (unsigned char)s[i];

		if (isalnum(c) == 0 && strchr("-._~!$&'()*+,;=%", c) == NULL &&
		    (c == '\0' || strchr(extra, c) == NULL)) {
			return false;
		}
	}

	return true;
}

/* The length of the host at the start of s: a bracketed IPv6 literal, or a
 * name or IPv4 literal up to the port. 0 when there is no valid host.
 */
static size_t host_len(const char *s)
{
	size_t len;

	if (s[0] == '[') {
		const char *end = strchr(s, ']');

		if (end == NULL || end == s + 1 || !host_chars_valid(s + 1, end - s - 1, ":")) {
			return 0;
		}

		return end - s + 1;
	}

	len = strcspn(s, ":");

	return host_chars_valid(s, len, "") ? len : 0;
}

/* An authority is a host with an optional decimal port of at most 65535
 * (RFC 3986, Section 3.2; RFC 9113, Section 8.3.1).
 */
static bool authority_is_valid(const char *authority)
{
	size_t len = host_len(authority);
	uint32_t port = 0U;

	if (len == 0) {
		return false;
	}

	authority += len;
	if (*authority == '\0') {
		return true;
	}

	if (*authority != ':' || authority[1] == '\0') {
		return false;
	}

	for (authority++; *authority != '\0'; authority++) {
		if (isdigit((unsigned char)*authority) == 0) {
			return false;
		}

		port = port * 10U + (uint32_t)(*authority - '0');
		if (port > UINT16_MAX) {
			return false;
		}
	}

	return true;
}

/* A session host is a host name, an IPv4 literal or a bracketed IPv6 literal,
 * without a port.
 */
bool http_client_host_is_valid(const char *host)
{
	size_t len = host_len(host);

	return len > 0 && host[len] == '\0';
}

/* A request target carries neither control characters nor spaces. */
static bool url_is_valid(const char *url)
{
	if (url[0] == '\0') {
		return false;
	}

	/* A request target carries no fragment (RFC 9112, Section 3.2). */
	for (; *url != '\0'; url++) {
		unsigned char c = (unsigned char)*url;

		if (c == ' ' || c == '#' || iscntrl(c) != 0) {
			return false;
		}
	}

	return true;
}

/* Whether the request target is an absolute URL: a URI scheme (RFC 3986,
 * Section 3.1) followed by an authority. A colon on its own may well be part
 * of a path.
 */
static bool url_has_scheme(const char *url)
{
	if (isalpha((unsigned char)url[0]) == 0) {
		return false;
	}

	for (const char *p = url + 1; *p != '\0'; p++) {
		unsigned char c = (unsigned char)*p;

		if (c == ':') {
			return p[1] == '/' && p[2] == '/';
		}

		if (isalnum(c) == 0 && c != '+' && c != '-' && c != '.') {
			return false;
		}
	}

	return false;
}

/* Format host[:port] for the request. A configured host name takes precedence
 * over the address literal. An IPv6 literal is bracketed (RFC 3986,
 * Section 3.2.2) and the default port of the scheme is omitted.
 */
static int format_authority(struct http_client *client, const char *scheme, char *buf,
			    size_t buflen)
{
	const struct net_sockaddr *addr = (const struct net_sockaddr *)&client->session.server_addr;
	const char *host = client->session.host;
	char addr_str[NET_INET6_ADDRSTRLEN];
	bool bracket = false;
	bool default_port;
	uint16_t port;
	int ret;

	if (addr->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *addr4 = net_sin(addr);

		port = sys_be16_to_cpu(addr4->sin_port);
		if (net_addr_ntop(NET_AF_INET, &addr4->sin_addr, addr_str, sizeof(addr_str)) ==
		    NULL) {
			return -EINVAL;
		}
	} else if (addr->sa_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *addr6 = net_sin6(addr);

		port = sys_be16_to_cpu(addr6->sin6_port);
		if (net_addr_ntop(NET_AF_INET6, &addr6->sin6_addr, addr_str, sizeof(addr_str)) ==
		    NULL) {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	if (host == NULL) {
		host = addr_str;
		bracket = (addr->sa_family == NET_AF_INET6);
	}

	default_port = (strcmp(scheme, "https") == 0 && port == 443) ||
		       (strcmp(scheme, "http") == 0 && port == 80);

	if (bracket) {
		ret = default_port ? snprintk(buf, buflen, "[%s]", host) :
				     snprintk(buf, buflen, "[%s]:%u", host, port);
	} else {
		ret = default_port ? snprintk(buf, buflen, "%s", host) :
				     snprintk(buf, buflen, "%s:%u", host, port);
	}

	if (ret < 0 || ret >= buflen) {
		return -ENOBUFS;
	}

	return ret;
}

int http_client_request_line_get(struct http_client *client,
				 const struct http_client_request *req, char *buf, size_t buflen,
				 struct http_client_request_line *line)
{
	const char *authority;
	int ret;

	if (req->url == NULL) {
		return -EINVAL;
	}

	/* A tunnel has no request target and its body is the tunnel itself,
	 * which the request API does not express.
	 */
	if (req->method == HTTP_CONNECT) {
		return -ENOTSUP;
	}

	if (url_has_scheme(req->url)) {
		NET_ERR("Absolute URLs are not supported");
		return -EINVAL;
	}

	if (!url_is_valid(req->url)) {
		NET_ERR("Invalid request URL");
		return -EINVAL;
	}

	line->method = http_method_str(req->method);
	line->scheme = client->transport_ops->secure ? "https" : "http";

	/* An application Host header names the authority; the protocol layer
	 * carries it in its own way (:authority or Host), so the walker skips
	 * the header itself.
	 */
	authority = find_header_value(req->header_fields, req->headers_count, "host");
	if (authority != NULL && !authority_is_valid(authority)) {
		NET_ERR("Invalid Host header value");
		return -EINVAL;
	}

	if (authority == NULL) {
		ret = format_authority(client, line->scheme, buf, buflen);
		if (ret < 0) {
			return ret;
		}

		authority = buf;
		buf += ret + 1;
		buflen -= ret + 1;
	}

	line->authority = authority;
	line->path = req->url;

	/* The asterisk form is only meaningful for OPTIONS (RFC 9110,
	 * Section 7.1).
	 */
	if (req->method == HTTP_OPTIONS && strcmp(req->url, "*") == 0) {
		return 0;
	}

	if (req->url[0] != '/') {
		ret = snprintk(buf, buflen, "/%s", req->url);
		if (ret < 0 || ret >= buflen) {
			return -ENOBUFS;
		}

		line->path = buf;
	}

	return 0;
}

/* Field names are lowercase on the wire: HTTP/2 and HTTP/3 require it and
 * HTTP/1.1 does not care. Only a name with uppercase characters is copied.
 */
static int field_name_lower(const char *name, char *buf, size_t buflen, const char **out)
{
	size_t len = strlen(name);
	size_t i;

	for (i = 0; i < len; i++) {
		if (isupper((unsigned char)name[i]) != 0) {
			break;
		}
	}

	if (i == len) {
		*out = name;
		return 0;
	}

	if (len >= buflen) {
		return -ENOBUFS;
	}

	for (i = 0; i < len; i++) {
		buf[i] = (char)tolower((unsigned char)name[i]);
	}

	buf[len] = '\0';
	*out = buf;

	return 0;
}

/* Application fields the library sends itself or that have no place in the
 * request.
 */
struct field_filter {
	/* The protocol multiplexes the connection, so connection-specific
	 * fields are dropped (RFC 9113, Section 8.2.2).
	 */
	bool connection_specific;
	bool content_type;
	bool content_length;
};

/* Whether an application field is left out of the request. */
static bool field_is_filtered(const struct http_client_header *hdr,
			      const struct field_filter *filter)
{
	/* Host is carried by the request line. */
	if (strcasecmp(hdr->name, "host") == 0) {
		return true;
	}

	if (filter->content_type && strcasecmp(hdr->name, "content-type") == 0) {
		return true;
	}

	if (filter->content_length && strcasecmp(hdr->name, "content-length") == 0) {
		return true;
	}

	if (!filter->connection_specific) {
		return false;
	}

	if (http_client_field_is_connection_specific(hdr->name, strlen(hdr->name))) {
		return true;
	}

	/* TE may only ask for trailers. */
	if (strcasecmp(hdr->name, "te") == 0 && strcasecmp(hdr->value, "trailers") != 0) {
		return true;
	}

	return false;
}

static bool field_is_valid(const struct http_client_header *hdr)
{
	return hdr->name != NULL && hdr->value != NULL && field_name_is_valid(hdr->name) &&
	       field_value_is_valid(hdr->value);
}

/* Emit the application fields that pass the filter. The fields given up
 * front were validated before anything was sent; those from the callback
 * are validated here.
 */
static int emit_application_fields(const struct http_client_header *headers, size_t count,
				   bool validate, const struct field_filter *filter,
				   char *lower, size_t lower_len, http_client_field_cb_t cb,
				   void *user_data)
{
	int ret;

	if (headers == NULL) {
		return (count == 0) ? 0 : -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		const char *name;

		if (validate && !field_is_valid(&headers[i])) {
			NET_ERR("Invalid request header field");
			return -EINVAL;
		}

		/* The authority was settled before the callback ran. */
		if (validate && strcasecmp(headers[i].name, "host") == 0) {
			NET_ERR("Host header from the headers callback");
			return -EINVAL;
		}

		if (field_is_filtered(&headers[i], filter)) {
			NET_DBG("Request header %s dropped", headers[i].name);
			continue;
		}

		ret = field_name_lower(headers[i].name, lower, lower_len, &name);
		if (ret < 0) {
			return ret;
		}

		ret = cb(name, headers[i].value, user_data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int http_client_request_fields_foreach(struct http_client *client,
				       struct http_client_stream *stream, bool pseudo_headers,
				       http_client_field_cb_t cb, void *user_data)
{
	const struct http_client_request *req = stream->request;
	char buf[HTTP_CLIENT_REQUEST_FIELD_MAX_LEN];
	/* The length is only known up front for a payload passed in one piece. */
	bool content_length_known =
		(req->payload != NULL && req->payload_len > 0 && req->payload_cb == NULL);
	const struct field_filter filter = {
		.connection_specific = pseudo_headers,
		.content_type = (req->content_type_value != NULL),
		.content_length = content_length_known,
	};
	bool more = true;
	int ret;

	/* Nothing goes out before the fields given up front are known to be
	 * valid, so that a request is never left incomplete on the wire.
	 */
	if (req->header_fields == NULL && req->headers_count > 0) {
		return -EINVAL;
	}

	if (req->content_type_value != NULL && !field_value_is_valid(req->content_type_value)) {
		NET_ERR("Invalid content type");
		return -EINVAL;
	}

	for (size_t i = 0; i < req->headers_count; i++) {
		if (!field_is_valid(&req->header_fields[i])) {
			NET_ERR("Invalid request header field");
			return -EINVAL;
		}
	}

	if (pseudo_headers) {
		struct http_client_request_line line;
		const char *const names[] = { ":method", ":scheme", ":path", ":authority" };
		const char *values[ARRAY_SIZE(names)];

		ret = http_client_request_line_get(client, req, buf, sizeof(buf), &line);
		if (ret < 0) {
			return ret;
		}

		values[0] = line.method;
		values[1] = line.scheme;
		values[2] = line.path;
		values[3] = line.authority;

		for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
			ret = cb(names[i], values[i], user_data);
			if (ret < 0) {
				return ret;
			}
		}
	}

	if (req->content_type_value != NULL) {
		ret = cb("content-type", req->content_type_value, user_data);
		if (ret < 0) {
			return ret;
		}
	}

	if (content_length_known) {
		char content_len[CONTENT_LENGTH_STR_SIZE];

		ret = snprintk(content_len, sizeof(content_len), "%zu", req->payload_len);
		if (ret < 0 || ret >= sizeof(content_len)) {
			return -ENOBUFS;
		}

		ret = cb("content-length", content_len, user_data);
		if (ret < 0) {
			return ret;
		}
	}

	ret = emit_application_fields(req->header_fields, req->headers_count, false, &filter,
				      buf, sizeof(buf), cb, user_data);
	if (ret < 0) {
		return ret;
	}

	if (req->headers_cb == NULL) {
		return 0;
	}

	while (more) {
		const struct http_client_header *headers = NULL;
		size_t count = 0;

		more = false;
		ret = req->headers_cb(&headers, &count, &more, stream->user_data);
		if (ret < 0) {
			return ret;
		}

		ret = emit_application_fields(headers, count, true, &filter, buf, sizeof(buf), cb,
					      user_data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
