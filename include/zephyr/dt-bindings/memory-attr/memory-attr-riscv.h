/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V memory attribute DT binding definitions.
 * @ingroup dt_binding_mem_attr_riscv
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_RISCV_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_RISCV_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/**
 * @defgroup dt_binding_mem_attr_riscv RISC-V memory attributes
 * @ingroup dt_memory_attr_architecture
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define DT_MEM_RISCV_MASK			DT_MEM_ARCH_ATTR_MASK
#define DT_MEM_RISCV(x)				((x) << DT_MEM_ARCH_ATTR_SHIFT)

#define  ATTR_RISCV_TYPE_MAIN			BIT(0)
#define  ATTR_RISCV_TYPE_IO			BIT(1)
#define  ATTR_RISCV_TYPE_IO_R			BIT(2)
#define  ATTR_RISCV_TYPE_IO_W			BIT(3)
#define  ATTR_RISCV_TYPE_IO_X			BIT(4)
#define  ATTR_RISCV_TYPE_EMPTY			BIT(5)
#define  ATTR_RISCV_AMO_SWAP			BIT(6)
#define  ATTR_RISCV_AMO_LOGICAL			BIT(7)
#define  ATTR_RISCV_AMO_ARITHMETIC		BIT(8)
#define  ATTR_RISCV_IO_IDEMPOTENT_READ		BIT(9)
#define  ATTR_RISCV_IO_IDEMPOTENT_WRITE		BIT(10)
/** @endcond */

/**
 * @brief Extract RISC-V-specific bits from a full <tt>zephyr,memory-attr</tt> value.
 *
 * @param x Value to extract RISC-V-specific memory attribute bits from.
 *
 * @return RISC-V-specific memory attribute bits.
 */
 #define DT_MEM_RISCV_GET(x)			((x) & DT_MEM_RISCV_MASK)

/** Main (DRAM) memory type. */
#define DT_MEM_RISCV_TYPE_MAIN			DT_MEM_RISCV(ATTR_RISCV_TYPE_MAIN)
/** I/O memory type. */
#define DT_MEM_RISCV_TYPE_IO			DT_MEM_RISCV(ATTR_RISCV_TYPE_IO)
/** I/O memory with read attribute. */
#define DT_MEM_RISCV_TYPE_IO_R			DT_MEM_RISCV(ATTR_RISCV_TYPE_IO_R)
/** I/O memory with write attribute. */
#define DT_MEM_RISCV_TYPE_IO_W			DT_MEM_RISCV(ATTR_RISCV_TYPE_IO_W)
/** I/O memory with execute attribute. */
#define DT_MEM_RISCV_TYPE_IO_X			DT_MEM_RISCV(ATTR_RISCV_TYPE_IO_X)
/** Empty or reserved memory type. */
#define DT_MEM_RISCV_TYPE_EMPTY			DT_MEM_RISCV(ATTR_RISCV_TYPE_EMPTY)
/** AMO swap operations supported. */
#define DT_MEM_RISCV_AMO_SWAP			DT_MEM_RISCV(ATTR_RISCV_AMO_SWAP)
/** AMO logical operations supported. */
#define DT_MEM_RISCV_AMO_LOGICAL		DT_MEM_RISCV(ATTR_RISCV_AMO_LOGICAL)
/** AMO arithmetic operations supported. */
#define DT_MEM_RISCV_AMO_ARITHMETIC		DT_MEM_RISCV(ATTR_RISCV_AMO_ARITHMETIC)
/** I/O memory with idempotent read attribute. */
#define DT_MEM_RISCV_IO_IDEMPOTENT_READ		DT_MEM_RISCV(ATTR_RISCV_IO_IDEMPOTENT_READ)
/** I/O memory with idempotent write attribute. */
#define DT_MEM_RISCV_IO_IDEMPOTENT_WRITE	DT_MEM_RISCV(ATTR_RISCV_IO_IDEMPOTENT_WRITE)
/** Unknown or unsupported RISC-V memory type. */
#define DT_MEM_RISCV_UNKNOWN			DT_MEM_ARCH_ATTR_UNKNOWN

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_RISCV_H_ */
