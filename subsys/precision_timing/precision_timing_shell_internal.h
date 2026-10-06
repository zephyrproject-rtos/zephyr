/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PRECISION_TIMING_PRECISION_TIMING_SHELL_INTERNAL_H_
#define ZEPHYR_SUBSYS_PRECISION_TIMING_PRECISION_TIMING_SHELL_INTERNAL_H_

#include <zephyr/precision_timing/precision_timing_shell.h>

/*
 * Internal handle used to pin one registry entry while a shell command calls
 * its clock provider. The copied name remains stable when the registry array is
 * reordered.
 */
struct precision_timing_shell_operation {
	const struct precision_clock *clock;
	char name[PRECISION_TIMING_SHELL_NAME_MAX + 1];
};

int precision_timing_shell_operation_acquire(const char *name,
					     struct precision_timing_shell_operation *operation);

void precision_timing_shell_operation_release(struct precision_timing_shell_operation *operation);

#endif /* ZEPHYR_SUBSYS_PRECISION_TIMING_PRECISION_TIMING_SHELL_INTERNAL_H_ */
