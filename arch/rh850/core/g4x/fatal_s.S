/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "context_macros.inc"

  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_fatal_error)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_enter)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_exit)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_stack_enter)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_isr_stack_exit)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_irq_switch)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_arch_switch_isr)
  RH850_ASM_DIRECTIVE_SECTION(CODE, 2, .text)
  RH850_ASM_DIRECTIVE_GLOBAL(_z_rh850_fe_exc)
  RH850_ASM_DIRECTIVE_GLOBAL(_z_rh850_fe_spurious)
  RH850_ASM_DIRECTIVE_GLOBAL(_z_rh850_ei_exc)
  RH850_ASM_DIRECTIVE_GLOBAL(_z_rh850_ei_spurious)
  RH850_ASM_DIRECTIVE_GLOBAL(_z_rh850_fatal)

/*
 * The reason codes below are hard-coded: 0 is K_ERR_CPU_EXCEPTION and 1 is
 * K_ERR_SPURIOUS_IRQ. fatal.c checks these values with BUILD_ASSERT.
 */

/* Unhandled FE level interrupt (FENMI, FEINT): reason is K_ERR_SPURIOUS_IRQ */
_z_rh850_fe_spurious:
  SAVE_FULL_CONTEXT_FE
  mov 1, r29                 /* r29 = reason */
  br fe_exc_common

/* FE level CPU exception: reason is K_ERR_CPU_EXCEPTION */
_z_rh850_fe_exc:
  SAVE_FULL_CONTEXT_FE
  mov r0, r29                /* r29 = reason */

fe_exc_common:
  mov sp, r28                /* r28 = esf */
  jarl _z_rh850_fatal_common, lp

  RESTORE_FULL_CONTEXT_FERET

/* ARCH_EXCEPT (trap 0x10): reason is passed in r6 */
_z_rh850_fatal:
  SAVE_FULL_CONTEXT_EI
  mov r6, r29                /* r29 = reason */
  br ei_exc_common

/* Unhandled EI level interrupt (direct vector): reason is K_ERR_SPURIOUS_IRQ */
_z_rh850_ei_spurious:
  SAVE_FULL_CONTEXT_EI
  mov 1, r29                 /* r29 = reason */
  br ei_exc_common

/* EI level CPU exception (FPE, FXE): reason is K_ERR_CPU_EXCEPTION */
_z_rh850_ei_exc:
  SAVE_FULL_CONTEXT_EI
  mov r0, r29                /* r29 = reason */

ei_exc_common:
  mov sp, r28                /* r28 = esf */
  jarl _z_rh850_fatal_common, lp

  RESTORE_FULL_CONTEXT_EIRET

/*
 * Common fatal exception handling, mirroring __isr_wrapper.
 *
 * Entry: sp = r28 = esf, r29 = reason, interrupts disabled.
 * Exit:  sp = context frame to restore. It is the faulting context, or
 *        the next thread when the fatal error handler aborted the
 *        current thread.
 *
 * r27, r28 and r29 are callee-saved and their original values are
 * already stored in the frame.
 */
_z_rh850_fatal_common:
  mov lp, r27

/* nested++ so that arch_is_in_isr() reports exception context */
  jarl _z_rh850_isr_enter, lp

/* entry stack switch */
  mov r28, r6
  jarl _z_rh850_isr_stack_enter, lp
/* sp = returned SP (irq_stack or same) */
  mov r10, sp

/* z_rh850_fatal_error(esf, reason) */
  mov r28, r6
  mov r29, r7
  jarl _z_rh850_fatal_error, lp

/* exit stack switch */
  mov sp, r6
  jarl _z_rh850_isr_stack_exit, lp
/* sp = thread SP or irq_stack */
  mov r10, sp

/* bool z_rh850_irq_switch(void **switch_to, void **switch_from); */
  addi -8, sp, sp            /* allocate 8 bytes local on the current stack */
  mov sp, r6                 /* r6 = &switch_to */
  addi 4, sp, r7             /* r7 = &switch_from */
  jarl _z_rh850_irq_switch, lp

  cmp 0, r10
  be fatal_no_switch

  ld.w 0[sp], r6             /* r6 = switch_to */
  ld.w 4[sp], r7             /* r7 = switch_from */
  addi 8, sp, sp             /* free local storage */

/* The current thread was aborted: switch to the next thread */
  jarl _z_rh850_arch_switch_isr, lp
  br fatal_switch_done

fatal_no_switch:
  addi 8, sp, sp             /* free local storage */

fatal_switch_done:
  jarl _z_rh850_isr_exit, lp

  mov r27, lp
  jmp [lp]

  RH850_ASM_DIRECTIVE_END
