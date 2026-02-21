/*
 * Copyright (c) 2026 Guy Shilman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tftp_server, CONFIG_TFTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/tftp_common.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tftp_server.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/kernel.h>
#include <string.h>

/**
 * @brief TFTP client session structure
 *
 * Represents an active TFTP client session. Each client connection
 * has its own session that tracks the transfer state, file handle,
 * and client address.
 */
struct tftp_session {
	/** List node for session management */
	sys_snode_t node;

	/** Session state */
	enum tftp_session_state state;

	/** Current block number (1-based, 0 for initial ACK) */
	uint16_t block_num;

	/** Client address (for UDP transport) */
	struct net_sockaddr client_addr;
	net_socklen_t client_addr_len;

	/** File handle for the transfer */
	struct fs_file_t file;

	/** Transfer direction: true for read (RRQ), false for write (WRQ) */
	bool is_read;

	/** Total bytes transferred */
	size_t bytes_transferred;

	/** Session timeout work structure */
	struct k_work_delayable timeout_work;

	/** Pointer back to the server */
	struct tftp_server *server;

	/** Transport-specific context */
	void *transport_ctx;

	/** Path of the file on the filesystem (for cleanup on failure) */
	char file_path[MAX_FILE_NAME + 1];
};

/**
 * @brief TFTP server context structure
 *
 * Main server context that manages all client sessions and
 * coordinates transport operations.
 */
struct tftp_server {
	/** List of active client sessions */
	sys_slist_t sessions;

	/** Transport operations */
	struct tftp_transport_ops *transport_ops;

	/** Transport-specific context */
	void *transport_ctx;

	/** Server thread */
	struct k_thread server_thread;
	k_thread_stack_t *stack;
	size_t stack_size;

	/** Server running flag */
	atomic_t running;

	/** Server mutex for session management */
	struct k_mutex sessions_mutex;

	/** Session error callback (optional) */
	tftp_session_error_cb_t session_error_cb;

	/** Server error callback (optional) */
	tftp_server_error_cb_t server_error_cb;
};

/* Static server instance */
static struct tftp_server tftp_server_inst;

/* Session pool for client connections */
static struct tftp_session session_pool[CONFIG_TFTPS_MAX_CLIENTS];
static sys_slist_t free_sessions;

/* Server thread stack */
static K_THREAD_STACK_DEFINE(tftp_server_stack, CONFIG_TFTPS_STACK_SIZE);

/* Forward declarations */
static void tftp_session_cleanup(struct tftp_server *server, struct tftp_session *session,
				 enum tftp_error_code error_code, const char *error_msg);
static void tftp_session_free_internal(struct tftp_server *server, struct tftp_session *session);
static void tftp_session_free(struct tftp_server *server, struct tftp_session *session);

/**
 * @brief Timeout handler for TFTP sessions
 *
 * Handles session timeouts by cleaning up the session and notifying
 * the application via the session error callback.
 *
 * @param work Delayable work structure
 */
static void tftp_session_timeout_handler(struct k_work *work)
{
	struct tftp_session *session = CONTAINER_OF(work, struct tftp_session, timeout_work.work);
	struct tftp_server *server = session->server;

	if (server == NULL) {
		return;
	}

	NET_ERR("[%p] Session timeout after %zu bytes", (void *)session,
		session->bytes_transferred);

	/* Clean up the session due to timeout */
	tftp_session_cleanup(server, session, TFTP_ERROR_SESSION_TIMEOUT, "Session timeout");

	/* Free the session back to the pool */
	tftp_session_free(server, session);
}

/**
 * @brief Initialize a TFTP session structure
 *
 * @param session Session to initialize
 */
static void tftp_session_init(struct tftp_session *session)
{
	if (session == NULL) {
		return;
	}

	fs_file_t_init(&session->file);
	session->state = TFTP_SESSION_IDLE;
	session->block_num = 0;
	session->bytes_transferred = 0;
	session->server = NULL;
	session->transport_ctx = NULL;
	session->file_path[0] = '\0';

	/* Initialize the timeout work */
	k_work_init_delayable(&session->timeout_work, tftp_session_timeout_handler);
}

