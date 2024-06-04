/* cpu.h - automatically selects the correct arch.h file to include */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_CPU_H_

#include <zephyr/arch/arch_interface.h>

#if defined(CONFIG_X86)
#include <zephyr/arch/x86/arch.h>
#elif defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/arch.h>
#elif defined(CONFIG_ARM)
#include <zephyr/arch/arm/arch.h>
#elif defined(CONFIG_ARC)
#include <zephyr/arch/arc/arch.h>
#elif defined(CONFIG_NIOS2)
#include <zephyr/arch/nios2/arch.h>
#elif defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/arch.h>
#elif defined(CONFIG_XTENSA)
#include <zephyr/arch/xtensa/arch.h>
#elif defined(CONFIG_MIPS)
#include <zephyr/arch/mips/arch.h>
#elif defined(CONFIG_ARCH_POSIX)
#include <zephyr/arch/posix/arch.h>
#elif defined(CONFIG_SPARC)
#include <zephyr/arch/sparc/arch.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_CPU_H_ */
