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

#ifndef _SAM3XA_PIOC_INSTANCE_
#define _SAM3XA_PIOC_INSTANCE_

/* ========== Register definition for PIOC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_PIOC_PER                     (0x400E1200U) /**< \brief (PIOC) PIO Enable Register */
  #define REG_PIOC_PDR                     (0x400E1204U) /**< \brief (PIOC) PIO Disable Register */
  #define REG_PIOC_PSR                     (0x400E1208U) /**< \brief (PIOC) PIO Status Register */
  #define REG_PIOC_OER                     (0x400E1210U) /**< \brief (PIOC) Output Enable Register */
  #define REG_PIOC_ODR                     (0x400E1214U) /**< \brief (PIOC) Output Disable Register */
  #define REG_PIOC_OSR                     (0x400E1218U) /**< \brief (PIOC) Output Status Register */
  #define REG_PIOC_IFER                    (0x400E1220U) /**< \brief (PIOC) Glitch Input Filter Enable Register */
  #define REG_PIOC_IFDR                    (0x400E1224U) /**< \brief (PIOC) Glitch Input Filter Disable Register */
  #define REG_PIOC_IFSR                    (0x400E1228U) /**< \brief (PIOC) Glitch Input Filter Status Register */
  #define REG_PIOC_SODR                    (0x400E1230U) /**< \brief (PIOC) Set Output Data Register */
  #define REG_PIOC_CODR                    (0x400E1234U) /**< \brief (PIOC) Clear Output Data Register */
  #define REG_PIOC_ODSR                    (0x400E1238U) /**< \brief (PIOC) Output Data Status Register */
  #define REG_PIOC_PDSR                    (0x400E123CU) /**< \brief (PIOC) Pin Data Status Register */
  #define REG_PIOC_IER                     (0x400E1240U) /**< \brief (PIOC) Interrupt Enable Register */
  #define REG_PIOC_IDR                     (0x400E1244U) /**< \brief (PIOC) Interrupt Disable Register */
  #define REG_PIOC_IMR                     (0x400E1248U) /**< \brief (PIOC) Interrupt Mask Register */
  #define REG_PIOC_ISR                     (0x400E124CU) /**< \brief (PIOC) Interrupt Status Register */
  #define REG_PIOC_MDER                    (0x400E1250U) /**< \brief (PIOC) Multi-driver Enable Register */
  #define REG_PIOC_MDDR                    (0x400E1254U) /**< \brief (PIOC) Multi-driver Disable Register */
  #define REG_PIOC_MDSR                    (0x400E1258U) /**< \brief (PIOC) Multi-driver Status Register */
  #define REG_PIOC_PUDR                    (0x400E1260U) /**< \brief (PIOC) Pull-up Disable Register */
  #define REG_PIOC_PUER                    (0x400E1264U) /**< \brief (PIOC) Pull-up Enable Register */
  #define REG_PIOC_PUSR                    (0x400E1268U) /**< \brief (PIOC) Pad Pull-up Status Register */
  #define REG_PIOC_ABSR                    (0x400E1270U) /**< \brief (PIOC) Peripheral AB Select Register */
  #define REG_PIOC_SCIFSR                  (0x400E1280U) /**< \brief (PIOC) System Clock Glitch Input Filter Select Register */
  #define REG_PIOC_DIFSR                   (0x400E1284U) /**< \brief (PIOC) Debouncing Input Filter Select Register */
  #define REG_PIOC_IFDGSR                  (0x400E1288U) /**< \brief (PIOC) Glitch or Debouncing Input Filter Clock Selection Status Register */
  #define REG_PIOC_SCDR                    (0x400E128CU) /**< \brief (PIOC) Slow Clock Divider Debouncing Register */
  #define REG_PIOC_OWER                    (0x400E12A0U) /**< \brief (PIOC) Output Write Enable */
  #define REG_PIOC_OWDR                    (0x400E12A4U) /**< \brief (PIOC) Output Write Disable */
  #define REG_PIOC_OWSR                    (0x400E12A8U) /**< \brief (PIOC) Output Write Status Register */
  #define REG_PIOC_AIMER                   (0x400E12B0U) /**< \brief (PIOC) Additional Interrupt Modes Enable Register */
  #define REG_PIOC_AIMDR                   (0x400E12B4U) /**< \brief (PIOC) Additional Interrupt Modes Disables Register */
  #define REG_PIOC_AIMMR                   (0x400E12B8U) /**< \brief (PIOC) Additional Interrupt Modes Mask Register */
  #define REG_PIOC_ESR                     (0x400E12C0U) /**< \brief (PIOC) Edge Select Register */
  #define REG_PIOC_LSR                     (0x400E12C4U) /**< \brief (PIOC) Level Select Register */
  #define REG_PIOC_ELSR                    (0x400E12C8U) /**< \brief (PIOC) Edge/Level Status Register */
  #define REG_PIOC_FELLSR                  (0x400E12D0U) /**< \brief (PIOC) Falling Edge/Low Level Select Register */
  #define REG_PIOC_REHLSR                  (0x400E12D4U) /**< \brief (PIOC) Rising Edge/ High Level Select Register */
  #define REG_PIOC_FRLHSR                  (0x400E12D8U) /**< \brief (PIOC) Fall/Rise - Low/High Status Register */
  #define REG_PIOC_LOCKSR                  (0x400E12E0U) /**< \brief (PIOC) Lock Status */
  #define REG_PIOC_WPMR                    (0x400E12E4U) /**< \brief (PIOC) Write Protect Mode Register */
  #define REG_PIOC_WPSR                    (0x400E12E8U) /**< \brief (PIOC) Write Protect Status Register */
