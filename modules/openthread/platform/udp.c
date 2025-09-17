/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread/udp.h"
#include "openthread/ip6.h"
#include "openthread/platform/udp.h"
#include "openthread_border_router.h"
#include "sockets_internal.h"
#include <common/code_utils.hpp>
#include <errno.h>
#include <openthread.h>
#include <openthread/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/util.h>

static struct zsock_pollfd sockfd_udp[CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_UDP_SERVICES];

static struct otInstance *ot_instance_ptr;
static struct net_if *ot_iface_ptr, *ail_iface_ptr;
static uint32_t ot_iface_index, ail_iface_index;
static uint8_t sock_cnt;

static void udp_receive_handler(struct net_socket_service_event *evt);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(handle_udp_receive, udp_receive_handler,
				      CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_UDP_SERVICES);

otError udp_plat_init(otInstance *ot_instance, struct net_if *ail_iface, struct net_if *ot_iface)
{
	ot_instance_ptr = ot_instance;
	ot_iface_ptr = ot_iface;
	ail_iface_ptr = ail_iface;
	ot_iface_index = (uint32_t)net_if_get_by_iface(ot_iface);
	ail_iface_index = (uint32_t)net_if_get_by_iface(ail_iface);

	for (uint8_t i = 0; i < CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_UDP_SERVICES; i++) {
		sockfd_udp[i].fd = -1;
	}

	return OT_ERROR_NONE;
}

void udp_plat_deinit(void)
{
	ARRAY_FOR_EACH(sockfd_udp, idx) {
		if (sockfd_udp[idx].fd != -1) {
			zsock_close(sockfd_udp[idx].fd);
			sockfd_udp[idx].fd = -1;
		}
	}
}

otError otPlatUdpSocket(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;
	int sock = -1;
	uint8_t idx;

	VerifyOrExit(aUdpSocket != NULL, error = OT_ERROR_INVALID_ARGS);
	VerifyOrExit(sock_cnt < CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_UDP_SERVICES,
		     error = OT_ERROR_INVALID_STATE);

	sock = zsock_socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
	VerifyOrExit(sock >= 0, error = OT_ERROR_FAILED);

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV4_MAPPING_TO_IPV6)
	int off = 0;

	VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) == 0,
		     error = OT_ERROR_FAILED);
#endif

	aUdpSocket->mHandle = INT_TO_POINTER(sock);

	for (idx = 0; idx < CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_UDP_SERVICES; idx++) {
		if (sockfd_udp[idx].fd < 0) {
			sockfd_udp[idx].fd = sock;
			sockfd_udp[idx].events = ZSOCK_POLLIN;
			sock_cnt++;
			break;
		}
	}

	VerifyOrExit(net_socket_service_register(&handle_udp_receive, sockfd_udp,
						 ARRAY_SIZE(sockfd_udp), NULL) == 0,
						 error = OT_ERROR_FAILED);

exit:
	return error;
}

otError otPlatUdpClose(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	int sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);

	VerifyOrExit(zsock_close(sock) == 0, error = OT_ERROR_FAILED);
	aUdpSocket->mHandle = NULL;
	sock_cnt--;

	for (uint8_t i = 0; i < CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MAX_UDP_SERVICES; i++) {
		if (sockfd_udp[i].fd == sock) {
			sockfd_udp[i].fd = -1;
			break;
		}
	}

exit:
	return error;
}

otError otPlatUdpBind(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;
	int sock = -1;
	struct sockaddr_in6 addr = {0};
	int on = 1;

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);
	VerifyOrExit(sock >= 0, error = OT_ERROR_INVALID_ARGS);
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(aUdpSocket->mSockName.mPort);
	memcpy(&addr.sin6_addr, &aUdpSocket->mSockName.mAddress, sizeof(otIp6Address));

	VerifyOrExit(zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}

otError otPlatUdpBindToNetif(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier)
{
	otError error = OT_ERROR_NONE;
	int sock = -1;
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};
	struct ifreq if_req = {0};

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);
	VerifyOrExit(sock >= 0, error = OT_ERROR_INVALID_ARGS);

	switch (aNetifIdentifier) {
	case OT_NETIF_UNSPECIFIED:
		VerifyOrExit(zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, NULL, 0) == 0,
			     error = OT_ERROR_FAILED);
		break;
	case OT_NETIF_THREAD_HOST:
		VerifyOrExit(net_if_get_name(ot_iface_ptr, name, CONFIG_NET_INTERFACE_NAME_LEN) > 0,
			     error = OT_ERROR_FAILED);
		memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
		VerifyOrExit(zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &if_req,
					      sizeof(if_req)) == 0,
			     error = OT_ERROR_FAILED);
		break;
	case OT_NETIF_THREAD_INTERNAL:
		assert(false);
	case OT_NETIF_BACKBONE:
		VerifyOrExit(net_if_get_name(ail_iface_ptr, name,
					     CONFIG_NET_INTERFACE_NAME_LEN) > 0,
			     error = OT_ERROR_FAILED);
		memcpy(if_req.ifr_name, name, MIN(sizeof(name) - 1, sizeof(if_req.ifr_name) - 1));
		VerifyOrExit(zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &if_req,
					      sizeof(if_req)) == 0,
			     error = OT_ERROR_FAILED);
		break;
	default:
		break;
	}
