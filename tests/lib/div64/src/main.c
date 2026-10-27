/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/div64.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define RANDOM_DIVIDENDS 1000
#define RANDOM_DIVISORS  5000

/* Divisors with and without the bias, with and without the short multiply,
 * powers of two and their neighbors, and the rates time conversions use.
 */
#define FOR_EACH_DIVISOR(X)                                                                        \
	X(1) X(2) X(3) X(5) X(6) X(7) X(10) X(12) X(60) X(100) X(400) X(641) X(1000) X(3600)    \
	X(10000) X(15625) X(32767) X(32768) X(32769) X(60000) X(86400) X(1000000) X(6700417)   \
	X(19200000) X(24000000) X(32000000) X(1000000000) X(0x55555555) X(0x7fffffff)          \
	X(0x80000000) X(0x80000001) X(0xaaaaaaab) X(3000000000) X(0xfffffffe) X(0xffffffff)

static uint64_t xorshift(uint64_t *s)
{
	*s ^= *s << 13;
	*s ^= *s >> 7;
	*s ^= *s << 17;

	return *s;
}

/* Edge cases first, then random values of random width. */
static uint64_t dividend(uint32_t i, uint32_t d, uint64_t *s)
{
	uint64_t last = (UINT64_MAX / d) * d;

	switch (i) {
	case 0:
		return 0U;
	case 1:
		return d - 1U;
	case 2:
		return d;
	case 3:
		return last - 1U;
	case 4:
		return last;
	case 5:
		return UINT64_MAX - 1U;
	case 6:
		return UINT64_MAX;
	case 7:
		return BIT64(63);
	case 8:
		return BIT64(32) - 1U;
	case 9:
		return BIT64(32);
	default:
		break;
	}

	uint64_t v = xorshift(s);

	return v >> (xorshift(s) & 63U);
}

static void check_recip(const struct sys_div64_recip *r, uint32_t d, uint32_t count, uint64_t *s)
{
	for (uint32_t i = 0; i < count; i++) {
		uint64_t n = dividend(i, d, s);

		zassert_equal(sys_div64_recip_div(n, r), n / d, "%llu / %u",
			      (unsigned long long)n, d);
		zassert_equal(sys_div64_recip_mod(n, r), (uint32_t)(n % d), "%llu %% %u",
			      (unsigned long long)n, d);
	}
}

/* One function per divisor, so the divisor stays a constant expression and the
 * stack holds one divisor's locals at a time.
 */
#define DEFINE_CHECK_CONST(d)                                                                      \
	static void check_const_##d(uint64_t *s)                                                   \
	{                                                                                          \
		for (uint32_t i = 0; i < RANDOM_DIVIDENDS; i++) {                                  \
			uint64_t n = dividend(i, d, s);                                            \
			int64_t sn = (int64_t)n;                                                   \
                                                                                                   \
			zassert_equal(sys_div64_u32_const(n, d), n / (d), "%llu / %u",             \
				      (unsigned long long)n, (uint32_t)(d));                       \
			zassert_equal(sys_div64_u32_const_inline(n, d), n / (d), "%llu / %u",      \
				      (unsigned long long)n, (uint32_t)(d));                       \
			zassert_equal(sys_mod64_u32_const(n, d), (uint32_t)(n % (d)),              \
				      "%llu %% %u", (unsigned long long)n, (uint32_t)(d));         \
			zassert_equal(sys_sdiv64_u32_const(sn, d), sn / (int64_t)(d),              \
				      "%lld / %u", (long long)sn, (uint32_t)(d));                  \
		}                                                                                  \
	}

FOR_EACH_DIVISOR(DEFINE_CHECK_CONST)

#define CHECK_CONST_ENTRY(d) check_const_##d,

ZTEST(div64, test_const)
{
	static void (*const checks[])(uint64_t *s) = {FOR_EACH_DIVISOR(CHECK_CONST_ENTRY)};
	uint64_t s = 0x139408dcbbf7a44ULL;

	ARRAY_FOR_EACH(checks, i) {
		checks[i](&s);
	}
}

