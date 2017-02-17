/** @file
 * @brief Misc network utility functions
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_UTILS)
#define SYS_LOG_DOMAIN "net/utils"
#define NET_LOG_ENABLED 1
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_core.h>

char *net_byte_to_hex(char *ptr, uint8_t byte, char base, bool pad)
{
	int i, val;

	for (i = 0, val = (byte & 0xf0) >> 4; i < 2; i++, val = byte & 0x0f) {
		if (i == 0 && !pad && !val) {
			continue;
		}
		if (val < 10) {
			*ptr++ = (char) (val + '0');
		} else {
			*ptr++ = (char) (val - 10 + base);
		}
	}

	*ptr = '\0';

	return ptr;
}

char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
			     char *buf, int buflen)
{
	uint8_t i, len, blen;
	char *ptr = buf;

	switch (ll_len) {
	case 8:
		len = 8;
		break;
	case 6:
		len = 6;
		break;
	default:
		len = 6;
		break;
	}

	for (i = 0, blen = buflen; i < len && blen > 0; i++) {
		ptr = net_byte_to_hex(ptr, (char)ll[i], 'A', true);
		*ptr++ = ':';
		blen -= 3;
	}

	if (!(ptr - buf)) {
		return NULL;
	}

	*(ptr - 1) = '\0';
	return buf;
}

static int net_value_to_udec(char *buf, uint32_t value, int precision)
{
	uint32_t divisor;
	int i;
	int temp;
	char *start = buf;

	divisor = 1000000000;
	if (precision < 0)
		precision = 1;
	for (i = 9; i >= 0; i--, divisor /= 10) {
		temp = value / divisor;
		value = value % divisor;
		if ((precision > i) || (temp != 0)) {
			precision = i;
			*buf++ = (char) (temp + '0');
		}
	}
	*buf = 0;

	return buf - start;
}

char *net_addr_ntop(sa_family_t family, const void *src,
		    char *dst, size_t size)
{
	struct in_addr *addr;
	struct in6_addr *addr6;
	uint16_t *w;
	uint8_t i, bl, bh, longest = 1;
	int8_t pos = -1;
	char delim = ':';
	unsigned char zeros[8] = { 0 };
	char *ptr = dst;
	int len = -1;
	uint16_t value;
	bool needcolon = false;

	if (family == AF_INET6) {
		addr6 = (struct in6_addr *)src;
		w = (uint16_t *)addr6->s6_addr16;
		len = 8;

		for (i = 0; i < 8; i++) {
			uint8_t j;

			for (j = i; j < 8; j++) {
				if (w[j] != 0) {
					break;
				}

				zeros[i]++;
			}
		}

		for (i = 0; i < 8; i++) {
			if (zeros[i] > longest) {
				longest = zeros[i];
				pos = i;
			}
		}

		if (longest == 1) {
			pos = -1;
		}

	} else if (family == AF_INET) {
		addr = (struct in_addr *)src;
		len = 4;
		delim = '.';
	} else {
		return NULL;
	}

	for (i = 0; i < len; i++) {
		/* IPv4 address a.b.c.d */
		if (len == 4) {
			uint8_t l;

			value = (uint32_t)addr->s4_addr[i];

			/* net_byte_to_udec() eats 0 */
			if (value == 0) {
				*ptr++ = '0';
				*ptr++ = delim;
				continue;
			}

			l = net_value_to_udec(ptr, value, 0);

			ptr += l;
			*ptr++ = delim;

			continue;
		}

		/* IPv6 address */
		if (i == pos) {
			if (needcolon || i == 0) {
				*ptr++ = ':';
			}

			*ptr++ = ':';
			needcolon = false;
			i += longest - 1;

			continue;
		}

		if (needcolon) {
			*ptr++ = ':';
			needcolon = false;
		}

		value = (uint32_t)sys_be16_to_cpu(w[i]);
		bh = value >> 8;
		bl = value & 0xff;

		if (bh) {
			if (bh > 0x0f) {
				ptr = net_byte_to_hex(ptr, bh, 'a', false);
			} else {
				if (bh < 10) {
					*ptr++ = (char)(bh + '0');
				} else {
					*ptr++ = (char) (bh - 10 + 'a');
				}
			}

			ptr = net_byte_to_hex(ptr, bl, 'a', true);
		} else if (bl > 0x0f) {
			ptr = net_byte_to_hex(ptr, bl, 'a', false);
		} else {
			if (bl < 10) {
				*ptr++ = (char)(bl + '0');
			} else {
				*ptr++ = (char) (bl - 10 + 'a');
			}
		}

		needcolon = true;
	}

	if (!(ptr - dst)) {
		return NULL;
	}

	if (family == AF_INET) {
		*(ptr - 1) = '\0';
	} else {
		*ptr = '\0';
	}

	return dst;
}

