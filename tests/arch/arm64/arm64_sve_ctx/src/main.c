/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <stdint.h>

/* Helper function for SVE vector length */
static inline uint32_t sve_get_vl(void)
{
	uint32_t vl;

	__asm__("rdvl %0, #1" : "=r"(vl));
	return vl;
}

ZTEST(arm64_sve_ctx, test_sve_basic_instructions)
{
	/* Check if SVE is actually available */
	uint64_t pfr0 = read_id_aa64pfr0_el1();
	bool sve = is_sve_implemented();

	TC_PRINT("=== SVE Feature Check ===\n");
	TC_PRINT("ID_AA64PFR0_EL1: 0x%016llx\n", pfr0);
	TC_PRINT("SVE support: %s\n", sve ? "YES" : "NO");
	zassert_true(sve, "SVE support required for this test");

	/* Simple test: just try to read SVE vector length */
	TC_PRINT("About to test SVE access...\n");
	uint32_t vl = sve_get_vl();

	TC_PRINT("SVE vector length: %u bytes\n", vl);
	zassert_not_equal(vl, 0, "SVE vector length should not be zero");

	/* Verify vector length is within expected bounds */
	zassert_true(vl >= 16, "SVE vector length must be at least 16 bytes");
	zassert_true(vl <= CONFIG_ARM64_SVE_VL_MAX,
		     "SVE vector length %u exceeds maximum %u", vl, CONFIG_ARM64_SVE_VL_MAX);
	if (vl < CONFIG_ARM64_SVE_VL_MAX) {
		TC_PRINT("Warning: CONFIG_ARM64_SVE_VL_MAX=%u while the hardware "
			 "vector length is %u.\n", CONFIG_ARM64_SVE_VL_MAX, vl);
		TC_PRINT("Warning: This will waste memory in struct k_thread.\n");
	}
}

#define STACK_SIZE 4096
#define THREAD_PRIORITY 1

K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

static struct k_thread thread1_data;
static struct k_thread thread2_data;

/* Synchronization */
static struct k_sem sync_sem;
static struct k_sem done_sem;

/* User space memory partition for test results */
K_APPMEM_PARTITION_DEFINE(sve_test_partition);
K_APP_DMEM(sve_test_partition) static volatile bool thread1_sve_ok;
K_APP_DMEM(sve_test_partition) static volatile bool thread2_sve_ok;

/* Set unique patterns in SVE Z registers for thread identification */
static inline void sve_set_thread_pattern(uint32_t thread_id)
{
	/* Create unique 32-bit pattern based on thread ID */
	uint32_t pattern = 0x12340000 | (thread_id & 0xFFF);

	/* Use SVE DUP instruction to fill Z registers with pattern */
	__asm__ volatile (
		"mov w0, %w0\n"
		"sve_pattern_loop_%=:\n"
		"dup z0.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z1.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z2.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z3.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z4.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z5.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z6.s, w0\n"
		"add w0, w0, #0x1000\n"
		"dup z7.s, w0\n"
		:
		: "r" (pattern)
		: "w0", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "memory"
	);
}

/* Set patterns in SVE P (predicate) registers */
static inline void sve_set_predicate_pattern(uint32_t thread_id)
{
	/* Set alternating patterns in predicate registers */
	if (thread_id & 1) {
		__asm__ volatile (
			"ptrue p0.b\n"
			"pfalse p1.b\n"
			"ptrue p2.s\n"
			"pfalse p3.b\n"
			::: "p0", "p1", "p2", "p3", "memory"
		);
	} else {
		__asm__ volatile (
			"pfalse p0.b\n"
			"ptrue p1.h\n"
			"pfalse p2.b\n"
			"ptrue p3.d\n"
			::: "p0", "p1", "p2", "p3", "memory"
		);
	}
}

/* Verify SVE Z register patterns */
static inline bool sve_verify_z_pattern(uint32_t thread_id)
{
	/* Use stack-allocated buffer for user threads */
	uint32_t actual_buffer[8 * 256/4] __aligned(8);
	uint32_t *actual_p = actual_buffer;
	uint32_t vl = sve_get_vl();
	uint32_t expected_base = 0x12340000 | (thread_id & 0xFFF);
	bool result = true;

	/* Store elements from Z registers to memory, then read back */
	__asm__ volatile (
		"str z0, [%0, #0, MUL VL]\n"
		"str z1, [%0, #1, MUL VL]\n"
		"str z2, [%0, #2, MUL VL]\n"
		"str z3, [%0, #3, MUL VL]\n"
		"str z4, [%0, #4, MUL VL]\n"
		"str z5, [%0, #5, MUL VL]\n"
		"str z6, [%0, #6, MUL VL]\n"
		"str z7, [%0, #7, MUL VL]\n"
		:
		: "r" (actual_buffer)
		: "memory"
	);

	/* Verify each register has expected sequential pattern */
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < vl/4; j++) {
			uint32_t expected = expected_base + (i * 0x1000);
			uint32_t actual = *actual_p++;

			if (actual != expected) {
				TC_PRINT("Thread %u: Z%d mismatch - "
					 "expected 0x%x, got 0x%x\n",
					 thread_id, i, expected, actual);
				result = false;
			}
		}
	}

	return result;
}

