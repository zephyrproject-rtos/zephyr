/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/parser_url.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/mgmt/fota_http.h>

LOG_MODULE_REGISTER(fota_http, CONFIG_FOTA_HTTP_LOG_LEVEL);

#define FOTA_HTTP_HOST_MAX         128
#define FOTA_HTTP_PORT_MAX         8
#define FOTA_HTTP_URL_MAX          CONFIG_FOTA_HTTP_URL_MAX_LEN
#define FOTA_HTTP_HEADER_NAME_MAX  16
#define FOTA_HTTP_RANGE_HEADER_MAX 48
#define FOTA_HTTP_PROGRESS_KEY     "fota_http/progress"
#define FOTA_HTTP_SOURCE_KEY       "fota_http/source"
#define FOTA_HTTP_VALIDATOR_MAX    64
#define FOTA_HTTP_IMAGE_MAGIC      0x96f3b83dU

/* The leading fields of an MCUboot v1 image header as stored in flash. */
struct fota_http_raw_header {
	uint32_t magic;
	uint32_t load_addr;
	uint16_t hdr_size;
	uint16_t protect_tlv_size;
	uint32_t img_size;
};

struct fota_http_source {
	uint32_t url_hash;
	uint8_t image_index;
	char validator[FOTA_HTTP_VALIDATOR_MAX];
};

struct fota_http_url {
	char host[FOTA_HTTP_HOST_MAX];
	char host_hdr[FOTA_HTTP_HOST_MAX + 2];
	char path[FOTA_HTTP_URL_MAX];
	char port[FOTA_HTTP_PORT_MAX];
	bool tls;
};

struct fota_http_ctx {
	struct flash_img_context flash;
	struct fota_http_progress progress;
	struct fota_http_download_params params;
	struct fota_http_url url;
	char url_buf[FOTA_HTTP_URL_MAX];
	char location[FOTA_HTTP_URL_MAX];
	size_t location_len;
	char header_name[FOTA_HTTP_HEADER_NAME_MAX];
	size_t header_name_len;
	bool header_name_done;
	bool header_is_location;
	bool header_is_range;
	char content_range[FOTA_HTTP_RANGE_HEADER_MAX];
	size_t content_range_len;
	bool header_is_etag;
	bool header_is_modified;
	char etag[FOTA_HTTP_VALIDATOR_MAX];
	size_t etag_len;
	char last_modified[FOTA_HTTP_VALIDATOR_MAX];
	size_t last_modified_len;
	struct fota_http_source source;
	size_t resume_offset;
	size_t saved_offset;
	int area_id;
	int err;
	bool redirect;
	bool restart;
	bool headers_done;
};

static struct fota_http_ctx ctx;
static uint8_t recv_buf[CONFIG_FOTA_HTTP_RECV_BUF_SIZE];
static atomic_t busy;
static atomic_t cancel_requested;

static K_KERNEL_STACK_DEFINE(fota_http_stack, CONFIG_FOTA_HTTP_THREAD_STACK_SIZE);
static struct k_work_q fota_http_work_q;
static struct k_work async_work;
static struct fota_http_download_params async_params;
static char async_url[FOTA_HTTP_URL_MAX];
static fota_http_done_cb_t async_done_cb;
static bool work_q_started;

#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 2
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot3_partition), "Image 1 has no secondary slot partition");
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 3
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot5_partition), "Image 2 has no secondary slot partition");
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 4
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot7_partition), "Image 3 has no secondary slot partition");
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 5
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot9_partition), "Image 4 has no secondary slot partition");
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 6
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot11_partition), "Image 5 has no secondary slot partition");
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 7
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot13_partition), "Image 6 has no secondary slot partition");
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 8
BUILD_ASSERT(FIXED_PARTITION_EXISTS(slot15_partition), "Image 7 has no secondary slot partition");
#endif

static int upload_area_id(uint8_t image_index)
{
	switch (image_index) {
	case 0:
		return flash_img_get_upload_slot();
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 2
	case 1:
		return FIXED_PARTITION_ID(slot3_partition);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 3
	case 2:
		return FIXED_PARTITION_ID(slot5_partition);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 4
	case 3:
		return FIXED_PARTITION_ID(slot7_partition);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 5
	case 4:
		return FIXED_PARTITION_ID(slot9_partition);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 6
	case 5:
		return FIXED_PARTITION_ID(slot11_partition);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 7
	case 6:
		return FIXED_PARTITION_ID(slot13_partition);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER >= 8
	case 7:
		return FIXED_PARTITION_ID(slot15_partition);
#endif
	default:
		return -ENODEV;
	}
}

