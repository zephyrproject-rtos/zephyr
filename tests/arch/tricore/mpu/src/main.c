/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>

#define FLASH_BASE ((uintptr_t)CONFIG_FLASH_BASE_ADDRESS)
#define FLASH_TOP  (FLASH_BASE + (uintptr_t)CONFIG_FLASH_SIZE * 1024U)
#define SRAM_BASE  ((uintptr_t)CONFIG_SRAM_BASE_ADDRESS)
#define SRAM_TOP   (SRAM_BASE + (uintptr_t)CONFIG_SRAM_SIZE * 1024U)

static K_THREAD_STACK_DEFINE(worker_stack, 1536);
static struct k_thread worker_thread;
static uint32_t scratch[16];
static volatile uintptr_t access_addr;
static volatile bool access_write;
static volatile bool access_traps;
static volatile bool trapped;

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);

	trapped = true;
}

static void worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* The fault is expected in this thread, not in the test thread */
	if (access_traps) {
		ztest_set_fault_valid(true);
	}

	if (access_write) {
		*(volatile uint32_t *)access_addr = 0xdeadbeefU;
	} else {
		(void)*(volatile uint32_t *)access_addr;
	}
}

/* Access addr from a kernel thread; an access the MPU denies traps */
static void check_access(uintptr_t addr, bool write, bool expect_trap)
{
	k_tid_t tid;

	access_addr = addr;
	access_write = write;
	access_traps = expect_trap;
	trapped = false;

	tid = k_thread_create(&worker_thread, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
			      worker_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(tid, K_MSEC(500)), "worker for 0x%08lx did not end",
		   (unsigned long)addr);
	zassert_equal(trapped, expect_trap, "%s 0x%08lx: trap %d, expected %d",
		      write ? "write" : "read", (unsigned long)addr, trapped, expect_trap);
}

ZTEST(tricore_mpu, test_allowed_access)
{
	check_access(FLASH_BASE + 0x100U, false, false);
	check_access((uintptr_t)scratch, false, false);
	check_access((uintptr_t)scratch, true, false);
}

ZTEST(tricore_mpu, test_flash_write)
{
	check_access(FLASH_BASE + 0x100U, true, true);
}

ZTEST(tricore_mpu, test_outside_regions)
{
	if (SRAM_BASE >= 0x10000U) {
		check_access(SRAM_BASE - 0x10U, false, true);
		check_access(FLASH_BASE - 0x10U, false, true);
	}

	check_access(SRAM_TOP + 0x10U, false, true);

	if (FLASH_TOP + 0x10U < 0xF0000000U) {
		check_access(FLASH_TOP + 0x10U, false, true);
	}

	check_access(0x10000000U, false, true);
	check_access(0x60000000U, false, true);
}

ZTEST_SUITE(tricore_mpu, NULL, NULL, NULL, NULL, NULL);
