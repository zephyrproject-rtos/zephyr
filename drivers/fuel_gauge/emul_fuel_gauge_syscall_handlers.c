/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/emul_fuel_gauge.h>
#include <zephyr/sys/iterable_sections.h>

/* Emulator syscalls just need to exist as stubs as these are only called by tests. */

/*
 * The emulator instance is chosen by the caller. Both implementations read
 * target->backend_api and make an indirect call through it, so an unchecked
 * pointer from user mode is an arbitrary call in supervisor context. struct emul
 * is not a kernel object and cannot be validated with K_SYSCALL_OBJ(), so bound
 * the pointer against the set of emulators the image actually defines.
 */
static inline bool emul_fuel_gauge_target_is_valid(const struct emul *target)
{
	STRUCT_SECTION_FOREACH(emul, e) {
		if (e == target) {
			return true;
		}
	}

	return false;
}

static inline int z_vrfy_emul_fuel_gauge_is_battery_cutoff(const struct emul *target, bool *cutoff)
{
	K_OOPS(K_SYSCALL_VERIFY_MSG(emul_fuel_gauge_target_is_valid(target),
				    "invalid emul instance"));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(cutoff, sizeof(bool)));

	return z_impl_emul_fuel_gauge_is_battery_cutoff(target, cutoff);
}

#include <zephyr/syscalls/emul_fuel_gauge_is_battery_cutoff_mrsh.c>

static inline int z_vrfy_emul_fuel_gauge_set_battery_charging(const struct emul *target,
							      uint32_t uV, int uA)
{
	K_OOPS(K_SYSCALL_VERIFY_MSG(emul_fuel_gauge_target_is_valid(target),
				    "invalid emul instance"));

	return z_impl_emul_fuel_gauge_set_battery_charging(target, uV, uA);
}

#include <zephyr/syscalls/emul_fuel_gauge_set_battery_charging_mrsh.c>
