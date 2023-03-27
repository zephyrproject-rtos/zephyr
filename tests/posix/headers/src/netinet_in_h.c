/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <netinet/in.h>
#else
#include <zephyr/posix/netinet/in.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
/**
 * @brief existence test for `<netinet/in.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/netinet_in.h.html">netinet/in.h</a>
 */
ZTEST(posix_headers, test_netinet_in_h)
{
	zassert_equal(sizeof(in_port_t), sizeof(uint16_t));
	zassert_equal(sizeof(in_addr_t), sizeof(uint32_t));

	zassert_not_equal(-1, offsetof(struct in_addr, s_addr));

	zassert_not_equal(-1, offsetof(struct sockaddr_in, sin_family));
	zassert_not_equal(-1, offsetof(struct sockaddr_in, sin_port));
	zassert_not_equal(-1, offsetof(struct sockaddr_in, sin_addr));

	zassert_not_equal(-1, offsetof(struct in6_addr, s6_addr));
	zassert_equal(sizeof(((struct in6_addr *)NULL)->s6_addr), 16 * sizeof(uint8_t));

	zassert_not_equal(-1, offsetof(struct sockaddr_in6, sin6_family));
	zassert_not_equal(-1, offsetof(struct sockaddr_in6, sin6_port));
	/* not implemented */
	/* zassert_not_equal(-1, offsetof(struct sockaddr_in6, sin6_flowinfo)); */
	zassert_not_equal(-1, offsetof(struct sockaddr_in6, sin6_addr));
	/* not implemented */
	/* zassert_not_equal(-1, offsetof(struct sockaddr_in6, scope_id)); */

	zassert_not_null(&in6addr_loopback);
	struct in6_addr any6 = IN6ADDR_ANY_INIT;
	struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;

	/* not implemented */
	/* zassert_not_equal(-1, offsetof(struct ipv6_mreq, ipv6mr_multiaddr)); */
	/* not implemented */
	/* zassert_not_equal(-1, offsetof(struct ipv6_mreq, ipv6mr_interface)); */

	zassert_not_equal(-1, IPPROTO_IP);
	zassert_not_equal(-1, IPPROTO_IPV6);
	zassert_not_equal(-1, IPPROTO_ICMP);
	zassert_not_equal(-1, IPPROTO_RAW);
	zassert_not_equal(-1, IPPROTO_TCP);
	zassert_not_equal(-1, IPPROTO_UDP);

	zassert_not_equal(-1, INADDR_ANY);
	/* zassert_not_equal(-1, INADDR_BROADCAST); */ /* not implemented */

	zassert_equal(INET_ADDRSTRLEN, 16);
	zassert_equal(INET6_ADDRSTRLEN, 46);

	/* zassert_not_equal(-1, IPV6_JOIN_GROUP); */ /* not implemented */
	/* zassert_not_equal(-1, IPV6_LEAVE_GROUP); */ /* not implemented */
	/* zassert_not_equal(-1, IPV6_MULTICAST_HOPS); */ /* not implemented */
	/* zassert_not_equal(-1, IPV6_MULTICAST_IF); */ /* not implemented */
	/* zassert_not_equal(-1, IPV6_MULTICAST_LOOP); */ /* not implemented */
	/* zassert_not_equal(-1, IPV6_UNICAST_HOPS); */ /* not implemented */
	zassert_not_equal(-1, IPV6_V6ONLY);

	/* IN6_IS_ADDR_UNSPECIFIED(&any6); */ /* not implemented */
	/* IN6_IS_ADDR_LOOPBACK(&lo6); */ /* not implemented */

	/* IN6_IS_ADDR_MULTICAST(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_LINKLOCAL(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_SITELOCAL(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_V4MAPPED(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_V4COMPAT(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_MC_NODELOCAL(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_MC_LINKLOCAL(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_MC_SITELOCAL(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_MC_ORGLOCAL(&lo6); */ /* not implemented */
	/* IN6_IS_ADDR_MC_GLOBAL(&lo6); */ /* not implemented */
}
#pragma GCC diagnostic pop
