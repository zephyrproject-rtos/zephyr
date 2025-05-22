/* exception.h - automatically selects the correct exception.h file to include */

/*
 * Copyright (c) 2024 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_EXCEPTION_H_

#if defined(CONFIG_X86_64)
#include <zephyr/arch/x86/intel64/exception.h>
#elif defined(CONFIG_X86)
#include <zephyr/arch/x86/ia32/exception.h>
#elif defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/exception.h>
#elif defined(CONFIG_ARM)
#include <zephyr/arch/arm/exception.h>
#elif defined(CONFIG_ARC)
#include <zephyr/arch/arc/v2/exception.h>
#elif defined(CONFIG_NIOS2)
#include <zephyr/arch/nios2/exception.h>
#elif defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/exception.h>
#elif defined(CONFIG_XTENSA)
#include <zephyr/arch/xtensa/exception.h>
#elif defined(CONFIG_MIPS)
#include <zephyr/arch/mips/exception.h>
#elif defined(CONFIG_ARCH_POSIX)
#include <zephyr/arch/posix/exception.h>
#elif defined(CONFIG_SPARC)
#include <zephyr/arch/sparc/exception.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_EXCEPTION_H_ */
