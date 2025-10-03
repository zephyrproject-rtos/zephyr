/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread_border_router.h"
#include <common/code_utils.hpp>
#include <openthread/platform/infra_if.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include "sockets_internal.h"

#define DHCPV6_SERVER_PORT 547
#define DHCPV6_CLIENT_PORT 546
#define DHCPV6_PD_CLIENT_NUM_SERVICES 1

static struct zsock_pollfd sockfd_udp[DHCPV6_PD_CLIENT_NUM_SERVICES];
static struct otInstance *ot_instance_ptr;
static uint32_t ail_iface_idx;
static int dhcpv6_pd_client_sock = -1;

static void dhcpv6_pd_client_receive_handler(struct net_socket_service_event *evt);
static void process_dhcpv6_pd_client_message(struct otbr_msg_ctx *msg_ctx_ptr);
static void dhcpv6_pd_client_socket_init(uint32_t infra_if_index);
static void dhcpv6_pd_client_socket_deinit(uint32_t infra_if_index);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(dhcpv6_pd_client_udp_receive,
				      dhcpv6_pd_client_receive_handler,
				      DHCPV6_PD_CLIENT_NUM_SERVICES);

otError dhcpv6_pd_client_init(otInstance *ot_instance, uint32_t ail_iface_index)
{
	ot_instance_ptr = ot_instance;
	ail_iface_idx = ail_iface_index;

	sockfd_udp[0].fd = -1;

	return OT_ERROR_NONE;
}

void otPlatInfraIfDhcp6PdClientSetListeningEnabled(otInstance *aInstance, bool aEnable,
						   uint32_t aInfraIfIndex)
{
	if (aEnable) {
		dhcpv6_pd_client_socket_init(aInfraIfIndex);
	} else {
		dhcpv6_pd_client_socket_deinit(aInfraIfIndex);
	}
}

void otPlatInfraIfDhcp6PdClientSend(otInstance *aInstance,
				    otMessage *aMessage,
				    otIp6Address *aDestAddress,
				    uint32_t aInfraIfIndex)
{
	struct otbr_msg_ctx *req = NULL;

	VerifyOrExit(dhcpv6_pd_client_sock != -1 && aInfraIfIndex == ail_iface_idx);
	uint16_t length = otMessageGetLength(aMessage);
	struct sockaddr_in6 dest_sock_addr = {.sin6_family = AF_INET6,
					      .sin6_port = htons(DHCPV6_SERVER_PORT),
					      .sin6_addr = IN6ADDR_ANY_INIT,
					      .sin6_scope_id = 0};

	memcpy(&dest_sock_addr.sin6_addr, aDestAddress, sizeof(otIp6Address));

	VerifyOrExit(length <= OTBR_MESSAGE_SIZE);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) ==
		     OT_ERROR_NONE);
	VerifyOrExit(otMessageRead(aMessage, 0, req->buffer, length) == length);

	VerifyOrExit(zsock_sendto(dhcpv6_pd_client_sock, req->buffer, length, 0,
				  (struct sockaddr *)&dest_sock_addr,
				  sizeof(dest_sock_addr)) == length);

exit:
	otMessageFree(aMessage);
	if (req != NULL) {
		openthread_border_router_deallocate_message((void *)req);
	}
}

static void dhcpv6_pd_client_receive_handler(struct net_socket_service_event *evt)
{
	struct sockaddr_in6 addr = {0};
	socklen_t addrlen = sizeof(addr);
	ssize_t len = 0;
	struct otbr_msg_ctx *req = NULL;

	VerifyOrExit(evt->event.revents & ZSOCK_POLLIN);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE);
	memset(req, 0, sizeof(struct otbr_msg_ctx));

	len = zsock_recvfrom(dhcpv6_pd_client_sock, req->buffer, sizeof(req->buffer), 0,
			     (struct sockaddr *)&addr, &addrlen);
	VerifyOrExit(len > 0, openthread_border_router_deallocate_message((void *)req));

	req->length = (uint16_t)len;
	req->sock_addr.mPort = ntohs(addr.sin6_port);
	req->cb = process_dhcpv6_pd_client_message;

	openthread_border_router_post_message(req);
exit:
	return;
}

static void dhcpv6_pd_client_socket_init(uint32_t infra_if_index)
{
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};
	struct ifreq if_req = {0};
	int hop_limit = 255;
	int on = 1;
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(DHCPV6_CLIENT_PORT),
				    .sin6_addr = IN6ADDR_ANY_INIT,
				    .sin6_scope_id = 0};

	dhcpv6_pd_client_sock = zsock_socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
	VerifyOrExit(dhcpv6_pd_client_sock >= 0);

	VerifyOrExit(zsock_bind(dhcpv6_pd_client_sock, (struct sockaddr *)&addr,
				sizeof(addr)) == 0);

	VerifyOrExit(zsock_setsockopt(dhcpv6_pd_client_sock, SOL_SOCKET, SO_REUSEADDR,
				      &on, sizeof(on)) == 0);
	VerifyOrExit(zsock_setsockopt(dhcpv6_pd_client_sock, SOL_SOCKET, SO_REUSEPORT,
				      &on, sizeof(on)) == 0);

	VerifyOrExit(net_if_get_name(net_if_get_by_index(infra_if_index), name,
				     CONFIG_NET_INTERFACE_NAME_LEN) > 0);
	memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
	VerifyOrExit(zsock_setsockopt(dhcpv6_pd_client_sock, SOL_SOCKET, SO_BINDTODEVICE, &if_req,
				      sizeof(if_req)) == 0);
	VerifyOrExit(zsock_setsockopt(dhcpv6_pd_client_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
				      &hop_limit, sizeof(hop_limit)) == 0);
	VerifyOrExit(zsock_setsockopt(dhcpv6_pd_client_sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
				      &hop_limit, sizeof(hop_limit)) == 0);
	VerifyOrExit(zsock_setsockopt(dhcpv6_pd_client_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
				      &infra_if_index, sizeof(infra_if_index)) == 0);

	sockfd_udp[0].fd = dhcpv6_pd_client_sock;
	sockfd_udp[0].events = ZSOCK_POLLIN;

	VerifyOrExit(net_socket_service_register(&dhcpv6_pd_client_udp_receive, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0);

exit:
	return;
}

static void dhcpv6_pd_client_socket_deinit(uint32_t infra_if_index)
{
	VerifyOrExit(dhcpv6_pd_client_sock != -1);
	VerifyOrExit(zsock_close(dhcpv6_pd_client_sock) == 0);

	sockfd_udp[0].fd = -1;
	dhcpv6_pd_client_sock = -1;
exit:
	return;
}

static void process_dhcpv6_pd_client_message(struct otbr_msg_ctx *msg_ctx_ptr)
{
	otMessage *ot_message = NULL;

	ot_message = otIp6NewMessage(ot_instance_ptr, NULL);
	VerifyOrExit(ot_message);
	VerifyOrExit(otMessageAppend(ot_message, msg_ctx_ptr->buffer,
				     msg_ctx_ptr->length) == OT_ERROR_NONE);

	/* Ownership of the message is passed to OT stack, no need to free it here */
	otPlatInfraIfDhcp6PdClientHandleReceived(ot_instance_ptr, ot_message, ail_iface_idx);

exit:
	return;
}
