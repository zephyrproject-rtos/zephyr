/*
 * HTTP web server
 *
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024, Nordic Semiconductor
 *
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
#include <zephyr/shell/shell_websocket.h>
#endif
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_http, LOG_LEVEL_INF);

#include "http.h"
#include "config.h"

static uint16_t http_service_port = 80;
HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port,
		    3, 10, NULL, NULL, NULL);

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
DEFINE_WEBSOCKET_SERVICE(http_service);
#endif

#if defined(CONFIG_BMC_APP_HTTPS)

#include "certificate.h"

#ifndef CONFIG_NET_SOCKETS_SOCKOPT_TLS
#error "CONFIG_NET_SOCKETS_SOCKOPT_TLS=n"
#endif

#if defined(CONFIG_BMC_APP_HTTPS_PSK)
#ifndef CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#error "CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED=n"
#endif
#endif

enum tls_tag {
#if defined(CONFIG_BMC_APP_HTTPS_PUBLIC_KEY)
        /** Used for both the public and private server keys */
        HTTP_SERVER_CERTIFICATE_TAG,
#endif
#if defined(CONFIG_BMC_APP_HTTPS_PSK)
        PSK_TAG,
#endif
};

static const sec_tag_t sec_tag_list_verify_none[] = {
#if defined(CONFIG_BMC_APP_HTTPS_PUBLIC_KEY)
		HTTP_SERVER_CERTIFICATE_TAG,
#endif
#if defined(CONFIG_BMC_APP_HTTPS_PSK)
		PSK_TAG,
#endif
	};

static uint16_t https_service_port = 443;
HTTPS_SERVICE_DEFINE(https_service, NULL, &https_service_port,
		     3, 10, NULL, NULL, NULL, sec_tag_list_verify_none,
		     sizeof(sec_tag_list_verify_none));

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
DEFINE_WEBSOCKET_SERVICE(https_service);
#endif

static int setup_tls(void)
{
	int err = 0;

#if defined(CONFIG_BMC_APP_HTTPS_PUBLIC_KEY)
	err = tls_credential_add(HTTP_SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	err = tls_credential_add(HTTP_SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
		return err;
	}
#endif

#if defined(CONFIG_BMC_APP_HTTPS_PSK)
	const char *psk;
	int psk_len;
	if (config_bmc_https_psk(&psk, &psk_len)) {
		err = tls_credential_add(PSK_TAG,
					 TLS_CREDENTIAL_PSK,
					 psk,
					 psk_len);
		if (err < 0) {
			LOG_ERR("Failed to register PSK: %d", err);
			return err;
		}

		static const char psk_id[] = "bmc";
		err = tls_credential_add(PSK_TAG,
					 TLS_CREDENTIAL_PSK_ID,
					 psk_id,
					 sizeof(psk_id) - 1);
		if (err < 0) {
			LOG_ERR("Failed to register PSK ID: %d", err);
			return err;
		}
	}
#endif

	return err;
}
#else /* defined(CONFIG_BMC_APP_HTTPS) */
static int setup_tls(void)
{
	return 0;
}
#endif /* defined(CONFIG_BMC_APP_HTTPS) */

#include <zephyr/net/websocket.h>
#define CREDENTIALS_MAX_LEN 64
int ws_validate_auth(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	static const char auth_prefix[] = "Auth:";
	uint8_t rx_buf[sizeof(auth_prefix) + CREDENTIALS_MAX_LEN];
	uint32_t message_type;
	uint64_t remaining;
	int rc;
	int32_t timeout_ms = 3000;

	rc = websocket_recv_msg(ws_socket, rx_buf, sizeof(rx_buf) - 1,
				&message_type, &remaining, timeout_ms);
	if (rc <= 0) {
		LOG_WRN("No websocket auth message (err=%d)", rc);
		websocket_disconnect(ws_socket);
		return -EIO;
	}

	rx_buf[rc] = '\0'; /* Nul terminate so we can strcmp it */

	if (memcmp(rx_buf, auth_prefix, sizeof(auth_prefix) - 1)) {
		LOG_WRN("No websocket auth message (%s)", rx_buf);
		websocket_disconnect(ws_socket);
		return -EINVAL;
	}

	const char *auth = rx_buf + sizeof(auth_prefix) - 1;

	uint8_t expected[CREDENTIALS_MAX_LEN];
	snprintf(expected, sizeof(expected), "%s_%s", "admin", config_bmc_admin_password());

	if (strcmp(auth, expected)) {
		LOG_WRN("Websocket auth incorrect (%s)", rx_buf);
		websocket_disconnect(ws_socket);
		return -EPERM;
	}

	return 0;
}

static int (*shell_http_ws_cb)(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);
static int (*shell_https_ws_cb)(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);

int shell_http_ws_auth_cb(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	int rc = ws_validate_auth(ws_socket, request_ctx, user_data);
	if (rc < 0)
		return rc;

	return shell_http_ws_cb(ws_socket, request_ctx, user_data);
}

int shell_https_ws_auth_cb(int ws_socket, struct http_request_ctx *request_ctx, void *user_data)
{
	int rc = ws_validate_auth(ws_socket, request_ctx, user_data);
	if (rc < 0)
		return rc;

	return shell_https_ws_cb(ws_socket, request_ctx, user_data);
}

int app_http_server_init(void)
{
	int err;

	err = setup_tls();
	if (err)
		return err;

#if defined(CONFIG_SHELL_BACKEND_WEBSOCKET)
	shell_http_ws_cb = GET_WS_DETAIL_NAME(http_service).cb;
	GET_WS_DETAIL_NAME(http_service).cb = shell_http_ws_auth_cb;
	err = shell_websocket_enable(&GET_WS_SHELL_NAME(http_service));
	if (err)
		return err;
#if defined(CONFIG_BMC_APP_HTTPS)
	shell_https_ws_cb = GET_WS_DETAIL_NAME(https_service).cb;
	GET_WS_DETAIL_NAME(https_service).cb = shell_https_ws_auth_cb;
	err = shell_websocket_enable(&GET_WS_SHELL_NAME(https_service));
	if (err)
		return err;
#endif
#endif

	return http_server_start();
}
