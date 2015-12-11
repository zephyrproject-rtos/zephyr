/* net_driver_loopback.c - Loopback driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include "contiki/ip/uip-debug.h"

#include <net/net_core.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_socket.h>

/* The following uIP includes are for testing purposes only. Never
 * ever use them in your application.
 */
#include "contiki/ipv6/uip-ds6-route.h"  /* to set the route */
#include "contiki/ipv6/uip-ds6-nbr.h"    /* to set the neighbor cache */

static int net_driver_loopback_open(void)
{
	const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
	uint8_t eui64[8] = { };

	net_set_mac(eui64, sizeof(eui64));

	if (!uip_ds6_addr_add((uip_ipaddr_t *)&in6addr_loopback, 0,
				ADDR_MANUAL))
		return -EINVAL;

	if (!uip_ds6_nbr_add((uip_ipaddr_t *)&in6addr_loopback,
				&uip_lladdr, 0, NBR_REACHABLE)) {
		NET_DBG("Cannot add neighbor cache\n");
		return -EINVAL;
	}

	if (!uip_ds6_route_add((uip_ipaddr_t *)&in6addr_loopback, 128,
				(uip_ipaddr_t *)&in6addr_loopback)) {
		NET_DBG("Cannot add localhost route\n");
		return -EINVAL;
	}

	NET_DBG("initialized loopback driver\n");

	return 0;
}

static int net_driver_loopback_send(struct net_buf *buf)
{
	NET_DBG("received %d bytes\n", buf->len);

	net_recv(buf);

	return 1;
}

static struct net_driver net_driver_loopback = {
	.head_reserve = 0,
	.open = net_driver_loopback_open,
	.send = net_driver_loopback_send,
};

int net_driver_loopback_init(void)
{
	net_register_driver(&net_driver_loopback);

	return 0;
}
