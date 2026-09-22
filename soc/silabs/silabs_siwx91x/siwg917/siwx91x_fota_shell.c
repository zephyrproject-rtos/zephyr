/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/parser_url.h>
#include <zephyr/net/http/client.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#include <sl_wifi.h>
#include <firmware_upgradation.h>

LOG_MODULE_REGISTER(siwx91x_ota, LOG_LEVEL_INF);

struct fota_ctx {
	/* Internal buffer to store URL components with \0 between them */
	char bufurl[512];
	const char *host;
	const char *path;
	const char *port;
	bool use_tls;

	const struct shell *sh;
	int socket;
	int range_start;
	size_t image_size;
	int http_status;

	/* http_client requires a temporary buffers for the http responces (headers + body) */
	uint8_t http_buffer[1024];
	/* Larger buffers require fewer HTTP requests, so downloading the payload is faster. */
	uint8_t flash_buffer[8 * 1024];
	int flash_buffer_len;
};

static int fota_get_url_field(const char *url, const struct http_parser_url *parser,
			     enum http_parser_url_fields field, char *out, int outsize)
{
	if (parser->field_data[field].len + 1 > outsize) {
		return -ENOMEM;
	}
	if (!(parser->field_set & BIT(field))) {
		return -EINVAL;
	}
	memcpy(out, url + parser->field_data[field].off, parser->field_data[field].len);
	out[parser->field_data[field].len] = '\0';
	return parser->field_data[field].len + 1;
}

static int fota_cook_url(struct fota_ctx *ctx, const char *url)
{
	struct http_parser_url parser;
	char schema[9];
	char *bufptr = ctx->bufurl;
	int bufsize = sizeof(ctx->bufurl);
	int ret;

	http_parser_url_init(&parser);
	ret = http_parser_parse_url(url, strlen(url), 0, &parser);
	if (ret < 0) {
		return -EINVAL;
	}

	ret = fota_get_url_field(url, &parser, UF_SCHEMA, schema, sizeof(schema));
	if (ret < 0) {
		return ret;
	}
	if (!strcmp(schema, "https") && IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		ctx->use_tls = true;
	} else if (!strcmp(schema, "http")) {
		ctx->use_tls = false;
	} else {
		return -EINVAL;
	}

	ret = fota_get_url_field(url, &parser, UF_HOST, bufptr, bufsize);
	if (ret < 0) {
		return ret;
	}
	ctx->host = bufptr;
	bufptr += ret;
	bufsize -= ret;

	ret = fota_get_url_field(url, &parser, UF_PATH, bufptr, bufsize);
	if (ret < 0 && ret != -EINVAL) {
		return ret;
	}
	if (ret != -EINVAL) {
		ctx->path = bufptr;
		bufptr += ret;
		bufsize -= ret;
	} else {
		ctx->path = "/";
	}

	ret = fota_get_url_field(url, &parser, UF_PORT, bufptr, bufsize);
	if (ret < 0 && ret != -EINVAL) {
		return ret;
	}
	if (ret != -EINVAL) {
		ctx->port = bufptr;
	} else if (ctx->use_tls) {
		ctx->port = "443";
	} else {
		ctx->port = "80";
	}

	return 0;
}