/**
 * @brief Handle TFTP server fatal error
 *
 * Called from the server thread when a fatal error occurs. Sets the running
 * flag to 0 so the server thread will exit its main loop, calls the server
 * error callback, and returns. The caller (server thread) will then exit
 * naturally, and the application can call tftp_server_stop() to clean up.
 *
 * @param server Server context
 * @param error_code Error code indicating the type of fatal error
 * @param error_msg Error message string describing the error condition
 */
static void tftp_server_handle_fatal_error(struct tftp_server *server,
					   enum tftp_error_code error_code, const char *error_msg)
{
	NET_ERR("TFTP server fatal error: %s (code=%d), stopping server...", error_msg, error_code);

	/* Signal the server thread to stop */
	atomic_set(&server->running, 0);

	if (server->server_error_cb != NULL) {
		server->server_error_cb(error_code);
	}
}

/**
 * @brief Cleanup a TFTP session
 *
 * Closes any open file handles, resets session state, and optionally reports errors.
 * If server and error_code are provided, the session error callback will be called.
 *
 * @param server Server context (can be NULL for basic cleanup)
 * @param session Session to cleanup
 * @param error_code Error code for the session (TFTP_ERROR_NONE for no error)
 * @param error_msg Error message string (for logging only, not passed to callback)
 */
static void tftp_session_cleanup(struct tftp_server *server, struct tftp_session *session,
				 enum tftp_error_code error_code, const char *error_msg)
{
	if (error_code != TFTP_ERROR_NONE) {
		NET_ERR("[%p] Session error: %s (state=%d, bytes=%zu)", (void *)session,
			error_msg != NULL ? error_msg : "Unknown error", session->state,
			session->bytes_transferred);
	} else {
		NET_DBG("[%p] Session cleanup (state=%d, bytes=%zu)", (void *)session,
			session->state, session->bytes_transferred);
	}

	/* Call session error callback if configured (no error message passed) */
	if (server != NULL && error_code != TFTP_ERROR_NONE && server->session_error_cb != NULL) {
		/* Prepare error information if error is provided */
		struct tftp_session_error session_error = {
			.client_addr = session->client_addr,
			.client_addr_len = session->client_addr_len,
			.session_state = session->state,
			.error_code = error_code,
			.bytes_transferred = session->bytes_transferred,
		};

		server->session_error_cb(&session_error);
	}

	if (session->state != TFTP_SESSION_IDLE) {
		NET_INFO("[%p] Closing file (state=%d, is_read=%d)", (void *)session,
			 session->state, session->is_read);
		fs_close(&session->file);

		/* If a write transfer was active but didn't complete, remove the partial file */
		if (!session->is_read && session->state != TFTP_SESSION_COMPLETE &&
		    session->file_path[0] != '\0') {
			LOG_INF("[%p] Removing partial file '%s'", (void *)session,
				session->file_path);
			fs_unlink(session->file_path);
		}
	}
	k_work_cancel_delayable(&session->timeout_work);
	tftp_session_init(session);
}

static void tftp_session_free_internal(struct tftp_server *server, struct tftp_session *session)
{
	NET_INFO("[%p] Freeing session", (void *)session);
	k_work_cancel_delayable(&session->timeout_work);
	tftp_session_cleanup(server, session, TFTP_ERROR_NONE, NULL);
	sys_slist_find_and_remove(&server->sessions, &session->node);
	sys_slist_prepend(&free_sessions, &session->node);
}

/**
 * @brief Find a session by client address
 *
 * Searches the active session list for a session matching the given
 * client address. For UDP transport, this matches the socket address.
 * For serial transport, there's typically only one client.
 *
 * Uses read lock to allow concurrent session lookups.
 *
 * @param server Server context
 * @param addr Client address
 * @param addr_len Address length
 * @return Session pointer or NULL if not found
 */
struct tftp_session *tftp_session_find(struct tftp_server *server, const struct net_sockaddr *addr,
				       net_socklen_t addr_len)
{
	struct tftp_session *session;
	int ret;

	/* Use mutex for session lookups */
	ret = k_mutex_lock(&server->sessions_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to acquire mutex for session lookup: %d", ret);
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&server->sessions, session, node) {
		if (addr_len == session->client_addr_len &&
		    memcmp(&session->client_addr, addr, addr_len) == 0) {
			k_mutex_unlock(&server->sessions_mutex);
			return session;
		}
	}

	k_mutex_unlock(&server->sessions_mutex);
	return NULL;
}

