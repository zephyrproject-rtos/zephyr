/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread/trel.h"
#include "openthread_border_router.h"
#include <common/code_utils.hpp>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include "sockets_internal.h"

#define MAX_SERVICES   1

static struct pollfd sockfd_udp[MAX_SERVICES];
static int trel_sock = -1;
static struct otInstance *ot_instance_ptr;
static otPlatTrelCounters s_trel_counters;
static bool s_trel_is_enabled;
static void trel_receive_handler(struct net_socket_service_event *evt);
static void process_trel_message(struct otbr_msg_ctx *msg_ctx_ptr);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(trel_udp_service, trel_receive_handler, MAX_SERVICES);

extern struct k_mem_slab border_router_messages_slab;

void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort)
{
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = 0,
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};
	socklen_t len = sizeof(addr);

	trel_sock = zsock_socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
	VerifyOrExit(trel_sock >= 0);
	VerifyOrExit(zsock_bind(trel_sock, (struct sockaddr *)&addr, sizeof(addr)) == 0);

	VerifyOrExit(zsock_getsockname(trel_sock, (struct sockaddr *)&addr, &len) == 0);

	*aUdpPort = ntohs(addr.sin6_port);

	otPlatTrelResetCounters(aInstance);

	s_trel_is_enabled = true;

exit:
	return;

}

void otPlatTrelDisable(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	VerifyOrExit(s_trel_is_enabled);

	VerifyOrExit(trel_sock != -1);
	VerifyOrExit(zsock_close(trel_sock) == 0);

	sockfd_udp[0].fd = -1;
	trel_sock = -1;

	s_trel_is_enabled = false;

exit:
	return;
}

void otPlatTrelSend(otInstance *aInstance, const uint8_t *aUdpPayload, uint16_t aUdpPayloadLen,
		    const otSockAddr *aDestSockAddr)
{
	VerifyOrExit(s_trel_is_enabled);

	struct sockaddr_in6 dest_sock_addr = {.sin6_family = AF_INET6,
				.sin6_port = htons(aDestSockAddr->mPort),
				.sin6_addr = {{{0}}},
				.sin6_scope_id = 0};

	memcpy(&dest_sock_addr.sin6_addr, &aDestSockAddr->mAddress, sizeof(otIp6Address));

	if (zsock_sendto(trel_sock, aUdpPayload, aUdpPayloadLen, 0,
				(struct sockaddr *)&dest_sock_addr,
				sizeof(dest_sock_addr)) == aUdpPayloadLen) {
		s_trel_counters.mTxBytes += aUdpPayloadLen;
		s_trel_counters.mTxPackets++;
	} else {
		s_trel_counters.mTxFailure++;
	}

exit:
	return;
}

const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return &s_trel_counters;
}

void otPlatTrelResetCounters(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	memset(&s_trel_counters, 0, sizeof(s_trel_counters));
}

static void trel_receive_handler(struct net_socket_service_event *evt)
{
	struct sockaddr_in6 addr = {0};
	socklen_t addrlen = sizeof(addr);
	ssize_t len = 0;
	struct otbr_msg_ctx *req = NULL;

	s_trel_counters.mRxPackets++;

	VerifyOrExit(evt->event.revents & ZSOCK_POLLIN);
	VerifyOrExit(k_mem_slab_alloc(&border_router_messages_slab, (void **)&req, K_NO_WAIT) == 0);
	memset(req, 0, sizeof(struct otbr_msg_ctx));

	len = recvfrom(trel_sock, req->buffer, sizeof(req->buffer), 0,
		       (struct sockaddr *)&addr, &addrlen);
	VerifyOrExit(len > 0);

	s_trel_counters.mRxBytes += len;

	memcpy(&req->sock_addr.mAddress, &addr.sin6_addr, sizeof(otIp6Address));
	req->length = (uint16_t)len;
	req->sock_addr.mPort = ntohs(addr.sin6_port);
	req->cb = process_trel_message;

	openthread_border_router_post_message(req);

exit:
	return;
}

static void process_trel_message(struct otbr_msg_ctx *msg_ctx_ptr)
{
	otPlatTrelHandleReceived(ot_instance_ptr, msg_ctx_ptr->buffer, msg_ctx_ptr->length,
				 &msg_ctx_ptr->sock_addr);
}

otError trel_plat_init(otInstance *instance, struct net_if *ail_iface_ptr)
{
	otError error = OT_ERROR_NONE;
	struct ifreq if_req = {0};
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};

	ot_instance_ptr = instance;

	VerifyOrExit(net_if_get_name(ail_iface_ptr, name,
				     CONFIG_NET_INTERFACE_NAME_LEN) > 0,
		     error = OT_ERROR_FAILED);
	memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
	VerifyOrExit(zsock_setsockopt(trel_sock, SOL_SOCKET, SO_BINDTODEVICE, &if_req,
				      sizeof(if_req)) == 0,
		      error = OT_ERROR_FAILED);

	sockfd_udp[0].fd = trel_sock;
	sockfd_udp[0].events = POLLIN;

	VerifyOrExit(net_socket_service_register(&trel_udp_service, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0,
		     error = OT_ERROR_FAILED);
exit:
	return error;
}
