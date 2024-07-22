/** @file
 * @brief CoAP client API
 *
 * An API for applications to do CoAP requests
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_NET_COAP_CLIENT_H_
#define ZEPHYR_INCLUDE_NET_COAP_CLIENT_H_

/**
 * @brief CoAP client API
 * @defgroup coap_client CoAP client API
 * @ingroup networking
 * @{
 */

#include <zephyr/net/coap.h>
#include <zephyr/kernel.h>

/** Maximum size of a CoAP message */
#define MAX_COAP_MSG_LEN (CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE + \
			  CONFIG_COAP_CLIENT_MESSAGE_SIZE)

/**
 * @typedef coap_client_response_cb_t
 * @brief Callback for CoAP request.
 *
 * This callback is called for responses to CoAP client requests.
 * It is used to indicate errors, response codes from server or to deliver payload.
 * Blockwise transfers cause this callback to be called sequentially with increasing payload offset
 * and only partial content in buffer pointed by payload parameter.
 *
 * @param result_code Result code of the response. Negative if there was a failure in send.
 *                    @ref coap_response_code for positive.
 * @param offset Payload offset from the beginning of a blockwise transfer.
 * @param payload Buffer containing the payload from the response. NULL for empty payload.
 * @param len Size of the payload.
 * @param last_block Indicates the last block of the response.
 * @param user_data User provided context.
 */
typedef void (*coap_client_response_cb_t)(int16_t result_code,
					  size_t offset, const uint8_t *payload, size_t len,
					  bool last_block, void *user_data);

/**
 * @brief Representation of a CoAP client request.
 */
struct coap_client_request {
	enum coap_method method;            /**< Method of the request */
	bool confirmable;	            /**< CoAP Confirmable/Non-confirmable message */
	const char *path;	            /**< Path of the requested resource */
	enum coap_content_format fmt;       /**< Content format to be used */
	uint8_t *payload;	            /**< User allocated buffer for send request */
	size_t len;		            /**< Length of the payload */
	coap_client_response_cb_t cb;       /**< Callback when response received */
	struct coap_client_option *options; /**< Extra options to be added to request */
	uint8_t num_options;                /**< Number of extra options */
	void *user_data;	            /**< User provided context */
};

/**
 * @brief Representation of extra options for the CoAP client request
 */
struct coap_client_option {
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
struct coap_client_internal_request {
	uint8_t request_token[COAP_TOKEN_MAX_LEN];
	uint32_t offset;
	uint32_t last_id;
	uint8_t request_tkl;
	bool request_ongoing;
	atomic_t in_callback;
	struct coap_block_context recv_blk_ctx;
	struct coap_block_context send_blk_ctx;
	struct coap_pending pending;
	struct coap_client_request coap_request;
	struct coap_packet request;
	uint8_t request_tag[COAP_TOKEN_MAX_LEN];

	/* For GETs with observe option set */
	bool is_observe;
	int last_response_id;
};

struct coap_client {
	int fd;
	struct sockaddr address;
	socklen_t socklen;
	bool response_ready;
	struct k_mutex lock;
	uint8_t send_buf[MAX_COAP_MSG_LEN];
	uint8_t recv_buf[MAX_COAP_MSG_LEN];
	struct coap_client_internal_request requests[CONFIG_COAP_CLIENT_MAX_REQUESTS];
	struct coap_option echo_option;
	bool send_echo;
};
/** @endcond */

/**
 * @brief Initialize the CoAP client.
 *
 * @param[in] client Client instance.
 * @param[in] info Name for the receiving thread of the client. Setting this NULL will result as
 *                 default name of "coap_client".
 *
 * @return int Zero on success, otherwise a negative error code.
 */
int coap_client_init(struct coap_client *client, const char *info);

/**
 * @brief Send CoAP request
 *
 * Operation is handled asynchronously using a background thread.
 * If the socket isn't connected to a destination address, user must provide a destination address,
 * otherwise the address should be set as NULL.
 * Once the callback is called with last block set as true, socket can be closed or
 * used for another query.
 *
 * @param client Client instance.
 * @param sock Open socket file descriptor.
 * @param addr the destination address of the request, NULL if socket is already connected.
 * @param req CoAP request structure
 * @param params Pointer to transmission parameters structure or NULL to use default values.
 * @return zero when operation started successfully or negative error code otherwise.
 */

int coap_client_req(struct coap_client *client, int sock, const struct sockaddr *addr,
		    struct coap_client_request *req, struct coap_transmission_parameters *params);

/**
 * @brief Cancel all current requests.
 *
 * This is intended for canceling long-running requests (e.g. GETs with the OBSERVE option set)
 * which has gone stale for some reason.
 *
 * @param client Client instance.
 */
void coap_client_cancel_requests(struct coap_client *client);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_COAP_CLIENT_H_ */