static int copy_field(const char *url, const struct http_parser_url *u,
		      enum http_parser_url_fields field, char *dst, size_t dst_len)
{
	uint16_t off = u->field_data[field].off;
	uint16_t len = u->field_data[field].len;

	if (len >= dst_len) {
		return -EINVAL;
	}

	memcpy(dst, url + off, len);
	dst[len] = '\0';

	return 0;
}

static int parse_url(const char *url, struct fota_http_url *out)
{
	struct http_parser_url u;
	size_t path_len;
	int ret;

	http_parser_url_init(&u);
	if (http_parser_parse_url(url, strlen(url), 0, &u) != 0) {
		return -EINVAL;
	}

	if (!(u.field_set & BIT(UF_HOST))) {
		return -EINVAL;
	}

	ret = copy_field(url, &u, UF_HOST, out->host, sizeof(out->host));
	if (ret != 0) {
		return ret;
	}

	out->tls = false;
	if (u.field_set & BIT(UF_SCHEMA)) {
		const char *schema = url + u.field_data[UF_SCHEMA].off;
		uint16_t len = u.field_data[UF_SCHEMA].len;

		if (len == 5 && strncmp(schema, "https", len) == 0) {
			out->tls = true;
		} else if (len != 4 || strncmp(schema, "http", len) != 0) {
			return -ENOTSUP;
		}
	}

	if (out->tls && !IS_ENABLED(CONFIG_FOTA_HTTP_TLS)) {
		return -ENOTSUP;
	}

	if (u.field_set & BIT(UF_PORT)) {
		snprintk(out->port, sizeof(out->port), "%u", u.port);
	} else {
		strcpy(out->port, out->tls ? "443" : "80");
	}

	if (u.field_set & BIT(UF_PATH)) {
		ret = copy_field(url, &u, UF_PATH, out->path, sizeof(out->path));
		if (ret != 0) {
			return ret;
		}
	} else {
		strcpy(out->path, "/");
	}

	if (strchr(out->host, ':') != NULL) {
		snprintk(out->host_hdr, sizeof(out->host_hdr), "[%s]", out->host);
	} else {
		strcpy(out->host_hdr, out->host);
	}

	if (u.field_set & BIT(UF_QUERY)) {
		path_len = strlen(out->path);
		if (path_len + 1 >= sizeof(out->path)) {
			return -EINVAL;
		}
		out->path[path_len++] = '?';
		ret = copy_field(url, &u, UF_QUERY, out->path + path_len,
				 sizeof(out->path) - path_len);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#if defined(CONFIG_FOTA_HTTP_TLS)
static int open_tls_socket(const struct fota_http_ctx *dl, int family, int type)
{
	sec_tag_t default_tags[] = {CONFIG_FOTA_HTTP_TLS_SEC_TAG};
	const sec_tag_t *tags = default_tags;
	size_t tags_len = sizeof(default_tags);
	const char *hostname = dl->url.host;
	int verify = CONFIG_FOTA_HTTP_TLS_PEER_VERIFY;
	int proto = IS_ENABLED(CONFIG_FOTA_HTTP_TLS_VERSION_1_3) ? NET_IPPROTO_TLS_1_3
								 : NET_IPPROTO_TLS_1_2;
	int sock;

	if (dl->params.sec_tags != NULL && dl->params.sec_tag_count > 0) {
		tags = dl->params.sec_tags;
		tags_len = dl->params.sec_tag_count * sizeof(sec_tag_t);
	}

	if (dl->params.tls_hostname != NULL) {
		hostname = dl->params.tls_hostname;
	}

	if (verify == ZSOCK_TLS_PEER_VERIFY_NONE) {
		LOG_WRN("TLS peer verification is disabled");
	}

	sock = zsock_socket(family, type, proto);
	if (sock < 0) {
		return -errno;
	}

	if (zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, tags, tags_len) < 0 ||
	    zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, hostname,
			     strlen(hostname) + 1) < 0 ||
	    zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify, sizeof(verify)) <
		    0) {
		int err = -errno;

		LOG_ERR("Failed to configure TLS socket (%d)", err);
		zsock_close(sock);
		return err;
	}

	return sock;
}
#else
static int open_tls_socket(const struct fota_http_ctx *dl, int family, int type)
{
	ARG_UNUSED(dl);
	ARG_UNUSED(family);
	ARG_UNUSED(type);

	return -ENOTSUP;
}
#endif /* CONFIG_FOTA_HTTP_TLS */

