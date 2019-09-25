/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV32_OPENISA_RV32M1_SOC_RI5CY_H_
#define SOC_RISCV32_OPENISA_RV32M1_SOC_RI5CY_H_

/* Control and Status Registers (CSRs) available for RI5CY. */
#define RI5CY_USTATUS   0x000
#define RI5CY_UTVEC     0x005
#define RI5CY_UHARTID   0x014
#define RI5CY_UEPC      0x041
#define RI5CY_UCAUSE    0x042
#define RI5CY_MSTATUS   0x300
#define RI5CY_MTVEC     0x305
#define RI5CY_MEPC      0x341
#define RI5CY_MCAUSE    0x342
#define RI5CY_PCCR0     0x780
#define RI5CY_PCCR1     0x781
#define RI5CY_PCCR2     0x782
#define RI5CY_PCCR3     0x783
#define RI5CY_PCCR4     0x784
#define RI5CY_PCCR5     0x785
#define RI5CY_PCCR6     0x786
#define RI5CY_PCCR7     0x787
#define RI5CY_PCCR8     0x788
#define RI5CY_PCCR9     0x789
#define RI5CY_PCCR10    0x78A
#define RI5CY_PCCR11    0x78B
#define RI5CY_PCER      0x7A0
#define RI5CY_PCMR      0x7A1
#define RI5CY_LPSTART0  0x7B0
#define RI5CY_LPEND0    0x7B1
#define RI5CY_LPCOUNT0  0x7B2
#define RI5CY_LPSTART1  0x7B4
#define RI5CY_LPEND1    0x7B5
#define RI5CY_LPCOUNT1  0x7B6
#define RI5CY_PRIVLV    0xC10
#define RI5CY_MHARTID   0xF14

/*
 * Map from SoC-specific configuration to generic Zephyr macros.
 *
 * These are expected by the code in arch/, and must be provided for
 * the kernel to work (or even build at all).
 *
 * Some of these may also apply to ZERO-RISCY; needs investigation.
 */

/*
 * MSTATUS CSR number. (Note this is the standard value in the RISC-V
 * privileged ISA v1.10).
 */
#define SOC_MSTATUS_REG RI5CY_MSTATUS
/* MSTATUS's interrupt enable mask. This is also standard. */
#define SOC_MSTATUS_IEN (1U << 3)
/*
 * Exception code mask. Use of the bottom five bits is a subset of
 * what the standard allocates (which is XLEN-1 bits).
 */
#define SOC_MCAUSE_EXP_MASK 0x1F
/*
 * Assembler instruction to exit from interrupt in machine mode.
 * The name "ERET" is a leftover from pre-v1.10 privileged ISA specs.
 * The "mret" mnemonic works properly with the Pulpino toolchain;
 * YMMV if using a generic toolchain.
 */
#define SOC_ERET mret
/* The ecall exception number. This is a standard value. */
#define SOC_MCAUSE_ECALL_EXP 11
/*
 * Default MSTATUS value to write when scheduling in a new thread for
 * the first time.
 *
 * - Preserve machine privileges in MPP. If you see any documentation
 *   telling you that MPP is read-only on this SoC, don't believe its
 *   lies.
 * - Enable interrupts when exiting from exception into a new thread
 *   by setting MPIE now, so it will be copied into IE on mret.
 */
#define RI5CY_MSTATUS_MPP_M     (0x3U << 11)
#define RI5CY_MSTATUS_MPIE_EN   (1U << 7)
#define SOC_MSTATUS_DEF_RESTORE (RI5CY_MSTATUS_MPP_M |	\
				 RI5CY_MSTATUS_MPIE_EN)

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_RI5CY_H_ */
