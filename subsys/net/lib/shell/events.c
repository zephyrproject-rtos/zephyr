/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/kernel.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/coap_mgmt.h>
#include <zephyr/net/ethernet_mgmt.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
#define THREAD_PRIORITY K_PRIO_COOP(2)
#define MAX_EVENT_INFO_SIZE NET_EVENT_INFO_MAX_SIZE
#define MONITOR_L2_MASK (_NET_EVENT_IF_BASE)
#define MONITOR_L3_IPV4_MASK (_NET_EVENT_IPV4_BASE | NET_MGMT_COMMAND_MASK)
#define MONITOR_L3_IPV6_MASK (_NET_EVENT_IPV6_BASE | NET_MGMT_COMMAND_MASK)
#define MONITOR_L4_MASK (_NET_EVENT_L4_BASE | NET_MGMT_COMMAND_MASK)

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
#define MONITOR_L2_ETHERNET_MASK (_NET_ETHERNET_BASE)
static struct net_mgmt_event_callback l2_ethernet_cb;
#endif

static bool net_event_monitoring;
static bool net_event_shutting_down;
static struct net_mgmt_event_callback l2_cb;
static struct net_mgmt_event_callback l3_ipv4_cb;
static struct net_mgmt_event_callback l3_ipv6_cb;
static struct net_mgmt_event_callback l4_cb;
static struct k_thread event_mon;
static K_THREAD_STACK_DEFINE(event_mon_stack, CONFIG_NET_MGMT_EVENT_MONITOR_STACK_SIZE);

static const char unknown_event_str[] = "<unknown event>";

struct event_msg {
	struct net_if *iface;
	size_t len;
	uint32_t event;
	uint8_t data[MAX_EVENT_INFO_SIZE];
};

K_MSGQ_DEFINE(event_mon_msgq, sizeof(struct event_msg),
	      CONFIG_NET_MGMT_EVENT_QUEUE_SIZE, sizeof(intptr_t));

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	struct event_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.len = MIN(sizeof(msg.data), cb->info_length);
	msg.event = mgmt_event;
	msg.iface = iface;

	if (cb->info_length > 0) {
		memcpy(msg.data, cb->info, msg.len);
	}

	ret = k_msgq_put(&event_mon_msgq, (void *)&msg, K_MSEC(10));
	if (ret < 0) {
		NET_ERR("Cannot write to msgq (%d)\n", ret);
	}
}

static char *get_l2_desc(struct event_msg *msg,
			 const char **desc, const char **desc2,
			 char *extra_info, size_t extra_info_len)
{
	static const char *desc_unknown = unknown_event_str;
	char *info = NULL;

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
#define MAX_VLAN_TAG_INFO_STR_LEN sizeof("tag xxxx disabled")
	static char vlan_buf[MAX_VLAN_TAG_INFO_STR_LEN + 1];
#endif

	*desc = desc_unknown;

	switch (msg->event) {
	case NET_EVENT_IF_DOWN:
		*desc = "interface";
		*desc2 = "down";
		break;
	case NET_EVENT_IF_UP:
		*desc = "interface";
		*desc2 = "up";
		break;
	case NET_EVENT_IF_ADMIN_UP:
		*desc = "interface";
		*desc2 = "admin up";
		break;
	case NET_EVENT_IF_ADMIN_DOWN:
		*desc = "interface";
		*desc2 = "admin down";
		break;
	case NET_EVENT_ETHERNET_CARRIER_ON:
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
		*desc = "Ethernet";
		*desc2 = "carrier on";
#endif
		break;
	case NET_EVENT_ETHERNET_CARRIER_OFF:
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
		*desc = "Ethernet";
		*desc2 = "carrier off";
#endif
		break;
	case NET_EVENT_ETHERNET_VLAN_TAG_ENABLED: {
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
		uint16_t tag = *((uint16_t *)msg->data);

		*desc = "VLAN";
		snprintk(vlan_buf, MAX_VLAN_TAG_INFO_STR_LEN,
			 "tag %u enabled", (uint16_t)tag);
		*desc2 = vlan_buf;
#endif
		break;
	}
	case NET_EVENT_ETHERNET_VLAN_TAG_DISABLED: {
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
		uint16_t tag = *((uint16_t *)msg->data);

		*desc = "VLAN";
		snprintk(vlan_buf, MAX_VLAN_TAG_INFO_STR_LEN,
			 "tag %u enabled", (uint16_t)tag);
		*desc2 = vlan_buf;
#endif
		break;
	}
	}

	return info;
}

