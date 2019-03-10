/**
 * \file
 *
 * \brief Header file for SAME54P20A
 *
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAME54P20A_
#define _SAME54P20A_

/**
 * \ingroup SAME54_definitions
 * \addtogroup SAME54P20A_definitions SAME54P20A definitions
 * This file defines all structures and symbols for SAME54P20A:
 *   - registers and bitfields
 *   - peripheral base address
 *   - peripheral ID
 *   - PIO definitions
*/
/*@{*/

#ifdef __cplusplus
 extern "C" {
#endif

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#include <stdint.h>
#ifndef __cplusplus
typedef volatile const uint32_t RoReg;   /**< Read only 32-bit register (volatile const unsigned int) */
typedef volatile const uint16_t RoReg16; /**< Read only 16-bit register (volatile const unsigned int) */
typedef volatile const uint8_t  RoReg8;  /**< Read only  8-bit register (volatile const unsigned int) */
#else
typedef volatile       uint32_t RoReg;   /**< Read only 32-bit register (volatile const unsigned int) */
typedef volatile       uint16_t RoReg16; /**< Read only 16-bit register (volatile const unsigned int) */
typedef volatile       uint8_t  RoReg8;  /**< Read only  8-bit register (volatile const unsigned int) */
#endif
typedef volatile       uint32_t WoReg;   /**< Write only 32-bit register (volatile unsigned int) */
typedef volatile       uint16_t WoReg16; /**< Write only 16-bit register (volatile unsigned int) */
typedef volatile       uint8_t  WoReg8;  /**< Write only  8-bit register (volatile unsigned int) */
typedef volatile       uint32_t RwReg;   /**< Read-Write 32-bit register (volatile unsigned int) */
typedef volatile       uint16_t RwReg16; /**< Read-Write 16-bit register (volatile unsigned int) */
typedef volatile       uint8_t  RwReg8;  /**< Read-Write  8-bit register (volatile unsigned int) */
#endif

#if !defined(SKIP_INTEGER_LITERALS)
#if defined(_U_) || defined(_L_) || defined(_UL_)
  #error "Integer Literals macros already defined elsewhere"
#endif

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/* Macros that deal with adding suffixes to integer literal constants for C/C++ */
#define _U_(x)         x ## U            /**< C code: Unsigned integer literal constant value */
#define _L_(x)         x ## L            /**< C code: Long integer literal constant value */
#define _UL_(x)        x ## UL           /**< C code: Unsigned Long integer literal constant value */
#else /* Assembler */
#define _U_(x)         x                 /**< Assembler: Unsigned integer literal constant value */
#define _L_(x)         x                 /**< Assembler: Long integer literal constant value */
#define _UL_(x)        x                 /**< Assembler: Unsigned Long integer literal constant value */
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* SKIP_INTEGER_LITERALS */

/* ************************************************************************** */
/**  CMSIS DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */
/** \defgroup SAME54P20A_cmsis CMSIS Definitions */
/*@{*/

/** Interrupt Number Definition */
typedef enum IRQn
{
  /******  Cortex-M4 Processor Exceptions Numbers *******************/
  NonMaskableInt_IRQn      = -14,/**<  2 Non Maskable Interrupt      */
  HardFault_IRQn           = -13,/**<  3 Hard Fault Interrupt        */
  MemoryManagement_IRQn    = -12,/**<  4 Memory Management Interrupt */
  BusFault_IRQn            = -11,/**<  5 Bus Fault Interrupt         */
  UsageFault_IRQn          = -10,/**<  6 Usage Fault Interrupt       */
  SVCall_IRQn              = -5, /**< 11 SV Call Interrupt           */
  DebugMonitor_IRQn        = -4, /**< 12 Debug Monitor Interrupt     */
  PendSV_IRQn              = -2, /**< 14 Pend SV Interrupt           */
  SysTick_IRQn             = -1, /**< 15 System Tick Interrupt       */
  /******  SAME54P20A-specific Interrupt Numbers *********************/
  PM_IRQn                  =  0, /**<  0 SAME54P20A Power Manager (PM) */
  MCLK_IRQn                =  1, /**<  1 SAME54P20A Main Clock (MCLK) */
  OSCCTRL_0_IRQn           =  2, /**<  2 SAME54P20A Oscillators Control (OSCCTRL) IRQ 0 */
  OSCCTRL_1_IRQn           =  3, /**<  3 SAME54P20A Oscillators Control (OSCCTRL) IRQ 1 */
  OSCCTRL_2_IRQn           =  4, /**<  4 SAME54P20A Oscillators Control (OSCCTRL) IRQ 2 */
  OSCCTRL_3_IRQn           =  5, /**<  5 SAME54P20A Oscillators Control (OSCCTRL) IRQ 3 */
  OSCCTRL_4_IRQn           =  6, /**<  6 SAME54P20A Oscillators Control (OSCCTRL) IRQ 4 */
  OSC32KCTRL_IRQn          =  7, /**<  7 SAME54P20A 32kHz Oscillators Control (OSC32KCTRL) */
  SUPC_0_IRQn              =  8, /**<  8 SAME54P20A Supply Controller (SUPC) IRQ 0 */
  SUPC_1_IRQn              =  9, /**<  9 SAME54P20A Supply Controller (SUPC) IRQ 1 */
  WDT_IRQn                 = 10, /**< 10 SAME54P20A Watchdog Timer (WDT) */
  RTC_IRQn                 = 11, /**< 11 SAME54P20A Real-Time Counter (RTC) */
  EIC_0_IRQn               = 12, /**< 12 SAME54P20A External Interrupt Controller (EIC) IRQ 0 */
  EIC_1_IRQn               = 13, /**< 13 SAME54P20A External Interrupt Controller (EIC) IRQ 1 */
  EIC_2_IRQn               = 14, /**< 14 SAME54P20A External Interrupt Controller (EIC) IRQ 2 */
  EIC_3_IRQn               = 15, /**< 15 SAME54P20A External Interrupt Controller (EIC) IRQ 3 */
  EIC_4_IRQn               = 16, /**< 16 SAME54P20A External Interrupt Controller (EIC) IRQ 4 */
  EIC_5_IRQn               = 17, /**< 17 SAME54P20A External Interrupt Controller (EIC) IRQ 5 */
  EIC_6_IRQn               = 18, /**< 18 SAME54P20A External Interrupt Controller (EIC) IRQ 6 */
  EIC_7_IRQn               = 19, /**< 19 SAME54P20A External Interrupt Controller (EIC) IRQ 7 */
  EIC_8_IRQn               = 20, /**< 20 SAME54P20A External Interrupt Controller (EIC) IRQ 8 */
  EIC_9_IRQn               = 21, /**< 21 SAME54P20A External Interrupt Controller (EIC) IRQ 9 */
  EIC_10_IRQn              = 22, /**< 22 SAME54P20A External Interrupt Controller (EIC) IRQ 10 */
  EIC_11_IRQn              = 23, /**< 23 SAME54P20A External Interrupt Controller (EIC) IRQ 11 */
  EIC_12_IRQn              = 24, /**< 24 SAME54P20A External Interrupt Controller (EIC) IRQ 12 */
  EIC_13_IRQn              = 25, /**< 25 SAME54P20A External Interrupt Controller (EIC) IRQ 13 */
  EIC_14_IRQn              = 26, /**< 26 SAME54P20A External Interrupt Controller (EIC) IRQ 14 */
  EIC_15_IRQn              = 27, /**< 27 SAME54P20A External Interrupt Controller (EIC) IRQ 15 */
  FREQM_IRQn               = 28, /**< 28 SAME54P20A Frequency Meter (FREQM) */
  NVMCTRL_0_IRQn           = 29, /**< 29 SAME54P20A Non-Volatile Memory Controller (NVMCTRL) IRQ 0 */
  NVMCTRL_1_IRQn           = 30, /**< 30 SAME54P20A Non-Volatile Memory Controller (NVMCTRL) IRQ 1 */
  DMAC_0_IRQn              = 31, /**< 31 SAME54P20A Direct Memory Access Controller (DMAC) IRQ 0 */
  DMAC_1_IRQn              = 32, /**< 32 SAME54P20A Direct Memory Access Controller (DMAC) IRQ 1 */
  DMAC_2_IRQn              = 33, /**< 33 SAME54P20A Direct Memory Access Controller (DMAC) IRQ 2 */
  DMAC_3_IRQn              = 34, /**< 34 SAME54P20A Direct Memory Access Controller (DMAC) IRQ 3 */
  DMAC_4_IRQn              = 35, /**< 35 SAME54P20A Direct Memory Access Controller (DMAC) IRQ 4 */
  EVSYS_0_IRQn             = 36, /**< 36 SAME54P20A Event System Interface (EVSYS) IRQ 0 */
  EVSYS_1_IRQn             = 37, /**< 37 SAME54P20A Event System Interface (EVSYS) IRQ 1 */
  EVSYS_2_IRQn             = 38, /**< 38 SAME54P20A Event System Interface (EVSYS) IRQ 2 */
  EVSYS_3_IRQn             = 39, /**< 39 SAME54P20A Event System Interface (EVSYS) IRQ 3 */
  EVSYS_4_IRQn             = 40, /**< 40 SAME54P20A Event System Interface (EVSYS) IRQ 4 */
  PAC_IRQn                 = 41, /**< 41 SAME54P20A Peripheral Access Controller (PAC) */
  RAMECC_IRQn              = 45, /**< 45 SAME54P20A RAM ECC (RAMECC) */
  SERCOM0_0_IRQn           = 46, /**< 46 SAME54P20A Serial Communication Interface 0 (SERCOM0) IRQ 0 */
  SERCOM0_1_IRQn           = 47, /**< 47 SAME54P20A Serial Communication Interface 0 (SERCOM0) IRQ 1 */
  SERCOM0_2_IRQn           = 48, /**< 48 SAME54P20A Serial Communication Interface 0 (SERCOM0) IRQ 2 */
  SERCOM0_3_IRQn           = 49, /**< 49 SAME54P20A Serial Communication Interface 0 (SERCOM0) IRQ 3 */
  SERCOM1_0_IRQn           = 50, /**< 50 SAME54P20A Serial Communication Interface 1 (SERCOM1) IRQ 0 */
  SERCOM1_1_IRQn           = 51, /**< 51 SAME54P20A Serial Communication Interface 1 (SERCOM1) IRQ 1 */
  SERCOM1_2_IRQn           = 52, /**< 52 SAME54P20A Serial Communication Interface 1 (SERCOM1) IRQ 2 */
  SERCOM1_3_IRQn           = 53, /**< 53 SAME54P20A Serial Communication Interface 1 (SERCOM1) IRQ 3 */
  SERCOM2_0_IRQn           = 54, /**< 54 SAME54P20A Serial Communication Interface 2 (SERCOM2) IRQ 0 */
  SERCOM2_1_IRQn           = 55, /**< 55 SAME54P20A Serial Communication Interface 2 (SERCOM2) IRQ 1 */
  SERCOM2_2_IRQn           = 56, /**< 56 SAME54P20A Serial Communication Interface 2 (SERCOM2) IRQ 2 */
  SERCOM2_3_IRQn           = 57, /**< 57 SAME54P20A Serial Communication Interface 2 (SERCOM2) IRQ 3 */
  SERCOM3_0_IRQn           = 58, /**< 58 SAME54P20A Serial Communication Interface 3 (SERCOM3) IRQ 0 */
  SERCOM3_1_IRQn           = 59, /**< 59 SAME54P20A Serial Communication Interface 3 (SERCOM3) IRQ 1 */
  SERCOM3_2_IRQn           = 60, /**< 60 SAME54P20A Serial Communication Interface 3 (SERCOM3) IRQ 2 */
  SERCOM3_3_IRQn           = 61, /**< 61 SAME54P20A Serial Communication Interface 3 (SERCOM3) IRQ 3 */
  SERCOM4_0_IRQn           = 62, /**< 62 SAME54P20A Serial Communication Interface 4 (SERCOM4) IRQ 0 */
  SERCOM4_1_IRQn           = 63, /**< 63 SAME54P20A Serial Communication Interface 4 (SERCOM4) IRQ 1 */
  SERCOM4_2_IRQn           = 64, /**< 64 SAME54P20A Serial Communication Interface 4 (SERCOM4) IRQ 2 */
  SERCOM4_3_IRQn           = 65, /**< 65 SAME54P20A Serial Communication Interface 4 (SERCOM4) IRQ 3 */
  SERCOM5_0_IRQn           = 66, /**< 66 SAME54P20A Serial Communication Interface 5 (SERCOM5) IRQ 0 */
  SERCOM5_1_IRQn           = 67, /**< 67 SAME54P20A Serial Communication Interface 5 (SERCOM5) IRQ 1 */
  SERCOM5_2_IRQn           = 68, /**< 68 SAME54P20A Serial Communication Interface 5 (SERCOM5) IRQ 2 */
  SERCOM5_3_IRQn           = 69, /**< 69 SAME54P20A Serial Communication Interface 5 (SERCOM5) IRQ 3 */
  SERCOM6_0_IRQn           = 70, /**< 70 SAME54P20A Serial Communication Interface 6 (SERCOM6) IRQ 0 */
  SERCOM6_1_IRQn           = 71, /**< 71 SAME54P20A Serial Communication Interface 6 (SERCOM6) IRQ 1 */
  SERCOM6_2_IRQn           = 72, /**< 72 SAME54P20A Serial Communication Interface 6 (SERCOM6) IRQ 2 */
  SERCOM6_3_IRQn           = 73, /**< 73 SAME54P20A Serial Communication Interface 6 (SERCOM6) IRQ 3 */
  SERCOM7_0_IRQn           = 74, /**< 74 SAME54P20A Serial Communication Interface 7 (SERCOM7) IRQ 0 */
  SERCOM7_1_IRQn           = 75, /**< 75 SAME54P20A Serial Communication Interface 7 (SERCOM7) IRQ 1 */
  SERCOM7_2_IRQn           = 76, /**< 76 SAME54P20A Serial Communication Interface 7 (SERCOM7) IRQ 2 */
  SERCOM7_3_IRQn           = 77, /**< 77 SAME54P20A Serial Communication Interface 7 (SERCOM7) IRQ 3 */
  CAN0_IRQn                = 78, /**< 78 SAME54P20A Control Area Network 0 (CAN0) */
  CAN1_IRQn                = 79, /**< 79 SAME54P20A Control Area Network 1 (CAN1) */
  USB_0_IRQn               = 80, /**< 80 SAME54P20A Universal Serial Bus (USB) IRQ 0 */
  USB_1_IRQn               = 81, /**< 81 SAME54P20A Universal Serial Bus (USB) IRQ 1 */
  USB_2_IRQn               = 82, /**< 82 SAME54P20A Universal Serial Bus (USB) IRQ 2 */
  USB_3_IRQn               = 83, /**< 83 SAME54P20A Universal Serial Bus (USB) IRQ 3 */
  GMAC_IRQn                = 84, /**< 84 SAME54P20A Ethernet MAC (GMAC) */
  TCC0_0_IRQn              = 85, /**< 85 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 0 */
  TCC0_1_IRQn              = 86, /**< 86 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 1 */
  TCC0_2_IRQn              = 87, /**< 87 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 2 */
  TCC0_3_IRQn              = 88, /**< 88 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 3 */
  TCC0_4_IRQn              = 89, /**< 89 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 4 */
  TCC0_5_IRQn              = 90, /**< 90 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 5 */
  TCC0_6_IRQn              = 91, /**< 91 SAME54P20A Timer Counter Control 0 (TCC0) IRQ 6 */
  TCC1_0_IRQn              = 92, /**< 92 SAME54P20A Timer Counter Control 1 (TCC1) IRQ 0 */
  TCC1_1_IRQn              = 93, /**< 93 SAME54P20A Timer Counter Control 1 (TCC1) IRQ 1 */
  TCC1_2_IRQn              = 94, /**< 94 SAME54P20A Timer Counter Control 1 (TCC1) IRQ 2 */
  TCC1_3_IRQn              = 95, /**< 95 SAME54P20A Timer Counter Control 1 (TCC1) IRQ 3 */
  TCC1_4_IRQn              = 96, /**< 96 SAME54P20A Timer Counter Control 1 (TCC1) IRQ 4 */
  TCC2_0_IRQn              = 97, /**< 97 SAME54P20A Timer Counter Control 2 (TCC2) IRQ 0 */
  TCC2_1_IRQn              = 98, /**< 98 SAME54P20A Timer Counter Control 2 (TCC2) IRQ 1 */
  TCC2_2_IRQn              = 99, /**< 99 SAME54P20A Timer Counter Control 2 (TCC2) IRQ 2 */
  TCC2_3_IRQn              = 100, /**< 100 SAME54P20A Timer Counter Control 2 (TCC2) IRQ 3 */
  TCC3_0_IRQn              = 101, /**< 101 SAME54P20A Timer Counter Control 3 (TCC3) IRQ 0 */
  TCC3_1_IRQn              = 102, /**< 102 SAME54P20A Timer Counter Control 3 (TCC3) IRQ 1 */
  TCC3_2_IRQn              = 103, /**< 103 SAME54P20A Timer Counter Control 3 (TCC3) IRQ 2 */
  TCC4_0_IRQn              = 104, /**< 104 SAME54P20A Timer Counter Control 4 (TCC4) IRQ 0 */
  TCC4_1_IRQn              = 105, /**< 105 SAME54P20A Timer Counter Control 4 (TCC4) IRQ 1 */
  TCC4_2_IRQn              = 106, /**< 106 SAME54P20A Timer Counter Control 4 (TCC4) IRQ 2 */
  TC0_IRQn                 = 107, /**< 107 SAME54P20A Basic Timer Counter 0 (TC0) */
  TC1_IRQn                 = 108, /**< 108 SAME54P20A Basic Timer Counter 1 (TC1) */
  TC2_IRQn                 = 109, /**< 109 SAME54P20A Basic Timer Counter 2 (TC2) */
  TC3_IRQn                 = 110, /**< 110 SAME54P20A Basic Timer Counter 3 (TC3) */
  TC4_IRQn                 = 111, /**< 111 SAME54P20A Basic Timer Counter 4 (TC4) */
  TC5_IRQn                 = 112, /**< 112 SAME54P20A Basic Timer Counter 5 (TC5) */
  TC6_IRQn                 = 113, /**< 113 SAME54P20A Basic Timer Counter 6 (TC6) */
  TC7_IRQn                 = 114, /**< 114 SAME54P20A Basic Timer Counter 7 (TC7) */
  PDEC_0_IRQn              = 115, /**< 115 SAME54P20A Quadrature Decodeur (PDEC) IRQ 0 */
  PDEC_1_IRQn              = 116, /**< 116 SAME54P20A Quadrature Decodeur (PDEC) IRQ 1 */
  PDEC_2_IRQn              = 117, /**< 117 SAME54P20A Quadrature Decodeur (PDEC) IRQ 2 */
  ADC0_0_IRQn              = 118, /**< 118 SAME54P20A Analog Digital Converter 0 (ADC0) IRQ 0 */
  ADC0_1_IRQn              = 119, /**< 119 SAME54P20A Analog Digital Converter 0 (ADC0) IRQ 1 */
  ADC1_0_IRQn              = 120, /**< 120 SAME54P20A Analog Digital Converter 1 (ADC1) IRQ 0 */
  ADC1_1_IRQn              = 121, /**< 121 SAME54P20A Analog Digital Converter 1 (ADC1) IRQ 1 */
  AC_IRQn                  = 122, /**< 122 SAME54P20A Analog Comparators (AC) */
  DAC_0_IRQn               = 123, /**< 123 SAME54P20A Digital-to-Analog Converter (DAC) IRQ 0 */
  DAC_1_IRQn               = 124, /**< 124 SAME54P20A Digital-to-Analog Converter (DAC) IRQ 1 */
  DAC_2_IRQn               = 125, /**< 125 SAME54P20A Digital-to-Analog Converter (DAC) IRQ 2 */
  DAC_3_IRQn               = 126, /**< 126 SAME54P20A Digital-to-Analog Converter (DAC) IRQ 3 */
  DAC_4_IRQn               = 127, /**< 127 SAME54P20A Digital-to-Analog Converter (DAC) IRQ 4 */
  I2S_IRQn                 = 128, /**< 128 SAME54P20A Inter-IC Sound Interface (I2S) */
  PCC_IRQn                 = 129, /**< 129 SAME54P20A Parallel Capture Controller (PCC) */
  AES_IRQn                 = 130, /**< 130 SAME54P20A Advanced Encryption Standard (AES) */
  TRNG_IRQn                = 131, /**< 131 SAME54P20A True Random Generator (TRNG) */
  ICM_IRQn                 = 132, /**< 132 SAME54P20A Integrity Check Monitor (ICM) */
  PUKCC_IRQn               = 133, /**< 133 SAME54P20A PUblic-Key Cryptography Controller (PUKCC) */
  QSPI_IRQn                = 134, /**< 134 SAME54P20A Quad SPI interface (QSPI) */
  SDHC0_IRQn               = 135, /**< 135 SAME54P20A SD/MMC Host Controller 0 (SDHC0) */
  SDHC1_IRQn               = 136, /**< 136 SAME54P20A SD/MMC Host Controller 1 (SDHC1) */

  PERIPH_COUNT_IRQn        = 137  /**< Number of peripheral IDs */
} IRQn_Type;

typedef struct _DeviceVectors
{
  /* Stack pointer */
  void* pvStack;

  /* Cortex-M handlers */
  void* pfnReset_Handler;
  void* pfnNonMaskableInt_Handler;
  void* pfnHardFault_Handler;
  void* pfnMemManagement_Handler;
  void* pfnBusFault_Handler;
  void* pfnUsageFault_Handler;
  void* pvReservedM9;
  void* pvReservedM8;
  void* pvReservedM7;
  void* pvReservedM6;
  void* pfnSVCall_Handler;
  void* pfnDebugMonitor_Handler;
  void* pvReservedM3;
  void* pfnPendSV_Handler;
  void* pfnSysTick_Handler;

  /* Peripheral handlers */
  void* pfnPM_Handler;                    /*  0 Power Manager */
  void* pfnMCLK_Handler;                  /*  1 Main Clock */
  void* pfnOSCCTRL_0_Handler;             /*  2 Oscillators Control IRQ 0 */
  void* pfnOSCCTRL_1_Handler;             /*  3 Oscillators Control IRQ 1 */
  void* pfnOSCCTRL_2_Handler;             /*  4 Oscillators Control IRQ 2 */
  void* pfnOSCCTRL_3_Handler;             /*  5 Oscillators Control IRQ 3 */
  void* pfnOSCCTRL_4_Handler;             /*  6 Oscillators Control IRQ 4 */
  void* pfnOSC32KCTRL_Handler;            /*  7 32kHz Oscillators Control */
  void* pfnSUPC_0_Handler;                /*  8 Supply Controller IRQ 0 */
  void* pfnSUPC_1_Handler;                /*  9 Supply Controller IRQ 1 */
  void* pfnWDT_Handler;                   /* 10 Watchdog Timer */
  void* pfnRTC_Handler;                   /* 11 Real-Time Counter */
  void* pfnEIC_0_Handler;                 /* 12 External Interrupt Controller IRQ 0 */
  void* pfnEIC_1_Handler;                 /* 13 External Interrupt Controller IRQ 1 */
  void* pfnEIC_2_Handler;                 /* 14 External Interrupt Controller IRQ 2 */
  void* pfnEIC_3_Handler;                 /* 15 External Interrupt Controller IRQ 3 */
  void* pfnEIC_4_Handler;                 /* 16 External Interrupt Controller IRQ 4 */
  void* pfnEIC_5_Handler;                 /* 17 External Interrupt Controller IRQ 5 */
  void* pfnEIC_6_Handler;                 /* 18 External Interrupt Controller IRQ 6 */
  void* pfnEIC_7_Handler;                 /* 19 External Interrupt Controller IRQ 7 */
  void* pfnEIC_8_Handler;                 /* 20 External Interrupt Controller IRQ 8 */
  void* pfnEIC_9_Handler;                 /* 21 External Interrupt Controller IRQ 9 */
  void* pfnEIC_10_Handler;                /* 22 External Interrupt Controller IRQ 10 */
  void* pfnEIC_11_Handler;                /* 23 External Interrupt Controller IRQ 11 */
  void* pfnEIC_12_Handler;                /* 24 External Interrupt Controller IRQ 12 */
  void* pfnEIC_13_Handler;                /* 25 External Interrupt Controller IRQ 13 */
  void* pfnEIC_14_Handler;                /* 26 External Interrupt Controller IRQ 14 */
  void* pfnEIC_15_Handler;                /* 27 External Interrupt Controller IRQ 15 */
  void* pfnFREQM_Handler;                 /* 28 Frequency Meter */
  void* pfnNVMCTRL_0_Handler;             /* 29 Non-Volatile Memory Controller IRQ 0 */
  void* pfnNVMCTRL_1_Handler;             /* 30 Non-Volatile Memory Controller IRQ 1 */
  void* pfnDMAC_0_Handler;                /* 31 Direct Memory Access Controller IRQ 0 */
  void* pfnDMAC_1_Handler;                /* 32 Direct Memory Access Controller IRQ 1 */
  void* pfnDMAC_2_Handler;                /* 33 Direct Memory Access Controller IRQ 2 */
  void* pfnDMAC_3_Handler;                /* 34 Direct Memory Access Controller IRQ 3 */
  void* pfnDMAC_4_Handler;                /* 35 Direct Memory Access Controller IRQ 4 */
  void* pfnEVSYS_0_Handler;               /* 36 Event System Interface IRQ 0 */
  void* pfnEVSYS_1_Handler;               /* 37 Event System Interface IRQ 1 */
  void* pfnEVSYS_2_Handler;               /* 38 Event System Interface IRQ 2 */
  void* pfnEVSYS_3_Handler;               /* 39 Event System Interface IRQ 3 */
  void* pfnEVSYS_4_Handler;               /* 40 Event System Interface IRQ 4 */
  void* pfnPAC_Handler;                   /* 41 Peripheral Access Controller */
  void* pvReserved42;
  void* pvReserved43;
  void* pvReserved44;
  void* pfnRAMECC_Handler;                /* 45 RAM ECC */
  void* pfnSERCOM0_0_Handler;             /* 46 Serial Communication Interface 0 IRQ 0 */
  void* pfnSERCOM0_1_Handler;             /* 47 Serial Communication Interface 0 IRQ 1 */
  void* pfnSERCOM0_2_Handler;             /* 48 Serial Communication Interface 0 IRQ 2 */
  void* pfnSERCOM0_3_Handler;             /* 49 Serial Communication Interface 0 IRQ 3 */
  void* pfnSERCOM1_0_Handler;             /* 50 Serial Communication Interface 1 IRQ 0 */
  void* pfnSERCOM1_1_Handler;             /* 51 Serial Communication Interface 1 IRQ 1 */
  void* pfnSERCOM1_2_Handler;             /* 52 Serial Communication Interface 1 IRQ 2 */
  void* pfnSERCOM1_3_Handler;             /* 53 Serial Communication Interface 1 IRQ 3 */
  void* pfnSERCOM2_0_Handler;             /* 54 Serial Communication Interface 2 IRQ 0 */
  void* pfnSERCOM2_1_Handler;             /* 55 Serial Communication Interface 2 IRQ 1 */
  void* pfnSERCOM2_2_Handler;             /* 56 Serial Communication Interface 2 IRQ 2 */
  void* pfnSERCOM2_3_Handler;             /* 57 Serial Communication Interface 2 IRQ 3 */
  void* pfnSERCOM3_0_Handler;             /* 58 Serial Communication Interface 3 IRQ 0 */
  void* pfnSERCOM3_1_Handler;             /* 59 Serial Communication Interface 3 IRQ 1 */
  void* pfnSERCOM3_2_Handler;             /* 60 Serial Communication Interface 3 IRQ 2 */
  void* pfnSERCOM3_3_Handler;             /* 61 Serial Communication Interface 3 IRQ 3 */
  void* pfnSERCOM4_0_Handler;             /* 62 Serial Communication Interface 4 IRQ 0 */
  void* pfnSERCOM4_1_Handler;             /* 63 Serial Communication Interface 4 IRQ 1 */
  void* pfnSERCOM4_2_Handler;             /* 64 Serial Communication Interface 4 IRQ 2 */
  void* pfnSERCOM4_3_Handler;             /* 65 Serial Communication Interface 4 IRQ 3 */
  void* pfnSERCOM5_0_Handler;             /* 66 Serial Communication Interface 5 IRQ 0 */
  void* pfnSERCOM5_1_Handler;             /* 67 Serial Communication Interface 5 IRQ 1 */
  void* pfnSERCOM5_2_Handler;             /* 68 Serial Communication Interface 5 IRQ 2 */
  void* pfnSERCOM5_3_Handler;             /* 69 Serial Communication Interface 5 IRQ 3 */
  void* pfnSERCOM6_0_Handler;             /* 70 Serial Communication Interface 6 IRQ 0 */
  void* pfnSERCOM6_1_Handler;             /* 71 Serial Communication Interface 6 IRQ 1 */
  void* pfnSERCOM6_2_Handler;             /* 72 Serial Communication Interface 6 IRQ 2 */
  void* pfnSERCOM6_3_Handler;             /* 73 Serial Communication Interface 6 IRQ 3 */
  void* pfnSERCOM7_0_Handler;             /* 74 Serial Communication Interface 7 IRQ 0 */
  void* pfnSERCOM7_1_Handler;             /* 75 Serial Communication Interface 7 IRQ 1 */
  void* pfnSERCOM7_2_Handler;             /* 76 Serial Communication Interface 7 IRQ 2 */
  void* pfnSERCOM7_3_Handler;             /* 77 Serial Communication Interface 7 IRQ 3 */
  void* pfnCAN0_Handler;                  /* 78 Control Area Network 0 */
  void* pfnCAN1_Handler;                  /* 79 Control Area Network 1 */
  void* pfnUSB_0_Handler;                 /* 80 Universal Serial Bus IRQ 0 */
  void* pfnUSB_1_Handler;                 /* 81 Universal Serial Bus IRQ 1 */
  void* pfnUSB_2_Handler;                 /* 82 Universal Serial Bus IRQ 2 */
  void* pfnUSB_3_Handler;                 /* 83 Universal Serial Bus IRQ 3 */
  void* pfnGMAC_Handler;                  /* 84 Ethernet MAC */
  void* pfnTCC0_0_Handler;                /* 85 Timer Counter Control 0 IRQ 0 */
  void* pfnTCC0_1_Handler;                /* 86 Timer Counter Control 0 IRQ 1 */
  void* pfnTCC0_2_Handler;                /* 87 Timer Counter Control 0 IRQ 2 */
  void* pfnTCC0_3_Handler;                /* 88 Timer Counter Control 0 IRQ 3 */
  void* pfnTCC0_4_Handler;                /* 89 Timer Counter Control 0 IRQ 4 */
  void* pfnTCC0_5_Handler;                /* 90 Timer Counter Control 0 IRQ 5 */
  void* pfnTCC0_6_Handler;                /* 91 Timer Counter Control 0 IRQ 6 */
  void* pfnTCC1_0_Handler;                /* 92 Timer Counter Control 1 IRQ 0 */
  void* pfnTCC1_1_Handler;                /* 93 Timer Counter Control 1 IRQ 1 */
  void* pfnTCC1_2_Handler;                /* 94 Timer Counter Control 1 IRQ 2 */
  void* pfnTCC1_3_Handler;                /* 95 Timer Counter Control 1 IRQ 3 */
  void* pfnTCC1_4_Handler;                /* 96 Timer Counter Control 1 IRQ 4 */
  void* pfnTCC2_0_Handler;                /* 97 Timer Counter Control 2 IRQ 0 */
  void* pfnTCC2_1_Handler;                /* 98 Timer Counter Control 2 IRQ 1 */
  void* pfnTCC2_2_Handler;                /* 99 Timer Counter Control 2 IRQ 2 */
  void* pfnTCC2_3_Handler;                /* 100 Timer Counter Control 2 IRQ 3 */
  void* pfnTCC3_0_Handler;                /* 101 Timer Counter Control 3 IRQ 0 */
  void* pfnTCC3_1_Handler;                /* 102 Timer Counter Control 3 IRQ 1 */
  void* pfnTCC3_2_Handler;                /* 103 Timer Counter Control 3 IRQ 2 */
  void* pfnTCC4_0_Handler;                /* 104 Timer Counter Control 4 IRQ 0 */
  void* pfnTCC4_1_Handler;                /* 105 Timer Counter Control 4 IRQ 1 */
  void* pfnTCC4_2_Handler;                /* 106 Timer Counter Control 4 IRQ 2 */
  void* pfnTC0_Handler;                   /* 107 Basic Timer Counter 0 */
  void* pfnTC1_Handler;                   /* 108 Basic Timer Counter 1 */
  void* pfnTC2_Handler;                   /* 109 Basic Timer Counter 2 */
  void* pfnTC3_Handler;                   /* 110 Basic Timer Counter 3 */
  void* pfnTC4_Handler;                   /* 111 Basic Timer Counter 4 */
  void* pfnTC5_Handler;                   /* 112 Basic Timer Counter 5 */
  void* pfnTC6_Handler;                   /* 113 Basic Timer Counter 6 */
  void* pfnTC7_Handler;                   /* 114 Basic Timer Counter 7 */
  void* pfnPDEC_0_Handler;                /* 115 Quadrature Decodeur IRQ 0 */
  void* pfnPDEC_1_Handler;                /* 116 Quadrature Decodeur IRQ 1 */
  void* pfnPDEC_2_Handler;                /* 117 Quadrature Decodeur IRQ 2 */
  void* pfnADC0_0_Handler;                /* 118 Analog Digital Converter 0 IRQ 0 */
  void* pfnADC0_1_Handler;                /* 119 Analog Digital Converter 0 IRQ 1 */
  void* pfnADC1_0_Handler;                /* 120 Analog Digital Converter 1 IRQ 0 */
  void* pfnADC1_1_Handler;                /* 121 Analog Digital Converter 1 IRQ 1 */
  void* pfnAC_Handler;                    /* 122 Analog Comparators */
  void* pfnDAC_0_Handler;                 /* 123 Digital-to-Analog Converter IRQ 0 */
  void* pfnDAC_1_Handler;                 /* 124 Digital-to-Analog Converter IRQ 1 */
  void* pfnDAC_2_Handler;                 /* 125 Digital-to-Analog Converter IRQ 2 */
  void* pfnDAC_3_Handler;                 /* 126 Digital-to-Analog Converter IRQ 3 */
  void* pfnDAC_4_Handler;                 /* 127 Digital-to-Analog Converter IRQ 4 */
  void* pfnI2S_Handler;                   /* 128 Inter-IC Sound Interface */
  void* pfnPCC_Handler;                   /* 129 Parallel Capture Controller */
  void* pfnAES_Handler;                   /* 130 Advanced Encryption Standard */
  void* pfnTRNG_Handler;                  /* 131 True Random Generator */
  void* pfnICM_Handler;                   /* 132 Integrity Check Monitor */
  void* pfnPUKCC_Handler;                 /* 133 PUblic-Key Cryptography Controller */
  void* pfnQSPI_Handler;                  /* 134 Quad SPI interface */
  void* pfnSDHC0_Handler;                 /* 135 SD/MMC Host Controller 0 */
  void* pfnSDHC1_Handler;                 /* 136 SD/MMC Host Controller 1 */
} DeviceVectors;

/* Cortex-M4 processor handlers */
void Reset_Handler               ( void );
void NonMaskableInt_Handler      ( void );
void HardFault_Handler           ( void );
void MemManagement_Handler       ( void );
void BusFault_Handler            ( void );
void UsageFault_Handler          ( void );
void SVCall_Handler              ( void );
void DebugMonitor_Handler        ( void );
void PendSV_Handler              ( void );
void SysTick_Handler             ( void );

/* Peripherals handlers */
void PM_Handler                  ( void );
void MCLK_Handler                ( void );
void OSCCTRL_0_Handler           ( void );
void OSCCTRL_1_Handler           ( void );
void OSCCTRL_2_Handler           ( void );
void OSCCTRL_3_Handler           ( void );
void OSCCTRL_4_Handler           ( void );
void OSC32KCTRL_Handler          ( void );
void SUPC_0_Handler              ( void );
void SUPC_1_Handler              ( void );
void WDT_Handler                 ( void );
void RTC_Handler                 ( void );
void EIC_0_Handler               ( void );
void EIC_1_Handler               ( void );
void EIC_2_Handler               ( void );
void EIC_3_Handler               ( void );
void EIC_4_Handler               ( void );
void EIC_5_Handler               ( void );
void EIC_6_Handler               ( void );
void EIC_7_Handler               ( void );
void EIC_8_Handler               ( void );
void EIC_9_Handler               ( void );
void EIC_10_Handler              ( void );
void EIC_11_Handler              ( void );
void EIC_12_Handler              ( void );
void EIC_13_Handler              ( void );
void EIC_14_Handler              ( void );
void EIC_15_Handler              ( void );
void FREQM_Handler               ( void );
void NVMCTRL_0_Handler           ( void );
void NVMCTRL_1_Handler           ( void );
void DMAC_0_Handler              ( void );
void DMAC_1_Handler              ( void );
void DMAC_2_Handler              ( void );
void DMAC_3_Handler              ( void );
void DMAC_4_Handler              ( void );
void EVSYS_0_Handler             ( void );
void EVSYS_1_Handler             ( void );
void EVSYS_2_Handler             ( void );
void EVSYS_3_Handler             ( void );
void EVSYS_4_Handler             ( void );
void PAC_Handler                 ( void );
void RAMECC_Handler              ( void );
void SERCOM0_0_Handler           ( void );
void SERCOM0_1_Handler           ( void );
void SERCOM0_2_Handler           ( void );
void SERCOM0_3_Handler           ( void );
void SERCOM1_0_Handler           ( void );
void SERCOM1_1_Handler           ( void );
void SERCOM1_2_Handler           ( void );
void SERCOM1_3_Handler           ( void );
void SERCOM2_0_Handler           ( void );
void SERCOM2_1_Handler           ( void );
void SERCOM2_2_Handler           ( void );
void SERCOM2_3_Handler           ( void );
void SERCOM3_0_Handler           ( void );
void SERCOM3_1_Handler           ( void );
void SERCOM3_2_Handler           ( void );
void SERCOM3_3_Handler           ( void );
void SERCOM4_0_Handler           ( void );
void SERCOM4_1_Handler           ( void );
void SERCOM4_2_Handler           ( void );
void SERCOM4_3_Handler           ( void );
void SERCOM5_0_Handler           ( void );
void SERCOM5_1_Handler           ( void );
void SERCOM5_2_Handler           ( void );
void SERCOM5_3_Handler           ( void );
void SERCOM6_0_Handler           ( void );
void SERCOM6_1_Handler           ( void );
void SERCOM6_2_Handler           ( void );
void SERCOM6_3_Handler           ( void );
void SERCOM7_0_Handler           ( void );
void SERCOM7_1_Handler           ( void );
void SERCOM7_2_Handler           ( void );
void SERCOM7_3_Handler           ( void );
void CAN0_Handler                ( void );
void CAN1_Handler                ( void );
void USB_0_Handler               ( void );
void USB_1_Handler               ( void );
void USB_2_Handler               ( void );
void USB_3_Handler               ( void );
void GMAC_Handler                ( void );
void TCC0_0_Handler              ( void );
void TCC0_1_Handler              ( void );
void TCC0_2_Handler              ( void );
void TCC0_3_Handler              ( void );
void TCC0_4_Handler              ( void );
void TCC0_5_Handler              ( void );
void TCC0_6_Handler              ( void );
void TCC1_0_Handler              ( void );
void TCC1_1_Handler              ( void );
void TCC1_2_Handler              ( void );
void TCC1_3_Handler              ( void );
void TCC1_4_Handler              ( void );
void TCC2_0_Handler              ( void );
void TCC2_1_Handler              ( void );
void TCC2_2_Handler              ( void );
void TCC2_3_Handler              ( void );
void TCC3_0_Handler              ( void );
void TCC3_1_Handler              ( void );
void TCC3_2_Handler              ( void );
void TCC4_0_Handler              ( void );
void TCC4_1_Handler              ( void );
void TCC4_2_Handler              ( void );
void TC0_Handler                 ( void );
void TC1_Handler                 ( void );
void TC2_Handler                 ( void );
void TC3_Handler                 ( void );
void TC4_Handler                 ( void );
void TC5_Handler                 ( void );
void TC6_Handler                 ( void );
void TC7_Handler                 ( void );
void PDEC_0_Handler              ( void );
void PDEC_1_Handler              ( void );
void PDEC_2_Handler              ( void );
void ADC0_0_Handler              ( void );
void ADC0_1_Handler              ( void );
void ADC1_0_Handler              ( void );
void ADC1_1_Handler              ( void );
void AC_Handler                  ( void );
void DAC_0_Handler               ( void );
void DAC_1_Handler               ( void );
void DAC_2_Handler               ( void );
void DAC_3_Handler               ( void );
void DAC_4_Handler               ( void );
void I2S_Handler                 ( void );
void PCC_Handler                 ( void );
void AES_Handler                 ( void );
void TRNG_Handler                ( void );
void ICM_Handler                 ( void );
void PUKCC_Handler               ( void );
void QSPI_Handler                ( void );
void SDHC0_Handler               ( void );
void SDHC1_Handler               ( void );

/*
 * \brief Configuration of the Cortex-M4 Processor and Core Peripherals
 */

#define __CM4_REV              1         /*!< Core revision r0p1 */
#define __DEBUG_LVL            3         /*!< Full debug plus DWT data matching */
#define __FPU_PRESENT          1         /*!< FPU present or not */
#define __MPU_PRESENT          1         /*!< MPU present or not */
#define __NVIC_PRIO_BITS       3         /*!< Number of bits used for Priority Levels */
#define __TRACE_LVL            2         /*!< Full trace: ITM, DWT triggers and counters, ETM */
#define __VTOR_PRESENT         1         /*!< VTOR present or not */
#define __Vendor_SysTickConfig 0         /*!< Set to 1 if different SysTick Config is used */

/**
 * \brief CMSIS includes
 */

#include <core_cm4.h>
#if !defined DONT_USE_CMSIS_INIT
#include "system_same54.h"
#endif /* DONT_USE_CMSIS_INIT */

/*@}*/

/* ************************************************************************** */
/**  SOFTWARE PERIPHERAL API DEFINITION FOR SAME54P20A */
/* ************************************************************************** */
/** \defgroup SAME54P20A_api Peripheral Software API */
/*@{*/

#include "component/ac.h"
#include "component/adc.h"
#include "component/aes.h"
#include "component/can.h"
#include "component/ccl.h"
#include "component/cmcc.h"
#include "component/dac.h"
#include "component/dmac.h"
#include "component/dsu.h"
#include "component/eic.h"
#include "component/evsys.h"
#include "component/freqm.h"
#include "component/gclk.h"
#include "component/gmac.h"
#include "component/hmatrixb.h"
#include "component/icm.h"
#include "component/i2s.h"
#include "component/mclk.h"
#include "component/nvmctrl.h"
#include "component/oscctrl.h"
#include "component/osc32kctrl.h"
#include "component/pac.h"
#include "component/pcc.h"
#include "component/pdec.h"
#include "component/pm.h"
#include "component/port.h"
#include "component/qspi.h"
#include "component/ramecc.h"
#include "component/rstc.h"
#include "component/rtc.h"
#include "component/sdhc.h"
#include "component/sercom.h"
#include "component/supc.h"
#include "component/tc.h"
#include "component/tcc.h"
#include "component/trng.h"
#include "component/usb.h"
#include "component/wdt.h"
/*@}*/

/* ************************************************************************** */
/**  REGISTERS ACCESS DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */
/** \defgroup SAME54P20A_reg Registers Access Definitions */
/*@{*/

#include "instance/ac.h"
#include "instance/adc0.h"
#include "instance/adc1.h"
#include "instance/aes.h"
#include "instance/can0.h"
#include "instance/can1.h"
#include "instance/ccl.h"
#include "instance/cmcc.h"
#include "instance/dac.h"
#include "instance/dmac.h"
#include "instance/dsu.h"
#include "instance/eic.h"
#include "instance/evsys.h"
#include "instance/freqm.h"
#include "instance/gclk.h"
#include "instance/gmac.h"
#include "instance/hmatrix.h"
#include "instance/icm.h"
#include "instance/i2s.h"
#include "instance/mclk.h"
#include "instance/nvmctrl.h"
#include "instance/oscctrl.h"
#include "instance/osc32kctrl.h"
#include "instance/pac.h"
#include "instance/pcc.h"
#include "instance/pdec.h"
#include "instance/pm.h"
#include "instance/port.h"
#include "instance/pukcc.h"
#include "instance/qspi.h"
#include "instance/ramecc.h"
#include "instance/rstc.h"
#include "instance/rtc.h"
#include "instance/sdhc0.h"
#include "instance/sdhc1.h"
#include "instance/sercom0.h"
#include "instance/sercom1.h"
#include "instance/sercom2.h"
#include "instance/sercom3.h"
#include "instance/sercom4.h"
#include "instance/sercom5.h"
#include "instance/sercom6.h"
#include "instance/sercom7.h"
#include "instance/supc.h"
#include "instance/tc0.h"
#include "instance/tc1.h"
#include "instance/tc2.h"
#include "instance/tc3.h"
#include "instance/tc4.h"
#include "instance/tc5.h"
#include "instance/tc6.h"
#include "instance/tc7.h"
#include "instance/tcc0.h"
#include "instance/tcc1.h"
#include "instance/tcc2.h"
#include "instance/tcc3.h"
#include "instance/tcc4.h"
#include "instance/trng.h"
#include "instance/usb.h"
#include "instance/wdt.h"
/*@}*/

/* ************************************************************************** */
/**  PERIPHERAL ID DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */
/** \defgroup SAME54P20A_id Peripheral Ids Definitions */
/*@{*/

// Peripheral instances on HPB0 bridge
#define ID_PAC            0 /**< \brief Peripheral Access Controller (PAC) */
#define ID_PM             1 /**< \brief Power Manager (PM) */
#define ID_MCLK           2 /**< \brief Main Clock (MCLK) */
#define ID_RSTC           3 /**< \brief Reset Controller (RSTC) */
#define ID_OSCCTRL        4 /**< \brief Oscillators Control (OSCCTRL) */
#define ID_OSC32KCTRL     5 /**< \brief 32kHz Oscillators Control (OSC32KCTRL) */
#define ID_SUPC           6 /**< \brief Supply Controller (SUPC) */
#define ID_GCLK           7 /**< \brief Generic Clock Generator (GCLK) */
#define ID_WDT            8 /**< \brief Watchdog Timer (WDT) */
#define ID_RTC            9 /**< \brief Real-Time Counter (RTC) */
#define ID_EIC           10 /**< \brief External Interrupt Controller (EIC) */
#define ID_FREQM         11 /**< \brief Frequency Meter (FREQM) */
#define ID_SERCOM0       12 /**< \brief Serial Communication Interface 0 (SERCOM0) */
#define ID_SERCOM1       13 /**< \brief Serial Communication Interface 1 (SERCOM1) */
#define ID_TC0           14 /**< \brief Basic Timer Counter 0 (TC0) */
#define ID_TC1           15 /**< \brief Basic Timer Counter 1 (TC1) */

// Peripheral instances on HPB1 bridge
#define ID_USB           32 /**< \brief Universal Serial Bus (USB) */
#define ID_DSU           33 /**< \brief Device Service Unit (DSU) */
#define ID_NVMCTRL       34 /**< \brief Non-Volatile Memory Controller (NVMCTRL) */
#define ID_CMCC          35 /**< \brief Cortex M Cache Controller (CMCC) */
#define ID_PORT          36 /**< \brief Port Module (PORT) */
#define ID_DMAC          37 /**< \brief Direct Memory Access Controller (DMAC) */
#define ID_HMATRIX       38 /**< \brief HSB Matrix (HMATRIX) */
#define ID_EVSYS         39 /**< \brief Event System Interface (EVSYS) */
#define ID_SERCOM2       41 /**< \brief Serial Communication Interface 2 (SERCOM2) */
#define ID_SERCOM3       42 /**< \brief Serial Communication Interface 3 (SERCOM3) */
#define ID_TCC0          43 /**< \brief Timer Counter Control 0 (TCC0) */
#define ID_TCC1          44 /**< \brief Timer Counter Control 1 (TCC1) */
#define ID_TC2           45 /**< \brief Basic Timer Counter 2 (TC2) */
#define ID_TC3           46 /**< \brief Basic Timer Counter 3 (TC3) */
#define ID_RAMECC        48 /**< \brief RAM ECC (RAMECC) */

// Peripheral instances on HPB2 bridge
#define ID_CAN0          64 /**< \brief Control Area Network 0 (CAN0) */
#define ID_CAN1          65 /**< \brief Control Area Network 1 (CAN1) */
#define ID_GMAC          66 /**< \brief Ethernet MAC (GMAC) */
#define ID_TCC2          67 /**< \brief Timer Counter Control 2 (TCC2) */
#define ID_TCC3          68 /**< \brief Timer Counter Control 3 (TCC3) */
#define ID_TC4           69 /**< \brief Basic Timer Counter 4 (TC4) */
#define ID_TC5           70 /**< \brief Basic Timer Counter 5 (TC5) */
#define ID_PDEC          71 /**< \brief Quadrature Decodeur (PDEC) */
#define ID_AC            72 /**< \brief Analog Comparators (AC) */
#define ID_AES           73 /**< \brief Advanced Encryption Standard (AES) */
#define ID_TRNG          74 /**< \brief True Random Generator (TRNG) */
#define ID_ICM           75 /**< \brief Integrity Check Monitor (ICM) */
#define ID_PUKCC         76 /**< \brief PUblic-Key Cryptography Controller (PUKCC) */
#define ID_QSPI          77 /**< \brief Quad SPI interface (QSPI) */
#define ID_CCL           78 /**< \brief Configurable Custom Logic (CCL) */

// Peripheral instances on HPB3 bridge
#define ID_SERCOM4       96 /**< \brief Serial Communication Interface 4 (SERCOM4) */
#define ID_SERCOM5       97 /**< \brief Serial Communication Interface 5 (SERCOM5) */
#define ID_SERCOM6       98 /**< \brief Serial Communication Interface 6 (SERCOM6) */
#define ID_SERCOM7       99 /**< \brief Serial Communication Interface 7 (SERCOM7) */
#define ID_TCC4         100 /**< \brief Timer Counter Control 4 (TCC4) */
#define ID_TC6          101 /**< \brief Basic Timer Counter 6 (TC6) */
#define ID_TC7          102 /**< \brief Basic Timer Counter 7 (TC7) */
#define ID_ADC0         103 /**< \brief Analog Digital Converter 0 (ADC0) */
#define ID_ADC1         104 /**< \brief Analog Digital Converter 1 (ADC1) */
#define ID_DAC          105 /**< \brief Digital-to-Analog Converter (DAC) */
#define ID_I2S          106 /**< \brief Inter-IC Sound Interface (I2S) */
#define ID_PCC          107 /**< \brief Parallel Capture Controller (PCC) */

// Peripheral instances on AHB (as if on bridge 4)
#define ID_SDHC0        128 /**< \brief SD/MMC Host Controller (SDHC0) */
#define ID_SDHC1        129 /**< \brief SD/MMC Host Controller (SDHC1) */

#define ID_PERIPH_COUNT 130 /**< \brief Max number of peripheral IDs */
/*@}*/

/* ************************************************************************** */
/**  BASE ADDRESS DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */
/** \defgroup SAME54P20A_base Peripheral Base Address Definitions */
/*@{*/

#if defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)
#define AC                            (0x42002000) /**< \brief (AC) APB Base Address */
#define ADC0                          (0x43001C00) /**< \brief (ADC0) APB Base Address */
#define ADC1                          (0x43002000) /**< \brief (ADC1) APB Base Address */
#define AES                           (0x42002400) /**< \brief (AES) APB Base Address */
#define CAN0                          (0x42000000) /**< \brief (CAN0) APB Base Address */
#define CAN1                          (0x42000400) /**< \brief (CAN1) APB Base Address */
#define CCL                           (0x42003800) /**< \brief (CCL) APB Base Address */
#define CMCC                          (0x41006000) /**< \brief (CMCC) APB Base Address */
#define CMCC_AHB                      (0x03000000) /**< \brief (CMCC) AHB Base Address */
#define DAC                           (0x43002400) /**< \brief (DAC) APB Base Address */
#define DMAC                          (0x4100A000) /**< \brief (DMAC) APB Base Address */
#define DSU                           (0x41002000) /**< \brief (DSU) APB Base Address */
#define EIC                           (0x40002800) /**< \brief (EIC) APB Base Address */
#define EVSYS                         (0x4100E000) /**< \brief (EVSYS) APB Base Address */
#define FREQM                         (0x40002C00) /**< \brief (FREQM) APB Base Address */
#define GCLK                          (0x40001C00) /**< \brief (GCLK) APB Base Address */
#define GMAC                          (0x42000800) /**< \brief (GMAC) APB Base Address */
#define HMATRIX                       (0x4100C000) /**< \brief (HMATRIX) APB Base Address */
#define ICM                           (0x42002C00) /**< \brief (ICM) APB Base Address */
#define I2S                           (0x43002800) /**< \brief (I2S) APB Base Address */
#define MCLK                          (0x40000800) /**< \brief (MCLK) APB Base Address */
#define NVMCTRL                       (0x41004000) /**< \brief (NVMCTRL) APB Base Address */
#define NVMCTRL_SW0                   (0x00800080) /**< \brief (NVMCTRL) SW0 Base Address */
#define NVMCTRL_TEMP_LOG              (0x00800100) /**< \brief (NVMCTRL) TEMP_LOG Base Address */
#define NVMCTRL_USER                  (0x00804000) /**< \brief (NVMCTRL) USER Base Address */
#define OSCCTRL                       (0x40001000) /**< \brief (OSCCTRL) APB Base Address */
#define OSC32KCTRL                    (0x40001400) /**< \brief (OSC32KCTRL) APB Base Address */
#define PAC                           (0x40000000) /**< \brief (PAC) APB Base Address */
#define PCC                           (0x43002C00) /**< \brief (PCC) APB Base Address */
#define PDEC                          (0x42001C00) /**< \brief (PDEC) APB Base Address */
#define PM                            (0x40000400) /**< \brief (PM) APB Base Address */
#define PORT                          (0x41008000) /**< \brief (PORT) APB Base Address */
#define PUKCC                         (0x42003000) /**< \brief (PUKCC) APB Base Address */
#define PUKCC_AHB                     (0x02000000) /**< \brief (PUKCC) AHB Base Address */
#define QSPI                          (0x42003400) /**< \brief (QSPI) APB Base Address */
#define QSPI_AHB                      (0x04000000) /**< \brief (QSPI) AHB Base Address */
#define RAMECC                        (0x41020000) /**< \brief (RAMECC) APB Base Address */
#define RSTC                          (0x40000C00) /**< \brief (RSTC) APB Base Address */
#define RTC                           (0x40002400) /**< \brief (RTC) APB Base Address */
#define SDHC0                         (0x45000000) /**< \brief (SDHC0) AHB Base Address */
#define SDHC1                         (0x46000000) /**< \brief (SDHC1) AHB Base Address */
#define SERCOM0                       (0x40003000) /**< \brief (SERCOM0) APB Base Address */
#define SERCOM1                       (0x40003400) /**< \brief (SERCOM1) APB Base Address */
#define SERCOM2                       (0x41012000) /**< \brief (SERCOM2) APB Base Address */
#define SERCOM3                       (0x41014000) /**< \brief (SERCOM3) APB Base Address */
#define SERCOM4                       (0x43000000) /**< \brief (SERCOM4) APB Base Address */
#define SERCOM5                       (0x43000400) /**< \brief (SERCOM5) APB Base Address */
#define SERCOM6                       (0x43000800) /**< \brief (SERCOM6) APB Base Address */
#define SERCOM7                       (0x43000C00) /**< \brief (SERCOM7) APB Base Address */
#define SUPC                          (0x40001800) /**< \brief (SUPC) APB Base Address */
#define TC0                           (0x40003800) /**< \brief (TC0) APB Base Address */
#define TC1                           (0x40003C00) /**< \brief (TC1) APB Base Address */
#define TC2                           (0x4101A000) /**< \brief (TC2) APB Base Address */
#define TC3                           (0x4101C000) /**< \brief (TC3) APB Base Address */
#define TC4                           (0x42001400) /**< \brief (TC4) APB Base Address */
#define TC5                           (0x42001800) /**< \brief (TC5) APB Base Address */
#define TC6                           (0x43001400) /**< \brief (TC6) APB Base Address */
#define TC7                           (0x43001800) /**< \brief (TC7) APB Base Address */
#define TCC0                          (0x41016000) /**< \brief (TCC0) APB Base Address */
#define TCC1                          (0x41018000) /**< \brief (TCC1) APB Base Address */
#define TCC2                          (0x42000C00) /**< \brief (TCC2) APB Base Address */
#define TCC3                          (0x42001000) /**< \brief (TCC3) APB Base Address */
#define TCC4                          (0x43001000) /**< \brief (TCC4) APB Base Address */
#define TRNG                          (0x42002800) /**< \brief (TRNG) APB Base Address */
#define USB                           (0x41000000) /**< \brief (USB) APB Base Address */
#define WDT                           (0x40002000) /**< \brief (WDT) APB Base Address */
#else
#define AC                ((Ac       *)0x42002000UL) /**< \brief (AC) APB Base Address */
#define AC_INST_NUM       1                          /**< \brief (AC) Number of instances */
#define AC_INSTS          { AC }                     /**< \brief (AC) Instances List */

#define ADC0              ((Adc      *)0x43001C00UL) /**< \brief (ADC0) APB Base Address */
#define ADC1              ((Adc      *)0x43002000UL) /**< \brief (ADC1) APB Base Address */
#define ADC_INST_NUM      2                          /**< \brief (ADC) Number of instances */
#define ADC_INSTS         { ADC0, ADC1 }             /**< \brief (ADC) Instances List */

#define AES               ((Aes      *)0x42002400UL) /**< \brief (AES) APB Base Address */
#define AES_INST_NUM      1                          /**< \brief (AES) Number of instances */
#define AES_INSTS         { AES }                    /**< \brief (AES) Instances List */

#define CAN0              ((Can      *)0x42000000UL) /**< \brief (CAN0) APB Base Address */
#define CAN1              ((Can      *)0x42000400UL) /**< \brief (CAN1) APB Base Address */
#define CAN_INST_NUM      2                          /**< \brief (CAN) Number of instances */
#define CAN_INSTS         { CAN0, CAN1 }             /**< \brief (CAN) Instances List */

#define CCL               ((Ccl      *)0x42003800UL) /**< \brief (CCL) APB Base Address */
#define CCL_INST_NUM      1                          /**< \brief (CCL) Number of instances */
#define CCL_INSTS         { CCL }                    /**< \brief (CCL) Instances List */

#define CMCC              ((Cmcc     *)0x41006000UL) /**< \brief (CMCC) APB Base Address */
#define CMCC_AHB                      (0x03000000UL) /**< \brief (CMCC) AHB Base Address */
#define CMCC_INST_NUM     1                          /**< \brief (CMCC) Number of instances */
#define CMCC_INSTS        { CMCC }                   /**< \brief (CMCC) Instances List */

#define DAC               ((Dac      *)0x43002400UL) /**< \brief (DAC) APB Base Address */
#define DAC_INST_NUM      1                          /**< \brief (DAC) Number of instances */
#define DAC_INSTS         { DAC }                    /**< \brief (DAC) Instances List */

#define DMAC              ((Dmac     *)0x4100A000UL) /**< \brief (DMAC) APB Base Address */
#define DMAC_INST_NUM     1                          /**< \brief (DMAC) Number of instances */
#define DMAC_INSTS        { DMAC }                   /**< \brief (DMAC) Instances List */

#define DSU               ((Dsu      *)0x41002000UL) /**< \brief (DSU) APB Base Address */
#define DSU_INST_NUM      1                          /**< \brief (DSU) Number of instances */
#define DSU_INSTS         { DSU }                    /**< \brief (DSU) Instances List */

#define EIC               ((Eic      *)0x40002800UL) /**< \brief (EIC) APB Base Address */
#define EIC_INST_NUM      1                          /**< \brief (EIC) Number of instances */
#define EIC_INSTS         { EIC }                    /**< \brief (EIC) Instances List */

#define EVSYS             ((Evsys    *)0x4100E000UL) /**< \brief (EVSYS) APB Base Address */
#define EVSYS_INST_NUM    1                          /**< \brief (EVSYS) Number of instances */
#define EVSYS_INSTS       { EVSYS }                  /**< \brief (EVSYS) Instances List */

#define FREQM             ((Freqm    *)0x40002C00UL) /**< \brief (FREQM) APB Base Address */
#define FREQM_INST_NUM    1                          /**< \brief (FREQM) Number of instances */
#define FREQM_INSTS       { FREQM }                  /**< \brief (FREQM) Instances List */

#define GCLK              ((Gclk     *)0x40001C00UL) /**< \brief (GCLK) APB Base Address */
#define GCLK_INST_NUM     1                          /**< \brief (GCLK) Number of instances */
#define GCLK_INSTS        { GCLK }                   /**< \brief (GCLK) Instances List */

#define GMAC              ((Gmac     *)0x42000800UL) /**< \brief (GMAC) APB Base Address */
#define GMAC_INST_NUM     1                          /**< \brief (GMAC) Number of instances */
#define GMAC_INSTS        { GMAC }                   /**< \brief (GMAC) Instances List */

#define HMATRIX           ((Hmatrixb *)0x4100C000UL) /**< \brief (HMATRIX) APB Base Address */
#define HMATRIXB_INST_NUM 1                          /**< \brief (HMATRIXB) Number of instances */
#define HMATRIXB_INSTS    { HMATRIX }                /**< \brief (HMATRIXB) Instances List */

#define ICM               ((Icm      *)0x42002C00UL) /**< \brief (ICM) APB Base Address */
#define ICM_INST_NUM      1                          /**< \brief (ICM) Number of instances */
#define ICM_INSTS         { ICM }                    /**< \brief (ICM) Instances List */

#define I2S               ((I2s      *)0x43002800UL) /**< \brief (I2S) APB Base Address */
#define I2S_INST_NUM      1                          /**< \brief (I2S) Number of instances */
#define I2S_INSTS         { I2S }                    /**< \brief (I2S) Instances List */

#define MCLK              ((Mclk     *)0x40000800UL) /**< \brief (MCLK) APB Base Address */
#define MCLK_INST_NUM     1                          /**< \brief (MCLK) Number of instances */
#define MCLK_INSTS        { MCLK }                   /**< \brief (MCLK) Instances List */

#define NVMCTRL           ((Nvmctrl  *)0x41004000UL) /**< \brief (NVMCTRL) APB Base Address */
#define NVMCTRL_SW0                   (0x00800080UL) /**< \brief (NVMCTRL) SW0 Base Address */
#define NVMCTRL_TEMP_LOG              (0x00800100UL) /**< \brief (NVMCTRL) TEMP_LOG Base Address */
#define NVMCTRL_USER                  (0x00804000UL) /**< \brief (NVMCTRL) USER Base Address */
#define NVMCTRL_INST_NUM  1                          /**< \brief (NVMCTRL) Number of instances */
#define NVMCTRL_INSTS     { NVMCTRL }                /**< \brief (NVMCTRL) Instances List */

#define OSCCTRL           ((Oscctrl  *)0x40001000UL) /**< \brief (OSCCTRL) APB Base Address */
#define OSCCTRL_INST_NUM  1                          /**< \brief (OSCCTRL) Number of instances */
#define OSCCTRL_INSTS     { OSCCTRL }                /**< \brief (OSCCTRL) Instances List */

#define OSC32KCTRL        ((Osc32kctrl *)0x40001400UL) /**< \brief (OSC32KCTRL) APB Base Address */
#define OSC32KCTRL_INST_NUM 1                          /**< \brief (OSC32KCTRL) Number of instances */
#define OSC32KCTRL_INSTS  { OSC32KCTRL }             /**< \brief (OSC32KCTRL) Instances List */

#define PAC               ((Pac      *)0x40000000UL) /**< \brief (PAC) APB Base Address */
#define PAC_INST_NUM      1                          /**< \brief (PAC) Number of instances */
#define PAC_INSTS         { PAC }                    /**< \brief (PAC) Instances List */

#define PCC               ((Pcc      *)0x43002C00UL) /**< \brief (PCC) APB Base Address */
#define PCC_INST_NUM      1                          /**< \brief (PCC) Number of instances */
#define PCC_INSTS         { PCC }                    /**< \brief (PCC) Instances List */

#define PDEC              ((Pdec     *)0x42001C00UL) /**< \brief (PDEC) APB Base Address */
#define PDEC_INST_NUM     1                          /**< \brief (PDEC) Number of instances */
#define PDEC_INSTS        { PDEC }                   /**< \brief (PDEC) Instances List */

#define PM                ((Pm       *)0x40000400UL) /**< \brief (PM) APB Base Address */
#define PM_INST_NUM       1                          /**< \brief (PM) Number of instances */
#define PM_INSTS          { PM }                     /**< \brief (PM) Instances List */

#define PORT              ((Port     *)0x41008000UL) /**< \brief (PORT) APB Base Address */
#define PORT_INST_NUM     1                          /**< \brief (PORT) Number of instances */
#define PORT_INSTS        { PORT }                   /**< \brief (PORT) Instances List */

#define PUKCC             ((void     *)0x42003000UL) /**< \brief (PUKCC) APB Base Address */
#define PUKCC_AHB         ((void     *)0x02000000UL) /**< \brief (PUKCC) AHB Base Address */
#define PUKCC_INST_NUM    1                          /**< \brief (PUKCC) Number of instances */
#define PUKCC_INSTS       { PUKCC }                  /**< \brief (PUKCC) Instances List */

#define QSPI              ((Qspi     *)0x42003400UL) /**< \brief (QSPI) APB Base Address */
#define QSPI_AHB                      (0x04000000UL) /**< \brief (QSPI) AHB Base Address */
#define QSPI_INST_NUM     1                          /**< \brief (QSPI) Number of instances */
#define QSPI_INSTS        { QSPI }                   /**< \brief (QSPI) Instances List */

#define RAMECC            ((Ramecc   *)0x41020000UL) /**< \brief (RAMECC) APB Base Address */
#define RAMECC_INST_NUM   1                          /**< \brief (RAMECC) Number of instances */
#define RAMECC_INSTS      { RAMECC }                 /**< \brief (RAMECC) Instances List */

#define RSTC              ((Rstc     *)0x40000C00UL) /**< \brief (RSTC) APB Base Address */
#define RSTC_INST_NUM     1                          /**< \brief (RSTC) Number of instances */
#define RSTC_INSTS        { RSTC }                   /**< \brief (RSTC) Instances List */

#define RTC               ((Rtc      *)0x40002400UL) /**< \brief (RTC) APB Base Address */
#define RTC_INST_NUM      1                          /**< \brief (RTC) Number of instances */
#define RTC_INSTS         { RTC }                    /**< \brief (RTC) Instances List */

#define SDHC0             ((Sdhc     *)0x45000000UL) /**< \brief (SDHC0) AHB Base Address */
#define SDHC1             ((Sdhc     *)0x46000000UL) /**< \brief (SDHC1) AHB Base Address */
#define SDHC_INST_NUM     2                          /**< \brief (SDHC) Number of instances */
#define SDHC_INSTS        { SDHC0, SDHC1 }           /**< \brief (SDHC) Instances List */

#define SERCOM0           ((Sercom   *)0x40003000UL) /**< \brief (SERCOM0) APB Base Address */
#define SERCOM1           ((Sercom   *)0x40003400UL) /**< \brief (SERCOM1) APB Base Address */
#define SERCOM2           ((Sercom   *)0x41012000UL) /**< \brief (SERCOM2) APB Base Address */
#define SERCOM3           ((Sercom   *)0x41014000UL) /**< \brief (SERCOM3) APB Base Address */
#define SERCOM4           ((Sercom   *)0x43000000UL) /**< \brief (SERCOM4) APB Base Address */
#define SERCOM5           ((Sercom   *)0x43000400UL) /**< \brief (SERCOM5) APB Base Address */
#define SERCOM6           ((Sercom   *)0x43000800UL) /**< \brief (SERCOM6) APB Base Address */
#define SERCOM7           ((Sercom   *)0x43000C00UL) /**< \brief (SERCOM7) APB Base Address */
#define SERCOM_INST_NUM   8                          /**< \brief (SERCOM) Number of instances */
#define SERCOM_INSTS      { SERCOM0, SERCOM1, SERCOM2, SERCOM3, SERCOM4, SERCOM5, SERCOM6, SERCOM7 } /**< \brief (SERCOM) Instances List */

#define SUPC              ((Supc     *)0x40001800UL) /**< \brief (SUPC) APB Base Address */
#define SUPC_INST_NUM     1                          /**< \brief (SUPC) Number of instances */
#define SUPC_INSTS        { SUPC }                   /**< \brief (SUPC) Instances List */

#define TC0               ((Tc       *)0x40003800UL) /**< \brief (TC0) APB Base Address */
#define TC1               ((Tc       *)0x40003C00UL) /**< \brief (TC1) APB Base Address */
#define TC2               ((Tc       *)0x4101A000UL) /**< \brief (TC2) APB Base Address */
#define TC3               ((Tc       *)0x4101C000UL) /**< \brief (TC3) APB Base Address */
#define TC4               ((Tc       *)0x42001400UL) /**< \brief (TC4) APB Base Address */
#define TC5               ((Tc       *)0x42001800UL) /**< \brief (TC5) APB Base Address */
#define TC6               ((Tc       *)0x43001400UL) /**< \brief (TC6) APB Base Address */
#define TC7               ((Tc       *)0x43001800UL) /**< \brief (TC7) APB Base Address */
#define TC_INST_NUM       8                          /**< \brief (TC) Number of instances */
#define TC_INSTS          { TC0, TC1, TC2, TC3, TC4, TC5, TC6, TC7 } /**< \brief (TC) Instances List */

#define TCC0              ((Tcc      *)0x41016000UL) /**< \brief (TCC0) APB Base Address */
#define TCC1              ((Tcc      *)0x41018000UL) /**< \brief (TCC1) APB Base Address */
#define TCC2              ((Tcc      *)0x42000C00UL) /**< \brief (TCC2) APB Base Address */
#define TCC3              ((Tcc      *)0x42001000UL) /**< \brief (TCC3) APB Base Address */
#define TCC4              ((Tcc      *)0x43001000UL) /**< \brief (TCC4) APB Base Address */
#define TCC_INST_NUM      5                          /**< \brief (TCC) Number of instances */
#define TCC_INSTS         { TCC0, TCC1, TCC2, TCC3, TCC4 } /**< \brief (TCC) Instances List */

#define TRNG              ((Trng     *)0x42002800UL) /**< \brief (TRNG) APB Base Address */
#define TRNG_INST_NUM     1                          /**< \brief (TRNG) Number of instances */
#define TRNG_INSTS        { TRNG }                   /**< \brief (TRNG) Instances List */

#define USB               ((Usb      *)0x41000000UL) /**< \brief (USB) APB Base Address */
#define USB_INST_NUM      1                          /**< \brief (USB) Number of instances */
#define USB_INSTS         { USB }                    /**< \brief (USB) Instances List */

#define WDT               ((Wdt      *)0x40002000UL) /**< \brief (WDT) APB Base Address */
#define WDT_INST_NUM      1                          /**< \brief (WDT) Number of instances */
#define WDT_INSTS         { WDT }                    /**< \brief (WDT) Instances List */

#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/*@}*/

/* ************************************************************************** */
/**  PORT DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */
/** \defgroup SAME54P20A_port PORT Definitions */
/*@{*/

#include "pio/same54p20a.h"
/*@}*/

/* ************************************************************************** */
/**  MEMORY MAPPING DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */

#define HSRAM_SIZE            _UL_(0x00040000) /* 256 kB */
#define FLASH_SIZE            _UL_(0x00100000) /* 1024 kB */
#define FLASH_PAGE_SIZE       512
#define FLASH_NB_OF_PAGES     2048
#define FLASH_USER_PAGE_SIZE  512
#define BKUPRAM_SIZE          _UL_(0x00002000) /* 8 kB */
#define QSPI_SIZE             _UL_(0x01000000) /* 16384 kB */

#define FLASH_ADDR            _UL_(0x00000000) /**< FLASH base address */
#define CMCC_DATARAM_ADDR     _UL_(0x03000000) /**< CMCC_DATARAM base address */
#define CMCC_DATARAM_SIZE     _UL_(0x00001000) /**< CMCC_DATARAM size */
#define CMCC_TAGRAM_ADDR      _UL_(0x03001000) /**< CMCC_TAGRAM base address */
#define CMCC_TAGRAM_SIZE      _UL_(0x00000400) /**< CMCC_TAGRAM size */
#define CMCC_VALIDRAM_ADDR    _UL_(0x03002000) /**< CMCC_VALIDRAM base address */
#define CMCC_VALIDRAM_SIZE    _UL_(0x00000040) /**< CMCC_VALIDRAM size */
#define HSRAM_ADDR            _UL_(0x20000000) /**< HSRAM base address */
#define HSRAM_ETB_ADDR        _UL_(0x20000000) /**< HSRAM_ETB base address */
#define HSRAM_ETB_SIZE        _UL_(0x00008000) /**< HSRAM_ETB size */
#define HSRAM_RET1_ADDR       _UL_(0x20000000) /**< HSRAM_RET1 base address */
#define HSRAM_RET1_SIZE       _UL_(0x00008000) /**< HSRAM_RET1 size */
#define HPB0_ADDR             _UL_(0x40000000) /**< HPB0 base address */
#define HPB1_ADDR             _UL_(0x41000000) /**< HPB1 base address */
#define HPB2_ADDR             _UL_(0x42000000) /**< HPB2 base address */
#define HPB3_ADDR             _UL_(0x43000000) /**< HPB3 base address */
#define SEEPROM_ADDR          _UL_(0x44000000) /**< SEEPROM base address */
#define BKUPRAM_ADDR          _UL_(0x47000000) /**< BKUPRAM base address */
#define PPB_ADDR              _UL_(0xE0000000) /**< PPB base address */

#define DSU_DID_RESETVALUE    _UL_(0x61840300)
#define ADC0_TOUCH_LINES_NUM  32
#define PORT_GROUPS           4

/* ************************************************************************** */
/**  ELECTRICAL DEFINITIONS FOR SAME54P20A */
/* ************************************************************************** */


#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* SAME54P20A_H */
