/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/shell/shell_websocket.h>

static const char index_html_gz[] = {
#include "index.html.gz.inc"
};

struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static const char style_css_gz[] = {
#include "style.css.gz.inc"
};

struct http_resource_detail_static style_css_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
		},
	.static_data = style_css_gz,
	.static_data_len = sizeof(style_css_gz),
};

static const char favicon_16x16_png_gz[] = {
#include "favicon-16x16.png.gz.inc"
};

struct http_resource_detail_static favicon_16x16_png_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
		},
	.static_data = favicon_16x16_png_gz,
	.static_data_len = sizeof(favicon_16x16_png_gz),
};

#if defined(CONFIG_NET_SAMPLE_HTTPS_SERVICE)
#include <zephyr/net/tls_credentials.h>
#include "../certificate.h"

static const sec_tag_t sec_tag_list_verify_none[] = {
		SERVER_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
		PSK_TAG,
#endif
	};

#define SEC_TAG_LIST sec_tag_list_verify_none
#define SEC_TAG_LIST_LEN sizeof(sec_tag_list_verify_none)
#else
#define SEC_TAG_LIST NULL
#define SEC_TAG_LIST_LEN 0
#endif /* CONFIG_NET_SAMPLE_HTTPS_SERVICE */

WEBSOCKET_CONSOLE_DEFINE(ws_console_service, SEC_TAG_LIST,
			 SEC_TAG_LIST_LEN);

HTTP_RESOURCE_DEFINE(root_resource, ws_console_service, "/",
		     &index_html_gz_resource_detail);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, ws_console_service, "/index.html",
		     &index_html_gz_resource_detail);

HTTP_RESOURCE_DEFINE(style_css_gz_resource, ws_console_service, "/style.css",
		     &style_css_gz_resource_detail);

HTTP_RESOURCE_DEFINE(favicon_16x16_png_gz_resource, ws_console_service,
		     "/favicon-16x16.png",
		     &favicon_16x16_png_gz_resource_detail);

int init_ws(void)
{
	int ret;

	WEBSOCKET_CONSOLE_ENABLE(ws_console_service);

	ret = http_server_start();
	if (ret < 0) {
		LOG_DBG("Cannot start websocket console (%d)", ret);
	} else {
		LOG_DBG("Starting websocket console");
	}

	return 0;
}
