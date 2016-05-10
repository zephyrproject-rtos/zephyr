/** @file
 @brief Network stack private header

 This is not to be included by the application.
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

#include <errno.h>

extern void net_nbuf_init(void);
extern int net_if_init(void);
extern void net_context_init(void);

#if NET_DEBUG > 0
static inline char *net_byte_to_hex(uint8_t *ptr, uint8_t byte, char base,
				    bool pad)
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

static inline char *net_sprint_ll_addr_buf(uint8_t *ll, uint8_t ll_len,
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
		ptr = net_byte_to_hex(ptr, ll[i], 'A', true);
		*ptr++ = ':';
		blen -= 3;
	}

	if (!(ptr - buf)) {
		return NULL;
	}

	*(ptr - 1) = '\0';
	return buf;
}

static inline char *net_sprint_ll_addr(uint8_t *ll, uint8_t ll_len)
{
	static char buf[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	return net_sprint_ll_addr_buf(ll, ll_len, (char *)buf, sizeof(buf));
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

static inline char *net_sprint_ip_addr_buf(uint8_t *ip, int ip_len,
					   char *buf, int buflen)
{
	uint16_t *w = (uint16_t *)ip;
	uint8_t i, bl, bh, longest = 1;
	int8_t pos = -1;
	char delim = ':';
	unsigned char zeros[8] = { 0 };
	char *ptr = buf;
	int len = -1;
	uint16_t value;
	bool needcolon = false;

	switch (ip_len) {
	case 16:
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
		break;

	case 4:
		len = 4;
		delim = '.';
		break;
	default:
		break;
	}

	/* Invalid len, bail out */
	if (len < 0) {
		return NULL;
	}

	for (i = 0; i < len; i++) {

		/* IPv4 address a.b.c.d */
		if (len == 4) {
			uint8_t l;

			value = (uint32_t)ip[i];

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

	if (!(ptr - buf)) {
		return NULL;
	}

	if (ip_len == 4) {
		*(ptr - 1) = '\0';
	} else {
		*ptr = '\0';
	}

	return buf;
}

static inline char *net_sprint_ip_addr_ptr(uint8_t *ptr, uint8_t len)
{
	static char buf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")];

	return net_sprint_ip_addr_buf(ptr, len, (char *)buf, sizeof(buf));
}

static inline char *net_sprint_ipv6_addr(struct in6_addr *addr)
{
#if defined(CONFIG_NET_IPV6)
	return net_sprint_ip_addr_ptr(addr->s6_addr, 16);
#else
	return NULL;
#endif
}

static inline char *net_sprint_ipv4_addr(struct in_addr *addr)
{
#if defined(CONFIG_NET_IPV4)
	return net_sprint_ip_addr_ptr(addr->s4_addr, 4);
#else
	return NULL;
#endif
}

static inline char *net_sprint_ip_addr(struct net_addr *addr)
{
	switch (addr->family) {
	case AF_INET6:
#if defined(CONFIG_NET_IPV6)
		return net_sprint_ipv6_addr(&addr->in6_addr);
#else
		break;
#endif
	case AF_INET:
#if defined(CONFIG_NET_IPV4)
		return net_sprint_ipv4_addr(&addr->in_addr);
#else
		break;
#endif
	default:
		break;
	}

	return NULL;
}

#else /* NET_DEBUG */

static inline char *net_sprint_ll_addr(uint8_t *ll, uint8_t ll_len)
{
	return NULL;
}

static inline char *net_sprint_ip_addr_ptr(uint8_t *ptr, uint8_t len)
{
	return NULL;
}

static inline char *net_sprint_ipv6_addr(struct in6_addr *addr)
{
	return NULL;
}

static inline char *net_sprint_ipv4_addr(struct in_addr *addr)
{
	return NULL;
}

static inline char *net_sprint_ip_addr(struct net_addr *addr)
{
	return NULL;
}
#endif /* NET_DEBUG */
