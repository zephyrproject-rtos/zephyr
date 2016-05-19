/** @file
 * @brief ICMPv4 related functions
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

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_ICMPV4
#define SYS_LOG_DOMAIN "net/icmpv4"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <misc/slist.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_stats.h>
#include "net_private.h"
#include "icmpv4.h"

static inline enum net_verdict handle_echo_request(struct net_buf *buf)
{
	/* Note that we send the same data buffers back and just swap
	 * the addresses etc.
	 */
	struct in_addr addr;

#if NET_DEBUG > 0
	char out[sizeof("xxx.xxx.xxx.xxx")];

	snprintf(out, sizeof(out),
		 net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst));
	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src), out);
#endif /* NET_DEBUG > 0 */

	net_ipaddr_copy(&addr, &NET_IPV4_BUF(buf)->src);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->src,
			&NET_IPV4_BUF(buf)->dst);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, &addr);

	NET_ICMP_BUF(buf)->type = NET_ICMPV4_ECHO_REPLY;
	NET_ICMP_BUF(buf)->code = 0;
	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv4(buf);

#if NET_DEBUG > 0
	snprintf(out, sizeof(out),
		 net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst));
	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src), out);
#endif /* NET_DEBUG > 0 */

	if (net_send_data(buf) < 0) {
		NET_STATS(++net_stats.icmp.drop);
		return NET_DROP;
	}

	NET_STATS(++net_stats.icmp.sent);

	return NET_OK;
}

enum net_verdict net_icmpv4_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code)
{
	switch (type) {
	case NET_ICMPV4_ECHO_REQUEST:
		return handle_echo_request(buf);
	}

	return NET_DROP;
}
