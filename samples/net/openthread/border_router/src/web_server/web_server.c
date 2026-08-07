/*
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <string.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/mdns_responder.h>
#include "web_server.h"
#include "rest_api.h"

LOG_MODULE_REGISTER(web_server, CONFIG_OTBR_LOG_LEVEL);

/* mDNS service configuration */
static const uint16_t http_port = 8080;

static const struct dns_sd_rec otbr_http_service = {
	.instance = CONFIG_NET_HOSTNAME,
	.service = "_http",
	.proto = "_tcp",
	.domain = "local",
	.text = DNS_SD_EMPTY_TXT,
	.text_size = 0,
	.port = &http_port,
};

static struct net_mgmt_event_callback l4_cb;
static K_SEM_DEFINE(network_connected, 0, 1);

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");
		k_sem_give(&network_connected);
	}
}

/* HTML */
static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

/* CSS */
static uint8_t style_css_gz[] = {
#include "style.css.gz.inc"
};

/* JavaScript */
static uint8_t app_js_gz[] = {
#include "app.js.gz.inc"
};

/* Logo Left */
static uint8_t vendor_logo_png[] = {
#include "images/vendor_logo.png.gz.inc"
};
/* Logo Right */
static uint8_t zephyr_logo_png[] = {
#include "images/zephyr_logo.png.gz.inc"
};

/* Static resource detail for index page */
static struct http_resource_detail_static index_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/html",
	},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz) - 1,
};

static struct http_resource_detail_static style_css_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/css",
	},
	.static_data = style_css_gz,
	.static_data_len = sizeof(style_css_gz) - 1,
};

static struct http_resource_detail_static app_js_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "application/javascript",
	},
	.static_data = app_js_gz,
	.static_data_len = sizeof(app_js_gz) - 1,
};

static struct http_resource_detail_static vendor_logo_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "image/png",
	},
	.static_data = vendor_logo_png,
	.static_data_len = sizeof(vendor_logo_png) - 1,
};

static struct http_resource_detail_static zephyr_logo_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "image/png",
	},
	.static_data = zephyr_logo_png,
	.static_data_len = sizeof(zephyr_logo_png) - 1,
};

/* Dynamic resource details for APIs */
static struct http_resource_detail_dynamic api_status_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = rest_api_status_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_dataset_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = rest_api_dataset_get_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_config_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = rest_api_network_config_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_start_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = rest_api_thread_start_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_stop_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = rest_api_thread_stop_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_dataset_hex_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = rest_api_dataset_hex_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_topology_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = rest_api_topology_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_node_info_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = rest_api_node_info_handler,
	.user_data = NULL,
};

static struct http_resource_detail_dynamic api_node_diagnostics_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = rest_api_node_diagnostics_handler,
	.user_data = NULL,
};


/* web server state */
static bool web_server_started;

/* Define HTTP service */
static uint16_t border_router_service_port = 8080;
HTTP_SERVICE_DEFINE(border_router_service, NULL, &border_router_service_port,
		   CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

/* Define HTTP resources */
HTTP_RESOURCE_DEFINE(index_resource, border_router_service, "/",
		    &index_resource_detail);

HTTP_RESOURCE_DEFINE(style_css_resource, border_router_service, "/style.css",
		    &style_css_detail);
HTTP_RESOURCE_DEFINE(app_js_resource, border_router_service, "/app.js",
		    &app_js_detail);
HTTP_RESOURCE_DEFINE(vendor_logo_resource, border_router_service, "/images/vendor_logo.png",
		    &vendor_logo_detail);
HTTP_RESOURCE_DEFINE(zephyr_logo_resource, border_router_service, "/images/zephyr_logo.png",
		    &zephyr_logo_detail);
HTTP_RESOURCE_DEFINE(api_status_resource, border_router_service, "/api/status",
		    &api_status_detail);
HTTP_RESOURCE_DEFINE(api_dataset_resource, border_router_service, "/api/dataset",
		    &api_dataset_detail);
HTTP_RESOURCE_DEFINE(api_config_resource, border_router_service, "/api/network/config",
		    &api_config_detail);
HTTP_RESOURCE_DEFINE(api_start_resource, border_router_service, "/api/thread/start",
		    &api_start_detail);
HTTP_RESOURCE_DEFINE(api_stop_resource, border_router_service, "/api/thread/stop",
		    &api_stop_detail);
HTTP_RESOURCE_DEFINE(api_dataset_hex_resource, border_router_service, "/api/dataset/hex",
		    &api_dataset_hex_detail);
HTTP_RESOURCE_DEFINE(api_topology_resource, border_router_service, "/api/topology",
		    &api_topology_detail);
HTTP_RESOURCE_DEFINE(api_node_info_resource, border_router_service, "/api/node/info",
		    &api_node_info_detail);
HTTP_RESOURCE_DEFINE(api_node_diagnostics_resource, border_router_service, "/api/node/diagnostics",
		    &api_node_diagnostics_detail);

static void mdns_register_delayed(struct k_work *work)
{
	int ret;

	ret = mdns_responder_set_ext_records(&otbr_http_service, 1);
	if (ret < 0) {
		LOG_WRN("Failed to register mDNS service: %d", ret);
	} else {
		LOG_INF("mDNS service discoverable at: http://%s:%d/",
				otbr_http_service.instance, border_router_service_port);
	}
}

K_WORK_DEFINE(mdns_work, mdns_register_delayed);

int web_server_setup(void)
{
	LOG_INF("Setting up web server...");

	/* Register for L4 connected event */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler,
				     NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&l4_cb);

	/* Wait for network to be ready */
	LOG_INF("Waiting for network connection...");
	k_sem_take(&network_connected, K_FOREVER);

	/* Network is ready, start web server */
	LOG_INF("Network ready, starting web server");
	return web_server_init();
}

int web_server_init(void)
{
	int ret;

	if (!web_server_started) {
		LOG_DBG("Initializing web server on port %d", border_router_service_port);

		/* Start the HTTP server */
		ret = http_server_start();
		if (ret != 0) {
			LOG_ERR("Failed to start HTTP service: %d", ret);
			return ret;
		}

		/* Schedule mDNS registration */
		k_work_submit(&mdns_work);

		web_server_started = true;
		LOG_INF("HTTP server started successfully");

		LOG_DBG("REST API endpoints:");
		LOG_DBG("  GET  /api/status		- Border router status");
		LOG_DBG("  GET  /api/dataset		- Get current network dataset");
		LOG_DBG("  POST /api/network/config	- Configure Thread network");
		LOG_DBG("  GET  /api/thread/start	- Start Thread network");
		LOG_DBG("  GET  /api/thread/stop	- Stop Thread network");
		LOG_DBG("  GET  /api/dataset/hex	- Get dataset in hex format\");\n");
		LOG_DBG("  GET  /api/topology		- Get network topology\");\n");
		LOG_DBG("  GET  /api/node/info		- Get node information\");\n");
		LOG_DBG("  POST /api/node/diagnostics	- Get node network diagnostics");
	} else {
		LOG_INF("Web Server already started");
	}

	return 0;
}
