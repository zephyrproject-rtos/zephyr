/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <wctype.h>
#include <stddef.h>
#include <string.h>
#include <sys/cbprintf.h>
#include <sys/util.h>

#define CBPRINTF_VIA_UNIT_TEST

/* Unit testing doesn't use Kconfig, so if we're not building from
 * twister force selection of all features.  If we are use flags to
 * determine which features are desired.  Yes, this is a mess.
 */
#ifndef VIA_TWISTER
/* Set this to truthy to use libc's snprintf as external validation.
 * This should be used with all options enabled.
 */
#define USE_LIBC 0
#define USE_PACKAGED 0
#define CONFIG_CBPRINTF_COMPLETE 1
#define CONFIG_CBPRINTF_FULL_INTEGRAL 1
#define CONFIG_CBPRINTF_FP_SUPPORT 1
#define CONFIG_CBPRINTF_FP_A_SUPPORT 1
#define CONFIG_CBPRINTF_N_SPECIFIER 1
#define CONFIG_CBPRINTF_LIBC_SUBSTS 1

/* compensate for selects */
#if (CONFIG_CBPRINTF_FP_A_SUPPORT - 0)
#undef CONFIG_CBPRINTF_REDUCED_INTEGRAL
#undef CONFIG_CBPRINTF_FULL_INTEGRAL
#undef CONFIG_CBPRINTF_FP_SUPPORT
#define CONFIG_CBPRINTF_FULL_INTEGRAL 1
#define CONFIG_CBPRINTF_FP_SUPPORT 1
#endif

#if !(CONFIG_CBPRINTF_FP_SUPPORT - 0)
#undef CONFIG_CBPRINTF_FP_SUPPORT
#endif

/* Convert choice selections to one defined flag */
#if !(CONFIG_CBPRINTF_COMPLETE - 0)
#ifdef CONFIG_CBPRINTF_FP_SUPPORT
#error Inconsistent configuration
#endif
#undef CONFIG_CBPRINTF_COMPLETE
#define CONFIG_CBPRINTF_NANO 1
#endif
#if !(CONFIG_CBPRINTF_FULL_INTEGRAL - 0)
#undef CONFIG_CBPRINTF_FULL_INTEGRAL
#define CONFIG_CBPRINTF_REDUCED_INTEGRAL 1
#endif

#else /* VIA_TWISTER */
#if (VIA_TWISTER & 0x01) != 0
#define CONFIG_CBPRINTF_FULL_INTEGRAL 1
#else
#define CONFIG_CBPRINTF_REDUCED_INTEGRAL 1
#endif
#if (VIA_TWISTER & 0x02) != 0
#define CONFIG_CBPRINTF_FP_SUPPORT 1
#endif
#if (VIA_TWISTER & 0x04) != 0
#define CONFIG_CBPRINTF_FP_A_SUPPORT 1
#endif
#if (VIA_TWISTER & 0x08) != 0
#define CONFIG_CBPRINTF_N_SPECIFIER 1
#endif
#if (VIA_TWISTER & 0x40) != 0
#define CONFIG_CBPRINTF_FP_ALWAYS_A 1
#endif
#if (VIA_TWISTER & 0x80) != 0
#define CONFIG_CBPRINTF_NANO 1
#else /* 0x80 */
#define CONFIG_CBPRINTF_COMPLETE 1
#endif /* 0x80 */
#if (VIA_TWISTER & 0x100) != 0
#define CONFIG_CBPRINTF_LIBC_SUBSTS 1
#endif
#if (VIA_TWISTER & 0x200) != 0
#define USE_PACKAGED 1
#endif

#if (VIA_TWISTER & 0x400) != 0
#define USE_PACKAGE_FMT_AS_PTR 1
#else
#define USE_PACKAGE_FMT_AS_PTR 0
#endif

#endif /* VIA_TWISTER */

#include "../../../lib/os/cbprintf.c"

#if defined(CONFIG_CBPRINTF_COMPLETE)
#include "../../../lib/os/cbprintf_complete.c"
#elif defined(CONFIG_CBPRINTF_NANO)
#include "../../../lib/os/cbprintf_nano.c"
#endif

/* We can't determine at build-time whether int is 64-bit, so assume
 * it is.  If not the values are truncated at build time, and the str
 * pointers will be updated during test initialization.
 */
static const unsigned int pfx_val = (unsigned int)0x7b6b5b4b3b2b1b0b;
static const char pfx_str64[] = "7b6b5b4b3b2b1b0b";
static const char *pfx_str = pfx_str64;
static const unsigned int sfx_val = (unsigned int)0xe7e6e5e4e3e2e1e0;
static const char sfx_str64[] = "e7e6e5e4e3e2e1e0";
static const char *sfx_str = sfx_str64;
static const uint32_t package_flags = IS_ENABLED(USE_PACKAGE_FMT_AS_PTR) ?
					CBPRINTF_PACKAGE_FMT_AS_PTR : 0;

/* Buffer adequate to hold packaged state for all tested
 * configurations.
 */
static uint8_t packaged[256];

#define WRAP_FMT(_fmt) "%x" _fmt "%x"
#define PASS_ARG(...) pfx_val, __VA_ARGS__, sfx_val

static inline int match_str(const char **strp,
			    const char *expected,
			    size_t len)
{
	const char *str = *strp;
	int rv =  strncmp(str, expected, len);

	*strp += len;
	return rv;
}

static inline int match_pfx(const char **ptr)
{
	return match_str(ptr, pfx_str, 2U * sizeof(pfx_val));
}

static inline int match_sfx(const char **ptr)
{
	return match_str(ptr, sfx_str, 2U * sizeof(sfx_val));
}

/* This has to be more than 255 so we can test over-sized widths. */
static char buf[512];
static char *bp;

static inline void reset_out(void)
{
	bp = buf;
	*bp = 0;
}

