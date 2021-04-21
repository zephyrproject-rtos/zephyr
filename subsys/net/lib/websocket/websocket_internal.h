/** @file
 @brief Websocket private header

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain/common.h>

#define WS_SHA1_OUTPUT_LEN 20

/* Min Websocket header length */
#define MIN_HEADER_LEN 2

/* Max Websocket header length */
#define MAX_HEADER_LEN 14

/* From RFC 6455 chapter 4.2.2 */
#define WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/**
 * Websocket connection information
 */
__net_socket struct websocket_context {
	union {
		/** User data.
		 */
		void *user_data;

		/** This is used during HTTP handshake to verify that the
		 * peer sent proper Sec-WebSocket-Accept key.
		 */
		uint8_t *sec_accept_key;
	};

	/** Reference count.
	 */
	atomic_t refcount;

	/** Internal lock for protecting this context from multiple access.
	 */
	struct k_mutex lock;

	/* The socket number is valid only after HTTP handshake is done
	 * so we can share the memory for these.
	 */
	union {
		/** HTTP parser settings for the application usage */
		const struct http_parser_settings *http_cb;

		/** The Websocket socket id. If data is sent via this socket, it
		 * will automatically add Websocket headers etc into the data.
		 */
		int sock;
	};

	/** Temporary buffers used for HTTP handshakes and Websocket protocol
	 * headers. User must provide the actual buffer where the headers are
	 * stored temporarily.
	 */
	uint8_t *tmp_buf;

	/** Temporary buffer length.
	 */
	size_t tmp_buf_len;

	/** Current reading position in the tmp_buf
	 */
	size_t tmp_buf_pos;

	/** The real TCP socket to use when sending Websocket data to peer.
	 */
	int real_sock;

	/** Websocket connection masking value */
	uint32_t masking_value;

	/** Amount of data received. */
	uint64_t total_read;

	/** Message length */
	uint64_t message_len;

	/** Message type */
	uint32_t message_type;

	/** Is the message masked */
	uint8_t masked : 1;

	/** Did we receive Sec-WebSocket-Accept: field */
	uint8_t sec_accept_present : 1;

	/** Is Sec-WebSocket-Accept field correct */
	uint8_t sec_accept_ok : 1;

	/** Did we receive all from peer during HTTP handshake */
	uint8_t all_received : 1;

	/** Header received */
	uint8_t header_received : 1;
};

/**
 * @brief Disconnect the Websocket.
 *
 * @param sock Websocket id returned by websocket_connect() call.
 *
 * @return 0 if ok, <0 if error
 */
int websocket_disconnect(int sock);

/**
 * @typedef websocket_context_cb_t
 * @brief Callback used while iterating over websocket contexts
 *
 * @param context A valid pointer on current websocket context
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*websocket_context_cb_t)(struct websocket_context *ctx,
				       void *user_data);

/**
 * @brief Iterate over websocket context. This is mainly used by net-shell
 * to show information about websockets.
 *
 * @param cb Websocket context callback
 * @param user_data Caller specific data.
 */
void websocket_context_foreach(websocket_context_cb_t cb, void *user_data);
