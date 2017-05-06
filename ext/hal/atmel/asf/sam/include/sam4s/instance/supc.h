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

#ifndef _SAM4S_SUPC_INSTANCE_
#define _SAM4S_SUPC_INSTANCE_

/* ========== Register definition for SUPC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_SUPC_CR                    (0x400E1410U) /**< \brief (SUPC) Supply Controller Control Register */
  #define REG_SUPC_SMMR                  (0x400E1414U) /**< \brief (SUPC) Supply Controller Supply Monitor Mode Register */
  #define REG_SUPC_MR                    (0x400E1418U) /**< \brief (SUPC) Supply Controller Mode Register */
  #define REG_SUPC_WUMR                  (0x400E141CU) /**< \brief (SUPC) Supply Controller Wake-up Mode Register */
  #define REG_SUPC_WUIR                  (0x400E1420U) /**< \brief (SUPC) Supply Controller Wake-up Inputs Register */
  #define REG_SUPC_SR                    (0x400E1424U) /**< \brief (SUPC) Supply Controller Status Register */
#else
  #define REG_SUPC_CR   (*(__O  uint32_t*)0x400E1410U) /**< \brief (SUPC) Supply Controller Control Register */
  #define REG_SUPC_SMMR (*(__IO uint32_t*)0x400E1414U) /**< \brief (SUPC) Supply Controller Supply Monitor Mode Register */
  #define REG_SUPC_MR   (*(__IO uint32_t*)0x400E1418U) /**< \brief (SUPC) Supply Controller Mode Register */
  #define REG_SUPC_WUMR (*(__IO uint32_t*)0x400E141CU) /**< \brief (SUPC) Supply Controller Wake-up Mode Register */
  #define REG_SUPC_WUIR (*(__IO uint32_t*)0x400E1420U) /**< \brief (SUPC) Supply Controller Wake-up Inputs Register */
  #define REG_SUPC_SR   (*(__I  uint32_t*)0x400E1424U) /**< \brief (SUPC) Supply Controller Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM4S_SUPC_INSTANCE_ */
