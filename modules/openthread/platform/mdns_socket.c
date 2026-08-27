/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread/platform/mdns_socket.h"
#include <openthread/nat64.h>
#include <openthread/platform/messagepool.h>
#include "openthread/instance.h"
#include "openthread_border_router.h"
#include "platform-zephyr.h"
#include <common/code_utils.hpp>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/socket_service.h>
#include "sockets_internal.h"
#include <errno.h>

#define MULTICAST_PORT 5353
#define MAX_SERVICES CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_MDNS_SERVICES
#define PKT_THRESHOLD_VAL CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MDNS_BUFFER_THRESHOLD

static struct zsock_pollfd sockfd_udp[MAX_SERVICES];
static int mdns_sock_v6 = -1;
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static int mdns_sock_v4 = -1;
#endif /* #if CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

static struct otInstance *ot_instance_ptr;
static uint32_t ail_iface_index;

static otError mdns_socket_init_v6(uint32_t ail_iface_idx);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static otError mdns_socket_init_v4(uint32_t ail_iface_idx);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */
static otError mdns_socket_deinit(void);
static otError set_listening_enable(otInstance *instance, bool enable, uint32_t ail_iface_idx);
static void mdns_send_multicast(otMessage *message, uint32_t ail_iface_idx);
static void mdns_send_unicast(otMessage *message, const otPlatMdnsAddressInfo *address);
static void mdns_send_multicast_v6(uint8_t *message, uint16_t length);
static void mdns_send_unicast_v6(uint8_t *message, uint16_t length,
				 const otPlatMdnsAddressInfo *address);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static void mdns_send_multicast_v4(uint8_t *message, uint16_t length);
static void mdns_send_unicast_v4(uint8_t *message, uint16_t length,
				 const otPlatMdnsAddressInfo *address);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */
static void mdns_receive_handler(struct net_socket_service_event *evt);
static void process_mdns_message(struct otbr_msg_ctx *msg_ctx_ptr);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(mdns_udp_service, mdns_receive_handler, MAX_SERVICES);

otError mdns_plat_socket_init(otInstance *ot_instance, uint32_t ail_iface_idx)
{
	ot_instance_ptr = ot_instance;
	ail_iface_index = ail_iface_idx;

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
	otError error = OT_ERROR_NONE;

	if (enable) {
		SuccessOrExit(error = mdns_socket_init_v6(ail_iface_idx));
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
		SuccessOrExit(error = mdns_socket_init_v4(ail_iface_idx));
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

		mdns_plat_monitor_interface(net_if_get_by_index(ail_iface_idx));
		ExitNow();
	}

	SuccessOrExit(error = mdns_socket_deinit());
exit:
	return error;

}

