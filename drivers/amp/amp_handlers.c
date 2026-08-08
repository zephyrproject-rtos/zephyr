/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

// TODO: This needs to be tested

#include <zephyr/drivers/amp.h>
#include <zephyr/internal/syscall_handler.h>

static inline size_t z_vrfy_amp_get_core_option_size(const struct device *dev,
			       const struct amp_core_identification *core_identification)
{
	K_OOPS(K_SYSCALL_DRIVER_AMP(dev, amp_get_core_option_size));

	struct amp_core_identification core_identification_local;
	K_OOPS(k_usermode_from_copy(&core_identification_local, core_identification, sizeof(struct amp_core_identification)));

	return z_impl_amp_get_core_option_size(dev, &core_identification_local);
}
#include <zephyr/syscalls/amp_get_core_option_size_mrsh.c>

static inline int z_vrfy_amp_prepare_core(const struct device *dev,
			       const struct amp_core_identification *core_identification,
			       const void *core_options)
{
	K_OOPS(K_SYSCALL_DRIVER_AMP(dev, amp_get_core_option_size));

	struct amp_core_identification core_identification_local;
	K_OOPS(k_usermode_from_copy(&core_identification_local, core_identification, sizeof(struct amp_core_identification)));

	const size_t core_options_size = z_impl_amp_get_core_option_size(dev, &core_identification_local);

	uint8_t core_options_local[core_options_size];

	K_OOPS(k_usermode_from_copy(&core_options_local, core_options, core_options_size));

	return z_impl_amp_prepare_core(dev, &core_identification_local, core_options_local);
}
#include <zephyr/syscalls/amp_prepare_core_mrsh.c>

static inline int z_vrfy_amp_start_core(const struct device *dev,
			     const struct amp_core_identification *core_identification)
{
	K_OOPS(K_SYSCALL_DRIVER_AMP(dev, amp_get_core_option_size));

	struct amp_core_identification core_identification_local;
	K_OOPS(k_usermode_from_copy(&core_identification_local, core_identification, sizeof(struct amp_core_identification)));

	return z_impl_amp_start_core(dev, &core_identification_local);
}
#include <zephyr/syscalls/amp_start_core_mrsh.c>

static inline int z_vrfy_amp_stop_core(const struct device *dev,
			    const struct amp_core_identification *core_identification)
{
	K_OOPS(K_SYSCALL_DRIVER_AMP(dev, amp_get_core_option_size));

	struct amp_core_identification core_identification_local;
	K_OOPS(k_usermode_from_copy(&core_identification_local, core_identification, sizeof(struct amp_core_identification)));

	return z_impl_amp_stop_core(dev, &core_identification_local);
}
#include <zephyr/syscalls/amp_stop_core_mrsh.c>

static inline size_t z_vrfy_amp_get_dt_core_options(const struct device *dev,
			       const struct amp_core_identification *core_identification)
{
	K_OOPS(K_SYSCALL_DRIVER_AMP(dev, amp_get_core_option_size));

	struct amp_core_identification core_identification_local;
	K_OOPS(k_usermode_from_copy(&core_identification_local, core_identification, sizeof(struct amp_core_identification)));

	return z_impl_amp_get_dt_core_config(dev, &core_identification_local);
}
#include <zephyr/syscalls/amp_get_dt_core_options_mrsh.c>

static inline int z_vrfy_amp_get_virtual_address(const struct device *dev,
				      const struct amp_core_identification *core_identification,
				      struct amp_memory_mapping *mapping)
{
	K_OOPS(K_SYSCALL_DRIVER_AMP(dev, amp_get_core_option_size));

	struct amp_core_identification core_identification_local;
	K_OOPS(k_usermode_from_copy(&core_identification_local, core_identification, sizeof(struct amp_core_identification)));

	struct amp_memory_mapping mapping_local;
	K_OOPS(k_usermode_from_copy(&mapping_local, mapping, sizeof(struct amp_memory_mapping)));

	int ret;

	ret = z_impl_amp_get_virtual_address(dev, &core_identification_local, &mapping_local);

	K_OOPS(k_usermode_to_copy(mapping, &mapping_local, sizeof(struct amp_memory_mapping)));

	return ret;
}
#include <zephyr/syscalls/amp_get_virtual_address_mrsh.c>
