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

#ifdef CONFIG_EXCEPTION_DEBUG
/*
 * Explicit padding is needed for VPRs, because they use hardware stacking on part of arch_esf and
 * ESF_SW_IRQ_SIZEOF needs to be calculated accordingly.
 */
#define ESF_CSF                                                                                    \
	_callee_saved_t *csf;                                                                      \
	unsigned long padding1;                                                                    \
	unsigned long padding2;                                                                    \
	unsigned long padding3;
#else
#define ESF_CSF
#endif /* CONFIG_EXCEPTION_DEBUG */

#if DT_PROP(VPR_CPU, nordic_bus_width) == 64

#define SOC_ISR_STACKING_ESF_DECLARE                                                               \
	struct arch_esf {                                                                          \
		unsigned long s0;                                                                  \
		unsigned long mstatus;                                                             \
		struct soc_esf soc_context;                                                        \
		ESF_CSF;                                                                           \
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
	struct arch_esf {                                                                          \
		unsigned long s0;                                                                  \
		unsigned long mstatus;                                                             \
		struct soc_esf soc_context;                                                        \
		ESF_CSF;                                                                           \
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
		unsigned long _mcause;                                                             \
		unsigned long mepc;                                                                \
	} __aligned(16);

#endif /* DT_PROP(VPR_CPU, nordic_bus_width) == 64 */

/*
 * VPR stacked mcause needs to have proper value on initial stack.
 * Initial mret will restore this value.
 */
#define SOC_ISR_STACKING_ESR_INIT                                                                  \
	stack_init->_mcause = 0;

#else /* _ASMLANGUAGE */

/*
 * Size of the HW managed part of the ESF:
 *    sizeof(_mcause) + sizeof(_mepc)
 */
#define ESF_HW_SIZEOF	  (0x8)

/*
 * Size of the SW managed part of the ESF in case of exception
 */
#define ESF_SW_EXC_SIZEOF (__struct_arch_esf_SIZEOF - ESF_HW_SIZEOF)

/*
 * Size of the SW managed part of the ESF in case of interrupt
 * sizeof(s0) + sizeof(mstatus) + sizeof(soc_context) +...+ sizeof(ESF_CSF)
 */
#ifdef CONFIG_EXCEPTION_DEBUG
#define ESF_SW_IRQ_SIZEOF      (0x20)
#else
#define ESF_SW_IRQ_SIZEOF      (0x10)
#endif
/*
 * VPR needs aligned(8) SP when doing HW stacking, if this condition is not fulfilled it will move
 * SP by additional 4 bytes when HW stacking is done. This will be indicated by LSB bit in stacked
 * MEPC.
 *
 * The stacked MEPC slot does not hold the return address of the trap: it holds the MEPC value of
 * the interrupted context, which MRET loads back into MEPC after jumping to the current MEPC.
 * Zephyr uses the slot as the return address of the frame, so the original slot is kept in
 * sp_align and written back before MRET. Otherwise an interrupt preempting another one before its
 * first instruction makes the preempted handler see its own address as return address.
 */
#define STORE_SP_ALIGN_BIT_FROM_MEPC				\
	addi t1, sp, __struct_arch_esf_soc_context_OFFSET;	\
	lr t0, __struct_arch_esf_mepc_OFFSET(sp);		\
	sr t0, __soc_esf_t_sp_align_OFFSET(t1)

#define RESTORE_SP_ALIGN_BIT_TO_MEPC				\
	addi t1, sp, __struct_arch_esf_soc_context_OFFSET;	\
	lr t0, __soc_esf_t_sp_align_OFFSET(t1);			\
	sr t0, __struct_arch_esf_mepc_OFFSET(sp)

/*
 * VPR does not clear MSTATUS.MIE on interrupt entry, so a higher level interrupt may preempt the
 * trap handler while it updates the nesting counter, switches stacks or reschedules. Mask
 * interrupts with the first instruction of the trap handler and allow nesting only while the ISR
 * itself runs on the interrupt stack. MRET restores the interrupted MIE state.
 */
#define SOC_ISR_SW_STACKING			\
	csrci mstatus, MSTATUS_IEN;		\
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
stacking_keep_going:				\
	STORE_SP_ALIGN_BIT_FROM_MEPC

#define SOC_ISR_SW_UNSTACKING			\
	RESTORE_SP_ALIGN_BIT_TO_MEPC;		\
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

#define SOC_ISR_ENABLE_NESTING csrsi mstatus, MSTATUS_IEN
#define SOC_ISR_DISABLE_NESTING csrci mstatus, MSTATUS_IEN

/*
 * An interrupt can still preempt another one before the first instruction of the trap handler.
 * The preempted handler has not been accounted in the nesting counter yet, so the preempting one
 * runs as the outermost interrupt and its return address points to the trap handler. It must return
 * to the preempted handler on the same stack pointer the hardware stacked it on, so it must never
 * reschedule.
 */
#define SOC_ISR_SKIP_RESCHEDULE				\
	lr t0, __struct_arch_esf_mepc_OFFSET(sp);	\
	la t1, _isr_wrapper;				\
	beq t0, t1, no_reschedule

#endif /* _ASMLANGUAGE */

#endif /* SOC_RISCV_NORDIC_NRF_COMMON_VPR_SOC_ISR_STACKING_H_ */
