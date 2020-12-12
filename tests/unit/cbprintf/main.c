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

/* Unit testing doesn't use Kconfig, so if we're not building from
 * twister force selection of all features.  If we are use flags to
 * determine which features are desired.  Yes, this is a mess.
 */
#ifndef VIA_TWISTER
/* Set this to truthy to use libc's snprintf as external validation.
 * This should be used with all options enabled.
 */
#define USE_LIBC 0
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

#endif /* VIA_TWISTER */

/* Can't use IS_ENABLED on symbols that don't start with CONFIG_
 * without checkpatch complaints, so do something else.
 */
#if USE_LIBC
#define ENABLED_USE_LIBC true
#else
#define ENABLED_USE_LIBC false
#endif

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

__printf_like(1, 2)
static int prf(const char *format, ...)
{
	va_list ap;
	int rv;

	reset_out();
	va_start(ap, format);
#if USE_LIBC
	rv = vsnprintf(buf, sizeof(buf), format, ap);
#else
	reset_out();
	rv = cbvprintf(out, NULL, format, ap);
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
	rv = cbvprintf(out, NULL, format, ap);
	va_end(ap);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)
	    && !IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) {
		zassert_equal(rv, 0, NULL);
		rv = bp - buf;
	}
	return rv;
}

#define TEST_PRF(_fmt, ...) prf(WRAP_FMT(_fmt), PASS_ARG(__VA_ARGS__))

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
	int rc = TEST_PRF("/%%/%c/", 'a');

	PRF_CHECK("/%/a/", rc);
}