exit:
	return error;
}

otError otPlatUdpConnect(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;
	int sock = -1;
	struct sockaddr_in6 addr = {0};

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);

	if (aUdpSocket->mPeerName.mPort != 0 &&
	    !otIp6IsAddressUnspecified(&aUdpSocket->mPeerName.mAddress)) {
		addr.sin6_family = AF_INET6;
		memcpy(&addr.sin6_addr, &aUdpSocket->mPeerName.mAddress, sizeof(otIp6Address));
		addr.sin6_port = htons(aUdpSocket->mPeerName.mPort);

		VerifyOrExit(zsock_connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0,
			     error = OT_ERROR_FAILED);
	}

exit:
	return error;
}

otError otPlatUdpSend(otUdpSocket *aUdpSocket,
		      otMessage *aMessage,
		      const otMessageInfo *aMessageInfo)
{
	otError error = OT_ERROR_NONE;
	struct otbr_msg_ctx *req = NULL;
	uint8_t control_buf[CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int))] = {0};
	uint16_t len = otMessageGetLength(aMessage);
	size_t control_len = 0;
	struct sockaddr_in6 peer = {0};
	int sock = -1;
	struct iovec iov = {0};
	struct msghdr msg_hdr = {0};
	struct cmsghdr *cmsg_hdr = NULL;
	int hop_limit = 0;
	int on = 1, off = 0;
	int optval;
	socklen_t optlen = sizeof(optval);

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);
	VerifyOrExit(sock >= 0, error = OT_ERROR_INVALID_ARGS);

	VerifyOrExit(len <= OTBR_MESSAGE_SIZE, error = OT_ERROR_FAILED);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE);
	VerifyOrExit(otMessageRead(aMessage, 0, req->buffer, len) == len, error = OT_ERROR_FAILED);

	iov.iov_base = req->buffer;
	iov.iov_len = len;

	peer.sin6_family = AF_INET6;
	peer.sin6_port = htons(aMessageInfo->mPeerPort);
	memcpy(&peer.sin6_addr, &aMessageInfo->mPeerAddr, sizeof(otIp6Address));

	if (((aMessageInfo->mPeerAddr.mFields.m8[0] == 0xfe) &&
	    ((aMessageInfo->mPeerAddr.mFields.m8[1] & 0xc0) == 0x80)) &&
	    !aMessageInfo->mIsHostInterface) {
		peer.sin6_scope_id = ail_iface_index;
	}

	msg_hdr.msg_name = &peer;
	msg_hdr.msg_namelen = sizeof(peer);
	msg_hdr.msg_iov = &iov;
	msg_hdr.msg_iovlen = 1;
	msg_hdr.msg_control = control_buf;
	msg_hdr.msg_controllen = sizeof(control_buf);

	msg_hdr.msg_flags = 0;

	cmsg_hdr = CMSG_FIRSTHDR(&msg_hdr);
	cmsg_hdr->cmsg_level = IPPROTO_IPV6;
	cmsg_hdr->cmsg_type = IPV6_HOPLIMIT;
	cmsg_hdr->cmsg_len = CMSG_LEN(sizeof(int));

	hop_limit = (aMessageInfo->mHopLimit ? aMessageInfo->mHopLimit : 255);
	memcpy(CMSG_DATA(cmsg_hdr), &hop_limit, sizeof(int));
	control_len += CMSG_SPACE(sizeof(int));

	if (!otIp6IsAddressUnspecified(&aMessageInfo->mSockAddr)) {
		struct in6_pktinfo pktinfo;

		cmsg_hdr = CMSG_NXTHDR(&msg_hdr, cmsg_hdr);
		cmsg_hdr->cmsg_level = IPPROTO_IPV6;
		cmsg_hdr->cmsg_type = IPV6_PKTINFO;
		cmsg_hdr->cmsg_len = CMSG_LEN(sizeof(pktinfo));

		pktinfo.ipi6_ifindex = aMessageInfo->mIsHostInterface ? 0 : ail_iface_index;

		memcpy(&pktinfo.ipi6_addr, &aMessageInfo->mSockAddr, sizeof(otIp6Address));
		memcpy(CMSG_DATA(cmsg_hdr), &pktinfo, sizeof(pktinfo));

		control_len += CMSG_SPACE(sizeof(pktinfo));
	}

	msg_hdr.msg_controllen = control_len;

	if (aMessageInfo->mMulticastLoop) {
		VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &on,
					      sizeof(on)) == 0,
			     error = OT_ERROR_FAILED);
	}

	VerifyOrExit(zsock_sendmsg(sock, &msg_hdr, 0) > 0,
		     error = OT_ERROR_FAILED);
	otMessageFree(aMessage);
	aMessage = NULL;

	VerifyOrExit(zsock_getsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optval,
				      &optlen) == 0,
		     error = OT_ERROR_FAILED);

	if (optval) {
		VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &off,
					      sizeof(off)) == 0,
			     error = OT_ERROR_FAILED);
	}

