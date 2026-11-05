/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*-----------------------------------------------------------------------------
 *  Start Section for exception table
 *-----------------------------------------------------------------------------*/
#ifdef __IAR_SYSTEMS_ASM__
/* Assembly syntax for IAR assembler */
/* ROOT(9) = 512-byte alignment */
  section `.exc_vector_table`:CONST:ROOT(9)
  extern _z_rh850_reset
  extern _z_irq_spurious
  extern _z_rh850_rie
  extern _z_rh850_ucpop
  extern _z_rh850_mae
  extern _z_rh850_fatal
  extern _z_rh850_switch

/* RESET */
  align  4
  jr _z_rh850_reset
/* SYSERR */
  align  4
  jr  _z_irq_spurious
/* (R.F.U.) */
  align  4
  jr  _z_irq_spurious
/* FETRAP */
  align  4
  jr  _z_irq_spurious
/* TRAP0 */
  align  4
  jr  _z_rh850_switch
/* TRAP1 */
  align  4
  jr  _z_rh850_fatal
/* RIE */
  align  4
  jr  _z_rh850_rie
/* FPP/FPI */
  align  4
  jr  _z_irq_spurious
/* UCPOP */
  align  4
  jr  _z_rh850_ucpop
/* MIP/MDP */
  align  4
  jr  _z_irq_spurious
/* PIE */
  align  4
  jr  _z_irq_spurious
/* (R.F.U.) */
  align  4
  jr  _z_irq_spurious
/* MAE */
  align  4
  jr  _z_rh850_mae
/* (R.F.U.) */
  align  4
  jr  _z_irq_spurious
/* FENMI */
  align  4
  jr  _z_irq_spurious
/* FEINT */
  align  4
  jr  _z_irq_spurious
/* INTn(priority0) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority1) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority2) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority3) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority4) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority5) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority6) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority7) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority8) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority9) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority10) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority11) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority12) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority13) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority14) */
  align  4
  jr  _z_irq_spurious
/* INTn(priority15) */
  align  4
  jr  _z_irq_spurious

  end

#else
  #error "Unsupported toolchain"
#endif
