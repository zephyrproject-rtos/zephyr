#include <errno.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>

#include "http_server.h"

LOG_MODULE_REGISTER(w6300_http_server, LOG_LEVEL_INF);

#define DHCP_WAIT_TIMEOUT K_SECONDS(10)

static struct net_mgmt_event_callback mgmt_cb;
static struct k_sem dhcp_ready_sem;

static void net_event_handler(struct net_mgmt_event_callback *cb,
			      uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		k_sem_give(&dhcp_ready_sem);
	}
}

static void setup_ipv4_watch(void)
{
	k_sem_init(&dhcp_ready_sem, 0, 1);
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);
}

static void print_network_info(struct net_if *iface)
{
	struct net_if_config *config = net_if_get_config(iface);
	char buf[NET_IPV4_ADDR_LEN];

	LOG_INF("Network Information:");
	
	if (config->ip.ipv4) {
		for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (config->ip.ipv4->unicast[i].ipv4.is_used) {
				net_addr_ntop(AF_INET, 
					      &config->ip.ipv4->unicast[i].ipv4.address.in_addr, 
					      buf, sizeof(buf));
				LOG_INF("  IP      : %s", buf);
				
				net_addr_ntop(AF_INET, 
					      &config->ip.ipv4->unicast[i].netmask, 
					      buf, sizeof(buf));
				LOG_INF("  Subnet  : %s", buf);
				
				net_addr_ntop(AF_INET, 
					      &config->ip.ipv4->gw, 
					      buf, sizeof(buf));
				LOG_INF("  Gateway : %s", buf);
			}
		}
	}

	struct net_linkaddr *link = net_if_get_link_addr(iface);
	if (link->len >= 6) {
		LOG_INF("  MAC     : %02X:%02X:%02X:%02X:%02X:%02X",
			link->addr[0], link->addr[1], link->addr[2],
			link->addr[3], link->addr[4], link->addr[5]);
	}
}

static void wait_for_ipv4_address(void)
{
	int ret;

	ret = k_sem_take(&dhcp_ready_sem, DHCP_WAIT_TIMEOUT);
	if (ret == 0) {
		LOG_INF("IPv4 address acquired");
		print_network_info(net_if_get_default());
	} else {
		LOG_WRN("Timed out waiting for IPv4 address");
	}
}

void main(void)
{
	LOG_INF("Starting W6300 HTTP server");

	setup_ipv4_watch();
	(void)net_config_init_app(NULL, "Configuring network");
	wait_for_ipv4_address();

	http_server_init();
	http_server_run();
}