exit:
	if (error == OT_ERROR_NONE && aMessage) {
		otMessageFree(aMessage);
	}
	openthread_border_router_deallocate_message((void *)req);
	return error;
}

otError otPlatUdpJoinMulticastGroup(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier,
				    const otIp6Address *aAddress)
{
	otError error = OT_ERROR_NONE;
	struct ipv6_mreq mreq = {0};
	int sock;

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);
	memcpy(&mreq.ipv6mr_multiaddr, aAddress, sizeof(otIp6Address));

	switch (aNetifIdentifier) {
	case OT_NETIF_UNSPECIFIED:
		break;
	case OT_NETIF_THREAD_HOST:
		mreq.ipv6mr_ifindex = ot_iface_index;
		break;
	case OT_NETIF_THREAD_INTERNAL:
		assert(false);
	case OT_NETIF_BACKBONE:
		mreq.ipv6mr_ifindex = ail_iface_index;
		break;
	default:
		break;
	}

	VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq,
				      sizeof(mreq)) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}

otError otPlatUdpLeaveMulticastGroup(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier,
				     const otIp6Address *aAddress)
{
	otError error = OT_ERROR_NONE;
	struct ipv6_mreq mreq = {0};
	int sock;

	VerifyOrExit(aUdpSocket != NULL && aUdpSocket->mHandle != NULL,
		     error = OT_ERROR_INVALID_ARGS);

	sock = (int)POINTER_TO_INT(aUdpSocket->mHandle);
	memcpy(&mreq.ipv6mr_multiaddr, aAddress, sizeof(otIp6Address));

	switch (aNetifIdentifier) {
	case OT_NETIF_UNSPECIFIED:
		break;
	case OT_NETIF_THREAD_HOST:
		mreq.ipv6mr_ifindex = ot_iface_index;
		break;
	case OT_NETIF_THREAD_INTERNAL:
		assert(false);
	case OT_NETIF_BACKBONE:
		mreq.ipv6mr_ifindex = ail_iface_index;
		break;
	default:
		break;
	}

	VerifyOrExit(zsock_setsockopt(sock, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq,
				      sizeof(mreq)) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}

static void udp_receive_handler(struct net_socket_service_event *evt)
{
	uint8_t control[CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct in6_pktinfo))] = {0};
	struct msghdr msg = {0};
	struct iovec iov = {0};
	struct cmsghdr *cmsg = NULL;
	struct sockaddr_in6 peer_addr = {0};
	struct otbr_msg_ctx *req = NULL;
	ssize_t rval = 0;

	VerifyOrExit(evt->event.revents & ZSOCK_POLLIN);
	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE);

	openthread_mutex_lock();
	for (otUdpSocket *ot_socket = otUdpGetSockets(ot_instance_ptr); ot_socket != NULL;
	     ot_socket = ot_socket->mNext) {
		if ((int)POINTER_TO_INT(ot_socket->mHandle) == evt->event.fd) {
			req->socket = ot_socket;
			break;
		}
	}
	openthread_mutex_unlock();

	VerifyOrExit(req->socket >= 0, openthread_border_router_deallocate_message((void *)req));

	iov.iov_base = req->buffer;
	iov.iov_len = sizeof(req->buffer);

	msg.msg_name = &peer_addr;
	msg.msg_namelen = sizeof(peer_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	msg.msg_flags = 0;

	rval = zsock_recvmsg(evt->event.fd, &msg, 0);
	VerifyOrExit(rval > 0);
	req->length = (uint16_t)rval;

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IPV6) {
			if (cmsg->cmsg_type == IPV6_PKTINFO &&
			    cmsg->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo))) {
				struct in6_pktinfo pktinfo;

				memcpy(&pktinfo, CMSG_DATA(cmsg), sizeof(pktinfo));
				memcpy(&req->message_info.mSockAddr, &pktinfo.ipi6_addr,
				       sizeof(otIp6Address));
				req->message_info.mIsHostInterface =
					(pktinfo.ipi6_ifindex == (unsigned int)ail_iface_index);
			} else if (cmsg->cmsg_type == IPV6_HOPLIMIT &&
				   cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
				int hoplimit;

				memcpy(&hoplimit, CMSG_DATA(cmsg), sizeof(hoplimit));
				req->message_info.mHopLimit = (uint8_t)hoplimit;
			} else {
				continue;
			}

		}
	}

	memcpy(&req->message_info.mPeerAddr, &peer_addr.sin6_addr, sizeof(otIp6Address));
	req->message_info.mPeerPort = ntohs(peer_addr.sin6_port);
	req->message_info.mSockPort = req->socket->mSockName.mPort;

	openthread_border_router_post_message(req);

exit:
	return;
}
