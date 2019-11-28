/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_l2_openthread, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_mgmt.h>
#include <net/openthread.h>

#include <net_private.h>

#include <init.h>
#include <sys/util.h>
#include <sys/__assert.h>

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/dataset.h>
#include <openthread/joiner.h>
#include <openthread-system.h>
#include <openthread-config-generic.h>

#include <platform-zephyr.h>

#include "openthread_utils.h"

#define OT_STACK_SIZE (CONFIG_OPENTHREAD_THREAD_STACK_SIZE)
#define OT_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)

#define OT_NETWORK_NAME CONFIG_OPENTHREAD_NETWORK_NAME
#define OT_CHANNEL CONFIG_OPENTHREAD_CHANNEL
#define OT_PANID CONFIG_OPENTHREAD_PANID
#define OT_XPANID CONFIG_OPENTHREAD_XPANID

#if defined(CONFIG_OPENTHREAD_JOINER_PSKD)
#define OT_JOINER_PSKD CONFIG_OPENTHREAD_JOINER_PSKD
#else
#define OT_JOINER_PSKD ""
#endif

#if defined(CONFIG_OPENTHREAD_PLATFORM_INFO)
#define OT_PLATFORM_INFO CONFIG_OPENTHREAD_PLATFORM_INFO
#else
#define OT_PLATFORM_INFO ""
#endif

extern void platformShellInit(otInstance *aInstance);

K_SEM_DEFINE(ot_sem, 0, 1);

K_THREAD_STACK_DEFINE(ot_stack_area, OT_STACK_SIZE);
static struct k_thread ot_thread_data;
static k_tid_t ot_tid;
static struct net_linkaddr *ll_addr;

static struct net_mgmt_event_callback ip6_addr_cb;

k_tid_t openthread_thread_id_get(void)
{
	return ot_tid;
}

static void ipv6_addr_event_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(OPENTHREAD)) {
		return;
	}

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

void otSysEventSignalPending(void)
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

	if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED) {
		NET_DBG("Ipv6 multicast address removed");
		rm_ipv6_maddr_from_zephyr(ot_context);
	}

	if (flags & OT_CHANGED_IP6_MULTICAST_SUBSCRIBED) {
		NET_DBG("Ipv6 multicast address added");
		add_ipv6_maddr_to_zephyr(ot_context);
	}
}

void ot_receive_handler(otMessage *aMessage, void *context)
{
	struct openthread_context *ot_context = context;

	u16_t offset = 0U;
	u16_t read_len;
	struct net_pkt *pkt;
	struct net_buf *pkt_buf;

	pkt = net_pkt_rx_alloc_with_buffer(ot_context->iface,
					   otMessageGetLength(aMessage),
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("Failed to reserve net pkt");
		goto out;
	}

	pkt_buf = pkt->buffer;

	while (1) {
		read_len = otMessageRead(aMessage,
					 offset,
					 pkt_buf->data,
					 net_buf_tailroom(pkt_buf));
		if (!read_len) {
			break;
		}

		net_buf_add(pkt_buf, read_len);

		offset += read_len;

		if (!net_buf_tailroom(pkt_buf)) {
			pkt_buf = pkt_buf->frags;
			if (!pkt_buf) {
				break;
			}
		}
	}

	NET_DBG("Injecting Ip6 packet to Zephyr net stack");

#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)
	net_pkt_hexdump(pkt, "Received IPv6 packet");
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

void ot_joiner_start_handler(otError error, void *context)
{
	struct openthread_context *ot_context = context;

	switch (error) {
	case OT_ERROR_NONE:
		NET_INFO("Join success");
		otThreadSetEnabled(ot_context->instance, true);
		break;
	default:
		NET_ERR("Join failed [%d]", error);
		break;
	}
}

static void openthread_process(void *context, void *arg2, void *arg3)
{
	struct openthread_context *ot_context = context;

	while (1) {
		while (otTaskletsArePending(ot_context->instance)) {
			otTaskletsProcess(ot_context->instance);
		}

		otSysProcessDrivers(ot_context->instance);

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
		net_pkt_hexdump(pkt, "Injected IPv6 packet");
#endif
		return NET_CONTINUE;
	}

	NET_DBG("Got 802.15.4 packet, sending to OT");

	otRadioFrame recv_frame;

	recv_frame.mPsdu = net_buf_frag_last(pkt->buffer)->data;
	/* Length inc. CRC. */
	recv_frame.mLength = net_buf_frags_len(pkt->buffer);
	recv_frame.mChannel = platformRadioChannelGet(ot_context->instance);
	recv_frame.mInfo.mRxInfo.mLqi = net_pkt_ieee802154_lqi(pkt);
	recv_frame.mInfo.mRxInfo.mRssi = net_pkt_ieee802154_rssi(pkt);

#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4)
	net_pkt_hexdump(pkt, "Received 802.15.4 frame");
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

int openthread_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);
	int len = net_pkt_get_len(pkt);
	struct net_buf *buf;
	otMessage *message;
	otMessageSettings settings;

	NET_DBG("Sending Ip6 packet to ot stack");

	settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;
	settings.mLinkSecurityEnabled = true;
	message = otIp6NewMessage(ot_context->instance, &settings);
	if (message == NULL) {
		goto exit;
	}

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		if (otMessageAppend(message, buf->data,
				    buf->len) != OT_ERROR_NONE) {

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
	net_pkt_hexdump(pkt, "Sent IPv6 packet");
#endif

exit:
	net_pkt_unref(pkt);

	return len;
}

enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
					     struct net_buf *buf)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(buf);
	NET_DBG("");

	return NET_CONTINUE;
}

