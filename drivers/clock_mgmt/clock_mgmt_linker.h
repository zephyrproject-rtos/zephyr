/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Note: this file is included by the linker snippet.
 * Avoid including other headers from this one, as it will likely
 * cause linker errors
 */

/**
 * @brief Define clock management callback linker section
 *
 * This macro defines the linker section for a given clock management callback.
 * The section definitions allow us to only link in data structures for clocks
 * that are actually referenced by a compiled driver, rather than for all
 * clocks in the devicetree.
 * @param clock_id clock ID, taken from clock node's clock-id property
 */

#define CLOCK_CALLBACK_LD_SECTION(clock_id) \
	clock_callback_##clock_id##_start = .; \
	*(.clock_callback_##clock_id) \
	clock_callback_##clock_id##_end = .;
