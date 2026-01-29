/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_NET_COAP_CLIENT_TCP_H_
#define ZEPHYR_INCLUDE_NET_COAP_CLIENT_TCP_H_

/**
 * @brief CoAP TCP Client API
 * @defgroup coap_client_tcp CoAP TCP Client API
 * @ingroup networking
 * @{
 */

#include <zephyr/net/coap.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum size of a CoAP TCP message */
#define MAX_COAP_TCP_MSG_LEN (CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE + \
			      CONFIG_COAP_CLIENT_MESSAGE_SIZE)

/**
 * @brief Representation for CoAP TCP client response data.
 */
struct coap_client_tcp_response_data {
	/** Result code of the response. Negative if there was a failure. */
	int16_t result_code;
	/** A pointer to the response CoAP packet. NULL for error result. */
	const struct coap_packet *packet;
	/** Payload offset from the beginning of a blockwise transfer. */
	size_t offset;
	/** Buffer containing the payload from the response. NULL for empty payload. */
	const uint8_t *payload;
	/** Size of the payload. */
	size_t payload_len;
	/** Indicates the last block of the response. */
	bool last_block;
};

/**
 * @typedef coap_client_tcp_response_cb_t
 * @brief Callback for CoAP TCP request.
 *
 * @param data The CoAP response data.
 * @param user_data User provided context.
 */
typedef void (*coap_client_tcp_response_cb_t)(const struct coap_client_tcp_response_data *data,
					      void *user_data);

/**
 * @typedef coap_client_tcp_payload_cb_t
 * @brief Callback for providing a payload for the CoAP TCP request.
 *
 * @param offset Payload offset from the beginning of a blockwise transfer.
 * @param payload A pointer for the buffer containing the payload block.
 * @param len Requested (maximum) block size on input. The actual payload length on output.
 * @param last_block A pointer to the flag indicating whether more payload blocks are expected.
 * @param user_data User provided context.
 *
 * @return Zero on success, a negative error code to abort upload.
 */
typedef int (*coap_client_tcp_payload_cb_t)(size_t offset, const uint8_t **payload,
					    size_t *len, bool *last_block,
					    void *user_data);

typedef int (*coap_client_tcp_socket_config_cb_t)(int fd, void *user_data);

/* Forward declaration for event callback */
struct coap_client_tcp;

/**
 * @brief CoAP TCP client event types for signaling
 */
enum coap_client_tcp_event {
	/** CSM capabilities have been updated from server */
	COAP_CLIENT_TCP_EVENT_CSM_UPDATED,
	/** Pong received in response to Ping */
	COAP_CLIENT_TCP_EVENT_PONG_RECEIVED,
	/** Server initiated connection release (RFC 8323) */
	COAP_CLIENT_TCP_EVENT_RELEASE,
	/** Server aborted connection (RFC 8323) */
	COAP_CLIENT_TCP_EVENT_ABORT,
};

/**
 * @brief Event data for CoAP TCP signaling events
 */
union coap_client_tcp_event_data {
	/** Data for RELEASE event */
	struct {
		/** Alternative address to reconnect (NULL if not provided) */
		const char *alt_addr;
		/** Hold-off time in seconds before reconnecting */
		uint32_t hold_off_sec;
	} release;
	/** Data for ABORT event */
	struct {
		/** Bad CSM option code that caused abort (0 if not provided) */
		uint16_t bad_csm_option;
	} abort;
};

/**
 * @typedef coap_client_tcp_event_cb_t
 * @brief Callback for CoAP TCP signaling events (Release/Abort)
 *
 * @param client The client instance that received the event.
 * @param event The signaling event type.
 * @param data Event-specific data (may be NULL for some events).
 * @param user_data User provided context.
 */
typedef void (*coap_client_tcp_event_cb_t)(struct coap_client_tcp *client,
					   enum coap_client_tcp_event event,
					   const union coap_client_tcp_event_data *data,
					   void *user_data);

/**
 * @brief Representation of extra options for the CoAP TCP client request
 */
struct coap_client_tcp_option {
	/** Option code */
	uint16_t code;
#if defined(CONFIG_COAP_EXTENDED_OPTIONS_LEN)
	/** Option len */
	uint16_t len;
	/** Buffer for the length */
	uint8_t value[CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE];
#else
	/** Option len */
	uint8_t len;
	/** Buffer for the length */
	uint8_t value[12];
#endif
};

/** @cond INTERNAL_HIDDEN */
#define MAX_TCP_PATH_SIZE (CONFIG_COAP_CLIENT_MAX_PATH_LENGTH + 1)
#define MAX_TCP_EXTRA_OPTIONS CONFIG_COAP_CLIENT_MAX_EXTRA_OPTIONS
/** @endcond */

/**
 * @brief Representation of a CoAP TCP client request.
 */
struct coap_client_tcp_request {
	enum coap_method method;                      /**< Method of the request */
	char path[MAX_TCP_PATH_SIZE];                 /**< Path of the requested resource */
	enum coap_content_format fmt;                 /**< Content format to be used */
	const uint8_t *payload;                       /**< User allocated buffer for send request */
	size_t len;                                   /**< Length of the payload */
	coap_client_tcp_payload_cb_t payload_cb;      /**< Optional payload callback */
	coap_client_tcp_response_cb_t cb;             /**< Callback when response received */
	struct coap_client_tcp_option
		options[MAX_TCP_EXTRA_OPTIONS];       /**< Extra options to be added to request */
	uint8_t num_options;                          /**< Number of extra options */
	void *user_data;                              /**< User provided context */
};

