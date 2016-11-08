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

#ifndef _SAM3XA_PIOF_INSTANCE_
#define _SAM3XA_PIOF_INSTANCE_

/* ========== Register definition for PIOF peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_PIOF_PER                     (0x400E1800U) /**< \brief (PIOF) PIO Enable Register */
  #define REG_PIOF_PDR                     (0x400E1804U) /**< \brief (PIOF) PIO Disable Register */
  #define REG_PIOF_PSR                     (0x400E1808U) /**< \brief (PIOF) PIO Status Register */
  #define REG_PIOF_OER                     (0x400E1810U) /**< \brief (PIOF) Output Enable Register */
  #define REG_PIOF_ODR                     (0x400E1814U) /**< \brief (PIOF) Output Disable Register */
  #define REG_PIOF_OSR                     (0x400E1818U) /**< \brief (PIOF) Output Status Register */
  #define REG_PIOF_IFER                    (0x400E1820U) /**< \brief (PIOF) Glitch Input Filter Enable Register */
  #define REG_PIOF_IFDR                    (0x400E1824U) /**< \brief (PIOF) Glitch Input Filter Disable Register */
  #define REG_PIOF_IFSR                    (0x400E1828U) /**< \brief (PIOF) Glitch Input Filter Status Register */
  #define REG_PIOF_SODR                    (0x400E1830U) /**< \brief (PIOF) Set Output Data Register */
  #define REG_PIOF_CODR                    (0x400E1834U) /**< \brief (PIOF) Clear Output Data Register */
  #define REG_PIOF_ODSR                    (0x400E1838U) /**< \brief (PIOF) Output Data Status Register */
  #define REG_PIOF_PDSR                    (0x400E183CU) /**< \brief (PIOF) Pin Data Status Register */
  #define REG_PIOF_IER                     (0x400E1840U) /**< \brief (PIOF) Interrupt Enable Register */
  #define REG_PIOF_IDR                     (0x400E1844U) /**< \brief (PIOF) Interrupt Disable Register */
  #define REG_PIOF_IMR                     (0x400E1848U) /**< \brief (PIOF) Interrupt Mask Register */
  #define REG_PIOF_ISR                     (0x400E184CU) /**< \brief (PIOF) Interrupt Status Register */
  #define REG_PIOF_MDER                    (0x400E1850U) /**< \brief (PIOF) Multi-driver Enable Register */
  #define REG_PIOF_MDDR                    (0x400E1854U) /**< \brief (PIOF) Multi-driver Disable Register */
  #define REG_PIOF_MDSR                    (0x400E1858U) /**< \brief (PIOF) Multi-driver Status Register */
  #define REG_PIOF_PUDR                    (0x400E1860U) /**< \brief (PIOF) Pull-up Disable Register */
  #define REG_PIOF_PUER                    (0x400E1864U) /**< \brief (PIOF) Pull-up Enable Register */
  #define REG_PIOF_PUSR                    (0x400E1868U) /**< \brief (PIOF) Pad Pull-up Status Register */
  #define REG_PIOF_ABSR                    (0x400E1870U) /**< \brief (PIOF) Peripheral AB Select Register */
  #define REG_PIOF_SCIFSR                  (0x400E1880U) /**< \brief (PIOF) System Clock Glitch Input Filter Select Register */
  #define REG_PIOF_DIFSR                   (0x400E1884U) /**< \brief (PIOF) Debouncing Input Filter Select Register */
  #define REG_PIOF_IFDGSR                  (0x400E1888U) /**< \brief (PIOF) Glitch or Debouncing Input Filter Clock Selection Status Register */
  #define REG_PIOF_SCDR                    (0x400E188CU) /**< \brief (PIOF) Slow Clock Divider Debouncing Register */
  #define REG_PIOF_OWER                    (0x400E18A0U) /**< \brief (PIOF) Output Write Enable */
  #define REG_PIOF_OWDR                    (0x400E18A4U) /**< \brief (PIOF) Output Write Disable */
  #define REG_PIOF_OWSR                    (0x400E18A8U) /**< \brief (PIOF) Output Write Status Register */
  #define REG_PIOF_AIMER                   (0x400E18B0U) /**< \brief (PIOF) Additional Interrupt Modes Enable Register */
  #define REG_PIOF_AIMDR                   (0x400E18B4U) /**< \brief (PIOF) Additional Interrupt Modes Disables Register */
  #define REG_PIOF_AIMMR                   (0x400E18B8U) /**< \brief (PIOF) Additional Interrupt Modes Mask Register */
  #define REG_PIOF_ESR                     (0x400E18C0U) /**< \brief (PIOF) Edge Select Register */
  #define REG_PIOF_LSR                     (0x400E18C4U) /**< \brief (PIOF) Level Select Register */
  #define REG_PIOF_ELSR                    (0x400E18C8U) /**< \brief (PIOF) Edge/Level Status Register */
  #define REG_PIOF_FELLSR                  (0x400E18D0U) /**< \brief (PIOF) Falling Edge/Low Level Select Register */
  #define REG_PIOF_REHLSR                  (0x400E18D4U) /**< \brief (PIOF) Rising Edge/ High Level Select Register */
  #define REG_PIOF_FRLHSR                  (0x400E18D8U) /**< \brief (PIOF) Fall/Rise - Low/High Status Register */
  #define REG_PIOF_LOCKSR                  (0x400E18E0U) /**< \brief (PIOF) Lock Status */
  #define REG_PIOF_WPMR                    (0x400E18E4U) /**< \brief (PIOF) Write Protect Mode Register */
  #define REG_PIOF_WPSR                    (0x400E18E8U) /**< \brief (PIOF) Write Protect Status Register */
