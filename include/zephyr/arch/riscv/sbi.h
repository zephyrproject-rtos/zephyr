/*
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V Supervisor Binary Interface (SBI) definitions.
 *
 * Defines SBI extension IDs, function IDs, and error codes used by
 * S-mode code to request M-mode firmware services via the @c ecall
 * instruction.  The subset defined here covers only the extensions
 * used by Zephyr's in-tree minimal SBI runtime
 * (@c arch/riscv/core/sbi.S).
 *
 * References: RISC-V SBI Specification v2.0
 * (https://github.com/riscv-non-isa/riscv-sbi-doc)
 */

#ifndef ZEPHYR_ARCH_RISCV_INCLUDE_SBI_H_
#define ZEPHYR_ARCH_RISCV_INCLUDE_SBI_H_

/** @brief SBI extension ID for the Timer extension (TIME) */
#define SBI_EXT_TIME		0x54494D45

/** @brief SBI_EXT_TIME function ID: set the next timer deadline */
#define SBI_FUNC_SET_TIMER	0

/** @brief SBI return code: call completed successfully */
#define SBI_SUCCESS		0
/** @brief SBI return code: requested extension/function is not available */
#define SBI_ERR_NOT_SUPPORTED	-1

#endif /* ZEPHYR_ARCH_RISCV_INCLUDE_SBI_H_ */
