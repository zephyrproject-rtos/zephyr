/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_HTTP_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_HTTP_H_

/**
 * @file
 * @brief HTTP services hosted by the BMC.
 */

#include <errno.h>

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC HTTP services
 * @defgroup bmc_http BMC HTTP services
 * @ingroup bmc_api
 * @{
 *
 * The BMC owns the HTTP service, and optionally an HTTPS service, that its
 * Redfish and web endpoints are published on. Applications attach their own
 * resources to those services with BMC_HTTP_RESOURCE_DEFINE(), or with plain
 * HTTP_RESOURCE_DEFINE() naming @c bmc_http_service / @c bmc_https_service
 * when a resource should only appear on one of them.
 */

/**
 * @brief Publish a resource on every BMC HTTP service.
 *
 * Registers @p _resource on the plain HTTP service and, when CONFIG_BMC_HTTPS
 * is enabled, on the HTTPS service as well.
 *
 * To publish on only one of them, call HTTP_RESOURCE_DEFINE() directly with
 * the service name @c bmc_http_service or @c bmc_https_service. Note that
 * HTTP_RESOURCE_DEFINE() pastes the service name into a section name, so the
 * name has to be spelled out rather than passed through another macro.
 *
 * @param _name Unique C symbol prefix for the resource.
 * @param _resource URL path of the resource.
 * @param _detail Pointer to the matching `http_resource_detail_*` structure.
 */
#if defined(CONFIG_BMC_HTTPS) || defined(__DOXYGEN__)
#define BMC_HTTP_RESOURCE_DEFINE(_name, _resource, _detail)                                        \
	HTTP_RESOURCE_DEFINE(_name##_http, bmc_http_service, _resource, _detail);                  \
	HTTP_RESOURCE_DEFINE(_name##_https, bmc_https_service, _resource, _detail)
#else
#define BMC_HTTP_RESOURCE_DEFINE(_name, _resource, _detail)                                        \
	HTTP_RESOURCE_DEFINE(_name##_http, bmc_http_service, _resource, _detail)
#endif

/**
 * @brief Authenticate an incoming websocket connection.
 *
 * Reads the `Auth:<user>_<password>` handshake message that the BMC web UI
 * sends as the first websocket frame and validates it through the registered
 * @ref bmc_auth_ops. The socket is disconnected if authentication fails.
 *
 * Applications adding their own websocket endpoints to the BMC HTTP services
 * call this from their `http_resource_detail_websocket` callback.
 *
 * @param ws_socket Websocket socket descriptor.
 * @param request_ctx HTTP request context, unused.
 * @param user_data Resource user data, unused.
 *
 * @return 0 when the client is authenticated, negative errno otherwise.
 */
int bmc_http_ws_auth(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_HTTP_H_ */
