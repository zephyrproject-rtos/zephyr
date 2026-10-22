/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Networking public headers have to be usable from C++ applications. Nothing
 * here is executed, the value of the test is that every header below is parsed
 * by a C++ compiler. C++ rejects constructs that C accepts, for example a
 * struct member sharing the name of the struct that encloses it, and such a
 * header breaks every C++ application that includes it.
 *
 * Add a header here when it is meant to be included by applications. Headers
 * that only device drivers include are out of scope.
 */

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_stats.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_poll.h>
#include <zephyr/net/socket_service.h>

#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv6.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/mdns_responder.h>

#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>
#include <zephyr/net/coap_link_format.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/mqtt_sn.h>
#include <zephyr/net/tftp.h>

#include <zephyr/ztest.h>

/* Reference the members that public headers keep for source compatibility, so
 * that the test covers the members and not only the enclosing types.
 */
static struct dns_resolve_context test_dns_ctx;
static struct coap_pending test_coap_pending;

ZTEST(net_cpp, test_headers_are_cpp_compatible)
{
	test_dns_ctx.servers[0].dns_server_addr.ss_family = NET_AF_INET;
	zassert_equal(test_dns_ctx.servers[0].dns_server.sa_family, NET_AF_INET);

	test_coap_pending.addr_storage.ss_family = NET_AF_INET6;
	zassert_equal(test_coap_pending.addr.sa_family, NET_AF_INET6);
}

ZTEST_SUITE(net_cpp, NULL, NULL, NULL, NULL, NULL);
