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

#ifndef _SAM4S_PMC_INSTANCE_
#define _SAM4S_PMC_INSTANCE_

/* ========== Register definition for PMC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_PMC_SCER                    (0x400E0400U) /**< \brief (PMC) System Clock Enable Register */
  #define REG_PMC_SCDR                    (0x400E0404U) /**< \brief (PMC) System Clock Disable Register */
  #define REG_PMC_SCSR                    (0x400E0408U) /**< \brief (PMC) System Clock Status Register */
  #define REG_PMC_PCER0                   (0x400E0410U) /**< \brief (PMC) Peripheral Clock Enable Register 0 */
  #define REG_PMC_PCDR0                   (0x400E0414U) /**< \brief (PMC) Peripheral Clock Disable Register 0 */
  #define REG_PMC_PCSR0                   (0x400E0418U) /**< \brief (PMC) Peripheral Clock Status Register 0 */
  #define REG_CKGR_MOR                    (0x400E0420U) /**< \brief (PMC) Main Oscillator Register */
  #define REG_CKGR_MCFR                   (0x400E0424U) /**< \brief (PMC) Main Clock Frequency Register */
  #define REG_CKGR_PLLAR                  (0x400E0428U) /**< \brief (PMC) PLLA Register */
  #define REG_CKGR_PLLBR                  (0x400E042CU) /**< \brief (PMC) PLLB Register */
  #define REG_PMC_MCKR                    (0x400E0430U) /**< \brief (PMC) Master Clock Register */
  #define REG_PMC_USB                     (0x400E0438U) /**< \brief (PMC) USB Clock Register */
  #define REG_PMC_PCK                     (0x400E0440U) /**< \brief (PMC) Programmable Clock 0 Register */
  #define REG_PMC_IER                     (0x400E0460U) /**< \brief (PMC) Interrupt Enable Register */
  #define REG_PMC_IDR                     (0x400E0464U) /**< \brief (PMC) Interrupt Disable Register */
  #define REG_PMC_SR                      (0x400E0468U) /**< \brief (PMC) Status Register */
  #define REG_PMC_IMR                     (0x400E046CU) /**< \brief (PMC) Interrupt Mask Register */
  #define REG_PMC_FSMR                    (0x400E0470U) /**< \brief (PMC) Fast Startup Mode Register */
  #define REG_PMC_FSPR                    (0x400E0474U) /**< \brief (PMC) Fast Startup Polarity Register */
  #define REG_PMC_FOCR                    (0x400E0478U) /**< \brief (PMC) Fault Output Clear Register */
  #define REG_PMC_WPMR                    (0x400E04E4U) /**< \brief (PMC) Write Protection Mode Register */
  #define REG_PMC_WPSR                    (0x400E04E8U) /**< \brief (PMC) Write Protection Status Register */
  #define REG_PMC_PCER1                   (0x400E0500U) /**< \brief (PMC) Peripheral Clock Enable Register 1 */
  #define REG_PMC_PCDR1                   (0x400E0504U) /**< \brief (PMC) Peripheral Clock Disable Register 1 */
  #define REG_PMC_PCSR1                   (0x400E0508U) /**< \brief (PMC) Peripheral Clock Status Register 1 */
  #define REG_PMC_OCR                     (0x400E0510U) /**< \brief (PMC) Oscillator Calibration Register */
#else
  #define REG_PMC_SCER   (*(__O  uint32_t*)0x400E0400U) /**< \brief (PMC) System Clock Enable Register */
  #define REG_PMC_SCDR   (*(__O  uint32_t*)0x400E0404U) /**< \brief (PMC) System Clock Disable Register */
  #define REG_PMC_SCSR   (*(__I  uint32_t*)0x400E0408U) /**< \brief (PMC) System Clock Status Register */
  #define REG_PMC_PCER0  (*(__O  uint32_t*)0x400E0410U) /**< \brief (PMC) Peripheral Clock Enable Register 0 */
  #define REG_PMC_PCDR0  (*(__O  uint32_t*)0x400E0414U) /**< \brief (PMC) Peripheral Clock Disable Register 0 */
  #define REG_PMC_PCSR0  (*(__I  uint32_t*)0x400E0418U) /**< \brief (PMC) Peripheral Clock Status Register 0 */
  #define REG_CKGR_MOR   (*(__IO uint32_t*)0x400E0420U) /**< \brief (PMC) Main Oscillator Register */
  #define REG_CKGR_MCFR  (*(__IO uint32_t*)0x400E0424U) /**< \brief (PMC) Main Clock Frequency Register */
  #define REG_CKGR_PLLAR (*(__IO uint32_t*)0x400E0428U) /**< \brief (PMC) PLLA Register */
  #define REG_CKGR_PLLBR (*(__IO uint32_t*)0x400E042CU) /**< \brief (PMC) PLLB Register */
  #define REG_PMC_MCKR   (*(__IO uint32_t*)0x400E0430U) /**< \brief (PMC) Master Clock Register */
  #define REG_PMC_USB    (*(__IO uint32_t*)0x400E0438U) /**< \brief (PMC) USB Clock Register */
  #define REG_PMC_PCK    (*(__IO uint32_t*)0x400E0440U) /**< \brief (PMC) Programmable Clock 0 Register */
  #define REG_PMC_IER    (*(__O  uint32_t*)0x400E0460U) /**< \brief (PMC) Interrupt Enable Register */
  #define REG_PMC_IDR    (*(__O  uint32_t*)0x400E0464U) /**< \brief (PMC) Interrupt Disable Register */
  #define REG_PMC_SR     (*(__I  uint32_t*)0x400E0468U) /**< \brief (PMC) Status Register */
  #define REG_PMC_IMR    (*(__I  uint32_t*)0x400E046CU) /**< \brief (PMC) Interrupt Mask Register */
  #define REG_PMC_FSMR   (*(__IO uint32_t*)0x400E0470U) /**< \brief (PMC) Fast Startup Mode Register */
  #define REG_PMC_FSPR   (*(__IO uint32_t*)0x400E0474U) /**< \brief (PMC) Fast Startup Polarity Register */
  #define REG_PMC_FOCR   (*(__O  uint32_t*)0x400E0478U) /**< \brief (PMC) Fault Output Clear Register */
  #define REG_PMC_WPMR   (*(__IO uint32_t*)0x400E04E4U) /**< \brief (PMC) Write Protection Mode Register */
  #define REG_PMC_WPSR   (*(__I  uint32_t*)0x400E04E8U) /**< \brief (PMC) Write Protection Status Register */
  #define REG_PMC_PCER1  (*(__O  uint32_t*)0x400E0500U) /**< \brief (PMC) Peripheral Clock Enable Register 1 */
  #define REG_PMC_PCDR1  (*(__O  uint32_t*)0x400E0504U) /**< \brief (PMC) Peripheral Clock Disable Register 1 */
  #define REG_PMC_PCSR1  (*(__I  uint32_t*)0x400E0508U) /**< \brief (PMC) Peripheral Clock Status Register 1 */
  #define REG_PMC_OCR    (*(__IO uint32_t*)0x400E0510U) /**< \brief (PMC) Oscillator Calibration Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM4S_PMC_INSTANCE_ */
