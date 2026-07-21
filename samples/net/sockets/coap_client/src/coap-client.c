/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/coap.h>

#include "net_sample_common.h"

LOG_MODULE_REGISTER(net_coap_client_sample, LOG_LEVEL_DBG);

#include "net_private.h"

#define MAX_COAP_MSG_LEN 256
#define COAP_DEFAULT_PORT 5683
#define COAP_DEFAULT_PORT_STR STRINGIFY(COAP_DEFAULT_PORT)
#define COAP_PEER_HOST_MAX_LEN 128
#define COAP_PEER_PORT_MAX_LEN 6

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

static int wait_for_reply(void)
{
	int timeout_ms = -1;
	int ret;

#if CONFIG_NET_SAMPLE_COAP_CLIENT_REPLY_TIMEOUT_MS > 0
	timeout_ms = CONFIG_NET_SAMPLE_COAP_CLIENT_REPLY_TIMEOUT_MS;
#endif

	ret = poll(fds, nfds, timeout_ms);
	if (ret < 0) {
		LOG_ERR("Error in poll:%d", errno);
		return -errno;
	}

	if (ret == 0) {
		LOG_WRN("Timed out waiting for CoAP reply");
		return -ETIMEDOUT;
	}

	return 0;
}

static void prepare_fds(void)
{
	fds[nfds].fd = sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

static int resolve_peer_addr(struct sockaddr_storage *addr, socklen_t *addrlen)
{
	const char *peer = CONFIG_NET_SAMPLE_COAP_CLIENT_PEER;
	struct sockaddr_storage parsed_addr = { 0 };
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
		.ai_protocol = IPPROTO_UDP,
	};
	struct addrinfo *res = NULL;
	struct addrinfo *entry;
	char host[COAP_PEER_HOST_MAX_LEN];
	char port[COAP_PEER_PORT_MAX_LEN];
	const char *host_name = peer;
	const char *port_ptr;
	const char *last_colon;
	int ret;
	size_t host_len;

	if (net_ipaddr_parse(peer, strlen(peer), net_sad(&parsed_addr))) {
		switch (net_sad(&parsed_addr)->sa_family) {
		case AF_INET:
			if (!IS_ENABLED(CONFIG_NET_IPV4)) {
				return -EAFNOSUPPORT;
			}

			if (net_sin(net_sad(&parsed_addr))->sin_port == 0U) {
				net_sin(net_sad(&parsed_addr))->sin_port =
					htons(COAP_DEFAULT_PORT);
			}

			*addrlen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			if (!IS_ENABLED(CONFIG_NET_IPV6)) {
				return -EAFNOSUPPORT;
			}

			if (net_sin6(net_sad(&parsed_addr))->sin6_port == 0U) {
				net_sin6(net_sad(&parsed_addr))->sin6_port =
					htons(COAP_DEFAULT_PORT);
			}

			*addrlen = sizeof(struct sockaddr_in6);
			break;
		default:
			return -EINVAL;
		}

		memcpy(addr, &parsed_addr, *addrlen);
		return 0;
	}

	memcpy(port, COAP_DEFAULT_PORT_STR, sizeof(COAP_DEFAULT_PORT_STR));

	port_ptr = strstr(peer, ":");
	last_colon = strrchr(peer, ':');
	if (port_ptr != NULL) {
		if (port_ptr != last_colon || port_ptr == peer || port_ptr[1] == '\0') {
			LOG_ERR("Invalid peer %s", peer);
			return -EINVAL;
		}

		host_len = port_ptr - peer;
		if (host_len >= sizeof(host)) {
			LOG_ERR("Invalid peer %s", peer);
			return -EINVAL;
		}

		memcpy(host, peer, host_len);
		host[host_len] = '\0';
		host_name = host;

		if (strlen(port_ptr + 1) >= sizeof(port)) {
			LOG_ERR("Invalid peer %s", peer);
			return -EINVAL;
		}

		memcpy(port, port_ptr + 1, strlen(port_ptr + 1) + 1);
	}

	ret = getaddrinfo(host_name, port, &hints, &res);
	if (ret != 0) {
		LOG_ERR("Cannot resolve %s (%d)", peer, ret);
		return -ENOENT;
	}

	ret = -EAFNOSUPPORT;

	for (entry = res; entry != NULL; entry = entry->ai_next) {
		if (entry->ai_addrlen > sizeof(*addr)) {
			continue;
		}

		if (entry->ai_family == AF_INET && !IS_ENABLED(CONFIG_NET_IPV4)) {
			continue;
		}

		if (entry->ai_family == AF_INET6 && !IS_ENABLED(CONFIG_NET_IPV6)) {
			continue;
		}

		memcpy(addr, entry->ai_addr, entry->ai_addrlen);
		*addrlen = entry->ai_addrlen;
		ret = 0;
		break;
	}

	freeaddrinfo(res);

	return ret;
}

static int start_coap_client(void)
{
	int ret = 0;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addrlen;

	ret = resolve_peer_addr(&peer_addr, &peer_addrlen);
	if (ret < 0) {
		return ret;
	}

	sock = socket((net_sad(&peer_addr))->sa_family,
		      SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = connect(sock, net_sad(&peer_addr), peer_addrlen);
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
	uint8_t *data = NULL;
	int rcvd;
	int ret;

	ret = wait_for_reply();
	if (ret < 0) {
		goto end;
	}

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
	uint8_t *data = NULL;
	bool last_block;
	int rcvd;
	int ret;

	ret = wait_for_reply();
	if (ret < 0) {
		goto end;
	}

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
	int retries = 0;
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
			if (r == -ETIMEDOUT &&
			    retries < CONFIG_NET_SAMPLE_COAP_CLIENT_BLOCKWISE_MAX_RETRIES) {
				retries++;
				LOG_WRN("Timed out waiting for block %zd, retry %d/%d",
					blk_ctx.current / 64 /* COAP_BLOCK_64 */,
					retries,
					CONFIG_NET_SAMPLE_COAP_CLIENT_BLOCKWISE_MAX_RETRIES);
				continue;
			}
			return r;
		}

		retries = 0;

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
	uint8_t *data = NULL;
	int rcvd;
	int ret;

	ret = wait_for_reply();
	if (ret < 0) {
		goto end;
	}

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
		LOG_DBG("Ignoring unrelated CoAP response");
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
	wait_for_network();

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