/* Verify SVE P register patterns */
static inline bool sve_verify_p_pattern(uint32_t thread_id)
{
	uint8_t p_buffer[4 * 256/8];
	uint8_t *p_p = p_buffer;
	uint32_t vl = sve_get_vl();
	bool result = true;

	/* Store predicate registers to memory and read them back */
	__asm__ volatile (
		"str p0, [%0, #0, MUL VL]\n"
		"str p1, [%0, #1, MUL VL]\n"
		"str p2, [%0, #2, MUL VL]\n"
		"str p3, [%0, #3, MUL VL]\n"
		:
		: "r" (p_buffer)
		: "memory"
	);

	/* Check expected patterns based on thread ID */
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < vl/8; j++) {
			/* Thread 1: p0=true, p1=false, p2=true, p3=false */
			/* Thread 2: p0=false, p1=true, p2=false, p3=true */
			/* p0 = b, p1 = h, p2 = s, p3 = d */
			static const uint8_t patterns[4] = { 0xff, 0x55, 0x11, 0x01 };
			uint8_t expected = ((thread_id ^ i) & 1) ? patterns[i] : 0;
			uint8_t actual = *p_p++;

			if (actual != expected) {
				TC_PRINT("Thread %u: P%d mismatch - "
					 "expected 0x%x, got 0x%x\n",
					 thread_id, i, expected, actual);
				result = false;
			}
		}
	}

	return result;
}

/*
 * Test thread functions
 */
static void sve_test_thread1(void *arg1, void *arg2, void *arg3)
{
	const uint32_t thread_id = 1;

	TC_PRINT("Thread 1: Starting SVE context test\n");

	/* Set initial SVE patterns */
	sve_set_thread_pattern(thread_id);
	sve_set_predicate_pattern(thread_id);

	/* Immediate validation after setting patterns - NO function calls in between */
	zassert_true(sve_verify_z_pattern(thread_id),
		     "Thread 1: Initial Z pattern validation failed");
	zassert_true(sve_verify_p_pattern(thread_id),
		     "Thread 1: Initial P pattern validation failed");

	TC_PRINT("Thread 1: Set initial SVE patterns\n");

	/* Signal that we're ready and wait for other thread */
	k_sem_give(&sync_sem);
	k_msleep(1);
	k_sem_take(&sync_sem, K_FOREVER);

	/* Verify our patterns are still intact */
	bool z_ok = sve_verify_z_pattern(thread_id);
	bool p_ok = sve_verify_p_pattern(thread_id);

	thread1_sve_ok = z_ok && p_ok;

	TC_PRINT("Thread 1: SVE verification %s (Z:%s P:%s)\n",
		 thread1_sve_ok ? "PASSED" : "FAILED",
		 z_ok ? "OK" : "FAIL", p_ok ? "OK" : "FAIL");

	k_sem_give(&sync_sem);
}

static void sve_test_thread2(void *arg1, void *arg2, void *arg3)
{
	const uint32_t thread_id = 2;

	TC_PRINT("Thread 2: Starting SVE context test\n");

	/* Wait for thread 1 to be ready */
	k_sem_take(&sync_sem, K_FOREVER);

	/* Set our own SVE patterns */
	sve_set_thread_pattern(thread_id);
	sve_set_predicate_pattern(thread_id);

	/* Immediate validation after setting patterns - NO function calls in between */
	zassert_true(sve_verify_z_pattern(thread_id),
		     "Thread 2: Initial Z pattern validation failed");
	zassert_true(sve_verify_p_pattern(thread_id),
		     "Thread 2: Initial P pattern validation failed");

	TC_PRINT("Thread 2: Set initial SVE patterns\n");

	/* Signal thread 1 to continue */
	k_sem_give(&sync_sem);
	k_msleep(1);
	k_sem_take(&sync_sem, K_FOREVER);

	/* Verify our patterns are still intact */
	bool z_ok = sve_verify_z_pattern(thread_id);
	bool p_ok = sve_verify_p_pattern(thread_id);

	thread2_sve_ok = z_ok && p_ok;

	TC_PRINT("Thread 2: SVE verification %s (Z:%s P:%s)\n",
		 thread2_sve_ok ? "PASSED" : "FAILED",
		 z_ok ? "OK" : "FAIL", p_ok ? "OK" : "FAIL");

	k_sem_give(&done_sem);
}

/*
 * Test suite setup and tests
 */
static struct k_mem_domain sve_test_domain;

