/** @file
 * @brief Misc network utility functions
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_utils, CONFIG_NET_UTILS_LOG_LEVEL);

#include <kernel.h>
#include <stdlib.h>
#include <syscall_handler.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/socket_can.h>

char *net_sprint_addr(sa_family_t af, const void *addr)
{
#define NBUFS 3
	static char buf[NBUFS][NET_IPV6_ADDR_LEN];
	static int i;
	char *s = buf[++i % NBUFS];

	return net_addr_ntop(af, addr, s, NET_IPV6_ADDR_LEN);
}

const char *net_proto2str(int family, int proto)
{
	if (family == AF_INET || family == AF_INET6) {
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
	} else if (family == AF_CAN) {
		switch (proto) {
		case CAN_RAW:
			return "CAN_RAW";
		default:
			break;
		}
	}

	return "UNK_PROTO";
}

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

	if (ll == NULL) {
		return "<unknown>";
	}

	switch (ll_len) {
	case 8:
		len = 8U;
		break;
	case 6:
		len = 6U;
		break;
	case 2:
		len = 2U;
		break;
	default:
		len = 6U;
		break;
	}

	for (i = 0U, blen = buflen; i < len && blen > 0; i++) {
		ptr = net_byte_to_hex(ptr, (char)ll[i], 'A', true);
		*ptr++ = ':';
		blen -= 3U;
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

	divisor = 1000000000U;
	if (precision < 0) {
		precision = 1;
	}

	for (i = 9; i >= 0; i--, divisor /= 10U) {
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

char *z_impl_net_addr_ntop(sa_family_t family, const void *src,
			   char *dst, size_t size)
{
	struct in_addr *addr;
	struct in6_addr *addr6;
	uint16_t *w;
	uint8_t i, bl, bh, longest = 1U;
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

		for (i = 0U; i < 8; i++) {
			uint8_t j;

			for (j = i; j < 8; j++) {
				if (UNALIGNED_GET(&w[j]) != 0) {
					break;
				}

				zeros[i]++;
			}
		}

		for (i = 0U; i < 8; i++) {
			if (zeros[i] > longest) {
				longest = zeros[i];
				pos = i;
			}
		}

		if (longest == 1U) {
			pos = -1;
		}

	} else if (family == AF_INET) {
		addr = (struct in_addr *)src;
		len = 4;
		delim = '.';
	} else {
		return NULL;
	}

	for (i = 0U; i < len; i++) {
		/* IPv4 address a.b.c.d */
		if (len == 4) {
			uint8_t l;

			value = (uint32_t)addr->s4_addr[i];

			/* net_byte_to_udec() eats 0 */
			if (value == 0U) {
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
			if (needcolon || i == 0U) {
				*ptr++ = ':';
			}

			*ptr++ = ':';
			needcolon = false;
			i += longest - 1U;

			continue;
		}

		if (needcolon) {
			*ptr++ = ':';
			needcolon = false;
		}

		value = (uint32_t)sys_be16_to_cpu(UNALIGNED_GET(&w[i]));
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

#if defined(CONFIG_USERSPACE)
char *z_vrfy_net_addr_ntop(sa_family_t family, const void *src,
			   char *dst, size_t size)
{
	char str[INET6_ADDRSTRLEN];
	struct in6_addr addr6;
	struct in_addr addr4;
	char *out;
	const void *addr;

	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(dst, size));

	if (family == AF_INET) {
		Z_OOPS(z_user_from_copy(&addr4, (const void *)src,
					sizeof(addr4)));
		addr = &addr4;
	} else if (family == AF_INET6) {
		Z_OOPS(z_user_from_copy(&addr6, (const void *)src,
					sizeof(addr6)));
		addr = &addr6;
	} else {
		return 0;
	}

	out = z_impl_net_addr_ntop(family, addr, str, sizeof(str));
	if (!out) {
		return 0;
	}

	Z_OOPS(z_user_to_copy((void *)dst, str, MIN(size, sizeof(str))));

	return dst;
}
#include <syscalls/net_addr_ntop_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_net_addr_pton(sa_family_t family, const char *src,
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
			    src[i] != '.' && src[i] != ':') {
				return -EINVAL;
			}
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