#else
  #define REG_PIOF_PER    (*(__O  uint32_t*)0x400E1800U) /**< \brief (PIOF) PIO Enable Register */
  #define REG_PIOF_PDR    (*(__O  uint32_t*)0x400E1804U) /**< \brief (PIOF) PIO Disable Register */
  #define REG_PIOF_PSR    (*(__I  uint32_t*)0x400E1808U) /**< \brief (PIOF) PIO Status Register */
  #define REG_PIOF_OER    (*(__O  uint32_t*)0x400E1810U) /**< \brief (PIOF) Output Enable Register */
  #define REG_PIOF_ODR    (*(__O  uint32_t*)0x400E1814U) /**< \brief (PIOF) Output Disable Register */
  #define REG_PIOF_OSR    (*(__I  uint32_t*)0x400E1818U) /**< \brief (PIOF) Output Status Register */
  #define REG_PIOF_IFER   (*(__O  uint32_t*)0x400E1820U) /**< \brief (PIOF) Glitch Input Filter Enable Register */
  #define REG_PIOF_IFDR   (*(__O  uint32_t*)0x400E1824U) /**< \brief (PIOF) Glitch Input Filter Disable Register */
  #define REG_PIOF_IFSR   (*(__I  uint32_t*)0x400E1828U) /**< \brief (PIOF) Glitch Input Filter Status Register */
  #define REG_PIOF_SODR   (*(__O  uint32_t*)0x400E1830U) /**< \brief (PIOF) Set Output Data Register */
  #define REG_PIOF_CODR   (*(__O  uint32_t*)0x400E1834U) /**< \brief (PIOF) Clear Output Data Register */
  #define REG_PIOF_ODSR   (*(__IO uint32_t*)0x400E1838U) /**< \brief (PIOF) Output Data Status Register */
  #define REG_PIOF_PDSR   (*(__I  uint32_t*)0x400E183CU) /**< \brief (PIOF) Pin Data Status Register */
  #define REG_PIOF_IER    (*(__O  uint32_t*)0x400E1840U) /**< \brief (PIOF) Interrupt Enable Register */
  #define REG_PIOF_IDR    (*(__O  uint32_t*)0x400E1844U) /**< \brief (PIOF) Interrupt Disable Register */
  #define REG_PIOF_IMR    (*(__I  uint32_t*)0x400E1848U) /**< \brief (PIOF) Interrupt Mask Register */
  #define REG_PIOF_ISR    (*(__I  uint32_t*)0x400E184CU) /**< \brief (PIOF) Interrupt Status Register */
  #define REG_PIOF_MDER   (*(__O  uint32_t*)0x400E1850U) /**< \brief (PIOF) Multi-driver Enable Register */
  #define REG_PIOF_MDDR   (*(__O  uint32_t*)0x400E1854U) /**< \brief (PIOF) Multi-driver Disable Register */
  #define REG_PIOF_MDSR   (*(__I  uint32_t*)0x400E1858U) /**< \brief (PIOF) Multi-driver Status Register */
  #define REG_PIOF_PUDR   (*(__O  uint32_t*)0x400E1860U) /**< \brief (PIOF) Pull-up Disable Register */
  #define REG_PIOF_PUER   (*(__O  uint32_t*)0x400E1864U) /**< \brief (PIOF) Pull-up Enable Register */
  #define REG_PIOF_PUSR   (*(__I  uint32_t*)0x400E1868U) /**< \brief (PIOF) Pad Pull-up Status Register */
  #define REG_PIOF_ABSR   (*(__IO uint32_t*)0x400E1870U) /**< \brief (PIOF) Peripheral AB Select Register */
  #define REG_PIOF_SCIFSR (*(__O  uint32_t*)0x400E1880U) /**< \brief (PIOF) System Clock Glitch Input Filter Select Register */
  #define REG_PIOF_DIFSR  (*(__O  uint32_t*)0x400E1884U) /**< \brief (PIOF) Debouncing Input Filter Select Register */
  #define REG_PIOF_IFDGSR (*(__I  uint32_t*)0x400E1888U) /**< \brief (PIOF) Glitch or Debouncing Input Filter Clock Selection Status Register */
  #define REG_PIOF_SCDR   (*(__IO uint32_t*)0x400E188CU) /**< \brief (PIOF) Slow Clock Divider Debouncing Register */
  #define REG_PIOF_OWER   (*(__O  uint32_t*)0x400E18A0U) /**< \brief (PIOF) Output Write Enable */
  #define REG_PIOF_OWDR   (*(__O  uint32_t*)0x400E18A4U) /**< \brief (PIOF) Output Write Disable */
  #define REG_PIOF_OWSR   (*(__I  uint32_t*)0x400E18A8U) /**< \brief (PIOF) Output Write Status Register */
  #define REG_PIOF_AIMER  (*(__O  uint32_t*)0x400E18B0U) /**< \brief (PIOF) Additional Interrupt Modes Enable Register */
  #define REG_PIOF_AIMDR  (*(__O  uint32_t*)0x400E18B4U) /**< \brief (PIOF) Additional Interrupt Modes Disables Register */
  #define REG_PIOF_AIMMR  (*(__I  uint32_t*)0x400E18B8U) /**< \brief (PIOF) Additional Interrupt Modes Mask Register */
  #define REG_PIOF_ESR    (*(__O  uint32_t*)0x400E18C0U) /**< \brief (PIOF) Edge Select Register */
  #define REG_PIOF_LSR    (*(__O  uint32_t*)0x400E18C4U) /**< \brief (PIOF) Level Select Register */
  #define REG_PIOF_ELSR   (*(__I  uint32_t*)0x400E18C8U) /**< \brief (PIOF) Edge/Level Status Register */
  #define REG_PIOF_FELLSR (*(__O  uint32_t*)0x400E18D0U) /**< \brief (PIOF) Falling Edge/Low Level Select Register */
  #define REG_PIOF_REHLSR (*(__O  uint32_t*)0x400E18D4U) /**< \brief (PIOF) Rising Edge/ High Level Select Register */
  #define REG_PIOF_FRLHSR (*(__I  uint32_t*)0x400E18D8U) /**< \brief (PIOF) Fall/Rise - Low/High Status Register */
  #define REG_PIOF_LOCKSR (*(__I  uint32_t*)0x400E18E0U) /**< \brief (PIOF) Lock Status */
  #define REG_PIOF_WPMR   (*(__IO uint32_t*)0x400E18E4U) /**< \brief (PIOF) Write Protect Mode Register */
  #define REG_PIOF_WPSR   (*(__I  uint32_t*)0x400E18E8U) /**< \brief (PIOF) Write Protect Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_PIOF_INSTANCE_ */
