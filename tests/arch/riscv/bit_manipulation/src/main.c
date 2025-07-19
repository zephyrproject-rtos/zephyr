/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#if CONFIG_RISCV_ISA_EXT_ZBA
ZTEST(riscv_bit_manipulation, test_zba)
{
	unsigned long i = 1;
	unsigned long j = 5;
	unsigned long k = 0;

	/* sh1add: (1 << 1) + 5 = 7 */

	__asm__ volatile("sh1add %0, %1, %2;"
			: "=r"(k)
			: "r"(i), "r"(j));
	zexpect_equal(k, 7, "sh1add");

	/* sh2add: (1 << 2) + 5 = 9 */

	__asm__ volatile("sh2add %0, %1, %2;"
			: "=r"(k)
			: "r"(i), "r"(j));
	zexpect_equal(k, 9, "sh2add");

	/* sh3add: (1 << 3) + 5 = 13 */

	__asm__ volatile("sh3add %0, %1, %2;"
			: "=r"(k)
			: "r"(i), "r"(j));
	zexpect_equal(k, 13, "sh3add");

#if CONFIG_RISCV_ISA_RV64I
	i = UINT64_MAX - UINT32_MAX;
	j = (1ULL << 32) + 1;

	/* add.uw:
	 * (UINT64_MAX - UINT32_MAX) + (((1ULL << 32) + 1) & UINT32_MAX) =
	 * UINT64_MAX - UINT32_MAX + 1
	 */
	__asm__ volatile("add.uw %0, %1, %2;"
			: "=r"(k)
			: "r"(j), "r"(i));
	zexpect_equal(k, UINT64_MAX - UINT32_MAX + 1, "add.uw");

	/* sh1add.uw:
	 * (UINT64_MAX - UINT32_MAX) + ((((1ULL << 32) + 1) & UINT32_MAX) << 1) =
	 * UINT64_MAX - UINT32_MAX + 2
	 */
	__asm__ volatile("sh1add.uw %0, %1, %2;"
			: "=r"(k)
			: "r"(j), "r"(i));
	zexpect_equal(k, UINT64_MAX - UINT32_MAX + 2, "sh1add.uw");

	/* sh2add.uw:
	 * (UINT64_MAX - UINT32_MAX) + ((((1ULL << 32) + 1) & UINT32_MAX) << 2) = UINT64_MAX
	 * - UINT32_MAX + 4
	 */
	__asm__ volatile("sh2add.uw %0, %1, %2;"
			: "=r"(k)
			: "r"(j), "r"(i));
	zexpect_equal(k, UINT64_MAX - UINT32_MAX + 4, "sh2add.uw");

	/* sh3add.uw:
	 * (UINT64_MAX - UINT32_MAX) + ((((1ULL << 32) + 1) & UINT32_MAX) << 2) = UINT64_MAX
	 * - UINT32_MAX + 8
	 */
	__asm__ volatile("sh3add.uw %0, %1, %2;"
			: "=r"(k)
			: "r"(j), "r"(i));
	zexpect_equal(k, UINT64_MAX - UINT32_MAX + 8, "sh3add.uw");

	/* slli.uw:
	 * (((1ULL << 32) + 1) & UINT32_MAX) << 5 = 32
	 */
	__asm__ volatile("slli.uw %0, %1, 5;"
			: "=r"(k)
			: "r"(j));
	zexpect_equal(k, 32, "slli.uw");
#endif
}
#endif