static char *get_l3_desc(struct event_msg *msg,
			 const char **desc, const char **desc2,
			 char *extra_info, size_t extra_info_len)
{
	static const char *desc_unknown = unknown_event_str;
	char *info = NULL;

#if defined(CONFIG_NET_PMTU)
#define MAX_PMTU_INFO_STR_LEN sizeof("changed MTU xxxxx for")
	static char pmtu_buf[MAX_PMTU_INFO_STR_LEN + 1];
#endif

	*desc = desc_unknown;

	switch (msg->event) {
	case NET_EVENT_IPV6_ADDR_ADD:
		*desc = "IPv6 address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ADDR_DEPRECATED:
		*desc = "IPv6 address";
		*desc2 = "deprecated";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ADDR_DEL:
		*desc = "IPv6 address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MADDR_ADD:
		*desc = "IPv6 mcast address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MADDR_DEL:
		*desc = "IPv6 mcast address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_PREFIX_ADD:
		*desc = "IPv6 prefix";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_PREFIX_DEL:
		*desc = "IPv6 prefix";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MCAST_JOIN:
		*desc = "IPv6 mcast";
		*desc2 = "join";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MCAST_LEAVE:
		*desc = "IPv6 mcast";
		*desc2 = "leave";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTER_ADD:
		*desc = "IPv6 router";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTER_DEL:
		*desc = "IPv6 router";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTE_ADD:
		*desc = "IPv6 route";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTE_DEL:
		*desc = "IPv6 route";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_DAD_SUCCEED:
		*desc = "IPv6 DAD";
		*desc2 = "ok";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_DAD_FAILED:
		*desc = "IPv6 DAD";
		*desc2 = "fail";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_NBR_ADD:
		*desc = "IPv6 neighbor";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_NBR_DEL:
		*desc = "IPv6 neighbor";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_PE_ENABLED:
		*desc = "IPv6 PE";
		*desc2 = "enabled";
		break;
	case NET_EVENT_IPV6_PE_DISABLED:
		*desc = "IPv6 PE";
		*desc2 = "disabled";
		break;
	case NET_EVENT_IPV6_PE_FILTER_ADD:
		*desc = "IPv6 PE filter";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_PE_FILTER_DEL:
		*desc = "IPv6 PE filter";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ADDR_ADD:
		*desc = "IPv4 address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		*desc = "IPv4 address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_MADDR_ADD:
		*desc = "IPv4 mcast address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_MADDR_DEL:
		*desc = "IPv4 mcast address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ROUTER_ADD:
		*desc = "IPv4 router";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ROUTER_DEL:
		*desc = "IPv4 router";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_DHCP_START:
		*desc = "DHCPv4";
		*desc2 = "start";
		break;
	case NET_EVENT_IPV4_DHCP_BOUND:
		*desc = "DHCPv4";
		*desc2 = "bound";
#if defined(CONFIG_NET_DHCPV4)
		struct net_if_dhcpv4 *data = (struct net_if_dhcpv4 *)msg->data;

		info = net_addr_ntop(AF_INET, &data->requested_ip, extra_info,
				     extra_info_len);
#endif
		break;
	case NET_EVENT_IPV4_DHCP_STOP:
		*desc = "DHCPv4";
		*desc2 = "stop";
		break;
	case NET_EVENT_IPV4_MCAST_JOIN:
		*desc = "IPv4 mcast";
		*desc2 = "join";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_MCAST_LEAVE:
		*desc = "IPv4 mcast";
		*desc2 = "leave";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ACD_SUCCEED:
		*desc = "IPv4 ACD";
		*desc2 = "ok";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ACD_FAILED:
		*desc = "IPv4 ACD";
		*desc2 = "fail";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_PMTU_CHANGED: {
#if defined(CONFIG_NET_IPV4_PMTU)
		struct net_event_ipv4_pmtu_info *pmtu_info =
			(struct net_event_ipv4_pmtu_info *)msg->data;

		*desc = "IPV4 PMTU";
		*desc2 = pmtu_buf;
		snprintk(pmtu_buf, MAX_PMTU_INFO_STR_LEN,
			 "changed MTU %u for", (uint16_t)pmtu_info->mtu);
		info = net_addr_ntop(AF_INET, &pmtu_info->dst, extra_info,
				     extra_info_len);
#endif
		break;
	}
	case NET_EVENT_IPV6_PMTU_CHANGED: {
#if defined(CONFIG_NET_IPV6_PMTU)
		struct net_event_ipv6_pmtu_info *pmtu_info =
			(struct net_event_ipv6_pmtu_info *)msg->data;

		*desc = "IPV6 PMTU";
		*desc2 = pmtu_buf;
		snprintk(pmtu_buf, MAX_PMTU_INFO_STR_LEN,
			 "changed MTU %u for", (uint16_t)pmtu_info->mtu);
		info = net_addr_ntop(AF_INET6, &pmtu_info->dst, extra_info,
				     extra_info_len);
#endif
		break;
	}
	}

	return info;
}

