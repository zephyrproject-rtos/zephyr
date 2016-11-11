/*
 * Copyright (c) 2016 Intel Corporation.
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

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/nbuf.h>

static inline enum net_verdict dummy_recv(struct net_if *iface,
					  struct net_buf *buf)
{
	net_nbuf_ll_src(buf)->addr = NULL;
	net_nbuf_ll_src(buf)->len = 0;
	net_nbuf_ll_dst(buf)->addr = NULL;
	net_nbuf_ll_dst(buf)->len = 0;

	return NET_CONTINUE;
}

static inline enum net_verdict dummy_send(struct net_if *iface,
					  struct net_buf *buf)
{
	net_if_queue_tx(iface, buf);

	return NET_OK;
}

static inline uint16_t dummy_reserve(struct net_if *iface, void *unused)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(unused);

	return 0;
}

NET_L2_INIT(DUMMY_L2, dummy_recv, dummy_send, dummy_reserve);
