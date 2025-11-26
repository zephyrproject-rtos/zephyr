/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "web_server.h"
#include "rest_api.h"

LOG_MODULE_REGISTER(web_server, LOG_LEVEL_DBG);

/* Enhanced HTML content with working REST API integration */
static const char index_html[] = 
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "    <title>OpenThread Border Router - REST API Dashboard</title>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <style>\n"
    "        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n"
    "        .container { max-width: 1200px; margin: 0 auto; }\n"
    "        .header { background: #2c3e50; color: white; padding: 20px; text-align: center; border-radius: 8px; margin-bottom: 20px; }\n"
    "        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }\n"
    "        .card { background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); overflow: hidden; }\n"
    "        .card-header { background: #34495e; color: white; padding: 15px; font-weight: bold; }\n"
    "        .card-content { padding: 20px; }\n"
    "        .button { background: #3498db; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }\n"
    "        .button:hover { background: #2980b9; }\n"
    "        .button.success { background: #27ae60; }\n"
    "        .button.danger { background: #e74c3c; }\n"
    "        pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; font-size: 12px; }\n"
    "        .loading { color: #7f8c8d; font-style: italic; }\n"
    "        .error { color: #e74c3c; font-weight: bold; }\n"
    "        .status-online { color: #27ae60; font-weight: bold; }\n"
    "        .status-offline { color: #e74c3c; font-weight: bold; }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"container\">\n"
    "        <div class=\"header\">\n"
    "            <h1>🌐 OpenThread Border Router</h1>\n"
    "            <p>REST API Dashboard & Control Panel</p>\n"
    "            <div id=\"connection-status\" class=\"status-online\">🟢 Connected</div>\n"
    "        </div>\n"
    "        <div class=\"grid\">\n"
    "            <div class=\"card\">\n"
    "                <div class=\"card-header\">�� Control Panel</div>\n"
    "                <div class=\"card-content\">\n"
    "                    <button class=\"button success\" onclick=\"startThread()\">▶️ Start Thread</button>\n"
    "                    <button class=\"button danger\" onclick=\"stopThread()\">⏹️ Stop Thread</button>\n"
    "                    <button class=\"button\" onclick=\"refreshAll()\">🔄 Refresh All</button>\n"
    "                    <div id=\"control-result\" style=\"margin-top: 10px;\"></div>\n"
    "                </div>\n"
    "            </div>\n"
    "            <div class=\"card\">\n"
    "                <div class=\"card-header\">📊 Border Router Status</div>\n"
    "                <div class=\"card-content\">\n"
    "                    <div id=\"status-content\" class=\"loading\">Loading status...</div>\n"
    "                </div>\n"
    "            </div>\n"
    "            <div class=\"card\">\n"
    "                <div class=\"card-header\">🔌 Available REST APIs</div>\n"
    "                <div class=\"card-content\">\n"
    "                    <div><strong>GET /api/status</strong> - Border router status</div>\n"
    "                    <div><strong>GET /api/thread/start</strong> - Start Thread network</div>\n"
    "                    <div><strong>GET /api/thread/stop</strong> - Stop Thread network</div>\n"
    "                    <br>\n"
    "                    <button class=\"button\" onclick=\"testAPI('/api/status')\">Test Status API</button>\n"
    "                </div>\n"
    "            </div>\n"
    "        </div>\n"
    "    </div>\n"
    "    <script>\n"
    "        function fetchAPI(url, method = 'GET') {\n"
    "            return fetch(url, { method: method })\n"
    "                .then(response => {\n"
    "                    if (response.headers.get('content-type')?.includes('application/json')) {\n"
    "                        return response.json();\n"
    "                    } else {\n"
    "                        return response.text().then(text => {\n"
    "                            try { return JSON.parse(text); } catch(e) { return {data: text}; }\n"
    "                        });\n"
    "                    }\n"
    "                })\n"
    "                .catch(error => ({ error: error.message }));\n"
    "        }\n"
    "        function refreshStatus() {\n"
    "            fetchAPI('/api/status').then(data => {\n"
    "                document.getElementById('status-content').innerHTML = '<pre>' + JSON.stringify(data, null, 2) + '</pre>';\n"
    "                updateConnectionStatus(true);\n"
    "            }).catch(error => {\n"
    "                document.getElementById('status-content').innerHTML = '<div class=\"error\">Failed to load status</div>';\n"
    "                updateConnectionStatus(false);\n"
    "            });\n"
    "        }\n"
    "        function updateConnectionStatus(connected) {\n"
    "            const statusEl = document.getElementById('connection-status');\n"
    "            if (connected) {\n"
    "                statusEl.innerHTML = '�� Connected';\n"
    "                statusEl.className = 'status-online';\n"
    "            } else {\n"
    "                statusEl.innerHTML = '🔴 Disconnected';\n"
    "                statusEl.className = 'status-offline';\n"
    "            }\n"
    "        }\n"
    "        function refreshAll() { refreshStatus(); }\n"
    "        function startThread() {\n"
    "            fetchAPI('/api/thread/start').then(data => {\n"
    "                document.getElementById('control-result').innerHTML = '<pre>' + JSON.stringify(data, null, 2) + '</pre>';\n"
    "                setTimeout(refreshStatus, 1000);\n"
    "            });\n"
    "        }\n"
    "        function stopThread() {\n"
    "            fetchAPI('/api/thread/stop').then(data => {\n"
    "                document.getElementById('control-result').innerHTML = '<pre>' + JSON.stringify(data, null, 2) + '</pre>';\n"
    "                setTimeout(refreshStatus, 1000);\n"
    "            });\n"
    "        }\n"
    "        function testAPI(endpoint) {\n"
    "            fetchAPI(endpoint).then(data => {\n"
    "                alert('API Test Result:\\n' + JSON.stringify(data, null, 2));\n"
    "            });\n"
    "        }\n"
    "        setInterval(refreshStatus, 10000);\n"
    "        refreshStatus();\n"
    "    </script>\n"
    "</body>\n"
    "</html>";