/**
 * @brief Allocate a new session from the pool
 *
 * @param server Server context
 * @return Session pointer or NULL if pool is exhausted
 */
struct tftp_session *tftp_session_alloc(struct tftp_server *server)
{
	struct tftp_session *session = NULL;
	sys_snode_t *node;
	int ret;

	/* Use mutex for session allocation to modify the sessions list */
	ret = k_mutex_lock(&server->sessions_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to acquire mutex for session allocation: %d", ret);
		return NULL;
	}

	node = sys_slist_get(&free_sessions);
	if (node != NULL) {
		session = CONTAINER_OF(node, struct tftp_session, node);
		tftp_session_init(session);
		session->server = server; /* Set the server pointer */
		sys_slist_prepend(&server->sessions, &session->node);
	}

	k_mutex_unlock(&server->sessions_mutex);

	return session;
}

/**
 * @brief Free a session back to the pool
 *
 * @param server Server context
 * @param session Session to free
 */
static void tftp_session_free(struct tftp_server *server, struct tftp_session *session)
{
	int ret;

	/* Use mutex for session deallocation to modify the sessions list */
	ret = k_mutex_lock(&server->sessions_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to acquire mutex for session deallocation: %d", ret);
		return;
	}

	tftp_session_free_internal(server, session);

	k_mutex_unlock(&server->sessions_mutex);
}

/**
 * @brief Open a TFTP file path with minimal copying.
 *
 * For absolute paths, this opens directly with no reformatting.
 * For relative paths, mount iteration is required because Zephyr does not
 * provide a direct reverse index from "filename" to mounted FS; we must try
 * mount points until an open succeeds (RRQ), or pick the first writable mount
 * (WRQ create path).
 */
static int tftp_open_filepath(struct fs_file_t *file, const char *filepath, fs_mode_t flags,
			      bool search_all_mounts, char *opened_path, size_t opened_path_len)
{
	const char *mount_point;
	int mount_idx = 0;
	char candidate[MAX_FILE_NAME + 1];
	int ret;

	if (file == NULL || filepath == NULL) {
		return -EINVAL;
	}

	/* Absolute path: no normalization/reformatting */
	if (filepath[0] == '/') {
		ret = fs_open(file, filepath, flags);
		if (ret == 0 && opened_path != NULL && opened_path_len > 0) {
			snprintk(opened_path, opened_path_len, "%s", filepath);
		}
		return ret;
	}

	while (fs_readmount(&mount_idx, &mount_point) == 0 && mount_point != NULL) {
		ret = snprintk(candidate, sizeof(candidate), "%s/%s", mount_point, filepath);
		if (ret < 0 || ret >= sizeof(candidate)) {
			continue;
		}

		ret = fs_open(file, candidate, flags);
		if (ret == 0) {
			if (opened_path != NULL && opened_path_len > 0) {
				snprintk(opened_path, opened_path_len, "%s", candidate);
			}
			return 0;
		}

		if (!search_all_mounts) {
			break;
		}
	}

	return -ENOENT;
}

/**
 * @brief Send an error packet directly to an address
 *
 * @param server Server context
 * @param addr Client address
 * @param addr_len Address length
 * @param error_code TFTP error code
 * @param error_msg Error message string
 * @return 0 on success, negative errno on failure
 */
static int tftp_send_error_direct(struct tftp_server *server, const struct net_sockaddr *addr,
				  net_socklen_t addr_len, uint16_t error_code,
				  const char *error_msg)
{
	uint8_t buf[TFTP_MAX_PACKET_SIZE];
	uint16_t opcode = sys_cpu_to_be16(TFTP_OPCODE_ERROR);
	uint16_t code = sys_cpu_to_be16(error_code);
	size_t len = 4; /* Opcode + error code */
	int ret;

	memcpy(buf, &opcode, 2);
	memcpy(buf + 2, &code, 2);

	if (error_msg != NULL) {
		size_t msg_len = strlen(error_msg);
		size_t max_msg = sizeof(buf) - len - 1; /* -1 for null terminator */

		if (msg_len > max_msg) {
			msg_len = max_msg;
		}

		memcpy(buf + len, error_msg, msg_len);
		len += msg_len;
		buf[len++] = '\0'; /* Null-terminate error message */
	}

	ret = server->transport_ops->send(server->transport_ctx, buf, len, addr, addr_len);

	if (ret < 0) {
		LOG_ERR("Failed to send error packet: %d", ret);
	}

	return ret;
}