ZTEST(div64, test_const_fold)
{
	static const uint64_t folded[] = {
		sys_div64_u32_const(123456789012345ULL, 1000),
		sys_div64_u32_const(UINT64_MAX, 10),
		sys_div64_u32_const(UINT64_MAX, 0xffffffff),
		sys_div64_u32_const(BIT64(40), 32768),
		sys_mod64_u32_const(123456789012345ULL, 1000),
		sys_sdiv64_u32_const(-123456789012345LL, 1000),
	};

	zassert_equal(folded[0], 123456789012ULL);
	zassert_equal(folded[1], UINT64_MAX / 10U);
	zassert_equal(folded[2], 0x100000001ULL);
	zassert_equal(folded[3], BIT64(25));
	zassert_equal(folded[4], 345U);
	zassert_equal((int64_t)folded[5], -123456789012LL);
}

ZTEST(div64, test_single_eval)
{
	static const int64_t v[] = {-123456789012345LL, 123456789012345LL};
	uint32_t i = 0U;

	zassert_equal(sys_sdiv64_u32_const(v[i++], 1000), -123456789012LL);
	zassert_equal(sys_sdiv64_u32_const(v[i++], 1000), 123456789012LL);
	zassert_equal(i, 2U);
	zassert_equal(sys_div64_u32_const(v[--i], 1000), 123456789012ULL);
	zassert_equal(sys_mod64_u32_const(v[i--], 1000), 345U);
	zassert_equal(i, 0U);
}

#if defined(CONFIG_64BIT)
#define CHECK_RECIP_FIELDS(r, rs, d)
#else
#define CHECK_RECIP_FIELDS(r, rs, d)                                                               \
	zassert_equal((r).m, (rs).m, "m for %u", (uint32_t)(d));                                   \
	zassert_equal((r).flags, (rs).flags, "flags for %u", (uint32_t)(d))
#endif

/* SYS_DIV64_RECIP_INIT() divides correctly and matches sys_div64_recip_init(). */
#define DEFINE_CHECK_STATIC(d)                                                                     \
	static void check_static_##d(uint64_t *s)                                                  \
	{                                                                                          \
		static const struct sys_div64_recip rs = SYS_DIV64_RECIP_INIT(d);                  \
		struct sys_div64_recip r;                                                          \
                                                                                                   \
		sys_div64_recip_init(&r, d);                                                       \
		zassert_equal(r.div, rs.div, "divisor %u", (uint32_t)(d));                         \
		CHECK_RECIP_FIELDS(r, rs, d);                                                      \
		check_recip(&rs, d, RANDOM_DIVIDENDS, s);                                          \
	}

FOR_EACH_DIVISOR(DEFINE_CHECK_STATIC)

#define CHECK_STATIC_ENTRY(d) check_static_##d,

ZTEST(div64, test_recip_static)
{
	static void (*const checks[])(uint64_t *s) = {FOR_EACH_DIVISOR(CHECK_STATIC_ENTRY)};
	uint64_t s = 0x2545f4914f6cdd1dULL;

	ARRAY_FOR_EACH(checks, i) {
		checks[i](&s);
	}
}

#if !defined(CONFIG_64BIT)
/* The shared reciprocals divide by their value. */
#define CHECK_SHARED(name, v)                                                                      \
	if ((v) > 1U) {                                                                            \
		check_recip(&z_div64_recip_##name, (v), RANDOM_DIVIDENDS, &s);                     \
	}

ZTEST(div64, test_shared)
{
	uint64_t s = 0x5851f42d4c957f2dULL;

	Z_DIV64_SHARED_LIST(CHECK_SHARED)
}
#endif

ZTEST(div64, test_recip_runtime)
{
	uint64_t s = 0x9e3779b97f4a7c15ULL;
	struct sys_div64_recip r;

	for (uint32_t d = 1U; d <= 4096U; d++) {
		sys_div64_recip_init(&r, d);
		check_recip(&r, d, 16U, &s);
	}

	for (uint32_t k = 0U; k < 32U; k++) {
		uint32_t pow2 = BIT(k);

		sys_div64_recip_init(&r, pow2);
		check_recip(&r, pow2, 32U, &s);
		sys_div64_recip_init(&r, pow2 + 1U);
		check_recip(&r, pow2 + 1U, 32U, &s);
		if (pow2 > 1U) {
			sys_div64_recip_init(&r, pow2 - 1U);
			check_recip(&r, pow2 - 1U, 32U, &s);
		}
	}

	for (uint32_t j = 0U; j < RANDOM_DIVISORS; j++) {
		uint32_t d = (uint32_t)xorshift(&s) >> (xorshift(&s) & 31U);

		if (d == 0U) {
			continue;
		}
		sys_div64_recip_init(&r, d);
		check_recip(&r, d, 12U, &s);
	}
}

ZTEST_SUITE(div64, NULL, NULL, NULL, NULL, NULL);
