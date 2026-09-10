/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * POSIX ARCH specific public inline "assembler" functions and macros
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain/common.h>
#include <zephyr/types.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/posix/posix_soc_if.h>

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_GCC_H_ */
