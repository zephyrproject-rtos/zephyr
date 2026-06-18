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

/** @endcond */

/** Authentication methods supported by SSH transports. */
enum ssh_auth_type {
	/** No authentication method selected. */
	SSH_AUTH_NONE,
	/** Username and password authentication. */
	SSH_AUTH_PASSWORD,
	/** Public key authentication. */
	SSH_AUTH_PUBKEY
};

/** Types of SSH channel requests. */
enum ssh_channel_request_type {
	/** Unknown or unsupported channel request type. */
	SSH_CHANNEL_REQUEST_UNKNOWN,
	/** Request a pseudo-terminal. */
	SSH_CHANNEL_REQUEST_PSEUDO_TERMINAL,
	/** Request X11 forwarding. */
	SSH_CHANNEL_REQUEST_X11_FORWARD,
	/** Set an environment variable. */
	SSH_CHANNEL_REQUEST_ENV_VAR,
	/** Start an interactive shell. */
	SSH_CHANNEL_REQUEST_SHELL,
	/** Execute a command. */
	SSH_CHANNEL_REQUEST_EXEC,
	/** Start a subsystem. */
	SSH_CHANNEL_REQUEST_SUBSYSTEM,
	/** Notify of terminal window size changes. */
	SSH_CHANNEL_REQUEST_WINDOW_CHANGE,
	/** Request flow control. */
	SSH_CHANNEL_REQUEST_FLOW_CONTROL,
	/** Send a signal to the remote process. */
	SSH_CHANNEL_REQUEST_SIGNAL,
	/** Report process exit status. */
	SSH_CHANNEL_REQUEST_EXIT_STATUS,
	/** Report process exit signal. */
	SSH_CHANNEL_REQUEST_EXIT_SIGNAL
};

/** Types of events emitted for an SSH channel. */
enum ssh_channel_event_type {
	/** Result of an open request on the client side. */
	SSH_CHANNEL_EVENT_OPEN_RESULT,		/* Client only */
	/** Incoming channel request on the server side. */
	SSH_CHANNEL_EVENT_REQUEST,		/* Server only */
	/** Result of a previously sent channel request on the client side. */
	SSH_CHANNEL_EVENT_REQUEST_RESULT,	/* Client only */
	/** Channel data is available to read. */
	SSH_CHANNEL_EVENT_RX_DATA_READY,
	/** Channel can accept more transmit data. */
	SSH_CHANNEL_EVENT_TX_DATA_READY,
	/** Standard error data is available to read. */
	SSH_CHANNEL_EVENT_RX_STDERR_DATA_READY,
	/** End-of-file received for the channel. */
	SSH_CHANNEL_EVENT_EOF,
	/** Channel has been closed. */
	SSH_CHANNEL_EVENT_CLOSED
};

/** Data for SSH_CHANNEL_EVENT_REQUEST event. */
struct ssh_channel_event_channel_request {
	/** Requested channel operation. */
	enum ssh_channel_request_type type;
	/** True if the peer expects a reply. */
	bool want_reply;
	/* TODO: Union of request type data */
};

/** Data for SSH_CHANNEL_EVENT_REQUEST_RESULT event. */
struct ssh_channel_event_channel_request_result {
	/** True if the request succeeded. */
	bool success;
};

/** SSH channel event information. */
struct ssh_channel_event {
	/** Types of events emitted for an SSH channel. */
	enum ssh_channel_event_type type;

	/** Channel event payload data. */
	union {
		/** Data for SSH_CHANNEL_EVENT_REQUEST. */
		struct ssh_channel_event_channel_request channel_request;

		/** Data for SSH_CHANNEL_EVENT_REQUEST_RESULT. */
		struct ssh_channel_event_channel_request_result channel_request_result;
	};
};

/**
 * @brief SSH channel event callback.
 *
 * @param channel Pointer to the SSH channel that produced the event.
 * @param event Pointer to channel event data.
 * @param user_data User-provided context pointer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*ssh_channel_event_callback_t)(struct ssh_channel *channel,
					    const struct ssh_channel_event *event,
					    void *user_data);

/** Types of events emitted for an SSH transport. */
enum ssh_transport_event_type {
	/** Transport has been closed. */
	SSH_TRANSPORT_EVENT_CLOSED,
	/** Requested SSH service was accepted by the peer. */
	SSH_TRANSPORT_EVENT_SERVICE_ACCEPTED,		/* Client only */
	/** Authentication attempt result. */
	SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT,	/* Client only */
	/** Server received a request to open a channel. */
	SSH_TRANSPORT_EVENT_CHANNEL_OPEN,		/* Server only */
};

/** Data for SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT event. */
struct ssh_transport_event_authenticate_result {
	/** True if authentication succeeded. */
	bool success;
};

