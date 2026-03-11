/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/mux.h>

static inline int z_vrfy_mux_configure(const struct device *dev,
				       struct mux_control *control,
				       uint32_t state)
{
	struct mux_control k_ctrl;
	/* len is validated after copying the struct; guard against an
	 * unreasonably large value by capping at compile time using a
	 * fixed-size buffer that matches the maximum sensible specifier size.
	 */
	uint32_t k_cells[CONFIG_MUX_CONTROL_MAX_CELLS];

	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, configure));
	K_OOPS(k_usermode_from_copy(&k_ctrl, control, sizeof(k_ctrl)));
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_ctrl.len <= CONFIG_MUX_CONTROL_MAX_CELLS,
				    "mux_control cells len %zu exceeds maximum %d",
				    k_ctrl.len, CONFIG_MUX_CONTROL_MAX_CELLS));
	if (k_ctrl.len > 0) {
		K_OOPS(k_usermode_from_copy(k_cells, k_ctrl.cells,
					    k_ctrl.len * sizeof(uint32_t)));
	}
	k_ctrl.cells = k_cells;

	return z_impl_mux_configure((const struct device *)dev, &k_ctrl, state);
}
#include <zephyr/syscalls/mux_configure_mrsh.c>

static inline int z_vrfy_mux_state_get(const struct device *dev,
					struct mux_control *control,
					uint32_t *state)
{
	struct mux_control k_ctrl;
	uint32_t k_cells[CONFIG_MUX_CONTROL_MAX_CELLS];
	uint32_t k_state;
	int ret;

	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, state_get));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(state, sizeof(*state)));
	K_OOPS(k_usermode_from_copy(&k_ctrl, control, sizeof(k_ctrl)));
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_ctrl.len <= CONFIG_MUX_CONTROL_MAX_CELLS,
				    "mux_control cells len %zu exceeds maximum %d",
				    k_ctrl.len, CONFIG_MUX_CONTROL_MAX_CELLS));
	if (k_ctrl.len > 0) {
		K_OOPS(k_usermode_from_copy(k_cells, k_ctrl.cells,
					    k_ctrl.len * sizeof(uint32_t)));
	}
	k_ctrl.cells = k_cells;

	ret = z_impl_mux_state_get((const struct device *)dev, &k_ctrl, &k_state);
	K_OOPS(k_usermode_to_copy(state, &k_state, sizeof(k_state)));

	return ret;
}
#include <zephyr/syscalls/mux_state_get_mrsh.c>

static inline int z_vrfy_mux_lock(const struct device *dev,
				  struct mux_control *control,
				  bool lock)
{
	struct mux_control k_ctrl;
	uint32_t k_cells[CONFIG_MUX_CONTROL_MAX_CELLS];

	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, lock));
	K_OOPS(k_usermode_from_copy(&k_ctrl, control, sizeof(k_ctrl)));
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_ctrl.len <= CONFIG_MUX_CONTROL_MAX_CELLS,
				    "mux_control cells len %zu exceeds maximum %d",
				    k_ctrl.len, CONFIG_MUX_CONTROL_MAX_CELLS));
	if (k_ctrl.len > 0) {
		K_OOPS(k_usermode_from_copy(k_cells, k_ctrl.cells,
					    k_ctrl.len * sizeof(uint32_t)));
	}
	k_ctrl.cells = k_cells;

	return z_impl_mux_lock((const struct device *)dev, &k_ctrl, lock);
}
#include <zephyr/syscalls/mux_lock_mrsh.c>
