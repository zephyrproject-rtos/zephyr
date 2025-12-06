/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

static K_MUTEX_DEFINE(coap_client_tcp_mutex);
static struct coap_client_tcp *tcp_clients[CONFIG_COAP_CLIENT_MAX_INSTANCES];
static int num_tcp_clients;
static K_SEM_DEFINE(coap_client_tcp_recv_sem, 0, 1);

/* Forward declarations */
static bool timeout_expired(struct coap_client_tcp_internal_request *internal_req);
static void cancel_requests_with(struct coap_client_tcp *client, int error);
static int recv_response_tcp(struct coap_client_tcp *client, struct coap_packet *response,
			     bool *truncated);
static int handle_response_tcp(struct coap_client_tcp *client, const struct coap_packet *response,
			       bool response_truncated);
static void report_callback_error(struct coap_client_tcp_internal_request *internal_req,
				  int error_code);

/**
 * @brief Send a CoAP message over TCP
 */
static int send_request_tcp(int sock, const void *buf, size_t len, int flags)
{
	int ret;

	ret = zsock_send(sock, buf, len, flags);
	return ret >= 0 ? ret : -errno;
}

/**
 * @brief Receive a complete CoAP message from TCP socket
 *
 * CoAP over TCP uses a length-prefixed framing. The first byte contains:
 * - Bits 7-4: Length field (Len)
 * - Bits 3-0: Token Length (TKL)
 *
 * If Len is 0-12: Options and payload length is Len bytes
 * If Len is 13: Extended length follows (1 byte, value = ext + 13)
 * If Len is 14: Extended length follows (2 bytes, value = ext + 269)
 * If Len is 15: Extended length follows (4 bytes, value = ext + 65805)
 */
static int receive_tcp(int sock, void *buf, size_t max_len, int flags)
{
	ssize_t err;
	uint8_t *byte_buf = (uint8_t *)buf;
	uint16_t packet_length = 0;

	/* Read first byte of header */
	err = zsock_recv(sock, &byte_buf[0], 1, flags);
	if (err < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -EAGAIN;
		}
		LOG_ERR("Failed to read header byte (%d)", -errno);
		return -errno;
	}
	if (err == 0) {
		return -ECONNRESET;
	}

	uint8_t pkt_hdr = byte_buf[0];
	uint32_t opt_and_payload_len = (pkt_hdr >> 4) & 0x0F;
	uint8_t tkl = pkt_hdr & 0x0F;

	packet_length = 1;

	if (tkl > COAP_TOKEN_MAX_LEN) {
		LOG_ERR("[RX/TCP] Invalid token length: %d", tkl);
		return -EINVAL;
	}

	/* Handle extended length encoding */
	if (opt_and_payload_len >= 13) {
		uint8_t ext_len_bytes = 1 << (opt_and_payload_len - 13);
		uint32_t extended_length = 0;

		for (uint8_t i = 0; i < ext_len_bytes; i++) {
			err = zsock_recv(sock, &byte_buf[packet_length], 1, flags);
			if (err < 0) {
				LOG_ERR("Failed to read extended length byte %d/%d (%d)",
					i + 1, ext_len_bytes, -errno);
				return -errno;
			}
			extended_length <<= 8;
			extended_length |= byte_buf[packet_length];
			++packet_length;
		}

		switch (opt_and_payload_len) {
		case 13:
			opt_and_payload_len = extended_length + 13;
			break;
		case 14:
			opt_and_payload_len = extended_length + 269;
			break;
		case 15:
			opt_and_payload_len = extended_length + 65805;
			break;
		}
	}

	/* Remaining: Code (1) + Token (tkl) + Options/Payload (opt_and_payload_len) */
	ssize_t bytes_remaining = 1 + tkl + opt_and_payload_len;

	if ((packet_length + bytes_remaining) > max_len) {
		LOG_ERR("Packet too large: %zd > %zu", packet_length + bytes_remaining, max_len);
		return -EMSGSIZE;
	}

	while (bytes_remaining > 0) {
		err = zsock_recv(sock, &byte_buf[packet_length], bytes_remaining, flags);
		if (err < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			LOG_ERR("Error receiving: %d", -errno);
			return -errno;
		}
		if (err == 0) {
			LOG_ERR("Connection closed during receive");
			return -ECONNRESET;
		}
		bytes_remaining -= err;
		packet_length += err;
	}

	return packet_length;
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

