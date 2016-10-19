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

#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/yaip/nbuf.h>
#include <net/net_if.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include "udp_cfg.h"
#include "udp.h"

static struct in_addr client_addr = CLIENT_IP_ADDR;
static struct in_addr server_addr = SERVER_IP_ADDR;

static void udp_received(struct net_context *context,
			 struct net_buf *buf, int status, void *user_data)
{
	struct udp_context *ctx = user_data;

	ctx->rx_nbuf = buf;
}

static inline void udp_sent(struct net_context *context,
			    int status, void *bytes_sent, void *user_data)
{
	int *data = user_data;

	if (!status) {
		*data = POINTER_TO_UINT(bytes_sent);
	}
}

int udp_tx(void *context, const unsigned char *buf, size_t size)
{
	struct udp_context *ctx = context;
	struct net_context *udp_ctx;
	struct net_buf *send_buf;
	struct sockaddr_in dst_addr;
	int rc, len, data_sent;

	udp_ctx = ctx->net_ctx;

	send_buf = net_nbuf_get_tx(udp_ctx);
	if (!send_buf) {
		printk("cannot create buf\n");
		return -EIO;
	}

	rc = net_nbuf_append(send_buf, size, (uint8_t *) buf);
	if (!rc) {
		printk("cannot write buf\n");
		return -EIO;
	}

	net_ipaddr_copy(&dst_addr.sin_addr, &server_addr);
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(SERVER_PORT);

	len = net_buf_frags_len(send_buf);
	fiber_sleep(UDP_TX_TIMEOUT);

	rc = net_context_sendto(send_buf, (struct sockaddr *)&dst_addr,
				sizeof(struct sockaddr_in),
				udp_sent, TICKS_UNLIMITED,
				UINT_TO_POINTER(len), &data_sent);

	if (rc < 0) {
		printk("Cannot send IPv4 data to peer (%d)\n", rc);
		net_nbuf_unref(send_buf);
		return -EIO;
	} else {
		return data_sent;
	}
}

int udp_rx(void *context, unsigned char *buf, size_t size)
{
	struct udp_context *ctx = context;
	uint8_t *ptr;
	int pos = 0;
	int len;
	struct net_buf *rx_buf;
	int rc, read_bytes;

	rc = net_context_recv(ctx->net_ctx, udp_received, TICKS_UNLIMITED, ctx);
	if (rc != 0) {
		printk("[%s:%d] TX: %d\n", __func__, __LINE__, rc);
		rc = 0;
	}
	read_bytes = net_nbuf_appdatalen(ctx->rx_nbuf);

	ptr = net_nbuf_appdata(ctx->rx_nbuf);
	rx_buf = ctx->rx_nbuf->frags;
	len = rx_buf->len - (ptr - rx_buf->data);
	while (rx_buf) {
		memcpy(buf + pos, ptr, len);
		pos += len;
		rx_buf = rx_buf->frags;
		if (!rx_buf) {
			break;
		}
		ptr = rx_buf->data;
		len = rx_buf->len;
	}

	if (read_bytes != pos) {
		return -EIO;
	}
	rc = read_bytes;
	net_nbuf_unref(ctx->rx_nbuf);
	ctx->remaining = 0;
	ctx->rx_nbuf = NULL;
	return rc;
}

int udp_init(struct udp_context *ctx)
{
	struct net_context *udp_ctx = { 0 };
	struct sockaddr_in my_addr4 = { 0 };
	int rc;

	net_ipaddr_copy(&my_addr4.sin_addr, &client_addr);
	my_addr4.sin_family = AF_INET;
	my_addr4.sin_port = htons(CLIENT_PORT);

	rc = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &udp_ctx);
	if (rc < 0) {
		printk("Cannot get network context for IPv4 UDP (%d)", rc);
		return -EIO;
	}

	rc = net_context_bind(udp_ctx, (struct sockaddr *)&my_addr4,
			      sizeof(struct sockaddr_in));
	if (rc < 0) {
		printk("Cannot bind IPv4 UDP port %d (%d)", CLIENT_PORT, rc);
		goto error;
	}

	ctx->rx_nbuf = NULL;
	ctx->remaining = 0;
	ctx->net_ctx = udp_ctx;

	return 0;

error:
	net_context_put(udp_ctx);
	return -EINVAL;
}
