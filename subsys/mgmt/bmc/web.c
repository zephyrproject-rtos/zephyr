/*
 * HTTP web landing page
 *
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024, Nordic Semiconductor
 *
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/data/json.h>
#include <stdio.h>

#include "power.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_webui, LOG_LEVEL_INF);

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static const uint8_t logo_jpeg_gz[] = {
#include "logo.jpeg.gz.inc"
};

static struct http_resource_detail_static logo_jpeg_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "image/jpeg",
		},
	.static_data = logo_jpeg_gz,
	.static_data_len = sizeof(logo_jpeg_gz),
};

static const uint8_t favicon_png_gz[] = {
#include "favicon.png.gz.inc"
};

static struct http_resource_detail_static favicon_png_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "image/png",
		},
	.static_data = favicon_png_gz,
	.static_data_len = sizeof(favicon_png_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, http_service, "/",
		     &index_html_gz_resource_detail);
HTTP_RESOURCE_DEFINE(logo_jpeg_gz_resource, http_service, "/logo.jpeg",
		     &logo_jpeg_gz_resource_detail);
HTTP_RESOURCE_DEFINE(favicon_png_gz_resource, http_service, "/favicon.png",
		     &favicon_png_gz_resource_detail);
#if defined(CONFIG_BMC_APP_HTTPS)
HTTP_RESOURCE_DEFINE(index_html_gz_resource_https, https_service, "/",
		     &index_html_gz_resource_detail);
HTTP_RESOURCE_DEFINE(logo_jpeg_gz_resource_https, https_service, "/logo.jpeg",
		     &logo_jpeg_gz_resource_detail);
HTTP_RESOURCE_DEFINE(favicon_png_gz_resource_https, https_service, "/favicon.png",
		     &favicon_png_gz_resource_detail);
#endif /* defined(CONFIG_BMC_APP_HTTPS) */
