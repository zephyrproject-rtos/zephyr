/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2025 Ellenby Technologies Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file coap_client_tcp.c
 * @brief CoAP Client for reliable transports (TCP/TLS/WebSockets)
 *
 * This implements CoAP over reliable transports per RFC 8323.
 * Key differences from UDP CoAP:
 * - No retransmission (transport handles reliability)
 * - Different header format (no Type/Message ID)
 * - Extended length encoding for larger messages
 * - CSM (Capabilities and Settings Message) signaling
 * - BERT (Block-wise Extension for Reliable Transport) support
 */

#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/socket.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client_tcp.h>

#define COAP_PERIODIC_TIMEOUT 500

/* RFC 8323 Section 5.3.1: Base value for Max-Message-Size */
#define COAP_TCP_DEFAULT_MAX_MESSAGE_SIZE 1152

static K_MUTEX_DEFINE(coap_client_tcp_mutex);
static struct coap_client_tcp *tcp_clients[CONFIG_COAP_CLIENT_MAX_INSTANCES];
static int num_tcp_clients;
static K_SEM_DEFINE(coap_client_tcp_recv_sem, 0, 1);

/* Forward declarations */
static bool timeout_expired(struct coap_client_tcp_internal_request *internal_req);
static void cancel_requests_with(struct coap_client_tcp *client, int error);
static int recv_response_tcp(struct coap_client_tcp *client, struct coap_packet *response);
static int handle_response_tcp(struct coap_client_tcp *client, const struct coap_packet *response);
static void report_callback_error(struct coap_client_tcp_internal_request *internal_req,
				  int error_code);

/**
 * @brief Send a CoAP message over TCP
 */
static int send_request_tcp(int sock, const void *buf, size_t len, int flags)
{
	int ret;

	size_t bytes_written = 0;

	while (len > bytes_written) {
		ret = zsock_send(sock, buf, len, flags);
		if (ret < 0) {
			return -errno;
		}
		bytes_written += ret;
	}

	return bytes_written;
}

/**
 * @brief Receive a complete CoAP message from TCP socket (non-blocking state machine)
 *
 * CoAP over TCP uses a length-prefixed framing. The first byte contains:
 * - Bits 7-4: Length field (Len)
 * - Bits 3-0: Token Length (TKL)
 *
 * If Len is 0-12: Options and payload length is Len bytes
 * If Len is 13: Extended length follows (1 byte, value = ext + 13)
 * If Len is 14: Extended length follows (2 bytes, value = ext + 269)
 * If Len is 15: Extended length follows (4 bytes, value = ext + 65805)
 *
 * This function tracks partial receives in client->recv_offset and returns
 * -EAGAIN if the packet is incomplete. On next call, it resumes from where
 * it left off without busy-looping.
 */
static int receive_tcp(struct coap_client_tcp *client, int flags)
{
	uint8_t *buf = client->recv_buf;
	size_t offset = client->recv_offset;
	ssize_t received;
	uint8_t len_field;
	uint8_t tkl;
	size_t header_size;
	uint32_t opt_payload_len;
	size_t total_expected;

	/* Step 1: Try to receive more data (single non-blocking call) */
	received = zsock_recv(client->fd, buf + offset,
			      sizeof(client->recv_buf) - offset, flags);
	if (received < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -EAGAIN;
		}
		LOG_ERR("Error receiving: %d", -errno);
		return -errno;
	}
	if (received == 0) {
		LOG_ERR("Connection closed");
		return -ECONNRESET;
	}

	offset += received;
	client->recv_offset = offset;

	/* Step 2: Check if we have enough bytes to parse header */
	if (offset < 1) {
		return -EAGAIN;
	}

	/* Step 3: Parse header to determine total packet size */
	len_field = (buf[0] >> 4) & 0x0F;
	tkl = buf[0] & 0x0F;

	if (tkl > COAP_TOKEN_MAX_LEN) {
		LOG_ERR("[RX/TCP] Invalid token length: %d", tkl);
		client->recv_offset = 0;
		return -EINVAL;
	}

	header_size = 1;
	opt_payload_len = len_field;

	/* Handle extended length encoding */
	if (len_field >= COAP_TCP_HEADER_LEN_EXT_1B) {
		size_t ext_bytes = 1 << (len_field - COAP_TCP_HEADER_LEN_EXT_1B);

		header_size += ext_bytes;

		if (offset < header_size) {
			return -EAGAIN;
		}

		/* Parse extended length */
		uint32_t ext_val = 0;

		for (size_t i = 0; i < ext_bytes; i++) {
			ext_val = (ext_val << 8) | buf[1 + i];
		}

		switch (len_field) {
		case COAP_TCP_HEADER_LEN_EXT_1B:
			opt_payload_len = ext_val + COAP_TCP_HEADER_LEN_EXT_0B_MAX;
			break;
		case COAP_TCP_HEADER_LEN_EXT_2B:
			opt_payload_len = ext_val + COAP_TCP_HEADER_LEN_EXT_1B_MAX;
			break;
		case COAP_TCP_HEADER_LEN_EXT_4B:
			opt_payload_len = ext_val + COAP_TCP_HEADER_LEN_EXT_2B_MAX;
			break;
		}
	}

	/* Step 4: Calculate total expected packet size */
	total_expected = header_size + 1 + tkl + opt_payload_len;

	if (total_expected > sizeof(client->recv_buf)) {
		LOG_ERR("Packet too large: %zu > %zu", total_expected, sizeof(client->recv_buf));
		client->recv_offset = 0;
		return -EMSGSIZE;
	}

	/* Step 5: Check if we have complete packet */
	if (offset < total_expected) {
		return -EAGAIN;
	}

	/* Step 6: Complete packet found - return its size. */
	return total_expected;
}

