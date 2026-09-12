/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon public kernel interface
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_ARCH_H_

#include <zephyr/devicetree.h>
#include <zephyr/arch/hexagon/thread.h>
#include <zephyr/arch/hexagon/exception.h>
#include <zephyr/arch/hexagon/error.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/mm.h>
#include <hexagon_vm.h>
#include <irq.h>
#include <irq_connect.h>
#include <zephyr/sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Architecture-specific kernel initialization.
 *
 * No additional initialization needed for Hexagon.
 */
static inline void arch_kernel_init(void)
{
}

/**
 * @brief Get the current system clock cycle count as a 32-bit value.
 *
 * @return Current 32-bit cycle count.
 */
extern uint32_t sys_clock_cycle_get_32(void);

/**
 * @brief Get the current cycle count (32-bit).
 *
 * @return Current 32-bit hardware cycle count.
 */
static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

/**
 * @brief Get the current system clock cycle count as a 64-bit value.
 *
 * @return Current 64-bit cycle count.
 */
extern uint64_t sys_clock_cycle_get_64(void);

/**
 * @brief Get the current cycle count (64-bit).
 *
 * @return Current 64-bit hardware cycle count.
 */
static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

/**
 * @brief Execute a no-op instruction.
 */
static inline void arch_nop(void)
{
	__asm__ volatile("nop");
}

/* ISR nesting counter -- incremented/decremented in irq_manage.c */
extern uint32_t z_hexagon_isr_nesting;

/**
 * @brief Check if currently executing in interrupt context.
 *
 * @retval true if in ISR context.
 * @retval false if in thread context.
 */
static inline bool arch_is_in_isr(void)
{
	return z_hexagon_isr_nesting != 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_ARCH_H_ */