static int connect_socket(const struct fota_http_ctx *dl)
{
	const struct fota_http_url *url = &dl->url;
	struct zsock_addrinfo hints = {
		.ai_family = NET_AF_UNSPEC,
		.ai_socktype = NET_SOCK_STREAM,
	};
	struct zsock_addrinfo *res = NULL;
	int sock;
	int ret;

	ret = zsock_getaddrinfo(url->host, url->port, &hints, &res);
	if (ret != 0 || res == NULL) {
		LOG_ERR("Failed to resolve %s (%d)", url->host, ret);
		return -EHOSTUNREACH;
	}

	if (url->tls) {
		sock = open_tls_socket(dl, res->ai_family, res->ai_socktype);
	} else {
		sock = zsock_socket(res->ai_family, res->ai_socktype, NET_IPPROTO_TCP);
		if (sock < 0) {
			sock = -errno;
		}
	}

	if (sock < 0) {
		LOG_ERR("Failed to create socket (%d)", sock);
		zsock_freeaddrinfo(res);
		return sock;
	}

	ret = zsock_connect(sock, res->ai_addr, res->ai_addrlen);
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Failed to connect to %s:%s (%d)", url->host, url->port, ret);
		zsock_close(sock);
		zsock_freeaddrinfo(res);
		return ret;
	}

	zsock_freeaddrinfo(res);

	return sock;
}

static bool is_redirect(int status)
{
	return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

#if defined(CONFIG_FOTA_HTTP_RESUME)
static uint32_t url_hash(const char *url)
{
	uint32_t hash = 2166136261U;

	while (*url != '\0') {
		hash ^= (uint8_t)*url++;
		hash *= 16777619U;
	}

	return hash;
}

struct source_load {
	struct fota_http_source src;
	bool loaded;
};

static int source_load_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			  void *param)
{
	struct source_load *load = param;

	if (settings_name_next(key, NULL) != 0 || len != sizeof(load->src)) {
		return 0;
	}

	if (read_cb(cb_arg, &load->src, sizeof(load->src)) == sizeof(load->src)) {
		load->loaded = true;
	}

	return 0;
}

static bool source_matches(struct fota_http_ctx *dl)
{
	struct source_load load = {0};

	(void)settings_load_subtree_direct(FOTA_HTTP_SOURCE_KEY, source_load_cb, &load);
	if (!load.loaded || load.src.url_hash != url_hash(dl->params.url) ||
	    load.src.image_index != dl->params.image_index) {
		return false;
	}

	memcpy(&dl->source, &load.src, sizeof(dl->source));

	return true;
}

static void source_save(struct fota_http_ctx *dl)
{
	struct fota_http_source src = {
		.url_hash = url_hash(dl->params.url),
		.image_index = dl->params.image_index,
	};
	const char *validator = (dl->etag_len > 0) ? dl->etag : dl->last_modified;
	size_t len = (dl->etag_len > 0) ? dl->etag_len : dl->last_modified_len;

	if (len > 0) {
		memcpy(src.validator, validator, len);
	}

	(void)settings_save_one(FOTA_HTTP_SOURCE_KEY, &src, sizeof(src));
}
#endif /* CONFIG_FOTA_HTTP_RESUME */

static void progress_clear(struct fota_http_ctx *dl)
{
#if defined(CONFIG_FOTA_HTTP_RESUME)
	(void)stream_flash_progress_clear(&dl->flash.stream, FOTA_HTTP_PROGRESS_KEY);
	(void)settings_delete(FOTA_HTTP_SOURCE_KEY);
#else
	ARG_UNUSED(dl);
#endif
}

static int progress_save(struct fota_http_ctx *dl, bool force)
{
#if defined(CONFIG_FOTA_HTTP_RESUME)
	int ret;

	if (!force &&
	    dl->progress.written - dl->saved_offset < CONFIG_FOTA_HTTP_RESUME_SAVE_INTERVAL) {
		return 0;
	}

	/* Without a validator a resume cannot tell that the file changed on the
	 * server, so drop the progress and fetch the image again next time.
	 */
	if (dl->etag_len >= FOTA_HTTP_VALIDATOR_MAX ||
	    dl->last_modified_len >= FOTA_HTTP_VALIDATOR_MAX) {
		LOG_WRN("Validator too long, progress will not be resumed");
		progress_clear(dl);
		return 0;
	}

	ret = stream_flash_progress_save(&dl->flash.stream, FOTA_HTTP_PROGRESS_KEY);
	if (ret != 0) {
		LOG_WRN("Failed to save download progress (%d)", ret);
		return ret;
	}

	source_save(dl);
	dl->saved_offset = dl->progress.written;
#else
	ARG_UNUSED(dl);
	ARG_UNUSED(force);
#endif

	return 0;
}