static void reset_internal_request(struct coap_client_tcp_internal_request *request)
{
	memset(request, 0, sizeof(*request));
}

static void release_internal_request(struct coap_client_tcp_internal_request *request)
{
	request->request_ongoing = false;
	request->tcp_t0 = 0;
	request->tcp_timeout_ms = 0;
}

static int coap_client_tcp_schedule_poll(struct coap_client_tcp *client,
					 struct coap_client_tcp_request *req,
					 struct coap_client_tcp_internal_request *internal_req)
{
	memcpy(&internal_req->coap_request, req, sizeof(struct coap_client_tcp_request));
	internal_req->request_ongoing = true;

	k_sem_give(&coap_client_tcp_recv_sem);

	return 0;
}

static bool exchange_lifetime_exceeded(struct coap_client_tcp_internal_request *internal_req)
{
	int64_t time_since_t0;

	if (internal_req->is_observe && internal_req->request_ongoing) {
		return false;
	}

	if (internal_req->tcp_t0 == 0) {
		return true;
	}

	time_since_t0 = k_uptime_get() - internal_req->tcp_t0;
	return time_since_t0 > CONFIG_COAP_CLIENT_TCP_EXCHANGE_LIFETIME;
}

static bool has_ongoing_request(struct coap_client_tcp *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing) {
			return true;
		}
	}
	return false;
}

static bool has_ongoing_exchange(struct coap_client_tcp *client)
{
	/* Check if ping is pending */
	if (client->ping_pending) {
		return true;
	}

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing &&
		    !exchange_lifetime_exceeded(&client->requests[i])) {
			return true;
		}
	}
	return false;
}

static bool has_timeout_expired(struct coap_client_tcp *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (timeout_expired(&client->requests[i])) {
			return true;
		}
	}
	return false;
}

static struct coap_client_tcp_internal_request *get_free_request(struct coap_client_tcp *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (!client->requests[i].request_ongoing &&
		    exchange_lifetime_exceeded(&client->requests[i])) {
			return &client->requests[i];
		}
	}

	/* Find oldest non-observe request to reuse */
	struct coap_client_tcp_internal_request *oldest = NULL;
	int64_t oldest_time = INT64_MAX;

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing &&
		    !client->requests[i].is_observe &&
		    exchange_lifetime_exceeded(&client->requests[i])) {
			int64_t request_time = client->requests[i].tcp_t0;

			if (request_time > 0 && request_time < oldest_time) {
				oldest_time = request_time;
				oldest = &client->requests[i];
			}
		}
	}

	if (oldest) {
		report_callback_error(oldest, -ECANCELED);
		release_internal_request(oldest);
		return oldest;
	}

	return NULL;
}

static bool has_ongoing_exchanges(void)
{
	for (int i = 0; i < num_tcp_clients; i++) {
		if (has_ongoing_exchange(tcp_clients[i])) {
			return true;
		}
	}
	return false;
}