#if CONFIG_RISCV_ISA_EXT_ZBB
ZTEST(riscv_bit_manipulation, test_zbb)
{
	unsigned long a = 1;
	unsigned long b = 3;
	unsigned long c = 0;

	/* andn: 3 & ~1 = 2 */
	__asm__ volatile("andn %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 2, "andn");

	/* orn: 3 | ~1 = ULONG_MAX */
	__asm__ volatile("orn %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, ULONG_MAX, "orn");

	/* xnor: ~(3 ^ 1) = ULONG_MAX - 2 */
	__asm__ volatile("xnor %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, ULONG_MAX - 2, "xnor");

	/* clz: CLZ(1) = sizeof(unsigned long) * 8 - 1 */
	__asm__ volatile("clz %0, %1;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, sizeof(a) * 8 - 1, "clz");

	/* ctz: CTZ(1) = 0 */
	__asm__ volatile("ctz %0, %1;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, 0, "ctz");

#if CONFIG_RISCV_ISA_RV64I
	/* clzw: CLZW(1) = sizeof(uint32_t) * 8 - 1 */
	__asm__ volatile("clzw %0, %1;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, sizeof(uint32_t) * 8 - 1, "clzw");

	a = 0;
	/* ctzw: CTZW(0) = sizeof(uint32_t) * 8 */
	__asm__ volatile("ctzw %0, %1;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, sizeof(uint32_t) * 8, "ctzw");
	a = 1;
#endif

	/* ctz: CPOP(3) = 2 */
	__asm__ volatile("cpop %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, 2, "cpop");

#if CONFIG_RISCV_ISA_RV64I
	b = UINT64_MAX;
	/* ctz: CPOP(UINT64_MAX & UINT32_MAX) = 32 */
	__asm__ volatile("cpopw %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, 32, "cpopw");
#endif

	b = ULONG_MAX;
	/* max: MAX_SIGNED(1, ULONG_MAX) = 1 */
	__asm__ volatile("max %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 1, "max");

	/* maxu: MAX_UNSIGNED(1, ULONG_MAX) = ULONG_MAX */
	__asm__ volatile("maxu %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, ULONG_MAX, "maxu");

	/* min: MIN_SIGNED(1, ULONG_MAX) = ULONG_MAX */
	__asm__ volatile("min %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, ULONG_MAX, "min");

	/* maxu: MIN_UNSIGNED(1, ULONG_MAX) = 1 */
	__asm__ volatile("minu %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 1, "minu");

	b = UINT8_MAX;
	/* sext.b: EXTEND_SIGNED_8(UINT8_MAX) = ULONG_MAX */
	__asm__ volatile("sext.b %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, ULONG_MAX, "sext.b");

	/* sext.h: EXDEND_SIGNED_16(UINT8_MAX) = UINT8_MAX */
	__asm__ volatile("sext.h %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, UINT8_MAX, "sext.h");

	b = UINT16_MAX;
	/* zext.b: EXTEND_ZERO_16(UINT16_MAX) = UINT16_MAX */
	__asm__ volatile("zext.h %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, UINT16_MAX, "zext.h");

	b = 1UL << ((sizeof(b) * 8) - 1);
	/* rol: ROTATE_LEFT(1 << (sizeof(unsigned long) - 1), 1) = 1 */
	__asm__ volatile("rol %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 1, "rol");

	b = 1;
	/* ror: ROTATE_RIGHT(1, 1) = 1 << (sizeof(unsigned long) - 1) */
	__asm__ volatile("ror %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 1UL << ((sizeof(b) * 8) - 1), "ror");

	/* rori: ROTATE_RIGHT(1, 2) = 1 << (sizeof(unsigned long) - 2) */
	__asm__ volatile("rori %0, %1, 2;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, 1UL << ((sizeof(b) * 8) - 2), "rori");

#if CONFIG_RISCV_ISA_RV64I
	b = 1UL << 31;
	/* rolw: EXTEND_SIGNED_64(ROTATE_LEFT_32(1 << 31, 1)) = 1 */
	__asm__ volatile("rolw %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 1, "rolw");

	/* roriw: EXTEND_SIGNED_64(ROTATE_RIGHT_32(1, 2)) = 1 << 30 */
	__asm__ volatile("roriw %0, %1, 2;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, 1UL << 30, "roliw");

	b = 1;
	/* rorw:
	 * EXTEND_SIGNED_64(ROTATE_RIGHT_32(1, 1)) =
	 * (1 << 31) | (UINT64_MAX & ~(uint64_t)UINT32_MAX)
	 */
	__asm__ volatile("rorw %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, (1UL << 31) | (UINT64_MAX & ~((uint64_t)UINT32_MAX)), "rorw");

	b = 1UL << 9;
	/* orc.b: OR_COMBINE_8(1 << 9) = 0xFF00 */
	__asm__ volatile("orc.b %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, 0xFF00, "orc.b");

	/* rev8: REVERSE_8(1 << 9) = 1 << ((sizeof(unsigned long) * 8) - 14) */
	__asm__ volatile("rev8 %0, %1;"
			: "=r"(c)
			: "r"(b));
	zexpect_equal(c, 1UL << ((sizeof(unsigned long) * 8) - 15), "rev8");
#endif
}
#endif

#if CONFIG_RISCV_ISA_EXT_ZBC
ZTEST(riscv_bit_manipulation, test_zbc)
{
	unsigned long a = ULONG_MAX;
	unsigned long b = ULONG_MAX;
	unsigned long c = 0;
	/* clmul:
	 * (CARRY_LESS_MULTIPLY(ULONG_MAX, ULONG_MAX) & ULONG_MAX) =
	 * 0x5555_5555_5555_5555 & ULONG_MAX
	 */
	__asm__ volatile("clmul %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 0x5555555555555555 & ULONG_MAX, "clmul");

	/* clmulh:
	 * (CARRY_LESS_MULTIPLY(ULONG_MAX, ULONG_MAX) >>
	 *  sizeof(unsigned long long)) & UINT32_MAX = 0x5555_5555_5555_5555 & ULONG_MAX
	 */
	__asm__ volatile("clmulh %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 0x5555555555555555 & ULONG_MAX, "clmulh");

	/* clmulr:
	 * (CARRY_LESS_MULTIPLY(ULONG_MAX, ULONG_MAX) >>
	 *  sizeof(unsigned long long)) & UINT32_MAX = 0xAAAA_AAAA_AAAA_AAAA & ULONG_MAX
	 */
	__asm__ volatile("clmulr %0, %1, %2;"
			: "=r"(c)
			: "r"(b), "r"(a));
	zexpect_equal(c, 0xAAAAAAAAAAAAAAAA & ULONG_MAX, "clmulr");
}
#endif

#if CONFIG_RISCV_ISA_EXT_ZBS
ZTEST(riscv_bit_manipulation, test_zbs)
{
	unsigned long a = 8;
	unsigned long b = 3;
	unsigned long c = 0;

	/* bclr: 8 & ~(1 << 3) = 0 */
	__asm__ volatile("bclr %0, %1, %2;"
			: "=r"(c)
			: "r"(a), "r"(b));
	zexpect_equal(c, 0, "bclr");

	/* bclri: 8 & ~(1 << 3) = 0 */
	__asm__ volatile("bclri %0, %1, 3;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, 0, "bclri");

	/* bext: (8 & (1 << 3)) >> 3 = 1 */
	__asm__ volatile("bext %0, %1, %2;"
			: "=r"(c)
			: "r"(a), "r"(b));
	zexpect_equal(c, 1, "bext");

	/* bexti: (8 & (1 << 3)) >> 3 = 1 */
	__asm__ volatile("bexti %0, %1, 3;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, 1, "bexti");

	/* binv: 8 ^ (1 << 3) = 0 */
	__asm__ volatile("binv %0, %1, %2;"
			: "=r"(c)
			: "r"(a), "r"(b));
	zexpect_equal(c, 0, "binv");

	/* binvi: 8 ^ (1 << 3) = 0 */
	__asm__ volatile("binv %0, %1, 3;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, 0, "binvi");

	b = 0;
	/* bset: 8 | (1 << 0) = 9 */
	__asm__ volatile("bset %0, %1, %2;"
			: "=r"(c)
			: "r"(a), "r"(b));
	zexpect_equal(c, 9, "bset");

	/* bseti: 8 | (1 << 0) = 9 */
	__asm__ volatile("bset %0, %1, 0;"
			: "=r"(c)
			: "r"(a));
	zexpect_equal(c, 9, "bset");

}
#endif

ZTEST_SUITE(riscv_bit_manipulation, NULL, NULL, NULL, NULL, NULL);
