/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test for coredump_app_dump() -- the application hook that adds custom
 * memory regions to a coredump regardless of the selected dump mode.
 *
 * Test strategy:
 *   1. Define a struct (app_region) with known magic values.
 *   2. Override coredump_app_dump() to dump it via coredump_memory_dump().
 *   3. Trigger a crash (k_oops) and let the coredump run.
 *   4. Copy the stored IN_MEMORY dump and search it for the magic values.
 *   5. Assert the custom region is present -- proving coredump_app_dump()
 *      was called and coredump_memory_dump() emitted the data correctly.
 *
 * A second test verifies that when the hook is inactive, the custom
 * region is NOT present (baseline check).
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/debug/coredump.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Custom data structure to dump via coredump_app_dump()
 * -----------------------------------------------------------------------
 */

/* Magic values chosen to be unlikely to appear by accident in the dump */
#define APP_REGION_MAGIC1  0xDEADC0DEU
#define APP_REGION_MAGIC2  0xCAFEBABEU
#define APP_REGION_PAYLOAD 0xABU

struct app_region {
	uint32_t magic1;     /* APP_REGION_MAGIC1 */
	uint32_t magic2;     /* APP_REGION_MAGIC2 */
	uint32_t value;      /* application-specific data */
	uint8_t  payload[8]; /* all APP_REGION_PAYLOAD */
} app_region_data;

/* Control flag: when true, coredump_app_dump() emits the custom region */
static bool g_dump_app_region;

/* -----------------------------------------------------------------------
 * coredump_app_dump() override -- called by the coredump subsystem
 * -----------------------------------------------------------------------
 * The hook is guarded by g_dump_app_region so we can run both
 * "hook active" and "hook inactive" test cases from the same image.
 * -----------------------------------------------------------------------
 */
void coredump_app_dump(void)
{
	if (!g_dump_app_region) {
		return;
	}

	coredump_memory_dump(POINTER_TO_UINT(&app_region_data),
			     POINTER_TO_UINT(&app_region_data) +
			     sizeof(app_region_data));
}

/* Static copy buffer avoids heap use after fatal/coredump on SMP HW. */
#define DUMP_BUF_SIZE CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE
static uint8_t dump_buf[DUMP_BUF_SIZE];

/* -----------------------------------------------------------------------
 * Crash thread -- triggers the coredump via k_oops()
 * -----------------------------------------------------------------------
 */
static struct k_thread dump_thread;
static K_THREAD_STACK_DEFINE(dump_stack, 2048);

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	if (reason == K_ERR_KERNEL_OOPS) {
		return;
	}

	k_fatal_halt(reason);
}

static void crash_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	k_oops();
}

