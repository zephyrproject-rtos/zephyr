/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_PLIC_VECTOR_H
#define ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_PLIC_VECTOR_H

/*
 * Should be added as part of the SOC_ESF_MEMBERS definition
 */
#define ANDES_PLIC_SOC_ESF_MEMBERS uint32_t plic_mcause

/*
 * Should be added as part of the SOC_ESF_INIT definition
 */
#define ANDES_PLIC_SOC_ESF_INIT 0

/*
 * Should be added as part of the GEN_SOC_OFFSET_SYMS definition
 */
#define ANDES_PLIC_GEN_SOC_OFFSET_SYMS() \
	GEN_OFFSET_SYM(soc_esf_t, plic_mcause);

/*
 * Must be called from __soc_save_context implementation.
 */
void andes_plic_save_context(void);

/*
 * Must be called from __soc_restore_context implementation.
 */
void andes_plic_restore_context(void);

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_ANDES_PLIC_VECTOR_H */