static int out(int c, void *dest)
{
	int rv = EOF;

	ARG_UNUSED(dest);
	if (bp < (buf + ARRAY_SIZE(buf))) {
		*bp++ = (char)(unsigned char)c;
		rv = (int)(unsigned char)c;
	}
	return rv;
}

static char *get_fmt(uint32_t flags, uint8_t **pstart, size_t *len)
{
	char *s;
	size_t slen;
	uint8_t *p = *pstart;

	if (flags & CBPRINTF_PACKAGE_FMT_AS_PTR) {
		s = *(char **)&p[0];
		slen = sizeof(char *);
	} else if (p[0]) {
		s = (char *)&p[1];
		slen = strlen(s) + sizeof(uint8_t);
	} else {
		s = *(char **)&p[1];
		slen = sizeof(char *) + sizeof(uint8_t);
	}

	*pstart += slen;
	*len -= slen;

	return s;
}

/* When packages are compared, they must match exactly except for the first
 * argument which is always a string. Packages are considered equal even when
 * different methods (pointer vs value) were used for storing the first string.
 * As long as those string matches.
 */
static int package_cmp(uint32_t flags, uint8_t *p0, size_t len0,
		       uint8_t *p1, size_t len1)
{
	int rv;
	char *fmt0 = get_fmt(flags, &p0, &len0);
	char *fmt1 = get_fmt(flags, &p1, &len1);

	rv = strcmp(fmt0, fmt1);
	if (rv) {
		return rv;
	}

	if (len0 != len1) {
		return rv;
	}

	return memcmp(p0, p1, len0);
}

/* Static package is created (if possible) in the macro, if not null it is
 * compared against package generated by cbvprintf_package.
 */
__printf_like(4, 5)
static int prf(uint8_t *static_package, size_t psize, bool skip_package_cmp,
	       const char *format, ...)
{
	va_list ap;
	int rv;

	reset_out();
	va_start(ap, format);
#if USE_LIBC
	rv = vsnprintf(buf, sizeof(buf), format, ap);
#else
	reset_out();
#if USE_PACKAGED
	/* If package is already statically created skip packaging and go
	 * directly to formatting.
	 */
	size_t len = sizeof(packaged);

	rv = cbvprintf_package(packaged, &len, package_flags, format, ap);
	if (len > sizeof(packaged)) {
		rv = -ENOSPC;
	} else {
		if (static_package && !skip_package_cmp) {
			rv = package_cmp(package_flags, static_package, psize,
					 packaged, len);
			if (rv) {
				rv = -EINVAL;
			}
		}

		if (rv >= 0) {
			rv = cbpprintf(out, NULL, package_flags, packaged);
		}
	}
#else
	rv = cbvprintf(out, NULL, format, ap);
#endif
	if (bp == (buf + ARRAY_SIZE(buf))) {
		--bp;
	}
	*bp = 0;
#endif
	va_end(ap);
	return rv;
}

static int rawprf(const char *format, ...)
{
	va_list ap;
	int rv;

	va_start(ap, format);
#if USE_PACKAGED
	size_t len = sizeof(packaged);

	rv = cbvprintf_package(packaged, &len, package_flags, format, ap);
	if (len > sizeof(packaged)) {
		rv = -ENOSPC;
	} else {
		rv = cbpprintf(out, NULL, package_flags, packaged);
	}
#else
	rv = cbvprintf(out, NULL, format, ap);
#endif
	va_end(ap);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)
	    && !IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) {
		zassert_equal(rv, 0, NULL);
		rv = bp - buf;
	}
	return rv;
}

#define TEST_PRF2(skip_package_cmp, rc, _fmt, ...) do { \
	if (!IS_ENABLED(USE_PACKAGED) || \
	    CBPRINTF_MUST_RUNTIME_PACKAGE(0, _fmt, __VA_ARGS__)) { \
		rc = prf(NULL, 0, skip_package_cmp, _fmt, __VA_ARGS__); \
	} else { \
		CBPRINTF_STATIC_PACKAGE_SIZE(_len, USE_PACKAGE_FMT_AS_PTR,\
					     _fmt, __VA_ARGS__);\
		uint8_t _buf[_len]; \
		\
		size_t _rt_len; \
		CBPRINTF_STATIC_PACKAGE(NULL, _rt_len, \
					USE_PACKAGE_FMT_AS_PTR, \
					_fmt, __VA_ARGS__); \
		zassert_equal(_rt_len, _len, \
			"Package size calculations give different results"); \
		\
		CBPRINTF_STATIC_PACKAGE(_buf, _rt_len, \
					USE_PACKAGE_FMT_AS_PTR, \
					_fmt, __VA_ARGS__); \
		zassert_equal(_rt_len, _len, \
			"Package size calculations give different results"); \
		\
		rc = prf(_buf, _len, skip_package_cmp, _fmt, __VA_ARGS__); \
	} \
} while (0)

#define TEST_PRF(skip_package_cmp, rc, _fmt, ...) \
	TEST_PRF2(skip_package_cmp, rc, WRAP_FMT(_fmt), PASS_ARG(__VA_ARGS__))

struct context {
	const char *expected;
	const char *got;
	const char *file;
	unsigned int line;
};

__printf_like(3, 4)
static bool prf_failed(const struct context *ctx,
		       const char *cp,
		       const char *format,
		       ...)
{
	va_list ap;

	va_start(ap, format);
	printf("%s:%u for '%s'\n", ctx->file, ctx->line, ctx->expected);
	printf("in: %s\nat: %*c%s\n", ctx->got,
	       (unsigned int)(cp - ctx->got), '>', ctx->expected);
	vprintf(format, ap);
	va_end(ap);
	return false;
}

