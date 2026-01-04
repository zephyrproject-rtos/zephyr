/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_H_
#define PMP_H_

#include <zephyr/dt-bindings/memory-attr/memory-attr-riscv.h>
#include <zephyr/sys/iterable_sections.h>

#define PMPCFG_STRIDE (__riscv_xlen / 8)

#define DT_MEM_RISCV_TO_PMP_PERM(dt_attr) (		\
	(((dt_attr) & DT_MEM_RISCV_TYPE_IO_R) ? PMP_R : 0) |	\
	(((dt_attr) & DT_MEM_RISCV_TYPE_IO_W) ? PMP_W : 0) |	\
	(((dt_attr) & DT_MEM_RISCV_TYPE_IO_X) ? PMP_X : 0))

/**
 * @brief SoC-specific PMP region descriptor.
 *
 * SoCs can define additional memory regions that need PMP protection
 * using the PMP_SOC_REGION_DEFINE macro.
 * These regions are automatically collected via iterable sections and
 * programmed into the PMP during initialization.
 *
 * Note: Uses start/end pointers instead of start/size to support regions
 * defined by linker symbols where the size is not a compile-time constant.
 */
struct pmp_soc_region {
	/** Start address of the region (must be aligned to PMP granularity) */
	const void *start;
	/** End address of the region (exclusive) */
	const void *end;
	/** PMP permission flags (PMP_R, PMP_W, PMP_X combinations) */
	uint8_t perm;
};

/**
 * @brief Define a SoC-specific PMP region.
 *
 * This macro allows SoCs to register memory regions that require PMP
 * protection. The regions are collected at link time and programmed
 * during PMP initialization.
 *
 * @param name Unique identifier for this region
 * @param _start Start address of the region (pointer or linker symbol)
 * @param _end End address of the region (pointer or linker symbol, exclusive)
 * @param _perm PMP permission flags (PMP_R | PMP_W | PMP_X)
 */
#define PMP_SOC_REGION_DEFINE(name, _start, _end, _perm)                                           \
	static const STRUCT_SECTION_ITERABLE(pmp_soc_region, name) = {                             \
		.start = (const void *)(_start),                                                   \
		.end = (const void *)(_end),                                                       \
		.perm = (_perm),                                                                   \
	}

void z_riscv_pmp_init(void);
void z_riscv_pmp_kernelmode_prepare(struct k_thread *thread);
void z_riscv_pmp_kernelmode_enable(struct k_thread *thread);
void z_riscv_pmp_kernelmode_disable(void);
void z_riscv_pmp_usermode_init(struct k_thread *thread);
void z_riscv_pmp_usermode_prepare(struct k_thread *thread);
void z_riscv_pmp_usermode_enable(struct k_thread *thread);

#ifdef CONFIG_ZTEST
void z_riscv_pmp_read_config(unsigned long *pmp_cfg, size_t pmp_cfg_size);
void z_riscv_pmp_read_addr(unsigned long *pmp_addr, size_t pmp_addr_size);
void pmp_decode_region(uint8_t cfg_byte, unsigned long *pmp_addr, unsigned int index,
		       unsigned long *start, unsigned long *end);
#endif /* CONFIG_ZTEST */

#endif /* PMP_H_ */
