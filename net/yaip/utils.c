/** @file
 * @brief Misc network utility functions
 *
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

#if defined(CONFIG_NET_DEBUG_UTILS)
#define SYS_LOG_DOMAIN "net/utils"
#define NET_DEBUG 1
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_core.h>

char *net_byte_to_hex(uint8_t *ptr, uint8_t byte, char base, bool pad)
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

char *net_sprint_ip_addr_buf(const uint8_t *ip, int ip_len,
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