static void test_c(void)
{
	int rc;

	rc = TEST_PRF("%c", 'a');
	PRF_CHECK("a", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	rc = TEST_PRF("%lc", (wint_t)'a');
	if (ENABLED_USE_LIBC) {
		PRF_CHECK("a", rc);
	} else {
		PRF_CHECK("%lc", rc);
	}

	rc = prf("/%256c/", 'a');

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

	rc = TEST_PRF("/%s/", s);
	PRF_CHECK("/123/", rc);

	rc = TEST_PRF("/%6s/%-6s/%2s/", s, s, s);
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("/123/123   /123/", rc);
	} else {
		PRF_CHECK("/   123/123   /123/", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	rc = TEST_PRF("/%.6s/%.2s/%.s/", s, s, s);
	PRF_CHECK("/123/12//", rc);

	rc = TEST_PRF("%ls", ws);
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
	int rc;

	rc = TEST_PRF("%d/%d", min, max);
	PRF_CHECK("-1234567890/1876543210", rc);

	if (!IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		rc = TEST_PRF("%hd/%hd", min, max);
		PRF_CHECK("-722/-14614", rc);

		rc = TEST_PRF("%hhd/%hhd", min, max);
		PRF_CHECK("46/-22", rc);
	}

	rc = TEST_PRF("%ld/%ld", (long)min, (long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(long) <= 4)
	    || IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%ld/%ld", rc);
	}

	rc = TEST_PRF("/%lld/%lld/", svll, -svll);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("/34621422135410688/-34621422135410688/", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_COMPLETE)) {
		PRF_CHECK("/%lld/%lld/", rc);
	} else if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("/ERR/ERR/", rc);
	} else {
		zassert_true(false, "Missed case!");
	}

	rc = TEST_PRF("%lld/%lld", (long long)min, (long long)max);
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

	rc = TEST_PRF("%jd/%jd", (intmax_t)min, (intmax_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		PRF_CHECK("-1234567890/1876543210", rc);
	} else {
		PRF_CHECK("%jd/%jd", rc);
	}

	rc = TEST_PRF("%zd/%td/%td", (size_t)min, (ptrdiff_t)min,
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
	rc = TEST_PRF("/%d/%-d/%+d/% d/",
		      sv, sv, sv, sv);
	PRF_CHECK("/123/123/+123/ 123/", rc);

	/* Stuff related to width padding */
	rc = TEST_PRF("/%1d/%4d/%-4d/%04d/%15d/%-15d/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/123/ 123/123 /0123/"
		  "            123/123            /", rc);

	/* Stuff related to precision */
	rc = TEST_PRF("/%.6d/%6.4d/", sv, sv);
	PRF_CHECK("/000123/  0123/", rc);

	/* Now with negative values */
	sv = -sv;
	rc = TEST_PRF("/%d/%-d/%+d/% d/",
		      sv, sv, sv, sv);
	PRF_CHECK("/-123/-123/-123/-123/", rc);

	rc = TEST_PRF("/%1d/%6d/%-6d/%06d/%13d/%-13d/",
		      sv, sv, sv, sv, sv, sv);
	PRF_CHECK("/-123/  -123/-123  /-00123/"
		  "         -123/-123         /", rc);

	rc = TEST_PRF("/%.6d/%6.4d/", sv, sv);
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

	rc = TEST_PRF("%x/%X", min, max);
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		PRF_CHECK("4c3c2c1c/4d3d2d1d", rc);
	} else {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("short test for nano\n");
		return;
	}

	rc = TEST_PRF("%hx/%hX", min, max);
	PRF_CHECK("2c1c/2D1D", rc);

	rc = TEST_PRF("%hhx/%hhX", min, max);
	PRF_CHECK("1c/1D", rc);

	rc = TEST_PRF("%lx/%lX", (unsigned long)min, (unsigned long)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(long) <= 4)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%lx/%lX", rc);
	}

	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)) {
		rc = TEST_PRF("%llx/%llX", (unsigned long long)min,
			      (unsigned long long)max);
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);

		rc = TEST_PRF("%jx/%jX", (uintmax_t)min, (uintmax_t)max);
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	}

	rc = TEST_PRF("%zx/%zX", (size_t)min, (size_t)max);
	if (IS_ENABLED(CONFIG_CBPRINTF_FULL_INTEGRAL)
	    || (sizeof(size_t) <= 4)) {
		PRF_CHECK("4c3c2c1c/4D3D2D1D", rc);
	} else {
		PRF_CHECK("%zx/%zX", rc);
	}

	rc = TEST_PRF("%tx/%tX", (ptrdiff_t)min, (ptrdiff_t)max);
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

		rc = TEST_PRF("%llx/%llX", (unsigned long long)min,
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
	rc = TEST_PRF("/%x/%-x/%#x/",
		      sv, sv, sv);
	PRF_CHECK("/123/123/0x123/", rc);

	/* Stuff related to width and padding */
	rc = TEST_PRF("/%1x/%4x/%-4x/%04x/%#15x/%-15x/",
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

	rc = TEST_PRF("%o", v);
	PRF_CHECK("1234567", rc);
	rc = TEST_PRF("%#o", v);
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

	rc = TEST_PRF("/%f/%F/", dv, dv);
	PRF_CHECK("/1234.567000/1234.567000/", rc);
	rc = TEST_PRF("%g", dv);
	PRF_CHECK("1234.57", rc);
	rc = TEST_PRF("%e", dv);
	PRF_CHECK("1.234567e+03", rc);
	rc = TEST_PRF("%E", dv);
	PRF_CHECK("1.234567E+03", rc);
	rc = TEST_PRF("%a", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x1.34a449ba5e354p+10", rc);
	} else {
		PRF_CHECK("%a", rc);
	}

	dv = 1E3;
	rc = TEST_PRF("%.2f", dv);
	PRF_CHECK("1000.00", rc);

	dv = 1E20;
	rc = TEST_PRF("%.0f", dv);
	PRF_CHECK("100000000000000000000", rc);
	rc = TEST_PRF("%.20e", dv);
	PRF_CHECK("1.00000000000000000000e+20", rc);

	dv = 1E-3;
	rc = TEST_PRF("%.3e", dv);
	PRF_CHECK("1.000e-03", rc);

	dv = 1E-3;
	rc = TEST_PRF("%g", dv);
	PRF_CHECK("0.001", rc);

	dv = 1234567.89;
	rc = TEST_PRF("%g", dv);
	PRF_CHECK("1.23457e+06", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		dv = (double)BIT64(40);
		rc = TEST_PRF("/%a/%.4a/%.20a/", dv, dv, dv);
		PRF_CHECK("/0x1p+40/0x1.0000p+40/"
			  "0x1.00000000000000000000p+40/", rc);

		dv += (double)BIT64(32);
		rc = TEST_PRF("%a", dv);
		PRF_CHECK("0x1.01p+40", rc);
	}

	dv = INFINITY;
	rc = TEST_PRF("%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("inf.f INF.F inf.e INF.E "
			  "inf.g INF.g inf.a INF.A", rc);
	} else {
		PRF_CHECK("inf.f INF.F inf.e INF.E "
			  "inf.g INF.g %a.a %A.A", rc);
	}

	dv = -INFINITY;
	rc = TEST_PRF("%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("-inf.f -INF.F -inf.e -INF.E "
			  "-inf.g -INF.g -inf.a -INF.A", rc);
	} else {
		PRF_CHECK("-inf.f -INF.F -inf.e -INF.E "
			  "-inf.g -INF.g %a.a %A.A", rc);
	}

	dv = NAN;
	rc = TEST_PRF("%f.f %F.F %e.e %E.E %g.g %G.g %a.a %A.A",
		      dv, dv, dv, dv, dv, dv, dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("nan.f NAN.F nan.e NAN.E "
			  "nan.g NAN.g nan.a NAN.A", rc);
	} else {
		PRF_CHECK("nan.f NAN.F nan.e NAN.E "
			  "nan.g NAN.g %a.a %A.A", rc);
	}

	dv = DBL_MIN;
	rc = TEST_PRF("%a %e", dv, dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		PRF_CHECK("0x1p-1022 2.225074e-308", rc);
	} else {
		PRF_CHECK("%a 2.225074e-308", rc);
	}

	dv /= 4;
	rc = TEST_PRF("%a %e", dv, dv);
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
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("0.125", rc);

	dv = 0x1.0p-4;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("0.0625", rc);

	dv = 0x1.8p-4;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("0.09375", rc);

	dv = 0x1.cp-4;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("0.109375", rc);

	dv = 0x1.9999999800000p-7;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("0.01249999999708962", rc);

	dv = 0x1.9999999ffffffp-8;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("0.006250000005820765", rc);

	dv = 0x1.0p+0;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("1", rc);

	dv = 0x1.fffffffffffffp-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("4.450147717014402e-308", rc);

	dv = 0x1.ffffffffffffep-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("4.450147717014402e-308", rc);

	dv = 0x1.ffffffffffffdp-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("4.450147717014401e-308", rc);

	dv = 0x1.0000000000001p-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("2.225073858507202e-308", rc);

	dv = 0x1p-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("2.225073858507201e-308", rc);

	dv = 0x0.fffffffffffffp-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("2.225073858507201e-308", rc);

	dv = 0x0.0000000000001p-1022;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("4.940656458412465e-324", rc);

	dv = 0x1.1fa182c40c60dp-1019;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("2e-307", rc);

	dv = 0x1.fffffffffffffp+1023;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("1.797693134862316e+308", rc);

	dv = 0x1.ffffffffffffep+1023;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("1.797693134862316e+308", rc);

	dv = 0x1.ffffffffffffdp+1023;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("1.797693134862315e+308", rc);

	dv = 0x1.0000000000001p+1023;
	rc = TEST_PRF("%.16g", dv);
	PRF_CHECK("8.988465674311582e+307", rc);

	dv = 0x1p+1023;
	rc = TEST_PRF("%.16g", dv);
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

	rc = TEST_PRF("/%g/", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%g/", rc);
	}

	rc = TEST_PRF("/%lg/", dv);
	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		PRF_CHECK("/1.2345/", rc);
	} else {
		PRF_CHECK("/%lg/", rc);
	}

	rc = TEST_PRF("/%Lg/", (long double)dv);
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

	rc = TEST_PRF("/%g/% g/%+g/", dv, dv, dv);
	PRF_CHECK("/1.23/ 1.23/+1.23/", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_A_SUPPORT)) {
		rc = TEST_PRF("/%a/%.1a/%.2a/", dv, dv, dv);
		PRF_CHECK("/0x1.3ae147ae147aep+0/"
			  "0x1.4p+0/0x1.3bp+0/", rc);
	}

	dv = -dv;
	rc = TEST_PRF("/%g/% g/%+g/", dv, dv, dv);
	PRF_CHECK("/-1.23/-1.23/-1.23/", rc);

	dv = 23;
	rc = TEST_PRF("/%g/%#g/%.0f/%#.0f/", dv, dv, dv, dv);
	PRF_CHECK("/23/23.0000/23/23./", rc);

	rc = prf("% .380f", 0x1p-400);
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

	rc = TEST_PRF("/%3c/%-3c/", 'a', 'a');
	PRF_CHECK("/  a/a  /", rc);

	rc = TEST_PRF("/%*c/%*c/", 3, 'a', -3, 'a');
	PRF_CHECK("/  a/a  /", rc);
}

