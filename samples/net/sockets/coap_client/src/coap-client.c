/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_coap_client_sample, LOG_LEVEL_DBG);

#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/coap.h>

#include "net_private.h"

#define PEER_PORT 5683
#define MAX_COAP_MSG_LEN 256

/* CoAP socket fd */
static int sock;

struct pollfd fds[1];
static int nfds;

/* CoAP Options */
static const char * const test_path[] = { "test", NULL };

static const char * const large_path[] = { "large", NULL };

static const char * const obs_path[] = { "obs", NULL };

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048

static struct coap_block_context blk_ctx;

static void wait(void)
{
	if (poll(fds, nfds, -1) < 0) {
		LOG_ERR("Error in poll:%d", errno);
	}
}

static void prepare_fds(void)
{
	fds[nfds].fd = sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

static int start_coap_client(void)
{
	int ret = 0;
	struct sockaddr_in6 addr6;

	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(PEER_PORT);
	addr6.sin6_scope_id = 0U;

	inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
		  &addr6.sin6_addr);

	sock = socket(addr6.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = connect(sock, (struct sockaddr *)&addr6, sizeof(addr6));
	if (ret < 0) {
		LOG_ERR("Cannot connect to UDP remote : %d", errno);
		return -errno;
	}

	prepare_fds();

	return 0;
}

static int process_simple_coap_reply(void)
{
	struct coap_packet reply;
	uint8_t *data;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

end:
	k_free(data);

	return ret;
}

static int send_simple_coap_request(uint8_t method)
{
	uint8_t payload[] = "payload";
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     method, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = test_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	switch (method) {
	case COAP_METHOD_GET:
	case COAP_METHOD_DELETE:
		break;

	case COAP_METHOD_PUT:
	case COAP_METHOD_POST:
		r = coap_packet_append_payload_marker(&request);
		if (r < 0) {
			LOG_ERR("Unable to append payload marker");
			goto end;
		}

		r = coap_packet_append_payload(&request, (uint8_t *)payload,
					       sizeof(payload) - 1);
		if (r < 0) {
			LOG_ERR("Not able to append payload");
			goto end;
		}

		break;
	default:
		r = -EINVAL;
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int send_simple_coap_msgs_and_wait_for_reply(void)
{
	uint8_t test_type = 0U;
	int r;

	while (1) {
		switch (test_type) {
		case 0:
			/* Test CoAP GET method */
			printk("\nCoAP client GET\n");
			r = send_simple_coap_request(COAP_METHOD_GET);
			if (r < 0) {
				return r;
			}

			break;
		case 1:
			/* Test CoAP PUT method */
			printk("\nCoAP client PUT\n");
			r = send_simple_coap_request(COAP_METHOD_PUT);
			if (r < 0) {
				return r;
			}

			break;
		case 2:
			/* Test CoAP POST method*/
			printk("\nCoAP client POST\n");
			r = send_simple_coap_request(COAP_METHOD_POST);
			if (r < 0) {
				return r;
			}

			break;
		case 3:
			/* Test CoAP DELETE method*/
			printk("\nCoAP client DELETE\n");
			r = send_simple_coap_request(COAP_METHOD_DELETE);
			if (r < 0) {
				return r;
			}

			break;
		default:
			return 0;
		}

		r = process_simple_coap_reply();
		if (r < 0) {
			return r;
		}

		test_type++;
	}

	return 0;
}

static int process_large_coap_reply(void)
{
	struct coap_packet reply;
	uint8_t *data;
	bool last_block;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	ret = coap_update_from_block(&reply, &blk_ctx);
	if (ret < 0) {
		goto end;
	}

	last_block = coap_next_block(&reply, &blk_ctx);
	if (!last_block) {
		ret = 1;
		goto end;
	}

	ret = 0;

end:
	k_free(data);

	return ret;
}

static int send_large_coap_request(void)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	if (blk_ctx.total_size == 0) {
		coap_block_transfer_init(&blk_ctx, COAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = large_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	r = coap_append_block2_option(&request, &blk_ctx);
	if (r < 0) {
		LOG_ERR("Unable to add block2 option.");
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int get_large_coap_msgs(void)
{
	int r;

	while (1) {
		/* Test CoAP Large GET method */
		printk("\nCoAP client Large GET (block %zd)\n",
		       blk_ctx.current / 64 /*COAP_BLOCK_64*/);
		r = send_large_coap_request();
		if (r < 0) {
			return r;
		}

		r = process_large_coap_reply();
		if (r < 0) {
			return r;
		}

		/* Received last block */
		if (r == 1) {
			memset(&blk_ctx, 0, sizeof(blk_ctx));
			return 0;
		}
	}

	return 0;
}

static void send_obs_reply_ack(uint16_t id)
{
	struct coap_packet request;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, 0, NULL, 0, id);
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	net_hexdump("ACK", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);
	if (r < 0) {
		LOG_ERR("Failed to send CoAP ACK");
	}
end:
	k_free(data);
}


static int obs_notification_cb(const struct coap_packet *response,
			       struct coap_reply *reply,
			       const struct sockaddr *from)
{
	uint16_t id = coap_header_get_id(response);
	uint8_t type = coap_header_get_type(response);
	uint8_t *counter = (uint8_t *)reply->user_data;

	ARG_UNUSED(from);

	printk("\nCoAP OBS Notification\n");

	(*counter)++;

	if (type == COAP_TYPE_CON) {
		send_obs_reply_ack(id);
	}

	return 0;
}

static int process_obs_coap_reply(struct coap_reply *reply)
{
	struct coap_packet reply_msg;
	uint8_t *data;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply_msg, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	if (coap_response_received(&reply_msg, NULL, reply, 1) == NULL) {
		printk("\nOther response received\n");
	}

end:
	k_free(data);

	return ret;
}

static int send_obs_coap_request(struct coap_reply *reply, void *user_data)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0) {
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	net_hexdump("Request", request.data, request.offset);

	coap_reply_init(reply, &request);
	reply->reply = obs_notification_cb;
	reply->user_data = user_data;

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int send_obs_reset_coap_request(struct coap_reply *reply)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     reply->tkl, reply->token,
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 1);
	if (r < 0) {
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int register_observer(void)
{
	struct coap_reply reply;
	uint8_t counter = 0;
	int r;

	printk("\nCoAP client OBS GET\n");
	r = send_obs_coap_request(&reply, &counter);
	if (r < 0) {
		return r;
	}

	while (1) {
		r = process_obs_coap_reply(&reply);
		if (r < 0) {
			return r;
		}

		if (counter >= 5) {
			/* TODO: Functionality can be verified byt waiting for
			 * some time and make sure client shouldn't receive
			 * any notifications. If client still receives
			 * notifications means, Observer is not removed.
			 */
			r = send_obs_reset_coap_request(&reply);
			if (r < 0) {
				return r;
			}

			/* Wait for the final ACK */
			r = process_obs_coap_reply(&reply);
			if (r < 0) {
				return r;
			}

			break;
		}
	}

	return 0;
}

int main(void)
{
	int r;

	LOG_DBG("Start CoAP-client sample");
	r = start_coap_client();
	if (r < 0) {
		goto quit;
	}

	/* GET, PUT, POST, DELETE */
	r = send_simple_coap_msgs_and_wait_for_reply();
	if (r < 0) {
		goto quit;
	}

	/* Block-wise transfer */
	r = get_large_coap_msgs();
	if (r < 0) {
		goto quit;
	}

	/* Register observer, get notifications and unregister */
	r = register_observer();
	if (r < 0) {
		goto quit;
	}

	/* Close the socket */
	(void)close(sock);

	LOG_DBG("Done");

	return 0;

quit:
	(void)close(sock);

	LOG_ERR("quit");
	return 0;
}
