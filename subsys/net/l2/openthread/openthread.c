/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_SYS_LOG_LEVEL CONFIG_OPENTHREAD_L2_LOG_LEVEL

#if defined(CONFIG_OPENTHREAD_L2_DEBUG)
#define NET_DOMAIN "net/openthread_l2"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_mgmt.h>
#include <net/openthread.h>

#include <net_private.h>

#include <init.h>
#include <misc/util.h>
#include <misc/__assert.h>
#include <openthread/openthread.h>
#include <openthread/cli.h>
#include <platform.h>
#include <platform-zephyr.h>

#include "openthread_utils.h"

#define OT_STACK_SIZE (CONFIG_OPENTHREAD_THREAD_STACK_SIZE)
#define OT_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)

extern void platformShellInit(otInstance *aInstance);

K_SEM_DEFINE(ot_sem, 0, 1);

K_THREAD_STACK_DEFINE(ot_stack_area, OT_STACK_SIZE);
static struct k_thread ot_thread_data;
static k_tid_t ot_tid;
static struct net_linkaddr *ll_addr;

static struct net_mgmt_event_callback ip6_addr_cb;

static void ipv6_addr_event_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		add_ipv6_addr_to_ot(ot_context);
	} else if (mgmt_event == NET_EVENT_IPV6_MADDR_ADD) {
		add_ipv6_maddr_to_ot(ot_context);
	}
}

void otPlatRadioGetIeeeEui64(otInstance *instance, uint8_t *ieee_eui64)
{
	ARG_UNUSED(instance);

	memcpy(ieee_eui64, ll_addr->addr, ll_addr->len);
}

void otTaskletsSignalPending(otInstance *instance)
{
	k_sem_give(&ot_sem);
}

void PlatformEventSignalPending(void)
{
	k_sem_give(&ot_sem);
}

void ot_state_changed_handler(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;

	NET_INFO("State changed! Flags: 0x%08" PRIx32 " Current role: %d",
		    flags, otThreadGetDeviceRole(ot_context->instance));

	if (flags & OT_CHANGED_IP6_ADDRESS_REMOVED) {
		NET_DBG("Ipv6 address removed");
		rm_ipv6_addr_from_zephyr(ot_context);
	}

	if (flags & OT_CHANGED_IP6_ADDRESS_ADDED) {
		NET_DBG("Ipv6 address added");
		add_ipv6_addr_to_zephyr(ot_context);
	}

	if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED) {
		NET_DBG("Ipv6 multicast address removed");
		rm_ipv6_maddr_from_zephyr(ot_context);
	}

	if (flags & OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED) {
		NET_DBG("Ipv6 multicast address added");
		add_ipv6_maddr_to_zephyr(ot_context);
	}
}

void ot_receive_handler(otMessage *aMessage, void *context)
{
	struct openthread_context *ot_context = context;

	u16_t offset = 0;
	u16_t read_len;
	struct net_pkt *pkt;
	struct net_buf *prev_buf = NULL;

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("Failed to reserve net pkt");
		goto out;
	}

	while (1) {
		struct net_buf *pkt_buf;

		pkt_buf = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!pkt_buf) {
			NET_ERR("Failed to get fragment buf");
			net_pkt_unref(pkt);
			goto out;
		}

		read_len = otMessageRead(aMessage,
					 offset,
					 pkt_buf->data,
					 net_buf_tailroom(pkt_buf));

		if (!read_len) {
			net_buf_unref(pkt_buf);
			break;
		}

		net_buf_add(pkt_buf, read_len);

		if (!prev_buf) {
			net_pkt_frag_insert(pkt, pkt_buf);
		} else {
			net_buf_frag_insert(prev_buf, pkt_buf);
		}

		prev_buf = pkt_buf;

		offset += read_len;
	}

	NET_DBG("Injecting Ip6 packet to Zephyr net stack");

#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)
	net_hexdump_frags("Received IPv6 packet", pkt, true);
#endif

	if (!pkt_list_is_full(ot_context)) {
		if (net_recv_data(ot_context->iface, pkt) < 0) {
			NET_ERR("net_recv_data failed");
			goto out;
		}

		pkt_list_add(ot_context, pkt);
		pkt = NULL;
	} else {
		NET_INFO("Pacet list is full");
	}
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	otMessageFree(aMessage);
}

static void openthread_process(void *context, void *arg2, void *arg3)
{
	struct openthread_context *ot_context = context;

	while (1) {
		while (otTaskletsArePending(ot_context->instance)) {
			otTaskletsProcess(ot_context->instance);
		}

		PlatformProcessDrivers(ot_context->instance);

		k_sem_take(&ot_sem, K_FOREVER);
	}
}

