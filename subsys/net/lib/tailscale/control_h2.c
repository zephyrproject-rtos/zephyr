/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/net/http/hpack.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/net/net_ip.h>

#include "control_internal.h"

#define H2_HEADER_SIZE   9
#define H2_FRAME_SIZE    16384
#define H2_DATA          0
#define H2_HEADERS       1
#define H2_RST_STREAM    3
#define H2_SETTINGS      4
#define H2_PING          6
#define H2_GOAWAY        7
#define H2_WINDOW_UPDATE 8
#define H2_CONTINUATION  9
#define H2_END_STREAM    1
#define H2_ACK           1
#define H2_END_HEADERS   4
#define H2_PADDED        8
#define H2_PRIORITY      32

struct frame_header {
	uint32_t len;
	uint32_t stream;
	uint8_t type;
	uint8_t flags;
};

static void encode_header(uint8_t out[H2_HEADER_SIZE], const struct frame_header *hdr)
{
	sys_put_be24(hdr->len, out);
	out[3] = hdr->type;
	out[4] = hdr->flags;
	sys_put_be32(hdr->stream, out + 5);
}

static int write_frame(struct control_h2 *h2, uint8_t type, uint8_t flags, uint32_t stream,
		       const void *body, size_t len)
{
	struct frame_header hdr = {.len = len, .stream = stream, .type = type, .flags = flags};
	uint8_t wire[H2_HEADER_SIZE];

	encode_header(wire, &hdr);
	int rc = control_noise_write(h2->noise, wire, sizeof(wire));

	return rc == 0 ? control_noise_write(h2->noise, body, len) : rc;
}

static int read_header(struct control_h2 *h2, struct frame_header *hdr)
{
	uint8_t wire[H2_HEADER_SIZE];

	if (h2->has_preface_header) {
		memcpy(wire, h2->preface_header, sizeof(wire));
		h2->has_preface_header = false;
	} else {
		int rc = control_noise_read(h2->noise, wire, sizeof(wire));

		if (rc != 0) {
			return rc;
		}
	}
	*hdr = (struct frame_header){
		.len = sys_get_be24(wire),
		.type = wire[3],
		.flags = wire[4],
		.stream = sys_get_be32(wire + 5) & 0x7fffffff,
	};
	return hdr->len <= H2_FRAME_SIZE ? 0 : -EMSGSIZE;
}

static int skip_bytes(struct control_h2 *h2, size_t len)
{
	uint8_t unused[128];

	while (len > 0) {
		size_t part = MIN(len, sizeof(unused));
		int rc = control_noise_read(h2->noise, unused, part);

		if (rc != 0) {
			return rc;
		}
		len -= part;
	}
	return 0;
}

static int read_padding(struct control_h2 *h2, struct frame_header *hdr, uint8_t *padding)
{
	*padding = 0;
	if ((hdr->flags & H2_PADDED) == 0) {
		return 0;
	}
	if (hdr->len == 0) {
		return -EPROTO;
	}
	int rc = control_noise_read(h2->noise, padding, 1);

	if (rc != 0) {
		return rc;
	}
	if (*padding >= hdr->len) {
		return -EPROTO;
	}
	hdr->len -= 1 + *padding;
	return 0;
}

static int process_settings(struct control_h2 *h2, const struct frame_header *hdr)
{
	if (hdr->stream != 0 || hdr->len % 6 != 0) {
		return -EPROTO;
	}
	if ((hdr->flags & H2_ACK) != 0) {
		return hdr->len == 0 ? 0 : -EPROTO;
	}
	int rc = skip_bytes(h2, hdr->len);

	return rc == 0 ? write_frame(h2, H2_SETTINGS, H2_ACK, 0, NULL, 0) : rc;
}

