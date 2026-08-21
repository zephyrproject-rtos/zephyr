/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal architecture contract for hardware debugpoints.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DEBUGPOINT_H_
#define ZEPHYR_INCLUDE_ARCH_DEBUGPOINT_H_

#include <zephyr/debug/debugpoint_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/*
 * Validate a request without changing hardware state. install_local() and
 * uninstall_local() update the backend's logical state and the current CPU.
 * A failed install must leave no hardware or logical resource allocated.
 * Repeated uninstall calls must be safe; -ENOENT means the point is already
 * inactive.
 */
int arch_debugpoint_validate(const struct z_debugpoint_config *config);
int arch_debugpoint_install_local(const struct z_debugpoint_config *config,
				  z_debugpoint_handle_t handle);
int arch_debugpoint_uninstall_local(z_debugpoint_handle_t handle);

/*
 * Reconcile the current CPU's hardware with backend logical state. This may
 * run with interrupts locked from IPI or power-management context and must
 * not sleep.
 */
int arch_debugpoint_cpu_sync(void);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_DEBUGPOINT_H_ */
