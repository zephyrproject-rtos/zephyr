/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/rh850/asm/asm_directive.h>
#include "context_macros.inc"

  RH850_ASM_DIRECTIVE_EXTERN(__sw_isr_table)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_irq_switch)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_enter)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_exit)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_arch_switch_isr)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_stack_enter)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_stack_exit)
  RH850_ASM_DIRECTIVE_SECTION(CODE, 2, .text)
  RH850_ASM_DIRECTIVE_GLOBAL(__isr_wrapper)
__isr_wrapper:
/* Context save */
  SAVE_FULL_CONTEXT_EI

/* nested++ for the current CPU */
  jarl _z_rh850_isr_enter, lp

/* entry stack switch */
  mov sp, r6
  jarl _z_rh850_isr_stack_enter, lp
/* sp = returned SP (irq_stack or same) */
  mov r10, sp

/* Get irq number and call corresponding handler */
/* Get current irq number */
  stsr CPU_REG_EIIC,r1,CPU_SEL_EIIC
  addi -4096,r1,r1

/* Enable interrupts */
  ei

/* Call corresponding handler from _sw_isr_table */
  shl 3,r1
  movhi RH850_HI(__sw_isr_table),r1,r28
  movea RH850_LO(__sw_isr_table),r28,r28
  ld.w 0[r28],r6
  ld.w 4[r28],r1
  jarl [r1],lp

/* Disable interrupts */
  di

/* exit stack switch */
  mov sp, r6
  jarl _z_rh850_isr_stack_exit, lp
/* sp = thread SP or irq_stack */
  mov r10, sp

/* IRQ EXIT / SCHEDULING */
/* bool z_rh850_irq_exit(void **switch_to, void **switch_from); */
  addi    -8, sp, sp         /* allocate 8 bytes local on the current stack */
  mov     sp, r6             /* a1 (r6) = &switch_to */
  addi    4, sp, r7          /* a2 (r7) = &switch_from */
  jarl    _z_rh850_irq_switch, lp

  cmp     0, r10
  be      no_switch          /* skip switch if z_rh850_irq_exit() returned false */

/* Load the switch handles from the local storage.
 *  switch_to   : new context handle (stack pointer) for the next thread
 *  switch_from : &current->switch_handle
 */
  ld.w    0[sp], r6          /* r6 = switch_to */
  ld.w    4[sp], r7          /* r7 = switch_from */

  addi    8, sp, sp          /* free local storage on current stack */

/* Perform the ISR-based context switch */
  jarl    _z_rh850_arch_switch_isr, lp
  br      end_switch

no_switch:
/* No context switch requested: just free the local storage. */
  addi    8, sp, sp

end_switch:
/* No context switch requested: just free the local storage. */
  jarl    _z_rh850_isr_exit, lp

/* Context restore */
  RESTORE_FULL_CONTEXT_EIRET
  nop

  RH850_ASM_DIRECTIVE_END
