/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CONFIG_CBPRINTF_LIBC_SUBSTS 1

#include <ztest.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <wctype.h>
#include <stddef.h>
#include <string.h>
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
#else
#define USE_PACKAGED 0
#endif
#if (VIA_TWISTER & 0x400) != 0
#define CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE 1
#endif
#if (VIA_TWISTER & 0x800) != 0
#define AVOID_C_GENERIC 1
#endif
#if (VIA_TWISTER & 0x1000) != 0
#define PKG_ALIGN_OFFSET sizeof(void *)
#endif

#if (VIA_TWISTER & 0x2000) != 0
#define PACKAGE_FLAGS CBPRINTF_PACKAGE_ADD_STRING_IDXS
#endif

#endif /* VIA_TWISTER */

/* Can't use IS_ENABLED on symbols that don't start with CONFIG_
 * without checkpatch complaints, so do something else.
 */
#if USE_LIBC
#define ENABLED_USE_LIBC true
#else
#define ENABLED_USE_LIBC false
#endif

#if USE_PACKAGED
#define ENABLED_USE_PACKAGED true
#else
#define ENABLED_USE_PACKAGED false
#endif

#if AVOID_C_GENERIC
#define Z_C_GENERIC 0
#endif

#ifndef PACKAGE_FLAGS
#define PACKAGE_FLAGS 0
#endif

#include <sys/cbprintf.h>
#include "../../../lib/os/cbprintf.c"

#if defined(CONFIG_CBPRINTF_COMPLETE)
#include "../../../lib/os/cbprintf_complete.c"
#elif defined(CONFIG_CBPRINTF_NANO)
#include "../../../lib/os/cbprintf_nano.c"
#endif

#if USE_PACKAGED
#include "../../../lib/os/cbprintf_packaged.c"
#endif

#ifndef PKG_ALIGN_OFFSET
#define PKG_ALIGN_OFFSET (size_t)0
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

/* Buffer adequate to hold packaged state for all tested
 * configurations.
 */
#if USE_PACKAGED
static uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) packaged[256];
#endif

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

struct out_buffer {
	char *buf;
	size_t idx;
	size_t size;
};

static struct out_buffer outbuf;

static inline void reset_out(void)
{
	outbuf.buf = buf;
	outbuf.size = ARRAY_SIZE(buf);
	outbuf.idx = 0;
}

static void outbuf_null_terminate(struct out_buffer *outbuf)
{
	int idx = outbuf->idx - ((outbuf->idx == outbuf->size) ? 1 : 0);

	outbuf->buf[idx] = 0;
}

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

__printf_like(2, 3)
static int prf(char *static_package_str, const char *format, ...)
{
	va_list ap;
	int rv;

	reset_out();
	va_start(ap, format);
#if USE_LIBC
	rv = vsnprintf(buf, sizeof(buf), format, ap);
#else
#if USE_PACKAGED
	rv = cbvprintf_package(packaged, sizeof(packaged), PACKAGE_FLAGS, format, ap);
	if (rv >= 0) {
		rv = cbpprintf(out, &outbuf, packaged);
		if (rv == 0 && static_package_str) {
			rv = strcmp(static_package_str, outbuf.buf);
		}
	}
#else
	rv = cbvprintf(out, &outbuf, format, ap);
#endif
	outbuf_null_terminate(&outbuf);
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
	va_list ap2;
	int len;
	uint8_t *pkg_buf = &packaged[PKG_ALIGN_OFFSET];

	va_copy(ap2, ap);
	len = cbvprintf_package(NULL, PKG_ALIGN_OFFSET, PACKAGE_FLAGS, format, ap2);
	va_end(ap2);

	if (len >= 0) {
		rv = cbvprintf_package(pkg_buf, len, PACKAGE_FLAGS, format, ap);
	} else {
		rv = len;
	}
	if (rv >= 0) {
		rv = cbpprintf(out, &outbuf, pkg_buf);
	}
#else
	rv = cbvprintf(out, &outbuf, format, ap);
#endif
	va_end(ap);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)
	    && !IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) {
		zassert_equal(rv, 0, NULL);
		rv = outbuf.idx;
	}
	return rv;
}

