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

/* Enhanced HTML content with Thread network configuration form */
static const char index_html[] = 
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "    <title>OpenThread Border Router - Configuration</title>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <style>\n"
    "        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n"
    "        .container { max-width: 1400px; margin: 0 auto; }\n"
    "        .header { background: #2c3e50; color: white; padding: 20px; text-align: center; border-radius: 8px; margin-bottom: 20px; }\n"
    "        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 20px; }\n"
    "        .card { background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); overflow: hidden; }\n"
    "        .card-header { background: #34495e; color: white; padding: 15px; font-weight: bold; }\n"
    "        .card-content { padding: 20px; }\n"
    "        .form-group { margin-bottom: 15px; }\n"
    "        .form-group label { display: block; margin-bottom: 5px; font-weight: bold; color: #2c3e50; }\n"
    "        .form-group input, .form-group select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }\n"
    "        .form-group small { color: #7f8c8d; font-size: 12px; }\n"
    "        .button { background: #3498db; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }\n"
    "        .button:hover { background: #2980b9; }\n"
    "        .button.success { background: #27ae60; }\n"
    "        .button.success:hover { background: #229954; }\n"
    "        .button.danger { background: #e74c3c; }\n"
    "        .button.danger:hover { background: #c0392b; }\n"
    "        .button.warning { background: #f39c12; }\n"
    "        .button.warning:hover { background: #d68910; }\n"
    "        pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; font-size: 12px; }\n"
    "        .loading { color: #7f8c8d; font-style: italic; }\n"
    "        .error { color: #e74c3c; font-weight: bold; }\n"
    "        .success-msg { color: #27ae60; font-weight: bold; }\n"
    "        .status-online { color: #27ae60; font-weight: bold; }\n"
    "        .status-offline { color: #e74c3c; font-weight: bold; }\n"
    "        .info-box { background: #e8f4f8; border-left: 4px solid #3498db; padding: 10px; margin: 10px 0; border-radius: 4px; }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"container\">\n"
    "        <div class=\"header\">\n"
    "            <h1>🌐 OpenThread Border Router</h1>\n"
    "            <p>Network Configuration & Control Panel</p>\n"
    "            <div id=\"connection-status\" class=\"status-online\">🟢 Connected</div>\n"
    "        </div>\n"
    "        <div class=\"grid\">\n"
    "            <div class=\"card\">\n"
    "                <div class=\"card-header\">⚙️ Thread Network Configuration</div>\n"
    "                <div class=\"card-content\">\n"
    "                    <form id=\"network-config-form\" onsubmit=\"submitNetworkConfig(event)\">\n"
    "                        <div class=\"form-group\">\n"
    "                            <label for=\"network_name\">Network Name:</label>\n"
    "                            <input type=\"text\" id=\"network_name\" name=\"network_name\" maxlength=\"16\" placeholder=\"MyThreadNetwork\">\n"
    "                            <small>Max 16 characters</small>\n"
    "                        </div>\n"
    "                        <div class=\"form-group\">\n"
    "                            <label for=\"pan_id\">PAN ID:</label>\n"
    "                            <input type=\"text\" id=\"pan_id\" name=\"pan_id\" placeholder=\"0x1234\">\n"
    "                            <small>Hexadecimal format (e.g., 0x1234)</small>\n"
    "                        </div>\n"
    "                        <div class=\"form-group\">\n"
    "                            <label for=\"extpanid\">Extended PAN ID:</label>\n"
    "                            <input type=\"text\" id=\"extpanid\" name=\"extpanid\" placeholder=\"0011223344556677\" maxlength=\"16\">\n"
    "                            <small>16 hex characters (8 bytes)</small>\n"
    "                            <button type=\"button\" class=\"button warning\" onclick=\"generateRandomExtPanId()\" style=\"margin-top: 5px;\">🎲 Generate Random Extended PAN ID</button>\n"
    "                        </div>\n"
    "                        <div class=\"form-group\">\n"
    "                            <label for=\"channel\">Channel:</label>\n"
    "                            <select id=\"channel\" name=\"channel\">\n"
    "                                <option value=\"\">-- Select Channel --</option>\n"
    "                                <option value=\"11\">11 (2405 MHz)</option>\n"
    "                                <option value=\"12\">12 (2410 MHz)</option>\n"
    "                                <option value=\"13\">13 (2415 MHz)</option>\n"
    "                                <option value=\"14\">14 (2420 MHz)</option>\n"
    "                                <option value=\"15\" selected>15 (2425 MHz)</option>\n"
    "                                <option value=\"16\">16 (2430 MHz)</option>\n"
    "                                <option value=\"17\">17 (2435 MHz)</option>\n"
    "                                <option value=\"18\">18 (2440 MHz)</option>\n"
    "                                <option value=\"19\">19 (2445 MHz)</option>\n"
    "                                <option value=\"20\">20 (2450 MHz)</option>\n"
    "                                <option value=\"21\">21 (2455 MHz)</option>\n"
    "                                <option value=\"22\">22 (2460 MHz)</option>\n"
    "                                <option value=\"23\">23 (2465 MHz)</option>\n"
    "                                <option value=\"24\">24 (2470 MHz)</option>\n"
    "                                <option value=\"25\">25 (2475 MHz)</option>\n"
    "                                <option value=\"26\">26 (2480 MHz)</option>\n"
    "                            </select>\n"
    "                            <small>IEEE 802.15.4 channel (11-26)</small>\n"
    "                        </div>\n"
    "                        <div class=\"form-group\">\n"
    "                            <label for=\"network_key\">Network Key:</label>\n"
    "                            <input type=\"text\" id=\"network_key\" name=\"network_key\" placeholder=\"00112233445566778899aabbccddeeff\" maxlength=\"32\">\n"
    "                            <small>32 hex characters (16 bytes)</small>\n"
    "                            <button type=\"button\" class=\"button warning\" onclick=\"generateRandomKey()\" style=\"margin-top: 5px;\">🎲 Generate Random Key</button>\n"
    "                        </div>\n"
    "                        <div class=\"info-box\">\n"
    "                            <strong>ℹ️ Info:</strong> A new dataset will be created with random values. Fill in fields to override specific parameters.\n"
    "                        </div>\n"
    "                        <button type=\"submit\" class=\"button success\" style=\"width: 100%;\">💾 Save Configuration</button>\n"
    "                    </form>\n"
    "                    <div id=\"config-result\" style=\"margin-top: 15px;\"></div>\n"
    "                </div>\n"
    "            </div>\n"
    "            <div class=\"card\">\n"
    "                <div class=\"card-header\">🎛️ Control Panel</div>\n"
    "                <div class=\"card-content\">\n"
    "                    <button class=\"button success\" onclick=\"startThread()\" style=\"width: 100%; margin-bottom: 10px;\">▶️ Start Thread Network</button>\n"
    "                    <button class=\"button danger\" onclick=\"stopThread()\" style=\"width: 100%; margin-bottom: 10px;\">⏹️ Stop Thread Network</button>\n"
    "                    <button class=\"button\" onclick=\"loadCurrentConfig()\" style=\"width: 100%; margin-bottom: 10px;\">📥 Load Current Config</button>\n"
    "                    <button class=\"button\" onclick=\"refreshAll()\" style=\"width: 100%;\">🔄 Refresh Status</button>\n"
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
    "                <div class=\"card-header\">📡 Current Network Dataset</div>\n"
    "                <div class=\"card-content\">\n"
    "                    <div id=\"dataset-content\" class=\"loading\">Loading dataset...</div>\n"
    "                </div>\n"
    "            </div>\n"
    "        </div>\n"
    "    </div>\n"
    "    <script>\n"
    "        function fetchAPI(url, method = 'GET', body = null) {\n"
    "            const options = { method: method };\n"
    "            if (body) {\n"
    "                options.headers = { 'Content-Type': 'application/x-www-form-urlencoded' };\n"
    "                options.body = body;\n"
    "            }\n"
    "            return fetch(url, options)\n"
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
    "        function refreshDataset() {\n"
    "            fetchAPI('/api/dataset').then(data => {\n"
    "                document.getElementById('dataset-content').innerHTML = '<pre>' + JSON.stringify(data, null, 2) + '</pre>';\n"
    "            }).catch(error => {\n"
    "                document.getElementById('dataset-content').innerHTML = '<div class=\"error\">Failed to load dataset</div>';\n"
    "            });\n"
    "        }\n"
    "        function updateConnectionStatus(connected) {\n"
    "            const statusEl = document.getElementById('connection-status');\n"
    "            if (connected) {\n"
    "                statusEl.innerHTML = '🟢 Connected';\n"
    "                statusEl.className = 'status-online';\n"
    "            } else {\n"
    "                statusEl.innerHTML = '🔴 Disconnected';\n"
    "                statusEl.className = 'status-offline';\n"
    "            }\n"
    "        }\n"
    "        function refreshAll() {\n"
    "            refreshStatus();\n"
    "            refreshDataset();\n"
    "        }\n"
    "        function startThread() {\n"
    "            fetchAPI('/api/thread/start').then(data => {\n"
    "                document.getElementById('control-result').innerHTML = '<pre>' + JSON.stringify(data, null, 2) + '</pre>';\n"
    "                setTimeout(refreshAll, 1000);\n"
    "            });\n"
    "        }\n"
    "        function stopThread() {\n"
    "            fetchAPI('/api/thread/stop').then(data => {\n"
    "                document.getElementById('control-result').innerHTML = '<pre>' + JSON.stringify(data, null, 2) + '</pre>';\n"
    "                setTimeout(refreshAll, 1000);\n"
    "            });\n"
    "        }\n"
    "        function submitNetworkConfig(event) {\n"
    "            event.preventDefault();\n"
    "            const form = document.getElementById('network-config-form');\n"
    "            const formData = new FormData(form);\n"
    "            const params = new URLSearchParams();\n"
    "            for (const [key, value] of formData) {\n"
    "                if (value) params.append(key, value);\n"
    "            }\n"
    "            document.getElementById('config-result').innerHTML = '<div class=\"loading\">Saving configuration...</div>';\n"
    "            fetchAPI('/api/network/config', 'POST', params.toString()).then(data => {\n"
    "                if (data.error) {\n"
    "                    document.getElementById('config-result').innerHTML = '<div class=\"error\">Error: ' + data.error + '</div>';\n"
    "                } else {\n"
    "                    document.getElementById('config-result').innerHTML = '<div class=\"success-msg\">✅ ' + data.message + '</div>';\n"
    "                    setTimeout(refreshDataset, 500);\n"
    "                }\n"
    "            });\n"
    "        }\n"
    "        function loadCurrentConfig() {\n"
    "            fetchAPI('/api/dataset').then(data => {\n"
    "                if (data.network_name) {\n"
    "                    document.getElementById('network_name').value = data.network_name;\n"
    "                }\n"
    "                if (data.pan_id) {\n"
    "                    document.getElementById('pan_id').value = data.pan_id;\n"
    "                }\n"
    "                if (data.extpanid && data.extpanid !== 'null') {\n"
    "                    document.getElementById('extpanid').value = data.extpanid;\n"
    "                }\n"
    "                if (data.channel) {\n"
    "                    document.getElementById('channel').value = data.channel;\n"
    "                }\n"
    "                if (data.network_key && data.network_key !== 'null') {\n"
    "                    document.getElementById('network_key').value = data.network_key;\n"
    "                }\n"
    "                document.getElementById('control-result').innerHTML = '<div class=\"success-msg\">✅ Configuration loaded</div>';\n"
    "            }).catch(error => {\n"
    "                document.getElementById('control-result').innerHTML = '<div class=\"error\">Failed to load configuration</div>';\n"
    "            });\n"
    "        }\n"
    "        function generateRandomKey() {\n"
    "            const array = new Uint8Array(16);\n"
    "            crypto.getRandomValues(array);\n"
    "            const hexKey = Array.from(array).map(b => b.toString(16).padStart(2, '0')).join('');\n"
    "            document.getElementById('network_key').value = hexKey;\n"
    "        }\n"
    "        function generateRandomExtPanId() {\n"
    "            const array = new Uint8Array(8);\n"
    "            crypto.getRandomValues(array);\n"
    "            const hexExtPanId = Array.from(array).map(b => b.toString(16).padStart(2, '0')).join('');\n"
    "            document.getElementById('extpanid').value = hexExtPanId;\n"
    "        }\n"
    "        setInterval(refreshStatus, 10000);\n"
    "        refreshStatus();\n"
    "        refreshDataset();\n"
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

