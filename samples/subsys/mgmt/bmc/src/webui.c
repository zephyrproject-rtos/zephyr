/*
 * The product dashboard. Everything here is policy: the pages, the branding
 * and the feature advertisement are attached to the BMC HTTP services from the
 * application, without the BMC core knowing about them.
 *
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024, Nordic Semiconductor
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc/http.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>

LOG_MODULE_REGISTER(bmc_webui, LOG_LEVEL_INF);

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static struct http_resource_detail_static index_html_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/html",
	},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

BMC_HTTP_RESOURCE_DEFINE(webui_index, "/", &index_html_detail);

static const uint8_t logo_jpeg_gz[] = {
#include "logo.jpeg.gz.inc"
};

static struct http_resource_detail_static logo_jpeg_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "image/jpeg",
	},
	.static_data = logo_jpeg_gz,
	.static_data_len = sizeof(logo_jpeg_gz),
};

BMC_HTTP_RESOURCE_DEFINE(webui_logo, "/logo.jpeg", &logo_jpeg_detail);

static const uint8_t favicon_png_gz[] = {
#include "favicon.png.gz.inc"
};

static struct http_resource_detail_static favicon_png_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "image/png",
	},
	.static_data = favicon_png_gz,
	.static_data_len = sizeof(favicon_png_gz),
};

BMC_HTTP_RESOURCE_DEFINE(webui_favicon, "/favicon.png", &favicon_png_detail);

#if defined(BMC_SAMPLE_WEB_VENDOR_ASSETS)
/*
 * The browser libraries the dashboard uses are served from the BMC rather than
 * from a CDN, so that it also works on an isolated management network. They
 * are an optional build input, see web/README.md.
 */
#define WEBUI_VENDOR_RESOURCE(_sym, _url, _content_type)                                           \
	static struct http_resource_detail_static _sym##_detail = {                                \
		.common = {                                                                        \
			.type = HTTP_RESOURCE_TYPE_STATIC,                                         \
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),                        \
			.content_encoding = "gzip",                                                \
			.content_type = _content_type,                                             \
		},                                                                                 \
		.static_data = _sym##_gz,                                                          \
		.static_data_len = sizeof(_sym##_gz),                                              \
	};                                                                                         \
	BMC_HTTP_RESOURCE_DEFINE(_sym, _url, &_sym##_detail)

static const uint8_t webui_xterm_css_gz[] = {
#include "xterm.min.css.gz.inc"
};
WEBUI_VENDOR_RESOURCE(webui_xterm_css, "/vendor/xterm.min.css", "text/css");

static const uint8_t webui_xterm_js_gz[] = {
#include "xterm.min.js.gz.inc"
};
WEBUI_VENDOR_RESOURCE(webui_xterm_js, "/vendor/xterm.min.js", "text/javascript");

static const uint8_t webui_xterm_attach_js_gz[] = {
#include "xterm-addon-attach.min.js.gz.inc"
};
WEBUI_VENDOR_RESOURCE(webui_xterm_attach_js, "/vendor/xterm-addon-attach.min.js",
		      "text/javascript");

static const uint8_t webui_xterm_fit_js_gz[] = {
#include "xterm-addon-fit.min.js.gz.inc"
};
WEBUI_VENDOR_RESOURCE(webui_xterm_fit_js, "/vendor/xterm-addon-fit.min.js", "text/javascript");

static const uint8_t webui_chart_js_gz[] = {
#include "chart.umd.min.js.gz.inc"
};
WEBUI_VENDOR_RESOURCE(webui_chart_js, "/vendor/chart.umd.min.js", "text/javascript");
#endif /* BMC_SAMPLE_WEB_VENDOR_ASSETS */

#if defined(CONFIG_BMC_CONSOLE_BRIDGE_WS)
#define WEBUI_HOST_CONSOLE "true"
#else
#define WEBUI_HOST_CONSOLE "false"
#endif

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
#define WEBUI_BMC_SHELL "true"
#else
#define WEBUI_BMC_SHELL "false"
#endif

/*
 * The dashboard asks which optional consoles this build carries so that it can
 * hide the tabs it cannot use. Both answers are known at build time, so the
 * body is a constant and no per-request buffer is needed.
 */
static const char webui_features_json[] =
	"{\"hostConsole\":" WEBUI_HOST_CONSOLE ",\"bmcShell\":" WEBUI_BMC_SHELL "}";

static int webui_features_handler(struct http_client_ctx *client,
				  enum http_transaction_status status,
				  const struct http_request_ctx *request_ctx,
				  struct http_response_ctx *response_ctx, void *user_data)
{
	ARG_UNUSED(client);
	ARG_UNUSED(request_ctx);
	ARG_UNUSED(user_data);

	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		return 0;
	}

	response_ctx->status = HTTP_200_OK;
	response_ctx->body = webui_features_json;
	response_ctx->body_len = sizeof(webui_features_json) - 1;
	response_ctx->final_chunk = true;

	return 0;
}

static struct http_resource_detail_dynamic webui_features_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_type = "application/json",
	},
	.cb = webui_features_handler,
	.user_data = NULL,
};

BMC_HTTP_RESOURCE_DEFINE(webui_features, "/webui/features", &webui_features_detail);