static int restart_from_zero(struct fota_http_ctx *dl)
{
	int ret;

	progress_clear(dl);
	dl->resume_offset = 0;
	dl->saved_offset = 0;
	dl->progress.written = 0;
	dl->progress.total = 0;

	ret = flash_img_init_id(&dl->flash, dl->area_id);
	if (ret != 0) {
		LOG_ERR("Failed to reinitialize flash image context (%d)", ret);
	}

	return ret;
}

static bool header_is(const struct fota_http_ctx *dl, const char *name)
{
	size_t len = strlen(name);

	return dl->header_name_len == len && strncasecmp(dl->header_name, name, len) == 0;
}

static void append_value(char *buf, size_t buf_len, size_t *used, const char *at, size_t length)
{
	if (*used + length < buf_len) {
		memcpy(buf + *used, at, length);
		*used += length;
		buf[*used] = '\0';
	} else {
		*used = buf_len;
	}
}

static int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	struct fota_http_ctx *dl = &ctx;

	ARG_UNUSED(parser);

	if (dl->header_name_done) {
		dl->header_name_done = false;
		dl->header_name_len = 0;
		dl->header_is_location = false;
		dl->header_is_range = false;
		dl->header_is_etag = false;
		dl->header_is_modified = false;
	}

	if (dl->header_name_len + length < sizeof(dl->header_name)) {
		memcpy(dl->header_name + dl->header_name_len, at, length);
		dl->header_name_len += length;
	} else {
		dl->header_name_len = sizeof(dl->header_name);
	}

	return 0;
}

static int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
	struct fota_http_ctx *dl = &ctx;

	ARG_UNUSED(parser);

	if (!dl->header_name_done) {
		dl->header_name_done = true;
		dl->header_is_location = header_is(dl, "Location");
		dl->header_is_range = header_is(dl, "Content-Range");
		if (dl->header_is_location) {
			dl->location_len = 0;
		}
		if (dl->header_is_range) {
			dl->content_range_len = 0;
		}
		dl->header_is_etag = header_is(dl, "ETag");
		dl->header_is_modified = header_is(dl, "Last-Modified");
		if (dl->header_is_etag) {
			dl->etag_len = 0;
		}
		if (dl->header_is_modified) {
			dl->last_modified_len = 0;
		}
	}

	if (dl->header_is_location) {
		append_value(dl->location, sizeof(dl->location), &dl->location_len, at, length);
	} else if (dl->header_is_range) {
		append_value(dl->content_range, sizeof(dl->content_range), &dl->content_range_len,
			     at, length);
	} else if (dl->header_is_etag) {
		append_value(dl->etag, sizeof(dl->etag), &dl->etag_len, at, length);
	} else if (dl->header_is_modified) {
		append_value(dl->last_modified, sizeof(dl->last_modified), &dl->last_modified_len,
			     at, length);
	}

	return 0;
}

static int parse_content_range(const struct fota_http_ctx *dl, size_t *start, size_t *total)
{
	const char *p = dl->content_range;
	char *end;

	if (dl->content_range_len == 0 || dl->content_range_len >= sizeof(dl->content_range) ||
	    strncmp(p, "bytes ", 6) != 0) {
		return -EBADMSG;
	}

	p += 6;
	*start = strtoul(p, &end, 10);
	if (end == p || *end != '-') {
		return -EBADMSG;
	}

	p = strchr(end, '/');
	if (p == NULL) {
		return -EBADMSG;
	}

	*total = (p[1] == '*') ? 0 : strtoul(p + 1, NULL, 10);

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	ctx.headers_done = true;

	return 0;
}

static const struct http_parser_settings parser_cb = {
	.on_header_field = on_header_field,
	.on_header_value = on_header_value,
	.on_headers_complete = on_headers_complete,
};

