/*
 * Copyright (c) 2016 Intel Corporation
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

#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include "udp_cfg.h"
#include "udp.h"

#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif

#include "mbedtls/ssl.h"

uip_ipaddr_t uip_hostaddr = { {CLIENT_IPADDR0, CLIENT_IPADDR1,
			       CLIENT_IPADDR2, CLIENT_IPADDR3}
};

uip_ipaddr_t uip_netmask = { {NETMASK0, NETMASK1, NETMASK2, NETMASK3}
};

#define INET_FAMILY		AF_INET

int udp_tx(void *context, const unsigned char *buf, size_t size)
{
	struct udp_context *ctx = context;
	struct net_buf *nbuf;
	uint8_t *ptr;
	int rc = 0;

	nbuf = ip_buf_get_tx(ctx->net_ctx);

	printk("\nin udp TX\n");
	if (nbuf == NULL) {
		printk("in udp TX !nbuf\n");
		return -EINVAL;
	}

	ptr = net_buf_add(nbuf, size);
	memcpy(ptr, buf, size);
	ip_buf_appdatalen(nbuf) = size;

	do {
		fiber_sleep(10);
		rc = net_send(nbuf);
		fiber_sleep(10);
		if (rc >= 0) {
			return size;
		}
		switch (rc) {
		case -EINPROGRESS:
			printk("in udp TX inpro\n");
			break;
		case -EAGAIN:
		case -ECONNRESET:
			printk("in udp TX again\n");
			break;
		default:
			ip_buf_unref(nbuf);
			printk("in udp TX ERROR!!\n");
			return -EIO;
		}
	} while (0);

	ip_buf_unref(nbuf);
	return 0;
}

int udp_rx(void *context, unsigned char *buf, size_t size)
{
	struct udp_context *ctx = context;
	struct net_buf *nbuf;
	int8_t *ptr;
	int rc, read_bytes;

	printk("\nin udp RX size = %d\n", (int)size);
	if (ctx->rx_nbuf == NULL) {
		fiber_sleep(10);
		nbuf = net_receive(ctx->net_ctx, UDP_RX_TIMEOUT);
		fiber_sleep(10);
		rc = -EIO;
		if (nbuf != NULL) {
			read_bytes = ip_buf_appdatalen(nbuf);
			printk("\nin udp RX size = %d read = %d\n", (int)size,
			       read_bytes);
			if (read_bytes > size) {
				memcpy(buf, ip_buf_appdata(nbuf), size);
				ctx->rx_nbuf = nbuf;
				ctx->remaining = read_bytes - size;
				return size;
			} else {
				memcpy(buf, ip_buf_appdata(nbuf), read_bytes);
				rc = read_bytes;
			}
			ip_buf_unref(nbuf);
		}
	} else {
		ptr = ip_buf_appdata(ctx->rx_nbuf);
		read_bytes = ip_buf_appdatalen(ctx->rx_nbuf) - ctx->remaining;
		ptr += read_bytes;
		if (ctx->remaining > size) {
			memcpy(buf, ptr, size);
			ctx->remaining -= size;
			rc = size;
		} else {
			read_bytes =
			    size < ctx->remaining ? size : ctx->remaining;
			memcpy(buf, ptr, read_bytes);
			ip_buf_unref(ctx->rx_nbuf);
			ctx->remaining = 0;
			ctx->rx_nbuf = NULL;
			rc = read_bytes;
		}
	}

	return rc;
}

int udp_init(struct udp_context *ctx)
{
	static struct in_addr server_addr = SERVER_IP_ADDR;
	static struct in_addr client_addr = CLIENT_IP_ADDR;
	static struct net_addr server;
	static struct net_addr client;

	server.in_addr = server_addr;
	server.family = AF_INET;

	client.in_addr = client_addr;
	client.family = AF_INET;

	ctx->rx_nbuf = NULL;
	ctx->remaining = 0;

	ctx->net_ctx = net_context_get(IPPROTO_UDP,
				       &server, SERVER_PORT,
				       &client, CLIENT_PORT);
	if (ctx->net_ctx == NULL) {
		return -EINVAL;
	}
	return 0;
}
