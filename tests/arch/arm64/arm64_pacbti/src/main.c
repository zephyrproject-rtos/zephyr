/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#ifdef CONFIG_ARM_PAC_PER_THREAD
#include <zephyr/arch/arm64/pac.h>
#endif

static void assert_pac_support(void)
{
	if (!IS_ENABLED(CONFIG_ARM_PAC)) {
		ztest_test_skip();
	}

	/* Check hardware capability */
	uint64_t isar1 = read_id_aa64isar1_el1();
	uint64_t pauth_api = (isar1 >> 4) & 0xf;
	uint64_t pauth_apa = (isar1 >> 8) & 0xf;

	zassert_true((pauth_api != 0) || (pauth_apa != 0),
		     "PAC hardware support required when CONFIG_ARM_PAC=y");

	/* Check that PAC is actually enabled in SCTLR_EL1 */
	uint64_t sctlr = read_sctlr_el1();
	bool pac_enabled = (sctlr & SCTLR_EnIA_BIT) != 0;

	zassert_true(pac_enabled, "PAC must be enabled in SCTLR_EL1.EnIA when CONFIG_ARM_PAC=y");
}

ZTEST(arm64_pacbti, test_pac_detection)
{
	assert_pac_support();
}

static void assert_bti_support(void)
{
	if (!IS_ENABLED(CONFIG_ARM_BTI)) {
		ztest_test_skip();
	}

	/* Check hardware capability */
	uint64_t pfr1 = read_id_aa64pfr1_el1();
	uint64_t bti = (pfr1 >> 0) & 0xf;

	zassert_true(bti != 0, "BTI hardware support required when CONFIG_ARM_BTI=y");

	/* Check that BTI is actually enabled in SCTLR_EL1 */
	uint64_t sctlr = read_sctlr_el1();
	bool bti0_enabled = (sctlr & SCTLR_BT0_BIT) != 0;
	bool bti1_enabled = (sctlr & SCTLR_BT1_BIT) != 0;

	zassert_true(bti0_enabled,
		     "BTI must be enabled in SCTLR_EL1.BT0 for EL0 when CONFIG_ARM_BTI=y");
	zassert_true(bti1_enabled,
		     "BTI must be enabled in SCTLR_EL1.BT1 for EL1 when CONFIG_ARM_BTI=y");
}

ZTEST(arm64_pacbti, test_bti_detection)
{
	assert_bti_support();
}

#ifdef CONFIG_ARM_PAC_PER_THREAD

ZTEST(arm64_pacbti, test_pac_key_management)
{
	assert_pac_support();

	/* Test that PAC key management functions work */
	struct pac_keys test_keys;
	struct pac_keys current_keys;
	struct pac_keys restored_keys;

	/* Generate some test keys */
	z_arm64_pac_keys_generate(&test_keys);

	/* Keys should be non-zero (extremely unlikely to be all zeros) */
	zassert_not_equal(test_keys.apia.lo, 0, "APIA key low should not be zero");
	zassert_not_equal(test_keys.apia.hi, 0, "APIA key high should not be zero");

	/* Save current keys */
	z_arm64_pac_keys_save(&current_keys);

	/* Restore test keys */
	z_arm64_pac_keys_restore(&test_keys);

	/* Save again and verify they match */
	z_arm64_pac_keys_save(&restored_keys);
	zassert_equal(restored_keys.apia.lo, test_keys.apia.lo, "APIA key low mismatch");
	zassert_equal(restored_keys.apia.hi, test_keys.apia.hi, "APIA key high mismatch");

	/* Make sure this is different from the originalkeys */
	zassert_not_equal(restored_keys.apia.lo, current_keys.apia.lo,
			  "low keys are the same");
	zassert_not_equal(restored_keys.apia.hi, current_keys.apia.hi,
			  "high keys are the same");

	/* Restore original keys */
	z_arm64_pac_keys_restore(&current_keys);
}