int control_h2_start(struct control_h2 *h2, char *early, size_t cap, size_t *early_len)
{
	static const char preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
	static const uint8_t settings[] = {0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0};
	static const uint8_t early_magic[] = {0xff, 0xff, 0xff, 'T', 'S'};
	struct frame_header hdr;
	int rc = control_noise_write(h2->noise, preface, sizeof(preface) - 1);

	*early_len = 0;
	if (rc == 0) {
		rc = write_frame(h2, H2_SETTINGS, 0, 0, settings, sizeof(settings));
	}
	if (rc == 0) {
		rc = control_noise_read(h2->noise, h2->preface_header, sizeof(h2->preface_header));
	}
	if (rc != 0) {
		return rc;
	}
	if (memcmp(h2->preface_header, early_magic, sizeof(early_magic)) == 0) {
		*early_len = sys_get_be32(h2->preface_header + sizeof(early_magic));
		if (*early_len >= cap) {
			return -EMSGSIZE;
		}
		rc = control_noise_read(h2->noise, early, *early_len);
		if (rc != 0) {
			return rc;
		}
		early[*early_len] = '\0';
	} else {
		h2->has_preface_header = true;
	}
	rc = read_header(h2, &hdr);
	if (rc != 0) {
		return rc;
	}
	if (hdr.type != H2_SETTINGS || (hdr.flags & H2_ACK) != 0) {
		return -EPROTO;
	}
	return process_settings(h2, &hdr);
}

static uint8_t *hpack_string(uint8_t *dest, const char *value)
{
	size_t len = strlen(value);

	if (len >= 127U) {
		*dest++ = 127U;
		*dest++ = len - 127U;
	} else {
		*dest++ = len;
	}
	memcpy(dest, value, len);
	return dest + len;
}

static uint8_t *hpack_literal(uint8_t *dest, uint8_t name, const char *value)
{
	if (name >= 15) {
		*dest++ = 15;
		*dest++ = name - 15;
	} else {
		*dest++ = name;
	}
	return hpack_string(dest, value);
}

int control_h2_request(struct control_h2 *h2, uint32_t stream, const char *path,
		       const char *node_hex, const char *body, size_t len)
{
	/* Endpoint publication can interrupt a read using h2->frame. */
	uint8_t headers[512];
	uint8_t *end = headers;
	char node_key[sizeof("nodekey:") + 2 * CONTROL_KEY_SIZE];

	if (strlen(path) >= 127U || sizeof(CONTROL_HOST) > 254U || len > H2_FRAME_SIZE) {
		return -EMSGSIZE;
	}
	memcpy(node_key, "nodekey:", sizeof("nodekey:") - 1);
	memcpy(node_key + sizeof("nodekey:") - 1, node_hex, 2 * CONTROL_KEY_SIZE + 1);

	*end++ = 0x80 | HTTP_SERVER_HPACK_METHOD_POST;
	*end++ = 0x80 | HTTP_SERVER_HPACK_SCHEME_HTTPS;
	end = hpack_literal(end, HTTP_SERVER_HPACK_PATH_ROOT, path);
	end = hpack_literal(end, HTTP_SERVER_HPACK_AUTHORITY, CONTROL_HOST);
	end = hpack_literal(end, HTTP_SERVER_HPACK_CONTENT_TYPE, "application/json");
	*end++ = 0;
	end = hpack_string(end, "ts-lb");
	end = hpack_string(end, node_key);
	int rc = write_frame(h2, H2_HEADERS, H2_END_HEADERS, stream, headers, end - headers);

	return rc == 0 ? write_frame(h2, H2_DATA, H2_END_STREAM, stream, body, len) : rc;
}

static int response_status(const char *value, size_t len)
{
	if (len != 3U || value[1] < '0' || value[1] > '9' ||
	    value[2] < '0' || value[2] > '9') {
		return -EPROTO;
	}
	if (memcmp(value, "200", 3) == 0) {
		return 0;
	}
	if (memcmp(value, "401", 3) == 0 || memcmp(value, "403", 3) == 0) {
		return -EACCES;
	}
	if (memcmp(value, "429", 3) == 0 || value[0] == '5') {
		return -EAGAIN;
	}
	return -EPROTO;
}