/**
 * @brief Send an error packet
 *
 * Constructs and sends a TFTP ERROR packet according to RFC1350.
 *
 * @param server Server context
 * @param session Client session
 * @param error_code TFTP error code
 * @param error_msg Error message string
 * @return 0 on success, negative errno on failure
 */
static int tftp_send_error(struct tftp_server *server, struct tftp_session *session,
			   uint16_t error_code, const char *error_msg)
{
	return tftp_send_error_direct(server, &session->client_addr, session->client_addr_len,
				      error_code, error_msg);
}

/**
 * @brief Send an ACK packet
 *
 * Constructs and sends a TFTP ACK packet according to RFC1350.
 *
 * @param server Server context
 * @param session Client session
 * @param block_num Block number to acknowledge
 * @return 0 on success, negative errno on failure
 */
static int tftp_send_ack(struct tftp_server *server, struct tftp_session *session,
			 uint16_t block_num)
{
	uint8_t buf[TFTP_HEADER_SIZE];
	uint16_t opcode = sys_cpu_to_be16(TFTP_OPCODE_ACK);
	uint16_t block = sys_cpu_to_be16(block_num);
	int ret;

	memcpy(buf, &opcode, 2);
	memcpy(buf + 2, &block, 2);

	ret = server->transport_ops->send(server->transport_ctx, buf, sizeof(buf),
					  &session->client_addr, session->client_addr_len);

	if (ret < 0) {
		LOG_ERR("[%p] Failed to send ACK: %d", (void *)session, ret);
	} else {
		LOG_DBG("[%p] Sent ACK for block %u", (void *)session, block_num);
	}

	return ret;
}

/**
 * @brief Send a data packet
 *
 * Reads data from the file and sends it as a TFTP DATA packet.
 *
 * @param server Server context
 * @param session Client session
 * @return 0 on success, negative errno on failure
 */
static int tftp_send_data(struct tftp_server *server, struct tftp_session *session)
{
	uint8_t buf[TFTP_MAX_PACKET_SIZE];
	uint16_t opcode = sys_cpu_to_be16(TFTP_OPCODE_DATA);
	uint16_t block = sys_cpu_to_be16(session->block_num);
	ssize_t bytes_read;
	size_t packet_len;
	int ret;

	memcpy(buf, &opcode, 2);
	memcpy(buf + 2, &block, 2);

	/* Read data block from file */
	bytes_read = fs_read(&session->file, buf + TFTP_HEADER_SIZE, TFTP_BLOCK_SIZE);
	if (bytes_read < 0) {
		LOG_ERR("[%p] Failed to read file: %d", (void *)session, (int)bytes_read);
		return (int)bytes_read;
	}

	packet_len = TFTP_HEADER_SIZE + bytes_read;
	session->bytes_transferred += bytes_read;

	ret = server->transport_ops->send(server->transport_ctx, buf, packet_len,
					  &session->client_addr, session->client_addr_len);

	if (ret < 0) {
		LOG_ERR("[%p] Failed to send data packet: %d", (void *)session, ret);
	} else {
		LOG_DBG("[%p] Sent DATA block %u, %d bytes", (void *)session, session->block_num,
			(int)bytes_read);
	}

	/* Check if this is the last block (less than 512 bytes) */
	if (bytes_read < TFTP_BLOCK_SIZE) {
		/* Transfer complete - wait for final ACK.
		 */
		LOG_INF("[%p] Transfer complete: %zu bytes", (void *)session,
			session->bytes_transferred);
		session->state = TFTP_SESSION_COMPLETE;
	} else {
		/* Wait for ACK before sending next block */
		session->block_num++;
	}
	k_work_reschedule(&session->timeout_work, K_MSEC(CONFIG_TFTPS_REQUEST_TIMEOUT));

	return ret;
}

/**
 * @brief Process a TFTP read request (RRQ)
 *
 * Handles a client's request to read a file from the server.
 *
 * @param server Server context
 * @param session Client session
 * @param filename Requested filename
 * @param mode Transfer mode (typically "octet")
 */