#else
  #define REG_PIOC_PER    (*(__O  uint32_t*)0x400E1200U) /**< \brief (PIOC) PIO Enable Register */
  #define REG_PIOC_PDR    (*(__O  uint32_t*)0x400E1204U) /**< \brief (PIOC) PIO Disable Register */
  #define REG_PIOC_PSR    (*(__I  uint32_t*)0x400E1208U) /**< \brief (PIOC) PIO Status Register */
  #define REG_PIOC_OER    (*(__O  uint32_t*)0x400E1210U) /**< \brief (PIOC) Output Enable Register */
  #define REG_PIOC_ODR    (*(__O  uint32_t*)0x400E1214U) /**< \brief (PIOC) Output Disable Register */
  #define REG_PIOC_OSR    (*(__I  uint32_t*)0x400E1218U) /**< \brief (PIOC) Output Status Register */
  #define REG_PIOC_IFER   (*(__O  uint32_t*)0x400E1220U) /**< \brief (PIOC) Glitch Input Filter Enable Register */
  #define REG_PIOC_IFDR   (*(__O  uint32_t*)0x400E1224U) /**< \brief (PIOC) Glitch Input Filter Disable Register */
  #define REG_PIOC_IFSR   (*(__I  uint32_t*)0x400E1228U) /**< \brief (PIOC) Glitch Input Filter Status Register */
  #define REG_PIOC_SODR   (*(__O  uint32_t*)0x400E1230U) /**< \brief (PIOC) Set Output Data Register */
  #define REG_PIOC_CODR   (*(__O  uint32_t*)0x400E1234U) /**< \brief (PIOC) Clear Output Data Register */
  #define REG_PIOC_ODSR   (*(__IO uint32_t*)0x400E1238U) /**< \brief (PIOC) Output Data Status Register */
  #define REG_PIOC_PDSR   (*(__I  uint32_t*)0x400E123CU) /**< \brief (PIOC) Pin Data Status Register */
  #define REG_PIOC_IER    (*(__O  uint32_t*)0x400E1240U) /**< \brief (PIOC) Interrupt Enable Register */
  #define REG_PIOC_IDR    (*(__O  uint32_t*)0x400E1244U) /**< \brief (PIOC) Interrupt Disable Register */
  #define REG_PIOC_IMR    (*(__I  uint32_t*)0x400E1248U) /**< \brief (PIOC) Interrupt Mask Register */
  #define REG_PIOC_ISR    (*(__I  uint32_t*)0x400E124CU) /**< \brief (PIOC) Interrupt Status Register */
  #define REG_PIOC_MDER   (*(__O  uint32_t*)0x400E1250U) /**< \brief (PIOC) Multi-driver Enable Register */
  #define REG_PIOC_MDDR   (*(__O  uint32_t*)0x400E1254U) /**< \brief (PIOC) Multi-driver Disable Register */
  #define REG_PIOC_MDSR   (*(__I  uint32_t*)0x400E1258U) /**< \brief (PIOC) Multi-driver Status Register */
  #define REG_PIOC_PUDR   (*(__O  uint32_t*)0x400E1260U) /**< \brief (PIOC) Pull-up Disable Register */
  #define REG_PIOC_PUER   (*(__O  uint32_t*)0x400E1264U) /**< \brief (PIOC) Pull-up Enable Register */
  #define REG_PIOC_PUSR   (*(__I  uint32_t*)0x400E1268U) /**< \brief (PIOC) Pad Pull-up Status Register */
  #define REG_PIOC_ABSR   (*(__IO uint32_t*)0x400E1270U) /**< \brief (PIOC) Peripheral AB Select Register */
  #define REG_PIOC_SCIFSR (*(__O  uint32_t*)0x400E1280U) /**< \brief (PIOC) System Clock Glitch Input Filter Select Register */
  #define REG_PIOC_DIFSR  (*(__O  uint32_t*)0x400E1284U) /**< \brief (PIOC) Debouncing Input Filter Select Register */
  #define REG_PIOC_IFDGSR (*(__I  uint32_t*)0x400E1288U) /**< \brief (PIOC) Glitch or Debouncing Input Filter Clock Selection Status Register */
  #define REG_PIOC_SCDR   (*(__IO uint32_t*)0x400E128CU) /**< \brief (PIOC) Slow Clock Divider Debouncing Register */
  #define REG_PIOC_OWER   (*(__O  uint32_t*)0x400E12A0U) /**< \brief (PIOC) Output Write Enable */
  #define REG_PIOC_OWDR   (*(__O  uint32_t*)0x400E12A4U) /**< \brief (PIOC) Output Write Disable */
  #define REG_PIOC_OWSR   (*(__I  uint32_t*)0x400E12A8U) /**< \brief (PIOC) Output Write Status Register */
  #define REG_PIOC_AIMER  (*(__O  uint32_t*)0x400E12B0U) /**< \brief (PIOC) Additional Interrupt Modes Enable Register */
  #define REG_PIOC_AIMDR  (*(__O  uint32_t*)0x400E12B4U) /**< \brief (PIOC) Additional Interrupt Modes Disables Register */
  #define REG_PIOC_AIMMR  (*(__I  uint32_t*)0x400E12B8U) /**< \brief (PIOC) Additional Interrupt Modes Mask Register */
  #define REG_PIOC_ESR    (*(__O  uint32_t*)0x400E12C0U) /**< \brief (PIOC) Edge Select Register */
  #define REG_PIOC_LSR    (*(__O  uint32_t*)0x400E12C4U) /**< \brief (PIOC) Level Select Register */
  #define REG_PIOC_ELSR   (*(__I  uint32_t*)0x400E12C8U) /**< \brief (PIOC) Edge/Level Status Register */
  #define REG_PIOC_FELLSR (*(__O  uint32_t*)0x400E12D0U) /**< \brief (PIOC) Falling Edge/Low Level Select Register */
  #define REG_PIOC_REHLSR (*(__O  uint32_t*)0x400E12D4U) /**< \brief (PIOC) Rising Edge/ High Level Select Register */
  #define REG_PIOC_FRLHSR (*(__I  uint32_t*)0x400E12D8U) /**< \brief (PIOC) Fall/Rise - Low/High Status Register */
  #define REG_PIOC_LOCKSR (*(__I  uint32_t*)0x400E12E0U) /**< \brief (PIOC) Lock Status */
  #define REG_PIOC_WPMR   (*(__IO uint32_t*)0x400E12E4U) /**< \brief (PIOC) Write Protect Mode Register */
  #define REG_PIOC_WPSR   (*(__I  uint32_t*)0x400E12E8U) /**< \brief (PIOC) Write Protect Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_PIOC_INSTANCE_ */