/** @cond INTERNAL_HIDDEN */
struct coap_client_tcp_internal_request {
	uint8_t request_token[COAP_TOKEN_MAX_LEN];
	uint32_t offset;
	uint8_t request_tkl;
	bool request_ongoing;
	atomic_t in_callback;
	struct coap_block_context recv_blk_ctx;
	struct coap_block_context send_blk_ctx;
	struct coap_client_tcp_request coap_request;
	struct coap_packet request;
	uint8_t request_tag[COAP_TOKEN_MAX_LEN];
	uint8_t send_buf[MAX_COAP_TCP_MSG_LEN];

	/* TCP-specific timeout handling (no retransmission) */
	int64_t tcp_t0;
	uint32_t tcp_timeout_ms;

	/* For blockwise transfers */
	uint32_t last_payload_len;

	/* For GETs with observe option set */
	bool is_observe;
};

struct coap_client_tcp {
	int fd;
	struct k_mutex lock;
	uint8_t recv_buf[MAX_COAP_TCP_MSG_LEN];
	size_t recv_offset; /* Bytes received so far into recv_buf */
	struct coap_client_tcp_internal_request requests[CONFIG_COAP_CLIENT_MAX_REQUESTS];
	struct coap_option echo_option;
	bool send_echo;

	/* Max block/message size from CSM negotiation */
	uint32_t max_block_size;

	/* Blockwise transfer capability (negotiated via CSM BWT option) */
	bool blockwise_enabled;

	/* Ping/Pong tracking */
	bool ping_pending;
	int64_t ping_t0;

	/* Event callback for Release/Abort signals */
	coap_client_tcp_event_cb_t event_cb;
	void *event_cb_user_data;

	coap_client_tcp_socket_config_cb_t socket_config_cb;
	void *socket_config_cb_user_data;
};
/** @endcond */

/**
 * @brief Initialize the TCP CoAP client.
 *
 * @param[in] client Client instance.
 * @param[in] info Name for the receiving thread (NULL for default).
 *
 * @return 0 on success, negative error code otherwise.
 */
int coap_client_tcp_init(struct coap_client_tcp *client, const char *info);

int coap_client_tcp_connect(struct coap_client_tcp *client, const struct net_sockaddr *addr,
			    net_socklen_t addrlen, int proto);

int coap_client_tcp_close(struct coap_client_tcp *client);

/**
 * @brief Send CoAP request over TCP
 *
 * @param client Client instance (must be connected via coap_client_tcp_connect).
 * @param req CoAP request structure.
 * @return 0 on success, -ENOTCONN if not connected, negative error code otherwise.
 */
int coap_client_tcp_req(struct coap_client_tcp *client,
			struct coap_client_tcp_request *req);

/**
 * @brief Send CSM (Capabilities and Settings Message) over TCP
 *
 * Per RFC 8323, CSM should be exchanged when a CoAP-over-TCP connection is
 * established. This negotiates capabilities like max message size and BERT.
 *
 * @param client Client instance (must be connected via coap_client_tcp_connect).
 * @param max_block_size Maximum block size to advertise.
 * @param cb Response callback (can be NULL since server may not respond).
 * @param user_data User data for callback.
 * @return 0 on success, -ENOTCONN if not connected, negative error code otherwise.
 */
int coap_client_tcp_csm_req(struct coap_client_tcp *client,
			    uint32_t max_block_size, coap_client_tcp_response_cb_t cb,
			    void *user_data);

/**
 * @brief Cancel all current TCP requests.
 *
 * @param client Client instance.
 */
void coap_client_tcp_cancel_requests(struct coap_client_tcp *client);

/**
 * @brief Cancel and fully reset all requests when changing socket.
 *
 * Use when tearing down a transport to ensure clean state for a new socket.
 *
 * @param client Client instance.
 */
void coap_client_tcp_cancel_and_reset_all(struct coap_client_tcp *client);

/**
 * @brief Get initial Block2 option for TCP (BERT-aware)
 *
 * @param client Client instance.
 * @return CoAP client initial Block2 option structure.
 */
struct coap_client_tcp_option coap_client_tcp_option_initial_block2(struct coap_client_tcp *client);

/**
 * @brief Check if TCP client has ongoing exchange.
 *
 * @param client Pointer to the CoAP TCP client instance.
 * @return true if there is an ongoing exchange, false otherwise.
 */
bool coap_client_tcp_has_ongoing_exchange(struct coap_client_tcp *client);

/**
 * @brief Send a Ping signal to the server (RFC 8323)
 *
 * This sends a Ping signal and waits for Pong response. Can be used
 * for connection keep-alive or RTT measurement.
 *
 * @param client Client instance (must be connected).
 * @return 0 on success, negative error code otherwise.
 */
int coap_client_tcp_ping(struct coap_client_tcp *client);

/**
 * @brief Send a Release signal to the server (RFC 8323)
 *
 * This signals graceful connection termination. The server should
 * complete pending requests before closing.
 *
 * @param client Client instance (must be connected).
 * @param alt_addr Alternative address suggestion (NULL if none).
 * @param hold_off_sec Hold-off time in seconds (0 if none).
 * @return 0 on success, negative error code otherwise.
 */
int coap_client_tcp_release(struct coap_client_tcp *client,
			    const char *alt_addr, uint32_t hold_off_sec);

/**
 * @brief Set callback for signaling events (Release/Abort)
 *
 * @param client Client instance.
 * @param cb Callback function to invoke on events.
 * @param user_data User data passed to callback.
 */
void coap_client_tcp_set_event_cb(struct coap_client_tcp *client,
				  coap_client_tcp_event_cb_t cb,
				  void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_COAP_CLIENT_TCP_H_ */
