/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/rh850/asm/asm_directive.h>

/*-----------------------------------------------------------------------------
 *  Start Section for exception table
 *-----------------------------------------------------------------------------*/
/* 512-byte alignment */
  RH850_ASM_DIRECTIVE_SECTION(CODE_ROOT, 9, .exc_vector_table)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_reset)
  RH850_ASM_DIRECTIVE_EXTERN(_z_irq_spurious)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_rie)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_ucpop)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_mae)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_fatal)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_switch)

/* RESET */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr _z_rh850_reset
/* SYSERR */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* (R.F.U.) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* FETRAP */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* TRAP0 */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_switch
/* TRAP1 */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fatal
/* RIE */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_rie
/* FPP/FPI */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* UCPOP */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ucpop
/* MIP/MDP */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* PIE */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* (R.F.U.) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* MAE */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_mae
/* (R.F.U.) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* FENMI */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* FEINT */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority0) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority1) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority2) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority3) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority4) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority5) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority6) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority7) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority8) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority9) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority10) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority11) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority12) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority13) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority14) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious
/* INTn(priority15) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_irq_spurious

  RH850_ASM_DIRECTIVE_END