#define TEST_PRF2(rc, _fmt, ...) do { \
	char _buf[512]; \
	char *sp_buf = NULL; /* static package buffer */\
	if (USE_PACKAGED && !CBPRINTF_MUST_RUNTIME_PACKAGE(0, _fmt, __VA_ARGS__)) { \
		int rv = 0; \
		size_t _len; \
		struct out_buffer package_buf = { \
			.buf = _buf, .size = ARRAY_SIZE(_buf), .idx = 0 \
		}; \
		CBPRINTF_STATIC_PACKAGE(NULL, 0, _len, PKG_ALIGN_OFFSET, \
					PACKAGE_FLAGS, _fmt, __VA_ARGS__); \
		uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) \
			package[_len + PKG_ALIGN_OFFSET]; \
		int st_pkg_rv; \
		CBPRINTF_STATIC_PACKAGE(&package[PKG_ALIGN_OFFSET], _len - 1, \
					st_pkg_rv, PKG_ALIGN_OFFSET, \
					PACKAGE_FLAGS, _fmt, __VA_ARGS__); \
		zassert_equal(st_pkg_rv, -ENOSPC, NULL); \
		CBPRINTF_STATIC_PACKAGE(&package[PKG_ALIGN_OFFSET], _len, \
					st_pkg_rv, PKG_ALIGN_OFFSET, \
					PACKAGE_FLAGS, _fmt, __VA_ARGS__); \
		zassert_equal(st_pkg_rv, _len, NULL); \
		rv = cbpprintf(out, &package_buf, &package[PKG_ALIGN_OFFSET]); \
		if (rv >= 0) { \
			sp_buf = _buf; \
		} \
	} \
	*rc = prf(sp_buf, _fmt, __VA_ARGS__); \
} while (0)

#define TEST_PRF(rc, _fmt, ...) \
	TEST_PRF2(rc, WRAP_FMT(_fmt), PASS_ARG(__VA_ARGS__))

#ifdef CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE
#define TEST_PRF_LONG_DOUBLE(rc, _fmt, ...) \
	TEST_PRF(rc, _fmt, __VA_ARGS__)
#else
/* Skip testing of static packaging if long double support not enabled. */
#define TEST_PRF_LONG_DOUBLE(rc, _fmt, ...) \
	*rc = prf(NULL, WRAP_FMT(_fmt), PASS_ARG(__VA_ARGS__))
#endif

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

	TEST_PRF(&rc, "/%%/%c/", 'a');

	PRF_CHECK("/%/a/", rc);
}

static void test_c(void)
{
	int rc;

	TEST_PRF(&rc, "%c", 'a');
	PRF_CHECK("a", rc);

	rc = prf(NULL, "/%256c/", 'a');

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

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	TEST_PRF(&rc, "%lc", (wint_t)'a');
	if (ENABLED_USE_LIBC) {
		PRF_CHECK("a", rc);
	} else {
		PRF_CHECK("%lc", rc);
	}
}

