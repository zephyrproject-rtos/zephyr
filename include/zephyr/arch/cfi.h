/*
 * Copyright (c) 2025 NXP Semicondutors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DWARF Control Flow Integrity (CFI) support for architectures.
 */

#ifndef ZEPHYR_INCLUDE_CFI_H_
#define ZEPHYR_INCLUDE_CFI_H_

#include <zephyr/arch/arch_interface.h>

#if defined(__GNUC__) || defined(__clang__)
/* DWARF Control Flow Integrity (CFI) support for architectures is only
 * supported by LLVM and GCC toolchains.
 */
#if defined(CONFIG_X86_64)
#include <zephyr/arch/x86/intel64/cfi.h>
#elif defined(CONFIG_X86)
#include <zephyr/arch/x86/ia32/cfi.h>
#elif defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/cfi.h>
#elif defined(CONFIG_ARM)
#include <zephyr/arch/arm/cfi.h>
#elif defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/cfi.h>
#endif
#endif

/*
 * Inform the unwinder that the return address is undefined.
 * This prevents bogus unwinding and potential faults.
 */
#ifndef ARCH_CFI_UNDEFINED_RETURN_ADDRESS
#define ARCH_CFI_UNDEFINED_RETURN_ADDRESS()
#endif

#endif /* ZEPHYR_INCLUDE_CFI_H_ */