#if defined(CONFIG_USERSPACE)
int z_vrfy_net_addr_pton(sa_family_t family, const char *src,
			 void *dst)
{
	char str[INET6_ADDRSTRLEN];
	struct in6_addr addr6;
	struct in_addr addr4;
	void *addr;
	size_t size;
	size_t nlen;
	int err;

	if (family == AF_INET) {
		size = sizeof(struct in_addr);
		addr = &addr4;
	} else if (family == AF_INET6) {
		size = sizeof(struct in6_addr);
		addr = &addr6;
	} else {
		return -EINVAL;
	}

	memset(str, 0, sizeof(str));

	nlen = z_user_string_nlen((const char *)src, sizeof(str), &err);
	if (err) {
		return -EINVAL;
	}

	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(dst, size));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(src, nlen));
	Z_OOPS(z_user_from_copy(str, (const void *)src, nlen));

	err = z_impl_net_addr_pton(family, str, addr);
	if (err) {
		return err;
	}

	Z_OOPS(z_user_to_copy((void *)dst, addr, size));

	return 0;
}
#include <syscalls/net_addr_pton_mrsh.c>
#endif /* CONFIG_USERSPACE */

static uint16_t calc_chksum(uint16_t sum, const uint8_t *data, size_t len)
{
	const uint8_t *end;
	uint16_t tmp;

	end = data + len - 1;

	while (data < end) {
		tmp = (data[0] << 8) + data[1];
		sum += tmp;
		if (sum < tmp) {
			sum++;
		}

		data += 2;
	}

	if (data == end) {
		tmp = data[0] << 8;
		sum += tmp;
		if (sum < tmp) {
			sum++;
		}
	}

	return sum;
}

static inline uint16_t pkt_calc_chksum(struct net_pkt *pkt, uint16_t sum)
{
	struct net_pkt_cursor *cur = &pkt->cursor;
	size_t len;

	if (!cur->buf || !cur->pos) {
		return sum;
	}

	len = cur->buf->len - (cur->pos - cur->buf->data);

	while (cur->buf) {
		sum = calc_chksum(sum, cur->pos, len);

		cur->buf = cur->buf->frags;
		if (!cur->buf || !cur->buf->len) {
			break;
		}

		cur->pos = cur->buf->data;

		if (len % 2) {
			sum += *cur->pos;
			if (sum < *cur->pos) {
				sum++;
			}

			cur->pos++;
			len = cur->buf->len - 1;
		} else {
			len = cur->buf->len;
		}
	}

	return sum;
}

uint16_t net_calc_chksum(struct net_pkt *pkt, uint8_t proto)
{
	size_t len = 0U;
	uint16_t sum = 0U;
	struct net_pkt_cursor backup;
	bool ow;

	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_pkt_family(pkt) == AF_INET) {
		if (proto != IPPROTO_ICMP) {
			len = 2 * sizeof(struct in_addr);
			sum = net_pkt_get_len(pkt) -
				net_pkt_ip_hdr_len(pkt) -
				net_pkt_ipv4_opts_len(pkt) + proto;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		len = 2 * sizeof(struct in6_addr);
		sum =  net_pkt_get_len(pkt) -
			net_pkt_ip_hdr_len(pkt) -
			net_pkt_ipv6_ext_len(pkt) + proto;
	} else {
		NET_DBG("Unknown protocol family %d", net_pkt_family(pkt));
		return 0;
	}

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	ow = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) - len);

	sum = calc_chksum(sum, pkt->cursor.pos, len);
	net_pkt_skip(pkt, len + net_pkt_ip_opts_len(pkt));

	sum = pkt_calc_chksum(pkt, sum);

	sum = (sum == 0U) ? 0xffff : htons(sum);

	net_pkt_cursor_restore(pkt, &backup);

	net_pkt_set_overwrite(pkt, ow);

	return ~sum;
}