static void test_s(void)
{
	const char *s = "123";
	static wchar_t ws[] = L"abc";
	int rc;

	TEST_PRF(&rc, "/%s/", s);
	PRF_CHECK("/123/", rc);

	TEST_PRF(&rc, "/%6s/%-6s/%2s/", s, s, s);
	PRF_CHECK("/   123/123   /123/", rc);

	TEST_PRF(&rc, "/%.6s/%.2s/%.s/", s, s, s);
	PRF_CHECK("/123/12//", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	TEST_PRF(&rc, "%ls", ws);
	if (ENABLED_USE_LIBC) {
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
	if (!ENABLED_USE_LIBC) {
		zassert_equal(buf[1], 'b', "wth %x", buf[1]);
	}
}

static void test_d_length(void)
{
	int min = -1234567890;
	int max = 1876543210;
	long long svll = 123LL << 48;
	long long svll2 = -2LL;
	int rc;

	TEST_PRF(&rc, "%d/%d", min, max);
	PRF_CHECK("-1234567890/1876543210", rc);

	TEST_PRF(&rc, "%u/%u", min, max);
	PRF_CHECK("3060399406/1876543210", rc);

	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TEST_PRF(&rc, "%hd/%hd", min, max);
		PRF_CHECK("-722/-14614", rc);

		TEST_PRF(&rc, "%hhd/%hhd", min, max);
		PRF_CHECK("46/-22", rc);
	}

	TEST_PRF(&rc, "%ld/%ld", (long)min, (long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(long) <= 4)
	    || IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%ld/%ld", rc);
	}

	TEST_PRF(&rc, "/%lld/%lld/%lld/", svll, -svll, svll2);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("/34621422135410688/-34621422135410688/-2/", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		PRF_CHECK("/%lld/%lld/%lld/", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("/ERR/ERR/-2/", rc);
	} else {
		zassert_true(false, "Missed case!");
	}

	TEST_PRF(&rc, "%lld/%lld", (long long)min, (long long)max);
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

	TEST_PRF(&rc, "%jd/%jd", (intmax_t)min, (intmax_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%jd/%jd", rc);
	}

	TEST_PRF(&rc, "%zd/%td/%td", (size_t)min, (ptrdiff_t)min,
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

	/* Stuff related to sign */
	TEST_PRF(&rc, "/%d/%-d/%+d/% d/",
		      sv, sv, sv, sv);
	PRF_CHECK("/123/123/+123/ 123/", rc);

	/* Stuff related to width padding */
	TEST_PRF(&rc, "/%1d/%4d/%-4d/%04d/%15d/%-15d/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/123/ 123/123 /0123/"
		  "            123/123            /", rc);

	/* Stuff related to precision */
	TEST_PRF(&rc, "/%.6d/%6.4d/", sv, sv);
	PRF_CHECK("/000123/  0123/", rc);

	/* Now with negative values */
	sv = -sv;
	TEST_PRF(&rc, "/%d/%-d/%+d/% d/",
		      sv, sv, sv, sv);
	PRF_CHECK("/-123/-123/-123/-123/", rc);

	TEST_PRF(&rc, "/%1d/%6d/%-6d/%06d/%13d/%-13d/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/-123/  -123/-123  /-00123/"
		  "         -123/-123         /", rc);

	TEST_PRF(&rc, "/%.6d/%6.4d/", sv, sv);
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

	TEST_PRF(&rc, "%x/%X", min, max);
	PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);

	TEST_PRF(&rc, "%lx/%lX", (unsigned long)min, (unsigned long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(long) <= 4)
	    || IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%lx/%lX", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	TEST_PRF(&rc, "%hx/%hX", min, max);
	PRF_CHECK("2c1c/2D1D", rc);

	TEST_PRF(&rc, "%hhx/%hhX", min, max);
	PRF_CHECK("1c/1D", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		TEST_PRF(&rc, "%llx/%llX", (unsigned long long)min,
			      (unsigned long long)max);
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);

		TEST_PRF(&rc, "%jx/%jX", (uintmax_t)min, (uintmax_t)max);
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	}

	TEST_PRF(&rc, "%zx/%zX", (size_t)min, (size_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(size_t) <= 4)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%zx/%zX", rc);
	}

	TEST_PRF(&rc, "%tx/%tX", (ptrdiff_t)min, (ptrdiff_t)max);
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

		TEST_PRF(&rc, "%llx/%llX", (unsigned long long)min,
			      (unsigned long long)max);
		PRF_CHECK("8c7c6c5c4c3c2c1c/8D7D6D5D4D3D2D1D", rc);
	}
}

static void test_x_flags(void)
{
	unsigned int sv = 0x123;
	int rc;

	/* Stuff related to sign flags, which have no effect on
	 * unsigned conversions, and alternate form
	 */
	TEST_PRF(&rc, "/%x/%-x/%#x/",
		      sv, sv, sv);
	PRF_CHECK("/123/123/0x123/", rc);

	/* Stuff related to width and padding */
	TEST_PRF(&rc, "/%1x/%4x/%-4x/%04x/%#15x/%-15x/",
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

	TEST_PRF(&rc, "%o", v);
	PRF_CHECK("1234567", rc);
	TEST_PRF(&rc, "%#o", v);
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

	TEST_PRF(&rc, "/%f/%F/", dv, dv);
	PRF_CHECK("/1234.567000/1234.567000/", rc);
	TEST_PRF(&rc, "%g", dv);
	PRF_CHECK("1234.57", rc);
	TEST_PRF(&rc, "%e", dv);
	PRF_CHECK("1.234567e+03", rc);
	TEST_PRF(&rc, "%E", dv);
	PRF_CHECK("1.234567E+03", rc);
	TEST_PRF(&rc, "%a", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x1.34a449ba5e354p+10", rc);
	} else {
		PRF_CHECK("%a", rc);
	}

	dv = 1E3;
	TEST_PRF(&rc, "%.2f", dv);
	PRF_CHECK("1000.00", rc);

	dv = 1E20;
	TEST_PRF(&rc, "%.0f", dv);
	PRF_CHECK("100000000000000000000", rc);
	TEST_PRF(&rc, "%.20e", dv);
	PRF_CHECK("1.00000000000000000000e+20", rc);

	dv = 1E-3;
	TEST_PRF(&rc, "%.3e", dv);
	PRF_CHECK("1.000e-03", rc);

	dv = 1E-3;
	TEST_PRF(&rc, "%g", dv);
	PRF_CHECK("0.001", rc);

	dv = 1234567.89;
	TEST_PRF(&rc, "%g", dv);
	PRF_CHECK("1.23457e+06", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		dv = (double)BIT64(40);
		TEST_PRF(&rc, "/%a/%.4a/%.20a/", dv, dv, dv);
		PRF_CHECK("/0x1p+40/0x1.0000p+40/"
			  "0x1.00000000000000000000p+40/", rc);

		dv += (double)BIT64(32);
		TEST_PRF(&rc, "%a", dv);
		PRF_CHECK("0x1.01p+40", rc);
	}

	dv = INFINITY;
	TEST_PRF(&rc, "%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("inf.f INF.F inf.e INF.E "
			  "inf.g INF.g inf.a INF.A", rc);
	} else {
		PRF_CHECK("inf.f INF.F inf.e INF.E "
			  "inf.g INF.g %a.a %A.A", rc);
	}

	dv = -INFINITY;
	TEST_PRF(&rc, "%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("-inf.f -INF.F -inf.e -INF.E "
			  "-inf.g -INF.g -inf.a -INF.A", rc);
	} else {
		PRF_CHECK("-inf.f -INF.F -inf.e -INF.E "
			  "-inf.g -INF.g %a.a %A.A", rc);
	}

	dv = NAN;
	TEST_PRF(&rc, "%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("nan.f NAN.F nan.e NAN.E "
			  "nan.g NAN.g nan.a NAN.A", rc);
	} else {
		PRF_CHECK("nan.f NAN.F nan.e NAN.E "
			  "nan.g NAN.g %a.a %A.A", rc);
	}

	dv = DBL_MIN;
	TEST_PRF(&rc, "%a %e", dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x1p-1022 2.225074e-308", rc);
	} else {
		PRF_CHECK("%a 2.225074e-308", rc);
	}

	dv /= 4;
	TEST_PRF(&rc, "%a %e", dv, dv);
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
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("0.125", rc);

	dv = 0x1.0p-4;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("0.0625", rc);

	dv = 0x1.8p-4;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("0.09375", rc);

	dv = 0x1.cp-4;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("0.109375", rc);

	dv = 0x1.9999999800000p-7;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("0.01249999999708962", rc);

	dv = 0x1.9999999ffffffp-8;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("0.006250000005820765", rc);

	dv = 0x1.0p+0;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("1", rc);

	dv = 0x1.fffffffffffffp-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("4.450147717014402e-308", rc);

	dv = 0x1.ffffffffffffep-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("4.450147717014402e-308", rc);

	dv = 0x1.ffffffffffffdp-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("4.450147717014401e-308", rc);

	dv = 0x1.0000000000001p-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("2.225073858507202e-308", rc);

	dv = 0x1p-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("2.225073858507201e-308", rc);

	dv = 0x0.fffffffffffffp-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("2.225073858507201e-308", rc);

	dv = 0x0.0000000000001p-1022;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("4.940656458412465e-324", rc);

	dv = 0x1.1fa182c40c60dp-1019;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("2e-307", rc);

	dv = 0x1.fffffffffffffp+1023;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("1.797693134862316e+308", rc);

	dv = 0x1.ffffffffffffep+1023;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("1.797693134862316e+308", rc);

	dv = 0x1.ffffffffffffdp+1023;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("1.797693134862315e+308", rc);

	dv = 0x1.0000000000001p+1023;
	TEST_PRF(&rc, "%.16g", dv);
	PRF_CHECK("8.988465674311582e+307", rc);

	dv = 0x1p+1023;
	TEST_PRF(&rc, "%.16g", dv);
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

	TEST_PRF(&rc, "/%g/", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%g/", rc);
	}

	TEST_PRF(&rc, "/%lg/", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%lg/", rc);
	}

	TEST_PRF_LONG_DOUBLE(&rc, "/%Lg/", (long double)dv);
	if (ENABLED_USE_LIBC) {
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

	TEST_PRF(&rc, "/%g/% g/%+g/", dv, dv, dv);
	PRF_CHECK("/1.23/ 1.23/+1.23/", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		TEST_PRF(&rc, "/%a/%.1a/%.2a/", dv, dv, dv);
		PRF_CHECK("/0x1.3ae147ae147aep+0/"
			  "0x1.4p+0/0x1.3bp+0/", rc);
	}

	dv = -dv;
	TEST_PRF(&rc, "/%g/% g/%+g/", dv, dv, dv);
	PRF_CHECK("/-1.23/-1.23/-1.23/", rc);

	dv = 23;
	TEST_PRF(&rc, "/%g/%#g/%.0f/%#.0f/", dv, dv, dv, dv);
	PRF_CHECK("/23/23.0000/23/23./", rc);

	rc = prf(NULL, "% .380f", 0x1p-400);
	zassert_equal(rc, 383, NULL);
	zassert_equal(strncmp(buf, " 0.000", 6), 0, NULL);
	zassert_equal(strncmp(&buf[119], "00003872", 8), 0, NULL);
}

static void test_star_width(void)
{
	int rc;

	TEST_PRF(&rc, "/%3c/%-3c/", 'a', 'a');
	PRF_CHECK("/  a/a  /", rc);

	TEST_PRF(&rc, "/%*c/%*c/", 3, 'a', -3, 'a');
	PRF_CHECK("/  a/a  /", rc);
}

static void test_star_precision(void)
{
	int rc;

	TEST_PRF(&rc, "/%.*x/%10.*x/",
		      5, 0x12, 5, 0x12);
	PRF_CHECK("/00012/     00012/", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		double dv = 1.2345678;

		TEST_PRF(&rc, "/%.3g/%.5g/%.8g/%g/",
			      dv, dv, dv, dv);
		PRF_CHECK("/1.23/1.2346/1.2345678/1.23457/", rc);

		TEST_PRF(&rc, "/%.*g/%.*g/%.*g/%.*g/",
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

	rc = prf(NULL, "12345%n", &l);
	zassert_equal(l, rc, "%d != %d", l, rc);
	zassert_equal(rc, 5, NULL);


	rc = prf(NULL, "12345%hn", &l_h);
	zassert_equal(l_h, rc, NULL);

	rc = prf(NULL, "12345%hhn", &l_hh);
	zassert_equal(l_hh, rc, NULL);

	rc = prf(NULL, "12345%ln", &l_l);
	zassert_equal(l_l, rc, NULL);

	rc = prf(NULL, "12345%lln", &l_ll);
	zassert_equal(l_ll, rc, NULL);

	rc = prf(NULL, "12345%jn", &l_j);
	zassert_equal(l_j, rc, NULL);

	rc = prf(NULL, "12345%zn", &l_z);
	zassert_equal(l_z, rc, NULL);

	rc = prf(NULL, "12345%tn", &l_t);
	zassert_equal(l_t, rc, NULL);
}

#define EXPECTED_1ARG(_t) (IS_ENABLED(CONFIG_CBPRINTF_NANO) \
			   ? 1U : (sizeof(_t) / sizeof(int)))

static void test_p(void)
{
	if (ENABLED_USE_LIBC) {
		TC_PRINT("skipping on libc\n");
		return;
	}

	uintptr_t uip = 0xcafe21;
	void *ptr = (void *)uip;
	int rc;

	TEST_PRF(&rc, "%p", ptr);
	PRF_CHECK("0xcafe21", rc);
	TEST_PRF(&rc, "%p", NULL);
	PRF_CHECK("(nil)", rc);

	reset_out();
	rc = rawprf("/%12p/", ptr);
	zassert_equal(rc, 14, NULL);
	zassert_equal(strncmp("/    0xcafe21/", buf, rc), 0, NULL);

	reset_out();
	rc = rawprf("/%12p/", NULL);
	zassert_equal(rc, 14, NULL);
	zassert_equal(strncmp("/       (nil)/", buf, rc), 0, NULL);

	reset_out();
	rc = rawprf("/%-12p/", ptr);
	zassert_equal(rc, 14, NULL);
	zassert_equal(strncmp("/0xcafe21    /", buf, rc), 0, NULL);

	reset_out();
	rc = rawprf("/%-12p/", NULL);
	zassert_equal(rc, 14, NULL);
	zassert_equal(strncmp("/(nil)       /", buf, rc), 0, NULL);

	reset_out();
	rc = rawprf("/%.8p/", ptr);
	zassert_equal(rc, 12, NULL);
	zassert_equal(strncmp("/0x00cafe21/", buf, rc), 0, NULL);
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
	if (!ENABLED_USE_PACKAGED) {
		TC_PRINT("disabled\n");
		return;
	}

	int rc;
	char fmt[] = "/%i/";	/* not const */

	/* Verify we can calculate length without storing */
	rc = cbprintf_package(NULL, PKG_ALIGN_OFFSET, PACKAGE_FLAGS, fmt, 3);
	zassert_true(rc > sizeof(int), NULL);

	/* Capture the base package information for future tests. */
	size_t len = rc;
	/* Create a buffer aligned to max argument. */
	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) buf[len + PKG_ALIGN_OFFSET];

	/* Verify we get same length when storing. Pass buffer which may be
	 * unaligned. Same alignment offset was used for space calculation.
	 */
	rc = cbprintf_package(&buf[PKG_ALIGN_OFFSET], len, PACKAGE_FLAGS, fmt, 3);
	zassert_equal(rc, len, NULL);

	/* Verify we get an error if can't store */
	len -= 1;
	rc = cbprintf_package(&buf[PKG_ALIGN_OFFSET], len, PACKAGE_FLAGS, fmt, 3);
	zassert_equal(rc, -ENOSPC, NULL);
}

/* Test using @ref CBPRINTF_PACKAGE_ADD_STRING_IDXS flag.
 * Note that only static packaging is tested here because ro string detection
 * does not work on host testing.
 */
static void test_cbprintf_package_rw_string_indexes(void)
{
	if (!ENABLED_USE_PACKAGED) {
		TC_PRINT("disabled\n");
		return;
	}

	if (!Z_C_GENERIC) {
		/* runtime packaging will not detect ro strings. */
		return;
	}

	int len0, len1;
	static const char *test_str = "test %d %s";
	static const char *test_str1 = "lorem ipsum";
	uint8_t str_idx;
	char *addr;

	CBPRINTF_STATIC_PACKAGE(NULL, 0, len0, 0, 0, test_str, 100, test_str1);
	CBPRINTF_STATIC_PACKAGE(NULL, 0, len1, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);
	/* package with string indexes will contain two more bytes holding indexes
	 * of string parameter locations.
	 */
	zassert_equal(len0 + 2, len1, NULL);

	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package0[len0];
	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package1[len1];

	CBPRINTF_STATIC_PACKAGE(package0, sizeof(package0), len0, 0, 0,
				test_str, 100, test_str1);
	CBPRINTF_STATIC_PACKAGE(package1, sizeof(package1), len1, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);

	struct z_cbprintf_desc *desc0 = (struct z_cbprintf_desc *)package0;
	struct z_cbprintf_desc *desc1 = (struct z_cbprintf_desc *)package1;

	/* Compare descriptor content. Second package has one ro string index. */
	zassert_equal(desc0->ro_str_cnt, 0, NULL);
	zassert_equal(desc1->ro_str_cnt, 2, NULL);
	zassert_equal(len0 + 2, len1, NULL);

	int *p = (int *)package1;

	str_idx = package1[len0];
	addr = *(char **)&p[str_idx];
	zassert_equal(addr, test_str, NULL);

	str_idx = package1[len0 + 1];
	addr = *(char **)&p[str_idx];
	zassert_equal(addr, test_str1, NULL);
}

static int fsc_package_cb(int c, void *ctx)
{
	char **p = ctx;

	(*p)[0] = c;
	*p = *p + 1;

	return c;
}

/* Test for validating convesion to fully self-contained package. */
static void test_cbprintf_fsc_package(void)
{
	if (!ENABLED_USE_PACKAGED) {
		TC_PRINT("disabled\n");
		return;
	}

	if (!Z_C_GENERIC) {
		/* runtime packaging will not detect ro strings. */
		return;
	}

	char test_str[] = "test %d %s";
	char *test_str1 = "lorem ipsum";
	char exp_str0[256];
	char exp_str1[256];
	char out_str[256];
	char *pout;
	int len;
	int fsc_len;
	int err;

	snprintf(exp_str0, sizeof(exp_str0), test_str, 100, test_str1);

	CBPRINTF_STATIC_PACKAGE(NULL, 0, len, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);

	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) package[len];

	CBPRINTF_STATIC_PACKAGE(package, sizeof(package), len, 0,
				CBPRINTF_PACKAGE_ADD_STRING_IDXS,
				test_str, 100, test_str1);

	/* Get length of fsc package. */
	fsc_len = cbprintf_fsc_package(package, len, NULL, 0);

	int exp_len = len + (int)strlen(test_str) + 1 + (int)strlen(test_str1) + 1;

	zassert_equal(exp_len, fsc_len, NULL);

	uint8_t __aligned(CBPRINTF_PACKAGE_ALIGNMENT) fsc_package[fsc_len];

	err = cbprintf_fsc_package(package, len, fsc_package, fsc_len - 1);
	zassert_equal(err, -ENOSPC, NULL);

	err = cbprintf_fsc_package(package, len, fsc_package, fsc_len);
	zassert_equal(err, fsc_len, NULL);

	/* Now overwrite a char in original string, confirm that fsc package
	 * contains string without that change because ro string is copied into
	 * the package.
	 */
	test_str[0] = 'w';
	snprintf(exp_str1, sizeof(exp_str1), test_str, 100, test_str1);

	pout = out_str;
	cbpprintf(fsc_package_cb, &pout, package);
	*pout = '\0';

	zassert_equal(strcmp(out_str, exp_str1), 0, NULL);
	zassert_true(strcmp(exp_str0, exp_str1) != 0, NULL);

	/* FSC package contains original content. */
	pout = out_str;
	cbpprintf(fsc_package_cb, &pout, fsc_package);
	*pout = '\0';
	zassert_equal(strcmp(out_str, exp_str0), 0, NULL);
}

