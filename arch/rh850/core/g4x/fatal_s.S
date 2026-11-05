/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "context_macros.inc"

  extern _z_rh850_fatal_error
  extern _z_rh850_isr_stack_enter
  extern _z_rh850_isr_stack_exit
  section .text:CODE(2)
  public _z_rh850_rie
  public _z_rh850_ucpop
  public _z_rh850_mae
  public _z_rh850_fatal

_z_rh850_rie:
_z_rh850_ucpop:
_z_rh850_mae:
  SAVE_FULL_CONTEXT_FE

/* entry stack switch */
  mov r0, r7
  mov sp, r6
  jarl _z_rh850_isr_stack_enter, lp
/* sp = returned SP (irq_stack or same) */
  mov r10, sp

  jarl _z_rh850_fatal_error, lp

/* exit stack switch */
  mov sp, r6
  jarl _z_rh850_isr_stack_exit, lp
/* sp = thread SP or irq_stack */
  mov r10, sp

  RESTORE_FULL_CONTEXT_FERET


_z_rh850_fatal:
  SAVE_FULL_CONTEXT_EI

/* entry stack switch */
  mov r6, r7
  mov sp, r6
  jarl _z_rh850_isr_stack_enter, lp
/* sp = returned SP (irq_stack or same) */
  mov r10, sp

  jarl _z_rh850_fatal_error, lp

/* exit stack switch */
  mov sp, r6
  jarl _z_rh850_isr_stack_exit, lp
/* sp = thread SP or irq_stack */
  mov r10, sp

  RESTORE_FULL_CONTEXT_EIRET

  end
