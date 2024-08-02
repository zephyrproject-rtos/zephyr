/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(net, test_inet_addr)
{
	in_addr_t ret;
	static const struct parm {
		const char *in;
		int out;
	} parms[] = {
	/* expect failure */
#ifndef CONFIG_ARCH_POSIX
		{NULL, (uint32_t)-1}, /* this value will segfault using the host libc */
#endif
		{".", (uint32_t)-1},
		{"..", (uint32_t)-1},
		{"...", (uint32_t)-1},
		{"-1.-2.-3.-4", (uint32_t)-1},
		{"256.65536.4294967296.18446744073709551616", (uint32_t)-1},
		{"a.b.c.d", (uint32_t)-1},
		{"0.0.0.1234", (uint32_t)-1},
		{"0.0.0.12a", (uint32_t)-1},
		{" 1.2.3.4", (uint32_t)-1},

		/* expect success */
		{"0.0.0.0", htonl(0)},
		{"000.00.0.0", htonl(0)},
		{"127.0.0.1", htonl(0x7f000001)},
		{"1.2.3.4", htonl(0x01020304)},
		{"1.2.3.4    ", htonl(0x01020304)},
		{"0.0.0.123 a", htonl(0x0000007b)},
		{"255.255.255.255", htonl(0xffffffff)},
	};

	ARRAY_FOR_EACH_PTR(parms, p) {
		ret = inet_addr(p->in);
		zexpect_equal(ret, p->out, "inet_addr(%s) failed. expect: %d actual: %d", p->in,
			      p->out, ret);
	}
}