static void tftp_handle_rrq(struct tftp_server *server, struct tftp_session *session,
			    const char *filename, const char *mode)
{
	char resolved_path[MAX_FILE_NAME + 1];
	int ret;

	LOG_INF("[%p] RRQ: filename='%s', mode='%s'", (void *)session, filename, mode);

	/* Open directly with minimal path manipulation */
	ret = tftp_open_filepath(&session->file, filename, FS_O_READ, true, resolved_path,
				 sizeof(resolved_path));
	if (ret != 0) {
		LOG_ERR("[%p] Failed to open file '%s': %d", (void *)session, filename, ret);
		tftp_send_error(server, session, TFTP_ERROR_ACCESS, "Access violation");
		tftp_session_cleanup(server, session, TFTP_ERROR_FILE_SYSTEM, "File not found");
		return;
	}

	/* Initialize session for read transfer */
	session->state = TFTP_SESSION_RRQ;
	session->block_num = 1;
	session->is_read = true;
	session->bytes_transferred = 0;
	k_work_reschedule(&session->timeout_work, K_MSEC(CONFIG_TFTPS_REQUEST_TIMEOUT));

	/* Send first data block */
	tftp_send_data(server, session);
}

/**
 * @brief Process a TFTP write request (WRQ)
 *
 * Handles a client's request to write a file to the server.
 *
 * @param server Server context
 * @param session Client session
 * @param filename Requested filename
 * @param mode Transfer mode (typically "octet")
 */
static void tftp_handle_wrq(struct tftp_server *server, struct tftp_session *session,
			    const char *filename, const char *mode)
{
	char resolved_path[MAX_FILE_NAME + 1];
	int ret;

	LOG_INF("[%p] WRQ: filename='%s', mode='%s'", (void *)session, filename, mode);

	/* Open directly; for relative paths we pick first mount for create */
	ret = tftp_open_filepath(&session->file, filename, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC,
				 false, resolved_path, sizeof(resolved_path));
	if (ret != 0) {
		LOG_ERR("[%p] Failed to create file '%s': %d", (void *)session, filename, ret);
		tftp_send_error(server, session, TFTP_ERROR_DISK_FULL,
				"Disk full or allocation exceeded");
		tftp_session_free(server, session);
		return;
	}

	/* Store the resolved path for potential cleanup if transfer fails */
	strncpy(session->file_path, resolved_path, sizeof(session->file_path) - 1);
	session->file_path[sizeof(session->file_path) - 1] = '\0';

	/* Initialize session for write transfer */
	session->state = TFTP_SESSION_WRQ;
	session->block_num = 0; /* Initial ACK is for block 0 */
	session->is_read = false;
	session->bytes_transferred = 0;
	k_work_reschedule(&session->timeout_work, K_MSEC(CONFIG_TFTPS_REQUEST_TIMEOUT));

	/* Send initial ACK (block 0) */
	tftp_send_ack(server, session, 0);
	session->block_num = 1; /* Next expected block */
}

/**
 * @brief Process a TFTP request packet (RRQ or WRQ)
 *
 * Parses and handles incoming read or write requests from clients.
 *
 * @param server Server context
 * @param buf Packet buffer
 * @param len Packet length
 * @param from_addr Source address
 * @param from_len Address length
 */
