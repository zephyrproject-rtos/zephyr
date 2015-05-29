/* net_driver_loopback.c - Loopback driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include <net/net_buf.h>
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
	uint8_t eui64[] = { };

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

	return 0;
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
