/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SSH_SERVER_H_
#define ZEPHYR_INCLUDE_NET_SSH_SERVER_H_

/**
 * @file server.h
 *
 * @brief SSH server API
 *
 * @defgroup ssh_server SSH Server Library
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ssh/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct ssh_server;

/** @endcond */

/** Server event types */
enum ssh_server_event_type {
	/** Server has been closed */
	SSH_SERVER_EVENT_CLOSED,
	/** Client has connected to the server */
	SSH_SERVER_EVENT_CLIENT_CONNECTED,
	/** Client has disconnected from the server */
	SSH_SERVER_EVENT_CLIENT_DISCONNECTED,
};

/** Client connection transport data */
struct ssh_server_event_client_connected {
	/** Transport associated with the connected client */
	struct ssh_transport *transport;
};

/** Client disconnection transport data */
struct ssh_server_event_client_disconnected {
	/** Transport associated with the disconnected client */
	struct ssh_transport *transport;
};

/**
 * @brief SSH server event
 *
 * Different events that can occur in the SSH server, such as client
 * connections and disconnections.
 */
struct ssh_server_event {
	/** Type of the event */
	enum ssh_server_event_type type;

	/** Event data */
	union {
		/** Data specific to client connection */
		struct ssh_server_event_client_connected client_connected;

		/** Data specific to client disconnection */
		struct ssh_server_event_client_disconnected client_disconnected;
	};
};

/**
 * @typedef ssh_server_event_callback_t
 * @brief Callback function type for SSH server events
 *
 * @details This callback is invoked by the SSH server to notify the application of
 * various events related to client connections and disconnections. The callback
 * receives a pointer to the SSH server instance, an event structure containing
 * details about the event, and user data provided during server initialization.
 *
 * @param sshd Pointer to the SSH server instance that is invoking the callback.
 * @param event Pointer to a structure containing details about the event that occurred.
 * @param user_data Pointer to user-defined data that was provided when the server
 * was started,
 *
 * @return 0 on success, or a non-zero value to indicate an error.
 */
typedef int (*ssh_server_event_callback_t)(struct ssh_server *sshd,
					   const struct ssh_server_event *event,
					   void *user_data);

/** @cond INTERNAL_HIDDEN */

struct ssh_server *ssh_server_instance(int instance);

/** @endcond */


/**
 * @brief Start an SSH server
 *
 * @param sshd Pointer to the SSH server instance to start.
 * @param bind_addr Network address to bind the SSH server to.
 * @param host_key_index Index of the host key to use for the SSH server.
 * @param username Username for SSH authentication. Set to NULL or empty string
 *        to allow any username to be used for incoming connections.
 * @param password Password for SSH authentication. Set to NULL or empty string
 *        to disable password authentication.
 * @param authorized_keys Array of indices for authorized public keys. Set to NULL
 *        to disable public key authentication.
 * @param authorized_keys_len Length of the authorized_keys array.
 * @param server_callback Callback function to handle SSH server events.
 * @param transport_callback Callback function to handle SSH transport events.
 * @param user_data Pointer to user-defined data that will be passed to the callback
 *        functions.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_server_start(struct ssh_server *sshd,
		     const struct net_sockaddr *bind_addr,
		     int host_key_index,
		     const char *username,
		     const char *password,
		     const int *authorized_keys,
		     size_t authorized_keys_len,
		     ssh_server_event_callback_t server_callback,
		     ssh_transport_event_callback_t transport_callback,
		     void *user_data);

/**
 * @brief Stop an SSH server
 *
 * @param sshd Pointer to the SSH server instance to stop.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_server_stop(struct ssh_server *sshd);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SSH_SERVER_H_ */