static int coap_client_tcp_schedule_poll(struct coap_client_tcp *client, int sock,
					 struct coap_client_tcp_request *req,
					 struct coap_client_tcp_internal_request *internal_req)
{
	client->fd = sock;
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

	if (!reconstruct) {
		uint8_t *token = coap_next_token();

		internal_req->request_tkl = COAP_TOKEN_MAX_LEN & 0x0F;
		memcpy(internal_req->request_token, token, internal_req->request_tkl);
	}

	/* Initialize TCP CoAP packet */
	ret = coap_packet_init_tcp(&internal_req->request, internal_req->send_buf,
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

		ret = coap_append_block2_option_tcp(&internal_req->request,
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

		/* Check if blockwise send is needed */
		if (internal_req->send_blk_ctx.total_size > 0 ||
		    total_len > CONFIG_COAP_CLIENT_MESSAGE_SIZE) {

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
		}

		ret = coap_packet_append_payload_marker(&internal_req->request);
		if (ret < 0) {
			LOG_ERR("Failed to append payload marker");
			return ret;
		}

		if (internal_req->send_blk_ctx.total_size > 0) {
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

		if (internal_req->send_blk_ctx.total_size > 0) {
			coap_next_block_tcp(&internal_req->request, &internal_req->send_blk_ctx);
		}
	}

	return ret;
}

int coap_client_tcp_req(struct coap_client_tcp *client, int sock,
			const struct net_sockaddr *addr,
			struct coap_client_tcp_request *req)
{
	int ret;
	struct coap_client_tcp_internal_request *internal_req;

	if (client == NULL || sock < 0 || req == NULL) {
		return -EINVAL;
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

	if (client->fd != sock && has_ongoing_request(client)) {
		ret = -EALREADY;
		goto release;
	}

	if (addr != NULL) {
		if (memcmp(&client->address, addr, sizeof(*addr)) != 0) {
			if (has_ongoing_request(client)) {
				LOG_WRN("Can't change address, request ongoing");
				ret = -EALREADY;
				goto release;
			}
			memcpy(&client->address, addr, sizeof(*addr));
			client->socklen = sizeof(client->address);
		}
	} else {
		if (client->socklen != 0 && has_ongoing_request(client)) {
			LOG_WRN("Can't clear address, request ongoing");
			ret = -EALREADY;
			goto release;
		}
		memset(&client->address, 0, sizeof(client->address));
		client->socklen = 0;
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

	ret = coap_packet_tcp_update_len(&internal_req->request);
	if (ret < 0) {
		LOG_ERR("Failed to update packet length");
		goto release;
	}

	ret = coap_client_tcp_schedule_poll(client, sock, req, internal_req);
	if (ret < 0) {
		LOG_ERR("Failed to schedule polling");
		goto release;
	}

	internal_req->tcp_t0 = k_uptime_get();
	internal_req->tcp_timeout_ms = CONFIG_COAP_CLIENT_TCP_REQUEST_TIMEOUT;

	internal_req->is_observe = coap_request_is_observe(&internal_req->request);
	LOG_DBG("Request is_observe %d", internal_req->is_observe);

	ret = send_request_tcp(sock, internal_req->request.data, internal_req->request.offset, 0);
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

int coap_client_tcp_csm_req(struct coap_client_tcp *client, int sock,
			    const struct net_sockaddr *addr,
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

	return coap_client_tcp_req(client, sock, addr, &req);
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
		short events = (has_ongoing_exchange(tcp_clients[i]) ? ZSOCK_POLLIN : 0) |
			       (has_timeout_expired(tcp_clients[i]) ? ZSOCK_POLLOUT : 0);

		if (events == 0) {
			continue;
		}
		fds[nfds].fd = tcp_clients[i]->fd;
		fds[nfds].events = events;
		fds[nfds].revents = 0;
		nfds++;
	}

	ret = zsock_poll(fds, nfds, COAP_PERIODIC_TIMEOUT);
	if (ret < 0) {
		return -errno;
	} else if (ret == 0) {
		return 0;
	}

	for (int i = 0; i < nfds; i++) {
		struct coap_client_tcp *client = get_tcp_client(fds[i].fd);

		if (!client) {
			continue;
		}

		if (fds[i].revents & ZSOCK_POLLOUT) {
			coap_client_tcp_timeout_handler(client);
		}

		if (fds[i].revents & ZSOCK_POLLIN) {
			struct coap_packet response;
			bool response_truncated = false;

			ret = recv_response_tcp(client, &response, &response_truncated);
			if (ret < 0) {
				if (ret == -EAGAIN) {
					continue;
				}
				cancel_requests_with(client, -EIO);
				continue;
			}

			k_mutex_lock(&client->lock, K_FOREVER);
			handle_response_tcp(client, &response, response_truncated);
			k_mutex_unlock(&client->lock);
		}

		if (fds[i].revents & ZSOCK_POLLERR) {
			cancel_requests_with(client, -EIO);
		}

		if (fds[i].revents & ZSOCK_POLLHUP) {
			cancel_requests_with(client, -ECONNRESET);
		}

		if (fds[i].revents & ZSOCK_POLLNVAL) {
			cancel_requests_with(client, -EIO);
		}
	}

	return 0;
}

static int recv_response_tcp(struct coap_client_tcp *client, struct coap_packet *response,
			     bool *truncated)
{
	int total_len;
	int available_len;
	int ret;
	int flags = ZSOCK_MSG_DONTWAIT;

	if (IS_ENABLED(CONFIG_COAP_CLIENT_TRUNCATE_MSGS)) {
		flags |= ZSOCK_MSG_TRUNC;
	}

	memset(client->recv_buf, 0, sizeof(client->recv_buf));
	total_len = receive_tcp(client->fd, client->recv_buf, sizeof(client->recv_buf), flags);

	if (total_len < 0) {
		return total_len;
	} else if (total_len == 0) {
		return 0;
	}

	available_len = MIN(total_len, sizeof(client->recv_buf));
	*truncated = available_len < total_len;

	LOG_DBG("Received %d bytes", available_len);

	ret = coap_packet_parse_tcp(response, client->recv_buf, available_len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

	return ret;
}

static struct coap_client_tcp_internal_request *get_request_with_token_tcp(
	struct coap_client_tcp *client, const struct coap_packet *resp)
{
	uint8_t response_token[COAP_TOKEN_MAX_LEN];
	uint8_t response_tkl;

	response_tkl = coap_header_get_token_tcp(resp, response_token);

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

static int handle_response_tcp(struct coap_client_tcp *client, const struct coap_packet *response,
			       bool response_truncated)
{
	int ret = 0;
	int block_option;
	int block_num;
	bool blockwise_transfer = false;
	bool last_block = false;
	struct coap_client_tcp_internal_request *internal_req;

	uint32_t payload_len;
	uint8_t response_code = coap_header_get_code_tcp(response);
	const uint8_t *payload = coap_packet_get_payload_tcp(response, &payload_len);

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

			ret = coap_packet_tcp_update_len(&internal_req->request);
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

	if (block_option > 0 || response_truncated) {
		blockwise_transfer = true;
		last_block = response_truncated ? false : !GET_MORE(block_option);
		block_num = (block_option > 0) ? GET_BLOCK_NUM(block_option) : 0;

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

		ret = coap_update_from_block_tcp(response, &internal_req->recv_blk_ctx);
		if (ret < 0) {
			LOG_ERR("Error updating block context");
		}
		coap_next_block_tcp(response, &internal_req->recv_blk_ctx);
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

		ret = coap_packet_tcp_update_len(&internal_req->request);
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

	client->max_block_size = CONFIG_COAP_CLIENT_BLOCK_SIZE;

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

#define COAP_CLIENT_TCP_THREAD_PRIORITY CLAMP(CONFIG_COAP_CLIENT_THREAD_PRIORITY, \
					      K_HIGHEST_APPLICATION_THREAD_PRIO, \
					      K_LOWEST_APPLICATION_THREAD_PRIO)

K_THREAD_DEFINE(coap_client_tcp_recv_thread, CONFIG_COAP_CLIENT_STACK_SIZE,
		coap_client_tcp_recv, NULL, NULL, NULL,
		COAP_CLIENT_TCP_THREAD_PRIORITY, 0, 0);
