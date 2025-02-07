/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/socket.h>

#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>

#define COAP_VERSION 1
#define COAP_SEPARATE_TIMEOUT 6000
#define COAP_PERIODIC_TIMEOUT 500
#define COAP_EXCHANGE_LIFETIME_FACTOR 3
#define BLOCK1_OPTION_SIZE 4
#define PAYLOAD_MARKER_SIZE 1

static K_MUTEX_DEFINE(coap_client_mutex);
static struct coap_client *clients[CONFIG_COAP_CLIENT_MAX_INSTANCES];
static int num_clients;
static K_SEM_DEFINE(coap_client_recv_sem, 0, 1);

static bool timeout_expired(struct coap_client_internal_request *internal_req);
static void cancel_requests_with(struct coap_client *client, int error);
static int recv_response(struct coap_client *client, struct coap_packet *response, bool *truncated);
static int handle_response(struct coap_client *client, const struct coap_packet *response,
			   bool response_truncated);
static struct coap_client_internal_request *get_request_with_mid(struct coap_client *client,
								 uint16_t mid);

static int send_request(int sock, const void *buf, size_t len, int flags,
			const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int ret;

	LOG_HEXDUMP_DBG(buf, len, "Send CoAP Request:");
	if (addrlen == 0) {
		ret = zsock_sendto(sock, buf, len, flags, NULL, 0);
	} else {
		ret = zsock_sendto(sock, buf, len, flags, dest_addr, addrlen);
	}

	return ret >= 0 ? ret : -errno;
}

static int receive(int sock, void *buf, size_t max_len, int flags,
		   struct sockaddr *src_addr, socklen_t *addrlen)
{
	ssize_t err;

	if (*addrlen == 0) {
		err = zsock_recvfrom(sock, buf, max_len, flags, NULL, NULL);
	} else {
		err = zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
	}
	if (err > 0) {
		LOG_HEXDUMP_DBG(buf, err, "Receive CoAP Response:");
	}
	return err >= 0 ? err : -errno;
}

/** Reset all fields to zero.
 * Use when a new request is filled in.
 */
static void reset_internal_request(struct coap_client_internal_request *request)
{
	*request = (struct coap_client_internal_request){
		.last_response_id = -1,
	};
}

/** Release a request structure.
 * Use when a request is no longer needed, but we might still receive
 * responses for it, which must be handled.
 */
static void release_internal_request(struct coap_client_internal_request *request)
{
	request->request_ongoing = false;
	request->pending.timeout = 0;
}

static int coap_client_schedule_poll(struct coap_client *client, int sock,
				     struct coap_client_request *req,
				     struct coap_client_internal_request *internal_req)
{
	client->fd = sock;
	memcpy(&internal_req->coap_request, req, sizeof(struct coap_client_request));
	internal_req->request_ongoing = true;

	k_sem_give(&coap_client_recv_sem);

	return 0;
}

static bool exchange_lifetime_exceeded(struct coap_client_internal_request *internal_req)
{
	int64_t time_since_t0, exchange_lifetime;

	if (coap_header_get_type(&internal_req->request) == COAP_TYPE_NON_CON) {
		return true;
	}

	if (internal_req->pending.t0 == 0) {
		return true;
	}

	time_since_t0 = k_uptime_get() - internal_req->pending.t0;
	exchange_lifetime =
		(internal_req->pending.params.ack_timeout * COAP_EXCHANGE_LIFETIME_FACTOR);

	return time_since_t0 > exchange_lifetime;
}

static bool has_ongoing_request(struct coap_client *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing == true) {
			return true;
		}
	}

	return false;
}

static bool has_ongoing_exchange(struct coap_client *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing == true ||
		    !exchange_lifetime_exceeded(&client->requests[i])) {
			return true;
		}
	}

	return false;
}

static bool has_timeout_expired(struct coap_client *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (timeout_expired(&client->requests[i])) {
			return true;
		}
	}
	return false;
}

