/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/cbprintf.h>

#define CBPRINTF_DEBUG 1

struct out_buffer {
	char *buf;
	size_t idx;
	size_t size;
};

static int out(int c, void *dest)
{
	int rv = EOF;
	struct out_buffer *buf = dest;

	if (buf->idx < buf->size) {
		buf->buf[buf->idx++] = (char)(unsigned char)c;
		rv = (int)(unsigned char)c;
	}
	return rv;
}

static char runtime_buf[128];
static char compare_buf[128];

void dump(const char *desc, uint8_t *package, size_t len)
{
	printk("%s package %p:\n", desc, package);
	for (size_t i = 0; i < len; i++) {
		printk("%02x ", package[i]);
	}
	printk("\n");
}

void unpack(const char *desc, struct out_buffer *buf,
	    uint8_t *package, size_t len)
{
	cbpprintf(out, buf, package);
	buf->buf[buf->idx] = 0;
	zassert_equal(strcmp(buf->buf, compare_buf), 0,
		      "Strings differ\nexp: |%s|\ngot: |%s|\n",
		      compare_buf, buf->buf);
}

#define TEST_PACKAGING(fmt, ...) do { \
	snprintf(compare_buf, sizeof(compare_buf), fmt, __VA_ARGS__); \
	printk("-----------------------------------------\n"); \
	printk("%s\n", compare_buf); \
	struct out_buffer rt_buf = { \
		.buf = runtime_buf, .idx = 0, .size = sizeof(runtime_buf) \
	}; \
	int rc = cbprintf_package(NULL, 0, fmt, __VA_ARGS__); \
	zassert_true(rc > 0, "cbprintf_package() returned %d", rc); \
	size_t len = rc; \
	uint8_t __aligned(8) rt_package[len]; \
	memset(rt_package, 0, len); \
	rc = cbprintf_package(rt_package, len, fmt, __VA_ARGS__); \
	zassert_equal(rc, len, "cbprintf_package() returned %d, expected %d", rc, len); \
	dump("runtime", rt_package, len); \
	unpack("runtime", &rt_buf, rt_package, len); \
} while (0)

void test_cbprintf_package(void)
{
	volatile signed char sc = -11;
	int i = 100;
	char c = 'a';
	static const short s = -300;
	long li = -1111111111;
	long long lli = 0x1122334455667788;
	unsigned char uc = 100;
	unsigned int ui = 0x12345;
	unsigned short us = 0x1234;
	unsigned long ul = 0xaabbaabb;
	unsigned long long ull = 0xaabbaabbaabb;
	float f = -1.234;
	double d = 1.2333;

	/* tests to exercize different element alignments */
	TEST_PACKAGING("test long %x %lx %x", 0xb1b2b3b4, li, 0xe4e3e2e1);
	TEST_PACKAGING("test long long %x %llx %x", 0xb1b2b3b4, lli, 0xe4e3e2e1);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		TEST_PACKAGING("test double %x %f %x", 0xb1b2b3b4, d, 0xe4e3e2e1);
	}

	/* tests with varied elements */
	TEST_PACKAGING("test %d %hd %hhd", i, s, sc);
	TEST_PACKAGING("test %ld %llx %hhu %hu %u", li, lli, uc, us, ui);
	TEST_PACKAGING("test %lu %llu", ul, ull);
	TEST_PACKAGING("test %c %p", c, &c);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		TEST_PACKAGING("test %f %a", f, d);
	}
}

void test_main(void)
{
	printk("sizeof:  int=%zu long=%zu ptr=%zu long long=%zu double=%zu long double=%zu\n",
	       sizeof(int), sizeof(long), sizeof(void *), sizeof(long long),
	       sizeof(double), sizeof(long double));
	printk("alignof: int=%zu long=%zu ptr=%zu long long=%zu double=%zu long double=%zu\n",
	       __alignof__(int), __alignof__(long), __alignof__(void *),
	       __alignof__(long long), __alignof__(double), __alignof__(long double));

	ztest_test_suite(cbprintf_package,
			 ztest_unit_test(test_cbprintf_package)
			 );

	ztest_run_test_suite(cbprintf_package);
}
