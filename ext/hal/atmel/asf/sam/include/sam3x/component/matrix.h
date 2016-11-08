/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) %copyright_year%, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _SAM3XA_MATRIX_COMPONENT_
#define _SAM3XA_MATRIX_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR AHB Bus Matrix */
/* ============================================================================= */
/** \addtogroup SAM3XA_MATRIX AHB Bus Matrix */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Matrix hardware registers */
typedef struct {
  __IO uint32_t MATRIX_MCFG[6]; /**< \brief (Matrix Offset: 0x0000) Master Configuration Register */
  __I  uint32_t Reserved1[10];
  __IO uint32_t MATRIX_SCFG[9]; /**< \brief (Matrix Offset: 0x0040) Slave Configuration Register */
  __I  uint32_t Reserved2[7];
  __IO uint32_t MATRIX_PRAS0;   /**< \brief (Matrix Offset: 0x0080) Priority Register A for Slave 0 */
  __I  uint32_t Reserved3[1];
  __IO uint32_t MATRIX_PRAS1;   /**< \brief (Matrix Offset: 0x0088) Priority Register A for Slave 1 */
  __I  uint32_t Reserved4[1];
  __IO uint32_t MATRIX_PRAS2;   /**< \brief (Matrix Offset: 0x0090) Priority Register A for Slave 2 */
  __I  uint32_t Reserved5[1];
  __IO uint32_t MATRIX_PRAS3;   /**< \brief (Matrix Offset: 0x0098) Priority Register A for Slave 3 */
  __I  uint32_t Reserved6[1];
  __IO uint32_t MATRIX_PRAS4;   /**< \brief (Matrix Offset: 0x00A0) Priority Register A for Slave 4 */
  __I  uint32_t Reserved7[1];
  __IO uint32_t MATRIX_PRAS5;   /**< \brief (Matrix Offset: 0x00A8) Priority Register A for Slave 5 */
  __I  uint32_t Reserved8[1];
  __IO uint32_t MATRIX_PRAS6;   /**< \brief (Matrix Offset: 0x00B0) Priority Register A for Slave 6 */
  __I  uint32_t Reserved9[1];
  __IO uint32_t MATRIX_PRAS7;   /**< \brief (Matrix Offset: 0x00B8) Priority Register A for Slave 7 */
  __I  uint32_t Reserved10[1];
  __IO uint32_t MATRIX_PRAS8;   /**< \brief (Matrix Offset: 0x00C0) Priority Register A for Slave 8 */
  __I  uint32_t Reserved11[1];
  __I  uint32_t Reserved12[14];
  __IO uint32_t MATRIX_MRCR;    /**< \brief (Matrix Offset: 0x0100) Master Remap Control Register */
  __I  uint32_t Reserved13[4];
  __IO uint32_t CCFG_SYSIO;     /**< \brief (Matrix Offset: 0x0114) System I/O Configuration register */
  __I  uint32_t Reserved14[51];
  __IO uint32_t MATRIX_WPMR;    /**< \brief (Matrix Offset: 0x1E4) Write Protect Mode Register */
  __I  uint32_t MATRIX_WPSR;    /**< \brief (Matrix Offset: 0x1E8) Write Protect Status Register */
} Matrix;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- MATRIX_MCFG[6] : (MATRIX Offset: 0x0000) Master Configuration Register -------- */
#define MATRIX_MCFG_ULBT_Pos 0
#define MATRIX_MCFG_ULBT_Msk (0x7u << MATRIX_MCFG_ULBT_Pos) /**< \brief (MATRIX_MCFG[6]) Undefined Length Burst Type */
#define MATRIX_MCFG_ULBT(value) ((MATRIX_MCFG_ULBT_Msk & ((value) << MATRIX_MCFG_ULBT_Pos)))
/* -------- MATRIX_SCFG[9] : (MATRIX Offset: 0x0040) Slave Configuration Register -------- */
#define MATRIX_SCFG_SLOT_CYCLE_Pos 0
#define MATRIX_SCFG_SLOT_CYCLE_Msk (0xffu << MATRIX_SCFG_SLOT_CYCLE_Pos) /**< \brief (MATRIX_SCFG[9]) Maximum Number of Allowed Cycles for a Burst */
#define MATRIX_SCFG_SLOT_CYCLE(value) ((MATRIX_SCFG_SLOT_CYCLE_Msk & ((value) << MATRIX_SCFG_SLOT_CYCLE_Pos)))
#define MATRIX_SCFG_DEFMSTR_TYPE_Pos 16
#define MATRIX_SCFG_DEFMSTR_TYPE_Msk (0x3u << MATRIX_SCFG_DEFMSTR_TYPE_Pos) /**< \brief (MATRIX_SCFG[9]) Default Master Type */
#define MATRIX_SCFG_DEFMSTR_TYPE(value) ((MATRIX_SCFG_DEFMSTR_TYPE_Msk & ((value) << MATRIX_SCFG_DEFMSTR_TYPE_Pos)))
#define MATRIX_SCFG_FIXED_DEFMSTR_Pos 18
#define MATRIX_SCFG_FIXED_DEFMSTR_Msk (0x7u << MATRIX_SCFG_FIXED_DEFMSTR_Pos) /**< \brief (MATRIX_SCFG[9]) Fixed Default Master */
#define MATRIX_SCFG_FIXED_DEFMSTR(value) ((MATRIX_SCFG_FIXED_DEFMSTR_Msk & ((value) << MATRIX_SCFG_FIXED_DEFMSTR_Pos)))
#define MATRIX_SCFG_ARBT_Pos 24
#define MATRIX_SCFG_ARBT_Msk (0x3u << MATRIX_SCFG_ARBT_Pos) /**< \brief (MATRIX_SCFG[9]) Arbitration Type */
#define MATRIX_SCFG_ARBT(value) ((MATRIX_SCFG_ARBT_Msk & ((value) << MATRIX_SCFG_ARBT_Pos)))
/* -------- MATRIX_PRAS0 : (MATRIX Offset: 0x0080) Priority Register A for Slave 0 -------- */
#define MATRIX_PRAS0_M0PR_Pos 0
#define MATRIX_PRAS0_M0PR_Msk (0x3u << MATRIX_PRAS0_M0PR_Pos) /**< \brief (MATRIX_PRAS0) Master 0 Priority */
#define MATRIX_PRAS0_M0PR(value) ((MATRIX_PRAS0_M0PR_Msk & ((value) << MATRIX_PRAS0_M0PR_Pos)))
#define MATRIX_PRAS0_M1PR_Pos 4
#define MATRIX_PRAS0_M1PR_Msk (0x3u << MATRIX_PRAS0_M1PR_Pos) /**< \brief (MATRIX_PRAS0) Master 1 Priority */
#define MATRIX_PRAS0_M1PR(value) ((MATRIX_PRAS0_M1PR_Msk & ((value) << MATRIX_PRAS0_M1PR_Pos)))
#define MATRIX_PRAS0_M2PR_Pos 8
#define MATRIX_PRAS0_M2PR_Msk (0x3u << MATRIX_PRAS0_M2PR_Pos) /**< \brief (MATRIX_PRAS0) Master 2 Priority */
#define MATRIX_PRAS0_M2PR(value) ((MATRIX_PRAS0_M2PR_Msk & ((value) << MATRIX_PRAS0_M2PR_Pos)))
#define MATRIX_PRAS0_M3PR_Pos 12
#define MATRIX_PRAS0_M3PR_Msk (0x3u << MATRIX_PRAS0_M3PR_Pos) /**< \brief (MATRIX_PRAS0) Master 3 Priority */
#define MATRIX_PRAS0_M3PR(value) ((MATRIX_PRAS0_M3PR_Msk & ((value) << MATRIX_PRAS0_M3PR_Pos)))
#define MATRIX_PRAS0_M4PR_Pos 16
#define MATRIX_PRAS0_M4PR_Msk (0x3u << MATRIX_PRAS0_M4PR_Pos) /**< \brief (MATRIX_PRAS0) Master 4 Priority */
#define MATRIX_PRAS0_M4PR(value) ((MATRIX_PRAS0_M4PR_Msk & ((value) << MATRIX_PRAS0_M4PR_Pos)))
#define MATRIX_PRAS0_M5PR_Pos 20
#define MATRIX_PRAS0_M5PR_Msk (0x3u << MATRIX_PRAS0_M5PR_Pos) /**< \brief (MATRIX_PRAS0) Master 5 Priority */
#define MATRIX_PRAS0_M5PR(value) ((MATRIX_PRAS0_M5PR_Msk & ((value) << MATRIX_PRAS0_M5PR_Pos)))
/* -------- MATRIX_PRAS1 : (MATRIX Offset: 0x0088) Priority Register A for Slave 1 -------- */
#define MATRIX_PRAS1_M0PR_Pos 0
#define MATRIX_PRAS1_M0PR_Msk (0x3u << MATRIX_PRAS1_M0PR_Pos) /**< \brief (MATRIX_PRAS1) Master 0 Priority */
#define MATRIX_PRAS1_M0PR(value) ((MATRIX_PRAS1_M0PR_Msk & ((value) << MATRIX_PRAS1_M0PR_Pos)))
#define MATRIX_PRAS1_M1PR_Pos 4
#define MATRIX_PRAS1_M1PR_Msk (0x3u << MATRIX_PRAS1_M1PR_Pos) /**< \brief (MATRIX_PRAS1) Master 1 Priority */
#define MATRIX_PRAS1_M1PR(value) ((MATRIX_PRAS1_M1PR_Msk & ((value) << MATRIX_PRAS1_M1PR_Pos)))
#define MATRIX_PRAS1_M2PR_Pos 8
#define MATRIX_PRAS1_M2PR_Msk (0x3u << MATRIX_PRAS1_M2PR_Pos) /**< \brief (MATRIX_PRAS1) Master 2 Priority */
#define MATRIX_PRAS1_M2PR(value) ((MATRIX_PRAS1_M2PR_Msk & ((value) << MATRIX_PRAS1_M2PR_Pos)))
#define MATRIX_PRAS1_M3PR_Pos 12
#define MATRIX_PRAS1_M3PR_Msk (0x3u << MATRIX_PRAS1_M3PR_Pos) /**< \brief (MATRIX_PRAS1) Master 3 Priority */
#define MATRIX_PRAS1_M3PR(value) ((MATRIX_PRAS1_M3PR_Msk & ((value) << MATRIX_PRAS1_M3PR_Pos)))
#define MATRIX_PRAS1_M4PR_Pos 16
#define MATRIX_PRAS1_M4PR_Msk (0x3u << MATRIX_PRAS1_M4PR_Pos) /**< \brief (MATRIX_PRAS1) Master 4 Priority */
#define MATRIX_PRAS1_M4PR(value) ((MATRIX_PRAS1_M4PR_Msk & ((value) << MATRIX_PRAS1_M4PR_Pos)))
#define MATRIX_PRAS1_M5PR_Pos 20
#define MATRIX_PRAS1_M5PR_Msk (0x3u << MATRIX_PRAS1_M5PR_Pos) /**< \brief (MATRIX_PRAS1) Master 5 Priority */
#define MATRIX_PRAS1_M5PR(value) ((MATRIX_PRAS1_M5PR_Msk & ((value) << MATRIX_PRAS1_M5PR_Pos)))
/* -------- MATRIX_PRAS2 : (MATRIX Offset: 0x0090) Priority Register A for Slave 2 -------- */
#define MATRIX_PRAS2_M0PR_Pos 0
#define MATRIX_PRAS2_M0PR_Msk (0x3u << MATRIX_PRAS2_M0PR_Pos) /**< \brief (MATRIX_PRAS2) Master 0 Priority */
#define MATRIX_PRAS2_M0PR(value) ((MATRIX_PRAS2_M0PR_Msk & ((value) << MATRIX_PRAS2_M0PR_Pos)))
#define MATRIX_PRAS2_M1PR_Pos 4
#define MATRIX_PRAS2_M1PR_Msk (0x3u << MATRIX_PRAS2_M1PR_Pos) /**< \brief (MATRIX_PRAS2) Master 1 Priority */
#define MATRIX_PRAS2_M1PR(value) ((MATRIX_PRAS2_M1PR_Msk & ((value) << MATRIX_PRAS2_M1PR_Pos)))
#define MATRIX_PRAS2_M2PR_Pos 8
#define MATRIX_PRAS2_M2PR_Msk (0x3u << MATRIX_PRAS2_M2PR_Pos) /**< \brief (MATRIX_PRAS2) Master 2 Priority */
#define MATRIX_PRAS2_M2PR(value) ((MATRIX_PRAS2_M2PR_Msk & ((value) << MATRIX_PRAS2_M2PR_Pos)))
#define MATRIX_PRAS2_M3PR_Pos 12
#define MATRIX_PRAS2_M3PR_Msk (0x3u << MATRIX_PRAS2_M3PR_Pos) /**< \brief (MATRIX_PRAS2) Master 3 Priority */
#define MATRIX_PRAS2_M3PR(value) ((MATRIX_PRAS2_M3PR_Msk & ((value) << MATRIX_PRAS2_M3PR_Pos)))
#define MATRIX_PRAS2_M4PR_Pos 16
#define MATRIX_PRAS2_M4PR_Msk (0x3u << MATRIX_PRAS2_M4PR_Pos) /**< \brief (MATRIX_PRAS2) Master 4 Priority */
#define MATRIX_PRAS2_M4PR(value) ((MATRIX_PRAS2_M4PR_Msk & ((value) << MATRIX_PRAS2_M4PR_Pos)))
#define MATRIX_PRAS2_M5PR_Pos 20
#define MATRIX_PRAS2_M5PR_Msk (0x3u << MATRIX_PRAS2_M5PR_Pos) /**< \brief (MATRIX_PRAS2) Master 5 Priority */
#define MATRIX_PRAS2_M5PR(value) ((MATRIX_PRAS2_M5PR_Msk & ((value) << MATRIX_PRAS2_M5PR_Pos)))
/* -------- MATRIX_PRAS3 : (MATRIX Offset: 0x0098) Priority Register A for Slave 3 -------- */
#define MATRIX_PRAS3_M0PR_Pos 0
#define MATRIX_PRAS3_M0PR_Msk (0x3u << MATRIX_PRAS3_M0PR_Pos) /**< \brief (MATRIX_PRAS3) Master 0 Priority */
#define MATRIX_PRAS3_M0PR(value) ((MATRIX_PRAS3_M0PR_Msk & ((value) << MATRIX_PRAS3_M0PR_Pos)))
#define MATRIX_PRAS3_M1PR_Pos 4
#define MATRIX_PRAS3_M1PR_Msk (0x3u << MATRIX_PRAS3_M1PR_Pos) /**< \brief (MATRIX_PRAS3) Master 1 Priority */
#define MATRIX_PRAS3_M1PR(value) ((MATRIX_PRAS3_M1PR_Msk & ((value) << MATRIX_PRAS3_M1PR_Pos)))
#define MATRIX_PRAS3_M2PR_Pos 8
#define MATRIX_PRAS3_M2PR_Msk (0x3u << MATRIX_PRAS3_M2PR_Pos) /**< \brief (MATRIX_PRAS3) Master 2 Priority */
#define MATRIX_PRAS3_M2PR(value) ((MATRIX_PRAS3_M2PR_Msk & ((value) << MATRIX_PRAS3_M2PR_Pos)))
#define MATRIX_PRAS3_M3PR_Pos 12
#define MATRIX_PRAS3_M3PR_Msk (0x3u << MATRIX_PRAS3_M3PR_Pos) /**< \brief (MATRIX_PRAS3) Master 3 Priority */
#define MATRIX_PRAS3_M3PR(value) ((MATRIX_PRAS3_M3PR_Msk & ((value) << MATRIX_PRAS3_M3PR_Pos)))
#define MATRIX_PRAS3_M4PR_Pos 16
#define MATRIX_PRAS3_M4PR_Msk (0x3u << MATRIX_PRAS3_M4PR_Pos) /**< \brief (MATRIX_PRAS3) Master 4 Priority */
#define MATRIX_PRAS3_M4PR(value) ((MATRIX_PRAS3_M4PR_Msk & ((value) << MATRIX_PRAS3_M4PR_Pos)))
#define MATRIX_PRAS3_M5PR_Pos 20
#define MATRIX_PRAS3_M5PR_Msk (0x3u << MATRIX_PRAS3_M5PR_Pos) /**< \brief (MATRIX_PRAS3) Master 5 Priority */
#define MATRIX_PRAS3_M5PR(value) ((MATRIX_PRAS3_M5PR_Msk & ((value) << MATRIX_PRAS3_M5PR_Pos)))
/* -------- MATRIX_PRAS4 : (MATRIX Offset: 0x00A0) Priority Register A for Slave 4 -------- */
#define MATRIX_PRAS4_M0PR_Pos 0
#define MATRIX_PRAS4_M0PR_Msk (0x3u << MATRIX_PRAS4_M0PR_Pos) /**< \brief (MATRIX_PRAS4) Master 0 Priority */
#define MATRIX_PRAS4_M0PR(value) ((MATRIX_PRAS4_M0PR_Msk & ((value) << MATRIX_PRAS4_M0PR_Pos)))
#define MATRIX_PRAS4_M1PR_Pos 4
#define MATRIX_PRAS4_M1PR_Msk (0x3u << MATRIX_PRAS4_M1PR_Pos) /**< \brief (MATRIX_PRAS4) Master 1 Priority */
#define MATRIX_PRAS4_M1PR(value) ((MATRIX_PRAS4_M1PR_Msk & ((value) << MATRIX_PRAS4_M1PR_Pos)))
#define MATRIX_PRAS4_M2PR_Pos 8
#define MATRIX_PRAS4_M2PR_Msk (0x3u << MATRIX_PRAS4_M2PR_Pos) /**< \brief (MATRIX_PRAS4) Master 2 Priority */
#define MATRIX_PRAS4_M2PR(value) ((MATRIX_PRAS4_M2PR_Msk & ((value) << MATRIX_PRAS4_M2PR_Pos)))
#define MATRIX_PRAS4_M3PR_Pos 12
#define MATRIX_PRAS4_M3PR_Msk (0x3u << MATRIX_PRAS4_M3PR_Pos) /**< \brief (MATRIX_PRAS4) Master 3 Priority */
#define MATRIX_PRAS4_M3PR(value) ((MATRIX_PRAS4_M3PR_Msk & ((value) << MATRIX_PRAS4_M3PR_Pos)))
#define MATRIX_PRAS4_M4PR_Pos 16
#define MATRIX_PRAS4_M4PR_Msk (0x3u << MATRIX_PRAS4_M4PR_Pos) /**< \brief (MATRIX_PRAS4) Master 4 Priority */
#define MATRIX_PRAS4_M4PR(value) ((MATRIX_PRAS4_M4PR_Msk & ((value) << MATRIX_PRAS4_M4PR_Pos)))
#define MATRIX_PRAS4_M5PR_Pos 20
#define MATRIX_PRAS4_M5PR_Msk (0x3u << MATRIX_PRAS4_M5PR_Pos) /**< \brief (MATRIX_PRAS4) Master 5 Priority */
#define MATRIX_PRAS4_M5PR(value) ((MATRIX_PRAS4_M5PR_Msk & ((value) << MATRIX_PRAS4_M5PR_Pos)))
/* -------- MATRIX_PRAS5 : (MATRIX Offset: 0x00A8) Priority Register A for Slave 5 -------- */
#define MATRIX_PRAS5_M0PR_Pos 0
#define MATRIX_PRAS5_M0PR_Msk (0x3u << MATRIX_PRAS5_M0PR_Pos) /**< \brief (MATRIX_PRAS5) Master 0 Priority */
#define MATRIX_PRAS5_M0PR(value) ((MATRIX_PRAS5_M0PR_Msk & ((value) << MATRIX_PRAS5_M0PR_Pos)))
#define MATRIX_PRAS5_M1PR_Pos 4
#define MATRIX_PRAS5_M1PR_Msk (0x3u << MATRIX_PRAS5_M1PR_Pos) /**< \brief (MATRIX_PRAS5) Master 1 Priority */
#define MATRIX_PRAS5_M1PR(value) ((MATRIX_PRAS5_M1PR_Msk & ((value) << MATRIX_PRAS5_M1PR_Pos)))
#define MATRIX_PRAS5_M2PR_Pos 8
#define MATRIX_PRAS5_M2PR_Msk (0x3u << MATRIX_PRAS5_M2PR_Pos) /**< \brief (MATRIX_PRAS5) Master 2 Priority */
#define MATRIX_PRAS5_M2PR(value) ((MATRIX_PRAS5_M2PR_Msk & ((value) << MATRIX_PRAS5_M2PR_Pos)))
#define MATRIX_PRAS5_M3PR_Pos 12
#define MATRIX_PRAS5_M3PR_Msk (0x3u << MATRIX_PRAS5_M3PR_Pos) /**< \brief (MATRIX_PRAS5) Master 3 Priority */
#define MATRIX_PRAS5_M3PR(value) ((MATRIX_PRAS5_M3PR_Msk & ((value) << MATRIX_PRAS5_M3PR_Pos)))
#define MATRIX_PRAS5_M4PR_Pos 16
#define MATRIX_PRAS5_M4PR_Msk (0x3u << MATRIX_PRAS5_M4PR_Pos) /**< \brief (MATRIX_PRAS5) Master 4 Priority */
#define MATRIX_PRAS5_M4PR(value) ((MATRIX_PRAS5_M4PR_Msk & ((value) << MATRIX_PRAS5_M4PR_Pos)))
#define MATRIX_PRAS5_M5PR_Pos 20
#define MATRIX_PRAS5_M5PR_Msk (0x3u << MATRIX_PRAS5_M5PR_Pos) /**< \brief (MATRIX_PRAS5) Master 5 Priority */
#define MATRIX_PRAS5_M5PR(value) ((MATRIX_PRAS5_M5PR_Msk & ((value) << MATRIX_PRAS5_M5PR_Pos)))
/* -------- MATRIX_PRAS6 : (MATRIX Offset: 0x00B0) Priority Register A for Slave 6 -------- */
#define MATRIX_PRAS6_M0PR_Pos 0
#define MATRIX_PRAS6_M0PR_Msk (0x3u << MATRIX_PRAS6_M0PR_Pos) /**< \brief (MATRIX_PRAS6) Master 0 Priority */
#define MATRIX_PRAS6_M0PR(value) ((MATRIX_PRAS6_M0PR_Msk & ((value) << MATRIX_PRAS6_M0PR_Pos)))
#define MATRIX_PRAS6_M1PR_Pos 4
#define MATRIX_PRAS6_M1PR_Msk (0x3u << MATRIX_PRAS6_M1PR_Pos) /**< \brief (MATRIX_PRAS6) Master 1 Priority */
#define MATRIX_PRAS6_M1PR(value) ((MATRIX_PRAS6_M1PR_Msk & ((value) << MATRIX_PRAS6_M1PR_Pos)))
#define MATRIX_PRAS6_M2PR_Pos 8
#define MATRIX_PRAS6_M2PR_Msk (0x3u << MATRIX_PRAS6_M2PR_Pos) /**< \brief (MATRIX_PRAS6) Master 2 Priority */
#define MATRIX_PRAS6_M2PR(value) ((MATRIX_PRAS6_M2PR_Msk & ((value) << MATRIX_PRAS6_M2PR_Pos)))
#define MATRIX_PRAS6_M3PR_Pos 12
#define MATRIX_PRAS6_M3PR_Msk (0x3u << MATRIX_PRAS6_M3PR_Pos) /**< \brief (MATRIX_PRAS6) Master 3 Priority */
#define MATRIX_PRAS6_M3PR(value) ((MATRIX_PRAS6_M3PR_Msk & ((value) << MATRIX_PRAS6_M3PR_Pos)))
#define MATRIX_PRAS6_M4PR_Pos 16
#define MATRIX_PRAS6_M4PR_Msk (0x3u << MATRIX_PRAS6_M4PR_Pos) /**< \brief (MATRIX_PRAS6) Master 4 Priority */
#define MATRIX_PRAS6_M4PR(value) ((MATRIX_PRAS6_M4PR_Msk & ((value) << MATRIX_PRAS6_M4PR_Pos)))
#define MATRIX_PRAS6_M5PR_Pos 20
#define MATRIX_PRAS6_M5PR_Msk (0x3u << MATRIX_PRAS6_M5PR_Pos) /**< \brief (MATRIX_PRAS6) Master 5 Priority */
#define MATRIX_PRAS6_M5PR(value) ((MATRIX_PRAS6_M5PR_Msk & ((value) << MATRIX_PRAS6_M5PR_Pos)))
/* -------- MATRIX_PRAS7 : (MATRIX Offset: 0x00B8) Priority Register A for Slave 7 -------- */
#define MATRIX_PRAS7_M0PR_Pos 0
#define MATRIX_PRAS7_M0PR_Msk (0x3u << MATRIX_PRAS7_M0PR_Pos) /**< \brief (MATRIX_PRAS7) Master 0 Priority */
#define MATRIX_PRAS7_M0PR(value) ((MATRIX_PRAS7_M0PR_Msk & ((value) << MATRIX_PRAS7_M0PR_Pos)))
#define MATRIX_PRAS7_M1PR_Pos 4
#define MATRIX_PRAS7_M1PR_Msk (0x3u << MATRIX_PRAS7_M1PR_Pos) /**< \brief (MATRIX_PRAS7) Master 1 Priority */
#define MATRIX_PRAS7_M1PR(value) ((MATRIX_PRAS7_M1PR_Msk & ((value) << MATRIX_PRAS7_M1PR_Pos)))
#define MATRIX_PRAS7_M2PR_Pos 8
#define MATRIX_PRAS7_M2PR_Msk (0x3u << MATRIX_PRAS7_M2PR_Pos) /**< \brief (MATRIX_PRAS7) Master 2 Priority */
#define MATRIX_PRAS7_M2PR(value) ((MATRIX_PRAS7_M2PR_Msk & ((value) << MATRIX_PRAS7_M2PR_Pos)))
#define MATRIX_PRAS7_M3PR_Pos 12
#define MATRIX_PRAS7_M3PR_Msk (0x3u << MATRIX_PRAS7_M3PR_Pos) /**< \brief (MATRIX_PRAS7) Master 3 Priority */
#define MATRIX_PRAS7_M3PR(value) ((MATRIX_PRAS7_M3PR_Msk & ((value) << MATRIX_PRAS7_M3PR_Pos)))
#define MATRIX_PRAS7_M4PR_Pos 16
#define MATRIX_PRAS7_M4PR_Msk (0x3u << MATRIX_PRAS7_M4PR_Pos) /**< \brief (MATRIX_PRAS7) Master 4 Priority */
#define MATRIX_PRAS7_M4PR(value) ((MATRIX_PRAS7_M4PR_Msk & ((value) << MATRIX_PRAS7_M4PR_Pos)))
#define MATRIX_PRAS7_M5PR_Pos 20
#define MATRIX_PRAS7_M5PR_Msk (0x3u << MATRIX_PRAS7_M5PR_Pos) /**< \brief (MATRIX_PRAS7) Master 5 Priority */
#define MATRIX_PRAS7_M5PR(value) ((MATRIX_PRAS7_M5PR_Msk & ((value) << MATRIX_PRAS7_M5PR_Pos)))
/* -------- MATRIX_PRAS8 : (MATRIX Offset: 0x00C0) Priority Register A for Slave 8 -------- */
#define MATRIX_PRAS8_M0PR_Pos 0
#define MATRIX_PRAS8_M0PR_Msk (0x3u << MATRIX_PRAS8_M0PR_Pos) /**< \brief (MATRIX_PRAS8) Master 0 Priority */
#define MATRIX_PRAS8_M0PR(value) ((MATRIX_PRAS8_M0PR_Msk & ((value) << MATRIX_PRAS8_M0PR_Pos)))
#define MATRIX_PRAS8_M1PR_Pos 4
#define MATRIX_PRAS8_M1PR_Msk (0x3u << MATRIX_PRAS8_M1PR_Pos) /**< \brief (MATRIX_PRAS8) Master 1 Priority */
#define MATRIX_PRAS8_M1PR(value) ((MATRIX_PRAS8_M1PR_Msk & ((value) << MATRIX_PRAS8_M1PR_Pos)))
#define MATRIX_PRAS8_M2PR_Pos 8
#define MATRIX_PRAS8_M2PR_Msk (0x3u << MATRIX_PRAS8_M2PR_Pos) /**< \brief (MATRIX_PRAS8) Master 2 Priority */
#define MATRIX_PRAS8_M2PR(value) ((MATRIX_PRAS8_M2PR_Msk & ((value) << MATRIX_PRAS8_M2PR_Pos)))
#define MATRIX_PRAS8_M3PR_Pos 12
#define MATRIX_PRAS8_M3PR_Msk (0x3u << MATRIX_PRAS8_M3PR_Pos) /**< \brief (MATRIX_PRAS8) Master 3 Priority */
#define MATRIX_PRAS8_M3PR(value) ((MATRIX_PRAS8_M3PR_Msk & ((value) << MATRIX_PRAS8_M3PR_Pos)))
#define MATRIX_PRAS8_M4PR_Pos 16
#define MATRIX_PRAS8_M4PR_Msk (0x3u << MATRIX_PRAS8_M4PR_Pos) /**< \brief (MATRIX_PRAS8) Master 4 Priority */
#define MATRIX_PRAS8_M4PR(value) ((MATRIX_PRAS8_M4PR_Msk & ((value) << MATRIX_PRAS8_M4PR_Pos)))
#define MATRIX_PRAS8_M5PR_Pos 20
#define MATRIX_PRAS8_M5PR_Msk (0x3u << MATRIX_PRAS8_M5PR_Pos) /**< \brief (MATRIX_PRAS8) Master 5 Priority */
#define MATRIX_PRAS8_M5PR(value) ((MATRIX_PRAS8_M5PR_Msk & ((value) << MATRIX_PRAS8_M5PR_Pos)))
/* -------- MATRIX_MRCR : (MATRIX Offset: 0x0100) Master Remap Control Register -------- */
#define MATRIX_MRCR_RCB0 (0x1u << 0) /**< \brief (MATRIX_MRCR) Remap Command Bit for AHB Master 0 */
#define MATRIX_MRCR_RCB1 (0x1u << 1) /**< \brief (MATRIX_MRCR) Remap Command Bit for AHB Master 1 */
#define MATRIX_MRCR_RCB2 (0x1u << 2) /**< \brief (MATRIX_MRCR) Remap Command Bit for AHB Master 2 */
#define MATRIX_MRCR_RCB3 (0x1u << 3) /**< \brief (MATRIX_MRCR) Remap Command Bit for AHB Master 3 */
#define MATRIX_MRCR_RCB4_Pos 4
#define MATRIX_MRCR_RCB4_Msk (0x3u << MATRIX_MRCR_RCB4_Pos) /**< \brief (MATRIX_MRCR) Remap Command Bit for AHB Master 4 */
#define MATRIX_MRCR_RCB4(value) ((MATRIX_MRCR_RCB4_Msk & ((value) << MATRIX_MRCR_RCB4_Pos)))
#define MATRIX_MRCR_RCB5 (0x1u << 6) /**< \brief (MATRIX_MRCR) Remap Command Bit for AHB Master 5 */
/* -------- CCFG_SYSIO : (MATRIX Offset: 0x0114) System I/O Configuration register -------- */
#define CCFG_SYSIO_SYSIO12 (0x1u << 12) /**< \brief (CCFG_SYSIO) PC0 or ERASE Assignment */
/* -------- MATRIX_WPMR : (MATRIX Offset: 0x1E4) Write Protect Mode Register -------- */
#define MATRIX_WPMR_WPEN (0x1u << 0) /**< \brief (MATRIX_WPMR) Write Protect ENable */
#define MATRIX_WPMR_WPKEY_Pos 8
#define MATRIX_WPMR_WPKEY_Msk (0xffffffu << MATRIX_WPMR_WPKEY_Pos) /**< \brief (MATRIX_WPMR) Write Protect KEY (Write-only) */
#define MATRIX_WPMR_WPKEY(value) ((MATRIX_WPMR_WPKEY_Msk & ((value) << MATRIX_WPMR_WPKEY_Pos)))
/* -------- MATRIX_WPSR : (MATRIX Offset: 0x1E8) Write Protect Status Register -------- */
#define MATRIX_WPSR_WPVS (0x1u << 0) /**< \brief (MATRIX_WPSR) Write Protect Violation Status */
#define MATRIX_WPSR_WPVSRC_Pos 8
#define MATRIX_WPSR_WPVSRC_Msk (0xffffu << MATRIX_WPSR_WPVSRC_Pos) /**< \brief (MATRIX_WPSR) Write Protect Violation Source */

/*@}*/


#endif /* _SAM3XA_MATRIX_COMPONENT_ */
