/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/emul_fuel_gauge.h>

/* Emulator syscalls just need to exist as stubs as these are only called by tests. */

static inline int z_vrfy_emul_fuel_gauge_is_battery_cutoff(const struct emul *target, bool *cutoff)
{
	return z_impl_emul_fuel_gauge_is_battery_cutoff(target, cutoff);
}

#include <syscalls/emul_fuel_gauge_is_battery_cutoff_mrsh.c>

static inline int z_vrfy_emul_fuel_gauge_set_battery_charging(const struct emul *target,
							      uint32_t uV, int uA)
{
	return z_impl_emul_fuel_gauge_set_battery_charging(target, uV, uA);
}

#include <syscalls/emul_fuel_gauge_set_battery_charging_mrsh.c>
