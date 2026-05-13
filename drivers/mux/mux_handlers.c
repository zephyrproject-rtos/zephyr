/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Userspace syscall verification handlers for the MUX subsystem.
 *
 * Each handler copies the caller-supplied struct mux_control (and the
 * embedded cells[] array) into the kernel stack before invoking the
 * z_impl_ entry point. This avoids:
 *   - TOCTOU: userspace cannot mutate the struct after verification.
 *   - len overflow: a malicious len that would overflow len*sizeof(uint32_t)
 *     is rejected up-front against CONFIG_MUX_MAX_CELLS.
 *
 */

#include <zephyr/drivers/mux.h>
#include <zephyr/internal/syscall_handler.h>

/* Copy a userspace struct mux_control into k_ctrl and its cells[] array into
 * the caller-provided k_cells buffer; on success, k_ctrl.cells points at
 * k_cells. Returns 0 on success and a verification failure on fault or len
 * overflow.
 */
static inline int mux_control_copy_from_user(struct mux_control *k_ctrl,
					     uint32_t *k_cells,
					     const struct mux_control *u_ctrl)
{
	if (k_usermode_from_copy(k_ctrl, u_ctrl, sizeof(*k_ctrl)) != 0) {
		return -EFAULT;
	}

	if (!K_SYSCALL_VERIFY_MSG(k_ctrl->len <= CONFIG_MUX_MAX_CELLS,
				  "mux_control cells len %zu exceeds maximum %d",
				  k_ctrl->len, CONFIG_MUX_MAX_CELLS)) {
		return -EINVAL;
	}

	if (k_ctrl->len > 0 &&
	    k_usermode_from_copy(k_cells, k_ctrl->cells,
				 k_ctrl->len * sizeof(uint32_t)) != 0) {
		return -EFAULT;
	}

	k_ctrl->cells = k_cells;
	return 0;
}

static inline int z_vrfy_mux_control_set(const struct device *dev,
					 const struct mux_control *control,
					 uint32_t state)
{
	struct mux_control k_ctrl;
	uint32_t k_cells[CONFIG_MUX_MAX_CELLS];

	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, set));
	K_OOPS(mux_control_copy_from_user(&k_ctrl, k_cells, control));

	return z_impl_mux_control_set(dev, &k_ctrl, state);
}
#include <zephyr/syscalls/mux_control_set_mrsh.c>

static inline int z_vrfy_mux_state_apply(const struct device *dev,
					 const struct mux_state *mstate)
{
	struct mux_state k_mstate;
	struct mux_control k_ctrl;
	uint32_t k_cells[CONFIG_MUX_MAX_CELLS];

	/* set is mandatory in the driver API; use the slot as the type check. */
	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, set));
	K_OOPS(k_usermode_from_copy(&k_mstate, mstate, sizeof(k_mstate)));
	K_OOPS(mux_control_copy_from_user(&k_ctrl, k_cells, k_mstate.control));

	k_mstate.control = &k_ctrl;
	return z_impl_mux_state_apply(dev, &k_mstate);
}
#include <zephyr/syscalls/mux_state_apply_mrsh.c>

static inline int z_vrfy_mux_state_get(const struct device *dev,
					       const struct mux_control *control,
					       uint32_t *state)
{
	struct mux_control k_ctrl;
	uint32_t k_cells[CONFIG_MUX_MAX_CELLS];
	uint32_t k_state;
	int ret;

	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, set));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(state, sizeof(*state)));
	K_OOPS(mux_control_copy_from_user(&k_ctrl, k_cells, control));

	ret = z_impl_mux_state_get(dev, &k_ctrl, &k_state);
	if (ret == 0) {
		K_OOPS(k_usermode_to_copy(state, &k_state, sizeof(k_state)));
	}
	return ret;
}
#include <zephyr/syscalls/mux_state_get_mrsh.c>

static inline int z_vrfy_mux_lock(const struct device *dev,
					  const struct mux_control *control,
					  bool lock)
{
	struct mux_control k_ctrl;
	uint32_t k_cells[CONFIG_MUX_MAX_CELLS];

	K_OOPS(K_SYSCALL_DRIVER_MUX_CONTROL(dev, set));
	K_OOPS(mux_control_copy_from_user(&k_ctrl, k_cells, control));

	return z_impl_mux_lock(dev, &k_ctrl, lock);
}
#include <zephyr/syscalls/mux_lock_mrsh.c>
