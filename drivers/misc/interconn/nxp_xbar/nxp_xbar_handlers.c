/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/misc/interconn/nxp_xbar/nxp_xbar.h>

static inline int z_vrfy_nxp_xbar_set_connection(const struct device *dev,
						  uint32_t output,
						  uint32_t input)
{
	K_OOPS(K_SYSCALL_DRIVER_NXP_XBAR(dev, set_connection));
	return z_impl_nxp_xbar_set_connection(dev, output, input);
}
#include <zephyr/syscalls/nxp_xbar_set_connection_mrsh.c>

static inline int z_vrfy_nxp_xbar_set_output_config(const struct device *dev,
						     uint32_t output,
						     const struct nxp_xbar_control_config *config)
{
	struct nxp_xbar_control_config config_copy;

	K_OOPS(K_SYSCALL_DRIVER_NXP_XBAR(dev, set_output_config));
	K_OOPS(k_usermode_from_copy(&config_copy, config, sizeof(config_copy)));

	return z_impl_nxp_xbar_set_output_config(dev, output, &config_copy);
}
#include <zephyr/syscalls/nxp_xbar_set_output_config_mrsh.c>

static inline int z_vrfy_nxp_xbar_get_status_flag(const struct device *dev,
						   uint32_t output,
						   bool *flag)
{
	bool flag_copy;
	int ret;

	K_OOPS(K_SYSCALL_DRIVER_NXP_XBAR(dev, get_status_flag));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(flag, sizeof(*flag)));

	ret = z_impl_nxp_xbar_get_status_flag(dev, output, &flag_copy);
	if (ret == 0) {
		K_OOPS(k_usermode_to_copy(flag, &flag_copy, sizeof(*flag)));
	}

	return ret;
}
#include <zephyr/syscalls/nxp_xbar_get_status_flag_mrsh.c>

static inline int z_vrfy_nxp_xbar_clear_status_flag(const struct device *dev,
						     uint32_t output)
{
	K_OOPS(K_SYSCALL_DRIVER_NXP_XBAR(dev, clear_status_flag));
	return z_impl_nxp_xbar_clear_status_flag(dev, output);
}
#include <zephyr/syscalls/nxp_xbar_clear_status_flag_mrsh.c>

#if defined(CONFIG_NXP_XBAR_WRITE_PROTECT)
static inline int z_vrfy_nxp_xbar_lock_sel_reg(const struct device *dev,
						uint32_t output)
{
	K_OOPS(K_SYSCALL_DRIVER_NXP_XBAR(dev, lock_sel_reg));
	return z_impl_nxp_xbar_lock_sel_reg(dev, output);
}
#include <zephyr/syscalls/nxp_xbar_lock_sel_reg_mrsh.c>

static inline int z_vrfy_nxp_xbar_lock_ctrl_reg(const struct device *dev,
						 uint32_t output)
{
	K_OOPS(K_SYSCALL_DRIVER_NXP_XBAR(dev, lock_ctrl_reg));
	return z_impl_nxp_xbar_lock_ctrl_reg(dev, output);
}
#include <zephyr/syscalls/nxp_xbar_lock_ctrl_reg_mrsh.c>
#endif /* CONFIG_NXP_XBAR_WRITE_PROTECT */
