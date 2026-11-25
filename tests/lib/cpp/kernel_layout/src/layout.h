/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KERNEL_LAYOUT_H_
#define KERNEL_LAYOUT_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sizes of kernel structures as one language sees them. */
struct kernel_layout {
	size_t cpu_arch; /* sizeof(struct _cpu_arch) */
	size_t cpu;      /* sizeof(struct _cpu) */
	size_t ready_q;  /* offsetof(struct z_kernel, ready_q), behind cpus[] */
	size_t kernel;   /* sizeof(struct z_kernel) */
};

/* Implemented in layout_c.c, a C translation unit: the sizes as C sees them. */
struct kernel_layout kernel_layout_in_c(void);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_LAYOUT_H_ */
