/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>

#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netinet/in.h>

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