static void tftp_process_request(struct tftp_server *server, const uint8_t *buf, size_t len,
				 const struct net_sockaddr *from_addr, net_socklen_t from_len)
{
	uint16_t opcode;
	const char *filename, *filename_end, *mode, *mode_end;
	struct tftp_session *session;
	size_t filename_len;

	if (len < TFTP_HEADER_SIZE) {
		NET_ERR("Request packet too short");
		return;
	}

	opcode = sys_get_be16(buf);
	buf += sizeof(opcode);
	len -= sizeof(opcode);

	/* Extract filename */
	filename = (const char *)buf;
	filename_end = memchr(filename, '\0', len);

	if (filename_end == NULL) {
		NET_ERR("Invalid request: filename not null-terminated");
		return;
	}

	filename_len = filename_end - filename;

	buf += filename_len + 1;
	len -= filename_len + 1;

	/* Extract mode */
	mode = (const char *)buf;
	mode_end = memchr(mode, '\0', len);

	if (mode_end == NULL) {
		NET_ERR("Invalid request: mode not null-terminated");
		return;
	}

	/* Find or allocate session */
	session = tftp_session_find(server, from_addr, from_len);
	if (session == NULL) {
		session = tftp_session_alloc(server);
		if (session == NULL) {
			LOG_ERR("No free sessions available");
			tftp_send_error_direct(server, from_addr, from_len, TFTP_ERROR_DISK_FULL,
					       "Server busy");
			return;
		}

		/* Store client address */
		memcpy(&session->client_addr, from_addr, from_len);
		session->client_addr_len = from_len;
		NET_INFO("[%p] Allocated new session", (void *)session);
	} else {
		/* Reuse existing session - must cleanup previous state first */
		NET_INFO("[%p] Reusing session (implicit abort of previous transfer)",
			 (void *)session);
		tftp_session_cleanup(server, session, TFTP_ERROR_NONE, NULL);

		/* Restore client address after cleanup */
		memcpy(&session->client_addr, from_addr, from_len);
		session->client_addr_len = from_len;
	}

	/* Handle request based on opcode */
	if (opcode == TFTP_OPCODE_RRQ) {
		tftp_handle_rrq(server, session, filename, mode);
	} else if (opcode == TFTP_OPCODE_WRQ) {
		tftp_handle_wrq(server, session, filename, mode);
	} else {
		NET_ERR("[%p] Invalid opcode in request: %u", (void *)session, opcode);
		tftp_send_error(server, session, TFTP_ERROR_ILLEGAL_OP, "Illegal TFTP operation");
		tftp_session_free(server, session);
	}
}

/**
 * @brief Process a TFTP data packet (for write requests)
 *
 * Handles incoming data packets during a write transfer.
 *
 * @param server Server context
 * @param buf Packet buffer
 * @param len Packet length
 * @param from_addr Source address
 * @param from_len Address length
 */
static void tftp_process_data(struct tftp_server *server, const uint8_t *buf, size_t len,
			      const struct net_sockaddr *from_addr, net_socklen_t from_len)
{
	uint16_t opcode, block_num;
	struct tftp_session *session;
	ssize_t data_len;
	int ret;

	if (len < TFTP_HEADER_SIZE) {
		NET_ERR("Data packet too short");
		return;
	}

	opcode = sys_get_be16(buf);
	block_num = sys_get_be16(buf + 2);
	data_len = len - TFTP_HEADER_SIZE;

	if (opcode != TFTP_OPCODE_DATA) {
		NET_ERR("Invalid opcode in data packet: %u", opcode);
		return;
	}

	/* Find session */
	session = tftp_session_find(server, from_addr, from_len);
	if (session == NULL) {
		NET_ERR("Data packet for unknown session");
		return;
	}

	if (session->state != TFTP_SESSION_WRQ) {
		NET_ERR("Data packet for session in wrong state");
		return;
	}

	/* Check block number */
	if (block_num != session->block_num) {
		LOG_DBG("[%p] Unexpected block number: got %u, expected %u", (void *)session,
			block_num, session->block_num);
		/* Send ACK for the block we received (might be duplicate) */
		tftp_send_ack(server, session, block_num);
		return;
	}

	/* Write data to file */
	ret = fs_write(&session->file, buf + TFTP_HEADER_SIZE, data_len);
	if (ret < (int)data_len) {
		LOG_ERR("[%p] Failed to write file: %d (wrote %d/%zu)", (void *)session, ret, ret,
			data_len);
		tftp_send_error(server, session, TFTP_ERROR_DISK_FULL,
				"Disk full or allocation exceeded");
		tftp_session_cleanup(server, session, TFTP_ERROR_SESSION_IO,
				     "Failed to write file");
		return;
	}

	session->bytes_transferred += data_len;

	/* Send ACK */
	ret = tftp_send_ack(server, session, block_num);
	if (ret < 0) {
		LOG_ERR("[%p] Failed to send ACK", (void *)session);
		tftp_session_free(server, session);
		return;
	}

	/* Check if this is the last block */
	if (data_len < TFTP_BLOCK_SIZE) {
		/* Transfer complete */
		LOG_INF("[%p] Write transfer complete: %zu bytes", (void *)session,
			session->bytes_transferred);
		session->state = TFTP_SESSION_COMPLETE;
		tftp_session_free(server, session);
	} else {
		/* Wait for next data block */
		session->block_num++;
		k_work_reschedule(&session->timeout_work, K_MSEC(CONFIG_TFTPS_REQUEST_TIMEOUT));
	}
}

