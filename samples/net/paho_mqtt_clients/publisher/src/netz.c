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

#include <netz.h>

#include <net/net_core.h>
#include <net/net_socket.h>

#include <string.h>
#include <errno.h>

void netz_host(struct netz_ctx_t *ctx, struct net_addr *host)
{
	return netz_host_ipv4(ctx, host->in_addr.in4_u.u4_addr8[0],
				   host->in_addr.in4_u.u4_addr8[1],
				   host->in_addr.in4_u.u4_addr8[2],
				   host->in_addr.in4_u.u4_addr8[3]);
}

void netz_host_ipv4(struct netz_ctx_t *ctx, uint8_t a1, uint8_t a2,
		    uint8_t a3, uint8_t a4)
{
	uip_ipaddr_t host_addr;

	uip_ipaddr(&host_addr, a1, a2, a3, a4);
	uip_sethostaddr(&host_addr);

	ctx->host.in_addr.in4_u.u4_addr8[0] = a1;
	ctx->host.in_addr.in4_u.u4_addr8[1] = a2;
	ctx->host.in_addr.in4_u.u4_addr8[2] = a3;
	ctx->host.in_addr.in4_u.u4_addr8[3] = a4;
	ctx->host.family = AF_INET;
}

void netz_netmask(struct netz_ctx_t *ctx, struct net_addr *netmask)
{
	return netz_netmask_ipv4(ctx, netmask->in_addr.in4_u.u4_addr8[0],
				      netmask->in_addr.in4_u.u4_addr8[1],
				      netmask->in_addr.in4_u.u4_addr8[2],
				      netmask->in_addr.in4_u.u4_addr8[3]);
}

void netz_netmask_ipv4(struct netz_ctx_t *ctx, uint8_t n1, uint8_t n2,
		       uint8_t n3, uint8_t n4)
{
	ARG_UNUSED(ctx);

	uip_ipaddr_t netmask;

	uip_ipaddr(&netmask, n1, n2, n3, n4);
	uip_setnetmask(&netmask);
}

void netz_remote(struct netz_ctx_t *ctx, struct net_addr *remote, int port)
{
	return netz_remote_ipv4(ctx, remote->in_addr.in4_u.u4_addr8[0],
				     remote->in_addr.in4_u.u4_addr8[1],
				     remote->in_addr.in4_u.u4_addr8[2],
				     remote->in_addr.in4_u.u4_addr8[3], port);
}

void netz_remote_ipv4(struct netz_ctx_t *ctx, uint8_t a1, uint8_t a2,
		      uint8_t a3, uint8_t a4, int port)
{
	ctx->remote.in_addr.in4_u.u4_addr8[0] = a1;
	ctx->remote.in_addr.in4_u.u4_addr8[1] = a2;
	ctx->remote.in_addr.in4_u.u4_addr8[2] = a3;
	ctx->remote.in_addr.in4_u.u4_addr8[3] = a4;
	ctx->remote.family = AF_INET;

	ctx->remote_port = port;
}

static int netz_prepare(struct netz_ctx_t *ctx, enum ip_protocol proto)
{
#ifdef CONFIG_NETWORKING_WITH_TCP
	uint8_t data = 0;
	struct app_buf_t buf = APP_BUF_INIT(&data, 1, 1);
	int rc;
#endif

	ctx->connected = 0;
	ctx->proto = proto;

	ctx->net_ctx = net_context_get(ctx->proto,
				       &ctx->remote, ctx->remote_port,
				       &ctx->host, 0);
	if (ctx->net_ctx == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_NETWORKING_WITH_TCP
	/* workaround to activate the IP stack */
	rc = netz_tx(ctx, &buf);
	if (rc != 0) {
		return rc;
	}
#endif
	ctx->connected = 1;
	return 0;
}

int netz_tcp(struct netz_ctx_t *ctx)
{
	return netz_prepare(ctx, IPPROTO_TCP);
}

int netz_udp(struct netz_ctx_t *ctx)
{
	return netz_prepare(ctx, IPPROTO_UDP);
}

static void netz_sleep(int sleep_ticks)
{
	struct nano_timer timer;

	nano_timer_init(&timer, NULL);
	nano_fiber_timer_start(&timer, sleep_ticks);
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);
}

static int tcp_tx(struct net_context *ctx, uint8_t *buf, size_t size,
		  int tx_retry_timeout)
{
	struct net_buf *nbuf;
	uint8_t *ptr;
	int rc;

	nbuf = ip_buf_get_tx(ctx);
	if (nbuf == NULL) {
		return -EINVAL;
	}

	ptr = net_buf_add(nbuf, size);
	memcpy(ptr, buf, size);
	ip_buf_appdatalen(nbuf) = size;

	do {
		rc = net_send(nbuf);

		if (rc >= 0) {
			return 0;
		}
		switch (rc) {
		case -EINPROGRESS:
			netz_sleep(tx_retry_timeout);
			break;
		case -EAGAIN:
		case -ECONNRESET:
			netz_sleep(tx_retry_timeout);
			break;
		default:
			ip_buf_unref(nbuf);
			return -EIO;
		}
	} while (1);

	return -EIO;
}

static int tcp_rx(struct net_context *ctx, uint8_t *buf, size_t *read_bytes,
		  size_t size, int rx_timeout)
{
	struct net_buf *nbuf;
	int rc;

	nbuf = net_receive(ctx, rx_timeout);
	if (nbuf == NULL) {
		return -EIO;
	}

	*read_bytes = ip_buf_appdatalen(nbuf);
	if (*read_bytes > size) {
		*read_bytes = size;
		rc = -ENOMEM;
	} else {
		rc = 0;
	}

	memcpy(buf, ip_buf_appdata(nbuf), *read_bytes);
	ip_buf_unref(nbuf);

	return rc;
}

int netz_tx(struct netz_ctx_t *ctx, struct app_buf_t *buf)
{
	int rc;

	/* We don't evaluate if we are connected. */

	rc = tcp_tx(ctx->net_ctx, buf->buf, buf->length,
		    ctx->tx_retry_timeout);

	/* space left for debugging */
	return rc;
}

int netz_rx(struct netz_ctx_t *ctx, struct app_buf_t *buf)
{
	int rc;

	if (ctx->connected != 1) {
		return -ENOTCONN;
	}

	rc = tcp_rx(ctx->net_ctx, buf->buf, &buf->length, buf->size,
		    ctx->rx_timeout);

	/* space left for debugging */
	return rc;
}