static void openthread_start(struct openthread_context *ot_context)
{
	otInstance *ot_instance = ot_context->instance;
	otError error;

	if (otDatasetIsCommissioned(ot_instance)) {
		/* OpenThread already has dataset stored - skip the
		 * configuration.
		 */
		NET_DBG("OpenThread already commissioned.");
	} else if (IS_ENABLED(CONFIG_OPENTHREAD_JOINER_AUTOSTART)) {
		/* No dataset - initiate network join procedure. */
		NET_DBG("Starting OpenThread join procedure.");

		error = otJoinerStart(ot_instance, OT_JOINER_PSKD, NULL,
				      PACKAGE_NAME, OT_PLATFORM_INFO,
				      PACKAGE_VERSION, NULL,
				      &ot_joiner_start_handler, ot_context);

		if (error != OT_ERROR_NONE) {
			NET_ERR("Failed to start joiner [%d]", error);
		}

		return;
	} else {
		/* No dataset - load the default configuration. */
		NET_DBG("Loading OpenThread default configuration.");

		otExtendedPanId xpanid;

		otThreadSetNetworkName(ot_instance, OT_NETWORK_NAME);
		otLinkSetChannel(ot_instance, OT_CHANNEL);
		otLinkSetPanId(ot_instance, OT_PANID);
		net_bytes_from_str(xpanid.m8, 8, (char *)OT_XPANID);
		otThreadSetExtendedPanId(ot_instance, &xpanid);
	}

	NET_INFO("OpenThread version: %s", otGetVersionString());
	NET_INFO("Network name: %s",
		 log_strdup(otThreadGetNetworkName(ot_instance)));

	/* Start the network. */
	error = otThreadSetEnabled(ot_instance, true);
	if (error != OT_ERROR_NONE) {
		NET_ERR("Failed to start the OpenThread network [%d]", error);
	}
}

static int openthread_init(struct net_if *iface)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	NET_DBG("openthread_init");

	otSysInit(0, NULL);

	ot_context->instance = otInstanceInitSingle();
	ot_context->iface = iface;

	__ASSERT(ot_context->instance, "OT instance is NULL");

#if defined(CONFIG_OPENTHREAD_SHELL)
	platformShellInit(ot_context->instance);
#endif


	otIp6SetEnabled(ot_context->instance, true);

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
	k_thread_name_set(&ot_thread_data, "openthread");

	openthread_start(ot_context);

	return 0;
}

void ieee802154_init(struct net_if *iface)
{
	openthread_init(iface);
}

static enum net_l2_flags openthread_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST;
}

NET_L2_INIT(OPENTHREAD_L2, openthread_recv, openthread_send,
	    NULL, openthread_flags);