static int coap_client_tcp_init_request(struct coap_client_tcp *client,
					struct coap_client_tcp_request *req,
					struct coap_client_tcp_internal_request *internal_req,
					bool reconstruct)
{
	int ret = 0;
	bool block2 = false;

	memset(internal_req->send_buf, 0, sizeof(internal_req->send_buf));

	/* Generate new token unless reconstructing blockwise with token reuse enabled */
	if (!reconstruct || !IS_ENABLED(CONFIG_COAP_CLIENT_TCP_BLOCKWISE_REUSE_TOKEN)) {
		uint8_t *token = coap_next_token();

		internal_req->request_tkl = COAP_TOKEN_MAX_LEN & 0x0F;
		memcpy(internal_req->request_token, token, internal_req->request_tkl);
	}

	/* Initialize TCP CoAP packet */
	ret = coap_tcp_packet_init(&internal_req->request, internal_req->send_buf,
				   MAX_COAP_TCP_MSG_LEN, COAP_TOKEN_MAX_LEN,
				   internal_req->request_token, req->method);
	if (ret < 0) {
		LOG_ERR("Failed to init CoAP TCP message: %d", ret);
		return ret;
	}

	/* Add path options if present */
	if (req->path[0] != '\0') {
		ret = coap_packet_set_path(&internal_req->request, req->path);
		if (ret < 0) {
			LOG_ERR("Failed to parse path: %d", ret);
			return ret;
		}
	}

	/* Add content format if there is a payload */
	if (req->payload || req->payload_cb) {
		ret = coap_append_option_int(&internal_req->request,
					     COAP_OPTION_CONTENT_FORMAT, req->fmt);
		if (ret < 0) {
			LOG_ERR("Failed to append content format option");
			return ret;
		}
	}

	/* Blockwise receive ongoing, request next block */
	if (internal_req->recv_blk_ctx.current > 0) {
		block2 = true;

		ret = coap_tcp_append_block2_option(&internal_req->request,
						    &internal_req->recv_blk_ctx);
		if (ret < 0) {
			LOG_ERR("Failed to append block2 option");
			return ret;
		}
	}

	/* Add extra options */
	for (int i = 0; i < req->num_options; i++) {
		if (COAP_OPTION_BLOCK2 == req->options[i].code && block2) {
			continue;
		}

		ret = coap_packet_append_option(&internal_req->request, req->options[i].code,
						req->options[i].value, req->options[i].len);
		if (ret < 0) {
			LOG_ERR("Failed to append option %d", req->options[i].code);
			return ret;
		}
	}

	/* Handle payload */
	if (req->payload || req->payload_cb) {
		uint16_t payload_len;
		uint16_t offset;
		const uint8_t *payload_ptr = req->payload;
		size_t total_len = req->len;

		/* Use payload callback if provided */
		if (req->payload_cb && !reconstruct) {
			bool last_block = true;
			size_t cb_len = CONFIG_COAP_CLIENT_MESSAGE_SIZE;

			ret = req->payload_cb(0, &payload_ptr, &cb_len, &last_block,
					      req->user_data);
			if (ret < 0) {
				LOG_ERR("Payload callback failed: %d", ret);
				return ret;
			}
			total_len = cb_len;
			if (!last_block) {
				/* Trigger blockwise - we don't know total size yet */
				total_len = cb_len + 1;
			}
		}

		/* Check if blockwise send is needed.
		 * Only use blockwise if enabled via CSM negotiation AND payload exceeds limit
		 * or we're continuing a blockwise transfer.
		 */
		bool use_blockwise = client->blockwise_enabled &&
				     (internal_req->send_blk_ctx.total_size > 0 ||
				      total_len > CONFIG_COAP_CLIENT_MESSAGE_SIZE);

		if (use_blockwise) {
			if (internal_req->send_blk_ctx.total_size == 0) {
				coap_block_transfer_init(&internal_req->send_blk_ctx,
					coap_bytes_to_block_size(client->max_block_size),
					total_len);
				uint8_t *tag = coap_next_token();

				memcpy(internal_req->request_tag, tag, COAP_TOKEN_MAX_LEN);
			}

			ret = coap_append_block1_option(&internal_req->request,
							&internal_req->send_blk_ctx);
			if (ret < 0) {
				LOG_ERR("Failed to append block1 option");
				return ret;
			}

			ret = coap_packet_append_option(&internal_req->request,
							COAP_OPTION_REQUEST_TAG,
							internal_req->request_tag,
							COAP_TOKEN_MAX_LEN);
			if (ret < 0) {
				LOG_ERR("Failed to append request tag option");
				return ret;
			}
		} else if (total_len > CONFIG_COAP_CLIENT_MESSAGE_SIZE) {
			/* TCP streaming mode - no Block options, TCP handles large messages */
			LOG_DBG("Using TCP streaming for large payload (%zu bytes)", total_len);
		}

		ret = coap_packet_append_payload_marker(&internal_req->request);
		if (ret < 0) {
			LOG_ERR("Failed to append payload marker");
			return ret;
		}

		if (use_blockwise && internal_req->send_blk_ctx.total_size > 0) {
			/* Blockwise mode: chunk the payload */
			payload_len = internal_req->send_blk_ctx.total_size -
				      internal_req->send_blk_ctx.current;

			if (COAP_BLOCK_BERT == internal_req->send_blk_ctx.block_size) {
				if (payload_len > CONFIG_COAP_CLIENT_MESSAGE_SIZE) {
					payload_len = 1024 *
						(CONFIG_COAP_CLIENT_MESSAGE_SIZE / 1024);
				}
			} else {
				uint16_t block_bytes = coap_block_size_to_bytes(
					internal_req->send_blk_ctx.block_size);

				if (payload_len > block_bytes) {
					payload_len = block_bytes;
				}
			}

			offset = internal_req->send_blk_ctx.current;
		} else {
			/* Non-blockwise mode: send entire payload (TCP handles fragmentation) */
			payload_len = total_len;
			offset = 0;
		}

		internal_req->last_payload_len = payload_len;

		ret = coap_packet_append_payload(&internal_req->request, payload_ptr + offset,
						 payload_len);
		if (ret < 0) {
			LOG_ERR("Failed to append payload");
			return ret;
		}

		if (use_blockwise && internal_req->send_blk_ctx.total_size > 0) {
			coap_tcp_next_block(&internal_req->request, &internal_req->send_blk_ctx);
		}
	}

	return ret;
}

int coap_client_tcp_req(struct coap_client_tcp *client,
			struct coap_client_tcp_request *req)
{
	int ret;
	struct coap_client_tcp_internal_request *internal_req;

	if (client == NULL || req == NULL) {
		return -EINVAL;
	}

	if (client->fd < 0) {
		return -ENOTCONN;
	}

