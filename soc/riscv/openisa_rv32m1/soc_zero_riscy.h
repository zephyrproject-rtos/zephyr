/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV32_OPENISA_RV32M1_SOC_ZERO_RISCY_H_
#define SOC_RISCV32_OPENISA_RV32M1_SOC_ZERO_RISCY_H_

/* Control and Status Registers (CSRs) available for ZERO_RISCY. */
#define ZERO_RISCY_MSTATUS 0x300U
#define ZERO_RISCY_MTVEC   0x305U
#define ZERO_RISCY_MEPC    0x341U
#define ZERO_RISCY_MCAUSE  0x342U
#define ZERO_RISCY_PCCR0   0x780U
#define ZERO_RISCY_PCCR1   0x781U
#define ZERO_RISCY_PCCR2   0x782U
#define ZERO_RISCY_PCCR3   0x783U
#define ZERO_RISCY_PCCR4   0x784U
#define ZERO_RISCY_PCCR5   0x785U
#define ZERO_RISCY_PCCR6   0x786U
#define ZERO_RISCY_PCCR7   0x787U
#define ZERO_RISCY_PCCR8   0x788U
#define ZERO_RISCY_PCCR9   0x789U
#define ZERO_RISCY_PCCR10  0x78AU
#define ZERO_RISCY_PCCR    0x78BU
#define ZERO_RISCY_PCER    0x7A0U
#define ZERO_RISCY_PCMR    0x7A1U
#define ZERO_RISCY_MHARTID 0xF14U

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
#define SOC_MSTATUS_REG ZERO_RISCY_MSTATUS
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
#define ZERO_RISCY_MSTATUS_MPP_M     (0x3U << 11)
#define ZERO_RISCY_MSTATUS_MPIE_EN   (1U << 7)
#define SOC_MSTATUS_DEF_RESTORE (ZERO_RISCY_MSTATUS_MPP_M |	\
				 ZERO_RISCY_MSTATUS_MPIE_EN)

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_ZERO_RISCY_H_ */