static inline bool prf_check(const char *expected,
			     int rv,
			     const char *file,
			     unsigned int line)
{
	const struct context ctx = {
		.expected = expected,
		.got = buf,
		.file = file,
		.line = line,
	};

	const char *str = buf;
	const char *sp;
	int rc;

	sp = str;
	rc = match_pfx(&str);
	if (rc != 0) {
		return prf_failed(&ctx, sp, "pfx mismatch %d\n", rc);
	}

	sp = str;
	rc = match_str(&str, expected, strlen(expected));
	if (rc != 0) {
		return prf_failed(&ctx, sp, "str mismatch %d\n", rc);
	}

	sp = str;
	rc = match_sfx(&str);
	if (rc != 0) {
		return prf_failed(&ctx, sp, "sfx mismatch, %d\n", rc);
	}

	rc = (*str != 0);
	if (rc != 0) {
		return prf_failed(&ctx, str, "no eos %02x\n", *str);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)
	    && !IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) {
		if (rv != 0) {
			return prf_failed(&ctx, str, "nano rv %d != 0\n", rc);
		}
	} else {
		int len = (int)(str - buf);

		if (rv != len) {
			return prf_failed(&ctx, str, "rv %d != expected %d\n",
					  rv, len);
		}
	}

	return true;
}

#define PRF_CHECK(expected, rv)	\
	zassert_true(prf_check(expected, rv, __FILE__, __LINE__), \
		     NULL)

static void test_pct(void)
{
	int rc;

	TEST_PRF(true, rc, "/%%/%c/", 'a');

	PRF_CHECK("/%/a/", rc);
}

static void test_c(void)
{
	int rc;

	TEST_PRF(true, rc, "%c", 'a');
	PRF_CHECK("a", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	TEST_PRF(true, rc, "%lc", (wint_t)'a');
	if (IS_ENABLED(USE_LIBC)) {
		PRF_CHECK("a", rc);
	} else {
		PRF_CHECK("%lc", rc);
	}

	rc = prf(NULL, 0, false, "/%256c/", 'a');

	const char *bp = buf;
	size_t spaces = 255;

	zassert_equal(rc, 258, "len %d", rc);
	zassert_equal(*bp, '/', NULL);
	++bp;
	while (spaces-- > 0) {
		zassert_equal(*bp, ' ', NULL);
		++bp;
	}
	zassert_equal(*bp, 'a', NULL);
	++bp;
	zassert_equal(*bp, '/', NULL);
}

static void test_s(void)
{
	const char *s = "123";
	static wchar_t ws[] = L"abc";
	int rc;

	TEST_PRF(false, rc, "/%s/", s);
	PRF_CHECK("/123/", rc);

	TEST_PRF(false, rc, "/%6s/%-6s/%2s/", s, s, s);
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("/123/123   /123/", rc);
	} else {
		PRF_CHECK("/   123/123   /123/", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	TEST_PRF(false, rc, "/%.6s/%.2s/%.s/", s, s, s);
	PRF_CHECK("/123/12//", rc);

	TEST_PRF(false, rc, "%ls", ws);
	if (IS_ENABLED(USE_LIBC)) {
		PRF_CHECK("abc", rc);
	} else {
		PRF_CHECK("%ls", rc);
	}
}

static void test_v_c(void)
{
	int rc;

	reset_out();
	buf[1] = 'b';
	rc = rawprf("%c", 'a');
	zassert_equal(rc, 1, NULL);
	zassert_equal(buf[0], 'a', NULL);
	if (!IS_ENABLED(USE_LIBC)) {
		zassert_equal(buf[1], 'b', "wth %x", buf[1]);
	}
}

static void test_d_length(void)
{
	int min = -1234567890;
	int max = 1876543210;
	long long svll = 123LL << 48;
	int rc;

	TEST_PRF(false, rc, "%d/%d", min, max);
	PRF_CHECK("-1234567890/1876543210", rc);

	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		/* Due to padding package created on runtime may be different
		 * than statically created. Statically created packages for
		 * that formatting is tested in the dedicated test.
		 */

		TEST_PRF(true, rc, "%hd/%hd", min, max);
		PRF_CHECK("-722/-14614", rc);

		TEST_PRF(true, rc, "%hhd/%hhd", min, max);
		PRF_CHECK("46/-22", rc);
	}

	TEST_PRF(false, rc, "%ld/%ld", (long)min, (long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(long) <= 4)
	    || IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%ld/%ld", rc);
	}

	/* Due to padding package created on runtime may be different than
	 * statically created. Statically created packages for that formatting
	 * is tested in the dedicated test.
	 */
	TEST_PRF(true, rc, "/%lld/%lld/", svll, -svll);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("/34621422135410688/-34621422135410688/", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		PRF_CHECK("/%lld/%lld/", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("/ERR/ERR/", rc);
	} else {
		zassert_true(false, "Missed case!");
	}

	TEST_PRF(false, rc, "%lld/%lld", (long long)min, (long long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%lld/%lld", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	TEST_PRF(false, rc, "%jd/%jd", (intmax_t)min, (intmax_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%jd/%jd", rc);
	}

	TEST_PRF(false, rc, "%zd/%td/%td", (size_t)min, (ptrdiff_t)min,
		      (ptrdiff_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(size_t) <= 4)) {
		PRF_CHECK("-1234567890/-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%zd/%td/%td", rc);
	}

	/* These have to be tested without the format validation
	 * attribute because they produce diagnostics, but we have
	 * intended behavior so we have to test them.
	 */
	reset_out();
	rc = rawprf("/%Ld/", max);
	zassert_equal(rc, 5, "len %d", rc);
	zassert_equal(strncmp("/%Ld/", buf, rc), 0, NULL);
}

static void test_d_flags(void)
{
	int sv = 123;
	int rc;

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	/* Stuff related to sign */
	TEST_PRF(false, rc, "/%d/%-d/%+d/% d/",
		      sv, sv, sv, sv);
	PRF_CHECK("/123/123/+123/ 123/", rc);

	/* Stuff related to width padding */
	TEST_PRF(false, rc, "/%1d/%4d/%-4d/%04d/%15d/%-15d/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/123/ 123/123 /0123/"
		  "            123/123            /", rc);

	/* Stuff related to precision */
	TEST_PRF(false, rc, "/%.6d/%6.4d/", sv, sv);
	PRF_CHECK("/000123/  0123/", rc);

	/* Now with negative values */
	sv = -sv;
	TEST_PRF(false, rc, "/%d/%-d/%+d/% d/",
		      sv, sv, sv, sv);
	PRF_CHECK("/-123/-123/-123/-123/", rc);

	TEST_PRF(false, rc, "/%1d/%6d/%-6d/%06d/%13d/%-13d/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/-123/  -123/-123  /-00123/"
		  "         -123/-123         /", rc);

	TEST_PRF(false, rc, "/%.6d/%6.4d/", sv, sv);
	PRF_CHECK("/-000123/ -0123/", rc);

	/* These have to be tested without the format validation
	 * attribute because they produce diagnostics, but the
	 * standard specifies behavior so we have to test them.
	 */
	sv = 123;
	reset_out();
	rc = rawprf("/%#d/% +d/%-04d/%06.4d/", sv, sv, sv, sv);
	zassert_equal(rc, 22, "rc %d", rc);
	zassert_equal(strncmp("/123/+123/123 /  0123/",
			      buf, rc), 0, NULL);
}

static void test_x_length(void)
{
	unsigned int min = 0x4c3c2c1c;
	unsigned int max = 0x4d3d2d1d;
	int rc;

	TEST_PRF(false, rc, "%x/%X", min, max);
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("4c3c2c1c/4d3d2d1d", rc);
	} else {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	/* Due to padding package created on runtime may be different than
	 * statically created. Statically created packages for that formatting
	 * is tested in the dedicated test.
	 */
	TEST_PRF(true, rc, "%hx/%hX", min, max);
	PRF_CHECK("2c1c/2D1D", rc);

	TEST_PRF(true, rc, "%hhx/%hhX", min, max);
	PRF_CHECK("1c/1D", rc);

	TEST_PRF(false, rc, "%lx/%lX", (unsigned long)min, (unsigned long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(long) <= 4)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%lx/%lX", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		TEST_PRF(false, rc, "%llx/%llX", (unsigned long long)min,
			      (unsigned long long)max);
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);

		TEST_PRF(false, rc, "%jx/%jX", (uintmax_t)min, (uintmax_t)max);
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	}

	TEST_PRF(false, rc, "%zx/%zX", (size_t)min, (size_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(size_t) <= 4)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%zx/%zX", rc);
	}

	TEST_PRF(false, rc, "%tx/%tX", (ptrdiff_t)min, (ptrdiff_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(ptrdiff_t) <= 4)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%tx/%tX", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    && (sizeof(long long) > sizeof(int))) {
		unsigned long long min = 0x8c7c6c5c4c3c2c1cULL;
		unsigned long long max = 0x8d7d6d5d4d3d2d1dULL;

		TEST_PRF(false, rc, "%llx/%llX", (unsigned long long)min,
			      (unsigned long long)max);
		PRF_CHECK("8c7c6c5c4c3c2c1c/8D7D6D5D4D3D2D1D", rc);
	}
}

