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
#include <zephyr/types.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_core.h>

char *net_sprint_addr(sa_family_t af, const void *addr)
{
#define NBUFS 3
	static char buf[NBUFS][NET_IPV6_ADDR_LEN];
	static int i;
	char *s = buf[++i % NBUFS];

	return net_addr_ntop(af, addr, s, NET_IPV6_ADDR_LEN);
}

const char *net_proto2str(enum net_ip_protocol proto)
{
	switch (proto) {
	case IPPROTO_ICMP:
		return "ICMPv4";
	case IPPROTO_TCP:
		return "TCP";
	case IPPROTO_UDP:
		return "UDP";
	case IPPROTO_ICMPV6:
		return "ICMPv6";
	default:
		break;
	}

	return "UNK_PROTO";
}

char *net_byte_to_hex(char *ptr, u8_t byte, char base, bool pad)
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

char *net_sprint_ll_addr_buf(const u8_t *ll, u8_t ll_len,
			     char *buf, int buflen)
{
	u8_t i, len, blen;
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

static int net_value_to_udec(char *buf, u32_t value, int precision)
{
	u32_t divisor;
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
	u16_t *w;
	u8_t i, bl, bh, longest = 1;
	s8_t pos = -1;
	char delim = ':';
	unsigned char zeros[8] = { 0 };
	char *ptr = dst;
	int len = -1;
	u16_t value;
	bool needcolon = false;

	if (family == AF_INET6) {
		addr6 = (struct in6_addr *)src;
		w = (u16_t *)addr6->s6_addr16;
		len = 8;

		for (i = 0; i < 8; i++) {
			u8_t j;

			for (j = i; j < 8; j++) {
				if (UNALIGNED_GET(&w[j]) != 0) {
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
			u8_t l;

			value = (u32_t)addr->s4_addr[i];

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

		value = (u32_t)sys_be16_to_cpu(UNALIGNED_GET(&w[i]));
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

		(void)memset(addr, 0, sizeof(struct in_addr));

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
				UNALIGNED_PUT(htons(strtol(src, NULL, 16)),
					      &addr->s6_addr16[i]);
				src = strchr(src, ':');
				if (src) {
					src++;
				} else {
					if (i < expected_groups - 1) {
						return -EINVAL;
					}
				}

				continue;
			}

			/* Two colons in a row */

			for (; i < expected_groups; i++) {
				UNALIGNED_PUT(0, &addr->s6_addr16[i]);
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

				if (i < 0) {
					return -EINVAL;
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
				if (src) {
					src++;
				} else {
					if (i < 3) {
						return -EINVAL;
					}
				}
			}
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static u16_t calc_chksum(u16_t sum, const u8_t *ptr, u16_t len)
{
	u16_t tmp;
	const u8_t *end;

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

static inline u16_t calc_chksum_pkt(u16_t sum, struct net_pkt *pkt,
				    u16_t upper_layer_len)
{
	u16_t proto_len = net_pkt_ip_hdr_len(pkt) +
		net_pkt_ipv6_ext_len(pkt);
	struct net_buf *frag;
	u16_t offset;
	s16_t len;
	u8_t *ptr;

	ARG_UNUSED(upper_layer_len);

	frag = net_frag_skip(pkt->frags, proto_len, &offset, 0);
	if (!frag) {
		NET_DBG("Trying to read past pkt len (proto len %d)",
			proto_len);
		return 0;
	}

	NET_ASSERT(offset <= frag->len);

	ptr = frag->data + offset;
	len = frag->len - offset;

	while (frag) {
		sum = calc_chksum(sum, ptr, len);
		frag = frag->frags;
		if (!frag) {
			break;
		}

		ptr = frag->data;

		/* Do we need to take first byte from next fragment */
		if (len % 2) {
			u16_t tmp = *ptr;
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

u16_t net_calc_chksum(struct net_pkt *pkt, u8_t proto)
{
	u16_t upper_layer_len;
	u16_t sum = 0;

	switch (net_pkt_family(pkt)) {
#if defined(CONFIG_NET_IPV4)
	case AF_INET:
		upper_layer_len = ntohs(NET_IPV4_HDR(pkt)->len) -
			net_pkt_ipv6_ext_len(pkt) -
			net_pkt_ip_hdr_len(pkt);

		if (proto != IPPROTO_ICMP) {
			sum = calc_chksum(upper_layer_len + proto,
					  (u8_t *)&NET_IPV4_HDR(pkt)->src,
					  2 * sizeof(struct in_addr));
		}
		break;
#endif
#if defined(CONFIG_NET_IPV6)
	case AF_INET6:
		upper_layer_len = ntohs(NET_IPV6_HDR(pkt)->len) -
			net_pkt_ipv6_ext_len(pkt);
		sum = calc_chksum(upper_layer_len + proto,
				  (u8_t *)&NET_IPV6_HDR(pkt)->src,
				  2 * sizeof(struct in6_addr));
		break;
#endif
	default:
		NET_DBG("Unknown protocol family %d", net_pkt_family(pkt));
		return 0;
	}

	sum = calc_chksum_pkt(sum, pkt, upper_layer_len);

	sum = (sum == 0) ? 0xffff : htons(sum);

	return sum;
}

#if defined(CONFIG_NET_IPV4)
u16_t net_calc_chksum_ipv4(struct net_pkt *pkt)
{
	u16_t sum;

	sum = calc_chksum(0, (u8_t *)NET_IPV4_HDR(pkt), NET_IPV4H_LEN);

	sum = (sum == 0) ? 0xffff : htons(sum);

	return sum;
}
#endif /* CONFIG_NET_IPV4 */

/* Check if the first fragment of the packet can hold certain size
 * memory area. The start of the said area must be inside the first
 * fragment. This helper is used when checking whether various protocol
 * headers are split between two fragments.
 */
bool net_header_fits(struct net_pkt *pkt, u8_t *hdr, size_t hdr_size)
{
	if (hdr && hdr > pkt->frags->data &&
	    (hdr + hdr_size) <= (pkt->frags->data + pkt->frags->len)) {
		return true;
	}

	return false;
}

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_IPV4)
static bool convert_port(const char *buf, u16_t *port)
{
	unsigned long tmp;
	char *endptr;

	tmp = strtoul(buf, &endptr, 10);
	if ((endptr == buf && tmp == 0) ||
	    !(*buf != '\0' && *endptr == '\0') ||
	    ((unsigned long)(unsigned short)tmp != tmp)) {
		return false;
	}

	*port = tmp;

	return true;
}
#endif /* CONFIG_NET_IPV6 || CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6)
static bool parse_ipv6(const char *str, size_t str_len,
		       struct sockaddr *addr, bool has_port)
{
	char *ptr = NULL;
	struct in6_addr *addr6;
	char ipaddr[INET6_ADDRSTRLEN + 1];
	int end, len, ret, i;
	u16_t port;

	len = min(INET6_ADDRSTRLEN, str_len);

	for (i = 0; i < len; i++) {
		if (!str[i]) {
			len = i;
			break;
		}
	}

	if (has_port) {
		/* IPv6 address with port number */
		ptr = memchr(str, ']', len);
		if (!ptr) {
			return false;
		}

		end = min(len, ptr - (str + 1));
		memcpy(ipaddr, str + 1, end);
	} else {
		end = len;
		memcpy(ipaddr, str, end);
	}

	ipaddr[end] = '\0';

	addr6 = &net_sin6(addr)->sin6_addr;

	ret = net_addr_pton(AF_INET6, ipaddr, addr6);
	if (ret < 0) {
		return false;
	}

	net_sin6(addr)->sin6_family = AF_INET6;

	if (!has_port) {
		return true;
	}

	if ((ptr + 1) < (str + str_len) && *(ptr + 1) == ':') {
		len = str_len - end;

		/* Re-use the ipaddr buf for port conversion */
		memcpy(ipaddr, ptr + 2, len);
		ipaddr[len] = '\0';

		ret = convert_port(ipaddr, &port);
		if (!ret) {
			return false;
		}

		net_sin6(addr)->sin6_port = htons(port);

		NET_DBG("IPv6 host %s port %d",
			net_addr_ntop(AF_INET6, addr6,
				      ipaddr, sizeof(ipaddr) - 1),
			port);
	} else {
		NET_DBG("IPv6 host %s",
			net_addr_ntop(AF_INET6, addr6,
				      ipaddr, sizeof(ipaddr) - 1));
	}

	return true;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static bool parse_ipv4(const char *str, size_t str_len,
		       struct sockaddr *addr, bool has_port)
{
	char *ptr = NULL;
	char ipaddr[NET_IPV4_ADDR_LEN + 1];
	struct in_addr *addr4;
	int end, len, ret, i;
	u16_t port;

	len = min(NET_IPV4_ADDR_LEN, str_len);

	for (i = 0; i < len; i++) {
		if (!str[i]) {
			len = i;
			break;
		}
	}

	if (has_port) {
		/* IPv4 address with port number */
		ptr = memchr(str, ':', len);
		if (!ptr) {
			return false;
		}

		end = min(len, ptr - str);
	} else {
		end = len;
	}

	memcpy(ipaddr, str, end);
	ipaddr[end] = '\0';

	addr4 = &net_sin(addr)->sin_addr;

	ret = net_addr_pton(AF_INET, ipaddr, addr4);
	if (ret < 0) {
		return false;
	}

	net_sin(addr)->sin_family = AF_INET;

	if (!has_port) {
		return true;
	}

	memcpy(ipaddr, ptr + 1, str_len - end);
	ipaddr[str_len - end] = '\0';

	ret = convert_port(ipaddr, &port);
	if (!ret) {
		return false;
	}

	net_sin(addr)->sin_port = htons(port);

	NET_DBG("IPv4 host %s port %d",
		net_addr_ntop(AF_INET, addr4,
			      ipaddr, sizeof(ipaddr) - 1),
		port);
	return true;
}
#endif /* CONFIG_NET_IPV4 */

bool net_ipaddr_parse(const char *str, size_t str_len, struct sockaddr *addr)
{
	int i, count;

	if (!str || str_len == 0) {
		return false;
	}

	/* We cannot accept empty string here */
	if (*str == '\0') {
		return false;
	}

	if (*str == '[') {
#if defined(CONFIG_NET_IPV6)
		return parse_ipv6(str, str_len, addr, true);
#else
		return false;
#endif /* CONFIG_NET_IPV6 */
	}

	for (count = i = 0; str[i] && i < str_len; i++) {
		if (str[i] == ':') {
			count++;
		}
	}

	if (count == 1) {
#if defined(CONFIG_NET_IPV4)
		return parse_ipv4(str, str_len, addr, true);
#else
		return false;
#endif /* CONFIG_NET_IPV4 */
	}

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	if (!parse_ipv4(str, str_len, addr, false)) {
		return parse_ipv6(str, str_len, addr, false);
	}

	return true;
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	return parse_ipv4(str, str_len, addr, false);
#endif

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	return parse_ipv6(str, str_len, addr, false);
#endif
}

int net_bytes_from_str(u8_t *buf, int buf_len, const char *src)
{
	unsigned int i;
	char *endptr;

	for (i = 0; i < strlen(src); i++) {
		if (!(src[i] >= '0' && src[i] <= '9') &&
		    !(src[i] >= 'A' && src[i] <= 'F') &&
		    !(src[i] >= 'a' && src[i] <= 'f') &&
		    src[i] != ':') {
			return -EINVAL;
		}
	}

	(void)memset(buf, 0, buf_len);

	for (i = 0; i < buf_len; i++) {
		buf[i] = strtol(src, &endptr, 16);
		src = ++endptr;
	}

	return 0;
}