	/* For CSM, path is empty which is valid */
	if (req->method != (enum coap_method)COAP_SIGNAL_CODE_CSM && req->path[0] == '\0') {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	internal_req = get_free_request(client);
	if (internal_req == NULL) {
		LOG_DBG("No more free requests");
		ret = -EAGAIN;
		goto out;
	}

	reset_internal_request(internal_req);

	ret = coap_client_tcp_init_request(client, req, internal_req, false);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CoAP request");
		goto release;
	}

	if (client->send_echo) {
		ret = coap_packet_append_option(&internal_req->request, COAP_OPTION_ECHO,
						client->echo_option.value, client->echo_option.len);
		if (ret < 0) {
			LOG_ERR("Failed to append echo option");
			goto release;
		}
		client->send_echo = false;
	}

	ret = coap_tcp_packet_update_len(&internal_req->request);
	if (ret < 0) {
		LOG_ERR("Failed to update packet length");
		goto release;
	}

	ret = coap_client_tcp_schedule_poll(client, req, internal_req);
	if (ret < 0) {
		LOG_ERR("Failed to schedule polling");
		goto release;
	}

	internal_req->tcp_t0 = k_uptime_get();
	internal_req->tcp_timeout_ms = CONFIG_COAP_CLIENT_TCP_REQUEST_TIMEOUT;

	internal_req->is_observe = coap_request_is_observe(&internal_req->request);
	LOG_DBG("Request is_observe %d", internal_req->is_observe);

	ret = send_request_tcp(client->fd, internal_req->request.data,
			       internal_req->request.offset, 0);
	if (ret < 0) {
		ret = -errno;
	}

release:
	if (ret < 0) {
		LOG_ERR("Failed to send request: %d", ret);
		reset_internal_request(internal_req);
	} else {
		ret = 0;
	}
out:
	k_mutex_unlock(&client->lock);
	return ret;
}

static int write_csm_option_value_u32(struct coap_client_tcp_option *option, size_t length,
				      uint32_t val)
{
	unsigned int n, i;

	for (n = 0, i = val; i && n < sizeof(val); ++n) {
		i >>= 8;
	}

	if (n > length) {
		return 0;
	}

	i = n;
	while (i--) {
		option->value[i] = val & 0xff;
		val >>= 8;
	}

	return n;
}

static struct coap_client_tcp_option tcp_csm_options[2];

int coap_client_tcp_csm_req(struct coap_client_tcp *client,
			    uint32_t max_block_size, coap_client_tcp_response_cb_t cb,
			    void *user_data)
{
	struct coap_client_tcp_request req;

	client->max_block_size = max_block_size;

	memset(&tcp_csm_options[0], 0, sizeof(tcp_csm_options));

	/* Option 2: Max-Message-Size */
	tcp_csm_options[0].code = COAP_OPTION_SIGNAL_701_MMS;
	tcp_csm_options[0].len = write_csm_option_value_u32(&tcp_csm_options[0], 4, max_block_size);

	/* Option 4: Block-Wise-Transfer (empty = BERT support) */
	tcp_csm_options[1].code = COAP_OPTION_SIGNAL_701_BWT;
	tcp_csm_options[1].len = 0;

	memset(&req, 0, sizeof(req));
	req.method = (enum coap_method)COAP_SIGNAL_CODE_CSM;
	/* path stays empty for CSM */
	req.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN;
	req.payload = NULL;
	req.len = 0;
	req.cb = cb;
	memcpy(&req.options[0], &tcp_csm_options[0], sizeof(tcp_csm_options));
	req.num_options = 2;
	req.user_data = user_data;

	return coap_client_tcp_req(client, &req);
}

static void report_callback_error(struct coap_client_tcp_internal_request *internal_req,
				  int error_code)
{
	if (internal_req->coap_request.cb) {
		if (!atomic_set(&internal_req->in_callback, 1)) {
			struct coap_client_tcp_response_data data = {
				.result_code = error_code,
				.packet = NULL,
				.offset = 0,
				.payload = NULL,
				.payload_len = 0,
				.last_block = true,
			};
			internal_req->coap_request.cb(&data, internal_req->coap_request.user_data);
			atomic_clear(&internal_req->in_callback);
		} else {
			LOG_DBG("Cannot call the callback; already in it.");
		}
	}
}

static bool timeout_expired(struct coap_client_tcp_internal_request *internal_req)
{
	if (internal_req->tcp_timeout_ms == 0) {
		return false;
	}
	return (internal_req->request_ongoing &&
		internal_req->tcp_timeout_ms <= (k_uptime_get() - internal_req->tcp_t0));
}

static void coap_client_tcp_timeout_handler(struct coap_client_tcp *client)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (timeout_expired(&client->requests[i])) {
			report_callback_error(&client->requests[i], -ETIMEDOUT);
			release_internal_request(&client->requests[i]);
		}
	}

	k_mutex_unlock(&client->lock);
}

static struct coap_client_tcp *get_tcp_client(int sock)
{
	for (int i = 0; i < num_tcp_clients; i++) {
		if (tcp_clients[i]->fd == sock) {
			return tcp_clients[i];
		}
	}
	return NULL;
}

