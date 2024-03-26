/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Zephyr hardware breakpoint API
 *
 * Provides an API for setting and removing hardware breakpoints.
 */

#include <zephyr/kernel.h>

/**
 * Breakpoint types
 */
enum hw_bp_type_t {
	/**
	 * Instruction breakpoint
	 */
	HW_BP_TYPE_INSTRUCTION,
	/**
	 * Memory breakpoint
	 */
	HW_BP_TYPE_MEMORY,
	/**
	 * Combined memory and instruction breakpoint
	 */
	HW_BP_TYPE_COMBINED,
};

/**
 * Hardware breakpoint flags
 */
enum hw_bp_flags_t {
	/**
	 * No flags
	 */
	HW_BP_FLAGS_NONE,
	/**
	 * Break on load
	 */
	HW_BP_FLAGS_LOAD = BIT(0),
	/**
	 * Break on store
	 */
	HW_BP_FLAGS_STORE = BIT(1),
	/**
	 * Breakpoint length 1 byte
	 */
	HW_BP_FLAGS_LEN_1 = (0 << 2),
	/**
	 * Breakpoint length 2 bytes
	 */
	HW_BP_FLAGS_LEN_2 = (1 << 2),
	/**
	 * Breakpoint length 4 bytes
	 */
	HW_BP_FLAGS_LEN_4 = (2 << 2),
	/**
	 * Breakpoint length 8 bytes
	 */
	HW_BP_FLAGS_LEN_8 = (3 << 2),
};

/**
 * Hardware breakpoint callback
 *
 * @param handle  Breakpoint handle
 * @param esf     Exception stack frame
 * @param data    Callback data
 */
typedef void (*hw_bp_callback_t)(int handle, z_arch_esf_t *esf, void *data);

/**
 * Query hardware breakpoint support
 *
 * @param type    Breakpoint type
 * @return        Number of breakpoints supported
 */
int hw_bp_query(enum hw_bp_type_t type);

/**
 * Set hardware breakpoint
 *
 * @param addr    Breakpoint address
 * @param type    Breakpoint type
 * @param flags   Breakpoint flags
 * @param cb      Breakpoint callback
 * @param data    Breakpoint data
 * @return        Breakpoint handle
 *                -EAGAIN if all breakpoints are in use
 *                -EINVAL is the breakpoint settings are not supported
 */
int hw_bp_set(uintptr_t addr, enum hw_bp_type_t type, enum hw_bp_flags_t flags,
		hw_bp_callback_t cb, void *data);

/**
 * Remove hardware breakpoint
 *
 * @param bp      Breakpoint number
 * @return        0 on success
				  -EINVAL if the handle is not valid
 */
int hw_bp_remove(int hande);