static int fota_http_connect(const struct shell *sh, const char *host,
			    const char *port, bool use_tls)
{
	struct zsock_addrinfo *res = NULL;
	int peer_verify = ZSOCK_TLS_PEER_VERIFY_NONE;
	int fd = -1;
	int ret;

	shell_print(sh, "Connecting to %s:%s", host, port);
	ret = zsock_getaddrinfo(host, port, NULL, &res);
	if (ret < 0) {
		shell_error(sh, "Cannot resolve %s:%s: %s", host, port, zsock_gai_strerror(ret));
		return -EINVAL;
	}
	if (use_tls) {
		fd = zsock_socket(res->ai_addr->sa_family, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	} else {
		fd = zsock_socket(res->ai_addr->sa_family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	}
	if (fd < 0) {
		shell_error(sh, "Cannot open socket: %s", strerror(errno));
		zsock_freeaddrinfo(res);
		return -errno;
	}
	if (use_tls) {
		/* For development, it is easier to not check identity of the remote. This should be
		 * changed for production
		 */
		ret = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
				       &peer_verify, sizeof(peer_verify));
		if (ret < 0) {
			shell_error(sh, "Cannot configure TLS: %s", strerror(errno));
			zsock_close(fd);
			zsock_freeaddrinfo(res);
			return -errno;
		}
	}
	ret = zsock_connect(fd, res->ai_addr, res->ai_addrlen);
	if (ret < 0) {
		shell_error(sh, "Cannot connect to %s:%s: %s", host, port, strerror(errno));
		zsock_close(fd);
		zsock_freeaddrinfo(res);
		return -errno;
	}
	zsock_freeaddrinfo(res);
	return fd;
}

static int fota_http_response_cb(struct http_response *rsp, enum http_final_call final_data,
				void *user_data)
{
	struct fota_ctx *ctx = user_data;

	if (rsp->http_status_code / 10 != 200 / 10) {
		ctx->http_status = rsp->http_status_code;
		return -EIO;
	}
	ctx->http_status = 0;
	if (rsp->content_range.total == 0 || rsp->content_range.start != ctx->range_start) {
		shell_error(ctx->sh, "Server returned an unexpected byte range");
		return -EIO;
	}
	if (!ctx->range_start) {
		ctx->image_size = rsp->content_range.total;
	}
	if (ctx->image_size != rsp->content_range.total) {
		shell_error(ctx->sh, "Server changed image size mid-transfer");
		return -EIO;
	}
	if (ctx->flash_buffer_len + (int)rsp->body_frag_len > (int)sizeof(ctx->flash_buffer)) {
		shell_error(ctx->sh, "Server sent more bytes than expected for chunk");
		return -EIO;
	}
	memcpy(ctx->flash_buffer + ctx->flash_buffer_len,
	       rsp->body_frag_start, rsp->body_frag_len);
	ctx->flash_buffer_len += rsp->body_frag_len;
	return 0;
}

static int fota_recv_chunk(struct fota_ctx *ctx)
{
	char range_header[64];
	char const *headers[] = { range_header, NULL };
	struct http_request req = {
		.method = HTTP_GET,
		.protocol = "HTTP/1.1",
		.header_fields = headers,
		.host = ctx->host,
		.url = ctx->path,
		.response = fota_http_response_cb,
		.recv_buf = ctx->http_buffer,
		.recv_buf_len = sizeof(ctx->http_buffer),
	};
	int range_end;
	int ret;

	ctx->flash_buffer_len = 0;
	range_end = ctx->range_start + sizeof(ctx->flash_buffer) - 1;
	if (ctx->range_start) {
		range_end = MIN(range_end, (int)ctx->image_size - 1);
	}
	snprintf(range_header, sizeof(range_header), "Range: bytes=%d-%d\r\n",
		 ctx->range_start, range_end);

	shell_print(ctx->sh, "Downloading bytes %d-%d", ctx->range_start, range_end);
	ret = http_client_req(ctx->socket, &req, 3000, ctx);
	if (ret < 0) {
		shell_error(ctx->sh, "HTTP request failed: %s", strerror(-ret));
		return ret;
	}
	if (ctx->http_status) {
		shell_error(ctx->sh, "HTTP error status: %d", ctx->http_status);
		return -EIO;
	}
	return 0;
}

static int fota_load_chunk(struct fota_ctx *ctx)
{
	const sl_wifi_firmware_header_t *hdr;
	size_t image_size;
	int buf_offset = 0;
	int ret;

	if (!ctx->range_start) {
		hdr = (const sl_wifi_firmware_header_t *)ctx->flash_buffer;
		buf_offset = sizeof(sl_wifi_firmware_header_t);
		/* BIT(7) in control_flags indicates a combined firmware image. */
		if (hdr->control_flags & BIT(7)) {
			image_size = hdr->reserved1[0];
		} else {
			image_size = hdr->image_size + sizeof(sl_wifi_firmware_header_t);
		}
		if (image_size != ctx->image_size) {
			shell_error(ctx->sh, "Corrupted payload (%zu bytes vs %zu expected)",
				    image_size, ctx->image_size);
			return -EINVAL;
		}
		ret = sl_si91x_fwup_start(ctx->flash_buffer);
		if (ret) {
			shell_error(ctx->sh, "Failed to start firmware upgrade: %d", ret);
			return -EIO;
		}
		shell_print(ctx->sh, "Flashing %zu bytes...", ctx->image_size);
	}
	while (buf_offset < ctx->flash_buffer_len) {
		int chunk_len = MIN(SLI_MAX_FWUP_CHUNK_SIZE, ctx->flash_buffer_len - buf_offset);

		ret = sl_si91x_fwup_load(ctx->flash_buffer + buf_offset, chunk_len);
		if (ret == SL_STATUS_SI91X_FW_UPDATE_DONE) {
			shell_print(ctx->sh, "Firmware update complete. Rebooting...");
			/* Give time for the message above to be printed */
			k_msleep(1);
			sys_reboot(SYS_REBOOT_COLD);
		}
		if (ret) {
			shell_error(ctx->sh, "Firmware write failed: %d", ret);
			return -EIO;
		}
		buf_offset += chunk_len;
	}
	return 0;
}

static int cmd_fota(const struct shell *sh, size_t argc, char *argv[])
{
	struct fota_ctx *ctx;
	int ret;

	ctx = calloc(1, sizeof(struct fota_ctx));
	if (!ctx) {
		return -ENOMEM;
	}
	ctx->sh = sh;
	ctx->socket = -1;

	if (strlen(argv[1]) >= sizeof(ctx->bufurl)) {
		shell_error(sh, "URL too long (max %zu characters)", sizeof(ctx->bufurl));
		ret = -EINVAL;
		goto out;
	}

	ret = fota_cook_url(ctx, argv[1]);
	if (ret) {
		shell_error(sh, "Invalid URL: %s", argv[1]);
		goto out;
	}
	while (true) {
		if (ctx->socket < 0) {
			ctx->socket = fota_http_connect(sh, ctx->host, ctx->port, ctx->use_tls);
		}
		if (ctx->socket < 0) {
			ret = ctx->socket;
			break;
		}

		ret = fota_recv_chunk(ctx);
		if (ret == -ECONNABORTED || ret == -ECONNRESET) {
			shell_print(sh, "Connection lost, reconnecting...");
			zsock_close(ctx->socket);
			ctx->socket = -1;
			continue;
		}
		if (ret < 0) {
			break;
		}

		ret = fota_load_chunk(ctx);
		if (ret < 0) {
			break;
		}
		ctx->range_start += ctx->flash_buffer_len;
	}
	zsock_close(ctx->socket);
out:
	free(ctx);
	return ret;
}

SHELL_SUBCMD_ADD((silabs), fota, NULL,
		 SHELL_HELP("Upgrade Firmware Over-The-Air via HTTP(S)", "<URL>"),
		 cmd_fota, 2, 0);
