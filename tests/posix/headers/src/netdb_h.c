/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <netdb.h>
#else
#include <zephyr/posix/netdb.h>
#endif

/**
 * @brief existence test for `<netdb.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/netdb.h.html>netdb.h</a>
 */
ZTEST(posix_headers, test_netdb_h)
{
	zassert_not_equal(-1, offsetof(struct hostent, h_name));
	zassert_not_equal(-1, offsetof(struct hostent, h_aliases));
	zassert_not_equal(-1, offsetof(struct hostent, h_addrtype));
	zassert_not_equal(-1, offsetof(struct hostent, h_length));
	zassert_not_equal(-1, offsetof(struct hostent, h_addr_list));

	zassert_not_equal(-1, offsetof(struct netent, n_name));
	zassert_not_equal(-1, offsetof(struct netent, n_aliases));
	zassert_not_equal(-1, offsetof(struct netent, n_addrtype));
	zassert_not_equal(-1, offsetof(struct netent, n_net));

	zassert_not_equal(-1, offsetof(struct protoent, p_name));
	zassert_not_equal(-1, offsetof(struct protoent, p_aliases));
	zassert_not_equal(-1, offsetof(struct protoent, p_proto));

	zassert_not_equal(-1, offsetof(struct servent, s_name));
	zassert_not_equal(-1, offsetof(struct servent, s_aliases));
	zassert_not_equal(-1, offsetof(struct servent, s_port));
	zassert_not_equal(-1, offsetof(struct servent, s_proto));

	/* zassert_equal(IPPORT_RESERVED, UINT16_MAX); */ /* not implemented */

	zassert_not_equal(-1, offsetof(struct addrinfo, ai_flags));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_family));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_socktype));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_protocol));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_addrlen));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_addr));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_canonname));
	zassert_not_equal(-1, offsetof(struct addrinfo, ai_next));

	zassert_not_equal(-1, AI_PASSIVE);
	zassert_not_equal(-1, AI_CANONNAME);
	zassert_not_equal(-1, AI_NUMERICHOST);
	zassert_not_equal(-1, AI_NUMERICSERV);
	zassert_not_equal(-1, AI_V4MAPPED);
	zassert_not_equal(-1, AI_ALL);
	zassert_not_equal(-1, AI_ADDRCONFIG);

	zassert_not_equal(-1, NI_NOFQDN);
	zassert_not_equal(-1, NI_NUMERICHOST);
	zassert_not_equal(-1, NI_NAMEREQD);
	zassert_not_equal(-1, NI_NUMERICSERV);
	/* zassert_not_equal(-1, NI_NUMERICSCOPE); */ /* not implemented */
	zassert_not_equal(-1, NI_DGRAM);

	zassert_not_equal(-1, EAI_AGAIN);
	zassert_equal(-1, EAI_BADFLAGS);
	zassert_not_equal(-1, EAI_FAIL);
	zassert_not_equal(-1, EAI_FAMILY);
	zassert_not_equal(-1, EAI_MEMORY);
	zassert_not_equal(-1, EAI_NONAME);
	zassert_not_equal(-1, EAI_SERVICE);
	zassert_not_equal(-1, EAI_SOCKTYPE);
	zassert_not_equal(-1, EAI_SYSTEM);
	zassert_not_equal(-1, EAI_OVERFLOW);

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(endhostent);
		zassert_not_null(endnetent);
		zassert_not_null(endprotoent);
		zassert_not_null(endservent);
		zassert_not_null(freeaddrinfo);
		zassert_not_null(gai_strerror);
		zassert_not_null(getaddrinfo);
		zassert_not_null(gethostent);
		zassert_not_null(getnameinfo);
		zassert_not_null(getnetbyaddr);
		zassert_not_null(getnetbyname);
		zassert_not_null(getnetent);
		zassert_not_null(getprotobyname);
		zassert_not_null(getprotobynumber);
		zassert_not_null(getprotoent);
		zassert_not_null(getservbyname);
		zassert_not_null(getservbyport);
		zassert_not_null(getservent);
		zassert_not_null(sethostent);
		zassert_not_null(setnetent);
		zassert_not_null(setprotoent);
		zassert_not_null(setservent);
	}
}