static void test_x_flags(void)
{
	unsigned int sv = 0x123;
	int rc;

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	/* Stuff related to sign flags, which have no effect on
	 * unsigned conversions, and alternate form
	 */
	TEST_PRF(false, rc, "/%x/%-x/%#x/",
		      sv, sv, sv);
	PRF_CHECK("/123/123/0x123/", rc);

	/* Stuff related to width and padding */
	TEST_PRF(false, rc, "/%1x/%4x/%-4x/%04x/%#15x/%-15x/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/123/ 123/123 /0123/"
		  "          0x123/123            /", rc);

	/* These have to be tested without the format validation
	 * attribute because they produce diagnostics, but the
	 * standard specifies behavior so we have to test them.
	 */
	reset_out();
	rc = rawprf("/%+x/% x/", sv, sv);
	zassert_equal(rc, 9, "rc %d", rc);
	zassert_equal(strncmp("/123/123/", buf, rc), 0, NULL);
}

static void test_o(void)
{
	unsigned int v = 01234567;
	int rc;

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	TEST_PRF(false, rc, "%o", v);
	PRF_CHECK("1234567", rc);
	TEST_PRF(false, rc, "%#o", v);
	PRF_CHECK("01234567", rc);
}

static void test_fp_value(void)
{
	if (!IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		TC_PRINT("skipping unsupported feature\n");
		return;
	}

	double dv = 1234.567;
	int rc;

	TEST_PRF(false, rc, "/%f/%F/", dv, dv);
	PRF_CHECK("/1234.567000/1234.567000/", rc);
	TEST_PRF(false, rc, "%g", dv);
	PRF_CHECK("1234.57", rc);
	TEST_PRF(false, rc, "%e", dv);
	PRF_CHECK("1.234567e+03", rc);
	TEST_PRF(false, rc, "%E", dv);
	PRF_CHECK("1.234567E+03", rc);
	TEST_PRF(false, rc, "%a", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x1.34a449ba5e354p+10", rc);
	} else {
		PRF_CHECK("%a", rc);
	}

	dv = 1E3;
	TEST_PRF(false, rc, "%.2f", dv);
	PRF_CHECK("1000.00", rc);

	dv = 1E20;
	TEST_PRF(false, rc, "%.0f", dv);
	PRF_CHECK("100000000000000000000", rc);
	TEST_PRF(false, rc, "%.20e", dv);
	PRF_CHECK("1.00000000000000000000e+20", rc);

	dv = 1E-3;
	TEST_PRF(false, rc, "%.3e", dv);
	PRF_CHECK("1.000e-03", rc);

	dv = 1E-3;
	TEST_PRF(false, rc, "%g", dv);
	PRF_CHECK("0.001", rc);

	dv = 1234567.89;
	TEST_PRF(false, rc, "%g", dv);
	PRF_CHECK("1.23457e+06", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		dv = (double)BIT64(40);
		TEST_PRF(false, rc, "/%a/%.4a/%.20a/", dv, dv, dv);
		PRF_CHECK("/0x1p+40/0x1.0000p+40/"
			  "0x1.00000000000000000000p+40/", rc);

		dv += (double)BIT64(32);
		TEST_PRF(false, rc, "%a", dv);
		PRF_CHECK("0x1.01p+40", rc);
	}

	dv = INFINITY;
	TEST_PRF(false, rc, "%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("inf.f INF.F inf.e INF.E "
			  "inf.g INF.g inf.a INF.A", rc);
	} else {
		PRF_CHECK("inf.f INF.F inf.e INF.E "
			  "inf.g INF.g %a.a %A.A", rc);
	}

	dv = -INFINITY;
	TEST_PRF(false, rc, "%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("-inf.f -INF.F -inf.e -INF.E "
			  "-inf.g -INF.g -inf.a -INF.A", rc);
	} else {
		PRF_CHECK("-inf.f -INF.F -inf.e -INF.E "
			  "-inf.g -INF.g %a.a %A.A", rc);
	}

	dv = NAN;
	TEST_PRF(false, rc, "%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("nan.f NAN.F nan.e NAN.E "
			  "nan.g NAN.g nan.a NAN.A", rc);
	} else {
		PRF_CHECK("nan.f NAN.F nan.e NAN.E "
			  "nan.g NAN.g %a.a %A.A", rc);
	}

	dv = DBL_MIN;
	TEST_PRF(false, rc, "%a %e", dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x1p-1022 2.225074e-308", rc);
	} else {
		PRF_CHECK("%a 2.225074e-308", rc);
	}

	dv /= 4;
	TEST_PRF(false, rc, "%a %e", dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x0.4p-1022 5.562685e-309", rc);
	} else {
		PRF_CHECK("%a 5.562685e-309", rc);
	}

	/*
	 * The following tests are tailored to exercise edge cases in
	 * lib/os/cbprintf_complete.c:encode_float() and related functions.
	 */

	dv = 0x1.0p-3;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("0.125", rc);

	dv = 0x1.0p-4;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("0.0625", rc);

	dv = 0x1.8p-4;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("0.09375", rc);

	dv = 0x1.cp-4;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("0.109375", rc);

	dv = 0x1.9999999800000p-7;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("0.01249999999708962", rc);

	dv = 0x1.9999999ffffffp-8;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("0.006250000005820765", rc);

	dv = 0x1.0p+0;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("1", rc);

	dv = 0x1.fffffffffffffp-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("4.450147717014402e-308", rc);

	dv = 0x1.ffffffffffffep-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("4.450147717014402e-308", rc);

	dv = 0x1.ffffffffffffdp-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("4.450147717014401e-308", rc);

	dv = 0x1.0000000000001p-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("2.225073858507202e-308", rc);

	dv = 0x1p-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("2.225073858507201e-308", rc);

	dv = 0x0.fffffffffffffp-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("2.225073858507201e-308", rc);

	dv = 0x0.0000000000001p-1022;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("4.940656458412465e-324", rc);

	dv = 0x1.1fa182c40c60dp-1019;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("2e-307", rc);

	dv = 0x1.fffffffffffffp+1023;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("1.797693134862316e+308", rc);

	dv = 0x1.ffffffffffffep+1023;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("1.797693134862316e+308", rc);

	dv = 0x1.ffffffffffffdp+1023;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("1.797693134862315e+308", rc);

	dv = 0x1.0000000000001p+1023;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("8.988465674311582e+307", rc);

	dv = 0x1p+1023;
	TEST_PRF(false, rc, "%.16g", dv);
	PRF_CHECK("8.98846567431158e+307", rc);
}

