/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_ISR_STACKING_H_
#define SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_ISR_STACKING_H_

#include <zephyr/arch/riscv/irq.h>

#if !defined(_ASMLANGUAGE)

#include <zephyr/devicetree.h>

#define VPR_CPU DT_INST(0, nordic_vpr)

#if DT_PROP(VPR_CPU, nordic_bus_width) == 64

#define SOC_ISR_STACKING_ESF_DECLARE                                                               \
	struct __esf {                                                                             \
		unsigned long s0;                                                                  \
		unsigned long mstatus;                                                             \
		unsigned long tp;                                                                  \
		struct soc_esf soc_context;                                                        \
                                                                                                   \
		unsigned long t2;                                                                  \
		unsigned long ra;                                                                  \
		unsigned long t0;                                                                  \
		unsigned long t1;                                                                  \
		unsigned long a4;                                                                  \
		unsigned long a5;                                                                  \
		unsigned long a2;                                                                  \
		unsigned long a3;                                                                  \
		unsigned long a0;                                                                  \
		unsigned long a1;                                                                  \
		unsigned long mepc;                                                                \
		unsigned long _mcause;                                                             \
	} __aligned(16);

#else /* DT_PROP(VPR_CPU, nordic_bus_width) == 32 */

#define SOC_ISR_STACKING_ESF_DECLARE                                                               \
	struct __esf {                                                                             \
		unsigned long s0;                                                                  \
		unsigned long mstatus;                                                             \
		unsigned long tp;                                                                  \
		struct soc_esf soc_context;                                                        \
                                                                                                   \
		unsigned long ra;                                                                  \
		unsigned long t2;                                                                  \
		unsigned long t1;                                                                  \
		unsigned long t0;                                                                  \
		unsigned long a5;                                                                  \
		unsigned long a4;                                                                  \
		unsigned long a3;                                                                  \
		unsigned long a2;                                                                  \
		unsigned long a1;                                                                  \
		unsigned long a0;                                                                  \
		unsigned long mepc;                                                                \
		unsigned long _mcause;                                                             \
	} __aligned(16);

#endif /* DT_PROP(VPR_CPU, nordic_bus_width) == 64 */

#else /* _ASMLANGUAGE */

/*
 * Size of the HW managed part of the ESF:
 *    sizeof(_mcause) + sizeof(_mepc)
 */
#define ESF_HW_SIZEOF	  (0x8)

/*
 * Size of the SW managed part of the ESF in case of exception
 */
#define ESF_SW_EXC_SIZEOF (__z_arch_esf_t_SIZEOF - ESF_HW_SIZEOF)

/*
 * Size of the SW managed part of the ESF in case of interrupt
 *   sizeof(__padding) + ... + sizeof(soc_context)
 */
#define ESF_SW_IRQ_SIZEOF (0x10)

#define SOC_ISR_SW_STACKING			\
	csrw mscratch, t0;			\
						\
	csrr t0, mcause;			\
	srli t0, t0, RISCV_MCAUSE_IRQ_POS;	\
	bnez t0, stacking_is_interrupt;		\
						\
	csrrw t0, mscratch, zero;		\
						\
	addi sp, sp, -ESF_SW_EXC_SIZEOF;	\
	DO_CALLER_SAVED(sr);			\
	j stacking_keep_going;			\
						\
stacking_is_interrupt:				\
	addi sp, sp, -ESF_SW_IRQ_SIZEOF;	\
						\
stacking_keep_going:

#define SOC_ISR_SW_UNSTACKING			\
	csrr t0, mcause;			\
	srli t0, t0, RISCV_MCAUSE_IRQ_POS;	\
	bnez t0, unstacking_is_interrupt;	\
						\
	DO_CALLER_SAVED(lr);			\
	addi sp, sp, ESF_SW_EXC_SIZEOF;		\
	j unstacking_keep_going;		\
						\
unstacking_is_interrupt:			\
	addi sp, sp, ESF_SW_IRQ_SIZEOF;		\
						\
unstacking_keep_going:

#endif /* _ASMLANGUAGE */

#endif /* SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_ISR_STACKING_H_ */
