/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_ALIGNMENT_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_ALIGNMENT_H_

#define ARCH_STACK_PTR_ALIGN 4UL

/*
 * Dynamic thread object memory alignment.
 *
 * If support for SSEx extensions is enabled, then a 16 byte boundary is
 * required as both the 'fxsave' and 'fxrstor' instructions require this.
 * In all other cases a 4 byte boundary is sufficient.
 */

#if defined(CONFIG_EAGER_FPU_SHARING) || defined(CONFIG_LAZY_FPU_SHARING)
#ifdef CONFIG_SSE
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT     16
#else
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT     4
#endif
#else
/* No special alignment requirements, simply align on pointer size. */
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT     4
#endif /* CONFIG_*_FP_SHARING */

#endif  /* ZEPHYR_INCLUDE_ARCH_X86_IA32_ALIGNMENT_H_ */
