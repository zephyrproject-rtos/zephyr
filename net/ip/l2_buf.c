/** @file
 @brief Network buffers for IP stack

 IP data is passed between application and IP stack via
 a net_buf struct.
 */

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

#include <zephyr.h>
#include <toolchain.h>
#include <string.h>
#include <stdint.h>

#include <net/net_core.h>
#include <net/buf.h>
#include <net/l2_buf.h>
#include <net/net_ip.h>

#include "ip/uip.h"

#if !defined(CONFIG_NETWORK_IP_STACK_DEBUG_NET_BUF)
#undef NET_DBG
#define NET_DBG(...)
#endif

/* Available (free) layer 2 (MAC/L2) buffers queue */
#ifndef NET_NUM_L2_BUFS
/* Default value is 13 (receiving side) which means that max. UDP data
 * (1232 bytes) can be received in one go. In sending side we need 1
 * mbuf + some extras.
 */
#define NET_NUM_L2_BUFS		16
#endif

#ifdef DEBUG_L2_BUFS
static int num_free_l2_bufs = NET_NUM_L2_BUFS;

static inline void dec_free_l2_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_l2_bufs--;
	if (num_free_l2_bufs < 0) {
		NET_DBG("*** ERROR *** Invalid L2 buffer count.\n");
		num_free_l2_bufs = 0;
	}
}

static inline void inc_free_l2_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_l2_bufs++;
}

static inline int get_free_l2_bufs(void)
{
	return num_free_l2_bufs;
}

#define inc_free_l2_bufs_func inc_free_l2_bufs

#else
#define dec_free_l2_bufs(...)
#define inc_free_l2_bufs(...)
#define get_free_l2_bufs(...)
#define inc_free_l2_bufs_func(...)
#endif

static struct nano_fifo free_l2_bufs;

static inline void free_l2_bufs_func(struct net_buf *buf)
{
	inc_free_l2_bufs_func(buf);

	nano_fifo_put(buf->free, buf);
}

static NET_BUF_POOL(l2_buffers, NET_NUM_L2_BUFS, NET_L2_BUF_MAX_SIZE, \
		    &free_l2_bufs, free_l2_bufs_func, \
		    sizeof(struct l2_buf));

#ifdef DEBUG_L2_BUFS
struct net_buf *l2_buf_get_reserve_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_buf *l2_buf_get_reserve(uint16_t reserve_head)
#endif
{
	struct net_buf *buf;

	buf = net_buf_get(&free_l2_bufs, reserve_head);
	if (!buf) {
#ifdef DEBUG_L2_BUFS
		NET_ERR("Failed to get free L2 buffer (%s():%d)\n",
			caller, line);
#else
		NET_ERR("Failed to get free L2 buffer\n");
#endif
		return NULL;
	}

	dec_free_l2_bufs(buf);

	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

#ifdef DEBUG_L2_BUFS
	NET_DBG("[%d] buf %p reserve %u ref %d (%s():%d)\n",
		get_free_l2_bufs(), buf, reserve_head, buf->ref,
		caller, line);
#else
	NET_DBG("buf %p reserve %u ref %d\n", buf, reserve_head, buf->ref);
#endif

	packetbuf_clear(buf);
#if defined(CONFIG_NETWORKING_WITH_15_4)
	LIST_STRUCT_INIT(((struct l2_buf *)net_buf_user_data((buf))), neighbor_list);
#endif

	return buf;
}

#ifdef DEBUG_L2_BUFS
void l2_buf_unref_debug(struct net_buf *buf, const char *caller, int line)
#else
void l2_buf_unref(struct net_buf *buf)
#endif
{
       if (!buf) {
#ifdef DEBUG_L2_BUFS
               NET_DBG("*** ERROR *** buf %p (%s():%d)\n", buf, caller, line);
#else
               NET_DBG("*** ERROR *** buf %p\n", buf);
#endif
               return;
       }

#ifdef DEBUG_L2_BUFS
       NET_DBG("[%d] buf %p ref %d (%s():%d)\n",
	       get_free_l2_bufs() + 1, buf, buf->ref, caller, line);
#else
       NET_DBG("buf %p ref %d\n", buf, buf->ref);
#endif

       net_buf_unref(buf);
}

void l2_buf_init(void)
{
	NET_DBG("Allocating %d L2 buffers\n", NET_NUM_L2_BUFS);

	net_buf_pool_init(l2_buffers);
}
