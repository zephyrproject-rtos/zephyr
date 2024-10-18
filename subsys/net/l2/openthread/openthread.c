/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_openthread, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/openthread.h>

#include <net_private.h>

#include <zephyr/init.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/version.h>

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
#include <utils/uart.h>

#include <platform-zephyr.h>

#include "openthread_utils.h"

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
#include <openthread/nat64.h>
#endif  /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

#define PKT_IS_IPv4(_p) ((NET_IPV6_HDR(_p)->vtc & 0xf0) == 0x40)

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

#if defined(CONFIG_OPENTHREAD_NETWORKKEY)
#define OT_NETWORKKEY CONFIG_OPENTHREAD_NETWORKKEY
#else
#define OT_NETWORKKEY ""
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

#define ZEPHYR_PACKAGE_NAME "Zephyr"
#define PACKAGE_VERSION KERNEL_VERSION_STRING

extern void platformShellInit(otInstance *aInstance);

K_KERNEL_STACK_DEFINE(ot_stack_area, OT_STACK_SIZE);

static struct net_linkaddr *ll_addr;
static otStateChangedCallback state_changed_cb;

k_tid_t openthread_thread_id_get(void)
{
	struct openthread_context *ot_context = openthread_get_default_context();

	return ot_context ? (k_tid_t)&ot_context->work_q.thread : 0;
}

#ifdef CONFIG_NET_MGMT_EVENT
static struct net_mgmt_event_callback ip6_addr_cb;

static void ipv6_addr_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(OPENTHREAD)) {
		return;
	}

#ifdef CONFIG_NET_MGMT_EVENT_INFO
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (cb->info == NULL || cb->info_length != sizeof(struct in6_addr)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		add_ipv6_addr_to_ot(ot_context, (const struct in6_addr *)cb->info);
	} else if (mgmt_event == NET_EVENT_IPV6_MADDR_ADD) {
		add_ipv6_maddr_to_ot(ot_context, (const struct in6_addr *)cb->info);
	}
#else
	NET_WARN("No address info provided with event, "
		 "please enable CONFIG_NET_MGMT_EVENT_INFO");
#endif /* CONFIG_NET_MGMT_EVENT_INFO */
}
#endif /* CONFIG_NET_MGMT_EVENT */

static int ncp_hdlc_send(const uint8_t *buf, uint16_t len)
{
	otError err;

	err = otPlatUartSend(buf, len);
	if (err != OT_ERROR_NONE) {
		return 0;
	}

	return len;
}

#ifndef CONFIG_HDLC_RCP_IF
void otPlatRadioGetIeeeEui64(otInstance *instance, uint8_t *ieee_eui64)
{
	ARG_UNUSED(instance);

	memcpy(ieee_eui64, ll_addr->addr, ll_addr->len);
}
#endif /* CONFIG_HDLC_RCP_IF */

void otTaskletsSignalPending(otInstance *instance)
{
	struct openthread_context *ot_context = openthread_get_default_context();

	if (ot_context) {
		k_work_submit_to_queue(&ot_context->work_q, &ot_context->api_work);
	}
}

void otSysEventSignalPending(void)
{
	otTaskletsSignalPending(NULL);
}

static void ot_state_changed_handler(uint32_t flags, void *context)
{
	struct openthread_state_changed_cb *entry, *next;
	struct openthread_context *ot_context = context;

	NET_INFO("State changed! Flags: 0x%08" PRIx32 " Current role: %s",
		flags,
		otThreadDeviceRoleToString(otThreadGetDeviceRole(ot_context->instance))
		);

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			net_if_dormant_off(ot_context->iface);
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
		default:
			net_if_dormant_on(ot_context->iface);
			break;
		}
	}

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

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

	if (flags & OT_CHANGED_NAT64_TRANSLATOR_STATE) {
		NET_DBG("Nat64 translator state changed to %x",
			otNat64GetTranslatorState(ot_context->instance));
	}

