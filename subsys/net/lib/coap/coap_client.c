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
#define COAP_PATH_ELEM_DELIM '/'
#define COAP_PATH_ELEM_QUERY '?'
#define COAP_PATH_ELEM_AMP '&'
#define COAP_SEPARATE_TIMEOUT 6000
#define DEFAULT_RETRY_AMOUNT 5
#define BLOCK1_OPTION_SIZE 4
#define PAYLOAD_MARKER_SIZE 1

static int coap_client_schedule_poll(struct coap_client *client, int sock,
				     struct coap_client_request *req)
{
	client->fd = sock;
	client->coap_request = req;

	k_sem_give(&client->coap_client_recv_sem);
	atomic_set(&client->coap_client_recv_active, 1);

	return 0;
}

static int send_request(int sock, const void *buf, size_t len, int flags,
			const struct sockaddr *dest_addr, socklen_t addrlen)
{
	if (addrlen == 0) {
		return sendto(sock, buf, len, flags, NULL, 0);
	} else {
		return sendto(sock, buf, len, flags, dest_addr, addrlen);
	}
}

static int receive(int sock, void *buf, size_t max_len, int flags,
		   struct sockaddr *src_addr, socklen_t *addrlen)
{
	if (*addrlen == 0) {
		return recvfrom(sock, buf, max_len, flags, NULL, NULL);
	} else {
		return recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
	}
}

static void reset_block_contexts(struct coap_client *client)
{
	client->recv_blk_ctx.block_size = 0;
	client->recv_blk_ctx.total_size = 0;
	client->recv_blk_ctx.current = 0;

	client->send_blk_ctx.block_size = 0;
	client->send_blk_ctx.total_size = 0;
	client->send_blk_ctx.current = 0;
}

static int coap_client_init_path_options(struct coap_packet *pckt, const char *path)
{
	int ret = 0;
	int path_start, path_end;
	int path_length;
	bool contains_query = false;
	int i;

	path_start = 0;
	path_end = 0;
	path_length = strlen(path);
	for (i = 0; i < path_length; i++) {
		path_end = i;
		if (path[i] == COAP_PATH_ELEM_DELIM) {
			/* Guard for preceding delimiters */
			if (path_start < path_end) {
				ret = coap_packet_append_option(pckt, COAP_OPTION_URI_PATH,
								path + path_start,
								path_end - path_start);
				if (ret < 0) {
					LOG_ERR("Failed to append path to CoAP message");
					goto out;
				}
			}
			/* Check if there is a new path after delimiter,
			 * if not, point to the end of string to not add
			 * new option after this
			 */
			if (path_length > i + 1) {
				path_start = i + 1;
			} else {
				path_start = path_length;
			}
		} else if (path[i] == COAP_PATH_ELEM_QUERY) {
			/* Guard for preceding delimiters */
			if (path_start < path_end) {
				ret = coap_packet_append_option(pckt, COAP_OPTION_URI_PATH,
								path + path_start,
								path_end - path_start);
				if (ret < 0) {
					LOG_ERR("Failed to append path to CoAP message");
					goto out;
				}
			}
			/* Rest of the path is query */
			contains_query = true;
			if (path_length > i + 1) {
				path_start = i + 1;
			} else {
				path_start = path_length;
			}
			break;
		}
	}

	if (contains_query) {
		for (i = path_start; i < path_length; i++) {
			path_end = i;
			if (path[i] == COAP_PATH_ELEM_AMP || path[i] == COAP_PATH_ELEM_QUERY) {
				/* Guard for preceding delimiters */
				if (path_start < path_end) {
					ret = coap_packet_append_option(pckt, COAP_OPTION_URI_QUERY,
									path + path_start,
									path_end - path_start);
					if (ret < 0) {
						LOG_ERR("Failed to append path to CoAP message");
						goto out;
					}
				}
				/* Check if there is a new query option after delimiter,
				 * if not, point to the end of string to not add
				 * new option after this
				 */
				if (path_length > i + 1) {
					path_start = i + 1;
				} else {
					path_start = path_length;
				}
			}
		}
	}

	if (path_start < path_end) {
		if (contains_query) {
			ret = coap_packet_append_option(pckt, COAP_OPTION_URI_QUERY,
							path + path_start,
							path_end - path_start + 1);
		} else {
			ret = coap_packet_append_option(pckt, COAP_OPTION_URI_PATH,
							path + path_start,
							path_end - path_start + 1);
		}
		if (ret < 0) {
			LOG_ERR("Failed to append path to CoAP message");
			goto out;
		}
	}

out:
	return ret;
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
				    struct coap_client_request *req)
{
	int ret = 0;
	int i;