static enum net_verdict openthread_recv(struct net_if *iface,
					struct net_pkt *pkt)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (pkt_list_peek(ot_context) == pkt) {
		pkt_list_remove_last(ot_context);
		NET_DBG("Got injected Ip6 packet, "
			    "sending to upper layers");
#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)
		net_hexdump_frags("Injected IPv6 packet", pkt, true);
#endif
		return NET_CONTINUE;
	}

	NET_DBG("Got 802.15.4 packet, sending to OT");

	otRadioFrame recv_frame;

	recv_frame.mPsdu = net_buf_frag_last(pkt->frags)->data;
	/* Length inc. CRC. */
	recv_frame.mLength = net_buf_frags_len(pkt->frags);
	recv_frame.mChannel = platformRadioChannelGet(ot_context->instance);
	recv_frame.mLqi = net_pkt_ieee802154_lqi(pkt);
	recv_frame.mRssi = net_pkt_ieee802154_rssi(pkt);

#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4)
	net_hexdump_frags("Received 802.15.4 frame", pkt, true);
#endif

#if OPENTHREAD_ENABLE_DIAG
	if (otPlatDiagModeGet()) {
		otPlatDiagRadioReceiveDone(ot_context->instance,
					   &recv_frame, OT_ERROR_NONE);
	} else
#endif
	{
		otPlatRadioReceiveDone(ot_context->instance,
				       &recv_frame, OT_ERROR_NONE);
	}

	net_pkt_unref(pkt);

	return NET_OK;
}

enum net_verdict openthread_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);
	enum net_verdict ret = NET_OK;
	otMessage *message;
	struct net_buf *frag;

	NET_DBG("Sending Ip6 packet to ot stack");

	message = otIp6NewMessage(ot_context->instance, true);
	if (message == NULL) {
		goto exit;
	}

	for (frag = pkt->frags; frag; frag = frag->frags) {
		if (otMessageAppend(message, frag->data,
				    frag->len) != OT_ERROR_NONE) {

			NET_ERR("Error while appending to otMessage");
			otMessageFree(message);
			goto exit;
		}
	}

	if (otIp6Send(ot_context->instance, message) != OT_ERROR_NONE) {
		NET_ERR("Error while calling otIp6Send");
		goto exit;
	}

#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)
	net_hexdump_frags("Sent IPv6 packet", pkt, true);
#endif

exit:
	net_pkt_unref(pkt);

	return ret;
}

static u16_t openthread_reserve(struct net_if *iface, void *arg)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(arg);

	return 0;
}

enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
					     struct net_buf *buf)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(buf);
	NET_DBG("");

	return NET_CONTINUE;
}

static int openthread_init(struct net_if *iface)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);
	u8_t xpanid[8];

	net_bytes_from_str(xpanid, 8, (char *)CONFIG_OPENTHREAD_XPANID);

	NET_DBG("openthread_init");

	PlatformInit(0, NULL);

	ot_context->instance = otInstanceInitSingle();
	ot_context->iface = iface;

	__ASSERT(ot_context->instance, "OT instance is NULL");

#if defined(CONFIG_OPENTHREAD_SHELL)
	platformShellInit(ot_context->instance);
#endif

	NET_INFO("OpenThread version: %s",
		    otGetVersionString());

	otThreadSetNetworkName(ot_context->instance, CONFIG_OPENTHREAD_NETWORK_NAME);
	NET_INFO("Network name:   %s",
		    otThreadGetNetworkName(ot_context->instance));

	otLinkSetChannel(ot_context->instance, CONFIG_OPENTHREAD_CHANNEL);
	otLinkSetPanId(ot_context->instance, CONFIG_OPENTHREAD_PANID);
	otThreadSetExtendedPanId(ot_context->instance, xpanid);
	otIp6SetEnabled(ot_context->instance, true);
	otThreadSetEnabled(ot_context->instance, true);
	otIp6SetReceiveFilterEnabled(ot_context->instance, true);
	otIp6SetReceiveCallback(ot_context->instance,
				ot_receive_handler, ot_context);
	otSetStateChangedCallback(ot_context->instance,
				  &ot_state_changed_handler, ot_context);

	ll_addr = net_if_get_link_addr(iface);

	net_mgmt_init_event_callback(&ip6_addr_cb, ipv6_addr_event_handler,
				     NET_EVENT_IPV6_ADDR_ADD |
				     NET_EVENT_IPV6_MADDR_ADD);
	net_mgmt_add_event_callback(&ip6_addr_cb);

	ot_tid = k_thread_create(&ot_thread_data, ot_stack_area,
				 K_THREAD_STACK_SIZEOF(ot_stack_area),
				 openthread_process,
				 ot_context, NULL, NULL,
				 OT_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

void ieee802154_init(struct net_if *iface)
{
	openthread_init(iface);
}

int ieee802154_radio_send(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	/* Shouldn't be here */
	__ASSERT(false, "OpenThread L2 should never reach here");

	return -EIO;
}

static enum net_l2_flags openthread_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST;
}

NET_L2_INIT(OPENTHREAD_L2, openthread_recv, openthread_send,
	    openthread_reserve, NULL, openthread_flags);
