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
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_fe_exc)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_fe_spurious)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_ei_exc)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_ei_spurious)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_fatal)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_switch)

/* RESET */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr _z_rh850_reset
/* SYSERR (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* (R.F.U.) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* FETRAP (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* TRAP0 (EI level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_switch
/* TRAP1 (EI level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fatal
/* RIE (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* FPE/FXE (EI level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_exc
/* UCPOP (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* MIP/MDP (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* PIE (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* (R.F.U.) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* MAE (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* (R.F.U.) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_exc
/* FENMI (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_spurious
/* FEINT (FE level) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_fe_spurious
/* INTn(priority0) (EI level, direct vector method) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority1) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority2) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority3) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority4) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority5) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority6) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority7) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority8) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority9) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority10) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority11) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority12) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority13) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority14) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious
/* INTn(priority15) */
  RH850_ASM_DIRECTIVE_P2ALIGN(4)
  jr  _z_rh850_ei_spurious

  RH850_ASM_DIRECTIVE_END
