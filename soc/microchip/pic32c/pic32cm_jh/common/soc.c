/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc.c
 * @brief Microchip PIC32CM JH family initialization code
 */

#include <zephyr/devicetree.h>
#include <soc.h>

#define SRAM0_NODE DT_CHOSEN(zephyr_sram)
#define SRAM0_BASE DT_REG_ADDR(SRAM0_NODE)
#define SRAM0_SIZE DT_REG_SIZE(SRAM0_NODE)

/**
 * @brief Initialize SUPC peripheral from devicetree settings.
 *
 * Enables MCLK for SUPC and configures voltage regulator and
 * voltage reference (on-demand and standby mode) based on
 * standby-regulator-sel, vref-on-demand, and vref-standby-mode
 * devicetree properties of the supc node.
 *
 * @retval 0 On success.
 */
__attribute__((always_inline)) static inline int soc_supc_init(void)
{
	supc_registers_t *supc_regs = (supc_registers_t *)DT_REG_ADDR(DT_NODELABEL(supc));
	int standby_regulator_sel = DT_PROP(DT_NODELABEL(supc), standby_regulator_sel);
	int vref_on_demand = DT_PROP(DT_NODELABEL(supc), vref_on_demand);
	int vref_standby_mode = DT_PROP(DT_NODELABEL(supc), vref_standby_mode);

	uint32_t vref_val = supc_regs->SUPC_VREF;

	/* Configure VREG. Mask the values loaded from NVM during reset.*/
	supc_regs->SUPC_VREG = (standby_regulator_sel != 0) ?
			(supc_regs->SUPC_VREG | SUPC_VREG_RUNSTDBY_Msk) : supc_regs->SUPC_VREG;

	/* Configure SUPC VREF On demand */
	vref_val = (vref_on_demand != 0) ?
			(vref_val | SUPC_VREF_ONDEMAND_Msk) : vref_val;

	/* Configure SUPC VREF Standby mode*/
	vref_val = (vref_standby_mode != 0) ?
			(vref_val | SUPC_VREF_RUNSTDBY_Msk) : vref_val;

	supc_regs->SUPC_VREF = vref_val;

	return 0;
}

/**
 * @brief Early Reset hook to run SoC-specific initialization.
 *
 * This function performs 32-bit writes to clear the entire SRAM very early at reset.
 *
 * After reset, SRAM content (data + ECC bits) is random and ECC is enabled by default.
 * Any 8-bit or 16-bit write may trigger single or double ECC errors due to the internal
 * read-modify-write of 32-bit words. Therefore, the SRAM must be initialized before use
 * to ensure ECC correctness.
 *
 * Implementing it naked with assembly code guarantees no stack access happens during
 * the initialization.
 */
__attribute__((naked)) void soc_early_reset_hook(void)
{
	/* sram initialization */
	__asm__ volatile(
		"	ldr  r0, =%c[start]\n"
		"	ldr  r1, =%c[end]\n"
		"	movs r2, #0\n"
		"loop:\n"
		"	stmia r0!, {r2}\n"
		"	cmp  r0, r1\n"
		"	blo  loop\n"
		"	bx   lr\n"
		:: [start] "i" (SRAM0_BASE), [end] "i" (SRAM0_BASE + SRAM0_SIZE));
}

/**
 * @brief Reset hook to run SoC-specific initialization.
 *
 * This is invoked early at reset.
 */
void soc_reset_hook(void)
{
	soc_supc_init();
}
