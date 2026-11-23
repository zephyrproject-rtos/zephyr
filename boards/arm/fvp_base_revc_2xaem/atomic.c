/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

/*
 * ARM64 atomic operations with explicit YIELD hints, for FVP.
 *
 * This is a workaround for the FastModel's quantum-based execution model,
 * not for any cache-coherency or atomic-emulation defect.
 *
 * The FastModel CPU simulators are single-threaded: each CPU runs a fixed
 * block of instructions (a "quantum", default 10 000) before the simulator
 * switches to the next CPU. Per the Arm Fast Models Reference Manual
 * ("Timing accuracy of Fast Models"), the conditions that can cause the
 * processor to yield to another thread before the quantum expires are:
 * non-DMI memory transactions, signal changes, WFE/WFI instructions,
 * barrier instructions, YIELD instructions, and Generic Timer accesses.
 * Plain load-acquires (LDAR/LDAXR) and LSE atomics (LDADDAL, SWPAL,
 * CAS, ...) are not on that list. As a result, a CPU spinning on a
 * load-acquire waiting for a release from another CPU can consume its
 * entire quantum before the producing CPU has had a chance to run. From
 * the OS's point of view this looks like livelock: the spinning thread
 * yields, the scheduler finds no other runnable thread on that core
 * (the producer is pinned to a different core and hasn't run yet), and
 * reschedules the same thread, repeating until the quantum ends.
 *
 * Inserting an explicit YIELD before each atomic operation that may
 * observe a value gives the simulator an opportunity to switch CPUs,
 * letting other CPUs make progress and breaking the apparent livelock.
 * The hint is placed on atomic reads (atomic_get, atomic_ptr_get),
 * compare-and-swap (atomic_cas, atomic_ptr_cas), and read-modify-write
 * operations (atomic_add and friends) -- everywhere the CPU might be
 * waiting for a release from another CPU. Pure unconditional writes
 * (atomic_set, atomic_ptr_set, and the *_clear helpers built on them)
 * carry no hint: nothing busy-waits on the writer side, and a reader
 * on another CPU picks up the update on its own next quantum yield.
 *
 * Without these hints, kernel scheduler code (e.g. switch_handle in
 * kernel/sched.c) and lockfree algorithms (e.g. tests/lib/lockfree,
 * k_sem and k_msgq fast paths) run pathologically slowly on
 * fvp_base_revc_2xaem -- often slow enough to appear hung and exceed
 * test timeouts.
 *
 * Caveats:
 *   - Reducing the quantum at the model level (e.g. `-Q 50`) avoids the
 *     pathological slowdown without any code change, at the cost of
 *     simulation throughput on every other workload.
 *   - Enabling cache state modeling (`-C cache_state_modelled=1`) reduces
 *     but does not eliminate the issue, because the dominant effect is
 *     scheduling, not cache state.
 */

bool atomic_cas(atomic_t *target, atomic_val_t old_value, atomic_val_t new_value)
{
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

bool atomic_ptr_cas(atomic_ptr_t *target, void *old_value, void *new_value)
{
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	__asm__ __volatile__("yield" ::: "memory");
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
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

void *atomic_ptr_get(const atomic_ptr_t *target)
{
	__asm__ __volatile__("yield" ::: "memory");
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
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	__asm__ __volatile__("yield" ::: "memory");
	return __atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST);
}
