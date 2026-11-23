/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_ZERO_RISCY_CSR_H_
#define ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_ZERO_RISCY_CSR_H_

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_RISCV_CUSTOM_OPENISA_ZERO_RISCY_CSR_H_ */
