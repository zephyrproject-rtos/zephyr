/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Precision timing shell static clock registration.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_SHELL_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_SHELL_H_

#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Timing Shell APIs
 * @defgroup precision_timing_shell Precision Timing Shell
 * @since 4.6
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Immutable shell registration for a lifetime-static precision clock. */
struct precision_timing_shell_entry {
	/** Shell instance name. */
	const char *name;
	/** Clock exposed by this entry. */
	const struct precision_clock *clock;
};

/**
 * @brief Statically register a precision clock with the precision timing shell.
 *
 * Defines an immutable, linker-collected registry entry that exposes @p clock_ptr
 * under the name @p _name in the ``precision_clock`` shell command. The @p _name
 * identifier is converted to a string literal and must be a valid C identifier
 * composed of ASCII letters, digits, and underscores. Duplicate names are not
 * detected at compile time; avoid them, because the shell reports the first
 * matching entry.
 *
 * The clock object referenced by @p clock_ptr must have static storage duration
 * (for example, a file-scope or @c static object). Its @ref precision_clock.api
 * pointer may be @c NULL until the owning adapter initializes it; shell commands
 * that read an uninitialized clock return @c -ENODEV. Initialize the adapter
 * before allowing concurrent shell access; do not reinitialize it afterwards.
 *
 * Place this macro at file scope, not inside a function.
 *
 * @param _name Unique shell instance name (C identifier). Converted to a string.
 * @param clock_ptr Pointer to a lifetime-static @ref precision_clock.
 */
#define PRECISION_TIMING_SHELL_REGISTER(_name, clock_ptr)                                    \
	static const STRUCT_SECTION_ITERABLE(precision_timing_shell_entry,                   \
					     _precision_timing_shell_##_name) = {                  \
		.name = STRINGIFY(_name),                                                    \
		.clock = (clock_ptr),                                                        \
	}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_SHELL_H_ */
