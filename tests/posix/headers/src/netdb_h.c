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
	/* zassert_not_equal(-1, offsetof(struct hostent, h_name)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct hostent, h_aliases)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct hostent, h_addrtype)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct hostent, h_length)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct hostent, h_addr_list)); */ /* not implemented */

	/* zassert_not_equal(-1, offsetof(struct netent, n_name)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct netent, n_aliases)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct netent, n_addrtype)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct netent, n_net)); */ /* not implemented */

	/* zassert_not_equal(-1, offsetof(struct protoent, p_name)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct protoent, p_aliases)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct protoent, p_proto)); */ /* not implemented */

	/* zassert_not_equal(-1, offsetof(struct servent, s_name)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct servent, s_aliases)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct servent, s_port)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct servent, s_proto)); */ /* not implemented */

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
		/* zassert_not_null(endhostent); */ /* not implemented */
		/* zassert_not_null(endnetent); */ /* not implemented */
		/* zassert_not_null(endprotoent); */ /* not implemented */
		/* zassert_not_null(endservent); */ /* not implemented */
		zassert_not_null(freeaddrinfo);
		zassert_not_null(gai_strerror);
		zassert_not_null(getaddrinfo);
		/* zassert_not_null(gethostent); */ /* not implemented */
		zassert_not_null(getnameinfo);
		/* zassert_not_null(getnetbyaddr); */ /* not implemented */
		/* zassert_not_null(getnetbyname); */ /* not implemented */
		/* zassert_not_null(getnetent); */ /* not implemented */
		/* zassert_not_null(getprotobyname); */ /* not implemented */
		/* zassert_not_null(getprotobynumber); */ /* not implemented */
		/* zassert_not_null(getprotoent); */ /* not implemented */
		/* zassert_not_null(getservbyname); */ /* not implemented */
		/* zassert_not_null(getservbyport); */ /* not implemented */
		/* zassert_not_null(getservent); */ /* not implemented */
		/* zassert_not_null(sethostent); */ /* not implemented */
		/* zassert_not_null(setnetent); */ /* not implemented */
		/* zassert_not_null(setprotoent); */ /* not implemented */
		/* zassert_not_null(setservent); */ /* not implemented */
	}
}
