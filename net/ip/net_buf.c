/** @file
 @brief Network buffers

 Network data is passed between application and IP stack via
 a net_buf struct.
 */

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
#include <stdint.h>

#include <net/net_core.h>
#include <net/net_buf.h>
#include <net/net_ip.h>

#include "ip/uip.h"

extern struct net_tuple *net_context_get_tuple(struct net_context *context);

/* Available (free) buffers queue */
#ifndef NET_NUM_BUFS
#define NET_NUM_BUFS		2
#endif
static struct net_buf		buffers[NET_NUM_BUFS];
static struct nano_fifo		free_bufs;

/* Available (free) MAC buffers queue */
#ifndef NET_NUM_MAC_BUFS
/* Default value is 13 (receiving side) which means that max. UDP data
 * (1232 bytes) can be received in one go. In sending side we need 1
 * mbuf + some extras.
 */
#define NET_NUM_MAC_BUFS		16
#endif
static struct net_mbuf		mac_buffers[NET_NUM_MAC_BUFS];
static struct nano_fifo		free_mbufs;

#ifdef DEBUG_NET_BUFS
struct net_buf *net_buf_get_reserve_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_buf *net_buf_get_reserve(uint16_t reserve_head)
#endif
{
	struct net_buf *buf;

	buf = nano_fifo_get(&free_bufs);
	if (!buf) {
#ifdef DEBUG_NET_BUFS
		NET_ERR("Failed to get free buffer (%s():%d)\n", caller, line);
#else
		NET_ERR("Failed to get free buffer\n");
#endif
		return NULL;
	}

	buf->data = buf->buf + reserve_head;
	buf->len = 0;

	NET_BUF_CHECK_IF_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("buf %p reserve %u inuse %d (%s():%d)\n", buf, reserve_head,
		buf->in_use, caller, line);
#else
	NET_DBG("buf %p reserve %u inuse %d\n", buf, reserve_head, buf->in_use);
#endif
	buf->in_use = true;

	return buf;
}

#ifdef DEBUG_NET_BUFS
struct net_buf *net_buf_get_debug(struct net_context *context, const char *caller, int line)
#else
struct net_buf *net_buf_get(struct net_context *context)
#endif
{
	struct net_buf *buf;
	struct net_tuple *tuple;
	uint16_t reserve = 0;

	tuple = net_context_get_tuple(context);
	if (!tuple) {
		return NULL;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		reserve = UIP_IPUDPH_LEN;
		break;
	case IPPROTO_TCP:
		reserve = UIP_IPTCPH_LEN;
		break;
	case IPPROTO_ICMPV6:
		reserve = UIP_IPICMPH_LEN;
		break;
	}

#ifdef DEBUG_NET_BUFS
	buf = net_buf_get_reserve_debug(reserve, caller, line);
#else
	buf = net_buf_get_reserve(reserve);
#endif
	if (!buf) {
		return buf;
	}

	buf->context = context;

	return buf;
}

#ifdef DEBUG_NET_BUFS
void net_buf_put_debug(struct net_buf *buf, const char *caller, int line)
#else
void net_buf_put(struct net_buf *buf)
#endif
{
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("buf %p inuse %d (%s():%d)\n", buf, buf->in_use, caller, line);
#else
	NET_DBG("buf %p inuse %d\n", buf, buf->in_use);
#endif

	buf->in_use = false;

	nano_fifo_put(&free_bufs, buf);
}

uint8_t *net_buf_add(struct net_buf *buf, uint16_t len)
{
	uint8_t *tail = buf->data + buf->len;
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);
	buf->len += len;
	return tail;
}

uint8_t *net_buf_push(struct net_buf *buf, uint16_t len)
{
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);
	buf->data -= len;
	buf->len += len;
	return buf->data;
}

uint8_t *net_buf_pull(struct net_buf *buf, uint16_t len)
{
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);
	buf->len -= len;
	return buf->data += len;
}

struct net_mbuf *net_mbuf_get_reserve(uint16_t reserve_head)
{
	struct net_mbuf *buf;

	buf = nano_fifo_get(&free_mbufs);
	if (!buf) {
		NET_ERR("Failed to get free mac buffer\n");
		return NULL;
	}

	NET_BUF_CHECK_IF_IN_USE(buf);

	NET_DBG("buf %p reserve %u inuse %d\n", buf, reserve_head, buf->in_use);

	buf->in_use = true;

	return buf;
}

void net_mbuf_put(struct net_mbuf *buf)
{
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

	NET_DBG("buf %p inuse %d\n", buf, buf->in_use);

	buf->in_use = false;

	nano_fifo_put(&free_mbufs, buf);
}

static void net_mbuf_init(void)
{
	nano_fifo_init(&free_mbufs);

	for (int i = 0; i < NET_NUM_MAC_BUFS; i++) {
		nano_fifo_put(&free_mbufs, &mac_buffers[i]);
	}
}

void net_buf_init(void)
{
	nano_fifo_init(&free_bufs);

	for (int i = 0; i < NET_NUM_BUFS; i++) {
		nano_fifo_put(&free_bufs, &buffers[i]);
	}

	net_mbuf_init();
}
