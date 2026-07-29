/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Early boot for split TCM/OCM images on Versal RPU.
 *
 * This stub is linked at the start of rom_start (TCM @ 0x0) and is the first
 * code executed on reset. The arch vector table follows in rom_start and is
 * copied to 0x0 by z_arm_relocate_vector_table() after the MPU maps OCM.
 * The rest of the image is linked at CONFIG_SRAM_BASE_ADDRESS (typically OCM).
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/arm/cortex_a_r/cpu.h>

#define RPU_0_CFG		0xFF9A0100
#define SCTLR_HIVECS		0x00002000

GDATA(z_interrupt_stacks)
GDATA(z_arm_svc_stack)
GDATA(z_arm_sys_stack)
GDATA(z_arm_fiq_stack)
GDATA(z_arm_abort_stack)
GDATA(z_arm_undef_stack)
GTEXT(z_arm_reset)

_ASM_FILE_PROLOGUE

	.section .text.boot, "ax"
	.global versal_rpu_ocm_boot
	.type versal_rpu_ocm_boot, %function
	.align 2

/*
 * Program one MPU region: select by index, set base, attributes, size+enable.
 * r0=base, r1=(size<<1)|1, r2=attr, r3=index
 */
.macro mpu_program base, size_en, attr, index
	mov r3, #\index
	mcr p15, 0, r3, c6, c2, 0
	isb
	ldr r0, =\base
	mcr p15, 0, r0, c6, c1, 0
	ldr r2, =\attr
	mcr p15, 0, r2, c6, c1, 4
	ldr r1, =\size_en
	mcr p15, 0, r1, c6, c1, 2
	dsb
	isb
.endm

versal_rpu_ocm_boot:
	/*
	 * Point every mode SP at OCM stacks (constants only — no OCM access yet).
	 * Same layout as z_arm_reset in reset.S.
	 */
	ldr r5, =(z_arm_fiq_stack + CONFIG_ARMV7_FIQ_STACK_SIZE)
	ldr r6, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	ldr r7, =(z_arm_abort_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)
	ldr r8, =(z_arm_undef_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)
	ldr r9, =(z_arm_svc_stack + CONFIG_ARMV7_SVC_STACK_SIZE)
	ldr r10, =(z_arm_sys_stack + CONFIG_ARMV7_SYS_STACK_SIZE)

	msr CPSR_c, #(MODE_FIQ | I_BIT | F_BIT)
	mov sp, r5

	msr CPSR_c, #(MODE_IRQ | I_BIT | F_BIT)
	mov sp, r6

	msr CPSR_c, #(MODE_ABT | I_BIT | F_BIT)
	mov sp, r7

	msr CPSR_c, #(MODE_UND | I_BIT | F_BIT)
	mov sp, r8

	msr CPSR_c, #(MODE_SVC | I_BIT | F_BIT)
	mov sp, r9

	msr CPSR_c, #(MODE_SYS | I_BIT | F_BIT)
	mov sp, r10

	msr CPSR_c, #(MODE_SVC | I_BIT | F_BIT)

#if defined(CONFIG_CPU_HAS_FPU)
	mrc p15, 0, r0, c1, c0, 2
	orr r0, r0, #(CPACR_CP10(CPACR_FA) | CPACR_CP11(CPACR_FA))
	mcr p15, 0, r0, c1, c0, 2
	isb
#endif

	/* Disable MPU and caches. */
	mrc p15, 0, r2, c1, c0, 0
	bic r2, r2, #0x5
	bic r2, r2, #SCTLR_HIVECS
	bic r2, r2, #0x1000
	mcr p15, 0, r2, c1, c0, 0
	isb

	mov r2, #0
	mcr p15, 0, r2, c7, c5, 0
	mcr p15, 0, r2, c15, c5, 0
	dsb
	isb

	/* Disable all 16 MPU regions. */
	mov r3, #0
1:	mcr p15, 0, r3, c6, c2, 0
	mrc p15, 0, r2, c6, c1, 2
	bic r2, r2, #1
	dsb
	mcr p15, 0, r2, c6, c1, 2
	dsb
	isb
	add r3, r3, #1
	cmp r3, #16
	bne 1b

	/*
	 * Minimal MPU map (must stay in TCM assembly — mpu_config lives in OCM
	 * and cannot be accessed before these regions are enabled):
	 *   0: TCM 512K cacheable (vectors at 0x0 after relocation)
	 *   1: device 2G
	 *   2: OCM SRAM (same window as the working arch+SoC boot stub)
	 */
	mpu_program 0x00000000, 0x25, 0x10b, 0
	mpu_program 0x80000000, 0x3d, 0x105, 1
	mpu_program CONFIG_SRAM_BASE_ADDRESS, 0x23, 0x30b, 2

	/* Clear VINITHI after device region is mapped. */
	ldr r0, =RPU_0_CFG
	ldr r1, [r0]
	bic r1, r1, #0x4
	str r1, [r0]
	dsb

	/* Enable MPU + I/D-cache (SCTLR bits M=1, C=1, I=12). */
	mrc p15, 0, r1, c1, c0, 0
	bic r1, r1, #SCTLR_HIVECS
	ldr r0, =0x1005
	orr r1, r1, r0
	dsb
	mcr p15, 0, r1, c1, c0, 0
	isb

	b z_arm_reset