	memset(client->send_buf, 0, sizeof(client->send_buf));
	ret = coap_packet_init(&client->request, client->send_buf, MAX_COAP_MSG_LEN, 1,
			       req->confirmable ? COAP_TYPE_CON : COAP_TYPE_NON_CON,
			       COAP_TOKEN_MAX_LEN, coap_next_token(), req->method,
			       coap_next_id());

	if (ret < 0) {
		LOG_ERR("Failed to init CoAP message %d", ret);
		goto out;
	}

	ret = coap_client_init_path_options(&client->request, req->path);

	if (ret < 0) {
		LOG_ERR("Failed to parse path to options %d", ret);
		goto out;
	}

	ret = coap_append_option_int(&client->request, COAP_OPTION_CONTENT_FORMAT, req->fmt);

	if (ret < 0) {
		LOG_ERR("Failed to append content format option");
		goto out;
	}

	/* Blockwise receive ongoing, request next block. */
	if (client->recv_blk_ctx.current > 0) {
		ret = coap_append_block2_option(&client->request, &client->recv_blk_ctx);

		if (ret < 0) {
			LOG_ERR("Failed to append block 2 option");
			goto out;
		}
	}

	/* Add extra options if any */
	for (i = 0; i < req->num_options; i++) {
		ret = coap_packet_append_option(&client->request, req->options[i].code,
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
		if (client->send_blk_ctx.total_size > 0 ||
		   (req->len > CONFIG_COAP_CLIENT_MESSAGE_SIZE)) {

			if (client->send_blk_ctx.total_size == 0) {
				coap_block_transfer_init(&client->send_blk_ctx,
							 coap_client_default_block_size(),
							 req->len);
			}
			ret = coap_append_block1_option(&client->request, &client->send_blk_ctx);

			if (ret < 0) {
				LOG_ERR("Failed to append block1 option");
				goto out;
			}
		}

		ret = coap_packet_append_payload_marker(&client->request);

		if (ret < 0) {
			LOG_ERR("Failed to append payload marker to CoAP message");
			goto out;
		}

		if (client->send_blk_ctx.total_size > 0) {
			uint16_t block_in_bytes =
				coap_block_size_to_bytes(client->send_blk_ctx.block_size);

			payload_len = client->send_blk_ctx.total_size -
				      client->send_blk_ctx.current;
			if (payload_len > block_in_bytes) {
				payload_len = block_in_bytes;
			}
			offset = client->send_blk_ctx.current;
		} else {
			payload_len = req->len;
			offset = 0;
		}

		ret = coap_packet_append_payload(&client->request, req->payload + offset,
						 payload_len);

		if (ret < 0) {
			LOG_ERR("Failed to append payload to CoAP message");
			goto out;
		}

		if (client->send_blk_ctx.total_size > 0) {
			coap_next_block(&client->request, &client->send_blk_ctx);
		}
	}
	client->request_tkl = coap_header_get_token(&client->request, client->request_token);
out:
	return ret;
}


int coap_client_req(struct coap_client *client, int sock, const struct sockaddr *addr,
		    struct coap_client_request *req, int retries)
{
	int ret;

	if (client->coap_client_recv_active) {
		return -EAGAIN;
	}

	if (sock < 0 || req == NULL || req->path == NULL) {
		return -EINVAL;
	}

