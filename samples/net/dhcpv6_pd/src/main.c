/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dhcpv6_pd_sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/dhcpv6.h>
#if defined(CONFIG_NET_DHCPV6_SERVER)
#include <zephyr/net/dhcpv6_server.h>
#endif

static struct net_mgmt_event_callback mgmt_cb;

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint64_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV6_DHCP_BOUND) {
		LOG_INF("DHCPv6 bound on interface %d",
			net_if_get_by_iface(iface));
	} else if (mgmt_event == NET_EVENT_IPV6_PREFIX_ADD) {
		LOG_INF("IPv6 prefix added on interface %d",
			net_if_get_by_iface(iface));
	}
}

#if !defined(CONFIG_NET_DHCPV6_SERVER)
/* Return the first interface that is not @p exclude, or NULL. */
static struct net_if *second_iface(struct net_if *exclude)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (iface != exclude) {
			return iface;
		}
	}

	return NULL;
}
#endif

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No default network interface");
		return -ENODEV;
	}

	net_mgmt_init_event_callback(&mgmt_cb, event_handler,
				     NET_EVENT_IPV6_DHCP_BOUND |
				     NET_EVENT_IPV6_PREFIX_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

#if defined(CONFIG_NET_DHCPV6_SERVER)
	{
		struct net_dhcpv6_server_params params = {
			.prefix = { { { 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0 } } },
			.prefix_len = 64,
			.addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0xab, 0xcd, 0, 0,
				      0, 0, 0, 0, 0, 0, 0, 0 } } },
			.offer_addr = true,
			.offer_prefix = true,
		};

		LOG_INF("Starting DHCPv6-PD server");
		net_dhcpv6_server_start(iface, &params);
	}
#else
	{
		struct net_if *down = second_iface(iface);
		struct net_dhcpv6_params params = {
			.request_addr = true,
			.request_prefix = true,
		};

		if (down != NULL) {
			params.downstream_ifaces[0] = net_if_get_by_iface(down);
			params.downstream_count = 1;
			LOG_INF("Requesting router: upstream iface %d, "
				"downstream iface %d",
				net_if_get_by_iface(iface),
				net_if_get_by_iface(down));
		} else {
			LOG_WRN("No downstream interface found, the delegated "
				"prefix will only be installed on iface %d",
				net_if_get_by_iface(iface));
		}

		LOG_INF("Starting DHCPv6 requesting router (prefix delegation)");
		net_dhcpv6_start(iface, &params);
	}
#endif

	return 0;
}
