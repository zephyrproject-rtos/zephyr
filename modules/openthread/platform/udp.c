#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_context.h>

#include "openthread/ip6.h"
#include "openthread/udp.h"
#include "openthread/platform/udp.h"

static void udp_plat_receive_callback(struct net_context *context, struct net_pkt *pkt,
				      union net_ip_header *ip_hdr,
				      union net_proto_header *proto_hdr, int status,
				      void *user_data);

otError otPlatUdpSocket(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;
	int ret;
	struct net_context *ctx = NULL;

	if (aUdpSocket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &ctx);
	// VerifyOrExit(ret == 0, error = OT_ERROR_FAILED);

	net_context_recv(ctx, udp_plat_receive_callback, K_NO_WAIT, aUdpSocket);

	aUdpSocket->mHandle = ctx;

exit:

	return error;
}

otError otPlatUdpClose(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;

	struct net_context *ctx = (struct net_context *)aUdpSocket->mHandle;
	if (ctx == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	net_context_put(ctx);

	aUdpSocket->mHandle = NULL;

exit:
	return error;
}

otError otPlatUdpBind(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;
	int ret;
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = aUdpSocket->mSockName.mPort,
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};

	if (aUdpSocket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	struct net_context *ctx = (struct net_context *)aUdpSocket->mHandle;
	// VerifyOrExit(ctx != NULL, error = OT_ERROR_INVALID_ARGS);

	memcpy(&addr.sin6_addr, aUdpSocket->mSockName.mAddress.mFields.m8, sizeof(struct in6_addr));
	ret = net_context_bind(ctx, (struct sockaddr *)&addr, sizeof(struct sockaddr_in6));

	// VerifyOrExit(ret == 0, error = OT_ERROR_FAILED);

exit:
	return error;
}

otError otPlatUdpBindToNetif(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier)
{
	otError error = OT_ERROR_NONE;
	struct net_if *iface = NULL;

	if (aUdpSocket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}
	struct net_context *ctx = (struct net_context *)aUdpSocket->mHandle;
	// VerifyOrExit(ctx != NULL, error = OT_ERROR_INVALID_ARGS);

	switch (aNetifIdentifier) {
	case OT_NETIF_THREAD_HOST:
		iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
		break;

	case OT_NETIF_BACKBONE:
		iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
		break;
	case OT_NETIF_UNSPECIFIED:
	case OT_NETIF_THREAD_INTERNAL:
	default:
		break;
	}

	if (iface != NULL) {
		net_context_bind_iface(ctx, iface);
	}

exit:
	return error;
}

otError otPlatUdpConnect(otUdpSocket *aUdpSocket)
{
	otError error = OT_ERROR_NONE;
	int ret;
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(aUdpSocket->mPeerName.mPort),
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};

	if (aUdpSocket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	struct net_context *ctx = (struct net_context *)aUdpSocket->mHandle;
	// VerifyOrExit(ctx != NULL, error = OT_ERROR_INVALID_ARGS);

	if ((aUdpSocket->mPeerName.mPort != 0) &&
	    !otIp6IsAddressUnspecified(&aUdpSocket->mPeerName.mAddress)) {
		memcpy(&addr.sin6_addr, aUdpSocket->mPeerName.mAddress.mFields.m8,
		       sizeof(struct in6_addr));
		ret = net_context_connect(ctx, (struct sockaddr *)&addr,
					  sizeof(struct sockaddr_in6), NULL, K_NO_WAIT, NULL);
		// VerifyOrExit(ret == 0, error = OT_ERROR_FAILED);
	} else {
		; // tdb how to implement this
	}

exit:
	return error;
}

otError otPlatUdpSend(otUdpSocket *aUdpSocket, otMessage *aMessage,
		      const otMessageInfo *aMessageInfo)
{
	otError error = OT_ERROR_NONE;

	if (aUdpSocket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	struct net_context *ctx = (struct net_context *)aUdpSocket->mHandle;
	// VerifyOrExit(ctx != NULL, error = OT_ERROR_INVALID_ARGS);

	struct sockaddr_in6 dst_addr = {.sin6_family = AF_INET6,
					.sin6_port = htons(aMessageInfo->mPeerPort)};
	memcpy(&dst_addr.sin6_addr, aMessageInfo->mPeerAddr.mFields.m8, sizeof(struct in6_addr));
	uint8_t buffer[512];
	otMessageRead(aMessage, 0, buffer, otMessageGetLength(aMessage));

	net_context_sendto(ctx, buffer, otMessageGetLength(aMessage), (struct sockaddr *)&dst_addr,
			   sizeof(struct sockaddr_in6), NULL, K_NO_WAIT, NULL);

exit:
	otMessageFree(aMessage);
	return error;
}

otError otPlatUdpJoinMulticastGroup(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier,
				    const otIp6Address *aAddress)
{
	otError error = OT_ERROR_NONE;
	if (aUdpSocket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	struct net_context *ctx = (struct net_context *)aUdpSocket->mHandle;
	if (ctx == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	switch (aNetifIdentifier) {
	case OT_NETIF_UNSPECIFIED:
		break;
	case OT_NETIF_THREAD_HOST:
		break;
	case OT_NETIF_THREAD_INTERNAL:
		assert(false);
	case OT_NETIF_BACKBONE:
		break;
	}

exit:
	return error;
}

otError otPlatUdpLeaveMulticastGroup(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier,
				     const otIp6Address *aAddress)
{
	OT_UNUSED_VARIABLE(aUdpSocket);
	OT_UNUSED_VARIABLE(aNetifIdentifier);
	OT_UNUSED_VARIABLE(aAddress);

	return OT_ERROR_NOT_IMPLEMENTED;
}

static void udp_plat_receive_callback(struct net_context *context, struct net_pkt *pkt,
				      union net_ip_header *ip_hdr,
				      union net_proto_header *proto_hdr, int status,
				      void *user_data)
{
	OT_UNUSED_VARIABLE(context);
	OT_UNUSED_VARIABLE(pkt);
	OT_UNUSED_VARIABLE(ip_hdr);
	OT_UNUSED_VARIABLE(proto_hdr);
	OT_UNUSED_VARIABLE(status);
	OT_UNUSED_VARIABLE(user_data);
}
