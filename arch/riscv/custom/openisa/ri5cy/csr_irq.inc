/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <ri5cy_csr.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

	.macro __custom_csr_save_context __soc_esf_reg reg

	csrr \reg, RI5CY_LPSTART0
	sw \reg, __soc_esf_t_lpstart0_OFFSET(\__soc_esf_reg)
	csrr \reg, RI5CY_LPEND0
	sw \reg, __soc_esf_t_lpend0_OFFSET(\__soc_esf_reg)
	csrr \reg, RI5CY_LPCOUNT0
	sw \reg, __soc_esf_t_lpcount0_OFFSET(\__soc_esf_reg)
	csrr \reg, RI5CY_LPSTART1
	sw \reg, __soc_esf_t_lpstart1_OFFSET(\__soc_esf_reg)
	csrr \reg, RI5CY_LPEND1
	sw \reg, __soc_esf_t_lpend1_OFFSET(\__soc_esf_reg)
	csrr \reg, RI5CY_LPCOUNT1
	sw \reg, __soc_esf_t_lpcount1_OFFSET(\__soc_esf_reg)

	.endm

	.macro __custom_csr_restore_context __soc_esf_reg reg

	lw \reg, __soc_esf_t_lpstart0_OFFSET(\__soc_esf_reg)
	csrw RI5CY_LPSTART0, \reg
	lw \reg, __soc_esf_t_lpend0_OFFSET(\__soc_esf_reg)
	csrw RI5CY_LPEND0, \reg
	lw \reg, __soc_esf_t_lpcount0_OFFSET(\__soc_esf_reg)
	csrw RI5CY_LPCOUNT0, \reg
	lw \reg, __soc_esf_t_lpstart1_OFFSET(\__soc_esf_reg)
	csrw RI5CY_LPSTART1, \reg
	lw \reg, __soc_esf_t_lpend1_OFFSET(\__soc_esf_reg)
	csrw RI5CY_LPEND1, \reg
	lw \reg, __soc_esf_t_lpcount1_OFFSET(\__soc_esf_reg)
	csrw RI5CY_LPCOUNT1, \reg

	.endm

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