	if (addr != NULL) {
		memcpy(&client->address, addr, sizeof(*addr));
		client->socklen = sizeof(client->address);
	} else {
		memset(&client->address, 0, sizeof(client->address));
		client->socklen = 0;
	}

	if (retries == -1) {
		client->retry_count = DEFAULT_RETRY_AMOUNT;
	} else {
		client->retry_count = retries;
	}

	ret = coap_client_init_request(client, req);
	if (ret < 0) {
		LOG_ERR("Failed to initialize coap request");
		return ret;
	}

	ret = coap_client_schedule_poll(client, sock, req);
	if (ret < 0) {
		LOG_ERR("Failed to schedule polling");
		goto out;
	}

	ret = coap_pending_init(&client->pending, &client->request, &client->address,
				client->retry_count);

	if (ret < 0) {
		LOG_ERR("Failed to initialize pending struct");
		goto out;
	}

	coap_pending_cycle(&client->pending);

	ret = send_request(sock, client->request.data, client->request.offset, 0, &client->address,
			   client->socklen);

	if (ret < 0) {
		LOG_ERR("Transmission failed: %d", errno);
	} else {
		/* Do not return the number of bytes sent */
		ret = 0;
	}
out:
	return ret;
}

static int handle_poll(struct coap_client *client)
{
	int ret = 0;

	while (1) {
		struct pollfd fds;

		fds.fd = client->fd;
		fds.events = POLLIN;
		fds.revents = 0;
		/* rfc7252#section-5.2.2, use separate timeout value for a separate response */
		if (client->pending.timeout != 0) {
			ret = poll(&fds, 1, client->pending.timeout);
		} else {
			ret = poll(&fds, 1, COAP_SEPARATE_TIMEOUT);
		}

		if (ret < 0) {
			LOG_ERR("Error in poll:%d", errno);
			errno = 0;
			return ret;
		} else if (ret == 0) {
			if (client->pending.timeout != 0 && coap_pending_cycle(&client->pending)) {
				LOG_ERR("Timeout in poll, retrying send");
				ret = send_request(client->fd, client->request.data,
						   client->request.offset, 0, &client->address,
						   client->socklen);
				if (ret < 0) {
					LOG_ERR("Transmission failed: %d", errno);
					ret = -errno;
					break;
				}
			} else {
				/* No more retries left, don't retry */
				LOG_ERR("Timeout in poll, no more retries");
				ret = -EFAULT;
				break;
			}
		} else {
			if (fds.revents & POLLERR) {
				LOG_ERR("Error in poll");
				ret = -EIO;
				break;
			}

			if (fds.revents & POLLHUP) {
				LOG_ERR("Error in poll: POLLHUP");
				ret = -ECONNRESET;
				break;
			}

			if (fds.revents & POLLNVAL) {
				LOG_ERR("Error in poll: POLLNVAL - fd not open");
				ret = -EINVAL;
				break;
			}

			if (!(fds.revents & POLLIN)) {
				LOG_ERR("Unknown poll error");
				ret = -EINVAL;
				break;
			}

			ret = 0;
			break;
		}
	}

	return ret;
}

static bool token_compare(struct coap_client *client, const struct coap_packet *resp)
{
	uint8_t response_token[COAP_TOKEN_MAX_LEN];
	uint8_t response_tkl;

	response_tkl = coap_header_get_token(resp, response_token);

	if (client->request_tkl != response_tkl) {
		return false;
	}

	return memcmp(&client->request_token, &response_token, response_tkl) == 0;
}

static int recv_response(struct coap_client *client, struct coap_packet *response)
{
	int len;
	int ret;

	memset(client->recv_buf, 0, sizeof(client->recv_buf));
	len = receive(client->fd, client->recv_buf, sizeof(client->recv_buf), MSG_DONTWAIT,
		      &client->address, &client->socklen);

	if (len < 0) {
		LOG_ERR("Error reading response: %d", errno);
		return -EINVAL;
	} else if (len == 0) {
		LOG_ERR("Zero length recv");
		return -EINVAL;
	}

	LOG_DBG("Received %d bytes", len);

	ret = coap_packet_parse(response, client->recv_buf, len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		return ret;
	}

	return ret;
}

static void report_callback_error(struct coap_client *client, int error_code)
{
	if (client->coap_request->cb) {
		client->coap_request->cb(error_code, 0, NULL, 0, true,
					 client->coap_request->user_data);
	}
}

static int send_ack(struct coap_client *client, const struct coap_packet *req,
		    uint8_t response_code)
{
	int ret;

	ret = coap_ack_init(&client->request, req, client->send_buf, MAX_COAP_MSG_LEN,
			    response_code);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CoAP ACK-message");
		return ret;
	}