static int check_response(struct fota_http_ctx *dl, const struct http_response *rsp)
{
	int status = rsp->http_status_code;
	size_t start;
	size_t total;
	int ret;

	dl->progress.http_status = status;
	ret = 0;

	if (is_redirect(status)) {
		dl->redirect = true;

		return -ECANCELED;
	}

	if (status == 206 && dl->resume_offset > 0) {
		ret = parse_content_range(dl, &start, &total);
		if (ret != 0 || start != dl->resume_offset) {
			LOG_ERR("Unexpected Content-Range for offset %zu", dl->resume_offset);
			return -EBADMSG;
		}
		if (dl->progress.total == 0) {
			dl->progress.total = total;
		}
		ret = 0;
	} else if (status == 200) {
		if (dl->resume_offset > 0) {
			LOG_WRN("Server ignored the Range request, restarting from zero");
			ret = restart_from_zero(dl);
			if (ret != 0) {
				return ret;
			}
		}
		if (dl->progress.total == 0) {
			dl->progress.total = rsp->content_length;
		}
		ret = 0;
	} else if (status == 416 && dl->resume_offset > 0) {
		/* The file shrank below the saved offset, so the range is gone
		 * and the whole image has to be fetched again.
		 */
		LOG_WRN("Saved offset is past the end of the file, restarting from zero");
		ret = restart_from_zero(dl);
		if (ret != 0) {
			return ret;
		}
		dl->restart = true;
		ret = -ECANCELED;
	} else {
		LOG_ERR("Server answered %d", status);
		ret = -EBADMSG;
	}

	return ret;
}

static int response_cb(struct http_response *rsp, enum http_final_call final, void *user_data)
{
	struct fota_http_ctx *dl = user_data;
	int ret;

	if (dl->err != 0 || dl->redirect || dl->restart) {
		return -ECANCELED;
	}

	if (atomic_get(&cancel_requested) != 0) {
		dl->err = -ECANCELED;
		return -ECANCELED;
	}

	if (dl->progress.http_status == 0) {
		if (!dl->headers_done) {
			return 0;
		}

		ret = check_response(dl, rsp);
		if (ret != 0) {
			if (!dl->redirect && !dl->restart) {
				dl->err = ret;
			}
			return -ECANCELED;
		}
	}

	if (rsp->body_frag_len > 0) {
		ret = flash_img_buffered_write(&dl->flash, rsp->body_frag_start, rsp->body_frag_len,
					       false);
		if (ret != 0) {
			LOG_ERR("Flash write failed at %zu bytes (%d)", dl->progress.written, ret);
			dl->err = ret;
			return -ECANCELED;
		}
	}

	if (final == HTTP_DATA_FINAL) {
		ret = flash_img_buffered_write(&dl->flash, NULL, 0, true);
		if (ret != 0) {
			LOG_ERR("Flash flush failed (%d)", ret);
			dl->err = ret;
			return -ECANCELED;
		}
	}

	if (rsp->body_frag_len > 0 || final == HTTP_DATA_FINAL) {
		dl->progress.written = flash_img_bytes_written(&dl->flash);
		(void)progress_save(dl, false);

		if (dl->params.progress_cb != NULL) {
			dl->params.progress_cb(&dl->progress, dl->params.user_data);
		}
	}

	return 0;
}

static int follow_redirect(struct fota_http_ctx *dl)
{
	int len;

	if (dl->location_len == 0 || dl->location_len >= sizeof(dl->location)) {
		LOG_ERR("Redirect without a usable Location header");
		return -EBADMSG;
	}

	if (dl->location[0] == '/') {
		len = snprintk(dl->url_buf, sizeof(dl->url_buf), "%s://%s:%s%s",
			       dl->url.tls ? "https" : "http", dl->url.host_hdr, dl->url.port,
			       dl->location);
		if (len < 0 || (size_t)len >= sizeof(dl->url_buf)) {
			return -EINVAL;
		}
	} else {
		strcpy(dl->url_buf, dl->location);
	}

	LOG_INF("Redirected to %s", dl->url_buf);

	return 0;
}