/* web server state */
static bool web_server_started = false;

/* Define HTTP service */
static uint16_t border_router_service_port = 8080;
HTTP_SERVICE_DEFINE(border_router_service, NULL, &border_router_service_port,
                    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

/* Define HTTP resources */
HTTP_RESOURCE_DEFINE(index_resource, border_router_service, "/", &index_resource_detail);
HTTP_RESOURCE_DEFINE(api_status_resource, border_router_service, "/api/status", &api_status_detail);
HTTP_RESOURCE_DEFINE(api_dataset_resource, border_router_service, "/api/dataset", &api_dataset_detail);
HTTP_RESOURCE_DEFINE(api_config_resource, border_router_service, "/api/network/config", &api_config_detail);
HTTP_RESOURCE_DEFINE(api_start_resource, border_router_service, "/api/thread/start", &api_start_detail);
HTTP_RESOURCE_DEFINE(api_stop_resource, border_router_service, "/api/thread/stop", &api_stop_detail);

int web_server_init(void)
{
    int ret;

    if ( !web_server_started)
    {
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
        web_server_started = true;
		LOG_INF("HTTP server started successfully");
		LOG_INF("Web interface available at: http://<device-ip>:%d/", border_router_service_port);
		LOG_INF("REST API endpoints:");
		LOG_INF("  GET  /api/status              - Border router status");
		LOG_INF("  GET  /api/dataset             - Get current network dataset");
		LOG_INF("  POST /api/network/config      - Configure Thread network");
		LOG_INF("  GET  /api/thread/start        - Start Thread network");
		LOG_INF("  GET  /api/thread/stop         - Stop Thread network");
	}
    else{
        LOG_INF("Web Server already started");
	}

    return 0;
}
