/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if (OTBR_ENABLE_WEB_SERVER == 1)
    #include <zephyr/kernel.h>
    #include <zephyr/net/net_if.h>
    #include <zephyr/net/net_mgmt.h>
    #include <zephyr/net/dhcpv4.h>
    #include <zephyr/logging/log.h>
    #include "web_server.h"

    LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

    static struct net_mgmt_event_callback net_cb;
    static bool web_server_started = false;

    static void net_event_handler(struct net_mgmt_event_callback *cb,
                      uint64_t mgmt_event, struct net_if *iface)
    {
        LOG_INF("*** NETWORK EVENT: 0x%016llx ***", mgmt_event);

        /* Print the event in a more readable format */
        LOG_INF("Event details:");
        LOG_INF("  - Event ID: 0x%016llx", mgmt_event);
        LOG_INF("  - Interface: %p", iface);

        /* Check for specific events we care about */
        if (mgmt_event & NET_EVENT_IPV4_DHCP_BOUND) {
            LOG_INF("  - DHCP BOUND detected!");
            goto start_web_server;
        }

        if (mgmt_event & NET_EVENT_IPV4_ADDR_ADD) {
            LOG_INF("  - IPv4 ADDR ADD detected!");
            goto start_web_server;
        }

        if (mgmt_event & NET_EVENT_IPV6_ADDR_ADD) {
            LOG_INF("  - IPv6 ADDR ADD detected!");
            goto start_web_server;
        }

        /* Log all other events for debugging */
        if (mgmt_event & NET_EVENT_IF_UP) {
            LOG_INF("  - Interface UP");
        }
        if (mgmt_event & NET_EVENT_IF_DOWN) {
            LOG_INF("  - Interface DOWN");
        }
        if (mgmt_event & NET_EVENT_IPV4_DHCP_START) {
            LOG_INF("  - DHCP START");
        }
        if (mgmt_event & NET_EVENT_IPV4_DHCP_STOP) {
            LOG_INF("  - DHCP STOP");
        }

        return;

    start_web_server:
        if (!web_server_started) {
            LOG_INF("Starting web server due to network event");
            if (web_server_init() == 0) {
                web_server_started = true;
                LOG_INF("Web server started successfully");
            } else {
                LOG_ERR("Failed to start web server");
            }
        } else {
            LOG_INF("Web server already started");
        }
    }

    /* Register multiple callbacks to catch different event types */
    static struct net_mgmt_event_callback dhcp_cb;
    static struct net_mgmt_event_callback addr_cb;
    static struct net_mgmt_event_callback if_cb;

    static void dhcp_event_handler(struct net_mgmt_event_callback *cb,
                       uint64_t mgmt_event, struct net_if *iface)
    {
        LOG_INF("DHCP Event: 0x%016llx", mgmt_event);
        net_event_handler(cb, mgmt_event, iface);
    }

    static void addr_event_handler(struct net_mgmt_event_callback *cb,
                       uint64_t mgmt_event, struct net_if *iface)
    {
        LOG_INF("Address Event: 0x%016llx", mgmt_event);
        net_event_handler(cb, mgmt_event, iface);
    }

    static void if_event_handler(struct net_mgmt_event_callback *cb,
                     uint64_t mgmt_event, struct net_if *iface)
    {
        LOG_INF("Interface Event: 0x%016llx", mgmt_event);
        net_event_handler(cb, mgmt_event, iface);
    }

    int main(void)
    {
        LOG_INF("OpenThread Border Router starting...");

        /* Register multiple specific event callbacks */
        LOG_INF("Registering DHCP event callbacks...");
        net_mgmt_init_event_callback(&dhcp_cb, dhcp_event_handler,
                         NET_EVENT_IPV4_DHCP_START |
                         NET_EVENT_IPV4_DHCP_BOUND |
                         NET_EVENT_IPV4_DHCP_STOP);
        net_mgmt_add_event_callback(&dhcp_cb);

        LOG_INF("Registering address event callbacks...");
        net_mgmt_init_event_callback(&addr_cb, addr_event_handler,
                         NET_EVENT_IPV4_ADDR_ADD |
                         NET_EVENT_IPV4_ADDR_DEL |
                         NET_EVENT_IPV6_ADDR_ADD |
                         NET_EVENT_IPV6_ADDR_DEL);
        net_mgmt_add_event_callback(&addr_cb);

        LOG_INF("Registering interface event callbacks...");
        net_mgmt_init_event_callback(&if_cb, if_event_handler,
                         NET_EVENT_IF_UP |
                         NET_EVENT_IF_DOWN);
        net_mgmt_add_event_callback(&if_cb);

        /* Also register a catch-all callback */
        LOG_INF("Registering catch-all event callback...");
        net_mgmt_init_event_callback(&net_cb, net_event_handler, 0xFFFFFFFFFFFFFFFFULL);
        net_mgmt_add_event_callback(&net_cb);

        LOG_INF("All network event callbacks registered");
        LOG_INF("Waiting for network events...");

        return 0;
    }
#else
    int main(void)
    {
        /* Nothing to do here. The Border Router is automatically started in the background. */
        return 0;
    }
#endif
