/*
 * HTTP and HTTPS services hosted by the BMC.
 *
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024, Nordic Semiconductor
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/auth.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/mgmt/bmc/http.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/websocket.h>

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
#include <zephyr/shell/shell_websocket.h>
#endif

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

static uint16_t http_service_port = CONFIG_BMC_HTTP_PORT;
HTTP_SERVICE_DEFINE(bmc_http_service, NULL, &http_service_port, CONFIG_BMC_HTTP_CONCURRENT,
		    CONFIG_BMC_HTTP_BACKLOG, NULL, NULL, NULL);

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
DEFINE_WEBSOCKET_SERVICE(bmc_http_service);
#endif

#if defined(CONFIG_BMC_HTTPS)
BUILD_ASSERT(IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS),
	     "CONFIG_BMC_HTTPS needs CONFIG_NET_SOCKETS_SOCKOPT_TLS");

/*
 * The credentials for this tag are supplied by the application before
 * bmc_init() runs, so that the BMC core carries no keys of its own.
 */
static const sec_tag_t bmc_sec_tag_list[] = {
	CONFIG_BMC_HTTPS_SEC_TAG,
};

static uint16_t https_service_port = CONFIG_BMC_HTTPS_PORT;
HTTPS_SERVICE_DEFINE(bmc_https_service, NULL, &https_service_port, CONFIG_BMC_HTTP_CONCURRENT,
		     CONFIG_BMC_HTTP_BACKLOG, NULL, NULL, NULL, bmc_sec_tag_list,
		     sizeof(bmc_sec_tag_list));

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
DEFINE_WEBSOCKET_SERVICE(bmc_https_service);
#endif
#endif /* CONFIG_BMC_HTTPS */

/* "Auth:" followed by "<user>_<password>". */
#define WS_AUTH_PREFIX      "Auth:"
#define WS_CREDENTIALS_MAX  (BMC_CONFIG_USER_MAX_LEN + 1 + BMC_CONFIG_PASSWORD_MAX_LEN)
#define WS_AUTH_TIMEOUT_MS  3000

int bmc_http_ws_auth(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	char rx_buf[sizeof(WS_AUTH_PREFIX) + WS_CREDENTIALS_MAX];
	uint32_t message_type;
	uint64_t remaining;
	int ret;

	ARG_UNUSED(request_ctx);
	ARG_UNUSED(user_data);

	ret = websocket_recv_msg(ws_socket, rx_buf, sizeof(rx_buf) - 1, &message_type, &remaining,
				 WS_AUTH_TIMEOUT_MS);
	if (ret <= 0) {
		LOG_WRN("No websocket authentication message (err=%d)", ret < 0 ? ret : -EIO);
		websocket_disconnect(ws_socket);
		return ret < 0 ? ret : -EIO;
	}

	rx_buf[ret] = '\0';

	if (strncmp(rx_buf, WS_AUTH_PREFIX, sizeof(WS_AUTH_PREFIX) - 1) != 0) {
		LOG_WRN("Malformed websocket authentication message");
		websocket_disconnect(ws_socket);
		return -EINVAL;
	}

	ret = bmc_auth_check_pair(rx_buf + sizeof(WS_AUTH_PREFIX) - 1, '_');
	if (ret < 0) {
		LOG_WRN("Websocket authentication failed (err=%d)", ret);
		websocket_disconnect(ws_socket);
		return -EACCES;
	}

	return 0;
}

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
/*
 * The websocket shell backend installs its own resource callback, so wrap it
 * to authenticate the client before the shell session is handed over.
 */
static http_resource_websocket_cb_t shell_ws_cb[IS_ENABLED(CONFIG_BMC_HTTPS) ? 2 : 1];

static int shell_ws_auth_cb(int ws_socket, struct http_request_ctx *request_ctx, void *user_data,
			    unsigned int index)
{
	int ret;

	ret = bmc_http_ws_auth(ws_socket, request_ctx, user_data);
	if (ret < 0) {
		return ret;
	}

	return shell_ws_cb[index](ws_socket, request_ctx, user_data);
}

static int shell_http_ws_auth_cb(int ws_socket, struct http_request_ctx *request_ctx,
				 void *user_data)
{
	return shell_ws_auth_cb(ws_socket, request_ctx, user_data, 0);
}

#if defined(CONFIG_BMC_HTTPS)
static int shell_https_ws_auth_cb(int ws_socket, struct http_request_ctx *request_ctx,
				  void *user_data)
{
	return shell_ws_auth_cb(ws_socket, request_ctx, user_data, 1);
}
#endif

static int shell_ws_start(void)
{
	int ret;

	shell_ws_cb[0] = GET_WS_DETAIL_NAME(bmc_http_service).cb;
	GET_WS_DETAIL_NAME(bmc_http_service).cb = shell_http_ws_auth_cb;

	ret = shell_websocket_enable(&GET_WS_SHELL_NAME(bmc_http_service));
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_BMC_HTTPS)
	shell_ws_cb[1] = GET_WS_DETAIL_NAME(bmc_https_service).cb;
	GET_WS_DETAIL_NAME(bmc_https_service).cb = shell_https_ws_auth_cb;

	ret = shell_websocket_enable(&GET_WS_SHELL_NAME(bmc_https_service));
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}
#else /* CONFIG_SHELL_BACKEND_WEBSOCKET */
static int shell_ws_start(void)
{
	return 0;
}
#endif /* CONFIG_SHELL_BACKEND_WEBSOCKET */

int bmc_http_start(void)
{
	int ret;

	ret = shell_ws_start();
	if (ret < 0) {
		LOG_ERR("Could not enable the websocket shell (err=%d)", ret);
		return ret;
	}

	return http_server_start();
}

BMC_COMPONENT_DEFINE(bmc_http, BMC_INIT_PHASE_SERVICE, bmc_http_start, false);