/* Test data for thread context switching */
static int thread_results[2];
static struct k_thread thread1_data, thread2_data;
static K_THREAD_STACK_DEFINE(thread1_stack, 1024);
static K_THREAD_STACK_DEFINE(thread2_stack, 1024);
static struct k_sem sync_sem;

/* Test function that should be protected by PAC in each thread */
__attribute__((noinline))
static int pac_test_function(int base_value, int thread_id)
{
	int result = base_value + thread_id * 10;

	/* Add some calls to make sure keys are actually used */
	k_sleep(K_MSEC(1));
	k_sleep(K_MSEC(10));

	/* This function's return address is PAC-protected */
	return result;
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);
	struct pac_keys *thread_keys = p1;
	int thread_id = (uintptr_t)p2;

	thread_results[thread_id - 1] = pac_test_function(100 * thread_id, thread_id);

	/* Each thread should have unique PAC keys */
	z_arm64_pac_keys_save(thread_keys);

	k_sem_give(&sync_sem);

	/* This function's return address is PAC-protected too */
}

ZTEST(arm64_pacbti, test_pac_context_switching)
{
	assert_pac_support();

	k_sem_init(&sync_sem, 0, 2);

	/* Test that each thread gets unique PAC keys during creation */
	struct pac_keys main_keys;
	struct pac_keys thread1_keys;
	struct pac_keys thread2_keys;

	/* Save main thread's keys */
	z_arm64_pac_keys_save(&main_keys);

	/* Clear thread key storage */
	memset(&thread1_keys, 0, sizeof(thread1_keys));
	memset(&thread2_keys, 0, sizeof(thread2_keys));

	/* Create two threads that will have different PAC keys */
	k_thread_create(&thread1_data, thread1_stack,
			K_THREAD_STACK_SIZEOF(thread1_stack),
			thread_entry, &thread1_keys, (void *)1, NULL,
			K_PRIO_COOP(1), 0, K_NO_WAIT);

	k_thread_create(&thread2_data, thread2_stack,
			K_THREAD_STACK_SIZEOF(thread2_stack),
			thread_entry, &thread2_keys, (void *)2, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	/* Wait for both threads to complete */
	k_sem_take(&sync_sem, K_FOREVER);
	k_sem_take(&sync_sem, K_FOREVER);

	/* Verify both threads executed successfully with their own PAC keys */
	int expected1 = 1 * 100 + 1 * 10;
	int expected2 = 2 * 100 + 2 * 10;

	zassert_equal(thread_results[0], expected1, "Thread 1 PAC context switching failed");
	zassert_equal(thread_results[1], expected2, "Thread 2 PAC context switching failed");

	/* Clean up threads */
	k_thread_join(&thread1_data, K_FOREVER);
	k_thread_join(&thread2_data, K_FOREVER);

	/* Main thread keys should still be intact */
	struct pac_keys current_main_keys;

	z_arm64_pac_keys_save(&current_main_keys);
	zassert_equal(current_main_keys.apia.lo, main_keys.apia.lo,
		      "Main thread APIA key corrupted");
	zassert_equal(current_main_keys.apia.hi, main_keys.apia.hi,
		      "Main thread APIA key corrupted");

	/* Make sure each thread had proper keys */
	zassert_not_equal(thread1_keys.apia.lo, 0, "unexpected zero value");
	zassert_not_equal(thread1_keys.apia.hi, 0, "unexpected zero value");
	zassert_not_equal(thread2_keys.apia.lo, 0, "unexpected zero value");
	zassert_not_equal(thread2_keys.apia.hi, 0, "unexpected zero value");

	/* Make sure each thread had different keys */
	zassert_not_equal(thread1_keys.apia.lo, thread2_keys.apia.lo, "low keys are the same");
	zassert_not_equal(thread1_keys.apia.hi, thread2_keys.apia.hi, "high keys are the same");

	/* Make sure thread1's keys were different from the main thread's */
	zassert_not_equal(thread1_keys.apia.lo, main_keys.apia.lo, "low keys are the same");
	zassert_not_equal(thread1_keys.apia.hi, main_keys.apia.hi, "high keys are the same");

	/* Make sure thread2's keys were different from the main thread's */
	zassert_not_equal(thread2_keys.apia.lo, main_keys.apia.lo, "low keys are the same");
	zassert_not_equal(thread2_keys.apia.hi, main_keys.apia.hi, "high keys are the same");
}

#endif /* CONFIG_ARM_PAC */

/*
 * Failure Test Cases - These should trigger PAC/BTI violations
 */

/* Test case types for failure scenarios */
static ZTEST_BMEM enum {
	PAC_AUTH_FAILURE_TEST,
	BTI_VIOLATION_TEST
} fault_test_case;

/**
 * @brief Fatal error hook for PAC/BTI failure tests
 */
void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	switch (fault_test_case) {
	case PAC_AUTH_FAILURE_TEST:
		/* Expect CPU exception due to PAC authentication failure */
		if (reason == K_ERR_CPU_EXCEPTION) {
			ztest_test_pass();
		} else {
			zassert_true(false, "Expected PAC authentication failure, "
				     "got reason: %u", reason);
		}
		break;

	case BTI_VIOLATION_TEST:
		/* Expect CPU exception due to BTI violation */
		if (reason == K_ERR_CPU_EXCEPTION) {
			ztest_test_pass();
		} else {
			zassert_true(false, "Expected BTI violation, got reason: %u", reason);
		}
		break;

	default:
		/* should never get here */
		ztest_test_fail();
		break;
	}
}

