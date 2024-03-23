/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/net/net_if.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/net/if.h>

in_addr_t inet_addr(const char *cp)
{
	int val = 0;
	int len = 0;
	int dots = 0;
	int digits = 0;

	/* error checking */
	if (cp == NULL) {
		return -1;
	}

	for (int i = 0, subdigits = 0; i <= INET_ADDRSTRLEN; ++i, ++len) {
		if (subdigits > 3) {
			return -1;
		}
		if (cp[i] == '\0') {
			break;
		} else if (cp[i] == '.') {
			if (subdigits == 0) {
				return -1;
			}
			++dots;
			subdigits = 0;
			continue;
		} else if (isdigit((int)cp[i])) {
			++digits;
			++subdigits;
			continue;
		} else if (isspace((int)cp[i])) {
			break;
		}

		return -1;
	}

	if (dots != 3 || digits < 4) {
		return -1;
	}

	/* conversion */
	for (int i = 0, tmp = 0; i < len; ++i, ++cp) {
		if (*cp != '.') {
			tmp *= 10;
			tmp += *cp - '0';
		}

		if (*cp == '.' || i == len - 1) {
			val <<= 8;
			val |= tmp;
			tmp = 0;
		}
	}

	return htonl(val);
}

char *inet_ntoa(struct in_addr in)
{
	static char buf[INET_ADDRSTRLEN];
	unsigned char *bytes = (unsigned char *)&in.s_addr;

	snprintf(buf, sizeof(buf), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);

	return buf;
}

char *if_indextoname(unsigned int ifindex, char *ifname)
{
	int ret;

	if (!IS_ENABLED(CONFIG_NET_INTERFACE_NAME)) {
		errno = ENOTSUP;
		return NULL;
	}

	ret = net_if_get_name(net_if_get_by_index(ifindex), ifname, IF_NAMESIZE);
	if (ret < 0) {
		errno = ENXIO;
		return NULL;
	}

	return ifname;
}

void if_freenameindex(struct if_nameindex *ptr)
{
	size_t n;

	if (ptr == NULL || !IS_ENABLED(CONFIG_NET_INTERFACE_NAME)) {
		return;
	}

	NET_IFACE_COUNT(&n);

	for (size_t i = 0; i < n; ++i) {
		if (IS_ENABLED(CONFIG_NET_INTERFACE_NAME) && ptr[i].if_name != NULL) {
			free(ptr[i].if_name);
		}
	}

	free(ptr);
}

struct if_nameindex *if_nameindex(void)
{
	size_t n;
	char *name;
	struct if_nameindex *ni;

	if (!IS_ENABLED(CONFIG_NET_INTERFACE_NAME)) {
		errno = ENOTSUP;
		return NULL;
	}

	/* FIXME: would be nice to use this without malloc */
	NET_IFACE_COUNT(&n);
	ni = malloc((n + 1) * sizeof(*ni));
	if (ni == NULL) {
		goto return_err;
	}

	for (size_t i = 0; i < n; ++i) {
		ni[i].if_index = i + 1;

		ni[i].if_name = malloc(IF_NAMESIZE);
		if (ni[i].if_name == NULL) {
			goto return_err;
		}

		name = if_indextoname(i + 1, ni[i].if_name);
		__ASSERT_NO_MSG(name != NULL);
	}

	ni[n].if_index = 0;
	ni[n].if_name = NULL;

	return ni;

return_err:
	if_freenameindex(ni);
	errno = ENOBUFS;

	return NULL;
}

unsigned int if_nametoindex(const char *ifname)
{
	int ret;

	if (!IS_ENABLED(CONFIG_NET_INTERFACE_NAME)) {
		return 0;
	}

	ret = net_if_get_by_name(ifname);
	if (ret < 0) {
		return 0;
	}

	return ret;
}
