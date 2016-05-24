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

#include "config.h"
#include "tcp.h"

#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>

#include <errno.h>
#include <stdio.h>

uip_ipaddr_t uip_hostaddr =	{ { CLIENT_IPADDR0, CLIENT_IPADDR1,
				    CLIENT_IPADDR2, CLIENT_IPADDR3 } };

uip_ipaddr_t uip_netmask =	{ { NETMASK0, NETMASK1, NETMASK2, NETMASK3 } };


#define CLIENT_IP_ADDR		{ { { CLIENT_IPADDR0, CLIENT_IPADDR1,	\
				      CLIENT_IPADDR2, CLIENT_IPADDR3 } } }

#define SERVER_IP_ADDR		{ { { SERVER_IPADDR0, SERVER_IPADDR1,	\
				      SERVER_IPADDR2, SERVER_IPADDR3 } } }

#define INET_FAMILY		AF_INET


int tcp_tx(struct net_context *ctx, uint8_t *buf,  size_t size)
{
	int rc = 0;
	uint8_t *ptr;
	struct net_buf *nbuf = NULL;

	nbuf = ip_buf_get_tx(ctx);
	if (nbuf == NULL) {
		printf("[%s:%d] Unable to get buffer\n", __func__, __LINE__);
		return -1;
	}

	ptr = net_buf_add(nbuf, size);
	memcpy(ptr, buf, size);
	ip_buf_appdatalen(nbuf) = size;

	do {
		rc = net_send(nbuf);
		if (rc >= 0) {
			ip_buf_unref(nbuf);
			return 0;
		}
		switch (rc) {
		case -EINPROGRESS:
			printf("%s: no connection yet, try again\n", __func__);
			fiber_sleep(TCP_RETRY_TIMEOUT);
			break;
		case -EAGAIN:
		case -ECONNRESET:
			printf("%s: no connection, try again later\n", __func__);
			fiber_sleep(TCP_RETRY_TIMEOUT);
			break;
		default:
			printf("%s: sending %d bytes failed\n",
			      __func__, size);
			ip_buf_unref(nbuf);
			return -EIO;
		}
	} while (1);

	return 0;
}

int tcp_rx(struct net_context *ctx, uint8_t *buf, size_t *read_bytes, size_t size)
{
	int rc;
	struct net_buf *nbuf;

	nbuf = net_receive(ctx, TCP_RX_TIMEOUT);
	rc = -EIO;
	if (nbuf != NULL) {
		*read_bytes = ip_buf_appdatalen(nbuf);
		if (*read_bytes > size) {
			rc = -ENOMEM;
		} else {
			memcpy(buf, ip_buf_appdata(nbuf), *read_bytes);
			rc = 0;
		}
		ip_buf_unref(nbuf);
	}
	return rc;
}

int tcp_init(struct net_context **ctx)
{
	static struct in_addr server_addr = SERVER_IP_ADDR;
	static struct in_addr client_addr = CLIENT_IP_ADDR;
	static struct net_addr server;
	static struct net_addr client;

	server.in_addr = server_addr;
	server.family = AF_INET;

	client.in_addr = client_addr;
	client.family = AF_INET;

	*ctx = net_context_get(IPPROTO_TCP,
			       &server, SERVER_MQTT_PORT,
			       &client, CLIENT_MQTT_PORT);
	if (*ctx == NULL) {
		printf("%s: Unable to get network context\n", __func__);
		return -EINVAL;
	}
	return 0;
}