static int read_response_headers(struct control_h2 *h2, uint32_t stream, struct frame_header hdr)
{
	uint8_t padding;
	size_t used = 0;
	int rc = read_padding(h2, &hdr, &padding);

	if (rc != 0) {
		return rc;
	}
	if ((hdr.flags & H2_PRIORITY) != 0) {
		if (hdr.len < 5) {
			return -EPROTO;
		}
		rc = skip_bytes(h2, 5);
		if (rc != 0) {
			return rc;
		}
		hdr.len -= 5;
	}
	for (;;) {
		if (hdr.len > sizeof(h2->frame) - used) {
			return -EMSGSIZE;
		}
		rc = control_noise_read(h2->noise, h2->frame + used, hdr.len);
		if (rc == 0) {
			rc = skip_bytes(h2, padding);
		}
		if (rc != 0) {
			return rc;
		}
		used += hdr.len;
		if ((hdr.flags & H2_END_HEADERS) != 0) {
			break;
		}
		rc = read_header(h2, &hdr);
		if (rc != 0) {
			return rc;
		}
		if (hdr.type != H2_CONTINUATION || hdr.stream != stream) {
			return -EPROTO;
		}
		padding = 0;
	}

	/* The advertised zero-sized table applies before this request is received. */
	for (size_t offset = 0; offset < used;) {
		struct http_hpack_header_buf field;

		rc = http_hpack_decode_header(h2->frame + offset, used - offset, &field, NULL);
		if (rc <= 0) {
			return -EPROTO;
		}
		offset += rc;
		if (field.type == HTTP_HPACK_ENTRY_FIELD && !field.oversized && !field.unresolved &&
		    field.name_len == sizeof(":status") - 1 &&
		    memcmp(field.name, ":status", sizeof(":status") - 1) == 0) {
			return response_status(field.value, field.value_len);
		}
	}
	return -EPROTO;
}

static int update_window(struct control_h2 *h2, uint32_t stream, uint32_t bytes)
{
	uint8_t increment[4];

	sys_put_be32(bytes, increment);
	return write_frame(h2, H2_WINDOW_UPDATE, 0, stream, increment, sizeof(increment));
}

static int read_other_frame(struct control_h2 *h2, const struct frame_header *hdr)
{
	if (hdr->type == H2_SETTINGS) {
		return process_settings(h2, hdr);
	}
	if (hdr->type == H2_PING) {
		uint8_t ping[8];

		if (hdr->len != sizeof(ping) || hdr->stream != 0) {
			return -EPROTO;
		}
		int rc = control_noise_read(h2->noise, ping, sizeof(ping));

		if (rc == 0 && (hdr->flags & H2_ACK) == 0) {
			rc = write_frame(h2, H2_PING, H2_ACK, 0, ping, sizeof(ping));
		}
		return rc;
	}
	return skip_bytes(h2, hdr->len);
}

static int read_body_frame(struct control_h2 *h2, uint32_t stream, struct frame_header *hdr)
{
	for (;;) {
		int rc = read_header(h2, hdr);

		if (rc != 0) {
			return rc;
		}

#ifdef CONFIG_TAILSCALE_TRANSPORT
		if (h2->status_stream != 0 && hdr->stream == h2->status_stream &&
		    (hdr->type == H2_HEADERS || hdr->type == H2_DATA ||
		     hdr->type == H2_RST_STREAM)) {
			if (hdr->type == H2_HEADERS && !h2->status_headers) {
				rc = read_response_headers(h2, hdr->stream, *hdr);
				h2->status_headers = true;
			} else if (!(h2->status_headers && hdr->type == H2_DATA && hdr->len == 0)) {
				return -EPROTO;
			}
			if (rc != 0) {
				return rc;
			}
			if (hdr->flags & H2_END_STREAM) {
				h2->status_stream = 0;
			}
			continue;
		}
#endif
		if (hdr->type == H2_GOAWAY ||
		    (hdr->stream == stream && hdr->type == H2_RST_STREAM)) {
			return -ECONNRESET;
		}
		if (hdr->stream == stream && (hdr->type == H2_HEADERS || hdr->type == H2_DATA)) {
			return 0;
		}
		rc = read_other_frame(h2, hdr);
		if (rc != 0) {
			return rc;
		}
	}
}