static void test_fp_length(void)
{
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	double dv = 1.2345;
	int rc;

	/* Due to padding package created on runtime may be different than
	 * statically created. Statically created packages for that formatting
	 * is tested in the dedicated test.
	 */
	TEST_PRF(true, rc, "/%g/", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%g/", rc);
	}

	TEST_PRF(true, rc, "/%lg/", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%lg/", rc);
	}

	TEST_PRF(true, rc, "/%Lg/", (long double)dv);
	if (IS_ENABLED(USE_LIBC)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%Lg/", rc);
	}

	/* These have to be tested without the format validation
	 * attribute because they produce diagnostics, but we have
	 * intended behavior so we have to test them.
	 */
	reset_out();
	rc = rawprf("/%hf/", dv);
	zassert_equal(rc, 5, "len %d", rc);
	zassert_equal(strncmp("/%hf/", buf, rc), 0, NULL);
}

static void test_fp_flags(void)
{
	if (!IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		TC_PRINT("skipping unsupported feature\n");
		return;
	}

	double dv = 1.23;
	int rc;

	TEST_PRF(false, rc, "/%g/% g/%+g/", dv, dv, dv);
	PRF_CHECK("/1.23/ 1.23/+1.23/", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		TEST_PRF(false, rc, "/%a/%.1a/%.2a/", dv, dv, dv);
		PRF_CHECK("/0x1.3ae147ae147aep+0/"
			  "0x1.4p+0/0x1.3bp+0/", rc);
	}

	dv = -dv;
	TEST_PRF(false, rc, "/%g/% g/%+g/", dv, dv, dv);
	PRF_CHECK("/-1.23/-1.23/-1.23/", rc);

	dv = 23;
	TEST_PRF(false, rc, "/%g/%#g/%.0f/%#.0f/", dv, dv, dv, dv);
	PRF_CHECK("/23/23.0000/23/23./", rc);

	rc = prf(NULL, 0, false, "% .380f", 0x1p-400);
	zassert_equal(rc, 383, NULL);
	zassert_equal(strncmp(buf, " 0.000", 6), 0, NULL);
	zassert_equal(strncmp(&buf[119], "00003872", 8), 0, NULL);
}

