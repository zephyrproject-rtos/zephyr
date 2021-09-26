/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/cbprintf.h>

#define CBPRINTF_DEBUG 1

#ifndef CBPRINTF_PACKAGE_ALIGN_OFFSET
#define CBPRINTF_PACKAGE_ALIGN_OFFSET 0
#endif

#define ALIGN_OFFSET (sizeof(void *) * CBPRINTF_PACKAGE_ALIGN_OFFSET)

struct out_buffer {
	char *buf;
	size_t idx;
	size_t size;
};

static int out(int c, void *dest)
{
	int rv = EOF;
	struct out_buffer *buf = (struct out_buffer *)dest;

	if (buf->idx < buf->size) {
		buf->buf[buf->idx++] = (char)(unsigned char)c;
		rv = (int)(unsigned char)c;
	}
	return rv;
}

static char static_buf[512];
static char runtime_buf[512];
static char compare_buf[128];

static void dump(const char *desc, uint8_t *package, size_t len)
{
	printk("%s package %p:\n", desc, package);
	for (size_t i = 0; i < len; i++) {
		printk("%02x ", package[i]);
	}
	printk("\n");
}

static void unpack(const char *desc, struct out_buffer *buf,
	    uint8_t *package, size_t len)
{
	cbpprintf((cbprintf_cb)out, buf, package);
	buf->buf[buf->idx] = 0;
	zassert_equal(strcmp(buf->buf, compare_buf), 0,
		      "Strings differ\nexp: |%s|\ngot: |%s|\n",
		      compare_buf, buf->buf);
}

#define TEST_PACKAGING(fmt, ...) do { \
	int must_runtime = CBPRINTF_MUST_RUNTIME_PACKAGE(0, fmt, __VA_ARGS__); \
	zassert_equal(must_runtime, !Z_C_GENERIC, NULL); \
	snprintfcb(compare_buf, sizeof(compare_buf), fmt, __VA_ARGS__); \
	printk("-----------------------------------------\n"); \
	printk("%s\n", compare_buf); \
	uint8_t *pkg; \
	struct out_buffer rt_buf = { \
		.buf = runtime_buf, .idx = 0, .size = sizeof(runtime_buf) \
	}; \
	int rc = cbprintf_package(NULL, ALIGN_OFFSET, 0, fmt, __VA_ARGS__); \
	zassert_true(rc > 0, "cbprintf_package() returned %d", rc); \
	int len = rc; \
	/* Aligned so the package is similar to the static one. */ \
	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) \
			rt_package[len + ALIGN_OFFSET]; \
	memset(rt_package, 0, len + ALIGN_OFFSET); \
	pkg = &rt_package[ALIGN_OFFSET]; \
	rc = cbprintf_package(pkg, len, 0, fmt, __VA_ARGS__); \
	zassert_equal(rc, len, "cbprintf_package() returned %d, expected %d", \
		      rc, len); \
	dump("runtime", pkg, len); \
	unpack("runtime", &rt_buf, pkg, len); \
	struct out_buffer st_buf = { \
		.buf = static_buf, .idx = 0, .size = sizeof(static_buf) \
	}; \
	CBPRINTF_STATIC_PACKAGE(NULL, 0, len, ALIGN_OFFSET, 0, fmt, __VA_ARGS__); \
	zassert_true(len > 0, "CBPRINTF_STATIC_PACKAGE() returned %d", len); \
	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) \
		package[len + ALIGN_OFFSET];\
	int outlen; \
	pkg = &package[ALIGN_OFFSET]; \
	CBPRINTF_STATIC_PACKAGE(pkg, len, outlen, ALIGN_OFFSET, 0, fmt, __VA_ARGS__);\
	zassert_equal(len, outlen, NULL); \
	dump("static", pkg, len); \
	unpack("static", &st_buf, pkg, len); \
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
	void *vp = NULL;


	/* tests to exercize different element alignments */
	TEST_PACKAGING("test long %x %lx %x", 0xb1b2b3b4, li, 0xe4e3e2e1);
	TEST_PACKAGING("test long long %x %llx %x", 0xb1b2b3b4, lli, 0xe4e3e2e1);

	/* tests with varied elements */
	TEST_PACKAGING("test %d %hd %hhd", i, s, sc);
	TEST_PACKAGING("test %ld %llx %hhu %hu %u", li, lli, uc, us, ui);
	TEST_PACKAGING("test %lu %llu", ul, ull);
	TEST_PACKAGING("test %c %p", c, vp);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		float f = -1.234;
		double d = 1.2333;

		TEST_PACKAGING("test double %x %f %x", 0xb1b2b3b4, d, 0xe4e3e2e1);
		TEST_PACKAGING("test %f %a", f, d);
#if CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE
		long double ld = 1.2333;

		TEST_PACKAGING("test %Lf", ld);
#endif
	}
}