/**
 * @brief Attack simulation target - this is where the corrupted return should jump
 * This simulates an attacker's payload that should never execute if PAC is working
 */
static ZTEST_BMEM bool demo_mode;	/* Flag to control payload behavior */
static void __attribute__((noinline)) simulated_attack_payload(void)
{
	if (demo_mode) {
		/* Demonstration mode - this is expected behavior without PAC */
		printk("SUCCESS: Attack payload executed (as expected without PAC protection)\n");
		ztest_test_pass();
	} else {
		/* PAC failure test mode - this should never execute if PAC is working */
		printk("CRITICAL SECURITY FAILURE: Attack payload executed!\n");
		printk("PAC authentication did not detect return address corruption!\n");
		ztest_test_fail();
	}
}

/**
 * @brief Simulate a ROP attack by corrupting the return address on the stack
 *
 * Locates the caller's return address on the stack and overwrites it with
 * an attacker-controlled address. When PAC is enabled, authentication will
 * fail on return, preventing the attack. Without PAC, execution would jump
 * to the attacker's payload.
 */
static ALWAYS_INLINE int stage_rop_attack(void)
{
	/* Use GCC builtin to get the return address */
	void *return_addr = __builtin_return_address(0);

	printk("Return address from builtin: %p\n", return_addr);

	/* Get the stack pointer to find where the return address is stored */
	volatile uint64_t *sp;

	__asm__ volatile ("mov %0, sp" : "=r" (sp));

	/* Search a small area around the stack pointer for the return address.
	 * The return address should be in the first few stack slots.
	 */
	bool return_addr_found = false;

	for (int i = 0; i < 8; i++) {
		uint64_t stack_value = sp[i];
		/*
		 * PAC occupies (55 - CONFIG_ARM64_VA_BITS) bits in the upper portion.
		 * The actual virtual address uses CONFIG_ARM64_VA_BITS bits.
		 * Create mask to extract just the virtual address bits.
		 */
		uint64_t pac_mask = GENMASK64(CONFIG_ARM64_VA_BITS - 1, 0);
		uint64_t stack_base = stack_value & pac_mask;
		uint64_t return_base = (uint64_t)return_addr;

		/* Check if this stack location contains our return address */
		if (stack_base == return_base) {
			printk("Found return address at sp[%d]: 0x%016llx\n", i, stack_value);

			/* Corrupt the return address to point to attack payload */
			sp[i] = (uint64_t)simulated_attack_payload;

			printk("Corrupted return address to: %p\n", simulated_attack_payload);
			return_addr_found = true;
			break;
		}
	}

	if (!return_addr_found) {
		uint64_t pac_mask = GENMASK64(CONFIG_ARM64_VA_BITS - 1, 0);

		printk("ERROR: Could not locate return address on stack\n");
		printk("CONFIG_ARM64_VA_BITS=%d, PAC mask=0x%016llx\n",
		       CONFIG_ARM64_VA_BITS, pac_mask);
		printk("Expected return address: %p\n", return_addr);
		printk("Stack contents:\n");
		for (int i = 0; i < 8; i++) {
			uint64_t addr = sp[i];
			uint64_t masked = addr & pac_mask;

			printk("  sp[%2d]: 0x%016llx (masked: 0x%016llx)\n",
			       i, addr, masked);
		}
		ztest_test_fail();
		return -1;
	}

	/* When this function returns, it should attempt to jump to simulated_attack_payload
	 * but PAC authentication should fail first, preventing the attack
	 */
	return 42;
}

