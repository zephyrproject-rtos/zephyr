/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/psci.h>
#include <syscall_handler.h>
#include <string.h>

static inline uint32_t z_vrfy_psci_get_version(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PSCI(dev, get_version));

	return z_impl_psci_get_version(dev);
}
#include <syscalls/psci_get_version_mrsh.c>

static inline int z_vrfy_psci_cpu_off(const struct device *dev, uint32_t state)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PSCI(dev, cpu_off));

	return z_impl_psci_cpu_off(dev, state);
}
#include <syscalls/psci_cpu_off_mrsh.c>

static inline int z_vrfy_psci_cpu_on(const struct device *dev,
				     unsigned long cpuid,
				     unsigned long entry_point) {
	Z_OOPS(Z_SYSCALL_DRIVER_PSCI(dev, cpu_on));

	return z_impl_psci_cpu_on(dev, cpuid, entry_point);
}
#include <syscalls/psci_cpu_on_mrsh.c>

static inline int z_vrfy_psci_affinity_info(const struct device *dev,
					    unsigned long target_affinity,
					    unsigned long lowest_affinity_level)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PSCI(dev, affinity_info));

	return z_impl_psci_affinity_info(dev, target_affinity,
					 lowest_affinity_level);
}
#include <syscalls/psci_affinity_info_mrsh.c>
