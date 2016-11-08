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

#ifndef _SAM3XA_PIOB_INSTANCE_
#define _SAM3XA_PIOB_INSTANCE_

/* ========== Register definition for PIOB peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_PIOB_PER                     (0x400E1000U) /**< \brief (PIOB) PIO Enable Register */
  #define REG_PIOB_PDR                     (0x400E1004U) /**< \brief (PIOB) PIO Disable Register */
  #define REG_PIOB_PSR                     (0x400E1008U) /**< \brief (PIOB) PIO Status Register */
  #define REG_PIOB_OER                     (0x400E1010U) /**< \brief (PIOB) Output Enable Register */
  #define REG_PIOB_ODR                     (0x400E1014U) /**< \brief (PIOB) Output Disable Register */
  #define REG_PIOB_OSR                     (0x400E1018U) /**< \brief (PIOB) Output Status Register */
  #define REG_PIOB_IFER                    (0x400E1020U) /**< \brief (PIOB) Glitch Input Filter Enable Register */
  #define REG_PIOB_IFDR                    (0x400E1024U) /**< \brief (PIOB) Glitch Input Filter Disable Register */
  #define REG_PIOB_IFSR                    (0x400E1028U) /**< \brief (PIOB) Glitch Input Filter Status Register */
  #define REG_PIOB_SODR                    (0x400E1030U) /**< \brief (PIOB) Set Output Data Register */
  #define REG_PIOB_CODR                    (0x400E1034U) /**< \brief (PIOB) Clear Output Data Register */
  #define REG_PIOB_ODSR                    (0x400E1038U) /**< \brief (PIOB) Output Data Status Register */
  #define REG_PIOB_PDSR                    (0x400E103CU) /**< \brief (PIOB) Pin Data Status Register */
  #define REG_PIOB_IER                     (0x400E1040U) /**< \brief (PIOB) Interrupt Enable Register */
  #define REG_PIOB_IDR                     (0x400E1044U) /**< \brief (PIOB) Interrupt Disable Register */
  #define REG_PIOB_IMR                     (0x400E1048U) /**< \brief (PIOB) Interrupt Mask Register */
  #define REG_PIOB_ISR                     (0x400E104CU) /**< \brief (PIOB) Interrupt Status Register */
  #define REG_PIOB_MDER                    (0x400E1050U) /**< \brief (PIOB) Multi-driver Enable Register */
  #define REG_PIOB_MDDR                    (0x400E1054U) /**< \brief (PIOB) Multi-driver Disable Register */
  #define REG_PIOB_MDSR                    (0x400E1058U) /**< \brief (PIOB) Multi-driver Status Register */
  #define REG_PIOB_PUDR                    (0x400E1060U) /**< \brief (PIOB) Pull-up Disable Register */
  #define REG_PIOB_PUER                    (0x400E1064U) /**< \brief (PIOB) Pull-up Enable Register */
  #define REG_PIOB_PUSR                    (0x400E1068U) /**< \brief (PIOB) Pad Pull-up Status Register */
  #define REG_PIOB_ABSR                    (0x400E1070U) /**< \brief (PIOB) Peripheral AB Select Register */
  #define REG_PIOB_SCIFSR                  (0x400E1080U) /**< \brief (PIOB) System Clock Glitch Input Filter Select Register */
  #define REG_PIOB_DIFSR                   (0x400E1084U) /**< \brief (PIOB) Debouncing Input Filter Select Register */
  #define REG_PIOB_IFDGSR                  (0x400E1088U) /**< \brief (PIOB) Glitch or Debouncing Input Filter Clock Selection Status Register */
  #define REG_PIOB_SCDR                    (0x400E108CU) /**< \brief (PIOB) Slow Clock Divider Debouncing Register */
  #define REG_PIOB_OWER                    (0x400E10A0U) /**< \brief (PIOB) Output Write Enable */
  #define REG_PIOB_OWDR                    (0x400E10A4U) /**< \brief (PIOB) Output Write Disable */
  #define REG_PIOB_OWSR                    (0x400E10A8U) /**< \brief (PIOB) Output Write Status Register */
  #define REG_PIOB_AIMER                   (0x400E10B0U) /**< \brief (PIOB) Additional Interrupt Modes Enable Register */
  #define REG_PIOB_AIMDR                   (0x400E10B4U) /**< \brief (PIOB) Additional Interrupt Modes Disables Register */
  #define REG_PIOB_AIMMR                   (0x400E10B8U) /**< \brief (PIOB) Additional Interrupt Modes Mask Register */
  #define REG_PIOB_ESR                     (0x400E10C0U) /**< \brief (PIOB) Edge Select Register */
  #define REG_PIOB_LSR                     (0x400E10C4U) /**< \brief (PIOB) Level Select Register */
  #define REG_PIOB_ELSR                    (0x400E10C8U) /**< \brief (PIOB) Edge/Level Status Register */
  #define REG_PIOB_FELLSR                  (0x400E10D0U) /**< \brief (PIOB) Falling Edge/Low Level Select Register */
  #define REG_PIOB_REHLSR                  (0x400E10D4U) /**< \brief (PIOB) Rising Edge/ High Level Select Register */
  #define REG_PIOB_FRLHSR                  (0x400E10D8U) /**< \brief (PIOB) Fall/Rise - Low/High Status Register */
  #define REG_PIOB_LOCKSR                  (0x400E10E0U) /**< \brief (PIOB) Lock Status */
  #define REG_PIOB_WPMR                    (0x400E10E4U) /**< \brief (PIOB) Write Protect Mode Register */
  #define REG_PIOB_WPSR                    (0x400E10E8U) /**< \brief (PIOB) Write Protect Status Register */
