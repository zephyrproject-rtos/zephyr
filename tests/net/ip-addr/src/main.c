/* main.c - Application main entry point */

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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <misc/printk.h>
#include <net/net_core.h>
#include <net/nbuf.h>

#define NET_DEBUG 1
#include "net_private.h"

#define TEST_BYTE_1(value, expected)				 \
	do {							 \
		char out[3];					 \
		net_byte_to_hex(out, value, 'A', true);		 \
		if (strcmp(out, expected)) {			 \
			printk("Test 0x%s failed.\n", expected); \
			return;					 \
		}						 \
	} while (0)

#define TEST_BYTE_2(value, expected)				 \
	do {							 \
		char out[3];					 \
		net_byte_to_hex(out, value, 'a', true);		 \
		if (strcmp(out, expected)) {			 \
			printk("Test 0x%s failed.\n", expected); \
			return;					 \
		}						 \
	} while (0)

#define TEST_LL_6(a, b, c, d, e, f, expected)				\
	do {								\
		uint8_t ll[] = { a, b, c, d, e, f };			\
		if (strcmp(net_sprint_ll_addr(ll, sizeof(ll)),		\
			   expected)) {					\
			printk("Test %s failed.\n", expected);		\
			return;						\
		}							\
	} while (0)

#define TEST_LL_8(a, b, c, d, e, f, g, h, expected)			\
	do {								\
		uint8_t ll[] = { a, b, c, d, e, f, g, h };		\
		if (strcmp(net_sprint_ll_addr(ll, sizeof(ll)),		\
			   expected)) {					\
			printk("Test %s failed.\n", expected);		\
			return;						\
		}							\
	} while (0)

#define TEST_LL_6_TWO(a, b, c, d, e, f, expected)			\
	do {								\
		uint8_t ll1[] = { a, b, c, d, e, f };			\
		uint8_t ll2[] = { f, e, d, c, b, a };			\
		char out[2 * sizeof("xx:xx:xx:xx:xx:xx") + 1 + 1];	\
		sprintf(out, "%s ",					\
			net_sprint_ll_addr(ll1, sizeof(ll1)));		\
		sprintf(out + sizeof("xx:xx:xx:xx:xx:xx"), "%s",	\
			net_sprint_ll_addr(ll2, sizeof(ll2)));		\
		if (strcmp(out, expected)) {				\
			printk("Test %s failed, got %s\n", expected, out); \
			return;						\
		}							\
	} while (0)

#define TEST_IPV6(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, expected)  \
	do {								     \
		struct in6_addr addr = { { { a, b, c, d, e, f, g, h,	     \
					       i, j, k, l, m, n, o, p } } }; \
		char *ptr = net_sprint_ipv6_addr(&addr);		     \
		if (strcmp(ptr, expected)) {				     \
			printk("Test %s failed, got %s\n", expected, ptr);   \
			return;						     \
		}							     \
	} while (0)

#define TEST_IPV4(a, b, c, d, expected)					\
	do {								\
		struct in_addr addr = { { { a, b, c, d } } };		\
		char *ptr = net_sprint_ipv4_addr(&addr);		\
		if (strcmp(ptr, expected)) {				\
			printk("Test %s failed, got %s\n", expected, ptr); \
			return;						\
		}							\
	} while (0)

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	TEST_BYTE_1(0xde, "DE");
	TEST_BYTE_1(0x09, "09");
	TEST_BYTE_2(0xa9, "a9");
	TEST_BYTE_2(0x80, "80");

	TEST_LL_6(0x12, 0x9f, 0xe3, 0x01, 0x7f, 0x00, "12:9F:E3:01:7F:00");
	TEST_LL_8(0x12, 0x9f, 0xe3, 0x01, 0x7f, 0x00, 0xff, 0x0f, \
		  "12:9F:E3:01:7F:00:FF:0F");

	TEST_LL_6_TWO(0x12, 0x9f, 0xe3, 0x01, 0x7f, 0x00,	\
		      "12:9F:E3:01:7F:00 00:7F:01:E3:9F:12");

	TEST_IPV6(0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \
		  "2001:db8::1");

	TEST_IPV6(0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0x56, 0x78,	\
		  0x9a, 0xbc, 0xde, 0xf0, 0x01, 0x02, 0x03, 0x04,	\
		  "2001:db8:1234:5678:9abc:def0:102:304");

	TEST_IPV6(0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  0x0c, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,	\
		  "fe80::cb8:0:0:2");

	TEST_IPV6(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,	\
		  "::1");

	TEST_IPV6(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  "::");

	TEST_IPV4(192, 168, 0, 1, "192.168.0.1");
	TEST_IPV4(0, 0, 0, 0, "0.0.0.0");
	TEST_IPV4(127, 0, 0, 1, "127.0.0.1");

	printk("IP address print tests passed\n");
}