static char *sockaddr_ntop(const struct sockaddr *addr, char *dst, size_t size)
{
	if (addr->sa_family == AF_INET) {
		return net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr, dst, size);
	} else if (addr->sa_family == AF_INET6) {
		return net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, dst, size);
	}

	return NULL;
}

static char *get_l4_desc(struct event_msg *msg,
			 const char **desc, const char **desc2,
			 char *extra_info, size_t extra_info_len)
{
	static const char *desc_unknown = unknown_event_str;
	char *info = NULL;

	*desc = desc_unknown;

	switch (msg->event) {
	case NET_EVENT_L4_CONNECTED:
		*desc = "connected";
		break;
	case NET_EVENT_L4_DISCONNECTED:
		*desc = "disconnected";
		break;
	case NET_EVENT_L4_IPV4_CONNECTED:
		*desc = "IPv4 connectivity";
		*desc2 = "available";
		break;
	case NET_EVENT_L4_IPV4_DISCONNECTED:
		*desc = "IPv4 connectivity";
		*desc2 = "lost";
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		*desc = "IPv6 connectivity";
		*desc2 = "available";
		break;
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		*desc = "IPv6 connectivity";
		*desc2 = "lost";
		break;
	case NET_EVENT_DNS_SERVER_ADD: {
		struct sockaddr *addr = (struct sockaddr *)msg->data;

		*desc = "DNS server";
		*desc2 = "add";

		info = sockaddr_ntop(addr, extra_info, extra_info_len);

		break;
	}
	case NET_EVENT_DNS_SERVER_DEL: {
		struct sockaddr *addr = (struct sockaddr *)msg->data;

		*desc = "DNS server";
		*desc2 = "del";

		info = sockaddr_ntop(addr, extra_info, extra_info_len);

		break;
	}
	case NET_EVENT_HOSTNAME_CHANGED:
		*desc = "Hostname changed";
		break;
	case NET_EVENT_COAP_SERVICE_STARTED:
		*desc = "CoAP service";
		*desc2 = "started";
		break;
	case NET_EVENT_COAP_SERVICE_STOPPED:
		*desc = "CoAP service";
		*desc2 = "stopped";
		break;
	case NET_EVENT_COAP_OBSERVER_ADDED:
		*desc = "CoAP observer";
		*desc2 = "added";
		break;
	case NET_EVENT_COAP_OBSERVER_REMOVED:
		*desc = "CoAP observer";
		*desc2 = "removed";
		break;
	case NET_EVENT_CAPTURE_STARTED:
		*desc = "Capture";
		*desc2 = "started";
		break;
	case NET_EVENT_CAPTURE_STOPPED:
		*desc = "Capture";
		*desc2 = "stopped";
		break;
	}

	return info;
}

/* We use a separate thread in order not to do any shell printing from
 * event handler callback (to avoid stack size issues).
 */
