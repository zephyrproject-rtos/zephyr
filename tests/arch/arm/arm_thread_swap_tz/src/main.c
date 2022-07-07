/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <psa/crypto.h>
#include <zephyr/kernel.h>

#ifndef EXC_RETURN_S
/* bit [6] stack used to push registers: 0=Non-secure 1=Secure */
#define EXC_RETURN_S               (0x00000040UL)
#endif

#define HASH_LEN 64

static struct k_work_delayable interrupting_work;
static volatile bool work_done;
static char dummy_string[0x1000];
static char dummy_digest_correct[HASH_LEN];
static const uint32_t delay_ms = 1;
static volatile const struct k_thread *main_thread;

static void do_hash(char *hash)
{
	size_t len;

	/* Calculate correct hash. */
	psa_status_t status = psa_hash_compute(PSA_ALG_SHA_512, dummy_string,
			sizeof(dummy_string), hash, HASH_LEN, &len);

	zassert_equal(PSA_SUCCESS, status, "psa_hash_compute_fail: %d\n", status);
	zassert_equal(HASH_LEN, len, "hash length not correct\n");
}

static void work_func(struct k_work *work)
{
#ifdef CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS
	/* Check that the main thread was executing in secure mode. */
	zassert_true(main_thread->arch.mode_exc_return & EXC_RETURN_S,
		"EXC_RETURN not secure: 0x%x\n", main_thread->arch.mode_exc_return);

#else
	/* Check that the main thread was executing in nonsecure mode. */
	zassert_false(main_thread->arch.mode_exc_return & EXC_RETURN_S,
		"EXC_RETURN not nonsecure: 0x%x\n", main_thread->arch.mode_exc_return);
#endif

	work_done = true;

	/* If FPU is available, clobber FPU context in this thread to check that
	 * the correct context is restored in the other thread.
	 */
#ifdef CONFIG_CPU_HAS_FPU
	uint32_t clobber_val[16] = {
		0xdeadbee0, 0xdeadbee1, 0xdeadbee2, 0xdeadbee3,
		0xdeadbee4, 0xdeadbee5, 0xdeadbee6, 0xdeadbee7,
		0xdeadbee8, 0xdeadbee9, 0xdeadbeea, 0xdeadbeeb,
		0xdeadbeec, 0xdeadbeed, 0xdeadbeee, 0xdeadbeef,
	};

	__asm__ volatile(
		"vldmia %0, {s0-s15}\n"
		"vldmia %0, {s16-s31}\n"
		:: "r" (clobber_val) :
	);
#endif /* CONFIG_CPU_HAS_FPU */

	/* Call a secure service here as well, to test the added complexity of
	 * calling secure services from two threads.
	 */
	psa_status_t status = psa_hash_compare(PSA_ALG_SHA_512, dummy_string,
			sizeof(dummy_string), dummy_digest_correct, HASH_LEN);

	zassert_equal(PSA_SUCCESS, status, "psa_hash_compare failed\n");

}

void test_thread_swap_tz(void)
{
	int err;
	char dummy_digest[HASH_LEN];
	k_timeout_t delay = K_MSEC(delay_ms);
	k_tid_t curr;
	psa_status_t status;

	curr = k_current_get();
	main_thread = (struct k_thread *)curr;

	status = psa_crypto_init();
	zassert_equal(PSA_SUCCESS, status, NULL);

	/* Calculate correct hash. */
	do_hash(dummy_digest_correct);

	/* Set up interrupting_work to fire while call_tfm() is executing.
	 * This tests that it is safe to switch threads while a secure service
	 * is running.
	 */
	k_work_init_delayable(&interrupting_work, work_func);

	err = k_work_reschedule(&interrupting_work, delay);
	zassert_equal(1, err, "k_work_reschedule failed: %d\n", err);

	/* If FPU is available, check that FPU context is preserved when calling
	 * a secure function.
	 */
#ifdef CONFIG_CPU_HAS_FPU
	uint32_t test_val0[16] = {
		0x1a2b3c40, 0x1a2b3c41, 0x1a2b3c42, 0x1a2b3c43,
		0x1a2b3c44, 0x1a2b3c45, 0x1a2b3c46, 0x1a2b3c47,
		0x1a2b3c48, 0x1a2b3c49, 0x1a2b3c4a, 0x1a2b3c4b,
		0x1a2b3c4c, 0x1a2b3c4d, 0x1a2b3c4e, 0x1a2b3c4f,
	};
	uint32_t test_val1[16] = {
		0x2b3c4d50, 0x2b3c4d51, 0x2b3c4d52, 0x2b3c4d53,
		0x2b3c4d54, 0x2b3c4d55, 0x2b3c4d56, 0x2b3c4d57,
		0x2b3c4d58, 0x2b3c4d59, 0x2b3c4d5a, 0x2b3c4d5b,
		0x2b3c4d5c, 0x2b3c4d5d, 0x2b3c4d5e, 0x2b3c4d5f,
	};
	uint32_t test_val_res0[16];
	uint32_t test_val_res1[16];

	__asm__ volatile(
		"vldmia %0, {s0-s15}\n"
		"vldmia %1, {s16-s31}\n"
		:: "r" (test_val0), "r" (test_val1) :
	);

#endif /* CONFIG_CPU_HAS_FPU */

	work_done = false;
	do_hash(dummy_digest);
	zassert_true(work_done, "Interrupting work never happened\n");

#ifdef CONFIG_CPU_HAS_FPU
	__asm__ volatile(
		"vstmia %0, {s0-s15}\n"
		"vstmia %1, {s16-s31}\n"
		:: "r" (test_val_res0), "r" (test_val_res1) :
	);

	zassert_mem_equal(dummy_digest, dummy_digest_correct, HASH_LEN, NULL);
	zassert_mem_equal(test_val0, test_val_res0, sizeof(test_val0), NULL);
	zassert_mem_equal(test_val1, test_val_res1, sizeof(test_val1), NULL);
#endif /* CONFIG_CPU_HAS_FPU */
}

void test_main(void)
{
	ztest_test_suite(test_thread_swap_tz,
			ztest_unit_test(test_thread_swap_tz)
			);
	ztest_run_test_suite(test_thread_swap_tz);
}