	ret = send_request(client->fd, client->request.data, client->request.offset, 0,
			   &client->address, client->socklen);
	if (ret < 0) {
		LOG_ERR("Error sending a CoAP ACK-message");
		return ret;
	}

	return 0;
}

static int send_reset(struct coap_client *client, const struct coap_packet *req,
		      uint8_t response_code)
{
	int ret;
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;

	id = coap_header_get_id(req);
	tkl = response_code ? coap_header_get_token(req, token) : 0;
	ret = coap_packet_init(&client->request, client->send_buf, MAX_COAP_MSG_LEN, COAP_VERSION,
			       COAP_TYPE_RESET, tkl, token, response_code, id);

	if (ret < 0) {
		LOG_ERR("Error creating CoAP reset message");
		return ret;
	}

	ret = send_request(client->fd, client->request.data, client->request.offset, 0,
			   &client->address, client->socklen);
	if (ret < 0) {
		LOG_ERR("Error sending CoAP reset message");
		return ret;
	}

	return 0;
}

static int handle_response(struct coap_client *client, const struct coap_packet *response)
{
	int ret = 0;
	int response_type;
	int block_option;
	int block_num;
	bool blockwise_transfer = false;
	bool last_block = false;

	/* Handle different types, ACK might be separate or piggybacked
	 * CON and NCON contains a separate response, CON needs an empty response
	 * CON request results as ACK and possibly separate CON or NCON response
	 * NCON request results only as a separate CON or NCON message as there is no ACK
	 * With RESET, just drop gloves and call the callback.
	 */
	response_type = coap_header_get_type(response);

	/* Reset and Ack need to match the message ID with request */
	if ((response_type == COAP_TYPE_ACK || response_type == COAP_TYPE_RESET) &&
	     coap_header_get_id(response) != client->pending.id)  {
		LOG_ERR("Unexpected ACK or Reset");
		return -EFAULT;
	} else if (response_type == COAP_TYPE_RESET) {
		coap_pending_clear(&client->pending);
	}

	/* CON, NON_CON and piggybacked ACK need to match the token with original request */
	uint16_t payload_len;
	uint8_t response_code = coap_header_get_code(response);
	const uint8_t *payload = coap_packet_get_payload(response, &payload_len);

	/* Separate response */
	if (payload_len == 0 && response_type == COAP_TYPE_ACK &&
	    response_code == COAP_CODE_EMPTY) {
		/* Clear the pending, poll uses now the separate timeout for the response. */
		coap_pending_clear(&client->pending);
		return 1;
	}

	/* Check for tokens */
	if (!token_compare(client, response)) {
		LOG_ERR("Not matching tokens, respond with reset");
		ret = send_reset(client, response, COAP_RESPONSE_CODE_NOT_FOUND);
		return 1;
	}

	/* Send ack for CON */
	if (response_type == COAP_TYPE_CON) {
		/* CON response is always a separate response, respond with empty ACK. */
		ret = send_ack(client, response, COAP_CODE_EMPTY);
		if (ret < 0) {
			goto fail;
		}
	}

	if (client->pending.timeout != 0) {
		coap_pending_clear(&client->pending);
	}

	/* Check if block2 exists */
	block_option = coap_get_option_int(response, COAP_OPTION_BLOCK2);
	if (block_option > 0) {
		blockwise_transfer = true;
		last_block = !GET_MORE(block_option);
		block_num = GET_BLOCK_NUM(block_option);

		if (block_num == 0) {
			coap_block_transfer_init(&client->recv_blk_ctx,
						 coap_client_default_block_size(),
						 0);
			client->offset = 0;
		}

		ret = coap_update_from_block(response, &client->recv_blk_ctx);
		if (ret < 0) {
			LOG_ERR("Error updating block context");
		}
		coap_next_block(response, &client->recv_blk_ctx);
	} else {
		client->offset = 0;
		last_block = true;
	}

	/* Check if this was a response to last blockwise send */
	if (client->send_blk_ctx.total_size > 0) {
		blockwise_transfer = true;
		if (client->send_blk_ctx.total_size == client->send_blk_ctx.current) {
			last_block = true;
		} else {
			last_block = false;
		}
	}

	/* Call user callback */
	if (client->coap_request->cb) {
		client->coap_request->cb(response_code, client->offset, payload, payload_len,
					 last_block, client->coap_request->user_data);

		/* Update the offset for next callback in a blockwise transfer */
		if (blockwise_transfer) {
			client->offset += payload_len;
		}
	}

	/* If this wasn't last block, send the next request */
	if (blockwise_transfer && !last_block) {
		ret = coap_client_init_request(client, client->coap_request);

		if (ret < 0) {
			LOG_ERR("Error creating a CoAP request");
			goto fail;
		}

		if (client->pending.timeout != 0) {
			LOG_ERR("Previous pending hasn't arrived");
			goto fail;
		}

		ret = coap_pending_init(&client->pending, &client->request, &client->address,
					client->retry_count);
		if (ret < 0) {
			LOG_ERR("Error creating pending");
			goto fail;
		}
		coap_pending_cycle(&client->pending);

		ret = send_request(client->fd, client->request.data, client->request.offset, 0,
				   &client->address, client->socklen);
		if (ret < 0) {
			LOG_ERR("Error sending a CoAP request");
			goto fail;
		} else {
			return 1;
		}
	}
fail:
	return ret;
}

