/** @file
 * @brief IPv4 related functions
 */

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

#ifdef CONFIG_NET_DEBUG_IPV4
#define SYS_LOG_DOMAIN "net/ipv4"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include "net_private.h"
#include "ipv4.h"

struct net_buf *net_ipv4_create(struct net_context *context,
				struct net_buf *buf,
				const struct in_addr *addr)
{
	struct net_buf *header;

	header = net_nbuf_get_data(context);

	net_buf_frag_insert(buf, header);

	NET_IPV4_BUF(buf)->vhl = 0x45;
	NET_IPV4_BUF(buf)->tos = 0x00;
	NET_IPV4_BUF(buf)->proto = 0;

	NET_IPV4_BUF(buf)->ttl =
		net_if_ipv4_get_ttl(net_context_get_iface(context));
	NET_IPV4_BUF(buf)->offset[0] = NET_IPV4_BUF(buf)->offset[1] = 0;
	NET_IPV4_BUF(buf)->id[0] = NET_IPV4_BUF(buf)->id[1] = 0;

	net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, addr);

	NET_ASSERT(((struct sockaddr_in_ptr *)&context->local)->sin_addr);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->src,
		((struct sockaddr_in_ptr *)&context->local)->sin_addr);

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		NET_IPV4_BUF(buf)->proto = IPPROTO_UDP;
	}
#endif /* CONFIG_NET_UDP */

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));

	net_buf_add(header, sizeof(struct net_ipv4_hdr));

	return buf;
}

struct net_buf *net_ipv4_finalize(struct net_context *context,
				  struct net_buf *buf)
{
	/* Set the length of the IPv4 header */
	size_t total_len;

	net_nbuf_compact(buf);

	total_len = net_buf_frags_len(buf->frags);

	NET_IPV4_BUF(buf)->len[0] = total_len / 256;
	NET_IPV4_BUF(buf)->len[1] = total_len - NET_IPV4_BUF(buf)->len[0] * 256;

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		NET_UDP_BUF(buf)->chksum = 0;
		NET_UDP_BUF(buf)->chksum = ~net_calc_chksum_udp(buf);
	}
#endif

	return buf;
}
