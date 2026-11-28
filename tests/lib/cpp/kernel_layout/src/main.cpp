/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The C++ side of the comparison: every size that layout_c.c reports for C
 * must be the size C++ sees, otherwise C++ code reads kernel data at the
 * wrong offset.
 */

#include <zephyr/kernel_structs.h>
#include <zephyr/ztest.h>

#include "layout.h"

ZTEST_SUITE(cxx_kernel_layout, NULL, NULL, NULL, NULL, NULL);

ZTEST(cxx_kernel_layout, test_cpu_arch_size)
{
	const struct kernel_layout c_layout = kernel_layout_in_c();

	zassert_equal(sizeof(struct _cpu_arch), c_layout.cpu_arch,
		      "struct _cpu_arch: C++ %zu bytes, C %zu bytes", sizeof(struct _cpu_arch),
		      c_layout.cpu_arch);
}

ZTEST(cxx_kernel_layout, test_cpu_size)
{
	const struct kernel_layout c_layout = kernel_layout_in_c();

	zassert_equal(sizeof(struct _cpu), c_layout.cpu, "struct _cpu: C++ %zu bytes, C %zu bytes",
		      sizeof(struct _cpu), c_layout.cpu);
}

ZTEST(cxx_kernel_layout, test_kernel_ready_q_offset)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	ztest_test_skip(); /* the ready queue is per CPU, not in struct z_kernel */
#else
	const struct kernel_layout c_layout = kernel_layout_in_c();

	zassert_equal(offsetof(struct z_kernel, ready_q), c_layout.ready_q,
		      "_kernel.ready_q: at offset %zu in C++, %zu in C",
		      offsetof(struct z_kernel, ready_q), c_layout.ready_q);
#endif
}

ZTEST(cxx_kernel_layout, test_kernel_size)
{
	const struct kernel_layout c_layout = kernel_layout_in_c();

	zassert_equal(sizeof(struct z_kernel), c_layout.kernel,
		      "struct z_kernel: C++ %zu bytes, C %zu bytes", sizeof(struct z_kernel),
		      c_layout.kernel);
}
