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
#else /* NET_DEBUG */
static inline char *net_sprint_ll_addr(uint8_t *ll, uint8_t ll_len)
{
	return NULL;
}
#endif /* NET_DEBUG */
