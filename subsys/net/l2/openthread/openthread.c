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
#include <version.h>

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/link_raw.h>
#include <openthread/ncp.h>
#include <openthread/message.h>
#include <openthread/platform/diag.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/dataset.h>
#include <openthread/joiner.h>
#include <openthread-system.h>
#include <openthread-config-generic.h>

#include <platform-zephyr.h>

#include "openthread_utils.h"

#define OT_STACK_SIZE (CONFIG_OPENTHREAD_THREAD_STACK_SIZE)

#if defined(CONFIG_OPENTHREAD_THREAD_PREEMPTIVE)
#define OT_PRIORITY K_PRIO_PREEMPT(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#else
#define OT_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#endif

#if defined(CONFIG_OPENTHREAD_NETWORK_NAME)
#define OT_NETWORK_NAME CONFIG_OPENTHREAD_NETWORK_NAME
#else
#define OT_NETWORK_NAME ""
#endif

#if defined(CONFIG_OPENTHREAD_CHANNEL)
#define OT_CHANNEL CONFIG_OPENTHREAD_CHANNEL
#else
#define OT_CHANNEL 0
#endif

#if defined(CONFIG_OPENTHREAD_PANID)
#define OT_PANID CONFIG_OPENTHREAD_PANID
#else
#define OT_PANID 0
#endif

#if defined(CONFIG_OPENTHREAD_XPANID)
#define OT_XPANID CONFIG_OPENTHREAD_XPANID
#else
#define OT_XPANID ""
#endif

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

#if defined(CONFIG_OPENTHREAD_POLL_PERIOD)
#define OT_POLL_PERIOD CONFIG_OPENTHREAD_POLL_PERIOD
#else
#define OT_POLL_PERIOD 0
#endif

#define PACKAGE_NAME "Zephyr"
#define PACKAGE_VERSION KERNEL_VERSION_STRING

extern void platformShellInit(otInstance *aInstance);

K_SEM_DEFINE(ot_sem, 0, 1);

K_KERNEL_STACK_DEFINE(ot_stack_area, OT_STACK_SIZE);
static struct k_thread ot_thread_data;
static k_tid_t ot_tid;
static struct net_linkaddr *ll_addr;
static otStateChangedCallback state_changed_cb;

k_tid_t openthread_thread_id_get(void)
{
	return ot_tid;
}

#ifdef CONFIG_NET_MGMT_EVENT
static struct net_mgmt_event_callback ip6_addr_cb;

static void ipv6_addr_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
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
#endif

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

static void ot_state_changed_handler(uint32_t flags, void *context)
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

	if (state_changed_cb) {
		state_changed_cb(flags, context);
	}
}

static void ot_receive_handler(otMessage *aMessage, void *context)
{
	struct openthread_context *ot_context = context;

	uint16_t offset = 0U;
	uint16_t read_len;
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

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		net_pkt_hexdump(pkt, "Received IPv6 packet");
	}

	if (!pkt_list_is_full(ot_context)) {
		if (pkt_list_add(ot_context, pkt) != 0) {
			NET_ERR("pkt_list_add failed");
			goto out;
		}

		if (net_recv_data(ot_context->iface, pkt) < 0) {
			NET_ERR("net_recv_data failed");
			pkt_list_remove_first(ot_context);
			goto out;
		}

		pkt = NULL;
	} else {
		NET_INFO("Packet list is full");
	}
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	otMessageFree(aMessage);
}

static void ot_joiner_start_handler(otError error, void *context)
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
		openthread_api_mutex_lock(ot_context);

		while (otTaskletsArePending(ot_context->instance)) {
			otTaskletsProcess(ot_context->instance);
		}

		otSysProcessDrivers(ot_context->instance);

		openthread_api_mutex_unlock(ot_context);

		k_sem_take(&ot_sem, K_FOREVER);
	}
}

static enum net_verdict openthread_recv(struct net_if *iface,
					struct net_pkt *pkt)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (pkt_list_peek(ot_context) == pkt) {
		pkt_list_remove_last(ot_context);
		NET_DBG("Got injected Ip6 packet, sending to upper layers");

		if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
			net_pkt_hexdump(pkt, "Injected IPv6 packet");
		}

		return NET_CONTINUE;
	}

	NET_DBG("Got 802.15.4 packet, sending to OT");

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		net_pkt_hexdump(pkt, "Received 802.15.4 frame");
	}

	if (notify_new_rx_frame(pkt) != 0) {
		NET_ERR("Failed to queue RX packet for OpenThread");
		return NET_DROP;
	}

	return NET_OK;
}

int openthread_send(struct net_if *iface, struct net_pkt *pkt)
{
	int len = net_pkt_get_len(pkt);

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		net_pkt_hexdump(pkt, "IPv6 packet to send");
	}

	if (notify_new_tx_frame(pkt) != 0) {
		net_pkt_unref(pkt);
	}

	return len;
}