void test_cbprintf_rw_str_indexes(void)
{
	int len0, len1, len2;
	static const char *test_str = "test %d %s";
	static const char *test_str1 = "lorem ipsum";
	uint8_t str_idx;
	char *addr;

	len0 = cbprintf_package(NULL, 0, 0, test_str, 100, test_str1);
	if (len0 > (int)(4 * sizeof(void *))) {
		TC_PRINT("Skipping test, platform does not detect RO strings.\n");
		ztest_test_skip();
	}

	len1 = cbprintf_package(NULL, 0, CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);
	CBPRINTF_STATIC_PACKAGE(NULL, 0, len2, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);

	/* package with string indexes will contain two more bytes holding indexes
	 * of string parameter locations.
	 */
	zassert_equal(len0 + 2, len1, NULL);
	zassert_equal(len0 + 2, len2, NULL);

	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package0[len0];
	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package1[len1];
	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package2[len2];

	len0 = cbprintf_package(package0, sizeof(package0), 0,
				test_str, 100, test_str1);

	len1 = cbprintf_package(package1, sizeof(package1) - 1,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);
	zassert_equal(-ENOSPC, len1, NULL);

	CBPRINTF_STATIC_PACKAGE(package2, sizeof(package2) - 1, len2, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);
	zassert_equal(-ENOSPC, len2, NULL);

	len1 = cbprintf_package(package1, sizeof(package1),
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);
	zassert_equal(len0 + 2, len1, NULL);

	CBPRINTF_STATIC_PACKAGE(package2, sizeof(package2), len2, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);
	zassert_equal(len0 + 2, len2, NULL);

	struct z_cbprintf_desc *desc0 = (struct z_cbprintf_desc *)package0;
	struct z_cbprintf_desc *desc1 = (struct z_cbprintf_desc *)package1;
	struct z_cbprintf_desc *desc2 = (struct z_cbprintf_desc *)package2;

	/* Compare descriptor content. Second package has one ro string index. */
	zassert_equal(desc0->ro_str_cnt, 0, NULL);
	zassert_equal(desc1->ro_str_cnt, 2, NULL);
	zassert_equal(desc2->ro_str_cnt, 2, NULL);

	int *p = (int *)package1;

	str_idx = package1[len0];
	addr = *(char **)&p[str_idx];
	zassert_equal(addr, test_str, NULL);

	str_idx = package2[len0];
	addr = *(char **)&p[str_idx];
	zassert_equal(addr, test_str, NULL);

	str_idx = package1[len0 + 1];
	addr = *(char **)&p[str_idx];
	zassert_equal(addr, test_str1, NULL);

	str_idx = package2[len0 + 1];
	addr = *(char **)&p[str_idx];
	zassert_equal(addr, test_str1, NULL);
}

static void test_cbprintf_fsc_package(void)
{
	int len;
	static const char *test_str = "test %d %s";
	static const char *test_str1 = "lorem ipsum";
	char *addr;
	int fsc_len;

	len = cbprintf_package(NULL, 0, CBPRINTF_PACKAGE_ADD_STRING_IDXS,
			       test_str, 100, test_str1);
	if (len > (int)(4 * sizeof(void *) + 2)) {
		TC_PRINT("Skipping test, platform does not detect RO strings.\n");
		ztest_test_skip();
	}

	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package[len];

	len = cbprintf_package(package, sizeof(package),
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);

	struct z_cbprintf_desc *desc = (struct z_cbprintf_desc *)package;

	zassert_equal(desc->ro_str_cnt, 2, NULL);
	zassert_equal(desc->str_cnt, 0, NULL);

	/* Get length of fsc package. */
	fsc_len = cbprintf_fsc_package(package, len, NULL, 0);

	int exp_len = len + (int)strlen(test_str) + 1 + (int)strlen(test_str1) + 1;

	zassert_equal(exp_len, fsc_len, NULL);

	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) fsc_package[fsc_len];

	fsc_len = cbprintf_fsc_package(package, len, fsc_package, sizeof(fsc_package) - 1);
	zassert_equal(fsc_len, -ENOSPC, NULL);

	fsc_len = cbprintf_fsc_package(package, len, fsc_package, sizeof(fsc_package));
	zassert_equal((int)sizeof(fsc_package), fsc_len, NULL);

	/* New package has no RO string locations, only copied one. */
	desc = (struct z_cbprintf_desc *)fsc_package;
	zassert_equal(desc->ro_str_cnt, 0, NULL);
	zassert_equal(desc->str_cnt, 2, NULL);

	/* Get pointer to the first string in the package. */
	addr = (char *)&fsc_package[desc->len * sizeof(int) + 1];

	zassert_equal(strcmp(test_str, addr), 0, NULL);

	/* Get address of the second string. */
	addr += strlen(addr) + 2;
	zassert_equal(strcmp(test_str1, addr), 0, NULL);
}

#if __cplusplus
extern "C" void test_cxx(void);
void test_cxx(void)
#else
void test_cc(void)
#endif
{
#ifdef __cplusplus
	printk("C++\n");
#else
	printk("sizeof:  int=%zu long=%zu ptr=%zu long long=%zu double=%zu long double=%zu\n",
	       sizeof(int), sizeof(long), sizeof(void *), sizeof(long long),
	       sizeof(double), sizeof(long double));
	printk("alignof: int=%zu long=%zu ptr=%zu long long=%zu double=%zu long double=%zu\n",
	       __alignof__(int), __alignof__(long), __alignof__(void *),
	       __alignof__(long long), __alignof__(double), __alignof__(long double));
	printk("%s C11 _Generic\n", Z_C_GENERIC ? "With" : "Without");
#endif

	ztest_test_suite(cbprintf_package,
			 ztest_unit_test(test_cbprintf_package),
			 ztest_unit_test(test_cbprintf_rw_str_indexes),
			 ztest_unit_test(test_cbprintf_fsc_package)
			 );

	ztest_run_test_suite(cbprintf_package);
}
