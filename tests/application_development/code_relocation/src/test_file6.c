/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

/* src/dup/reloc_dup.c, relocated to SRAM2 */
extern uint32_t reloc_dup_app_data;
extern void reloc_dup_app_func(void);
/* dup_lib/reloc_dup.c, same file name, not relocated */
extern void reloc_dup_lib_func(void);
/* src/multi/reloc_multi.c, compiled into app and dup_lib, relocated to SRAM2 */
extern void reloc_multi_app_func(void);
extern void reloc_multi_lib_func(void);

extern uintptr_t __sram2_text_reloc_start;
extern uintptr_t __sram2_text_reloc_end;
extern uintptr_t __sram2_data_reloc_start;
extern uintptr_t __sram2_data_reloc_end;

/*
 * src/dup/reloc_dup.c is requested through a path containing "./". Only that
 * file must be relocated, not dup_lib/reloc_dup.c which has the same name.
 */
ZTEST(code_relocation, test_same_file_name_relocate)
{
	printk("Address of reloc_dup_app_func %p\n", &reloc_dup_app_func);
	printk("Address of reloc_dup_app_data %p\n", &reloc_dup_app_data);
	printk("Address of reloc_dup_lib_func %p\n\n", &reloc_dup_lib_func);

	zassert_between_inclusive(
		(uintptr_t)&reloc_dup_app_func, (uintptr_t)&__sram2_text_reloc_start,
		(uintptr_t)&__sram2_text_reloc_end, "reloc_dup_app_func not in sram2_text region");
	zassert_between_inclusive(
		(uintptr_t)&reloc_dup_app_data, (uintptr_t)&__sram2_data_reloc_start,
		(uintptr_t)&__sram2_data_reloc_end, "reloc_dup_app_data not in sram2_data region");
	zassert_between_inclusive((uintptr_t)&reloc_dup_lib_func, (uintptr_t)&__text_region_start,
				  (uintptr_t)&__text_region_end,
				  "reloc_dup_lib_func is not in flash region");

	reloc_dup_app_func();
	reloc_dup_lib_func();
}

/*
 * src/multi/reloc_multi.c is compiled into two targets with different
 * defines and requested through a path containing "..". The sections of
 * both objects must be relocated.
 */
ZTEST(code_relocation, test_multi_target_relocate)
{
	printk("Address of reloc_multi_app_func %p\n", &reloc_multi_app_func);
	printk("Address of reloc_multi_lib_func %p\n\n", &reloc_multi_lib_func);

	zassert_between_inclusive((uintptr_t)&reloc_multi_app_func,
				  (uintptr_t)&__sram2_text_reloc_start,
				  (uintptr_t)&__sram2_text_reloc_end,
				  "reloc_multi_app_func not in sram2_text region");
	zassert_between_inclusive((uintptr_t)&reloc_multi_lib_func,
				  (uintptr_t)&__sram2_text_reloc_start,
				  (uintptr_t)&__sram2_text_reloc_end,
				  "reloc_multi_lib_func not in sram2_text region");

	reloc_multi_app_func();
	reloc_multi_lib_func();
}