int control_h2_open_body(struct control_h2 *h2, uint32_t stream, struct control_h2_body *body)
{
	struct frame_header header;
	int rc = read_body_frame(h2, stream, &header);

	if (rc != 0) {
		return rc;
	}
	if (header.type != H2_HEADERS) {
		return -EPROTO;
	}
	rc = read_response_headers(h2, stream, header);
	if (rc == 0) {
		*body = (struct control_h2_body){
			.stream = stream, .end_stream = (header.flags & H2_END_STREAM) != 0,
		};
	}
	return rc;
}

static int finish_body_frame(struct control_h2 *h2, const struct control_h2_body *body)
{
	int rc = skip_bytes(h2, body->padding);

	if (rc == 0 && body->credit > 0U) {
		rc = update_window(h2, 0, body->credit);
	}
	if (rc == 0 && body->credit > 0U && !body->end_stream) {
		rc = update_window(h2, body->stream, body->credit);
	}
	return rc;
}

int control_h2_read_body(struct control_h2 *h2, struct control_h2_body *body, const uint8_t **data,
			 size_t *len)
{
	size_t count;
	int rc;

	*data = NULL;
	*len = 0;
	while (body->remaining == 0U) {
		struct frame_header header;

		if (body->end_stream) {
			return 0;
		}
		rc = read_body_frame(h2, body->stream, &header);
		if (rc != 0) {
			return rc;
		}
		if (header.type != H2_DATA) {
			return -EPROTO;
		}
		body->end_stream = (header.flags & H2_END_STREAM) != 0;
		body->credit = header.len;
		rc = read_padding(h2, &header, &body->padding);
		if (rc != 0) {
			return rc;
		}
		body->remaining = header.len;
		if (body->remaining == 0U) {
			rc = finish_body_frame(h2, body);
			if (rc != 0) {
				return rc;
			}
		}
	}

	count = MIN(body->remaining, sizeof(h2->frame));
	rc = control_noise_read(h2->noise, h2->frame, count);
	if (rc != 0) {
		return rc;
	}
	body->remaining -= count;
	if (body->remaining == 0U) {
		rc = finish_body_frame(h2, body);
	}
	if (rc == 0) {
		*data = h2->frame;
		*len = count;
	}
	return rc;
}

int control_h2_publish_endpoint(struct control_h2 *h2, uint32_t stream,
				const struct control_h2_map_request *request, uint16_t home,
				char *scratch, size_t capacity)
{
	static const char publish_json[] =
		"{\"Version\":" CONTROL_VERSION_STRING ",\"NodeKey\":\"nodekey:%s\","
		"\"DiscoKey\":\"discokey:%s\",\"Stream\":false,\"OmitPeers\":true,"
		"\"Hostinfo\":{\"Hostname\":\"%s\",\"OS\":\"linux\",\"OSVersion\":\"Zephyr\"%s},"
		"\"Endpoints\":[%s],\"EndpointTypes\":[%s]}";
	const bool proxied = IS_ENABLED(CONFIG_TAILSCALE_REQUIRED_PROXY);
	const char *const addresses[] = {
		proxied ? NULL : request->local_ipv4,
		proxied ? NULL : request->local_ipv6,
		request->public_endpoint,
	};
	char endpoints[3 * (NET_INET6_ADDRSTRLEN + sizeof("[]:65535,") + 2)] = "";
	char types[2 * ARRAY_SIZE(addresses)] = "";
	size_t used = 0U;
	size_t type_count = 0U;

	for (size_t i = 0; i < ARRAY_SIZE(addresses); i++) {
		if (addresses[i] == NULL) {
			continue;
		}
		int n = snprintf(endpoints + used, sizeof(endpoints) - used, "%s\"%s\"",
				 used == 0U ? "" : ",", addresses[i]);

		if (n < 0 || (size_t)n >= sizeof(endpoints) - used) {
			return -EMSGSIZE;
		}
		if (type_count != 0U) {
			types[type_count++] = ',';
		}
		types[type_count++] = i == ARRAY_SIZE(addresses) - 1U ? '2' : '1';
		used += n;
	}
	char netinfo[48] = "";

	if (home != 0U || stream != 3U) {
		snprintf(netinfo, sizeof(netinfo), ",\"NetInfo\":{\"PreferredDERP\":%u}", home);
	}
	int size = snprintf(scratch, capacity, publish_json, request->node_hex, request->disco_hex,
			    request->hostname, netinfo, endpoints, types);

	if (size < 0 || size >= capacity) {
		return -EMSGSIZE;
	}
	return control_h2_request(h2, stream, "/machine/map", request->node_hex, scratch, size);
}