static void test_star_precision(void)
{
	if (IS_ENABLED(CONFIG_CBPRINTF_NANO)) {
		TC_PRINT("skipped test for nano\n");
		return;
	}

	int rc;

	rc = TEST_PRF("/%.*x/%10.*x/",
		      5, 0x12, 5, 0x12);
	PRF_CHECK("/00012/     00012/", rc);

	if (IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT)) {
		double dv = 1.2345678;

		rc = TEST_PRF("/%.3g/%.5g/%.8g/%g/",
			      dv, dv, dv, dv);
		PRF_CHECK("/1.23/1.2346/1.2345678/1.23457/", rc);

		rc = TEST_PRF("/%.*g/%.*g/%.*g/%.*g/",
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

	rc = prf("12345%n", &l);
	zassert_equal(l, rc, "%d != %d", l, rc);
	zassert_equal(rc, 5, NULL);


	rc = prf("12345%hn", &l_h);
	zassert_equal(l_h, rc, NULL);

	rc = prf("12345%hhn", &l_hh);
	zassert_equal(l_hh, rc, NULL);

	rc = prf("12345%ln", &l_l);
	zassert_equal(l_l, rc, NULL);

	rc = prf("12345%lln", &l_ll);
	zassert_equal(l_ll, rc, NULL);

	rc = prf("12345%jn", &l_j);
	zassert_equal(l_j, rc, NULL);

	rc = prf("12345%zn", &l_z);
	zassert_equal(l_z, rc, NULL);

	rc = prf("12345%tn", &l_t);
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

	rc = TEST_PRF("%p", ptr);
	PRF_CHECK("0xcafe21", rc);
	rc = TEST_PRF("%p", NULL);
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
		TC_PRINT(" COMPLETE\n");
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
			 ztest_unit_test(test_nop)
			 );
	ztest_run_test_suite(test_prf);
}