int openthread_start(struct openthread_context *ot_context)
{
	otInstance *ot_instance = ot_context->instance;
	otError error;

	openthread_api_mutex_lock(ot_context);

	/* Sleepy End Device specific configuration. */
	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		otLinkModeConfig ot_mode = otThreadGetLinkMode(ot_instance);

		/* A SED should always attach the network as a SED to indicate
		 * increased buffer requirement to a parent.
		 */
		ot_mode.mRxOnWhenIdle = false;

		otThreadSetLinkMode(ot_context->instance, ot_mode);
		otLinkSetPollPeriod(ot_context->instance, OT_POLL_PERIOD);
	}

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

		goto exit;
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

exit:
	openthread_api_mutex_unlock(ot_context);

	return error == OT_ERROR_NONE ? 0 : -EIO;
}

int openthread_stop(struct openthread_context *ot_context)
{
	otError error;

	openthread_api_mutex_lock(ot_context);

	error = otThreadSetEnabled(ot_context->instance, false);
	if (error == OT_ERROR_INVALID_STATE) {
		NET_DBG("Openthread interface was not up [%d]", error);
	}

	openthread_api_mutex_unlock(ot_context);

	return 0;
}

static int openthread_init(struct net_if *iface)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	NET_DBG("openthread_init");

	k_mutex_init(&ot_context->api_lock);

	ll_addr = net_if_get_link_addr(iface);

	otSysInit(0, NULL);

	ot_context->instance = otInstanceInitSingle();
	ot_context->iface = iface;

	__ASSERT(ot_context->instance, "OT instance is NULL");

	if (IS_ENABLED(CONFIG_OPENTHREAD_SHELL)) {
		platformShellInit(ot_context->instance);
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_NCP)) {
		otNcpInit(ot_context->instance);
	}

	if (!IS_ENABLED(CONFIG_OPENTHREAD_RAW)) {
		otIp6SetEnabled(ot_context->instance, true);
	}

	if (!IS_ENABLED(CONFIG_OPENTHREAD_NCP)) {
		otIp6SetReceiveFilterEnabled(ot_context->instance, true);
		otIp6SetReceiveCallback(ot_context->instance,
					ot_receive_handler, ot_context);
		otSetStateChangedCallback(
					ot_context->instance,
					&ot_state_changed_handler,
					ot_context);
	}

	net_mgmt_init_event_callback(&ip6_addr_cb, ipv6_addr_event_handler,
				     NET_EVENT_IPV6_ADDR_ADD |
				     NET_EVENT_IPV6_MADDR_ADD);
	net_mgmt_add_event_callback(&ip6_addr_cb);

	ot_tid = k_thread_create(&ot_thread_data, ot_stack_area,
				 K_KERNEL_STACK_SIZEOF(ot_stack_area),
				 openthread_process,
				 ot_context, NULL, NULL,
				 OT_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&ot_thread_data, "openthread");

	return 0;
}

void ieee802154_init(struct net_if *iface)
{
	if (IS_ENABLED(CONFIG_IEEE802154_NET_IF_NO_AUTO_START)) {
		LOG_DBG("Interface auto start disabled.");
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}

	openthread_init(iface);
}

static enum net_l2_flags openthread_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST;
}

static int openthread_enable(struct net_if *iface, bool state)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	NET_DBG("iface %p %s", iface, state ? "up" : "down");

	if (state) {
		if (IS_ENABLED(CONFIG_OPENTHREAD_MANUAL_START)) {
			NET_DBG("OpenThread manual start.");
			return 0;
		}

		return openthread_start(ot_context);
	}

	return openthread_stop(ot_context);
}

struct openthread_context *openthread_get_default_context(void)
{
	struct net_if *iface;
	struct openthread_context *ot_context = NULL;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
	if (!iface) {
		NET_ERR("There is no net interface for OpenThread");
		goto exit;
	}

	ot_context = net_if_l2_data(iface);
	if (!ot_context) {
		NET_ERR("There is no Openthread context in net interface data");
		goto exit;
	}

exit:
	return ot_context;
}

struct otInstance *openthread_get_default_instance(void)
{
	struct openthread_context *ot_context =
		openthread_get_default_context();

	return ot_context ? ot_context->instance : NULL;
}

void openthread_set_state_changed_cb(otStateChangedCallback cb)
{
	state_changed_cb = cb;
}

void openthread_api_mutex_lock(struct openthread_context *ot_context)
{
	(void)k_mutex_lock(&ot_context->api_lock, K_FOREVER);
}

void openthread_api_mutex_unlock(struct openthread_context *ot_context)
{
	(void)k_mutex_unlock(&ot_context->api_lock);
}

NET_L2_INIT(OPENTHREAD_L2, openthread_recv, openthread_send, openthread_enable,
	    openthread_flags);
