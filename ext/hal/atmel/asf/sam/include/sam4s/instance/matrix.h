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

#ifndef _SAM4S_MATRIX_INSTANCE_
#define _SAM4S_MATRIX_INSTANCE_

/* ========== Register definition for MATRIX peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_MATRIX_MCFG                   (0x400E0200U) /**< \brief (MATRIX) Master Configuration Register */
  #define REG_MATRIX_SCFG                   (0x400E0240U) /**< \brief (MATRIX) Slave Configuration Register */
  #define REG_MATRIX_PRAS0                  (0x400E0280U) /**< \brief (MATRIX) Priority Register A for Slave 0 */
  #define REG_MATRIX_PRAS1                  (0x400E0288U) /**< \brief (MATRIX) Priority Register A for Slave 1 */
  #define REG_MATRIX_PRAS2                  (0x400E0290U) /**< \brief (MATRIX) Priority Register A for Slave 2 */
  #define REG_MATRIX_PRAS3                  (0x400E0298U) /**< \brief (MATRIX) Priority Register A for Slave 3 */
  #define REG_MATRIX_PRAS4                  (0x400E02A0U) /**< \brief (MATRIX) Priority Register A for Slave 4 */
  #define REG_CCFG_SYSIO                    (0x400E0314U) /**< \brief (MATRIX) System I/O Configuration register */
  #define REG_CCFG_SMCNFCS                  (0x400E031CU) /**< \brief (MATRIX) SMC Chip Select NAND Flash Assignment Register */
  #define REG_MATRIX_WPMR                   (0x400E03E4U) /**< \brief (MATRIX) Write Protection Mode Register */
  #define REG_MATRIX_WPSR                   (0x400E03E8U) /**< \brief (MATRIX) Write Protection Status Register */
#else
  #define REG_MATRIX_MCFG  (*(__IO uint32_t*)0x400E0200U) /**< \brief (MATRIX) Master Configuration Register */
  #define REG_MATRIX_SCFG  (*(__IO uint32_t*)0x400E0240U) /**< \brief (MATRIX) Slave Configuration Register */
  #define REG_MATRIX_PRAS0 (*(__IO uint32_t*)0x400E0280U) /**< \brief (MATRIX) Priority Register A for Slave 0 */
  #define REG_MATRIX_PRAS1 (*(__IO uint32_t*)0x400E0288U) /**< \brief (MATRIX) Priority Register A for Slave 1 */
  #define REG_MATRIX_PRAS2 (*(__IO uint32_t*)0x400E0290U) /**< \brief (MATRIX) Priority Register A for Slave 2 */
  #define REG_MATRIX_PRAS3 (*(__IO uint32_t*)0x400E0298U) /**< \brief (MATRIX) Priority Register A for Slave 3 */
  #define REG_MATRIX_PRAS4 (*(__IO uint32_t*)0x400E02A0U) /**< \brief (MATRIX) Priority Register A for Slave 4 */
  #define REG_CCFG_SYSIO   (*(__IO uint32_t*)0x400E0314U) /**< \brief (MATRIX) System I/O Configuration register */
  #define REG_CCFG_SMCNFCS (*(__IO uint32_t*)0x400E031CU) /**< \brief (MATRIX) SMC Chip Select NAND Flash Assignment Register */
  #define REG_MATRIX_WPMR  (*(__IO uint32_t*)0x400E03E4U) /**< \brief (MATRIX) Write Protection Mode Register */
  #define REG_MATRIX_WPSR  (*(__I  uint32_t*)0x400E03E8U) /**< \brief (MATRIX) Write Protection Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM4S_MATRIX_INSTANCE_ */
