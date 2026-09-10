/*
 * Copyright (c) 2026 Andy Lin <andylinpersonal@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if CONFIG_MPU
#include "sys_wrapper.h"
#endif

#if CONFIG_HAS_RPI_PICO
#include <pico/bootrom.h>
#include <pico/bootrom/sf_table.h>
#endif

#if CONFIG_SOC_SERIES_RP2040

#define TEST_U32 0xdeadbeefu
#define TEST_F32 3.14f
#define TEST_F64 3.14

typedef uint32_t (*rom_float2uint_fn)(float);
typedef uint64_t (*rom_double2uint64_fn)(double);

static void test_access_bootrom_rp2040(void)
{
	rom_popcount32_fn rom_popcount32 = rom_func_lookup(ROM_FUNC_POPCOUNT32);

	zassert_not_null(rom_popcount32, "_popcount32 should be available on RP2040");
	zassert_equal(rom_popcount32(TEST_U32), __builtin_popcount(TEST_U32),
		      "Failed to invoke _popcount32");

	rom_ctz32_fn rom_ctz32 = rom_func_lookup(ROM_FUNC_CTZ32);

	zassert_not_null(rom_ctz32, "_ctz32 should be available on RP2040");
	zassert_equal(rom_ctz32(TEST_U32), __builtin_ctz(TEST_U32), "Failed to invoke _ctz32");

	const void **soft_float_table = rom_data_lookup(rom_table_code('S', 'F'));

	zassert_not_null(soft_float_table, "soft_float_table should be available on RP2040");

	rom_float2uint_fn rom_float2uint =
		soft_float_table[SF_TABLE_FLOAT2UINT / sizeof(void *)];

	zassert_not_null(rom_float2uint, "_float2uint should be available on RP2040");
	zassert_equal(rom_float2uint(TEST_F32), 3, "Failed to invoke _float2uint");

	uint8_t rom_ver = rp2040_rom_version();

	TC_PRINT("ROM version: %u\n", rom_ver);

	if (rom_ver >= 2) {
		const void **soft_double_table =
			rom_data_lookup(rom_table_code('S', 'D'));

		zassert_not_null(soft_double_table,
				 "soft_double_table should be available on RP2040 v2 and later");
		rom_double2uint64_fn rom_double2uint64 =
			soft_double_table[SF_TABLE_FLOAT2UINT64 / sizeof(void *)];

		zassert_not_null(rom_double2uint64,
				 "_double2uint64 should be available on RP2040 v2 and later");
		zassert_equal(rom_double2uint64(TEST_F64), 3, "Failed to invoke _double2uint64");
	}

	const char *copyright = rom_data_lookup(ROM_DATA_COPYRIGHT);

	zassert_not_null(copyright);
	TC_PRINT("Copyright message: %s\n", copyright);
}
#endif

#if CONFIG_SOC_SERIES_RP2350
static void test_access_bootrom_rp2350(void)
{
	uint32_t sys_info_buffer[2];
	uint32_t sys_info_flags = SYS_INFO_CPU_INFO;
	int fetched_word_num;

	rom_get_sys_info_fn rom_get_sys_info = rom_func_lookup(ROM_FUNC_GET_SYS_INFO);

	zassert_not_null(rom_get_sys_info, "get_sys_info should be available on RP2350A");

#if CONFIG_MPU
	sys_wrapper_iPmmm(rom_get_sys_info, &fetched_word_num,
		sys_info_buffer, ARRAY_SIZE(sys_info_buffer), sys_info_flags);
#else
	fetched_word_num = rom_get_sys_info(
		sys_info_buffer, ARRAY_SIZE(sys_info_buffer), sys_info_flags);
#endif

	zassert_equal(fetched_word_num, 2, "Failed to get CPU_INFO");
	zassert_equal(sys_info_buffer[0], sys_info_flags, "Failed to get CPU_INFO");
#if CONFIG_ARM
	zassert_equal(sys_info_buffer[1], 0, "CPU_INFO should be ARM (0)");
#elif CONFIG_RISCV
	zassert_equal(sys_info_buffer[1], 1, "CPU_INFO should be RISC-V (1)");
#else
	zassert_unreachable("Unknown CPU_INFO value");
#endif
}
#endif

static void test_access_bootrom(void)
{
	TC_PRINT("In userspace: %u\n", k_is_user_context());

#if CONFIG_SOC_SERIES_RP2040
	test_access_bootrom_rp2040();
#endif
#if CONFIG_SOC_SERIES_RP2350
	test_access_bootrom_rp2350();
#endif
#if CONFIG_HAS_RPI_PICO
	const uint32_t *git_rev = rom_data_lookup(ROM_DATA_SOFTWARE_GIT_REVISION);

	zassert_not_null(git_rev);

	TC_PRINT("Git rev of ROM: 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x\n",
		 git_rev[0], git_rev[1], git_rev[2], git_rev[3], git_rev[4], git_rev[5], git_rev[6],
		 git_rev[7]);
#endif
}

ZTEST(bootrom_test, test_access_bootrom)
{
	test_access_bootrom();
}

#if CONFIG_USERSPACE
ZTEST_USER(bootrom_test, test_access_bootrom_user)
{
	test_access_bootrom();
}
#endif

ZTEST_SUITE(bootrom_test, NULL, NULL, NULL, NULL, NULL);