#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

	if (state_changed_cb) {
		state_changed_cb(flags, context);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ot_context->state_change_cbs, entry, next, node) {
		if (entry->state_changed_cb != NULL) {
			entry->state_changed_cb(flags, ot_context, entry->user_data);
		}
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
		read_len = otMessageRead(aMessage, offset, pkt_buf->data,
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

	NET_DBG("Injecting %s packet to Zephyr net stack",
		PKT_IS_IPv4(pkt) ? "translated IPv4" : "Ip6");

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		if (IS_ENABLED(CONFIG_OPENTHREAD_NAT64_TRANSLATOR) && PKT_IS_IPv4(pkt)) {
			net_pkt_hexdump(pkt, "Received NAT64 IPv4 packet");
		} else {
			net_pkt_hexdump(pkt, "Received IPv6 packet");
		}
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

static void openthread_process(struct k_work *work)
{
	struct openthread_context *ot_context
		= CONTAINER_OF(work, struct openthread_context, api_work);

	openthread_api_mutex_lock(ot_context);

	while (otTaskletsArePending(ot_context->instance)) {
		otTaskletsProcess(ot_context->instance);
	}

	otSysProcessDrivers(ot_context->instance);

	openthread_api_mutex_unlock(ot_context);
}

static bool is_ipv6_frag(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *hdr;

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		return false;
	}

	return hdr->nexthdr == NET_IPV6_NEXTHDR_FRAG ? true : false;
}

static enum net_verdict openthread_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (pkt_list_peek(ot_context) == pkt) {
		pkt_list_remove_last(ot_context);
		NET_DBG("Got injected Ip6 packet, sending to upper layers");

		if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
			net_pkt_hexdump(pkt, "Injected IPv6 packet");
		}

		if (IS_ENABLED(CONFIG_OPENTHREAD_IP6_FRAGM) && is_ipv6_frag(pkt)) {
			return NET_DROP;
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

	net_capture_pkt(iface, pkt);

	if (notify_new_tx_frame(pkt) != 0) {
		net_pkt_unref(pkt);
	}

	return len;
}

int openthread_start(struct openthread_context *ot_context)
{
	otInstance *ot_instance = ot_context->instance;
	otError error = OT_ERROR_NONE;

	openthread_api_mutex_lock(ot_context);

	NET_INFO("OpenThread version: %s", otGetVersionString());

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		NET_DBG("OpenThread co-processor.");
		goto exit;
	}

	otIp6SetEnabled(ot_context->instance, true);

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

		error = otJoinerStart(ot_instance, OT_JOINER_PSKD, NULL, ZEPHYR_PACKAGE_NAME,
				      OT_PLATFORM_INFO, PACKAGE_VERSION, NULL,
				      &ot_joiner_start_handler, ot_context);

		if (error != OT_ERROR_NONE) {
			NET_ERR("Failed to start joiner [%d]", error);
		}

		goto exit;
	} else {
		/* No dataset - load the default configuration. */
		NET_DBG("Loading OpenThread default configuration.");

		otExtendedPanId xpanid;
		otNetworkKey    networkKey;

		otThreadSetNetworkName(ot_instance, OT_NETWORK_NAME);
		otLinkSetChannel(ot_instance, OT_CHANNEL);
		otLinkSetPanId(ot_instance, OT_PANID);
		net_bytes_from_str(xpanid.m8, 8, (char *)OT_XPANID);
		otThreadSetExtendedPanId(ot_instance, &xpanid);

		if (strlen(OT_NETWORKKEY)) {
			net_bytes_from_str(networkKey.m8, OT_NETWORK_KEY_SIZE,
					   (char *)OT_NETWORKKEY);
			otThreadSetNetworkKey(ot_instance, &networkKey);
		}
	}

	NET_INFO("Network name: %s",
		 otThreadGetNetworkName(ot_instance));

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

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		return 0;
	}

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
	struct k_work_queue_config q_cfg = {
		.name = "openthread",
		.no_yield = true,
	};
	otError err;

	NET_DBG("openthread_init");

	k_mutex_init(&ot_context->api_lock);
	k_work_init(&ot_context->api_work, openthread_process);

	ll_addr = net_if_get_link_addr(iface);

	openthread_api_mutex_lock(ot_context);

	otSysInit(0, NULL);

	ot_context->instance = otInstanceInitSingle();
	ot_context->iface = iface;

	__ASSERT(ot_context->instance, "OT instance is NULL");

	if (IS_ENABLED(CONFIG_OPENTHREAD_SHELL)) {
		platformShellInit(ot_context->instance);
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		otPlatUartEnable();
		otNcpHdlcInit(ot_context->instance, ncp_hdlc_send);
	} else {
		otIp6SetReceiveFilterEnabled(ot_context->instance, true);
		otIp6SetReceiveCallback(ot_context->instance,
					ot_receive_handler, ot_context);

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

		otIp4Cidr nat64_cidr;

		if (otIp4CidrFromString(CONFIG_OPENTHREAD_NAT64_CIDR, &nat64_cidr) ==
		    OT_ERROR_NONE) {
			if (otNat64SetIp4Cidr(openthread_get_default_instance(), &nat64_cidr) !=
			    OT_ERROR_NONE) {
				NET_ERR("Incorrect NAT64 CIDR");
			}
		} else {
			NET_ERR("Failed to parse NAT64 CIDR");
		}
		otNat64SetReceiveIp4Callback(ot_context->instance, ot_receive_handler, ot_context);

#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

		sys_slist_init(&ot_context->state_change_cbs);
		err = otSetStateChangedCallback(ot_context->instance,
						&ot_state_changed_handler,
						ot_context);
		if (err != OT_ERROR_NONE) {
			NET_ERR("Could not set state changed callback: %d", err);
		}

		net_mgmt_init_event_callback(
			&ip6_addr_cb, ipv6_addr_event_handler,
			NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_MADDR_ADD);
		net_mgmt_add_event_callback(&ip6_addr_cb);

		net_if_dormant_on(iface);
	}

	openthread_api_mutex_unlock(ot_context);

	k_work_queue_start(&ot_context->work_q, ot_stack_area,
			   K_KERNEL_STACK_SIZEOF(ot_stack_area),
			   OT_PRIORITY, &q_cfg);

	(void)k_work_submit_to_queue(&ot_context->work_q, &ot_context->api_work);

	return 0;
}

