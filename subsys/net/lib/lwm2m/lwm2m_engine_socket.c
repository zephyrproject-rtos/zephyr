/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lib/lwm2m_engine_socket"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>

#include <zephyr/types.h>
#include <errno.h>
#include <net/net_ip.h>
#include <net/socket.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_engine_socket.h"

#define RECEIVE_LOOP_DELAY	K_MSEC(1000)

int lwm2m_nl_socket_msg_send(struct lwm2m_message *msg)
{
	if (!msg) {
		return -EINVAL;
	}

	if (send(((struct net_layer_socket *)
		  lwm2m_nl_api_from_ctx(msg->ctx)->nl_user_data)->sock_fd,
		 msg->cpkt.fbuf.buf, msg->cpkt.fbuf.buf_len, 0) < 0) {
		return -errno;
	}

	return 0;
}

/* LwM2M main work loop */
static void socket_receive_loop(struct k_work *work)
{
	/* TODO: 32 is just a guess at "non-payload" room needed */
	static u8_t in_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE + 32];
	struct net_layer_socket *nl_data;
	static struct sockaddr from_addr;
	socklen_t from_addr_len;
	ssize_t len;

	nl_data = CONTAINER_OF(work, struct net_layer_socket, receive_work);
	while (1) {
		len = recvfrom(nl_data->sock_fd,
			       in_buf, sizeof(in_buf) - 1, 0,
			       &from_addr, &from_addr_len);

		if (len < 0) {
			SYS_LOG_ERR("Error reading response: %d", errno);
			continue;
		}

		if (len == 0) {
			SYS_LOG_ERR("Zero length recv");
			continue;
		}

		in_buf[len] = 0;

		lwm2m_udp_receive(nl_data->ctx, in_buf, len, &from_addr, false,
				  lwm2m_handle_request);

	}
}

int lwm2m_nl_socket_start(struct lwm2m_ctx *client_ctx,
			  char *peer_str, u16_t peer_port)
{
	struct net_layer_socket *nl_data;
#if defined(CONFIG_DNS_RESOLVER)
	struct addrinfo hints;
	struct addrinfo *res;
#endif
	int ret = -EINVAL;

	nl_data = (struct net_layer_socket *)
		lwm2m_nl_api_from_ctx(client_ctx)->nl_user_data;
	(void)memset(nl_data, 0, sizeof(*nl_data));
	nl_data->ctx = client_ctx;

	k_delayed_work_init(&nl_data->receive_work, socket_receive_loop);

	/* try setting PEER directly */
	(void)memset(&client_ctx->remote_addr, 0,
		     sizeof(client_ctx->remote_addr));
#if defined(CONFIG_NET_IPV6)
	client_ctx->remote_addr.sa_family = AF_INET6;
	net_sin6(&client_ctx->remote_addr)->sin6_port = htons(peer_port);
	ret = inet_pton(AF_INET6, peer_str,
			&((struct sockaddr_in6 *)
				&client_ctx->remote_addr)->sin6_addr);
#elif defined(CONFIG_NET_IPV4)
	client_ctx->remote_addr.sa_family = AF_INET;
	net_sin(&client_ctx->remote_addr)->sin_port = htons(peer_port);
	ret = inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			&((struct sockaddr_in *)
				&client_ctx->remote_addr)->sin_addr);
#endif

	if (ret < 0) {
		SYS_LOG_DBG("Address not an IP.  Trying resolve?");
	}

#if defined(CONFIG_DNS_RESOLVER)
	hints.ai_family = client_ctx->remote_addr.sa_family;
	hints.ai_socktype = SOCK_DGRAM;
	ret = getaddrinfo(peer_str, peer_port, &hints, &res);
	if (ret != 0) {
		SYS_LOG_ERR("Unable to resolve address");
		return -ENOENT;
	}
#endif

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	nl_data->sock_fd = socket(client_ctx->remote_addr.sa_family,
				  SOCK_DGRAM, IPPROTO_DTLS_1_2);
#else
	nl_data->sock_fd = socket(client_ctx->remote_addr.sa_family,
				  SOCK_DGRAM, IPPROTO_UDP);
#endif
	if (nl_data->sock_fd < 0) {
		SYS_LOG_ERR("Failed to create socket: %d", errno);
		return -errno;
	}

	if (connect(nl_data->sock_fd, &client_ctx->remote_addr,
		    NET_SOCKADDR_MAX_SIZE) < 0) {
		SYS_LOG_ERR("Cannot connect UDP (-%d)", errno);
		return -errno;
	}

	k_delayed_work_submit(&nl_data->receive_work, RECEIVE_LOOP_DELAY);
	return 0;
}

static struct net_layer_socket nl_socket_data;

static struct lwm2m_net_layer_api nl_socket_api = {
	.nl_start     = lwm2m_nl_socket_start,
	.nl_msg_send  = lwm2m_nl_socket_msg_send,
	.nl_user_data = &nl_socket_data,
};

struct lwm2m_net_layer_api *lwm2m_engine_nl_socket_api(void)
{
	return &nl_socket_api;
}