void coap_client_recv(void *coap_cl, void *a, void *b)
{
	int ret;
	struct coap_client *const client = coap_cl;

	reset_block_contexts(client);
	k_sem_take(&client->coap_client_recv_sem, K_FOREVER);
	while (true) {
		struct coap_packet response;

		atomic_set(&client->coap_client_recv_active, 1);
		ret = handle_poll(client);
		if (ret < 0) {
			/* Error in polling, clear pending. */
			LOG_ERR("Error in poll");
			coap_pending_clear(&client->pending);
			report_callback_error(client, ret);
			goto idle;
		}

		ret = recv_response(client, &response);
		if (ret < 0) {
			LOG_ERR("Error receiving response");
			report_callback_error(client, ret);
			goto idle;
		}

		ret = handle_response(client, &response);
		if (ret < 0) {
			LOG_ERR("Error handling respnse");
			report_callback_error(client, ret);
			goto idle;
		}

		/* There are more messages coming for the original request */
		if (ret > 0) {
			continue;
		} else {
idle:
			reset_block_contexts(client);
			atomic_set(&client->coap_client_recv_active, 0);
			k_sem_take(&client->coap_client_recv_sem, K_FOREVER);
		}
	}
}

int coap_client_init(struct coap_client *client, const char *info)
{
	if (client == NULL) {
		return -EINVAL;
	}

	client->fd = -1;
	k_sem_init(&client->coap_client_recv_sem, 0, 1);

	client->tid =
		k_thread_create(&client->thread, client->coap_thread_stack,
				K_THREAD_STACK_SIZEOF(client->coap_thread_stack),
				coap_client_recv, client, NULL, NULL,
				CONFIG_COAP_CLIENT_THREAD_PRIORITY, 0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		if (info != NULL) {
			k_thread_name_set(client->tid, info);
		} else {
			k_thread_name_set(client->tid, "coap_client");
		}
	}

	return 0;
}
