/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SSH_COMMON_H_
#define ZEPHYR_INCLUDE_NET_SSH_COMMON_H_

/**
 * @file common.h
 *
 * @brief SSH client/server common API
 *
 * @defgroup ssh_common SSH client/server common API
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct ssh_transport;
struct ssh_channel;
struct ssh_client;
struct ssh_server;

enum ssh_auth_type {
	SSH_AUTH_NONE,
	SSH_AUTH_PASSWORD,
	SSH_AUTH_PUBKEY
};

enum ssh_channel_request_type {
	SSH_CHANNEL_REQUEST_UNKNOWN,
	SSH_CHANNEL_REQUEST_PSEUDO_TERMINAL,
	SSH_CHANNEL_REQUEST_X11_FORWARD,
	SSH_CHANNEL_REQUEST_ENV_VAR,
	SSH_CHANNEL_REQUEST_SHELL,
	SSH_CHANNEL_REQUEST_EXEC,
	SSH_CHANNEL_REQUEST_SUBSYSTEM,
	SSH_CHANNEL_REQUEST_WINDOW_CHANGE,
	SSH_CHANNEL_REQUEST_FLOW_CONTROL,
	SSH_CHANNEL_REQUEST_SIGNAL,
	SSH_CHANNEL_REQUEST_EXIT_STATUS,
	SSH_CHANNEL_REQUEST_EXIT_SIGNAL
};

struct ssh_channel_event {
	enum ssh_channel_event_type {
		SSH_CHANNEL_EVENT_OPEN_RESULT,		/* Client only */
		SSH_CHANNEL_EVENT_REQUEST,		/* Server only */
		SSH_CHANNEL_EVENT_REQUEST_RESULT,	/* Client only */
		SSH_CHANNEL_EVENT_RX_DATA_READY,
		SSH_CHANNEL_EVENT_TX_DATA_READY,
		SSH_CHANNEL_EVENT_RX_STDERR_DATA_READY,
		SSH_CHANNEL_EVENT_EOF,
		SSH_CHANNEL_EVENT_CLOSED
	} type;
	union {
		struct ssh_channel_event_channel_request {
			enum ssh_channel_request_type type;
			bool want_reply;
			/* TODO: Union of request type data */
		} channel_request;
		struct ssh_channel_event_channel_request_result {
			bool success;
		} channel_request_result;
	};
};

typedef int (*ssh_channel_event_callback_t)(struct ssh_channel *channel,
					    const struct ssh_channel_event *event,
					    void *user_data);

struct ssh_transport_event {
	enum ssh_transport_event_type {
		SSH_TRANSPORT_EVENT_CLOSED,
		SSH_TRANSPORT_EVENT_SERVICE_ACCEPTED,		/* Client only */
		SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT,	/* Client only */
		SSH_TRANSPORT_EVENT_CHANNEL_OPEN,		/* Server only */
	} type;
	union {
		struct ssh_transport_event_authenticate_result {
			bool success;
		} authenticate_result;
		struct ssh_transport_event_channel_open {
			struct ssh_channel *channel;
		} channel_open;
	};
};

typedef int (*ssh_transport_event_callback_t)(struct ssh_transport *transport,
					      const struct ssh_transport_event *event,
					      void *user_data);

#define SSH_EXTENDED_DATA_STDERR 1

#ifdef CONFIG_SSH_CLIENT
const char *ssh_transport_client_user_name(struct ssh_transport *transport);
int ssh_transport_auth_password(struct ssh_transport *transport, const char *user_name,
				const char *password);
int ssh_transport_channel_open(struct ssh_transport *transport,
			       ssh_channel_event_callback_t callback, void *user_data);
#endif

#ifdef CONFIG_SSH_SERVER
int ssh_channel_open_result(struct ssh_channel *channel, bool success,
			    ssh_channel_event_callback_t callback, void *user_data);
#endif
int ssh_channel_request_result(struct ssh_channel *channel, bool success);

#ifdef CONFIG_SSH_CLIENT
int ssh_channel_request_shell(struct ssh_channel *channel);
#endif

int ssh_channel_read(struct ssh_channel *channel, void *data, uint32_t len);
int ssh_channel_write(struct ssh_channel *channel, const void *data, uint32_t len);
int ssh_channel_read_stderr(struct ssh_channel *channel, void *data, uint32_t len);
int ssh_channel_write_stderr(struct ssh_channel *channel, const void *data, uint32_t len);

/** @endcond */

/**
 * @typedef ssh_service_client_cb_t
 * @brief Callback used while iterating over SSH client connections
 *
 * @param ssh Pointer to the SSH client instance
 * @param instance SSH client instance id
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*ssh_service_client_cb_t)(struct ssh_client *ssh,
					int instance,
					void *user_data);

/**
 * @brief Go through all SSH client connections.
 *
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void ssh_client_foreach(ssh_service_client_cb_t cb, void *user_data);

/**
 * @typedef ssh_service_server_cb_t
 * @brief Callback used while iterating over SSH server connections
 *
 * @param sshd Pointer to the SSH server instance
 * @param instance SSH server instance id
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*ssh_service_server_cb_t)(struct ssh_server *sshd,
					int instance,
					void *user_data);

/**
 * @brief Go through all SSH server connections.
 *
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void ssh_server_foreach(ssh_service_server_cb_t cb, void *user_data);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SSH_COMMON_H_ */