static int do_request(struct fota_http_ctx *dl)
{
	struct http_request req;
	char range_header[FOTA_HTTP_RANGE_HEADER_MAX];
	char if_range_header[FOTA_HTTP_VALIDATOR_MAX + 16];
	const char *range_headers[3] = {NULL, NULL, NULL};
	int sock;
	int ret;

	ret = parse_url(dl->url_buf, &dl->url);
	if (ret != 0) {
		LOG_ERR("Invalid URL %s (%d)", dl->url_buf, ret);
		return ret;
	}

	dl->redirect = false;
	dl->restart = false;
	dl->headers_done = false;
	dl->progress.http_status = 0;
	dl->location_len = 0;
	dl->header_name_len = 0;
	dl->header_name_done = false;
	dl->header_is_location = false;
	dl->header_is_range = false;
	dl->content_range_len = 0;
	dl->header_is_etag = false;
	dl->header_is_modified = false;
	dl->etag_len = 0;
	dl->last_modified_len = 0;

	sock = connect_socket(dl);
	if (sock < 0) {
		return sock;
	}

	LOG_INF("Downloading %s from %s:%s", dl->url.path, dl->url.host, dl->url.port);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = dl->url.path;
	req.host = dl->url.host_hdr;
	req.port = dl->url.port;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.http_cb = &parser_cb;
	req.header_fields = dl->params.headers;
	req.recv_buf = recv_buf;
	req.recv_buf_len = sizeof(recv_buf);

	if (dl->resume_offset > 0) {
		snprintk(range_header, sizeof(range_header), "Range: bytes=%zu-\r\n",
			 dl->resume_offset);
		range_headers[0] = range_header;
		if (dl->source.validator[0] != '\0') {
			snprintk(if_range_header, sizeof(if_range_header), "If-Range: %s\r\n",
				 dl->source.validator);
			range_headers[1] = if_range_header;
		}
		req.optional_headers = range_headers;
		LOG_INF("Resuming at %zu bytes", dl->resume_offset);
	}

	ret = http_client_req(sock, &req, CONFIG_FOTA_HTTP_TIMEOUT_MS, dl);
	zsock_close(sock);

	if (ret == -ECONNABORTED && (dl->err != 0 || dl->redirect || dl->restart)) {
		return 0;
	}

	if (ret < 0) {
		LOG_ERR("HTTP request failed (%d)", ret);
	}

	return ret < 0 ? ret : 0;
}

/* A response without a Content-Length gives no total to compare the
 * download against, so derive a lower bound from the image header: the
 * header, the payload and the protected TLVs must all be present. The
 * unprotected TLVs are not counted; MCUboot rejects an image cut inside
 * them when it verifies the signature.
 */
static int check_length(struct fota_http_ctx *dl)
{
	const struct flash_area *fa;
	struct fota_http_raw_header hdr;
	size_t min_len;
	int ret;

	ret = flash_area_open(dl->area_id, &fa);
	if (ret != 0) {
		return ret;
	}

	ret = flash_area_read(fa, 0, &hdr, sizeof(hdr));
	flash_area_close(fa);
	if (ret != 0) {
		return ret;
	}

	if (sys_le32_to_cpu(hdr.magic) != FOTA_HTTP_IMAGE_MAGIC) {
		/* Not an image at all, check_image() reports that. */
		return 0;
	}

	min_len = (size_t)sys_le16_to_cpu(hdr.hdr_size) + sys_le32_to_cpu(hdr.img_size) +
		  sys_le16_to_cpu(hdr.protect_tlv_size);
	if (dl->progress.written < min_len) {
		LOG_ERR("Truncated image: %zu of at least %zu bytes", dl->progress.written,
			min_len);
		return -EMSGSIZE;
	}

	return 0;
}

static int check_image(struct fota_http_ctx *dl)
{
	struct mcuboot_img_header header;
	int ret;

#if defined(CONFIG_FOTA_HTTP_SHA256_CHECK)
	if (dl->params.sha256 != NULL) {
		struct flash_img_check fic = {
			.match = dl->params.sha256,
			.clen = dl->progress.written,
		};

		ret = flash_img_check(&dl->flash, &fic, dl->area_id);
		if (ret != 0) {
			LOG_ERR("SHA-256 check failed (%d)", ret);
			return ret;
		}
	}
#else
	if (dl->params.sha256 != NULL) {
		return -ENOTSUP;
	}
#endif

	ret = boot_read_bank_header(dl->area_id, &header, sizeof(header));
	if (ret != 0) {
		LOG_ERR("Downloaded data is not an MCUboot image (%d)", ret);
		return -ENOEXEC;
	}

	LOG_INF("Image version %u.%u.%u+%u, %u bytes", header.h.v1.sem_ver.major,
		header.h.v1.sem_ver.minor, header.h.v1.sem_ver.revision,
		header.h.v1.sem_ver.build_num, header.h.v1.image_size);

#if defined(CONFIG_FOTA_HTTP_REJECT_DOWNGRADE)
	if (dl->params.image_index == 0) {
		struct mcuboot_img_header running;
		const struct mcuboot_img_sem_ver *new = &header.h.v1.sem_ver;
		const struct mcuboot_img_sem_ver *cur;

		ret = boot_read_bank_header(boot_fetch_active_slot(), &running, sizeof(running));
		if (ret == 0) {
			cur = &running.h.v1.sem_ver;
			if (new->major < cur->major ||
			    (new->major == cur->major && new->minor < cur->minor) ||
			    (new->major == cur->major && new->minor == cur->minor &&
			     new->revision < cur->revision) ||
			    (new->major == cur->major && new->minor == cur->minor &&
			     new->revision == cur->revision && new->build_num < cur->build_num)) {
				LOG_ERR("Image is older than the running %u.%u.%u+%u", cur->major,
					cur->minor, cur->revision, cur->build_num);
				return -EPERM;
			}
		}
	}
#endif

	return 0;
}