/**
 * @brief Function that will trigger PAC failure by corrupting its return address
 * This function is NOT compiled with branch-protection=none, so it uses PAC
 */
static __attribute__((noinline)) int pac_failure_target(void)
{
	return stage_rop_attack();
}

/**
 * @brief Vulnerable function WITHOUT PAC protection (for demonstration)
 * This function is identical to pac_failure_target but compiled without PAC protection
 * to demonstrate that the attack would succeed without PAC.
 */
static __attribute__((noinline)) __attribute__((target("branch-protection=none")))
int unprotected_failure_target(void)
{
	return stage_rop_attack();
}

ZTEST_USER(arm64_pacbti, test_pac_authentication_failure)
{
	if (k_is_user_context()) {
		printk("Test is executing in user context\n");
	}

	printk("Testing PAC authentication failure detection...\n");

	/* Ensure we're not in demonstration mode for this test */
	demo_mode = false;
	fault_test_case = PAC_AUTH_FAILURE_TEST;
	ztest_set_fault_valid(true);

	/* This call should corrupt its own return address and fail on return */
	int result = pac_failure_target();

	/* Should not reach here if PAC is working */
	printk("ERROR: PAC failure was not detected! Result: %d\n", result);

	/* Should not reach here - the PAC failure should have been caught */
	ztest_test_fail();
}

ZTEST_USER(arm64_pacbti, test_pac_attack_demonstration)
{
	if (k_is_user_context()) {
		printk("Test is executing in user context\n");
	}

	printk("Demonstrating attack success without PAC protection...\n");

	/* Enable demonstration mode so payload execution is expected */
	demo_mode = true;

	/* This call will corrupt its own return address and succeed without PAC */
	int result = unprotected_failure_target();

	/* Should not reach here - the corrupted return should jump to payload */
	printk("ERROR: Attack on unprotected function failed unexpectedly! Result: %d\n", result);

	/* If we reach here, something went wrong */
	ztest_test_fail();
}

/**
 * @brief Function compiled without BTI to trigger BTI violation
 * This function is explicitly compiled without branch protection
 */
static int __attribute__((noinline)) __attribute__((target("branch-protection=none")))
bti_less_target(void)
{
	return 100;
}

static void __no_optimization indirect_call(int (*func_ptr)(void))
{
	int result = func_ptr();

	printk("Indirect call to function returned: %d\n", result);
}

ZTEST_USER(arm64_pacbti, test_bti_violation_detection)
{
	int ret;

	if (k_is_user_context()) {
		printk("Test is executing in user context\n");
	}

	printk("Testing direct call to BTI-less function\n");
	ret = bti_less_target();
	zassert_equal(ret, 100);

	printk("Testing BTI violation detection...\n");
	fault_test_case = BTI_VIOLATION_TEST;
	ztest_set_fault_valid(true);

	/* Trigger BTI violation */
	indirect_call(bti_less_target);

	/* Should not reach here - the BTI violation should have been caught */
	ztest_test_fail();
}

ZTEST_SUITE(arm64_pacbti, NULL, NULL, NULL, NULL, NULL);