int net_addr_pton(sa_family_t family, const char *src,
		  void *dst)
{
	if (family == AF_INET) {
		struct in_addr *addr = (struct in_addr *)dst;
		size_t i, len;

		len = strlen(src);
		for (i = 0; i < len; i++) {
			if (!(src[i] >= '0' && src[i] <= '9') &&
			    src[i] != '.') {
				return -EINVAL;
			}
		}

		memset(addr, 0, sizeof(struct in_addr));

		for (i = 0; i < sizeof(struct in_addr); i++) {
			char *endptr;

			addr->s4_addr[i] = strtol(src, &endptr, 10);

			src = ++endptr;
		}

	} else if (family == AF_INET6) {
		/* If the string contains a '.', it means it's of the form
		 * X:X:X:X:X:X:x.x.x.x, and contains only 6 16-bit pieces
		 */
		int expected_groups = strchr(src, '.') ? 6 : 8;
		struct in6_addr *addr = (struct in6_addr *)dst;
		int i, len;

		if (*src == ':') {
			/* Ignore a leading colon, makes parsing neater */
			src++;
		}

		len = strlen(src);
		for (i = 0; i < len; i++) {
			if (!(src[i] >= '0' && src[i] <= '9') &&
			    !(src[i] >= 'A' && src[i] <= 'F') &&
			    !(src[i] >= 'a' && src[i] <= 'f') &&
			    src[i] != '.' && src[i] != ':')
				return -EINVAL;
		}

		for (i = 0; i < expected_groups; i++) {
			char *tmp;

			if (!src || *src == '\0') {
				return -EINVAL;
			}

			if (*src != ':') {
				/* Normal IPv6 16-bit piece */
				addr->s6_addr16[i] = htons(strtol(src, NULL,
								  16));
				src = strchr(src, ':');
				if (!src && i < expected_groups - 1) {
					return -EINVAL;
				}

				src++;
				continue;
			}

			/* Two colons in a row */

			for (; i < expected_groups; i++) {
				addr->s6_addr16[i] = 0;
			}

			tmp = strrchr(src, ':');
			if (src == tmp && (expected_groups == 6 || !src[1])) {
				src++;
				break;
			}

			if (expected_groups == 6) {
				/* we need to drop the trailing
				 * colon since it's between the
				 * ipv6 and ipv4 addresses, rather than being
				 * a part of the ipv6 address
				 */
				tmp--;
			}

			/* Calculate the amount of skipped zeros */
			i = expected_groups - 1;
			do {
				if (*tmp == ':') {
					i--;
				}
			} while (tmp-- != src);

			src++;
		}

		if (expected_groups == 6) {
			/* Parse the IPv4 part */
			for (i = 0; i < 4; i++) {
				if (!src || !*src) {
					return -EINVAL;
				}

				addr->s6_addr[12 + i] = strtol(src, NULL, 10);

				src = strchr(src, '.');
				if (!src && i < 3) {
					return -EINVAL;
				}

				src++;
			}
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static uint16_t calc_chksum(uint16_t sum, const uint8_t *ptr, uint16_t len)
{
	uint16_t tmp;
	const uint8_t *end;

	end = ptr + len - 1;

	while (ptr < end) {
		tmp = (ptr[0] << 8) + ptr[1];
		sum += tmp;
		if (sum < tmp) {
			sum++;
		}
		ptr += 2;
	}

	if (ptr == end) {
		tmp = ptr[0] << 8;
		sum += tmp;
		if (sum < tmp) {
			sum++;
		}
	}

	return sum;
}

static inline uint16_t calc_chksum_buf(uint16_t sum, struct net_buf *buf,
				       uint16_t upper_layer_len)
{
	struct net_buf *frag = buf->frags;
	uint16_t proto_len = net_nbuf_ip_hdr_len(buf) + net_nbuf_ext_len(buf);
	int16_t len = frag->len - proto_len;
	uint8_t *ptr = frag->data + proto_len;

	ARG_UNUSED(upper_layer_len);

	if (len < 0) {
		NET_DBG("1st fragment len %u < IP header len %u",
			frag->len, proto_len);
		return 0;
	}

	while (frag) {
		sum = calc_chksum(sum, ptr, len);
		frag = frag->frags;
		if (!frag) {
			break;
		}

		ptr = frag->data;

		/* Do we need to take first byte from next fragment */
		if (len % 2) {
			uint16_t tmp = *ptr;
			sum += tmp;
			if (sum < tmp) {
				sum++;
			}
			len = frag->len - 1;
			ptr++;
		} else {
			len = frag->len;
		}
	}

	return sum;
}

uint16_t net_calc_chksum(struct net_buf *buf, uint8_t proto)
{
	uint16_t upper_layer_len;
	uint16_t sum;

	switch (net_nbuf_family(buf)) {
#if defined(CONFIG_NET_IPV4)
	case AF_INET:
		upper_layer_len = (NET_IPV4_BUF(buf)->len[0] << 8) +
			NET_IPV4_BUF(buf)->len[1] -
			net_nbuf_ext_len(buf) -
			net_nbuf_ip_hdr_len(buf);

		if (proto == IPPROTO_ICMP) {
			return htons(calc_chksum(0, net_nbuf_ip_data(buf) +
						 net_nbuf_ip_hdr_len(buf),
						 upper_layer_len));
		} else {
			sum = calc_chksum(upper_layer_len + proto,
					  (uint8_t *)&NET_IPV4_BUF(buf)->src,
					  2 * sizeof(struct in_addr));
		}
		break;
#endif
#if defined(CONFIG_NET_IPV6)
	case AF_INET6:
		upper_layer_len = (NET_IPV6_BUF(buf)->len[0] << 8) +
			NET_IPV6_BUF(buf)->len[1] - net_nbuf_ext_len(buf);
		sum = calc_chksum(upper_layer_len + proto,
				  (uint8_t *)&NET_IPV6_BUF(buf)->src,
				  2 * sizeof(struct in6_addr));
		break;
#endif
	default:
		NET_DBG("Unknown protocol family %d", net_nbuf_family(buf));
		return 0;
	}

	sum = calc_chksum_buf(sum, buf, upper_layer_len);

	sum = (sum == 0) ? 0xffff : htons(sum);

	return sum;
}

#if defined(CONFIG_NET_IPV4)
uint16_t net_calc_chksum_ipv4(struct net_buf *buf)
{
	uint16_t sum;

	sum = calc_chksum(0, (uint8_t *)NET_IPV4_BUF(buf), NET_IPV4H_LEN);

	sum = (sum == 0) ? 0xffff : htons(sum);

	return sum;
}
#endif /* CONFIG_NET_IPV4 */