/* Helper: trigger crash, return stored dump size */
static int trigger_and_get_dump_size(void)
{
	k_tid_t tid = k_thread_create(&dump_thread, dump_stack,
				      K_THREAD_STACK_SIZEOF(dump_stack),
				      crash_entry, NULL, NULL, NULL,
				      0, 0, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
	k_yield();

	return coredump_query(COREDUMP_QUERY_GET_STORED_DUMP_SIZE, NULL);
}

/* Helper: search for a 4-byte pattern in the dump buffer */
static bool find_u32_in_dump(const uint8_t *dump, int dump_len, uint32_t val)
{
	uint32_t tmp;

	for (int i = 0; i <= dump_len - (int)sizeof(uint32_t); i++) {
		memcpy(&tmp, dump + i, sizeof(tmp));
		if (tmp == val) {
			return true;
		}
	}
	return false;
}

/* Helper: copy the stored dump into a caller-provided buffer */
static int copy_dump(uint8_t *buf, int buf_len)
{
	struct coredump_cmd_copy_arg arg = {
		.offset = 0,
		.buffer = buf,
		.length = buf_len,
	};

	return coredump_cmd(COREDUMP_CMD_COPY_STORED_DUMP, &arg);
}

/* -----------------------------------------------------------------------
 * Test setup / teardown
 * -----------------------------------------------------------------------
 */

static void *suite_setup(void)
{
	/* Initialise the custom struct with known values */
	app_region_data.magic1 = APP_REGION_MAGIC1;
	app_region_data.magic2 = APP_REGION_MAGIC2;
	app_region_data.value  = 0x12345678U;
	memset(app_region_data.payload, APP_REGION_PAYLOAD,
	       sizeof(app_region_data.payload));
	return NULL;
}

static void before_each(void *f)
{
	ARG_UNUSED(f);
	/* Erase any previous dump so each test starts clean */
	coredump_cmd(COREDUMP_CMD_ERASE_STORED_DUMP, NULL);
	g_dump_app_region = false;
}

/* -----------------------------------------------------------------------
 * Test 1: coredump_app_dump() adds the custom region to the dump
 *
 * With g_dump_app_region=true the hook emits app_region_data.
 * After the crash, copy the dump and assert both magic values are present.
 * -----------------------------------------------------------------------
 */
ZTEST(coredump_app_dump, test_custom_region_present)
{
	int dump_size;
	int ret;
	bool found_magic1;
	bool found_magic2;

	/* Activate the hook */
	g_dump_app_region = true;

	dump_size = trigger_and_get_dump_size();
	zassert_true(dump_size > 0, "Expected a non-empty dump, got %d",
		     dump_size);
	zassert_true(dump_size <= DUMP_BUF_SIZE,
		     "Dump size %d exceeds buffer %d", dump_size, DUMP_BUF_SIZE);

	TC_PRINT("Dump size with app region: %d bytes\n", dump_size);

	ret = copy_dump(dump_buf, dump_size);
	zassert_ok(ret, "copy_dump failed: %d", ret);

	found_magic1 = find_u32_in_dump(dump_buf, dump_size, APP_REGION_MAGIC1);
	found_magic2 = find_u32_in_dump(dump_buf, dump_size, APP_REGION_MAGIC2);

	zassert_true(found_magic1,
		     "APP_REGION_MAGIC1 (0x%08x) not found in dump -- "
		     "coredump_app_dump() did not emit the custom region",
		     APP_REGION_MAGIC1);
	zassert_true(found_magic2,
		     "APP_REGION_MAGIC2 (0x%08x) not found in dump",
		     APP_REGION_MAGIC2);

	TC_PRINT("PASS: both magic values found in dump\n");
}

/* -----------------------------------------------------------------------
 * Test 2: when the hook is inactive, the custom region is NOT in the dump
 *
 * With g_dump_app_region=false the override returns without dumping.
 * The magic values must NOT appear in the dump (baseline sanity check).
 * -----------------------------------------------------------------------
 */
ZTEST(coredump_app_dump, test_custom_region_absent_when_hook_inactive)
{
	int dump_size;
	int ret;
	bool found_magic1;
	bool found_magic2;

	/* Hook is inactive */
	g_dump_app_region = false;

	dump_size = trigger_and_get_dump_size();
	zassert_true(dump_size > 0, "Expected a non-empty dump, got %d",
		     dump_size);
	zassert_true(dump_size <= DUMP_BUF_SIZE,
		     "Dump size %d exceeds buffer %d", dump_size, DUMP_BUF_SIZE);

	TC_PRINT("Dump size without app region: %d bytes\n", dump_size);

	ret = copy_dump(dump_buf, dump_size);
	zassert_ok(ret, "copy_dump failed: %d", ret);

	found_magic1 = find_u32_in_dump(dump_buf, dump_size, APP_REGION_MAGIC1);
	found_magic2 = find_u32_in_dump(dump_buf, dump_size, APP_REGION_MAGIC2);

	zassert_false(found_magic1,
		      "APP_REGION_MAGIC1 unexpectedly found in dump "
		      "without hook -- magic value collision?");
	zassert_false(found_magic2,
		      "APP_REGION_MAGIC2 unexpectedly found in dump "
		      "without hook -- magic value collision?");

	TC_PRINT("PASS: magic values absent from dump (as expected)\n");
}

/* -----------------------------------------------------------------------
 * Test 3: dump size grows by exactly one app region + mem section header
 *         when the hook is active
 * -----------------------------------------------------------------------
 */
ZTEST(coredump_app_dump, test_dump_size_increases_by_app_region)
{
	int size_without;
	int size_with;
	int delta;
	int expected;

	/* Baseline: hook inactive */
	g_dump_app_region = false;
	coredump_cmd(COREDUMP_CMD_ERASE_STORED_DUMP, NULL);
	size_without = trigger_and_get_dump_size();
	zassert_true(size_without > 0, "Baseline dump empty");

	/* With hook active */
	g_dump_app_region = true;
	coredump_cmd(COREDUMP_CMD_ERASE_STORED_DUMP, NULL);
	size_with = trigger_and_get_dump_size();
	zassert_true(size_with > 0, "Hook dump empty");

	delta = size_with - size_without;
	expected = (int)(sizeof(app_region_data) + sizeof(struct coredump_mem_hdr_t));

	TC_PRINT("Size without hook: %d bytes\n", size_without);
	TC_PRINT("Size with hook   : %d bytes\n", size_with);
	TC_PRINT("Delta            : %d bytes (expected %d)\n",
		 delta, expected);

	zassert_equal(delta, expected,
		      "Expected dump to grow by %d bytes (struct %zu + "
		      "mem header %zu), got delta=%d",
		      expected, sizeof(app_region_data),
		      sizeof(struct coredump_mem_hdr_t), delta);

	TC_PRINT("PASS: dump grew by exactly %d bytes\n", expected);
}

ZTEST_SUITE(coredump_app_dump, NULL, suite_setup, before_each, NULL, NULL);
