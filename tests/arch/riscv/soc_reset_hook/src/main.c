/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>

#define HOOK_RAN_MAGIC 0xc01df00dU

/* Not zeroed at boot, so it keeps the value written by the hook. */
uint32_t soc_reset_hook_ran __noinit;

/*
 * With CONFIG_RISCV_S_MODE_EXTERNAL_SBI the external SBI passes the hart id in
 * a0 and the DTB address in a1. A C function is free to use them as scratch
 * registers, so write the hook in assembly and clobber them the way compiled
 * code may. The hook runs before the stack is set up and must not use it.
 *
 * If the startup code does not preserve a0 across the hook, it takes the boot
 * hart for a secondary one, waits for a wake-up that never comes, and the test
 * never reaches main().
 */
__asm__(".global soc_reset_hook\n"
	".type soc_reset_hook, @function\n"
	"soc_reset_hook:\n"
	"	li a0, -1\n"
	"	li a1, -1\n"
	"	lla t0, soc_reset_hook_ran\n"
	"	li t1, " STRINGIFY(HOOK_RAN_MAGIC) "\n"
	"	sw t1, 0(t0)\n"
	"	ret\n"
	".size soc_reset_hook, . - soc_reset_hook\n");

ZTEST(soc_reset_hook, test_boot_after_clobbering_hook)
{
	zassert_equal(soc_reset_hook_ran, HOOK_RAN_MAGIC, "soc_reset_hook() did not run");
	zassert_equal(arch_proc_id(), CONFIG_RV_BOOT_HART, "not running on the boot hart");
}

ZTEST_SUITE(soc_reset_hook, NULL, NULL, NULL, NULL, NULL);