static otError mdns_socket_init_v6(uint32_t ail_iface_idx)
{
	otError error = OT_ERROR_NONE;
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};
	struct net_ifreq if_req = {0};
	struct net_ipv6_mreq mreq_v6 = {0};
	int mcast_hops = 255;
	int on = 1;
	int mcast_join_ret = 0;

	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(MULTICAST_PORT),
		.sin6_addr = {{{0}}},
		.sin6_scope_id = 0,
	};

	mdns_sock_v6 = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	VerifyOrExit(mdns_sock_v6 >= 0, error = OT_ERROR_FAILED);
	VerifyOrExit(net_if_get_name(net_if_get_by_index(ail_iface_idx), name,
				     CONFIG_NET_INTERFACE_NAME_LEN) > 0,
		     error = OT_ERROR_FAILED);
	memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
				      &if_req, sizeof(if_req)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR,
				      &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEPORT,
				      &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &on,
				      sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	mreq_v6.ipv6mr_ifindex = ail_iface_idx;
	net_ipv6_addr_create(&mreq_v6.ipv6mr_multiaddr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);
	mcast_join_ret = zsock_setsockopt(mdns_sock_v6, NET_IPPROTO_IPV6, ZSOCK_IPV6_ADD_MEMBERSHIP,
					  &mreq_v6, sizeof(mreq_v6));
	VerifyOrExit(mcast_join_ret == 0 || (mcast_join_ret == -1 && errno == EALREADY),
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_LOOP,
				      &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_IF,
				      &ail_iface_index, sizeof(ail_iface_index)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_HOPS,
				      &mcast_hops, sizeof(mcast_hops)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v6, NET_IPPROTO_IPV6, ZSOCK_IPV6_UNICAST_HOPS,
				      &mcast_hops, sizeof(mcast_hops)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_bind(mdns_sock_v6, (struct net_sockaddr *)&addr, sizeof(addr)) == 0,
		     error = OT_ERROR_FAILED);

	sockfd_udp[0].fd = mdns_sock_v6;
	sockfd_udp[0].events = ZSOCK_POLLIN;

	VerifyOrExit(net_socket_service_register(&mdns_udp_service, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static otError mdns_socket_init_v4(uint32_t ail_iface_idx)
{
	otError error = OT_ERROR_NONE;
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};
	struct net_ifreq if_req = {0};
	struct net_ip_mreqn mreq_v4 = {0};
	int mcast_hops = 255;
	int on = 1;
	int mcast_join_ret = 0;

	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(MULTICAST_PORT),
		.sin_addr = {{{0}}},
	};

	mdns_sock_v4 = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	VerifyOrExit(mdns_sock_v4 >= 0, error = OT_ERROR_FAILED);
	VerifyOrExit(net_if_get_name(net_if_get_by_index(ail_iface_idx), name,
				     CONFIG_NET_INTERFACE_NAME_LEN) > 0,
		     error = OT_ERROR_FAILED);
	memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
				      &if_req, sizeof(if_req)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR,
				      &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEPORT,
				      &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	mreq_v4.imr_ifindex = ail_iface_idx;
	mreq_v4.imr_multiaddr.s_addr = net_htonl(0xE00000FB);
	mcast_join_ret = zsock_setsockopt(mdns_sock_v4, NET_IPPROTO_IP, ZSOCK_IP_ADD_MEMBERSHIP,
					  &mreq_v4, sizeof(mreq_v4));
	VerifyOrExit(mcast_join_ret == 0 || (mcast_join_ret == -1 && errno == EALREADY),
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_LOOP,
				      &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_IF,
				      &mreq_v4, sizeof(mreq_v4)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_TTL,
				      &mcast_hops, sizeof(mcast_hops)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(mdns_sock_v4, NET_IPPROTO_IP, ZSOCK_IP_TTL,
				      &mcast_hops, sizeof(mcast_hops)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_bind(mdns_sock_v4, (struct net_sockaddr *)&addr, sizeof(addr)) == 0,
		     error = OT_ERROR_FAILED);

	sockfd_udp[1].fd = mdns_sock_v4;
	sockfd_udp[1].events = ZSOCK_POLLIN;

	VerifyOrExit(net_socket_service_register(&mdns_udp_service, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

static otError mdns_socket_deinit(void)
{
	otError error = OT_ERROR_NONE;

	VerifyOrExit(mdns_sock_v6 != -1, error = OT_ERROR_INVALID_STATE);
	VerifyOrExit(zsock_close(mdns_sock_v6) == 0, error = OT_ERROR_FAILED);

	sockfd_udp[0].fd = -1;
	mdns_sock_v6 = -1;

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
	VerifyOrExit(mdns_sock_v4 != -1, error = OT_ERROR_INVALID_STATE);
	VerifyOrExit(zsock_close(mdns_sock_v4) == 0, error = OT_ERROR_FAILED);

	sockfd_udp[1].fd = -1;
	mdns_sock_v4 = -1;
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

	VerifyOrExit(net_socket_service_register(&mdns_udp_service, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0,
						 error = OT_ERROR_FAILED);
exit:
	return error;
}

static void mdns_send_multicast(otMessage *message, uint32_t ail_iface_idx)
{
	uint16_t length = otMessageGetLength(message);
	struct otbr_msg_ctx *req = NULL;

	VerifyOrExit(length <= OTBR_MESSAGE_SIZE);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE);
	VerifyOrExit(otMessageRead(message, 0, req->buffer, length) == length);

	mdns_send_multicast_v6(req->buffer, length);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
	mdns_send_multicast_v4(req->buffer, length);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

exit:
	otMessageFree(message);
	if (req != NULL) {
		openthread_border_router_deallocate_message((void *)req);
	}
}

static void mdns_send_multicast_v6(uint8_t *message, uint16_t length)
{
	struct net_sockaddr_in6 addr_v6 = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(MULTICAST_PORT),
		.sin6_addr = NET_IN6ADDR_ANY_INIT,
		.sin6_scope_id = 0,
	};

	net_ipv6_addr_create(&addr_v6.sin6_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	VerifyOrExit(zsock_sendto(mdns_sock_v6, message, length, 0,
		     (struct net_sockaddr *)&addr_v6, sizeof(addr_v6)) == length);

exit:
	return;
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static void mdns_send_multicast_v4(uint8_t *message, uint16_t length)
{
	if (openthread_border_router_has_ipv4_connectivity()) {
		struct net_sockaddr_in addr_v4 = {
			.sin_family = NET_AF_INET,
			.sin_port = net_htons(MULTICAST_PORT),
			.sin_addr = NET_INADDR_ANY_INIT,
		};

		addr_v4.sin_addr.s_addr = net_htonl(0xE00000FB);
		VerifyOrExit(zsock_sendto(mdns_sock_v4, message, length, 0,
			     (struct net_sockaddr *)&addr_v4, sizeof(addr_v4)) == length);
	}

exit:
	return;
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

static void mdns_send_unicast(otMessage *message, const otPlatMdnsAddressInfo *address)
{
	uint16_t length = otMessageGetLength(message);
	struct otbr_msg_ctx *req = NULL;
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)

#endif /* CONFIG_NET_IPV4*/

	VerifyOrExit(length <= OTBR_MESSAGE_SIZE);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE);
	VerifyOrExit(otMessageRead(message, 0, req->buffer, length) == length);

	mdns_send_unicast_v6(req->buffer, length, address);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
	mdns_send_unicast_v4(req->buffer, length, address);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

exit:
	otMessageFree(message);
	if (req != NULL) {
		openthread_border_router_deallocate_message((void *)req);
	}
}

static void mdns_send_unicast_v6(uint8_t *message, uint16_t length,
				 const otPlatMdnsAddressInfo *address)
{
	struct net_sockaddr_in6 addr_v6 = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(address->mPort),
		.sin6_addr = NET_IN6ADDR_ANY_INIT,
		.sin6_scope_id = 0,
	};

	memcpy(&addr_v6.sin6_addr, &address->mAddress, sizeof(otIp6Address));

	VerifyOrExit(zsock_sendto(mdns_sock_v6, message, length, 0,
		     (struct net_sockaddr *)&addr_v6, sizeof(addr_v6)) == length);

exit:
	return;
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static void mdns_send_unicast_v4(uint8_t *message, uint16_t length,
				 const otPlatMdnsAddressInfo *address)
{
	struct net_sockaddr_in addr_v4 = {0};
	otIp4Address ot_ipv4_addr = {0};

	if (openthread_border_router_has_ipv4_connectivity() &&
	    otIp4FromIp4MappedIp6Address(&address->mAddress, &ot_ipv4_addr) == OT_ERROR_NONE) {
		addr_v4.sin_family = NET_AF_INET;
		addr_v4.sin_port = net_htons(address->mPort);
		memcpy(&addr_v4.sin_addr.s_addr, &ot_ipv4_addr, sizeof(otIp4Address));
		VerifyOrExit(zsock_sendto(mdns_sock_v4, message, length, 0,
			     (struct net_sockaddr *)&addr_v4, sizeof(addr_v4)) == length);
	}

exit:
	return;
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

static void mdns_receive_handler(struct net_socket_service_event *evt)
{
	struct net_sockaddr_in6 addr_v6 = {0};
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
	struct net_sockaddr_in addr_v4 = {0};
	net_socklen_t optlen = sizeof(int);
	int family;
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */
	net_socklen_t addrlen;
	ssize_t len = 0;
	struct otbr_msg_ctx *req = NULL;

	VerifyOrExit(evt->event.revents & ZSOCK_POLLIN);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE);

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
	(void)zsock_getsockopt(evt->event.fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_DOMAIN, &family, &optlen);

	if (family == NET_AF_INET) {
		addrlen = sizeof(addr_v4);
		len = zsock_recvfrom(mdns_sock_v4, req->buffer, sizeof(req->buffer), 0,
				     (struct net_sockaddr *)&addr_v4, &addrlen);
		VerifyOrExit(len > 0);
		otIp4ToIp4MappedIp6Address((otIp4Address *)&addr_v4.sin_addr.s_addr,
					   &req->addr_info.mAddress);
		req->addr_info.mPort = net_ntohs(addr_v4.sin_port);
	} else {
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

		addrlen = sizeof(addr_v6);
		len = zsock_recvfrom(mdns_sock_v6, req->buffer, sizeof(req->buffer), 0,
				     (struct net_sockaddr *)&addr_v6, &addrlen);
		VerifyOrExit(len > 0);
		memcpy(&req->addr_info.mAddress, &addr_v6.sin6_addr,
		       sizeof(req->addr_info.mAddress));
		req->addr_info.mPort = net_ntohs(addr_v6.sin6_port);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
	}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */
	req->length = (uint16_t)len;
	req->addr_info.mInfraIfIndex = ail_iface_index;
	req->cb = process_mdns_message;

	openthread_border_router_post_message(req);

exit:
	return;
}

static void process_mdns_message(struct otbr_msg_ctx *msg_ctx_ptr)
{
	otMessageSettings ot_message_settings = {true, OT_MESSAGE_PRIORITY_NORMAL};
	otMessage *ot_message = NULL;
	otBufferInfo buffer_info;
	uint16_t req_buff_num;

	/** In large networks with high traffic, we have observed that mDNS module
	 * might jump to assert when trying to allocate OT message buffers for a new
	 * query/response that has to be sent.
	 * Here, we calculate the approximate number of OT message buffers that will be required
	 * to hold the incoming mDNS packet. If the number of free OT message buffers will drop
	 * below the imposed limit after the conversion has been perfomed, the incoming packet
	 * will be silently dropped.
	 * A possible scenario would be when multipackets (TC bit set) are received from multiple
	 * hosts, as mDNS module stores the incoming messages for a period of time.
	 * This mechanism tries to make sure that there are enough free buffers for mDNS module
	 * to perform it's execution.
	 */

	req_buff_num = (msg_ctx_ptr->length /
			(CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE - sizeof(otMessageBuffer))) + 1;
	otMessageGetBufferInfo(ot_instance_ptr, &buffer_info);

	VerifyOrExit((buffer_info.mFreeBuffers - req_buff_num) >=
		     ((PKT_THRESHOLD_VAL * CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS) / 100));

	ot_message = otIp6NewMessage(ot_instance_ptr, &ot_message_settings);
	VerifyOrExit(ot_message);
	VerifyOrExit(otMessageAppend(ot_message, msg_ctx_ptr->buffer,
				     msg_ctx_ptr->length) == OT_ERROR_NONE);
	otPlatMdnsHandleReceive(ot_instance_ptr, ot_message, /* aIsUnicast */ false,
				&msg_ctx_ptr->addr_info);
	ot_message = NULL;
exit:
	if (ot_message != NULL) {
		otMessageFree(ot_message);
	}
}

void mdns_plat_monitor_interface(struct net_if *ail_iface)
{
	struct net_if_ipv6 *ipv6 = NULL;
	otIp6Address ip6_addr = {0};
	struct net_if_addr *unicast = NULL;

	net_if_lock(ail_iface);

	otPlatMdnsHandleHostAddressRemoveAll(ot_instance_ptr, ail_iface_index);

	ipv6 = ail_iface->config.ip.ipv6;
	ARRAY_FOR_EACH(ipv6->unicast, idx) {
		unicast = &ipv6->unicast[idx];

		if (!unicast->is_used) {
			continue;
		}
		memcpy(&ip6_addr.mFields.m32, &unicast->address.in6_addr.s6_addr32,
		       sizeof(otIp6Address));
		otPlatMdnsHandleHostAddressEvent(ot_instance_ptr, &ip6_addr, true,
						 ail_iface_index);
	}
	if (IS_ENABLED(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4) &&
	    IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6) &&
	    openthread_border_router_has_ipv4_connectivity()) {
		struct net_if_ipv4 *ipv4 = NULL;
		otIp4Address ip4_addr = {0};

		ipv4 = ail_iface->config.ip.ipv4;
		ARRAY_FOR_EACH(ipv4->unicast, idx) {
			unicast = &ipv4->unicast[idx].ipv4;

			if (!unicast->is_used) {
				continue;
			}
			memcpy(&ip4_addr.mFields.m32, &unicast->address.in_addr.s4_addr32,
			       sizeof(otIp4Address));
			otIp4ToIp4MappedIp6Address(&ip4_addr, &ip6_addr);
			otPlatMdnsHandleHostAddressEvent(ot_instance_ptr, &ip6_addr, true,
							 ail_iface_index);
		}
	}

	net_if_unlock(ail_iface);
}