/**
 * @brief Process a TFTP ACK packet (for read requests)
 *
 * Handles incoming ACK packets during a read transfer.
 *
 * @param server Server context
 * @param buf Packet buffer
 * @param len Packet length
 * @param from_addr Source address
 * @param from_len Address length
 */
static void tftp_process_ack(struct tftp_server *server, const uint8_t *buf, size_t len,
			     const struct net_sockaddr *from_addr, net_socklen_t from_len)
{
	uint16_t opcode, block_num;
	struct tftp_session *session;

	if (len < TFTP_HEADER_SIZE) {
		NET_ERR("ACK packet too short");
		return;
	}

	opcode = sys_get_be16(buf);
	block_num = sys_get_be16(buf + 2);

	if (opcode != TFTP_OPCODE_ACK) {
		NET_ERR("Invalid opcode in ACK packet: %u", opcode);
		return;
	}

	/* Find session */
	session = tftp_session_find(server, from_addr, from_len);
	if (session == NULL) {
		/* Session not found - could be a late packet after transfer completed
		 * and session was freed. This is normal behavior, just ignore.
		 */
		NET_ERR("ACK packet for unknown session");
		return;
	}

	/* Handle TFTP_SESSION_COMPLETE state (transfer finished, waiting for final ACK) */
	if (session->state == TFTP_SESSION_COMPLETE) {
		/* This is the final ACK for a completed transfer, free the session */
		NET_INFO("[%p] Received final ACK, freeing session", (void *)session);
		tftp_session_free(server, session);
		return;
	}

	if (session->state != TFTP_SESSION_RRQ) {
		NET_ERR("ACK packet for session in wrong state");
		return;
	}

	/* Check block number */
	if (block_num != session->block_num - 1) {
		LOG_DBG("[%p] Unexpected ACK block number: got %u, expected %u", (void *)session,
			block_num, session->block_num - 1);
		/* Might be duplicate ACK, ignore */
		return;
	}

	/* Send next data block */
	tftp_send_data(server, session);
}

/**
 * @brief Server thread function
 *
 * Main server loop that receives packets and processes them.
 *
 * @param server_ptr Server context pointer
 * @param arg2 Unused
 * @param arg3 Unused
 */
static void tftp_server_thread(void *server_ptr, void *arg2, void *arg3)
{
	struct tftp_server *server = (struct tftp_server *)server_ptr;
	uint8_t recv_buf[TFTP_MAX_PACKET_SIZE];
	struct net_sockaddr from_addr;
	net_socklen_t from_len;
	int ret;
	uint16_t opcode;

	NET_DBG("TFTP server thread started");

	while (atomic_get(&server->running)) {
		/* Receive packet, block until data arrives or we get interrupted.
		 */
		NET_DBG("Polling sockets with K_FOREVER timeout...");
		ret = server->transport_ops->poll(server->transport_ctx, -1);

		if (ret < 0) {
			/* Fatal error - stop server and let user decide */
			tftp_server_handle_fatal_error(server, TFTP_ERROR_TRANSPORT, "Poll failed");
			return;
		}

		if (ret == 0) {
			/* Timeout - no data available, continue polling */
			continue;
		}

		/* Data available - proceed to receive */
		from_len = sizeof(from_addr);
		ret = server->transport_ops->recv(server->transport_ctx, recv_buf, sizeof(recv_buf),
						  &from_addr, &from_len); /* Non-blocking recv */

		if (ret < 0) {
			if (ret == -EAGAIN || ret == -EWOULDBLOCK) {
				/* No data available - continue polling */
				continue;
			}

			tftp_server_handle_fatal_error(server, TFTP_ERROR_TRANSPORT,
						       "Receive failed");
			return;
		}

		if (ret < TFTP_HEADER_SIZE) {
			NET_ERR("Packet too short");
			continue;
		}

		/* Parse opcode */
		opcode = sys_get_be16(recv_buf);

		NET_DBG("Received packet: opcode=%u, len=%d", opcode, ret);

		/* Route packet to appropriate handler */
		switch (opcode) {
		case TFTP_OPCODE_RRQ:
		case TFTP_OPCODE_WRQ:
			tftp_process_request(server, recv_buf, ret, &from_addr, from_len);
			break;

		case TFTP_OPCODE_DATA:
			tftp_process_data(server, recv_buf, ret, &from_addr, from_len);
			break;

		case TFTP_OPCODE_ACK:
			tftp_process_ack(server, recv_buf, ret, &from_addr, from_len);
			break;

		case TFTP_OPCODE_ERROR:
			LOG_DBG("Received error packet from client");
			break;

		default:
			NET_ERR("Unknown opcode: %u", opcode);
			break;
		}
	}

	NET_INFO("TFTP server thread stopped");
}

