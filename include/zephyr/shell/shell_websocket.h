/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_WEBSOCKET_H_
#define ZEPHYR_INCLUDE_SHELL_WEBSOCKET_H_

#include <zephyr/net/socket.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHELL_WEBSOCKET_SERVICE_COUNT CONFIG_SHELL_WEBSOCKET_BACKEND_COUNT

/** Line buffer structure. */
struct shell_websocket_line_buf {
	/** Line buffer. */
	char buf[CONFIG_SHELL_WEBSOCKET_LINE_BUF_SIZE];

	/** Current line length. */
	uint16_t len;
};

/** WEBSOCKET-based shell transport. */
struct shell_websocket {
	/** Handler function registered by shell. */
	shell_transport_handler_t shell_handler;

	/** Context registered by shell. */
	void *shell_context;

	/** Buffer for outgoing line. */
	struct shell_websocket_line_buf line_out;

	/** Array for sockets used by the websocket service. */
	struct zsock_pollfd fds[1];

	/** Input buffer. */
	uint8_t rx_buf[CONFIG_SHELL_CMD_BUFF_SIZE];

	/** Number of data bytes within the input buffer. */
	size_t rx_len;

	/** Mutex protecting the input buffer access. */
	struct k_mutex rx_lock;

	/** The delayed work is used to send non-lf terminated output that has
	 *  been around for "too long". This will prove to be useful
	 *  to send the shell prompt for instance.
	 */
	struct k_work_delayable send_work;
	struct k_work_sync work_sync;

	/** If set, no output is sent to the WEBSOCKET client. */
	bool output_lock;
};

extern const struct shell_transport_api shell_websocket_transport_api;
int shell_websocket_setup(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);
int shell_websocket_enable(const struct shell *sh);

#define GET_WS_NAME(_service) ws_ctx_##_service
#define GET_WS_SHELL_NAME(_name) shell_websocket_##_name
#define GET_WS_TRANSPORT_NAME(_service) transport_shell_ws_##_service
#define GET_WS_DETAIL_NAME(_service) ws_res_detail_##_service

#define SHELL_WEBSOCKET_DEFINE(_service)					\
	static struct shell_websocket GET_WS_NAME(_service);			\
	static struct shell_transport GET_WS_TRANSPORT_NAME(_service) = {	\
		.api = &shell_websocket_transport_api,				\
		.ctx = &GET_WS_NAME(_service),					\
	}

#define SHELL_WS_PORT_NAME(_service)	http_service_##_service
#define SHELL_WS_BUF_NAME(_service)	ws_recv_buffer_##_service
#define SHELL_WS_TEMP_RECV_BUF_SIZE 256

#define DEFINE_WEBSOCKET_HTTP_SERVICE(_service)					\
	uint8_t SHELL_WS_BUF_NAME(_service)[SHELL_WS_TEMP_RECV_BUF_SIZE];	\
	struct http_resource_detail_websocket					\
	    GET_WS_DETAIL_NAME(_service) = {					\
		.common = {							\
			.type = HTTP_RESOURCE_TYPE_WEBSOCKET,			\
										\
			/* We need HTTP/1.1 GET method for upgrading */		\
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),	\
		},								\
		.cb = shell_websocket_setup,					\
		.data_buffer = SHELL_WS_BUF_NAME(_service),			\
		.data_buffer_len = sizeof(SHELL_WS_BUF_NAME(_service)),		\
		.user_data = &GET_WS_NAME(_service),				\
	};									\
	HTTP_RESOURCE_DEFINE(ws_resource_##_service, _service,			\
			     "/" CONFIG_SHELL_WEBSOCKET_ENDPOINT_URL,		\
			     &GET_WS_DETAIL_NAME(_service))

#define DEFINE_WEBSOCKET_SERVICE(_service)					\
	SHELL_WEBSOCKET_DEFINE(_service);					\
	SHELL_DEFINE(shell_websocket_##_service,				\
		     CONFIG_SHELL_WEBSOCKET_PROMPT,				\
		     &GET_WS_TRANSPORT_NAME(_service),				\
		     CONFIG_SHELL_WEBSOCKET_LOG_MESSAGE_QUEUE_SIZE,		\
		     CONFIG_SHELL_WEBSOCKET_LOG_MESSAGE_QUEUE_TIMEOUT,		\
		     SHELL_FLAG_OLF_CRLF);					\
	DEFINE_WEBSOCKET_HTTP_SERVICE(_service)

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
/* Use a secure connection only for Websocket. */
#define WEBSOCKET_CONSOLE_DEFINE(_service, _sec_tag_list, _sec_tag_list_size) \
	static uint16_t SHELL_WS_PORT_NAME(_service) =			  \
		CONFIG_SHELL_WEBSOCKET_PORT;				  \
	HTTPS_SERVICE_DEFINE(_service,					  \
			     CONFIG_SHELL_WEBSOCKET_IP_ADDR,		  \
			     &SHELL_WS_PORT_NAME(_service),		  \
			     SHELL_WEBSOCKET_SERVICE_COUNT,		  \
			     SHELL_WEBSOCKET_SERVICE_COUNT,		  \
			     NULL,					  \
			     NULL,                                        \
			     _sec_tag_list,				  \
			     _sec_tag_list_size);			  \
	DEFINE_WEBSOCKET_SERVICE(_service);				  \


#else /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
/* TLS not possible so define only normal HTTP service */
#define WEBSOCKET_CONSOLE_DEFINE(_service, _sec_tag_list, _sec_tag_list_size) \
	static uint16_t SHELL_WS_PORT_NAME(_service) =			\
		CONFIG_SHELL_WEBSOCKET_PORT;				\
	HTTP_SERVICE_DEFINE(_service,					\
			    CONFIG_SHELL_WEBSOCKET_IP_ADDR,		\
			    &SHELL_WS_PORT_NAME(_service),		\
			    SHELL_WEBSOCKET_SERVICE_COUNT,		\
			    SHELL_WEBSOCKET_SERVICE_COUNT,		\
			    NULL, NULL);				\
	DEFINE_WEBSOCKET_SERVICE(_service)

#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

#define WEBSOCKET_CONSOLE_ENABLE(_service)				\
	(void)shell_websocket_enable(&GET_WS_SHELL_NAME(_service))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_WEBSOCKET_H_ */
