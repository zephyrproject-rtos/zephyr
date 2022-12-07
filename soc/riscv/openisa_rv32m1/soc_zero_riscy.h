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
 * Exception code mask. Use of the bottom five bits is a subset of
 * what the standard allocates (which is XLEN-1 bits).
 */
#define SOC_MCAUSE_EXP_MASK 0x1F

/* The ecall exception number. This is a standard value. */
#define SOC_MCAUSE_ECALL_EXP 11

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_ZERO_RISCY_H_ */