static void *sve_ctx_setup(void)
{
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&done_sem, 0, 1);

	/* Initialize memory domain for user threads */
	struct k_mem_partition *parts[] = { &sve_test_partition };

	k_mem_domain_init(&sve_test_domain, 1, parts);

	return NULL;
}

static void sve_ctx_before(void *fixture)
{
	/* Reset test results and semaphores before each test */
	thread1_sve_ok = false;
	thread2_sve_ok = false;
	k_sem_reset(&sync_sem);
	k_sem_reset(&done_sem);
}

ZTEST(arm64_sve_ctx, test_sve_context_switching_privileged)
{
	TC_PRINT("=== Testing SVE Context Switching: Privileged vs Privileged ===\n");

	/* Create privileged kernel threads that will use SVE */
	k_thread_create(&thread1_data, thread1_stack, STACK_SIZE,
			sve_test_thread1, NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread1_data, "sve_priv_thread1");

	k_thread_create(&thread2_data, thread2_stack, STACK_SIZE,
			sve_test_thread2, NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread2_data, "sve_priv_thread2");

	/* Wait for both threads to complete */
	k_sem_take(&done_sem, K_FOREVER);

	/* Clean up */
	k_thread_join(&thread1_data, K_FOREVER);
	k_thread_join(&thread2_data, K_FOREVER);

	/* Verify both threads maintained their SVE context */
	zassert_true(thread1_sve_ok, "Privileged Thread 1 SVE context was corrupted");
	zassert_true(thread2_sve_ok, "Privileged Thread 2 SVE context was corrupted");
}

ZTEST(arm64_sve_ctx, test_sve_context_switching_user)
{
	TC_PRINT("=== Testing SVE Context Switching: User vs User ===\n");

	/* Create user threads that will use SVE */
	k_thread_create(&thread1_data, thread1_stack, STACK_SIZE,
			sve_test_thread1, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);
	k_thread_name_set(&thread1_data, "sve_user_thread1");

	/* Grant permission to semaphores for user thread 1 */
	k_object_access_grant(&sync_sem, &thread1_data);
	k_object_access_grant(&done_sem, &thread1_data);

	/* Add thread 1 to memory domain for accessing result variables */
	k_mem_domain_add_thread(&sve_test_domain, &thread1_data);

	k_thread_create(&thread2_data, thread2_stack, STACK_SIZE,
			sve_test_thread2, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);
	k_thread_name_set(&thread2_data, "sve_user_thread2");

	/* Grant permission to semaphores for user thread 2 */
	k_object_access_grant(&sync_sem, &thread2_data);
	k_object_access_grant(&done_sem, &thread2_data);

	/* Add thread 2 to memory domain for accessing result variables */
	k_mem_domain_add_thread(&sve_test_domain, &thread2_data);

	/* Wait for both threads to complete */
	k_sem_take(&done_sem, K_FOREVER);

	/* Clean up */
	k_thread_join(&thread1_data, K_FOREVER);
	k_thread_join(&thread2_data, K_FOREVER);

	/* Verify both threads maintained their SVE context */
	zassert_true(thread1_sve_ok, "User Thread 1 SVE context was corrupted");
	zassert_true(thread2_sve_ok, "User Thread 2 SVE context was corrupted");
}

ZTEST(arm64_sve_ctx, test_sve_context_switching_mixed)
{
	TC_PRINT("=== Testing SVE Context Switching: User vs Privileged ===\n");

	/* Create mixed privilege threads: thread 1 = user, thread 2 = privileged */
	k_thread_create(&thread1_data, thread1_stack, STACK_SIZE,
			sve_test_thread1, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);
	k_thread_name_set(&thread1_data, "sve_user_thread1");

	/* Grant permission to semaphores for user thread 1 */
	k_object_access_grant(&sync_sem, &thread1_data);
	k_object_access_grant(&done_sem, &thread1_data);

	/* Add thread 1 to memory domain for accessing result variables */
	k_mem_domain_add_thread(&sve_test_domain, &thread1_data);

	k_thread_create(&thread2_data, thread2_stack, STACK_SIZE,
			sve_test_thread2, NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread2_data, "sve_priv_thread2");

	/* Privileged thread doesn't need special permissions */

	/* Wait for both threads to complete */
	k_sem_take(&done_sem, K_FOREVER);

	/* Clean up */
	k_thread_join(&thread1_data, K_FOREVER);
	k_thread_join(&thread2_data, K_FOREVER);

	/* Verify both threads maintained their SVE context */
	zassert_true(thread1_sve_ok, "User Thread 1 SVE context was corrupted");
	zassert_true(thread2_sve_ok, "Privileged Thread 2 SVE context was corrupted");
}

ZTEST_SUITE(arm64_sve_ctx, NULL, sve_ctx_setup, sve_ctx_before, NULL, NULL);