/* Export server instance and initialization function */
struct tftp_server *tftp_server_get_instance(void)
{
	return &tftp_server_inst;
}

int tftp_server_init(struct tftp_server *server, struct tftp_transport_ops *transport_ops,
		     void *transport_ctx, const char *config,
		     tftp_server_error_cb_t server_error_cb,
		     tftp_session_error_cb_t session_error_cb)
{
	int ret;

	if (server == NULL || transport_ops == NULL || transport_ctx == NULL ||
	    transport_ops->init == NULL || transport_ops->recv == NULL ||
	    transport_ops->send == NULL) {
		return -EINVAL;
	}

	memset(server, 0, sizeof(*server));

	/* Initialize session pool */
	sys_slist_init(&free_sessions);
	for (int i = 0; i < CONFIG_TFTPS_MAX_CLIENTS; i++) {
		tftp_session_init(&session_pool[i]);
		sys_slist_prepend(&free_sessions, &session_pool[i].node);
	}

	sys_slist_init(&server->sessions);

	/* Initialize mutex for session management */
	ret = k_mutex_init(&server->sessions_mutex);
	if (ret != 0) {
		LOG_ERR("Failed to initialize mutex: %d", ret);
		return ret;
	}

	server->transport_ops = transport_ops;
	server->transport_ctx = transport_ctx;
	server->stack = tftp_server_stack;
	server->stack_size = CONFIG_TFTPS_STACK_SIZE;
	server->server_error_cb = server_error_cb;
	server->session_error_cb = session_error_cb;

	ret = server->transport_ops->init(server->transport_ctx, config);
	if (ret != 0) {
		return ret;
	}

	atomic_set(&server->running, 0);

	NET_INFO("TFTP server initialized");

	return 0;
}

/**
 * @brief Start TFTP server
 *
 * Starts the server thread that processes incoming packets.
 *
 * @param server Server context
 * @return 0 on success, negative errno on failure
 */
int tftp_server_start(struct tftp_server *server)
{
	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->running)) {
		return -EALREADY;
	}

	atomic_set(&server->running, 1);

	k_thread_create(&server->server_thread, server->stack, server->stack_size,
			tftp_server_thread, server, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	NET_INFO("TFTP server started");

	return 0;
}

/**
 * @brief Stop TFTP server
 *
 * Stops the server thread and cleans up resources.
 *
 * @param server Server context
 * @return 0 on success, negative errno on failure
 */
int tftp_server_stop(struct tftp_server *server)
{
	if (server == NULL) {
		return -EINVAL;
	}

	/* Set running flag to stop the server thread */
	atomic_set(&server->running, 0);

	/* Cleanup transport first to wake up the server thread if it's blocked in recv */
	if (server->transport_ops && server->transport_ops->cleanup) {
		server->transport_ops->cleanup(server->transport_ctx);
	}

	/* Wait for thread to finish */
	int ret = k_thread_join(&server->server_thread, K_FOREVER);

	if (ret != 0) {
		NET_ERR("Failed to join TFTP server thread: %d", ret);
		return ret;
	}

	/* Cleanup all sessions */
	struct tftp_session *session;

	ret = k_mutex_lock(&server->sessions_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to acquire mutex for session cleanup: %d", ret);
		return ret;
	}

	while ((session = SYS_SLIST_PEEK_HEAD_CONTAINER(&server->sessions, session, node)) !=
	       NULL) {
		tftp_session_free_internal(server, session);
	}

	ret = k_mutex_unlock(&server->sessions_mutex);
	if (ret != 0) {
		LOG_ERR("Failed to unlock mutex during cleanup: %d", ret);
		return ret;
	}

	NET_INFO("TFTP server stopped");

	return 0;
}