static int handle_poll(void)
{
	int ret = 0;
	struct zsock_pollfd fds[CONFIG_COAP_CLIENT_MAX_INSTANCES] = {0};
	int nfds = 0;

	for (int i = 0; i < num_tcp_clients; i++) {
		if (!has_ongoing_exchange(tcp_clients[i])) {
			continue;
		}
		fds[nfds].fd = tcp_clients[i]->fd;
		fds[nfds].events = ZSOCK_POLLIN;
		fds[nfds].revents = 0;
		nfds++;
	}

	ret = zsock_poll(fds, nfds, COAP_PERIODIC_TIMEOUT);
	if (ret < 0) {
		return -errno;
	}

	/* Check timeouts for all clients after poll (including on timeout) */
	for (int i = 0; i < num_tcp_clients; i++) {
		if (has_timeout_expired(tcp_clients[i])) {
			coap_client_tcp_timeout_handler(tcp_clients[i]);
		}
	}

	if (ret == 0) {
		return 0;
	}

	for (int i = 0; i < nfds; i++) {
		struct coap_client_tcp *client = get_tcp_client(fds[i].fd);

		if (!client) {
			continue;
		}

		if (fds[i].revents & ZSOCK_POLLIN) {
			struct coap_packet response;
			int pkt_len;

			pkt_len = recv_response_tcp(client, &response);
			if (pkt_len < 0) {
				if (pkt_len == -EAGAIN) {
					continue;
				}
				cancel_requests_with(client, -EIO);
				zsock_close(client->fd);
				client->fd = -1;
				continue;
			}

			k_mutex_lock(&client->lock, K_FOREVER);
			handle_response_tcp(client, &response);
			k_mutex_unlock(&client->lock);

			/* Consume processed packet and preserve any remaining bytes */
			if (client->recv_offset > pkt_len) {
				memmove(client->recv_buf, client->recv_buf + pkt_len,
					client->recv_offset - pkt_len);
				client->recv_offset -= pkt_len;
			} else {
				client->recv_offset = 0;
			}
		}

		if (fds[i].revents & ZSOCK_POLLERR) {
			cancel_requests_with(client, -EIO);
			zsock_close(client->fd);
			client->fd = -1;
		}

		if (fds[i].revents & ZSOCK_POLLHUP) {
			cancel_requests_with(client, -ECONNRESET);
			zsock_close(client->fd);
			client->fd = -1;
		}

		if (fds[i].revents & ZSOCK_POLLNVAL) {
			cancel_requests_with(client, -EIO);
			client->fd = -1;
		}
	}

	return 0;
}

static int recv_response_tcp(struct coap_client_tcp *client, struct coap_packet *response)
{
	int ret;
	int flags = ZSOCK_MSG_DONTWAIT;

	ret = receive_tcp(client, flags);
	if (ret < 0) {
		return ret;
	}

	int total_len = ret;

	LOG_DBG("Received complete packet: %d bytes", total_len);

	ret = coap_tcp_packet_parse(response, client->recv_buf, total_len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		return ret;
	}

	return total_len;
}

static struct coap_client_tcp_internal_request *get_request_with_token_tcp(
	struct coap_client_tcp *client, const struct coap_packet *resp)
{
	uint8_t response_token[COAP_TOKEN_MAX_LEN];
	uint8_t response_tkl;

	response_tkl = coap_tcp_header_get_token(resp, response_token);

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing ||
		    !exchange_lifetime_exceeded(&client->requests[i])) {
			if (client->requests[i].request_tkl == 0) {
				continue;
			}
			if (client->requests[i].request_tkl != response_tkl) {
				continue;
			}
			if (memcmp(&client->requests[i].request_token, &response_token,
				   response_tkl) == 0) {
				return &client->requests[i];
			}
		}
	}

	return NULL;
}

static bool find_echo_option(const struct coap_packet *response, struct coap_option *option)
{
	return coap_find_options(response, COAP_OPTION_ECHO, option, 1);
}

static int send_pong(struct coap_client_tcp *client)
{
	struct coap_packet pkt;
	uint8_t buf[COAP_TCP_BASIC_HEADER_SIZE];
	int ret;

	ret = coap_tcp_packet_init(&pkt, buf, sizeof(buf), 0,
				   NULL, COAP_SIGNAL_CODE_PONG);
	if (ret < 0) {
		return ret;
	}

	ret = coap_tcp_packet_update_len(&pkt);
	if (ret < 0) {
		return ret;
	}

	return send_request_tcp(client->fd, pkt.data, pkt.offset, 0);
}

