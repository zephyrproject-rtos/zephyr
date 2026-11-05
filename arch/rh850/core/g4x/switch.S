/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

  section  .text:CODE(2)
#include "context_macros.inc"

/* -----------------------------------------------------------------------------
 * TRAP0: yield path
 *-----------------------------------------------------------------------------*/
  public _z_rh850_switch
_z_rh850_switch:
  SAVE_FULL_CONTEXT_EI

/* Perform context switch */
  jarl _z_rh850_arch_switch_isr, lp

  RESTORE_FULL_CONTEXT_EIRET

/* void z_rh850_arch_switch_thread(void *switch_to, void **switched_from)
 * IAR RH850 ABI:
 *   a1 (r6) = switch_to      (context handle = frame pointer)
 *   a2 (r7) = switched_from  (pointer to handle output) */
  public _z_rh850_arch_switch_thread

_z_rh850_arch_switch_thread:
/* Trigger TRAP0 to call _z_rh850_switch */
  trap 0

/* When return, restore the current context thread */
  jmp [lp]

/* void z_rh850_arch_switch_isr(void *switch_to, void **switched_from)
 *  r6 = switch_to (new handle = frame ptr)
 *  r7 = switched_from (void **) */
  public _z_rh850_arch_switch_isr
_z_rh850_arch_switch_isr:
/* Save current SP (frame of the current thread) into *switched_from */
  st.w sp, 0[r7]

/* Switch SP to new handle */
  mov r6, sp
  jmp [lp]

  end
