/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

/*
 * ARM64 atomic operations with explicit memory barriers for FVP.
 *
 * This implementation adds DMB (Data Memory Barrier) instructions before
 * atomic operations that read from memory. According to the ARM Architecture
 * Reference Manual, atomic instructions with acquire/release semantics
 * (LDAR, LDAXR, LDADDAL, etc.) should already provide the necessary memory
 * ordering and cache coherency guarantees. The LDAR instruction in particular
 * is expected to ensure that the loaded value reflects any prior stores from
 * other CPUs.
 *
 * However, on FVP (ARM Fixed Virtual Platform), these guarantees do not appear
 * to be properly implemented, leading to race conditions where one CPU may read
 * stale cached values even after another CPU has performed an atomic update.
 * This manifests as assertion failures in kernel/sched.c (switch_handle checks)
 * notably with the tests/kernel/smp_abort test, and performance issues in
 * lockfree data structures e.g. tests/lib/lockfree test hanging.
 *
 * The explicit DMB SY barriers work around this FVP issue by forcing cache
 * invalidation before reads, ensuring that CPUs observe the latest values
 * written by other CPUs in SMP configurations.
 *
 * Note: Setting FVP's cache_state_modelled parameter improves lockfree test
 * performance but does not fully resolve the switch_handle race condition,
 * suggesting the issue is in FVP's atomic instruction emulation rather than
 * just cache modeling granularity.
 */

bool atomic_cas(atomic_t *target, atomic_val_t old_value, atomic_val_t new_value)
{
	/* Add barrier before read for compare-and-swap */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

bool atomic_ptr_cas(atomic_ptr_t *target, void *old_value, void *new_value)
{
	/* Add barrier before read for compare-and-swap */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for fetch-and-add */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for fetch-and-sub */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_fetch_sub(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);
}

atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_sub(target, 1);
}

atomic_val_t atomic_get(const atomic_t *target)
{
	/* Add explicit barrier before read to ensure cache coherency */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

void *atomic_ptr_get(const atomic_ptr_t *target)
{
	/* Add explicit barrier before read to ensure cache coherency */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for atomic exchange */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

void *atomic_ptr_set(atomic_ptr_t *target, void *value)
{
	/* Add barrier before read for atomic exchange */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

void *atomic_ptr_clear(atomic_ptr_t *target)
{
	return atomic_ptr_set(target, NULL);
}

atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for fetch-and-or */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for fetch-and-xor */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for fetch-and-and */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	/* Add barrier before read for fetch-and-nand */
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST);
}