static int check_params(const struct fota_http_download_params *params)
{
	if (params == NULL || params->url == NULL) {
		return -EINVAL;
	}

	if (strlen(params->url) >= FOTA_HTTP_URL_MAX) {
		return -EINVAL;
	}

	if (params->resume && !IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		return -ENOTSUP;
	}

	if (params->sha256 != NULL && !IS_ENABLED(CONFIG_FOTA_HTTP_SHA256_CHECK)) {
		return -ENOTSUP;
	}

	if (!boot_is_img_confirmed()) {
		LOG_ERR("Running image is not confirmed, the secondary slot holds its fallback");
		return -EPERM;
	}

	return 0;
}

static int download_locked(const struct fota_http_download_params *params)
{
	struct fota_http_ctx *dl = &ctx;
	unsigned int redirects = 0;
	int ret;

	memset(dl, 0, sizeof(*dl));
	dl->params = *params;
	strcpy(dl->url_buf, params->url);

	dl->area_id = upload_area_id(params->image_index);
	if (dl->area_id < 0) {
		ret = dl->area_id;
		goto out;
	}

	ret = flash_img_init_id(&dl->flash, dl->area_id);
	if (ret != 0) {
		LOG_ERR("Failed to initialize flash image context (%d)", ret);
		goto out;
	}

#if defined(CONFIG_FOTA_HTTP_RESUME)
	if (params->resume) {
		ret = stream_flash_progress_load(&dl->flash.stream, FOTA_HTTP_PROGRESS_KEY);
		if (ret != 0) {
			LOG_WRN("No download progress to resume (%d)", ret);
		} else if (!source_matches(dl)) {
			LOG_WRN("Saved progress belongs to another download, starting over");
			progress_clear(dl);
			ret = flash_img_init_id(&dl->flash, dl->area_id);
			if (ret != 0) {
				goto out;
			}
		}
		dl->resume_offset = flash_img_bytes_written(&dl->flash);
		dl->saved_offset = dl->resume_offset;
		dl->progress.written = dl->resume_offset;
	} else {
		progress_clear(dl);
	}
#endif

	while (true) {
		ret = do_request(dl);
		if (ret != 0) {
			break;
		}

		if (dl->err != 0) {
			ret = dl->err;
			break;
		}

		if (dl->restart) {
			continue;
		}

		if (!dl->redirect) {
			break;
		}

		if (redirects >= CONFIG_FOTA_HTTP_MAX_REDIRECTS) {
			LOG_ERR("Too many redirects");
			ret = -ELOOP;
			break;
		}
		redirects++;

		ret = follow_redirect(dl);
		if (ret != 0) {
			break;
		}
	}

	if (ret != 0) {
		(void)progress_save(dl, true);
		if ((ret == -ECONNRESET || ret == -EBADMSG) && dl->progress.total != 0 &&
		    dl->progress.written > 0 && dl->progress.written < dl->progress.total) {
			LOG_ERR("Connection lost after %zu of %zu bytes (%d)", dl->progress.written,
				dl->progress.total, ret);
			ret = -EMSGSIZE;
		}
		goto out;
	}

	if (dl->progress.written == 0) {
		LOG_ERR("Server sent an empty body");
		ret = -ENODATA;
		goto out;
	}

	if (dl->progress.total != 0 && dl->progress.written != dl->progress.total) {
		LOG_ERR("Truncated transfer: %zu of %zu bytes", dl->progress.written,
			dl->progress.total);
		(void)progress_save(dl, true);
		ret = -EMSGSIZE;
		goto out;
	}

	ret = check_length(dl);
	if (ret != 0) {
		(void)progress_save(dl, true);
		goto out;
	}

	progress_clear(dl);

	ret = check_image(dl);
	if (ret != 0) {
		goto out;
	}

	LOG_INF("Wrote %zu bytes to flash area %d", dl->progress.written, dl->area_id);

out:
	return ret;
}

int fota_http_download(const struct fota_http_download_params *params)
{
	int ret;

	ret = check_params(params);
	if (ret != 0) {
		return ret;
	}

	if (!atomic_cas(&busy, 0, 1)) {
		return -EBUSY;
	}

	atomic_clear(&cancel_requested);
	ret = download_locked(params);
	atomic_clear(&busy);

	return ret;
}

