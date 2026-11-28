/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The C side of the comparison, deliberately a C file. */

#include <zephyr/kernel_structs.h>
#include <zephyr/sys/util.h>

#include "layout.h"

/* An empty struct has size 0 in C (GNU extension) and size 1 in C++. With
 * CONFIG_CPP, struct _cpu_arch must not be empty in C either, or the two
 * languages disagree on struct _cpu and on every member of _kernel behind
 * cpus[]. Checked at build time as well, so build-only runs catch it too.
 */
BUILD_ASSERT(sizeof(struct _cpu_arch) > 0,
	     "struct _cpu_arch is empty in C although CONFIG_CPP=y: "
	     "C and C++ disagree on the layout of struct _cpu and _kernel");

struct kernel_layout kernel_layout_in_c(void)
{
	const struct kernel_layout layout = {
		.cpu_arch = sizeof(struct _cpu_arch),
		.cpu = sizeof(struct _cpu),
#ifndef CONFIG_SCHED_CPU_MASK_PIN_ONLY
		.ready_q = offsetof(struct z_kernel, ready_q),
#endif
		.kernel = sizeof(struct z_kernel),
	};

	return layout;
}
