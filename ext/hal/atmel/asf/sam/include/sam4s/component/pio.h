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

#ifndef _SAM4S_PIO_COMPONENT_
#define _SAM4S_PIO_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Parallel Input/Output Controller */
/* ============================================================================= */
/** \addtogroup SAM4S_PIO Parallel Input/Output Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Pio hardware registers */
typedef struct {
  __O  uint32_t PIO_PER;        /**< \brief (Pio Offset: 0x0000) PIO Enable Register */
  __O  uint32_t PIO_PDR;        /**< \brief (Pio Offset: 0x0004) PIO Disable Register */
  __I  uint32_t PIO_PSR;        /**< \brief (Pio Offset: 0x0008) PIO Status Register */
  __I  uint32_t Reserved1[1];
  __O  uint32_t PIO_OER;        /**< \brief (Pio Offset: 0x0010) Output Enable Register */
  __O  uint32_t PIO_ODR;        /**< \brief (Pio Offset: 0x0014) Output Disable Register */
  __I  uint32_t PIO_OSR;        /**< \brief (Pio Offset: 0x0018) Output Status Register */
  __I  uint32_t Reserved2[1];
  __O  uint32_t PIO_IFER;       /**< \brief (Pio Offset: 0x0020) Glitch Input Filter Enable Register */
  __O  uint32_t PIO_IFDR;       /**< \brief (Pio Offset: 0x0024) Glitch Input Filter Disable Register */
  __I  uint32_t PIO_IFSR;       /**< \brief (Pio Offset: 0x0028) Glitch Input Filter Status Register */
  __I  uint32_t Reserved3[1];
  __O  uint32_t PIO_SODR;       /**< \brief (Pio Offset: 0x0030) Set Output Data Register */
  __O  uint32_t PIO_CODR;       /**< \brief (Pio Offset: 0x0034) Clear Output Data Register */
  __IO uint32_t PIO_ODSR;       /**< \brief (Pio Offset: 0x0038) Output Data Status Register */
  __I  uint32_t PIO_PDSR;       /**< \brief (Pio Offset: 0x003C) Pin Data Status Register */
  __O  uint32_t PIO_IER;        /**< \brief (Pio Offset: 0x0040) Interrupt Enable Register */
  __O  uint32_t PIO_IDR;        /**< \brief (Pio Offset: 0x0044) Interrupt Disable Register */
  __I  uint32_t PIO_IMR;        /**< \brief (Pio Offset: 0x0048) Interrupt Mask Register */
  __I  uint32_t PIO_ISR;        /**< \brief (Pio Offset: 0x004C) Interrupt Status Register */
  __O  uint32_t PIO_MDER;       /**< \brief (Pio Offset: 0x0050) Multi-driver Enable Register */
  __O  uint32_t PIO_MDDR;       /**< \brief (Pio Offset: 0x0054) Multi-driver Disable Register */
  __I  uint32_t PIO_MDSR;       /**< \brief (Pio Offset: 0x0058) Multi-driver Status Register */
  __I  uint32_t Reserved4[1];
  __O  uint32_t PIO_PUDR;       /**< \brief (Pio Offset: 0x0060) Pull-up Disable Register */
  __O  uint32_t PIO_PUER;       /**< \brief (Pio Offset: 0x0064) Pull-up Enable Register */
  __I  uint32_t PIO_PUSR;       /**< \brief (Pio Offset: 0x0068) Pad Pull-up Status Register */
  __I  uint32_t Reserved5[1];
  __IO uint32_t PIO_ABCDSR[2];  /**< \brief (Pio Offset: 0x0070) Peripheral Select Register */
  __I  uint32_t Reserved6[2];
  __O  uint32_t PIO_IFSCDR;     /**< \brief (Pio Offset: 0x0080) Input Filter Slow Clock Disable Register */
  __O  uint32_t PIO_IFSCER;     /**< \brief (Pio Offset: 0x0084) Input Filter Slow Clock Enable Register */
  __I  uint32_t PIO_IFSCSR;     /**< \brief (Pio Offset: 0x0088) Input Filter Slow Clock Status Register */
  __IO uint32_t PIO_SCDR;       /**< \brief (Pio Offset: 0x008C) Slow Clock Divider Debouncing Register */
  __O  uint32_t PIO_PPDDR;      /**< \brief (Pio Offset: 0x0090) Pad Pull-down Disable Register */
  __O  uint32_t PIO_PPDER;      /**< \brief (Pio Offset: 0x0094) Pad Pull-down Enable Register */
  __I  uint32_t PIO_PPDSR;      /**< \brief (Pio Offset: 0x0098) Pad Pull-down Status Register */
  __I  uint32_t Reserved7[1];
  __O  uint32_t PIO_OWER;       /**< \brief (Pio Offset: 0x00A0) Output Write Enable */
  __O  uint32_t PIO_OWDR;       /**< \brief (Pio Offset: 0x00A4) Output Write Disable */
  __I  uint32_t PIO_OWSR;       /**< \brief (Pio Offset: 0x00A8) Output Write Status Register */
  __I  uint32_t Reserved8[1];
  __O  uint32_t PIO_AIMER;      /**< \brief (Pio Offset: 0x00B0) Additional Interrupt Modes Enable Register */
  __O  uint32_t PIO_AIMDR;      /**< \brief (Pio Offset: 0x00B4) Additional Interrupt Modes Disable Register */
  __I  uint32_t PIO_AIMMR;      /**< \brief (Pio Offset: 0x00B8) Additional Interrupt Modes Mask Register */
  __I  uint32_t Reserved9[1];
  __O  uint32_t PIO_ESR;        /**< \brief (Pio Offset: 0x00C0) Edge Select Register */
  __O  uint32_t PIO_LSR;        /**< \brief (Pio Offset: 0x00C4) Level Select Register */
  __I  uint32_t PIO_ELSR;       /**< \brief (Pio Offset: 0x00C8) Edge/Level Status Register */
  __I  uint32_t Reserved10[1];
  __O  uint32_t PIO_FELLSR;     /**< \brief (Pio Offset: 0x00D0) Falling Edge/Low-Level Select Register */
  __O  uint32_t PIO_REHLSR;     /**< \brief (Pio Offset: 0x00D4) Rising Edge/High-Level Select Register */
  __I  uint32_t PIO_FRLHSR;     /**< \brief (Pio Offset: 0x00D8) Fall/Rise - Low/High Status Register */
  __I  uint32_t Reserved11[1];
  __I  uint32_t PIO_LOCKSR;     /**< \brief (Pio Offset: 0x00E0) Lock Status */
  __IO uint32_t PIO_WPMR;       /**< \brief (Pio Offset: 0x00E4) Write Protection Mode Register */
  __I  uint32_t PIO_WPSR;       /**< \brief (Pio Offset: 0x00E8) Write Protection Status Register */
  __I  uint32_t Reserved12[5];
  __IO uint32_t PIO_SCHMITT;    /**< \brief (Pio Offset: 0x0100) Schmitt Trigger Register */
  __I  uint32_t Reserved13[19];
  __IO uint32_t PIO_PCMR;       /**< \brief (Pio Offset: 0x0150) Parallel Capture Mode Register */
  __O  uint32_t PIO_PCIER;      /**< \brief (Pio Offset: 0x0154) Parallel Capture Interrupt Enable Register */
  __O  uint32_t PIO_PCIDR;      /**< \brief (Pio Offset: 0x0158) Parallel Capture Interrupt Disable Register */
  __I  uint32_t PIO_PCIMR;      /**< \brief (Pio Offset: 0x015C) Parallel Capture Interrupt Mask Register */
  __I  uint32_t PIO_PCISR;      /**< \brief (Pio Offset: 0x0160) Parallel Capture Interrupt Status Register */
  __I  uint32_t PIO_PCRHR;      /**< \brief (Pio Offset: 0x0164) Parallel Capture Reception Holding Register */
  __IO uint32_t PIO_RPR;        /**< \brief (Pio Offset: 0x168) Receive Pointer Register */
  __IO uint32_t PIO_RCR;        /**< \brief (Pio Offset: 0x16C) Receive Counter Register */
  __I  uint32_t Reserved14[2];
  __IO uint32_t PIO_RNPR;       /**< \brief (Pio Offset: 0x178) Receive Next Pointer Register */
  __IO uint32_t PIO_RNCR;       /**< \brief (Pio Offset: 0x17C) Receive Next Counter Register */
  __I  uint32_t Reserved15[2];
  __O  uint32_t PIO_PTCR;       /**< \brief (Pio Offset: 0x188) Transfer Control Register */
  __I  uint32_t PIO_PTSR;       /**< \brief (Pio Offset: 0x18C) Transfer Status Register */
} Pio;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- PIO_PER : (PIO Offset: 0x0000) PIO Enable Register -------- */
#define PIO_PER_P0 (0x1u << 0) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P1 (0x1u << 1) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P2 (0x1u << 2) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P3 (0x1u << 3) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P4 (0x1u << 4) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P5 (0x1u << 5) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P6 (0x1u << 6) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P7 (0x1u << 7) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P8 (0x1u << 8) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P9 (0x1u << 9) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P10 (0x1u << 10) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P11 (0x1u << 11) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P12 (0x1u << 12) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P13 (0x1u << 13) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P14 (0x1u << 14) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P15 (0x1u << 15) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P16 (0x1u << 16) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P17 (0x1u << 17) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P18 (0x1u << 18) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P19 (0x1u << 19) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P20 (0x1u << 20) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P21 (0x1u << 21) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P22 (0x1u << 22) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P23 (0x1u << 23) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P24 (0x1u << 24) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P25 (0x1u << 25) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P26 (0x1u << 26) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P27 (0x1u << 27) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P28 (0x1u << 28) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P29 (0x1u << 29) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P30 (0x1u << 30) /**< \brief (PIO_PER) PIO Enable */
#define PIO_PER_P31 (0x1u << 31) /**< \brief (PIO_PER) PIO Enable */
/* -------- PIO_PDR : (PIO Offset: 0x0004) PIO Disable Register -------- */
#define PIO_PDR_P0 (0x1u << 0) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P1 (0x1u << 1) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P2 (0x1u << 2) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P3 (0x1u << 3) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P4 (0x1u << 4) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P5 (0x1u << 5) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P6 (0x1u << 6) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P7 (0x1u << 7) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P8 (0x1u << 8) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P9 (0x1u << 9) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P10 (0x1u << 10) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P11 (0x1u << 11) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P12 (0x1u << 12) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P13 (0x1u << 13) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P14 (0x1u << 14) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P15 (0x1u << 15) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P16 (0x1u << 16) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P17 (0x1u << 17) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P18 (0x1u << 18) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P19 (0x1u << 19) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P20 (0x1u << 20) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P21 (0x1u << 21) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P22 (0x1u << 22) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P23 (0x1u << 23) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P24 (0x1u << 24) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P25 (0x1u << 25) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P26 (0x1u << 26) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P27 (0x1u << 27) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P28 (0x1u << 28) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P29 (0x1u << 29) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P30 (0x1u << 30) /**< \brief (PIO_PDR) PIO Disable */
#define PIO_PDR_P31 (0x1u << 31) /**< \brief (PIO_PDR) PIO Disable */
/* -------- PIO_PSR : (PIO Offset: 0x0008) PIO Status Register -------- */
#define PIO_PSR_P0 (0x1u << 0) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P1 (0x1u << 1) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P2 (0x1u << 2) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P3 (0x1u << 3) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P4 (0x1u << 4) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P5 (0x1u << 5) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P6 (0x1u << 6) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P7 (0x1u << 7) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P8 (0x1u << 8) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P9 (0x1u << 9) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P10 (0x1u << 10) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P11 (0x1u << 11) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P12 (0x1u << 12) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P13 (0x1u << 13) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P14 (0x1u << 14) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P15 (0x1u << 15) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P16 (0x1u << 16) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P17 (0x1u << 17) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P18 (0x1u << 18) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P19 (0x1u << 19) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P20 (0x1u << 20) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P21 (0x1u << 21) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P22 (0x1u << 22) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P23 (0x1u << 23) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P24 (0x1u << 24) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P25 (0x1u << 25) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P26 (0x1u << 26) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P27 (0x1u << 27) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P28 (0x1u << 28) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P29 (0x1u << 29) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P30 (0x1u << 30) /**< \brief (PIO_PSR) PIO Status */
#define PIO_PSR_P31 (0x1u << 31) /**< \brief (PIO_PSR) PIO Status */
/* -------- PIO_OER : (PIO Offset: 0x0010) Output Enable Register -------- */
#define PIO_OER_P0 (0x1u << 0) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P1 (0x1u << 1) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P2 (0x1u << 2) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P3 (0x1u << 3) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P4 (0x1u << 4) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P5 (0x1u << 5) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P6 (0x1u << 6) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P7 (0x1u << 7) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P8 (0x1u << 8) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P9 (0x1u << 9) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P10 (0x1u << 10) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P11 (0x1u << 11) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P12 (0x1u << 12) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P13 (0x1u << 13) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P14 (0x1u << 14) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P15 (0x1u << 15) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P16 (0x1u << 16) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P17 (0x1u << 17) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P18 (0x1u << 18) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P19 (0x1u << 19) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P20 (0x1u << 20) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P21 (0x1u << 21) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P22 (0x1u << 22) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P23 (0x1u << 23) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P24 (0x1u << 24) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P25 (0x1u << 25) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P26 (0x1u << 26) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P27 (0x1u << 27) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P28 (0x1u << 28) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P29 (0x1u << 29) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P30 (0x1u << 30) /**< \brief (PIO_OER) Output Enable */
#define PIO_OER_P31 (0x1u << 31) /**< \brief (PIO_OER) Output Enable */
/* -------- PIO_ODR : (PIO Offset: 0x0014) Output Disable Register -------- */
#define PIO_ODR_P0 (0x1u << 0) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P1 (0x1u << 1) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P2 (0x1u << 2) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P3 (0x1u << 3) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P4 (0x1u << 4) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P5 (0x1u << 5) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P6 (0x1u << 6) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P7 (0x1u << 7) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P8 (0x1u << 8) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P9 (0x1u << 9) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P10 (0x1u << 10) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P11 (0x1u << 11) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P12 (0x1u << 12) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P13 (0x1u << 13) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P14 (0x1u << 14) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P15 (0x1u << 15) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P16 (0x1u << 16) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P17 (0x1u << 17) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P18 (0x1u << 18) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P19 (0x1u << 19) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P20 (0x1u << 20) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P21 (0x1u << 21) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P22 (0x1u << 22) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P23 (0x1u << 23) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P24 (0x1u << 24) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P25 (0x1u << 25) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P26 (0x1u << 26) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P27 (0x1u << 27) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P28 (0x1u << 28) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P29 (0x1u << 29) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P30 (0x1u << 30) /**< \brief (PIO_ODR) Output Disable */
#define PIO_ODR_P31 (0x1u << 31) /**< \brief (PIO_ODR) Output Disable */
/* -------- PIO_OSR : (PIO Offset: 0x0018) Output Status Register -------- */
#define PIO_OSR_P0 (0x1u << 0) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P1 (0x1u << 1) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P2 (0x1u << 2) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P3 (0x1u << 3) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P4 (0x1u << 4) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P5 (0x1u << 5) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P6 (0x1u << 6) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P7 (0x1u << 7) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P8 (0x1u << 8) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P9 (0x1u << 9) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P10 (0x1u << 10) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P11 (0x1u << 11) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P12 (0x1u << 12) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P13 (0x1u << 13) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P14 (0x1u << 14) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P15 (0x1u << 15) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P16 (0x1u << 16) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P17 (0x1u << 17) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P18 (0x1u << 18) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P19 (0x1u << 19) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P20 (0x1u << 20) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P21 (0x1u << 21) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P22 (0x1u << 22) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P23 (0x1u << 23) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P24 (0x1u << 24) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P25 (0x1u << 25) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P26 (0x1u << 26) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P27 (0x1u << 27) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P28 (0x1u << 28) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P29 (0x1u << 29) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P30 (0x1u << 30) /**< \brief (PIO_OSR) Output Status */
#define PIO_OSR_P31 (0x1u << 31) /**< \brief (PIO_OSR) Output Status */
/* -------- PIO_IFER : (PIO Offset: 0x0020) Glitch Input Filter Enable Register -------- */
#define PIO_IFER_P0 (0x1u << 0) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P1 (0x1u << 1) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P2 (0x1u << 2) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P3 (0x1u << 3) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P4 (0x1u << 4) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P5 (0x1u << 5) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P6 (0x1u << 6) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P7 (0x1u << 7) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P8 (0x1u << 8) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P9 (0x1u << 9) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P10 (0x1u << 10) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P11 (0x1u << 11) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P12 (0x1u << 12) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P13 (0x1u << 13) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P14 (0x1u << 14) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P15 (0x1u << 15) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P16 (0x1u << 16) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P17 (0x1u << 17) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P18 (0x1u << 18) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P19 (0x1u << 19) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P20 (0x1u << 20) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P21 (0x1u << 21) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P22 (0x1u << 22) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P23 (0x1u << 23) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P24 (0x1u << 24) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P25 (0x1u << 25) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P26 (0x1u << 26) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P27 (0x1u << 27) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P28 (0x1u << 28) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P29 (0x1u << 29) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P30 (0x1u << 30) /**< \brief (PIO_IFER) Input Filter Enable */
#define PIO_IFER_P31 (0x1u << 31) /**< \brief (PIO_IFER) Input Filter Enable */
/* -------- PIO_IFDR : (PIO Offset: 0x0024) Glitch Input Filter Disable Register -------- */
#define PIO_IFDR_P0 (0x1u << 0) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P1 (0x1u << 1) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P2 (0x1u << 2) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P3 (0x1u << 3) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P4 (0x1u << 4) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P5 (0x1u << 5) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P6 (0x1u << 6) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P7 (0x1u << 7) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P8 (0x1u << 8) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P9 (0x1u << 9) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P10 (0x1u << 10) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P11 (0x1u << 11) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P12 (0x1u << 12) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P13 (0x1u << 13) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P14 (0x1u << 14) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P15 (0x1u << 15) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P16 (0x1u << 16) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P17 (0x1u << 17) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P18 (0x1u << 18) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P19 (0x1u << 19) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P20 (0x1u << 20) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P21 (0x1u << 21) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P22 (0x1u << 22) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P23 (0x1u << 23) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P24 (0x1u << 24) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P25 (0x1u << 25) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P26 (0x1u << 26) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P27 (0x1u << 27) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P28 (0x1u << 28) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P29 (0x1u << 29) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P30 (0x1u << 30) /**< \brief (PIO_IFDR) Input Filter Disable */
#define PIO_IFDR_P31 (0x1u << 31) /**< \brief (PIO_IFDR) Input Filter Disable */
/* -------- PIO_IFSR : (PIO Offset: 0x0028) Glitch Input Filter Status Register -------- */
#define PIO_IFSR_P0 (0x1u << 0) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P1 (0x1u << 1) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P2 (0x1u << 2) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P3 (0x1u << 3) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P4 (0x1u << 4) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P5 (0x1u << 5) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P6 (0x1u << 6) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P7 (0x1u << 7) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P8 (0x1u << 8) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P9 (0x1u << 9) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P10 (0x1u << 10) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P11 (0x1u << 11) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P12 (0x1u << 12) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P13 (0x1u << 13) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P14 (0x1u << 14) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P15 (0x1u << 15) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P16 (0x1u << 16) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P17 (0x1u << 17) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P18 (0x1u << 18) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P19 (0x1u << 19) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P20 (0x1u << 20) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P21 (0x1u << 21) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P22 (0x1u << 22) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P23 (0x1u << 23) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P24 (0x1u << 24) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P25 (0x1u << 25) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P26 (0x1u << 26) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P27 (0x1u << 27) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P28 (0x1u << 28) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P29 (0x1u << 29) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P30 (0x1u << 30) /**< \brief (PIO_IFSR) Input Filter Status */
#define PIO_IFSR_P31 (0x1u << 31) /**< \brief (PIO_IFSR) Input Filter Status */
/* -------- PIO_SODR : (PIO Offset: 0x0030) Set Output Data Register -------- */
#define PIO_SODR_P0 (0x1u << 0) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P1 (0x1u << 1) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P2 (0x1u << 2) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P3 (0x1u << 3) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P4 (0x1u << 4) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P5 (0x1u << 5) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P6 (0x1u << 6) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P7 (0x1u << 7) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P8 (0x1u << 8) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P9 (0x1u << 9) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P10 (0x1u << 10) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P11 (0x1u << 11) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P12 (0x1u << 12) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P13 (0x1u << 13) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P14 (0x1u << 14) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P15 (0x1u << 15) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P16 (0x1u << 16) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P17 (0x1u << 17) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P18 (0x1u << 18) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P19 (0x1u << 19) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P20 (0x1u << 20) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P21 (0x1u << 21) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P22 (0x1u << 22) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P23 (0x1u << 23) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P24 (0x1u << 24) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P25 (0x1u << 25) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P26 (0x1u << 26) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P27 (0x1u << 27) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P28 (0x1u << 28) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P29 (0x1u << 29) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P30 (0x1u << 30) /**< \brief (PIO_SODR) Set Output Data */
#define PIO_SODR_P31 (0x1u << 31) /**< \brief (PIO_SODR) Set Output Data */
/* -------- PIO_CODR : (PIO Offset: 0x0034) Clear Output Data Register -------- */
#define PIO_CODR_P0 (0x1u << 0) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P1 (0x1u << 1) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P2 (0x1u << 2) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P3 (0x1u << 3) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P4 (0x1u << 4) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P5 (0x1u << 5) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P6 (0x1u << 6) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P7 (0x1u << 7) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P8 (0x1u << 8) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P9 (0x1u << 9) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P10 (0x1u << 10) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P11 (0x1u << 11) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P12 (0x1u << 12) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P13 (0x1u << 13) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P14 (0x1u << 14) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P15 (0x1u << 15) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P16 (0x1u << 16) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P17 (0x1u << 17) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P18 (0x1u << 18) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P19 (0x1u << 19) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P20 (0x1u << 20) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P21 (0x1u << 21) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P22 (0x1u << 22) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P23 (0x1u << 23) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P24 (0x1u << 24) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P25 (0x1u << 25) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P26 (0x1u << 26) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P27 (0x1u << 27) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P28 (0x1u << 28) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P29 (0x1u << 29) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P30 (0x1u << 30) /**< \brief (PIO_CODR) Clear Output Data */
#define PIO_CODR_P31 (0x1u << 31) /**< \brief (PIO_CODR) Clear Output Data */
/* -------- PIO_ODSR : (PIO Offset: 0x0038) Output Data Status Register -------- */
#define PIO_ODSR_P0 (0x1u << 0) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P1 (0x1u << 1) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P2 (0x1u << 2) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P3 (0x1u << 3) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P4 (0x1u << 4) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P5 (0x1u << 5) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P6 (0x1u << 6) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P7 (0x1u << 7) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P8 (0x1u << 8) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P9 (0x1u << 9) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P10 (0x1u << 10) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P11 (0x1u << 11) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P12 (0x1u << 12) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P13 (0x1u << 13) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P14 (0x1u << 14) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P15 (0x1u << 15) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P16 (0x1u << 16) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P17 (0x1u << 17) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P18 (0x1u << 18) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P19 (0x1u << 19) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P20 (0x1u << 20) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P21 (0x1u << 21) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P22 (0x1u << 22) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P23 (0x1u << 23) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P24 (0x1u << 24) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P25 (0x1u << 25) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P26 (0x1u << 26) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P27 (0x1u << 27) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P28 (0x1u << 28) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P29 (0x1u << 29) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P30 (0x1u << 30) /**< \brief (PIO_ODSR) Output Data Status */
#define PIO_ODSR_P31 (0x1u << 31) /**< \brief (PIO_ODSR) Output Data Status */
/* -------- PIO_PDSR : (PIO Offset: 0x003C) Pin Data Status Register -------- */
#define PIO_PDSR_P0 (0x1u << 0) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P1 (0x1u << 1) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P2 (0x1u << 2) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P3 (0x1u << 3) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P4 (0x1u << 4) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P5 (0x1u << 5) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P6 (0x1u << 6) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P7 (0x1u << 7) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P8 (0x1u << 8) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P9 (0x1u << 9) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P10 (0x1u << 10) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P11 (0x1u << 11) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P12 (0x1u << 12) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P13 (0x1u << 13) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P14 (0x1u << 14) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P15 (0x1u << 15) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P16 (0x1u << 16) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P17 (0x1u << 17) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P18 (0x1u << 18) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P19 (0x1u << 19) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P20 (0x1u << 20) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P21 (0x1u << 21) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P22 (0x1u << 22) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P23 (0x1u << 23) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P24 (0x1u << 24) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P25 (0x1u << 25) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P26 (0x1u << 26) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P27 (0x1u << 27) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P28 (0x1u << 28) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P29 (0x1u << 29) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P30 (0x1u << 30) /**< \brief (PIO_PDSR) Output Data Status */
#define PIO_PDSR_P31 (0x1u << 31) /**< \brief (PIO_PDSR) Output Data Status */
/* -------- PIO_IER : (PIO Offset: 0x0040) Interrupt Enable Register -------- */
#define PIO_IER_P0 (0x1u << 0) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P1 (0x1u << 1) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P2 (0x1u << 2) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P3 (0x1u << 3) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P4 (0x1u << 4) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P5 (0x1u << 5) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P6 (0x1u << 6) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P7 (0x1u << 7) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P8 (0x1u << 8) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P9 (0x1u << 9) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P10 (0x1u << 10) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P11 (0x1u << 11) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P12 (0x1u << 12) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P13 (0x1u << 13) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P14 (0x1u << 14) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P15 (0x1u << 15) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P16 (0x1u << 16) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P17 (0x1u << 17) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P18 (0x1u << 18) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P19 (0x1u << 19) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P20 (0x1u << 20) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P21 (0x1u << 21) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P22 (0x1u << 22) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P23 (0x1u << 23) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P24 (0x1u << 24) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P25 (0x1u << 25) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P26 (0x1u << 26) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P27 (0x1u << 27) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P28 (0x1u << 28) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P29 (0x1u << 29) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P30 (0x1u << 30) /**< \brief (PIO_IER) Input Change Interrupt Enable */
#define PIO_IER_P31 (0x1u << 31) /**< \brief (PIO_IER) Input Change Interrupt Enable */
/* -------- PIO_IDR : (PIO Offset: 0x0044) Interrupt Disable Register -------- */
#define PIO_IDR_P0 (0x1u << 0) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P1 (0x1u << 1) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P2 (0x1u << 2) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P3 (0x1u << 3) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P4 (0x1u << 4) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P5 (0x1u << 5) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P6 (0x1u << 6) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P7 (0x1u << 7) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P8 (0x1u << 8) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P9 (0x1u << 9) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P10 (0x1u << 10) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P11 (0x1u << 11) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P12 (0x1u << 12) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P13 (0x1u << 13) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P14 (0x1u << 14) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P15 (0x1u << 15) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P16 (0x1u << 16) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P17 (0x1u << 17) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P18 (0x1u << 18) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P19 (0x1u << 19) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P20 (0x1u << 20) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P21 (0x1u << 21) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P22 (0x1u << 22) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P23 (0x1u << 23) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P24 (0x1u << 24) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P25 (0x1u << 25) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P26 (0x1u << 26) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P27 (0x1u << 27) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P28 (0x1u << 28) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P29 (0x1u << 29) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P30 (0x1u << 30) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
#define PIO_IDR_P31 (0x1u << 31) /**< \brief (PIO_IDR) Input Change Interrupt Disable */
/* -------- PIO_IMR : (PIO Offset: 0x0048) Interrupt Mask Register -------- */
#define PIO_IMR_P0 (0x1u << 0) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P1 (0x1u << 1) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P2 (0x1u << 2) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P3 (0x1u << 3) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P4 (0x1u << 4) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P5 (0x1u << 5) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P6 (0x1u << 6) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P7 (0x1u << 7) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P8 (0x1u << 8) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P9 (0x1u << 9) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P10 (0x1u << 10) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P11 (0x1u << 11) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P12 (0x1u << 12) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P13 (0x1u << 13) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P14 (0x1u << 14) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P15 (0x1u << 15) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P16 (0x1u << 16) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P17 (0x1u << 17) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P18 (0x1u << 18) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P19 (0x1u << 19) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P20 (0x1u << 20) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P21 (0x1u << 21) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P22 (0x1u << 22) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P23 (0x1u << 23) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P24 (0x1u << 24) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P25 (0x1u << 25) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P26 (0x1u << 26) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P27 (0x1u << 27) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P28 (0x1u << 28) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P29 (0x1u << 29) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P30 (0x1u << 30) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
#define PIO_IMR_P31 (0x1u << 31) /**< \brief (PIO_IMR) Input Change Interrupt Mask */
/* -------- PIO_ISR : (PIO Offset: 0x004C) Interrupt Status Register -------- */
#define PIO_ISR_P0 (0x1u << 0) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P1 (0x1u << 1) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P2 (0x1u << 2) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P3 (0x1u << 3) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P4 (0x1u << 4) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P5 (0x1u << 5) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P6 (0x1u << 6) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P7 (0x1u << 7) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P8 (0x1u << 8) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P9 (0x1u << 9) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P10 (0x1u << 10) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P11 (0x1u << 11) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P12 (0x1u << 12) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P13 (0x1u << 13) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P14 (0x1u << 14) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P15 (0x1u << 15) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P16 (0x1u << 16) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P17 (0x1u << 17) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P18 (0x1u << 18) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P19 (0x1u << 19) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P20 (0x1u << 20) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P21 (0x1u << 21) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P22 (0x1u << 22) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P23 (0x1u << 23) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P24 (0x1u << 24) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P25 (0x1u << 25) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P26 (0x1u << 26) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P27 (0x1u << 27) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P28 (0x1u << 28) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P29 (0x1u << 29) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P30 (0x1u << 30) /**< \brief (PIO_ISR) Input Change Interrupt Status */
#define PIO_ISR_P31 (0x1u << 31) /**< \brief (PIO_ISR) Input Change Interrupt Status */
/* -------- PIO_MDER : (PIO Offset: 0x0050) Multi-driver Enable Register -------- */
#define PIO_MDER_P0 (0x1u << 0) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P1 (0x1u << 1) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P2 (0x1u << 2) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P3 (0x1u << 3) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P4 (0x1u << 4) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P5 (0x1u << 5) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P6 (0x1u << 6) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P7 (0x1u << 7) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P8 (0x1u << 8) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P9 (0x1u << 9) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P10 (0x1u << 10) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P11 (0x1u << 11) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P12 (0x1u << 12) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P13 (0x1u << 13) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P14 (0x1u << 14) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P15 (0x1u << 15) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P16 (0x1u << 16) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P17 (0x1u << 17) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P18 (0x1u << 18) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P19 (0x1u << 19) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P20 (0x1u << 20) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P21 (0x1u << 21) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P22 (0x1u << 22) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P23 (0x1u << 23) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P24 (0x1u << 24) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P25 (0x1u << 25) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P26 (0x1u << 26) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P27 (0x1u << 27) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P28 (0x1u << 28) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P29 (0x1u << 29) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P30 (0x1u << 30) /**< \brief (PIO_MDER) Multi-drive Enable */
#define PIO_MDER_P31 (0x1u << 31) /**< \brief (PIO_MDER) Multi-drive Enable */
/* -------- PIO_MDDR : (PIO Offset: 0x0054) Multi-driver Disable Register -------- */
#define PIO_MDDR_P0 (0x1u << 0) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P1 (0x1u << 1) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P2 (0x1u << 2) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P3 (0x1u << 3) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P4 (0x1u << 4) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P5 (0x1u << 5) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P6 (0x1u << 6) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P7 (0x1u << 7) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P8 (0x1u << 8) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P9 (0x1u << 9) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P10 (0x1u << 10) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P11 (0x1u << 11) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P12 (0x1u << 12) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P13 (0x1u << 13) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P14 (0x1u << 14) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P15 (0x1u << 15) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P16 (0x1u << 16) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P17 (0x1u << 17) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P18 (0x1u << 18) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P19 (0x1u << 19) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P20 (0x1u << 20) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P21 (0x1u << 21) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P22 (0x1u << 22) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P23 (0x1u << 23) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P24 (0x1u << 24) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P25 (0x1u << 25) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P26 (0x1u << 26) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P27 (0x1u << 27) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P28 (0x1u << 28) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P29 (0x1u << 29) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P30 (0x1u << 30) /**< \brief (PIO_MDDR) Multi-drive Disable */
#define PIO_MDDR_P31 (0x1u << 31) /**< \brief (PIO_MDDR) Multi-drive Disable */
/* -------- PIO_MDSR : (PIO Offset: 0x0058) Multi-driver Status Register -------- */
#define PIO_MDSR_P0 (0x1u << 0) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P1 (0x1u << 1) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P2 (0x1u << 2) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P3 (0x1u << 3) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P4 (0x1u << 4) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P5 (0x1u << 5) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P6 (0x1u << 6) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P7 (0x1u << 7) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P8 (0x1u << 8) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P9 (0x1u << 9) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P10 (0x1u << 10) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P11 (0x1u << 11) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P12 (0x1u << 12) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P13 (0x1u << 13) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P14 (0x1u << 14) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P15 (0x1u << 15) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P16 (0x1u << 16) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P17 (0x1u << 17) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P18 (0x1u << 18) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P19 (0x1u << 19) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P20 (0x1u << 20) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P21 (0x1u << 21) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P22 (0x1u << 22) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P23 (0x1u << 23) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P24 (0x1u << 24) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P25 (0x1u << 25) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P26 (0x1u << 26) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P27 (0x1u << 27) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P28 (0x1u << 28) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P29 (0x1u << 29) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P30 (0x1u << 30) /**< \brief (PIO_MDSR) Multi-drive Status */
#define PIO_MDSR_P31 (0x1u << 31) /**< \brief (PIO_MDSR) Multi-drive Status */
/* -------- PIO_PUDR : (PIO Offset: 0x0060) Pull-up Disable Register -------- */
#define PIO_PUDR_P0 (0x1u << 0) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P1 (0x1u << 1) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P2 (0x1u << 2) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P3 (0x1u << 3) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P4 (0x1u << 4) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P5 (0x1u << 5) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P6 (0x1u << 6) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P7 (0x1u << 7) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P8 (0x1u << 8) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P9 (0x1u << 9) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P10 (0x1u << 10) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P11 (0x1u << 11) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P12 (0x1u << 12) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P13 (0x1u << 13) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P14 (0x1u << 14) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P15 (0x1u << 15) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P16 (0x1u << 16) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P17 (0x1u << 17) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P18 (0x1u << 18) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P19 (0x1u << 19) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P20 (0x1u << 20) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P21 (0x1u << 21) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P22 (0x1u << 22) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P23 (0x1u << 23) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P24 (0x1u << 24) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P25 (0x1u << 25) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P26 (0x1u << 26) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P27 (0x1u << 27) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P28 (0x1u << 28) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P29 (0x1u << 29) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P30 (0x1u << 30) /**< \brief (PIO_PUDR) Pull-Up Disable */
#define PIO_PUDR_P31 (0x1u << 31) /**< \brief (PIO_PUDR) Pull-Up Disable */
/* -------- PIO_PUER : (PIO Offset: 0x0064) Pull-up Enable Register -------- */
#define PIO_PUER_P0 (0x1u << 0) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P1 (0x1u << 1) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P2 (0x1u << 2) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P3 (0x1u << 3) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P4 (0x1u << 4) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P5 (0x1u << 5) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P6 (0x1u << 6) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P7 (0x1u << 7) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P8 (0x1u << 8) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P9 (0x1u << 9) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P10 (0x1u << 10) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P11 (0x1u << 11) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P12 (0x1u << 12) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P13 (0x1u << 13) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P14 (0x1u << 14) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P15 (0x1u << 15) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P16 (0x1u << 16) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P17 (0x1u << 17) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P18 (0x1u << 18) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P19 (0x1u << 19) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P20 (0x1u << 20) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P21 (0x1u << 21) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P22 (0x1u << 22) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P23 (0x1u << 23) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P24 (0x1u << 24) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P25 (0x1u << 25) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P26 (0x1u << 26) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P27 (0x1u << 27) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P28 (0x1u << 28) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P29 (0x1u << 29) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P30 (0x1u << 30) /**< \brief (PIO_PUER) Pull-Up Enable */
#define PIO_PUER_P31 (0x1u << 31) /**< \brief (PIO_PUER) Pull-Up Enable */
/* -------- PIO_PUSR : (PIO Offset: 0x0068) Pad Pull-up Status Register -------- */
#define PIO_PUSR_P0 (0x1u << 0) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P1 (0x1u << 1) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P2 (0x1u << 2) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P3 (0x1u << 3) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P4 (0x1u << 4) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P5 (0x1u << 5) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P6 (0x1u << 6) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P7 (0x1u << 7) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P8 (0x1u << 8) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P9 (0x1u << 9) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P10 (0x1u << 10) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P11 (0x1u << 11) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P12 (0x1u << 12) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P13 (0x1u << 13) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P14 (0x1u << 14) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P15 (0x1u << 15) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P16 (0x1u << 16) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P17 (0x1u << 17) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P18 (0x1u << 18) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P19 (0x1u << 19) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P20 (0x1u << 20) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P21 (0x1u << 21) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P22 (0x1u << 22) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P23 (0x1u << 23) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P24 (0x1u << 24) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P25 (0x1u << 25) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P26 (0x1u << 26) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P27 (0x1u << 27) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P28 (0x1u << 28) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P29 (0x1u << 29) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P30 (0x1u << 30) /**< \brief (PIO_PUSR) Pull-Up Status */
#define PIO_PUSR_P31 (0x1u << 31) /**< \brief (PIO_PUSR) Pull-Up Status */
/* -------- PIO_ABCDSR[2] : (PIO Offset: 0x0070) Peripheral Select Register -------- */
#define PIO_ABCDSR_P0 (0x1u << 0) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P1 (0x1u << 1) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P2 (0x1u << 2) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P3 (0x1u << 3) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P4 (0x1u << 4) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P5 (0x1u << 5) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P6 (0x1u << 6) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P7 (0x1u << 7) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P8 (0x1u << 8) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P9 (0x1u << 9) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P10 (0x1u << 10) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P11 (0x1u << 11) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P12 (0x1u << 12) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P13 (0x1u << 13) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P14 (0x1u << 14) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P15 (0x1u << 15) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P16 (0x1u << 16) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P17 (0x1u << 17) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P18 (0x1u << 18) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P19 (0x1u << 19) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P20 (0x1u << 20) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P21 (0x1u << 21) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P22 (0x1u << 22) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P23 (0x1u << 23) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P24 (0x1u << 24) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P25 (0x1u << 25) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P26 (0x1u << 26) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P27 (0x1u << 27) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P28 (0x1u << 28) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P29 (0x1u << 29) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P30 (0x1u << 30) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
#define PIO_ABCDSR_P31 (0x1u << 31) /**< \brief (PIO_ABCDSR[2]) Peripheral Select */
/* -------- PIO_IFSCDR : (PIO Offset: 0x0080) Input Filter Slow Clock Disable Register -------- */
#define PIO_IFSCDR_P0 (0x1u << 0) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P1 (0x1u << 1) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P2 (0x1u << 2) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P3 (0x1u << 3) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P4 (0x1u << 4) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P5 (0x1u << 5) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P6 (0x1u << 6) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P7 (0x1u << 7) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P8 (0x1u << 8) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P9 (0x1u << 9) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P10 (0x1u << 10) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P11 (0x1u << 11) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P12 (0x1u << 12) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P13 (0x1u << 13) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P14 (0x1u << 14) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P15 (0x1u << 15) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P16 (0x1u << 16) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P17 (0x1u << 17) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P18 (0x1u << 18) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P19 (0x1u << 19) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P20 (0x1u << 20) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P21 (0x1u << 21) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P22 (0x1u << 22) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P23 (0x1u << 23) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P24 (0x1u << 24) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P25 (0x1u << 25) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P26 (0x1u << 26) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P27 (0x1u << 27) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P28 (0x1u << 28) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P29 (0x1u << 29) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P30 (0x1u << 30) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P31 (0x1u << 31) /**< \brief (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select */
/* -------- PIO_IFSCER : (PIO Offset: 0x0084) Input Filter Slow Clock Enable Register -------- */
#define PIO_IFSCER_P0 (0x1u << 0) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P1 (0x1u << 1) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P2 (0x1u << 2) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P3 (0x1u << 3) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P4 (0x1u << 4) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P5 (0x1u << 5) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P6 (0x1u << 6) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P7 (0x1u << 7) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P8 (0x1u << 8) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P9 (0x1u << 9) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P10 (0x1u << 10) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P11 (0x1u << 11) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P12 (0x1u << 12) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P13 (0x1u << 13) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P14 (0x1u << 14) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P15 (0x1u << 15) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P16 (0x1u << 16) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P17 (0x1u << 17) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P18 (0x1u << 18) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P19 (0x1u << 19) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P20 (0x1u << 20) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P21 (0x1u << 21) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P22 (0x1u << 22) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P23 (0x1u << 23) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P24 (0x1u << 24) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P25 (0x1u << 25) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P26 (0x1u << 26) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P27 (0x1u << 27) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P28 (0x1u << 28) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P29 (0x1u << 29) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P30 (0x1u << 30) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P31 (0x1u << 31) /**< \brief (PIO_IFSCER) Slow Clock Debouncing Filtering Select */
/* -------- PIO_IFSCSR : (PIO Offset: 0x0088) Input Filter Slow Clock Status Register -------- */
#define PIO_IFSCSR_P0 (0x1u << 0) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P1 (0x1u << 1) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P2 (0x1u << 2) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P3 (0x1u << 3) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P4 (0x1u << 4) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P5 (0x1u << 5) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P6 (0x1u << 6) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P7 (0x1u << 7) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P8 (0x1u << 8) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P9 (0x1u << 9) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P10 (0x1u << 10) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P11 (0x1u << 11) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P12 (0x1u << 12) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P13 (0x1u << 13) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P14 (0x1u << 14) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P15 (0x1u << 15) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P16 (0x1u << 16) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P17 (0x1u << 17) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P18 (0x1u << 18) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P19 (0x1u << 19) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P20 (0x1u << 20) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P21 (0x1u << 21) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P22 (0x1u << 22) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P23 (0x1u << 23) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P24 (0x1u << 24) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P25 (0x1u << 25) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P26 (0x1u << 26) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P27 (0x1u << 27) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P28 (0x1u << 28) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P29 (0x1u << 29) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P30 (0x1u << 30) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P31 (0x1u << 31) /**< \brief (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status */
/* -------- PIO_SCDR : (PIO Offset: 0x008C) Slow Clock Divider Debouncing Register -------- */
#define PIO_SCDR_DIV_Pos 0
#define PIO_SCDR_DIV_Msk (0x3fffu << PIO_SCDR_DIV_Pos) /**< \brief (PIO_SCDR) Slow Clock Divider Selection for Debouncing */
#define PIO_SCDR_DIV(value) ((PIO_SCDR_DIV_Msk & ((value) << PIO_SCDR_DIV_Pos)))
/* -------- PIO_PPDDR : (PIO Offset: 0x0090) Pad Pull-down Disable Register -------- */
#define PIO_PPDDR_P0 (0x1u << 0) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P1 (0x1u << 1) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P2 (0x1u << 2) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P3 (0x1u << 3) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P4 (0x1u << 4) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P5 (0x1u << 5) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P6 (0x1u << 6) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P7 (0x1u << 7) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P8 (0x1u << 8) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P9 (0x1u << 9) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P10 (0x1u << 10) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P11 (0x1u << 11) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P12 (0x1u << 12) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P13 (0x1u << 13) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P14 (0x1u << 14) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P15 (0x1u << 15) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P16 (0x1u << 16) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P17 (0x1u << 17) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P18 (0x1u << 18) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P19 (0x1u << 19) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P20 (0x1u << 20) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P21 (0x1u << 21) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P22 (0x1u << 22) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P23 (0x1u << 23) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P24 (0x1u << 24) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P25 (0x1u << 25) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P26 (0x1u << 26) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P27 (0x1u << 27) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P28 (0x1u << 28) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P29 (0x1u << 29) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P30 (0x1u << 30) /**< \brief (PIO_PPDDR) Pull-Down Disable */
#define PIO_PPDDR_P31 (0x1u << 31) /**< \brief (PIO_PPDDR) Pull-Down Disable */
/* -------- PIO_PPDER : (PIO Offset: 0x0094) Pad Pull-down Enable Register -------- */
#define PIO_PPDER_P0 (0x1u << 0) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P1 (0x1u << 1) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P2 (0x1u << 2) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P3 (0x1u << 3) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P4 (0x1u << 4) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P5 (0x1u << 5) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P6 (0x1u << 6) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P7 (0x1u << 7) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P8 (0x1u << 8) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P9 (0x1u << 9) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P10 (0x1u << 10) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P11 (0x1u << 11) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P12 (0x1u << 12) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P13 (0x1u << 13) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P14 (0x1u << 14) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P15 (0x1u << 15) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P16 (0x1u << 16) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P17 (0x1u << 17) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P18 (0x1u << 18) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P19 (0x1u << 19) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P20 (0x1u << 20) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P21 (0x1u << 21) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P22 (0x1u << 22) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P23 (0x1u << 23) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P24 (0x1u << 24) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P25 (0x1u << 25) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P26 (0x1u << 26) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P27 (0x1u << 27) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P28 (0x1u << 28) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P29 (0x1u << 29) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P30 (0x1u << 30) /**< \brief (PIO_PPDER) Pull-Down Enable */
#define PIO_PPDER_P31 (0x1u << 31) /**< \brief (PIO_PPDER) Pull-Down Enable */
/* -------- PIO_PPDSR : (PIO Offset: 0x0098) Pad Pull-down Status Register -------- */
#define PIO_PPDSR_P0 (0x1u << 0) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P1 (0x1u << 1) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P2 (0x1u << 2) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P3 (0x1u << 3) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P4 (0x1u << 4) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P5 (0x1u << 5) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P6 (0x1u << 6) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P7 (0x1u << 7) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P8 (0x1u << 8) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P9 (0x1u << 9) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P10 (0x1u << 10) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P11 (0x1u << 11) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P12 (0x1u << 12) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P13 (0x1u << 13) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P14 (0x1u << 14) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P15 (0x1u << 15) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P16 (0x1u << 16) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P17 (0x1u << 17) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P18 (0x1u << 18) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P19 (0x1u << 19) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P20 (0x1u << 20) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P21 (0x1u << 21) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P22 (0x1u << 22) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P23 (0x1u << 23) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P24 (0x1u << 24) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P25 (0x1u << 25) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P26 (0x1u << 26) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P27 (0x1u << 27) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P28 (0x1u << 28) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P29 (0x1u << 29) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P30 (0x1u << 30) /**< \brief (PIO_PPDSR) Pull-Down Status */
#define PIO_PPDSR_P31 (0x1u << 31) /**< \brief (PIO_PPDSR) Pull-Down Status */
/* -------- PIO_OWER : (PIO Offset: 0x00A0) Output Write Enable -------- */
#define PIO_OWER_P0 (0x1u << 0) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P1 (0x1u << 1) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P2 (0x1u << 2) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P3 (0x1u << 3) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P4 (0x1u << 4) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P5 (0x1u << 5) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P6 (0x1u << 6) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P7 (0x1u << 7) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P8 (0x1u << 8) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P9 (0x1u << 9) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P10 (0x1u << 10) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P11 (0x1u << 11) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P12 (0x1u << 12) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P13 (0x1u << 13) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P14 (0x1u << 14) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P15 (0x1u << 15) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P16 (0x1u << 16) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P17 (0x1u << 17) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P18 (0x1u << 18) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P19 (0x1u << 19) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P20 (0x1u << 20) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P21 (0x1u << 21) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P22 (0x1u << 22) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P23 (0x1u << 23) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P24 (0x1u << 24) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P25 (0x1u << 25) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P26 (0x1u << 26) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P27 (0x1u << 27) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P28 (0x1u << 28) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P29 (0x1u << 29) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P30 (0x1u << 30) /**< \brief (PIO_OWER) Output Write Enable */
#define PIO_OWER_P31 (0x1u << 31) /**< \brief (PIO_OWER) Output Write Enable */
/* -------- PIO_OWDR : (PIO Offset: 0x00A4) Output Write Disable -------- */
#define PIO_OWDR_P0 (0x1u << 0) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P1 (0x1u << 1) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P2 (0x1u << 2) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P3 (0x1u << 3) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P4 (0x1u << 4) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P5 (0x1u << 5) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P6 (0x1u << 6) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P7 (0x1u << 7) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P8 (0x1u << 8) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P9 (0x1u << 9) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P10 (0x1u << 10) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P11 (0x1u << 11) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P12 (0x1u << 12) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P13 (0x1u << 13) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P14 (0x1u << 14) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P15 (0x1u << 15) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P16 (0x1u << 16) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P17 (0x1u << 17) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P18 (0x1u << 18) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P19 (0x1u << 19) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P20 (0x1u << 20) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P21 (0x1u << 21) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P22 (0x1u << 22) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P23 (0x1u << 23) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P24 (0x1u << 24) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P25 (0x1u << 25) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P26 (0x1u << 26) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P27 (0x1u << 27) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P28 (0x1u << 28) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P29 (0x1u << 29) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P30 (0x1u << 30) /**< \brief (PIO_OWDR) Output Write Disable */
#define PIO_OWDR_P31 (0x1u << 31) /**< \brief (PIO_OWDR) Output Write Disable */
/* -------- PIO_OWSR : (PIO Offset: 0x00A8) Output Write Status Register -------- */
#define PIO_OWSR_P0 (0x1u << 0) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P1 (0x1u << 1) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P2 (0x1u << 2) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P3 (0x1u << 3) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P4 (0x1u << 4) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P5 (0x1u << 5) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P6 (0x1u << 6) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P7 (0x1u << 7) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P8 (0x1u << 8) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P9 (0x1u << 9) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P10 (0x1u << 10) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P11 (0x1u << 11) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P12 (0x1u << 12) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P13 (0x1u << 13) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P14 (0x1u << 14) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P15 (0x1u << 15) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P16 (0x1u << 16) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P17 (0x1u << 17) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P18 (0x1u << 18) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P19 (0x1u << 19) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P20 (0x1u << 20) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P21 (0x1u << 21) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P22 (0x1u << 22) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P23 (0x1u << 23) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P24 (0x1u << 24) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P25 (0x1u << 25) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P26 (0x1u << 26) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P27 (0x1u << 27) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P28 (0x1u << 28) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P29 (0x1u << 29) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P30 (0x1u << 30) /**< \brief (PIO_OWSR) Output Write Status */
#define PIO_OWSR_P31 (0x1u << 31) /**< \brief (PIO_OWSR) Output Write Status */
/* -------- PIO_AIMER : (PIO Offset: 0x00B0) Additional Interrupt Modes Enable Register -------- */
#define PIO_AIMER_P0 (0x1u << 0) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P1 (0x1u << 1) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P2 (0x1u << 2) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P3 (0x1u << 3) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P4 (0x1u << 4) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P5 (0x1u << 5) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P6 (0x1u << 6) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P7 (0x1u << 7) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P8 (0x1u << 8) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P9 (0x1u << 9) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P10 (0x1u << 10) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P11 (0x1u << 11) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P12 (0x1u << 12) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P13 (0x1u << 13) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P14 (0x1u << 14) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P15 (0x1u << 15) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P16 (0x1u << 16) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P17 (0x1u << 17) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P18 (0x1u << 18) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P19 (0x1u << 19) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P20 (0x1u << 20) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P21 (0x1u << 21) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P22 (0x1u << 22) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P23 (0x1u << 23) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P24 (0x1u << 24) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P25 (0x1u << 25) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P26 (0x1u << 26) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P27 (0x1u << 27) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P28 (0x1u << 28) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P29 (0x1u << 29) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P30 (0x1u << 30) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
#define PIO_AIMER_P31 (0x1u << 31) /**< \brief (PIO_AIMER) Additional Interrupt Modes Enable */
/* -------- PIO_AIMDR : (PIO Offset: 0x00B4) Additional Interrupt Modes Disable Register -------- */
#define PIO_AIMDR_P0 (0x1u << 0) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P1 (0x1u << 1) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P2 (0x1u << 2) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P3 (0x1u << 3) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P4 (0x1u << 4) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P5 (0x1u << 5) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P6 (0x1u << 6) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P7 (0x1u << 7) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P8 (0x1u << 8) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P9 (0x1u << 9) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P10 (0x1u << 10) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P11 (0x1u << 11) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P12 (0x1u << 12) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P13 (0x1u << 13) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P14 (0x1u << 14) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P15 (0x1u << 15) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P16 (0x1u << 16) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P17 (0x1u << 17) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P18 (0x1u << 18) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P19 (0x1u << 19) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P20 (0x1u << 20) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P21 (0x1u << 21) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P22 (0x1u << 22) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P23 (0x1u << 23) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P24 (0x1u << 24) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P25 (0x1u << 25) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P26 (0x1u << 26) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P27 (0x1u << 27) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P28 (0x1u << 28) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P29 (0x1u << 29) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P30 (0x1u << 30) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P31 (0x1u << 31) /**< \brief (PIO_AIMDR) Additional Interrupt Modes Disable */
/* -------- PIO_AIMMR : (PIO Offset: 0x00B8) Additional Interrupt Modes Mask Register -------- */
#define PIO_AIMMR_P0 (0x1u << 0) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P1 (0x1u << 1) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P2 (0x1u << 2) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P3 (0x1u << 3) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P4 (0x1u << 4) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P5 (0x1u << 5) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P6 (0x1u << 6) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P7 (0x1u << 7) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P8 (0x1u << 8) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P9 (0x1u << 9) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P10 (0x1u << 10) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P11 (0x1u << 11) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P12 (0x1u << 12) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P13 (0x1u << 13) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P14 (0x1u << 14) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P15 (0x1u << 15) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P16 (0x1u << 16) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P17 (0x1u << 17) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P18 (0x1u << 18) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P19 (0x1u << 19) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P20 (0x1u << 20) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P21 (0x1u << 21) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P22 (0x1u << 22) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P23 (0x1u << 23) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P24 (0x1u << 24) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P25 (0x1u << 25) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P26 (0x1u << 26) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P27 (0x1u << 27) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P28 (0x1u << 28) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P29 (0x1u << 29) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P30 (0x1u << 30) /**< \brief (PIO_AIMMR) IO Line Index */
#define PIO_AIMMR_P31 (0x1u << 31) /**< \brief (PIO_AIMMR) IO Line Index */
/* -------- PIO_ESR : (PIO Offset: 0x00C0) Edge Select Register -------- */
#define PIO_ESR_P0 (0x1u << 0) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P1 (0x1u << 1) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P2 (0x1u << 2) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P3 (0x1u << 3) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P4 (0x1u << 4) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P5 (0x1u << 5) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P6 (0x1u << 6) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P7 (0x1u << 7) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P8 (0x1u << 8) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P9 (0x1u << 9) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P10 (0x1u << 10) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P11 (0x1u << 11) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P12 (0x1u << 12) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P13 (0x1u << 13) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P14 (0x1u << 14) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P15 (0x1u << 15) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P16 (0x1u << 16) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P17 (0x1u << 17) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P18 (0x1u << 18) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P19 (0x1u << 19) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P20 (0x1u << 20) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P21 (0x1u << 21) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P22 (0x1u << 22) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P23 (0x1u << 23) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P24 (0x1u << 24) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P25 (0x1u << 25) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P26 (0x1u << 26) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P27 (0x1u << 27) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P28 (0x1u << 28) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P29 (0x1u << 29) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P30 (0x1u << 30) /**< \brief (PIO_ESR) Edge Interrupt Selection */
#define PIO_ESR_P31 (0x1u << 31) /**< \brief (PIO_ESR) Edge Interrupt Selection */
/* -------- PIO_LSR : (PIO Offset: 0x00C4) Level Select Register -------- */
#define PIO_LSR_P0 (0x1u << 0) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P1 (0x1u << 1) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P2 (0x1u << 2) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P3 (0x1u << 3) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P4 (0x1u << 4) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P5 (0x1u << 5) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P6 (0x1u << 6) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P7 (0x1u << 7) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P8 (0x1u << 8) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P9 (0x1u << 9) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P10 (0x1u << 10) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P11 (0x1u << 11) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P12 (0x1u << 12) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P13 (0x1u << 13) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P14 (0x1u << 14) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P15 (0x1u << 15) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P16 (0x1u << 16) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P17 (0x1u << 17) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P18 (0x1u << 18) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P19 (0x1u << 19) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P20 (0x1u << 20) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P21 (0x1u << 21) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P22 (0x1u << 22) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P23 (0x1u << 23) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P24 (0x1u << 24) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P25 (0x1u << 25) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P26 (0x1u << 26) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P27 (0x1u << 27) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P28 (0x1u << 28) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P29 (0x1u << 29) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P30 (0x1u << 30) /**< \brief (PIO_LSR) Level Interrupt Selection */
#define PIO_LSR_P31 (0x1u << 31) /**< \brief (PIO_LSR) Level Interrupt Selection */
/* -------- PIO_ELSR : (PIO Offset: 0x00C8) Edge/Level Status Register -------- */
#define PIO_ELSR_P0 (0x1u << 0) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P1 (0x1u << 1) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P2 (0x1u << 2) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P3 (0x1u << 3) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P4 (0x1u << 4) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P5 (0x1u << 5) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P6 (0x1u << 6) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P7 (0x1u << 7) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P8 (0x1u << 8) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P9 (0x1u << 9) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P10 (0x1u << 10) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P11 (0x1u << 11) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P12 (0x1u << 12) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P13 (0x1u << 13) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P14 (0x1u << 14) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P15 (0x1u << 15) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P16 (0x1u << 16) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P17 (0x1u << 17) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P18 (0x1u << 18) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P19 (0x1u << 19) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P20 (0x1u << 20) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P21 (0x1u << 21) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P22 (0x1u << 22) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P23 (0x1u << 23) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P24 (0x1u << 24) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P25 (0x1u << 25) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P26 (0x1u << 26) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P27 (0x1u << 27) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P28 (0x1u << 28) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P29 (0x1u << 29) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P30 (0x1u << 30) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P31 (0x1u << 31) /**< \brief (PIO_ELSR) Edge/Level Interrupt Source Selection */
/* -------- PIO_FELLSR : (PIO Offset: 0x00D0) Falling Edge/Low-Level Select Register -------- */
#define PIO_FELLSR_P0 (0x1u << 0) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P1 (0x1u << 1) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P2 (0x1u << 2) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P3 (0x1u << 3) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P4 (0x1u << 4) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P5 (0x1u << 5) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P6 (0x1u << 6) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P7 (0x1u << 7) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P8 (0x1u << 8) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P9 (0x1u << 9) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P10 (0x1u << 10) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P11 (0x1u << 11) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P12 (0x1u << 12) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P13 (0x1u << 13) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P14 (0x1u << 14) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P15 (0x1u << 15) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P16 (0x1u << 16) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P17 (0x1u << 17) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P18 (0x1u << 18) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P19 (0x1u << 19) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P20 (0x1u << 20) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P21 (0x1u << 21) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P22 (0x1u << 22) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P23 (0x1u << 23) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P24 (0x1u << 24) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P25 (0x1u << 25) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P26 (0x1u << 26) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P27 (0x1u << 27) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P28 (0x1u << 28) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P29 (0x1u << 29) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P30 (0x1u << 30) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P31 (0x1u << 31) /**< \brief (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection */
/* -------- PIO_REHLSR : (PIO Offset: 0x00D4) Rising Edge/High-Level Select Register -------- */
#define PIO_REHLSR_P0 (0x1u << 0) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P1 (0x1u << 1) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P2 (0x1u << 2) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P3 (0x1u << 3) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P4 (0x1u << 4) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P5 (0x1u << 5) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P6 (0x1u << 6) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P7 (0x1u << 7) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P8 (0x1u << 8) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P9 (0x1u << 9) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P10 (0x1u << 10) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P11 (0x1u << 11) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P12 (0x1u << 12) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P13 (0x1u << 13) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P14 (0x1u << 14) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P15 (0x1u << 15) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P16 (0x1u << 16) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P17 (0x1u << 17) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P18 (0x1u << 18) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P19 (0x1u << 19) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P20 (0x1u << 20) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P21 (0x1u << 21) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P22 (0x1u << 22) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P23 (0x1u << 23) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P24 (0x1u << 24) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P25 (0x1u << 25) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P26 (0x1u << 26) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P27 (0x1u << 27) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P28 (0x1u << 28) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P29 (0x1u << 29) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P30 (0x1u << 30) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P31 (0x1u << 31) /**< \brief (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection */
/* -------- PIO_FRLHSR : (PIO Offset: 0x00D8) Fall/Rise - Low/High Status Register -------- */
#define PIO_FRLHSR_P0 (0x1u << 0) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P1 (0x1u << 1) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P2 (0x1u << 2) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P3 (0x1u << 3) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P4 (0x1u << 4) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P5 (0x1u << 5) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P6 (0x1u << 6) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P7 (0x1u << 7) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P8 (0x1u << 8) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P9 (0x1u << 9) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P10 (0x1u << 10) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P11 (0x1u << 11) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P12 (0x1u << 12) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P13 (0x1u << 13) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P14 (0x1u << 14) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P15 (0x1u << 15) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P16 (0x1u << 16) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P17 (0x1u << 17) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P18 (0x1u << 18) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P19 (0x1u << 19) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P20 (0x1u << 20) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P21 (0x1u << 21) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P22 (0x1u << 22) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P23 (0x1u << 23) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P24 (0x1u << 24) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P25 (0x1u << 25) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P26 (0x1u << 26) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P27 (0x1u << 27) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P28 (0x1u << 28) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P29 (0x1u << 29) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P30 (0x1u << 30) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P31 (0x1u << 31) /**< \brief (PIO_FRLHSR) Edge/Level Interrupt Source Selection */
/* -------- PIO_LOCKSR : (PIO Offset: 0x00E0) Lock Status -------- */
#define PIO_LOCKSR_P0 (0x1u << 0) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P1 (0x1u << 1) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P2 (0x1u << 2) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P3 (0x1u << 3) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P4 (0x1u << 4) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P5 (0x1u << 5) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P6 (0x1u << 6) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P7 (0x1u << 7) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P8 (0x1u << 8) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P9 (0x1u << 9) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P10 (0x1u << 10) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P11 (0x1u << 11) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P12 (0x1u << 12) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P13 (0x1u << 13) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P14 (0x1u << 14) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P15 (0x1u << 15) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P16 (0x1u << 16) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P17 (0x1u << 17) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P18 (0x1u << 18) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P19 (0x1u << 19) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P20 (0x1u << 20) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P21 (0x1u << 21) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P22 (0x1u << 22) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P23 (0x1u << 23) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P24 (0x1u << 24) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P25 (0x1u << 25) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P26 (0x1u << 26) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P27 (0x1u << 27) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P28 (0x1u << 28) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P29 (0x1u << 29) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P30 (0x1u << 30) /**< \brief (PIO_LOCKSR) Lock Status */
#define PIO_LOCKSR_P31 (0x1u << 31) /**< \brief (PIO_LOCKSR) Lock Status */
/* -------- PIO_WPMR : (PIO Offset: 0x00E4) Write Protection Mode Register -------- */
#define PIO_WPMR_WPEN (0x1u << 0) /**< \brief (PIO_WPMR) Write Protection Enable */
#define PIO_WPMR_WPKEY_Pos 8
#define PIO_WPMR_WPKEY_Msk (0xffffffu << PIO_WPMR_WPKEY_Pos) /**< \brief (PIO_WPMR) Write Protection Key */
#define PIO_WPMR_WPKEY(value) ((PIO_WPMR_WPKEY_Msk & ((value) << PIO_WPMR_WPKEY_Pos)))
#define   PIO_WPMR_WPKEY_PASSWD (0x50494Fu << 8) /**< \brief (PIO_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0. */
/* -------- PIO_WPSR : (PIO Offset: 0x00E8) Write Protection Status Register -------- */
#define PIO_WPSR_WPVS (0x1u << 0) /**< \brief (PIO_WPSR) Write Protection Violation Status */
#define PIO_WPSR_WPVSRC_Pos 8
#define PIO_WPSR_WPVSRC_Msk (0xffffu << PIO_WPSR_WPVSRC_Pos) /**< \brief (PIO_WPSR) Write Protection Violation Source */
/* -------- PIO_SCHMITT : (PIO Offset: 0x0100) Schmitt Trigger Register -------- */
#define PIO_SCHMITT_SCHMITT0 (0x1u << 0) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT1 (0x1u << 1) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT2 (0x1u << 2) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT3 (0x1u << 3) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT4 (0x1u << 4) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT5 (0x1u << 5) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT6 (0x1u << 6) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT7 (0x1u << 7) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT8 (0x1u << 8) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT9 (0x1u << 9) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT10 (0x1u << 10) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT11 (0x1u << 11) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT12 (0x1u << 12) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT13 (0x1u << 13) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT14 (0x1u << 14) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT15 (0x1u << 15) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT16 (0x1u << 16) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT17 (0x1u << 17) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT18 (0x1u << 18) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT19 (0x1u << 19) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT20 (0x1u << 20) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT21 (0x1u << 21) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT22 (0x1u << 22) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT23 (0x1u << 23) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT24 (0x1u << 24) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT25 (0x1u << 25) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT26 (0x1u << 26) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT27 (0x1u << 27) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT28 (0x1u << 28) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT29 (0x1u << 29) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT30 (0x1u << 30) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT31 (0x1u << 31) /**< \brief (PIO_SCHMITT) Schmitt Trigger Control */
/* -------- PIO_PCMR : (PIO Offset: 0x0150) Parallel Capture Mode Register -------- */
#define PIO_PCMR_PCEN (0x1u << 0) /**< \brief (PIO_PCMR) Parallel Capture Mode Enable */
#define PIO_PCMR_DSIZE_Pos 4
#define PIO_PCMR_DSIZE_Msk (0x3u << PIO_PCMR_DSIZE_Pos) /**< \brief (PIO_PCMR) Parallel Capture Mode Data Size */
#define PIO_PCMR_DSIZE(value) ((PIO_PCMR_DSIZE_Msk & ((value) << PIO_PCMR_DSIZE_Pos)))
#define   PIO_PCMR_DSIZE_BYTE (0x0u << 4) /**< \brief (PIO_PCMR) The reception data in the PIO_PCRHR is a byte (8-bit) */
#define   PIO_PCMR_DSIZE_HALFWORD (0x1u << 4) /**< \brief (PIO_PCMR) The reception data in the PIO_PCRHR is a half-word (16-bit) */
#define   PIO_PCMR_DSIZE_WORD (0x2u << 4) /**< \brief (PIO_PCMR) The reception data in the PIO_PCRHR is a word (32-bit) */
#define PIO_PCMR_ALWYS (0x1u << 9) /**< \brief (PIO_PCMR) Parallel Capture Mode Always Sampling */
#define PIO_PCMR_HALFS (0x1u << 10) /**< \brief (PIO_PCMR) Parallel Capture Mode Half Sampling */
#define PIO_PCMR_FRSTS (0x1u << 11) /**< \brief (PIO_PCMR) Parallel Capture Mode First Sample */
/* -------- PIO_PCIER : (PIO Offset: 0x0154) Parallel Capture Interrupt Enable Register -------- */
#define PIO_PCIER_DRDY (0x1u << 0) /**< \brief (PIO_PCIER) Parallel Capture Mode Data Ready Interrupt Enable */
#define PIO_PCIER_OVRE (0x1u << 1) /**< \brief (PIO_PCIER) Parallel Capture Mode Overrun Error Interrupt Enable */
#define PIO_PCIER_ENDRX (0x1u << 2) /**< \brief (PIO_PCIER) End of Reception Transfer Interrupt Enable */
#define PIO_PCIER_RXBUFF (0x1u << 3) /**< \brief (PIO_PCIER) Reception Buffer Full Interrupt Enable */
/* -------- PIO_PCIDR : (PIO Offset: 0x0158) Parallel Capture Interrupt Disable Register -------- */
#define PIO_PCIDR_DRDY (0x1u << 0) /**< \brief (PIO_PCIDR) Parallel Capture Mode Data Ready Interrupt Disable */
#define PIO_PCIDR_OVRE (0x1u << 1) /**< \brief (PIO_PCIDR) Parallel Capture Mode Overrun Error Interrupt Disable */
#define PIO_PCIDR_ENDRX (0x1u << 2) /**< \brief (PIO_PCIDR) End of Reception Transfer Interrupt Disable */
#define PIO_PCIDR_RXBUFF (0x1u << 3) /**< \brief (PIO_PCIDR) Reception Buffer Full Interrupt Disable */
/* -------- PIO_PCIMR : (PIO Offset: 0x015C) Parallel Capture Interrupt Mask Register -------- */
#define PIO_PCIMR_DRDY (0x1u << 0) /**< \brief (PIO_PCIMR) Parallel Capture Mode Data Ready Interrupt Mask */
#define PIO_PCIMR_OVRE (0x1u << 1) /**< \brief (PIO_PCIMR) Parallel Capture Mode Overrun Error Interrupt Mask */
#define PIO_PCIMR_ENDRX (0x1u << 2) /**< \brief (PIO_PCIMR) End of Reception Transfer Interrupt Mask */
#define PIO_PCIMR_RXBUFF (0x1u << 3) /**< \brief (PIO_PCIMR) Reception Buffer Full Interrupt Mask */
/* -------- PIO_PCISR : (PIO Offset: 0x0160) Parallel Capture Interrupt Status Register -------- */
#define PIO_PCISR_DRDY (0x1u << 0) /**< \brief (PIO_PCISR) Parallel Capture Mode Data Ready */
#define PIO_PCISR_OVRE (0x1u << 1) /**< \brief (PIO_PCISR) Parallel Capture Mode Overrun Error */
#define PIO_PCISR_ENDRX (0x1u << 2) /**< \brief (PIO_PCISR)  */
#define PIO_PCISR_RXBUFF (0x1u << 3) /**< \brief (PIO_PCISR)  */
/* -------- PIO_PCRHR : (PIO Offset: 0x0164) Parallel Capture Reception Holding Register -------- */
#define PIO_PCRHR_RDATA_Pos 0
#define PIO_PCRHR_RDATA_Msk (0xffffffffu << PIO_PCRHR_RDATA_Pos) /**< \brief (PIO_PCRHR) Parallel Capture Mode Reception Data */
/* -------- PIO_RPR : (PIO Offset: 0x168) Receive Pointer Register -------- */
#define PIO_RPR_RXPTR_Pos 0
#define PIO_RPR_RXPTR_Msk (0xffffffffu << PIO_RPR_RXPTR_Pos) /**< \brief (PIO_RPR) Receive Pointer Register */
#define PIO_RPR_RXPTR(value) ((PIO_RPR_RXPTR_Msk & ((value) << PIO_RPR_RXPTR_Pos)))
/* -------- PIO_RCR : (PIO Offset: 0x16C) Receive Counter Register -------- */
#define PIO_RCR_RXCTR_Pos 0
#define PIO_RCR_RXCTR_Msk (0xffffu << PIO_RCR_RXCTR_Pos) /**< \brief (PIO_RCR) Receive Counter Register */
#define PIO_RCR_RXCTR(value) ((PIO_RCR_RXCTR_Msk & ((value) << PIO_RCR_RXCTR_Pos)))
/* -------- PIO_RNPR : (PIO Offset: 0x178) Receive Next Pointer Register -------- */
#define PIO_RNPR_RXNPTR_Pos 0
#define PIO_RNPR_RXNPTR_Msk (0xffffffffu << PIO_RNPR_RXNPTR_Pos) /**< \brief (PIO_RNPR) Receive Next Pointer */
#define PIO_RNPR_RXNPTR(value) ((PIO_RNPR_RXNPTR_Msk & ((value) << PIO_RNPR_RXNPTR_Pos)))
/* -------- PIO_RNCR : (PIO Offset: 0x17C) Receive Next Counter Register -------- */
#define PIO_RNCR_RXNCTR_Pos 0
#define PIO_RNCR_RXNCTR_Msk (0xffffu << PIO_RNCR_RXNCTR_Pos) /**< \brief (PIO_RNCR) Receive Next Counter */
#define PIO_RNCR_RXNCTR(value) ((PIO_RNCR_RXNCTR_Msk & ((value) << PIO_RNCR_RXNCTR_Pos)))
/* -------- PIO_PTCR : (PIO Offset: 0x188) Transfer Control Register -------- */
#define PIO_PTCR_RXTEN (0x1u << 0) /**< \brief (PIO_PTCR) Receiver Transfer Enable */
#define PIO_PTCR_RXTDIS (0x1u << 1) /**< \brief (PIO_PTCR) Receiver Transfer Disable */
#define PIO_PTCR_TXTEN (0x1u << 8) /**< \brief (PIO_PTCR) Transmitter Transfer Enable */
#define PIO_PTCR_TXTDIS (0x1u << 9) /**< \brief (PIO_PTCR) Transmitter Transfer Disable */
/* -------- PIO_PTSR : (PIO Offset: 0x18C) Transfer Status Register -------- */
#define PIO_PTSR_RXTEN (0x1u << 0) /**< \brief (PIO_PTSR) Receiver Transfer Enable */
#define PIO_PTSR_TXTEN (0x1u << 8) /**< \brief (PIO_PTSR) Transmitter Transfer Enable */

/*@}*/


#endif /* _SAM4S_PIO_COMPONENT_ */