static void async_work_handler(struct k_work *work)
{
	fota_http_done_cb_t done_cb = async_done_cb;
	void *user_data = async_params.user_data;
	int ret;

	ARG_UNUSED(work);

	ret = download_locked(&async_params);
	atomic_clear(&busy);

	if (done_cb != NULL) {
		done_cb(ret, user_data);
	}
}

int fota_http_download_async(const struct fota_http_download_params *params,
			     fota_http_done_cb_t done_cb)
{
	int ret;

	ret = check_params(params);
	if (ret != 0) {
		return ret;
	}

	if (!atomic_cas(&busy, 0, 1)) {
		return -EBUSY;
	}

	atomic_clear(&cancel_requested);

	if (!work_q_started) {
		k_work_init(&async_work, async_work_handler);
		k_work_queue_start(&fota_http_work_q, fota_http_stack,
				   K_KERNEL_STACK_SIZEOF(fota_http_stack),
				   CONFIG_FOTA_HTTP_THREAD_PRIORITY, NULL);
		k_thread_name_set(k_work_queue_thread_get(&fota_http_work_q), "fota_http");
		work_q_started = true;
	}

	async_params = *params;
	strcpy(async_url, params->url);
	async_params.url = async_url;
	async_done_cb = done_cb;
	k_work_submit_to_queue(&fota_http_work_q, &async_work);

	return 0;
}

int fota_http_cancel(void)
{
	if (atomic_get(&busy) == 0) {
		return -EALREADY;
	}

	atomic_set(&cancel_requested, 1);

	return 0;
}

int fota_http_apply(uint8_t image_index, bool permanent)
{
	struct mcuboot_img_header header;
	int area_id;
	int ret;

	area_id = upload_area_id(image_index);
	if (area_id < 0) {
		return area_id;
	}

	ret = boot_read_bank_header(area_id, &header, sizeof(header));
	if (ret != 0) {
		LOG_ERR("No valid image in flash area %d (%d)", area_id, ret);
		return -ENOEXEC;
	}

	ret = boot_request_upgrade_multi(image_index,
					 permanent ? BOOT_UPGRADE_PERMANENT : BOOT_UPGRADE_TEST);
	if (ret != 0) {
		LOG_ERR("Failed to request upgrade (%d)", ret);
		return ret;
	}

	LOG_INF("Upgrade requested (%s)", permanent ? "permanent" : "test");

	return 0;
}

bool fota_http_confirm_pending(void)
{
	return !boot_is_img_confirmed();
}

int fota_http_upload_slot(uint8_t image_index)
{
	return upload_area_id(image_index);
}

int fota_http_image_header(uint8_t image_index, struct mcuboot_img_header *header)
{
	int area_id;

	if (header == NULL) {
		return -EINVAL;
	}

	area_id = upload_area_id(image_index);
	if (area_id < 0) {
		return area_id;
	}

	return boot_read_bank_header(area_id, header, sizeof(*header));
}

size_t fota_http_bytes_written(void)
{
	return ctx.progress.written;
}

#if defined(CONFIG_FOTA_HTTP_TLS)
static int add_credential(enum tls_credential_type type, const void *cred, size_t len)
{
	int ret;

	ret = tls_credential_add(CONFIG_FOTA_HTTP_TLS_SEC_TAG, type, cred, len);
	if (ret != 0 && ret != -EEXIST) {
		LOG_ERR("Failed to register credential type %d (%d)", type, ret);
		return ret;
	}

	return 0;
}

int fota_http_tls_add_ca(const void *cert, size_t len)
{
	return add_credential(TLS_CREDENTIAL_CA_CERTIFICATE, cert, len);
}

int fota_http_tls_add_client_cert(const void *cert, size_t cert_len, const void *key,
				  size_t key_len)
{
	int ret;

	ret = add_credential(TLS_CREDENTIAL_PUBLIC_CERTIFICATE, cert, cert_len);
	if (ret != 0) {
		return ret;
	}

	return add_credential(TLS_CREDENTIAL_PRIVATE_KEY, key, key_len);
}
#endif /* CONFIG_FOTA_HTTP_TLS */

int fota_http_confirm(uint8_t image_index)
{
	int ret;

	if (image_index >= CONFIG_UPDATEABLE_IMAGE_NUMBER) {
		return -ENODEV;
	}

	if (CONFIG_UPDATEABLE_IMAGE_NUMBER > 1) {
		ret = boot_write_img_confirmed_multi(image_index);
	} else {
		ret = boot_write_img_confirmed();
	}

	if (ret != 0) {
		LOG_ERR("Failed to confirm image (%d)", ret);
		return ret;
	}

	LOG_INF("Image confirmed");

	return 0;
}
