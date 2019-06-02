/* asm_inline_gcc.h - ARC inline assembler and macros for public functions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ASM_INLINE_GCC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stddef.h>

/**
 *  @brief read timestamp register (CPU frequency)
 */
extern u64_t z_tsc_read(void);



#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ASM_INLINE_GCC_H_ */