static int handle_response_tcp(struct coap_client_tcp *client, const struct coap_packet *response)
{
	int ret = 0;
	int block_option;
	int block_num;
	bool blockwise_transfer = false;
	bool last_block = false;
	struct coap_client_tcp_internal_request *internal_req;
	struct coap_option csm_option;
	uint8_t cms_option_num;

	uint32_t payload_len;
	uint8_t response_code = coap_tcp_header_get_code(response);
	const uint8_t *payload = coap_tcp_packet_get_payload(response, &payload_len);
	uint8_t code_class = response_code >> 5;

	/* RFC 8323 signaling codes (class 7) - handle separately */
	if (code_class == 7) {
		switch (response_code) {
		case COAP_SIGNAL_CODE_CSM:
			/* Process Max-Message-Size option - always replace per RFC 8323 */
			cms_option_num = coap_find_options(response,
				COAP_OPTION_SIGNAL_701_MMS, &csm_option, 1);
			if (cms_option_num > 0) {
				client->max_block_size = coap_option_value_to_int(&csm_option);
				LOG_DBG("CSM: Max message size updated to %d",
					client->max_block_size);
			}

			/* Process Block-Wise-Transfer option */
			cms_option_num = coap_find_options(response,
				COAP_OPTION_SIGNAL_701_BWT, &csm_option, 1);
			if (cms_option_num > 0) {
				client->blockwise_enabled = true;
				LOG_INF("CSM: Blockwise transfer enabled");
			}

			/* Notify callback of CSM update */
			if (client->event_cb != NULL) {
				client->event_cb(client, COAP_CLIENT_TCP_EVENT_CSM_UPDATED,
						 NULL, client->event_cb_user_data);
			}
			return 0; /* CSM handled - don't try to match request */

		case COAP_SIGNAL_CODE_PING:
			LOG_DBG("Received Ping, sending Pong");
			return send_pong(client);

		case COAP_SIGNAL_CODE_PONG:
			if (client->ping_pending) {
				client->ping_pending = false;
				LOG_DBG("Pong received, RTT: %lld ms",
					k_uptime_get() - client->ping_t0);
			}
			if (client->event_cb != NULL) {
				client->event_cb(client, COAP_CLIENT_TCP_EVENT_PONG_RECEIVED,
						 NULL, client->event_cb_user_data);
			}
			return 0;

		case COAP_SIGNAL_CODE_RELEASE:
			LOG_INF("Received Release signal from server");
			if (client->event_cb != NULL) {
				union coap_client_tcp_event_data event_data = {0};
				/* TODO: Parse release options for alt_addr and hold_off */
				client->event_cb(client, COAP_CLIENT_TCP_EVENT_RELEASE,
						 &event_data, client->event_cb_user_data);
			}
			return 0;

		case COAP_SIGNAL_CODE_ABORT:
			LOG_WRN("Received Abort signal from server");
			if (client->event_cb != NULL) {
				union coap_client_tcp_event_data event_data = {0};
				/* TODO: Parse abort options for bad_csm_option */
				client->event_cb(client, COAP_CLIENT_TCP_EVENT_ABORT,
						 &event_data, client->event_cb_user_data);
			}
			return 0;

		default:
			LOG_WRN("Unknown signal code: 0x%02x", response_code);
			return 0;
		}
	}

	internal_req = get_request_with_token_tcp(client, response);
	if (!internal_req) {
		LOG_WRN("No matching request for response");
		return 0;
	}

	/* Handle echo option */
	if (find_echo_option(response, &client->echo_option)) {
		if (response_code == COAP_RESPONSE_CODE_UNAUTHORIZED) {
			ret = coap_client_tcp_init_request(client, &internal_req->coap_request,
							   internal_req, false);
			if (ret < 0) {
				LOG_ERR("Error creating CoAP request");
				goto fail;
			}

			ret = coap_packet_append_option(&internal_req->request, COAP_OPTION_ECHO,
							client->echo_option.value,
							client->echo_option.len);
			if (ret < 0) {
				LOG_ERR("Failed to append echo option");
				goto fail;
			}

			ret = coap_tcp_packet_update_len(&internal_req->request);
			if (ret < 0) {
				LOG_ERR("Failed to update packet length");
				goto fail;
			}

			ret = send_request_tcp(client->fd, internal_req->request.data,
					       internal_req->request.offset, 0);
			if (ret < 0) {
				LOG_ERR("Error sending CoAP request");
				goto fail;
			}
			return 1;
		}

		client->send_echo = true;
	}

	if (!internal_req->request_ongoing) {
		if (internal_req->is_observe) {
			return 0;
		}
		LOG_DBG("Drop request, already handled");
		return 0;
	}

	internal_req->tcp_timeout_ms = 0;

	/* Check for Block2 option */
	block_option = coap_get_option_int(response, COAP_OPTION_BLOCK2);

	if (block_option > 0) {
		blockwise_transfer = true;
		last_block = !GET_MORE(block_option);
		block_num = GET_BLOCK_NUM(block_option);

		if (GET_BLOCK_SIZE(block_option) == COAP_BLOCK_BERT) {
			if (payload_len > CONFIG_COAP_CLIENT_MESSAGE_SIZE) {
				LOG_ERR("BERT payload %u exceeds max size", payload_len);
				ret = -EMSGSIZE;
				goto fail;
			}
		}

		if (block_num == 0) {
			coap_block_transfer_init(&internal_req->recv_blk_ctx,
						 coap_bytes_to_block_size(client->max_block_size),
						 0);
			internal_req->offset = 0;
		}

		ret = coap_tcp_update_from_block(response, &internal_req->recv_blk_ctx);
		if (ret < 0) {
			LOG_ERR("Error updating block context");
		}
		coap_tcp_next_block(response, &internal_req->recv_blk_ctx);
	} else {
		internal_req->offset = 0;
		last_block = true;
	}

	/* Check if this was response to blockwise send */
	if (internal_req->send_blk_ctx.total_size > 0) {
		blockwise_transfer = true;
		internal_req->offset = internal_req->send_blk_ctx.current;
		last_block = (internal_req->send_blk_ctx.total_size ==
			      internal_req->send_blk_ctx.current);
	}

	/* Call user callback */
	if (internal_req->coap_request.cb) {
		if (!atomic_set(&internal_req->in_callback, 1)) {
			struct coap_client_tcp_response_data data = {
				.result_code = response_code,
				.packet = response,
				.offset = internal_req->offset,
				.payload = payload,
				.payload_len = payload_len,
				.last_block = last_block,
			};
			internal_req->coap_request.cb(&data, internal_req->coap_request.user_data);
			atomic_clear(&internal_req->in_callback);
		}
		if (!internal_req->request_ongoing) {
			goto fail;
		}
		if (blockwise_transfer) {
			internal_req->offset += payload_len;
		}
	}

	/* Request next block if needed */
	if (blockwise_transfer && !last_block) {
		ret = coap_client_tcp_init_request(client, &internal_req->coap_request,
						   internal_req, true);
		if (ret < 0) {
			LOG_ERR("Error creating a CoAP request");
			goto fail;
		}

		ret = coap_tcp_packet_update_len(&internal_req->request);
		if (ret < 0) {
			goto fail;
		}

		internal_req->tcp_t0 = k_uptime_get();
		internal_req->tcp_timeout_ms = CONFIG_COAP_CLIENT_TCP_REQUEST_TIMEOUT;

		ret = send_request_tcp(client->fd, internal_req->request.data,
				       internal_req->request.offset, 0);
		if (ret < 0) {
			LOG_ERR("Error sending a CoAP request");
			goto fail;
		}
		return 1;
	}

fail:
	if (ret < 0) {
		report_callback_error(internal_req, ret);
	}
	if (!internal_req->is_observe) {
		release_internal_request(internal_req);
	}
	return ret;
}