static void event_mon_handler(const struct shell *sh, void *p2, void *p3)
{
	char extra_info[NET_IPV6_ADDR_LEN];
	struct event_msg msg;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	net_mgmt_init_event_callback(&l2_cb, event_handler,
				     MONITOR_L2_MASK);
	net_mgmt_add_event_callback(&l2_cb);

	net_mgmt_init_event_callback(&l3_ipv4_cb, event_handler,
				     MONITOR_L3_IPV4_MASK);
	net_mgmt_add_event_callback(&l3_ipv4_cb);

	net_mgmt_init_event_callback(&l3_ipv6_cb, event_handler,
				     MONITOR_L3_IPV6_MASK);
	net_mgmt_add_event_callback(&l3_ipv6_cb);

	net_mgmt_init_event_callback(&l4_cb, event_handler,
				     MONITOR_L4_MASK);
	net_mgmt_add_event_callback(&l4_cb);

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	net_mgmt_init_event_callback(&l2_ethernet_cb, event_handler,
				     MONITOR_L2_ETHERNET_MASK);
	net_mgmt_add_event_callback(&l2_ethernet_cb);
#endif

	while (net_event_shutting_down == false) {
		const char *layer_str = "<unknown layer>";
		const char *desc = "", *desc2 = "";
		char *info = NULL;
		uint32_t layer;

		(void)k_msgq_get(&event_mon_msgq, &msg, K_FOREVER);

		if (msg.iface == NULL && msg.event == 0 && msg.len == 0) {
			/* This is the stop message */
			continue;
		}

		layer = NET_MGMT_GET_LAYER(msg.event);
		if (layer == NET_MGMT_LAYER_L2) {
			layer_str = "L2";
			info = get_l2_desc(&msg, &desc, &desc2,
					   extra_info, NET_IPV6_ADDR_LEN);
		} else if (layer == NET_MGMT_LAYER_L3) {
			layer_str = "L3";
			info = get_l3_desc(&msg, &desc, &desc2,
					   extra_info, NET_IPV6_ADDR_LEN);
		} else if (layer == NET_MGMT_LAYER_L4) {
			layer_str = "L4";
			info = get_l4_desc(&msg, &desc, &desc2,
					   extra_info, NET_IPV6_ADDR_LEN);
		}

		if (desc == unknown_event_str) {
			PR_INFO("EVENT: %s [%d] %s%s%s%s%s (0x%08x)\n", layer_str,
				net_if_get_by_iface(msg.iface), desc,
				desc2 ? " " : "", desc2 ? desc2 : "",
				info ? " " : "", info ? info : "", msg.event);
		} else {
			PR_INFO("EVENT: %s [%d] %s%s%s%s%s\n", layer_str,
				net_if_get_by_iface(msg.iface), desc,
				desc2 ? " " : "", desc2 ? desc2 : "",
				info ? " " : "", info ? info : "");
		}
	}

	net_mgmt_del_event_callback(&l2_cb);
	net_mgmt_del_event_callback(&l3_ipv4_cb);
	net_mgmt_del_event_callback(&l3_ipv6_cb);
	net_mgmt_del_event_callback(&l4_cb);

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	net_mgmt_del_event_callback(&l2_ethernet_cb);
#endif

	k_msgq_purge(&event_mon_msgq);

	net_event_monitoring = false;
	net_event_shutting_down = false;

	PR_INFO("Network event monitoring %s.\n", "disabled");
}
#endif

static int cmd_net_events_on(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
	k_tid_t tid;

	if (net_event_monitoring) {
		PR_INFO("Network event monitoring is already %s.\n",
			"enabled");
		return -ENOEXEC;
	}

	tid = k_thread_create(&event_mon, event_mon_stack,
			      K_THREAD_STACK_SIZEOF(event_mon_stack),
			      (k_thread_entry_t)event_mon_handler,
			      (void *)sh, NULL, NULL, THREAD_PRIORITY, 0,
			      K_FOREVER);
	if (!tid) {
		PR_ERROR("Cannot create network event monitor thread!");
		return -ENOEXEC;
	}

	k_thread_name_set(tid, "event_mon");

	PR_INFO("Network event monitoring %s.\n", "enabled");

	net_event_monitoring = true;
	net_event_shutting_down = false;

	k_thread_start(tid);
#else
	PR_INFO("Network management events are not supported. "
		"Set CONFIG_NET_MGMT_EVENT_MONITOR to enable it.\n");
#endif

	return 0;
}

static int cmd_net_events_off(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
	static const struct event_msg msg;
	int ret;

	if (!net_event_monitoring) {
		PR_INFO("Network event monitoring is already %s.\n",
			"disabled");
		return -ENOEXEC;
	}

	net_event_shutting_down = true;

	ret = k_msgq_put(&event_mon_msgq, (void *)&msg, K_MSEC(100));
	if (ret < 0) {
		PR_ERROR("Cannot write to msgq (%d)\n", ret);
		return -ENOEXEC;
	}
#else
	PR_INFO("Network management events are not supported. "
		"Set CONFIG_NET_MGMT_EVENT_MONITOR to enable it.\n");
#endif

	return 0;
}

static int cmd_net_events(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
	PR("Network event monitoring is %s.\n",
	   net_event_monitoring ? "enabled" : "disabled");

	if (!argv[1]) {
		PR_INFO("Give 'on' to enable event monitoring and "
			"'off' to disable it.\n");
	}
#else
	PR_INFO("Network management events are not supported. "
		"Set CONFIG_NET_MGMT_EVENT_MONITOR to enable it.\n");
#endif

	return 0;
}

void events_enable(void)
{
	static const char * const argv[] = {
		"on",
		NULL
	};

	(void)cmd_net_events_on(shell_backend_uart_get_ptr(), 1, (char **)argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_events,
	SHELL_CMD(on, NULL, "Turn on network event monitoring.",
		  cmd_net_events_on),
	SHELL_CMD(off, NULL, "Turn off network event monitoring.",
		  cmd_net_events_off),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), events, &net_cmd_events, "Monitor network management events.",
		 cmd_net_events, 1, 1);