static void test_cbpprintf(void)
{
	if (!ENABLED_USE_PACKAGED) {
		TC_PRINT("disabled\n");
		return;
	}

	int rc;

	/* This only checks error conditions.  Formatting is checked
	 * by diverting prf() and related helpers to use the packaged
	 * version.
	 */
	reset_out();
	rc = cbpprintf(out, &outbuf, NULL);
	zassert_equal(rc, -EINVAL, NULL);
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
	if (ENABLED_USE_LIBC) {
		TC_PRINT(" LIBC");
	}
	if (IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		TC_PRINT(" COMPLETE");
	} else {
		TC_PRINT(" NANO\n");
	}
	if (ENABLED_USE_PACKAGED) {
		TC_PRINT(" PACKAGED %s C11 _Generic\n",
				Z_C_GENERIC ? "with" : "without");
	} else {
		TC_PRINT(" VA_LIST\n");
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

	printf("sizeof:  int=%zu long=%zu ptr=%zu long long=%zu double=%zu long double=%zu\n",
	       sizeof(int), sizeof(long), sizeof(void *), sizeof(long long),
	       sizeof(double), sizeof(long double));
	printf("alignof: int=%zu long=%zu ptr=%zu long long=%zu double=%zu long double=%zu\n",
	       __alignof__(int), __alignof__(long), __alignof__(void *),
	       __alignof__(long long), __alignof__(double), __alignof__(long double));
#ifdef CONFIG_CBPRINTF_COMPLETE
	printf("sizeof(conversion) = %zu\n", sizeof(struct conversion));
#endif

#ifdef USE_PACKAGED
	printf("package alignment offset = %zu\n", PKG_ALIGN_OFFSET);
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
			 ztest_unit_test(test_cbprintf_package_rw_string_indexes),
			 ztest_unit_test(test_cbprintf_fsc_package),
			 ztest_unit_test(test_nop)
			 );
	ztest_run_test_suite(test_prf);
}
