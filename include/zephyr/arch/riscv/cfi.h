/*
 * Copyright (c) 2025 NXP Semicondutors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_CFI_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_CFI_H_

#define ARCH_CFI_UNDEFINED_RETURN_ADDRESS() __asm__ volatile(".cfi_undefined ra")

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CFI_H_ */