#if defined(CONFIG_NET_IPV4)
uint16_t net_calc_chksum_ipv4(struct net_pkt *pkt)
{
	uint16_t sum;

	sum = calc_chksum(0, pkt->buffer->data,
			  net_pkt_ip_hdr_len(pkt) +
			  net_pkt_ipv4_opts_len(pkt));

	sum = (sum == 0U) ? 0xffff : htons(sum);

	return ~sum;
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_IPV4)
static bool convert_port(const char *buf, uint16_t *port)
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
	uint16_t port;

	len = MIN(INET6_ADDRSTRLEN, str_len);

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

		end = MIN(len, ptr - (str + 1));
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
		/* -1 as end does not contain first [
		 * -2 as pointer is advanced by 2, skipping ]:
		 */
		len = str_len - end - 1 - 2;

		ptr += 2;

		for (i = 0; i < len; i++) {
			if (!ptr[i]) {
				len = i;
				break;
			}
		}

		/* Re-use the ipaddr buf for port conversion */
		memcpy(ipaddr, ptr, len);
		ipaddr[len] = '\0';

		ret = convert_port(ipaddr, &port);
		if (!ret) {
			return false;
		}

		net_sin6(addr)->sin6_port = htons(port);

		NET_DBG("IPv6 host %s port %d",
			log_strdup(net_addr_ntop(AF_INET6, addr6,
						 ipaddr, sizeof(ipaddr) - 1)),
			port);
	} else {
		NET_DBG("IPv6 host %s",
			log_strdup(net_addr_ntop(AF_INET6, addr6,
						 ipaddr, sizeof(ipaddr) - 1)));
	}

	return true;
}
#else
static inline bool parse_ipv6(const char *str, size_t str_len,
			      struct sockaddr *addr, bool has_port)
{
	return false;
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
	uint16_t port;

	len = MIN(NET_IPV4_ADDR_LEN, str_len);

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

		end = MIN(len, ptr - str);
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
		log_strdup(net_addr_ntop(AF_INET, addr4,
					 ipaddr, sizeof(ipaddr) - 1)),
		port);
	return true;
}
#else
static inline bool parse_ipv4(const char *str, size_t str_len,
			      struct sockaddr *addr, bool has_port)
{
	return false;
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
		return parse_ipv6(str, str_len, addr, true);
	}

	for (count = i = 0; str[i] && i < str_len; i++) {
		if (str[i] == ':') {
			count++;
		}
	}

	if (count == 1) {
		return parse_ipv4(str, str_len, addr, true);
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
	return false;
}

int net_bytes_from_str(uint8_t *buf, int buf_len, const char *src)
{
	unsigned int i;
	char *endptr;

	for (i = 0U; i < strlen(src); i++) {
		if (!(src[i] >= '0' && src[i] <= '9') &&
		    !(src[i] >= 'A' && src[i] <= 'F') &&
		    !(src[i] >= 'a' && src[i] <= 'f') &&
		    src[i] != ':') {
			return -EINVAL;
		}
	}

	(void)memset(buf, 0, buf_len);

	for (i = 0U; i < buf_len; i++) {
		buf[i] = strtol(src, &endptr, 16);
		src = ++endptr;
	}

	return 0;
}

const char *net_family2str(sa_family_t family)
{
	switch (family) {
	case AF_UNSPEC:
		return "AF_UNSPEC";
	case AF_INET:
		return "AF_INET";
	case AF_INET6:
		return "AF_INET6";
	case AF_PACKET:
		return "AF_PACKET";
	case AF_CAN:
		return "AF_CAN";
	}

	return NULL;
}

const struct in_addr *net_ipv4_unspecified_address(void)
{
	static const struct in_addr addr;

	return &addr;
}

const struct in_addr *net_ipv4_broadcast_address(void)
{
	static const struct in_addr addr = { { { 255, 255, 255, 255 } } };

	return &addr;
}

/* IPv6 wildcard and loopback address defined by RFC2553 */
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

const struct in6_addr *net_ipv6_unspecified_address(void)
{
	return &in6addr_any;
}