void ieee802154_init(struct net_if *iface)
{
	if (IS_ENABLED(CONFIG_IEEE802154_NET_IF_NO_AUTO_START)) {
		LOG_DBG("Interface auto start disabled.");
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}

	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
	net_if_flag_set(iface, NET_IF_IPV6_NO_MLD);

	openthread_init(iface);
}

static enum net_l2_flags openthread_flags(struct net_if *iface)
{
	/* TODO: Should report NET_L2_PROMISC_MODE if the radio driver
	 *       reports the IEEE802154_HW_PROMISC capability.
	 */
	return NET_L2_MULTICAST | NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE;
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

int openthread_state_changed_cb_register(struct openthread_context *ot_context,
					 struct openthread_state_changed_cb *cb)
{
	CHECKIF(cb == NULL || cb->state_changed_cb == NULL) {
		return -EINVAL;
	}

	openthread_api_mutex_lock(ot_context);
	sys_slist_append(&ot_context->state_change_cbs, &cb->node);
	openthread_api_mutex_unlock(ot_context);

	return 0;
}

int openthread_state_changed_cb_unregister(struct openthread_context *ot_context,
					   struct openthread_state_changed_cb *cb)
{
	bool removed;

	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	openthread_api_mutex_lock(ot_context);
	removed = sys_slist_find_and_remove(&ot_context->state_change_cbs, &cb->node);
	openthread_api_mutex_unlock(ot_context);

	if (!removed) {
		return -EALREADY;
	}

	return 0;
}

void openthread_set_state_changed_cb(otStateChangedCallback cb)
{
	state_changed_cb = cb;
}

void openthread_api_mutex_lock(struct openthread_context *ot_context)
{
	(void)k_mutex_lock(&ot_context->api_lock, K_FOREVER);
}

int openthread_api_mutex_try_lock(struct openthread_context *ot_context)
{
	return k_mutex_lock(&ot_context->api_lock, K_NO_WAIT);
}

void openthread_api_mutex_unlock(struct openthread_context *ot_context)
{
	(void)k_mutex_unlock(&ot_context->api_lock);
}

NET_L2_INIT(OPENTHREAD_L2, openthread_recv, openthread_send, openthread_enable,
	    openthread_flags);