static struct coap_client_internal_request *get_free_request(struct coap_client *client)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing == false &&
		    exchange_lifetime_exceeded(&client->requests[i])) {
			return &client->requests[i];
		}
	}

	return NULL;
}

static bool has_ongoing_exchanges(void)
{
	for (int i = 0; i < num_clients; i++) {
		if (has_ongoing_exchange(clients[i])) {
			return true;
		}
	}

	return false;
}

static enum coap_block_size coap_client_default_block_size(void)
{
	switch (CONFIG_COAP_CLIENT_BLOCK_SIZE) {
	case 16:
		return COAP_BLOCK_16;
	case 32:
		return COAP_BLOCK_32;
	case 64:
		return COAP_BLOCK_64;
	case 128:
		return COAP_BLOCK_128;
	case 256:
		return COAP_BLOCK_256;
	case 512:
		return COAP_BLOCK_512;
	case 1024:
		return COAP_BLOCK_1024;
	}

	return COAP_BLOCK_256;
}

static int coap_client_init_request(struct coap_client *client,
				    struct coap_client_request *req,
				    struct coap_client_internal_request *internal_req,
				    bool reconstruct)
{
	int ret = 0;
	int i;
	bool block2 = false;

	memset(client->send_buf, 0, sizeof(client->send_buf));

	if (!reconstruct) {
		uint8_t *token = coap_next_token();

		internal_req->last_id = coap_next_id();
		internal_req->request_tkl = COAP_TOKEN_MAX_LEN & 0xf;
		memcpy(internal_req->request_token, token, internal_req->request_tkl);
	}

	ret = coap_packet_init(&internal_req->request, client->send_buf, MAX_COAP_MSG_LEN,
			       1, req->confirmable ? COAP_TYPE_CON : COAP_TYPE_NON_CON,
			       COAP_TOKEN_MAX_LEN, internal_req->request_token, req->method,
			       internal_req->last_id);

	if (ret < 0) {
		LOG_ERR("Failed to init CoAP message %d", ret);
		goto out;
	}

	ret = coap_packet_set_path(&internal_req->request, req->path);

	if (ret < 0) {
		LOG_ERR("Failed to parse path to options %d", ret);
		goto out;
	}

	/* Add content format option only if there is a payload */
	if (req->payload) {
		ret = coap_append_option_int(&internal_req->request,
					     COAP_OPTION_CONTENT_FORMAT, req->fmt);

		if (ret < 0) {
			LOG_ERR("Failed to append content format option");
			goto out;
		}
	}

	/* Blockwise receive ongoing, request next block. */
	if (internal_req->recv_blk_ctx.current > 0) {
		block2 = true;
		ret = coap_append_block2_option(&internal_req->request,
						&internal_req->recv_blk_ctx);

		if (ret < 0) {
			LOG_ERR("Failed to append block 2 option");
			goto out;
		}
	}

	/* Add extra options if any */
	for (i = 0; i < req->num_options; i++) {
		if (COAP_OPTION_BLOCK2 == req->options[i].code && block2) {
			/* After the first request, ignore any block2 option added by the
			 * application, since NUM (and possibly SZX) must be updated based on the
			 * server response.
			 */
			continue;
		}

		ret = coap_packet_append_option(&internal_req->request, req->options[i].code,
						req->options[i].value, req->options[i].len);

		if (ret < 0) {
			LOG_ERR("Failed to append %d option", req->options[i].code);
			goto out;
		}
	}

	if (req->payload) {
		uint16_t payload_len;
		uint16_t offset;

		/* Blockwise send ongoing, add block1 */
		if (internal_req->send_blk_ctx.total_size > 0 ||
		   (req->len > CONFIG_COAP_CLIENT_MESSAGE_SIZE)) {

			if (internal_req->send_blk_ctx.total_size == 0) {
				coap_block_transfer_init(&internal_req->send_blk_ctx,
							 coap_client_default_block_size(),
							 req->len);
				/* Generate request tag */
				uint8_t *tag = coap_next_token();

				memcpy(internal_req->request_tag, tag, COAP_TOKEN_MAX_LEN);
			}
			ret = coap_append_block1_option(&internal_req->request,
							&internal_req->send_blk_ctx);

			if (ret < 0) {
				LOG_ERR("Failed to append block1 option");
				goto out;
			}

			ret = coap_packet_append_option(&internal_req->request,
				COAP_OPTION_REQUEST_TAG, internal_req->request_tag,
				COAP_TOKEN_MAX_LEN);

			if (ret < 0) {
				LOG_ERR("Failed to append request tag option");
				goto out;
			}
		}

		ret = coap_packet_append_payload_marker(&internal_req->request);

		if (ret < 0) {
			LOG_ERR("Failed to append payload marker to CoAP message");
			goto out;
		}

		if (internal_req->send_blk_ctx.total_size > 0) {
			uint16_t block_in_bytes =
				coap_block_size_to_bytes(internal_req->send_blk_ctx.block_size);

			payload_len = internal_req->send_blk_ctx.total_size -
				      internal_req->send_blk_ctx.current;
			if (payload_len > block_in_bytes) {
				payload_len = block_in_bytes;
			}
			offset = internal_req->send_blk_ctx.current;
		} else {
			payload_len = req->len;
			offset = 0;
		}

		ret = coap_packet_append_payload(&internal_req->request, req->payload + offset,
						 payload_len);

		if (ret < 0) {
			LOG_ERR("Failed to append payload to CoAP message");
			goto out;
		}

		if (internal_req->send_blk_ctx.total_size > 0) {
			coap_next_block(&internal_req->request, &internal_req->send_blk_ctx);
		}
	}
out:
	return ret;
}