static void test_star_width(void)
{
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	int rc;

	TEST_PRF(false, rc, "/%3c/%-3c/", 'a', 'a');
	PRF_CHECK("/  a/a  /", rc);

	TEST_PRF(false, rc, "/%*c/%*c/", 3, 'a', -3, 'a');
	PRF_CHECK("/  a/a  /", rc);
}

static void test_star_precision(void)
{
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	int rc;

	TEST_PRF(false, rc, "/%.*x/%10.*x/",
		      5, 0x12, 5, 0x12);
	PRF_CHECK("/00012/     00012/", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		double dv = 1.2345678;

		TEST_PRF(false, rc, "/%.3g/%.5g/%.8g/%g/",
			      dv, dv, dv, dv);
		PRF_CHECK("/1.23/1.2346/1.2345678/1.23457/", rc);

		TEST_PRF(false, rc, "/%.*g/%.*g/%.*g/%.*g/",
			      3, dv,
			      5, dv,
			      8, dv,
			      -3, dv);
		PRF_CHECK("/1.23/1.2346/1.2345678/1.23457/", rc);
	}
}

static void test_n(void)
{
	if (!IS_ENABLED(CONFIG_CBPRINTF_N_SPECIFIER)) {
		TC_PRINT("skipping unsupported feature\n");
		return;
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	char l_hh = 0;
	short l_h = 0;
	int l = 0;
	long l_l = 0;
	long long l_ll = 0;
	intmax_t l_j = 0;
	size_t l_z = 0;
	ptrdiff_t l_t = 0;
	int rc;

	rc = prf(NULL, 0, false, "12345%n", &l);
	zassert_equal(l, rc, "%d != %d", l, rc);
	zassert_equal(rc, 5, NULL);


	rc = prf(NULL, 0, false, "12345%hn", &l_h);
	zassert_equal(l_h, rc, NULL);

	rc = prf(NULL, 0, false, "12345%hhn", &l_hh);
	zassert_equal(l_hh, rc, NULL);

	rc = prf(NULL, 0, false, "12345%ln", &l_l);
	zassert_equal(l_l, rc, NULL);

	rc = prf(NULL, 0, false, "12345%lln", &l_ll);
	zassert_equal(l_ll, rc, NULL);

	rc = prf(NULL, 0, false, "12345%jn", &l_j);
	zassert_equal(l_j, rc, NULL);

	rc = prf(NULL, 0, false, "12345%zn", &l_z);
	zassert_equal(l_z, rc, NULL);

	rc = prf(NULL, 0, false, "12345%tn", &l_t);
	zassert_equal(l_t, rc, NULL);
}

#define EXPECTED_1ARG(_t) (IS_ENABLED(CONFIG_CBPRINTF_NANO) \
			   ? 1U : (sizeof(_t) / sizeof(int)))

static void test_p(void)
{
	if (IS_ENABLED(USE_LIBC)) {
		TC_PRINT("skipping on libc\n");
		return;
	}

	uintptr_t uip = 0xcafe21;
	void *ptr = (void *)uip;
	int rc;

	TEST_PRF(false, rc, "%p", ptr);
	PRF_CHECK("0xcafe21", rc);
	TEST_PRF(false, rc, "%p", NULL);
	PRF_CHECK("(nil)", rc);

	/* Nano doesn't support left-justification of pointer
	 * values.
	 */
	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		reset_out();
		rc = rawprf("/%12p/", ptr);
		zassert_equal(rc, 14, NULL);
		zassert_equal(strncmp("/    0xcafe21/", buf, rc), 0, NULL);

		reset_out();
		rc = rawprf("/%12p/", NULL);
		zassert_equal(rc, 14, NULL);
		zassert_equal(strncmp("/       (nil)/", buf, rc), 0, NULL);
	}

	reset_out();
	rc = rawprf("/%-12p/", ptr);
	zassert_equal(rc, 14, NULL);
	zassert_equal(strncmp("/0xcafe21    /", buf, rc), 0, NULL);

	reset_out();
	rc = rawprf("/%-12p/", NULL);
	zassert_equal(rc, 14, NULL);
	zassert_equal(strncmp("/(nil)       /", buf, rc), 0, NULL);

	/* Nano doesn't support zero-padding of pointer values.
	 */
	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		reset_out();
		rc = rawprf("/%.8p/", ptr);
		zassert_equal(rc, 12, NULL);
		zassert_equal(strncmp("/0x00cafe21/", buf, rc), 0, NULL);
	}
}

static int out_counter(int c,
		       void *ctx)
{
	size_t *count = ctx;

	++*count;
	return c;
}

static int out_e42(int c,
		   void *ctx)
{
	return -42;
}

static void test_libc_substs(void)
{
	if (!IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) {
		TC_PRINT("not enabled\n");
		return;
	}

	char lbuf[8];
	char full_flag = 0xbf;
	size_t count = 0;
	size_t const len = sizeof(lbuf) - 1U;
	int rc;

	lbuf[len] = full_flag;

	rc = snprintfcb(lbuf, len, "%06d", 1);
	zassert_equal(rc, 6, NULL);
	zassert_equal(strncmp("000001", lbuf, rc), 0, NULL);
	zassert_equal(lbuf[7], full_flag, NULL);

	rc = snprintfcb(lbuf, len, "%07d", 1);
	zassert_equal(rc, 7, NULL);
	zassert_equal(strncmp("000000", lbuf, rc), 0, NULL);
	zassert_equal(lbuf[7], full_flag, NULL);

	rc = snprintfcb(lbuf, len, "%020d", 1);
	zassert_equal(rc, 20, "rc %d", rc);
	zassert_equal(lbuf[7], full_flag, NULL);
	zassert_equal(strncmp("000000", lbuf, rc), 0, NULL);

	rc = cbprintf(out_counter, &count, "%020d", 1);
	zassert_equal(rc, 20, "rc %d", rc);
	zassert_equal(count, 20, NULL);

	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		rc = cbprintf(out_e42, NULL, "%020d", 1);
		zassert_equal(rc, -42, "rc %d", rc);
	}
}