/** Data for SSH_TRANSPORT_EVENT_CHANNEL_OPEN event. */
struct ssh_transport_event_channel_open {
	/** Newly opened channel. */
	struct ssh_channel *channel;
};

/** SSH transport event information. */
struct ssh_transport_event {
	/** Types of events emitted for an SSH transport. */
	enum ssh_transport_event_type type;

	/** Transport event payload data. */
	union {
		/** Data for SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT. */
		struct ssh_transport_event_authenticate_result authenticate_result;

		/** Data for SSH_TRANSPORT_EVENT_CHANNEL_OPEN. */
		struct ssh_transport_event_channel_open channel_open;
	};
};

/**
 * @brief SSH transport event callback.
 *
 * @param transport Pointer to the SSH transport that produced the event.
 * @param event Pointer to transport event data.
 * @param user_data User-provided context pointer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*ssh_transport_event_callback_t)(struct ssh_transport *transport,
					      const struct ssh_transport_event *event,
					      void *user_data);

/** Extended data type for standard error channel data. */
#define SSH_EXTENDED_DATA_STDERR 1

#if defined(CONFIG_SSH_CLIENT) || defined(__DOXYGEN__)
/**
 * @brief Get the authenticated client username for a transport.
 *
 * @param transport Pointer to the SSH transport.
 *
 * @return Pointer to the username string.
 */
const char *ssh_transport_client_user_name(struct ssh_transport *transport);

/**
 * @brief Authenticate an SSH transport using a password.
 *
 * @param transport Pointer to the SSH transport.
 * @param user_name Username to authenticate with.
 * @param password Password to authenticate with.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_transport_auth_password(struct ssh_transport *transport, const char *user_name,
				const char *password);

/**
 * @brief Open a new channel on an SSH transport.
 *
 * @param transport Pointer to the SSH transport.
 * @param callback Channel event callback for the new channel.
 * @param user_data User-provided context pointer passed to @p callback.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_transport_channel_open(struct ssh_transport *transport,
			       ssh_channel_event_callback_t callback, void *user_data);

#endif /* CONFIG_SSH_CLIENT || __DOXYGEN__ */

#if defined(CONFIG_SSH_SERVER) || defined(__DOXYGEN__)
/**
 * @brief Reply to a channel open request.
 *
 * @param channel Pointer to the SSH channel.
 * @param success True to accept the open request, false to reject it.
 * @param callback Channel event callback to associate with the opened channel.
 * @param user_data User-provided context pointer passed to @p callback.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_channel_open_result(struct ssh_channel *channel, bool success,
			    ssh_channel_event_callback_t callback, void *user_data);

#endif /* CONFIG_SSH_SERVER || __DOXYGEN__ */

/**
 * @brief Reply to a channel request.
 *
 * @param channel Pointer to the SSH channel.
 * @param success True if the request succeeded, false otherwise.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_channel_request_result(struct ssh_channel *channel, bool success);

#if defined(CONFIG_SSH_CLIENT) || defined(__DOXYGEN__)
/**
 * @brief Request an interactive shell on a channel.
 *
 * @param channel Pointer to the SSH channel.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ssh_channel_request_shell(struct ssh_channel *channel);

#endif /* CONFIG_SSH_CLIENT || __DOXYGEN__ */

/**
 * @brief Read channel data.
 *
 * @param channel Pointer to the SSH channel.
 * @param data Buffer to store the received data.
 * @param len Maximum number of bytes to read.
 *
 * @return Number of bytes read, or a negative error code on failure.
 */
int ssh_channel_read(struct ssh_channel *channel, void *data, uint32_t len);

/**
 * @brief Write channel data.
 *
 * @param channel Pointer to the SSH channel.
 * @param data Buffer containing data to send.
 * @param len Number of bytes to write.
 *
 * @return Number of bytes written, or a negative error code on failure.
 */
int ssh_channel_write(struct ssh_channel *channel, const void *data, uint32_t len);

/**
 * @brief Read channel standard error data.
 *
 * @param channel Pointer to the SSH channel.
 * @param data Buffer to store the received standard error data.
 * @param len Maximum number of bytes to read.
 *
 * @return Number of bytes read, or a negative error code on failure.
 */
int ssh_channel_read_stderr(struct ssh_channel *channel, void *data, uint32_t len);

/**
 * @brief Write channel standard error data.
 *
 * @param channel Pointer to the SSH channel.
 * @param data Buffer containing standard error data to send.
 * @param len Number of bytes to write.
 *
 * @return Number of bytes written, or a negative error code on failure.
 */
int ssh_channel_write_stderr(struct ssh_channel *channel, const void *data, uint32_t len);

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