int coap_client_req(struct coap_client *client, int sock, const struct sockaddr *addr,
		    struct coap_client_request *req, struct coap_transmission_parameters *params)
{
	int ret;
	struct coap_client_internal_request *internal_req;

	if (client == NULL || sock < 0 || req == NULL || req->path == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	internal_req = get_free_request(client);

	if (internal_req == NULL) {
		LOG_DBG("No more free requests");
		ret = -EAGAIN;
		goto out;
	}

	/* Don't allow changing to a different socket if there is already request ongoing. */
	if (client->fd != sock && has_ongoing_request(client)) {
		ret = -EALREADY;
		goto release;
	}

	/* Don't allow changing to a different address if there is already request ongoing. */
	if (addr != NULL) {
		if (memcmp(&client->address, addr, sizeof(*addr)) != 0) {
			if (has_ongoing_request(client)) {
				LOG_WRN("Can't change to a different socket, request ongoing.");
				ret = -EALREADY;
				goto release;
			}

			memcpy(&client->address, addr, sizeof(*addr));
			client->socklen = sizeof(client->address);
		}
	} else {
		if (client->socklen != 0) {
			if (has_ongoing_request(client)) {
				LOG_WRN("Can't change to a different socket, request ongoing.");
				ret = -EALREADY;
				goto release;
			}

			memset(&client->address, 0, sizeof(client->address));
			client->socklen = 0;
		}
	}

	reset_internal_request(internal_req);

	ret = coap_client_init_request(client, req, internal_req, false);
	if (ret < 0) {
		LOG_ERR("Failed to initialize coap request");
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

	ret = coap_client_schedule_poll(client, sock, req, internal_req);
	if (ret < 0) {
		LOG_ERR("Failed to schedule polling");
		goto release;
	}

	ret = coap_pending_init(&internal_req->pending, &internal_req->request,
				&client->address, params);

	if (ret < 0) {
		LOG_ERR("Failed to initialize pending struct");
		goto release;
	}

	/* Non-Confirmable messages are not retried, but we still track the lifetime as
	 * replies are acceptable.
	 */
	if (coap_header_get_type(&internal_req->request) == COAP_TYPE_NON_CON) {
		internal_req->pending.retries = 0;
	}
	coap_pending_cycle(&internal_req->pending);
	internal_req->is_observe = coap_request_is_observe(&internal_req->request);
	LOG_DBG("Request is_observe %d", internal_req->is_observe);

	ret = send_request(sock, internal_req->request.data, internal_req->request.offset, 0,
			  &client->address, client->socklen);
	if (ret < 0) {
		ret = -errno;
	}

release:
	if (ret < 0) {
		LOG_ERR("Failed to send request: %d", ret);
		reset_internal_request(internal_req);
	} else {
		/* Do not return the number of bytes sent */
		ret = 0;
	}
out:
	k_mutex_unlock(&client->lock);
	return ret;
}

static void report_callback_error(struct coap_client_internal_request *internal_req, int error_code)
{
	if (internal_req->coap_request.cb) {
		if (!atomic_set(&internal_req->in_callback, 1)) {
			internal_req->coap_request.cb(error_code, 0, NULL, 0, true,
						      internal_req->coap_request.user_data);
			atomic_clear(&internal_req->in_callback);
		} else {
			LOG_DBG("Cannot call the callback; already in it.");
		}
	}
}

static bool timeout_expired(struct coap_client_internal_request *internal_req)
{
	if (internal_req->pending.timeout == 0) {
		return false;
	}

	return (internal_req->request_ongoing &&
		internal_req->pending.timeout <= (k_uptime_get() - internal_req->pending.t0));
}

static int resend_request(struct coap_client *client,
			  struct coap_client_internal_request *internal_req)
{
	int ret = 0;

	/* Copy the pending structure if we need to restore it */
	struct coap_pending tmp = internal_req->pending;

	if (internal_req->request_ongoing &&
	    internal_req->pending.timeout != 0 &&
	    coap_pending_cycle(&internal_req->pending)) {
		LOG_ERR("Timeout, retrying send");

		/* Reset send block context as it was updated in previous init from packet */
		if (internal_req->send_blk_ctx.total_size > 0) {
			internal_req->send_blk_ctx.current = internal_req->offset;
		}
		ret = coap_client_init_request(client, &internal_req->coap_request,
					       internal_req, true);
		if (ret < 0) {
			LOG_ERR("Error re-creating CoAP request %d", ret);
			return ret;
		}

		ret = send_request(client->fd, internal_req->request.data,
					internal_req->request.offset, 0, &client->address,
					client->socklen);
		if (ret > 0) {
			ret = 0;
		} else if (ret == -EAGAIN) {
			/* Restore the pending structure, retry later */
			internal_req->pending = tmp;
			/* Not a fatal socket error, will trigger a retry */
			ret = 0;
		} else {
			LOG_ERR("Failed to resend request, %d", ret);
		}
	} else {
		LOG_ERR("Timeout, no more retries left");
		ret = -ETIMEDOUT;
	}

	return ret;
}

static void coap_client_resend_handler(struct coap_client *client)
{
	int ret = 0;

	k_mutex_lock(&client->lock, K_FOREVER);

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (timeout_expired(&client->requests[i])) {
			if (!client->requests[i].coap_request.confirmable) {
				release_internal_request(&client->requests[i]);
				continue;
			}

			ret = resend_request(client, &client->requests[i]);
			if (ret < 0) {
				report_callback_error(&client->requests[i], ret);
				release_internal_request(&client->requests[i]);
			}
		}
	}

	k_mutex_unlock(&client->lock);
}

static struct coap_client *get_client(int sock)
{
	for (int i = 0; i < num_clients; i++) {
		if (clients[i]->fd == sock) {
			return clients[i];
		}
	}