static void test_cbprintf_package(void)
{
	if (!IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		TC_PRINT("skipped on nano\n");
		return;
	}

	size_t len = 0;
	int rc;
	char fmt[] = "/%i/";	/* not const */

	/* Verify we can calculate length without storing */
	rc = cbprintf_package(NULL, &len, package_flags, fmt, 3);

	zassert_equal(rc, 0, NULL);
	zassert_true(len > sizeof(int), NULL);

	/* We can't tell whether the fmt will be stored inline (6
	 * bytes) or as a pointer (1 + sizeof(void*)), but we know
	 * both of those will be at least 4 bytes.
	 */
	zassert_true(len >= (sizeof(int) + 4), NULL);

	/* Capture the base package information for future tests. */
	size_t lenp = len;

	/* Verify we get same length when storing */
	len = sizeof(packaged);
	rc = cbprintf_package(packaged, &len, package_flags, fmt, 3);
	zassert_equal(rc, lenp, NULL);
	zassert_equal(len, lenp, NULL);

	/* Verify we get an error if can't store */
	len = 1;
	rc = cbprintf_package(packaged, &len, package_flags, fmt, 3);
	zassert_equal(rc, -ENOSPC, NULL);
	zassert_equal(len, lenp, NULL);

	/* Verify we get an error if can't store */
	len = lenp - sizeof(int);
	rc = cbprintf_package(packaged, &len, package_flags, fmt, 3);
	zassert_equal(rc, -ENOSPC, NULL);
	zassert_equal(len, lenp, NULL);

	len = sizeof(fmt);
	rc = cbprintf_package(packaged, &len, package_flags, "%s", fmt);
}

static void test_cbpprintf(void)
{
	if (!IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		TC_PRINT("skipped on nano\n");
		return;
	}

	int rc;

	/* This only checks error conditions.  Formatting is checked
	 * by diverting prf() and related helpers to use the packaged
	 * version.
	 */
	reset_out();
	rc = cbpprintf(out, NULL, package_flags, NULL);
	zassert_equal(rc, -EINVAL, NULL);
}

static void test_MUST_RUNTIME_PACKAGE(void)
{
	int rv;
	/* Validate that value can be determined at compile time. */
	static const int rv0 = CBPRINTF_MUST_RUNTIME_PACKAGE(0, "test");

	zassert_equal(rv0, 0, NULL);

	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(0, "test %d", 100);
	zassert_equal(rv, 0, NULL);

	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(0, "test %s", "abc");
	zassert_equal(rv, 1, NULL);

	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(0, "test %d %s", 100, "s");
	zassert_equal(rv, 1, NULL);

	/* Test non zero skip where n strings are accepted. */
	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(1, "test %d", 100);
	zassert_equal(rv, 0, NULL);

	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(1, "test %d %s", 100, "abc");
	zassert_equal(rv, 0, NULL);

	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(1, "test %s %s", "abc", "abc");
	zassert_equal(rv, 1, NULL);

	rv = CBPRINTF_MUST_RUNTIME_PACKAGE(2, "test %s %s", "abc", "abc");
	zassert_equal(rv, 0, NULL);
}

#define TEST_STATIC_PACKAGE(...) do { \
	CBPRINTF_STATIC_PACKAGE_SIZE(len, USE_PACKAGE_FMT_AS_PTR, __VA_ARGS__);\
	uint8_t buf[len]; \
	uint8_t rt_buf[256]; \
	size_t _len = len; \
	int rc; \
	\
	CBPRINTF_STATIC_PACKAGE(buf, _len, USE_PACKAGE_FMT_AS_PTR, \
				__VA_ARGS__); \
	 \
	(void)_len; \
	_len = sizeof(rt_buf); \
	rc = cbprintf_package(rt_buf, &_len, package_flags, __VA_ARGS__); \
	zassert_true(rc >= 0, NULL); \
	\
	rc = package_cmp(package_flags, buf, len, rt_buf, _len); \
	zassert_equal(rc, 0, NULL); \
} while (0)

static void test_CBPRINTF_STATIC_PACKAGE(void)
{
	if (!IS_ENABLED(USE_PACKAGED)) {
		return;
	}

	if (!IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		return;
	}

	/* Test various arguments and validate that static packaging forms
	 * correct content.
	 */
	TEST_STATIC_PACKAGE("test");

	short s = -100;
	static const short cs = -100;
	volatile short vs = -100;
	static const volatile short cvs = -100;

	TEST_STATIC_PACKAGE("%d %d %d %d", s, cs, vs, cvs);

	float f = 1.11;
	static const float cf = 1.11;
	volatile float vf = 1.11;
	static const volatile float cvf = 1.11;

	TEST_STATIC_PACKAGE("%f %f %f %f", f, cf, vf, cvf);


	uint8_t *pc = NULL;
	static const void *cpc = NULL;
	volatile void *vpc = NULL;
	volatile double * const vcpc = NULL;
	static const volatile float *cvpc = NULL;

	TEST_STATIC_PACKAGE("%p %p %p %p %p %p",
			    pc, cpc, vpc, vcpc, cvpc, (const void *)NULL);

	unsigned long long ll = 0x1234567887654321;
	static const unsigned long long  cll = 0x1234567887654321;
	volatile unsigned long long  vll = 0x1234567887654321;
	static const volatile unsigned long long  cvll = 0x1234567887654321;


	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		TEST_STATIC_PACKAGE("%llu %llu %llu %llu", ll, cll, vll, cvll);
	}
}