static void cancel_requests_with(struct coap_client_tcp *client, int error)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(client->requests); i++) {
		if (client->requests[i].request_ongoing) {
			LOG_DBG("Cancelling request %d", i);
			report_callback_error(&client->requests[i], error);
			release_internal_request(&client->requests[i]);
		}
		if (error == -EIO) {
			reset_internal_request(&client->requests[i]);
		}
	}

	/* Reset partial receive state on connection errors */
	if (error == -EIO || error == -ECONNRESET) {
		client->recv_offset = 0;
	}

	k_mutex_unlock(&client->lock);
}

void coap_client_tcp_cancel_requests(struct coap_client_tcp *client)
{
	cancel_requests_with(client, -ECANCELED);
	k_sleep(K_MSEC(COAP_PERIODIC_TIMEOUT));
}

void coap_client_tcp_cancel_and_reset_all(struct coap_client_tcp *client)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing) {
			report_callback_error(&client->requests[i], -ECANCELED);
		}
		reset_internal_request(&client->requests[i]);
	}

	client->fd = -1;
	client->recv_offset = 0;
	client->max_block_size = COAP_TCP_DEFAULT_MAX_MESSAGE_SIZE;
	client->blockwise_enabled = false;

	k_mutex_unlock(&client->lock);

	k_sleep(K_MSEC(COAP_PERIODIC_TIMEOUT));
}

static void coap_client_tcp_recv(void *coap_cl, void *a, void *b)
{
	int ret;

	k_sem_take(&coap_client_tcp_recv_sem, K_FOREVER);
	while (true) {
		ret = handle_poll();
		if (ret < 0) {
			LOG_ERR("Error in poll");
			goto idle;
		}

		if (has_ongoing_exchanges()) {
			continue;
		} else {
idle:
			k_sem_take(&coap_client_tcp_recv_sem, K_FOREVER);
		}
	}
}

int coap_client_tcp_init(struct coap_client_tcp *client, const char *info)
{
	if (client == NULL) {
		return -EINVAL;
	}

	client->fd = -1;
	client->max_block_size = COAP_TCP_DEFAULT_MAX_MESSAGE_SIZE;
	client->blockwise_enabled = false;
	client->recv_offset = 0;
	client->ping_pending = false;
	client->ping_t0 = 0;
	client->event_cb = NULL;
	client->event_cb_user_data = NULL;

	k_mutex_lock(&coap_client_tcp_mutex, K_FOREVER);
	if (num_tcp_clients >= CONFIG_COAP_CLIENT_MAX_INSTANCES) {
		k_mutex_unlock(&coap_client_tcp_mutex);
		return -ENOSPC;
	}

	k_mutex_init(&client->lock);

	tcp_clients[num_tcp_clients] = client;
	num_tcp_clients++;

	k_mutex_unlock(&coap_client_tcp_mutex);
	return 0;
}