	return NULL;
}

static int handle_poll(void)
{
	int ret = 0;

	struct zsock_pollfd fds[CONFIG_COAP_CLIENT_MAX_INSTANCES] = {0};
	int nfds = 0;

	/* Use periodic timeouts */
	for (int i = 0; i < num_clients; i++) {
		short events = (has_ongoing_exchange(clients[i]) ? ZSOCK_POLLIN : 0) |
			       (has_timeout_expired(clients[i]) ? ZSOCK_POLLOUT : 0);

		if (events == 0) {
			/* Skip this socket */
			continue;
		}
		fds[nfds].fd = clients[i]->fd;
		fds[nfds].events = events;
		fds[nfds].revents = 0;
		nfds++;
	}

	ret = zsock_poll(fds, nfds, COAP_PERIODIC_TIMEOUT);

	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Error in poll:%d", ret);
		return ret;
	} else if (ret == 0) {
		return 0;
	}

	for (int i = 0; i < nfds; i++) {
		struct coap_client *client = get_client(fds[i].fd);

		if (!client) {
			LOG_ERR("No client found for socket %d", fds[i].fd);
			continue;
		}

		if (fds[i].revents & ZSOCK_POLLOUT) {
			coap_client_resend_handler(client);
		}
		if (fds[i].revents & ZSOCK_POLLIN) {
			struct coap_packet response;
			bool response_truncated = false;

			ret = recv_response(client, &response, &response_truncated);
			if (ret < 0) {
				if (ret == -EAGAIN) {
					continue;
				}
				LOG_ERR("Error receiving response");
				cancel_requests_with(client, -EIO);
				continue;
			}

			k_mutex_lock(&client->lock, K_FOREVER);
			ret = handle_response(client, &response, response_truncated);
			if (ret < 0) {
				LOG_ERR("Error handling response");
			}

			k_mutex_unlock(&client->lock);
		}
		if (fds[i].revents & ZSOCK_POLLERR) {
			LOG_ERR("Error in poll for socket %d", fds[i].fd);
			cancel_requests_with(client, -EIO);
		}
		if (fds[i].revents & ZSOCK_POLLHUP) {
			LOG_ERR("Error in poll: POLLHUP for socket %d", fds[i].fd);
			cancel_requests_with(client, -EIO);
		}
		if (fds[i].revents & ZSOCK_POLLNVAL) {
			LOG_ERR("Error in poll: POLLNVAL - fd %d not open", fds[i].fd);
			cancel_requests_with(client, -EIO);
		}
	}

