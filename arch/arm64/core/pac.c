/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/arch/arm64/pac.h>

/**
 * @brief Generate random PAC keys
 *
 * Uses a single sys_rand_get() call for efficiency.
 */
void z_arm64_pac_keys_generate(struct pac_keys *keys)
{
	/* Generate all keys with a single random call using the union */
	sys_rand_get(keys->raw_bytes, sizeof(keys->raw_bytes));
}

/**
 * @brief Save current PAC keys from hardware registers
 */
void z_arm64_pac_keys_save(struct pac_keys *keys)
{
	/* Save APIA key */
	keys->apia.lo = read_apiakeylo_el1();
	keys->apia.hi = read_apiakeyhi_el1();

	/* Save APIB key */
	keys->apib.lo = read_apibkeylo_el1();
	keys->apib.hi = read_apibkeyhi_el1();

	/* Save APDA key */
	keys->apda.lo = read_apdakeylo_el1();
	keys->apda.hi = read_apdakeyhi_el1();

	/* Save APDB key */
	keys->apdb.lo = read_apdbkeylo_el1();
	keys->apdb.hi = read_apdbkeyhi_el1();

	/* Save APGA key */
	keys->apga.lo = read_apgakeylo_el1();
	keys->apga.hi = read_apgakeyhi_el1();
}

/**
 * @brief Restore PAC keys to hardware registers
 *
 * This function is compiled without branch protection because it modifies the PAC key
 * registers. Return address authentication would fail on exit since the keys used to
 * sign the return address on entry would no longer match the modified keys on exit.
 */
__attribute__((target("branch-protection=none")))
void z_arm64_pac_keys_restore(const struct pac_keys *keys)
{
	/* Restore APIA key */
	write_apiakeylo_el1(keys->apia.lo);
	write_apiakeyhi_el1(keys->apia.hi);

	/* Restore APIB key */
	write_apibkeylo_el1(keys->apib.lo);
	write_apibkeyhi_el1(keys->apib.hi);

	/* Restore APDA key */
	write_apdakeylo_el1(keys->apda.lo);
	write_apdakeyhi_el1(keys->apda.hi);

	/* Restore APDB key */
	write_apdbkeylo_el1(keys->apdb.lo);
	write_apdbkeyhi_el1(keys->apdb.hi);

	/* Restore APGA key */
	write_apgakeylo_el1(keys->apga.lo);
	write_apgakeyhi_el1(keys->apga.hi);
}

/**
 * @brief Handle PAC key context switching between threads
 *
 * Called from z_arm64_context_switch() assembly routine during thread switches.
 * Saves the current thread's PAC keys and loads the next thread's PAC keys.
 * This function is compiled without branch protection because it modifies the PAC key
 * registers. Return address authentication would fail on exit since the keys used to
 * sign the return address on entry would no longer match the modified keys on exit.
 *
 * @param new_thread Pointer to the thread being switched to
 * @param old_thread Pointer to the thread being switched from
 */
__attribute__((target("branch-protection=none")))
void z_arm64_pac_thread_context_switch(struct k_thread *new_thread, struct k_thread *old_thread)
{
	/* Save current thread's PAC keys */
	z_arm64_pac_keys_save(&old_thread->arch.pac_keys);

	/* Load new thread's PAC keys */
	z_arm64_pac_keys_restore(&new_thread->arch.pac_keys);
}