#else
  #define REG_PIOB_PER    (*(__O  uint32_t*)0x400E1000U) /**< \brief (PIOB) PIO Enable Register */
  #define REG_PIOB_PDR    (*(__O  uint32_t*)0x400E1004U) /**< \brief (PIOB) PIO Disable Register */
  #define REG_PIOB_PSR    (*(__I  uint32_t*)0x400E1008U) /**< \brief (PIOB) PIO Status Register */
  #define REG_PIOB_OER    (*(__O  uint32_t*)0x400E1010U) /**< \brief (PIOB) Output Enable Register */
  #define REG_PIOB_ODR    (*(__O  uint32_t*)0x400E1014U) /**< \brief (PIOB) Output Disable Register */
  #define REG_PIOB_OSR    (*(__I  uint32_t*)0x400E1018U) /**< \brief (PIOB) Output Status Register */
  #define REG_PIOB_IFER   (*(__O  uint32_t*)0x400E1020U) /**< \brief (PIOB) Glitch Input Filter Enable Register */
  #define REG_PIOB_IFDR   (*(__O  uint32_t*)0x400E1024U) /**< \brief (PIOB) Glitch Input Filter Disable Register */
  #define REG_PIOB_IFSR   (*(__I  uint32_t*)0x400E1028U) /**< \brief (PIOB) Glitch Input Filter Status Register */
  #define REG_PIOB_SODR   (*(__O  uint32_t*)0x400E1030U) /**< \brief (PIOB) Set Output Data Register */
  #define REG_PIOB_CODR   (*(__O  uint32_t*)0x400E1034U) /**< \brief (PIOB) Clear Output Data Register */
  #define REG_PIOB_ODSR   (*(__IO uint32_t*)0x400E1038U) /**< \brief (PIOB) Output Data Status Register */
  #define REG_PIOB_PDSR   (*(__I  uint32_t*)0x400E103CU) /**< \brief (PIOB) Pin Data Status Register */
  #define REG_PIOB_IER    (*(__O  uint32_t*)0x400E1040U) /**< \brief (PIOB) Interrupt Enable Register */
  #define REG_PIOB_IDR    (*(__O  uint32_t*)0x400E1044U) /**< \brief (PIOB) Interrupt Disable Register */
  #define REG_PIOB_IMR    (*(__I  uint32_t*)0x400E1048U) /**< \brief (PIOB) Interrupt Mask Register */
  #define REG_PIOB_ISR    (*(__I  uint32_t*)0x400E104CU) /**< \brief (PIOB) Interrupt Status Register */
  #define REG_PIOB_MDER   (*(__O  uint32_t*)0x400E1050U) /**< \brief (PIOB) Multi-driver Enable Register */
  #define REG_PIOB_MDDR   (*(__O  uint32_t*)0x400E1054U) /**< \brief (PIOB) Multi-driver Disable Register */
  #define REG_PIOB_MDSR   (*(__I  uint32_t*)0x400E1058U) /**< \brief (PIOB) Multi-driver Status Register */
  #define REG_PIOB_PUDR   (*(__O  uint32_t*)0x400E1060U) /**< \brief (PIOB) Pull-up Disable Register */
  #define REG_PIOB_PUER   (*(__O  uint32_t*)0x400E1064U) /**< \brief (PIOB) Pull-up Enable Register */
  #define REG_PIOB_PUSR   (*(__I  uint32_t*)0x400E1068U) /**< \brief (PIOB) Pad Pull-up Status Register */
  #define REG_PIOB_ABSR   (*(__IO uint32_t*)0x400E1070U) /**< \brief (PIOB) Peripheral AB Select Register */
  #define REG_PIOB_SCIFSR (*(__O  uint32_t*)0x400E1080U) /**< \brief (PIOB) System Clock Glitch Input Filter Select Register */
  #define REG_PIOB_DIFSR  (*(__O  uint32_t*)0x400E1084U) /**< \brief (PIOB) Debouncing Input Filter Select Register */
  #define REG_PIOB_IFDGSR (*(__I  uint32_t*)0x400E1088U) /**< \brief (PIOB) Glitch or Debouncing Input Filter Clock Selection Status Register */
  #define REG_PIOB_SCDR   (*(__IO uint32_t*)0x400E108CU) /**< \brief (PIOB) Slow Clock Divider Debouncing Register */
  #define REG_PIOB_OWER   (*(__O  uint32_t*)0x400E10A0U) /**< \brief (PIOB) Output Write Enable */
  #define REG_PIOB_OWDR   (*(__O  uint32_t*)0x400E10A4U) /**< \brief (PIOB) Output Write Disable */
  #define REG_PIOB_OWSR   (*(__I  uint32_t*)0x400E10A8U) /**< \brief (PIOB) Output Write Status Register */
  #define REG_PIOB_AIMER  (*(__O  uint32_t*)0x400E10B0U) /**< \brief (PIOB) Additional Interrupt Modes Enable Register */
  #define REG_PIOB_AIMDR  (*(__O  uint32_t*)0x400E10B4U) /**< \brief (PIOB) Additional Interrupt Modes Disables Register */
  #define REG_PIOB_AIMMR  (*(__I  uint32_t*)0x400E10B8U) /**< \brief (PIOB) Additional Interrupt Modes Mask Register */
  #define REG_PIOB_ESR    (*(__O  uint32_t*)0x400E10C0U) /**< \brief (PIOB) Edge Select Register */
  #define REG_PIOB_LSR    (*(__O  uint32_t*)0x400E10C4U) /**< \brief (PIOB) Level Select Register */
  #define REG_PIOB_ELSR   (*(__I  uint32_t*)0x400E10C8U) /**< \brief (PIOB) Edge/Level Status Register */
  #define REG_PIOB_FELLSR (*(__O  uint32_t*)0x400E10D0U) /**< \brief (PIOB) Falling Edge/Low Level Select Register */
  #define REG_PIOB_REHLSR (*(__O  uint32_t*)0x400E10D4U) /**< \brief (PIOB) Rising Edge/ High Level Select Register */
  #define REG_PIOB_FRLHSR (*(__I  uint32_t*)0x400E10D8U) /**< \brief (PIOB) Fall/Rise - Low/High Status Register */
  #define REG_PIOB_LOCKSR (*(__I  uint32_t*)0x400E10E0U) /**< \brief (PIOB) Lock Status */
  #define REG_PIOB_WPMR   (*(__IO uint32_t*)0x400E10E4U) /**< \brief (PIOB) Write Protect Mode Register */
  #define REG_PIOB_WPSR   (*(__I  uint32_t*)0x400E10E8U) /**< \brief (PIOB) Write Protect Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_PIOB_INSTANCE_ */