	return 0;
}

static int recv_response(struct coap_client *client, struct coap_packet *response, bool *truncated)
{
	int total_len;
	int available_len;
	int ret;
	int flags = ZSOCK_MSG_DONTWAIT;

	if (IS_ENABLED(CONFIG_COAP_CLIENT_TRUNCATE_MSGS)) {
		flags |= ZSOCK_MSG_TRUNC;
	}

	memset(client->recv_buf, 0, sizeof(client->recv_buf));
	total_len = receive(client->fd, client->recv_buf, sizeof(client->recv_buf), flags,
			    &client->address, &client->socklen);

	if (total_len < 0) {
		ret = -errno;
		return ret;
	} else if (total_len == 0) {
		/* Ignore, UDP can be zero length, but it is not CoAP anymore */
		return 0;
	}

	available_len = MIN(total_len, sizeof(client->recv_buf));
	*truncated = available_len < total_len;

	LOG_DBG("Received %d bytes", available_len);

	ret = coap_packet_parse(response, client->recv_buf, available_len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

	return ret;
}

static int send_ack(struct coap_client *client, const struct coap_packet *req,
		    uint8_t response_code)
{
	int ret;
	struct coap_packet ack;

	ret = coap_ack_init(&ack, req, client->send_buf, MAX_COAP_MSG_LEN, response_code);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CoAP ACK-message");
		return ret;
	}

	ret = send_request(client->fd, ack.data, ack.offset, 0, &client->address, client->socklen);
	if (ret < 0) {
		LOG_ERR("Error sending a CoAP ACK-message");
		return ret;
	}

	return 0;
}

