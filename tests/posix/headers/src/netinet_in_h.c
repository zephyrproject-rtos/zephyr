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

/**
 * @brief existence test for `<netinet/in.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/netinet_in.h.html">netinet/in.h</a>
 */
ZTEST(posix_headers, test_netinet_in_h)
{
	zexpect_equal(sizeof(in_port_t), sizeof(uint16_t));
	zexpect_equal(sizeof(in_addr_t), sizeof(uint32_t));

	zexpect_not_equal(-1, offsetof(struct in_addr, s_addr));

	zexpect_not_equal(-1, offsetof(struct sockaddr_in, sin_family));
	zexpect_not_equal(-1, offsetof(struct sockaddr_in, sin_port));
	zexpect_not_equal(-1, offsetof(struct sockaddr_in, sin_addr));

	zexpect_not_equal(-1, offsetof(struct in6_addr, s6_addr));
	zexpect_equal(sizeof(((struct in6_addr *)NULL)->s6_addr), 16 * sizeof(uint8_t));

	zexpect_not_equal(-1, offsetof(struct sockaddr_in6, sin6_family));
	zexpect_not_equal(-1, offsetof(struct sockaddr_in6, sin6_port));
	/* not implemented */
	/* zexpect_not_equal(-1, offsetof(struct sockaddr_in6, sin6_flowinfo)); */
	zexpect_not_equal(-1, offsetof(struct sockaddr_in6, sin6_addr));
	zexpect_not_equal(-1, offsetof(struct sockaddr_in6, sin6_scope_id));

	zexpect_not_null(&in6addr_any);
	zexpect_not_null(&in6addr_loopback);
	struct in6_addr any6 = IN6ADDR_ANY_INIT;
	struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;

	struct in6_addr mcast6 = { { { 0xff, 0 } } };
	struct in6_addr ll6 = { { { 0xfe, 0x80, 0x01, 0x02,
				0, 0, 0, 0, 0, 0x01 } } };
	struct in6_addr sl6  = { { { 0xfe, 0xc0, 0, 0x01, 0x02 } } };
	struct in6_addr v4mapped = { { { 0, 0, 0, 0, 0, 0, 0, 0,
				0xff, 0xff, 0xff, 0xff, 0xc0, 0, 0x02, 0x01 } } };
	struct in6_addr mcnl6 = { { { 0xff, 0x01, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct in6_addr mcll6 = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct in6_addr mcsl6 = { { { 0xff, 0x05, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct in6_addr mcol6 = { { { 0xff, 0x08, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x01 } } };
	struct in6_addr mcg6 = { { { 0xff, 0x0e, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x01 } } };

	zexpect_not_equal(-1, offsetof(struct ipv6_mreq, ipv6mr_multiaddr));
	zexpect_not_equal(-1, offsetof(struct ipv6_mreq, ipv6mr_ifindex));

	zexpect_not_equal(-1, IPPROTO_IP);
	zexpect_not_equal(-1, IPPROTO_IPV6);
	zexpect_not_equal(-1, IPPROTO_ICMP);
	zexpect_not_equal(-1, IPPROTO_RAW);
	zexpect_not_equal(-1, IPPROTO_TCP);
	zexpect_not_equal(-1, IPPROTO_UDP);

	zexpect_not_equal(-1, INADDR_ANY);
	zexpect_equal(0xffffffff, INADDR_BROADCAST);

	zexpect_equal(INET_ADDRSTRLEN, 16);
	zexpect_equal(INET6_ADDRSTRLEN, 46);

	zexpect_equal(IPV6_ADD_MEMBERSHIP, IPV6_JOIN_GROUP);
	zexpect_equal(IPV6_DROP_MEMBERSHIP, IPV6_LEAVE_GROUP);
	zexpect_not_equal(-1, IPV6_MULTICAST_HOPS);
	/* zexpect_not_equal(-1, IPV6_MULTICAST_IF); */ /* not implemented */
	/* zexpect_not_equal(-1, IPV6_MULTICAST_LOOP); */ /* not implemented */
	zexpect_not_equal(-1, IPV6_UNICAST_HOPS);
	zexpect_not_equal(-1, IPV6_V6ONLY);

	zexpect_true(IN6_IS_ADDR_UNSPECIFIED(&any6));
	zexpect_true(IN6_IS_ADDR_LOOPBACK(&lo6));

	zexpect_true(IN6_IS_ADDR_MULTICAST(&mcast6));
	zexpect_true(IN6_IS_ADDR_LINKLOCAL(&ll6));
	zexpect_true(IN6_IS_ADDR_SITELOCAL(&sl6));
	zexpect_true(IN6_IS_ADDR_V4MAPPED(&v4mapped));
	/* IN6_IS_ADDR_V4COMPAT(&lo6); */ /* not implemented */
	zexpect_true(IN6_IS_ADDR_MC_NODELOCAL(&mcnl6));
	zexpect_true(IN6_IS_ADDR_MC_LINKLOCAL(&mcll6));
	zexpect_true(IN6_IS_ADDR_MC_SITELOCAL(&mcsl6));
	zexpect_true(IN6_IS_ADDR_MC_ORGLOCAL(&mcol6));
	zexpect_true(IN6_IS_ADDR_MC_GLOBAL(&mcg6));
}
