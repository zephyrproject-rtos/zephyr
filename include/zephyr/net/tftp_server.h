/**
 * @file
 * @brief TFTP server public header
 *
 * Copyright (c) 2026 Guy Shilman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_TFTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_TFTP_SERVER_H_

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tftp_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tftp_server_error TFTP Server Error Handling
 * @ingroup networking
 * @since 4.4
 * @version 0.1.0
 * @{
 */

/**
 * @brief TFTP server error codes
 *
 */
enum tftp_error_code {
	/** No error occurred */
	TFTP_ERROR_NONE = 0,

	/** Session timed out waiting for client response */
	TFTP_ERROR_SESSION_TIMEOUT,

	/** I/O error during file operations (read/write) */
	TFTP_ERROR_SESSION_IO,

	/** Protocol violation or invalid packet format */
	TFTP_ERROR_SESSION_PROTOCOL,

	/** Memory allocation failure */
	TFTP_ERROR_SESSION_MEMORY,

	/** Transport layer error (network/socket) */
	TFTP_ERROR_TRANSPORT,

	/** File system error (access denied, disk full, etc.) */
	TFTP_ERROR_FILE_SYSTEM,

	/** Network connectivity issue */
	TFTP_ERROR_NETWORK,

	/** Internal server error (unexpected condition) */
	TFTP_ERROR_INTERNAL,
};

/**
 * @brief TFTP session state
 *
 */
enum tftp_session_state {
	/** No active transfer */
	TFTP_SESSION_IDLE,

	/** Read request in progress */
	TFTP_SESSION_RRQ,

	/** Write request in progress */
	TFTP_SESSION_WRQ,

	/** Transfer complete, waiting for final ACK */
	TFTP_SESSION_COMPLETE,

	/** Error state */
	TFTP_SESSION_ERROR,
};

/**
 * @brief TFTP client session error information
 *
 * Contains comprehensive information about a session error, including
 * client address, session state, error code, and transfer progress.
 * This structure is passed to session error callbacks to provide
 * detailed context about the failure.
 */
struct tftp_session_error {
	/** Client address that experienced the error */
	struct net_sockaddr client_addr;

	/** Length of the client address */
	net_socklen_t client_addr_len;

	/** Session state at the time the error occurred */
	enum tftp_session_state session_state;

	/** Error code indicating the type of error */
	enum tftp_error_code error_code;

	/** Total bytes transferred before the error occurred */
	size_t bytes_transferred;
};

/**
 * @brief TFTP session error callback function type
 *
 * This callback is invoked when an individual client session encounters
 * an error. It provides detailed information about the session that failed,
 * including client address, session state, and transfer progress.
 *
 * @param session_error Pointer to session error information structure
 *                     containing details about the failed session
 *
 * @note This callback should not perform blocking operations or long-running
 *       computations. The session has already been cleaned up when this
 *       callback is invoked, so the error information is for logging and
 *       monitoring purposes only.
 */
typedef void (*tftp_session_error_cb_t)(const struct tftp_session_error *session_error);

/**
 * @brief TFTP server error callback function type
 *
 * This callback is invoked when the TFTP server encounters a fatal error
 * that requires server restart or shutdown. It provides information about
 * the server error condition.
 *
 * @param error_code Error code indicating the type of server error
 *
 * @note This callback should not perform blocking operations or long-running
 *       computations. The server thread will be stopped when this callback
 *       is invoked, so the error information is for logging and monitoring
 *       purposes only.
 */
typedef void (*tftp_server_error_cb_t)(enum tftp_error_code error_code);

/** @} */ /* end of tftp_server_error group */

/**
 * @brief Forward declaration of TFTP server structure.
 */
struct tftp_server;

/**
 * @brief TFTP transport operations.
 *
 * This structure defines the transport layer operations for the TFTP server.
 * Users must provide an implementation that handles UDP (or other transport)
 * send/receive operations.
 */
struct tftp_transport_ops {
	/** @brief Initialize the transport.
	 *
	 * @param transport Transport context pointer
	 * @param config Transport-specific configuration string
	 *
	 * @retval 0 Initialization successful
	 * @retval -errno Error during initialization
	 */
	int (*init)(void *transport, const char *config);

