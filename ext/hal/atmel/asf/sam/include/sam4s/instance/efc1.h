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

#ifndef _SAM4S_EFC1_INSTANCE_
#define _SAM4S_EFC1_INSTANCE_

/* ========== Register definition for EFC1 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_EFC1_FMR                  (0x400E0C00U) /**< \brief (EFC1) EEFC Flash Mode Register */
  #define REG_EFC1_FCR                  (0x400E0C04U) /**< \brief (EFC1) EEFC Flash Command Register */
  #define REG_EFC1_FSR                  (0x400E0C08U) /**< \brief (EFC1) EEFC Flash Status Register */
  #define REG_EFC1_FRR                  (0x400E0C0CU) /**< \brief (EFC1) EEFC Flash Result Register */
#else
  #define REG_EFC1_FMR (*(__IO uint32_t*)0x400E0C00U) /**< \brief (EFC1) EEFC Flash Mode Register */
  #define REG_EFC1_FCR (*(__O  uint32_t*)0x400E0C04U) /**< \brief (EFC1) EEFC Flash Command Register */
  #define REG_EFC1_FSR (*(__I  uint32_t*)0x400E0C08U) /**< \brief (EFC1) EEFC Flash Status Register */
  #define REG_EFC1_FRR (*(__I  uint32_t*)0x400E0C0CU) /**< \brief (EFC1) EEFC Flash Result Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM4S_EFC1_INSTANCE_ */
