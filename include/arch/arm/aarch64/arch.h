/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 specific kernel interface header
 *
 * This header contains the ARM64 specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/arm/aarch64/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_ARCH_H_

/* Add include for DTS generated information */
#include <devicetree.h>

#include <arch/arm/aarch64/thread.h>
#include <arch/arm/aarch64/exc.h>
#include <arch/arm/aarch64/irq.h>
#include <arch/arm/aarch64/misc.h>
#include <arch/arm/aarch64/asm_inline.h>
#include <arch/arm/aarch64/cpu.h>
#include <arch/arm/aarch64/macro.inc>
#include <arch/arm/aarch64/sys_io.h>
#include <arch/arm/aarch64/timer.h>
#include <arch/arm/aarch64/error.h>
#include <arch/arm/aarch64/arm_mmu.h>
#include <arch/common/addr_types.h>
#include <arch/common/sys_bitops.h>
#include <arch/common/ffs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/**
 * @brief Declare the ARCH_STACK_PTR_ALIGN
 *
 * Denotes the required alignment of the stack pointer on public API
 * boundaries
 *
 */
#define ARCH_STACK_PTR_ALIGN	16

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in k_mem_partition_attr_t
 * objects. The format of a k_mem_partition_attr_t object
 * is an uint32_t composed by permission and attribute flags
 * located in include/arch/arm/aarch64/arm_mmu.h
 */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
	{MT_P_RW_U_RW})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
	{MT_P_RW_U_NA})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
	{MT_P_RO_U_RO})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
	{MT_P_RO_U_NA})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RX_U_RX ((k_mem_partition_attr_t) \
	{MT_P_RX_U_RX})

/* Typedef for the k_mem_partition attribute */
typedef struct {
	uint32_t attrs;
} k_mem_partition_attr_t;

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_ARCH_H_ */
