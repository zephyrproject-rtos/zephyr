/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

/*
 * ARM64 atomic operations with selectively-placed DMB SY barriers, for FVP.
 *
 * This is a workaround for the FastModel's quantum-based execution model,
 * not for any cache-coherency or atomic-emulation defect.
 *
 * The FastModel CPU simulators are single-threaded: each CPU runs a fixed
 * block of instructions (a "quantum", default 10 000) before the simulator
 * switches to the next CPU. Some instructions end the quantum early --
 * notably WFI and DMB SY -- but plain load-acquires (LDAR/LDAXR) and LSE
 * atomics (LDADDAL, SWPAL, CAS, ...) do not. As a result, a CPU spinning
 * on a load-acquire waiting for a release from another CPU can consume
 * its entire quantum before the producing CPU has had a chance to run.
 * From the OS's point of view this looks like livelock: the spinning
 * thread yields, the scheduler finds no other runnable thread on that
 * core (the producer is pinned to a different core and hasn't run yet),
 * and reschedules the same thread, repeating until the quantum ends.
 *
 * Inserting an explicit DMB SY at strategic points forces the simulator
 * to end the quantum early, letting other CPUs make progress and breaking
 * the apparent livelock. The barrier is placed only where Zephyr code
 * may busy-wait:
 *
 *   - Before atomic reads (atomic_get / atomic_ptr_get): poll loops
 *     observe values via these, so each read yields the quantum so any
 *     update from another CPU has a chance to be applied first.
 *
 *   - After a *failed* compare-and-swap (atomic_cas / atomic_ptr_cas):
 *     the typical caller pattern is a CAS-retry loop, so a failure
 *     implies the caller will look again -- yielding the quantum first
 *     gives the contending CPU a chance to commit its update. Successful
 *     CAS does not yield: there is no retry, and the value is now ours.
 *
 *   - Plain unconditional writes (atomic_set, atomic_add, atomic_sub,
 *     atomic_or, atomic_xor, atomic_and, atomic_nand, atomic_ptr_set,
 *     and the *_clear / *_inc / *_dec helpers built on them) carry no
 *     barrier: nothing busy-waits on the writer side. The reader on the
 *     other CPU yields its own quantum on its next atomic_get and will
 *     pick up the update then.
 *
 * Without these barriers, kernel scheduler code (e.g. switch_handle in
 * kernel/sched.c) and lockfree algorithms (e.g. tests/lib/lockfree,
 * k_sem and k_msgq fast paths) hang or time out on fvp_base_revc_2xaem.
 *
 * Caveats:
 *   - The quantum-ending behavior of DMB is a FastModel implementation
 *     detail rather than an architectural guarantee. It is reliable on
 *     the FVP version targeted by this board, but is not guaranteed to
 *     hold across all future FVP releases.
 *   - Reducing the quantum at the model level (e.g. `-Q 50`) makes the
 *     hangs go away without any code change, at the cost of simulation
 *     throughput.
 *   - Enabling cache state modeling (`-C cache_state_modelled=1`) reduces
 *     but does not eliminate the issue, because the dominant effect is
 *     scheduling, not cache state.
 */

bool atomic_cas(atomic_t *target, atomic_val_t old_value, atomic_val_t new_value)
{
	bool ok = __atomic_compare_exchange_n(target, &old_value, new_value,
					      0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	if (!ok) {
		__asm__ __volatile__("dmb sy" ::: "memory");
	}
	return ok;
}

bool atomic_ptr_cas(atomic_ptr_t *target, void *old_value, void *new_value)
{
	bool ok = __atomic_compare_exchange_n(target, &old_value, new_value,
					      0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	if (!ok) {
		__asm__ __volatile__("dmb sy" ::: "memory");
	}
	return ok;
}

atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
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
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

void *atomic_ptr_get(const atomic_ptr_t *target)
{
	__asm__ __volatile__("dmb sy" ::: "memory");
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

void *atomic_ptr_set(atomic_ptr_t *target, void *value)
{
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
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST);
}