static int send_rst(struct coap_client *client, const struct coap_packet *req)
{
	int ret;
	struct coap_packet rst;

	ret = coap_rst_init(&rst, req, client->send_buf, MAX_COAP_MSG_LEN);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CoAP RST-message");
		return ret;
	}

	ret = send_request(client->fd, rst.data, rst.offset, 0, &client->address, client->socklen);
	if (ret < 0) {
		LOG_ERR("Error sending a CoAP RST-message");
		return ret;
	}

	return 0;
}

static struct coap_client_internal_request *get_request_with_token(
	struct coap_client *client, const struct coap_packet *resp)
{

	uint8_t response_token[COAP_TOKEN_MAX_LEN];
	uint8_t response_tkl;

	response_tkl = coap_header_get_token(resp, response_token);

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

static struct coap_client_internal_request *get_request_with_mid(struct coap_client *client,
								 uint16_t mid)
{
	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing) {
			if (client->requests[i].last_id == (int)mid) {
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

static int handle_response(struct coap_client *client, const struct coap_packet *response,
			   bool response_truncated)
{
	int ret = 0;
	int block_option;
	int block_num;
	bool blockwise_transfer = false;
	bool last_block = false;
	struct coap_client_internal_request *internal_req;

	/* Handle different types, ACK might be separate or piggybacked
	 * CON and NCON contains a separate response, CON needs an empty response
	 * CON request results as ACK and possibly separate CON or NCON response
	 * NCON request results only as a separate CON or NCON message as there is no ACK
	 * With RESET, just drop gloves and call the callback.
	 */

	/* CON, NON_CON and piggybacked ACK need to match the token with original request */
	uint16_t payload_len;
	uint8_t response_type = coap_header_get_type(response);
	uint8_t response_code = coap_header_get_code(response);
	uint16_t response_id = coap_header_get_id(response);
	const uint8_t *payload = coap_packet_get_payload(response, &payload_len);

	if (response_type == COAP_TYPE_RESET) {
		internal_req = get_request_with_mid(client, response_id);
		if (!internal_req) {
			LOG_WRN("No matching request for RESET");
			return 0;
		}
		report_callback_error(internal_req, -ECONNRESET);
		release_internal_request(internal_req);
		return 0;
	}

	/* Separate response coming */
	if (payload_len == 0 && response_type == COAP_TYPE_ACK &&
	    response_code == COAP_CODE_EMPTY) {
		internal_req = get_request_with_mid(client, response_id);
		if (!internal_req) {
			LOG_WRN("No matching request for ACK");
			return 0;
		}
		internal_req->pending.t0 = k_uptime_get();
		internal_req->pending.timeout = COAP_SEPARATE_TIMEOUT;
		internal_req->pending.retries = 0;
		return 1;
	}

	internal_req = get_request_with_token(client, response);
	if (!internal_req) {
		LOG_WRN("No matching request for response");
		(void) send_rst(client, response); /* Ignore errors, unrelated to our queries */
		return 0;
	}

	/* Received echo option */
	if (find_echo_option(response, &client->echo_option)) {
		 /* Resend request with echo option */
		if (response_code == COAP_RESPONSE_CODE_UNAUTHORIZED) {
			ret = coap_client_init_request(client, &internal_req->coap_request,
						       internal_req, false);

			if (ret < 0) {
				LOG_ERR("Error creating a CoAP request");
				goto fail;
			}

			ret = coap_packet_append_option(&internal_req->request, COAP_OPTION_ECHO,
							client->echo_option.value,
							client->echo_option.len);
			if (ret < 0) {
				LOG_ERR("Failed to append echo option");
				goto fail;
			}

			if (coap_header_get_type(&internal_req->request) == COAP_TYPE_CON) {
				struct coap_transmission_parameters params =
					internal_req->pending.params;
				ret = coap_pending_init(&internal_req->pending,
							&internal_req->request, &client->address,
							&params);
				if (ret < 0) {
					LOG_ERR("Error creating pending");
					goto fail;
				}

				coap_pending_cycle(&internal_req->pending);
			}

			ret = send_request(client->fd, internal_req->request.data,
					   internal_req->request.offset, 0, &client->address,
					   client->socklen);
			if (ret < 0) {
				LOG_ERR("Error sending a CoAP request");
				goto fail;
			} else {
				return 1;
			}
		} else {
			/* Send echo in next request */
			client->send_echo = true;
		}
	}

	/* Send ack for CON */
	if (response_type == COAP_TYPE_CON) {
		/* CON response is always a separate response, respond with empty ACK. */
		ret = send_ack(client, response, COAP_CODE_EMPTY);
		if (ret < 0) {
			goto fail;
		}
	}

	/* MID-based deduplication */
	if (response_id == internal_req->last_response_id) {
		LOG_WRN("Duplicate MID, dropping");
		return 0;
	}

	internal_req->last_response_id = response_id;

	if (!internal_req->request_ongoing) {
		if (internal_req->is_observe) {
			(void) send_rst(client, response);
			return 0;
		}
		LOG_DBG("Drop request, already handled");
		return 0;
	}

	if (internal_req->pending.timeout != 0) {
		coap_pending_clear(&internal_req->pending);
	}

	/* Check if block2 exists */
	block_option = coap_get_option_int(response, COAP_OPTION_BLOCK2);
	if (block_option > 0 || response_truncated) {
		blockwise_transfer = true;
		last_block = response_truncated ? false : !GET_MORE(block_option);
		block_num = (block_option > 0) ? GET_BLOCK_NUM(block_option) : 0;

		if (block_num == 0) {
			coap_block_transfer_init(&internal_req->recv_blk_ctx,
						 coap_client_default_block_size(),
						 0);
			internal_req->offset = 0;
		}

		ret = coap_update_from_block(response, &internal_req->recv_blk_ctx);
		if (ret < 0) {
			LOG_ERR("Error updating block context");
		}
		coap_next_block(response, &internal_req->recv_blk_ctx);
	} else {
		internal_req->offset = 0;
		last_block = true;
	}

	/* Check if this was a response to last blockwise send */
	if (internal_req->send_blk_ctx.total_size > 0) {
		blockwise_transfer = true;
		internal_req->offset = internal_req->send_blk_ctx.current;
		if (internal_req->send_blk_ctx.total_size == internal_req->send_blk_ctx.current) {
			last_block = true;
		} else {
			last_block = false;
		}
	}

	/* Until the last block of a transfer, limit data size sent to the application to the block
	 * size, to avoid data above block size being repeated when the next block is received.
	 */
	if (blockwise_transfer && !last_block) {
		payload_len = MIN(payload_len, CONFIG_COAP_CLIENT_BLOCK_SIZE);
	}

	/* Call user callback */
	if (internal_req->coap_request.cb) {
		if (!atomic_set(&internal_req->in_callback, 1)) {
			internal_req->coap_request.cb(response_code, internal_req->offset, payload,
						      payload_len, last_block,
						      internal_req->coap_request.user_data);
			atomic_clear(&internal_req->in_callback);
		}
		if (!internal_req->request_ongoing) {
			/* User callback must have called coap_client_cancel_requests(). */
			goto fail;
		}
		/* Update the offset for next callback in a blockwise transfer */
		if (blockwise_transfer) {
			internal_req->offset += payload_len;
		}
	}

	/* If this wasn't last block, send the next request */
	if (blockwise_transfer && !last_block) {
		ret = coap_client_init_request(client, &internal_req->coap_request, internal_req,
					       false);

		if (ret < 0) {
			LOG_ERR("Error creating a CoAP request");
			goto fail;
		}

		struct coap_transmission_parameters params = internal_req->pending.params;
		ret = coap_pending_init(&internal_req->pending, &internal_req->request,
					&client->address, &params);
		if (ret < 0) {
			LOG_ERR("Error creating pending");
			goto fail;
		}
		coap_pending_cycle(&internal_req->pending);

		ret = send_request(client->fd, internal_req->request.data,
				   internal_req->request.offset, 0, &client->address,
				   client->socklen);
		if (ret < 0) {
			LOG_ERR("Error sending a CoAP request");
			goto fail;
		} else {
			return 1;
		}
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

static void cancel_requests_with(struct coap_client *client, int error)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(client->requests); i++) {
		if (client->requests[i].request_ongoing == true) {
			LOG_DBG("Cancelling request %d", i);
			/* Report the request was cancelled. This will be skipped if
			 * this function was called from the user's callback so we
			 * do not reenter it. In that case, the user knows their
			 * request was cancelled anyway.
			 */
			report_callback_error(&client->requests[i], error);
			release_internal_request(&client->requests[i]);
		}
		/* If our socket has failed, clear all requests, even completed ones,
		 * so that our handle_poll() does not poll() anymore for this socket.
		 */
		if (error == -EIO) {
			reset_internal_request(&client->requests[i]);
		}
	}
	k_mutex_unlock(&client->lock);

}

void coap_client_cancel_requests(struct coap_client *client)
{
	cancel_requests_with(client, -ECANCELED);
	/* Wait until after zsock_poll() can time out and return. */
	k_sleep(K_MSEC(COAP_PERIODIC_TIMEOUT));
}

static bool requests_match(struct coap_client_request *a, struct coap_client_request *b)
{
	/* enum coap_method does not have value for zero, so differentiate valid values */
	if (a->method && b->method && a->method != b->method) {
		return false;
	}
	if (a->path && b->path && strcmp(a->path, b->path) != 0) {
		return false;
	}
	if (a->cb && b->cb && a->cb != b->cb) {
		return false;
	}
	if (a->user_data && b->user_data && a->user_data != b->user_data) {
		return false;
	}
	/* It is intentional that (struct coap_client_request){0} matches all */
	return true;
}

void coap_client_cancel_request(struct coap_client *client, struct coap_client_request *req)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		if (client->requests[i].request_ongoing &&
		    requests_match(&client->requests[i].coap_request, req)) {
			LOG_DBG("Cancelling request %d", i);
			report_callback_error(&client->requests[i], -ECANCELED);
			release_internal_request(&client->requests[i]);
		}
	}

	k_mutex_unlock(&client->lock);
}

void coap_client_recv(void *coap_cl, void *a, void *b)
{
	int ret;

	k_sem_take(&coap_client_recv_sem, K_FOREVER);
	while (true) {
		ret = handle_poll();
		if (ret < 0) {
			/* Error in polling */
			LOG_ERR("Error in poll");
			goto idle;
		}

		/* There are more messages coming */
		if (has_ongoing_exchanges()) {
			continue;
		} else {
idle:
			k_sem_take(&coap_client_recv_sem, K_FOREVER);
		}
	}
}

int coap_client_init(struct coap_client *client, const char *info)
{
	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&coap_client_mutex, K_FOREVER);
	if (num_clients >= CONFIG_COAP_CLIENT_MAX_INSTANCES) {
		k_mutex_unlock(&coap_client_mutex);
		return -ENOSPC;
	}

	k_mutex_init(&client->lock);

	clients[num_clients] = client;
	num_clients++;

	k_mutex_unlock(&coap_client_mutex);
	return 0;
}

struct coap_client_option coap_client_option_initial_block2(void)
{
	struct coap_client_option block2 = {
		.code = COAP_OPTION_BLOCK2,
		.len = 1,
		.value[0] = coap_bytes_to_block_size(CONFIG_COAP_CLIENT_BLOCK_SIZE),
	};

	return block2;
}

K_THREAD_DEFINE(coap_client_recv_thread, CONFIG_COAP_CLIENT_STACK_SIZE,
		coap_client_recv, NULL, NULL, NULL,
		CONFIG_COAP_CLIENT_THREAD_PRIORITY, 0, 0);