int control_h2_watch_open(struct control_h2 *h2, const struct control_h2_map_request *request,
			  struct control_h2_body *body, char *scratch, size_t capacity)
{
	static const char watch_json[] =
		"{\"Version\":" CONTROL_VERSION_STRING ",\"NodeKey\":\"nodekey:%s\","
		"\"DiscoKey\":\"discokey:%s\",\"Stream\":true,\"KeepAlive\":true,"
		"\"Hostinfo\":{\"Hostname\":\"%s\",\"OS\":\"linux\",\"OSVersion\":\"Zephyr\"}}";
	const uint8_t *data;
	size_t len;
	size_t discarded = 0U;
	int size;
	int rc;

	/* A streaming request is read-only at this capability version. Publish
	 * the endpoint first using the separate status-only operation.
	 */
	rc = control_h2_publish_endpoint(h2, 3, request, 0, scratch, capacity);
	if (rc == 0) {
		rc = control_h2_open_body(h2, 3, body);
	}
	if (rc != 0) {
		return rc;
	}
	do {
		rc = control_h2_read_body(h2, body, &data, &len);
		if (rc != 0) {
			return rc;
		}
		discarded += len;
		if (discarded > capacity) {
			return -EMSGSIZE;
		}
	} while (len != 0U);

	size = snprintf(scratch, capacity, watch_json, request->node_hex, request->disco_hex,
			request->hostname);
	if (size < 0 || size >= capacity) {
		return -EMSGSIZE;
	}
	rc = control_h2_request(h2, 5, "/machine/map", request->node_hex, scratch, size);
	return rc == 0 ? control_h2_open_body(h2, 5, body) : rc;
}

int control_h2_response(struct control_h2 *h2, uint32_t stream, char *body, size_t cap,
			size_t *body_len)
{
	bool headers = false;
	size_t used = 0;

	for (;;) {
		struct frame_header hdr;
		int rc = read_header(h2, &hdr);

		if (rc != 0) {
			return rc;
		}
		if (hdr.type == H2_GOAWAY || (hdr.type == H2_RST_STREAM && hdr.stream == stream)) {
			return -ECONNRESET;
		}
		bool end_stream = (hdr.flags & H2_END_STREAM) != 0;

		if (hdr.type == H2_HEADERS && hdr.stream == stream && !headers) {
			rc = read_response_headers(h2, stream, hdr);
			headers = true;
		} else if (hdr.type == H2_DATA && hdr.stream == stream && headers) {
			uint32_t credit = hdr.len;
			uint8_t padding;

			rc = read_padding(h2, &hdr, &padding);
			if (rc != 0) {
				return rc;
			}
			if (hdr.len >= cap - used) {
				return -EMSGSIZE;
			}
			rc = control_noise_read(h2->noise, body + used, hdr.len);
			used += hdr.len;
			if (rc == 0) {
				rc = skip_bytes(h2, padding);
			}
			if (rc == 0 && credit > 0) {
				rc = update_window(h2, 0, credit);
			}
			if (rc == 0 && credit > 0 && !end_stream) {
				rc = update_window(h2, stream, credit);
			}
		} else {
			rc = read_other_frame(h2, &hdr);
		}
		if (rc != 0) {
			return rc;
		}
		if (hdr.stream == stream && end_stream &&
		    (hdr.type == H2_HEADERS || hdr.type == H2_DATA)) {
			if (used == 0) {
				return -ENODATA;
			}
			body[used] = '\0';
			*body_len = used;
			return 0;
		}
	}
}
