/* net_driver_ethernet.c - Ethernet driver */

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
#include <stdbool.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <net/net_core.h>
#include "net_driver_ethernet.h"

#include "contiki/ipv4/uip_arp.h"

static bool opened;

static ethernet_tx_callback tx_cb;

void net_driver_ethernet_register_tx(ethernet_tx_callback cb)
{
	tx_cb = cb;
}

static int net_driver_ethernet_open(void)
{
	NET_DBG("Initialized Ethernet driver\n");

	opened = true;

	return 0;
}

bool net_driver_ethernet_is_opened(void)
{
	return opened;
}

static int net_driver_ethernet_send(struct net_buf *buf)
{
#ifdef CONFIG_NETWORKING_WITH_IPV6
	struct uip_eth_hdr *eth_hdr = (struct uip_eth_hdr *)uip_buf(buf);
#endif
	int res;

	NET_DBG("Sending %d bytes\n", buf->len);

	if (!tx_cb) {
		NET_ERR("Ethernet transmit callback is uninitialized.\n");
		return -1;
	}

#ifdef CONFIG_NETWORKING_WITH_IPV4
	/* Note that uip_arp_out overwrites the outgoing packet if it needs to
	 * send an ARP request.  It relies on higher layers to resend the
	 * original packet if necessary.
	 */
	uip_arp_out(buf);
#else
	memcpy(eth_hdr->dest.addr, ip_buf_ll_dest(buf).u8, UIP_LLADDR_LEN);
	memcpy(eth_hdr->src.addr, uip_lladdr.addr, UIP_LLADDR_LEN);
	eth_hdr->type = UIP_HTONS(UIP_ETHTYPE_IPV6);
	uip_len(buf) += sizeof(struct uip_eth_hdr);
#endif

	res = tx_cb(buf);
	if (res == 1) {
		/* Release the buffer because we sent all the data
		 * successfully.
		 */
		ip_buf_unref(buf);
	}

	return res;
}

void net_driver_ethernet_recv(struct net_buf *buf)
{
	struct uip_eth_hdr *eth_hdr = (struct uip_eth_hdr *)uip_buf(buf);

#ifdef CONFIG_NETWORKING_WITH_IPV4
	if (eth_hdr->type == uip_htons(UIP_ETHTYPE_ARP)) {
		uip_arp_arpin(buf);

		/* If uip_arp_arpin needs to send an ARP response, it
		 * overwrites the contents of buf and updates its
		 * length variable.  Otherwise, it zeroes out the
		 * length variable.
		 */
		if (uip_len(buf) == 0) {
			ip_buf_unref(buf);
			return;
		}

		if (!tx_cb) {
			NET_ERR("Ethernet transmit callback is uninitialized.\n");
			ip_buf_unref(buf);
			return;
		}

		if (tx_cb(buf) != 1) {
			NET_ERR("Failed to send ARP response.\n");
		}

		ip_buf_unref(buf);
	} else
#endif
	if (eth_hdr->type == uip_htons(UIP_ETHTYPE_IP) ||
	    eth_hdr->type == uip_htons(UIP_ETHTYPE_IPV6)) {
		if (net_recv(buf) != 0) {
			NET_ERR("Unexpected return value from net_recv.\n");
			ip_buf_unref(buf);
		}
	} else {
		NET_DBG("Dropping unknown ethertype %x\n",
			uip_ntohs(eth_hdr->type));
		ip_buf_unref(buf);
	}
}

static struct net_driver net_driver_ethernet = {
	.head_reserve = 0,
	.open = net_driver_ethernet_open,
	.send = net_driver_ethernet_send,
};

int net_driver_ethernet_init(void)
{
	net_register_driver(&net_driver_ethernet);

	return 0;
}