int coap_client_tcp_connect(struct coap_client_tcp *client,
				const struct net_sockaddr *addr, net_socklen_t addrlen, int proto)
{
	int ret;

	if (NULL == addr || 0 == addrlen) {
		return -EINVAL;
	}

	if (client->fd > 0) {
		ret = coap_client_tcp_close(client);
		if (ret < 0) {
			return ret;
		}
	}

	ret = zsock_socket(addr->sa_family, NET_SOCK_STREAM, proto);
	if (ret < 0) {
		return -errno;
	}

	client->fd = ret;

	if (NULL != client->socket_config_cb) {
		ret = client->socket_config_cb(client->fd, client->socket_config_cb_user_data);
		if (ret < 0) {
			zsock_close(client->fd);
			client->fd = -1;
			return ret;
		}
	}

	ret = zsock_connect(client->fd, addr, addrlen);
	if (ret < 0) {
		zsock_close(client->fd);
		client->fd = -1;
		return -errno;
	}

	/* Automatically send CSM to negotiate capabilities (best-effort) */
	ret = coap_client_tcp_csm_req(client, client->max_block_size, NULL, NULL);
	if (ret < 0) {
		LOG_WRN("Failed to send automatic CSM: %d", ret);
		/* Don't fail connection - requests can still work without CSM */
	}

	return 0;
}

int coap_client_tcp_close(struct coap_client_tcp *client)
{
	int ret;

	if (NULL == client || client->fd < 0) {
		return -EINVAL;
	}

	if (has_ongoing_exchange(client)) {
		coap_client_tcp_cancel_and_reset_all(client);
	}

	ret = zsock_close(client->fd);
	client->fd = -1;
	client->max_block_size = COAP_TCP_DEFAULT_MAX_MESSAGE_SIZE;
	client->blockwise_enabled = false;

	return ret;
}

struct coap_client_tcp_option coap_client_tcp_option_initial_block2(struct coap_client_tcp *client)
{
	struct coap_client_tcp_option block2 = {
		.code = COAP_OPTION_BLOCK2,
		.len = 1,
		.value[0] = coap_bytes_to_block_size(client->max_block_size),
	};

	return block2;
}

bool coap_client_tcp_has_ongoing_exchange(struct coap_client_tcp *client)
{
	if (client == NULL) {
		LOG_ERR("Invalid (NULL) client");
		return false;
	}

	return has_ongoing_exchange(client);
}

int coap_client_tcp_ping(struct coap_client_tcp *client)
{
	struct coap_packet pkt;
	uint8_t buf[COAP_TCP_BASIC_HEADER_SIZE];
	int ret;

	if (client == NULL || client->fd < 0) {
		return -EINVAL;
	}

	ret = coap_tcp_packet_init(&pkt, buf, sizeof(buf), 0,
				   NULL, COAP_SIGNAL_CODE_PING);
	if (ret < 0) {
		return ret;
	}

	ret = coap_tcp_packet_update_len(&pkt);
	if (ret < 0) {
		return ret;
	}

	client->ping_pending = true;
	client->ping_t0 = k_uptime_get();

	ret = send_request_tcp(client->fd, pkt.data, pkt.offset, 0);
	if (ret < 0) {
		client->ping_pending = false;
		return ret;
	}

	/* Wake receive thread to handle Pong */
	k_sem_give(&coap_client_tcp_recv_sem);

	return 0;
}

int coap_client_tcp_release(struct coap_client_tcp *client,
			    const char *alt_addr, uint32_t hold_off_sec)
{
	struct coap_packet pkt;
	uint8_t buf[64]; /* Larger buffer for options */
	int ret;

	if (client == NULL || client->fd < 0) {
		return -EINVAL;
	}

	ret = coap_tcp_packet_init(&pkt, buf, sizeof(buf), 0,
				   NULL, COAP_SIGNAL_CODE_RELEASE);
	if (ret < 0) {
		return ret;
	}

	/* Add alternative address option if provided */
	if (alt_addr != NULL && alt_addr[0] != '\0') {
		ret = coap_packet_append_option(&pkt, COAP_OPTION_SIGNAL_704_ALT_ADDR,
						alt_addr, strlen(alt_addr));
		if (ret < 0) {
			return ret;
		}
	}

	/* Add hold-off option if provided */
	if (hold_off_sec > 0) {
		ret = coap_append_option_int(&pkt, COAP_OPTION_SIGNAL_704_HOLD_OFF,
					     hold_off_sec);
		if (ret < 0) {
			return ret;
		}
	}

	ret = coap_tcp_packet_update_len(&pkt);
	if (ret < 0) {
		return ret;
	}

	return send_request_tcp(client->fd, pkt.data, pkt.offset, 0);
}

void coap_client_tcp_set_event_cb(struct coap_client_tcp *client,
				  coap_client_tcp_event_cb_t cb,
				  void *user_data)
{
	if (client != NULL) {
		client->event_cb = cb;
		client->event_cb_user_data = user_data;
	}
}

#define COAP_CLIENT_TCP_THREAD_PRIORITY CLAMP(CONFIG_COAP_CLIENT_THREAD_PRIORITY, \
					      K_HIGHEST_APPLICATION_THREAD_PRIO, \
					      K_LOWEST_APPLICATION_THREAD_PRIO)

K_THREAD_DEFINE(coap_client_tcp_recv_thread, CONFIG_COAP_CLIENT_STACK_SIZE,
		coap_client_tcp_recv, NULL, NULL, NULL,
		COAP_CLIENT_TCP_THREAD_PRIORITY, 0, 0);