/* Static resource detail for index page */
static struct http_resource_detail_static index_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_type = "text/html",
    },
    .static_data = index_html,
    .static_data_len = sizeof(index_html) - 1,
};

/* Dynamic resource details for APIs - using functions from rest_api.c */
static struct http_resource_detail_dynamic api_status_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
    },
    .cb = rest_api_status_handler,
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

/* Define HTTP service */
static uint16_t border_router_service_port = 8080;
HTTP_SERVICE_DEFINE(border_router_service, NULL, &border_router_service_port,
                    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

/* Define HTTP resources */
HTTP_RESOURCE_DEFINE(index_resource, border_router_service, "/", &index_resource_detail);
HTTP_RESOURCE_DEFINE(api_status_resource, border_router_service, "/api/status", &api_status_detail);
HTTP_RESOURCE_DEFINE(api_start_resource, border_router_service, "/api/thread/start", &api_start_detail);
HTTP_RESOURCE_DEFINE(api_stop_resource, border_router_service, "/api/thread/stop", &api_stop_detail);

int web_server_init(void)
{
    int ret;

    LOG_INF("Initializing web server on port %d", border_router_service_port);

    /* Initialize REST API */
    ret = rest_api_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize REST API: %d", ret);
        return ret;
    }

    /* Start the HTTP server */
    ret = http_server_start();
    if (ret < 0) {
        LOG_ERR("Failed to start HTTP service: %d", ret);
        return ret;
    }

    LOG_INF("HTTP server started successfully");
    LOG_INF("Web interface available at: http://<device-ip>:%d/", border_router_service_port);
    LOG_INF("REST API endpoints:");
    LOG_INF("  GET  /api/status         - Border router status");
    LOG_INF("  GET  /api/thread/start   - Start Thread network");
    LOG_INF("  GET  /api/thread/stop    - Stop Thread network");

    return 0;
}
