#include "openthread/platform/mdns_socket.h"
#include "openthread/instance.h"
#include "mdns_socket.h"
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/net_pkt.h>

#define MULTICAST_PORT 5353

static struct net_context *mdns_ipv6_ctx = NULL;
static otInstance *ot_instance;

static otError set_listening_enable(otInstance *instance, bool enable, uint32_t infraif_index);
static otError mdns_socket_init(uint32_t infraif_index);
static void mdns_socket_receive_callback(struct net_context *context, struct net_pkt *pkt,
					 union net_ip_header *ip_hdr,
					 union net_proto_header *proto_hdr, int status,
					 void *user_data);
static void mdns_send_multicast(otMessage *message, uint32_t infraif_index);

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
	OT_UNUSED_VARIABLE(aMessage);
	OT_UNUSED_VARIABLE(aAddress);
}

static otError set_listening_enable(otInstance *instance, bool enable, uint32_t infraif_index)
{

	if (enable) {
		return mdns_socket_init(infraif_index);
	} else {
		// TODO disable
		return OT_ERROR_NONE;
	}
}

static otError mdns_socket_init(uint32_t infraif_index)
{
	otError error = OT_ERROR_NONE;
	int ret;

	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(MULTICAST_PORT),
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &mdns_ipv6_ctx);
	if (ret != 0) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	ret = net_context_bind(mdns_ipv6_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in6));
	if (ret != 0) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	net_context_set_iface(mdns_ipv6_ctx, net_if_get_by_index(infraif_index));
	struct in6_addr dst;

	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	// net_context_set_ipv6_hop_limit(mdns_ipv6_ctx, 255);

	ret = net_ipv6_mld_join(net_if_get_by_index(infraif_index), &dst);
	if (ret != 0) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	ret = net_context_recv(mdns_ipv6_ctx, mdns_socket_receive_callback, K_NO_WAIT, NULL);
	if (ret != 0) {
		error = OT_ERROR_FAILED;
		goto exit;
	}
exit:
	return error;
}

static void mdns_socket_receive_callback(struct net_context *context, struct net_pkt *pkt,
					 union net_ip_header *ip_hdr,
					 union net_proto_header *proto_hdr, int status,
					 void *user_data)
{
	otMessage *message;
	otMessageSettings settings;
	otPlatMdnsAddressInfo addr_info_v6;

	settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;
	settings.mLinkSecurityEnabled = true;

	struct net_buf *buf;

	message = otIp6NewMessage(ot_instance, &settings);
	if (message == NULL) {
		goto exit;
	}

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		if (otMessageAppend(message, buf->data, buf->len) != OT_ERROR_NONE) {
			otMessageFree(message);
			goto exit;
		}
	}

	memcpy(&addr_info_v6.mAddress, ip_hdr->ipv6->src, sizeof(addr_info_v6.mAddress));
	addr_info_v6.mPort = proto_hdr->udp->src_port;
	addr_info_v6.mInfraIfIndex =
		net_if_get_by_iface(net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET)));

	otPlatMdnsHandleReceive(ot_instance, message, /* aInUnicast */ false, &addr_info_v6);

exit:
	return;
}

static void mdns_send_multicast(otMessage *message, uint32_t infraif_index)
{
	struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
				    .sin6_port = htons(MULTICAST_PORT),
				    .sin6_addr = {{{0}}},
				    .sin6_scope_id = 0};
	net_ipv6_addr_create(&addr.sin6_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	uint16_t length = otMessageGetLength(message);
	uint8_t *buf = (uint8_t *)k_calloc(1, length);
	if (buf == NULL) {
		goto exit;
	}
	otMessageRead(message, 0, buf, length);
	net_context_sendto(mdns_ipv6_ctx, buf, length, (struct sockaddr *)&addr,
			   sizeof(struct sockaddr_in6), NULL, K_FOREVER, NULL);
	k_free(buf);
	otMessageFree(message);
exit:
	return;
}

void mdns_socket_plat_init(otInstance *instance)
{
	ot_instance = instance;
}
