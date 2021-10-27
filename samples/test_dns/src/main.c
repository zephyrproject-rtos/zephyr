/*
 * main.c
 *
 *  Created on: Oct 17, 2021
 *      Author: Serhii Kulyk
 */


#include <zephyr.h>
#include <sys/printk.h>
#include <time.h>
#include <net/socket.h>
#include <net/net_ip.h>
#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define SERVER_ADDR "google.com"
#define SERVER_PORT 443

static bool dhcpAcquired = false;

static int getAddrinfo(struct zsock_addrinfo **haddr, const char *srv, int sPort) {
    struct zsock_addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = 0,
    };
    char port[6] = {0};
    snprintf(port, sizeof(port), "%u", SERVER_PORT);

    int rc = -EINVAL;
    rc = zsock_getaddrinfo(srv, port, &hints, haddr);
    if (!rc) {
        LOG_INF("DNS resolved");
    }
    return rc;
}

#ifdef CONFIG_NET_L2_ETHERNET
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
static struct net_mgmt_event_callback dhcp_cb;
static void handler_cb(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface) {
    if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
        return;
    }

    char buf[NET_IPV4_ADDR_LEN];
    LOG_INF("Your address: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.dhcpv4.requested_ip, buf, sizeof(buf))));
    LOG_INF("Lease time: %u seconds", iface->config.dhcpv4.lease_time);
    LOG_INF("Subnet: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->netmask, buf, sizeof(buf))));
    LOG_INF("Router: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf))));

    dhcpAcquired = true;
}
#endif

void main() {

#ifdef CONFIG_NET_L2_ETHERNET
    net_mgmt_init_event_callback(&dhcp_cb, handler_cb, NET_EVENT_IPV4_DHCP_BOUND);
    net_mgmt_add_event_callback(&dhcp_cb);

    struct net_if *iface = net_if_get_default();
    if (iface) {
        net_dhcpv4_start(iface);
    } else
        LOG_ERR("wifi interface not available");
#endif

    while (1) {
        k_sleep(K_MSEC(100));
        if (!dhcpAcquired)
            continue;

        struct zsock_addrinfo *haddr = NULL;
        int rc = getAddrinfo(&haddr, SERVER_ADDR, SERVER_PORT);
        if (rc) {
            LOG_INF("Hostname is not resolved %s:%d, %d", log_strdup(SERVER_ADDR), SERVER_PORT, rc);
            continue;
        }

        LOG_INF("Got add info");
        char addr[17] = { 0 };
        snprintf(addr, sizeof(addr), "%u.%u.%u.%u",
                        net_sin((haddr)->ai_addr)->sin_addr.s4_addr[0],
                        net_sin((haddr)->ai_addr)->sin_addr.s4_addr[1],
                        net_sin((haddr)->ai_addr)->sin_addr.s4_addr[2],
                        net_sin((haddr)->ai_addr)->sin_addr.s4_addr[3]);
        zsock_freeaddrinfo(haddr);
        LOG_INF("Got IP %s", log_strdup(addr));
    }
}