	/** @brief Receive data from the transport.
	 *
	 * @param transport Transport context pointer
	 * @param buf Buffer to receive data into
	 * @param len Maximum size of the buffer
	 * @param from_addr Pointer to store sender address
	 * @param from_len Pointer to store sender address length
	 *
	 * @return Number of bytes received, or -errno on error
	 */
	int (*recv)(void *transport, uint8_t *buf, size_t len, struct net_sockaddr *from_addr,
		    net_socklen_t *from_len);

	/** @brief Send data via the transport.
	 *
	 * @param transport Transport context pointer
	 * @param buf Buffer containing data to send
	 * @param len Number of bytes to send
	 * @param to_addr Destination address
	 * @param to_len Destination address length
	 *
	 * @return Number of bytes sent, or -errno on error
	 */
	int (*send)(void *transport, const uint8_t *buf, size_t len,
		    const struct net_sockaddr *to_addr, net_socklen_t to_len);

	/** @brief Clean up transport resources.
	 *
	 * @param transport Transport context pointer
	 */
	void (*cleanup)(void *transport);

	/** @brief Poll the transport for events.
	 *
	 * @param transport Transport context pointer
	 * @param timeout_ms Timeout in milliseconds, or -1 to block indefinitely
	 *
	 * @retval >0 Number of sockets ready for data
	 * @retval 0 Timeout
	 * @retval -errno Error during poll
	 */
	int (*poll)(void *transport, int timeout_ms);
};

/**
 * @brief Get the TFTP server instance
 *
 * Returns a pointer to the global TFTP server instance.
 * This instance should be initialized and started using the
 * tftp_server_init() and tftp_server_start() functions.
 *
 * @return Pointer to TFTP server instance
 */
struct tftp_server *tftp_server_get_instance(void);

/**
 * @brief Initialize TFTP server
 *
 * Initializes the TFTP server with caller-provided transport operations.
 * The server must be initialized before it can be started.
 *
 * @param server Server context (use tftp_server_get_instance())
 * @param transport_ops Transport operation table
 * @param transport_ctx Transport context object
 * @param config Transport configuration string (transport-specific)
 * @param server_error_cb Optional server error callback (can be NULL)
 * @param session_error_cb Optional session error callback (can be NULL)
 *
 * @retval 0 Server initialized successfully
 * @retval -EINVAL Invalid parameters
 * @retval -errno Error from transport init
 */
int tftp_server_init(struct tftp_server *server, struct tftp_transport_ops *transport_ops,
		     void *transport_ctx, const char *config,
		     tftp_server_error_cb_t server_error_cb,
		     tftp_session_error_cb_t session_error_cb);

/**
 * @brief Convenience initializer for built-in UDP transport.
 *
 * @param server Server context (use tftp_server_get_instance())
 * @param port UDP port number as string (for example, "69")
 * @param server_error_cb Optional server error callback (can be NULL)
 * @param session_error_cb Optional session error callback (can be NULL)
 *
 * @retval 0 Server initialized successfully
 * @retval -EINVAL Invalid parameters
 * @retval -errno Error from UDP transport init
 */
int tftp_server_init_udp(struct tftp_server *server, const char *port,
			 tftp_server_error_cb_t server_error_cb,
			 tftp_session_error_cb_t session_error_cb);

/**
 * @brief Start TFTP server
 *
 * Starts the TFTP server thread that processes incoming packets.
 * The server will handle multiple concurrent clients and serve files
 * from all mounted file systems.
 *
 * @param server Server context (use tftp_server_get_instance())
 *
 * @retval 0 Server started successfully
 * @retval -EINVAL Invalid server context
 * @retval -EALREADY Server is already running
 *
 * @note The server runs in a separate thread and processes packets
 *       asynchronously. Multiple clients can connect simultaneously.
 * @note The server automatically resolves file paths across all
 *       mounted file systems (e.g., /RAM:/file.txt, /SD:/file.txt).
 */
int tftp_server_start(struct tftp_server *server);

/**
 * @brief Stop TFTP server
 *
 * Stops the TFTP server thread and cleans up resources.
 * All active client sessions are terminated.
 *
 * @param server Server context (use tftp_server_get_instance())
 *
 * @retval 0 Server stopped successfully
 * @retval -EINVAL Invalid server context
 *
 * @note This function blocks until the server thread has stopped.
 * @note All active transfers are aborted when the server stops.
 */
int tftp_server_stop(struct tftp_server *server);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_TFTP_SERVER_H_ */
