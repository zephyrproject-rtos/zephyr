/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread/platform/mdns_socket.h"
#include "openthread/instance.h"
#include "openthread_border_router.h"
#include "platform-zephyr.h"
#include <common/code_utils.hpp>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/posix/sys/socket.h>

#define MULTICAST_PORT 5353
#define MAX_SERVICES   1

static struct pollfd sockfd_udp[MAX_SERVICES];
static int mdns_sock = -1;
static struct otInstance *ot_instance_ptr;
static uint32_t ail_iface_idx;

static otError mdns_socket_init(uint32_t ail_iface_idx);
static otError mdns_socket_deinit(void);
static otError set_listening_enable(otInstance *instance, bool enable, uint32_t ail_iface_idx);
static void mdns_send_multicast(otMessage *message, uint32_t ail_iface_idx);
static void mdns_send_unicast(otMessage *message, const otPlatMdnsAddressInfo *aAddress);
static void mdns_receive_handler(struct net_socket_service_event *evt);
static void process_mdns_message(struct otbr_msg_ctx *msg_ctx_ptr);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(mdns_udp_service, mdns_receive_handler, MAX_SERVICES);

extern struct k_mem_slab border_router_messages_slab;

otError mdns_plat_socket_init(otInstance *ot_instance, uint32_t ail_iface_idx)
{
	ot_instance_ptr = ot_instance;
	ail_iface_idx = ail_iface_idx;

	return OT_ERROR_NONE;
}

otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
	return set_listening_enable(aInstance, aEnable, aInfraIfIndex);
}

void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
	OT_UNUSED_VARIABLE(aInstance);

	mdns_send_multicast(aMessage, aInfraIfIndex);
}

void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage,
			   const otPlatMdnsAddressInfo *aAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	mdns_send_unicast(aMessage, aAddress);
}

static otError set_listening_enable(otInstance *instance, bool enable, uint32_t ail_iface_idx)
{
	OT_UNUSED_VARIABLE(instance);

	if (enable) {
		return mdns_socket_init(ail_iface_idx);
	}

	(void)ail_iface_idx;
	return mdns_socket_deinit();
}

static otError mdns_socket_init(uint32_t ail_iface_idx)
{
	otError error = OT_ERROR_NONE;
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};
	struct ifreq if_req = {0};
	struct ipv6_mreq mreq = {0};
	int mcast_hops = 255;
	int on = 1;

	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(MULTICAST_PORT),
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};

	mdns_sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	VerifyOrExit(mdns_sock >= 0, error = OT_ERROR_FAILED);
	VerifyOrExit(net_if_get_name(net_if_get_by_index(ail_iface_idx), name,
				     CONFIG_NET_INTERFACE_NAME_LEN) > 0,
		     error = OT_ERROR_FAILED);
	memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
	VerifyOrExit(zsock_setsockopt(mdns_sock, SOL_SOCKET, SO_BINDTODEVICE, &if_req,
				      sizeof(if_req)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	mreq.ipv6mr_ifindex = ail_iface_idx;
	net_ipv6_addr_create(&mreq.ipv6mr_multiaddr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);
	VerifyOrExit(zsock_setsockopt(mdns_sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq,
				      sizeof(mreq)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
				      &mcast_hops, sizeof(mcast_hops)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
				      &mcast_hops, sizeof(mcast_hops)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_bind(mdns_sock, (struct sockaddr *)&addr, sizeof(addr)) == 0,
		     error = OT_ERROR_FAILED);

	sockfd_udp[0].fd = mdns_sock;
	sockfd_udp[0].events = POLLIN;

	VerifyOrExit(net_socket_service_register(&mdns_udp_service, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}

static otError mdns_socket_deinit(void)
{
	otError error = OT_ERROR_NONE;

	VerifyOrExit(mdns_sock != -1, error = OT_ERROR_INVALID_STATE);
	VerifyOrExit(zsock_close(mdns_sock) == 0, error = OT_ERROR_FAILED);

	sockfd_udp[0].fd = -1;
	mdns_sock = -1;
exit:
	return error;
}

static void mdns_send_multicast(otMessage *message, uint32_t ail_iface_idx)
{
	uint8_t buffer[1500];
	uint16_t length;
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(MULTICAST_PORT),
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};

	net_ipv6_addr_create(&addr.sin6_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	length = otMessageGetLength(message);
	VerifyOrExit(otMessageRead(message, 0, buffer, length) == length);

	VerifyOrExit(zsock_sendto(mdns_sock, buffer, length, 0, (struct sockaddr *)&addr,
		     sizeof(addr)) > 0);
exit:
	otMessageFree(message);
}

static void mdns_send_unicast(otMessage *message, const otPlatMdnsAddressInfo *aAddress)
{
	uint8_t buffer[1500];
	uint16_t length;
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(aAddress->mPort),
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};

	memcpy(&addr.sin6_addr, &aAddress->mAddress, sizeof(otIp6Address));

	length = otMessageGetLength(message);
	VerifyOrExit(otMessageRead(message, 0, buffer, length) == length);

	VerifyOrExit(zsock_sendto(mdns_sock, buffer, length, 0, (struct sockaddr *)&addr,
		     sizeof(addr)) > 0);
exit:
	otMessageFree(message);
}

static void mdns_receive_handler(struct net_socket_service_event *evt)
{
	struct sockaddr_in6 addr = {0};
	socklen_t addrlen = sizeof(addr);
	ssize_t len = 0;
	struct otbr_msg_ctx *req = NULL;

	VerifyOrExit(evt->event.revents & ZSOCK_POLLIN);
	VerifyOrExit(k_mem_slab_alloc(&border_router_messages_slab, (void **)&req, K_NO_WAIT) == 0);
	memset(req, 0, sizeof(struct otbr_msg_ctx));

	len = recvfrom(mdns_sock, req->buffer, sizeof(req->buffer), 0,
		       (struct sockaddr *)&addr, &addrlen);
	VerifyOrExit(len > 0);

	memcpy(&req->addr_info.mAddress, &addr.sin6_addr, sizeof(req->addr_info.mAddress));
	req->length = (uint16_t)len;
	req->addr_info.mPort = ntohs(addr.sin6_port);
	req->addr_info.mInfraIfIndex = ail_iface_idx;
	req->cb = process_mdns_message;

	openthread_border_router_post_message(req);

exit:
	return;
}

static void process_mdns_message(struct otbr_msg_ctx *msg_ctx_ptr)
{
	otMessageSettings ot_message_settings = {true, OT_MESSAGE_PRIORITY_NORMAL};
	otMessage *ot_message = NULL;

	ot_message = otIp6NewMessage(ot_instance_ptr, &ot_message_settings);
	VerifyOrExit(ot_message);
	VerifyOrExit(otMessageAppend(ot_message, msg_ctx_ptr->buffer,
				     msg_ctx_ptr->length) == OT_ERROR_NONE);
	otPlatMdnsHandleReceive(ot_instance_ptr, ot_message, /* aIsUnicast */ false,
				&msg_ctx_ptr->addr_info);
	ot_message = NULL;
exit:
	if (ot_message) {
		otMessageFree(ot_message);
	}
}