#define TEST_STATIC_PACKAGE_PADDED(exp_str, ...) do { \
	CBPRINTF_STATIC_PACKAGE_SIZE(len, USE_PACKAGE_FMT_AS_PTR, __VA_ARGS__);\
	uint8_t _buf[len]; \
	size_t _len = len; \
	int rc; \
	\
	CBPRINTF_STATIC_PACKAGE(_buf, _len, USE_PACKAGE_FMT_AS_PTR, \
		       		__VA_ARGS__); \
	 \
	(void)_len; \
	reset_out(); \
	int l = cbpprintf(out, NULL, USE_PACKAGE_FMT_AS_PTR, _buf); \
	buf[l] = 0; \
	rc = strcmp(exp_str, (char *)buf); \
	zassert_equal(rc, 0, NULL); \
} while(0)

struct test_cbprintf_struct {
	uint32_t x:1;
	uint32_t y:31;
};

static void test_empty_func(void)
{

}

static void test_static_package_exceptions(void)
{
	if (!IS_ENABLED(USE_PACKAGED)) {
		return;
	}

	struct test_cbprintf_struct test_struct = { .x = 1, .y = 20 };


	char tmp_buf[64];

	sprintf(tmp_buf, "%p", test_empty_func);
	TEST_STATIC_PACKAGE_PADDED(tmp_buf, "%p", test_empty_func);

	/* Test bit fields */
	TEST_STATIC_PACKAGE_PADDED("1", "%d", test_struct.x);

	volatile char *vstr = "test";
	TEST_STATIC_PACKAGE_PADDED("test test", "%s %s", "test", vstr);

	char c = 'a';
	static const char cc = 'a';
	volatile char vc = 'a';
	static const volatile char cvc = 'a';

	TEST_STATIC_PACKAGE_PADDED("a a a a", "%c %c %c %c", c, cc, vc, cvc);

	short s = -100;
	static const short cs = -100;
	volatile short vs = -100;
	static const volatile short cvs = -100;

	TEST_STATIC_PACKAGE_PADDED("-100 -100 -100 -100", "%d %d %d %d",
				   (int)s, cs, vs, cvs);

	unsigned int min = 0x4c3c2c1c;
	unsigned int max = 0x4d3d2d1d;

	TEST_STATIC_PACKAGE_PADDED("2c1c/2D1D", "%hx/%hX", min, max);

	TEST_STATIC_PACKAGE_PADDED("1c/1D", "%hhx/%hhX", min, max);

	int dmin = -1234567890;
	int dmax = 1876543210;


	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TEST_STATIC_PACKAGE_PADDED("-722/-14614", "%hd/%hd",
					   dmin, dmax);

		TEST_STATIC_PACKAGE_PADDED("46/-22", "%hhd/%hhd", dmin, dmax);
	}

	double ld = 1.2345;

	TEST_STATIC_PACKAGE_PADDED("%Lg", "%Lg", (long double)ld);

	long long svll = 123LL << 48;

	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		TEST_STATIC_PACKAGE_PADDED(
				"/34621422135410688/-34621422135410688/",
				"/%lld/%lld/", svll, -svll);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		TEST_STATIC_PACKAGE_PADDED("1.2345", "%g", ld);
		TEST_STATIC_PACKAGE_PADDED("1.2345", "%lg", ld);
	}
}

static void test_nop(void)
{
}

/*test case main entry*/
void test_main(void)
{
	if (sizeof(int) == 4) {
		pfx_str += 8U;
		sfx_str += 8U;
	}

	TC_PRINT("Opts: " COND_CODE_1(M64_MODE, ("m64"), ("m32")) "\n");
	if (IS_ENABLED(USE_LIBC)) {
		TC_PRINT(" LIBC");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		TC_PRINT(" COMPLETE");
		if (IS_ENABLED(USE_PACKAGED)) {
			TC_PRINT(" PACKAGED\n");
		} else {
			TC_PRINT(" VA_LIST\n");
		}
		if (IS_ENABLED(USE_PACKAGE_FMT_AS_PTR)) {
			TC_PRINT(" FMT_AS_PTR\n");
		}
	} else {
		TC_PRINT(" NANO\n");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		TC_PRINT(" FULL_INTEGRAL\n");
	} else {
		TC_PRINT(" REDUCED_INTEGRAL\n");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		TC_PRINT(" FP_SUPPORT\n");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		TC_PRINT(" FP_A_SUPPORT\n");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_N_SPECIFIER)) {
		TC_PRINT(" FP_N_SPECIFIER\n");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) {
		TC_PRINT(" LIBC_SUBSTS\n");
	}

	printf("sizeof: int = %zu ; long = %zu ; ptr = %zu\n",
	       sizeof(int), sizeof(long), sizeof(void *));
#ifdef CONFIG_CBPRINTF_COMPLETE
	printf("sizeof(conversion) = %zu\n", sizeof(struct conversion));
#endif

	ztest_test_suite(test_prf,
			 ztest_unit_test(test_pct),
			 ztest_unit_test(test_v_c),
			 ztest_unit_test(test_c),
			 ztest_unit_test(test_s),
			 ztest_unit_test(test_d_length),
			 ztest_unit_test(test_d_flags),
			 ztest_unit_test(test_x_length),
			 ztest_unit_test(test_x_flags),
			 ztest_unit_test(test_o),
			 ztest_unit_test(test_fp_value),
			 ztest_unit_test(test_fp_length),
			 ztest_unit_test(test_fp_flags),
			 ztest_unit_test(test_star_width),
			 ztest_unit_test(test_star_precision),
			 ztest_unit_test(test_n),
			 ztest_unit_test(test_p),
			 ztest_unit_test(test_libc_substs),
			 ztest_unit_test(test_cbprintf_package),
			 ztest_unit_test(test_cbpprintf),
			 ztest_unit_test(test_CBPRINTF_STATIC_PACKAGE),
			 ztest_unit_test(test_MUST_RUNTIME_PACKAGE),
			 ztest_unit_test(test_static_package_exceptions),
			 ztest_unit_test(test_nop)
			 );
	ztest_run_test_suite(test_prf);
}
