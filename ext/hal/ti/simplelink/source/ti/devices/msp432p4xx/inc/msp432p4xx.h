/******************************************************************************
*
* Copyright (C) 2012 - 17 Texas Instruments Incorporated - http://www.ti.com/
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*  Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*
*  Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the
*  distribution.
*
*  Neither the name of Texas Instruments Incorporated nor the names of
*  its contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* MSP432P4XX Register Definitions
*
* This file includes CMSIS compliant component and register definitions
*
* For legacy components the definitions that are compatible with MSP430 code,
* are included with msp432p4xx_classic.h
* 
* With CMSIS definitions, the register defines have been reformatted:
*     ModuleName[ModuleInstance]->RegisterName
*
* Writing to CMSIS bit fields can be done through register level
* or via bitband area access:
*  - ADC14->CTL0 |= ADC14_CTL0_ENC;
*  - BITBAND_PERI(ADC14->CTL0, ADC14_CTL0_ENC_OFS) = 1;
*
* File creation date: 2017-08-03
*
******************************************************************************/

#ifndef __MSP432P4XX_H__
#define __MSP432P4XX_H__

/* Use standard integer types with explicit width */
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define __MSP432_HEADER_VERSION__ 3.202

/* WARNING: The msp432p4xx.h file is indented to be used to rebuild TI Drivers for MSP432 MCUs. Do not use this file to build target code.*/

/* Remap MSP432 intrinsics to ARM equivalents */
#include "msp_compatibility.h"

#ifndef __CMSIS_CONFIG__
#define __CMSIS_CONFIG__

/** @addtogroup MSP432P4XX_Definitions MSP432P4XX Definitions
  This file defines all structures and symbols for MSP432P4XX:
    - components and registers
    - peripheral base address
    - peripheral ID
    - Peripheral definitions
  @{
*/

/******************************************************************************
*                Processor and Core Peripherals                               *
******************************************************************************/
/** @addtogroup MSP432P4XX_CMSIS Device CMSIS Definitions
  Configuration of the Cortex-M4 Processor and Core Peripherals
  @{
*/

/******************************************************************************
* CMSIS-compatible Interrupt Number Definition                                *
******************************************************************************/
typedef enum IRQn
{
  /* Cortex-M4 Processor Exceptions Numbers */
  NonMaskableInt_IRQn         = -14,    /*  2 Non Maskable Interrupt */
  HardFault_IRQn              = -13,    /*  3 Hard Fault Interrupt */
  MemoryManagement_IRQn       = -12,    /*  4 Memory Management Interrupt */
  BusFault_IRQn               = -11,    /*  5 Bus Fault Interrupt */
  UsageFault_IRQn             = -10,    /*  6 Usage Fault Interrupt */
  SVCall_IRQn                 = -5,     /* 11 SV Call Interrupt */
  DebugMonitor_IRQn           = -4,     /* 12 Debug Monitor Interrupt */
  PendSV_IRQn                 = -2,     /* 14 Pend SV Interrupt */
  SysTick_IRQn                = -1,     /* 15 System Tick Interrupt */
  /*  Peripheral Exceptions Numbers */
  PSS_IRQn                    = 0,     /* 16 PSS Interrupt             */
  CS_IRQn                     = 1,     /* 17 CS Interrupt              */
  PCM_IRQn                    = 2,     /* 18 PCM Interrupt             */
  WDT_A_IRQn                  = 3,     /* 19 WDT_A Interrupt           */
  FPU_IRQn                    = 4,     /* 20 FPU Interrupt             */
  FLCTL_IRQn                  = 5,     /* 21 Flash Controller Interrupt*/
  COMP_E0_IRQn                = 6,     /* 22 COMP_E0 Interrupt         */
  COMP_E1_IRQn                = 7,     /* 23 COMP_E1 Interrupt         */
  TA0_0_IRQn                  = 8,     /* 24 TA0_0 Interrupt           */
  TA0_N_IRQn                  = 9,     /* 25 TA0_N Interrupt           */
  TA1_0_IRQn                  = 10,     /* 26 TA1_0 Interrupt           */
  TA1_N_IRQn                  = 11,     /* 27 TA1_N Interrupt           */
  TA2_0_IRQn                  = 12,     /* 28 TA2_0 Interrupt           */
  TA2_N_IRQn                  = 13,     /* 29 TA2_N Interrupt           */
  TA3_0_IRQn                  = 14,     /* 30 TA3_0 Interrupt           */
  TA3_N_IRQn                  = 15,     /* 31 TA3_N Interrupt           */
  EUSCIA0_IRQn                = 16,     /* 32 EUSCIA0 Interrupt         */
  EUSCIA1_IRQn                = 17,     /* 33 EUSCIA1 Interrupt         */
  EUSCIA2_IRQn                = 18,     /* 34 EUSCIA2 Interrupt         */
  EUSCIA3_IRQn                = 19,     /* 35 EUSCIA3 Interrupt         */
  EUSCIB0_IRQn                = 20,     /* 36 EUSCIB0 Interrupt         */
  EUSCIB1_IRQn                = 21,     /* 37 EUSCIB1 Interrupt         */
  EUSCIB2_IRQn                = 22,     /* 38 EUSCIB2 Interrupt         */
  EUSCIB3_IRQn                = 23,     /* 39 EUSCIB3 Interrupt         */
  ADC14_IRQn                  = 24,     /* 40 ADC14 Interrupt           */
  T32_INT1_IRQn               = 25,     /* 41 T32_INT1 Interrupt        */
  T32_INT2_IRQn               = 26,     /* 42 T32_INT2 Interrupt        */
  T32_INTC_IRQn               = 27,     /* 43 T32_INTC Interrupt        */
  AES256_IRQn                 = 28,     /* 44 AES256 Interrupt          */
  RTC_C_IRQn                  = 29,     /* 45 RTC_C Interrupt           */
  DMA_ERR_IRQn                = 30,     /* 46 DMA_ERR Interrupt         */
  DMA_INT3_IRQn               = 31,     /* 47 DMA_INT3 Interrupt        */
  DMA_INT2_IRQn               = 32,     /* 48 DMA_INT2 Interrupt        */
  DMA_INT1_IRQn               = 33,     /* 49 DMA_INT1 Interrupt        */
  DMA_INT0_IRQn               = 34,     /* 50 DMA_INT0 Interrupt        */
  PORT1_IRQn                  = 35,     /* 51 Port1 Interrupt           */
  PORT2_IRQn                  = 36,     /* 52 Port2 Interrupt           */
  PORT3_IRQn                  = 37,     /* 53 Port3 Interrupt           */
  PORT4_IRQn                  = 38,     /* 54 Port4 Interrupt           */
  PORT5_IRQn                  = 39,     /* 55 Port5 Interrupt           */
  FLCTL_A_IRQn                = 5,     /* 21 Flash Controller Interrupt*/
  PORT6_IRQn                  = 40,     /* 56 Port6 Interrupt           */
  LCD_F_IRQn                  = 41      /* 57 LCD_F Interrupt           */
} IRQn_Type;

/******************************************************************************
* Processor and Core Peripheral Section                                       *
******************************************************************************/
#define __CM4_REV               0x0001    /* Core revision r0p1 */
#define __MPU_PRESENT           1         /* MPU present or not */
#define __NVIC_PRIO_BITS        3         /* Number of Bits used for Prio Levels */
#define __Vendor_SysTickConfig  0         /* Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT           1         /* FPU present or not */

/******************************************************************************
* Available Peripherals                                                       *
******************************************************************************/
#define __MCU_HAS_ADC14__                                                        /*!< Module ADC14 is available */
#define __MCU_HAS_AES256__                                                       /*!< Module AES256 is available */
#define __MCU_HAS_CAPTIO0__                                                      /*!< Module CAPTIO0 is available */
#define __MCU_HAS_CAPTIO1__                                                      /*!< Module CAPTIO1 is available */
#define __MCU_HAS_COMP_E0__                                                      /*!< Module COMP_E0 is available */
#define __MCU_HAS_COMP_E1__                                                      /*!< Module COMP_E1 is available */
#define __MCU_HAS_CRC32__                                                        /*!< Module CRC32 is available */
#define __MCU_HAS_CS__                                                           /*!< Module CS is available */
#define __MCU_HAS_DIO__                                                          /*!< Module DIO is available */
#define __MCU_HAS_DMA__                                                          /*!< Module DMA is available */
#define __MCU_HAS_EUSCI_A0__                                                     /*!< Module EUSCI_A0 is available */
#define __MCU_HAS_EUSCI_A1__                                                     /*!< Module EUSCI_A1 is available */
#define __MCU_HAS_EUSCI_A2__                                                     /*!< Module EUSCI_A2 is available */
#define __MCU_HAS_EUSCI_A3__                                                     /*!< Module EUSCI_A3 is available */
#define __MCU_HAS_EUSCI_B0__                                                     /*!< Module EUSCI_B0 is available */
#define __MCU_HAS_EUSCI_B1__                                                     /*!< Module EUSCI_B1 is available */
#define __MCU_HAS_EUSCI_B2__                                                     /*!< Module EUSCI_B2 is available */
#define __MCU_HAS_EUSCI_B3__                                                     /*!< Module EUSCI_B3 is available */
#define __MCU_HAS_FLCTL__                                                        /*!< Module FLCTL is available */
#define __MCU_HAS_FL_BOOTOVER_MAILBOX__                                          /*!< Module FL_BOOTOVER_MAILBOX is available */
#define __MCU_HAS_PCM__                                                          /*!< Module PCM is available */
#define __MCU_HAS_PMAP__                                                         /*!< Module PMAP is available */
#define __MCU_HAS_PSS__                                                          /*!< Module PSS is available */
#define __MCU_HAS_REF_A__                                                        /*!< Module REF_A is available */
#define __MCU_HAS_RSTCTL__                                                       /*!< Module RSTCTL is available */
#define __MCU_HAS_RTC_C__                                                        /*!< Module RTC_C is available */
#define __MCU_HAS_SYSCTL__                                                       /*!< Module SYSCTL is available */
#define __MCU_HAS_TIMER32__                                                      /*!< Module TIMER32 is available */
#define __MCU_HAS_TIMER_A0__                                                     /*!< Module TIMER_A0 is available */
#define __MCU_HAS_TIMER_A1__                                                     /*!< Module TIMER_A1 is available */
#define __MCU_HAS_TIMER_A2__                                                     /*!< Module TIMER_A2 is available */
#define __MCU_HAS_TIMER_A3__                                                     /*!< Module TIMER_A3 is available */
#define __MCU_HAS_TLV__                                                          /*!< Module TLV is available */
#define __MCU_HAS_WDT_A__                                                        /*!< Module WDT_A is available */
#define __MCU_HAS_FLCTL_A__                                                      /*!< Module FLCTL_A is available */
#define __MCU_HAS_LCD_F__                                                        /*!< Module LCD_F is available */
#define __MCU_HAS_SYSCTL_A__                                                     /*!< Module SYSCTL_A is available */

#define __MSP432_HAS_PORTA_R__
#define __MSP432_HAS_PORTB_R__
#define __MSP432_HAS_PORTC_R__
#define __MSP432_HAS_PORTD_R__
#define __MSP432_HAS_PORTE_R__
#define __MSP432_HAS_PORTJ_R__
#define __MSP432_HAS_PORT1_R__
#define __MSP432_HAS_PORT2_R__
#define __MSP432_HAS_PORT3_R__
#define __MSP432_HAS_PORT4_R__
#define __MSP432_HAS_PORT5_R__
#define __MSP432_HAS_PORT6_R__
#define __MSP432_HAS_PORT7_R__
#define __MSP432_HAS_PORT8_R__
#define __MSP432_HAS_PORT9_R__
#define __MSP432_HAS_PORT10_R__


/*@}*/ /* end of group MSP432P4XX_CMSIS */

/* Include CMSIS Cortex-M4 Core Peripheral Access Layer Header File */
#ifdef __TI_ARM__
/* disable the TI ULP advisor check for the core header file definitions */
#pragma diag_push
#pragma CHECK_ULP("none")
#include "core_cm4.h"
#pragma diag_pop
#else
#include "core_cm4.h"
#endif

/* System Header */
#include "system_msp432p401r.h"

/******************************************************************************
* Definition of standard bits                                                 *
******************************************************************************/
#define BIT0                                     (uint16_t)(0x0001)
#define BIT1                                     (uint16_t)(0x0002)
#define BIT2                                     (uint16_t)(0x0004)
#define BIT3                                     (uint16_t)(0x0008)
#define BIT4                                     (uint16_t)(0x0010)
#define BIT5                                     (uint16_t)(0x0020)
#define BIT6                                     (uint16_t)(0x0040)
#define BIT7                                     (uint16_t)(0x0080)
#define BIT8                                     (uint16_t)(0x0100)
#define BIT9                                     (uint16_t)(0x0200)
#define BITA                                     (uint16_t)(0x0400)
#define BITB                                     (uint16_t)(0x0800)
#define BITC                                     (uint16_t)(0x1000)
#define BITD                                     (uint16_t)(0x2000)
#define BITE                                     (uint16_t)(0x4000)
#define BITF                                     (uint16_t)(0x8000)

/******************************************************************************
* Device and peripheral memory map                                            *
******************************************************************************/
/** @addtogroup MSP432P4XX_MemoryMap MSP432P4XX Memory Mapping
  @{
*/

#define FLASH_BASE                               ((uint32_t)0x00000000)          /*!< Main Flash memory start address */
#define SRAM_BASE                                ((uint32_t)0x20000000)          /*!< SRAM memory start address */
#define PERIPH_BASE                              ((uint32_t)0x40000000)          /*!< Peripherals start address */
#define PERIPH_BASE2                             ((uint32_t)0xE0000000)          /*!< Peripherals start address */
#define ADC14_BASE                            (PERIPH_BASE +0x00012000)          /*!< Base address of module ADC14 registers */
#define AES256_BASE                           (PERIPH_BASE +0x00003C00)          /*!< Base address of module AES256 registers */
#define CAPTIO0_BASE                          (PERIPH_BASE +0x00005400)          /*!< Base address of module CAPTIO0 registers */
#define CAPTIO1_BASE                          (PERIPH_BASE +0x00005800)          /*!< Base address of module CAPTIO1 registers */
#define COMP_E0_BASE                          (PERIPH_BASE +0x00003400)          /*!< Base address of module COMP_E0 registers */
#define COMP_E1_BASE                          (PERIPH_BASE +0x00003800)          /*!< Base address of module COMP_E1 registers */
#define CRC32_BASE                            (PERIPH_BASE +0x00004000)          /*!< Base address of module CRC32 registers */
#define CS_BASE                               (PERIPH_BASE +0x00010400)          /*!< Base address of module CS registers */
#define DIO_BASE                              (PERIPH_BASE +0x00004C00)          /*!< Base address of module DIO registers */
#define DMA_BASE                              (PERIPH_BASE +0x0000E000)          /*!< Base address of module DMA registers */
#define EUSCI_A0_BASE                         (PERIPH_BASE +0x00001000)          /*!< Base address of module EUSCI_A0 registers */
#define EUSCI_A0_SPI_BASE                     (PERIPH_BASE +0x00001000)          /*!< Base address of module EUSCI_A0 registers */
#define EUSCI_A1_BASE                         (PERIPH_BASE +0x00001400)          /*!< Base address of module EUSCI_A1 registers */
#define EUSCI_A1_SPI_BASE                     (PERIPH_BASE +0x00001400)          /*!< Base address of module EUSCI_A1 registers */
#define EUSCI_A2_BASE                         (PERIPH_BASE +0x00001800)          /*!< Base address of module EUSCI_A2 registers */
#define EUSCI_A2_SPI_BASE                     (PERIPH_BASE +0x00001800)          /*!< Base address of module EUSCI_A2 registers */
#define EUSCI_A3_BASE                         (PERIPH_BASE +0x00001C00)          /*!< Base address of module EUSCI_A3 registers */
#define EUSCI_A3_SPI_BASE                     (PERIPH_BASE +0x00001C00)          /*!< Base address of module EUSCI_A3 registers */
#define EUSCI_B0_BASE                         (PERIPH_BASE +0x00002000)          /*!< Base address of module EUSCI_B0 registers */
#define EUSCI_B0_SPI_BASE                     (PERIPH_BASE +0x00002000)          /*!< Base address of module EUSCI_B0 registers */
#define EUSCI_B1_BASE                         (PERIPH_BASE +0x00002400)          /*!< Base address of module EUSCI_B1 registers */
#define EUSCI_B1_SPI_BASE                     (PERIPH_BASE +0x00002400)          /*!< Base address of module EUSCI_B1 registers */
#define EUSCI_B2_BASE                         (PERIPH_BASE +0x00002800)          /*!< Base address of module EUSCI_B2 registers */
#define EUSCI_B2_SPI_BASE                     (PERIPH_BASE +0x00002800)          /*!< Base address of module EUSCI_B2 registers */
#define EUSCI_B3_BASE                         (PERIPH_BASE +0x00002C00)          /*!< Base address of module EUSCI_B3 registers */
#define EUSCI_B3_SPI_BASE                     (PERIPH_BASE +0x00002C00)          /*!< Base address of module EUSCI_B3 registers */
#define FLCTL_BASE                            (PERIPH_BASE +0x00011000)          /*!< Base address of module FLCTL registers */
#define FL_BOOTOVER_MAILBOX_BASE                 ((uint32_t)0x00200000)          /*!< Base address of module FL_BOOTOVER_MAILBOX registers */
#define PCM_BASE                              (PERIPH_BASE +0x00010000)          /*!< Base address of module PCM registers */
#define PMAP_BASE                             (PERIPH_BASE +0x00005000)          /*!< Base address of module PMAP registers */
#define PSS_BASE                              (PERIPH_BASE +0x00010800)          /*!< Base address of module PSS registers */
#define REF_A_BASE                            (PERIPH_BASE +0x00003000)          /*!< Base address of module REF_A registers */
#define RSTCTL_BASE                           (PERIPH_BASE2+0x00042000)          /*!< Base address of module RSTCTL registers */
#define RTC_C_BASE                            (PERIPH_BASE +0x00004400)          /*!< Base address of module RTC_C registers */
#define RTC_C_BCD_BASE                        (PERIPH_BASE +0x00004400)          /*!< Base address of module RTC_C registers */
#define SYSCTL_BASE                           (PERIPH_BASE2+0x00043000)          /*!< Base address of module SYSCTL registers */
#define TIMER32_BASE                          (PERIPH_BASE +0x0000C000)          /*!< Base address of module TIMER32 registers */
#define TIMER_A0_BASE                         (PERIPH_BASE +0x00000000)          /*!< Base address of module TIMER_A0 registers */
#define TIMER_A1_BASE                         (PERIPH_BASE +0x00000400)          /*!< Base address of module TIMER_A1 registers */
#define TIMER_A2_BASE                         (PERIPH_BASE +0x00000800)          /*!< Base address of module TIMER_A2 registers */
#define TIMER_A3_BASE                         (PERIPH_BASE +0x00000C00)          /*!< Base address of module TIMER_A3 registers */
#define TLV_BASE                                 ((uint32_t)0x00201000)          /*!< Base address of module TLV registers */
#define WDT_A_BASE                            (PERIPH_BASE +0x00004800)          /*!< Base address of module WDT_A registers */
#define TLV_START_ADDR                    (TLV_BASE + 0x0004)                    /*!< Start Address of the TLV structure */
#define FLCTL_A_BASE                          (PERIPH_BASE +0x00011000)          /*!< Base address of module FLCTL_A registers */
#define LCD_F_BASE                            (PERIPH_BASE +0x00012400)          /*!< Base address of module LCD_F registers */
#define SYSCTL_A_BASE                         (PERIPH_BASE2+0x00043000)          /*!< Base address of module SYSCTL_A registers */



/*@}*/ /* end of group MSP432P4XX_MemoryMap */

/******************************************************************************
* Definitions for bit band access                                             *
******************************************************************************/
#define BITBAND_SRAM_BASE                     ((uint32_t)(0x22000000))
#define BITBAND_PERI_BASE                     ((uint32_t)(0x42000000))

/* SRAM allows 32 bit bit band access */
#define BITBAND_SRAM(x, b)  (*((__IO uint32_t *) (BITBAND_SRAM_BASE +  (((uint32_t)(uint32_t *)&(x)) - SRAM_BASE  )*32 + (b)*4)))
/* peripherals with 8 bit or 16 bit register access allow only 8 bit or 16 bit bit band access, so cast to 8 bit always */
#define BITBAND_PERI(x, b)  (*((__IO  uint8_t *) (BITBAND_PERI_BASE +  (((uint32_t)(uint32_t *)&(x)) - PERIPH_BASE)*32 + (b)*4)))

/******************************************************************************
* Peripheral register definitions                                             *
******************************************************************************/
/** @addtogroup MSP432P4XX_Peripherals MSP432P4XX Peripherals
  MSP432P4XX Device Specific Peripheral registers structures
  @{
*/

#if defined ( __CC_ARM )
#pragma anon_unions
#endif



typedef struct {
  __IO uint32_t CTL0;                                                            /*!< Control 0 Register */
  __IO uint32_t CTL1;                                                            /*!< Control 1 Register */
  __IO uint32_t LO0;                                                             /*!< Window Comparator Low Threshold 0 Register */
  __IO uint32_t HI0;                                                             /*!< Window Comparator High Threshold 0 Register */
  __IO uint32_t LO1;                                                             /*!< Window Comparator Low Threshold 1 Register */
  __IO uint32_t HI1;                                                             /*!< Window Comparator High Threshold 1 Register */
  __IO uint32_t MCTL[32];                                                        /*!< Conversion Memory Control Register */
  __IO uint32_t MEM[32];                                                         /*!< Conversion Memory Register */
       uint32_t RESERVED0[9];
  __IO uint32_t IER0;                                                            /*!< Interrupt Enable 0 Register */
  __IO uint32_t IER1;                                                            /*!< Interrupt Enable 1 Register */
  __I  uint32_t IFGR0;                                                           /*!< Interrupt Flag 0 Register */
  __I  uint32_t IFGR1;                                                           /*!< Interrupt Flag 1 Register */
  __O  uint32_t CLRIFGR0;                                                        /*!< Clear Interrupt Flag 0 Register */
  __IO uint32_t CLRIFGR1;                                                        /*!< Clear Interrupt Flag 1 Register */
  __IO uint32_t IV;                                                              /*!< Interrupt Vector Register */
} ADC14_Type;

typedef struct {
  __IO uint16_t CTL0;                                                            /*!< AES Accelerator Control Register 0 */
  __IO uint16_t CTL1;                                                            /*!< AES Accelerator Control Register 1 */
  __IO uint16_t STAT;                                                            /*!< AES Accelerator Status Register */
  __O  uint16_t KEY;                                                             /*!< AES Accelerator Key Register */
  __O  uint16_t DIN;                                                             /*!< AES Accelerator Data In Register */
  __O  uint16_t DOUT;                                                            /*!< AES Accelerator Data Out Register */
  __O  uint16_t XDIN;                                                            /*!< AES Accelerator XORed Data In Register */
  __O  uint16_t XIN;                                                             /*!< AES Accelerator XORed Data In Register */
} AES256_Type;

typedef struct {
       uint16_t RESERVED0[7];
  __IO uint16_t CTL;                                                             /*!< Capacitive Touch IO x Control Register */
} CAPTIO_Type;

typedef struct {
  __IO uint16_t CTL0;                                                            /*!< Comparator Control Register 0 */
  __IO uint16_t CTL1;                                                            /*!< Comparator Control Register 1 */
  __IO uint16_t CTL2;                                                            /*!< Comparator Control Register 2 */
  __IO uint16_t CTL3;                                                            /*!< Comparator Control Register 3 */
       uint16_t RESERVED0[2];
  __IO uint16_t INT;                                                             /*!< Comparator Interrupt Control Register */
  __I  uint16_t IV;                                                              /*!< Comparator Interrupt Vector Word Register */
} COMP_E_Type;

typedef struct {
  __IO uint16_t DI32;                                                            /*!< Data Input for CRC32 Signature Computation */
       uint16_t RESERVED0;
  __IO uint16_t DIRB32;                                                          /*!< Data In Reverse for CRC32 Computation */
       uint16_t RESERVED1;
  __IO uint16_t INIRES32_LO;                                                     /*!< CRC32 Initialization and Result, lower 16 bits */
  __IO uint16_t INIRES32_HI;                                                     /*!< CRC32 Initialization and Result, upper 16 bits */
  __IO uint16_t RESR32_LO;                                                       /*!< CRC32 Result Reverse, lower 16 bits */
  __IO uint16_t RESR32_HI;                                                       /*!< CRC32 Result Reverse, Upper 16 bits */
  __IO uint16_t DI16;                                                            /*!< Data Input for CRC16 computation */
       uint16_t RESERVED2;
  __IO uint16_t DIRB16;                                                          /*!< CRC16 Data In Reverse */
       uint16_t RESERVED3;
  __IO uint16_t INIRES16;                                                        /*!< CRC16 Initialization and Result register */
       uint16_t RESERVED4[2];
  __IO uint16_t RESR16;                                                          /*!< CRC16 Result Reverse */
} CRC32_Type;

typedef struct {
  __IO uint32_t KEY;                                                             /*!< Key Register */
  __IO uint32_t CTL0;                                                            /*!< Control 0 Register */
  __IO uint32_t CTL1;                                                            /*!< Control 1 Register */
  __IO uint32_t CTL2;                                                            /*!< Control 2 Register */
  __IO uint32_t CTL3;                                                            /*!< Control 3 Register */
       uint32_t RESERVED0[7];
  __IO uint32_t CLKEN;                                                           /*!< Clock Enable Register */
  __I  uint32_t STAT;                                                            /*!< Status Register */
       uint32_t RESERVED1[2];
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
       uint32_t RESERVED2;
  __I  uint32_t IFG;                                                             /*!< Interrupt Flag Register */
       uint32_t RESERVED3;
  __O  uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
       uint32_t RESERVED4;
  __O  uint32_t SETIFG;                                                          /*!< Set Interrupt Flag Register */
       uint32_t RESERVED5;
  __IO uint32_t DCOERCAL0;                                                       /*!< DCO External Resistor Cailbration 0 Register */
  __IO uint32_t DCOERCAL1;                                                       /*!< DCO External Resistor Calibration 1 Register */
} CS_Type;

typedef struct {
  uint8_t RESERVED0;
  __I uint8_t IN;                                                                 /*!< Port Input */
  uint8_t RESERVED1;
  __IO uint8_t OUT;                                                               /*!< Port Output */
  uint8_t RESERVED2;
  __IO uint8_t DIR;                                                               /*!< Port Direction */
  uint8_t RESERVED3;
  __IO uint8_t REN;                                                               /*!< Port Resistor Enable */
  uint8_t RESERVED4;
  __IO uint8_t DS;                                                                /*!< Port Drive Strength */
  uint8_t RESERVED5;
  __IO uint8_t SEL0;                                                              /*!< Port Select 0 */
  uint8_t RESERVED6;
  __IO uint8_t SEL1;                                                              /*!< Port Select 1 */
  uint8_t RESERVED7[9];
  __IO uint8_t SELC;                                                              /*!< Port Complement Select */
  uint8_t RESERVED8;
  __IO uint8_t IES;                                                               /*!< Port Interrupt Edge Select */
  uint8_t RESERVED9;
  __IO uint8_t IE;                                                                /*!< Port Interrupt Enable */
  uint8_t RESERVED10;
  __IO uint8_t IFG;                                                               /*!< Port Interrupt Flag */
  __I uint16_t IV;                                                                /*!< Port Interrupt Vector Value */
} DIO_PORT_Even_Interruptable_Type;

typedef struct {
  union {
    __I uint16_t IN;                                                              /*!< Port Pair Input */
    struct {
      __I uint8_t IN_L;                                                           /*!< Low Port Input */
      __I uint8_t IN_H;                                                           /*!< High Port Input */
    };
  };
  union {
    __IO uint16_t OUT;                                                            /*!< Port Pair Output */
    struct {
      __IO uint8_t OUT_L;                                                         /*!< Low Port Output */
      __IO uint8_t OUT_H;                                                         /*!< High Port Output */
    };
  };
  union {
    __IO uint16_t DIR;                                                            /*!< Port Pair Direction */
    struct {
      __IO uint8_t DIR_L;                                                         /*!< Low Port Direction */
      __IO uint8_t DIR_H;                                                         /*!< High Port Direction */
    };
  };
  union {
    __IO uint16_t REN;                                                            /*!< Port Pair Resistor Enable */
    struct {
      __IO uint8_t REN_L;                                                         /*!< Low Port Resistor Enable */
      __IO uint8_t REN_H;                                                         /*!< High Port Resistor Enable */
    };
  };
  union {
    __IO uint16_t DS;                                                             /*!< Port Pair Drive Strength */
    struct {
      __IO uint8_t DS_L;                                                          /*!< Low Port Drive Strength */
      __IO uint8_t DS_H;                                                          /*!< High Port Drive Strength */
    };
  };
  union {
    __IO uint16_t SEL0;                                                           /*!< Port Pair Select 0 */
    struct {
      __IO uint8_t SEL0_L;                                                        /*!< Low Port Select 0 */
      __IO uint8_t SEL0_H;                                                        /*!< High Port Select 0 */
    };
  };
  union {
    __IO uint16_t SEL1;                                                           /*!< Port Pair Select 1 */
    struct {
      __IO uint8_t SEL1_L;                                                        /*!< Low Port Select 1 */
      __IO uint8_t SEL1_H;                                                        /*!< High Port Select 1 */
    };
  };
  __I  uint16_t IV_L;                                                             /*!< Low Port Interrupt Vector Value */
  uint16_t  RESERVED0[3];
  union {
    __IO uint16_t SELC;                                                           /*!< Port Pair Complement Select */
    struct {
      __IO uint8_t SELC_L;                                                        /*!< Low Port Complement Select */
      __IO uint8_t SELC_H;                                                        /*!< High Port Complement Select */
    };
  };
  union {
    __IO uint16_t IES;                                                            /*!< Port Pair Interrupt Edge Select */
    struct {
      __IO uint8_t IES_L;                                                         /*!< Low Port Interrupt Edge Select */
      __IO uint8_t IES_H;                                                         /*!< High Port Interrupt Edge Select */
    };
  };
  union {
    __IO uint16_t IE;                                                             /*!< Port Pair Interrupt Enable */
    struct {
      __IO uint8_t IE_L;                                                          /*!< Low Port Interrupt Enable */
      __IO uint8_t IE_H;                                                          /*!< High Port Interrupt Enable */
    };
  };
  union {
    __IO uint16_t IFG;                                                            /*!< Port Pair Interrupt Flag */
    struct {
      __IO uint8_t IFG_L;                                                         /*!< Low Port Interrupt Flag */
      __IO uint8_t IFG_H;                                                         /*!< High Port Interrupt Flag */
    };
  };
  __I uint16_t IV_H;                                                              /*!< High Port Interrupt Vector Value */
} DIO_PORT_Interruptable_Type;

typedef struct {
  union {
    __I uint16_t IN;                                                              /*!< Port Pair Input */
    struct {
      __I uint8_t IN_L;                                                           /*!< Low Port Input */
      __I uint8_t IN_H;                                                           /*!< High Port Input */
    };
  };
  union {
    __IO uint16_t OUT;                                                            /*!< Port Pair Output */
    struct {
      __IO uint8_t OUT_L;                                                         /*!< Low Port Output */
      __IO uint8_t OUT_H;                                                         /*!< High Port Output */
    };
  };
  union {
    __IO uint16_t DIR;                                                            /*!< Port Pair Direction */
    struct {
      __IO uint8_t DIR_L;                                                         /*!< Low Port Direction */
      __IO uint8_t DIR_H;                                                         /*!< High Port Direction */
    };
  };
  union {
    __IO uint16_t REN;                                                            /*!< Port Pair Resistor Enable */
    struct {
      __IO uint8_t REN_L;                                                         /*!< Low Port Resistor Enable */
      __IO uint8_t REN_H;                                                         /*!< High Port Resistor Enable */
    };
  };
  union {
    __IO uint16_t DS;                                                             /*!< Port Pair Drive Strength */
    struct {
      __IO uint8_t DS_L;                                                          /*!< Low Port Drive Strength */
      __IO uint8_t DS_H;                                                          /*!< High Port Drive Strength */
    };
  };
  union {
    __IO uint16_t SEL0;                                                           /*!< Port Pair Select 0 */
    struct {
      __IO uint8_t SEL0_L;                                                        /*!< Low Port Select 0 */
      __IO uint8_t SEL0_H;                                                        /*!< High Port Select 0 */
    };
  };
  union {
    __IO uint16_t SEL1;                                                           /*!< Port Pair Select 1 */
    struct {
      __IO uint8_t SEL1_L;                                                        /*!< Low Port Select 1 */
      __IO uint8_t SEL1_H;                                                        /*!< High Port Select 1 */
    };
  };
  uint16_t  RESERVED0[4];
  union {
    __IO uint16_t SELC;                                                           /*!< Port Pair Complement Select */
    struct {
      __IO uint8_t SELC_L;                                                        /*!< Low Port Complement Select */
      __IO uint8_t SELC_H;                                                        /*!< High Port Complement Select */
    };
  };
} DIO_PORT_Not_Interruptable_Type;

typedef struct {
  __I uint8_t IN;                                                                 /*!< Port Input */
  uint8_t RESERVED0;
  __IO uint8_t OUT;                                                               /*!< Port Output */
  uint8_t RESERVED1;
  __IO uint8_t DIR;                                                               /*!< Port Direction */
  uint8_t RESERVED2;
  __IO uint8_t REN;                                                               /*!< Port Resistor Enable */
  uint8_t RESERVED3;
  __IO uint8_t DS;                                                                /*!< Port Drive Strength */
  uint8_t RESERVED4;
  __IO uint8_t SEL0;                                                              /*!< Port Select 0 */
  uint8_t RESERVED5;
  __IO uint8_t SEL1;                                                              /*!< Port Select 1 */
  uint8_t RESERVED6;
  __I  uint16_t IV;                                                               /*!< Port Interrupt Vector Value */
  uint8_t RESERVED7[6];
  __IO uint8_t SELC;                                                              /*!< Port Complement Select */
  uint8_t RESERVED8;
  __IO uint8_t IES;                                                               /*!< Port Interrupt Edge Select */
  uint8_t RESERVED9;
  __IO uint8_t IE;                                                                /*!< Port Interrupt Enable */
  uint8_t RESERVED10;
  __IO uint8_t IFG;                                                               /*!< Port Interrupt Flag */
} DIO_PORT_Odd_Interruptable_Type;

typedef struct {
  __I  uint32_t DEVICE_CFG;                                                      /*!< Device Configuration Status */
  __IO uint32_t SW_CHTRIG;                                                       /*!< Software Channel Trigger Register */
       uint32_t RESERVED0[2];
  __IO uint32_t CH_SRCCFG[32];                                                   /*!< Channel n Source Configuration Register */
       uint32_t RESERVED1[28];
  __IO uint32_t INT1_SRCCFG;                                                     /*!< Interrupt 1 Source Channel Configuration */
  __IO uint32_t INT2_SRCCFG;                                                     /*!< Interrupt 2 Source Channel Configuration Register */
  __IO uint32_t INT3_SRCCFG;                                                     /*!< Interrupt 3 Source Channel Configuration Register */
       uint32_t RESERVED2;
  __I  uint32_t INT0_SRCFLG;                                                     /*!< Interrupt 0 Source Channel Flag Register */
  __O  uint32_t INT0_CLRFLG;                                                     /*!< Interrupt 0 Source Channel Clear Flag Register */
} DMA_Channel_Type;

typedef struct {
  __I  uint32_t STAT;                                                            /*!< Status Register */
  __O  uint32_t CFG;                                                             /*!< Configuration Register */
  __IO uint32_t CTLBASE;                                                         /*!< Channel Control Data Base Pointer Register */
  __I  uint32_t ALTBASE;                                                         /*!< Channel Alternate Control Data Base Pointer Register */
  __I  uint32_t WAITSTAT;                                                        /*!< Channel Wait on Request Status Register */
  __O  uint32_t SWREQ;                                                           /*!< Channel Software Request Register */
  __IO uint32_t USEBURSTSET;                                                     /*!< Channel Useburst Set Register */
  __O  uint32_t USEBURSTCLR;                                                     /*!< Channel Useburst Clear Register */
  __IO uint32_t REQMASKSET;                                                      /*!< Channel Request Mask Set Register */
  __O  uint32_t REQMASKCLR;                                                      /*!< Channel Request Mask Clear Register */
  __IO uint32_t ENASET;                                                          /*!< Channel Enable Set Register */
  __O  uint32_t ENACLR;                                                          /*!< Channel Enable Clear Register */
  __IO uint32_t ALTSET;                                                          /*!< Channel Primary-Alternate Set Register */
  __O  uint32_t ALTCLR;                                                          /*!< Channel Primary-Alternate Clear Register */
  __IO uint32_t PRIOSET;                                                         /*!< Channel Priority Set Register */
  __O  uint32_t PRIOCLR;                                                         /*!< Channel Priority Clear Register */
       uint32_t RESERVED4[3];
  __IO uint32_t ERRCLR;                                                          /*!< Bus Error Clear Register */
} DMA_Control_Type;

typedef struct {
  __IO uint16_t CTLW0;                                                           /*!< eUSCI_Ax Control Word Register 0 */
       uint16_t RESERVED0[2];
  __IO uint16_t BRW;                                                             /*!< eUSCI_Ax Bit Rate Control Register 1 */
       uint16_t RESERVED1;
  __IO uint16_t STATW; 
  __I  uint16_t RXBUF;                                                           /*!< eUSCI_Ax Receive Buffer Register */
  __IO uint16_t TXBUF;                                                           /*!< eUSCI_Ax Transmit Buffer Register */
       uint16_t RESERVED2[5];
  __IO uint16_t IE;                                                              /*!< eUSCI_Ax Interrupt Enable Register */
  __IO uint16_t IFG;                                                             /*!< eUSCI_Ax Interrupt Flag Register */
  __I  uint16_t IV;                                                              /*!< eUSCI_Ax Interrupt Vector Register */
} EUSCI_A_SPI_Type;

typedef struct {
  __IO uint16_t CTLW0;                                                           /*!< eUSCI_Ax Control Word Register 0 */
  __IO uint16_t CTLW1;                                                           /*!< eUSCI_Ax Control Word Register 1 */
       uint16_t RESERVED0;
  __IO uint16_t BRW;                                                             /*!< eUSCI_Ax Baud Rate Control Word Register */
  __IO uint16_t MCTLW;                                                           /*!< eUSCI_Ax Modulation Control Word Register */
  __IO uint16_t STATW;                                                           /*!< eUSCI_Ax Status Register */
  __I  uint16_t RXBUF;                                                           /*!< eUSCI_Ax Receive Buffer Register */
  __IO uint16_t TXBUF;                                                           /*!< eUSCI_Ax Transmit Buffer Register */
  __IO uint16_t ABCTL;                                                           /*!< eUSCI_Ax Auto Baud Rate Control Register */
  __IO uint16_t IRCTL;                                                           /*!< eUSCI_Ax IrDA Control Word Register */
       uint16_t RESERVED1[3];
  __IO uint16_t IE;                                                              /*!< eUSCI_Ax Interrupt Enable Register */
  __IO uint16_t IFG;                                                             /*!< eUSCI_Ax Interrupt Flag Register */
  __I  uint16_t IV;                                                              /*!< eUSCI_Ax Interrupt Vector Register */
} EUSCI_A_Type;

typedef struct {
  __IO uint16_t CTLW0;                                                           /*!< eUSCI_Bx Control Word Register 0 */
       uint16_t RESERVED0[2];
  __IO uint16_t BRW;                                                             /*!< eUSCI_Bx Bit Rate Control Register 1 */
  __IO uint16_t STATW; 
       uint16_t RESERVED1;
  __I  uint16_t RXBUF;                                                           /*!< eUSCI_Bx Receive Buffer Register */
  __IO uint16_t TXBUF;                                                           /*!< eUSCI_Bx Transmit Buffer Register */
       uint16_t RESERVED2[13];
  __IO uint16_t IE;                                                              /*!< eUSCI_Bx Interrupt Enable Register */
  __IO uint16_t IFG;                                                             /*!< eUSCI_Bx Interrupt Flag Register */
  __I  uint16_t IV;                                                              /*!< eUSCI_Bx Interrupt Vector Register */
} EUSCI_B_SPI_Type;

typedef struct {
  __IO uint16_t CTLW0;                                                           /*!< eUSCI_Bx Control Word Register 0 */
  __IO uint16_t CTLW1;                                                           /*!< eUSCI_Bx Control Word Register 1 */
       uint16_t RESERVED0;
  __IO uint16_t BRW;                                                             /*!< eUSCI_Bx Baud Rate Control Word Register */
  __IO uint16_t STATW;                                                           /*!< eUSCI_Bx Status Register */
  __IO uint16_t TBCNT;                                                           /*!< eUSCI_Bx Byte Counter Threshold Register */
  __I  uint16_t RXBUF;                                                           /*!< eUSCI_Bx Receive Buffer Register */
  __IO uint16_t TXBUF;                                                           /*!< eUSCI_Bx Transmit Buffer Register */
       uint16_t RESERVED1[2];
  __IO uint16_t I2COA0;                                                          /*!< eUSCI_Bx I2C Own Address 0 Register */
  __IO uint16_t I2COA1;                                                          /*!< eUSCI_Bx I2C Own Address 1 Register */
  __IO uint16_t I2COA2;                                                          /*!< eUSCI_Bx I2C Own Address 2 Register */
  __IO uint16_t I2COA3;                                                          /*!< eUSCI_Bx I2C Own Address 3 Register */
  __I  uint16_t ADDRX;                                                           /*!< eUSCI_Bx I2C Received Address Register */
  __IO uint16_t ADDMASK;                                                         /*!< eUSCI_Bx I2C Address Mask Register */
  __IO uint16_t I2CSA;                                                           /*!< eUSCI_Bx I2C Slave Address Register */
       uint16_t RESERVED2[4];
  __IO uint16_t IE;                                                              /*!< eUSCI_Bx Interrupt Enable Register */
  __IO uint16_t IFG;                                                             /*!< eUSCI_Bx Interrupt Flag Register */
  __I  uint16_t IV;                                                              /*!< eUSCI_Bx Interrupt Vector Register */
} EUSCI_B_Type;

typedef struct {
  __I  uint32_t POWER_STAT;                                                      /*!< Power Status Register */
       uint32_t RESERVED0[3];
  __IO uint32_t BANK0_RDCTL;                                                     /*!< Bank0 Read Control Register */
  __IO uint32_t BANK1_RDCTL;                                                     /*!< Bank1 Read Control Register */
       uint32_t RESERVED1[2];
  __IO uint32_t RDBRST_CTLSTAT;                                                  /*!< Read Burst/Compare Control and Status Register */
  __IO uint32_t RDBRST_STARTADDR;                                                /*!< Read Burst/Compare Start Address Register */
  __IO uint32_t RDBRST_LEN;                                                      /*!< Read Burst/Compare Length Register */
       uint32_t RESERVED2[4];
  __IO uint32_t RDBRST_FAILADDR;                                                 /*!< Read Burst/Compare Fail Address Register */
  __IO uint32_t RDBRST_FAILCNT;                                                  /*!< Read Burst/Compare Fail Count Register */
       uint32_t RESERVED3[3];
  __IO uint32_t PRG_CTLSTAT;                                                     /*!< Program Control and Status Register */
  __IO uint32_t PRGBRST_CTLSTAT;                                                 /*!< Program Burst Control and Status Register */
  __IO uint32_t PRGBRST_STARTADDR;                                               /*!< Program Burst Start Address Register */
       uint32_t RESERVED4;
  __IO uint32_t PRGBRST_DATA0_0;                                                 /*!< Program Burst Data0 Register0 */
  __IO uint32_t PRGBRST_DATA0_1;                                                 /*!< Program Burst Data0 Register1 */
  __IO uint32_t PRGBRST_DATA0_2;                                                 /*!< Program Burst Data0 Register2 */
  __IO uint32_t PRGBRST_DATA0_3;                                                 /*!< Program Burst Data0 Register3 */
  __IO uint32_t PRGBRST_DATA1_0;                                                 /*!< Program Burst Data1 Register0 */
  __IO uint32_t PRGBRST_DATA1_1;                                                 /*!< Program Burst Data1 Register1 */
  __IO uint32_t PRGBRST_DATA1_2;                                                 /*!< Program Burst Data1 Register2 */
  __IO uint32_t PRGBRST_DATA1_3;                                                 /*!< Program Burst Data1 Register3 */
  __IO uint32_t PRGBRST_DATA2_0;                                                 /*!< Program Burst Data2 Register0 */
  __IO uint32_t PRGBRST_DATA2_1;                                                 /*!< Program Burst Data2 Register1 */
  __IO uint32_t PRGBRST_DATA2_2;                                                 /*!< Program Burst Data2 Register2 */
  __IO uint32_t PRGBRST_DATA2_3;                                                 /*!< Program Burst Data2 Register3 */
  __IO uint32_t PRGBRST_DATA3_0;                                                 /*!< Program Burst Data3 Register0 */
  __IO uint32_t PRGBRST_DATA3_1;                                                 /*!< Program Burst Data3 Register1 */
  __IO uint32_t PRGBRST_DATA3_2;                                                 /*!< Program Burst Data3 Register2 */
  __IO uint32_t PRGBRST_DATA3_3;                                                 /*!< Program Burst Data3 Register3 */
  __IO uint32_t ERASE_CTLSTAT;                                                   /*!< Erase Control and Status Register */
  __IO uint32_t ERASE_SECTADDR;                                                  /*!< Erase Sector Address Register */
       uint32_t RESERVED5[2];
  __IO uint32_t BANK0_INFO_WEPROT;                                               /*!< Information Memory Bank0 Write/Erase Protection Register */
  __IO uint32_t BANK0_MAIN_WEPROT;                                               /*!< Main Memory Bank0 Write/Erase Protection Register */
       uint32_t RESERVED6[2];
  __IO uint32_t BANK1_INFO_WEPROT;                                               /*!< Information Memory Bank1 Write/Erase Protection Register */
  __IO uint32_t BANK1_MAIN_WEPROT;                                               /*!< Main Memory Bank1 Write/Erase Protection Register */
       uint32_t RESERVED7[2];
  __IO uint32_t BMRK_CTLSTAT;                                                    /*!< Benchmark Control and Status Register */
  __IO uint32_t BMRK_IFETCH;                                                     /*!< Benchmark Instruction Fetch Count Register */
  __IO uint32_t BMRK_DREAD;                                                      /*!< Benchmark Data Read Count Register */
  __IO uint32_t BMRK_CMP;                                                        /*!< Benchmark Count Compare Register */
       uint32_t RESERVED8[4];
  __IO uint32_t IFG;                                                             /*!< Interrupt Flag Register */
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
  __IO uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
  __IO uint32_t SETIFG;                                                          /*!< Set Interrupt Flag Register */
  __I  uint32_t READ_TIMCTL;                                                     /*!< Read Timing Control Register */
  __I  uint32_t READMARGIN_TIMCTL;                                               /*!< Read Margin Timing Control Register */
  __I  uint32_t PRGVER_TIMCTL;                                                   /*!< Program Verify Timing Control Register */
  __I  uint32_t ERSVER_TIMCTL;                                                   /*!< Erase Verify Timing Control Register */
  __I  uint32_t LKGVER_TIMCTL;                                                   /*!< Leakage Verify Timing Control Register */
  __I  uint32_t PROGRAM_TIMCTL;                                                  /*!< Program Timing Control Register */
  __I  uint32_t ERASE_TIMCTL;                                                    /*!< Erase Timing Control Register */
  __I  uint32_t MASSERASE_TIMCTL;                                                /*!< Mass Erase Timing Control Register */
  __I  uint32_t BURSTPRG_TIMCTL;                                                 /*!< Burst Program Timing Control Register */
       uint32_t RESERVED9[55];
  __IO uint32_t BANK0_MAIN_WEPROT0;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 0 */
  __IO uint32_t BANK0_MAIN_WEPROT1;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 1 */
  __IO uint32_t BANK0_MAIN_WEPROT2;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 2 */
  __IO uint32_t BANK0_MAIN_WEPROT3;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 3 */
  __IO uint32_t BANK0_MAIN_WEPROT4;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 4 */
  __IO uint32_t BANK0_MAIN_WEPROT5;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 5 */
  __IO uint32_t BANK0_MAIN_WEPROT6;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 6 */
  __IO uint32_t BANK0_MAIN_WEPROT7;                                              /*!< Main Memory Bank0 Write/Erase Protection Register 7 */
       uint32_t RESERVED10[8];
  __IO uint32_t BANK1_MAIN_WEPROT0;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 0 */
  __IO uint32_t BANK1_MAIN_WEPROT1;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 1 */
  __IO uint32_t BANK1_MAIN_WEPROT2;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 2 */
  __IO uint32_t BANK1_MAIN_WEPROT3;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 3 */
  __IO uint32_t BANK1_MAIN_WEPROT4;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 4 */
  __IO uint32_t BANK1_MAIN_WEPROT5;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 5 */
  __IO uint32_t BANK1_MAIN_WEPROT6;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 6 */
  __IO uint32_t BANK1_MAIN_WEPROT7;                                              /*!< Main Memory Bank1 Write/Erase Protection Register 7 */
} FLCTL_A_Type;

typedef struct {
  __I  uint32_t POWER_STAT;                                                      /*!< Power Status Register */
       uint32_t RESERVED0[3];
  __IO uint32_t BANK0_RDCTL;                                                     /*!< Bank0 Read Control Register */
  __IO uint32_t BANK1_RDCTL;                                                     /*!< Bank1 Read Control Register */
       uint32_t RESERVED1[2];
  __IO uint32_t RDBRST_CTLSTAT;                                                  /*!< Read Burst/Compare Control and Status Register */
  __IO uint32_t RDBRST_STARTADDR;                                                /*!< Read Burst/Compare Start Address Register */
  __IO uint32_t RDBRST_LEN;                                                      /*!< Read Burst/Compare Length Register */
       uint32_t RESERVED2[4];
  __IO uint32_t RDBRST_FAILADDR;                                                 /*!< Read Burst/Compare Fail Address Register */
  __IO uint32_t RDBRST_FAILCNT;                                                  /*!< Read Burst/Compare Fail Count Register */
       uint32_t RESERVED3[3];
  __IO uint32_t PRG_CTLSTAT;                                                     /*!< Program Control and Status Register */
  __IO uint32_t PRGBRST_CTLSTAT;                                                 /*!< Program Burst Control and Status Register */
  __IO uint32_t PRGBRST_STARTADDR;                                               /*!< Program Burst Start Address Register */
       uint32_t RESERVED4;
  __IO uint32_t PRGBRST_DATA0_0;                                                 /*!< Program Burst Data0 Register0 */
  __IO uint32_t PRGBRST_DATA0_1;                                                 /*!< Program Burst Data0 Register1 */
  __IO uint32_t PRGBRST_DATA0_2;                                                 /*!< Program Burst Data0 Register2 */
  __IO uint32_t PRGBRST_DATA0_3;                                                 /*!< Program Burst Data0 Register3 */
  __IO uint32_t PRGBRST_DATA1_0;                                                 /*!< Program Burst Data1 Register0 */
  __IO uint32_t PRGBRST_DATA1_1;                                                 /*!< Program Burst Data1 Register1 */
  __IO uint32_t PRGBRST_DATA1_2;                                                 /*!< Program Burst Data1 Register2 */
  __IO uint32_t PRGBRST_DATA1_3;                                                 /*!< Program Burst Data1 Register3 */
  __IO uint32_t PRGBRST_DATA2_0;                                                 /*!< Program Burst Data2 Register0 */
  __IO uint32_t PRGBRST_DATA2_1;                                                 /*!< Program Burst Data2 Register1 */
  __IO uint32_t PRGBRST_DATA2_2;                                                 /*!< Program Burst Data2 Register2 */
  __IO uint32_t PRGBRST_DATA2_3;                                                 /*!< Program Burst Data2 Register3 */
  __IO uint32_t PRGBRST_DATA3_0;                                                 /*!< Program Burst Data3 Register0 */
  __IO uint32_t PRGBRST_DATA3_1;                                                 /*!< Program Burst Data3 Register1 */
  __IO uint32_t PRGBRST_DATA3_2;                                                 /*!< Program Burst Data3 Register2 */
  __IO uint32_t PRGBRST_DATA3_3;                                                 /*!< Program Burst Data3 Register3 */
  __IO uint32_t ERASE_CTLSTAT;                                                   /*!< Erase Control and Status Register */
  __IO uint32_t ERASE_SECTADDR;                                                  /*!< Erase Sector Address Register */
       uint32_t RESERVED5[2];
  __IO uint32_t BANK0_INFO_WEPROT;                                               /*!< Information Memory Bank0 Write/Erase Protection Register */
  __IO uint32_t BANK0_MAIN_WEPROT;                                               /*!< Main Memory Bank0 Write/Erase Protection Register */
       uint32_t RESERVED6[2];
  __IO uint32_t BANK1_INFO_WEPROT;                                               /*!< Information Memory Bank1 Write/Erase Protection Register */
  __IO uint32_t BANK1_MAIN_WEPROT;                                               /*!< Main Memory Bank1 Write/Erase Protection Register */
       uint32_t RESERVED7[2];
  __IO uint32_t BMRK_CTLSTAT;                                                    /*!< Benchmark Control and Status Register */
  __IO uint32_t BMRK_IFETCH;                                                     /*!< Benchmark Instruction Fetch Count Register */
  __IO uint32_t BMRK_DREAD;                                                      /*!< Benchmark Data Read Count Register */
  __IO uint32_t BMRK_CMP;                                                        /*!< Benchmark Count Compare Register */
       uint32_t RESERVED8[4];
  __IO uint32_t IFG;                                                             /*!< Interrupt Flag Register */
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
  __IO uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
  __IO uint32_t SETIFG;                                                          /*!< Set Interrupt Flag Register */
  __I  uint32_t READ_TIMCTL;                                                     /*!< Read Timing Control Register */
  __I  uint32_t READMARGIN_TIMCTL;                                               /*!< Read Margin Timing Control Register */
  __I  uint32_t PRGVER_TIMCTL;                                                   /*!< Program Verify Timing Control Register */
  __I  uint32_t ERSVER_TIMCTL;                                                   /*!< Erase Verify Timing Control Register */
  __I  uint32_t LKGVER_TIMCTL;                                                   /*!< Leakage Verify Timing Control Register */
  __I  uint32_t PROGRAM_TIMCTL;                                                  /*!< Program Timing Control Register */
  __I  uint32_t ERASE_TIMCTL;                                                    /*!< Erase Timing Control Register */
  __I  uint32_t MASSERASE_TIMCTL;                                                /*!< Mass Erase Timing Control Register */
  __I  uint32_t BURSTPRG_TIMCTL;                                                 /*!< Burst Program Timing Control Register */
} FLCTL_Type;

typedef struct {
  __IO uint32_t CTL;                                                             /*!< LCD_F control */
  __IO uint32_t BMCTL;                                                           /*!< LCD_F blinking and memory control */
  __IO uint32_t VCTL;                                                            /*!< LCD_F voltage control */
  __IO uint32_t PCTL0;                                                           /*!< LCD_F port control 0 */
  __IO uint32_t PCTL1;                                                           /*!< LCD_F port control 1 */
  __IO uint32_t CSSEL0;                                                          /*!< LCD_F COM/SEG select register 0 */
  __IO uint32_t CSSEL1;                                                          /*!< LCD_F COM/SEG select register 1 */
  __IO uint32_t ANMCTL;                                                          /*!< LCD_F Animation Control Register */
       uint32_t RESERVED0[60];
  __IO uint32_t IE;                                                              /*!< LCD_F interrupt enable register */
  __I  uint32_t IFG;                                                             /*!< LCD_F interrupt flag register */
  __O  uint32_t SETIFG;                                                          /*!< LCD_F set interrupt flag register */
  __O  uint32_t CLRIFG;                                                          /*!< LCD_F clear interrupt flag register */
  __IO uint8_t M[48];                                                           /*!< LCD memory registers */
       uint8_t  RESERVED1[16];
  __IO uint8_t BM[48];                                                          /*!< LCD Blinking memory registers */
       uint8_t  RESERVED2[16];
  __IO uint8_t ANM[8];                                                          /*!< LCD Animation memory registers */
} LCD_F_Type;

typedef struct {
  __IO uint32_t CTL0;                                                            /*!< Control 0 Register */
  __IO uint32_t CTL1;                                                            /*!< Control 1 Register */
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
  __I  uint32_t IFG;                                                             /*!< Interrupt Flag Register */
  __O  uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
} PCM_Type;

typedef struct {
  __IO uint16_t KEYID;
  __IO uint16_t CTL;
} PMAP_COMMON_Type;

typedef struct {
  union {
    __IO uint16_t PMAP_REGISTER[4];
    struct {
      __IO uint8_t PMAP_REGISTER0;
      __IO uint8_t PMAP_REGISTER1;
      __IO uint8_t PMAP_REGISTER2;
      __IO uint8_t PMAP_REGISTER3;
      __IO uint8_t PMAP_REGISTER4;
      __IO uint8_t PMAP_REGISTER5;
      __IO uint8_t PMAP_REGISTER6;
      __IO uint8_t PMAP_REGISTER7;
    };
  };
} PMAP_REGISTER_Type;

typedef struct {
  __IO uint32_t KEY;                                                             /*!< Key Register */
  __IO uint32_t CTL0;                                                            /*!< Control 0 Register */
       uint32_t RESERVED0[11];
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
  __I  uint32_t IFG;                                                             /*!< Interrupt Flag Register */
  __IO uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
} PSS_Type;

typedef struct {
  __IO uint16_t CTL0;                                                            /*!< REF Control Register 0 */
} REF_A_Type;

typedef struct {
  __IO uint32_t RESET_REQ;                                                       /*!< Reset Request Register */
  __I  uint32_t HARDRESET_STAT;                                                  /*!< Hard Reset Status Register */
  __IO uint32_t HARDRESET_CLR;                                                   /*!< Hard Reset Status Clear Register */
  __IO uint32_t HARDRESET_SET;                                                   /*!< Hard Reset Status Set Register */
  __I  uint32_t SOFTRESET_STAT;                                                  /*!< Soft Reset Status Register */
  __IO uint32_t SOFTRESET_CLR;                                                   /*!< Soft Reset Status Clear Register */
  __IO uint32_t SOFTRESET_SET;                                                   /*!< Soft Reset Status Set Register */
       uint32_t RESERVED0[57];
  __I  uint32_t PSSRESET_STAT;                                                   /*!< PSS Reset Status Register */
  __IO uint32_t PSSRESET_CLR;                                                    /*!< PSS Reset Status Clear Register */
  __I  uint32_t PCMRESET_STAT;                                                   /*!< PCM Reset Status Register */
  __IO uint32_t PCMRESET_CLR;                                                    /*!< PCM Reset Status Clear Register */
  __I  uint32_t PINRESET_STAT;                                                   /*!< Pin Reset Status Register */
  __IO uint32_t PINRESET_CLR;                                                    /*!< Pin Reset Status Clear Register */
  __I  uint32_t REBOOTRESET_STAT;                                                /*!< Reboot Reset Status Register */
  __IO uint32_t REBOOTRESET_CLR;                                                 /*!< Reboot Reset Status Clear Register */
  __I  uint32_t CSRESET_STAT;                                                    /*!< CS Reset Status Register */
  __IO uint32_t CSRESET_CLR;                                                     /*!< CS Reset Status Clear Register */
} RSTCTL_Type;

typedef struct {
       uint16_t RESERVED0[8];
  __IO uint16_t TIM0;                                                            /*!< Real-Time Clock Seconds, Minutes Register - BCD Format */
  __IO uint16_t TIM1;                                                            /*!< Real-Time Clock Hour, Day of Week - BCD Format */
  __IO uint16_t DATE;                                                            /*!< Real-Time Clock Date - BCD Format */
  __IO uint16_t YEAR;                                                            /*!< Real-Time Clock Year Register - BCD Format */
  __IO uint16_t AMINHR;                                                          /*!< Real-Time Clock Minutes, Hour Alarm - BCD Format */
  __IO uint16_t ADOWDAY;                                                         /*!< Real-Time Clock Day of Week, Day of Month Alarm - BCD Format */
} RTC_C_BCD_Type;

typedef struct {
  __IO uint16_t CTL0;                                                            /*!< RTCCTL0 Register */
  __IO uint16_t CTL13;                                                           /*!< RTCCTL13 Register */
  __IO uint16_t OCAL;                                                            /*!< RTCOCAL Register */
  __IO uint16_t TCMP;                                                            /*!< RTCTCMP Register */
  __IO uint16_t PS0CTL;                                                          /*!< Real-Time Clock Prescale Timer 0 Control Register */
  __IO uint16_t PS1CTL;                                                          /*!< Real-Time Clock Prescale Timer 1 Control Register */
  __IO uint16_t PS;                                                              /*!< Real-Time Clock Prescale Timer Counter Register */
  __I  uint16_t IV;                                                              /*!< Real-Time Clock Interrupt Vector Register */
  __IO uint16_t TIM0;                                                            /*!< RTCTIM0 Register  Hexadecimal Format */
  __IO uint16_t TIM1;                                                            /*!< Real-Time Clock Hour, Day of Week */
  __IO uint16_t DATE;                                                            /*!< RTCDATE - Hexadecimal Format */
  __IO uint16_t YEAR;                                                            /*!< RTCYEAR Register  Hexadecimal Format */
  __IO uint16_t AMINHR;                                                          /*!< RTCMINHR - Hexadecimal Format */
  __IO uint16_t ADOWDAY;                                                         /*!< RTCADOWDAY - Hexadecimal Format */
  __IO uint16_t BIN2BCD;                                                         /*!< Binary-to-BCD Conversion Register */
  __IO uint16_t BCD2BIN;                                                         /*!< BCD-to-Binary Conversion Register */
} RTC_C_Type;

typedef struct {
  __IO uint32_t SEC_ZONE_SECEN;                                                  /*!< IP Protection Secure Zone Enable. */
  __IO uint32_t SEC_ZONE_START_ADDR;                                             /*!< Start address of IP protected secure zone. */
  __IO uint32_t SEC_ZONE_LENGTH;                                                 /*!< Length of IP protected secure zone in number of bytes. */
  __IO uint32_t SEC_ZONE_AESINIT_VECT[4];                                        /*!< IP protected secure zone 0 AES initialization vector */
  __IO uint32_t SEC_ZONE_SECKEYS[8];                                             /*!< AES-CBC security keys. */
  __IO uint32_t SEC_ZONE_UNENC_PWD[4];                                           /*!< Unencrypted password for authentication. */
  __IO uint32_t SEC_ZONE_ENCUPDATE_EN;                                           /*!< IP Protected Secure Zone Encrypted In-field Update Enable */
  __IO uint32_t SEC_ZONE_DATA_EN;                                                /*!< IP Protected Secure Zone Data Access Enable */
  __IO uint32_t SEC_ZONE_ACK;                                                    /*!< Acknowledgment for IP Protection Secure Zone Enable Command. */
       uint32_t RESERVED0[2];
} SEC_ZONE_PARAMS_Type;

typedef struct {
  __IO uint32_t SEC_ZONE_PAYLOADADDR;                                            /*!< Start address where the payload is loaded in the device. */
  __IO uint32_t SEC_ZONE_PAYLOADLEN;                                             /*!< Length of the payload in bytes. */
  __IO uint32_t SEC_ZONE_UPDATE_ACK;                                             /*!< Acknowledgment for the IP Protected Secure Zone Update Command */
       uint32_t RESERVED0;
} SEC_ZONE_UPDATE_Type;

typedef struct {
  __IO uint32_t MASTER_UNLOCK;                                                   /*!< Master Unlock Register */
  __IO uint32_t BOOTOVER_REQ[2];                                                 /*!< Boot Override Request Register */
  __IO uint32_t BOOTOVER_ACK;                                                    /*!< Boot Override Acknowledge Register */
  __IO uint32_t RESET_REQ;                                                       /*!< Reset Request Register */
  __IO uint32_t RESET_STATOVER;                                                  /*!< Reset Status and Override Register */
       uint32_t RESERVED10[2];
  __I  uint32_t SYSTEM_STAT;                                                     /*!< System Status Register */
} SYSCTL_A_Boot_Type;

typedef struct {
  __IO uint32_t REBOOT_CTL;                                                      /*!< Reboot Control Register */
  __IO uint32_t NMI_CTLSTAT;                                                     /*!< NMI Control and Status Register */
  __IO uint32_t WDTRESET_CTL;                                                    /*!< Watchdog Reset Control Register */
  __IO uint32_t PERIHALT_CTL;                                                    /*!< Peripheral Halt Control Register */
  __I  uint32_t SRAM_SIZE;                                                       /*!< SRAM Size Register */
  __I  uint32_t SRAM_NUMBANKS;                                                   /*!< SRAM Number of Banks Register */
  __I  uint32_t SRAM_NUMBLOCKS;                                                  /*!< SRAM Number of Blocks Register */
       uint32_t RESERVED0;
  __I  uint32_t MAINFLASH_SIZE;                                                  /*!< Flash Main Memory Size Register */
  __I  uint32_t INFOFLASH_SIZE;                                                  /*!< Flash Information Memory Size Register */
       uint32_t RESERVED1[2];
  __IO uint32_t DIO_GLTFLT_CTL;                                                  /*!< Digital I/O Glitch Filter Control Register */
       uint32_t RESERVED2[3];
  __IO uint32_t SECDATA_UNLOCK;                                                  /*!< IP Protected Secure Zone Data Access Unlock Register */
       uint32_t RESERVED3[3];
  __IO uint32_t SRAM_BANKEN_CTL0;                                                /*!< SRAM Bank Enable Control Register 0 */
  __IO uint32_t SRAM_BANKEN_CTL1;                                                /*!< SRAM Bank Enable Control Register 1 */
  __IO uint32_t SRAM_BANKEN_CTL2;                                                /*!< SRAM Bank Enable Control Register 2 */
  __IO uint32_t SRAM_BANKEN_CTL3;                                                /*!< SRAM Bank Enable Control Register 3 */
       uint32_t RESERVED4[4];
  __IO uint32_t SRAM_BLKRET_CTL0;                                                /*!< SRAM Block Retention Control Register 0 */
  __IO uint32_t SRAM_BLKRET_CTL1;                                                /*!< SRAM Block Retention Control Register 1 */
  __IO uint32_t SRAM_BLKRET_CTL2;                                                /*!< SRAM Block Retention Control Register 2 */
  __IO uint32_t SRAM_BLKRET_CTL3;                                                /*!< SRAM Block Retention Control Register 3 */
       uint32_t RESERVED5[4];
  __I  uint32_t SRAM_STAT;                                                       /*!< SRAM Status Register */
} SYSCTL_A_Type;

typedef struct {
  __IO uint32_t MASTER_UNLOCK;                                                   /*!< Master Unlock Register */
  __IO uint32_t BOOTOVER_REQ[2];                                                 /*!< Boot Override Request Register */
  __IO uint32_t BOOTOVER_ACK;                                                    /*!< Boot Override Acknowledge Register */
  __IO uint32_t RESET_REQ;                                                       /*!< Reset Request Register */
  __IO uint32_t RESET_STATOVER;                                                  /*!< Reset Status and Override Register */
       uint32_t RESERVED7[2];
  __I  uint32_t SYSTEM_STAT;                                                     /*!< System Status Register */
} SYSCTL_Boot_Type;

typedef struct {
  __IO uint32_t REBOOT_CTL;                                                      /*!< Reboot Control Register */
  __IO uint32_t NMI_CTLSTAT;                                                     /*!< NMI Control and Status Register */
  __IO uint32_t WDTRESET_CTL;                                                    /*!< Watchdog Reset Control Register */
  __IO uint32_t PERIHALT_CTL;                                                    /*!< Peripheral Halt Control Register */
  __I  uint32_t SRAM_SIZE;                                                       /*!< SRAM Size Register */
  __IO uint32_t SRAM_BANKEN;                                                     /*!< SRAM Bank Enable Register */
  __IO uint32_t SRAM_BANKRET;                                                    /*!< SRAM Bank Retention Control Register */
       uint32_t RESERVED0;
  __I  uint32_t FLASH_SIZE;                                                      /*!< Flash Size Register */
       uint32_t RESERVED1[3];
  __IO uint32_t DIO_GLTFLT_CTL;                                                  /*!< Digital I/O Glitch Filter Control Register */
       uint32_t RESERVED2[3];
  __IO uint32_t SECDATA_UNLOCK;                                                  /*!< IP Protected Secure Zone Data Access Unlock Register */
} SYSCTL_Type;

typedef struct {
  __I  uint32_t TLV_CHECKSUM;                                                    /*!< TLV Checksum */
  __I  uint32_t DEVICE_INFO_TAG;                                                 /*!< Device Info Tag */
  __I  uint32_t DEVICE_INFO_LEN;                                                 /*!< Device Info Length */
  __I  uint32_t DEVICE_ID;                                                       /*!< Device ID */
  __I  uint32_t HWREV;                                                           /*!< HW Revision */
  __I  uint32_t BCREV;                                                           /*!< Boot Code Revision */
  __I  uint32_t ROM_DRVLIB_REV;                                                  /*!< ROM Driver Library Revision */
  __I  uint32_t DIE_REC_TAG;                                                     /*!< Die Record Tag */
  __I  uint32_t DIE_REC_LEN;                                                     /*!< Die Record Length */
  __I  uint32_t DIE_XPOS;                                                        /*!< Die X-Position */
  __I  uint32_t DIE_YPOS;                                                        /*!< Die Y-Position */
  __I  uint32_t WAFER_ID;                                                        /*!< Wafer ID */
  __I  uint32_t LOT_ID;                                                          /*!< Lot ID */
  __I  uint32_t RESERVED0;                                                       /*!< Reserved */
  __I  uint32_t RESERVED1;                                                       /*!< Reserved */
  __I  uint32_t RESERVED2;                                                       /*!< Reserved */
  __I  uint32_t TEST_RESULTS;                                                    /*!< Test Results */
  __I  uint32_t CS_CAL_TAG;                                                      /*!< Clock System Calibration Tag */
  __I  uint32_t CS_CAL_LEN;                                                      /*!< Clock System Calibration Length */
  __I  uint32_t DCOIR_FCAL_RSEL04;                                               /*!< DCO IR mode: Frequency calibration for DCORSEL 0 to 4 */
  __I  uint32_t DCOIR_FCAL_RSEL5;                                                /*!< DCO IR mode: Frequency calibration for DCORSEL 5 */
  __I  uint32_t RESERVED3;                                                       /*!< Reserved */
  __I  uint32_t RESERVED4;                                                       /*!< Reserved */
  __I  uint32_t RESERVED5;                                                       /*!< Reserved */
  __I  uint32_t RESERVED6;                                                       /*!< Reserved */
  __I  uint32_t DCOIR_CONSTK_RSEL04;                                             /*!< DCO IR mode: DCO Constant (K) for DCORSEL 0 to 4 */
  __I  uint32_t DCOIR_CONSTK_RSEL5;                                              /*!< DCO IR mode: DCO Constant (K) for DCORSEL 5 */
  __I  uint32_t DCOER_FCAL_RSEL04;                                               /*!< DCO ER mode: Frequency calibration for DCORSEL 0 to 4 */
  __I  uint32_t DCOER_FCAL_RSEL5;                                                /*!< DCO ER mode: Frequency calibration for DCORSEL 5 */
  __I  uint32_t RESERVED7;                                                       /*!< Reserved */
  __I  uint32_t RESERVED8;                                                       /*!< Reserved */
  __I  uint32_t RESERVED9;                                                       /*!< Reserved */
  __I  uint32_t RESERVED10;                                                      /*!< Reserved */
  __I  uint32_t DCOER_CONSTK_RSEL04;                                             /*!< DCO ER mode: DCO Constant (K) for DCORSEL 0 to 4 */
  __I  uint32_t DCOER_CONSTK_RSEL5;                                              /*!< DCO ER mode: DCO Constant (K) for DCORSEL 5 */
  __I  uint32_t ADC14_CAL_TAG;                                                   /*!< ADC14 Calibration Tag */
  __I  uint32_t ADC14_CAL_LEN;                                                   /*!< ADC14 Calibration Length */
  __I  uint32_t ADC_GAIN_FACTOR;                                                 /*!< ADC Gain Factor */
  __I  uint32_t ADC_OFFSET;                                                      /*!< ADC Offset */
  __I  uint32_t RESERVED11;                                                      /*!< Reserved */
  __I  uint32_t RESERVED12;                                                      /*!< Reserved */
  __I  uint32_t RESERVED13;                                                      /*!< Reserved */
  __I  uint32_t RESERVED14;                                                      /*!< Reserved */
  __I  uint32_t RESERVED15;                                                      /*!< Reserved */
  __I  uint32_t RESERVED16;                                                      /*!< Reserved */
  __I  uint32_t RESERVED17;                                                      /*!< Reserved */
  __I  uint32_t RESERVED18;                                                      /*!< Reserved */
  __I  uint32_t RESERVED19;                                                      /*!< Reserved */
  __I  uint32_t RESERVED20;                                                      /*!< Reserved */
  __I  uint32_t RESERVED21;                                                      /*!< Reserved */
  __I  uint32_t RESERVED22;                                                      /*!< Reserved */
  __I  uint32_t RESERVED23;                                                      /*!< Reserved */
  __I  uint32_t RESERVED24;                                                      /*!< Reserved */
  __I  uint32_t RESERVED25;                                                      /*!< Reserved */
  __I  uint32_t RESERVED26;                                                      /*!< Reserved */
  __I  uint32_t ADC14_REF1P2V_TS30C;                                             /*!< ADC14 1.2V Reference Temp. Sensor 30C */
  __I  uint32_t ADC14_REF1P2V_TS85C;                                             /*!< ADC14 1.2V Reference Temp. Sensor 85C */
  __I  uint32_t ADC14_REF1P45V_TS30C;                                            /*!< ADC14 1.45V Reference Temp. Sensor 30C */
  __I  uint32_t ADC14_REF1P45V_TS85C;                                            /*!< ADC14 1.45V Reference Temp. Sensor 85C */
  __I  uint32_t ADC14_REF2P5V_TS30C;                                             /*!< ADC14 2.5V Reference Temp. Sensor 30C */
  __I  uint32_t ADC14_REF2P5V_TS85C;                                             /*!< ADC14 2.5V Reference Temp. Sensor 85C */
  __I  uint32_t REF_CAL_TAG;                                                     /*!< REF Calibration Tag */
  __I  uint32_t REF_CAL_LEN;                                                     /*!< REF Calibration Length */
  __I  uint32_t REF_1P2V;                                                        /*!< REF 1.2V Reference */
  __I  uint32_t REF_1P45V;                                                       /*!< REF 1.45V Reference */
  __I  uint32_t REF_2P5V;                                                        /*!< REF 2.5V Reference */
  __I  uint32_t FLASH_INFO_TAG;                                                  /*!< Flash Info Tag */
  __I  uint32_t FLASH_INFO_LEN;                                                  /*!< Flash Info Length */
  __I  uint32_t FLASH_MAX_PROG_PULSES;                                           /*!< Flash Maximum Programming Pulses */
  __I  uint32_t FLASH_MAX_ERASE_PULSES;                                          /*!< Flash Maximum Erase Pulses */
  __I  uint32_t RANDOM_NUM_TAG;                                                  /*!< 128-bit Random Number Tag */
  __I  uint32_t RANDOM_NUM_LEN;                                                  /*!< 128-bit Random Number Length */
  __I  uint32_t RANDOM_NUM_1;                                                    /*!< 32-bit Random Number 1 */
  __I  uint32_t RANDOM_NUM_2;                                                    /*!< 32-bit Random Number 2 */
  __I  uint32_t RANDOM_NUM_3;                                                    /*!< 32-bit Random Number 3 */
  __I  uint32_t RANDOM_NUM_4;                                                    /*!< 32-bit Random Number 4 */
  __I  uint32_t BSL_CFG_TAG;                                                     /*!< BSL Configuration Tag */
  __I  uint32_t BSL_CFG_LEN;                                                     /*!< BSL Configuration Length */
  __I  uint32_t BSL_PERIPHIF_SEL;                                                /*!< BSL Peripheral Interface Selection */
  __I  uint32_t BSL_PORTIF_CFG_UART;                                             /*!< BSL Port Interface Configuration for UART */
  __I  uint32_t BSL_PORTIF_CFG_SPI;                                              /*!< BSL Port Interface Configuration for SPI */
  __I  uint32_t BSL_PORTIF_CFG_I2C;                                              /*!< BSL Port Interface Configuration for I2C */
  __I  uint32_t TLV_END;                                                         /*!< TLV End Word */
} TLV_Type;

typedef struct {
  __IO uint32_t LOAD;                                                            /*!< Timer Load Register */
  __I  uint32_t VALUE;                                                           /*!< Timer Current Value Register */
  __IO uint32_t CONTROL;                                                         /*!< Timer Control Register */
  __O  uint32_t INTCLR;                                                          /*!< Timer Interrupt Clear Register */
  __I  uint32_t RIS;                                                             /*!< Timer Raw Interrupt Status Register */
  __I  uint32_t MIS;                                                             /*!< Timer Interrupt Status Register */
  __IO uint32_t BGLOAD;                                                          /*!< Timer Background Load Register */
} Timer32_Type;

typedef struct {
  __IO uint16_t CTL;                                                             /*!< TimerAx Control Register */
  __IO uint16_t CCTL[5];                                                         /*!< Timer_A Capture/Compare Control Register */
       uint16_t RESERVED0[2];
  __IO uint16_t R;                                                               /*!< TimerA register */
  __IO uint16_t CCR[5];                                                          /*!< Timer_A Capture/Compare  Register */
       uint16_t RESERVED1[2];
  __IO uint16_t EX0;                                                             /*!< TimerAx Expansion 0 Register */
       uint16_t RESERVED2[6];
  __I  uint16_t IV;                                                              /*!< TimerAx Interrupt Vector Register */
} Timer_A_Type;

typedef struct {
       uint16_t RESERVED0[6];
  __IO uint16_t CTL;                                                             /*!< Watchdog Timer Control Register */
} WDT_A_Type;

typedef struct {
  __IO uint32_t MB_START;                                                        /*!< Flash MailBox start: 0x0115ACF6 */
  __IO uint32_t CMD;                                                             /*!< Command for Boot override operations. */
       uint32_t RESERVED0[2];
  __IO uint32_t JTAG_SWD_LOCK_SECEN;                                             /*!< JTAG and SWD Lock Enable */
  __IO uint32_t JTAG_SWD_LOCK_AES_INIT_VECT[4];                                  /*!< JTAG and SWD lock AES initialization vector for AES-CBC */
  __IO uint32_t JTAG_SWD_LOCK_AES_SECKEYS[8];                                    /*!< JTAG and SWD lock AES CBC security Keys 0-7. */
  __IO uint32_t JTAG_SWD_LOCK_UNENC_PWD[4];                                      /*!< JTAG and SWD lock unencrypted password */
  __IO uint32_t JTAG_SWD_LOCK_ACK;                                               /*!< Acknowledgment for JTAG and SWD Lock command */
       uint32_t RESERVED1[2];
  SEC_ZONE_PARAMS_Type SEC_ZONE_PARAMS[4];                                              
  __IO uint32_t BSL_ENABLE;                                                      /*!< BSL Enable. */
  __IO uint32_t BSL_START_ADDRESS;                                               /*!< Contains the pointer to the BSL function. */
  __IO uint32_t BSL_PARAMETERS;                                                  /*!< BSL hardware invoke conifguration field. */
       uint32_t RESERVED2[2];
  __IO uint32_t BSL_ACK;                                                         /*!< Acknowledgment for the BSL Configuration Command */
  __IO uint32_t JTAG_SWD_LOCK_ENCPAYLOADADD;                                     /*!< Start address where the payload is loaded in the device. */
  __IO uint32_t JTAG_SWD_LOCK_ENCPAYLOADLEN;                                     /*!< Length of the encrypted payload in bytes */
  __IO uint32_t JTAG_SWD_LOCK_DST_ADDR;                                          /*!< Destination address where the final data needs to be stored into the device. */
  __IO uint32_t ENC_UPDATE_ACK;                                                  /*!< Acknowledgment for JTAG and SWD Lock Encrypted Update Command */
       uint32_t RESERVED3;
  SEC_ZONE_UPDATE_Type SEC_ZONE_UPDATE[4];                                              
       uint32_t RESERVED4;
  __IO uint32_t FACTORY_RESET_ENABLE;                                            /*!< Enable/Disable Factory Reset */
  __IO uint32_t FACTORY_RESET_PWDEN;                                             /*!< Factory reset password enable */
  __IO uint32_t FACTORY_RESET_PWD[4];                                            /*!< 128-bit Password for factory reset to be saved into the device. */
  __IO uint32_t FACTORY_RESET_PARAMS_ACK;                                        /*!< Acknowledgment for the Factory Reset Params Command */
       uint32_t RESERVED5;
  __IO uint32_t FACTORY_RESET_PASSWORD[4];                                       /*!< 128-bit Password for factory reset. */
  __IO uint32_t FACTORY_RESET_ACK;                                               /*!< Acknowledgment for the Factory Reset Command */
       uint32_t RESERVED6[2];
  __IO uint32_t MB_END;                                                          /*!< Mailbox end */
} FL_BOOTOVER_MAILBOX_Type;

#if defined ( __CC_ARM )
#pragma no_anon_unions
#endif

/*@}*/ /* end of group MSP432P4XX_Peripherals */

/******************************************************************************
* Peripheral declaration                                                      *
******************************************************************************/
/** @addtogroup MSP432P4XX_PeripheralDecl MSP432P4XX Peripheral Declaration
  @{
*/

#define ADC14                            ((ADC14_Type *) ADC14_BASE)   
#define AES256                           ((AES256_Type *) AES256_BASE) 
#define CAPTIO0                          ((CAPTIO_Type *) CAPTIO0_BASE)
#define CAPTIO1                          ((CAPTIO_Type *) CAPTIO1_BASE)
#define COMP_E0                          ((COMP_E_Type *) COMP_E0_BASE)
#define COMP_E1                          ((COMP_E_Type *) COMP_E1_BASE)
#define CRC32                            ((CRC32_Type *) CRC32_BASE)   
#define CS                               ((CS_Type *) CS_BASE)         
#define PA                               ((DIO_PORT_Interruptable_Type*) (DIO_BASE + 0x0000))
#define PB                               ((DIO_PORT_Interruptable_Type*) (DIO_BASE + 0x0020))
#define PC                               ((DIO_PORT_Interruptable_Type*) (DIO_BASE + 0x0040))
#define PD                               ((DIO_PORT_Interruptable_Type*) (DIO_BASE + 0x0060))
#define PE                               ((DIO_PORT_Interruptable_Type*) (DIO_BASE + 0x0080))
#define PJ                               ((DIO_PORT_Not_Interruptable_Type*) (DIO_BASE + 0x0120))
#define P1                               ((DIO_PORT_Odd_Interruptable_Type*)  (DIO_BASE + 0x0000))
#define P2                               ((DIO_PORT_Even_Interruptable_Type*) (DIO_BASE + 0x0000))
#define P3                               ((DIO_PORT_Odd_Interruptable_Type*)  (DIO_BASE + 0x0020))
#define P4                               ((DIO_PORT_Even_Interruptable_Type*) (DIO_BASE + 0x0020))
#define P5                               ((DIO_PORT_Odd_Interruptable_Type*)  (DIO_BASE + 0x0040))
#define P6                               ((DIO_PORT_Even_Interruptable_Type*) (DIO_BASE + 0x0040))
#define P7                               ((DIO_PORT_Odd_Interruptable_Type*)  (DIO_BASE + 0x0060))
#define P8                               ((DIO_PORT_Even_Interruptable_Type*) (DIO_BASE + 0x0060))
#define P9                               ((DIO_PORT_Odd_Interruptable_Type*)  (DIO_BASE + 0x0080))
#define P10                              ((DIO_PORT_Even_Interruptable_Type*) (DIO_BASE + 0x0080))
#define DMA_Channel                      ((DMA_Channel_Type *) DMA_BASE)
#define DMA_Control                      ((DMA_Control_Type *) (DMA_BASE + 0x1000))
#define EUSCI_A0                         ((EUSCI_A_Type *) EUSCI_A0_BASE)
#define EUSCI_A0_SPI                     ((EUSCI_A_SPI_Type *) EUSCI_A0_SPI_BASE)
#define EUSCI_A1                         ((EUSCI_A_Type *) EUSCI_A1_BASE)
#define EUSCI_A1_SPI                     ((EUSCI_A_SPI_Type *) EUSCI_A1_SPI_BASE)
#define EUSCI_A2                         ((EUSCI_A_Type *) EUSCI_A2_BASE)
#define EUSCI_A2_SPI                     ((EUSCI_A_SPI_Type *) EUSCI_A2_SPI_BASE)
#define EUSCI_A3                         ((EUSCI_A_Type *) EUSCI_A3_BASE)
#define EUSCI_A3_SPI                     ((EUSCI_A_SPI_Type *) EUSCI_A3_SPI_BASE)
#define EUSCI_B0                         ((EUSCI_B_Type *) EUSCI_B0_BASE)
#define EUSCI_B0_SPI                     ((EUSCI_B_SPI_Type *) EUSCI_B0_SPI_BASE)
#define EUSCI_B1                         ((EUSCI_B_Type *) EUSCI_B1_BASE)
#define EUSCI_B1_SPI                     ((EUSCI_B_SPI_Type *) EUSCI_B1_SPI_BASE)
#define EUSCI_B2                         ((EUSCI_B_Type *) EUSCI_B2_BASE)
#define EUSCI_B2_SPI                     ((EUSCI_B_SPI_Type *) EUSCI_B2_SPI_BASE)
#define EUSCI_B3                         ((EUSCI_B_Type *) EUSCI_B3_BASE)
#define EUSCI_B3_SPI                     ((EUSCI_B_SPI_Type *) EUSCI_B3_SPI_BASE)
#define FLCTL                            ((FLCTL_Type *) FLCTL_BASE)   
#define FL_BOOTOVER_MAILBOX              ((FL_BOOTOVER_MAILBOX_Type *) FL_BOOTOVER_MAILBOX_BASE)
#define PCM                              ((PCM_Type *) PCM_BASE)       
#define PMAP                             ((PMAP_COMMON_Type*) PMAP_BASE)
#define P1MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0008))
#define P2MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0010))
#define P3MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0018))
#define P4MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0020))
#define P5MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0028))
#define P6MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0030))
#define P7MAP                            ((PMAP_REGISTER_Type*) (PMAP_BASE + 0x0038))
#define PSS                              ((PSS_Type *) PSS_BASE)       
#define REF_A                            ((REF_A_Type *) REF_A_BASE)   
#define RSTCTL                           ((RSTCTL_Type *) RSTCTL_BASE) 
#define RTC_C                            ((RTC_C_Type *) RTC_C_BASE)   
#define RTC_C_BCD                        ((RTC_C_BCD_Type *) RTC_C_BCD_BASE)
#define SYSCTL                           ((SYSCTL_Type *) SYSCTL_BASE)
#define SYSCTL_Boot                      ((SYSCTL_Boot_Type *) (SYSCTL_BASE + 0x1000))
#define TIMER32_1                        ((Timer32_Type *) TIMER32_BASE)
#define TIMER32_2                        ((Timer32_Type *) (TIMER32_BASE + 0x00020))
#define TIMER_A0                         ((Timer_A_Type *) TIMER_A0_BASE)
#define TIMER_A1                         ((Timer_A_Type *) TIMER_A1_BASE)
#define TIMER_A2                         ((Timer_A_Type *) TIMER_A2_BASE)
#define TIMER_A3                         ((Timer_A_Type *) TIMER_A3_BASE)
#define TLV                              ((TLV_Type *) TLV_BASE)       
#define WDT_A                            ((WDT_A_Type *) WDT_A_BASE)   
#define FLCTL_A                          ((FLCTL_A_Type *) FLCTL_A_BASE)
#define LCD_F                            ((LCD_F_Type *) LCD_F_BASE)   
#define SYSCTL_A                         ((SYSCTL_A_Type *) SYSCTL_A_BASE)
#define SYSCTL_A_Boot                    ((SYSCTL_A_Boot_Type *) (SYSCTL_A_BASE + 0x1000))


/*@}*/ /* end of group MSP432P4XX_PeripheralDecl */

/*@}*/ /* end of group MSP432P4XX_Definitions */

#endif /* __CMSIS_CONFIG__ */

/******************************************************************************
* Peripheral register control bits                                            *
******************************************************************************/
#define ADC14_CTL0_SC_OFS                        ( 0)                            /*!< ADC14SC Bit Offset */
#define ADC14_CTL0_SC                            ((uint32_t)0x00000001)          /*!< ADC14 start conversion */
#define ADC14_CTL0_ENC_OFS                       ( 1)                            /*!< ADC14ENC Bit Offset */
#define ADC14_CTL0_ENC                           ((uint32_t)0x00000002)          /*!< ADC14 enable conversion */
#define ADC14_CTL0_ON_OFS                        ( 4)                            /*!< ADC14ON Bit Offset */
#define ADC14_CTL0_ON                            ((uint32_t)0x00000010)          /*!< ADC14 on */
#define ADC14_CTL0_MSC_OFS                       ( 7)                            /*!< ADC14MSC Bit Offset */
#define ADC14_CTL0_MSC                           ((uint32_t)0x00000080)          /*!< ADC14 multiple sample and conversion */
#define ADC14_CTL0_SHT0_OFS                      ( 8)                            /*!< ADC14SHT0 Bit Offset */
#define ADC14_CTL0_SHT0_MASK                     ((uint32_t)0x00000F00)          /*!< ADC14SHT0 Bit Mask */
#define ADC14_CTL0_SHT00                         ((uint32_t)0x00000100)          /*!< SHT0 Bit 0 */
#define ADC14_CTL0_SHT01                         ((uint32_t)0x00000200)          /*!< SHT0 Bit 1 */
#define ADC14_CTL0_SHT02                         ((uint32_t)0x00000400)          /*!< SHT0 Bit 2 */
#define ADC14_CTL0_SHT03                         ((uint32_t)0x00000800)          /*!< SHT0 Bit 3 */
#define ADC14_CTL0_SHT0_0                        ((uint32_t)0x00000000)          /*!< 4 */
#define ADC14_CTL0_SHT0_1                        ((uint32_t)0x00000100)          /*!< 8 */
#define ADC14_CTL0_SHT0_2                        ((uint32_t)0x00000200)          /*!< 16 */
#define ADC14_CTL0_SHT0_3                        ((uint32_t)0x00000300)          /*!< 32 */
#define ADC14_CTL0_SHT0_4                        ((uint32_t)0x00000400)          /*!< 64 */
#define ADC14_CTL0_SHT0_5                        ((uint32_t)0x00000500)          /*!< 96 */
#define ADC14_CTL0_SHT0_6                        ((uint32_t)0x00000600)          /*!< 128 */
#define ADC14_CTL0_SHT0_7                        ((uint32_t)0x00000700)          /*!< 192 */
#define ADC14_CTL0_SHT0__4                       ((uint32_t)0x00000000)          /*!< 4 */
#define ADC14_CTL0_SHT0__8                       ((uint32_t)0x00000100)          /*!< 8 */
#define ADC14_CTL0_SHT0__16                      ((uint32_t)0x00000200)          /*!< 16 */
#define ADC14_CTL0_SHT0__32                      ((uint32_t)0x00000300)          /*!< 32 */
#define ADC14_CTL0_SHT0__64                      ((uint32_t)0x00000400)          /*!< 64 */
#define ADC14_CTL0_SHT0__96                      ((uint32_t)0x00000500)          /*!< 96 */
#define ADC14_CTL0_SHT0__128                     ((uint32_t)0x00000600)          /*!< 128 */
#define ADC14_CTL0_SHT0__192                     ((uint32_t)0x00000700)          /*!< 192 */
#define ADC14_CTL0_SHT1_OFS                      (12)                            /*!< ADC14SHT1 Bit Offset */
#define ADC14_CTL0_SHT1_MASK                     ((uint32_t)0x0000F000)          /*!< ADC14SHT1 Bit Mask */
#define ADC14_CTL0_SHT10                         ((uint32_t)0x00001000)          /*!< SHT1 Bit 0 */
#define ADC14_CTL0_SHT11                         ((uint32_t)0x00002000)          /*!< SHT1 Bit 1 */
#define ADC14_CTL0_SHT12                         ((uint32_t)0x00004000)          /*!< SHT1 Bit 2 */
#define ADC14_CTL0_SHT13                         ((uint32_t)0x00008000)          /*!< SHT1 Bit 3 */
#define ADC14_CTL0_SHT1_0                        ((uint32_t)0x00000000)          /*!< 4 */
#define ADC14_CTL0_SHT1_1                        ((uint32_t)0x00001000)          /*!< 8 */
#define ADC14_CTL0_SHT1_2                        ((uint32_t)0x00002000)          /*!< 16 */
#define ADC14_CTL0_SHT1_3                        ((uint32_t)0x00003000)          /*!< 32 */
#define ADC14_CTL0_SHT1_4                        ((uint32_t)0x00004000)          /*!< 64 */
#define ADC14_CTL0_SHT1_5                        ((uint32_t)0x00005000)          /*!< 96 */
#define ADC14_CTL0_SHT1_6                        ((uint32_t)0x00006000)          /*!< 128 */
#define ADC14_CTL0_SHT1_7                        ((uint32_t)0x00007000)          /*!< 192 */
#define ADC14_CTL0_SHT1__4                       ((uint32_t)0x00000000)          /*!< 4 */
#define ADC14_CTL0_SHT1__8                       ((uint32_t)0x00001000)          /*!< 8 */
#define ADC14_CTL0_SHT1__16                      ((uint32_t)0x00002000)          /*!< 16 */
#define ADC14_CTL0_SHT1__32                      ((uint32_t)0x00003000)          /*!< 32 */
#define ADC14_CTL0_SHT1__64                      ((uint32_t)0x00004000)          /*!< 64 */
#define ADC14_CTL0_SHT1__96                      ((uint32_t)0x00005000)          /*!< 96 */
#define ADC14_CTL0_SHT1__128                     ((uint32_t)0x00006000)          /*!< 128 */
#define ADC14_CTL0_SHT1__192                     ((uint32_t)0x00007000)          /*!< 192 */
#define ADC14_CTL0_BUSY_OFS                      (16)                            /*!< ADC14BUSY Bit Offset */
#define ADC14_CTL0_BUSY                          ((uint32_t)0x00010000)          /*!< ADC14 busy */
#define ADC14_CTL0_CONSEQ_OFS                    (17)                            /*!< ADC14CONSEQ Bit Offset */
#define ADC14_CTL0_CONSEQ_MASK                   ((uint32_t)0x00060000)          /*!< ADC14CONSEQ Bit Mask */
#define ADC14_CTL0_CONSEQ0                       ((uint32_t)0x00020000)          /*!< CONSEQ Bit 0 */
#define ADC14_CTL0_CONSEQ1                       ((uint32_t)0x00040000)          /*!< CONSEQ Bit 1 */
#define ADC14_CTL0_CONSEQ_0                      ((uint32_t)0x00000000)          /*!< Single-channel, single-conversion */
#define ADC14_CTL0_CONSEQ_1                      ((uint32_t)0x00020000)          /*!< Sequence-of-channels */
#define ADC14_CTL0_CONSEQ_2                      ((uint32_t)0x00040000)          /*!< Repeat-single-channel */
#define ADC14_CTL0_CONSEQ_3                      ((uint32_t)0x00060000)          /*!< Repeat-sequence-of-channels */
#define ADC14_CTL0_SSEL_OFS                      (19)                            /*!< ADC14SSEL Bit Offset */
#define ADC14_CTL0_SSEL_MASK                     ((uint32_t)0x00380000)          /*!< ADC14SSEL Bit Mask */
#define ADC14_CTL0_SSEL0                         ((uint32_t)0x00080000)          /*!< SSEL Bit 0 */
#define ADC14_CTL0_SSEL1                         ((uint32_t)0x00100000)          /*!< SSEL Bit 1 */
#define ADC14_CTL0_SSEL2                         ((uint32_t)0x00200000)          /*!< SSEL Bit 2 */
#define ADC14_CTL0_SSEL_0                        ((uint32_t)0x00000000)          /*!< MODCLK */
#define ADC14_CTL0_SSEL_1                        ((uint32_t)0x00080000)          /*!< SYSCLK */
#define ADC14_CTL0_SSEL_2                        ((uint32_t)0x00100000)          /*!< ACLK */
#define ADC14_CTL0_SSEL_3                        ((uint32_t)0x00180000)          /*!< MCLK */
#define ADC14_CTL0_SSEL_4                        ((uint32_t)0x00200000)          /*!< SMCLK */
#define ADC14_CTL0_SSEL_5                        ((uint32_t)0x00280000)          /*!< HSMCLK */
#define ADC14_CTL0_SSEL__MODCLK                  ((uint32_t)0x00000000)          /*!< MODCLK */
#define ADC14_CTL0_SSEL__SYSCLK                  ((uint32_t)0x00080000)          /*!< SYSCLK */
#define ADC14_CTL0_SSEL__ACLK                    ((uint32_t)0x00100000)          /*!< ACLK */
#define ADC14_CTL0_SSEL__MCLK                    ((uint32_t)0x00180000)          /*!< MCLK */
#define ADC14_CTL0_SSEL__SMCLK                   ((uint32_t)0x00200000)          /*!< SMCLK */
#define ADC14_CTL0_SSEL__HSMCLK                  ((uint32_t)0x00280000)          /*!< HSMCLK */
#define ADC14_CTL0_DIV_OFS                       (22)                            /*!< ADC14DIV Bit Offset */
#define ADC14_CTL0_DIV_MASK                      ((uint32_t)0x01C00000)          /*!< ADC14DIV Bit Mask */
#define ADC14_CTL0_DIV0                          ((uint32_t)0x00400000)          /*!< DIV Bit 0 */
#define ADC14_CTL0_DIV1                          ((uint32_t)0x00800000)          /*!< DIV Bit 1 */
#define ADC14_CTL0_DIV2                          ((uint32_t)0x01000000)          /*!< DIV Bit 2 */
#define ADC14_CTL0_DIV_0                         ((uint32_t)0x00000000)          /*!< /1 */
#define ADC14_CTL0_DIV_1                         ((uint32_t)0x00400000)          /*!< /2 */
#define ADC14_CTL0_DIV_2                         ((uint32_t)0x00800000)          /*!< /3 */
#define ADC14_CTL0_DIV_3                         ((uint32_t)0x00C00000)          /*!< /4 */
#define ADC14_CTL0_DIV_4                         ((uint32_t)0x01000000)          /*!< /5 */
#define ADC14_CTL0_DIV_5                         ((uint32_t)0x01400000)          /*!< /6 */
#define ADC14_CTL0_DIV_6                         ((uint32_t)0x01800000)          /*!< /7 */
#define ADC14_CTL0_DIV_7                         ((uint32_t)0x01C00000)          /*!< /8 */
#define ADC14_CTL0_DIV__1                        ((uint32_t)0x00000000)          /*!< /1 */
#define ADC14_CTL0_DIV__2                        ((uint32_t)0x00400000)          /*!< /2 */
#define ADC14_CTL0_DIV__3                        ((uint32_t)0x00800000)          /*!< /3 */
#define ADC14_CTL0_DIV__4                        ((uint32_t)0x00C00000)          /*!< /4 */
#define ADC14_CTL0_DIV__5                        ((uint32_t)0x01000000)          /*!< /5 */
#define ADC14_CTL0_DIV__6                        ((uint32_t)0x01400000)          /*!< /6 */
#define ADC14_CTL0_DIV__7                        ((uint32_t)0x01800000)          /*!< /7 */
#define ADC14_CTL0_DIV__8                        ((uint32_t)0x01C00000)          /*!< /8 */
#define ADC14_CTL0_ISSH_OFS                      (25)                            /*!< ADC14ISSH Bit Offset */
#define ADC14_CTL0_ISSH                          ((uint32_t)0x02000000)          /*!< ADC14 invert signal sample-and-hold */
#define ADC14_CTL0_SHP_OFS                       (26)                            /*!< ADC14SHP Bit Offset */
#define ADC14_CTL0_SHP                           ((uint32_t)0x04000000)          /*!< ADC14 sample-and-hold pulse-mode select */
#define ADC14_CTL0_SHS_OFS                       (27)                            /*!< ADC14SHS Bit Offset */
#define ADC14_CTL0_SHS_MASK                      ((uint32_t)0x38000000)          /*!< ADC14SHS Bit Mask */
#define ADC14_CTL0_SHS0                          ((uint32_t)0x08000000)          /*!< SHS Bit 0 */
#define ADC14_CTL0_SHS1                          ((uint32_t)0x10000000)          /*!< SHS Bit 1 */
#define ADC14_CTL0_SHS2                          ((uint32_t)0x20000000)          /*!< SHS Bit 2 */
#define ADC14_CTL0_SHS_0                         ((uint32_t)0x00000000)          /*!< ADC14SC bit */
#define ADC14_CTL0_SHS_1                         ((uint32_t)0x08000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_SHS_2                         ((uint32_t)0x10000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_SHS_3                         ((uint32_t)0x18000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_SHS_4                         ((uint32_t)0x20000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_SHS_5                         ((uint32_t)0x28000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_SHS_6                         ((uint32_t)0x30000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_SHS_7                         ((uint32_t)0x38000000)          /*!< See device-specific data sheet for source */
#define ADC14_CTL0_PDIV_OFS                      (30)                            /*!< ADC14PDIV Bit Offset */
#define ADC14_CTL0_PDIV_MASK                     ((uint32_t)0xC0000000)          /*!< ADC14PDIV Bit Mask */
#define ADC14_CTL0_PDIV0                         ((uint32_t)0x40000000)          /*!< PDIV Bit 0 */
#define ADC14_CTL0_PDIV1                         ((uint32_t)0x80000000)          /*!< PDIV Bit 1 */
#define ADC14_CTL0_PDIV_0                        ((uint32_t)0x00000000)          /*!< Predivide by 1 */
#define ADC14_CTL0_PDIV_1                        ((uint32_t)0x40000000)          /*!< Predivide by 4 */
#define ADC14_CTL0_PDIV_2                        ((uint32_t)0x80000000)          /*!< Predivide by 32 */
#define ADC14_CTL0_PDIV_3                        ((uint32_t)0xC0000000)          /*!< Predivide by 64 */
#define ADC14_CTL0_PDIV__1                       ((uint32_t)0x00000000)          /*!< Predivide by 1 */
#define ADC14_CTL0_PDIV__4                       ((uint32_t)0x40000000)          /*!< Predivide by 4 */
#define ADC14_CTL0_PDIV__32                      ((uint32_t)0x80000000)          /*!< Predivide by 32 */
#define ADC14_CTL0_PDIV__64                      ((uint32_t)0xC0000000)          /*!< Predivide by 64 */
#define ADC14_CTL1_PWRMD_OFS                     ( 0)                            /*!< ADC14PWRMD Bit Offset */
#define ADC14_CTL1_PWRMD_MASK                    ((uint32_t)0x00000003)          /*!< ADC14PWRMD Bit Mask */
#define ADC14_CTL1_PWRMD0                        ((uint32_t)0x00000001)          /*!< PWRMD Bit 0 */
#define ADC14_CTL1_PWRMD1                        ((uint32_t)0x00000002)          /*!< PWRMD Bit 1 */
#define ADC14_CTL1_PWRMD_0                       ((uint32_t)0x00000000)          /*!< Regular power mode for use with any resolution setting. Sample rate can be  */
#define ADC14_CTL1_PWRMD_2                       ((uint32_t)0x00000002)          /*!< Low-power mode for 12-bit, 10-bit, and 8-bit resolution settings. Sample  */
#define ADC14_CTL1_REFBURST_OFS                  ( 2)                            /*!< ADC14REFBURST Bit Offset */
#define ADC14_CTL1_REFBURST                      ((uint32_t)0x00000004)          /*!< ADC14 reference buffer burst */
#define ADC14_CTL1_DF_OFS                        ( 3)                            /*!< ADC14DF Bit Offset */
#define ADC14_CTL1_DF                            ((uint32_t)0x00000008)          /*!< ADC14 data read-back format */
#define ADC14_CTL1_RES_OFS                       ( 4)                            /*!< ADC14RES Bit Offset */
#define ADC14_CTL1_RES_MASK                      ((uint32_t)0x00000030)          /*!< ADC14RES Bit Mask */
#define ADC14_CTL1_RES0                          ((uint32_t)0x00000010)          /*!< RES Bit 0 */
#define ADC14_CTL1_RES1                          ((uint32_t)0x00000020)          /*!< RES Bit 1 */
#define ADC14_CTL1_RES_0                         ((uint32_t)0x00000000)          /*!< 8 bit (9 clock cycle conversion time) */
#define ADC14_CTL1_RES_1                         ((uint32_t)0x00000010)          /*!< 10 bit (11 clock cycle conversion time) */
#define ADC14_CTL1_RES_2                         ((uint32_t)0x00000020)          /*!< 12 bit (14 clock cycle conversion time) */
#define ADC14_CTL1_RES_3                         ((uint32_t)0x00000030)          /*!< 14 bit (16 clock cycle conversion time) */
#define ADC14_CTL1_RES__8BIT                     ((uint32_t)0x00000000)          /*!< 8 bit (9 clock cycle conversion time) */
#define ADC14_CTL1_RES__10BIT                    ((uint32_t)0x00000010)          /*!< 10 bit (11 clock cycle conversion time) */
#define ADC14_CTL1_RES__12BIT                    ((uint32_t)0x00000020)          /*!< 12 bit (14 clock cycle conversion time) */
#define ADC14_CTL1_RES__14BIT                    ((uint32_t)0x00000030)          /*!< 14 bit (16 clock cycle conversion time) */
#define ADC14_CTL1_CSTARTADD_OFS                 (16)                            /*!< ADC14CSTARTADD Bit Offset */
#define ADC14_CTL1_CSTARTADD_MASK                ((uint32_t)0x001F0000)          /*!< ADC14CSTARTADD Bit Mask */
#define ADC14_CTL1_BATMAP_OFS                    (22)                            /*!< ADC14BATMAP Bit Offset */
#define ADC14_CTL1_BATMAP                        ((uint32_t)0x00400000)          /*!< Controls 1/2 AVCC ADC input channel selection */
#define ADC14_CTL1_TCMAP_OFS                     (23)                            /*!< ADC14TCMAP Bit Offset */
#define ADC14_CTL1_TCMAP                         ((uint32_t)0x00800000)          /*!< Controls temperature sensor ADC input channel selection */
#define ADC14_CTL1_CH0MAP_OFS                    (24)                            /*!< ADC14CH0MAP Bit Offset */
#define ADC14_CTL1_CH0MAP                        ((uint32_t)0x01000000)          /*!< Controls internal channel 0 selection to ADC input channel MAX-2 */
#define ADC14_CTL1_CH1MAP_OFS                    (25)                            /*!< ADC14CH1MAP Bit Offset */
#define ADC14_CTL1_CH1MAP                        ((uint32_t)0x02000000)          /*!< Controls internal channel 1 selection to ADC input channel MAX-3 */
#define ADC14_CTL1_CH2MAP_OFS                    (26)                            /*!< ADC14CH2MAP Bit Offset */
#define ADC14_CTL1_CH2MAP                        ((uint32_t)0x04000000)          /*!< Controls internal channel 2 selection to ADC input channel MAX-4 */
#define ADC14_CTL1_CH3MAP_OFS                    (27)                            /*!< ADC14CH3MAP Bit Offset */
#define ADC14_CTL1_CH3MAP                        ((uint32_t)0x08000000)          /*!< Controls internal channel 3 selection to ADC input channel MAX-5 */
#define ADC14_LO0_LO0_OFS                        ( 0)                            /*!< ADC14LO0 Bit Offset */
#define ADC14_LO0_LO0_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14LO0 Bit Mask */
#define ADC14_HI0_HI0_OFS                        ( 0)                            /*!< ADC14HI0 Bit Offset */
#define ADC14_HI0_HI0_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14HI0 Bit Mask */
#define ADC14_LO1_LO1_OFS                        ( 0)                            /*!< ADC14LO1 Bit Offset */
#define ADC14_LO1_LO1_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14LO1 Bit Mask */
#define ADC14_HI1_HI1_OFS                        ( 0)                            /*!< ADC14HI1 Bit Offset */
#define ADC14_HI1_HI1_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14HI1 Bit Mask */
#define ADC14_MCTLN_INCH_OFS                     ( 0)                            /*!< ADC14INCH Bit Offset */
#define ADC14_MCTLN_INCH_MASK                    ((uint32_t)0x0000001F)          /*!< ADC14INCH Bit Mask */
#define ADC14_MCTLN_INCH0                        ((uint32_t)0x00000001)          /*!< INCH Bit 0 */
#define ADC14_MCTLN_INCH1                        ((uint32_t)0x00000002)          /*!< INCH Bit 1 */
#define ADC14_MCTLN_INCH2                        ((uint32_t)0x00000004)          /*!< INCH Bit 2 */
#define ADC14_MCTLN_INCH3                        ((uint32_t)0x00000008)          /*!< INCH Bit 3 */
#define ADC14_MCTLN_INCH4                        ((uint32_t)0x00000010)          /*!< INCH Bit 4 */
#define ADC14_MCTLN_INCH_0                       ((uint32_t)0x00000000)          /*!< If ADC14DIF = 0: A0; If ADC14DIF = 1: Ain+ = A0, Ain- = A1 */
#define ADC14_MCTLN_INCH_1                       ((uint32_t)0x00000001)          /*!< If ADC14DIF = 0: A1; If ADC14DIF = 1: Ain+ = A0, Ain- = A1 */
#define ADC14_MCTLN_INCH_2                       ((uint32_t)0x00000002)          /*!< If ADC14DIF = 0: A2; If ADC14DIF = 1: Ain+ = A2, Ain- = A3 */
#define ADC14_MCTLN_INCH_3                       ((uint32_t)0x00000003)          /*!< If ADC14DIF = 0: A3; If ADC14DIF = 1: Ain+ = A2, Ain- = A3 */
#define ADC14_MCTLN_INCH_4                       ((uint32_t)0x00000004)          /*!< If ADC14DIF = 0: A4; If ADC14DIF = 1: Ain+ = A4, Ain- = A5 */
#define ADC14_MCTLN_INCH_5                       ((uint32_t)0x00000005)          /*!< If ADC14DIF = 0: A5; If ADC14DIF = 1: Ain+ = A4, Ain- = A5 */
#define ADC14_MCTLN_INCH_6                       ((uint32_t)0x00000006)          /*!< If ADC14DIF = 0: A6; If ADC14DIF = 1: Ain+ = A6, Ain- = A7 */
#define ADC14_MCTLN_INCH_7                       ((uint32_t)0x00000007)          /*!< If ADC14DIF = 0: A7; If ADC14DIF = 1: Ain+ = A6, Ain- = A7 */
#define ADC14_MCTLN_INCH_8                       ((uint32_t)0x00000008)          /*!< If ADC14DIF = 0: A8; If ADC14DIF = 1: Ain+ = A8, Ain- = A9 */
#define ADC14_MCTLN_INCH_9                       ((uint32_t)0x00000009)          /*!< If ADC14DIF = 0: A9; If ADC14DIF = 1: Ain+ = A8, Ain- = A9 */
#define ADC14_MCTLN_INCH_10                      ((uint32_t)0x0000000A)          /*!< If ADC14DIF = 0: A10; If ADC14DIF = 1: Ain+ = A10, Ain- = A11 */
#define ADC14_MCTLN_INCH_11                      ((uint32_t)0x0000000B)          /*!< If ADC14DIF = 0: A11; If ADC14DIF = 1: Ain+ = A10, Ain- = A11 */
#define ADC14_MCTLN_INCH_12                      ((uint32_t)0x0000000C)          /*!< If ADC14DIF = 0: A12; If ADC14DIF = 1: Ain+ = A12, Ain- = A13 */
#define ADC14_MCTLN_INCH_13                      ((uint32_t)0x0000000D)          /*!< If ADC14DIF = 0: A13; If ADC14DIF = 1: Ain+ = A12, Ain- = A13 */
#define ADC14_MCTLN_INCH_14                      ((uint32_t)0x0000000E)          /*!< If ADC14DIF = 0: A14; If ADC14DIF = 1: Ain+ = A14, Ain- = A15 */
#define ADC14_MCTLN_INCH_15                      ((uint32_t)0x0000000F)          /*!< If ADC14DIF = 0: A15; If ADC14DIF = 1: Ain+ = A14, Ain- = A15 */
#define ADC14_MCTLN_INCH_16                      ((uint32_t)0x00000010)          /*!< If ADC14DIF = 0: A16; If ADC14DIF = 1: Ain+ = A16, Ain- = A17 */
#define ADC14_MCTLN_INCH_17                      ((uint32_t)0x00000011)          /*!< If ADC14DIF = 0: A17; If ADC14DIF = 1: Ain+ = A16, Ain- = A17 */
#define ADC14_MCTLN_INCH_18                      ((uint32_t)0x00000012)          /*!< If ADC14DIF = 0: A18; If ADC14DIF = 1: Ain+ = A18, Ain- = A19 */
#define ADC14_MCTLN_INCH_19                      ((uint32_t)0x00000013)          /*!< If ADC14DIF = 0: A19; If ADC14DIF = 1: Ain+ = A18, Ain- = A19 */
#define ADC14_MCTLN_INCH_20                      ((uint32_t)0x00000014)          /*!< If ADC14DIF = 0: A20; If ADC14DIF = 1: Ain+ = A20, Ain- = A21 */
#define ADC14_MCTLN_INCH_21                      ((uint32_t)0x00000015)          /*!< If ADC14DIF = 0: A21; If ADC14DIF = 1: Ain+ = A20, Ain- = A21 */
#define ADC14_MCTLN_INCH_22                      ((uint32_t)0x00000016)          /*!< If ADC14DIF = 0: A22; If ADC14DIF = 1: Ain+ = A22, Ain- = A23 */
#define ADC14_MCTLN_INCH_23                      ((uint32_t)0x00000017)          /*!< If ADC14DIF = 0: A23; If ADC14DIF = 1: Ain+ = A22, Ain- = A23 */
#define ADC14_MCTLN_INCH_24                      ((uint32_t)0x00000018)          /*!< If ADC14DIF = 0: A24; If ADC14DIF = 1: Ain+ = A24, Ain- = A25 */
#define ADC14_MCTLN_INCH_25                      ((uint32_t)0x00000019)          /*!< If ADC14DIF = 0: A25; If ADC14DIF = 1: Ain+ = A24, Ain- = A25 */
#define ADC14_MCTLN_INCH_26                      ((uint32_t)0x0000001A)          /*!< If ADC14DIF = 0: A26; If ADC14DIF = 1: Ain+ = A26, Ain- = A27 */
#define ADC14_MCTLN_INCH_27                      ((uint32_t)0x0000001B)          /*!< If ADC14DIF = 0: A27; If ADC14DIF = 1: Ain+ = A26, Ain- = A27 */
#define ADC14_MCTLN_INCH_28                      ((uint32_t)0x0000001C)          /*!< If ADC14DIF = 0: A28; If ADC14DIF = 1: Ain+ = A28, Ain- = A29 */
#define ADC14_MCTLN_INCH_29                      ((uint32_t)0x0000001D)          /*!< If ADC14DIF = 0: A29; If ADC14DIF = 1: Ain+ = A28, Ain- = A29 */
#define ADC14_MCTLN_INCH_30                      ((uint32_t)0x0000001E)          /*!< If ADC14DIF = 0: A30; If ADC14DIF = 1: Ain+ = A30, Ain- = A31 */
#define ADC14_MCTLN_INCH_31                      ((uint32_t)0x0000001F)          /*!< If ADC14DIF = 0: A31; If ADC14DIF = 1: Ain+ = A30, Ain- = A31 */
#define ADC14_MCTLN_EOS_OFS                      ( 7)                            /*!< ADC14EOS Bit Offset */
#define ADC14_MCTLN_EOS                          ((uint32_t)0x00000080)          /*!< End of sequence */
#define ADC14_MCTLN_VRSEL_OFS                    ( 8)                            /*!< ADC14VRSEL Bit Offset */
#define ADC14_MCTLN_VRSEL_MASK                   ((uint32_t)0x00000F00)          /*!< ADC14VRSEL Bit Mask */
#define ADC14_MCTLN_VRSEL0                       ((uint32_t)0x00000100)          /*!< VRSEL Bit 0 */
#define ADC14_MCTLN_VRSEL1                       ((uint32_t)0x00000200)          /*!< VRSEL Bit 1 */
#define ADC14_MCTLN_VRSEL2                       ((uint32_t)0x00000400)          /*!< VRSEL Bit 2 */
#define ADC14_MCTLN_VRSEL3                       ((uint32_t)0x00000800)          /*!< VRSEL Bit 3 */
#define ADC14_MCTLN_VRSEL_0                      ((uint32_t)0x00000000)          /*!< V(R+) = AVCC, V(R-) = AVSS */
#define ADC14_MCTLN_VRSEL_1                      ((uint32_t)0x00000100)          /*!< V(R+) = VREF buffered, V(R-) = AVSS */
#define ADC14_MCTLN_VRSEL_14                     ((uint32_t)0x00000E00)          /*!< V(R+) = VeREF+, V(R-) = VeREF- */
#define ADC14_MCTLN_VRSEL_15                     ((uint32_t)0x00000F00)          /*!< V(R+) = VeREF+ buffered, V(R-) = VeREF */
#define ADC14_MCTLN_DIF_OFS                      (13)                            /*!< ADC14DIF Bit Offset */
#define ADC14_MCTLN_DIF                          ((uint32_t)0x00002000)          /*!< Differential mode */
#define ADC14_MCTLN_WINC_OFS                     (14)                            /*!< ADC14WINC Bit Offset */
#define ADC14_MCTLN_WINC                         ((uint32_t)0x00004000)          /*!< Comparator window enable */
#define ADC14_MCTLN_WINCTH_OFS                   (15)                            /*!< ADC14WINCTH Bit Offset */
#define ADC14_MCTLN_WINCTH                       ((uint32_t)0x00008000)          /*!< Window comparator threshold register selection */
#define ADC14_MEMN_CONVRES_OFS                   ( 0)                            /*!< Conversion_Results Bit Offset */
#define ADC14_MEMN_CONVRES_MASK                  ((uint32_t)0x0000FFFF)          /*!< Conversion_Results Bit Mask */
#define ADC14_IER0_IE0_OFS                       ( 0)                            /*!< ADC14IE0 Bit Offset */
#define ADC14_IER0_IE0                           ((uint32_t)0x00000001)          /*!< Interrupt enable */
#define ADC14_IER0_IE1_OFS                       ( 1)                            /*!< ADC14IE1 Bit Offset */
#define ADC14_IER0_IE1                           ((uint32_t)0x00000002)          /*!< Interrupt enable */
#define ADC14_IER0_IE2_OFS                       ( 2)                            /*!< ADC14IE2 Bit Offset */
#define ADC14_IER0_IE2                           ((uint32_t)0x00000004)          /*!< Interrupt enable */
#define ADC14_IER0_IE3_OFS                       ( 3)                            /*!< ADC14IE3 Bit Offset */
#define ADC14_IER0_IE3                           ((uint32_t)0x00000008)          /*!< Interrupt enable */
#define ADC14_IER0_IE4_OFS                       ( 4)                            /*!< ADC14IE4 Bit Offset */
#define ADC14_IER0_IE4                           ((uint32_t)0x00000010)          /*!< Interrupt enable */
#define ADC14_IER0_IE5_OFS                       ( 5)                            /*!< ADC14IE5 Bit Offset */
#define ADC14_IER0_IE5                           ((uint32_t)0x00000020)          /*!< Interrupt enable */
#define ADC14_IER0_IE6_OFS                       ( 6)                            /*!< ADC14IE6 Bit Offset */
#define ADC14_IER0_IE6                           ((uint32_t)0x00000040)          /*!< Interrupt enable */
#define ADC14_IER0_IE7_OFS                       ( 7)                            /*!< ADC14IE7 Bit Offset */
#define ADC14_IER0_IE7                           ((uint32_t)0x00000080)          /*!< Interrupt enable */
#define ADC14_IER0_IE8_OFS                       ( 8)                            /*!< ADC14IE8 Bit Offset */
#define ADC14_IER0_IE8                           ((uint32_t)0x00000100)          /*!< Interrupt enable */
#define ADC14_IER0_IE9_OFS                       ( 9)                            /*!< ADC14IE9 Bit Offset */
#define ADC14_IER0_IE9                           ((uint32_t)0x00000200)          /*!< Interrupt enable */
#define ADC14_IER0_IE10_OFS                      (10)                            /*!< ADC14IE10 Bit Offset */
#define ADC14_IER0_IE10                          ((uint32_t)0x00000400)          /*!< Interrupt enable */
#define ADC14_IER0_IE11_OFS                      (11)                            /*!< ADC14IE11 Bit Offset */
#define ADC14_IER0_IE11                          ((uint32_t)0x00000800)          /*!< Interrupt enable */
#define ADC14_IER0_IE12_OFS                      (12)                            /*!< ADC14IE12 Bit Offset */
#define ADC14_IER0_IE12                          ((uint32_t)0x00001000)          /*!< Interrupt enable */
#define ADC14_IER0_IE13_OFS                      (13)                            /*!< ADC14IE13 Bit Offset */
#define ADC14_IER0_IE13                          ((uint32_t)0x00002000)          /*!< Interrupt enable */
#define ADC14_IER0_IE14_OFS                      (14)                            /*!< ADC14IE14 Bit Offset */
#define ADC14_IER0_IE14                          ((uint32_t)0x00004000)          /*!< Interrupt enable */
#define ADC14_IER0_IE15_OFS                      (15)                            /*!< ADC14IE15 Bit Offset */
#define ADC14_IER0_IE15                          ((uint32_t)0x00008000)          /*!< Interrupt enable */
#define ADC14_IER0_IE16_OFS                      (16)                            /*!< ADC14IE16 Bit Offset */
#define ADC14_IER0_IE16                          ((uint32_t)0x00010000)          /*!< Interrupt enable */
#define ADC14_IER0_IE17_OFS                      (17)                            /*!< ADC14IE17 Bit Offset */
#define ADC14_IER0_IE17                          ((uint32_t)0x00020000)          /*!< Interrupt enable */
#define ADC14_IER0_IE19_OFS                      (19)                            /*!< ADC14IE19 Bit Offset */
#define ADC14_IER0_IE19                          ((uint32_t)0x00080000)          /*!< Interrupt enable */
#define ADC14_IER0_IE18_OFS                      (18)                            /*!< ADC14IE18 Bit Offset */
#define ADC14_IER0_IE18                          ((uint32_t)0x00040000)          /*!< Interrupt enable */
#define ADC14_IER0_IE20_OFS                      (20)                            /*!< ADC14IE20 Bit Offset */
#define ADC14_IER0_IE20                          ((uint32_t)0x00100000)          /*!< Interrupt enable */
#define ADC14_IER0_IE21_OFS                      (21)                            /*!< ADC14IE21 Bit Offset */
#define ADC14_IER0_IE21                          ((uint32_t)0x00200000)          /*!< Interrupt enable */
#define ADC14_IER0_IE22_OFS                      (22)                            /*!< ADC14IE22 Bit Offset */
#define ADC14_IER0_IE22                          ((uint32_t)0x00400000)          /*!< Interrupt enable */
#define ADC14_IER0_IE23_OFS                      (23)                            /*!< ADC14IE23 Bit Offset */
#define ADC14_IER0_IE23                          ((uint32_t)0x00800000)          /*!< Interrupt enable */
#define ADC14_IER0_IE24_OFS                      (24)                            /*!< ADC14IE24 Bit Offset */
#define ADC14_IER0_IE24                          ((uint32_t)0x01000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE25_OFS                      (25)                            /*!< ADC14IE25 Bit Offset */
#define ADC14_IER0_IE25                          ((uint32_t)0x02000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE26_OFS                      (26)                            /*!< ADC14IE26 Bit Offset */
#define ADC14_IER0_IE26                          ((uint32_t)0x04000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE27_OFS                      (27)                            /*!< ADC14IE27 Bit Offset */
#define ADC14_IER0_IE27                          ((uint32_t)0x08000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE28_OFS                      (28)                            /*!< ADC14IE28 Bit Offset */
#define ADC14_IER0_IE28                          ((uint32_t)0x10000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE29_OFS                      (29)                            /*!< ADC14IE29 Bit Offset */
#define ADC14_IER0_IE29                          ((uint32_t)0x20000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE30_OFS                      (30)                            /*!< ADC14IE30 Bit Offset */
#define ADC14_IER0_IE30                          ((uint32_t)0x40000000)          /*!< Interrupt enable */
#define ADC14_IER0_IE31_OFS                      (31)                            /*!< ADC14IE31 Bit Offset */
#define ADC14_IER0_IE31                          ((uint32_t)0x80000000)          /*!< Interrupt enable */
#define ADC14_IER1_INIE_OFS                      ( 1)                            /*!< ADC14INIE Bit Offset */
#define ADC14_IER1_INIE                          ((uint32_t)0x00000002)          /*!< Interrupt enable for ADC14MEMx within comparator window */
#define ADC14_IER1_LOIE_OFS                      ( 2)                            /*!< ADC14LOIE Bit Offset */
#define ADC14_IER1_LOIE                          ((uint32_t)0x00000004)          /*!< Interrupt enable for ADC14MEMx below comparator window */
#define ADC14_IER1_HIIE_OFS                      ( 3)                            /*!< ADC14HIIE Bit Offset */
#define ADC14_IER1_HIIE                          ((uint32_t)0x00000008)          /*!< Interrupt enable for ADC14MEMx above comparator window */
#define ADC14_IER1_OVIE_OFS                      ( 4)                            /*!< ADC14OVIE Bit Offset */
#define ADC14_IER1_OVIE                          ((uint32_t)0x00000010)          /*!< ADC14MEMx overflow-interrupt enable */
#define ADC14_IER1_TOVIE_OFS                     ( 5)                            /*!< ADC14TOVIE Bit Offset */
#define ADC14_IER1_TOVIE                         ((uint32_t)0x00000020)          /*!< ADC14 conversion-time-overflow interrupt enable */
#define ADC14_IER1_RDYIE_OFS                     ( 6)                            /*!< ADC14RDYIE Bit Offset */
#define ADC14_IER1_RDYIE                         ((uint32_t)0x00000040)          /*!< ADC14 local buffered reference ready interrupt enable */
#define ADC14_IFGR0_IFG0_OFS                     ( 0)                            /*!< ADC14IFG0 Bit Offset */
#define ADC14_IFGR0_IFG0                         ((uint32_t)0x00000001)          /*!< ADC14MEM0 interrupt flag */
#define ADC14_IFGR0_IFG1_OFS                     ( 1)                            /*!< ADC14IFG1 Bit Offset */
#define ADC14_IFGR0_IFG1                         ((uint32_t)0x00000002)          /*!< ADC14MEM1 interrupt flag */
#define ADC14_IFGR0_IFG2_OFS                     ( 2)                            /*!< ADC14IFG2 Bit Offset */
#define ADC14_IFGR0_IFG2                         ((uint32_t)0x00000004)          /*!< ADC14MEM2 interrupt flag */
#define ADC14_IFGR0_IFG3_OFS                     ( 3)                            /*!< ADC14IFG3 Bit Offset */
#define ADC14_IFGR0_IFG3                         ((uint32_t)0x00000008)          /*!< ADC14MEM3 interrupt flag */
#define ADC14_IFGR0_IFG4_OFS                     ( 4)                            /*!< ADC14IFG4 Bit Offset */
#define ADC14_IFGR0_IFG4                         ((uint32_t)0x00000010)          /*!< ADC14MEM4 interrupt flag */
#define ADC14_IFGR0_IFG5_OFS                     ( 5)                            /*!< ADC14IFG5 Bit Offset */
#define ADC14_IFGR0_IFG5                         ((uint32_t)0x00000020)          /*!< ADC14MEM5 interrupt flag */
#define ADC14_IFGR0_IFG6_OFS                     ( 6)                            /*!< ADC14IFG6 Bit Offset */
#define ADC14_IFGR0_IFG6                         ((uint32_t)0x00000040)          /*!< ADC14MEM6 interrupt flag */
#define ADC14_IFGR0_IFG7_OFS                     ( 7)                            /*!< ADC14IFG7 Bit Offset */
#define ADC14_IFGR0_IFG7                         ((uint32_t)0x00000080)          /*!< ADC14MEM7 interrupt flag */
#define ADC14_IFGR0_IFG8_OFS                     ( 8)                            /*!< ADC14IFG8 Bit Offset */
#define ADC14_IFGR0_IFG8                         ((uint32_t)0x00000100)          /*!< ADC14MEM8 interrupt flag */
#define ADC14_IFGR0_IFG9_OFS                     ( 9)                            /*!< ADC14IFG9 Bit Offset */
#define ADC14_IFGR0_IFG9                         ((uint32_t)0x00000200)          /*!< ADC14MEM9 interrupt flag */
#define ADC14_IFGR0_IFG10_OFS                    (10)                            /*!< ADC14IFG10 Bit Offset */
#define ADC14_IFGR0_IFG10                        ((uint32_t)0x00000400)          /*!< ADC14MEM10 interrupt flag */
#define ADC14_IFGR0_IFG11_OFS                    (11)                            /*!< ADC14IFG11 Bit Offset */
#define ADC14_IFGR0_IFG11                        ((uint32_t)0x00000800)          /*!< ADC14MEM11 interrupt flag */
#define ADC14_IFGR0_IFG12_OFS                    (12)                            /*!< ADC14IFG12 Bit Offset */
#define ADC14_IFGR0_IFG12                        ((uint32_t)0x00001000)          /*!< ADC14MEM12 interrupt flag */
#define ADC14_IFGR0_IFG13_OFS                    (13)                            /*!< ADC14IFG13 Bit Offset */
#define ADC14_IFGR0_IFG13                        ((uint32_t)0x00002000)          /*!< ADC14MEM13 interrupt flag */
#define ADC14_IFGR0_IFG14_OFS                    (14)                            /*!< ADC14IFG14 Bit Offset */
#define ADC14_IFGR0_IFG14                        ((uint32_t)0x00004000)          /*!< ADC14MEM14 interrupt flag */
#define ADC14_IFGR0_IFG15_OFS                    (15)                            /*!< ADC14IFG15 Bit Offset */
#define ADC14_IFGR0_IFG15                        ((uint32_t)0x00008000)          /*!< ADC14MEM15 interrupt flag */
#define ADC14_IFGR0_IFG16_OFS                    (16)                            /*!< ADC14IFG16 Bit Offset */
#define ADC14_IFGR0_IFG16                        ((uint32_t)0x00010000)          /*!< ADC14MEM16 interrupt flag */
#define ADC14_IFGR0_IFG17_OFS                    (17)                            /*!< ADC14IFG17 Bit Offset */
#define ADC14_IFGR0_IFG17                        ((uint32_t)0x00020000)          /*!< ADC14MEM17 interrupt flag */
#define ADC14_IFGR0_IFG18_OFS                    (18)                            /*!< ADC14IFG18 Bit Offset */
#define ADC14_IFGR0_IFG18                        ((uint32_t)0x00040000)          /*!< ADC14MEM18 interrupt flag */
#define ADC14_IFGR0_IFG19_OFS                    (19)                            /*!< ADC14IFG19 Bit Offset */
#define ADC14_IFGR0_IFG19                        ((uint32_t)0x00080000)          /*!< ADC14MEM19 interrupt flag */
#define ADC14_IFGR0_IFG20_OFS                    (20)                            /*!< ADC14IFG20 Bit Offset */
#define ADC14_IFGR0_IFG20                        ((uint32_t)0x00100000)          /*!< ADC14MEM20 interrupt flag */
#define ADC14_IFGR0_IFG21_OFS                    (21)                            /*!< ADC14IFG21 Bit Offset */
#define ADC14_IFGR0_IFG21                        ((uint32_t)0x00200000)          /*!< ADC14MEM21 interrupt flag */
#define ADC14_IFGR0_IFG22_OFS                    (22)                            /*!< ADC14IFG22 Bit Offset */
#define ADC14_IFGR0_IFG22                        ((uint32_t)0x00400000)          /*!< ADC14MEM22 interrupt flag */
#define ADC14_IFGR0_IFG23_OFS                    (23)                            /*!< ADC14IFG23 Bit Offset */
#define ADC14_IFGR0_IFG23                        ((uint32_t)0x00800000)          /*!< ADC14MEM23 interrupt flag */
#define ADC14_IFGR0_IFG24_OFS                    (24)                            /*!< ADC14IFG24 Bit Offset */
#define ADC14_IFGR0_IFG24                        ((uint32_t)0x01000000)          /*!< ADC14MEM24 interrupt flag */
#define ADC14_IFGR0_IFG25_OFS                    (25)                            /*!< ADC14IFG25 Bit Offset */
#define ADC14_IFGR0_IFG25                        ((uint32_t)0x02000000)          /*!< ADC14MEM25 interrupt flag */
#define ADC14_IFGR0_IFG26_OFS                    (26)                            /*!< ADC14IFG26 Bit Offset */
#define ADC14_IFGR0_IFG26                        ((uint32_t)0x04000000)          /*!< ADC14MEM26 interrupt flag */
#define ADC14_IFGR0_IFG27_OFS                    (27)                            /*!< ADC14IFG27 Bit Offset */
#define ADC14_IFGR0_IFG27                        ((uint32_t)0x08000000)          /*!< ADC14MEM27 interrupt flag */
#define ADC14_IFGR0_IFG28_OFS                    (28)                            /*!< ADC14IFG28 Bit Offset */
#define ADC14_IFGR0_IFG28                        ((uint32_t)0x10000000)          /*!< ADC14MEM28 interrupt flag */
#define ADC14_IFGR0_IFG29_OFS                    (29)                            /*!< ADC14IFG29 Bit Offset */
#define ADC14_IFGR0_IFG29                        ((uint32_t)0x20000000)          /*!< ADC14MEM29 interrupt flag */
#define ADC14_IFGR0_IFG30_OFS                    (30)                            /*!< ADC14IFG30 Bit Offset */
#define ADC14_IFGR0_IFG30                        ((uint32_t)0x40000000)          /*!< ADC14MEM30 interrupt flag */
#define ADC14_IFGR0_IFG31_OFS                    (31)                            /*!< ADC14IFG31 Bit Offset */
#define ADC14_IFGR0_IFG31                        ((uint32_t)0x80000000)          /*!< ADC14MEM31 interrupt flag */
#define ADC14_IFGR1_INIFG_OFS                    ( 1)                            /*!< ADC14INIFG Bit Offset */
#define ADC14_IFGR1_INIFG                        ((uint32_t)0x00000002)          /*!< Interrupt flag for ADC14MEMx within comparator window */
#define ADC14_IFGR1_LOIFG_OFS                    ( 2)                            /*!< ADC14LOIFG Bit Offset */
#define ADC14_IFGR1_LOIFG                        ((uint32_t)0x00000004)          /*!< Interrupt flag for ADC14MEMx below comparator window */
#define ADC14_IFGR1_HIIFG_OFS                    ( 3)                            /*!< ADC14HIIFG Bit Offset */
#define ADC14_IFGR1_HIIFG                        ((uint32_t)0x00000008)          /*!< Interrupt flag for ADC14MEMx above comparator window */
#define ADC14_IFGR1_OVIFG_OFS                    ( 4)                            /*!< ADC14OVIFG Bit Offset */
#define ADC14_IFGR1_OVIFG                        ((uint32_t)0x00000010)          /*!< ADC14MEMx overflow interrupt flag */
#define ADC14_IFGR1_TOVIFG_OFS                   ( 5)                            /*!< ADC14TOVIFG Bit Offset */
#define ADC14_IFGR1_TOVIFG                       ((uint32_t)0x00000020)          /*!< ADC14 conversion time overflow interrupt flag */
#define ADC14_IFGR1_RDYIFG_OFS                   ( 6)                            /*!< ADC14RDYIFG Bit Offset */
#define ADC14_IFGR1_RDYIFG                       ((uint32_t)0x00000040)          /*!< ADC14 local buffered reference ready interrupt flag */
#define ADC14_CLRIFGR0_CLRIFG0_OFS               ( 0)                            /*!< CLRADC14IFG0 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG0                   ((uint32_t)0x00000001)          /*!< clear ADC14IFG0 */
#define ADC14_CLRIFGR0_CLRIFG1_OFS               ( 1)                            /*!< CLRADC14IFG1 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG1                   ((uint32_t)0x00000002)          /*!< clear ADC14IFG1 */
#define ADC14_CLRIFGR0_CLRIFG2_OFS               ( 2)                            /*!< CLRADC14IFG2 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG2                   ((uint32_t)0x00000004)          /*!< clear ADC14IFG2 */
#define ADC14_CLRIFGR0_CLRIFG3_OFS               ( 3)                            /*!< CLRADC14IFG3 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG3                   ((uint32_t)0x00000008)          /*!< clear ADC14IFG3 */
#define ADC14_CLRIFGR0_CLRIFG4_OFS               ( 4)                            /*!< CLRADC14IFG4 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG4                   ((uint32_t)0x00000010)          /*!< clear ADC14IFG4 */
#define ADC14_CLRIFGR0_CLRIFG5_OFS               ( 5)                            /*!< CLRADC14IFG5 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG5                   ((uint32_t)0x00000020)          /*!< clear ADC14IFG5 */
#define ADC14_CLRIFGR0_CLRIFG6_OFS               ( 6)                            /*!< CLRADC14IFG6 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG6                   ((uint32_t)0x00000040)          /*!< clear ADC14IFG6 */
#define ADC14_CLRIFGR0_CLRIFG7_OFS               ( 7)                            /*!< CLRADC14IFG7 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG7                   ((uint32_t)0x00000080)          /*!< clear ADC14IFG7 */
#define ADC14_CLRIFGR0_CLRIFG8_OFS               ( 8)                            /*!< CLRADC14IFG8 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG8                   ((uint32_t)0x00000100)          /*!< clear ADC14IFG8 */
#define ADC14_CLRIFGR0_CLRIFG9_OFS               ( 9)                            /*!< CLRADC14IFG9 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG9                   ((uint32_t)0x00000200)          /*!< clear ADC14IFG9 */
#define ADC14_CLRIFGR0_CLRIFG10_OFS              (10)                            /*!< CLRADC14IFG10 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG10                  ((uint32_t)0x00000400)          /*!< clear ADC14IFG10 */
#define ADC14_CLRIFGR0_CLRIFG11_OFS              (11)                            /*!< CLRADC14IFG11 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG11                  ((uint32_t)0x00000800)          /*!< clear ADC14IFG11 */
#define ADC14_CLRIFGR0_CLRIFG12_OFS              (12)                            /*!< CLRADC14IFG12 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG12                  ((uint32_t)0x00001000)          /*!< clear ADC14IFG12 */
#define ADC14_CLRIFGR0_CLRIFG13_OFS              (13)                            /*!< CLRADC14IFG13 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG13                  ((uint32_t)0x00002000)          /*!< clear ADC14IFG13 */
#define ADC14_CLRIFGR0_CLRIFG14_OFS              (14)                            /*!< CLRADC14IFG14 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG14                  ((uint32_t)0x00004000)          /*!< clear ADC14IFG14 */
#define ADC14_CLRIFGR0_CLRIFG15_OFS              (15)                            /*!< CLRADC14IFG15 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG15                  ((uint32_t)0x00008000)          /*!< clear ADC14IFG15 */
#define ADC14_CLRIFGR0_CLRIFG16_OFS              (16)                            /*!< CLRADC14IFG16 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG16                  ((uint32_t)0x00010000)          /*!< clear ADC14IFG16 */
#define ADC14_CLRIFGR0_CLRIFG17_OFS              (17)                            /*!< CLRADC14IFG17 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG17                  ((uint32_t)0x00020000)          /*!< clear ADC14IFG17 */
#define ADC14_CLRIFGR0_CLRIFG18_OFS              (18)                            /*!< CLRADC14IFG18 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG18                  ((uint32_t)0x00040000)          /*!< clear ADC14IFG18 */
#define ADC14_CLRIFGR0_CLRIFG19_OFS              (19)                            /*!< CLRADC14IFG19 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG19                  ((uint32_t)0x00080000)          /*!< clear ADC14IFG19 */
#define ADC14_CLRIFGR0_CLRIFG20_OFS              (20)                            /*!< CLRADC14IFG20 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG20                  ((uint32_t)0x00100000)          /*!< clear ADC14IFG20 */
#define ADC14_CLRIFGR0_CLRIFG21_OFS              (21)                            /*!< CLRADC14IFG21 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG21                  ((uint32_t)0x00200000)          /*!< clear ADC14IFG21 */
#define ADC14_CLRIFGR0_CLRIFG22_OFS              (22)                            /*!< CLRADC14IFG22 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG22                  ((uint32_t)0x00400000)          /*!< clear ADC14IFG22 */
#define ADC14_CLRIFGR0_CLRIFG23_OFS              (23)                            /*!< CLRADC14IFG23 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG23                  ((uint32_t)0x00800000)          /*!< clear ADC14IFG23 */
#define ADC14_CLRIFGR0_CLRIFG24_OFS              (24)                            /*!< CLRADC14IFG24 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG24                  ((uint32_t)0x01000000)          /*!< clear ADC14IFG24 */
#define ADC14_CLRIFGR0_CLRIFG25_OFS              (25)                            /*!< CLRADC14IFG25 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG25                  ((uint32_t)0x02000000)          /*!< clear ADC14IFG25 */
#define ADC14_CLRIFGR0_CLRIFG26_OFS              (26)                            /*!< CLRADC14IFG26 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG26                  ((uint32_t)0x04000000)          /*!< clear ADC14IFG26 */
#define ADC14_CLRIFGR0_CLRIFG27_OFS              (27)                            /*!< CLRADC14IFG27 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG27                  ((uint32_t)0x08000000)          /*!< clear ADC14IFG27 */
#define ADC14_CLRIFGR0_CLRIFG28_OFS              (28)                            /*!< CLRADC14IFG28 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG28                  ((uint32_t)0x10000000)          /*!< clear ADC14IFG28 */
#define ADC14_CLRIFGR0_CLRIFG29_OFS              (29)                            /*!< CLRADC14IFG29 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG29                  ((uint32_t)0x20000000)          /*!< clear ADC14IFG29 */
#define ADC14_CLRIFGR0_CLRIFG30_OFS              (30)                            /*!< CLRADC14IFG30 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG30                  ((uint32_t)0x40000000)          /*!< clear ADC14IFG30 */
#define ADC14_CLRIFGR0_CLRIFG31_OFS              (31)                            /*!< CLRADC14IFG31 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG31                  ((uint32_t)0x80000000)          /*!< clear ADC14IFG31 */
#define ADC14_CLRIFGR1_CLRINIFG_OFS              ( 1)                            /*!< CLRADC14INIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRINIFG                  ((uint32_t)0x00000002)          /*!< clear ADC14INIFG */
#define ADC14_CLRIFGR1_CLRLOIFG_OFS              ( 2)                            /*!< CLRADC14LOIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRLOIFG                  ((uint32_t)0x00000004)          /*!< clear ADC14LOIFG */
#define ADC14_CLRIFGR1_CLRHIIFG_OFS              ( 3)                            /*!< CLRADC14HIIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRHIIFG                  ((uint32_t)0x00000008)          /*!< clear ADC14HIIFG */
#define ADC14_CLRIFGR1_CLROVIFG_OFS              ( 4)                            /*!< CLRADC14OVIFG Bit Offset */
#define ADC14_CLRIFGR1_CLROVIFG                  ((uint32_t)0x00000010)          /*!< clear ADC14OVIFG */
#define ADC14_CLRIFGR1_CLRTOVIFG_OFS             ( 5)                            /*!< CLRADC14TOVIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRTOVIFG                 ((uint32_t)0x00000020)          /*!< clear ADC14TOVIFG */
#define ADC14_CLRIFGR1_CLRRDYIFG_OFS             ( 6)                            /*!< CLRADC14RDYIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRRDYIFG                 ((uint32_t)0x00000040)          /*!< clear ADC14RDYIFG */
#define AES256_CTL0_OP_OFS                       ( 0)                            /*!< AESOPx Bit Offset */
#define AES256_CTL0_OP_MASK                      ((uint16_t)0x0003)              /*!< AESOPx Bit Mask */
#define AES256_CTL0_OP0                          ((uint16_t)0x0001)              /*!< OP Bit 0 */
#define AES256_CTL0_OP1                          ((uint16_t)0x0002)              /*!< OP Bit 1 */
#define AES256_CTL0_OP_0                         ((uint16_t)0x0000)              /*!< Encryption */
#define AES256_CTL0_OP_1                         ((uint16_t)0x0001)              /*!< Decryption. The provided key is the same key used for encryption */
#define AES256_CTL0_OP_2                         ((uint16_t)0x0002)              /*!< Generate first round key required for decryption */
#define AES256_CTL0_OP_3                         ((uint16_t)0x0003)              /*!< Decryption. The provided key is the first round key required for decryption */
#define AES256_CTL0_KL_OFS                       ( 2)                            /*!< AESKLx Bit Offset */
#define AES256_CTL0_KL_MASK                      ((uint16_t)0x000C)              /*!< AESKLx Bit Mask */
#define AES256_CTL0_KL0                          ((uint16_t)0x0004)              /*!< KL Bit 0 */
#define AES256_CTL0_KL1                          ((uint16_t)0x0008)              /*!< KL Bit 1 */
#define AES256_CTL0_KL_0                         ((uint16_t)0x0000)              /*!< AES128. The key size is 128 bit */
#define AES256_CTL0_KL_1                         ((uint16_t)0x0004)              /*!< AES192. The key size is 192 bit. */
#define AES256_CTL0_KL_2                         ((uint16_t)0x0008)              /*!< AES256. The key size is 256 bit */
#define AES256_CTL0_KL__128BIT                   ((uint16_t)0x0000)              /*!< AES128. The key size is 128 bit */
#define AES256_CTL0_KL__192BIT                   ((uint16_t)0x0004)              /*!< AES192. The key size is 192 bit. */
#define AES256_CTL0_KL__256BIT                   ((uint16_t)0x0008)              /*!< AES256. The key size is 256 bit */
#define AES256_CTL0_CM_OFS                       ( 5)                            /*!< AESCMx Bit Offset */
#define AES256_CTL0_CM_MASK                      ((uint16_t)0x0060)              /*!< AESCMx Bit Mask */
#define AES256_CTL0_CM0                          ((uint16_t)0x0020)              /*!< CM Bit 0 */
#define AES256_CTL0_CM1                          ((uint16_t)0x0040)              /*!< CM Bit 1 */
#define AES256_CTL0_CM_0                         ((uint16_t)0x0000)              /*!< ECB */
#define AES256_CTL0_CM_1                         ((uint16_t)0x0020)              /*!< CBC */
#define AES256_CTL0_CM_2                         ((uint16_t)0x0040)              /*!< OFB */
#define AES256_CTL0_CM_3                         ((uint16_t)0x0060)              /*!< CFB */
#define AES256_CTL0_CM__ECB                      ((uint16_t)0x0000)              /*!< ECB */
#define AES256_CTL0_CM__CBC                      ((uint16_t)0x0020)              /*!< CBC */
#define AES256_CTL0_CM__OFB                      ((uint16_t)0x0040)              /*!< OFB */
#define AES256_CTL0_CM__CFB                      ((uint16_t)0x0060)              /*!< CFB */
#define AES256_CTL0_SWRST_OFS                    ( 7)                            /*!< AESSWRST Bit Offset */
#define AES256_CTL0_SWRST                        ((uint16_t)0x0080)              /*!< AES software reset */
#define AES256_CTL0_RDYIFG_OFS                   ( 8)                            /*!< AESRDYIFG Bit Offset */
#define AES256_CTL0_RDYIFG                       ((uint16_t)0x0100)              /*!< AES ready interrupt flag */
#define AES256_CTL0_ERRFG_OFS                    (11)                            /*!< AESERRFG Bit Offset */
#define AES256_CTL0_ERRFG                        ((uint16_t)0x0800)              /*!< AES error flag */
#define AES256_CTL0_RDYIE_OFS                    (12)                            /*!< AESRDYIE Bit Offset */
#define AES256_CTL0_RDYIE                        ((uint16_t)0x1000)              /*!< AES ready interrupt enable */
#define AES256_CTL0_CMEN_OFS                     (15)                            /*!< AESCMEN Bit Offset */
#define AES256_CTL0_CMEN                         ((uint16_t)0x8000)              /*!< AES cipher mode enable */
#define AES256_CTL1_BLKCNT_OFS                   ( 0)                            /*!< AESBLKCNTx Bit Offset */
#define AES256_CTL1_BLKCNT_MASK                  ((uint16_t)0x00FF)              /*!< AESBLKCNTx Bit Mask */
#define AES256_CTL1_BLKCNT0                      ((uint16_t)0x0001)              /*!< BLKCNT Bit 0 */
#define AES256_CTL1_BLKCNT1                      ((uint16_t)0x0002)              /*!< BLKCNT Bit 1 */
#define AES256_CTL1_BLKCNT2                      ((uint16_t)0x0004)              /*!< BLKCNT Bit 2 */
#define AES256_CTL1_BLKCNT3                      ((uint16_t)0x0008)              /*!< BLKCNT Bit 3 */
#define AES256_CTL1_BLKCNT4                      ((uint16_t)0x0010)              /*!< BLKCNT Bit 4 */
#define AES256_CTL1_BLKCNT5                      ((uint16_t)0x0020)              /*!< BLKCNT Bit 5 */
#define AES256_CTL1_BLKCNT6                      ((uint16_t)0x0040)              /*!< BLKCNT Bit 6 */
#define AES256_CTL1_BLKCNT7                      ((uint16_t)0x0080)              /*!< BLKCNT Bit 7 */
#define AES256_STAT_BUSY_OFS                     ( 0)                            /*!< AESBUSY Bit Offset */
#define AES256_STAT_BUSY                         ((uint16_t)0x0001)              /*!< AES accelerator module busy */
#define AES256_STAT_KEYWR_OFS                    ( 1)                            /*!< AESKEYWR Bit Offset */
#define AES256_STAT_KEYWR                        ((uint16_t)0x0002)              /*!< All 16 bytes written to AESAKEY */
#define AES256_STAT_DINWR_OFS                    ( 2)                            /*!< AESDINWR Bit Offset */
#define AES256_STAT_DINWR                        ((uint16_t)0x0004)              /*!< All 16 bytes written to AESADIN, AESAXDIN or AESAXIN */
#define AES256_STAT_DOUTRD_OFS                   ( 3)                            /*!< AESDOUTRD Bit Offset */
#define AES256_STAT_DOUTRD                       ((uint16_t)0x0008)              /*!< All 16 bytes read from AESADOUT */
#define AES256_STAT_KEYCNT_OFS                   ( 4)                            /*!< AESKEYCNTx Bit Offset */
#define AES256_STAT_KEYCNT_MASK                  ((uint16_t)0x00F0)              /*!< AESKEYCNTx Bit Mask */
#define AES256_STAT_KEYCNT0                      ((uint16_t)0x0010)              /*!< KEYCNT Bit 0 */
#define AES256_STAT_KEYCNT1                      ((uint16_t)0x0020)              /*!< KEYCNT Bit 1 */
#define AES256_STAT_KEYCNT2                      ((uint16_t)0x0040)              /*!< KEYCNT Bit 2 */
#define AES256_STAT_KEYCNT3                      ((uint16_t)0x0080)              /*!< KEYCNT Bit 3 */
#define AES256_STAT_DINCNT_OFS                   ( 8)                            /*!< AESDINCNTx Bit Offset */
#define AES256_STAT_DINCNT_MASK                  ((uint16_t)0x0F00)              /*!< AESDINCNTx Bit Mask */
#define AES256_STAT_DINCNT0                      ((uint16_t)0x0100)              /*!< DINCNT Bit 0 */
#define AES256_STAT_DINCNT1                      ((uint16_t)0x0200)              /*!< DINCNT Bit 1 */
#define AES256_STAT_DINCNT2                      ((uint16_t)0x0400)              /*!< DINCNT Bit 2 */
#define AES256_STAT_DINCNT3                      ((uint16_t)0x0800)              /*!< DINCNT Bit 3 */
#define AES256_STAT_DOUTCNT_OFS                  (12)                            /*!< AESDOUTCNTx Bit Offset */
#define AES256_STAT_DOUTCNT_MASK                 ((uint16_t)0xF000)              /*!< AESDOUTCNTx Bit Mask */
#define AES256_STAT_DOUTCNT0                     ((uint16_t)0x1000)              /*!< DOUTCNT Bit 0 */
#define AES256_STAT_DOUTCNT1                     ((uint16_t)0x2000)              /*!< DOUTCNT Bit 1 */
#define AES256_STAT_DOUTCNT2                     ((uint16_t)0x4000)              /*!< DOUTCNT Bit 2 */
#define AES256_STAT_DOUTCNT3                     ((uint16_t)0x8000)              /*!< DOUTCNT Bit 3 */
#define AES256_KEY_KEY0_OFS                      ( 0)                            /*!< AESKEY0x Bit Offset */
#define AES256_KEY_KEY0_MASK                     ((uint16_t)0x00FF)              /*!< AESKEY0x Bit Mask */
#define AES256_KEY_KEY00                         ((uint16_t)0x0001)              /*!< KEY0 Bit 0 */
#define AES256_KEY_KEY01                         ((uint16_t)0x0002)              /*!< KEY0 Bit 1 */
#define AES256_KEY_KEY02                         ((uint16_t)0x0004)              /*!< KEY0 Bit 2 */
#define AES256_KEY_KEY03                         ((uint16_t)0x0008)              /*!< KEY0 Bit 3 */
#define AES256_KEY_KEY04                         ((uint16_t)0x0010)              /*!< KEY0 Bit 4 */
#define AES256_KEY_KEY05                         ((uint16_t)0x0020)              /*!< KEY0 Bit 5 */
#define AES256_KEY_KEY06                         ((uint16_t)0x0040)              /*!< KEY0 Bit 6 */
#define AES256_KEY_KEY07                         ((uint16_t)0x0080)              /*!< KEY0 Bit 7 */
#define AES256_KEY_KEY1_OFS                      ( 8)                            /*!< AESKEY1x Bit Offset */
#define AES256_KEY_KEY1_MASK                     ((uint16_t)0xFF00)              /*!< AESKEY1x Bit Mask */
#define AES256_KEY_KEY10                         ((uint16_t)0x0100)              /*!< KEY1 Bit 0 */
#define AES256_KEY_KEY11                         ((uint16_t)0x0200)              /*!< KEY1 Bit 1 */
#define AES256_KEY_KEY12                         ((uint16_t)0x0400)              /*!< KEY1 Bit 2 */
#define AES256_KEY_KEY13                         ((uint16_t)0x0800)              /*!< KEY1 Bit 3 */
#define AES256_KEY_KEY14                         ((uint16_t)0x1000)              /*!< KEY1 Bit 4 */
#define AES256_KEY_KEY15                         ((uint16_t)0x2000)              /*!< KEY1 Bit 5 */
#define AES256_KEY_KEY16                         ((uint16_t)0x4000)              /*!< KEY1 Bit 6 */
#define AES256_KEY_KEY17                         ((uint16_t)0x8000)              /*!< KEY1 Bit 7 */
#define AES256_DIN_DIN0_OFS                      ( 0)                            /*!< AESDIN0x Bit Offset */
#define AES256_DIN_DIN0_MASK                     ((uint16_t)0x00FF)              /*!< AESDIN0x Bit Mask */
#define AES256_DIN_DIN00                         ((uint16_t)0x0001)              /*!< DIN0 Bit 0 */
#define AES256_DIN_DIN01                         ((uint16_t)0x0002)              /*!< DIN0 Bit 1 */
#define AES256_DIN_DIN02                         ((uint16_t)0x0004)              /*!< DIN0 Bit 2 */
#define AES256_DIN_DIN03                         ((uint16_t)0x0008)              /*!< DIN0 Bit 3 */
#define AES256_DIN_DIN04                         ((uint16_t)0x0010)              /*!< DIN0 Bit 4 */
#define AES256_DIN_DIN05                         ((uint16_t)0x0020)              /*!< DIN0 Bit 5 */
#define AES256_DIN_DIN06                         ((uint16_t)0x0040)              /*!< DIN0 Bit 6 */
#define AES256_DIN_DIN07                         ((uint16_t)0x0080)              /*!< DIN0 Bit 7 */
#define AES256_DIN_DIN1_OFS                      ( 8)                            /*!< AESDIN1x Bit Offset */
#define AES256_DIN_DIN1_MASK                     ((uint16_t)0xFF00)              /*!< AESDIN1x Bit Mask */
#define AES256_DIN_DIN10                         ((uint16_t)0x0100)              /*!< DIN1 Bit 0 */
#define AES256_DIN_DIN11                         ((uint16_t)0x0200)              /*!< DIN1 Bit 1 */
#define AES256_DIN_DIN12                         ((uint16_t)0x0400)              /*!< DIN1 Bit 2 */
#define AES256_DIN_DIN13                         ((uint16_t)0x0800)              /*!< DIN1 Bit 3 */
#define AES256_DIN_DIN14                         ((uint16_t)0x1000)              /*!< DIN1 Bit 4 */
#define AES256_DIN_DIN15                         ((uint16_t)0x2000)              /*!< DIN1 Bit 5 */
#define AES256_DIN_DIN16                         ((uint16_t)0x4000)              /*!< DIN1 Bit 6 */
#define AES256_DIN_DIN17                         ((uint16_t)0x8000)              /*!< DIN1 Bit 7 */
#define AES256_DOUT_DOUT0_OFS                    ( 0)                            /*!< AESDOUT0x Bit Offset */
#define AES256_DOUT_DOUT0_MASK                   ((uint16_t)0x00FF)              /*!< AESDOUT0x Bit Mask */
#define AES256_DOUT_DOUT00                       ((uint16_t)0x0001)              /*!< DOUT0 Bit 0 */
#define AES256_DOUT_DOUT01                       ((uint16_t)0x0002)              /*!< DOUT0 Bit 1 */
#define AES256_DOUT_DOUT02                       ((uint16_t)0x0004)              /*!< DOUT0 Bit 2 */
#define AES256_DOUT_DOUT03                       ((uint16_t)0x0008)              /*!< DOUT0 Bit 3 */
#define AES256_DOUT_DOUT04                       ((uint16_t)0x0010)              /*!< DOUT0 Bit 4 */
#define AES256_DOUT_DOUT05                       ((uint16_t)0x0020)              /*!< DOUT0 Bit 5 */
#define AES256_DOUT_DOUT06                       ((uint16_t)0x0040)              /*!< DOUT0 Bit 6 */
#define AES256_DOUT_DOUT07                       ((uint16_t)0x0080)              /*!< DOUT0 Bit 7 */
#define AES256_DOUT_DOUT1_OFS                    ( 8)                            /*!< AESDOUT1x Bit Offset */
#define AES256_DOUT_DOUT1_MASK                   ((uint16_t)0xFF00)              /*!< AESDOUT1x Bit Mask */
#define AES256_DOUT_DOUT10                       ((uint16_t)0x0100)              /*!< DOUT1 Bit 0 */
#define AES256_DOUT_DOUT11                       ((uint16_t)0x0200)              /*!< DOUT1 Bit 1 */
#define AES256_DOUT_DOUT12                       ((uint16_t)0x0400)              /*!< DOUT1 Bit 2 */
#define AES256_DOUT_DOUT13                       ((uint16_t)0x0800)              /*!< DOUT1 Bit 3 */
#define AES256_DOUT_DOUT14                       ((uint16_t)0x1000)              /*!< DOUT1 Bit 4 */
#define AES256_DOUT_DOUT15                       ((uint16_t)0x2000)              /*!< DOUT1 Bit 5 */
#define AES256_DOUT_DOUT16                       ((uint16_t)0x4000)              /*!< DOUT1 Bit 6 */
#define AES256_DOUT_DOUT17                       ((uint16_t)0x8000)              /*!< DOUT1 Bit 7 */
#define AES256_XDIN_XDIN0_OFS                    ( 0)                            /*!< AESXDIN0x Bit Offset */
#define AES256_XDIN_XDIN0_MASK                   ((uint16_t)0x00FF)              /*!< AESXDIN0x Bit Mask */
#define AES256_XDIN_XDIN00                       ((uint16_t)0x0001)              /*!< XDIN0 Bit 0 */
#define AES256_XDIN_XDIN01                       ((uint16_t)0x0002)              /*!< XDIN0 Bit 1 */
#define AES256_XDIN_XDIN02                       ((uint16_t)0x0004)              /*!< XDIN0 Bit 2 */
#define AES256_XDIN_XDIN03                       ((uint16_t)0x0008)              /*!< XDIN0 Bit 3 */
#define AES256_XDIN_XDIN04                       ((uint16_t)0x0010)              /*!< XDIN0 Bit 4 */
#define AES256_XDIN_XDIN05                       ((uint16_t)0x0020)              /*!< XDIN0 Bit 5 */
#define AES256_XDIN_XDIN06                       ((uint16_t)0x0040)              /*!< XDIN0 Bit 6 */
#define AES256_XDIN_XDIN07                       ((uint16_t)0x0080)              /*!< XDIN0 Bit 7 */
#define AES256_XDIN_XDIN1_OFS                    ( 8)                            /*!< AESXDIN1x Bit Offset */
#define AES256_XDIN_XDIN1_MASK                   ((uint16_t)0xFF00)              /*!< AESXDIN1x Bit Mask */
#define AES256_XDIN_XDIN10                       ((uint16_t)0x0100)              /*!< XDIN1 Bit 0 */
#define AES256_XDIN_XDIN11                       ((uint16_t)0x0200)              /*!< XDIN1 Bit 1 */
#define AES256_XDIN_XDIN12                       ((uint16_t)0x0400)              /*!< XDIN1 Bit 2 */
#define AES256_XDIN_XDIN13                       ((uint16_t)0x0800)              /*!< XDIN1 Bit 3 */
#define AES256_XDIN_XDIN14                       ((uint16_t)0x1000)              /*!< XDIN1 Bit 4 */
#define AES256_XDIN_XDIN15                       ((uint16_t)0x2000)              /*!< XDIN1 Bit 5 */
#define AES256_XDIN_XDIN16                       ((uint16_t)0x4000)              /*!< XDIN1 Bit 6 */
#define AES256_XDIN_XDIN17                       ((uint16_t)0x8000)              /*!< XDIN1 Bit 7 */
#define AES256_XIN_XIN0_OFS                      ( 0)                            /*!< AESXIN0x Bit Offset */
#define AES256_XIN_XIN0_MASK                     ((uint16_t)0x00FF)              /*!< AESXIN0x Bit Mask */
#define AES256_XIN_XIN00                         ((uint16_t)0x0001)              /*!< XIN0 Bit 0 */
#define AES256_XIN_XIN01                         ((uint16_t)0x0002)              /*!< XIN0 Bit 1 */
#define AES256_XIN_XIN02                         ((uint16_t)0x0004)              /*!< XIN0 Bit 2 */
#define AES256_XIN_XIN03                         ((uint16_t)0x0008)              /*!< XIN0 Bit 3 */
#define AES256_XIN_XIN04                         ((uint16_t)0x0010)              /*!< XIN0 Bit 4 */
#define AES256_XIN_XIN05                         ((uint16_t)0x0020)              /*!< XIN0 Bit 5 */
#define AES256_XIN_XIN06                         ((uint16_t)0x0040)              /*!< XIN0 Bit 6 */
#define AES256_XIN_XIN07                         ((uint16_t)0x0080)              /*!< XIN0 Bit 7 */
#define AES256_XIN_XIN1_OFS                      ( 8)                            /*!< AESXIN1x Bit Offset */
#define AES256_XIN_XIN1_MASK                     ((uint16_t)0xFF00)              /*!< AESXIN1x Bit Mask */
#define AES256_XIN_XIN10                         ((uint16_t)0x0100)              /*!< XIN1 Bit 0 */
#define AES256_XIN_XIN11                         ((uint16_t)0x0200)              /*!< XIN1 Bit 1 */
#define AES256_XIN_XIN12                         ((uint16_t)0x0400)              /*!< XIN1 Bit 2 */
#define AES256_XIN_XIN13                         ((uint16_t)0x0800)              /*!< XIN1 Bit 3 */
#define AES256_XIN_XIN14                         ((uint16_t)0x1000)              /*!< XIN1 Bit 4 */
#define AES256_XIN_XIN15                         ((uint16_t)0x2000)              /*!< XIN1 Bit 5 */
#define AES256_XIN_XIN16                         ((uint16_t)0x4000)              /*!< XIN1 Bit 6 */
#define AES256_XIN_XIN17                         ((uint16_t)0x8000)              /*!< XIN1 Bit 7 */
#define CAPTIO_CTL_PISEL_OFS                     ( 1)                            /*!< CAPTIOPISELx Bit Offset */
#define CAPTIO_CTL_PISEL_MASK                    ((uint16_t)0x000E)              /*!< CAPTIOPISELx Bit Mask */
#define CAPTIO_CTL_PISEL0                        ((uint16_t)0x0002)              /*!< PISEL Bit 0 */
#define CAPTIO_CTL_PISEL1                        ((uint16_t)0x0004)              /*!< PISEL Bit 1 */
#define CAPTIO_CTL_PISEL2                        ((uint16_t)0x0008)              /*!< PISEL Bit 2 */
#define CAPTIO_CTL_PISEL_0                       ((uint16_t)0x0000)              /*!< Px.0 */
#define CAPTIO_CTL_PISEL_1                       ((uint16_t)0x0002)              /*!< Px.1 */
#define CAPTIO_CTL_PISEL_2                       ((uint16_t)0x0004)              /*!< Px.2 */
#define CAPTIO_CTL_PISEL_3                       ((uint16_t)0x0006)              /*!< Px.3 */
#define CAPTIO_CTL_PISEL_4                       ((uint16_t)0x0008)              /*!< Px.4 */
#define CAPTIO_CTL_PISEL_5                       ((uint16_t)0x000A)              /*!< Px.5 */
#define CAPTIO_CTL_PISEL_6                       ((uint16_t)0x000C)              /*!< Px.6 */
#define CAPTIO_CTL_PISEL_7                       ((uint16_t)0x000E)              /*!< Px.7 */
#define CAPTIO_CTL_POSEL_OFS                     ( 4)                            /*!< CAPTIOPOSELx Bit Offset */
#define CAPTIO_CTL_POSEL_MASK                    ((uint16_t)0x00F0)              /*!< CAPTIOPOSELx Bit Mask */
#define CAPTIO_CTL_POSEL0                        ((uint16_t)0x0010)              /*!< POSEL Bit 0 */
#define CAPTIO_CTL_POSEL1                        ((uint16_t)0x0020)              /*!< POSEL Bit 1 */
#define CAPTIO_CTL_POSEL2                        ((uint16_t)0x0040)              /*!< POSEL Bit 2 */
#define CAPTIO_CTL_POSEL3                        ((uint16_t)0x0080)              /*!< POSEL Bit 3 */
#define CAPTIO_CTL_POSEL_0                       ((uint16_t)0x0000)              /*!< Px = PJ */
#define CAPTIO_CTL_POSEL_1                       ((uint16_t)0x0010)              /*!< Px = P1 */
#define CAPTIO_CTL_POSEL_2                       ((uint16_t)0x0020)              /*!< Px = P2 */
#define CAPTIO_CTL_POSEL_3                       ((uint16_t)0x0030)              /*!< Px = P3 */
#define CAPTIO_CTL_POSEL_4                       ((uint16_t)0x0040)              /*!< Px = P4 */
#define CAPTIO_CTL_POSEL_5                       ((uint16_t)0x0050)              /*!< Px = P5 */
#define CAPTIO_CTL_POSEL_6                       ((uint16_t)0x0060)              /*!< Px = P6 */
#define CAPTIO_CTL_POSEL_7                       ((uint16_t)0x0070)              /*!< Px = P7 */
#define CAPTIO_CTL_POSEL_8                       ((uint16_t)0x0080)              /*!< Px = P8 */
#define CAPTIO_CTL_POSEL_9                       ((uint16_t)0x0090)              /*!< Px = P9 */
#define CAPTIO_CTL_POSEL_10                      ((uint16_t)0x00A0)              /*!< Px = P10 */
#define CAPTIO_CTL_POSEL_11                      ((uint16_t)0x00B0)              /*!< Px = P11 */
#define CAPTIO_CTL_POSEL_12                      ((uint16_t)0x00C0)              /*!< Px = P12 */
#define CAPTIO_CTL_POSEL_13                      ((uint16_t)0x00D0)              /*!< Px = P13 */
#define CAPTIO_CTL_POSEL_14                      ((uint16_t)0x00E0)              /*!< Px = P14 */
#define CAPTIO_CTL_POSEL_15                      ((uint16_t)0x00F0)              /*!< Px = P15 */
#define CAPTIO_CTL_POSEL__PJ                     ((uint16_t)0x0000)              /*!< Px = PJ */
#define CAPTIO_CTL_POSEL__P1                     ((uint16_t)0x0010)              /*!< Px = P1 */
#define CAPTIO_CTL_POSEL__P2                     ((uint16_t)0x0020)              /*!< Px = P2 */
#define CAPTIO_CTL_POSEL__P3                     ((uint16_t)0x0030)              /*!< Px = P3 */
#define CAPTIO_CTL_POSEL__P4                     ((uint16_t)0x0040)              /*!< Px = P4 */
#define CAPTIO_CTL_POSEL__P5                     ((uint16_t)0x0050)              /*!< Px = P5 */
#define CAPTIO_CTL_POSEL__P6                     ((uint16_t)0x0060)              /*!< Px = P6 */
#define CAPTIO_CTL_POSEL__P7                     ((uint16_t)0x0070)              /*!< Px = P7 */
#define CAPTIO_CTL_POSEL__P8                     ((uint16_t)0x0080)              /*!< Px = P8 */
#define CAPTIO_CTL_POSEL__P9                     ((uint16_t)0x0090)              /*!< Px = P9 */
#define CAPTIO_CTL_POSEL__P10                    ((uint16_t)0x00A0)              /*!< Px = P10 */
#define CAPTIO_CTL_POSEL__P11                    ((uint16_t)0x00B0)              /*!< Px = P11 */
#define CAPTIO_CTL_POSEL__P12                    ((uint16_t)0x00C0)              /*!< Px = P12 */
#define CAPTIO_CTL_POSEL__P13                    ((uint16_t)0x00D0)              /*!< Px = P13 */
#define CAPTIO_CTL_POSEL__P14                    ((uint16_t)0x00E0)              /*!< Px = P14 */
#define CAPTIO_CTL_POSEL__P15                    ((uint16_t)0x00F0)              /*!< Px = P15 */
#define CAPTIO_CTL_EN_OFS                        ( 8)                            /*!< CAPTIOEN Bit Offset */
#define CAPTIO_CTL_EN                            ((uint16_t)0x0100)              /*!< Capacitive Touch IO enable */
#define CAPTIO_CTL_STATE_OFS                     ( 9)                            /*!< CAPTIOSTATE Bit Offset */
#define CAPTIO_CTL_STATE                         ((uint16_t)0x0200)              /*!< Capacitive Touch IO state */
#define COMP_E_CTL0_IPSEL_OFS                    ( 0)                            /*!< CEIPSEL Bit Offset */
#define COMP_E_CTL0_IPSEL_MASK                   ((uint16_t)0x000F)              /*!< CEIPSEL Bit Mask */
#define COMP_E_CTL0_IPSEL0                       ((uint16_t)0x0001)              /*!< IPSEL Bit 0 */
#define COMP_E_CTL0_IPSEL1                       ((uint16_t)0x0002)              /*!< IPSEL Bit 1 */
#define COMP_E_CTL0_IPSEL2                       ((uint16_t)0x0004)              /*!< IPSEL Bit 2 */
#define COMP_E_CTL0_IPSEL3                       ((uint16_t)0x0008)              /*!< IPSEL Bit 3 */
#define COMP_E_CTL0_IPSEL_0                      ((uint16_t)0x0000)              /*!< Channel 0 selected */
#define COMP_E_CTL0_IPSEL_1                      ((uint16_t)0x0001)              /*!< Channel 1 selected */
#define COMP_E_CTL0_IPSEL_2                      ((uint16_t)0x0002)              /*!< Channel 2 selected */
#define COMP_E_CTL0_IPSEL_3                      ((uint16_t)0x0003)              /*!< Channel 3 selected */
#define COMP_E_CTL0_IPSEL_4                      ((uint16_t)0x0004)              /*!< Channel 4 selected */
#define COMP_E_CTL0_IPSEL_5                      ((uint16_t)0x0005)              /*!< Channel 5 selected */
#define COMP_E_CTL0_IPSEL_6                      ((uint16_t)0x0006)              /*!< Channel 6 selected */
#define COMP_E_CTL0_IPSEL_7                      ((uint16_t)0x0007)              /*!< Channel 7 selected */
#define COMP_E_CTL0_IPSEL_8                      ((uint16_t)0x0008)              /*!< Channel 8 selected */
#define COMP_E_CTL0_IPSEL_9                      ((uint16_t)0x0009)              /*!< Channel 9 selected */
#define COMP_E_CTL0_IPSEL_10                     ((uint16_t)0x000A)              /*!< Channel 10 selected */
#define COMP_E_CTL0_IPSEL_11                     ((uint16_t)0x000B)              /*!< Channel 11 selected */
#define COMP_E_CTL0_IPSEL_12                     ((uint16_t)0x000C)              /*!< Channel 12 selected */
#define COMP_E_CTL0_IPSEL_13                     ((uint16_t)0x000D)              /*!< Channel 13 selected */
#define COMP_E_CTL0_IPSEL_14                     ((uint16_t)0x000E)              /*!< Channel 14 selected */
#define COMP_E_CTL0_IPSEL_15                     ((uint16_t)0x000F)              /*!< Channel 15 selected */
#define COMP_E_CTL0_IPEN_OFS                     ( 7)                            /*!< CEIPEN Bit Offset */
#define COMP_E_CTL0_IPEN                         ((uint16_t)0x0080)              /*!< Channel input enable for the V+ terminal */
#define COMP_E_CTL0_IMSEL_OFS                    ( 8)                            /*!< CEIMSEL Bit Offset */
#define COMP_E_CTL0_IMSEL_MASK                   ((uint16_t)0x0F00)              /*!< CEIMSEL Bit Mask */
#define COMP_E_CTL0_IMSEL0                       ((uint16_t)0x0100)              /*!< IMSEL Bit 0 */
#define COMP_E_CTL0_IMSEL1                       ((uint16_t)0x0200)              /*!< IMSEL Bit 1 */
#define COMP_E_CTL0_IMSEL2                       ((uint16_t)0x0400)              /*!< IMSEL Bit 2 */
#define COMP_E_CTL0_IMSEL3                       ((uint16_t)0x0800)              /*!< IMSEL Bit 3 */
#define COMP_E_CTL0_IMSEL_0                      ((uint16_t)0x0000)              /*!< Channel 0 selected */
#define COMP_E_CTL0_IMSEL_1                      ((uint16_t)0x0100)              /*!< Channel 1 selected */
#define COMP_E_CTL0_IMSEL_2                      ((uint16_t)0x0200)              /*!< Channel 2 selected */
#define COMP_E_CTL0_IMSEL_3                      ((uint16_t)0x0300)              /*!< Channel 3 selected */
#define COMP_E_CTL0_IMSEL_4                      ((uint16_t)0x0400)              /*!< Channel 4 selected */
#define COMP_E_CTL0_IMSEL_5                      ((uint16_t)0x0500)              /*!< Channel 5 selected */
#define COMP_E_CTL0_IMSEL_6                      ((uint16_t)0x0600)              /*!< Channel 6 selected */
#define COMP_E_CTL0_IMSEL_7                      ((uint16_t)0x0700)              /*!< Channel 7 selected */
#define COMP_E_CTL0_IMSEL_8                      ((uint16_t)0x0800)              /*!< Channel 8 selected */
#define COMP_E_CTL0_IMSEL_9                      ((uint16_t)0x0900)              /*!< Channel 9 selected */
#define COMP_E_CTL0_IMSEL_10                     ((uint16_t)0x0A00)              /*!< Channel 10 selected */
#define COMP_E_CTL0_IMSEL_11                     ((uint16_t)0x0B00)              /*!< Channel 11 selected */
#define COMP_E_CTL0_IMSEL_12                     ((uint16_t)0x0C00)              /*!< Channel 12 selected */
#define COMP_E_CTL0_IMSEL_13                     ((uint16_t)0x0D00)              /*!< Channel 13 selected */
#define COMP_E_CTL0_IMSEL_14                     ((uint16_t)0x0E00)              /*!< Channel 14 selected */
#define COMP_E_CTL0_IMSEL_15                     ((uint16_t)0x0F00)              /*!< Channel 15 selected */
#define COMP_E_CTL0_IMEN_OFS                     (15)                            /*!< CEIMEN Bit Offset */
#define COMP_E_CTL0_IMEN                         ((uint16_t)0x8000)              /*!< Channel input enable for the - terminal */
#define COMP_E_CTL1_OUT_OFS                      ( 0)                            /*!< CEOUT Bit Offset */
#define COMP_E_CTL1_OUT                          ((uint16_t)0x0001)              /*!< Comparator output value */
#define COMP_E_CTL1_OUTPOL_OFS                   ( 1)                            /*!< CEOUTPOL Bit Offset */
#define COMP_E_CTL1_OUTPOL                       ((uint16_t)0x0002)              /*!< Comparator output polarity */
#define COMP_E_CTL1_F_OFS                        ( 2)                            /*!< CEF Bit Offset */
#define COMP_E_CTL1_F                            ((uint16_t)0x0004)              /*!< Comparator output filter */
#define COMP_E_CTL1_IES_OFS                      ( 3)                            /*!< CEIES Bit Offset */
#define COMP_E_CTL1_IES                          ((uint16_t)0x0008)              /*!< Interrupt edge select for CEIIFG and CEIFG */
#define COMP_E_CTL1_SHORT_OFS                    ( 4)                            /*!< CESHORT Bit Offset */
#define COMP_E_CTL1_SHORT                        ((uint16_t)0x0010)              /*!< Input short */
#define COMP_E_CTL1_EX_OFS                       ( 5)                            /*!< CEEX Bit Offset */
#define COMP_E_CTL1_EX                           ((uint16_t)0x0020)              /*!< Exchange */
#define COMP_E_CTL1_FDLY_OFS                     ( 6)                            /*!< CEFDLY Bit Offset */
#define COMP_E_CTL1_FDLY_MASK                    ((uint16_t)0x00C0)              /*!< CEFDLY Bit Mask */
#define COMP_E_CTL1_FDLY0                        ((uint16_t)0x0040)              /*!< FDLY Bit 0 */
#define COMP_E_CTL1_FDLY1                        ((uint16_t)0x0080)              /*!< FDLY Bit 1 */
#define COMP_E_CTL1_FDLY_0                       ((uint16_t)0x0000)              /*!< Typical filter delay of TBD (450) ns */
#define COMP_E_CTL1_FDLY_1                       ((uint16_t)0x0040)              /*!< Typical filter delay of TBD (900) ns */
#define COMP_E_CTL1_FDLY_2                       ((uint16_t)0x0080)              /*!< Typical filter delay of TBD (1800) ns */
#define COMP_E_CTL1_FDLY_3                       ((uint16_t)0x00C0)              /*!< Typical filter delay of TBD (3600) ns */
#define COMP_E_CTL1_PWRMD_OFS                    ( 8)                            /*!< CEPWRMD Bit Offset */
#define COMP_E_CTL1_PWRMD_MASK                   ((uint16_t)0x0300)              /*!< CEPWRMD Bit Mask */
#define COMP_E_CTL1_PWRMD0                       ((uint16_t)0x0100)              /*!< PWRMD Bit 0 */
#define COMP_E_CTL1_PWRMD1                       ((uint16_t)0x0200)              /*!< PWRMD Bit 1 */
#define COMP_E_CTL1_PWRMD_0                      ((uint16_t)0x0000)              /*!< High-speed mode */
#define COMP_E_CTL1_PWRMD_1                      ((uint16_t)0x0100)              /*!< Normal mode */
#define COMP_E_CTL1_PWRMD_2                      ((uint16_t)0x0200)              /*!< Ultra-low power mode */
#define COMP_E_CTL1_ON_OFS                       (10)                            /*!< CEON Bit Offset */
#define COMP_E_CTL1_ON                           ((uint16_t)0x0400)              /*!< Comparator On */
#define COMP_E_CTL1_MRVL_OFS                     (11)                            /*!< CEMRVL Bit Offset */
#define COMP_E_CTL1_MRVL                         ((uint16_t)0x0800)              /*!< This bit is valid of CEMRVS is set to 1 */
#define COMP_E_CTL1_MRVS_OFS                     (12)                            /*!< CEMRVS Bit Offset */
#define COMP_E_CTL1_MRVS                         ((uint16_t)0x1000)              
#define COMP_E_CTL2_REF0_OFS                     ( 0)                            /*!< CEREF0 Bit Offset */
#define COMP_E_CTL2_REF0_MASK                    ((uint16_t)0x001F)              /*!< CEREF0 Bit Mask */
#define COMP_E_CTL2_REF00                        ((uint16_t)0x0001)              /*!< REF0 Bit 0 */
#define COMP_E_CTL2_REF01                        ((uint16_t)0x0002)              /*!< REF0 Bit 1 */
#define COMP_E_CTL2_REF02                        ((uint16_t)0x0004)              /*!< REF0 Bit 2 */
#define COMP_E_CTL2_REF03                        ((uint16_t)0x0008)              /*!< REF0 Bit 3 */
#define COMP_E_CTL2_REF04                        ((uint16_t)0x0010)              /*!< REF0 Bit 4 */
#define COMP_E_CTL2_REF0_0                       ((uint16_t)0x0000)              /*!< Reference resistor tap for setting 0. */
#define COMP_E_CTL2_REF0_1                       ((uint16_t)0x0001)              /*!< Reference resistor tap for setting 1. */
#define COMP_E_CTL2_REF0_2                       ((uint16_t)0x0002)              /*!< Reference resistor tap for setting 2. */
#define COMP_E_CTL2_REF0_3                       ((uint16_t)0x0003)              /*!< Reference resistor tap for setting 3. */
#define COMP_E_CTL2_REF0_4                       ((uint16_t)0x0004)              /*!< Reference resistor tap for setting 4. */
#define COMP_E_CTL2_REF0_5                       ((uint16_t)0x0005)              /*!< Reference resistor tap for setting 5. */
#define COMP_E_CTL2_REF0_6                       ((uint16_t)0x0006)              /*!< Reference resistor tap for setting 6. */
#define COMP_E_CTL2_REF0_7                       ((uint16_t)0x0007)              /*!< Reference resistor tap for setting 7. */
#define COMP_E_CTL2_REF0_8                       ((uint16_t)0x0008)              /*!< Reference resistor tap for setting 8. */
#define COMP_E_CTL2_REF0_9                       ((uint16_t)0x0009)              /*!< Reference resistor tap for setting 9. */
#define COMP_E_CTL2_REF0_10                      ((uint16_t)0x000A)              /*!< Reference resistor tap for setting 10. */
#define COMP_E_CTL2_REF0_11                      ((uint16_t)0x000B)              /*!< Reference resistor tap for setting 11. */
#define COMP_E_CTL2_REF0_12                      ((uint16_t)0x000C)              /*!< Reference resistor tap for setting 12. */
#define COMP_E_CTL2_REF0_13                      ((uint16_t)0x000D)              /*!< Reference resistor tap for setting 13. */
#define COMP_E_CTL2_REF0_14                      ((uint16_t)0x000E)              /*!< Reference resistor tap for setting 14. */
#define COMP_E_CTL2_REF0_15                      ((uint16_t)0x000F)              /*!< Reference resistor tap for setting 15. */
#define COMP_E_CTL2_REF0_16                      ((uint16_t)0x0010)              /*!< Reference resistor tap for setting 16. */
#define COMP_E_CTL2_REF0_17                      ((uint16_t)0x0011)              /*!< Reference resistor tap for setting 17. */
#define COMP_E_CTL2_REF0_18                      ((uint16_t)0x0012)              /*!< Reference resistor tap for setting 18. */
#define COMP_E_CTL2_REF0_19                      ((uint16_t)0x0013)              /*!< Reference resistor tap for setting 19. */
#define COMP_E_CTL2_REF0_20                      ((uint16_t)0x0014)              /*!< Reference resistor tap for setting 20. */
#define COMP_E_CTL2_REF0_21                      ((uint16_t)0x0015)              /*!< Reference resistor tap for setting 21. */
#define COMP_E_CTL2_REF0_22                      ((uint16_t)0x0016)              /*!< Reference resistor tap for setting 22. */
#define COMP_E_CTL2_REF0_23                      ((uint16_t)0x0017)              /*!< Reference resistor tap for setting 23. */
#define COMP_E_CTL2_REF0_24                      ((uint16_t)0x0018)              /*!< Reference resistor tap for setting 24. */
#define COMP_E_CTL2_REF0_25                      ((uint16_t)0x0019)              /*!< Reference resistor tap for setting 25. */
#define COMP_E_CTL2_REF0_26                      ((uint16_t)0x001A)              /*!< Reference resistor tap for setting 26. */
#define COMP_E_CTL2_REF0_27                      ((uint16_t)0x001B)              /*!< Reference resistor tap for setting 27. */
#define COMP_E_CTL2_REF0_28                      ((uint16_t)0x001C)              /*!< Reference resistor tap for setting 28. */
#define COMP_E_CTL2_REF0_29                      ((uint16_t)0x001D)              /*!< Reference resistor tap for setting 29. */
#define COMP_E_CTL2_REF0_30                      ((uint16_t)0x001E)              /*!< Reference resistor tap for setting 30. */
#define COMP_E_CTL2_REF0_31                      ((uint16_t)0x001F)              /*!< Reference resistor tap for setting 31. */
#define COMP_E_CTL2_RSEL_OFS                     ( 5)                            /*!< CERSEL Bit Offset */
#define COMP_E_CTL2_RSEL                         ((uint16_t)0x0020)              /*!< Reference select */
#define COMP_E_CTL2_RS_OFS                       ( 6)                            /*!< CERS Bit Offset */
#define COMP_E_CTL2_RS_MASK                      ((uint16_t)0x00C0)              /*!< CERS Bit Mask */
#define COMP_E_CTL2_RS0                          ((uint16_t)0x0040)              /*!< RS Bit 0 */
#define COMP_E_CTL2_RS1                          ((uint16_t)0x0080)              /*!< RS Bit 1 */
#define COMP_E_CTL2_RS_0                         ((uint16_t)0x0000)              /*!< No current is drawn by the reference circuitry */
#define COMP_E_CTL2_RS_1                         ((uint16_t)0x0040)              /*!< VCC applied to the resistor ladder */
#define COMP_E_CTL2_RS_2                         ((uint16_t)0x0080)              /*!< Shared reference voltage applied to the resistor ladder */
#define COMP_E_CTL2_RS_3                         ((uint16_t)0x00C0)              /*!< Shared reference voltage supplied to V(CREF). Resistor ladder is off */
#define COMP_E_CTL2_REF1_OFS                     ( 8)                            /*!< CEREF1 Bit Offset */
#define COMP_E_CTL2_REF1_MASK                    ((uint16_t)0x1F00)              /*!< CEREF1 Bit Mask */
#define COMP_E_CTL2_REF10                        ((uint16_t)0x0100)              /*!< REF1 Bit 0 */
#define COMP_E_CTL2_REF11                        ((uint16_t)0x0200)              /*!< REF1 Bit 1 */
#define COMP_E_CTL2_REF12                        ((uint16_t)0x0400)              /*!< REF1 Bit 2 */
#define COMP_E_CTL2_REF13                        ((uint16_t)0x0800)              /*!< REF1 Bit 3 */
#define COMP_E_CTL2_REF14                        ((uint16_t)0x1000)              /*!< REF1 Bit 4 */
#define COMP_E_CTL2_REF1_0                       ((uint16_t)0x0000)              /*!< Reference resistor tap for setting 0. */
#define COMP_E_CTL2_REF1_1                       ((uint16_t)0x0100)              /*!< Reference resistor tap for setting 1. */
#define COMP_E_CTL2_REF1_2                       ((uint16_t)0x0200)              /*!< Reference resistor tap for setting 2. */
#define COMP_E_CTL2_REF1_3                       ((uint16_t)0x0300)              /*!< Reference resistor tap for setting 3. */
#define COMP_E_CTL2_REF1_4                       ((uint16_t)0x0400)              /*!< Reference resistor tap for setting 4. */
#define COMP_E_CTL2_REF1_5                       ((uint16_t)0x0500)              /*!< Reference resistor tap for setting 5. */
#define COMP_E_CTL2_REF1_6                       ((uint16_t)0x0600)              /*!< Reference resistor tap for setting 6. */
#define COMP_E_CTL2_REF1_7                       ((uint16_t)0x0700)              /*!< Reference resistor tap for setting 7. */
#define COMP_E_CTL2_REF1_8                       ((uint16_t)0x0800)              /*!< Reference resistor tap for setting 8. */
#define COMP_E_CTL2_REF1_9                       ((uint16_t)0x0900)              /*!< Reference resistor tap for setting 9. */
#define COMP_E_CTL2_REF1_10                      ((uint16_t)0x0A00)              /*!< Reference resistor tap for setting 10. */
#define COMP_E_CTL2_REF1_11                      ((uint16_t)0x0B00)              /*!< Reference resistor tap for setting 11. */
#define COMP_E_CTL2_REF1_12                      ((uint16_t)0x0C00)              /*!< Reference resistor tap for setting 12. */
#define COMP_E_CTL2_REF1_13                      ((uint16_t)0x0D00)              /*!< Reference resistor tap for setting 13. */
#define COMP_E_CTL2_REF1_14                      ((uint16_t)0x0E00)              /*!< Reference resistor tap for setting 14. */
#define COMP_E_CTL2_REF1_15                      ((uint16_t)0x0F00)              /*!< Reference resistor tap for setting 15. */
#define COMP_E_CTL2_REF1_16                      ((uint16_t)0x1000)              /*!< Reference resistor tap for setting 16. */
#define COMP_E_CTL2_REF1_17                      ((uint16_t)0x1100)              /*!< Reference resistor tap for setting 17. */
#define COMP_E_CTL2_REF1_18                      ((uint16_t)0x1200)              /*!< Reference resistor tap for setting 18. */
#define COMP_E_CTL2_REF1_19                      ((uint16_t)0x1300)              /*!< Reference resistor tap for setting 19. */
#define COMP_E_CTL2_REF1_20                      ((uint16_t)0x1400)              /*!< Reference resistor tap for setting 20. */
#define COMP_E_CTL2_REF1_21                      ((uint16_t)0x1500)              /*!< Reference resistor tap for setting 21. */
#define COMP_E_CTL2_REF1_22                      ((uint16_t)0x1600)              /*!< Reference resistor tap for setting 22. */
#define COMP_E_CTL2_REF1_23                      ((uint16_t)0x1700)              /*!< Reference resistor tap for setting 23. */
#define COMP_E_CTL2_REF1_24                      ((uint16_t)0x1800)              /*!< Reference resistor tap for setting 24. */
#define COMP_E_CTL2_REF1_25                      ((uint16_t)0x1900)              /*!< Reference resistor tap for setting 25. */
#define COMP_E_CTL2_REF1_26                      ((uint16_t)0x1A00)              /*!< Reference resistor tap for setting 26. */
#define COMP_E_CTL2_REF1_27                      ((uint16_t)0x1B00)              /*!< Reference resistor tap for setting 27. */
#define COMP_E_CTL2_REF1_28                      ((uint16_t)0x1C00)              /*!< Reference resistor tap for setting 28. */
#define COMP_E_CTL2_REF1_29                      ((uint16_t)0x1D00)              /*!< Reference resistor tap for setting 29. */
#define COMP_E_CTL2_REF1_30                      ((uint16_t)0x1E00)              /*!< Reference resistor tap for setting 30. */
#define COMP_E_CTL2_REF1_31                      ((uint16_t)0x1F00)              /*!< Reference resistor tap for setting 31. */
#define COMP_E_CTL2_REFL_OFS                     (13)                            /*!< CEREFL Bit Offset */
#define COMP_E_CTL2_REFL_MASK                    ((uint16_t)0x6000)              /*!< CEREFL Bit Mask */
#define COMP_E_CTL2_REFL0                        ((uint16_t)0x2000)              /*!< REFL Bit 0 */
#define COMP_E_CTL2_REFL1                        ((uint16_t)0x4000)              /*!< REFL Bit 1 */
#define COMP_E_CTL2_CEREFL_0                     ((uint16_t)0x0000)              /*!< Reference amplifier is disabled. No reference voltage is requested */
#define COMP_E_CTL2_CEREFL_1                     ((uint16_t)0x2000)              /*!< 1.2 V is selected as shared reference voltage input */
#define COMP_E_CTL2_CEREFL_2                     ((uint16_t)0x4000)              /*!< 2.0 V is selected as shared reference voltage input */
#define COMP_E_CTL2_CEREFL_3                     ((uint16_t)0x6000)              /*!< 2.5 V is selected as shared reference voltage input */
#define COMP_E_CTL2_REFL__OFF                    ((uint16_t)0x0000)              /*!< Reference amplifier is disabled. No reference voltage is requested */
#define COMP_E_CTL2_REFL__1P2V                   ((uint16_t)0x2000)              /*!< 1.2 V is selected as shared reference voltage input */
#define COMP_E_CTL2_REFL__2P0V                   ((uint16_t)0x4000)              /*!< 2.0 V is selected as shared reference voltage input */
#define COMP_E_CTL2_REFL__2P5V                   ((uint16_t)0x6000)              /*!< 2.5 V is selected as shared reference voltage input */
#define COMP_E_CTL2_REFACC_OFS                   (15)                            /*!< CEREFACC Bit Offset */
#define COMP_E_CTL2_REFACC                       ((uint16_t)0x8000)              /*!< Reference accuracy */
#define COMP_E_CTL3_PD0_OFS                      ( 0)                            /*!< CEPD0 Bit Offset */
#define COMP_E_CTL3_PD0                          ((uint16_t)0x0001)              /*!< Port disable */
#define COMP_E_CTL3_PD1_OFS                      ( 1)                            /*!< CEPD1 Bit Offset */
#define COMP_E_CTL3_PD1                          ((uint16_t)0x0002)              /*!< Port disable */
#define COMP_E_CTL3_PD2_OFS                      ( 2)                            /*!< CEPD2 Bit Offset */
#define COMP_E_CTL3_PD2                          ((uint16_t)0x0004)              /*!< Port disable */
#define COMP_E_CTL3_PD3_OFS                      ( 3)                            /*!< CEPD3 Bit Offset */
#define COMP_E_CTL3_PD3                          ((uint16_t)0x0008)              /*!< Port disable */
#define COMP_E_CTL3_PD4_OFS                      ( 4)                            /*!< CEPD4 Bit Offset */
#define COMP_E_CTL3_PD4                          ((uint16_t)0x0010)              /*!< Port disable */
#define COMP_E_CTL3_PD5_OFS                      ( 5)                            /*!< CEPD5 Bit Offset */
#define COMP_E_CTL3_PD5                          ((uint16_t)0x0020)              /*!< Port disable */
#define COMP_E_CTL3_PD6_OFS                      ( 6)                            /*!< CEPD6 Bit Offset */
#define COMP_E_CTL3_PD6                          ((uint16_t)0x0040)              /*!< Port disable */
#define COMP_E_CTL3_PD7_OFS                      ( 7)                            /*!< CEPD7 Bit Offset */
#define COMP_E_CTL3_PD7                          ((uint16_t)0x0080)              /*!< Port disable */
#define COMP_E_CTL3_PD8_OFS                      ( 8)                            /*!< CEPD8 Bit Offset */
#define COMP_E_CTL3_PD8                          ((uint16_t)0x0100)              /*!< Port disable */
#define COMP_E_CTL3_PD9_OFS                      ( 9)                            /*!< CEPD9 Bit Offset */
#define COMP_E_CTL3_PD9                          ((uint16_t)0x0200)              /*!< Port disable */
#define COMP_E_CTL3_PD10_OFS                     (10)                            /*!< CEPD10 Bit Offset */
#define COMP_E_CTL3_PD10                         ((uint16_t)0x0400)              /*!< Port disable */
#define COMP_E_CTL3_PD11_OFS                     (11)                            /*!< CEPD11 Bit Offset */
#define COMP_E_CTL3_PD11                         ((uint16_t)0x0800)              /*!< Port disable */
#define COMP_E_CTL3_PD12_OFS                     (12)                            /*!< CEPD12 Bit Offset */
#define COMP_E_CTL3_PD12                         ((uint16_t)0x1000)              /*!< Port disable */
#define COMP_E_CTL3_PD13_OFS                     (13)                            /*!< CEPD13 Bit Offset */
#define COMP_E_CTL3_PD13                         ((uint16_t)0x2000)              /*!< Port disable */
#define COMP_E_CTL3_PD14_OFS                     (14)                            /*!< CEPD14 Bit Offset */
#define COMP_E_CTL3_PD14                         ((uint16_t)0x4000)              /*!< Port disable */
#define COMP_E_CTL3_PD15_OFS                     (15)                            /*!< CEPD15 Bit Offset */
#define COMP_E_CTL3_PD15                         ((uint16_t)0x8000)              /*!< Port disable */
#define COMP_E_INT_IFG_OFS                       ( 0)                            /*!< CEIFG Bit Offset */
#define COMP_E_INT_IFG                           ((uint16_t)0x0001)              /*!< Comparator output interrupt flag */
#define COMP_E_INT_IIFG_OFS                      ( 1)                            /*!< CEIIFG Bit Offset */
#define COMP_E_INT_IIFG                          ((uint16_t)0x0002)              /*!< Comparator output inverted interrupt flag */
#define COMP_E_INT_RDYIFG_OFS                    ( 4)                            /*!< CERDYIFG Bit Offset */
#define COMP_E_INT_RDYIFG                        ((uint16_t)0x0010)              /*!< Comparator ready interrupt flag */
#define COMP_E_INT_IE_OFS                        ( 8)                            /*!< CEIE Bit Offset */
#define COMP_E_INT_IE                            ((uint16_t)0x0100)              /*!< Comparator output interrupt enable */
#define COMP_E_INT_IIE_OFS                       ( 9)                            /*!< CEIIE Bit Offset */
#define COMP_E_INT_IIE                           ((uint16_t)0x0200)              /*!< Comparator output interrupt enable inverted polarity */
#define COMP_E_INT_RDYIE_OFS                     (12)                            /*!< CERDYIE Bit Offset */
#define COMP_E_INT_RDYIE                         ((uint16_t)0x1000)              /*!< Comparator ready interrupt enable */
#define CS_KEY_KEY_OFS                           ( 0)                            /*!< CSKEY Bit Offset */
#define CS_KEY_KEY_MASK                          ((uint32_t)0x0000FFFF)          /*!< CSKEY Bit Mask */
#define CS_CTL0_DCOTUNE_OFS                      ( 0)                            /*!< DCOTUNE Bit Offset */
#define CS_CTL0_DCOTUNE_MASK                     ((uint32_t)0x000003FF)          /*!< DCOTUNE Bit Mask */
#define CS_CTL0_DCORSEL_OFS                      (16)                            /*!< DCORSEL Bit Offset */
#define CS_CTL0_DCORSEL_MASK                     ((uint32_t)0x00070000)          /*!< DCORSEL Bit Mask */
#define CS_CTL0_DCORSEL0                         ((uint32_t)0x00010000)          /*!< DCORSEL Bit 0 */
#define CS_CTL0_DCORSEL1                         ((uint32_t)0x00020000)          /*!< DCORSEL Bit 1 */
#define CS_CTL0_DCORSEL2                         ((uint32_t)0x00040000)          /*!< DCORSEL Bit 2 */
#define CS_CTL0_DCORSEL_0                        ((uint32_t)0x00000000)          /*!< Nominal DCO Frequency Range (MHz): 1 to 2 */
#define CS_CTL0_DCORSEL_1                        ((uint32_t)0x00010000)          /*!< Nominal DCO Frequency Range (MHz): 2 to 4 */
#define CS_CTL0_DCORSEL_2                        ((uint32_t)0x00020000)          /*!< Nominal DCO Frequency Range (MHz): 4 to 8 */
#define CS_CTL0_DCORSEL_3                        ((uint32_t)0x00030000)          /*!< Nominal DCO Frequency Range (MHz): 8 to 16 */
#define CS_CTL0_DCORSEL_4                        ((uint32_t)0x00040000)          /*!< Nominal DCO Frequency Range (MHz): 16 to 32 */
#define CS_CTL0_DCORSEL_5                        ((uint32_t)0x00050000)          /*!< Nominal DCO Frequency Range (MHz): 32 to 64 */
#define CS_CTL0_DCORES_OFS                       (22)                            /*!< DCORES Bit Offset */
#define CS_CTL0_DCORES                           ((uint32_t)0x00400000)          /*!< Enables the DCO external resistor mode */
#define CS_CTL0_DCOEN_OFS                        (23)                            /*!< DCOEN Bit Offset */
#define CS_CTL0_DCOEN                            ((uint32_t)0x00800000)          /*!< Enables the DCO oscillator */
#define CS_CTL1_SELM_OFS                         ( 0)                            /*!< SELM Bit Offset */
#define CS_CTL1_SELM_MASK                        ((uint32_t)0x00000007)          /*!< SELM Bit Mask */
#define CS_CTL1_SELM0                            ((uint32_t)0x00000001)          /*!< SELM Bit 0 */
#define CS_CTL1_SELM1                            ((uint32_t)0x00000002)          /*!< SELM Bit 1 */
#define CS_CTL1_SELM2                            ((uint32_t)0x00000004)          /*!< SELM Bit 2 */
#define CS_CTL1_SELM_0                           ((uint32_t)0x00000000)          /*!< when LFXT available, otherwise REFOCLK */
#define CS_CTL1_SELM_1                           ((uint32_t)0x00000001)          
#define CS_CTL1_SELM_2                           ((uint32_t)0x00000002)          
#define CS_CTL1_SELM_3                           ((uint32_t)0x00000003)          
#define CS_CTL1_SELM_4                           ((uint32_t)0x00000004)          
#define CS_CTL1_SELM_5                           ((uint32_t)0x00000005)          /*!< when HFXT available, otherwise DCOCLK */
#define CS_CTL1_SELM_6                           ((uint32_t)0x00000006)          /*!< when HFXT2 available, otherwise DCOCLK */
#define CS_CTL1_SELM__LFXTCLK                    ((uint32_t)0x00000000)          /*!< when LFXT available, otherwise REFOCLK */
#define CS_CTL1_SELM__VLOCLK                     ((uint32_t)0x00000001)          
#define CS_CTL1_SELM__REFOCLK                    ((uint32_t)0x00000002)          
#define CS_CTL1_SELM__DCOCLK                     ((uint32_t)0x00000003)          
#define CS_CTL1_SELM__MODOSC                     ((uint32_t)0x00000004)          
#define CS_CTL1_SELM__HFXTCLK                    ((uint32_t)0x00000005)          /*!< when HFXT available, otherwise DCOCLK */
#define CS_CTL1_SELM__HFXT2CLK                   ((uint32_t)0x00000006)          /*!< when HFXT2 available, otherwise DCOCLK */
#define CS_CTL1_SELS_OFS                         ( 4)                            /*!< SELS Bit Offset */
#define CS_CTL1_SELS_MASK                        ((uint32_t)0x00000070)          /*!< SELS Bit Mask */
#define CS_CTL1_SELS0                            ((uint32_t)0x00000010)          /*!< SELS Bit 0 */
#define CS_CTL1_SELS1                            ((uint32_t)0x00000020)          /*!< SELS Bit 1 */
#define CS_CTL1_SELS2                            ((uint32_t)0x00000040)          /*!< SELS Bit 2 */
#define CS_CTL1_SELS_0                           ((uint32_t)0x00000000)          /*!< when LFXT available, otherwise REFOCLK */
#define CS_CTL1_SELS_1                           ((uint32_t)0x00000010)          
#define CS_CTL1_SELS_2                           ((uint32_t)0x00000020)          
#define CS_CTL1_SELS_3                           ((uint32_t)0x00000030)          
#define CS_CTL1_SELS_4                           ((uint32_t)0x00000040)          
#define CS_CTL1_SELS_5                           ((uint32_t)0x00000050)          /*!< when HFXT available, otherwise DCOCLK */
#define CS_CTL1_SELS_6                           ((uint32_t)0x00000060)          /*!< when HFXT2 available, otherwise DCOCLK */
#define CS_CTL1_SELS__LFXTCLK                    ((uint32_t)0x00000000)          /*!< when LFXT available, otherwise REFOCLK */
#define CS_CTL1_SELS__VLOCLK                     ((uint32_t)0x00000010)          
#define CS_CTL1_SELS__REFOCLK                    ((uint32_t)0x00000020)          
#define CS_CTL1_SELS__DCOCLK                     ((uint32_t)0x00000030)          
#define CS_CTL1_SELS__MODOSC                     ((uint32_t)0x00000040)          
#define CS_CTL1_SELS__HFXTCLK                    ((uint32_t)0x00000050)          /*!< when HFXT available, otherwise DCOCLK */
#define CS_CTL1_SELS__HFXT2CLK                   ((uint32_t)0x00000060)          /*!< when HFXT2 available, otherwise DCOCLK */
#define CS_CTL1_SELA_OFS                         ( 8)                            /*!< SELA Bit Offset */
#define CS_CTL1_SELA_MASK                        ((uint32_t)0x00000700)          /*!< SELA Bit Mask */
#define CS_CTL1_SELA0                            ((uint32_t)0x00000100)          /*!< SELA Bit 0 */
#define CS_CTL1_SELA1                            ((uint32_t)0x00000200)          /*!< SELA Bit 1 */
#define CS_CTL1_SELA2                            ((uint32_t)0x00000400)          /*!< SELA Bit 2 */
#define CS_CTL1_SELA_0                           ((uint32_t)0x00000000)          /*!< when LFXT available, otherwise REFOCLK */
#define CS_CTL1_SELA_1                           ((uint32_t)0x00000100)          
#define CS_CTL1_SELA_2                           ((uint32_t)0x00000200)          
#define CS_CTL1_SELA__LFXTCLK                    ((uint32_t)0x00000000)          /*!< when LFXT available, otherwise REFOCLK */
#define CS_CTL1_SELA__VLOCLK                     ((uint32_t)0x00000100)          
#define CS_CTL1_SELA__REFOCLK                    ((uint32_t)0x00000200)          
#define CS_CTL1_SELB_OFS                         (12)                            /*!< SELB Bit Offset */
#define CS_CTL1_SELB                             ((uint32_t)0x00001000)          /*!< Selects the BCLK source */
#define CS_CTL1_DIVM_OFS                         (16)                            /*!< DIVM Bit Offset */
#define CS_CTL1_DIVM_MASK                        ((uint32_t)0x00070000)          /*!< DIVM Bit Mask */
#define CS_CTL1_DIVM0                            ((uint32_t)0x00010000)          /*!< DIVM Bit 0 */
#define CS_CTL1_DIVM1                            ((uint32_t)0x00020000)          /*!< DIVM Bit 1 */
#define CS_CTL1_DIVM2                            ((uint32_t)0x00040000)          /*!< DIVM Bit 2 */
#define CS_CTL1_DIVM_0                           ((uint32_t)0x00000000)          /*!< f(MCLK)/1 */
#define CS_CTL1_DIVM_1                           ((uint32_t)0x00010000)          /*!< f(MCLK)/2 */
#define CS_CTL1_DIVM_2                           ((uint32_t)0x00020000)          /*!< f(MCLK)/4 */
#define CS_CTL1_DIVM_3                           ((uint32_t)0x00030000)          /*!< f(MCLK)/8 */
#define CS_CTL1_DIVM_4                           ((uint32_t)0x00040000)          /*!< f(MCLK)/16 */
#define CS_CTL1_DIVM_5                           ((uint32_t)0x00050000)          /*!< f(MCLK)/32 */
#define CS_CTL1_DIVM_6                           ((uint32_t)0x00060000)          /*!< f(MCLK)/64 */
#define CS_CTL1_DIVM_7                           ((uint32_t)0x00070000)          /*!< f(MCLK)/128 */
#define CS_CTL1_DIVM__1                          ((uint32_t)0x00000000)          /*!< f(MCLK)/1 */
#define CS_CTL1_DIVM__2                          ((uint32_t)0x00010000)          /*!< f(MCLK)/2 */
#define CS_CTL1_DIVM__4                          ((uint32_t)0x00020000)          /*!< f(MCLK)/4 */
#define CS_CTL1_DIVM__8                          ((uint32_t)0x00030000)          /*!< f(MCLK)/8 */
#define CS_CTL1_DIVM__16                         ((uint32_t)0x00040000)          /*!< f(MCLK)/16 */
#define CS_CTL1_DIVM__32                         ((uint32_t)0x00050000)          /*!< f(MCLK)/32 */
#define CS_CTL1_DIVM__64                         ((uint32_t)0x00060000)          /*!< f(MCLK)/64 */
#define CS_CTL1_DIVM__128                        ((uint32_t)0x00070000)          /*!< f(MCLK)/128 */
#define CS_CTL1_DIVHS_OFS                        (20)                            /*!< DIVHS Bit Offset */
#define CS_CTL1_DIVHS_MASK                       ((uint32_t)0x00700000)          /*!< DIVHS Bit Mask */
#define CS_CTL1_DIVHS0                           ((uint32_t)0x00100000)          /*!< DIVHS Bit 0 */
#define CS_CTL1_DIVHS1                           ((uint32_t)0x00200000)          /*!< DIVHS Bit 1 */
#define CS_CTL1_DIVHS2                           ((uint32_t)0x00400000)          /*!< DIVHS Bit 2 */
#define CS_CTL1_DIVHS_0                          ((uint32_t)0x00000000)          /*!< f(HSMCLK)/1 */
#define CS_CTL1_DIVHS_1                          ((uint32_t)0x00100000)          /*!< f(HSMCLK)/2 */
#define CS_CTL1_DIVHS_2                          ((uint32_t)0x00200000)          /*!< f(HSMCLK)/4 */
#define CS_CTL1_DIVHS_3                          ((uint32_t)0x00300000)          /*!< f(HSMCLK)/8 */
#define CS_CTL1_DIVHS_4                          ((uint32_t)0x00400000)          /*!< f(HSMCLK)/16 */
#define CS_CTL1_DIVHS_5                          ((uint32_t)0x00500000)          /*!< f(HSMCLK)/32 */
#define CS_CTL1_DIVHS_6                          ((uint32_t)0x00600000)          /*!< f(HSMCLK)/64 */
#define CS_CTL1_DIVHS_7                          ((uint32_t)0x00700000)          /*!< f(HSMCLK)/128 */
#define CS_CTL1_DIVHS__1                         ((uint32_t)0x00000000)          /*!< f(HSMCLK)/1 */
#define CS_CTL1_DIVHS__2                         ((uint32_t)0x00100000)          /*!< f(HSMCLK)/2 */
#define CS_CTL1_DIVHS__4                         ((uint32_t)0x00200000)          /*!< f(HSMCLK)/4 */
#define CS_CTL1_DIVHS__8                         ((uint32_t)0x00300000)          /*!< f(HSMCLK)/8 */
#define CS_CTL1_DIVHS__16                        ((uint32_t)0x00400000)          /*!< f(HSMCLK)/16 */
#define CS_CTL1_DIVHS__32                        ((uint32_t)0x00500000)          /*!< f(HSMCLK)/32 */
#define CS_CTL1_DIVHS__64                        ((uint32_t)0x00600000)          /*!< f(HSMCLK)/64 */
#define CS_CTL1_DIVHS__128                       ((uint32_t)0x00700000)          /*!< f(HSMCLK)/128 */
#define CS_CTL1_DIVA_OFS                         (24)                            /*!< DIVA Bit Offset */
#define CS_CTL1_DIVA_MASK                        ((uint32_t)0x07000000)          /*!< DIVA Bit Mask */
#define CS_CTL1_DIVA0                            ((uint32_t)0x01000000)          /*!< DIVA Bit 0 */
#define CS_CTL1_DIVA1                            ((uint32_t)0x02000000)          /*!< DIVA Bit 1 */
#define CS_CTL1_DIVA2                            ((uint32_t)0x04000000)          /*!< DIVA Bit 2 */
#define CS_CTL1_DIVA_0                           ((uint32_t)0x00000000)          /*!< f(ACLK)/1 */
#define CS_CTL1_DIVA_1                           ((uint32_t)0x01000000)          /*!< f(ACLK)/2 */
#define CS_CTL1_DIVA_2                           ((uint32_t)0x02000000)          /*!< f(ACLK)/4 */
#define CS_CTL1_DIVA_3                           ((uint32_t)0x03000000)          /*!< f(ACLK)/8 */
#define CS_CTL1_DIVA_4                           ((uint32_t)0x04000000)          /*!< f(ACLK)/16 */
#define CS_CTL1_DIVA_5                           ((uint32_t)0x05000000)          /*!< f(ACLK)/32 */
#define CS_CTL1_DIVA_6                           ((uint32_t)0x06000000)          /*!< f(ACLK)/64 */
#define CS_CTL1_DIVA_7                           ((uint32_t)0x07000000)          /*!< f(ACLK)/128 */
#define CS_CTL1_DIVA__1                          ((uint32_t)0x00000000)          /*!< f(ACLK)/1 */
#define CS_CTL1_DIVA__2                          ((uint32_t)0x01000000)          /*!< f(ACLK)/2 */
#define CS_CTL1_DIVA__4                          ((uint32_t)0x02000000)          /*!< f(ACLK)/4 */
#define CS_CTL1_DIVA__8                          ((uint32_t)0x03000000)          /*!< f(ACLK)/8 */
#define CS_CTL1_DIVA__16                         ((uint32_t)0x04000000)          /*!< f(ACLK)/16 */
#define CS_CTL1_DIVA__32                         ((uint32_t)0x05000000)          /*!< f(ACLK)/32 */
#define CS_CTL1_DIVA__64                         ((uint32_t)0x06000000)          /*!< f(ACLK)/64 */
#define CS_CTL1_DIVA__128                        ((uint32_t)0x07000000)          /*!< f(ACLK)/128 */
#define CS_CTL1_DIVS_OFS                         (28)                            /*!< DIVS Bit Offset */
#define CS_CTL1_DIVS_MASK                        ((uint32_t)0x70000000)          /*!< DIVS Bit Mask */
#define CS_CTL1_DIVS0                            ((uint32_t)0x10000000)          /*!< DIVS Bit 0 */
#define CS_CTL1_DIVS1                            ((uint32_t)0x20000000)          /*!< DIVS Bit 1 */
#define CS_CTL1_DIVS2                            ((uint32_t)0x40000000)          /*!< DIVS Bit 2 */
#define CS_CTL1_DIVS_0                           ((uint32_t)0x00000000)          /*!< f(SMCLK)/1 */
#define CS_CTL1_DIVS_1                           ((uint32_t)0x10000000)          /*!< f(SMCLK)/2 */
#define CS_CTL1_DIVS_2                           ((uint32_t)0x20000000)          /*!< f(SMCLK)/4 */
#define CS_CTL1_DIVS_3                           ((uint32_t)0x30000000)          /*!< f(SMCLK)/8 */
#define CS_CTL1_DIVS_4                           ((uint32_t)0x40000000)          /*!< f(SMCLK)/16 */
#define CS_CTL1_DIVS_5                           ((uint32_t)0x50000000)          /*!< f(SMCLK)/32 */
#define CS_CTL1_DIVS_6                           ((uint32_t)0x60000000)          /*!< f(SMCLK)/64 */
#define CS_CTL1_DIVS_7                           ((uint32_t)0x70000000)          /*!< f(SMCLK)/128 */
#define CS_CTL1_DIVS__1                          ((uint32_t)0x00000000)          /*!< f(SMCLK)/1 */
#define CS_CTL1_DIVS__2                          ((uint32_t)0x10000000)          /*!< f(SMCLK)/2 */
#define CS_CTL1_DIVS__4                          ((uint32_t)0x20000000)          /*!< f(SMCLK)/4 */
#define CS_CTL1_DIVS__8                          ((uint32_t)0x30000000)          /*!< f(SMCLK)/8 */
#define CS_CTL1_DIVS__16                         ((uint32_t)0x40000000)          /*!< f(SMCLK)/16 */
#define CS_CTL1_DIVS__32                         ((uint32_t)0x50000000)          /*!< f(SMCLK)/32 */
#define CS_CTL1_DIVS__64                         ((uint32_t)0x60000000)          /*!< f(SMCLK)/64 */
#define CS_CTL1_DIVS__128                        ((uint32_t)0x70000000)          /*!< f(SMCLK)/128 */
#define CS_CTL2_LFXTDRIVE_OFS                    ( 0)                            /*!< LFXTDRIVE Bit Offset */
#define CS_CTL2_LFXTDRIVE_MASK                   ((uint32_t)0x00000003)          /*!< LFXTDRIVE Bit Mask */
#define CS_CTL2_LFXTDRIVE0                       ((uint32_t)0x00000001)          /*!< LFXTDRIVE Bit 0 */
#define CS_CTL2_LFXTDRIVE1                       ((uint32_t)0x00000002)          /*!< LFXTDRIVE Bit 1 */
#define CS_CTL2_LFXTDRIVE_0                      ((uint32_t)0x00000000)          /*!< Lowest drive strength and current consumption LFXT oscillator. */
#define CS_CTL2_LFXTDRIVE_1                      ((uint32_t)0x00000001)          /*!< Increased drive strength LFXT oscillator. */
#define CS_CTL2_LFXTDRIVE_2                      ((uint32_t)0x00000002)          /*!< Increased drive strength LFXT oscillator. */
#define CS_CTL2_LFXTDRIVE_3                      ((uint32_t)0x00000003)          /*!< Maximum drive strength and maximum current consumption LFXT oscillator. */
#define CS_CTL2_LFXT_EN_OFS                      ( 8)                            /*!< LFXT_EN Bit Offset */
#define CS_CTL2_LFXT_EN                          ((uint32_t)0x00000100)          /*!< Turns on the LFXT oscillator regardless if used as a clock resource */
#define CS_CTL2_LFXTBYPASS_OFS                   ( 9)                            /*!< LFXTBYPASS Bit Offset */
#define CS_CTL2_LFXTBYPASS                       ((uint32_t)0x00000200)          /*!< LFXT bypass select */
#define CS_CTL2_HFXTDRIVE_OFS                    (16)                            /*!< HFXTDRIVE Bit Offset */
#define CS_CTL2_HFXTDRIVE                        ((uint32_t)0x00010000)          /*!< HFXT oscillator drive selection */
#define CS_CTL2_HFXTFREQ_OFS                     (20)                            /*!< HFXTFREQ Bit Offset */
#define CS_CTL2_HFXTFREQ_MASK                    ((uint32_t)0x00700000)          /*!< HFXTFREQ Bit Mask */
#define CS_CTL2_HFXTFREQ0                        ((uint32_t)0x00100000)          /*!< HFXTFREQ Bit 0 */
#define CS_CTL2_HFXTFREQ1                        ((uint32_t)0x00200000)          /*!< HFXTFREQ Bit 1 */
#define CS_CTL2_HFXTFREQ2                        ((uint32_t)0x00400000)          /*!< HFXTFREQ Bit 2 */
#define CS_CTL2_HFXTFREQ_0                       ((uint32_t)0x00000000)          /*!< 1 MHz to 4 MHz */
#define CS_CTL2_HFXTFREQ_1                       ((uint32_t)0x00100000)          /*!< >4 MHz to 8 MHz */
#define CS_CTL2_HFXTFREQ_2                       ((uint32_t)0x00200000)          /*!< >8 MHz to 16 MHz */
#define CS_CTL2_HFXTFREQ_3                       ((uint32_t)0x00300000)          /*!< >16 MHz to 24 MHz */
#define CS_CTL2_HFXTFREQ_4                       ((uint32_t)0x00400000)          /*!< >24 MHz to 32 MHz */
#define CS_CTL2_HFXTFREQ_5                       ((uint32_t)0x00500000)          /*!< >32 MHz to 40 MHz */
#define CS_CTL2_HFXTFREQ_6                       ((uint32_t)0x00600000)          /*!< >40 MHz to 48 MHz */
#define CS_CTL2_HFXTFREQ_7                       ((uint32_t)0x00700000)          /*!< Reserved for future use. */
#define CS_CTL2_HFXT_EN_OFS                      (24)                            /*!< HFXT_EN Bit Offset */
#define CS_CTL2_HFXT_EN                          ((uint32_t)0x01000000)          /*!< Turns on the HFXT oscillator regardless if used as a clock resource */
#define CS_CTL2_HFXTBYPASS_OFS                   (25)                            /*!< HFXTBYPASS Bit Offset */
#define CS_CTL2_HFXTBYPASS                       ((uint32_t)0x02000000)          /*!< HFXT bypass select */
#define CS_CTL3_FCNTLF_OFS                       ( 0)                            /*!< FCNTLF Bit Offset */
#define CS_CTL3_FCNTLF_MASK                      ((uint32_t)0x00000003)          /*!< FCNTLF Bit Mask */
#define CS_CTL3_FCNTLF0                          ((uint32_t)0x00000001)          /*!< FCNTLF Bit 0 */
#define CS_CTL3_FCNTLF1                          ((uint32_t)0x00000002)          /*!< FCNTLF Bit 1 */
#define CS_CTL3_FCNTLF_0                         ((uint32_t)0x00000000)          /*!< 4096 cycles */
#define CS_CTL3_FCNTLF_1                         ((uint32_t)0x00000001)          /*!< 8192 cycles */
#define CS_CTL3_FCNTLF_2                         ((uint32_t)0x00000002)          /*!< 16384 cycles */
#define CS_CTL3_FCNTLF_3                         ((uint32_t)0x00000003)          /*!< 32768 cycles */
#define CS_CTL3_FCNTLF__4096                     ((uint32_t)0x00000000)          /*!< 4096 cycles */
#define CS_CTL3_FCNTLF__8192                     ((uint32_t)0x00000001)          /*!< 8192 cycles */
#define CS_CTL3_FCNTLF__16384                    ((uint32_t)0x00000002)          /*!< 16384 cycles */
#define CS_CTL3_FCNTLF__32768                    ((uint32_t)0x00000003)          /*!< 32768 cycles */
#define CS_CTL3_RFCNTLF_OFS                      ( 2)                            /*!< RFCNTLF Bit Offset */
#define CS_CTL3_RFCNTLF                          ((uint32_t)0x00000004)          /*!< Reset start fault counter for LFXT */
#define CS_CTL3_FCNTLF_EN_OFS                    ( 3)                            /*!< FCNTLF_EN Bit Offset */
#define CS_CTL3_FCNTLF_EN                        ((uint32_t)0x00000008)          /*!< Enable start fault counter for LFXT */
#define CS_CTL3_FCNTHF_OFS                       ( 4)                            /*!< FCNTHF Bit Offset */
#define CS_CTL3_FCNTHF_MASK                      ((uint32_t)0x00000030)          /*!< FCNTHF Bit Mask */
#define CS_CTL3_FCNTHF0                          ((uint32_t)0x00000010)          /*!< FCNTHF Bit 0 */
#define CS_CTL3_FCNTHF1                          ((uint32_t)0x00000020)          /*!< FCNTHF Bit 1 */
#define CS_CTL3_FCNTHF_0                         ((uint32_t)0x00000000)          /*!< 2048 cycles */
#define CS_CTL3_FCNTHF_1                         ((uint32_t)0x00000010)          /*!< 4096 cycles */
#define CS_CTL3_FCNTHF_2                         ((uint32_t)0x00000020)          /*!< 8192 cycles */
#define CS_CTL3_FCNTHF_3                         ((uint32_t)0x00000030)          /*!< 16384 cycles */
#define CS_CTL3_FCNTHF__2048                     ((uint32_t)0x00000000)          /*!< 2048 cycles */
#define CS_CTL3_FCNTHF__4096                     ((uint32_t)0x00000010)          /*!< 4096 cycles */
#define CS_CTL3_FCNTHF__8192                     ((uint32_t)0x00000020)          /*!< 8192 cycles */
#define CS_CTL3_FCNTHF__16384                    ((uint32_t)0x00000030)          /*!< 16384 cycles */
#define CS_CTL3_RFCNTHF_OFS                      ( 6)                            /*!< RFCNTHF Bit Offset */
#define CS_CTL3_RFCNTHF                          ((uint32_t)0x00000040)          /*!< Reset start fault counter for HFXT */
#define CS_CTL3_FCNTHF_EN_OFS                    ( 7)                            /*!< FCNTHF_EN Bit Offset */
#define CS_CTL3_FCNTHF_EN                        ((uint32_t)0x00000080)          /*!< Enable start fault counter for HFXT */
#define CS_CLKEN_ACLK_EN_OFS                     ( 0)                            /*!< ACLK_EN Bit Offset */
#define CS_CLKEN_ACLK_EN                         ((uint32_t)0x00000001)          /*!< ACLK system clock conditional request enable */
#define CS_CLKEN_MCLK_EN_OFS                     ( 1)                            /*!< MCLK_EN Bit Offset */
#define CS_CLKEN_MCLK_EN                         ((uint32_t)0x00000002)          /*!< MCLK system clock conditional request enable */
#define CS_CLKEN_HSMCLK_EN_OFS                   ( 2)                            /*!< HSMCLK_EN Bit Offset */
#define CS_CLKEN_HSMCLK_EN                       ((uint32_t)0x00000004)          /*!< HSMCLK system clock conditional request enable */
#define CS_CLKEN_SMCLK_EN_OFS                    ( 3)                            /*!< SMCLK_EN Bit Offset */
#define CS_CLKEN_SMCLK_EN                        ((uint32_t)0x00000008)          /*!< SMCLK system clock conditional request enable */
#define CS_CLKEN_VLO_EN_OFS                      ( 8)                            /*!< VLO_EN Bit Offset */
#define CS_CLKEN_VLO_EN                          ((uint32_t)0x00000100)          /*!< Turns on the VLO oscillator */
#define CS_CLKEN_REFO_EN_OFS                     ( 9)                            /*!< REFO_EN Bit Offset */
#define CS_CLKEN_REFO_EN                         ((uint32_t)0x00000200)          /*!< Turns on the REFO oscillator */
#define CS_CLKEN_MODOSC_EN_OFS                   (10)                            /*!< MODOSC_EN Bit Offset */
#define CS_CLKEN_MODOSC_EN                       ((uint32_t)0x00000400)          /*!< Turns on the MODOSC oscillator */
#define CS_CLKEN_REFOFSEL_OFS                    (15)                            /*!< REFOFSEL Bit Offset */
#define CS_CLKEN_REFOFSEL                        ((uint32_t)0x00008000)          /*!< Selects REFO nominal frequency */
#define CS_STAT_DCO_ON_OFS                       ( 0)                            /*!< DCO_ON Bit Offset */
#define CS_STAT_DCO_ON                           ((uint32_t)0x00000001)          /*!< DCO status */
#define CS_STAT_DCOBIAS_ON_OFS                   ( 1)                            /*!< DCOBIAS_ON Bit Offset */
#define CS_STAT_DCOBIAS_ON                       ((uint32_t)0x00000002)          /*!< DCO bias status */
#define CS_STAT_HFXT_ON_OFS                      ( 2)                            /*!< HFXT_ON Bit Offset */
#define CS_STAT_HFXT_ON                          ((uint32_t)0x00000004)          /*!< HFXT status */
#define CS_STAT_MODOSC_ON_OFS                    ( 4)                            /*!< MODOSC_ON Bit Offset */
#define CS_STAT_MODOSC_ON                        ((uint32_t)0x00000010)          /*!< MODOSC status */
#define CS_STAT_VLO_ON_OFS                       ( 5)                            /*!< VLO_ON Bit Offset */
#define CS_STAT_VLO_ON                           ((uint32_t)0x00000020)          /*!< VLO status */
#define CS_STAT_LFXT_ON_OFS                      ( 6)                            /*!< LFXT_ON Bit Offset */
#define CS_STAT_LFXT_ON                          ((uint32_t)0x00000040)          /*!< LFXT status */
#define CS_STAT_REFO_ON_OFS                      ( 7)                            /*!< REFO_ON Bit Offset */
#define CS_STAT_REFO_ON                          ((uint32_t)0x00000080)          /*!< REFO status */
#define CS_STAT_ACLK_ON_OFS                      (16)                            /*!< ACLK_ON Bit Offset */
#define CS_STAT_ACLK_ON                          ((uint32_t)0x00010000)          /*!< ACLK system clock status */
#define CS_STAT_MCLK_ON_OFS                      (17)                            /*!< MCLK_ON Bit Offset */
#define CS_STAT_MCLK_ON                          ((uint32_t)0x00020000)          /*!< MCLK system clock status */
#define CS_STAT_HSMCLK_ON_OFS                    (18)                            /*!< HSMCLK_ON Bit Offset */
#define CS_STAT_HSMCLK_ON                        ((uint32_t)0x00040000)          /*!< HSMCLK system clock status */
#define CS_STAT_SMCLK_ON_OFS                     (19)                            /*!< SMCLK_ON Bit Offset */
#define CS_STAT_SMCLK_ON                         ((uint32_t)0x00080000)          /*!< SMCLK system clock status */
#define CS_STAT_MODCLK_ON_OFS                    (20)                            /*!< MODCLK_ON Bit Offset */
#define CS_STAT_MODCLK_ON                        ((uint32_t)0x00100000)          /*!< MODCLK system clock status */
#define CS_STAT_VLOCLK_ON_OFS                    (21)                            /*!< VLOCLK_ON Bit Offset */
#define CS_STAT_VLOCLK_ON                        ((uint32_t)0x00200000)          /*!< VLOCLK system clock status */
#define CS_STAT_LFXTCLK_ON_OFS                   (22)                            /*!< LFXTCLK_ON Bit Offset */
#define CS_STAT_LFXTCLK_ON                       ((uint32_t)0x00400000)          /*!< LFXTCLK system clock status */
#define CS_STAT_REFOCLK_ON_OFS                   (23)                            /*!< REFOCLK_ON Bit Offset */
#define CS_STAT_REFOCLK_ON                       ((uint32_t)0x00800000)          /*!< REFOCLK system clock status */
#define CS_STAT_ACLK_READY_OFS                   (24)                            /*!< ACLK_READY Bit Offset */
#define CS_STAT_ACLK_READY                       ((uint32_t)0x01000000)          /*!< ACLK Ready status */
#define CS_STAT_MCLK_READY_OFS                   (25)                            /*!< MCLK_READY Bit Offset */
#define CS_STAT_MCLK_READY                       ((uint32_t)0x02000000)          /*!< MCLK Ready status */
#define CS_STAT_HSMCLK_READY_OFS                 (26)                            /*!< HSMCLK_READY Bit Offset */
#define CS_STAT_HSMCLK_READY                     ((uint32_t)0x04000000)          /*!< HSMCLK Ready status */
#define CS_STAT_SMCLK_READY_OFS                  (27)                            /*!< SMCLK_READY Bit Offset */
#define CS_STAT_SMCLK_READY                      ((uint32_t)0x08000000)          /*!< SMCLK Ready status */
#define CS_STAT_BCLK_READY_OFS                   (28)                            /*!< BCLK_READY Bit Offset */
#define CS_STAT_BCLK_READY                       ((uint32_t)0x10000000)          /*!< BCLK Ready status */
#define CS_IE_LFXTIE_OFS                         ( 0)                            /*!< LFXTIE Bit Offset */
#define CS_IE_LFXTIE                             ((uint32_t)0x00000001)          /*!< LFXT oscillator fault flag interrupt enable */
#define CS_IE_HFXTIE_OFS                         ( 1)                            /*!< HFXTIE Bit Offset */
#define CS_IE_HFXTIE                             ((uint32_t)0x00000002)          /*!< HFXT oscillator fault flag interrupt enable */
#define CS_IE_DCOR_OPNIE_OFS                     ( 6)                            /*!< DCOR_OPNIE Bit Offset */
#define CS_IE_DCOR_OPNIE                         ((uint32_t)0x00000040)          /*!< DCO external resistor open circuit fault flag interrupt enable. */
#define CS_IE_FCNTLFIE_OFS                       ( 8)                            /*!< FCNTLFIE Bit Offset */
#define CS_IE_FCNTLFIE                           ((uint32_t)0x00000100)          /*!< Start fault counter interrupt enable LFXT */
#define CS_IE_FCNTHFIE_OFS                       ( 9)                            /*!< FCNTHFIE Bit Offset */
#define CS_IE_FCNTHFIE                           ((uint32_t)0x00000200)          /*!< Start fault counter interrupt enable HFXT */
#define CS_IFG_LFXTIFG_OFS                       ( 0)                            /*!< LFXTIFG Bit Offset */
#define CS_IFG_LFXTIFG                           ((uint32_t)0x00000001)          /*!< LFXT oscillator fault flag */
#define CS_IFG_HFXTIFG_OFS                       ( 1)                            /*!< HFXTIFG Bit Offset */
#define CS_IFG_HFXTIFG                           ((uint32_t)0x00000002)          /*!< HFXT oscillator fault flag */
#define CS_IFG_DCOR_SHTIFG_OFS                   ( 5)                            /*!< DCOR_SHTIFG Bit Offset */
#define CS_IFG_DCOR_SHTIFG                       ((uint32_t)0x00000020)          /*!< DCO external resistor short circuit fault flag. */
#define CS_IFG_DCOR_OPNIFG_OFS                   ( 6)                            /*!< DCOR_OPNIFG Bit Offset */
#define CS_IFG_DCOR_OPNIFG                       ((uint32_t)0x00000040)          /*!< DCO external resistor open circuit fault flag. */
#define CS_IFG_FCNTLFIFG_OFS                     ( 8)                            /*!< FCNTLFIFG Bit Offset */
#define CS_IFG_FCNTLFIFG                         ((uint32_t)0x00000100)          /*!< Start fault counter interrupt flag LFXT */
#define CS_IFG_FCNTHFIFG_OFS                     ( 9)                            /*!< FCNTHFIFG Bit Offset */
#define CS_IFG_FCNTHFIFG                         ((uint32_t)0x00000200)          /*!< Start fault counter interrupt flag HFXT */
#define CS_CLRIFG_CLR_LFXTIFG_OFS                ( 0)                            /*!< CLR_LFXTIFG Bit Offset */
#define CS_CLRIFG_CLR_LFXTIFG                    ((uint32_t)0x00000001)          /*!< Clear LFXT oscillator fault interrupt flag */
#define CS_CLRIFG_CLR_HFXTIFG_OFS                ( 1)                            /*!< CLR_HFXTIFG Bit Offset */
#define CS_CLRIFG_CLR_HFXTIFG                    ((uint32_t)0x00000002)          /*!< Clear HFXT oscillator fault interrupt flag */
#define CS_CLRIFG_CLR_DCOR_OPNIFG_OFS            ( 6)                            /*!< CLR_DCOR_OPNIFG Bit Offset */
#define CS_CLRIFG_CLR_DCOR_OPNIFG                ((uint32_t)0x00000040)          /*!< Clear DCO external resistor open circuit fault interrupt flag. */
#define CS_CLRIFG_CLR_FCNTLFIFG_OFS              ( 8)                            /*!< CLR_FCNTLFIFG Bit Offset */
#define CS_CLRIFG_CLR_FCNTLFIFG                  ((uint32_t)0x00000100)          /*!< Start fault counter clear interrupt flag LFXT */
#define CS_CLRIFG_CLR_FCNTHFIFG_OFS              ( 9)                            /*!< CLR_FCNTHFIFG Bit Offset */
#define CS_CLRIFG_CLR_FCNTHFIFG                  ((uint32_t)0x00000200)          /*!< Start fault counter clear interrupt flag HFXT */
#define CS_SETIFG_SET_LFXTIFG_OFS                ( 0)                            /*!< SET_LFXTIFG Bit Offset */
#define CS_SETIFG_SET_LFXTIFG                    ((uint32_t)0x00000001)          /*!< Set LFXT oscillator fault interrupt flag */
#define CS_SETIFG_SET_HFXTIFG_OFS                ( 1)                            /*!< SET_HFXTIFG Bit Offset */
#define CS_SETIFG_SET_HFXTIFG                    ((uint32_t)0x00000002)          /*!< Set HFXT oscillator fault interrupt flag */
#define CS_SETIFG_SET_DCOR_OPNIFG_OFS            ( 6)                            /*!< SET_DCOR_OPNIFG Bit Offset */
#define CS_SETIFG_SET_DCOR_OPNIFG                ((uint32_t)0x00000040)          /*!< Set DCO external resistor open circuit fault interrupt flag. */
#define CS_SETIFG_SET_FCNTHFIFG_OFS              ( 9)                            /*!< SET_FCNTHFIFG Bit Offset */
#define CS_SETIFG_SET_FCNTHFIFG                  ((uint32_t)0x00000200)          /*!< Start fault counter set interrupt flag HFXT */
#define CS_SETIFG_SET_FCNTLFIFG_OFS              ( 8)                            /*!< SET_FCNTLFIFG Bit Offset */
#define CS_SETIFG_SET_FCNTLFIFG                  ((uint32_t)0x00000100)          /*!< Start fault counter set interrupt flag LFXT */
#define CS_DCOERCAL0_DCO_TCCAL_OFS               ( 0)                            /*!< DCO_TCCAL Bit Offset */
#define CS_DCOERCAL0_DCO_TCCAL_MASK              ((uint32_t)0x00000003)          /*!< DCO_TCCAL Bit Mask */
#define CS_DCOERCAL0_DCO_FCAL_RSEL04_OFS         (16)                            /*!< DCO_FCAL_RSEL04 Bit Offset */
#define CS_DCOERCAL0_DCO_FCAL_RSEL04_MASK        ((uint32_t)0x03FF0000)          /*!< DCO_FCAL_RSEL04 Bit Mask */
#define CS_DCOERCAL1_DCO_FCAL_RSEL5_OFS          ( 0)                            /*!< DCO_FCAL_RSEL5 Bit Offset */
#define CS_DCOERCAL1_DCO_FCAL_RSEL5_MASK         ((uint32_t)0x000003FF)          /*!< DCO_FCAL_RSEL5 Bit Mask */
#define CS_KEY_VAL                               ((uint32_t)0x0000695A)          /*!< CS control key value */
#define DIO_PORT_IV_OFS                          ( 0)                            /*!< DIO Port IV Bit Offset */
#define DIO_PORT_IV_MASK                         ((uint16_t)0x001F)              /*!< DIO Port IV Bit Mask */
#define DIO_PORT_IV0                             ((uint16_t)0x0001)              /*!< DIO Port IV Bit 0 */
#define DIO_PORT_IV1                             ((uint16_t)0x0002)              /*!< DIO Port IV Bit 1 */
#define DIO_PORT_IV2                             ((uint16_t)0x0004)              /*!< DIO Port IV Bit 2 */
#define DIO_PORT_IV3                             ((uint16_t)0x0008)              /*!< DIO Port IV Bit 3 */
#define DIO_PORT_IV4                             ((uint16_t)0x0010)              /*!< DIO Port IV Bit 4 */
#define DIO_PORT_IV_0                            ((uint16_t)0x0000)              /*!< No interrupt pending */
#define DIO_PORT_IV_2                            ((uint16_t)0x0002)              /*!< Interrupt Source: Port x.0 interrupt; Interrupt Flag: IFG0; Interrupt  */
#define DIO_PORT_IV_4                            ((uint16_t)0x0004)              /*!< Interrupt Source: Port x.1 interrupt; Interrupt Flag: IFG1 */
#define DIO_PORT_IV_6                            ((uint16_t)0x0006)              /*!< Interrupt Source: Port x.2 interrupt; Interrupt Flag: IFG2 */
#define DIO_PORT_IV_8                            ((uint16_t)0x0008)              /*!< Interrupt Source: Port x.3 interrupt; Interrupt Flag: IFG3 */
#define DIO_PORT_IV_10                           ((uint16_t)0x000A)              /*!< Interrupt Source: Port x.4 interrupt; Interrupt Flag: IFG4 */
#define DIO_PORT_IV_12                           ((uint16_t)0x000C)              /*!< Interrupt Source: Port x.5 interrupt; Interrupt Flag: IFG5 */
#define DIO_PORT_IV_14                           ((uint16_t)0x000E)              /*!< Interrupt Source: Port x.6 interrupt; Interrupt Flag: IFG6 */
#define DIO_PORT_IV_16                           ((uint16_t)0x0010)              /*!< Interrupt Source: Port x.7 interrupt; Interrupt Flag: IFG7; Interrupt  */
#define DIO_PORT_IV__NONE                        ((uint16_t)0x0000)              /*!< No interrupt pending */
#define DIO_PORT_IV__IFG0                        ((uint16_t)0x0002)              /*!< Interrupt Source: Port x.0 interrupt; Interrupt Flag: IFG0; Interrupt  */
#define DIO_PORT_IV__IFG1                        ((uint16_t)0x0004)              /*!< Interrupt Source: Port x.1 interrupt; Interrupt Flag: IFG1 */
#define DIO_PORT_IV__IFG2                        ((uint16_t)0x0006)              /*!< Interrupt Source: Port x.2 interrupt; Interrupt Flag: IFG2 */
#define DIO_PORT_IV__IFG3                        ((uint16_t)0x0008)              /*!< Interrupt Source: Port x.3 interrupt; Interrupt Flag: IFG3 */
#define DIO_PORT_IV__IFG4                        ((uint16_t)0x000A)              /*!< Interrupt Source: Port x.4 interrupt; Interrupt Flag: IFG4 */
#define DIO_PORT_IV__IFG5                        ((uint16_t)0x000C)              /*!< Interrupt Source: Port x.5 interrupt; Interrupt Flag: IFG5 */
#define DIO_PORT_IV__IFG6                        ((uint16_t)0x000E)              /*!< Interrupt Source: Port x.6 interrupt; Interrupt Flag: IFG6 */
#define DIO_PORT_IV__IFG7                        ((uint16_t)0x0010)              /*!< Interrupt Source: Port x.7 interrupt; Interrupt Flag: IFG7; Interrupt  */
#define DMA_DEVICE_CFG_NUM_DMA_CHANNELS_OFS      ( 0)                            /*!< NUM_DMA_CHANNELS Bit Offset */
#define DMA_DEVICE_CFG_NUM_DMA_CHANNELS_MASK     ((uint32_t)0x000000FF)          /*!< NUM_DMA_CHANNELS Bit Mask */
#define DMA_DEVICE_CFG_NUM_SRC_PER_CHANNEL_OFS   ( 8)                            /*!< NUM_SRC_PER_CHANNEL Bit Offset */
#define DMA_DEVICE_CFG_NUM_SRC_PER_CHANNEL_MASK  ((uint32_t)0x0000FF00)          /*!< NUM_SRC_PER_CHANNEL Bit Mask */
#define DMA_SW_CHTRIG_CH0_OFS                    ( 0)                            /*!< CH0 Bit Offset */
#define DMA_SW_CHTRIG_CH0                        ((uint32_t)0x00000001)          /*!< Write 1, triggers DMA_CHANNEL0 */
#define DMA_SW_CHTRIG_CH1_OFS                    ( 1)                            /*!< CH1 Bit Offset */
#define DMA_SW_CHTRIG_CH1                        ((uint32_t)0x00000002)          /*!< Write 1, triggers DMA_CHANNEL1 */
#define DMA_SW_CHTRIG_CH2_OFS                    ( 2)                            /*!< CH2 Bit Offset */
#define DMA_SW_CHTRIG_CH2                        ((uint32_t)0x00000004)          /*!< Write 1, triggers DMA_CHANNEL2 */
#define DMA_SW_CHTRIG_CH3_OFS                    ( 3)                            /*!< CH3 Bit Offset */
#define DMA_SW_CHTRIG_CH3                        ((uint32_t)0x00000008)          /*!< Write 1, triggers DMA_CHANNEL3 */
#define DMA_SW_CHTRIG_CH4_OFS                    ( 4)                            /*!< CH4 Bit Offset */
#define DMA_SW_CHTRIG_CH4                        ((uint32_t)0x00000010)          /*!< Write 1, triggers DMA_CHANNEL4 */
#define DMA_SW_CHTRIG_CH5_OFS                    ( 5)                            /*!< CH5 Bit Offset */
#define DMA_SW_CHTRIG_CH5                        ((uint32_t)0x00000020)          /*!< Write 1, triggers DMA_CHANNEL5 */
#define DMA_SW_CHTRIG_CH6_OFS                    ( 6)                            /*!< CH6 Bit Offset */
#define DMA_SW_CHTRIG_CH6                        ((uint32_t)0x00000040)          /*!< Write 1, triggers DMA_CHANNEL6 */
#define DMA_SW_CHTRIG_CH7_OFS                    ( 7)                            /*!< CH7 Bit Offset */
#define DMA_SW_CHTRIG_CH7                        ((uint32_t)0x00000080)          /*!< Write 1, triggers DMA_CHANNEL7 */
#define DMA_SW_CHTRIG_CH8_OFS                    ( 8)                            /*!< CH8 Bit Offset */
#define DMA_SW_CHTRIG_CH8                        ((uint32_t)0x00000100)          /*!< Write 1, triggers DMA_CHANNEL8 */
#define DMA_SW_CHTRIG_CH9_OFS                    ( 9)                            /*!< CH9 Bit Offset */
#define DMA_SW_CHTRIG_CH9                        ((uint32_t)0x00000200)          /*!< Write 1, triggers DMA_CHANNEL9 */
#define DMA_SW_CHTRIG_CH10_OFS                   (10)                            /*!< CH10 Bit Offset */
#define DMA_SW_CHTRIG_CH10                       ((uint32_t)0x00000400)          /*!< Write 1, triggers DMA_CHANNEL10 */
#define DMA_SW_CHTRIG_CH11_OFS                   (11)                            /*!< CH11 Bit Offset */
#define DMA_SW_CHTRIG_CH11                       ((uint32_t)0x00000800)          /*!< Write 1, triggers DMA_CHANNEL11 */
#define DMA_SW_CHTRIG_CH12_OFS                   (12)                            /*!< CH12 Bit Offset */
#define DMA_SW_CHTRIG_CH12                       ((uint32_t)0x00001000)          /*!< Write 1, triggers DMA_CHANNEL12 */
#define DMA_SW_CHTRIG_CH13_OFS                   (13)                            /*!< CH13 Bit Offset */
#define DMA_SW_CHTRIG_CH13                       ((uint32_t)0x00002000)          /*!< Write 1, triggers DMA_CHANNEL13 */
#define DMA_SW_CHTRIG_CH14_OFS                   (14)                            /*!< CH14 Bit Offset */
#define DMA_SW_CHTRIG_CH14                       ((uint32_t)0x00004000)          /*!< Write 1, triggers DMA_CHANNEL14 */
#define DMA_SW_CHTRIG_CH15_OFS                   (15)                            /*!< CH15 Bit Offset */
#define DMA_SW_CHTRIG_CH15                       ((uint32_t)0x00008000)          /*!< Write 1, triggers DMA_CHANNEL15 */
#define DMA_SW_CHTRIG_CH16_OFS                   (16)                            /*!< CH16 Bit Offset */
#define DMA_SW_CHTRIG_CH16                       ((uint32_t)0x00010000)          /*!< Write 1, triggers DMA_CHANNEL16 */
#define DMA_SW_CHTRIG_CH17_OFS                   (17)                            /*!< CH17 Bit Offset */
#define DMA_SW_CHTRIG_CH17                       ((uint32_t)0x00020000)          /*!< Write 1, triggers DMA_CHANNEL17 */
#define DMA_SW_CHTRIG_CH18_OFS                   (18)                            /*!< CH18 Bit Offset */
#define DMA_SW_CHTRIG_CH18                       ((uint32_t)0x00040000)          /*!< Write 1, triggers DMA_CHANNEL18 */
#define DMA_SW_CHTRIG_CH19_OFS                   (19)                            /*!< CH19 Bit Offset */
#define DMA_SW_CHTRIG_CH19                       ((uint32_t)0x00080000)          /*!< Write 1, triggers DMA_CHANNEL19 */
#define DMA_SW_CHTRIG_CH20_OFS                   (20)                            /*!< CH20 Bit Offset */
#define DMA_SW_CHTRIG_CH20                       ((uint32_t)0x00100000)          /*!< Write 1, triggers DMA_CHANNEL20 */
#define DMA_SW_CHTRIG_CH21_OFS                   (21)                            /*!< CH21 Bit Offset */
#define DMA_SW_CHTRIG_CH21                       ((uint32_t)0x00200000)          /*!< Write 1, triggers DMA_CHANNEL21 */
#define DMA_SW_CHTRIG_CH22_OFS                   (22)                            /*!< CH22 Bit Offset */
#define DMA_SW_CHTRIG_CH22                       ((uint32_t)0x00400000)          /*!< Write 1, triggers DMA_CHANNEL22 */
#define DMA_SW_CHTRIG_CH23_OFS                   (23)                            /*!< CH23 Bit Offset */
#define DMA_SW_CHTRIG_CH23                       ((uint32_t)0x00800000)          /*!< Write 1, triggers DMA_CHANNEL23 */
#define DMA_SW_CHTRIG_CH24_OFS                   (24)                            /*!< CH24 Bit Offset */
#define DMA_SW_CHTRIG_CH24                       ((uint32_t)0x01000000)          /*!< Write 1, triggers DMA_CHANNEL24 */
#define DMA_SW_CHTRIG_CH25_OFS                   (25)                            /*!< CH25 Bit Offset */
#define DMA_SW_CHTRIG_CH25                       ((uint32_t)0x02000000)          /*!< Write 1, triggers DMA_CHANNEL25 */
#define DMA_SW_CHTRIG_CH26_OFS                   (26)                            /*!< CH26 Bit Offset */
#define DMA_SW_CHTRIG_CH26                       ((uint32_t)0x04000000)          /*!< Write 1, triggers DMA_CHANNEL26 */
#define DMA_SW_CHTRIG_CH27_OFS                   (27)                            /*!< CH27 Bit Offset */
#define DMA_SW_CHTRIG_CH27                       ((uint32_t)0x08000000)          /*!< Write 1, triggers DMA_CHANNEL27 */
#define DMA_SW_CHTRIG_CH28_OFS                   (28)                            /*!< CH28 Bit Offset */
#define DMA_SW_CHTRIG_CH28                       ((uint32_t)0x10000000)          /*!< Write 1, triggers DMA_CHANNEL28 */
#define DMA_SW_CHTRIG_CH29_OFS                   (29)                            /*!< CH29 Bit Offset */
#define DMA_SW_CHTRIG_CH29                       ((uint32_t)0x20000000)          /*!< Write 1, triggers DMA_CHANNEL29 */
#define DMA_SW_CHTRIG_CH30_OFS                   (30)                            /*!< CH30 Bit Offset */
#define DMA_SW_CHTRIG_CH30                       ((uint32_t)0x40000000)          /*!< Write 1, triggers DMA_CHANNEL30 */
#define DMA_SW_CHTRIG_CH31_OFS                   (31)                            /*!< CH31 Bit Offset */
#define DMA_SW_CHTRIG_CH31                       ((uint32_t)0x80000000)          /*!< Write 1, triggers DMA_CHANNEL31 */
#define DMA_CHN_SRCCFG_DMA_SRC_OFS               ( 0)                            /*!< DMA_SRC Bit Offset */
#define DMA_CHN_SRCCFG_DMA_SRC_MASK              ((uint32_t)0x000000FF)          /*!< DMA_SRC Bit Mask */
#define DMA_INT1_SRCCFG_INT_SRC_OFS              ( 0)                            /*!< INT_SRC Bit Offset */
#define DMA_INT1_SRCCFG_INT_SRC_MASK             ((uint32_t)0x0000001F)          /*!< INT_SRC Bit Mask */
#define DMA_INT1_SRCCFG_EN_OFS                   ( 5)                            /*!< EN Bit Offset */
#define DMA_INT1_SRCCFG_EN                       ((uint32_t)0x00000020)          /*!< Enables DMA_INT1 mapping */
#define DMA_INT2_SRCCFG_INT_SRC_OFS              ( 0)                            /*!< INT_SRC Bit Offset */
#define DMA_INT2_SRCCFG_INT_SRC_MASK             ((uint32_t)0x0000001F)          /*!< INT_SRC Bit Mask */
#define DMA_INT2_SRCCFG_EN_OFS                   ( 5)                            /*!< EN Bit Offset */
#define DMA_INT2_SRCCFG_EN                       ((uint32_t)0x00000020)          /*!< Enables DMA_INT2 mapping */
#define DMA_INT3_SRCCFG_INT_SRC_OFS              ( 0)                            /*!< INT_SRC Bit Offset */
#define DMA_INT3_SRCCFG_INT_SRC_MASK             ((uint32_t)0x0000001F)          /*!< INT_SRC Bit Mask */
#define DMA_INT3_SRCCFG_EN_OFS                   ( 5)                            /*!< EN Bit Offset */
#define DMA_INT3_SRCCFG_EN                       ((uint32_t)0x00000020)          /*!< Enables DMA_INT3 mapping */
#define DMA_INT0_SRCFLG_CH0_OFS                  ( 0)                            /*!< CH0 Bit Offset */
#define DMA_INT0_SRCFLG_CH0                      ((uint32_t)0x00000001)          /*!< Channel 0 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH1_OFS                  ( 1)                            /*!< CH1 Bit Offset */
#define DMA_INT0_SRCFLG_CH1                      ((uint32_t)0x00000002)          /*!< Channel 1 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH2_OFS                  ( 2)                            /*!< CH2 Bit Offset */
#define DMA_INT0_SRCFLG_CH2                      ((uint32_t)0x00000004)          /*!< Channel 2 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH3_OFS                  ( 3)                            /*!< CH3 Bit Offset */
#define DMA_INT0_SRCFLG_CH3                      ((uint32_t)0x00000008)          /*!< Channel 3 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH4_OFS                  ( 4)                            /*!< CH4 Bit Offset */
#define DMA_INT0_SRCFLG_CH4                      ((uint32_t)0x00000010)          /*!< Channel 4 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH5_OFS                  ( 5)                            /*!< CH5 Bit Offset */
#define DMA_INT0_SRCFLG_CH5                      ((uint32_t)0x00000020)          /*!< Channel 5 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH6_OFS                  ( 6)                            /*!< CH6 Bit Offset */
#define DMA_INT0_SRCFLG_CH6                      ((uint32_t)0x00000040)          /*!< Channel 6 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH7_OFS                  ( 7)                            /*!< CH7 Bit Offset */
#define DMA_INT0_SRCFLG_CH7                      ((uint32_t)0x00000080)          /*!< Channel 7 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH8_OFS                  ( 8)                            /*!< CH8 Bit Offset */
#define DMA_INT0_SRCFLG_CH8                      ((uint32_t)0x00000100)          /*!< Channel 8 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH9_OFS                  ( 9)                            /*!< CH9 Bit Offset */
#define DMA_INT0_SRCFLG_CH9                      ((uint32_t)0x00000200)          /*!< Channel 9 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH10_OFS                 (10)                            /*!< CH10 Bit Offset */
#define DMA_INT0_SRCFLG_CH10                     ((uint32_t)0x00000400)          /*!< Channel 10 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH11_OFS                 (11)                            /*!< CH11 Bit Offset */
#define DMA_INT0_SRCFLG_CH11                     ((uint32_t)0x00000800)          /*!< Channel 11 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH12_OFS                 (12)                            /*!< CH12 Bit Offset */
#define DMA_INT0_SRCFLG_CH12                     ((uint32_t)0x00001000)          /*!< Channel 12 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH13_OFS                 (13)                            /*!< CH13 Bit Offset */
#define DMA_INT0_SRCFLG_CH13                     ((uint32_t)0x00002000)          /*!< Channel 13 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH14_OFS                 (14)                            /*!< CH14 Bit Offset */
#define DMA_INT0_SRCFLG_CH14                     ((uint32_t)0x00004000)          /*!< Channel 14 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH15_OFS                 (15)                            /*!< CH15 Bit Offset */
#define DMA_INT0_SRCFLG_CH15                     ((uint32_t)0x00008000)          /*!< Channel 15 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH16_OFS                 (16)                            /*!< CH16 Bit Offset */
#define DMA_INT0_SRCFLG_CH16                     ((uint32_t)0x00010000)          /*!< Channel 16 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH17_OFS                 (17)                            /*!< CH17 Bit Offset */
#define DMA_INT0_SRCFLG_CH17                     ((uint32_t)0x00020000)          /*!< Channel 17 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH18_OFS                 (18)                            /*!< CH18 Bit Offset */
#define DMA_INT0_SRCFLG_CH18                     ((uint32_t)0x00040000)          /*!< Channel 18 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH19_OFS                 (19)                            /*!< CH19 Bit Offset */
#define DMA_INT0_SRCFLG_CH19                     ((uint32_t)0x00080000)          /*!< Channel 19 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH20_OFS                 (20)                            /*!< CH20 Bit Offset */
#define DMA_INT0_SRCFLG_CH20                     ((uint32_t)0x00100000)          /*!< Channel 20 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH21_OFS                 (21)                            /*!< CH21 Bit Offset */
#define DMA_INT0_SRCFLG_CH21                     ((uint32_t)0x00200000)          /*!< Channel 21 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH22_OFS                 (22)                            /*!< CH22 Bit Offset */
#define DMA_INT0_SRCFLG_CH22                     ((uint32_t)0x00400000)          /*!< Channel 22 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH23_OFS                 (23)                            /*!< CH23 Bit Offset */
#define DMA_INT0_SRCFLG_CH23                     ((uint32_t)0x00800000)          /*!< Channel 23 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH24_OFS                 (24)                            /*!< CH24 Bit Offset */
#define DMA_INT0_SRCFLG_CH24                     ((uint32_t)0x01000000)          /*!< Channel 24 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH25_OFS                 (25)                            /*!< CH25 Bit Offset */
#define DMA_INT0_SRCFLG_CH25                     ((uint32_t)0x02000000)          /*!< Channel 25 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH26_OFS                 (26)                            /*!< CH26 Bit Offset */
#define DMA_INT0_SRCFLG_CH26                     ((uint32_t)0x04000000)          /*!< Channel 26 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH27_OFS                 (27)                            /*!< CH27 Bit Offset */
#define DMA_INT0_SRCFLG_CH27                     ((uint32_t)0x08000000)          /*!< Channel 27 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH28_OFS                 (28)                            /*!< CH28 Bit Offset */
#define DMA_INT0_SRCFLG_CH28                     ((uint32_t)0x10000000)          /*!< Channel 28 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH29_OFS                 (29)                            /*!< CH29 Bit Offset */
#define DMA_INT0_SRCFLG_CH29                     ((uint32_t)0x20000000)          /*!< Channel 29 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH30_OFS                 (30)                            /*!< CH30 Bit Offset */
#define DMA_INT0_SRCFLG_CH30                     ((uint32_t)0x40000000)          /*!< Channel 30 was the source of DMA_INT0 */
#define DMA_INT0_SRCFLG_CH31_OFS                 (31)                            /*!< CH31 Bit Offset */
#define DMA_INT0_SRCFLG_CH31                     ((uint32_t)0x80000000)          /*!< Channel 31 was the source of DMA_INT0 */
#define DMA_INT0_CLRFLG_CH0_OFS                  ( 0)                            /*!< CH0 Bit Offset */
#define DMA_INT0_CLRFLG_CH0                      ((uint32_t)0x00000001)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH1_OFS                  ( 1)                            /*!< CH1 Bit Offset */
#define DMA_INT0_CLRFLG_CH1                      ((uint32_t)0x00000002)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH2_OFS                  ( 2)                            /*!< CH2 Bit Offset */
#define DMA_INT0_CLRFLG_CH2                      ((uint32_t)0x00000004)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH3_OFS                  ( 3)                            /*!< CH3 Bit Offset */
#define DMA_INT0_CLRFLG_CH3                      ((uint32_t)0x00000008)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH4_OFS                  ( 4)                            /*!< CH4 Bit Offset */
#define DMA_INT0_CLRFLG_CH4                      ((uint32_t)0x00000010)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH5_OFS                  ( 5)                            /*!< CH5 Bit Offset */
#define DMA_INT0_CLRFLG_CH5                      ((uint32_t)0x00000020)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH6_OFS                  ( 6)                            /*!< CH6 Bit Offset */
#define DMA_INT0_CLRFLG_CH6                      ((uint32_t)0x00000040)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH7_OFS                  ( 7)                            /*!< CH7 Bit Offset */
#define DMA_INT0_CLRFLG_CH7                      ((uint32_t)0x00000080)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH8_OFS                  ( 8)                            /*!< CH8 Bit Offset */
#define DMA_INT0_CLRFLG_CH8                      ((uint32_t)0x00000100)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH9_OFS                  ( 9)                            /*!< CH9 Bit Offset */
#define DMA_INT0_CLRFLG_CH9                      ((uint32_t)0x00000200)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH10_OFS                 (10)                            /*!< CH10 Bit Offset */
#define DMA_INT0_CLRFLG_CH10                     ((uint32_t)0x00000400)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH11_OFS                 (11)                            /*!< CH11 Bit Offset */
#define DMA_INT0_CLRFLG_CH11                     ((uint32_t)0x00000800)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH12_OFS                 (12)                            /*!< CH12 Bit Offset */
#define DMA_INT0_CLRFLG_CH12                     ((uint32_t)0x00001000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH13_OFS                 (13)                            /*!< CH13 Bit Offset */
#define DMA_INT0_CLRFLG_CH13                     ((uint32_t)0x00002000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH14_OFS                 (14)                            /*!< CH14 Bit Offset */
#define DMA_INT0_CLRFLG_CH14                     ((uint32_t)0x00004000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH15_OFS                 (15)                            /*!< CH15 Bit Offset */
#define DMA_INT0_CLRFLG_CH15                     ((uint32_t)0x00008000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH16_OFS                 (16)                            /*!< CH16 Bit Offset */
#define DMA_INT0_CLRFLG_CH16                     ((uint32_t)0x00010000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH17_OFS                 (17)                            /*!< CH17 Bit Offset */
#define DMA_INT0_CLRFLG_CH17                     ((uint32_t)0x00020000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH18_OFS                 (18)                            /*!< CH18 Bit Offset */
#define DMA_INT0_CLRFLG_CH18                     ((uint32_t)0x00040000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH19_OFS                 (19)                            /*!< CH19 Bit Offset */
#define DMA_INT0_CLRFLG_CH19                     ((uint32_t)0x00080000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH20_OFS                 (20)                            /*!< CH20 Bit Offset */
#define DMA_INT0_CLRFLG_CH20                     ((uint32_t)0x00100000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH21_OFS                 (21)                            /*!< CH21 Bit Offset */
#define DMA_INT0_CLRFLG_CH21                     ((uint32_t)0x00200000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH22_OFS                 (22)                            /*!< CH22 Bit Offset */
#define DMA_INT0_CLRFLG_CH22                     ((uint32_t)0x00400000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH23_OFS                 (23)                            /*!< CH23 Bit Offset */
#define DMA_INT0_CLRFLG_CH23                     ((uint32_t)0x00800000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH24_OFS                 (24)                            /*!< CH24 Bit Offset */
#define DMA_INT0_CLRFLG_CH24                     ((uint32_t)0x01000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH25_OFS                 (25)                            /*!< CH25 Bit Offset */
#define DMA_INT0_CLRFLG_CH25                     ((uint32_t)0x02000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH26_OFS                 (26)                            /*!< CH26 Bit Offset */
#define DMA_INT0_CLRFLG_CH26                     ((uint32_t)0x04000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH27_OFS                 (27)                            /*!< CH27 Bit Offset */
#define DMA_INT0_CLRFLG_CH27                     ((uint32_t)0x08000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH28_OFS                 (28)                            /*!< CH28 Bit Offset */
#define DMA_INT0_CLRFLG_CH28                     ((uint32_t)0x10000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH29_OFS                 (29)                            /*!< CH29 Bit Offset */
#define DMA_INT0_CLRFLG_CH29                     ((uint32_t)0x20000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH30_OFS                 (30)                            /*!< CH30 Bit Offset */
#define DMA_INT0_CLRFLG_CH30                     ((uint32_t)0x40000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_INT0_CLRFLG_CH31_OFS                 (31)                            /*!< CH31 Bit Offset */
#define DMA_INT0_CLRFLG_CH31                     ((uint32_t)0x80000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
#define DMA_STAT_MASTEN_OFS                      ( 0)                            /*!< MASTEN Bit Offset */
#define DMA_STAT_MASTEN                          ((uint32_t)0x00000001)          
#define DMA_STAT_STATE_OFS                       ( 4)                            /*!< STATE Bit Offset */
#define DMA_STAT_STATE_MASK                      ((uint32_t)0x000000F0)          /*!< STATE Bit Mask */
#define DMA_STAT_STATE0                          ((uint32_t)0x00000010)          /*!< STATE Bit 0 */
#define DMA_STAT_STATE1                          ((uint32_t)0x00000020)          /*!< STATE Bit 1 */
#define DMA_STAT_STATE2                          ((uint32_t)0x00000040)          /*!< STATE Bit 2 */
#define DMA_STAT_STATE3                          ((uint32_t)0x00000080)          /*!< STATE Bit 3 */
#define DMA_STAT_STATE_0                         ((uint32_t)0x00000000)          /*!< idle */
#define DMA_STAT_STATE_1                         ((uint32_t)0x00000010)          /*!< reading channel controller data */
#define DMA_STAT_STATE_2                         ((uint32_t)0x00000020)          /*!< reading source data end pointer */
#define DMA_STAT_STATE_3                         ((uint32_t)0x00000030)          /*!< reading destination data end pointer */
#define DMA_STAT_STATE_4                         ((uint32_t)0x00000040)          /*!< reading source data */
#define DMA_STAT_STATE_5                         ((uint32_t)0x00000050)          /*!< writing destination data */
#define DMA_STAT_STATE_6                         ((uint32_t)0x00000060)          /*!< waiting for DMA request to clear */
#define DMA_STAT_STATE_7                         ((uint32_t)0x00000070)          /*!< writing channel controller data */
#define DMA_STAT_STATE_8                         ((uint32_t)0x00000080)          /*!< stalled */
#define DMA_STAT_STATE_9                         ((uint32_t)0x00000090)          /*!< done */
#define DMA_STAT_STATE_10                        ((uint32_t)0x000000A0)          /*!< peripheral scatter-gather transition */
#define DMA_STAT_STATE_11                        ((uint32_t)0x000000B0)          /*!< Reserved */
#define DMA_STAT_STATE_12                        ((uint32_t)0x000000C0)          /*!< Reserved */
#define DMA_STAT_STATE_13                        ((uint32_t)0x000000D0)          /*!< Reserved */
#define DMA_STAT_STATE_14                        ((uint32_t)0x000000E0)          /*!< Reserved */
#define DMA_STAT_STATE_15                        ((uint32_t)0x000000F0)          /*!< Reserved */
#define DMA_STAT_DMACHANS_OFS                    (16)                            /*!< DMACHANS Bit Offset */
#define DMA_STAT_DMACHANS_MASK                   ((uint32_t)0x001F0000)          /*!< DMACHANS Bit Mask */
#define DMA_STAT_DMACHANS0                       ((uint32_t)0x00010000)          /*!< DMACHANS Bit 0 */
#define DMA_STAT_DMACHANS1                       ((uint32_t)0x00020000)          /*!< DMACHANS Bit 1 */
#define DMA_STAT_DMACHANS2                       ((uint32_t)0x00040000)          /*!< DMACHANS Bit 2 */
#define DMA_STAT_DMACHANS3                       ((uint32_t)0x00080000)          /*!< DMACHANS Bit 3 */
#define DMA_STAT_DMACHANS4                       ((uint32_t)0x00100000)          /*!< DMACHANS Bit 4 */
#define DMA_STAT_DMACHANS_0                      ((uint32_t)0x00000000)          /*!< Controller configured to use 1 DMA channel */
#define DMA_STAT_DMACHANS_1                      ((uint32_t)0x00010000)          /*!< Controller configured to use 2 DMA channels */
#define DMA_STAT_DMACHANS_30                     ((uint32_t)0x001E0000)          /*!< Controller configured to use 31 DMA channels */
#define DMA_STAT_DMACHANS_31                     ((uint32_t)0x001F0000)          /*!< Controller configured to use 32 DMA channels */
#define DMA_STAT_TESTSTAT_OFS                    (28)                            /*!< TESTSTAT Bit Offset */
#define DMA_STAT_TESTSTAT_MASK                   ((uint32_t)0xF0000000)          /*!< TESTSTAT Bit Mask */
#define DMA_STAT_TESTSTAT0                       ((uint32_t)0x10000000)          /*!< TESTSTAT Bit 0 */
#define DMA_STAT_TESTSTAT1                       ((uint32_t)0x20000000)          /*!< TESTSTAT Bit 1 */
#define DMA_STAT_TESTSTAT2                       ((uint32_t)0x40000000)          /*!< TESTSTAT Bit 2 */
#define DMA_STAT_TESTSTAT3                       ((uint32_t)0x80000000)          /*!< TESTSTAT Bit 3 */
#define DMA_STAT_TESTSTAT_0                      ((uint32_t)0x00000000)          /*!< Controller does not include the integration test logic */
#define DMA_STAT_TESTSTAT_1                      ((uint32_t)0x10000000)          /*!< Controller includes the integration test logic */
#define DMA_CFG_MASTEN_OFS                       ( 0)                            /*!< MASTEN Bit Offset */
#define DMA_CFG_MASTEN                           ((uint32_t)0x00000001)          
#define DMA_CFG_CHPROTCTRL_OFS                   ( 5)                            /*!< CHPROTCTRL Bit Offset */
#define DMA_CFG_CHPROTCTRL_MASK                  ((uint32_t)0x000000E0)          /*!< CHPROTCTRL Bit Mask */
#define DMA_CTLBASE_ADDR_OFS                     ( 5)                            /*!< ADDR Bit Offset */
#define DMA_CTLBASE_ADDR_MASK                    ((uint32_t)0xFFFFFFE0)          /*!< ADDR Bit Mask */
#define DMA_ERRCLR_ERRCLR_OFS                    ( 0)                            /*!< ERRCLR Bit Offset */
#define DMA_ERRCLR_ERRCLR                        ((uint32_t)0x00000001)          
#define __MCU_NUM_DMA_CHANNELS__                8
#define DMA_CHANNEL_CONTROL_STRUCT_SIZE         0x10
#define DMA_CONTROL_MEMORY_ALIGNMENT            (__MCU_NUM_DMA_CHANNELS__ * DMA_CHANNEL_CONTROL_STRUCT_SIZE)
#define UDMA_STAT_DMACHANS_M                    ((uint32_t)0x001F0000)           /*!< Available uDMA Channels Minus 1 */
#define UDMA_STAT_STATE_M                       ((uint32_t)0x000000F0)           /*!< Control State Machine Status */
#define UDMA_STAT_STATE_IDLE                    ((uint32_t)0x00000000)           /*!< Idle */
#define UDMA_STAT_STATE_RD_CTRL                 ((uint32_t)0x00000010)           /*!< Reading channel controller data */
#define UDMA_STAT_STATE_RD_SRCENDP              ((uint32_t)0x00000020)           /*!< Reading source end pointer */
#define UDMA_STAT_STATE_RD_DSTENDP              ((uint32_t)0x00000030)           /*!< Reading destination end pointer */
#define UDMA_STAT_STATE_RD_SRCDAT               ((uint32_t)0x00000040)           /*!< Reading source data */
#define UDMA_STAT_STATE_WR_DSTDAT               ((uint32_t)0x00000050)           /*!< Writing destination data */
#define UDMA_STAT_STATE_WAIT                    ((uint32_t)0x00000060)           /*!< Waiting for uDMA request to clear */
#define UDMA_STAT_STATE_WR_CTRL                 ((uint32_t)0x00000070)           /*!< Writing channel controller data */
#define UDMA_STAT_STATE_STALL                   ((uint32_t)0x00000080)           /*!< Stalled */
#define UDMA_STAT_STATE_DONE                    ((uint32_t)0x00000090)           /*!< Done */
#define UDMA_STAT_STATE_UNDEF                   ((uint32_t)0x000000A0)           /*!< Undefined */
#define UDMA_STAT_MASTEN                        ((uint32_t)0x00000001)           /*!< Master Enable Status */
#define UDMA_STAT_DMACHANS_S                    (16)
#define UDMA_CFG_MASTEN                         ((uint32_t)0x00000001)           /*!< Controller Master Enable */
#define UDMA_CTLBASE_ADDR_M                     ((uint32_t)0xFFFFFC00)           /*!< Channel Control Base Address */
#define UDMA_CTLBASE_ADDR_S                     (10)
#define UDMA_ALTBASE_ADDR_M                     ((uint32_t)0xFFFFFFFF)           /*!< Alternate Channel Address Pointer */
#define UDMA_ALTBASE_ADDR_S                     ( 0)
#define UDMA_WAITSTAT_WAITREQ_M                 ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Wait Status */
#define UDMA_SWREQ_M                            ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Software Request */
#define UDMA_USEBURSTSET_SET_M                  ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Useburst Set */
#define UDMA_USEBURSTCLR_CLR_M                  ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Useburst Clear */
#define UDMA_REQMASKSET_SET_M                   ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Request Mask Set */
#define UDMA_REQMASKCLR_CLR_M                   ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Request Mask Clear */
#define UDMA_ENASET_SET_M                       ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Enable Set */
#define UDMA_ENACLR_CLR_M                       ((uint32_t)0xFFFFFFFF)           /*!< Clear Channel [n] Enable Clear */
#define UDMA_ALTSET_SET_M                       ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Alternate Set */
#define UDMA_ALTCLR_CLR_M                       ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Alternate Clear */
#define UDMA_PRIOSET_SET_M                      ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Priority Set */
#define UDMA_PRIOCLR_CLR_M                      ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Priority Clear */
#define UDMA_ERRCLR_ERRCLR                      ((uint32_t)0x00000001)           /*!< uDMA Bus Error Status */
#define UDMA_CHASGN_M                           ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Assignment Select */
#define UDMA_CHASGN_PRIMARY                     ((uint32_t)0x00000000)           /*!< Use the primary channel assignment */
#define UDMA_CHASGN_SECONDARY                   ((uint32_t)0x00000001)           /*!< Use the secondary channel assignment */
#define UDMA_O_SRCENDP                          ((uint32_t)0x00000000)           /*!< DMA Channel Source Address End Pointer */
#define UDMA_O_DSTENDP                          ((uint32_t)0x00000004)           /*!< DMA Channel Destination Address End Pointer */
#define UDMA_O_CHCTL                            ((uint32_t)0x00000008)           /*!< DMA Channel Control Word */
#define UDMA_SRCENDP_ADDR_M                     ((uint32_t)0xFFFFFFFF)           /*!< Source Address End Pointer */
#define UDMA_SRCENDP_ADDR_S                     ( 0)
#define UDMA_DSTENDP_ADDR_M                     ((uint32_t)0xFFFFFFFF)           /*!< Destination Address End Pointer */
#define UDMA_DSTENDP_ADDR_S                     ( 0)
#define UDMA_CHCTL_DSTINC_M                     ((uint32_t)0xC0000000)           /*!< Destination Address Increment */
#define UDMA_CHCTL_DSTINC_8                     ((uint32_t)0x00000000)           /*!< Byte */
#define UDMA_CHCTL_DSTINC_16                    ((uint32_t)0x40000000)           /*!< Half-word */
#define UDMA_CHCTL_DSTINC_32                    ((uint32_t)0x80000000)           /*!< Word */
#define UDMA_CHCTL_DSTINC_NONE                  ((uint32_t)0xC0000000)           /*!< No increment */
#define UDMA_CHCTL_DSTSIZE_M                    ((uint32_t)0x30000000)           /*!< Destination Data Size */
#define UDMA_CHCTL_DSTSIZE_8                    ((uint32_t)0x00000000)           /*!< Byte */
#define UDMA_CHCTL_DSTSIZE_16                   ((uint32_t)0x10000000)           /*!< Half-word */
#define UDMA_CHCTL_DSTSIZE_32                   ((uint32_t)0x20000000)           /*!< Word */
#define UDMA_CHCTL_SRCINC_M                     ((uint32_t)0x0C000000)           /*!< Source Address Increment */
#define UDMA_CHCTL_SRCINC_8                     ((uint32_t)0x00000000)           /*!< Byte */
#define UDMA_CHCTL_SRCINC_16                    ((uint32_t)0x04000000)           /*!< Half-word */
#define UDMA_CHCTL_SRCINC_32                    ((uint32_t)0x08000000)           /*!< Word */
#define UDMA_CHCTL_SRCINC_NONE                  ((uint32_t)0x0C000000)           /*!< No increment */
#define UDMA_CHCTL_SRCSIZE_M                    ((uint32_t)0x03000000)           /*!< Source Data Size */
#define UDMA_CHCTL_SRCSIZE_8                    ((uint32_t)0x00000000)           /*!< Byte */
#define UDMA_CHCTL_SRCSIZE_16                   ((uint32_t)0x01000000)           /*!< Half-word */
#define UDMA_CHCTL_SRCSIZE_32                   ((uint32_t)0x02000000)           /*!< Word */
#define UDMA_CHCTL_ARBSIZE_M                    ((uint32_t)0x0003C000)           /*!< Arbitration Size */
#define UDMA_CHCTL_ARBSIZE_1                    ((uint32_t)0x00000000)           /*!< 1 Transfer */
#define UDMA_CHCTL_ARBSIZE_2                    ((uint32_t)0x00004000)           /*!< 2 Transfers */
#define UDMA_CHCTL_ARBSIZE_4                    ((uint32_t)0x00008000)           /*!< 4 Transfers */
#define UDMA_CHCTL_ARBSIZE_8                    ((uint32_t)0x0000C000)           /*!< 8 Transfers */
#define UDMA_CHCTL_ARBSIZE_16                   ((uint32_t)0x00010000)           /*!< 16 Transfers */
#define UDMA_CHCTL_ARBSIZE_32                   ((uint32_t)0x00014000)           /*!< 32 Transfers */
#define UDMA_CHCTL_ARBSIZE_64                   ((uint32_t)0x00018000)           /*!< 64 Transfers */
#define UDMA_CHCTL_ARBSIZE_128                  ((uint32_t)0x0001C000)           /*!< 128 Transfers */
#define UDMA_CHCTL_ARBSIZE_256                  ((uint32_t)0x00020000)           /*!< 256 Transfers */
#define UDMA_CHCTL_ARBSIZE_512                  ((uint32_t)0x00024000)           /*!< 512 Transfers */
#define UDMA_CHCTL_ARBSIZE_1024                 ((uint32_t)0x00028000)           /*!< 1024 Transfers */
#define UDMA_CHCTL_XFERSIZE_M                   ((uint32_t)0x00003FF0)           /*!< Transfer Size (minus 1) */
#define UDMA_CHCTL_NXTUSEBURST                  ((uint32_t)0x00000008)           /*!< Next Useburst */
#define UDMA_CHCTL_XFERMODE_M                   ((uint32_t)0x00000007)           /*!< uDMA Transfer Mode */
#define UDMA_CHCTL_XFERMODE_STOP                ((uint32_t)0x00000000)           /*!< Stop */
#define UDMA_CHCTL_XFERMODE_BASIC               ((uint32_t)0x00000001)           /*!< Basic */
#define UDMA_CHCTL_XFERMODE_AUTO                ((uint32_t)0x00000002)           /*!< Auto-Request */
#define UDMA_CHCTL_XFERMODE_PINGPONG            ((uint32_t)0x00000003)           /*!< Ping-Pong */
#define UDMA_CHCTL_XFERMODE_MEM_SG              ((uint32_t)0x00000004)           /*!< Memory Scatter-Gather */
#define UDMA_CHCTL_XFERMODE_MEM_SGA             ((uint32_t)0x00000005)           /*!< Alternate Memory Scatter-Gather */
#define UDMA_CHCTL_XFERMODE_PER_SG              ((uint32_t)0x00000006)           /*!< Peripheral Scatter-Gather */
#define UDMA_CHCTL_XFERMODE_PER_SGA             ((uint32_t)0x00000007)           /*!< Alternate Peripheral Scatter-Gather */
#define UDMA_CHCTL_XFERSIZE_S                   ( 4)
#define EUSCI_A_CTLW0_SWRST_OFS                  ( 0)                            /*!< UCSWRST Bit Offset */
#define EUSCI_A_CTLW0_SWRST                      ((uint16_t)0x0001)              /*!< Software reset enable */
#define EUSCI_A_CTLW0_TXBRK_OFS                  ( 1)                            /*!< UCTXBRK Bit Offset */
#define EUSCI_A_CTLW0_TXBRK                      ((uint16_t)0x0002)              /*!< Transmit break */
#define EUSCI_A_CTLW0_TXADDR_OFS                 ( 2)                            /*!< UCTXADDR Bit Offset */
#define EUSCI_A_CTLW0_TXADDR                     ((uint16_t)0x0004)              /*!< Transmit address */
#define EUSCI_A_CTLW0_DORM_OFS                   ( 3)                            /*!< UCDORM Bit Offset */
#define EUSCI_A_CTLW0_DORM                       ((uint16_t)0x0008)              /*!< Dormant */
#define EUSCI_A_CTLW0_BRKIE_OFS                  ( 4)                            /*!< UCBRKIE Bit Offset */
#define EUSCI_A_CTLW0_BRKIE                      ((uint16_t)0x0010)              /*!< Receive break character interrupt enable */
#define EUSCI_A_CTLW0_RXEIE_OFS                  ( 5)                            /*!< UCRXEIE Bit Offset */
#define EUSCI_A_CTLW0_RXEIE                      ((uint16_t)0x0020)              /*!< Receive erroneous-character interrupt enable */
#define EUSCI_A_CTLW0_SSEL_OFS                   ( 6)                            /*!< UCSSEL Bit Offset */
#define EUSCI_A_CTLW0_SSEL_MASK                  ((uint16_t)0x00C0)              /*!< UCSSEL Bit Mask */
#define EUSCI_A_CTLW0_SSEL0                      ((uint16_t)0x0040)              /*!< SSEL Bit 0 */
#define EUSCI_A_CTLW0_SSEL1                      ((uint16_t)0x0080)              /*!< SSEL Bit 1 */
#define EUSCI_A_CTLW0_UCSSEL_0                   ((uint16_t)0x0000)              /*!< UCLK */
#define EUSCI_A_CTLW0_UCSSEL_1                   ((uint16_t)0x0040)              /*!< ACLK */
#define EUSCI_A_CTLW0_UCSSEL_2                   ((uint16_t)0x0080)              /*!< SMCLK */
#define EUSCI_A_CTLW0_SSEL__UCLK                 ((uint16_t)0x0000)              /*!< UCLK */
#define EUSCI_A_CTLW0_SSEL__ACLK                 ((uint16_t)0x0040)              /*!< ACLK */
#define EUSCI_A_CTLW0_SSEL__SMCLK                ((uint16_t)0x0080)              /*!< SMCLK */
#define EUSCI_A_CTLW0_SYNC_OFS                   ( 8)                            /*!< UCSYNC Bit Offset */
#define EUSCI_A_CTLW0_SYNC                       ((uint16_t)0x0100)              /*!< Synchronous mode enable */
#define EUSCI_A_CTLW0_MODE_OFS                   ( 9)                            /*!< UCMODE Bit Offset */
#define EUSCI_A_CTLW0_MODE_MASK                  ((uint16_t)0x0600)              /*!< UCMODE Bit Mask */
#define EUSCI_A_CTLW0_MODE0                      ((uint16_t)0x0200)              /*!< MODE Bit 0 */
#define EUSCI_A_CTLW0_MODE1                      ((uint16_t)0x0400)              /*!< MODE Bit 1 */
#define EUSCI_A_CTLW0_MODE_0                     ((uint16_t)0x0000)              /*!< UART mode */
#define EUSCI_A_CTLW0_MODE_1                     ((uint16_t)0x0200)              /*!< Idle-line multiprocessor mode */
#define EUSCI_A_CTLW0_MODE_2                     ((uint16_t)0x0400)              /*!< Address-bit multiprocessor mode */
#define EUSCI_A_CTLW0_MODE_3                     ((uint16_t)0x0600)              /*!< UART mode with automatic baud-rate detection */
#define EUSCI_A_CTLW0_SPB_OFS                    (11)                            /*!< UCSPB Bit Offset */
#define EUSCI_A_CTLW0_SPB                        ((uint16_t)0x0800)              /*!< Stop bit select */
#define EUSCI_A_CTLW0_SEVENBIT_OFS               (12)                            /*!< UC7BIT Bit Offset */
#define EUSCI_A_CTLW0_SEVENBIT                   ((uint16_t)0x1000)              /*!< Character length */
#define EUSCI_A_CTLW0_MSB_OFS                    (13)                            /*!< UCMSB Bit Offset */
#define EUSCI_A_CTLW0_MSB                        ((uint16_t)0x2000)              /*!< MSB first select */
#define EUSCI_A_CTLW0_PAR_OFS                    (14)                            /*!< UCPAR Bit Offset */
#define EUSCI_A_CTLW0_PAR                        ((uint16_t)0x4000)              /*!< Parity select */
#define EUSCI_A_CTLW0_PEN_OFS                    (15)                            /*!< UCPEN Bit Offset */
#define EUSCI_A_CTLW0_PEN                        ((uint16_t)0x8000)              /*!< Parity enable */
#define EUSCI_A_CTLW0_STEM_OFS                   ( 1)                            /*!< UCSTEM Bit Offset */
#define EUSCI_A_CTLW0_STEM                       ((uint16_t)0x0002)              /*!< STE mode select in master mode. */
#define EUSCI_A_CTLW0_MST_OFS                    (11)                            /*!< UCMST Bit Offset */
#define EUSCI_A_CTLW0_MST                        ((uint16_t)0x0800)              /*!< Master mode select */
#define EUSCI_A_CTLW0_CKPL_OFS                   (14)                            /*!< UCCKPL Bit Offset */
#define EUSCI_A_CTLW0_CKPL                       ((uint16_t)0x4000)              /*!< Clock polarity select */
#define EUSCI_A_CTLW0_CKPH_OFS                   (15)                            /*!< UCCKPH Bit Offset */
#define EUSCI_A_CTLW0_CKPH                       ((uint16_t)0x8000)              /*!< Clock phase select */
#define EUSCI_A_CTLW1_GLIT_OFS                   ( 0)                            /*!< UCGLIT Bit Offset */
#define EUSCI_A_CTLW1_GLIT_MASK                  ((uint16_t)0x0003)              /*!< UCGLIT Bit Mask */
#define EUSCI_A_CTLW1_GLIT0                      ((uint16_t)0x0001)              /*!< GLIT Bit 0 */
#define EUSCI_A_CTLW1_GLIT1                      ((uint16_t)0x0002)              /*!< GLIT Bit 1 */
#define EUSCI_A_CTLW1_GLIT_0                     ((uint16_t)0x0000)              /*!< Approximately 2 ns (equivalent of 1 delay element) */
#define EUSCI_A_CTLW1_GLIT_1                     ((uint16_t)0x0001)              /*!< Approximately 50 ns */
#define EUSCI_A_CTLW1_GLIT_2                     ((uint16_t)0x0002)              /*!< Approximately 100 ns */
#define EUSCI_A_CTLW1_GLIT_3                     ((uint16_t)0x0003)              /*!< Approximately 200 ns */
#define EUSCI_A_MCTLW_OS16_OFS                   ( 0)                            /*!< UCOS16 Bit Offset */
#define EUSCI_A_MCTLW_OS16                       ((uint16_t)0x0001)              /*!< Oversampling mode enabled */
#define EUSCI_A_MCTLW_BRF_OFS                    ( 4)                            /*!< UCBRF Bit Offset */
#define EUSCI_A_MCTLW_BRF_MASK                   ((uint16_t)0x00F0)              /*!< UCBRF Bit Mask */
#define EUSCI_A_MCTLW_BRS_OFS                    ( 8)                            /*!< UCBRS Bit Offset */
#define EUSCI_A_MCTLW_BRS_MASK                   ((uint16_t)0xFF00)              /*!< UCBRS Bit Mask */
#define EUSCI_A_STATW_BUSY_OFS                   ( 0)                            /*!< UCBUSY Bit Offset */
#define EUSCI_A_STATW_BUSY                       ((uint16_t)0x0001)              /*!< eUSCI_A busy */
#define EUSCI_A_STATW_ADDR_IDLE_OFS              ( 1)                            /*!< UCADDR_UCIDLE Bit Offset */
#define EUSCI_A_STATW_ADDR_IDLE                  ((uint16_t)0x0002)              /*!< Address received / Idle line detected */
#define EUSCI_A_STATW_RXERR_OFS                  ( 2)                            /*!< UCRXERR Bit Offset */
#define EUSCI_A_STATW_RXERR                      ((uint16_t)0x0004)              /*!< Receive error flag */
#define EUSCI_A_STATW_BRK_OFS                    ( 3)                            /*!< UCBRK Bit Offset */
#define EUSCI_A_STATW_BRK                        ((uint16_t)0x0008)              /*!< Break detect flag */
#define EUSCI_A_STATW_PE_OFS                     ( 4)                            /*!< UCPE Bit Offset */
#define EUSCI_A_STATW_PE                         ((uint16_t)0x0010)              
#define EUSCI_A_STATW_OE_OFS                     ( 5)                            /*!< UCOE Bit Offset */
#define EUSCI_A_STATW_OE                         ((uint16_t)0x0020)              /*!< Overrun error flag */
#define EUSCI_A_STATW_FE_OFS                     ( 6)                            /*!< UCFE Bit Offset */
#define EUSCI_A_STATW_FE                         ((uint16_t)0x0040)              /*!< Framing error flag */
#define EUSCI_A_STATW_LISTEN_OFS                 ( 7)                            /*!< UCLISTEN Bit Offset */
#define EUSCI_A_STATW_LISTEN                     ((uint16_t)0x0080)              /*!< Listen enable */
#define EUSCI_A_STATW_SPI_BUSY_OFS               ( 0)                            /*!< UCBUSY Bit Offset */
#define EUSCI_A_STATW_SPI_BUSY                   ((uint16_t)0x0001)              /*!< eUSCI_A busy */
#define EUSCI_A_RXBUF_RXBUF_OFS                  ( 0)                            /*!< UCRXBUF Bit Offset */
#define EUSCI_A_RXBUF_RXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCRXBUF Bit Mask */
#define EUSCI_A_TXBUF_TXBUF_OFS                  ( 0)                            /*!< UCTXBUF Bit Offset */
#define EUSCI_A_TXBUF_TXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCTXBUF Bit Mask */
#define EUSCI_A_ABCTL_ABDEN_OFS                  ( 0)                            /*!< UCABDEN Bit Offset */
#define EUSCI_A_ABCTL_ABDEN                      ((uint16_t)0x0001)              /*!< Automatic baud-rate detect enable */
#define EUSCI_A_ABCTL_BTOE_OFS                   ( 2)                            /*!< UCBTOE Bit Offset */
#define EUSCI_A_ABCTL_BTOE                       ((uint16_t)0x0004)              /*!< Break time out error */
#define EUSCI_A_ABCTL_STOE_OFS                   ( 3)                            /*!< UCSTOE Bit Offset */
#define EUSCI_A_ABCTL_STOE                       ((uint16_t)0x0008)              /*!< Synch field time out error */
#define EUSCI_A_ABCTL_DELIM_OFS                  ( 4)                            /*!< UCDELIM Bit Offset */
#define EUSCI_A_ABCTL_DELIM_MASK                 ((uint16_t)0x0030)              /*!< UCDELIM Bit Mask */
#define EUSCI_A_ABCTL_DELIM0                     ((uint16_t)0x0010)              /*!< DELIM Bit 0 */
#define EUSCI_A_ABCTL_DELIM1                     ((uint16_t)0x0020)              /*!< DELIM Bit 1 */
#define EUSCI_A_ABCTL_DELIM_0                    ((uint16_t)0x0000)              /*!< 1 bit time */
#define EUSCI_A_ABCTL_DELIM_1                    ((uint16_t)0x0010)              /*!< 2 bit times */
#define EUSCI_A_ABCTL_DELIM_2                    ((uint16_t)0x0020)              /*!< 3 bit times */
#define EUSCI_A_ABCTL_DELIM_3                    ((uint16_t)0x0030)              /*!< 4 bit times */
#define EUSCI_A_IRCTL_IREN_OFS                   ( 0)                            /*!< UCIREN Bit Offset */
#define EUSCI_A_IRCTL_IREN                       ((uint16_t)0x0001)              /*!< IrDA encoder/decoder enable */
#define EUSCI_A_IRCTL_IRTXCLK_OFS                ( 1)                            /*!< UCIRTXCLK Bit Offset */
#define EUSCI_A_IRCTL_IRTXCLK                    ((uint16_t)0x0002)              /*!< IrDA transmit pulse clock select */
#define EUSCI_A_IRCTL_IRTXPL_OFS                 ( 2)                            /*!< UCIRTXPL Bit Offset */
#define EUSCI_A_IRCTL_IRTXPL_MASK                ((uint16_t)0x00FC)              /*!< UCIRTXPL Bit Mask */
#define EUSCI_A_IRCTL_IRRXFE_OFS                 ( 8)                            /*!< UCIRRXFE Bit Offset */
#define EUSCI_A_IRCTL_IRRXFE                     ((uint16_t)0x0100)              /*!< IrDA receive filter enabled */
#define EUSCI_A_IRCTL_IRRXPL_OFS                 ( 9)                            /*!< UCIRRXPL Bit Offset */
#define EUSCI_A_IRCTL_IRRXPL                     ((uint16_t)0x0200)              /*!< IrDA receive input UCAxRXD polarity */
#define EUSCI_A_IRCTL_IRRXFL_OFS                 (10)                            /*!< UCIRRXFL Bit Offset */
#define EUSCI_A_IRCTL_IRRXFL_MASK                ((uint16_t)0x3C00)              /*!< UCIRRXFL Bit Mask */
#define EUSCI_A_IE_RXIE_OFS                      ( 0)                            /*!< UCRXIE Bit Offset */
#define EUSCI_A_IE_RXIE                          ((uint16_t)0x0001)              /*!< Receive interrupt enable */
#define EUSCI_A_IE_TXIE_OFS                      ( 1)                            /*!< UCTXIE Bit Offset */
#define EUSCI_A_IE_TXIE                          ((uint16_t)0x0002)              /*!< Transmit interrupt enable */
#define EUSCI_A_IE_STTIE_OFS                     ( 2)                            /*!< UCSTTIE Bit Offset */
#define EUSCI_A_IE_STTIE                         ((uint16_t)0x0004)              /*!< Start bit interrupt enable */
#define EUSCI_A_IE_TXCPTIE_OFS                   ( 3)                            /*!< UCTXCPTIE Bit Offset */
#define EUSCI_A_IE_TXCPTIE                       ((uint16_t)0x0008)              /*!< Transmit complete interrupt enable */
#define EUSCI_A_IFG_RXIFG_OFS                    ( 0)                            /*!< UCRXIFG Bit Offset */
#define EUSCI_A_IFG_RXIFG                        ((uint16_t)0x0001)              /*!< Receive interrupt flag */
#define EUSCI_A_IFG_TXIFG_OFS                    ( 1)                            /*!< UCTXIFG Bit Offset */
#define EUSCI_A_IFG_TXIFG                        ((uint16_t)0x0002)              /*!< Transmit interrupt flag */
#define EUSCI_A_IFG_STTIFG_OFS                   ( 2)                            /*!< UCSTTIFG Bit Offset */
#define EUSCI_A_IFG_STTIFG                       ((uint16_t)0x0004)              /*!< Start bit interrupt flag */
#define EUSCI_A_IFG_TXCPTIFG_OFS                 ( 3)                            /*!< UCTXCPTIFG Bit Offset */
#define EUSCI_A_IFG_TXCPTIFG                     ((uint16_t)0x0008)              /*!< Transmit ready interrupt enable */
#define EUSCI_A__RXIE_OFS                        EUSCI_A_IE_RXIE_OFS             /*!< UCRXIE Bit Offset */
#define EUSCI_A__RXIE                            EUSCI_A_IE_RXIE                 /*!< Receive interrupt enable */
#define EUSCI_A__TXIE_OFS                        EUSCI_A_IE_TXIE_OFS             /*!< UCTXIE Bit Offset */
#define EUSCI_A__TXIE                            EUSCI_A_IE_TXIE                 /*!< Transmit interrupt enable */
#define EUSCI_B_CTLW0_SWRST_OFS                  ( 0)                            /*!< UCSWRST Bit Offset */
#define EUSCI_B_CTLW0_SWRST                      ((uint16_t)0x0001)              /*!< Software reset enable */
#define EUSCI_B_CTLW0_TXSTT_OFS                  ( 1)                            /*!< UCTXSTT Bit Offset */
#define EUSCI_B_CTLW0_TXSTT                      ((uint16_t)0x0002)              /*!< Transmit START condition in master mode */
#define EUSCI_B_CTLW0_TXSTP_OFS                  ( 2)                            /*!< UCTXSTP Bit Offset */
#define EUSCI_B_CTLW0_TXSTP                      ((uint16_t)0x0004)              /*!< Transmit STOP condition in master mode */
#define EUSCI_B_CTLW0_TXNACK_OFS                 ( 3)                            /*!< UCTXNACK Bit Offset */
#define EUSCI_B_CTLW0_TXNACK                     ((uint16_t)0x0008)              /*!< Transmit a NACK */
#define EUSCI_B_CTLW0_TR_OFS                     ( 4)                            /*!< UCTR Bit Offset */
#define EUSCI_B_CTLW0_TR                         ((uint16_t)0x0010)              /*!< Transmitter/receiver */
#define EUSCI_B_CTLW0_TXACK_OFS                  ( 5)                            /*!< UCTXACK Bit Offset */
#define EUSCI_B_CTLW0_TXACK                      ((uint16_t)0x0020)              /*!< Transmit ACK condition in slave mode */
#define EUSCI_B_CTLW0_SSEL_OFS                   ( 6)                            /*!< UCSSEL Bit Offset */
#define EUSCI_B_CTLW0_SSEL_MASK                  ((uint16_t)0x00C0)              /*!< UCSSEL Bit Mask */
#define EUSCI_B_CTLW0_SSEL0                      ((uint16_t)0x0040)              /*!< SSEL Bit 0 */
#define EUSCI_B_CTLW0_SSEL1                      ((uint16_t)0x0080)              /*!< SSEL Bit 1 */
#define EUSCI_B_CTLW0_UCSSEL_0                   ((uint16_t)0x0000)              /*!< UCLKI */
#define EUSCI_B_CTLW0_UCSSEL_1                   ((uint16_t)0x0040)              /*!< ACLK */
#define EUSCI_B_CTLW0_UCSSEL_2                   ((uint16_t)0x0080)              /*!< SMCLK */
#define EUSCI_B_CTLW0_UCSSEL_3                   ((uint16_t)0x00C0)              /*!< SMCLK */
#define EUSCI_B_CTLW0_SSEL__UCLKI                ((uint16_t)0x0000)              /*!< UCLKI */
#define EUSCI_B_CTLW0_SSEL__ACLK                 ((uint16_t)0x0040)              /*!< ACLK */
#define EUSCI_B_CTLW0_SSEL__SMCLK                ((uint16_t)0x0080)              /*!< SMCLK */
#define EUSCI_B_CTLW0_SYNC_OFS                   ( 8)                            /*!< UCSYNC Bit Offset */
#define EUSCI_B_CTLW0_SYNC                       ((uint16_t)0x0100)              /*!< Synchronous mode enable */
#define EUSCI_B_CTLW0_MODE_OFS                   ( 9)                            /*!< UCMODE Bit Offset */
#define EUSCI_B_CTLW0_MODE_MASK                  ((uint16_t)0x0600)              /*!< UCMODE Bit Mask */
#define EUSCI_B_CTLW0_MODE0                      ((uint16_t)0x0200)              /*!< MODE Bit 0 */
#define EUSCI_B_CTLW0_MODE1                      ((uint16_t)0x0400)              /*!< MODE Bit 1 */
#define EUSCI_B_CTLW0_MODE_0                     ((uint16_t)0x0000)              /*!< 3-pin SPI */
#define EUSCI_B_CTLW0_MODE_1                     ((uint16_t)0x0200)              /*!< 4-pin SPI (master or slave enabled if STE = 1) */
#define EUSCI_B_CTLW0_MODE_2                     ((uint16_t)0x0400)              /*!< 4-pin SPI (master or slave enabled if STE = 0) */
#define EUSCI_B_CTLW0_MODE_3                     ((uint16_t)0x0600)              /*!< I2C mode */
#define EUSCI_B_CTLW0_MST_OFS                    (11)                            /*!< UCMST Bit Offset */
#define EUSCI_B_CTLW0_MST                        ((uint16_t)0x0800)              /*!< Master mode select */
#define EUSCI_B_CTLW0_MM_OFS                     (13)                            /*!< UCMM Bit Offset */
#define EUSCI_B_CTLW0_MM                         ((uint16_t)0x2000)              /*!< Multi-master environment select */
#define EUSCI_B_CTLW0_SLA10_OFS                  (14)                            /*!< UCSLA10 Bit Offset */
#define EUSCI_B_CTLW0_SLA10                      ((uint16_t)0x4000)              /*!< Slave addressing mode select */
#define EUSCI_B_CTLW0_A10_OFS                    (15)                            /*!< UCA10 Bit Offset */
#define EUSCI_B_CTLW0_A10                        ((uint16_t)0x8000)              /*!< Own addressing mode select */
#define EUSCI_B_CTLW0_STEM_OFS                   ( 1)                            /*!< UCSTEM Bit Offset */
#define EUSCI_B_CTLW0_STEM                       ((uint16_t)0x0002)              /*!< STE mode select in master mode. */
#define EUSCI_B_CTLW0_SEVENBIT_OFS               (12)                            /*!< UC7BIT Bit Offset */
#define EUSCI_B_CTLW0_SEVENBIT                   ((uint16_t)0x1000)              /*!< Character length */
#define EUSCI_B_CTLW0_MSB_OFS                    (13)                            /*!< UCMSB Bit Offset */
#define EUSCI_B_CTLW0_MSB                        ((uint16_t)0x2000)              /*!< MSB first select */
#define EUSCI_B_CTLW0_CKPL_OFS                   (14)                            /*!< UCCKPL Bit Offset */
#define EUSCI_B_CTLW0_CKPL                       ((uint16_t)0x4000)              /*!< Clock polarity select */
#define EUSCI_B_CTLW0_CKPH_OFS                   (15)                            /*!< UCCKPH Bit Offset */
#define EUSCI_B_CTLW0_CKPH                       ((uint16_t)0x8000)              /*!< Clock phase select */
#define EUSCI_B_CTLW1_GLIT_OFS                   ( 0)                            /*!< UCGLIT Bit Offset */
#define EUSCI_B_CTLW1_GLIT_MASK                  ((uint16_t)0x0003)              /*!< UCGLIT Bit Mask */
#define EUSCI_B_CTLW1_GLIT0                      ((uint16_t)0x0001)              /*!< GLIT Bit 0 */
#define EUSCI_B_CTLW1_GLIT1                      ((uint16_t)0x0002)              /*!< GLIT Bit 1 */
#define EUSCI_B_CTLW1_GLIT_0                     ((uint16_t)0x0000)              /*!< 50 ns */
#define EUSCI_B_CTLW1_GLIT_1                     ((uint16_t)0x0001)              /*!< 25 ns */
#define EUSCI_B_CTLW1_GLIT_2                     ((uint16_t)0x0002)              /*!< 12.5 ns */
#define EUSCI_B_CTLW1_GLIT_3                     ((uint16_t)0x0003)              /*!< 6.25 ns */
#define EUSCI_B_CTLW1_ASTP_OFS                   ( 2)                            /*!< UCASTP Bit Offset */
#define EUSCI_B_CTLW1_ASTP_MASK                  ((uint16_t)0x000C)              /*!< UCASTP Bit Mask */
#define EUSCI_B_CTLW1_ASTP0                      ((uint16_t)0x0004)              /*!< ASTP Bit 0 */
#define EUSCI_B_CTLW1_ASTP1                      ((uint16_t)0x0008)              /*!< ASTP Bit 1 */
#define EUSCI_B_CTLW1_ASTP_0                     ((uint16_t)0x0000)              /*!< No automatic STOP generation. The STOP condition is generated after the user  */
#define EUSCI_B_CTLW1_ASTP_1                     ((uint16_t)0x0004)              /*!< UCBCNTIFG is set with the byte counter reaches the threshold defined in  */
#define EUSCI_B_CTLW1_ASTP_2                     ((uint16_t)0x0008)              /*!< A STOP condition is generated automatically after the byte counter value  */
#define EUSCI_B_CTLW1_SWACK_OFS                  ( 4)                            /*!< UCSWACK Bit Offset */
#define EUSCI_B_CTLW1_SWACK                      ((uint16_t)0x0010)              /*!< SW or HW ACK control */
#define EUSCI_B_CTLW1_STPNACK_OFS                ( 5)                            /*!< UCSTPNACK Bit Offset */
#define EUSCI_B_CTLW1_STPNACK                    ((uint16_t)0x0020)              /*!< ACK all master bytes */
#define EUSCI_B_CTLW1_CLTO_OFS                   ( 6)                            /*!< UCCLTO Bit Offset */
#define EUSCI_B_CTLW1_CLTO_MASK                  ((uint16_t)0x00C0)              /*!< UCCLTO Bit Mask */
#define EUSCI_B_CTLW1_CLTO0                      ((uint16_t)0x0040)              /*!< CLTO Bit 0 */
#define EUSCI_B_CTLW1_CLTO1                      ((uint16_t)0x0080)              /*!< CLTO Bit 1 */
#define EUSCI_B_CTLW1_CLTO_0                     ((uint16_t)0x0000)              /*!< Disable clock low timeout counter */
#define EUSCI_B_CTLW1_CLTO_1                     ((uint16_t)0x0040)              /*!< 135 000 SYSCLK cycles (approximately 28 ms) */
#define EUSCI_B_CTLW1_CLTO_2                     ((uint16_t)0x0080)              /*!< 150 000 SYSCLK cycles (approximately 31 ms) */
#define EUSCI_B_CTLW1_CLTO_3                     ((uint16_t)0x00C0)              /*!< 165 000 SYSCLK cycles (approximately 34 ms) */
#define EUSCI_B_CTLW1_ETXINT_OFS                 ( 8)                            /*!< UCETXINT Bit Offset */
#define EUSCI_B_CTLW1_ETXINT                     ((uint16_t)0x0100)              /*!< Early UCTXIFG0 */
#define EUSCI_B_STATW_BBUSY_OFS                  ( 4)                            /*!< UCBBUSY Bit Offset */
#define EUSCI_B_STATW_BBUSY                      ((uint16_t)0x0010)              /*!< Bus busy */
#define EUSCI_B_STATW_GC_OFS                     ( 5)                            /*!< UCGC Bit Offset */
#define EUSCI_B_STATW_GC                         ((uint16_t)0x0020)              /*!< General call address received */
#define EUSCI_B_STATW_SCLLOW_OFS                 ( 6)                            /*!< UCSCLLOW Bit Offset */
#define EUSCI_B_STATW_SCLLOW                     ((uint16_t)0x0040)              /*!< SCL low */
#define EUSCI_B_STATW_BCNT_OFS                   ( 8)                            /*!< UCBCNT Bit Offset */
#define EUSCI_B_STATW_BCNT_MASK                  ((uint16_t)0xFF00)              /*!< UCBCNT Bit Mask */
#define EUSCI_B_STATW_SPI_BUSY_OFS               ( 0)                            /*!< UCBUSY Bit Offset */
#define EUSCI_B_STATW_SPI_BUSY                   ((uint16_t)0x0001)              /*!< eUSCI_B busy */
#define EUSCI_B_STATW_OE_OFS                     ( 5)                            /*!< UCOE Bit Offset */
#define EUSCI_B_STATW_OE                         ((uint16_t)0x0020)              /*!< Overrun error flag */
#define EUSCI_B_STATW_FE_OFS                     ( 6)                            /*!< UCFE Bit Offset */
#define EUSCI_B_STATW_FE                         ((uint16_t)0x0040)              /*!< Framing error flag */
#define EUSCI_B_STATW_LISTEN_OFS                 ( 7)                            /*!< UCLISTEN Bit Offset */
#define EUSCI_B_STATW_LISTEN                     ((uint16_t)0x0080)              /*!< Listen enable */
#define EUSCI_B_TBCNT_TBCNT_OFS                  ( 0)                            /*!< UCTBCNT Bit Offset */
#define EUSCI_B_TBCNT_TBCNT_MASK                 ((uint16_t)0x00FF)              /*!< UCTBCNT Bit Mask */
#define EUSCI_B_RXBUF_RXBUF_OFS                  ( 0)                            /*!< UCRXBUF Bit Offset */
#define EUSCI_B_RXBUF_RXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCRXBUF Bit Mask */
#define EUSCI_B_TXBUF_TXBUF_OFS                  ( 0)                            /*!< UCTXBUF Bit Offset */
#define EUSCI_B_TXBUF_TXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCTXBUF Bit Mask */
#define EUSCI_B_I2COA0_I2COA0_OFS                ( 0)                            /*!< I2COA0 Bit Offset */
#define EUSCI_B_I2COA0_I2COA0_MASK               ((uint16_t)0x03FF)              /*!< I2COA0 Bit Mask */
#define EUSCI_B_I2COA0_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA0_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
#define EUSCI_B_I2COA0_GCEN_OFS                  (15)                            /*!< UCGCEN Bit Offset */
#define EUSCI_B_I2COA0_GCEN                      ((uint16_t)0x8000)              /*!< General call response enable */
#define EUSCI_B_I2COA1_I2COA1_OFS                ( 0)                            /*!< I2COA1 Bit Offset */
#define EUSCI_B_I2COA1_I2COA1_MASK               ((uint16_t)0x03FF)              /*!< I2COA1 Bit Mask */
#define EUSCI_B_I2COA1_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA1_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
#define EUSCI_B_I2COA2_I2COA2_OFS                ( 0)                            /*!< I2COA2 Bit Offset */
#define EUSCI_B_I2COA2_I2COA2_MASK               ((uint16_t)0x03FF)              /*!< I2COA2 Bit Mask */
#define EUSCI_B_I2COA2_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA2_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
#define EUSCI_B_I2COA3_I2COA3_OFS                ( 0)                            /*!< I2COA3 Bit Offset */
#define EUSCI_B_I2COA3_I2COA3_MASK               ((uint16_t)0x03FF)              /*!< I2COA3 Bit Mask */
#define EUSCI_B_I2COA3_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA3_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
#define EUSCI_B_ADDRX_ADDRX_OFS                  ( 0)                            /*!< ADDRX Bit Offset */
#define EUSCI_B_ADDRX_ADDRX_MASK                 ((uint16_t)0x03FF)              /*!< ADDRX Bit Mask */
#define EUSCI_B_ADDRX_ADDRX0                     ((uint16_t)0x0001)              /*!< ADDRX Bit 0 */
#define EUSCI_B_ADDRX_ADDRX1                     ((uint16_t)0x0002)              /*!< ADDRX Bit 1 */
#define EUSCI_B_ADDRX_ADDRX2                     ((uint16_t)0x0004)              /*!< ADDRX Bit 2 */
#define EUSCI_B_ADDRX_ADDRX3                     ((uint16_t)0x0008)              /*!< ADDRX Bit 3 */
#define EUSCI_B_ADDRX_ADDRX4                     ((uint16_t)0x0010)              /*!< ADDRX Bit 4 */
#define EUSCI_B_ADDRX_ADDRX5                     ((uint16_t)0x0020)              /*!< ADDRX Bit 5 */
#define EUSCI_B_ADDRX_ADDRX6                     ((uint16_t)0x0040)              /*!< ADDRX Bit 6 */
#define EUSCI_B_ADDRX_ADDRX7                     ((uint16_t)0x0080)              /*!< ADDRX Bit 7 */
#define EUSCI_B_ADDRX_ADDRX8                     ((uint16_t)0x0100)              /*!< ADDRX Bit 8 */
#define EUSCI_B_ADDRX_ADDRX9                     ((uint16_t)0x0200)              /*!< ADDRX Bit 9 */
#define EUSCI_B_ADDMASK_ADDMASK_OFS              ( 0)                            /*!< ADDMASK Bit Offset */
#define EUSCI_B_ADDMASK_ADDMASK_MASK             ((uint16_t)0x03FF)              /*!< ADDMASK Bit Mask */
#define EUSCI_B_I2CSA_I2CSA_OFS                  ( 0)                            /*!< I2CSA Bit Offset */
#define EUSCI_B_I2CSA_I2CSA_MASK                 ((uint16_t)0x03FF)              /*!< I2CSA Bit Mask */
#define EUSCI_B_IE_RXIE0_OFS                     ( 0)                            /*!< UCRXIE0 Bit Offset */
#define EUSCI_B_IE_RXIE0                         ((uint16_t)0x0001)              /*!< Receive interrupt enable 0 */
#define EUSCI_B_IE_TXIE0_OFS                     ( 1)                            /*!< UCTXIE0 Bit Offset */
#define EUSCI_B_IE_TXIE0                         ((uint16_t)0x0002)              /*!< Transmit interrupt enable 0 */
#define EUSCI_B_IE_STTIE_OFS                     ( 2)                            /*!< UCSTTIE Bit Offset */
#define EUSCI_B_IE_STTIE                         ((uint16_t)0x0004)              /*!< START condition interrupt enable */
#define EUSCI_B_IE_STPIE_OFS                     ( 3)                            /*!< UCSTPIE Bit Offset */
#define EUSCI_B_IE_STPIE                         ((uint16_t)0x0008)              /*!< STOP condition interrupt enable */
#define EUSCI_B_IE_ALIE_OFS                      ( 4)                            /*!< UCALIE Bit Offset */
#define EUSCI_B_IE_ALIE                          ((uint16_t)0x0010)              /*!< Arbitration lost interrupt enable */
#define EUSCI_B_IE_NACKIE_OFS                    ( 5)                            /*!< UCNACKIE Bit Offset */
#define EUSCI_B_IE_NACKIE                        ((uint16_t)0x0020)              /*!< Not-acknowledge interrupt enable */
#define EUSCI_B_IE_BCNTIE_OFS                    ( 6)                            /*!< UCBCNTIE Bit Offset */
#define EUSCI_B_IE_BCNTIE                        ((uint16_t)0x0040)              /*!< Byte counter interrupt enable */
#define EUSCI_B_IE_CLTOIE_OFS                    ( 7)                            /*!< UCCLTOIE Bit Offset */
#define EUSCI_B_IE_CLTOIE                        ((uint16_t)0x0080)              /*!< Clock low timeout interrupt enable */
#define EUSCI_B_IE_RXIE1_OFS                     ( 8)                            /*!< UCRXIE1 Bit Offset */
#define EUSCI_B_IE_RXIE1                         ((uint16_t)0x0100)              /*!< Receive interrupt enable 1 */
#define EUSCI_B_IE_TXIE1_OFS                     ( 9)                            /*!< UCTXIE1 Bit Offset */
#define EUSCI_B_IE_TXIE1                         ((uint16_t)0x0200)              /*!< Transmit interrupt enable 1 */
#define EUSCI_B_IE_RXIE2_OFS                     (10)                            /*!< UCRXIE2 Bit Offset */
#define EUSCI_B_IE_RXIE2                         ((uint16_t)0x0400)              /*!< Receive interrupt enable 2 */
#define EUSCI_B_IE_TXIE2_OFS                     (11)                            /*!< UCTXIE2 Bit Offset */
#define EUSCI_B_IE_TXIE2                         ((uint16_t)0x0800)              /*!< Transmit interrupt enable 2 */
#define EUSCI_B_IE_RXIE3_OFS                     (12)                            /*!< UCRXIE3 Bit Offset */
#define EUSCI_B_IE_RXIE3                         ((uint16_t)0x1000)              /*!< Receive interrupt enable 3 */
#define EUSCI_B_IE_TXIE3_OFS                     (13)                            /*!< UCTXIE3 Bit Offset */
#define EUSCI_B_IE_TXIE3                         ((uint16_t)0x2000)              /*!< Transmit interrupt enable 3 */
#define EUSCI_B_IE_BIT9IE_OFS                    (14)                            /*!< UCBIT9IE Bit Offset */
#define EUSCI_B_IE_BIT9IE                        ((uint16_t)0x4000)              /*!< Bit position 9 interrupt enable */
#define EUSCI_B_IE_RXIE_OFS                      ( 0)                            /*!< UCRXIE Bit Offset */
#define EUSCI_B_IE_RXIE                          ((uint16_t)0x0001)              /*!< Receive interrupt enable */
#define EUSCI_B_IE_TXIE_OFS                      ( 1)                            /*!< UCTXIE Bit Offset */
#define EUSCI_B_IE_TXIE                          ((uint16_t)0x0002)              /*!< Transmit interrupt enable */
#define EUSCI_B_IFG_RXIFG0_OFS                   ( 0)                            /*!< UCRXIFG0 Bit Offset */
#define EUSCI_B_IFG_RXIFG0                       ((uint16_t)0x0001)              /*!< eUSCI_B receive interrupt flag 0 */
#define EUSCI_B_IFG_TXIFG0_OFS                   ( 1)                            /*!< UCTXIFG0 Bit Offset */
#define EUSCI_B_IFG_TXIFG0                       ((uint16_t)0x0002)              /*!< eUSCI_B transmit interrupt flag 0 */
#define EUSCI_B_IFG_STTIFG_OFS                   ( 2)                            /*!< UCSTTIFG Bit Offset */
#define EUSCI_B_IFG_STTIFG                       ((uint16_t)0x0004)              /*!< START condition interrupt flag */
#define EUSCI_B_IFG_STPIFG_OFS                   ( 3)                            /*!< UCSTPIFG Bit Offset */
#define EUSCI_B_IFG_STPIFG                       ((uint16_t)0x0008)              /*!< STOP condition interrupt flag */
#define EUSCI_B_IFG_ALIFG_OFS                    ( 4)                            /*!< UCALIFG Bit Offset */
#define EUSCI_B_IFG_ALIFG                        ((uint16_t)0x0010)              /*!< Arbitration lost interrupt flag */
#define EUSCI_B_IFG_NACKIFG_OFS                  ( 5)                            /*!< UCNACKIFG Bit Offset */
#define EUSCI_B_IFG_NACKIFG                      ((uint16_t)0x0020)              /*!< Not-acknowledge received interrupt flag */
#define EUSCI_B_IFG_BCNTIFG_OFS                  ( 6)                            /*!< UCBCNTIFG Bit Offset */
#define EUSCI_B_IFG_BCNTIFG                      ((uint16_t)0x0040)              /*!< Byte counter interrupt flag */
#define EUSCI_B_IFG_CLTOIFG_OFS                  ( 7)                            /*!< UCCLTOIFG Bit Offset */
#define EUSCI_B_IFG_CLTOIFG                      ((uint16_t)0x0080)              /*!< Clock low timeout interrupt flag */
#define EUSCI_B_IFG_RXIFG1_OFS                   ( 8)                            /*!< UCRXIFG1 Bit Offset */
#define EUSCI_B_IFG_RXIFG1                       ((uint16_t)0x0100)              /*!< eUSCI_B receive interrupt flag 1 */
#define EUSCI_B_IFG_TXIFG1_OFS                   ( 9)                            /*!< UCTXIFG1 Bit Offset */
#define EUSCI_B_IFG_TXIFG1                       ((uint16_t)0x0200)              /*!< eUSCI_B transmit interrupt flag 1 */
#define EUSCI_B_IFG_RXIFG2_OFS                   (10)                            /*!< UCRXIFG2 Bit Offset */
#define EUSCI_B_IFG_RXIFG2                       ((uint16_t)0x0400)              /*!< eUSCI_B receive interrupt flag 2 */
#define EUSCI_B_IFG_TXIFG2_OFS                   (11)                            /*!< UCTXIFG2 Bit Offset */
#define EUSCI_B_IFG_TXIFG2                       ((uint16_t)0x0800)              /*!< eUSCI_B transmit interrupt flag 2 */
#define EUSCI_B_IFG_RXIFG3_OFS                   (12)                            /*!< UCRXIFG3 Bit Offset */
#define EUSCI_B_IFG_RXIFG3                       ((uint16_t)0x1000)              /*!< eUSCI_B receive interrupt flag 3 */
#define EUSCI_B_IFG_TXIFG3_OFS                   (13)                            /*!< UCTXIFG3 Bit Offset */
#define EUSCI_B_IFG_TXIFG3                       ((uint16_t)0x2000)              /*!< eUSCI_B transmit interrupt flag 3 */
#define EUSCI_B_IFG_BIT9IFG_OFS                  (14)                            /*!< UCBIT9IFG Bit Offset */
#define EUSCI_B_IFG_BIT9IFG                      ((uint16_t)0x4000)              /*!< Bit position 9 interrupt flag */
#define EUSCI_B_IFG_RXIFG_OFS                    ( 0)                            /*!< UCRXIFG Bit Offset */
#define EUSCI_B_IFG_RXIFG                        ((uint16_t)0x0001)              /*!< Receive interrupt flag */
#define EUSCI_B_IFG_TXIFG_OFS                    ( 1)                            /*!< UCTXIFG Bit Offset */
#define EUSCI_B_IFG_TXIFG                        ((uint16_t)0x0002)              /*!< Transmit interrupt flag */
#define EUSCI_B__RXIE_OFS                        EUSCI_B_IE_RXIE_OFS             /*!< UCRXIE Bit Offset */
#define EUSCI_B__RXIE                            EUSCI_B_IE_RXIE                 /*!< Receive interrupt enable */
#define EUSCI_B__TXIE_OFS                        EUSCI_B_IE_TXIE_OFS             /*!< UCTXIE Bit Offset */
#define EUSCI_B__TXIE                            EUSCI_B_IE_TXIE                 /*!< Transmit interrupt enable */
#define FLCTL_POWER_STAT_PSTAT_OFS               ( 0)                            /*!< PSTAT Bit Offset */
#define FLCTL_POWER_STAT_PSTAT_MASK              ((uint32_t)0x00000007)          /*!< PSTAT Bit Mask */
#define FLCTL_POWER_STAT_PSTAT0                  ((uint32_t)0x00000001)          /*!< PSTAT Bit 0 */
#define FLCTL_POWER_STAT_PSTAT1                  ((uint32_t)0x00000002)          /*!< PSTAT Bit 1 */
#define FLCTL_POWER_STAT_PSTAT2                  ((uint32_t)0x00000004)          /*!< PSTAT Bit 2 */
#define FLCTL_POWER_STAT_PSTAT_0                 ((uint32_t)0x00000000)          /*!< Flash IP in power-down mode */
#define FLCTL_POWER_STAT_PSTAT_1                 ((uint32_t)0x00000001)          /*!< Flash IP Vdd domain power-up in progress */
#define FLCTL_POWER_STAT_PSTAT_2                 ((uint32_t)0x00000002)          /*!< PSS LDO_GOOD, IREF_OK and VREF_OK check in progress */
#define FLCTL_POWER_STAT_PSTAT_3                 ((uint32_t)0x00000003)          /*!< Flash IP SAFE_LV check in progress */
#define FLCTL_POWER_STAT_PSTAT_4                 ((uint32_t)0x00000004)          /*!< Flash IP Active */
#define FLCTL_POWER_STAT_PSTAT_5                 ((uint32_t)0x00000005)          /*!< Flash IP Active in Low-Frequency Active and Low-Frequency LPM0 modes. */
#define FLCTL_POWER_STAT_PSTAT_6                 ((uint32_t)0x00000006)          /*!< Flash IP in Standby mode */
#define FLCTL_POWER_STAT_PSTAT_7                 ((uint32_t)0x00000007)          /*!< Flash IP in Current mirror boost state */
#define FLCTL_POWER_STAT_LDOSTAT_OFS             ( 3)                            /*!< LDOSTAT Bit Offset */
#define FLCTL_POWER_STAT_LDOSTAT                 ((uint32_t)0x00000008)          /*!< PSS FLDO GOOD status */
#define FLCTL_POWER_STAT_VREFSTAT_OFS            ( 4)                            /*!< VREFSTAT Bit Offset */
#define FLCTL_POWER_STAT_VREFSTAT                ((uint32_t)0x00000010)          /*!< PSS VREF stable status */
#define FLCTL_POWER_STAT_IREFSTAT_OFS            ( 5)                            /*!< IREFSTAT Bit Offset */
#define FLCTL_POWER_STAT_IREFSTAT                ((uint32_t)0x00000020)          /*!< PSS IREF stable status */
#define FLCTL_POWER_STAT_TRIMSTAT_OFS            ( 6)                            /*!< TRIMSTAT Bit Offset */
#define FLCTL_POWER_STAT_TRIMSTAT                ((uint32_t)0x00000040)          /*!< PSS trim done status */
#define FLCTL_POWER_STAT_RD_2T_OFS               ( 7)                            /*!< RD_2T Bit Offset */
#define FLCTL_POWER_STAT_RD_2T                   ((uint32_t)0x00000080)          /*!< Indicates if Flash is being accessed in 2T mode */
#define FLCTL_BANK0_RDCTL_RD_MODE_OFS            ( 0)                            /*!< RD_MODE Bit Offset */
#define FLCTL_BANK0_RDCTL_RD_MODE_MASK           ((uint32_t)0x0000000F)          /*!< RD_MODE Bit Mask */
#define FLCTL_BANK0_RDCTL_RD_MODE0               ((uint32_t)0x00000001)          /*!< RD_MODE Bit 0 */
#define FLCTL_BANK0_RDCTL_RD_MODE1               ((uint32_t)0x00000002)          /*!< RD_MODE Bit 1 */
#define FLCTL_BANK0_RDCTL_RD_MODE2               ((uint32_t)0x00000004)          /*!< RD_MODE Bit 2 */
#define FLCTL_BANK0_RDCTL_RD_MODE3               ((uint32_t)0x00000008)          /*!< RD_MODE Bit 3 */
#define FLCTL_BANK0_RDCTL_RD_MODE_0              ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_BANK0_RDCTL_RD_MODE_1              ((uint32_t)0x00000001)          /*!< Read Margin 0 */
#define FLCTL_BANK0_RDCTL_RD_MODE_2              ((uint32_t)0x00000002)          /*!< Read Margin 1 */
#define FLCTL_BANK0_RDCTL_RD_MODE_3              ((uint32_t)0x00000003)          /*!< Program Verify */
#define FLCTL_BANK0_RDCTL_RD_MODE_4              ((uint32_t)0x00000004)          /*!< Erase Verify */
#define FLCTL_BANK0_RDCTL_RD_MODE_5              ((uint32_t)0x00000005)          /*!< Leakage Verify */
#define FLCTL_BANK0_RDCTL_RD_MODE_9              ((uint32_t)0x00000009)          /*!< Read Margin 0B */
#define FLCTL_BANK0_RDCTL_RD_MODE_10             ((uint32_t)0x0000000A)          /*!< Read Margin 1B */
#define FLCTL_BANK0_RDCTL_BUFI_OFS               ( 4)                            /*!< BUFI Bit Offset */
#define FLCTL_BANK0_RDCTL_BUFI                   ((uint32_t)0x00000010)          /*!< Enables read buffering feature for instruction fetches to this Bank */
#define FLCTL_BANK0_RDCTL_BUFD_OFS               ( 5)                            /*!< BUFD Bit Offset */
#define FLCTL_BANK0_RDCTL_BUFD                   ((uint32_t)0x00000020)          /*!< Enables read buffering feature for data reads to this Bank */
#define FLCTL_BANK0_RDCTL_WAIT_OFS               (12)                            /*!< WAIT Bit Offset */
#define FLCTL_BANK0_RDCTL_WAIT_MASK              ((uint32_t)0x0000F000)          /*!< WAIT Bit Mask */
#define FLCTL_BANK0_RDCTL_WAIT0                  ((uint32_t)0x00001000)          /*!< WAIT Bit 0 */
#define FLCTL_BANK0_RDCTL_WAIT1                  ((uint32_t)0x00002000)          /*!< WAIT Bit 1 */
#define FLCTL_BANK0_RDCTL_WAIT2                  ((uint32_t)0x00004000)          /*!< WAIT Bit 2 */
#define FLCTL_BANK0_RDCTL_WAIT3                  ((uint32_t)0x00008000)          /*!< WAIT Bit 3 */
#define FLCTL_BANK0_RDCTL_WAIT_0                 ((uint32_t)0x00000000)          /*!< 0 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_1                 ((uint32_t)0x00001000)          /*!< 1 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_2                 ((uint32_t)0x00002000)          /*!< 2 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_3                 ((uint32_t)0x00003000)          /*!< 3 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_4                 ((uint32_t)0x00004000)          /*!< 4 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_5                 ((uint32_t)0x00005000)          /*!< 5 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_6                 ((uint32_t)0x00006000)          /*!< 6 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_7                 ((uint32_t)0x00007000)          /*!< 7 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_8                 ((uint32_t)0x00008000)          /*!< 8 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_9                 ((uint32_t)0x00009000)          /*!< 9 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_10                ((uint32_t)0x0000A000)          /*!< 10 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_11                ((uint32_t)0x0000B000)          /*!< 11 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_12                ((uint32_t)0x0000C000)          /*!< 12 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_13                ((uint32_t)0x0000D000)          /*!< 13 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_14                ((uint32_t)0x0000E000)          /*!< 14 wait states */
#define FLCTL_BANK0_RDCTL_WAIT_15                ((uint32_t)0x0000F000)          /*!< 15 wait states */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_OFS     (16)                            /*!< RD_MODE_STATUS Bit Offset */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_MASK    ((uint32_t)0x000F0000)          /*!< RD_MODE_STATUS Bit Mask */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS0        ((uint32_t)0x00010000)          /*!< RD_MODE_STATUS Bit 0 */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS1        ((uint32_t)0x00020000)          /*!< RD_MODE_STATUS Bit 1 */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS2        ((uint32_t)0x00040000)          /*!< RD_MODE_STATUS Bit 2 */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS3        ((uint32_t)0x00080000)          /*!< RD_MODE_STATUS Bit 3 */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_0       ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_1       ((uint32_t)0x00010000)          /*!< Read Margin 0 */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_2       ((uint32_t)0x00020000)          /*!< Read Margin 1 */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_3       ((uint32_t)0x00030000)          /*!< Program Verify */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_4       ((uint32_t)0x00040000)          /*!< Erase Verify */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_5       ((uint32_t)0x00050000)          /*!< Leakage Verify */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_9       ((uint32_t)0x00090000)          /*!< Read Margin 0B */
#define FLCTL_BANK0_RDCTL_RD_MODE_STATUS_10      ((uint32_t)0x000A0000)          /*!< Read Margin 1B */
#define FLCTL_BANK1_RDCTL_RD_MODE_OFS            ( 0)                            /*!< RD_MODE Bit Offset */
#define FLCTL_BANK1_RDCTL_RD_MODE_MASK           ((uint32_t)0x0000000F)          /*!< RD_MODE Bit Mask */
#define FLCTL_BANK1_RDCTL_RD_MODE0               ((uint32_t)0x00000001)          /*!< RD_MODE Bit 0 */
#define FLCTL_BANK1_RDCTL_RD_MODE1               ((uint32_t)0x00000002)          /*!< RD_MODE Bit 1 */
#define FLCTL_BANK1_RDCTL_RD_MODE2               ((uint32_t)0x00000004)          /*!< RD_MODE Bit 2 */
#define FLCTL_BANK1_RDCTL_RD_MODE3               ((uint32_t)0x00000008)          /*!< RD_MODE Bit 3 */
#define FLCTL_BANK1_RDCTL_RD_MODE_0              ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_BANK1_RDCTL_RD_MODE_1              ((uint32_t)0x00000001)          /*!< Read Margin 0 */
#define FLCTL_BANK1_RDCTL_RD_MODE_2              ((uint32_t)0x00000002)          /*!< Read Margin 1 */
#define FLCTL_BANK1_RDCTL_RD_MODE_3              ((uint32_t)0x00000003)          /*!< Program Verify */
#define FLCTL_BANK1_RDCTL_RD_MODE_4              ((uint32_t)0x00000004)          /*!< Erase Verify */
#define FLCTL_BANK1_RDCTL_RD_MODE_5              ((uint32_t)0x00000005)          /*!< Leakage Verify */
#define FLCTL_BANK1_RDCTL_RD_MODE_9              ((uint32_t)0x00000009)          /*!< Read Margin 0B */
#define FLCTL_BANK1_RDCTL_RD_MODE_10             ((uint32_t)0x0000000A)          /*!< Read Margin 1B */
#define FLCTL_BANK1_RDCTL_BUFI_OFS               ( 4)                            /*!< BUFI Bit Offset */
#define FLCTL_BANK1_RDCTL_BUFI                   ((uint32_t)0x00000010)          /*!< Enables read buffering feature for instruction fetches to this Bank */
#define FLCTL_BANK1_RDCTL_BUFD_OFS               ( 5)                            /*!< BUFD Bit Offset */
#define FLCTL_BANK1_RDCTL_BUFD                   ((uint32_t)0x00000020)          /*!< Enables read buffering feature for data reads to this Bank */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_OFS     (16)                            /*!< RD_MODE_STATUS Bit Offset */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_MASK    ((uint32_t)0x000F0000)          /*!< RD_MODE_STATUS Bit Mask */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS0        ((uint32_t)0x00010000)          /*!< RD_MODE_STATUS Bit 0 */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS1        ((uint32_t)0x00020000)          /*!< RD_MODE_STATUS Bit 1 */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS2        ((uint32_t)0x00040000)          /*!< RD_MODE_STATUS Bit 2 */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS3        ((uint32_t)0x00080000)          /*!< RD_MODE_STATUS Bit 3 */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_0       ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_1       ((uint32_t)0x00010000)          /*!< Read Margin 0 */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_2       ((uint32_t)0x00020000)          /*!< Read Margin 1 */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_3       ((uint32_t)0x00030000)          /*!< Program Verify */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_4       ((uint32_t)0x00040000)          /*!< Erase Verify */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_5       ((uint32_t)0x00050000)          /*!< Leakage Verify */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_9       ((uint32_t)0x00090000)          /*!< Read Margin 0B */
#define FLCTL_BANK1_RDCTL_RD_MODE_STATUS_10      ((uint32_t)0x000A0000)          /*!< Read Margin 1B */
#define FLCTL_BANK1_RDCTL_WAIT_OFS               (12)                            /*!< WAIT Bit Offset */
#define FLCTL_BANK1_RDCTL_WAIT_MASK              ((uint32_t)0x0000F000)          /*!< WAIT Bit Mask */
#define FLCTL_BANK1_RDCTL_WAIT0                  ((uint32_t)0x00001000)          /*!< WAIT Bit 0 */
#define FLCTL_BANK1_RDCTL_WAIT1                  ((uint32_t)0x00002000)          /*!< WAIT Bit 1 */
#define FLCTL_BANK1_RDCTL_WAIT2                  ((uint32_t)0x00004000)          /*!< WAIT Bit 2 */
#define FLCTL_BANK1_RDCTL_WAIT3                  ((uint32_t)0x00008000)          /*!< WAIT Bit 3 */
#define FLCTL_BANK1_RDCTL_WAIT_0                 ((uint32_t)0x00000000)          /*!< 0 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_1                 ((uint32_t)0x00001000)          /*!< 1 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_2                 ((uint32_t)0x00002000)          /*!< 2 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_3                 ((uint32_t)0x00003000)          /*!< 3 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_4                 ((uint32_t)0x00004000)          /*!< 4 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_5                 ((uint32_t)0x00005000)          /*!< 5 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_6                 ((uint32_t)0x00006000)          /*!< 6 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_7                 ((uint32_t)0x00007000)          /*!< 7 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_8                 ((uint32_t)0x00008000)          /*!< 8 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_9                 ((uint32_t)0x00009000)          /*!< 9 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_10                ((uint32_t)0x0000A000)          /*!< 10 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_11                ((uint32_t)0x0000B000)          /*!< 11 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_12                ((uint32_t)0x0000C000)          /*!< 12 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_13                ((uint32_t)0x0000D000)          /*!< 13 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_14                ((uint32_t)0x0000E000)          /*!< 14 wait states */
#define FLCTL_BANK1_RDCTL_WAIT_15                ((uint32_t)0x0000F000)          /*!< 15 wait states */
#define FLCTL_RDBRST_CTLSTAT_START_OFS           ( 0)                            /*!< START Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_START               ((uint32_t)0x00000001)          /*!< Start of burst/compare operation */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_OFS        ( 1)                            /*!< MEM_TYPE Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_MASK       ((uint32_t)0x00000006)          /*!< MEM_TYPE Bit Mask */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE0           ((uint32_t)0x00000002)          /*!< MEM_TYPE Bit 0 */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE1           ((uint32_t)0x00000004)          /*!< MEM_TYPE Bit 1 */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_0          ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_1          ((uint32_t)0x00000002)          /*!< Information Memory */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_2          ((uint32_t)0x00000004)          /*!< Reserved */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_3          ((uint32_t)0x00000006)          /*!< Engineering Memory */
#define FLCTL_RDBRST_CTLSTAT_STOP_FAIL_OFS       ( 3)                            /*!< STOP_FAIL Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_STOP_FAIL           ((uint32_t)0x00000008)          /*!< Terminate burst/compare operation */
#define FLCTL_RDBRST_CTLSTAT_DATA_CMP_OFS        ( 4)                            /*!< DATA_CMP Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_DATA_CMP            ((uint32_t)0x00000010)          /*!< Data pattern used for comparison against memory read data */
#define FLCTL_RDBRST_CTLSTAT_TEST_EN_OFS         ( 6)                            /*!< TEST_EN Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_TEST_EN             ((uint32_t)0x00000040)          /*!< Enable comparison against test data compare registers */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_OFS       (16)                            /*!< BRST_STAT Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_MASK      ((uint32_t)0x00030000)          /*!< BRST_STAT Bit Mask */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT0          ((uint32_t)0x00010000)          /*!< BRST_STAT Bit 0 */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT1          ((uint32_t)0x00020000)          /*!< BRST_STAT Bit 1 */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_0         ((uint32_t)0x00000000)          /*!< Idle */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_1         ((uint32_t)0x00010000)          /*!< Burst/Compare START bit written, but operation pending */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_2         ((uint32_t)0x00020000)          /*!< Burst/Compare in progress */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_3         ((uint32_t)0x00030000)          /*!< Burst complete (status of completed burst remains in this state unless  */
#define FLCTL_RDBRST_CTLSTAT_CMP_ERR_OFS         (18)                            /*!< CMP_ERR Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_CMP_ERR             ((uint32_t)0x00040000)          /*!< Burst/Compare Operation encountered atleast one data */
#define FLCTL_RDBRST_CTLSTAT_ADDR_ERR_OFS        (19)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_ADDR_ERR            ((uint32_t)0x00080000)          /*!< Burst/Compare Operation was terminated due to access to */
#define FLCTL_RDBRST_CTLSTAT_CLR_STAT_OFS        (23)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_CLR_STAT            ((uint32_t)0x00800000)          /*!< Clear status bits 19-16 of this register */
#define FLCTL_RDBRST_STARTADDR_START_ADDRESS_OFS ( 0)                            /*!< START_ADDRESS Bit Offset */
#define FLCTL_RDBRST_STARTADDR_START_ADDRESS_MASK ((uint32_t)0x001FFFFF)          /*!< START_ADDRESS Bit Mask */
#define FLCTL_RDBRST_LEN_BURST_LENGTH_OFS        ( 0)                            /*!< BURST_LENGTH Bit Offset */
#define FLCTL_RDBRST_LEN_BURST_LENGTH_MASK       ((uint32_t)0x001FFFFF)          /*!< BURST_LENGTH Bit Mask */
#define FLCTL_RDBRST_FAILADDR_FAIL_ADDRESS_OFS   ( 0)                            /*!< FAIL_ADDRESS Bit Offset */
#define FLCTL_RDBRST_FAILADDR_FAIL_ADDRESS_MASK  ((uint32_t)0x001FFFFF)          /*!< FAIL_ADDRESS Bit Mask */
#define FLCTL_RDBRST_FAILCNT_FAIL_COUNT_OFS      ( 0)                            /*!< FAIL_COUNT Bit Offset */
#define FLCTL_RDBRST_FAILCNT_FAIL_COUNT_MASK     ((uint32_t)0x0001FFFF)          /*!< FAIL_COUNT Bit Mask */
#define FLCTL_PRG_CTLSTAT_ENABLE_OFS             ( 0)                            /*!< ENABLE Bit Offset */
#define FLCTL_PRG_CTLSTAT_ENABLE                 ((uint32_t)0x00000001)          /*!< Master control for all word program operations */
#define FLCTL_PRG_CTLSTAT_MODE_OFS               ( 1)                            /*!< MODE Bit Offset */
#define FLCTL_PRG_CTLSTAT_MODE                   ((uint32_t)0x00000002)          /*!< Write mode */
#define FLCTL_PRG_CTLSTAT_VER_PRE_OFS            ( 2)                            /*!< VER_PRE Bit Offset */
#define FLCTL_PRG_CTLSTAT_VER_PRE                ((uint32_t)0x00000004)          /*!< Controls automatic pre program verify operations */
#define FLCTL_PRG_CTLSTAT_VER_PST_OFS            ( 3)                            /*!< VER_PST Bit Offset */
#define FLCTL_PRG_CTLSTAT_VER_PST                ((uint32_t)0x00000008)          /*!< Controls automatic post program verify operations */
#define FLCTL_PRG_CTLSTAT_STATUS_OFS             (16)                            /*!< STATUS Bit Offset */
#define FLCTL_PRG_CTLSTAT_STATUS_MASK            ((uint32_t)0x00030000)          /*!< STATUS Bit Mask */
#define FLCTL_PRG_CTLSTAT_STATUS0                ((uint32_t)0x00010000)          /*!< STATUS Bit 0 */
#define FLCTL_PRG_CTLSTAT_STATUS1                ((uint32_t)0x00020000)          /*!< STATUS Bit 1 */
#define FLCTL_PRG_CTLSTAT_STATUS_0               ((uint32_t)0x00000000)          /*!< Idle (no program operation currently active) */
#define FLCTL_PRG_CTLSTAT_STATUS_1               ((uint32_t)0x00010000)          /*!< Single word program operation triggered, but pending */
#define FLCTL_PRG_CTLSTAT_STATUS_2               ((uint32_t)0x00020000)          /*!< Single word program in progress */
#define FLCTL_PRG_CTLSTAT_STATUS_3               ((uint32_t)0x00030000)          /*!< Reserved (Idle) */
#define FLCTL_PRG_CTLSTAT_BNK_ACT_OFS            (18)                            /*!< BNK_ACT Bit Offset */
#define FLCTL_PRG_CTLSTAT_BNK_ACT                ((uint32_t)0x00040000)          /*!< Bank active */
#define FLCTL_PRGBRST_CTLSTAT_START_OFS          ( 0)                            /*!< START Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_START              ((uint32_t)0x00000001)          /*!< Trigger start of burst program operation */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_OFS           ( 1)                            /*!< TYPE Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_MASK          ((uint32_t)0x00000006)          /*!< TYPE Bit Mask */
#define FLCTL_PRGBRST_CTLSTAT_TYPE0              ((uint32_t)0x00000002)          /*!< TYPE Bit 0 */
#define FLCTL_PRGBRST_CTLSTAT_TYPE1              ((uint32_t)0x00000004)          /*!< TYPE Bit 1 */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_0             ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_1             ((uint32_t)0x00000002)          /*!< Information Memory */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_2             ((uint32_t)0x00000004)          /*!< Reserved */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_3             ((uint32_t)0x00000006)          /*!< Engineering Memory */
#define FLCTL_PRGBRST_CTLSTAT_LEN_OFS            ( 3)                            /*!< LEN Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_LEN_MASK           ((uint32_t)0x00000038)          /*!< LEN Bit Mask */
#define FLCTL_PRGBRST_CTLSTAT_LEN0               ((uint32_t)0x00000008)          /*!< LEN Bit 0 */
#define FLCTL_PRGBRST_CTLSTAT_LEN1               ((uint32_t)0x00000010)          /*!< LEN Bit 1 */
#define FLCTL_PRGBRST_CTLSTAT_LEN2               ((uint32_t)0x00000020)          /*!< LEN Bit 2 */
#define FLCTL_PRGBRST_CTLSTAT_LEN_0              ((uint32_t)0x00000000)          /*!< No burst operation */
#define FLCTL_PRGBRST_CTLSTAT_LEN_1              ((uint32_t)0x00000008)          /*!< 1 word burst of 128 bits, starting with address in the  */
#define FLCTL_PRGBRST_CTLSTAT_LEN_2              ((uint32_t)0x00000010)          /*!< 2*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
#define FLCTL_PRGBRST_CTLSTAT_LEN_3              ((uint32_t)0x00000018)          /*!< 3*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
#define FLCTL_PRGBRST_CTLSTAT_LEN_4              ((uint32_t)0x00000020)          /*!< 4*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PRE_OFS       ( 6)                            /*!< AUTO_PRE Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PRE           ((uint32_t)0x00000040)          /*!< Auto-Verify operation before the Burst Program */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PST_OFS       ( 7)                            /*!< AUTO_PST Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PST           ((uint32_t)0x00000080)          /*!< Auto-Verify operation after the Burst Program */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_OFS   (16)                            /*!< BURST_STATUS Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK  ((uint32_t)0x00070000)          /*!< BURST_STATUS Bit Mask */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS0      ((uint32_t)0x00010000)          /*!< BURST_STATUS Bit 0 */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS1      ((uint32_t)0x00020000)          /*!< BURST_STATUS Bit 1 */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS2      ((uint32_t)0x00040000)          /*!< BURST_STATUS Bit 2 */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_0     ((uint32_t)0x00000000)          /*!< Idle (Burst not active) */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_1     ((uint32_t)0x00010000)          /*!< Burst program started but pending */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_2     ((uint32_t)0x00020000)          /*!< Burst active, with 1st 128 bit word being written into Flash */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_3     ((uint32_t)0x00030000)          /*!< Burst active, with 2nd 128 bit word being written into Flash */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_4     ((uint32_t)0x00040000)          /*!< Burst active, with 3rd 128 bit word being written into Flash */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_5     ((uint32_t)0x00050000)          /*!< Burst active, with 4th 128 bit word being written into Flash */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_6     ((uint32_t)0x00060000)          /*!< Reserved (Idle) */
#define FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_7     ((uint32_t)0x00070000)          /*!< Burst Complete (status of completed burst remains in this state unless  */
#define FLCTL_PRGBRST_CTLSTAT_PRE_ERR_OFS        (19)                            /*!< PRE_ERR Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_PRE_ERR            ((uint32_t)0x00080000)          /*!< Burst Operation encountered preprogram auto-verify errors */
#define FLCTL_PRGBRST_CTLSTAT_PST_ERR_OFS        (20)                            /*!< PST_ERR Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_PST_ERR            ((uint32_t)0x00100000)          /*!< Burst Operation encountered postprogram auto-verify errors */
#define FLCTL_PRGBRST_CTLSTAT_ADDR_ERR_OFS       (21)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_ADDR_ERR           ((uint32_t)0x00200000)          /*!< Burst Operation was terminated due to attempted program of reserved memory */
#define FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS       (23)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_CLR_STAT           ((uint32_t)0x00800000)          /*!< Clear status bits 21-16 of this register */
#define FLCTL_PRGBRST_STARTADDR_START_ADDRESS_OFS ( 0)                            /*!< START_ADDRESS Bit Offset */
#define FLCTL_PRGBRST_STARTADDR_START_ADDRESS_MASK ((uint32_t)0x003FFFFF)          /*!< START_ADDRESS Bit Mask */
#define FLCTL_ERASE_CTLSTAT_START_OFS            ( 0)                            /*!< START Bit Offset */
#define FLCTL_ERASE_CTLSTAT_START                ((uint32_t)0x00000001)          /*!< Start of Erase operation */
#define FLCTL_ERASE_CTLSTAT_MODE_OFS             ( 1)                            /*!< MODE Bit Offset */
#define FLCTL_ERASE_CTLSTAT_MODE                 ((uint32_t)0x00000002)          /*!< Erase mode selected by application */
#define FLCTL_ERASE_CTLSTAT_TYPE_OFS             ( 2)                            /*!< TYPE Bit Offset */
#define FLCTL_ERASE_CTLSTAT_TYPE_MASK            ((uint32_t)0x0000000C)          /*!< TYPE Bit Mask */
#define FLCTL_ERASE_CTLSTAT_TYPE0                ((uint32_t)0x00000004)          /*!< TYPE Bit 0 */
#define FLCTL_ERASE_CTLSTAT_TYPE1                ((uint32_t)0x00000008)          /*!< TYPE Bit 1 */
#define FLCTL_ERASE_CTLSTAT_TYPE_0               ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_ERASE_CTLSTAT_TYPE_1               ((uint32_t)0x00000004)          /*!< Information Memory */
#define FLCTL_ERASE_CTLSTAT_TYPE_2               ((uint32_t)0x00000008)          /*!< Reserved */
#define FLCTL_ERASE_CTLSTAT_TYPE_3               ((uint32_t)0x0000000C)          /*!< Engineering Memory */
#define FLCTL_ERASE_CTLSTAT_STATUS_OFS           (16)                            /*!< STATUS Bit Offset */
#define FLCTL_ERASE_CTLSTAT_STATUS_MASK          ((uint32_t)0x00030000)          /*!< STATUS Bit Mask */
#define FLCTL_ERASE_CTLSTAT_STATUS0              ((uint32_t)0x00010000)          /*!< STATUS Bit 0 */
#define FLCTL_ERASE_CTLSTAT_STATUS1              ((uint32_t)0x00020000)          /*!< STATUS Bit 1 */
#define FLCTL_ERASE_CTLSTAT_STATUS_0             ((uint32_t)0x00000000)          /*!< Idle (no program operation currently active) */
#define FLCTL_ERASE_CTLSTAT_STATUS_1             ((uint32_t)0x00010000)          /*!< Erase operation triggered to START but pending */
#define FLCTL_ERASE_CTLSTAT_STATUS_2             ((uint32_t)0x00020000)          /*!< Erase operation in progress */
#define FLCTL_ERASE_CTLSTAT_STATUS_3             ((uint32_t)0x00030000)          /*!< Erase operation completed (status of completed erase remains in this state  */
#define FLCTL_ERASE_CTLSTAT_ADDR_ERR_OFS         (18)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_ERASE_CTLSTAT_ADDR_ERR             ((uint32_t)0x00040000)          /*!< Erase Operation was terminated due to attempted erase of reserved memory  */
#define FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS         (19)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_ERASE_CTLSTAT_CLR_STAT             ((uint32_t)0x00080000)          /*!< Clear status bits 18-16 of this register */
#define FLCTL_ERASE_SECTADDR_SECT_ADDRESS_OFS    ( 0)                            /*!< SECT_ADDRESS Bit Offset */
#define FLCTL_ERASE_SECTADDR_SECT_ADDRESS_MASK   ((uint32_t)0x003FFFFF)          /*!< SECT_ADDRESS Bit Mask */
#define FLCTL_BANK0_INFO_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK0_INFO_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
#define FLCTL_BANK0_INFO_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK0_INFO_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT2_OFS        ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT2            ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT3_OFS        ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT3            ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT4_OFS        ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT4            ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT5_OFS        ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT5            ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT6_OFS        ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT6            ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT7_OFS        ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT7            ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT8_OFS        ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT8            ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT9_OFS        ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT9            ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT10_OFS       (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT10           ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT11_OFS       (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT11           ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT12_OFS       (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT12           ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT13_OFS       (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT13           ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT14_OFS       (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT14           ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT15_OFS       (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT15           ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT16_OFS       (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT16           ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT17_OFS       (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT17           ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT18_OFS       (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT18           ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT19_OFS       (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT19           ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT20_OFS       (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT20           ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT21_OFS       (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT21           ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT22_OFS       (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT22           ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT23_OFS       (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT23           ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT24_OFS       (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT24           ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT25_OFS       (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT25           ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT26_OFS       (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT26           ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT27_OFS       (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT27           ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT28_OFS       (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT28           ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT29_OFS       (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT29           ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT30_OFS       (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT30           ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase */
#define FLCTL_BANK0_MAIN_WEPROT_PROT31_OFS       (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT31           ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase */
#define FLCTL_BANK1_INFO_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK1_INFO_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase operations */
#define FLCTL_BANK1_INFO_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK1_INFO_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT2_OFS        ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT2            ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT3_OFS        ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT3            ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT4_OFS        ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT4            ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT5_OFS        ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT5            ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT6_OFS        ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT6            ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT7_OFS        ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT7            ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT8_OFS        ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT8            ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT9_OFS        ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT9            ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT10_OFS       (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT10           ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT11_OFS       (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT11           ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT12_OFS       (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT12           ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT13_OFS       (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT13           ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT14_OFS       (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT14           ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT15_OFS       (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT15           ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT16_OFS       (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT16           ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT17_OFS       (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT17           ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT18_OFS       (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT18           ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT19_OFS       (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT19           ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT20_OFS       (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT20           ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT21_OFS       (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT21           ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT22_OFS       (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT22           ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT23_OFS       (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT23           ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT24_OFS       (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT24           ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT25_OFS       (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT25           ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT26_OFS       (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT26           ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT27_OFS       (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT27           ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT28_OFS       (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT28           ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT29_OFS       (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT29           ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT30_OFS       (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT30           ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase operations */
#define FLCTL_BANK1_MAIN_WEPROT_PROT31_OFS       (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT31           ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase operations */
#define FLCTL_BMRK_CTLSTAT_I_BMRK_OFS            ( 0)                            /*!< I_BMRK Bit Offset */
#define FLCTL_BMRK_CTLSTAT_I_BMRK                ((uint32_t)0x00000001)          
#define FLCTL_BMRK_CTLSTAT_D_BMRK_OFS            ( 1)                            /*!< D_BMRK Bit Offset */
#define FLCTL_BMRK_CTLSTAT_D_BMRK                ((uint32_t)0x00000002)          
#define FLCTL_BMRK_CTLSTAT_CMP_EN_OFS            ( 2)                            /*!< CMP_EN Bit Offset */
#define FLCTL_BMRK_CTLSTAT_CMP_EN                ((uint32_t)0x00000004)          
#define FLCTL_BMRK_CTLSTAT_CMP_SEL_OFS           ( 3)                            /*!< CMP_SEL Bit Offset */
#define FLCTL_BMRK_CTLSTAT_CMP_SEL               ((uint32_t)0x00000008)          
#define FLCTL_IFG_RDBRST_OFS                     ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_IFG_RDBRST                         ((uint32_t)0x00000001)          
#define FLCTL_IFG_AVPRE_OFS                      ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_IFG_AVPRE                          ((uint32_t)0x00000002)          
#define FLCTL_IFG_AVPST_OFS                      ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_IFG_AVPST                          ((uint32_t)0x00000004)          
#define FLCTL_IFG_PRG_OFS                        ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_IFG_PRG                            ((uint32_t)0x00000008)          
#define FLCTL_IFG_PRGB_OFS                       ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_IFG_PRGB                           ((uint32_t)0x00000010)          
#define FLCTL_IFG_ERASE_OFS                      ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_IFG_ERASE                          ((uint32_t)0x00000020)          
#define FLCTL_IFG_BMRK_OFS                       ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_IFG_BMRK                           ((uint32_t)0x00000100)          
#define FLCTL_IFG_PRG_ERR_OFS                    ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_IFG_PRG_ERR                        ((uint32_t)0x00000200)          
#define FLCTL_IE_RDBRST_OFS                      ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_IE_RDBRST                          ((uint32_t)0x00000001)          
#define FLCTL_IE_AVPRE_OFS                       ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_IE_AVPRE                           ((uint32_t)0x00000002)          
#define FLCTL_IE_AVPST_OFS                       ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_IE_AVPST                           ((uint32_t)0x00000004)          
#define FLCTL_IE_PRG_OFS                         ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_IE_PRG                             ((uint32_t)0x00000008)          
#define FLCTL_IE_PRGB_OFS                        ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_IE_PRGB                            ((uint32_t)0x00000010)          
#define FLCTL_IE_ERASE_OFS                       ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_IE_ERASE                           ((uint32_t)0x00000020)          
#define FLCTL_IE_BMRK_OFS                        ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_IE_BMRK                            ((uint32_t)0x00000100)          
#define FLCTL_IE_PRG_ERR_OFS                     ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_IE_PRG_ERR                         ((uint32_t)0x00000200)          
#define FLCTL_CLRIFG_RDBRST_OFS                  ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_CLRIFG_RDBRST                      ((uint32_t)0x00000001)          
#define FLCTL_CLRIFG_AVPRE_OFS                   ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_CLRIFG_AVPRE                       ((uint32_t)0x00000002)          
#define FLCTL_CLRIFG_AVPST_OFS                   ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_CLRIFG_AVPST                       ((uint32_t)0x00000004)          
#define FLCTL_CLRIFG_PRG_OFS                     ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_CLRIFG_PRG                         ((uint32_t)0x00000008)          
#define FLCTL_CLRIFG_PRGB_OFS                    ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_CLRIFG_PRGB                        ((uint32_t)0x00000010)          
#define FLCTL_CLRIFG_ERASE_OFS                   ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_CLRIFG_ERASE                       ((uint32_t)0x00000020)          
#define FLCTL_CLRIFG_BMRK_OFS                    ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_CLRIFG_BMRK                        ((uint32_t)0x00000100)          
#define FLCTL_CLRIFG_PRG_ERR_OFS                 ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_CLRIFG_PRG_ERR                     ((uint32_t)0x00000200)          
#define FLCTL_SETIFG_RDBRST_OFS                  ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_SETIFG_RDBRST                      ((uint32_t)0x00000001)          
#define FLCTL_SETIFG_AVPRE_OFS                   ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_SETIFG_AVPRE                       ((uint32_t)0x00000002)          
#define FLCTL_SETIFG_AVPST_OFS                   ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_SETIFG_AVPST                       ((uint32_t)0x00000004)          
#define FLCTL_SETIFG_PRG_OFS                     ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_SETIFG_PRG                         ((uint32_t)0x00000008)          
#define FLCTL_SETIFG_PRGB_OFS                    ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_SETIFG_PRGB                        ((uint32_t)0x00000010)          
#define FLCTL_SETIFG_ERASE_OFS                   ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_SETIFG_ERASE                       ((uint32_t)0x00000020)          
#define FLCTL_SETIFG_BMRK_OFS                    ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_SETIFG_BMRK                        ((uint32_t)0x00000100)          
#define FLCTL_SETIFG_PRG_ERR_OFS                 ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_SETIFG_PRG_ERR                     ((uint32_t)0x00000200)          
#define FLCTL_READ_TIMCTL_SETUP_OFS              ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_READ_TIMCTL_SETUP_MASK             ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_READ_TIMCTL_IREF_BOOST1_OFS        (12)                            /*!< IREF_BOOST1 Bit Offset */
#define FLCTL_READ_TIMCTL_IREF_BOOST1_MASK       ((uint32_t)0x0000F000)          /*!< IREF_BOOST1 Bit Mask */
#define FLCTL_READ_TIMCTL_SETUP_LONG_OFS         (16)                            /*!< SETUP_LONG Bit Offset */
#define FLCTL_READ_TIMCTL_SETUP_LONG_MASK        ((uint32_t)0x00FF0000)          /*!< SETUP_LONG Bit Mask */
#define FLCTL_READMARGIN_TIMCTL_SETUP_OFS        ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_READMARGIN_TIMCTL_SETUP_MASK       ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_PRGVER_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_PRGVER_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_PRGVER_TIMCTL_ACTIVE_OFS           ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_PRGVER_TIMCTL_ACTIVE_MASK          ((uint32_t)0x00000F00)          /*!< ACTIVE Bit Mask */
#define FLCTL_PRGVER_TIMCTL_HOLD_OFS             (12)                            /*!< HOLD Bit Offset */
#define FLCTL_PRGVER_TIMCTL_HOLD_MASK            ((uint32_t)0x0000F000)          /*!< HOLD Bit Mask */
#define FLCTL_ERSVER_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_ERSVER_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_LKGVER_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_LKGVER_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_PROGRAM_TIMCTL_SETUP_OFS           ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_PROGRAM_TIMCTL_SETUP_MASK          ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_PROGRAM_TIMCTL_ACTIVE_OFS          ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_PROGRAM_TIMCTL_ACTIVE_MASK         ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
#define FLCTL_PROGRAM_TIMCTL_HOLD_OFS            (28)                            /*!< HOLD Bit Offset */
#define FLCTL_PROGRAM_TIMCTL_HOLD_MASK           ((uint32_t)0xF0000000)          /*!< HOLD Bit Mask */
#define FLCTL_ERASE_TIMCTL_SETUP_OFS             ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_ERASE_TIMCTL_SETUP_MASK            ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_ERASE_TIMCTL_ACTIVE_OFS            ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_ERASE_TIMCTL_ACTIVE_MASK           ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
#define FLCTL_ERASE_TIMCTL_HOLD_OFS              (28)                            /*!< HOLD Bit Offset */
#define FLCTL_ERASE_TIMCTL_HOLD_MASK             ((uint32_t)0xF0000000)          /*!< HOLD Bit Mask */
#define FLCTL_MASSERASE_TIMCTL_BOOST_ACTIVE_OFS  ( 0)                            /*!< BOOST_ACTIVE Bit Offset */
#define FLCTL_MASSERASE_TIMCTL_BOOST_ACTIVE_MASK ((uint32_t)0x000000FF)          /*!< BOOST_ACTIVE Bit Mask */
#define FLCTL_MASSERASE_TIMCTL_BOOST_HOLD_OFS    ( 8)                            /*!< BOOST_HOLD Bit Offset */
#define FLCTL_MASSERASE_TIMCTL_BOOST_HOLD_MASK   ((uint32_t)0x0000FF00)          /*!< BOOST_HOLD Bit Mask */
#define FLCTL_BURSTPRG_TIMCTL_ACTIVE_OFS         ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_BURSTPRG_TIMCTL_ACTIVE_MASK        ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
#define MPU_RASR_SIZE__32B                       ((uint32_t)0x00000008)          /*!< 32B */
#define MPU_RASR_SIZE__64B                       ((uint32_t)0x0000000A)          /*!< 64B */
#define MPU_RASR_SIZE__128B                      ((uint32_t)0x0000000C)          /*!< 128B */
#define MPU_RASR_SIZE__256B                      ((uint32_t)0x0000000E)          /*!< 256B */
#define MPU_RASR_SIZE__512B                      ((uint32_t)0x00000010)          /*!< 512B */
#define MPU_RASR_SIZE__1K                        ((uint32_t)0x00000012)          /*!< 1KB */
#define MPU_RASR_SIZE__2K                        ((uint32_t)0x00000014)          /*!< 2KB */
#define MPU_RASR_SIZE__4K                        ((uint32_t)0x00000016)          /*!< 4KB */
#define MPU_RASR_SIZE__8K                        ((uint32_t)0x00000018)          /*!< 8KB */
#define MPU_RASR_SIZE__16K                       ((uint32_t)0x0000001A)          /*!< 16KB */
#define MPU_RASR_SIZE__32K                       ((uint32_t)0x0000001C)          /*!< 32KB */
#define MPU_RASR_SIZE__64K                       ((uint32_t)0x0000001E)          /*!< 64KB */
#define MPU_RASR_SIZE__128K                      ((uint32_t)0x00000020)          /*!< 128KB */
#define MPU_RASR_SIZE__256K                      ((uint32_t)0x00000022)          /*!< 256KB */
#define MPU_RASR_SIZE__512K                      ((uint32_t)0x00000024)          /*!< 512KB */
#define MPU_RASR_SIZE__1M                        ((uint32_t)0x00000026)          /*!< 1MB */
#define MPU_RASR_SIZE__2M                        ((uint32_t)0x00000028)          /*!< 2MB */
#define MPU_RASR_SIZE__4M                        ((uint32_t)0x0000002A)          /*!< 4MB */
#define MPU_RASR_SIZE__8M                        ((uint32_t)0x0000002C)          /*!< 8MB */
#define MPU_RASR_SIZE__16M                       ((uint32_t)0x0000002E)          /*!< 16MB */
#define MPU_RASR_SIZE__32M                       ((uint32_t)0x00000030)          /*!< 32MB */
#define MPU_RASR_SIZE__64M                       ((uint32_t)0x00000032)          /*!< 64MB */
#define MPU_RASR_SIZE__128M                      ((uint32_t)0x00000034)          /*!< 128MB */
#define MPU_RASR_SIZE__256M                      ((uint32_t)0x00000036)          /*!< 256MB */
#define MPU_RASR_SIZE__512M                      ((uint32_t)0x00000038)          /*!< 512MB */
#define MPU_RASR_SIZE__1G                        ((uint32_t)0x0000003A)          /*!< 1GB */
#define MPU_RASR_SIZE__2G                        ((uint32_t)0x0000003C)          /*!< 2GB */
#define MPU_RASR_SIZE__4G                        ((uint32_t)0x0000003E)          /*!< 4GB */
#define MPU_RASR_AP_PRV_NO_USR_NO                ((uint32_t)0x00000000)          /*!< Privileged permissions: No access. User permissions: No access. */
#define MPU_RASR_AP_PRV_RW_USR_NO                ((uint32_t)0x01000000)          /*!< Privileged permissions: Read-write. User permissions: No access. */
#define MPU_RASR_AP_PRV_RW_USR_RO                ((uint32_t)0x02000000)          /*!< Privileged permissions: Read-write. User permissions: Read-only. */
#define MPU_RASR_AP_PRV_RW_USR_RW                ((uint32_t)0x03000000)          /*!< Privileged permissions: Read-write. User permissions: Read-write. */
#define MPU_RASR_AP_PRV_RO_USR_NO                ((uint32_t)0x05000000)          /*!< Privileged permissions: Read-only. User permissions: No access. */
#define MPU_RASR_AP_PRV_RO_USR_RO                ((uint32_t)0x06000000)          /*!< Privileged permissions: Read-only. User permissions: Read-only. */
#define MPU_RASR_AP_EXEC                         ((uint32_t)0x00000000)          /*!< Instruction access enabled */
#define MPU_RASR_AP_NOEXEC                       ((uint32_t)0x10000000)          /*!< Instruction access disabled */
#define NVIC_IPR0_PRI_0_OFS                      ( 0)                            /*!< PRI_0 Offset */
#define NVIC_IPR0_PRI_0_M                        ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR0_PRI_1_OFS                      ( 8)                            /*!< PRI_1 Offset */
#define NVIC_IPR0_PRI_1_M                        ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR0_PRI_2_OFS                      (16)                            /*!< PRI_2 Offset */
#define NVIC_IPR0_PRI_2_M                        ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR0_PRI_3_OFS                      (24)                            /*!< PRI_3 Offset */
#define NVIC_IPR0_PRI_3_M                        ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR1_PRI_4_OFS                      ( 0)                            /*!< PRI_4 Offset */
#define NVIC_IPR1_PRI_4_M                        ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR1_PRI_5_OFS                      ( 8)                            /*!< PRI_5 Offset */
#define NVIC_IPR1_PRI_5_M                        ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR1_PRI_6_OFS                      (16)                            /*!< PRI_6 Offset */
#define NVIC_IPR1_PRI_6_M                        ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR1_PRI_7_OFS                      (24)                            /*!< PRI_7 Offset */
#define NVIC_IPR1_PRI_7_M                        ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR2_PRI_8_OFS                      ( 0)                            /*!< PRI_8 Offset */
#define NVIC_IPR2_PRI_8_M                        ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR2_PRI_9_OFS                      ( 8)                            /*!< PRI_9 Offset */
#define NVIC_IPR2_PRI_9_M                        ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR2_PRI_10_OFS                     (16)                            /*!< PRI_10 Offset */
#define NVIC_IPR2_PRI_10_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR2_PRI_11_OFS                     (24)                            /*!< PRI_11 Offset */
#define NVIC_IPR2_PRI_11_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR3_PRI_12_OFS                     ( 0)                            /*!< PRI_12 Offset */
#define NVIC_IPR3_PRI_12_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR3_PRI_13_OFS                     ( 8)                            /*!< PRI_13 Offset */
#define NVIC_IPR3_PRI_13_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR3_PRI_14_OFS                     (16)                            /*!< PRI_14 Offset */
#define NVIC_IPR3_PRI_14_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR3_PRI_15_OFS                     (24)                            /*!< PRI_15 Offset */
#define NVIC_IPR3_PRI_15_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR4_PRI_16_OFS                     ( 0)                            /*!< PRI_16 Offset */
#define NVIC_IPR4_PRI_16_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR4_PRI_17_OFS                     ( 8)                            /*!< PRI_17 Offset */
#define NVIC_IPR4_PRI_17_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR4_PRI_18_OFS                     (16)                            /*!< PRI_18 Offset */
#define NVIC_IPR4_PRI_18_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR4_PRI_19_OFS                     (24)                            /*!< PRI_19 Offset */
#define NVIC_IPR4_PRI_19_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR5_PRI_20_OFS                     ( 0)                            /*!< PRI_20 Offset */
#define NVIC_IPR5_PRI_20_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR5_PRI_21_OFS                     ( 8)                            /*!< PRI_21 Offset */
#define NVIC_IPR5_PRI_21_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR5_PRI_22_OFS                     (16)                            /*!< PRI_22 Offset */
#define NVIC_IPR5_PRI_22_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR5_PRI_23_OFS                     (24)                            /*!< PRI_23 Offset */
#define NVIC_IPR5_PRI_23_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR6_PRI_24_OFS                     ( 0)                            /*!< PRI_24 Offset */
#define NVIC_IPR6_PRI_24_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR6_PRI_25_OFS                     ( 8)                            /*!< PRI_25 Offset */
#define NVIC_IPR6_PRI_25_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR6_PRI_26_OFS                     (16)                            /*!< PRI_26 Offset */
#define NVIC_IPR6_PRI_26_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR6_PRI_27_OFS                     (24)                            /*!< PRI_27 Offset */
#define NVIC_IPR6_PRI_27_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR7_PRI_28_OFS                     ( 0)                            /*!< PRI_28 Offset */
#define NVIC_IPR7_PRI_28_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR7_PRI_29_OFS                     ( 8)                            /*!< PRI_29 Offset */
#define NVIC_IPR7_PRI_29_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR7_PRI_30_OFS                     (16)                            /*!< PRI_30 Offset */
#define NVIC_IPR7_PRI_30_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR7_PRI_31_OFS                     (24)                            /*!< PRI_31 Offset */
#define NVIC_IPR7_PRI_31_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR8_PRI_32_OFS                     ( 0)                            /*!< PRI_32 Offset */
#define NVIC_IPR8_PRI_32_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR8_PRI_33_OFS                     ( 8)                            /*!< PRI_33 Offset */
#define NVIC_IPR8_PRI_33_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR8_PRI_34_OFS                     (16)                            /*!< PRI_34 Offset */
#define NVIC_IPR8_PRI_34_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR8_PRI_35_OFS                     (24)                            /*!< PRI_35 Offset */
#define NVIC_IPR8_PRI_35_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR9_PRI_36_OFS                     ( 0)                            /*!< PRI_36 Offset */
#define NVIC_IPR9_PRI_36_M                       ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR9_PRI_37_OFS                     ( 8)                            /*!< PRI_37 Offset */
#define NVIC_IPR9_PRI_37_M                       ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR9_PRI_38_OFS                     (16)                            /*!< PRI_38 Offset */
#define NVIC_IPR9_PRI_38_M                       ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR9_PRI_39_OFS                     (24)                            /*!< PRI_39 Offset */
#define NVIC_IPR9_PRI_39_M                       ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR10_PRI_40_OFS                    ( 0)                            /*!< PRI_40 Offset */
#define NVIC_IPR10_PRI_40_M                      ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR10_PRI_41_OFS                    ( 8)                            /*!< PRI_41 Offset */
#define NVIC_IPR10_PRI_41_M                      ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR10_PRI_42_OFS                    (16)                            /*!< PRI_42 Offset */
#define NVIC_IPR10_PRI_42_M                      ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR10_PRI_43_OFS                    (24)                            /*!< PRI_43 Offset */
#define NVIC_IPR10_PRI_43_M                      ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR11_PRI_44_OFS                    ( 0)                            /*!< PRI_44 Offset */
#define NVIC_IPR11_PRI_44_M                      ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR11_PRI_45_OFS                    ( 8)                            /*!< PRI_45 Offset */
#define NVIC_IPR11_PRI_45_M                      ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR11_PRI_46_OFS                    (16)                            /*!< PRI_46 Offset */
#define NVIC_IPR11_PRI_46_M                      ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR11_PRI_47_OFS                    (24)                            /*!< PRI_47 Offset */
#define NVIC_IPR11_PRI_47_M                      ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR12_PRI_48_OFS                    ( 0)                            /*!< PRI_48 Offset */
#define NVIC_IPR12_PRI_48_M                      ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR12_PRI_49_OFS                    ( 8)                            /*!< PRI_49 Offset */
#define NVIC_IPR12_PRI_49_M                      ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR12_PRI_50_OFS                    (16)                            /*!< PRI_50 Offset */
#define NVIC_IPR12_PRI_50_M                      ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR12_PRI_51_OFS                    (24)                            /*!< PRI_51 Offset */
#define NVIC_IPR12_PRI_51_M                      ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR13_PRI_52_OFS                    ( 0)                            /*!< PRI_52 Offset */
#define NVIC_IPR13_PRI_52_M                      ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR13_PRI_53_OFS                    ( 8)                            /*!< PRI_53 Offset */
#define NVIC_IPR13_PRI_53_M                      ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR13_PRI_54_OFS                    (16)                            /*!< PRI_54 Offset */
#define NVIC_IPR13_PRI_54_M                      ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR13_PRI_55_OFS                    (24)                            /*!< PRI_55 Offset */
#define NVIC_IPR13_PRI_55_M                      ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR14_PRI_56_OFS                    ( 0)                            /*!< PRI_56 Offset */
#define NVIC_IPR14_PRI_56_M                      ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR14_PRI_57_OFS                    ( 8)                            /*!< PRI_57 Offset */
#define NVIC_IPR14_PRI_57_M                      ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR14_PRI_58_OFS                    (16)                            /*!< PRI_58 Offset */
#define NVIC_IPR14_PRI_58_M                      ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR14_PRI_59_OFS                    (24)                            /*!< PRI_59 Offset */
#define NVIC_IPR14_PRI_59_M                      ((uint32_t)0xff000000)          /*  */
#define NVIC_IPR15_PRI_60_OFS                    ( 0)                            /*!< PRI_60 Offset */
#define NVIC_IPR15_PRI_60_M                      ((uint32_t)0x000000ff)          /*  */
#define NVIC_IPR15_PRI_61_OFS                    ( 8)                            /*!< PRI_61 Offset */
#define NVIC_IPR15_PRI_61_M                      ((uint32_t)0x0000ff00)          /*  */
#define NVIC_IPR15_PRI_62_OFS                    (16)                            /*!< PRI_62 Offset */
#define NVIC_IPR15_PRI_62_M                      ((uint32_t)0x00ff0000)          /*  */
#define NVIC_IPR15_PRI_63_OFS                    (24)                            /*!< PRI_63 Offset */
#define NVIC_IPR15_PRI_63_M                      ((uint32_t)0xff000000)          /*  */
#define PCM_CTL0_AMR_OFS                         ( 0)                            /*!< AMR Bit Offset */
#define PCM_CTL0_AMR_MASK                        ((uint32_t)0x0000000F)          /*!< AMR Bit Mask */
#define PCM_CTL0_AMR0                            ((uint32_t)0x00000001)          /*!< AMR Bit 0 */
#define PCM_CTL0_AMR1                            ((uint32_t)0x00000002)          /*!< AMR Bit 1 */
#define PCM_CTL0_AMR2                            ((uint32_t)0x00000004)          /*!< AMR Bit 2 */
#define PCM_CTL0_AMR3                            ((uint32_t)0x00000008)          /*!< AMR Bit 3 */
#define PCM_CTL0_AMR_0                           ((uint32_t)0x00000000)          /*!< LDO based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_AMR_1                           ((uint32_t)0x00000001)          /*!< LDO based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_AMR_4                           ((uint32_t)0x00000004)          /*!< DC-DC based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_AMR_5                           ((uint32_t)0x00000005)          /*!< DC-DC based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_AMR_8                           ((uint32_t)0x00000008)          /*!< Low-Frequency Active Mode at Core voltage setting 0. */
#define PCM_CTL0_AMR_9                           ((uint32_t)0x00000009)          /*!< Low-Frequency Active Mode at Core voltage setting 1. */
#define PCM_CTL0_AMR__AM_LDO_VCORE0              ((uint32_t)0x00000000)          /*!< LDO based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_AMR__AM_LDO_VCORE1              ((uint32_t)0x00000001)          /*!< LDO based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_AMR__AM_DCDC_VCORE0             ((uint32_t)0x00000004)          /*!< DC-DC based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_AMR__AM_DCDC_VCORE1             ((uint32_t)0x00000005)          /*!< DC-DC based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_AMR__AM_LF_VCORE0               ((uint32_t)0x00000008)          /*!< Low-Frequency Active Mode at Core voltage setting 0. */
#define PCM_CTL0_AMR__AM_LF_VCORE1               ((uint32_t)0x00000009)          /*!< Low-Frequency Active Mode at Core voltage setting 1. */
#define PCM_CTL0_LPMR_OFS                        ( 4)                            /*!< LPMR Bit Offset */
#define PCM_CTL0_LPMR_MASK                       ((uint32_t)0x000000F0)          /*!< LPMR Bit Mask */
#define PCM_CTL0_LPMR0                           ((uint32_t)0x00000010)          /*!< LPMR Bit 0 */
#define PCM_CTL0_LPMR1                           ((uint32_t)0x00000020)          /*!< LPMR Bit 1 */
#define PCM_CTL0_LPMR2                           ((uint32_t)0x00000040)          /*!< LPMR Bit 2 */
#define PCM_CTL0_LPMR3                           ((uint32_t)0x00000080)          /*!< LPMR Bit 3 */
#define PCM_CTL0_LPMR_0                          ((uint32_t)0x00000000)          /*!< LPM3. Core voltage setting is similar to the mode from which LPM3 is  */
#define PCM_CTL0_LPMR_10                         ((uint32_t)0x000000A0)          /*!< LPM3.5. Core voltage setting 0. */
#define PCM_CTL0_LPMR_12                         ((uint32_t)0x000000C0)          /*!< LPM4.5 */
#define PCM_CTL0_LPMR__LPM3                      ((uint32_t)0x00000000)          /*!< LPM3. Core voltage setting is similar to the mode from which LPM3 is  */
#define PCM_CTL0_LPMR__LPM35                     ((uint32_t)0x000000A0)          /*!< LPM3.5. Core voltage setting 0. */
#define PCM_CTL0_LPMR__LPM45                     ((uint32_t)0x000000C0)          /*!< LPM4.5 */
#define PCM_CTL0_CPM_OFS                         ( 8)                            /*!< CPM Bit Offset */
#define PCM_CTL0_CPM_MASK                        ((uint32_t)0x00003F00)          /*!< CPM Bit Mask */
#define PCM_CTL0_CPM0                            ((uint32_t)0x00000100)          /*!< CPM Bit 0 */
#define PCM_CTL0_CPM1                            ((uint32_t)0x00000200)          /*!< CPM Bit 1 */
#define PCM_CTL0_CPM2                            ((uint32_t)0x00000400)          /*!< CPM Bit 2 */
#define PCM_CTL0_CPM3                            ((uint32_t)0x00000800)          /*!< CPM Bit 3 */
#define PCM_CTL0_CPM4                            ((uint32_t)0x00001000)          /*!< CPM Bit 4 */
#define PCM_CTL0_CPM5                            ((uint32_t)0x00002000)          /*!< CPM Bit 5 */
#define PCM_CTL0_CPM_0                           ((uint32_t)0x00000000)          /*!< LDO based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_CPM_1                           ((uint32_t)0x00000100)          /*!< LDO based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_CPM_4                           ((uint32_t)0x00000400)          /*!< DC-DC based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_CPM_5                           ((uint32_t)0x00000500)          /*!< DC-DC based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_CPM_8                           ((uint32_t)0x00000800)          /*!< Low-Frequency Active Mode at Core voltage setting 0. */
#define PCM_CTL0_CPM_9                           ((uint32_t)0x00000900)          /*!< Low-Frequency Active Mode at Core voltage setting 1. */
#define PCM_CTL0_CPM_16                          ((uint32_t)0x00001000)          /*!< LDO based LPM0 at Core voltage setting 0. */
#define PCM_CTL0_CPM_17                          ((uint32_t)0x00001100)          /*!< LDO based LPM0 at Core voltage setting 1. */
#define PCM_CTL0_CPM_20                          ((uint32_t)0x00001400)          /*!< DC-DC based LPM0 at Core voltage setting 0. */
#define PCM_CTL0_CPM_21                          ((uint32_t)0x00001500)          /*!< DC-DC based LPM0 at Core voltage setting 1. */
#define PCM_CTL0_CPM_24                          ((uint32_t)0x00001800)          /*!< Low-Frequency LPM0 at Core voltage setting 0. */
#define PCM_CTL0_CPM_25                          ((uint32_t)0x00001900)          /*!< Low-Frequency LPM0 at Core voltage setting 1. */
#define PCM_CTL0_CPM_32                          ((uint32_t)0x00002000)          /*!< LPM3 */
#define PCM_CTL0_CPM__AM_LDO_VCORE0              ((uint32_t)0x00000000)          /*!< LDO based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_CPM__AM_LDO_VCORE1              ((uint32_t)0x00000100)          /*!< LDO based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_CPM__AM_DCDC_VCORE0             ((uint32_t)0x00000400)          /*!< DC-DC based Active Mode at Core voltage setting 0. */
#define PCM_CTL0_CPM__AM_DCDC_VCORE1             ((uint32_t)0x00000500)          /*!< DC-DC based Active Mode at Core voltage setting 1. */
#define PCM_CTL0_CPM__AM_LF_VCORE0               ((uint32_t)0x00000800)          /*!< Low-Frequency Active Mode at Core voltage setting 0. */
#define PCM_CTL0_CPM__AM_LF_VCORE1               ((uint32_t)0x00000900)          /*!< Low-Frequency Active Mode at Core voltage setting 1. */
#define PCM_CTL0_CPM__LPM0_LDO_VCORE0            ((uint32_t)0x00001000)          /*!< LDO based LPM0 at Core voltage setting 0. */
#define PCM_CTL0_CPM__LPM0_LDO_VCORE1            ((uint32_t)0x00001100)          /*!< LDO based LPM0 at Core voltage setting 1. */
#define PCM_CTL0_CPM__LPM0_DCDC_VCORE0           ((uint32_t)0x00001400)          /*!< DC-DC based LPM0 at Core voltage setting 0. */
#define PCM_CTL0_CPM__LPM0_DCDC_VCORE1           ((uint32_t)0x00001500)          /*!< DC-DC based LPM0 at Core voltage setting 1. */
#define PCM_CTL0_CPM__LPM0_LF_VCORE0             ((uint32_t)0x00001800)          /*!< Low-Frequency LPM0 at Core voltage setting 0. */
#define PCM_CTL0_CPM__LPM0_LF_VCORE1             ((uint32_t)0x00001900)          /*!< Low-Frequency LPM0 at Core voltage setting 1. */
#define PCM_CTL0_CPM__LPM3                       ((uint32_t)0x00002000)          /*!< LPM3 */
#define PCM_CTL0_KEY_OFS                         (16)                            /*!< PCMKEY Bit Offset */
#define PCM_CTL0_KEY_MASK                        ((uint32_t)0xFFFF0000)          /*!< PCMKEY Bit Mask */
#define PCM_CTL1_LOCKLPM5_OFS                    ( 0)                            /*!< LOCKLPM5 Bit Offset */
#define PCM_CTL1_LOCKLPM5                        ((uint32_t)0x00000001)          /*!< Lock LPM5 */
#define PCM_CTL1_LOCKBKUP_OFS                    ( 1)                            /*!< LOCKBKUP Bit Offset */
#define PCM_CTL1_LOCKBKUP                        ((uint32_t)0x00000002)          /*!< Lock Backup */
#define PCM_CTL1_FORCE_LPM_ENTRY_OFS             ( 2)                            /*!< FORCE_LPM_ENTRY Bit Offset */
#define PCM_CTL1_FORCE_LPM_ENTRY                 ((uint32_t)0x00000004)          /*!< Force LPM entry */
#define PCM_CTL1_PMR_BUSY_OFS                    ( 8)                            /*!< PMR_BUSY Bit Offset */
#define PCM_CTL1_PMR_BUSY                        ((uint32_t)0x00000100)          /*!< Power mode request busy flag */
#define PCM_CTL1_KEY_OFS                         (16)                            /*!< PCMKEY Bit Offset */
#define PCM_CTL1_KEY_MASK                        ((uint32_t)0xFFFF0000)          /*!< PCMKEY Bit Mask */
#define PCM_IE_LPM_INVALID_TR_IE_OFS             ( 0)                            /*!< LPM_INVALID_TR_IE Bit Offset */
#define PCM_IE_LPM_INVALID_TR_IE                 ((uint32_t)0x00000001)          /*!< LPM invalid transition interrupt enable */
#define PCM_IE_LPM_INVALID_CLK_IE_OFS            ( 1)                            /*!< LPM_INVALID_CLK_IE Bit Offset */
#define PCM_IE_LPM_INVALID_CLK_IE                ((uint32_t)0x00000002)          /*!< LPM invalid clock interrupt enable */
#define PCM_IE_AM_INVALID_TR_IE_OFS              ( 2)                            /*!< AM_INVALID_TR_IE Bit Offset */
#define PCM_IE_AM_INVALID_TR_IE                  ((uint32_t)0x00000004)          /*!< Active mode invalid transition interrupt enable */
#define PCM_IE_DCDC_ERROR_IE_OFS                 ( 6)                            /*!< DCDC_ERROR_IE Bit Offset */
#define PCM_IE_DCDC_ERROR_IE                     ((uint32_t)0x00000040)          /*!< DC-DC error interrupt enable */
#define PCM_IFG_LPM_INVALID_TR_IFG_OFS           ( 0)                            /*!< LPM_INVALID_TR_IFG Bit Offset */
#define PCM_IFG_LPM_INVALID_TR_IFG               ((uint32_t)0x00000001)          /*!< LPM invalid transition flag */
#define PCM_IFG_LPM_INVALID_CLK_IFG_OFS          ( 1)                            /*!< LPM_INVALID_CLK_IFG Bit Offset */
#define PCM_IFG_LPM_INVALID_CLK_IFG              ((uint32_t)0x00000002)          /*!< LPM invalid clock flag */
#define PCM_IFG_AM_INVALID_TR_IFG_OFS            ( 2)                            /*!< AM_INVALID_TR_IFG Bit Offset */
#define PCM_IFG_AM_INVALID_TR_IFG                ((uint32_t)0x00000004)          /*!< Active mode invalid transition flag */
#define PCM_IFG_DCDC_ERROR_IFG_OFS               ( 6)                            /*!< DCDC_ERROR_IFG Bit Offset */
#define PCM_IFG_DCDC_ERROR_IFG                   ((uint32_t)0x00000040)          /*!< DC-DC error flag */
#define PCM_CLRIFG_CLR_LPM_INVALID_TR_IFG_OFS    ( 0)                            /*!< CLR_LPM_INVALID_TR_IFG Bit Offset */
#define PCM_CLRIFG_CLR_LPM_INVALID_TR_IFG        ((uint32_t)0x00000001)          /*!< Clear LPM invalid transition flag */
#define PCM_CLRIFG_CLR_LPM_INVALID_CLK_IFG_OFS   ( 1)                            /*!< CLR_LPM_INVALID_CLK_IFG Bit Offset */
#define PCM_CLRIFG_CLR_LPM_INVALID_CLK_IFG       ((uint32_t)0x00000002)          /*!< Clear LPM invalid clock flag */
#define PCM_CLRIFG_CLR_AM_INVALID_TR_IFG_OFS     ( 2)                            /*!< CLR_AM_INVALID_TR_IFG Bit Offset */
#define PCM_CLRIFG_CLR_AM_INVALID_TR_IFG         ((uint32_t)0x00000004)          /*!< Clear active mode invalid transition flag */
#define PCM_CLRIFG_CLR_DCDC_ERROR_IFG_OFS        ( 6)                            /*!< CLR_DCDC_ERROR_IFG Bit Offset */
#define PCM_CLRIFG_CLR_DCDC_ERROR_IFG            ((uint32_t)0x00000040)          /*!< Clear DC-DC error flag */
#define PCM_CTL0_KEY_VAL                         ((uint32_t)0x695A0000)          /*!< PCM key value */
#define PCM_CTL1_KEY_VAL                         ((uint32_t)0x695A0000)          /*!< PCM key value */
#define PMAP_CTL_LOCKED_OFS                      ( 0)                            /*!< PMAPLOCKED Bit Offset */
#define PMAP_CTL_LOCKED                          ((uint16_t)0x0001)              /*!< Port mapping lock bit */
#define PMAP_CTL_PRECFG_OFS                      ( 1)                            /*!< PMAPRECFG Bit Offset */
#define PMAP_CTL_PRECFG                          ((uint16_t)0x0002)              /*!< Port mapping reconfiguration control bit */
#define PMAP_NONE                                            0
#define PMAP_UCA0CLK                                         1
#define PMAP_UCA0RXD                                         2
#define PMAP_UCA0SOMI                                        2
#define PMAP_UCA0TXD                                         3
#define PMAP_UCA0SIMO                                        3
#define PMAP_UCB0CLK                                         4
#define PMAP_UCB0SDA                                         5
#define PMAP_UCB0SIMO                                        5
#define PMAP_UCB0SCL                                         6
#define PMAP_UCB0SOMI                                        6
#define PMAP_UCA1STE                                         7
#define PMAP_UCA1CLK                                         8
#define PMAP_UCA1RXD                                         9
#define PMAP_UCA1SOMI                                        9
#define PMAP_UCA1TXD                                         10
#define PMAP_UCA1SIMO                                        10
#define PMAP_UCA2STE                                         11
#define PMAP_UCA2CLK                                         12
#define PMAP_UCA2RXD                                         13
#define PMAP_UCA2SOMI                                        13
#define PMAP_UCA2TXD                                         14
#define PMAP_UCA2SIMO                                        14
#define PMAP_UCB2STE                                         15
#define PMAP_UCB2CLK                                         16
#define PMAP_UCB2SDA                                         17
#define PMAP_UCB2SIMO                                        17
#define PMAP_UCB2SCL                                         18
#define PMAP_UCB2SOMI                                        18
#define PMAP_TA0CCR0A                                        19
#define PMAP_TA0CCR1A                                        20
#define PMAP_TA0CCR2A                                        21
#define PMAP_TA0CCR3A                                        22
#define PMAP_TA0CCR4A                                        23
#define PMAP_TA1CCR1A                                        24
#define PMAP_TA1CCR2A                                        25
#define PMAP_TA1CCR3A                                        26
#define PMAP_TA1CCR4A                                        27
#define PMAP_TA0CLK                                          28
#define PMAP_CE0OUT                                          28
#define PMAP_TA1CLK                                          29
#define PMAP_CE1OUT                                          29
#define PMAP_DMAE0                                           30
#define PMAP_SMCLK                                           30
#define PMAP_ANALOG                                          31
#define PMAP_KEYID_VAL                           ((uint16_t)0x2D52)              /*!< Port Mapping Key */
#define PSS_KEY_KEY_OFS                          ( 0)                            /*!< PSSKEY Bit Offset */
#define PSS_KEY_KEY_MASK                         ((uint32_t)0x0000FFFF)          /*!< PSSKEY Bit Mask */
#define PSS_CTL0_SVSMHOFF_OFS                    ( 0)                            /*!< SVSMHOFF Bit Offset */
#define PSS_CTL0_SVSMHOFF                        ((uint32_t)0x00000001)          /*!< SVSM high-side off */
#define PSS_CTL0_SVSMHLP_OFS                     ( 1)                            /*!< SVSMHLP Bit Offset */
#define PSS_CTL0_SVSMHLP                         ((uint32_t)0x00000002)          /*!< SVSM high-side low power normal performance mode */
#define PSS_CTL0_SVSMHS_OFS                      ( 2)                            /*!< SVSMHS Bit Offset */
#define PSS_CTL0_SVSMHS                          ((uint32_t)0x00000004)          /*!< Supply supervisor or monitor selection for the high-side */
#define PSS_CTL0_SVSMHTH_OFS                     ( 3)                            /*!< SVSMHTH Bit Offset */
#define PSS_CTL0_SVSMHTH_MASK                    ((uint32_t)0x00000038)          /*!< SVSMHTH Bit Mask */
#define PSS_CTL0_SVMHOE_OFS                      ( 6)                            /*!< SVMHOE Bit Offset */
#define PSS_CTL0_SVMHOE                          ((uint32_t)0x00000040)          /*!< SVSM high-side output enable */
#define PSS_CTL0_SVMHOUTPOLAL_OFS                ( 7)                            /*!< SVMHOUTPOLAL Bit Offset */
#define PSS_CTL0_SVMHOUTPOLAL                    ((uint32_t)0x00000080)          /*!< SVMHOUT pin polarity active low */
#define PSS_CTL0_DCDC_FORCE_OFS                  (10)                            /*!< DCDC_FORCE Bit Offset */
#define PSS_CTL0_DCDC_FORCE                      ((uint32_t)0x00000400)          /*!< Force DC-DC regulator operation */
#define PSS_CTL0_VCORETRAN_OFS                   (12)                            /*!< VCORETRAN Bit Offset */
#define PSS_CTL0_VCORETRAN_MASK                  ((uint32_t)0x00003000)          /*!< VCORETRAN Bit Mask */
#define PSS_CTL0_VCORETRAN0                      ((uint32_t)0x00001000)          /*!< VCORETRAN Bit 0 */
#define PSS_CTL0_VCORETRAN1                      ((uint32_t)0x00002000)          /*!< VCORETRAN Bit 1 */
#define PSS_CTL0_VCORETRAN_0                     ((uint32_t)0x00000000)          /*!< 32 s / 100 mV */
#define PSS_CTL0_VCORETRAN_1                     ((uint32_t)0x00001000)          /*!< 64 s / 100 mV */
#define PSS_CTL0_VCORETRAN_2                     ((uint32_t)0x00002000)          /*!< 128 s / 100 mV (default) */
#define PSS_CTL0_VCORETRAN_3                     ((uint32_t)0x00003000)          /*!< 256 s / 100 mV */
#define PSS_CTL0_VCORETRAN__32                   ((uint32_t)0x00000000)          /*!< 32 s / 100 mV */
#define PSS_CTL0_VCORETRAN__64                   ((uint32_t)0x00001000)          /*!< 64 s / 100 mV */
#define PSS_CTL0_VCORETRAN__128                  ((uint32_t)0x00002000)          /*!< 128 s / 100 mV (default) */
#define PSS_CTL0_VCORETRAN__256                  ((uint32_t)0x00003000)          /*!< 256 s / 100 mV */
#define PSS_IE_SVSMHIE_OFS                       ( 1)                            /*!< SVSMHIE Bit Offset */
#define PSS_IE_SVSMHIE                           ((uint32_t)0x00000002)          /*!< High-side SVSM interrupt enable */
#define PSS_IFG_SVSMHIFG_OFS                     ( 1)                            /*!< SVSMHIFG Bit Offset */
#define PSS_IFG_SVSMHIFG                         ((uint32_t)0x00000002)          /*!< High-side SVSM interrupt flag */
#define PSS_CLRIFG_CLRSVSMHIFG_OFS               ( 1)                            /*!< CLRSVSMHIFG Bit Offset */
#define PSS_CLRIFG_CLRSVSMHIFG                   ((uint32_t)0x00000002)          /*!< SVSMH clear interrupt flag */
#define PSS_KEY_KEY_VAL                           ((uint32_t)0x0000695A)          /*!< PSS control key value */
#define REF_A_CTL0_ON_OFS                        ( 0)                            /*!< REFON Bit Offset */
#define REF_A_CTL0_ON                            ((uint16_t)0x0001)              /*!< Reference enable */
#define REF_A_CTL0_OUT_OFS                       ( 1)                            /*!< REFOUT Bit Offset */
#define REF_A_CTL0_OUT                           ((uint16_t)0x0002)              /*!< Reference output buffer */
#define REF_A_CTL0_TCOFF_OFS                     ( 3)                            /*!< REFTCOFF Bit Offset */
#define REF_A_CTL0_TCOFF                         ((uint16_t)0x0008)              /*!< Temperature sensor disabled */
#define REF_A_CTL0_VSEL_OFS                      ( 4)                            /*!< REFVSEL Bit Offset */
#define REF_A_CTL0_VSEL_MASK                     ((uint16_t)0x0030)              /*!< REFVSEL Bit Mask */
#define REF_A_CTL0_VSEL0                         ((uint16_t)0x0010)              /*!< VSEL Bit 0 */
#define REF_A_CTL0_VSEL1                         ((uint16_t)0x0020)              /*!< VSEL Bit 1 */
#define REF_A_CTL0_VSEL_0                        ((uint16_t)0x0000)              /*!< 1.2 V available when reference requested or REFON = 1 */
#define REF_A_CTL0_VSEL_1                        ((uint16_t)0x0010)              /*!< 1.45 V available when reference requested or REFON = 1 */
#define REF_A_CTL0_VSEL_3                        ((uint16_t)0x0030)              /*!< 2.5 V available when reference requested or REFON = 1 */
#define REF_A_CTL0_GENOT_OFS                     ( 6)                            /*!< REFGENOT Bit Offset */
#define REF_A_CTL0_GENOT                         ((uint16_t)0x0040)              /*!< Reference generator one-time trigger */
#define REF_A_CTL0_BGOT_OFS                      ( 7)                            /*!< REFBGOT Bit Offset */
#define REF_A_CTL0_BGOT                          ((uint16_t)0x0080)              /*!< Bandgap and bandgap buffer one-time trigger */
#define REF_A_CTL0_GENACT_OFS                    ( 8)                            /*!< REFGENACT Bit Offset */
#define REF_A_CTL0_GENACT                        ((uint16_t)0x0100)              /*!< Reference generator active */
#define REF_A_CTL0_BGACT_OFS                     ( 9)                            /*!< REFBGACT Bit Offset */
#define REF_A_CTL0_BGACT                         ((uint16_t)0x0200)              /*!< Reference bandgap active */
#define REF_A_CTL0_GENBUSY_OFS                   (10)                            /*!< REFGENBUSY Bit Offset */
#define REF_A_CTL0_GENBUSY                       ((uint16_t)0x0400)              /*!< Reference generator busy */
#define REF_A_CTL0_BGMODE_OFS                    (11)                            /*!< BGMODE Bit Offset */
#define REF_A_CTL0_BGMODE                        ((uint16_t)0x0800)              /*!< Bandgap mode */
#define REF_A_CTL0_GENRDY_OFS                    (12)                            /*!< REFGENRDY Bit Offset */
#define REF_A_CTL0_GENRDY                        ((uint16_t)0x1000)              /*!< Variable reference voltage ready status */
#define REF_A_CTL0_BGRDY_OFS                     (13)                            /*!< REFBGRDY Bit Offset */
#define REF_A_CTL0_BGRDY                         ((uint16_t)0x2000)              /*!< Buffered bandgap voltage ready status */
#define RSTCTL_RESET_REQ_SOFT_REQ_OFS            ( 0)                            /*!< SOFT_REQ Bit Offset */
#define RSTCTL_RESET_REQ_SOFT_REQ                ((uint32_t)0x00000001)          /*!< Soft Reset request */
#define RSTCTL_RESET_REQ_HARD_REQ_OFS            ( 1)                            /*!< HARD_REQ Bit Offset */
#define RSTCTL_RESET_REQ_HARD_REQ                ((uint32_t)0x00000002)          /*!< Hard Reset request */
#define RSTCTL_RESET_REQ_RSTKEY_OFS              ( 8)                            /*!< RSTKEY Bit Offset */
#define RSTCTL_RESET_REQ_RSTKEY_MASK             ((uint32_t)0x0000FF00)          /*!< RSTKEY Bit Mask */
#define RSTCTL_HARDRESET_STAT_SRC0_OFS           ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC0               ((uint32_t)0x00000001)          /*!< Indicates that SRC0 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC1_OFS           ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC1               ((uint32_t)0x00000002)          /*!< Indicates that SRC1 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC2_OFS           ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC2               ((uint32_t)0x00000004)          /*!< Indicates that SRC2 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC3_OFS           ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC3               ((uint32_t)0x00000008)          /*!< Indicates that SRC3 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC4_OFS           ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC4               ((uint32_t)0x00000010)          /*!< Indicates that SRC4 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC5_OFS           ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC5               ((uint32_t)0x00000020)          /*!< Indicates that SRC5 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC6_OFS           ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC6               ((uint32_t)0x00000040)          /*!< Indicates that SRC6 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC7_OFS           ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC7               ((uint32_t)0x00000080)          /*!< Indicates that SRC7 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC8_OFS           ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC8               ((uint32_t)0x00000100)          /*!< Indicates that SRC8 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC9_OFS           ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC9               ((uint32_t)0x00000200)          /*!< Indicates that SRC9 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC10_OFS          (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC10              ((uint32_t)0x00000400)          /*!< Indicates that SRC10 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC11_OFS          (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC11              ((uint32_t)0x00000800)          /*!< Indicates that SRC11 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC12_OFS          (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC12              ((uint32_t)0x00001000)          /*!< Indicates that SRC12 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC13_OFS          (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC13              ((uint32_t)0x00002000)          /*!< Indicates that SRC13 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC14_OFS          (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC14              ((uint32_t)0x00004000)          /*!< Indicates that SRC14 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_STAT_SRC15_OFS          (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC15              ((uint32_t)0x00008000)          /*!< Indicates that SRC15 was the source of the Hard Reset */
#define RSTCTL_HARDRESET_CLR_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
#define RSTCTL_HARDRESET_CLR_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HRDRESETSTAT_REG */
#define RSTCTL_HARDRESET_SET_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_HARDRESET_SET_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
#define RSTCTL_SOFTRESET_STAT_SRC0_OFS           ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC0               ((uint32_t)0x00000001)          /*!< If 1, indicates that SRC0 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC1_OFS           ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC1               ((uint32_t)0x00000002)          /*!< If 1, indicates that SRC1 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC2_OFS           ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC2               ((uint32_t)0x00000004)          /*!< If 1, indicates that SRC2 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC3_OFS           ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC3               ((uint32_t)0x00000008)          /*!< If 1, indicates that SRC3 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC4_OFS           ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC4               ((uint32_t)0x00000010)          /*!< If 1, indicates that SRC4 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC5_OFS           ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC5               ((uint32_t)0x00000020)          /*!< If 1, indicates that SRC5 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC6_OFS           ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC6               ((uint32_t)0x00000040)          /*!< If 1, indicates that SRC6 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC7_OFS           ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC7               ((uint32_t)0x00000080)          /*!< If 1, indicates that SRC7 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC8_OFS           ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC8               ((uint32_t)0x00000100)          /*!< If 1, indicates that SRC8 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC9_OFS           ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC9               ((uint32_t)0x00000200)          /*!< If 1, indicates that SRC9 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC10_OFS          (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC10              ((uint32_t)0x00000400)          /*!< If 1, indicates that SRC10 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC11_OFS          (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC11              ((uint32_t)0x00000800)          /*!< If 1, indicates that SRC11 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC12_OFS          (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC12              ((uint32_t)0x00001000)          /*!< If 1, indicates that SRC12 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC13_OFS          (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC13              ((uint32_t)0x00002000)          /*!< If 1, indicates that SRC13 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC14_OFS          (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC14              ((uint32_t)0x00004000)          /*!< If 1, indicates that SRC14 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_STAT_SRC15_OFS          (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC15              ((uint32_t)0x00008000)          /*!< If 1, indicates that SRC15 was the source of the Soft Reset */
#define RSTCTL_SOFTRESET_CLR_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_CLR_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
#define RSTCTL_SOFTRESET_SET_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_SOFTRESET_SET_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
#define RSTCTL_PSSRESET_STAT_SVSMH_OFS           ( 1)                            /*!< SVSMH Bit Offset */
#define RSTCTL_PSSRESET_STAT_SVSMH               ((uint32_t)0x00000002)          /*!< Indicates if POR was caused by an SVSMH trip condition int the PSS */
#define RSTCTL_PSSRESET_STAT_BGREF_OFS           ( 2)                            /*!< BGREF Bit Offset */
#define RSTCTL_PSSRESET_STAT_BGREF               ((uint32_t)0x00000004)          /*!< Indicates if POR was caused by a BGREF not okay condition in the PSS */
#define RSTCTL_PSSRESET_STAT_VCCDET_OFS          ( 3)                            /*!< VCCDET Bit Offset */
#define RSTCTL_PSSRESET_STAT_VCCDET              ((uint32_t)0x00000008)          /*!< Indicates if POR was caused by a VCCDET trip condition in the PSS */
#define RSTCTL_PSSRESET_CLR_CLR_OFS              ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_PSSRESET_CLR_CLR                  ((uint32_t)0x00000001)          /*!< Write 1 clears all PSS Reset Flags in the RSTCTL_PSSRESET_STAT */
#define RSTCTL_PCMRESET_STAT_LPM35_OFS           ( 0)                            /*!< LPM35 Bit Offset */
#define RSTCTL_PCMRESET_STAT_LPM35               ((uint32_t)0x00000001)          /*!< Indicates if POR was caused by PCM due to an exit from LPM3.5 */
#define RSTCTL_PCMRESET_STAT_LPM45_OFS           ( 1)                            /*!< LPM45 Bit Offset */
#define RSTCTL_PCMRESET_STAT_LPM45               ((uint32_t)0x00000002)          /*!< Indicates if POR was caused by PCM due to an exit from LPM4.5 */
#define RSTCTL_PCMRESET_CLR_CLR_OFS              ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_PCMRESET_CLR_CLR                  ((uint32_t)0x00000001)          /*!< Write 1 clears all PCM Reset Flags in the RSTCTL_PCMRESET_STAT */
#define RSTCTL_PINRESET_STAT_RSTNMI_OFS          ( 0)                            /*!< RSTNMI Bit Offset */
#define RSTCTL_PINRESET_STAT_RSTNMI              ((uint32_t)0x00000001)          /*!< POR was caused by RSTn/NMI pin based reset event */
#define RSTCTL_PINRESET_CLR_CLR_OFS              ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_PINRESET_CLR_CLR                  ((uint32_t)0x00000001)          /*!< Write 1 clears the RSTn/NMI Pin Reset Flag in RSTCTL_PINRESET_STAT */
#define RSTCTL_REBOOTRESET_STAT_REBOOT_OFS       ( 0)                            /*!< REBOOT Bit Offset */
#define RSTCTL_REBOOTRESET_STAT_REBOOT           ((uint32_t)0x00000001)          /*!< Indicates if Reboot reset was caused by the SYSCTL module. */
#define RSTCTL_REBOOTRESET_CLR_CLR_OFS           ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_REBOOTRESET_CLR_CLR               ((uint32_t)0x00000001)          /*!< Write 1 clears the Reboot Reset Flag in RSTCTL_REBOOTRESET_STAT */
#define RSTCTL_CSRESET_STAT_DCOR_SHT_OFS         ( 0)                            /*!< DCOR_SHT Bit Offset */
#define RSTCTL_CSRESET_STAT_DCOR_SHT             ((uint32_t)0x00000001)          /*!< Indicates if POR was caused by DCO short circuit fault in the external  */
#define RSTCTL_CSRESET_CLR_CLR_OFS               ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_CSRESET_CLR_CLR                   ((uint32_t)0x00000001)          /*!< Write 1 clears the DCOR_SHT Flag in RSTCTL_CSRESET_STAT as well as  */
#define RSTCTL_RESETREQ_RSTKEY_VAL                 ((uint32_t)0x00006900)          /*!< Key value to enable writes to bits 1-0 */
#define RTC_C_CTL0_RDYIFG_OFS                    ( 0)                            /*!< RTCRDYIFG Bit Offset */
#define RTC_C_CTL0_RDYIFG                        ((uint16_t)0x0001)              /*!< Real-time clock ready interrupt flag */
#define RTC_C_CTL0_AIFG_OFS                      ( 1)                            /*!< RTCAIFG Bit Offset */
#define RTC_C_CTL0_AIFG                          ((uint16_t)0x0002)              /*!< Real-time clock alarm interrupt flag */
#define RTC_C_CTL0_TEVIFG_OFS                    ( 2)                            /*!< RTCTEVIFG Bit Offset */
#define RTC_C_CTL0_TEVIFG                        ((uint16_t)0x0004)              /*!< Real-time clock time event interrupt flag */
#define RTC_C_CTL0_OFIFG_OFS                     ( 3)                            /*!< RTCOFIFG Bit Offset */
#define RTC_C_CTL0_OFIFG                         ((uint16_t)0x0008)              /*!< 32-kHz crystal oscillator fault interrupt flag */
#define RTC_C_CTL0_RDYIE_OFS                     ( 4)                            /*!< RTCRDYIE Bit Offset */
#define RTC_C_CTL0_RDYIE                         ((uint16_t)0x0010)              /*!< Real-time clock ready interrupt enable */
#define RTC_C_CTL0_AIE_OFS                       ( 5)                            /*!< RTCAIE Bit Offset */
#define RTC_C_CTL0_AIE                           ((uint16_t)0x0020)              /*!< Real-time clock alarm interrupt enable */
#define RTC_C_CTL0_TEVIE_OFS                     ( 6)                            /*!< RTCTEVIE Bit Offset */
#define RTC_C_CTL0_TEVIE                         ((uint16_t)0x0040)              /*!< Real-time clock time event interrupt enable */
#define RTC_C_CTL0_OFIE_OFS                      ( 7)                            /*!< RTCOFIE Bit Offset */
#define RTC_C_CTL0_OFIE                          ((uint16_t)0x0080)              /*!< 32-kHz crystal oscillator fault interrupt enable */
#define RTC_C_CTL0_KEY_OFS                       ( 8)                            /*!< RTCKEY Bit Offset */
#define RTC_C_CTL0_KEY_MASK                      ((uint16_t)0xFF00)              /*!< RTCKEY Bit Mask */
#define RTC_C_CTL13_TEV_OFS                      ( 0)                            /*!< RTCTEV Bit Offset */
#define RTC_C_CTL13_TEV_MASK                     ((uint16_t)0x0003)              /*!< RTCTEV Bit Mask */
#define RTC_C_CTL13_TEV0                         ((uint16_t)0x0001)              /*!< TEV Bit 0 */
#define RTC_C_CTL13_TEV1                         ((uint16_t)0x0002)              /*!< TEV Bit 1 */
#define RTC_C_CTL13_TEV_0                        ((uint16_t)0x0000)              /*!< Minute changed */
#define RTC_C_CTL13_TEV_1                        ((uint16_t)0x0001)              /*!< Hour changed */
#define RTC_C_CTL13_TEV_2                        ((uint16_t)0x0002)              /*!< Every day at midnight (00:00) */
#define RTC_C_CTL13_TEV_3                        ((uint16_t)0x0003)              /*!< Every day at noon (12:00) */
#define RTC_C_CTL13_SSEL_OFS                     ( 2)                            /*!< RTCSSEL Bit Offset */
#define RTC_C_CTL13_SSEL_MASK                    ((uint16_t)0x000C)              /*!< RTCSSEL Bit Mask */
#define RTC_C_CTL13_SSEL0                        ((uint16_t)0x0004)              /*!< SSEL Bit 0 */
#define RTC_C_CTL13_SSEL1                        ((uint16_t)0x0008)              /*!< SSEL Bit 1 */
#define RTC_C_CTL13_SSEL_0                       ((uint16_t)0x0000)              /*!< BCLK */
#define RTC_C_CTL13_SSEL__BCLK                   ((uint16_t)0x0000)              /*!< BCLK */
#define RTC_C_CTL13_RDY_OFS                      ( 4)                            /*!< RTCRDY Bit Offset */
#define RTC_C_CTL13_RDY                          ((uint16_t)0x0010)              /*!< Real-time clock ready */
#define RTC_C_CTL13_MODE_OFS                     ( 5)                            /*!< RTCMODE Bit Offset */
#define RTC_C_CTL13_MODE                         ((uint16_t)0x0020)              
#define RTC_C_CTL13_HOLD_OFS                     ( 6)                            /*!< RTCHOLD Bit Offset */
#define RTC_C_CTL13_HOLD                         ((uint16_t)0x0040)              /*!< Real-time clock hold */
#define RTC_C_CTL13_BCD_OFS                      ( 7)                            /*!< RTCBCD Bit Offset */
#define RTC_C_CTL13_BCD                          ((uint16_t)0x0080)              /*!< Real-time clock BCD select */
#define RTC_C_CTL13_CALF_OFS                     ( 8)                            /*!< RTCCALF Bit Offset */
#define RTC_C_CTL13_CALF_MASK                    ((uint16_t)0x0300)              /*!< RTCCALF Bit Mask */
#define RTC_C_CTL13_CALF0                        ((uint16_t)0x0100)              /*!< CALF Bit 0 */
#define RTC_C_CTL13_CALF1                        ((uint16_t)0x0200)              /*!< CALF Bit 1 */
#define RTC_C_CTL13_CALF_0                       ((uint16_t)0x0000)              /*!< No frequency output to RTCCLK pin */
#define RTC_C_CTL13_CALF_1                       ((uint16_t)0x0100)              /*!< 512 Hz */
#define RTC_C_CTL13_CALF_2                       ((uint16_t)0x0200)              /*!< 256 Hz */
#define RTC_C_CTL13_CALF_3                       ((uint16_t)0x0300)              /*!< 1 Hz */
#define RTC_C_CTL13_CALF__NONE                   ((uint16_t)0x0000)              /*!< No frequency output to RTCCLK pin */
#define RTC_C_CTL13_CALF__512                    ((uint16_t)0x0100)              /*!< 512 Hz */
#define RTC_C_CTL13_CALF__256                    ((uint16_t)0x0200)              /*!< 256 Hz */
#define RTC_C_CTL13_CALF__1                      ((uint16_t)0x0300)              /*!< 1 Hz */
#define RTC_C_OCAL_OCAL_OFS                      ( 0)                            /*!< RTCOCAL Bit Offset */
#define RTC_C_OCAL_OCAL_MASK                     ((uint16_t)0x00FF)              /*!< RTCOCAL Bit Mask */
#define RTC_C_OCAL_OCALS_OFS                     (15)                            /*!< RTCOCALS Bit Offset */
#define RTC_C_OCAL_OCALS                         ((uint16_t)0x8000)              /*!< Real-time clock offset error calibration sign */
#define RTC_C_TCMP_TCMPX_OFS                     ( 0)                            /*!< RTCTCMP Bit Offset */
#define RTC_C_TCMP_TCMPX_MASK                    ((uint16_t)0x00FF)              /*!< RTCTCMP Bit Mask */
#define RTC_C_TCMP_TCOK_OFS                      (13)                            /*!< RTCTCOK Bit Offset */
#define RTC_C_TCMP_TCOK                          ((uint16_t)0x2000)              /*!< Real-time clock temperature compensation write OK */
#define RTC_C_TCMP_TCRDY_OFS                     (14)                            /*!< RTCTCRDY Bit Offset */
#define RTC_C_TCMP_TCRDY                         ((uint16_t)0x4000)              /*!< Real-time clock temperature compensation ready */
#define RTC_C_TCMP_TCMPS_OFS                     (15)                            /*!< RTCTCMPS Bit Offset */
#define RTC_C_TCMP_TCMPS                         ((uint16_t)0x8000)              /*!< Real-time clock temperature compensation sign */
#define RTC_C_PS0CTL_RT0PSIFG_OFS                ( 0)                            /*!< RT0PSIFG Bit Offset */
#define RTC_C_PS0CTL_RT0PSIFG                    ((uint16_t)0x0001)              /*!< Prescale timer 0 interrupt flag */
#define RTC_C_PS0CTL_RT0PSIE_OFS                 ( 1)                            /*!< RT0PSIE Bit Offset */
#define RTC_C_PS0CTL_RT0PSIE                     ((uint16_t)0x0002)              /*!< Prescale timer 0 interrupt enable */
#define RTC_C_PS0CTL_RT0IP_OFS                   ( 2)                            /*!< RT0IP Bit Offset */
#define RTC_C_PS0CTL_RT0IP_MASK                  ((uint16_t)0x001C)              /*!< RT0IP Bit Mask */
#define RTC_C_PS0CTL_RT0IP0                      ((uint16_t)0x0004)              /*!< RT0IP Bit 0 */
#define RTC_C_PS0CTL_RT0IP1                      ((uint16_t)0x0008)              /*!< RT0IP Bit 1 */
#define RTC_C_PS0CTL_RT0IP2                      ((uint16_t)0x0010)              /*!< RT0IP Bit 2 */
#define RTC_C_PS0CTL_RT0IP_0                     ((uint16_t)0x0000)              /*!< Divide by 2 */
#define RTC_C_PS0CTL_RT0IP_1                     ((uint16_t)0x0004)              /*!< Divide by 4 */
#define RTC_C_PS0CTL_RT0IP_2                     ((uint16_t)0x0008)              /*!< Divide by 8 */
#define RTC_C_PS0CTL_RT0IP_3                     ((uint16_t)0x000C)              /*!< Divide by 16 */
#define RTC_C_PS0CTL_RT0IP_4                     ((uint16_t)0x0010)              /*!< Divide by 32 */
#define RTC_C_PS0CTL_RT0IP_5                     ((uint16_t)0x0014)              /*!< Divide by 64 */
#define RTC_C_PS0CTL_RT0IP_6                     ((uint16_t)0x0018)              /*!< Divide by 128 */
#define RTC_C_PS0CTL_RT0IP_7                     ((uint16_t)0x001C)              /*!< Divide by 256 */
#define RTC_C_PS0CTL_RT0IP__2                    ((uint16_t)0x0000)              /*!< Divide by 2 */
#define RTC_C_PS0CTL_RT0IP__4                    ((uint16_t)0x0004)              /*!< Divide by 4 */
#define RTC_C_PS0CTL_RT0IP__8                    ((uint16_t)0x0008)              /*!< Divide by 8 */
#define RTC_C_PS0CTL_RT0IP__16                   ((uint16_t)0x000C)              /*!< Divide by 16 */
#define RTC_C_PS0CTL_RT0IP__32                   ((uint16_t)0x0010)              /*!< Divide by 32 */
#define RTC_C_PS0CTL_RT0IP__64                   ((uint16_t)0x0014)              /*!< Divide by 64 */
#define RTC_C_PS0CTL_RT0IP__128                  ((uint16_t)0x0018)              /*!< Divide by 128 */
#define RTC_C_PS0CTL_RT0IP__256                  ((uint16_t)0x001C)              /*!< Divide by 256 */
#define RTC_C_PS1CTL_RT1PSIFG_OFS                ( 0)                            /*!< RT1PSIFG Bit Offset */
#define RTC_C_PS1CTL_RT1PSIFG                    ((uint16_t)0x0001)              /*!< Prescale timer 1 interrupt flag */
#define RTC_C_PS1CTL_RT1PSIE_OFS                 ( 1)                            /*!< RT1PSIE Bit Offset */
#define RTC_C_PS1CTL_RT1PSIE                     ((uint16_t)0x0002)              /*!< Prescale timer 1 interrupt enable */
#define RTC_C_PS1CTL_RT1IP_OFS                   ( 2)                            /*!< RT1IP Bit Offset */
#define RTC_C_PS1CTL_RT1IP_MASK                  ((uint16_t)0x001C)              /*!< RT1IP Bit Mask */
#define RTC_C_PS1CTL_RT1IP0                      ((uint16_t)0x0004)              /*!< RT1IP Bit 0 */
#define RTC_C_PS1CTL_RT1IP1                      ((uint16_t)0x0008)              /*!< RT1IP Bit 1 */
#define RTC_C_PS1CTL_RT1IP2                      ((uint16_t)0x0010)              /*!< RT1IP Bit 2 */
#define RTC_C_PS1CTL_RT1IP_0                     ((uint16_t)0x0000)              /*!< Divide by 2 */
#define RTC_C_PS1CTL_RT1IP_1                     ((uint16_t)0x0004)              /*!< Divide by 4 */
#define RTC_C_PS1CTL_RT1IP_2                     ((uint16_t)0x0008)              /*!< Divide by 8 */
#define RTC_C_PS1CTL_RT1IP_3                     ((uint16_t)0x000C)              /*!< Divide by 16 */
#define RTC_C_PS1CTL_RT1IP_4                     ((uint16_t)0x0010)              /*!< Divide by 32 */
#define RTC_C_PS1CTL_RT1IP_5                     ((uint16_t)0x0014)              /*!< Divide by 64 */
#define RTC_C_PS1CTL_RT1IP_6                     ((uint16_t)0x0018)              /*!< Divide by 128 */
#define RTC_C_PS1CTL_RT1IP_7                     ((uint16_t)0x001C)              /*!< Divide by 256 */
#define RTC_C_PS1CTL_RT1IP__2                    ((uint16_t)0x0000)              /*!< Divide by 2 */
#define RTC_C_PS1CTL_RT1IP__4                    ((uint16_t)0x0004)              /*!< Divide by 4 */
#define RTC_C_PS1CTL_RT1IP__8                    ((uint16_t)0x0008)              /*!< Divide by 8 */
#define RTC_C_PS1CTL_RT1IP__16                   ((uint16_t)0x000C)              /*!< Divide by 16 */
#define RTC_C_PS1CTL_RT1IP__32                   ((uint16_t)0x0010)              /*!< Divide by 32 */
#define RTC_C_PS1CTL_RT1IP__64                   ((uint16_t)0x0014)              /*!< Divide by 64 */
#define RTC_C_PS1CTL_RT1IP__128                  ((uint16_t)0x0018)              /*!< Divide by 128 */
#define RTC_C_PS1CTL_RT1IP__256                  ((uint16_t)0x001C)              /*!< Divide by 256 */
#define RTC_C_PS_RT0PS_OFS                       ( 0)                            /*!< RT0PS Bit Offset */
#define RTC_C_PS_RT0PS_MASK                      ((uint16_t)0x00FF)              /*!< RT0PS Bit Mask */
#define RTC_C_PS_RT1PS_OFS                       ( 8)                            /*!< RT1PS Bit Offset */
#define RTC_C_PS_RT1PS_MASK                      ((uint16_t)0xFF00)              /*!< RT1PS Bit Mask */
#define RTC_C_TIM0_SEC_OFS                       ( 0)                            /*!< Seconds Bit Offset */
#define RTC_C_TIM0_SEC_MASK                      ((uint16_t)0x003F)              /*!< Seconds Bit Mask */
#define RTC_C_TIM0_MIN_OFS                       ( 8)                            /*!< Minutes Bit Offset */
#define RTC_C_TIM0_MIN_MASK                      ((uint16_t)0x3F00)              /*!< Minutes Bit Mask */
#define RTC_C_TIM0_SEC_LD_OFS                    ( 0)                            /*!< SecondsLowDigit Bit Offset */
#define RTC_C_TIM0_SEC_LD_MASK                   ((uint16_t)0x000F)              /*!< SecondsLowDigit Bit Mask */
#define RTC_C_TIM0_SEC_HD_OFS                    ( 4)                            /*!< SecondsHighDigit Bit Offset */
#define RTC_C_TIM0_SEC_HD_MASK                   ((uint16_t)0x0070)              /*!< SecondsHighDigit Bit Mask */
#define RTC_C_TIM0_MIN_LD_OFS                    ( 8)                            /*!< MinutesLowDigit Bit Offset */
#define RTC_C_TIM0_MIN_LD_MASK                   ((uint16_t)0x0F00)              /*!< MinutesLowDigit Bit Mask */
#define RTC_C_TIM0_MIN_HD_OFS                    (12)                            /*!< MinutesHighDigit Bit Offset */
#define RTC_C_TIM0_MIN_HD_MASK                   ((uint16_t)0x7000)              /*!< MinutesHighDigit Bit Mask */
#define RTC_C_TIM1_HOUR_OFS                      ( 0)                            /*!< Hours Bit Offset */
#define RTC_C_TIM1_HOUR_MASK                     ((uint16_t)0x001F)              /*!< Hours Bit Mask */
#define RTC_C_TIM1_DOW_OFS                       ( 8)                            /*!< DayofWeek Bit Offset */
#define RTC_C_TIM1_DOW_MASK                      ((uint16_t)0x0700)              /*!< DayofWeek Bit Mask */
#define RTC_C_TIM1_HOUR_LD_OFS                   ( 0)                            /*!< HoursLowDigit Bit Offset */
#define RTC_C_TIM1_HOUR_LD_MASK                  ((uint16_t)0x000F)              /*!< HoursLowDigit Bit Mask */
#define RTC_C_TIM1_HOUR_HD_OFS                   ( 4)                            /*!< HoursHighDigit Bit Offset */
#define RTC_C_TIM1_HOUR_HD_MASK                  ((uint16_t)0x0030)              /*!< HoursHighDigit Bit Mask */
#define RTC_C_DATE_DAY_OFS                       ( 0)                            /*!< Day Bit Offset */
#define RTC_C_DATE_DAY_MASK                      ((uint16_t)0x001F)              /*!< Day Bit Mask */
#define RTC_C_DATE_MON_OFS                       ( 8)                            /*!< Month Bit Offset */
#define RTC_C_DATE_MON_MASK                      ((uint16_t)0x0F00)              /*!< Month Bit Mask */
#define RTC_C_DATE_DAY_LD_OFS                    ( 0)                            /*!< DayLowDigit Bit Offset */
#define RTC_C_DATE_DAY_LD_MASK                   ((uint16_t)0x000F)              /*!< DayLowDigit Bit Mask */
#define RTC_C_DATE_DAY_HD_OFS                    ( 4)                            /*!< DayHighDigit Bit Offset */
#define RTC_C_DATE_DAY_HD_MASK                   ((uint16_t)0x0030)              /*!< DayHighDigit Bit Mask */
#define RTC_C_DATE_MON_LD_OFS                    ( 8)                            /*!< MonthLowDigit Bit Offset */
#define RTC_C_DATE_MON_LD_MASK                   ((uint16_t)0x0F00)              /*!< MonthLowDigit Bit Mask */
#define RTC_C_DATE_MON_HD_OFS                    (12)                            /*!< MonthHighDigit Bit Offset */
#define RTC_C_DATE_MON_HD                        ((uint16_t)0x1000)              /*!< Month  high digit (0 or 1) */
#define RTC_C_YEAR_YEAR_LB_OFS                   ( 0)                            /*!< YearLowByte Bit Offset */
#define RTC_C_YEAR_YEAR_LB_MASK                  ((uint16_t)0x00FF)              /*!< YearLowByte Bit Mask */
#define RTC_C_YEAR_YEAR_HB_OFS                   ( 8)                            /*!< YearHighByte Bit Offset */
#define RTC_C_YEAR_YEAR_HB_MASK                  ((uint16_t)0x0F00)              /*!< YearHighByte Bit Mask */
#define RTC_C_YEAR_YEAR_OFS                      ( 0)                            /*!< Year Bit Offset */
#define RTC_C_YEAR_YEAR_MASK                     ((uint16_t)0x000F)              /*!< Year Bit Mask */
#define RTC_C_YEAR_DEC_OFS                       ( 4)                            /*!< Decade Bit Offset */
#define RTC_C_YEAR_DEC_MASK                      ((uint16_t)0x00F0)              /*!< Decade Bit Mask */
#define RTC_C_YEAR_CENT_LD_OFS                   ( 8)                            /*!< CenturyLowDigit Bit Offset */
#define RTC_C_YEAR_CENT_LD_MASK                  ((uint16_t)0x0F00)              /*!< CenturyLowDigit Bit Mask */
#define RTC_C_YEAR_CENT_HD_OFS                   (12)                            /*!< CenturyHighDigit Bit Offset */
#define RTC_C_YEAR_CENT_HD_MASK                  ((uint16_t)0x7000)              /*!< CenturyHighDigit Bit Mask */
#define RTC_C_AMINHR_MIN_OFS                     ( 0)                            /*!< Minutes Bit Offset */
#define RTC_C_AMINHR_MIN_MASK                    ((uint16_t)0x003F)              /*!< Minutes Bit Mask */
#define RTC_C_AMINHR_MINAE_OFS                   ( 7)                            /*!< MINAE Bit Offset */
#define RTC_C_AMINHR_MINAE                       ((uint16_t)0x0080)              /*!< Alarm enable */
#define RTC_C_AMINHR_HOUR_OFS                    ( 8)                            /*!< Hours Bit Offset */
#define RTC_C_AMINHR_HOUR_MASK                   ((uint16_t)0x1F00)              /*!< Hours Bit Mask */
#define RTC_C_AMINHR_HOURAE_OFS                  (15)                            /*!< HOURAE Bit Offset */
#define RTC_C_AMINHR_HOURAE                      ((uint16_t)0x8000)              /*!< Alarm enable */
#define RTC_C_AMINHR_MIN_LD_OFS                  ( 0)                            /*!< MinutesLowDigit Bit Offset */
#define RTC_C_AMINHR_MIN_LD_MASK                 ((uint16_t)0x000F)              /*!< MinutesLowDigit Bit Mask */
#define RTC_C_AMINHR_MIN_HD_OFS                  ( 4)                            /*!< MinutesHighDigit Bit Offset */
#define RTC_C_AMINHR_MIN_HD_MASK                 ((uint16_t)0x0070)              /*!< MinutesHighDigit Bit Mask */
#define RTC_C_AMINHR_HOUR_LD_OFS                 ( 8)                            /*!< HoursLowDigit Bit Offset */
#define RTC_C_AMINHR_HOUR_LD_MASK                ((uint16_t)0x0F00)              /*!< HoursLowDigit Bit Mask */
#define RTC_C_AMINHR_HOUR_HD_OFS                 (12)                            /*!< HoursHighDigit Bit Offset */
#define RTC_C_AMINHR_HOUR_HD_MASK                ((uint16_t)0x3000)              /*!< HoursHighDigit Bit Mask */
#define RTC_C_ADOWDAY_DOW_OFS                    ( 0)                            /*!< DayofWeek Bit Offset */
#define RTC_C_ADOWDAY_DOW_MASK                   ((uint16_t)0x0007)              /*!< DayofWeek Bit Mask */
#define RTC_C_ADOWDAY_DOWAE_OFS                  ( 7)                            /*!< DOWAE Bit Offset */
#define RTC_C_ADOWDAY_DOWAE                      ((uint16_t)0x0080)              /*!< Alarm enable */
#define RTC_C_ADOWDAY_DAY_OFS                    ( 8)                            /*!< DayofMonth Bit Offset */
#define RTC_C_ADOWDAY_DAY_MASK                   ((uint16_t)0x1F00)              /*!< DayofMonth Bit Mask */
#define RTC_C_ADOWDAY_DAYAE_OFS                  (15)                            /*!< DAYAE Bit Offset */
#define RTC_C_ADOWDAY_DAYAE                      ((uint16_t)0x8000)              /*!< Alarm enable */
#define RTC_C_ADOWDAY_DAY_LD_OFS                 ( 8)                            /*!< DayLowDigit Bit Offset */
#define RTC_C_ADOWDAY_DAY_LD_MASK                ((uint16_t)0x0F00)              /*!< DayLowDigit Bit Mask */
#define RTC_C_ADOWDAY_DAY_HD_OFS                 (12)                            /*!< DayHighDigit Bit Offset */
#define RTC_C_ADOWDAY_DAY_HD_MASK                ((uint16_t)0x3000)              /*!< DayHighDigit Bit Mask */
#define RTC_C_KEY                                 ((uint16_t)0xA500)              /*!< RTC_C Key Value for RTC_C write access */
#define RTC_C_KEY_H                               ((uint16_t)0x00A5)              /*!< RTC_C Key Value for RTC_C write access */
#define RTC_C_KEY_VAL                             ((uint16_t)0xA500)              /*!< RTC_C Key Value for RTC_C write access */
#define SCB_PFR0_STATE0_OFS                      ( 0)                            /*!< STATE0 Bit Offset */
#define SCB_PFR0_STATE0_MASK                     ((uint32_t)0x0000000F)          /*!< STATE0 Bit Mask */
#define SCB_PFR0_STATE00                         ((uint32_t)0x00000001)          /*!< STATE0 Bit 0 */
#define SCB_PFR0_STATE01                         ((uint32_t)0x00000002)          /*!< STATE0 Bit 1 */
#define SCB_PFR0_STATE02                         ((uint32_t)0x00000004)          /*!< STATE0 Bit 2 */
#define SCB_PFR0_STATE03                         ((uint32_t)0x00000008)          /*!< STATE0 Bit 3 */
#define SCB_PFR0_STATE0_0                        ((uint32_t)0x00000000)          /*!< no ARM encoding */
#define SCB_PFR0_STATE0_1                        ((uint32_t)0x00000001)          /*!< N/A */
#define SCB_PFR0_STATE1_OFS                      ( 4)                            /*!< STATE1 Bit Offset */
#define SCB_PFR0_STATE1_MASK                     ((uint32_t)0x000000F0)          /*!< STATE1 Bit Mask */
#define SCB_PFR0_STATE10                         ((uint32_t)0x00000010)          /*!< STATE1 Bit 0 */
#define SCB_PFR0_STATE11                         ((uint32_t)0x00000020)          /*!< STATE1 Bit 1 */
#define SCB_PFR0_STATE12                         ((uint32_t)0x00000040)          /*!< STATE1 Bit 2 */
#define SCB_PFR0_STATE13                         ((uint32_t)0x00000080)          /*!< STATE1 Bit 3 */
#define SCB_PFR0_STATE1_0                        ((uint32_t)0x00000000)          /*!< N/A */
#define SCB_PFR0_STATE1_1                        ((uint32_t)0x00000010)          /*!< N/A */
#define SCB_PFR0_STATE1_2                        ((uint32_t)0x00000020)          /*!< Thumb-2 encoding with the 16-bit basic instructions plus 32-bit Buncond/BL  */
#define SCB_PFR0_STATE1_3                        ((uint32_t)0x00000030)          /*!< Thumb-2 encoding with all Thumb-2 basic instructions */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_OFS ( 8)                            /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit Offset */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_MASK ((uint32_t)0x00000F00)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit Mask */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL0 ((uint32_t)0x00000100)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 0 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL1 ((uint32_t)0x00000200)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 1 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL2 ((uint32_t)0x00000400)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 2 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL3 ((uint32_t)0x00000800)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 3 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_0 ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_2 ((uint32_t)0x00000200)          /*!< two-stack support */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_OFS (20)                            /*!< MICROCONTROLLER_DEBUG_MODEL Bit Offset */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_MASK ((uint32_t)0x00F00000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit Mask */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL0    ((uint32_t)0x00100000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 0 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL1    ((uint32_t)0x00200000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 1 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL2    ((uint32_t)0x00400000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 2 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL3    ((uint32_t)0x00800000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 3 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_0   ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_1   ((uint32_t)0x00100000)          /*!< Microcontroller debug v1 (ITMv1, DWTv1, optional ETM) */
#define SCB_MMFR0_PMSA_SUPPORT_OFS               ( 4)                            /*!< PMSA_SUPPORT Bit Offset */
#define SCB_MMFR0_PMSA_SUPPORT_MASK              ((uint32_t)0x000000F0)          /*!< PMSA_SUPPORT Bit Mask */
#define SCB_MMFR0_PMSA_SUPPORT0                  ((uint32_t)0x00000010)          /*!< PMSA_SUPPORT Bit 0 */
#define SCB_MMFR0_PMSA_SUPPORT1                  ((uint32_t)0x00000020)          /*!< PMSA_SUPPORT Bit 1 */
#define SCB_MMFR0_PMSA_SUPPORT2                  ((uint32_t)0x00000040)          /*!< PMSA_SUPPORT Bit 2 */
#define SCB_MMFR0_PMSA_SUPPORT3                  ((uint32_t)0x00000080)          /*!< PMSA_SUPPORT Bit 3 */
#define SCB_MMFR0_PMSA_SUPPORT_0                 ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_MMFR0_PMSA_SUPPORT_1                 ((uint32_t)0x00000010)          /*!< IMPLEMENTATION DEFINED (N/A) */
#define SCB_MMFR0_PMSA_SUPPORT_2                 ((uint32_t)0x00000020)          /*!< PMSA base (features as defined for ARMv6) (N/A) */
#define SCB_MMFR0_PMSA_SUPPORT_3                 ((uint32_t)0x00000030)          /*!< PMSAv7 (base plus subregion support) */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_OFS    ( 8)                            /*!< CACHE_COHERENCE_SUPPORT Bit Offset */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_MASK   ((uint32_t)0x00000F00)          /*!< CACHE_COHERENCE_SUPPORT Bit Mask */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT0       ((uint32_t)0x00000100)          /*!< CACHE_COHERENCE_SUPPORT Bit 0 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT1       ((uint32_t)0x00000200)          /*!< CACHE_COHERENCE_SUPPORT Bit 1 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT2       ((uint32_t)0x00000400)          /*!< CACHE_COHERENCE_SUPPORT Bit 2 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT3       ((uint32_t)0x00000800)          /*!< CACHE_COHERENCE_SUPPORT Bit 3 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_0      ((uint32_t)0x00000000)          /*!< no shared support */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_1      ((uint32_t)0x00000100)          /*!< partial-inner-shared coherency (coherency amongst some - but not all - of  */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_2      ((uint32_t)0x00000200)          /*!< full-inner-shared coherency (coherency amongst all of the entities within an  */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_3      ((uint32_t)0x00000300)          /*!< full coherency (coherency amongst all of the entities) */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_OFS (12)                            /*!< OUTER_NON_SHARABLE_SUPPORT Bit Offset */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_MASK ((uint32_t)0x0000F000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit Mask */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT0    ((uint32_t)0x00001000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 0 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT1    ((uint32_t)0x00002000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 1 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT2    ((uint32_t)0x00004000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 2 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT3    ((uint32_t)0x00008000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 3 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_0   ((uint32_t)0x00000000)          /*!< Outer non-sharable not supported */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_1   ((uint32_t)0x00001000)          /*!< Outer sharable supported */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_OFS  (20)                            /*!< AUXILIARY_REGISTER_SUPPORT Bit Offset */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_MASK ((uint32_t)0x00F00000)          /*!< AUXILIARY_REGISTER_SUPPORT Bit Mask */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT0     ((uint32_t)0x00100000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 0 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT1     ((uint32_t)0x00200000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 1 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT2     ((uint32_t)0x00400000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 2 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT3     ((uint32_t)0x00800000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 3 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_0    ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_1    ((uint32_t)0x00100000)          /*!< Auxiliary control register */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_OFS (24)                            /*!< WAIT_FOR_INTERRUPT_STALLING Bit Offset */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_MASK ((uint32_t)0x0F000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit Mask */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING0   ((uint32_t)0x01000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 0 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING1   ((uint32_t)0x02000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 1 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING2   ((uint32_t)0x04000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 2 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING3   ((uint32_t)0x08000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 3 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_0  ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_1  ((uint32_t)0x01000000)          /*!< wait for interrupt supported */
#define SCB_ISAR0_BITCOUNT_INSTRS_OFS            ( 4)                            /*!< BITCOUNT_INSTRS Bit Offset */
#define SCB_ISAR0_BITCOUNT_INSTRS_MASK           ((uint32_t)0x000000F0)          /*!< BITCOUNT_INSTRS Bit Mask */
#define SCB_ISAR0_BITCOUNT_INSTRS0               ((uint32_t)0x00000010)          /*!< BITCOUNT_INSTRS Bit 0 */
#define SCB_ISAR0_BITCOUNT_INSTRS1               ((uint32_t)0x00000020)          /*!< BITCOUNT_INSTRS Bit 1 */
#define SCB_ISAR0_BITCOUNT_INSTRS2               ((uint32_t)0x00000040)          /*!< BITCOUNT_INSTRS Bit 2 */
#define SCB_ISAR0_BITCOUNT_INSTRS3               ((uint32_t)0x00000080)          /*!< BITCOUNT_INSTRS Bit 3 */
#define SCB_ISAR0_BITCOUNT_INSTRS_0              ((uint32_t)0x00000000)          /*!< no bit-counting instructions present */
#define SCB_ISAR0_BITCOUNT_INSTRS_1              ((uint32_t)0x00000010)          /*!< adds CLZ */
#define SCB_ISAR0_BITFIELD_INSTRS_OFS            ( 8)                            /*!< BITFIELD_INSTRS Bit Offset */
#define SCB_ISAR0_BITFIELD_INSTRS_MASK           ((uint32_t)0x00000F00)          /*!< BITFIELD_INSTRS Bit Mask */
#define SCB_ISAR0_BITFIELD_INSTRS0               ((uint32_t)0x00000100)          /*!< BITFIELD_INSTRS Bit 0 */
#define SCB_ISAR0_BITFIELD_INSTRS1               ((uint32_t)0x00000200)          /*!< BITFIELD_INSTRS Bit 1 */
#define SCB_ISAR0_BITFIELD_INSTRS2               ((uint32_t)0x00000400)          /*!< BITFIELD_INSTRS Bit 2 */
#define SCB_ISAR0_BITFIELD_INSTRS3               ((uint32_t)0x00000800)          /*!< BITFIELD_INSTRS Bit 3 */
#define SCB_ISAR0_BITFIELD_INSTRS_0              ((uint32_t)0x00000000)          /*!< no bitfield instructions present */
#define SCB_ISAR0_BITFIELD_INSTRS_1              ((uint32_t)0x00000100)          /*!< adds BFC, BFI, SBFX, UBFX */
#define SCB_ISAR0_CMPBRANCH_INSTRS_OFS           (12)                            /*!< CMPBRANCH_INSTRS Bit Offset */
#define SCB_ISAR0_CMPBRANCH_INSTRS_MASK          ((uint32_t)0x0000F000)          /*!< CMPBRANCH_INSTRS Bit Mask */
#define SCB_ISAR0_CMPBRANCH_INSTRS0              ((uint32_t)0x00001000)          /*!< CMPBRANCH_INSTRS Bit 0 */
#define SCB_ISAR0_CMPBRANCH_INSTRS1              ((uint32_t)0x00002000)          /*!< CMPBRANCH_INSTRS Bit 1 */
#define SCB_ISAR0_CMPBRANCH_INSTRS2              ((uint32_t)0x00004000)          /*!< CMPBRANCH_INSTRS Bit 2 */
#define SCB_ISAR0_CMPBRANCH_INSTRS3              ((uint32_t)0x00008000)          /*!< CMPBRANCH_INSTRS Bit 3 */
#define SCB_ISAR0_CMPBRANCH_INSTRS_0             ((uint32_t)0x00000000)          /*!< no combined compare-and-branch instructions present */
#define SCB_ISAR0_CMPBRANCH_INSTRS_1             ((uint32_t)0x00001000)          /*!< adds CB{N}Z */
#define SCB_ISAR0_COPROC_INSTRS_OFS              (16)                            /*!< COPROC_INSTRS Bit Offset */
#define SCB_ISAR0_COPROC_INSTRS_MASK             ((uint32_t)0x000F0000)          /*!< COPROC_INSTRS Bit Mask */
#define SCB_ISAR0_COPROC_INSTRS0                 ((uint32_t)0x00010000)          /*!< COPROC_INSTRS Bit 0 */
#define SCB_ISAR0_COPROC_INSTRS1                 ((uint32_t)0x00020000)          /*!< COPROC_INSTRS Bit 1 */
#define SCB_ISAR0_COPROC_INSTRS2                 ((uint32_t)0x00040000)          /*!< COPROC_INSTRS Bit 2 */
#define SCB_ISAR0_COPROC_INSTRS3                 ((uint32_t)0x00080000)          /*!< COPROC_INSTRS Bit 3 */
#define SCB_ISAR0_COPROC_INSTRS_0                ((uint32_t)0x00000000)          /*!< no coprocessor support, other than for separately attributed architectures  */
#define SCB_ISAR0_COPROC_INSTRS_1                ((uint32_t)0x00010000)          /*!< adds generic CDP, LDC, MCR, MRC, STC */
#define SCB_ISAR0_COPROC_INSTRS_2                ((uint32_t)0x00020000)          /*!< adds generic CDP2, LDC2, MCR2, MRC2, STC2 */
#define SCB_ISAR0_COPROC_INSTRS_3                ((uint32_t)0x00030000)          /*!< adds generic MCRR, MRRC */
#define SCB_ISAR0_COPROC_INSTRS_4                ((uint32_t)0x00040000)          /*!< adds generic MCRR2, MRRC2 */
#define SCB_ISAR0_DEBUG_INSTRS_OFS               (20)                            /*!< DEBUG_INSTRS Bit Offset */
#define SCB_ISAR0_DEBUG_INSTRS_MASK              ((uint32_t)0x00F00000)          /*!< DEBUG_INSTRS Bit Mask */
#define SCB_ISAR0_DEBUG_INSTRS0                  ((uint32_t)0x00100000)          /*!< DEBUG_INSTRS Bit 0 */
#define SCB_ISAR0_DEBUG_INSTRS1                  ((uint32_t)0x00200000)          /*!< DEBUG_INSTRS Bit 1 */
#define SCB_ISAR0_DEBUG_INSTRS2                  ((uint32_t)0x00400000)          /*!< DEBUG_INSTRS Bit 2 */
#define SCB_ISAR0_DEBUG_INSTRS3                  ((uint32_t)0x00800000)          /*!< DEBUG_INSTRS Bit 3 */
#define SCB_ISAR0_DEBUG_INSTRS_0                 ((uint32_t)0x00000000)          /*!< no debug instructions present */
#define SCB_ISAR0_DEBUG_INSTRS_1                 ((uint32_t)0x00100000)          /*!< adds BKPT */
#define SCB_ISAR0_DIVIDE_INSTRS_OFS              (24)                            /*!< DIVIDE_INSTRS Bit Offset */
#define SCB_ISAR0_DIVIDE_INSTRS_MASK             ((uint32_t)0x0F000000)          /*!< DIVIDE_INSTRS Bit Mask */
#define SCB_ISAR0_DIVIDE_INSTRS0                 ((uint32_t)0x01000000)          /*!< DIVIDE_INSTRS Bit 0 */
#define SCB_ISAR0_DIVIDE_INSTRS1                 ((uint32_t)0x02000000)          /*!< DIVIDE_INSTRS Bit 1 */
#define SCB_ISAR0_DIVIDE_INSTRS2                 ((uint32_t)0x04000000)          /*!< DIVIDE_INSTRS Bit 2 */
#define SCB_ISAR0_DIVIDE_INSTRS3                 ((uint32_t)0x08000000)          /*!< DIVIDE_INSTRS Bit 3 */
#define SCB_ISAR0_DIVIDE_INSTRS_0                ((uint32_t)0x00000000)          /*!< no divide instructions present */
#define SCB_ISAR0_DIVIDE_INSTRS_1                ((uint32_t)0x01000000)          /*!< adds SDIV, UDIV (v1 quotient only result) */
#define SCB_ISAR1_ETEND_INSRS_OFS                (12)                            /*!< EXTEND_INSRS Bit Offset */
#define SCB_ISAR1_ETEND_INSRS_MASK               ((uint32_t)0x0000F000)          /*!< EXTEND_INSRS Bit Mask */
#define SCB_ISAR1_ETEND_INSRS0                   ((uint32_t)0x00001000)          /*!< ETEND_INSRS Bit 0 */
#define SCB_ISAR1_ETEND_INSRS1                   ((uint32_t)0x00002000)          /*!< ETEND_INSRS Bit 1 */
#define SCB_ISAR1_ETEND_INSRS2                   ((uint32_t)0x00004000)          /*!< ETEND_INSRS Bit 2 */
#define SCB_ISAR1_ETEND_INSRS3                   ((uint32_t)0x00008000)          /*!< ETEND_INSRS Bit 3 */
#define SCB_ISAR1_ETEND_INSRS_0                  ((uint32_t)0x00000000)          /*!< no scalar (i.e. non-SIMD) sign/zero-extend instructions present */
#define SCB_ISAR1_ETEND_INSRS_1                  ((uint32_t)0x00001000)          /*!< adds SXTB, SXTH, UXTB, UXTH */
#define SCB_ISAR1_ETEND_INSRS_2                  ((uint32_t)0x00002000)          /*!< N/A */
#define SCB_ISAR1_IFTHEN_INSTRS_OFS              (16)                            /*!< IFTHEN_INSTRS Bit Offset */
#define SCB_ISAR1_IFTHEN_INSTRS_MASK             ((uint32_t)0x000F0000)          /*!< IFTHEN_INSTRS Bit Mask */
#define SCB_ISAR1_IFTHEN_INSTRS0                 ((uint32_t)0x00010000)          /*!< IFTHEN_INSTRS Bit 0 */
#define SCB_ISAR1_IFTHEN_INSTRS1                 ((uint32_t)0x00020000)          /*!< IFTHEN_INSTRS Bit 1 */
#define SCB_ISAR1_IFTHEN_INSTRS2                 ((uint32_t)0x00040000)          /*!< IFTHEN_INSTRS Bit 2 */
#define SCB_ISAR1_IFTHEN_INSTRS3                 ((uint32_t)0x00080000)          /*!< IFTHEN_INSTRS Bit 3 */
#define SCB_ISAR1_IFTHEN_INSTRS_0                ((uint32_t)0x00000000)          /*!< IT instructions not present */
#define SCB_ISAR1_IFTHEN_INSTRS_1                ((uint32_t)0x00010000)          /*!< adds IT instructions (and IT bits in PSRs) */
#define SCB_ISAR1_IMMEDIATE_INSTRS_OFS           (20)                            /*!< IMMEDIATE_INSTRS Bit Offset */
#define SCB_ISAR1_IMMEDIATE_INSTRS_MASK          ((uint32_t)0x00F00000)          /*!< IMMEDIATE_INSTRS Bit Mask */
#define SCB_ISAR1_IMMEDIATE_INSTRS0              ((uint32_t)0x00100000)          /*!< IMMEDIATE_INSTRS Bit 0 */
#define SCB_ISAR1_IMMEDIATE_INSTRS1              ((uint32_t)0x00200000)          /*!< IMMEDIATE_INSTRS Bit 1 */
#define SCB_ISAR1_IMMEDIATE_INSTRS2              ((uint32_t)0x00400000)          /*!< IMMEDIATE_INSTRS Bit 2 */
#define SCB_ISAR1_IMMEDIATE_INSTRS3              ((uint32_t)0x00800000)          /*!< IMMEDIATE_INSTRS Bit 3 */
#define SCB_ISAR1_IMMEDIATE_INSTRS_0             ((uint32_t)0x00000000)          /*!< no special immediate-generating instructions present */
#define SCB_ISAR1_IMMEDIATE_INSTRS_1             ((uint32_t)0x00100000)          /*!< adds ADDW, MOVW, MOVT, SUBW */
#define SCB_ISAR1_INTERWORK_INSTRS_OFS           (24)                            /*!< INTERWORK_INSTRS Bit Offset */
#define SCB_ISAR1_INTERWORK_INSTRS_MASK          ((uint32_t)0x0F000000)          /*!< INTERWORK_INSTRS Bit Mask */
#define SCB_ISAR1_INTERWORK_INSTRS0              ((uint32_t)0x01000000)          /*!< INTERWORK_INSTRS Bit 0 */
#define SCB_ISAR1_INTERWORK_INSTRS1              ((uint32_t)0x02000000)          /*!< INTERWORK_INSTRS Bit 1 */
#define SCB_ISAR1_INTERWORK_INSTRS2              ((uint32_t)0x04000000)          /*!< INTERWORK_INSTRS Bit 2 */
#define SCB_ISAR1_INTERWORK_INSTRS3              ((uint32_t)0x08000000)          /*!< INTERWORK_INSTRS Bit 3 */
#define SCB_ISAR1_INTERWORK_INSTRS_0             ((uint32_t)0x00000000)          /*!< no interworking instructions supported */
#define SCB_ISAR1_INTERWORK_INSTRS_1             ((uint32_t)0x01000000)          /*!< adds BX (and T bit in PSRs) */
#define SCB_ISAR1_INTERWORK_INSTRS_2             ((uint32_t)0x02000000)          /*!< adds BLX, and PC loads have BX-like behavior */
#define SCB_ISAR1_INTERWORK_INSTRS_3             ((uint32_t)0x03000000)          /*!< N/A */
#define SCB_ISAR2_LOADSTORE_INSTRS_OFS           ( 0)                            /*!< LOADSTORE_INSTRS Bit Offset */
#define SCB_ISAR2_LOADSTORE_INSTRS_MASK          ((uint32_t)0x0000000F)          /*!< LOADSTORE_INSTRS Bit Mask */
#define SCB_ISAR2_LOADSTORE_INSTRS0              ((uint32_t)0x00000001)          /*!< LOADSTORE_INSTRS Bit 0 */
#define SCB_ISAR2_LOADSTORE_INSTRS1              ((uint32_t)0x00000002)          /*!< LOADSTORE_INSTRS Bit 1 */
#define SCB_ISAR2_LOADSTORE_INSTRS2              ((uint32_t)0x00000004)          /*!< LOADSTORE_INSTRS Bit 2 */
#define SCB_ISAR2_LOADSTORE_INSTRS3              ((uint32_t)0x00000008)          /*!< LOADSTORE_INSTRS Bit 3 */
#define SCB_ISAR2_LOADSTORE_INSTRS_0             ((uint32_t)0x00000000)          /*!< no additional normal load/store instructions present */
#define SCB_ISAR2_LOADSTORE_INSTRS_1             ((uint32_t)0x00000001)          /*!< adds LDRD/STRD */
#define SCB_ISAR2_MEMHINT_INSTRS_OFS             ( 4)                            /*!< MEMHINT_INSTRS Bit Offset */
#define SCB_ISAR2_MEMHINT_INSTRS_MASK            ((uint32_t)0x000000F0)          /*!< MEMHINT_INSTRS Bit Mask */
#define SCB_ISAR2_MEMHINT_INSTRS0                ((uint32_t)0x00000010)          /*!< MEMHINT_INSTRS Bit 0 */
#define SCB_ISAR2_MEMHINT_INSTRS1                ((uint32_t)0x00000020)          /*!< MEMHINT_INSTRS Bit 1 */
#define SCB_ISAR2_MEMHINT_INSTRS2                ((uint32_t)0x00000040)          /*!< MEMHINT_INSTRS Bit 2 */
#define SCB_ISAR2_MEMHINT_INSTRS3                ((uint32_t)0x00000080)          /*!< MEMHINT_INSTRS Bit 3 */
#define SCB_ISAR2_MEMHINT_INSTRS_0               ((uint32_t)0x00000000)          /*!< no memory hint instructions presen */
#define SCB_ISAR2_MEMHINT_INSTRS_1               ((uint32_t)0x00000010)          /*!< adds PLD */
#define SCB_ISAR2_MEMHINT_INSTRS_2               ((uint32_t)0x00000020)          /*!< adds PLD (ie a repeat on value 1) */
#define SCB_ISAR2_MEMHINT_INSTRS_3               ((uint32_t)0x00000030)          /*!< adds PLI */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_OFS      ( 8)                            /*!< MULTIACCESSINT_INSTRS Bit Offset */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_MASK     ((uint32_t)0x00000F00)          /*!< MULTIACCESSINT_INSTRS Bit Mask */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS0         ((uint32_t)0x00000100)          /*!< MULTIACCESSINT_INSTRS Bit 0 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS1         ((uint32_t)0x00000200)          /*!< MULTIACCESSINT_INSTRS Bit 1 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS2         ((uint32_t)0x00000400)          /*!< MULTIACCESSINT_INSTRS Bit 2 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS3         ((uint32_t)0x00000800)          /*!< MULTIACCESSINT_INSTRS Bit 3 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_0        ((uint32_t)0x00000000)          /*!< the (LDM/STM) instructions are non-interruptible */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_1        ((uint32_t)0x00000100)          /*!< the (LDM/STM) instructions are restartable */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_2        ((uint32_t)0x00000200)          /*!< the (LDM/STM) instructions are continuable */
#define SCB_ISAR2_MULT_INSTRS_OFS                (12)                            /*!< MULT_INSTRS Bit Offset */
#define SCB_ISAR2_MULT_INSTRS_MASK               ((uint32_t)0x0000F000)          /*!< MULT_INSTRS Bit Mask */
#define SCB_ISAR2_MULT_INSTRS0                   ((uint32_t)0x00001000)          /*!< MULT_INSTRS Bit 0 */
#define SCB_ISAR2_MULT_INSTRS1                   ((uint32_t)0x00002000)          /*!< MULT_INSTRS Bit 1 */
#define SCB_ISAR2_MULT_INSTRS2                   ((uint32_t)0x00004000)          /*!< MULT_INSTRS Bit 2 */
#define SCB_ISAR2_MULT_INSTRS3                   ((uint32_t)0x00008000)          /*!< MULT_INSTRS Bit 3 */
#define SCB_ISAR2_MULT_INSTRS_0                  ((uint32_t)0x00000000)          /*!< only MUL present */
#define SCB_ISAR2_MULT_INSTRS_1                  ((uint32_t)0x00001000)          /*!< adds MLA */
#define SCB_ISAR2_MULT_INSTRS_2                  ((uint32_t)0x00002000)          /*!< adds MLS */
#define SCB_ISAR2_MULTS_INSTRS_OFS               (16)                            /*!< MULTS_INSTRS Bit Offset */
#define SCB_ISAR2_MULTS_INSTRS_MASK              ((uint32_t)0x000F0000)          /*!< MULTS_INSTRS Bit Mask */
#define SCB_ISAR2_MULTS_INSTRS0                  ((uint32_t)0x00010000)          /*!< MULTS_INSTRS Bit 0 */
#define SCB_ISAR2_MULTS_INSTRS1                  ((uint32_t)0x00020000)          /*!< MULTS_INSTRS Bit 1 */
#define SCB_ISAR2_MULTS_INSTRS2                  ((uint32_t)0x00040000)          /*!< MULTS_INSTRS Bit 2 */
#define SCB_ISAR2_MULTS_INSTRS3                  ((uint32_t)0x00080000)          /*!< MULTS_INSTRS Bit 3 */
#define SCB_ISAR2_MULTS_INSTRS_0                 ((uint32_t)0x00000000)          /*!< no signed multiply instructions present */
#define SCB_ISAR2_MULTS_INSTRS_1                 ((uint32_t)0x00010000)          /*!< adds SMULL, SMLAL */
#define SCB_ISAR2_MULTS_INSTRS_2                 ((uint32_t)0x00020000)          /*!< N/A */
#define SCB_ISAR2_MULTS_INSTRS_3                 ((uint32_t)0x00030000)          /*!< N/A */
#define SCB_ISAR2_MULTU_INSTRS_OFS               (20)                            /*!< MULTU_INSTRS Bit Offset */
#define SCB_ISAR2_MULTU_INSTRS_MASK              ((uint32_t)0x00F00000)          /*!< MULTU_INSTRS Bit Mask */
#define SCB_ISAR2_MULTU_INSTRS0                  ((uint32_t)0x00100000)          /*!< MULTU_INSTRS Bit 0 */
#define SCB_ISAR2_MULTU_INSTRS1                  ((uint32_t)0x00200000)          /*!< MULTU_INSTRS Bit 1 */
#define SCB_ISAR2_MULTU_INSTRS2                  ((uint32_t)0x00400000)          /*!< MULTU_INSTRS Bit 2 */
#define SCB_ISAR2_MULTU_INSTRS3                  ((uint32_t)0x00800000)          /*!< MULTU_INSTRS Bit 3 */
#define SCB_ISAR2_MULTU_INSTRS_0                 ((uint32_t)0x00000000)          /*!< no unsigned multiply instructions present */
#define SCB_ISAR2_MULTU_INSTRS_1                 ((uint32_t)0x00100000)          /*!< adds UMULL, UMLAL */
#define SCB_ISAR2_MULTU_INSTRS_2                 ((uint32_t)0x00200000)          /*!< N/A */
#define SCB_ISAR2_REVERSAL_INSTRS_OFS            (28)                            /*!< REVERSAL_INSTRS Bit Offset */
#define SCB_ISAR2_REVERSAL_INSTRS_MASK           ((uint32_t)0xF0000000)          /*!< REVERSAL_INSTRS Bit Mask */
#define SCB_ISAR2_REVERSAL_INSTRS0               ((uint32_t)0x10000000)          /*!< REVERSAL_INSTRS Bit 0 */
#define SCB_ISAR2_REVERSAL_INSTRS1               ((uint32_t)0x20000000)          /*!< REVERSAL_INSTRS Bit 1 */
#define SCB_ISAR2_REVERSAL_INSTRS2               ((uint32_t)0x40000000)          /*!< REVERSAL_INSTRS Bit 2 */
#define SCB_ISAR2_REVERSAL_INSTRS3               ((uint32_t)0x80000000)          /*!< REVERSAL_INSTRS Bit 3 */
#define SCB_ISAR2_REVERSAL_INSTRS_0              ((uint32_t)0x00000000)          /*!< no reversal instructions present */
#define SCB_ISAR2_REVERSAL_INSTRS_1              ((uint32_t)0x10000000)          /*!< adds REV, REV16, REVSH */
#define SCB_ISAR2_REVERSAL_INSTRS_2              ((uint32_t)0x20000000)          /*!< adds RBIT */
#define SCB_ISAR3_SATRUATE_INSTRS_OFS            ( 0)                            /*!< SATRUATE_INSTRS Bit Offset */
#define SCB_ISAR3_SATRUATE_INSTRS_MASK           ((uint32_t)0x0000000F)          /*!< SATRUATE_INSTRS Bit Mask */
#define SCB_ISAR3_SATRUATE_INSTRS0               ((uint32_t)0x00000001)          /*!< SATRUATE_INSTRS Bit 0 */
#define SCB_ISAR3_SATRUATE_INSTRS1               ((uint32_t)0x00000002)          /*!< SATRUATE_INSTRS Bit 1 */
#define SCB_ISAR3_SATRUATE_INSTRS2               ((uint32_t)0x00000004)          /*!< SATRUATE_INSTRS Bit 2 */
#define SCB_ISAR3_SATRUATE_INSTRS3               ((uint32_t)0x00000008)          /*!< SATRUATE_INSTRS Bit 3 */
#define SCB_ISAR3_SATRUATE_INSTRS_0              ((uint32_t)0x00000000)          /*!< no non-SIMD saturate instructions present */
#define SCB_ISAR3_SATRUATE_INSTRS_1              ((uint32_t)0x00000001)          /*!< N/A */
#define SCB_ISAR3_SIMD_INSTRS_OFS                ( 4)                            /*!< SIMD_INSTRS Bit Offset */
#define SCB_ISAR3_SIMD_INSTRS_MASK               ((uint32_t)0x000000F0)          /*!< SIMD_INSTRS Bit Mask */
#define SCB_ISAR3_SIMD_INSTRS0                   ((uint32_t)0x00000010)          /*!< SIMD_INSTRS Bit 0 */
#define SCB_ISAR3_SIMD_INSTRS1                   ((uint32_t)0x00000020)          /*!< SIMD_INSTRS Bit 1 */
#define SCB_ISAR3_SIMD_INSTRS2                   ((uint32_t)0x00000040)          /*!< SIMD_INSTRS Bit 2 */
#define SCB_ISAR3_SIMD_INSTRS3                   ((uint32_t)0x00000080)          /*!< SIMD_INSTRS Bit 3 */
#define SCB_ISAR3_SIMD_INSTRS_0                  ((uint32_t)0x00000000)          /*!< no SIMD instructions present */
#define SCB_ISAR3_SIMD_INSTRS_1                  ((uint32_t)0x00000010)          /*!< adds SSAT, USAT (and the Q flag in the PSRs) */
#define SCB_ISAR3_SIMD_INSTRS_3                  ((uint32_t)0x00000030)          /*!< N/A */
#define SCB_ISAR3_SVC_INSTRS_OFS                 ( 8)                            /*!< SVC_INSTRS Bit Offset */
#define SCB_ISAR3_SVC_INSTRS_MASK                ((uint32_t)0x00000F00)          /*!< SVC_INSTRS Bit Mask */
#define SCB_ISAR3_SVC_INSTRS0                    ((uint32_t)0x00000100)          /*!< SVC_INSTRS Bit 0 */
#define SCB_ISAR3_SVC_INSTRS1                    ((uint32_t)0x00000200)          /*!< SVC_INSTRS Bit 1 */
#define SCB_ISAR3_SVC_INSTRS2                    ((uint32_t)0x00000400)          /*!< SVC_INSTRS Bit 2 */
#define SCB_ISAR3_SVC_INSTRS3                    ((uint32_t)0x00000800)          /*!< SVC_INSTRS Bit 3 */
#define SCB_ISAR3_SVC_INSTRS_0                   ((uint32_t)0x00000000)          /*!< no SVC (SWI) instructions present */
#define SCB_ISAR3_SVC_INSTRS_1                   ((uint32_t)0x00000100)          /*!< adds SVC (SWI) */
#define SCB_ISAR3_SYNCPRIM_INSTRS_OFS            (12)                            /*!< SYNCPRIM_INSTRS Bit Offset */
#define SCB_ISAR3_SYNCPRIM_INSTRS_MASK           ((uint32_t)0x0000F000)          /*!< SYNCPRIM_INSTRS Bit Mask */
#define SCB_ISAR3_SYNCPRIM_INSTRS0               ((uint32_t)0x00001000)          /*!< SYNCPRIM_INSTRS Bit 0 */
#define SCB_ISAR3_SYNCPRIM_INSTRS1               ((uint32_t)0x00002000)          /*!< SYNCPRIM_INSTRS Bit 1 */
#define SCB_ISAR3_SYNCPRIM_INSTRS2               ((uint32_t)0x00004000)          /*!< SYNCPRIM_INSTRS Bit 2 */
#define SCB_ISAR3_SYNCPRIM_INSTRS3               ((uint32_t)0x00008000)          /*!< SYNCPRIM_INSTRS Bit 3 */
#define SCB_ISAR3_SYNCPRIM_INSTRS_0              ((uint32_t)0x00000000)          /*!< no synchronization primitives present */
#define SCB_ISAR3_SYNCPRIM_INSTRS_1              ((uint32_t)0x00001000)          /*!< adds LDREX, STREX */
#define SCB_ISAR3_SYNCPRIM_INSTRS_2              ((uint32_t)0x00002000)          /*!< adds LDREXB, LDREXH, LDREXD, STREXB, STREXH, STREXD, CLREX(N/A) */
#define SCB_ISAR3_TABBRANCH_INSTRS_OFS           (16)                            /*!< TABBRANCH_INSTRS Bit Offset */
#define SCB_ISAR3_TABBRANCH_INSTRS_MASK          ((uint32_t)0x000F0000)          /*!< TABBRANCH_INSTRS Bit Mask */
#define SCB_ISAR3_TABBRANCH_INSTRS0              ((uint32_t)0x00010000)          /*!< TABBRANCH_INSTRS Bit 0 */
#define SCB_ISAR3_TABBRANCH_INSTRS1              ((uint32_t)0x00020000)          /*!< TABBRANCH_INSTRS Bit 1 */
#define SCB_ISAR3_TABBRANCH_INSTRS2              ((uint32_t)0x00040000)          /*!< TABBRANCH_INSTRS Bit 2 */
#define SCB_ISAR3_TABBRANCH_INSTRS3              ((uint32_t)0x00080000)          /*!< TABBRANCH_INSTRS Bit 3 */
#define SCB_ISAR3_TABBRANCH_INSTRS_0             ((uint32_t)0x00000000)          /*!< no table-branch instructions present */
#define SCB_ISAR3_TABBRANCH_INSTRS_1             ((uint32_t)0x00010000)          /*!< adds TBB, TBH */
#define SCB_ISAR3_THUMBCOPY_INSTRS_OFS           (20)                            /*!< THUMBCOPY_INSTRS Bit Offset */
#define SCB_ISAR3_THUMBCOPY_INSTRS_MASK          ((uint32_t)0x00F00000)          /*!< THUMBCOPY_INSTRS Bit Mask */
#define SCB_ISAR3_THUMBCOPY_INSTRS0              ((uint32_t)0x00100000)          /*!< THUMBCOPY_INSTRS Bit 0 */
#define SCB_ISAR3_THUMBCOPY_INSTRS1              ((uint32_t)0x00200000)          /*!< THUMBCOPY_INSTRS Bit 1 */
#define SCB_ISAR3_THUMBCOPY_INSTRS2              ((uint32_t)0x00400000)          /*!< THUMBCOPY_INSTRS Bit 2 */
#define SCB_ISAR3_THUMBCOPY_INSTRS3              ((uint32_t)0x00800000)          /*!< THUMBCOPY_INSTRS Bit 3 */
#define SCB_ISAR3_THUMBCOPY_INSTRS_0             ((uint32_t)0x00000000)          /*!< Thumb MOV(register) instruction does not allow low reg -> low reg */
#define SCB_ISAR3_THUMBCOPY_INSTRS_1             ((uint32_t)0x00100000)          /*!< adds Thumb MOV(register) low reg -> low reg and the CPY alias */
#define SCB_ISAR3_TRUENOP_INSTRS_OFS             (24)                            /*!< TRUENOP_INSTRS Bit Offset */
#define SCB_ISAR3_TRUENOP_INSTRS_MASK            ((uint32_t)0x0F000000)          /*!< TRUENOP_INSTRS Bit Mask */
#define SCB_ISAR3_TRUENOP_INSTRS0                ((uint32_t)0x01000000)          /*!< TRUENOP_INSTRS Bit 0 */
#define SCB_ISAR3_TRUENOP_INSTRS1                ((uint32_t)0x02000000)          /*!< TRUENOP_INSTRS Bit 1 */
#define SCB_ISAR3_TRUENOP_INSTRS2                ((uint32_t)0x04000000)          /*!< TRUENOP_INSTRS Bit 2 */
#define SCB_ISAR3_TRUENOP_INSTRS3                ((uint32_t)0x08000000)          /*!< TRUENOP_INSTRS Bit 3 */
#define SCB_ISAR3_TRUENOP_INSTRS_0               ((uint32_t)0x00000000)          /*!< true NOP instructions not present - that is, NOP instructions with no  */
#define SCB_ISAR3_TRUENOP_INSTRS_1               ((uint32_t)0x01000000)          /*!< adds "true NOP", and the capability of additional "NOP compatible hints" */
#define SCB_ISAR4_UNPRIV_INSTRS_OFS              ( 0)                            /*!< UNPRIV_INSTRS Bit Offset */
#define SCB_ISAR4_UNPRIV_INSTRS_MASK             ((uint32_t)0x0000000F)          /*!< UNPRIV_INSTRS Bit Mask */
#define SCB_ISAR4_UNPRIV_INSTRS0                 ((uint32_t)0x00000001)          /*!< UNPRIV_INSTRS Bit 0 */
#define SCB_ISAR4_UNPRIV_INSTRS1                 ((uint32_t)0x00000002)          /*!< UNPRIV_INSTRS Bit 1 */
#define SCB_ISAR4_UNPRIV_INSTRS2                 ((uint32_t)0x00000004)          /*!< UNPRIV_INSTRS Bit 2 */
#define SCB_ISAR4_UNPRIV_INSTRS3                 ((uint32_t)0x00000008)          /*!< UNPRIV_INSTRS Bit 3 */
#define SCB_ISAR4_UNPRIV_INSTRS_0                ((uint32_t)0x00000000)          /*!< no "T variant" instructions exist */
#define SCB_ISAR4_UNPRIV_INSTRS_1                ((uint32_t)0x00000001)          /*!< adds LDRBT, LDRT, STRBT, STRT */
#define SCB_ISAR4_UNPRIV_INSTRS_2                ((uint32_t)0x00000002)          /*!< adds LDRHT, LDRSBT, LDRSHT, STRHT */
#define SCB_ISAR4_WITHSHIFTS_INSTRS_OFS          ( 4)                            /*!< WITHSHIFTS_INSTRS Bit Offset */
#define SCB_ISAR4_WITHSHIFTS_INSTRS_MASK         ((uint32_t)0x000000F0)          /*!< WITHSHIFTS_INSTRS Bit Mask */
#define SCB_ISAR4_WITHSHIFTS_INSTRS0             ((uint32_t)0x00000010)          /*!< WITHSHIFTS_INSTRS Bit 0 */
#define SCB_ISAR4_WITHSHIFTS_INSTRS1             ((uint32_t)0x00000020)          /*!< WITHSHIFTS_INSTRS Bit 1 */
#define SCB_ISAR4_WITHSHIFTS_INSTRS2             ((uint32_t)0x00000040)          /*!< WITHSHIFTS_INSTRS Bit 2 */
#define SCB_ISAR4_WITHSHIFTS_INSTRS3             ((uint32_t)0x00000080)          /*!< WITHSHIFTS_INSTRS Bit 3 */
#define SCB_ISAR4_WITHSHIFTS_INSTRS_0            ((uint32_t)0x00000000)          /*!< non-zero shifts only support MOV and shift instructions (see notes) */
#define SCB_ISAR4_WITHSHIFTS_INSTRS_1            ((uint32_t)0x00000010)          /*!< shifts of loads/stores over the range LSL 0-3 */
#define SCB_ISAR4_WITHSHIFTS_INSTRS_3            ((uint32_t)0x00000030)          /*!< adds other constant shift options. */
#define SCB_ISAR4_WITHSHIFTS_INSTRS_4            ((uint32_t)0x00000040)          /*!< adds register-controlled shift options. */
#define SCB_ISAR4_WRITEBACK_INSTRS_OFS           ( 8)                            /*!< WRITEBACK_INSTRS Bit Offset */
#define SCB_ISAR4_WRITEBACK_INSTRS_MASK          ((uint32_t)0x00000F00)          /*!< WRITEBACK_INSTRS Bit Mask */
#define SCB_ISAR4_WRITEBACK_INSTRS0              ((uint32_t)0x00000100)          /*!< WRITEBACK_INSTRS Bit 0 */
#define SCB_ISAR4_WRITEBACK_INSTRS1              ((uint32_t)0x00000200)          /*!< WRITEBACK_INSTRS Bit 1 */
#define SCB_ISAR4_WRITEBACK_INSTRS2              ((uint32_t)0x00000400)          /*!< WRITEBACK_INSTRS Bit 2 */
#define SCB_ISAR4_WRITEBACK_INSTRS3              ((uint32_t)0x00000800)          /*!< WRITEBACK_INSTRS Bit 3 */
#define SCB_ISAR4_WRITEBACK_INSTRS_0             ((uint32_t)0x00000000)          /*!< only non-writeback addressing modes present, except that  */
#define SCB_ISAR4_WRITEBACK_INSTRS_1             ((uint32_t)0x00000100)          /*!< adds all currently-defined writeback addressing modes (ARMv7, Thumb-2) */
#define SCB_ISAR4_BARRIER_INSTRS_OFS             (16)                            /*!< BARRIER_INSTRS Bit Offset */
#define SCB_ISAR4_BARRIER_INSTRS_MASK            ((uint32_t)0x000F0000)          /*!< BARRIER_INSTRS Bit Mask */
#define SCB_ISAR4_BARRIER_INSTRS0                ((uint32_t)0x00010000)          /*!< BARRIER_INSTRS Bit 0 */
#define SCB_ISAR4_BARRIER_INSTRS1                ((uint32_t)0x00020000)          /*!< BARRIER_INSTRS Bit 1 */
#define SCB_ISAR4_BARRIER_INSTRS2                ((uint32_t)0x00040000)          /*!< BARRIER_INSTRS Bit 2 */
#define SCB_ISAR4_BARRIER_INSTRS3                ((uint32_t)0x00080000)          /*!< BARRIER_INSTRS Bit 3 */
#define SCB_ISAR4_BARRIER_INSTRS_0               ((uint32_t)0x00000000)          /*!< no barrier instructions supported */
#define SCB_ISAR4_BARRIER_INSTRS_1               ((uint32_t)0x00010000)          /*!< adds DMB, DSB, ISB barrier instructions */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_OFS       (20)                            /*!< SYNCPRIM_INSTRS_FRAC Bit Offset */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_MASK      ((uint32_t)0x00F00000)          /*!< SYNCPRIM_INSTRS_FRAC Bit Mask */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC0          ((uint32_t)0x00100000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 0 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC1          ((uint32_t)0x00200000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 1 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC2          ((uint32_t)0x00400000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 2 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC3          ((uint32_t)0x00800000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 3 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_0         ((uint32_t)0x00000000)          /*!< no additional support */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_3         ((uint32_t)0x00300000)          /*!< adds CLREX, LDREXB, STREXB, LDREXH, STREXH */
#define SCB_ISAR4_PSR_M_INSTRS_OFS               (24)                            /*!< PSR_M_INSTRS Bit Offset */
#define SCB_ISAR4_PSR_M_INSTRS_MASK              ((uint32_t)0x0F000000)          /*!< PSR_M_INSTRS Bit Mask */
#define SCB_ISAR4_PSR_M_INSTRS0                  ((uint32_t)0x01000000)          /*!< PSR_M_INSTRS Bit 0 */
#define SCB_ISAR4_PSR_M_INSTRS1                  ((uint32_t)0x02000000)          /*!< PSR_M_INSTRS Bit 1 */
#define SCB_ISAR4_PSR_M_INSTRS2                  ((uint32_t)0x04000000)          /*!< PSR_M_INSTRS Bit 2 */
#define SCB_ISAR4_PSR_M_INSTRS3                  ((uint32_t)0x08000000)          /*!< PSR_M_INSTRS Bit 3 */
#define SCB_ISAR4_PSR_M_INSTRS_0                 ((uint32_t)0x00000000)          /*!< instructions not present */
#define SCB_ISAR4_PSR_M_INSTRS_1                 ((uint32_t)0x01000000)          /*!< adds CPS, MRS, and MSR instructions (M-profile forms) */
#define SCB_CPACR_CP11_OFS                       (22)                            /*!< CP11 Bit Offset */
#define SCB_CPACR_CP11_MASK                      ((uint32_t)0x00C00000)          /*!< CP11 Bit Mask */
#define SCB_CPACR_CP10_OFS                       (20)                            /*!< CP10 Bit Offset */
#define SCB_CPACR_CP10_MASK                      ((uint32_t)0x00300000)          /*!< CP10 Bit Mask */
#define SCB_SHPR1_PRI_4_OFS                      ( 0)                            /*!< PRI_4 Offset */
#define SCB_SHPR1_PRI_4_M                        ((uint32_t)0x000000ff)          /*  */
#define SCB_SHPR1_PRI_5_OFS                      ( 8)                            /*!< PRI_5 Offset */
#define SCB_SHPR1_PRI_5_M                        ((uint32_t)0x0000ff00)          /*  */
#define SCB_SHPR1_PRI_6_OFS                      (16)                            /*!< PRI_6 Offset */
#define SCB_SHPR1_PRI_6_M                        ((uint32_t)0x00ff0000)          /*  */
#define SCB_SHPR1_PRI_7_OFS                      (24)                            /*!< PRI_7 Offset */
#define SCB_SHPR1_PRI_7_M                        ((uint32_t)0xff000000)          /*  */
#define SCB_SHPR2_PRI_8_OFS                      ( 0)                            /*!< PRI_8 Offset */
#define SCB_SHPR2_PRI_8_M                        ((uint32_t)0x000000ff)          /*  */
#define SCB_SHPR2_PRI_9_OFS                      ( 8)                            /*!< PRI_9 Offset */
#define SCB_SHPR2_PRI_9_M                        ((uint32_t)0x0000ff00)          /*  */
#define SCB_SHPR2_PRI_10_OFS                     (16)                            /*!< PRI_10 Offset */
#define SCB_SHPR2_PRI_10_M                       ((uint32_t)0x00ff0000)          /*  */
#define SCB_SHPR2_PRI_11_OFS                     (24)                            /*!< PRI_11 Offset */
#define SCB_SHPR2_PRI_11_M                       ((uint32_t)0xff000000)          /*  */
#define SCB_SHPR3_PRI_12_OFS                     ( 0)                            /*!< PRI_12 Offset */
#define SCB_SHPR3_PRI_12_M                       ((uint32_t)0x000000ff)          /*  */
#define SCB_SHPR3_PRI_13_OFS                     ( 8)                            /*!< PRI_13 Offset */
#define SCB_SHPR3_PRI_13_M                       ((uint32_t)0x0000ff00)          /*  */
#define SCB_SHPR3_PRI_14_OFS                     (16)                            /*!< PRI_14 Offset */
#define SCB_SHPR3_PRI_14_M                       ((uint32_t)0x00ff0000)          /*  */
#define SCB_SHPR3_PRI_15_OFS                     (24)                            /*!< PRI_15 Offset */
#define SCB_SHPR3_PRI_15_M                       ((uint32_t)0xff000000)          /*  */
#define SCB_CFSR_IACCVIOL_OFS                    ( 0)                            /*!< IACCVIOL Offset */
#define SCB_CFSR_IACCVIOL                        ((uint32_t)0x00000001)          /*  */
#define SCB_CFSR_DACCVIOL_OFS                    ( 1)                            /*!< DACCVIOL Offset */
#define SCB_CFSR_DACCVIOL                        ((uint32_t)0x00000002)          /*  */
#define SCB_CFSR_MUNSTKERR_OFS                   ( 3)                            /*!< MUNSTKERR Offset */
#define SCB_CFSR_MUNSTKERR                       ((uint32_t)0x00000008)          /*  */
#define SCB_CFSR_MSTKERR_OFS                     ( 4)                            /*!< MSTKERR Offset */
#define SCB_CFSR_MSTKERR                         ((uint32_t)0x00000010)          /*  */
#define SCB_CFSR_MMARVALID_OFS                   ( 7)                            /*!< MMARVALID Offset */
#define SCB_CFSR_MMARVALID                       ((uint32_t)0x00000080)          /*  */
#define SCB_CFSR_IBUSERR_OFS                     ( 8)                            /*!< IBUSERR Offset */
#define SCB_CFSR_IBUSERR                         ((uint32_t)0x00000100)          /*  */
#define SCB_CFSR_PRECISERR_OFS                   ( 9)                            /*!< PRECISERR Offset */
#define SCB_CFSR_PRECISERR                       ((uint32_t)0x00000200)          /*  */
#define SCB_CFSR_IMPRECISERR_OFS                 (10)                            /*!< IMPRECISERR Offset */
#define SCB_CFSR_IMPRECISERR                     ((uint32_t)0x00000400)          /*  */
#define SCB_CFSR_UNSTKERR_OFS                    (11)                            /*!< UNSTKERR Offset */
#define SCB_CFSR_UNSTKERR                        ((uint32_t)0x00000800)          /*  */
#define SCB_CFSR_STKERR_OFS                      (12)                            /*!< STKERR Offset */
#define SCB_CFSR_STKERR                          ((uint32_t)0x00001000)          /*  */
#define SCB_CFSR_BFARVALID_OFS                   (15)                            /*!< BFARVALID Offset */
#define SCB_CFSR_BFARVALID                       ((uint32_t)0x00008000)          /*  */
#define SCB_CFSR_UNDEFINSTR_OFS                  (16)                            /*!< UNDEFINSTR Offset */
#define SCB_CFSR_UNDEFINSTR                      ((uint32_t)0x00010000)          /*  */
#define SCB_CFSR_INVSTATE_OFS                    (17)                            /*!< INVSTATE Offset */
#define SCB_CFSR_INVSTATE                        ((uint32_t)0x00020000)          /*  */
#define SCB_CFSR_INVPC_OFS                       (18)                            /*!< INVPC Offset */
#define SCB_CFSR_INVPC                           ((uint32_t)0x00040000)          /*  */
#define SCB_CFSR_NOCP_OFS                        (19)                            /*!< NOCP Offset */
#define SCB_CFSR_NOCP                            ((uint32_t)0x00080000)          /*  */
#define SCB_CFSR_UNALIGNED_OFS                   (24)                            /*!< UNALIGNED Offset */
#define SCB_CFSR_UNALIGNED                       ((uint32_t)0x01000000)          /*  */
#define SCB_CFSR_DIVBYZERO_OFS                   (25)                            /*!< DIVBYZERO Offset */
#define SCB_CFSR_DIVBYZERO                       ((uint32_t)0x02000000)          /*  */
#define SCB_CFSR_MLSPERR_OFS                     ( 5)                            /*!< MLSPERR Offset */
#define SCB_CFSR_MLSPERR                         ((uint32_t)0x00000020)          /*  */
#define SCB_CFSR_LSPERR_OFS                      (13)                            /*!< LSPERR Offset */
#define SCB_CFSR_LSPERR                          ((uint32_t)0x00002000)          /*  */
#define SYSCTL_REBOOT_CTL_REBOOT_OFS             ( 0)                            /*!< REBOOT Bit Offset */
#define SYSCTL_REBOOT_CTL_REBOOT                 ((uint32_t)0x00000001)          /*!< Write 1 initiates a Reboot of the device */
#define SYSCTL_REBOOT_CTL_WKEY_OFS               ( 8)                            /*!< WKEY Bit Offset */
#define SYSCTL_REBOOT_CTL_WKEY_MASK              ((uint32_t)0x0000FF00)          /*!< WKEY Bit Mask */
#define SYSCTL_NMI_CTLSTAT_CS_SRC_OFS            ( 0)                            /*!< CS_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_CS_SRC                ((uint32_t)0x00000001)          /*!< CS interrupt as a source of NMI */
#define SYSCTL_NMI_CTLSTAT_PSS_SRC_OFS           ( 1)                            /*!< PSS_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PSS_SRC               ((uint32_t)0x00000002)          /*!< PSS interrupt as a source of NMI */
#define SYSCTL_NMI_CTLSTAT_PCM_SRC_OFS           ( 2)                            /*!< PCM_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PCM_SRC               ((uint32_t)0x00000004)          /*!< PCM interrupt as a source of NMI */
#define SYSCTL_NMI_CTLSTAT_PIN_SRC_OFS           ( 3)                            /*!< PIN_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PIN_SRC               ((uint32_t)0x00000008)          
#define SYSCTL_NMI_CTLSTAT_CS_FLG_OFS            (16)                            /*!< CS_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_CS_FLG                ((uint32_t)0x00010000)          /*!< CS interrupt was the source of NMI */
#define SYSCTL_NMI_CTLSTAT_PSS_FLG_OFS           (17)                            /*!< PSS_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PSS_FLG               ((uint32_t)0x00020000)          /*!< PSS interrupt was the source of NMI */
#define SYSCTL_NMI_CTLSTAT_PCM_FLG_OFS           (18)                            /*!< PCM_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PCM_FLG               ((uint32_t)0x00040000)          /*!< PCM interrupt was the source of NMI */
#define SYSCTL_NMI_CTLSTAT_PIN_FLG_OFS           (19)                            /*!< PIN_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PIN_FLG               ((uint32_t)0x00080000)          /*!< RSTn/NMI pin was the source of NMI */
#define SYSCTL_WDTRESET_CTL_TIMEOUT_OFS          ( 0)                            /*!< TIMEOUT Bit Offset */
#define SYSCTL_WDTRESET_CTL_TIMEOUT              ((uint32_t)0x00000001)          /*!< WDT timeout reset type */
#define SYSCTL_WDTRESET_CTL_VIOLATION_OFS        ( 1)                            /*!< VIOLATION Bit Offset */
#define SYSCTL_WDTRESET_CTL_VIOLATION            ((uint32_t)0x00000002)          /*!< WDT password violation reset type */
#define SYSCTL_PERIHALT_CTL_HALT_T16_0_OFS       ( 0)                            /*!< HALT_T16_0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_0           ((uint32_t)0x00000001)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_T16_1_OFS       ( 1)                            /*!< HALT_T16_1 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_1           ((uint32_t)0x00000002)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_T16_2_OFS       ( 2)                            /*!< HALT_T16_2 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_2           ((uint32_t)0x00000004)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_T16_3_OFS       ( 3)                            /*!< HALT_T16_3 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_3           ((uint32_t)0x00000008)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_T32_0_OFS       ( 4)                            /*!< HALT_T32_0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T32_0           ((uint32_t)0x00000010)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUA0_OFS        ( 5)                            /*!< HALT_eUA0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA0            ((uint32_t)0x00000020)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUA1_OFS        ( 6)                            /*!< HALT_eUA1 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA1            ((uint32_t)0x00000040)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUA2_OFS        ( 7)                            /*!< HALT_eUA2 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA2            ((uint32_t)0x00000080)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUA3_OFS        ( 8)                            /*!< HALT_eUA3 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA3            ((uint32_t)0x00000100)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUB0_OFS        ( 9)                            /*!< HALT_eUB0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB0            ((uint32_t)0x00000200)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUB1_OFS        (10)                            /*!< HALT_eUB1 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB1            ((uint32_t)0x00000400)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUB2_OFS        (11)                            /*!< HALT_eUB2 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB2            ((uint32_t)0x00000800)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_EUB3_OFS        (12)                            /*!< HALT_eUB3 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB3            ((uint32_t)0x00001000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_ADC_OFS         (13)                            /*!< HALT_ADC Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_ADC             ((uint32_t)0x00002000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_WDT_OFS         (14)                            /*!< HALT_WDT Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_WDT             ((uint32_t)0x00004000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_PERIHALT_CTL_HALT_DMA_OFS         (15)                            /*!< HALT_DMA Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_DMA             ((uint32_t)0x00008000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_SRAM_BANKEN_BNK0_EN_OFS           ( 0)                            /*!< BNK0_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK0_EN               ((uint32_t)0x00000001)          /*!< SRAM Bank0 enable */
#define SYSCTL_SRAM_BANKEN_BNK1_EN_OFS           ( 1)                            /*!< BNK1_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK1_EN               ((uint32_t)0x00000002)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_BNK2_EN_OFS           ( 2)                            /*!< BNK2_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK2_EN               ((uint32_t)0x00000004)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_BNK3_EN_OFS           ( 3)                            /*!< BNK3_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK3_EN               ((uint32_t)0x00000008)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_BNK4_EN_OFS           ( 4)                            /*!< BNK4_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK4_EN               ((uint32_t)0x00000010)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_BNK5_EN_OFS           ( 5)                            /*!< BNK5_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK5_EN               ((uint32_t)0x00000020)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_BNK6_EN_OFS           ( 6)                            /*!< BNK6_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK6_EN               ((uint32_t)0x00000040)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_BNK7_EN_OFS           ( 7)                            /*!< BNK7_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK7_EN               ((uint32_t)0x00000080)          /*!< SRAM Bank1 enable */
#define SYSCTL_SRAM_BANKEN_SRAM_RDY_OFS          (16)                            /*!< SRAM_RDY Bit Offset */
#define SYSCTL_SRAM_BANKEN_SRAM_RDY              ((uint32_t)0x00010000)          /*!< SRAM ready */
#define SYSCTL_SRAM_BANKRET_BNK0_RET_OFS         ( 0)                            /*!< BNK0_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK0_RET             ((uint32_t)0x00000001)          /*!< Bank0 retention */
#define SYSCTL_SRAM_BANKRET_BNK1_RET_OFS         ( 1)                            /*!< BNK1_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK1_RET             ((uint32_t)0x00000002)          /*!< Bank1 retention */
#define SYSCTL_SRAM_BANKRET_BNK2_RET_OFS         ( 2)                            /*!< BNK2_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK2_RET             ((uint32_t)0x00000004)          /*!< Bank2 retention */
#define SYSCTL_SRAM_BANKRET_BNK3_RET_OFS         ( 3)                            /*!< BNK3_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK3_RET             ((uint32_t)0x00000008)          /*!< Bank3 retention */
#define SYSCTL_SRAM_BANKRET_BNK4_RET_OFS         ( 4)                            /*!< BNK4_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK4_RET             ((uint32_t)0x00000010)          /*!< Bank4 retention */
#define SYSCTL_SRAM_BANKRET_BNK5_RET_OFS         ( 5)                            /*!< BNK5_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK5_RET             ((uint32_t)0x00000020)          /*!< Bank5 retention */
#define SYSCTL_SRAM_BANKRET_BNK6_RET_OFS         ( 6)                            /*!< BNK6_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK6_RET             ((uint32_t)0x00000040)          /*!< Bank6 retention */
#define SYSCTL_SRAM_BANKRET_BNK7_RET_OFS         ( 7)                            /*!< BNK7_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK7_RET             ((uint32_t)0x00000080)          /*!< Bank7 retention */
#define SYSCTL_SRAM_BANKRET_SRAM_RDY_OFS         (16)                            /*!< SRAM_RDY Bit Offset */
#define SYSCTL_SRAM_BANKRET_SRAM_RDY             ((uint32_t)0x00010000)          /*!< SRAM ready */
#define SYSCTL_DIO_GLTFLT_CTL_GLTCH_EN_OFS       ( 0)                            /*!< GLTCH_EN Bit Offset */
#define SYSCTL_DIO_GLTFLT_CTL_GLTCH_EN           ((uint32_t)0x00000001)          /*!< Glitch filter enable */
#define SYSCTL_SECDATA_UNLOCK_UNLKEY_OFS         ( 0)                            /*!< UNLKEY Bit Offset */
#define SYSCTL_SECDATA_UNLOCK_UNLKEY_MASK        ((uint32_t)0x0000FFFF)          /*!< UNLKEY Bit Mask */
#define SYSCTL_MASTER_UNLOCK_UNLKEY_OFS          ( 0)                            /*!< UNLKEY Bit Offset */
#define SYSCTL_MASTER_UNLOCK_UNLKEY_MASK         ((uint32_t)0x0000FFFF)          /*!< UNLKEY Bit Mask */
#define SYSCTL_RESET_REQ_POR_OFS                 ( 0)                            /*!< POR Bit Offset */
#define SYSCTL_RESET_REQ_POR                     ((uint32_t)0x00000001)          /*!< Generate POR */
#define SYSCTL_RESET_REQ_REBOOT_OFS              ( 1)                            /*!< REBOOT Bit Offset */
#define SYSCTL_RESET_REQ_REBOOT                  ((uint32_t)0x00000002)          /*!< Generate Reboot_Reset */
#define SYSCTL_RESET_REQ_WKEY_OFS                ( 8)                            /*!< WKEY Bit Offset */
#define SYSCTL_RESET_REQ_WKEY_MASK               ((uint32_t)0x0000FF00)          /*!< WKEY Bit Mask */
#define SYSCTL_RESET_STATOVER_SOFT_OFS           ( 0)                            /*!< SOFT Bit Offset */
#define SYSCTL_RESET_STATOVER_SOFT               ((uint32_t)0x00000001)          /*!< Indicates if SOFT Reset is active */
#define SYSCTL_RESET_STATOVER_HARD_OFS           ( 1)                            /*!< HARD Bit Offset */
#define SYSCTL_RESET_STATOVER_HARD               ((uint32_t)0x00000002)          /*!< Indicates if HARD Reset is active */
#define SYSCTL_RESET_STATOVER_REBOOT_OFS         ( 2)                            /*!< REBOOT Bit Offset */
#define SYSCTL_RESET_STATOVER_REBOOT             ((uint32_t)0x00000004)          /*!< Indicates if Reboot Reset is active */
#define SYSCTL_RESET_STATOVER_SOFT_OVER_OFS      ( 8)                            /*!< SOFT_OVER Bit Offset */
#define SYSCTL_RESET_STATOVER_SOFT_OVER          ((uint32_t)0x00000100)          /*!< SOFT_Reset overwrite request */
#define SYSCTL_RESET_STATOVER_HARD_OVER_OFS      ( 9)                            /*!< HARD_OVER Bit Offset */
#define SYSCTL_RESET_STATOVER_HARD_OVER          ((uint32_t)0x00000200)          /*!< HARD_Reset overwrite request */
#define SYSCTL_RESET_STATOVER_RBT_OVER_OFS       (10)                            /*!< RBT_OVER Bit Offset */
#define SYSCTL_RESET_STATOVER_RBT_OVER           ((uint32_t)0x00000400)          /*!< Reboot Reset overwrite request */
#define SYSCTL_REBOOT_CTL_WKEY_VAL              ((uint32_t)0x00006900)          /*!< Key value to enable writes to bit 0 */
#define TIMER32_CONTROL_ONESHOT_OFS              ( 0)                            /*!< ONESHOT Bit Offset */
#define TIMER32_CONTROL_ONESHOT                  ((uint32_t)0x00000001)          /*!< Selects one-shot or wrapping counter mode */
#define TIMER32_CONTROL_SIZE_OFS                 ( 1)                            /*!< SIZE Bit Offset */
#define TIMER32_CONTROL_SIZE                     ((uint32_t)0x00000002)          /*!< Selects 16 or 32 bit counter operation */
#define TIMER32_CONTROL_PRESCALE_OFS             ( 2)                            /*!< PRESCALE Bit Offset */
#define TIMER32_CONTROL_PRESCALE_MASK            ((uint32_t)0x0000000C)          /*!< PRESCALE Bit Mask */
#define TIMER32_CONTROL_PRESCALE0                ((uint32_t)0x00000004)          /*!< PRESCALE Bit 0 */
#define TIMER32_CONTROL_PRESCALE1                ((uint32_t)0x00000008)          /*!< PRESCALE Bit 1 */
#define TIMER32_CONTROL_PRESCALE_0               ((uint32_t)0x00000000)          /*!< 0 stages of prescale, clock is divided by 1 */
#define TIMER32_CONTROL_PRESCALE_1               ((uint32_t)0x00000004)          /*!< 4 stages of prescale, clock is divided by 16 */
#define TIMER32_CONTROL_PRESCALE_2               ((uint32_t)0x00000008)          /*!< 8 stages of prescale, clock is divided by 256 */
#define TIMER32_CONTROL_IE_OFS                   ( 5)                            /*!< IE Bit Offset */
#define TIMER32_CONTROL_IE                       ((uint32_t)0x00000020)          /*!< Interrupt enable bit */
#define TIMER32_CONTROL_MODE_OFS                 ( 6)                            /*!< MODE Bit Offset */
#define TIMER32_CONTROL_MODE                     ((uint32_t)0x00000040)          /*!< Mode bit */
#define TIMER32_CONTROL_ENABLE_OFS               ( 7)                            /*!< ENABLE Bit Offset */
#define TIMER32_CONTROL_ENABLE                   ((uint32_t)0x00000080)
#define TIMER32_RIS_RAW_IFG_OFS                  ( 0)                            /*!< RAW_IFG Bit Offset */
#define TIMER32_RIS_RAW_IFG                      ((uint32_t)0x00000001)          /*!< Raw interrupt status */
#define TIMER32_MIS_IFG_OFS                      ( 0)                            /*!< IFG Bit Offset */
#define TIMER32_MIS_IFG                          ((uint32_t)0x00000001)          /*!< Enabled interrupt status */
#define TIMER_A_CTL_IFG_OFS                      ( 0)                            /*!< TAIFG Bit Offset */
#define TIMER_A_CTL_IFG                          ((uint16_t)0x0001)              /*!< TimerA interrupt flag */
#define TIMER_A_CTL_IE_OFS                       ( 1)                            /*!< TAIE Bit Offset */
#define TIMER_A_CTL_IE                           ((uint16_t)0x0002)              /*!< TimerA interrupt enable */
#define TIMER_A_CTL_CLR_OFS                      ( 2)                            /*!< TACLR Bit Offset */
#define TIMER_A_CTL_CLR                          ((uint16_t)0x0004)              /*!< TimerA clear */
#define TIMER_A_CTL_MC_OFS                       ( 4)                            /*!< MC Bit Offset */
#define TIMER_A_CTL_MC_MASK                      ((uint16_t)0x0030)              /*!< MC Bit Mask */
#define TIMER_A_CTL_MC0                          ((uint16_t)0x0010)              /*!< MC Bit 0 */
#define TIMER_A_CTL_MC1                          ((uint16_t)0x0020)              /*!< MC Bit 1 */
#define TIMER_A_CTL_MC_0                         ((uint16_t)0x0000)              /*!< Stop mode: Timer is halted */
#define TIMER_A_CTL_MC_1                         ((uint16_t)0x0010)              /*!< Up mode: Timer counts up to TAxCCR0 */
#define TIMER_A_CTL_MC_2                         ((uint16_t)0x0020)              /*!< Continuous mode: Timer counts up to 0FFFFh */
#define TIMER_A_CTL_MC_3                         ((uint16_t)0x0030)              /*!< Up/down mode: Timer counts up to TAxCCR0 then down to 0000h */
#define TIMER_A_CTL_MC__STOP                     ((uint16_t)0x0000)              /*!< Stop mode: Timer is halted */
#define TIMER_A_CTL_MC__UP                       ((uint16_t)0x0010)              /*!< Up mode: Timer counts up to TAxCCR0 */
#define TIMER_A_CTL_MC__CONTINUOUS               ((uint16_t)0x0020)              /*!< Continuous mode: Timer counts up to 0FFFFh */
#define TIMER_A_CTL_MC__UPDOWN                   ((uint16_t)0x0030)              /*!< Up/down mode: Timer counts up to TAxCCR0 then down to 0000h */
#define TIMER_A_CTL_ID_OFS                       ( 6)                            /*!< ID Bit Offset */
#define TIMER_A_CTL_ID_MASK                      ((uint16_t)0x00C0)              /*!< ID Bit Mask */
#define TIMER_A_CTL_ID0                          ((uint16_t)0x0040)              /*!< ID Bit 0 */
#define TIMER_A_CTL_ID1                          ((uint16_t)0x0080)              /*!< ID Bit 1 */
#define TIMER_A_CTL_ID_0                         ((uint16_t)0x0000)              /*!< /1 */
#define TIMER_A_CTL_ID_1                         ((uint16_t)0x0040)              /*!< /2 */
#define TIMER_A_CTL_ID_2                         ((uint16_t)0x0080)              /*!< /4 */
#define TIMER_A_CTL_ID_3                         ((uint16_t)0x00C0)              /*!< /8 */
#define TIMER_A_CTL_ID__1                        ((uint16_t)0x0000)              /*!< /1 */
#define TIMER_A_CTL_ID__2                        ((uint16_t)0x0040)              /*!< /2 */
#define TIMER_A_CTL_ID__4                        ((uint16_t)0x0080)              /*!< /4 */
#define TIMER_A_CTL_ID__8                        ((uint16_t)0x00C0)              /*!< /8 */
#define TIMER_A_CTL_SSEL_OFS                     ( 8)                            /*!< TASSEL Bit Offset */
#define TIMER_A_CTL_SSEL_MASK                    ((uint16_t)0x0300)              /*!< TASSEL Bit Mask */
#define TIMER_A_CTL_SSEL0                        ((uint16_t)0x0100)              /*!< SSEL Bit 0 */
#define TIMER_A_CTL_SSEL1                        ((uint16_t)0x0200)              /*!< SSEL Bit 1 */
#define TIMER_A_CTL_TASSEL_0                     ((uint16_t)0x0000)              /*!< TAxCLK */
#define TIMER_A_CTL_TASSEL_1                     ((uint16_t)0x0100)              /*!< ACLK */
#define TIMER_A_CTL_TASSEL_2                     ((uint16_t)0x0200)              /*!< SMCLK */
#define TIMER_A_CTL_TASSEL_3                     ((uint16_t)0x0300)              /*!< INCLK */
#define TIMER_A_CTL_SSEL__TACLK                  ((uint16_t)0x0000)              /*!< TAxCLK */
#define TIMER_A_CTL_SSEL__ACLK                   ((uint16_t)0x0100)              /*!< ACLK */
#define TIMER_A_CTL_SSEL__SMCLK                  ((uint16_t)0x0200)              /*!< SMCLK */
#define TIMER_A_CTL_SSEL__INCLK                  ((uint16_t)0x0300)              /*!< INCLK */
#define TIMER_A_CCTLN_CCIFG_OFS                  ( 0)                            /*!< CCIFG Bit Offset */
#define TIMER_A_CCTLN_CCIFG                      ((uint16_t)0x0001)              /*!< Capture/compare interrupt flag */
#define TIMER_A_CCTLN_COV_OFS                    ( 1)                            /*!< COV Bit Offset */
#define TIMER_A_CCTLN_COV                        ((uint16_t)0x0002)              /*!< Capture overflow */
#define TIMER_A_CCTLN_OUT_OFS                    ( 2)                            /*!< OUT Bit Offset */
#define TIMER_A_CCTLN_OUT                        ((uint16_t)0x0004)              /*!< Output */
#define TIMER_A_CCTLN_CCI_OFS                    ( 3)                            /*!< CCI Bit Offset */
#define TIMER_A_CCTLN_CCI                        ((uint16_t)0x0008)              /*!< Capture/compare input */
#define TIMER_A_CCTLN_CCIE_OFS                   ( 4)                            /*!< CCIE Bit Offset */
#define TIMER_A_CCTLN_CCIE                       ((uint16_t)0x0010)              /*!< Capture/compare interrupt enable */
#define TIMER_A_CCTLN_OUTMOD_OFS                 ( 5)                            /*!< OUTMOD Bit Offset */
#define TIMER_A_CCTLN_OUTMOD_MASK                ((uint16_t)0x00E0)              /*!< OUTMOD Bit Mask */
#define TIMER_A_CCTLN_OUTMOD0                    ((uint16_t)0x0020)              /*!< OUTMOD Bit 0 */
#define TIMER_A_CCTLN_OUTMOD1                    ((uint16_t)0x0040)              /*!< OUTMOD Bit 1 */
#define TIMER_A_CCTLN_OUTMOD2                    ((uint16_t)0x0080)              /*!< OUTMOD Bit 2 */
#define TIMER_A_CCTLN_OUTMOD_0                   ((uint16_t)0x0000)              /*!< OUT bit value */
#define TIMER_A_CCTLN_OUTMOD_1                   ((uint16_t)0x0020)              /*!< Set */
#define TIMER_A_CCTLN_OUTMOD_2                   ((uint16_t)0x0040)              /*!< Toggle/reset */
#define TIMER_A_CCTLN_OUTMOD_3                   ((uint16_t)0x0060)              /*!< Set/reset */
#define TIMER_A_CCTLN_OUTMOD_4                   ((uint16_t)0x0080)              /*!< Toggle */
#define TIMER_A_CCTLN_OUTMOD_5                   ((uint16_t)0x00A0)              /*!< Reset */
#define TIMER_A_CCTLN_OUTMOD_6                   ((uint16_t)0x00C0)              /*!< Toggle/set */
#define TIMER_A_CCTLN_OUTMOD_7                   ((uint16_t)0x00E0)              /*!< Reset/set */
#define TIMER_A_CCTLN_CAP_OFS                    ( 8)                            /*!< CAP Bit Offset */
#define TIMER_A_CCTLN_CAP                        ((uint16_t)0x0100)              /*!< Capture mode */
#define TIMER_A_CCTLN_SCCI_OFS                   (10)                            /*!< SCCI Bit Offset */
#define TIMER_A_CCTLN_SCCI                       ((uint16_t)0x0400)              /*!< Synchronized capture/compare input */
#define TIMER_A_CCTLN_SCS_OFS                    (11)                            /*!< SCS Bit Offset */
#define TIMER_A_CCTLN_SCS                        ((uint16_t)0x0800)              /*!< Synchronize capture source */
#define TIMER_A_CCTLN_CCIS_OFS                   (12)                            /*!< CCIS Bit Offset */
#define TIMER_A_CCTLN_CCIS_MASK                  ((uint16_t)0x3000)              /*!< CCIS Bit Mask */
#define TIMER_A_CCTLN_CCIS0                      ((uint16_t)0x1000)              /*!< CCIS Bit 0 */
#define TIMER_A_CCTLN_CCIS1                      ((uint16_t)0x2000)              /*!< CCIS Bit 1 */
#define TIMER_A_CCTLN_CCIS_0                     ((uint16_t)0x0000)              /*!< CCIxA */
#define TIMER_A_CCTLN_CCIS_1                     ((uint16_t)0x1000)              /*!< CCIxB */
#define TIMER_A_CCTLN_CCIS_2                     ((uint16_t)0x2000)              /*!< GND */
#define TIMER_A_CCTLN_CCIS_3                     ((uint16_t)0x3000)              /*!< VCC */
#define TIMER_A_CCTLN_CCIS__CCIA                 ((uint16_t)0x0000)              /*!< CCIxA */
#define TIMER_A_CCTLN_CCIS__CCIB                 ((uint16_t)0x1000)              /*!< CCIxB */
#define TIMER_A_CCTLN_CCIS__GND                  ((uint16_t)0x2000)              /*!< GND */
#define TIMER_A_CCTLN_CCIS__VCC                  ((uint16_t)0x3000)              /*!< VCC */
#define TIMER_A_CCTLN_CM_OFS                     (14)                            /*!< CM Bit Offset */
#define TIMER_A_CCTLN_CM_MASK                    ((uint16_t)0xC000)              /*!< CM Bit Mask */
#define TIMER_A_CCTLN_CM0                        ((uint16_t)0x4000)              /*!< CM Bit 0 */
#define TIMER_A_CCTLN_CM1                        ((uint16_t)0x8000)              /*!< CM Bit 1 */
#define TIMER_A_CCTLN_CM_0                       ((uint16_t)0x0000)              /*!< No capture */
#define TIMER_A_CCTLN_CM_1                       ((uint16_t)0x4000)              /*!< Capture on rising edge */
#define TIMER_A_CCTLN_CM_2                       ((uint16_t)0x8000)              /*!< Capture on falling edge */
#define TIMER_A_CCTLN_CM_3                       ((uint16_t)0xC000)              /*!< Capture on both rising and falling edges */
#define TIMER_A_CCTLN_CM__NONE                   ((uint16_t)0x0000)              /*!< No capture */
#define TIMER_A_CCTLN_CM__RISING                 ((uint16_t)0x4000)              /*!< Capture on rising edge */
#define TIMER_A_CCTLN_CM__FALLING                ((uint16_t)0x8000)              /*!< Capture on falling edge */
#define TIMER_A_CCTLN_CM__BOTH                   ((uint16_t)0xC000)              /*!< Capture on both rising and falling edges */
#define TIMER_A_EX0_IDEX_OFS                     ( 0)                            /*!< TAIDEX Bit Offset */
#define TIMER_A_EX0_IDEX_MASK                    ((uint16_t)0x0007)              /*!< TAIDEX Bit Mask */
#define TIMER_A_EX0_IDEX0                        ((uint16_t)0x0001)              /*!< IDEX Bit 0 */
#define TIMER_A_EX0_IDEX1                        ((uint16_t)0x0002)              /*!< IDEX Bit 1 */
#define TIMER_A_EX0_IDEX2                        ((uint16_t)0x0004)              /*!< IDEX Bit 2 */
#define TIMER_A_EX0_TAIDEX_0                     ((uint16_t)0x0000)              /*!< Divide by 1 */
#define TIMER_A_EX0_TAIDEX_1                     ((uint16_t)0x0001)              /*!< Divide by 2 */
#define TIMER_A_EX0_TAIDEX_2                     ((uint16_t)0x0002)              /*!< Divide by 3 */
#define TIMER_A_EX0_TAIDEX_3                     ((uint16_t)0x0003)              /*!< Divide by 4 */
#define TIMER_A_EX0_TAIDEX_4                     ((uint16_t)0x0004)              /*!< Divide by 5 */
#define TIMER_A_EX0_TAIDEX_5                     ((uint16_t)0x0005)              /*!< Divide by 6 */
#define TIMER_A_EX0_TAIDEX_6                     ((uint16_t)0x0006)              /*!< Divide by 7 */
#define TIMER_A_EX0_TAIDEX_7                     ((uint16_t)0x0007)              /*!< Divide by 8 */
#define TIMER_A_EX0_IDEX__1                      ((uint16_t)0x0000)              /*!< Divide by 1 */
#define TIMER_A_EX0_IDEX__2                      ((uint16_t)0x0001)              /*!< Divide by 2 */
#define TIMER_A_EX0_IDEX__3                      ((uint16_t)0x0002)              /*!< Divide by 3 */
#define TIMER_A_EX0_IDEX__4                      ((uint16_t)0x0003)              /*!< Divide by 4 */
#define TIMER_A_EX0_IDEX__5                      ((uint16_t)0x0004)              /*!< Divide by 5 */
#define TIMER_A_EX0_IDEX__6                      ((uint16_t)0x0005)              /*!< Divide by 6 */
#define TIMER_A_EX0_IDEX__7                      ((uint16_t)0x0006)              /*!< Divide by 7 */
#define TIMER_A_EX0_IDEX__8                      ((uint16_t)0x0007)              /*!< Divide by 8 */
#define TLV_TAG_RESERVED1                                   1
#define TLV_TAG_RESERVED2                                   2
#define TLV_TAG_CS                                          3
#define TLV_TAG_FLASHCTL                                    4
#define TLV_TAG_ADC14                                       5
#define TLV_TAG_RESERVED6                                   6
#define TLV_TAG_RESERVED7                                   7
#define TLV_TAG_REF                                         8
#define TLV_TAG_RESERVED9                                   9
#define TLV_TAG_RESERVED10                                 10
#define TLV_TAG_DEVINFO                                    11
#define TLV_TAG_DIEREC                                     12
#define TLV_TAG_RANDNUM                                    13
#define TLV_TAG_RESERVED14                                 14
#define TLV_TAG_BSL                                        15
#define TLV_TAG_END                                        (0x0BD0E11D)
#define WDT_A_CTL_IS_OFS                         ( 0)                            /*!< WDTIS Bit Offset */
#define WDT_A_CTL_IS_MASK                        ((uint16_t)0x0007)              /*!< WDTIS Bit Mask */
#define WDT_A_CTL_IS0                            ((uint16_t)0x0001)              /*!< IS Bit 0 */
#define WDT_A_CTL_IS1                            ((uint16_t)0x0002)              /*!< IS Bit 1 */
#define WDT_A_CTL_IS2                            ((uint16_t)0x0004)              /*!< IS Bit 2 */
#define WDT_A_CTL_IS_0                           ((uint16_t)0x0000)              /*!< Watchdog clock source / (2^(31)) (18:12:16 at 32.768 kHz) */
#define WDT_A_CTL_IS_1                           ((uint16_t)0x0001)              /*!< Watchdog clock source /(2^(27)) (01:08:16 at 32.768 kHz) */
#define WDT_A_CTL_IS_2                           ((uint16_t)0x0002)              /*!< Watchdog clock source /(2^(23)) (00:04:16 at 32.768 kHz) */
#define WDT_A_CTL_IS_3                           ((uint16_t)0x0003)              /*!< Watchdog clock source /(2^(19)) (00:00:16 at 32.768 kHz) */
#define WDT_A_CTL_IS_4                           ((uint16_t)0x0004)              /*!< Watchdog clock source /(2^(15)) (1 s at 32.768 kHz) */
#define WDT_A_CTL_IS_5                           ((uint16_t)0x0005)              /*!< Watchdog clock source / (2^(13)) (250 ms at 32.768 kHz) */
#define WDT_A_CTL_IS_6                           ((uint16_t)0x0006)              /*!< Watchdog clock source / (2^(9)) (15.625 ms at 32.768 kHz) */
#define WDT_A_CTL_IS_7                           ((uint16_t)0x0007)              /*!< Watchdog clock source / (2^(6)) (1.95 ms at 32.768 kHz) */
#define WDT_A_CTL_CNTCL_OFS                      ( 3)                            /*!< WDTCNTCL Bit Offset */
#define WDT_A_CTL_CNTCL                          ((uint16_t)0x0008)              /*!< Watchdog timer counter clear */
#define WDT_A_CTL_TMSEL_OFS                      ( 4)                            /*!< WDTTMSEL Bit Offset */
#define WDT_A_CTL_TMSEL                          ((uint16_t)0x0010)              /*!< Watchdog timer mode select */
#define WDT_A_CTL_SSEL_OFS                       ( 5)                            /*!< WDTSSEL Bit Offset */
#define WDT_A_CTL_SSEL_MASK                      ((uint16_t)0x0060)              /*!< WDTSSEL Bit Mask */
#define WDT_A_CTL_SSEL0                          ((uint16_t)0x0020)              /*!< SSEL Bit 0 */
#define WDT_A_CTL_SSEL1                          ((uint16_t)0x0040)              /*!< SSEL Bit 1 */
#define WDT_A_CTL_SSEL_0                         ((uint16_t)0x0000)              /*!< SMCLK */
#define WDT_A_CTL_SSEL_1                         ((uint16_t)0x0020)              /*!< ACLK */
#define WDT_A_CTL_SSEL_2                         ((uint16_t)0x0040)              /*!< VLOCLK */
#define WDT_A_CTL_SSEL_3                         ((uint16_t)0x0060)              /*!< BCLK */
#define WDT_A_CTL_SSEL__SMCLK                    ((uint16_t)0x0000)              /*!< SMCLK */
#define WDT_A_CTL_SSEL__ACLK                     ((uint16_t)0x0020)              /*!< ACLK */
#define WDT_A_CTL_SSEL__VLOCLK                   ((uint16_t)0x0040)              /*!< VLOCLK */
#define WDT_A_CTL_SSEL__BCLK                     ((uint16_t)0x0060)              /*!< BCLK */
#define WDT_A_CTL_HOLD_OFS                       ( 7)                            /*!< WDTHOLD Bit Offset */
#define WDT_A_CTL_HOLD                           ((uint16_t)0x0080)              /*!< Watchdog timer hold */
#define WDT_A_CTL_PW_OFS                         ( 8)                            /*!< WDTPW Bit Offset */
#define WDT_A_CTL_PW_MASK                        ((uint16_t)0xFF00)              /*!< WDTPW Bit Mask */
#define WDT_A_CTL_PW                              ((uint16_t)0x5A00)              /*!< WDT Key Value for WDT write access */
#define FLCTL_A_POWER_STAT_PSTAT_OFS             ( 0)                            /*!< PSTAT Bit Offset */
#define FLCTL_A_POWER_STAT_PSTAT_MASK            ((uint32_t)0x00000007)          /*!< PSTAT Bit Mask */
#define FLCTL_A_POWER_STAT_PSTAT0                ((uint32_t)0x00000001)          /*!< PSTAT Bit 0 */
#define FLCTL_A_POWER_STAT_PSTAT1                ((uint32_t)0x00000002)          /*!< PSTAT Bit 1 */
#define FLCTL_A_POWER_STAT_PSTAT2                ((uint32_t)0x00000004)          /*!< PSTAT Bit 2 */
#define FLCTL_A_POWER_STAT_PSTAT_0               ((uint32_t)0x00000000)          /*!< Flash IP in power-down mode */
#define FLCTL_A_POWER_STAT_PSTAT_1               ((uint32_t)0x00000001)          /*!< Flash IP Vdd domain power-up in progress */
#define FLCTL_A_POWER_STAT_PSTAT_2               ((uint32_t)0x00000002)          /*!< PSS LDO_GOOD, IREF_OK and VREF_OK check in progress */
#define FLCTL_A_POWER_STAT_PSTAT_3               ((uint32_t)0x00000003)          /*!< Flash IP SAFE_LV check in progress */
#define FLCTL_A_POWER_STAT_PSTAT_4               ((uint32_t)0x00000004)          /*!< Flash IP Active */
#define FLCTL_A_POWER_STAT_PSTAT_5               ((uint32_t)0x00000005)          /*!< Flash IP Active in Low-Frequency Active and Low-Frequency LPM0 modes. */
#define FLCTL_A_POWER_STAT_PSTAT_6               ((uint32_t)0x00000006)          /*!< Flash IP in Standby mode */
#define FLCTL_A_POWER_STAT_PSTAT_7               ((uint32_t)0x00000007)          /*!< Flash IP in Current mirror boost state */
#define FLCTL_A_POWER_STAT_LDOSTAT_OFS           ( 3)                            /*!< LDOSTAT Bit Offset */
#define FLCTL_A_POWER_STAT_LDOSTAT               ((uint32_t)0x00000008)          /*!< PSS FLDO GOOD status */
#define FLCTL_A_POWER_STAT_VREFSTAT_OFS          ( 4)                            /*!< VREFSTAT Bit Offset */
#define FLCTL_A_POWER_STAT_VREFSTAT              ((uint32_t)0x00000010)          /*!< PSS VREF stable status */
#define FLCTL_A_POWER_STAT_IREFSTAT_OFS          ( 5)                            /*!< IREFSTAT Bit Offset */
#define FLCTL_A_POWER_STAT_IREFSTAT              ((uint32_t)0x00000020)          /*!< PSS IREF stable status */
#define FLCTL_A_POWER_STAT_TRIMSTAT_OFS          ( 6)                            /*!< TRIMSTAT Bit Offset */
#define FLCTL_A_POWER_STAT_TRIMSTAT              ((uint32_t)0x00000040)          /*!< PSS trim done status */
#define FLCTL_A_POWER_STAT_RD_2T_OFS             ( 7)                            /*!< RD_2T Bit Offset */
#define FLCTL_A_POWER_STAT_RD_2T                 ((uint32_t)0x00000080)          /*!< Indicates if Flash is being accessed in 2T mode */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_OFS          ( 0)                            /*!< RD_MODE Bit Offset */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_MASK         ((uint32_t)0x0000000F)          /*!< RD_MODE Bit Mask */
#define FLCTL_A_BANK0_RDCTL_RD_MODE0             ((uint32_t)0x00000001)          /*!< RD_MODE Bit 0 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE1             ((uint32_t)0x00000002)          /*!< RD_MODE Bit 1 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE2             ((uint32_t)0x00000004)          /*!< RD_MODE Bit 2 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE3             ((uint32_t)0x00000008)          /*!< RD_MODE Bit 3 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_0            ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_1            ((uint32_t)0x00000001)          /*!< Read Margin 0 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_2            ((uint32_t)0x00000002)          /*!< Read Margin 1 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_3            ((uint32_t)0x00000003)          /*!< Program Verify */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_4            ((uint32_t)0x00000004)          /*!< Erase Verify */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_5            ((uint32_t)0x00000005)          /*!< Leakage Verify */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_9            ((uint32_t)0x00000009)          /*!< Read Margin 0B */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_10           ((uint32_t)0x0000000A)          /*!< Read Margin 1B */
#define FLCTL_A_BANK0_RDCTL_BUFI_OFS             ( 4)                            /*!< BUFI Bit Offset */
#define FLCTL_A_BANK0_RDCTL_BUFI                 ((uint32_t)0x00000010)          /*!< Enables read buffering feature for instruction fetches to this Bank */
#define FLCTL_A_BANK0_RDCTL_BUFD_OFS             ( 5)                            /*!< BUFD Bit Offset */
#define FLCTL_A_BANK0_RDCTL_BUFD                 ((uint32_t)0x00000020)          /*!< Enables read buffering feature for data reads to this Bank */
#define FLCTL_A_BANK0_RDCTL_WAIT_OFS             (12)                            /*!< WAIT Bit Offset */
#define FLCTL_A_BANK0_RDCTL_WAIT_MASK            ((uint32_t)0x0000F000)          /*!< WAIT Bit Mask */
#define FLCTL_A_BANK0_RDCTL_WAIT0                ((uint32_t)0x00001000)          /*!< WAIT Bit 0 */
#define FLCTL_A_BANK0_RDCTL_WAIT1                ((uint32_t)0x00002000)          /*!< WAIT Bit 1 */
#define FLCTL_A_BANK0_RDCTL_WAIT2                ((uint32_t)0x00004000)          /*!< WAIT Bit 2 */
#define FLCTL_A_BANK0_RDCTL_WAIT3                ((uint32_t)0x00008000)          /*!< WAIT Bit 3 */
#define FLCTL_A_BANK0_RDCTL_WAIT_0               ((uint32_t)0x00000000)          /*!< 0 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_1               ((uint32_t)0x00001000)          /*!< 1 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_2               ((uint32_t)0x00002000)          /*!< 2 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_3               ((uint32_t)0x00003000)          /*!< 3 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_4               ((uint32_t)0x00004000)          /*!< 4 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_5               ((uint32_t)0x00005000)          /*!< 5 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_6               ((uint32_t)0x00006000)          /*!< 6 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_7               ((uint32_t)0x00007000)          /*!< 7 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_8               ((uint32_t)0x00008000)          /*!< 8 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_9               ((uint32_t)0x00009000)          /*!< 9 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_10              ((uint32_t)0x0000A000)          /*!< 10 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_11              ((uint32_t)0x0000B000)          /*!< 11 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_12              ((uint32_t)0x0000C000)          /*!< 12 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_13              ((uint32_t)0x0000D000)          /*!< 13 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_14              ((uint32_t)0x0000E000)          /*!< 14 wait states */
#define FLCTL_A_BANK0_RDCTL_WAIT_15              ((uint32_t)0x0000F000)          /*!< 15 wait states */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_OFS   (16)                            /*!< RD_MODE_STATUS Bit Offset */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_MASK  ((uint32_t)0x000F0000)          /*!< RD_MODE_STATUS Bit Mask */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS0      ((uint32_t)0x00010000)          /*!< RD_MODE_STATUS Bit 0 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS1      ((uint32_t)0x00020000)          /*!< RD_MODE_STATUS Bit 1 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS2      ((uint32_t)0x00040000)          /*!< RD_MODE_STATUS Bit 2 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS3      ((uint32_t)0x00080000)          /*!< RD_MODE_STATUS Bit 3 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_0     ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_1     ((uint32_t)0x00010000)          /*!< Read Margin 0 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_2     ((uint32_t)0x00020000)          /*!< Read Margin 1 */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_3     ((uint32_t)0x00030000)          /*!< Program Verify */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_4     ((uint32_t)0x00040000)          /*!< Erase Verify */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_5     ((uint32_t)0x00050000)          /*!< Leakage Verify */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_9     ((uint32_t)0x00090000)          /*!< Read Margin 0B */
#define FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_10    ((uint32_t)0x000A0000)          /*!< Read Margin 1B */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_OFS          ( 0)                            /*!< RD_MODE Bit Offset */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_MASK         ((uint32_t)0x0000000F)          /*!< RD_MODE Bit Mask */
#define FLCTL_A_BANK1_RDCTL_RD_MODE0             ((uint32_t)0x00000001)          /*!< RD_MODE Bit 0 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE1             ((uint32_t)0x00000002)          /*!< RD_MODE Bit 1 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE2             ((uint32_t)0x00000004)          /*!< RD_MODE Bit 2 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE3             ((uint32_t)0x00000008)          /*!< RD_MODE Bit 3 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_0            ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_1            ((uint32_t)0x00000001)          /*!< Read Margin 0 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_2            ((uint32_t)0x00000002)          /*!< Read Margin 1 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_3            ((uint32_t)0x00000003)          /*!< Program Verify */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_4            ((uint32_t)0x00000004)          /*!< Erase Verify */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_5            ((uint32_t)0x00000005)          /*!< Leakage Verify */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_9            ((uint32_t)0x00000009)          /*!< Read Margin 0B */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_10           ((uint32_t)0x0000000A)          /*!< Read Margin 1B */
#define FLCTL_A_BANK1_RDCTL_BUFI_OFS             ( 4)                            /*!< BUFI Bit Offset */
#define FLCTL_A_BANK1_RDCTL_BUFI                 ((uint32_t)0x00000010)          /*!< Enables read buffering feature for instruction fetches to this Bank */
#define FLCTL_A_BANK1_RDCTL_BUFD_OFS             ( 5)                            /*!< BUFD Bit Offset */
#define FLCTL_A_BANK1_RDCTL_BUFD                 ((uint32_t)0x00000020)          /*!< Enables read buffering feature for data reads to this Bank */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_OFS   (16)                            /*!< RD_MODE_STATUS Bit Offset */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_MASK  ((uint32_t)0x000F0000)          /*!< RD_MODE_STATUS Bit Mask */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS0      ((uint32_t)0x00010000)          /*!< RD_MODE_STATUS Bit 0 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS1      ((uint32_t)0x00020000)          /*!< RD_MODE_STATUS Bit 1 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS2      ((uint32_t)0x00040000)          /*!< RD_MODE_STATUS Bit 2 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS3      ((uint32_t)0x00080000)          /*!< RD_MODE_STATUS Bit 3 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_0     ((uint32_t)0x00000000)          /*!< Normal read mode */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_1     ((uint32_t)0x00010000)          /*!< Read Margin 0 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_2     ((uint32_t)0x00020000)          /*!< Read Margin 1 */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_3     ((uint32_t)0x00030000)          /*!< Program Verify */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_4     ((uint32_t)0x00040000)          /*!< Erase Verify */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_5     ((uint32_t)0x00050000)          /*!< Leakage Verify */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_9     ((uint32_t)0x00090000)          /*!< Read Margin 0B */
#define FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_10    ((uint32_t)0x000A0000)          /*!< Read Margin 1B */
#define FLCTL_A_BANK1_RDCTL_WAIT_OFS             (12)                            /*!< WAIT Bit Offset */
#define FLCTL_A_BANK1_RDCTL_WAIT_MASK            ((uint32_t)0x0000F000)          /*!< WAIT Bit Mask */
#define FLCTL_A_BANK1_RDCTL_WAIT0                ((uint32_t)0x00001000)          /*!< WAIT Bit 0 */
#define FLCTL_A_BANK1_RDCTL_WAIT1                ((uint32_t)0x00002000)          /*!< WAIT Bit 1 */
#define FLCTL_A_BANK1_RDCTL_WAIT2                ((uint32_t)0x00004000)          /*!< WAIT Bit 2 */
#define FLCTL_A_BANK1_RDCTL_WAIT3                ((uint32_t)0x00008000)          /*!< WAIT Bit 3 */
#define FLCTL_A_BANK1_RDCTL_WAIT_0               ((uint32_t)0x00000000)          /*!< 0 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_1               ((uint32_t)0x00001000)          /*!< 1 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_2               ((uint32_t)0x00002000)          /*!< 2 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_3               ((uint32_t)0x00003000)          /*!< 3 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_4               ((uint32_t)0x00004000)          /*!< 4 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_5               ((uint32_t)0x00005000)          /*!< 5 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_6               ((uint32_t)0x00006000)          /*!< 6 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_7               ((uint32_t)0x00007000)          /*!< 7 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_8               ((uint32_t)0x00008000)          /*!< 8 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_9               ((uint32_t)0x00009000)          /*!< 9 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_10              ((uint32_t)0x0000A000)          /*!< 10 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_11              ((uint32_t)0x0000B000)          /*!< 11 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_12              ((uint32_t)0x0000C000)          /*!< 12 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_13              ((uint32_t)0x0000D000)          /*!< 13 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_14              ((uint32_t)0x0000E000)          /*!< 14 wait states */
#define FLCTL_A_BANK1_RDCTL_WAIT_15              ((uint32_t)0x0000F000)          /*!< 15 wait states */
#define FLCTL_A_RDBRST_CTLSTAT_START_OFS         ( 0)                            /*!< START Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_START             ((uint32_t)0x00000001)          /*!< Start of burst/compare operation */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_OFS      ( 1)                            /*!< MEM_TYPE Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_MASK     ((uint32_t)0x00000006)          /*!< MEM_TYPE Bit Mask */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE0         ((uint32_t)0x00000002)          /*!< MEM_TYPE Bit 0 */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE1         ((uint32_t)0x00000004)          /*!< MEM_TYPE Bit 1 */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_0        ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_1        ((uint32_t)0x00000002)          /*!< Information Memory */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_2        ((uint32_t)0x00000004)          /*!< Reserved */
#define FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_3        ((uint32_t)0x00000006)          /*!< Engineering Memory */
#define FLCTL_A_RDBRST_CTLSTAT_STOP_FAIL_OFS     ( 3)                            /*!< STOP_FAIL Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_STOP_FAIL         ((uint32_t)0x00000008)          /*!< Terminate burst/compare operation */
#define FLCTL_A_RDBRST_CTLSTAT_DATA_CMP_OFS      ( 4)                            /*!< DATA_CMP Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_DATA_CMP          ((uint32_t)0x00000010)          /*!< Data pattern used for comparison against memory read data */
#define FLCTL_A_RDBRST_CTLSTAT_TEST_EN_OFS       ( 6)                            /*!< TEST_EN Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_TEST_EN           ((uint32_t)0x00000040)          /*!< Enable comparison against test data compare registers */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_OFS     (16)                            /*!< BRST_STAT Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_MASK    ((uint32_t)0x00030000)          /*!< BRST_STAT Bit Mask */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT0        ((uint32_t)0x00010000)          /*!< BRST_STAT Bit 0 */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT1        ((uint32_t)0x00020000)          /*!< BRST_STAT Bit 1 */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_0       ((uint32_t)0x00000000)          /*!< Idle */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_1       ((uint32_t)0x00010000)          /*!< Burst/Compare START bit written, but operation pending */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_2       ((uint32_t)0x00020000)          /*!< Burst/Compare in progress */
#define FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_3       ((uint32_t)0x00030000)          /*!< Burst complete (status of completed burst remains in this state unless  */
#define FLCTL_A_RDBRST_CTLSTAT_CMP_ERR_OFS       (18)                            /*!< CMP_ERR Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_CMP_ERR           ((uint32_t)0x00040000)          /*!< Burst/Compare Operation encountered atleast one data */
#define FLCTL_A_RDBRST_CTLSTAT_ADDR_ERR_OFS      (19)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_ADDR_ERR          ((uint32_t)0x00080000)          /*!< Burst/Compare Operation was terminated due to access to */
#define FLCTL_A_RDBRST_CTLSTAT_CLR_STAT_OFS      (23)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_A_RDBRST_CTLSTAT_CLR_STAT          ((uint32_t)0x00800000)          /*!< Clear status bits 19-16 of this register */
#define FLCTL_A_RDBRST_STARTADDR_START_ADDRESS_OFS ( 0)                            /*!< START_ADDRESS Bit Offset */
#define FLCTL_A_RDBRST_STARTADDR_START_ADDRESS_MASK ((uint32_t)0x001FFFFF)          /*!< START_ADDRESS Bit Mask */
#define FLCTL_A_RDBRST_LEN_BURST_LENGTH_OFS      ( 0)                            /*!< BURST_LENGTH Bit Offset */
#define FLCTL_A_RDBRST_LEN_BURST_LENGTH_MASK     ((uint32_t)0x001FFFFF)          /*!< BURST_LENGTH Bit Mask */
#define FLCTL_A_RDBRST_FAILADDR_FAIL_ADDRESS_OFS ( 0)                            /*!< FAIL_ADDRESS Bit Offset */
#define FLCTL_A_RDBRST_FAILADDR_FAIL_ADDRESS_MASK ((uint32_t)0x001FFFFF)          /*!< FAIL_ADDRESS Bit Mask */
#define FLCTL_A_RDBRST_FAILCNT_FAIL_COUNT_OFS    ( 0)                            /*!< FAIL_COUNT Bit Offset */
#define FLCTL_A_RDBRST_FAILCNT_FAIL_COUNT_MASK   ((uint32_t)0x0001FFFF)          /*!< FAIL_COUNT Bit Mask */
#define FLCTL_A_PRG_CTLSTAT_ENABLE_OFS           ( 0)                            /*!< ENABLE Bit Offset */
#define FLCTL_A_PRG_CTLSTAT_ENABLE               ((uint32_t)0x00000001)          /*!< Master control for all word program operations */
#define FLCTL_A_PRG_CTLSTAT_MODE_OFS             ( 1)                            /*!< MODE Bit Offset */
#define FLCTL_A_PRG_CTLSTAT_MODE                 ((uint32_t)0x00000002)          /*!< Write mode */
#define FLCTL_A_PRG_CTLSTAT_VER_PRE_OFS          ( 2)                            /*!< VER_PRE Bit Offset */
#define FLCTL_A_PRG_CTLSTAT_VER_PRE              ((uint32_t)0x00000004)          /*!< Controls automatic pre program verify operations */
#define FLCTL_A_PRG_CTLSTAT_VER_PST_OFS          ( 3)                            /*!< VER_PST Bit Offset */
#define FLCTL_A_PRG_CTLSTAT_VER_PST              ((uint32_t)0x00000008)          /*!< Controls automatic post program verify operations */
#define FLCTL_A_PRG_CTLSTAT_STATUS_OFS           (16)                            /*!< STATUS Bit Offset */
#define FLCTL_A_PRG_CTLSTAT_STATUS_MASK          ((uint32_t)0x00030000)          /*!< STATUS Bit Mask */
#define FLCTL_A_PRG_CTLSTAT_STATUS0              ((uint32_t)0x00010000)          /*!< STATUS Bit 0 */
#define FLCTL_A_PRG_CTLSTAT_STATUS1              ((uint32_t)0x00020000)          /*!< STATUS Bit 1 */
#define FLCTL_A_PRG_CTLSTAT_STATUS_0             ((uint32_t)0x00000000)          /*!< Idle (no program operation currently active) */
#define FLCTL_A_PRG_CTLSTAT_STATUS_1             ((uint32_t)0x00010000)          /*!< Single word program operation triggered, but pending */
#define FLCTL_A_PRG_CTLSTAT_STATUS_2             ((uint32_t)0x00020000)          /*!< Single word program in progress */
#define FLCTL_A_PRG_CTLSTAT_STATUS_3             ((uint32_t)0x00030000)          /*!< Reserved (Idle) */
#define FLCTL_A_PRG_CTLSTAT_BNK_ACT_OFS          (18)                            /*!< BNK_ACT Bit Offset */
#define FLCTL_A_PRG_CTLSTAT_BNK_ACT              ((uint32_t)0x00040000)          /*!< Bank active */
#define FLCTL_A_PRGBRST_CTLSTAT_START_OFS        ( 0)                            /*!< START Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_START            ((uint32_t)0x00000001)          /*!< Trigger start of burst program operation */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE_OFS         ( 1)                            /*!< TYPE Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE_MASK        ((uint32_t)0x00000006)          /*!< TYPE Bit Mask */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE0            ((uint32_t)0x00000002)          /*!< TYPE Bit 0 */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE1            ((uint32_t)0x00000004)          /*!< TYPE Bit 1 */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE_0           ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE_1           ((uint32_t)0x00000002)          /*!< Information Memory */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE_2           ((uint32_t)0x00000004)          /*!< Reserved */
#define FLCTL_A_PRGBRST_CTLSTAT_TYPE_3           ((uint32_t)0x00000006)          /*!< Engineering Memory */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_OFS          ( 3)                            /*!< LEN Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_MASK         ((uint32_t)0x00000038)          /*!< LEN Bit Mask */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN0             ((uint32_t)0x00000008)          /*!< LEN Bit 0 */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN1             ((uint32_t)0x00000010)          /*!< LEN Bit 1 */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN2             ((uint32_t)0x00000020)          /*!< LEN Bit 2 */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_0            ((uint32_t)0x00000000)          /*!< No burst operation */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_1            ((uint32_t)0x00000008)          /*!< 1 word burst of 128 bits, starting with address in the  */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_2            ((uint32_t)0x00000010)          /*!< 2*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_3            ((uint32_t)0x00000018)          /*!< 3*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
#define FLCTL_A_PRGBRST_CTLSTAT_LEN_4            ((uint32_t)0x00000020)          /*!< 4*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
#define FLCTL_A_PRGBRST_CTLSTAT_AUTO_PRE_OFS     ( 6)                            /*!< AUTO_PRE Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_AUTO_PRE         ((uint32_t)0x00000040)          /*!< Auto-Verify operation before the Burst Program */
#define FLCTL_A_PRGBRST_CTLSTAT_AUTO_PST_OFS     ( 7)                            /*!< AUTO_PST Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_AUTO_PST         ((uint32_t)0x00000080)          /*!< Auto-Verify operation after the Burst Program */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_OFS (16)                            /*!< BURST_STATUS Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK ((uint32_t)0x00070000)          /*!< BURST_STATUS Bit Mask */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS0    ((uint32_t)0x00010000)          /*!< BURST_STATUS Bit 0 */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS1    ((uint32_t)0x00020000)          /*!< BURST_STATUS Bit 1 */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS2    ((uint32_t)0x00040000)          /*!< BURST_STATUS Bit 2 */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_0   ((uint32_t)0x00000000)          /*!< Idle (Burst not active) */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_1   ((uint32_t)0x00010000)          /*!< Burst program started but pending */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_2   ((uint32_t)0x00020000)          /*!< Burst active, with 1st 128 bit word being written into Flash */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_3   ((uint32_t)0x00030000)          /*!< Burst active, with 2nd 128 bit word being written into Flash */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_4   ((uint32_t)0x00040000)          /*!< Burst active, with 3rd 128 bit word being written into Flash */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_5   ((uint32_t)0x00050000)          /*!< Burst active, with 4th 128 bit word being written into Flash */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_6   ((uint32_t)0x00060000)          /*!< Reserved (Idle) */
#define FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_7   ((uint32_t)0x00070000)          /*!< Burst Complete (status of completed burst remains in this state unless  */
#define FLCTL_A_PRGBRST_CTLSTAT_PRE_ERR_OFS      (19)                            /*!< PRE_ERR Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_PRE_ERR          ((uint32_t)0x00080000)          /*!< Burst Operation encountered preprogram auto-verify errors */
#define FLCTL_A_PRGBRST_CTLSTAT_PST_ERR_OFS      (20)                            /*!< PST_ERR Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_PST_ERR          ((uint32_t)0x00100000)          /*!< Burst Operation encountered postprogram auto-verify errors */
#define FLCTL_A_PRGBRST_CTLSTAT_ADDR_ERR_OFS     (21)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_ADDR_ERR         ((uint32_t)0x00200000)          /*!< Burst Operation was terminated due to attempted program of reserved memory */
#define FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT_OFS     (23)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT         ((uint32_t)0x00800000)          /*!< Clear status bits 21-16 of this register */
#define FLCTL_A_PRGBRST_STARTADDR_START_ADDRESS_OFS ( 0)                            /*!< START_ADDRESS Bit Offset */
#define FLCTL_A_PRGBRST_STARTADDR_START_ADDRESS_MASK ((uint32_t)0x003FFFFF)          /*!< START_ADDRESS Bit Mask */
#define FLCTL_A_ERASE_CTLSTAT_START_OFS          ( 0)                            /*!< START Bit Offset */
#define FLCTL_A_ERASE_CTLSTAT_START              ((uint32_t)0x00000001)          /*!< Start of Erase operation */
#define FLCTL_A_ERASE_CTLSTAT_MODE_OFS           ( 1)                            /*!< MODE Bit Offset */
#define FLCTL_A_ERASE_CTLSTAT_MODE               ((uint32_t)0x00000002)          /*!< Erase mode selected by application */
#define FLCTL_A_ERASE_CTLSTAT_TYPE_OFS           ( 2)                            /*!< TYPE Bit Offset */
#define FLCTL_A_ERASE_CTLSTAT_TYPE_MASK          ((uint32_t)0x0000000C)          /*!< TYPE Bit Mask */
#define FLCTL_A_ERASE_CTLSTAT_TYPE0              ((uint32_t)0x00000004)          /*!< TYPE Bit 0 */
#define FLCTL_A_ERASE_CTLSTAT_TYPE1              ((uint32_t)0x00000008)          /*!< TYPE Bit 1 */
#define FLCTL_A_ERASE_CTLSTAT_TYPE_0             ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_A_ERASE_CTLSTAT_TYPE_1             ((uint32_t)0x00000004)          /*!< Information Memory */
#define FLCTL_A_ERASE_CTLSTAT_TYPE_2             ((uint32_t)0x00000008)          /*!< Reserved */
#define FLCTL_A_ERASE_CTLSTAT_TYPE_3             ((uint32_t)0x0000000C)          /*!< Engineering Memory */
#define FLCTL_A_ERASE_CTLSTAT_STATUS_OFS         (16)                            /*!< STATUS Bit Offset */
#define FLCTL_A_ERASE_CTLSTAT_STATUS_MASK        ((uint32_t)0x00030000)          /*!< STATUS Bit Mask */
#define FLCTL_A_ERASE_CTLSTAT_STATUS0            ((uint32_t)0x00010000)          /*!< STATUS Bit 0 */
#define FLCTL_A_ERASE_CTLSTAT_STATUS1            ((uint32_t)0x00020000)          /*!< STATUS Bit 1 */
#define FLCTL_A_ERASE_CTLSTAT_STATUS_0           ((uint32_t)0x00000000)          /*!< Idle (no program operation currently active) */
#define FLCTL_A_ERASE_CTLSTAT_STATUS_1           ((uint32_t)0x00010000)          /*!< Erase operation triggered to START but pending */
#define FLCTL_A_ERASE_CTLSTAT_STATUS_2           ((uint32_t)0x00020000)          /*!< Erase operation in progress */
#define FLCTL_A_ERASE_CTLSTAT_STATUS_3           ((uint32_t)0x00030000)          /*!< Erase operation completed (status of completed erase remains in this state  */
#define FLCTL_A_ERASE_CTLSTAT_ADDR_ERR_OFS       (18)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_A_ERASE_CTLSTAT_ADDR_ERR           ((uint32_t)0x00040000)          /*!< Erase Operation was terminated due to attempted erase of reserved memory  */
#define FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS       (19)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_A_ERASE_CTLSTAT_CLR_STAT           ((uint32_t)0x00080000)          /*!< Clear status bits 18-16 of this register */
#define FLCTL_A_ERASE_SECTADDR_SECT_ADDRESS_OFS  ( 0)                            /*!< SECT_ADDRESS Bit Offset */
#define FLCTL_A_ERASE_SECTADDR_SECT_ADDRESS_MASK ((uint32_t)0x003FFFFF)          /*!< SECT_ADDRESS Bit Mask */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT0_OFS      ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT0          ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT1_OFS      ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT1          ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT2_OFS      ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT2          ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT3_OFS      ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_A_BANK0_INFO_WEPROT_PROT3          ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT0_OFS      ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT0          ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT1_OFS      ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT1          ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT2_OFS      ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT2          ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT3_OFS      ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT3          ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT4_OFS      ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT4          ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT5_OFS      ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT5          ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT6_OFS      ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT6          ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT7_OFS      ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT7          ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT8_OFS      ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT8          ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT9_OFS      ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT9          ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT10_OFS     (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT10         ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT11_OFS     (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT11         ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT12_OFS     (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT12         ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT13_OFS     (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT13         ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT14_OFS     (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT14         ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT15_OFS     (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT15         ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT16_OFS     (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT16         ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT17_OFS     (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT17         ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT18_OFS     (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT18         ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT19_OFS     (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT19         ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT20_OFS     (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT20         ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT21_OFS     (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT21         ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT22_OFS     (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT22         ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT23_OFS     (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT23         ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT24_OFS     (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT24         ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT25_OFS     (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT25         ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT26_OFS     (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT26         ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT27_OFS     (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT27         ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT28_OFS     (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT28         ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT29_OFS     (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT29         ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT30_OFS     (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT30         ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT31_OFS     (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT_PROT31         ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT0_OFS      ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT0          ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase operations */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT1_OFS      ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT1          ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase operations */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT2_OFS      ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT2          ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT3_OFS      ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_A_BANK1_INFO_WEPROT_PROT3          ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT0_OFS      ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT0          ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT1_OFS      ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT1          ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT2_OFS      ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT2          ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT3_OFS      ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT3          ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT4_OFS      ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT4          ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT5_OFS      ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT5          ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT6_OFS      ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT6          ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT7_OFS      ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT7          ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT8_OFS      ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT8          ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT9_OFS      ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT9          ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT10_OFS     (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT10         ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT11_OFS     (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT11         ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT12_OFS     (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT12         ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT13_OFS     (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT13         ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT14_OFS     (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT14         ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT15_OFS     (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT15         ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT16_OFS     (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT16         ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT17_OFS     (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT17         ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT18_OFS     (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT18         ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT19_OFS     (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT19         ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT20_OFS     (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT20         ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT21_OFS     (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT21         ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT22_OFS     (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT22         ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT23_OFS     (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT23         ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT24_OFS     (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT24         ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT25_OFS     (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT25         ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT26_OFS     (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT26         ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT27_OFS     (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT27         ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT28_OFS     (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT28         ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT29_OFS     (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT29         ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT30_OFS     (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT30         ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase operations */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT31_OFS     (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT_PROT31         ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase operations */
#define FLCTL_A_BMRK_CTLSTAT_I_BMRK_OFS          ( 0)                            /*!< I_BMRK Bit Offset */
#define FLCTL_A_BMRK_CTLSTAT_I_BMRK              ((uint32_t)0x00000001)          
#define FLCTL_A_BMRK_CTLSTAT_D_BMRK_OFS          ( 1)                            /*!< D_BMRK Bit Offset */
#define FLCTL_A_BMRK_CTLSTAT_D_BMRK              ((uint32_t)0x00000002)          
#define FLCTL_A_BMRK_CTLSTAT_CMP_EN_OFS          ( 2)                            /*!< CMP_EN Bit Offset */
#define FLCTL_A_BMRK_CTLSTAT_CMP_EN              ((uint32_t)0x00000004)          
#define FLCTL_A_BMRK_CTLSTAT_CMP_SEL_OFS         ( 3)                            /*!< CMP_SEL Bit Offset */
#define FLCTL_A_BMRK_CTLSTAT_CMP_SEL             ((uint32_t)0x00000008)          
#define FLCTL_A_IFG_RDBRST_OFS                   ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_A_IFG_RDBRST                       ((uint32_t)0x00000001)          
#define FLCTL_A_IFG_AVPRE_OFS                    ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_A_IFG_AVPRE                        ((uint32_t)0x00000002)          
#define FLCTL_A_IFG_AVPST_OFS                    ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_A_IFG_AVPST                        ((uint32_t)0x00000004)          
#define FLCTL_A_IFG_PRG_OFS                      ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_A_IFG_PRG                          ((uint32_t)0x00000008)          
#define FLCTL_A_IFG_PRGB_OFS                     ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_A_IFG_PRGB                         ((uint32_t)0x00000010)          
#define FLCTL_A_IFG_ERASE_OFS                    ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_A_IFG_ERASE                        ((uint32_t)0x00000020)          
#define FLCTL_A_IFG_BMRK_OFS                     ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_A_IFG_BMRK                         ((uint32_t)0x00000100)          
#define FLCTL_A_IFG_PRG_ERR_OFS                  ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_A_IFG_PRG_ERR                      ((uint32_t)0x00000200)          
#define FLCTL_A_IE_RDBRST_OFS                    ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_A_IE_RDBRST                        ((uint32_t)0x00000001)          
#define FLCTL_A_IE_AVPRE_OFS                     ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_A_IE_AVPRE                         ((uint32_t)0x00000002)          
#define FLCTL_A_IE_AVPST_OFS                     ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_A_IE_AVPST                         ((uint32_t)0x00000004)          
#define FLCTL_A_IE_PRG_OFS                       ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_A_IE_PRG                           ((uint32_t)0x00000008)          
#define FLCTL_A_IE_PRGB_OFS                      ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_A_IE_PRGB                          ((uint32_t)0x00000010)          
#define FLCTL_A_IE_ERASE_OFS                     ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_A_IE_ERASE                         ((uint32_t)0x00000020)          
#define FLCTL_A_IE_BMRK_OFS                      ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_A_IE_BMRK                          ((uint32_t)0x00000100)          
#define FLCTL_A_IE_PRG_ERR_OFS                   ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_A_IE_PRG_ERR                       ((uint32_t)0x00000200)          
#define FLCTL_A_CLRIFG_RDBRST_OFS                ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_A_CLRIFG_RDBRST                    ((uint32_t)0x00000001)          
#define FLCTL_A_CLRIFG_AVPRE_OFS                 ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_A_CLRIFG_AVPRE                     ((uint32_t)0x00000002)          
#define FLCTL_A_CLRIFG_AVPST_OFS                 ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_A_CLRIFG_AVPST                     ((uint32_t)0x00000004)          
#define FLCTL_A_CLRIFG_PRG_OFS                   ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_A_CLRIFG_PRG                       ((uint32_t)0x00000008)          
#define FLCTL_A_CLRIFG_PRGB_OFS                  ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_A_CLRIFG_PRGB                      ((uint32_t)0x00000010)          
#define FLCTL_A_CLRIFG_ERASE_OFS                 ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_A_CLRIFG_ERASE                     ((uint32_t)0x00000020)          
#define FLCTL_A_CLRIFG_BMRK_OFS                  ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_A_CLRIFG_BMRK                      ((uint32_t)0x00000100)          
#define FLCTL_A_CLRIFG_PRG_ERR_OFS               ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_A_CLRIFG_PRG_ERR                   ((uint32_t)0x00000200)          
#define FLCTL_A_SETIFG_RDBRST_OFS                ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_A_SETIFG_RDBRST                    ((uint32_t)0x00000001)          
#define FLCTL_A_SETIFG_AVPRE_OFS                 ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_A_SETIFG_AVPRE                     ((uint32_t)0x00000002)          
#define FLCTL_A_SETIFG_AVPST_OFS                 ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_A_SETIFG_AVPST                     ((uint32_t)0x00000004)          
#define FLCTL_A_SETIFG_PRG_OFS                   ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_A_SETIFG_PRG                       ((uint32_t)0x00000008)          
#define FLCTL_A_SETIFG_PRGB_OFS                  ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_A_SETIFG_PRGB                      ((uint32_t)0x00000010)          
#define FLCTL_A_SETIFG_ERASE_OFS                 ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_A_SETIFG_ERASE                     ((uint32_t)0x00000020)          
#define FLCTL_A_SETIFG_BMRK_OFS                  ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_A_SETIFG_BMRK                      ((uint32_t)0x00000100)          
#define FLCTL_A_SETIFG_PRG_ERR_OFS               ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_A_SETIFG_PRG_ERR                   ((uint32_t)0x00000200)          
#define FLCTL_A_READ_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_READ_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_READ_TIMCTL_IREF_BOOST1_OFS      (12)                            /*!< IREF_BOOST1 Bit Offset */
#define FLCTL_A_READ_TIMCTL_IREF_BOOST1_MASK     ((uint32_t)0x0000F000)          /*!< IREF_BOOST1 Bit Mask */
#define FLCTL_A_READ_TIMCTL_SETUP_LONG_OFS       (16)                            /*!< SETUP_LONG Bit Offset */
#define FLCTL_A_READ_TIMCTL_SETUP_LONG_MASK      ((uint32_t)0x00FF0000)          /*!< SETUP_LONG Bit Mask */
#define FLCTL_A_READMARGIN_TIMCTL_SETUP_OFS      ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_READMARGIN_TIMCTL_SETUP_MASK     ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_PRGVER_TIMCTL_SETUP_OFS          ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_PRGVER_TIMCTL_SETUP_MASK         ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_PRGVER_TIMCTL_ACTIVE_OFS         ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_A_PRGVER_TIMCTL_ACTIVE_MASK        ((uint32_t)0x00000F00)          /*!< ACTIVE Bit Mask */
#define FLCTL_A_PRGVER_TIMCTL_HOLD_OFS           (12)                            /*!< HOLD Bit Offset */
#define FLCTL_A_PRGVER_TIMCTL_HOLD_MASK          ((uint32_t)0x0000F000)          /*!< HOLD Bit Mask */
#define FLCTL_A_ERSVER_TIMCTL_SETUP_OFS          ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_ERSVER_TIMCTL_SETUP_MASK         ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_LKGVER_TIMCTL_SETUP_OFS          ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_LKGVER_TIMCTL_SETUP_MASK         ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_PROGRAM_TIMCTL_SETUP_OFS         ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_PROGRAM_TIMCTL_SETUP_MASK        ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_PROGRAM_TIMCTL_ACTIVE_OFS        ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_A_PROGRAM_TIMCTL_ACTIVE_MASK       ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
#define FLCTL_A_PROGRAM_TIMCTL_HOLD_OFS          (28)                            /*!< HOLD Bit Offset */
#define FLCTL_A_PROGRAM_TIMCTL_HOLD_MASK         ((uint32_t)0xF0000000)          /*!< HOLD Bit Mask */
#define FLCTL_A_ERASE_TIMCTL_SETUP_OFS           ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_A_ERASE_TIMCTL_SETUP_MASK          ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
#define FLCTL_A_ERASE_TIMCTL_ACTIVE_OFS          ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_A_ERASE_TIMCTL_ACTIVE_MASK         ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
#define FLCTL_A_ERASE_TIMCTL_HOLD_OFS            (28)                            /*!< HOLD Bit Offset */
#define FLCTL_A_ERASE_TIMCTL_HOLD_MASK           ((uint32_t)0xF0000000)          /*!< HOLD Bit Mask */
#define FLCTL_A_MASSERASE_TIMCTL_BOOST_ACTIVE_OFS ( 0)                            /*!< BOOST_ACTIVE Bit Offset */
#define FLCTL_A_MASSERASE_TIMCTL_BOOST_ACTIVE_MASK ((uint32_t)0x000000FF)          /*!< BOOST_ACTIVE Bit Mask */
#define FLCTL_A_MASSERASE_TIMCTL_BOOST_HOLD_OFS  ( 8)                            /*!< BOOST_HOLD Bit Offset */
#define FLCTL_A_MASSERASE_TIMCTL_BOOST_HOLD_MASK ((uint32_t)0x0000FF00)          /*!< BOOST_HOLD Bit Mask */
#define FLCTL_A_BURSTPRG_TIMCTL_ACTIVE_OFS       ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_A_BURSTPRG_TIMCTL_ACTIVE_MASK      ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT0_OFS     ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT0         ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT1_OFS     ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT1         ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT2_OFS     ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT2         ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT3_OFS     ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT3         ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT4_OFS     ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT4         ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT5_OFS     ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT5         ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT6_OFS     ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT6         ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT7_OFS     ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT7         ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT8_OFS     ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT8         ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT9_OFS     ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT9         ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT10_OFS    (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT10        ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT11_OFS    (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT11        ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT12_OFS    (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT12        ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT13_OFS    (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT13        ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT14_OFS    (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT14        ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT15_OFS    (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT15        ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT16_OFS    (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT16        ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT17_OFS    (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT17        ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT18_OFS    (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT18        ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT19_OFS    (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT19        ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT20_OFS    (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT20        ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT21_OFS    (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT21        ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT22_OFS    (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT22        ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT23_OFS    (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT23        ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT24_OFS    (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT24        ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT25_OFS    (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT25        ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT26_OFS    (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT26        ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT27_OFS    (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT27        ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT28_OFS    (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT28        ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT29_OFS    (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT29        ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT30_OFS    (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT30        ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT31_OFS    (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT0_PROT31        ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT32_OFS    ( 0)                            /*!< PROT32 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT32        ((uint32_t)0x00000001)          /*!< Protects Sector 32 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT33_OFS    ( 1)                            /*!< PROT33 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT33        ((uint32_t)0x00000002)          /*!< Protects Sector 33 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT34_OFS    ( 2)                            /*!< PROT34 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT34        ((uint32_t)0x00000004)          /*!< Protects Sector 34 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT35_OFS    ( 3)                            /*!< PROT35 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT35        ((uint32_t)0x00000008)          /*!< Protects Sector 35 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT36_OFS    ( 4)                            /*!< PROT36 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT36        ((uint32_t)0x00000010)          /*!< Protects Sector 36 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT37_OFS    ( 5)                            /*!< PROT37 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT37        ((uint32_t)0x00000020)          /*!< Protects Sector 37 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT38_OFS    ( 6)                            /*!< PROT38 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT38        ((uint32_t)0x00000040)          /*!< Protects Sector 38 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT39_OFS    ( 7)                            /*!< PROT39 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT39        ((uint32_t)0x00000080)          /*!< Protects Sector 39 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT40_OFS    ( 8)                            /*!< PROT40 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT40        ((uint32_t)0x00000100)          /*!< Protects Sector 40 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT41_OFS    ( 9)                            /*!< PROT41 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT41        ((uint32_t)0x00000200)          /*!< Protects Sector 41 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT42_OFS    (10)                            /*!< PROT42 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT42        ((uint32_t)0x00000400)          /*!< Protects Sector 42 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT43_OFS    (11)                            /*!< PROT43 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT43        ((uint32_t)0x00000800)          /*!< Protects Sector 43 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT44_OFS    (12)                            /*!< PROT44 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT44        ((uint32_t)0x00001000)          /*!< Protects Sector 44 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT45_OFS    (13)                            /*!< PROT45 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT45        ((uint32_t)0x00002000)          /*!< Protects Sector 45 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT46_OFS    (14)                            /*!< PROT46 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT46        ((uint32_t)0x00004000)          /*!< Protects Sector 46 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT47_OFS    (15)                            /*!< PROT47 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT47        ((uint32_t)0x00008000)          /*!< Protects Sector 47 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT48_OFS    (16)                            /*!< PROT48 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT48        ((uint32_t)0x00010000)          /*!< Protects Sector 48 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT49_OFS    (17)                            /*!< PROT49 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT49        ((uint32_t)0x00020000)          /*!< Protects Sector 49 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT50_OFS    (18)                            /*!< PROT50 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT50        ((uint32_t)0x00040000)          /*!< Protects Sector 50 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT51_OFS    (19)                            /*!< PROT51 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT51        ((uint32_t)0x00080000)          /*!< Protects Sector 51 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT52_OFS    (20)                            /*!< PROT52 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT52        ((uint32_t)0x00100000)          /*!< Protects Sector 52 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT53_OFS    (21)                            /*!< PROT53 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT53        ((uint32_t)0x00200000)          /*!< Protects Sector 53 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT54_OFS    (22)                            /*!< PROT54 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT54        ((uint32_t)0x00400000)          /*!< Protects Sector 54 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT55_OFS    (23)                            /*!< PROT55 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT55        ((uint32_t)0x00800000)          /*!< Protects Sector 55 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT56_OFS    (24)                            /*!< PROT56 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT56        ((uint32_t)0x01000000)          /*!< Protects Sector 56 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT57_OFS    (25)                            /*!< PROT57 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT57        ((uint32_t)0x02000000)          /*!< Protects Sector 57 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT58_OFS    (26)                            /*!< PROT58 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT58        ((uint32_t)0x04000000)          /*!< Protects Sector 58 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT59_OFS    (27)                            /*!< PROT59 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT59        ((uint32_t)0x08000000)          /*!< Protects Sector 59 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT60_OFS    (28)                            /*!< PROT60 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT60        ((uint32_t)0x10000000)          /*!< Protects Sector 60 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT61_OFS    (29)                            /*!< PROT61 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT61        ((uint32_t)0x20000000)          /*!< Protects Sector 61 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT62_OFS    (30)                            /*!< PROT62 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT62        ((uint32_t)0x40000000)          /*!< Protects Sector 62 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT63_OFS    (31)                            /*!< PROT63 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT1_PROT63        ((uint32_t)0x80000000)          /*!< Protects Sector 63 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT64_OFS    ( 0)                            /*!< PROT64 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT64        ((uint32_t)0x00000001)          /*!< Protects Sector 64 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT65_OFS    ( 1)                            /*!< PROT65 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT65        ((uint32_t)0x00000002)          /*!< Protects Sector 65 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT66_OFS    ( 2)                            /*!< PROT66 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT66        ((uint32_t)0x00000004)          /*!< Protects Sector 66 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT67_OFS    ( 3)                            /*!< PROT67 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT67        ((uint32_t)0x00000008)          /*!< Protects Sector 67 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT68_OFS    ( 4)                            /*!< PROT68 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT68        ((uint32_t)0x00000010)          /*!< Protects Sector 68 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT69_OFS    ( 5)                            /*!< PROT69 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT69        ((uint32_t)0x00000020)          /*!< Protects Sector 69 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT70_OFS    ( 6)                            /*!< PROT70 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT70        ((uint32_t)0x00000040)          /*!< Protects Sector 70 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT71_OFS    ( 7)                            /*!< PROT71 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT71        ((uint32_t)0x00000080)          /*!< Protects Sector 71 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT72_OFS    ( 8)                            /*!< PROT72 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT72        ((uint32_t)0x00000100)          /*!< Protects Sector 72 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT73_OFS    ( 9)                            /*!< PROT73 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT73        ((uint32_t)0x00000200)          /*!< Protects Sector 73 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT74_OFS    (10)                            /*!< PROT74 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT74        ((uint32_t)0x00000400)          /*!< Protects Sector 74 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT75_OFS    (11)                            /*!< PROT75 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT75        ((uint32_t)0x00000800)          /*!< Protects Sector 75 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT76_OFS    (12)                            /*!< PROT76 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT76        ((uint32_t)0x00001000)          /*!< Protects Sector 76 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT77_OFS    (13)                            /*!< PROT77 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT77        ((uint32_t)0x00002000)          /*!< Protects Sector 77 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT78_OFS    (14)                            /*!< PROT78 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT78        ((uint32_t)0x00004000)          /*!< Protects Sector 78 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT79_OFS    (15)                            /*!< PROT79 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT79        ((uint32_t)0x00008000)          /*!< Protects Sector 79 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT80_OFS    (16)                            /*!< PROT80 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT80        ((uint32_t)0x00010000)          /*!< Protects Sector 80 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT81_OFS    (17)                            /*!< PROT81 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT81        ((uint32_t)0x00020000)          /*!< Protects Sector 81 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT82_OFS    (18)                            /*!< PROT82 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT82        ((uint32_t)0x00040000)          /*!< Protects Sector 82 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT83_OFS    (19)                            /*!< PROT83 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT83        ((uint32_t)0x00080000)          /*!< Protects Sector 83 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT84_OFS    (20)                            /*!< PROT84 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT84        ((uint32_t)0x00100000)          /*!< Protects Sector 84 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT85_OFS    (21)                            /*!< PROT85 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT85        ((uint32_t)0x00200000)          /*!< Protects Sector 85 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT86_OFS    (22)                            /*!< PROT86 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT86        ((uint32_t)0x00400000)          /*!< Protects Sector 86 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT87_OFS    (23)                            /*!< PROT87 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT87        ((uint32_t)0x00800000)          /*!< Protects Sector 87 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT88_OFS    (24)                            /*!< PROT88 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT88        ((uint32_t)0x01000000)          /*!< Protects Sector 88 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT89_OFS    (25)                            /*!< PROT89 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT89        ((uint32_t)0x02000000)          /*!< Protects Sector 89 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT90_OFS    (26)                            /*!< PROT90 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT90        ((uint32_t)0x04000000)          /*!< Protects Sector 90 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT91_OFS    (27)                            /*!< PROT91 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT91        ((uint32_t)0x08000000)          /*!< Protects Sector 91 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT92_OFS    (28)                            /*!< PROT92 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT92        ((uint32_t)0x10000000)          /*!< Protects Sector 92 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT93_OFS    (29)                            /*!< PROT93 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT93        ((uint32_t)0x20000000)          /*!< Protects Sector 93 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT94_OFS    (30)                            /*!< PROT94 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT94        ((uint32_t)0x40000000)          /*!< Protects Sector 94 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT95_OFS    (31)                            /*!< PROT95 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT2_PROT95        ((uint32_t)0x80000000)          /*!< Protects Sector 95 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT96_OFS    ( 0)                            /*!< PROT96 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT96        ((uint32_t)0x00000001)          /*!< Protects Sector 96 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT97_OFS    ( 1)                            /*!< PROT97 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT97        ((uint32_t)0x00000002)          /*!< Protects Sector 97 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT98_OFS    ( 2)                            /*!< PROT98 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT98        ((uint32_t)0x00000004)          /*!< Protects Sector 98 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT99_OFS    ( 3)                            /*!< PROT99 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT99        ((uint32_t)0x00000008)          /*!< Protects Sector 99 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT100_OFS   ( 4)                            /*!< PROT100 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT100       ((uint32_t)0x00000010)          /*!< Protects Sector 100 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT101_OFS   ( 5)                            /*!< PROT101 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT101       ((uint32_t)0x00000020)          /*!< Protects Sector 101 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT102_OFS   ( 6)                            /*!< PROT102 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT102       ((uint32_t)0x00000040)          /*!< Protects Sector 102 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT103_OFS   ( 7)                            /*!< PROT103 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT103       ((uint32_t)0x00000080)          /*!< Protects Sector 103 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT104_OFS   ( 8)                            /*!< PROT104 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT104       ((uint32_t)0x00000100)          /*!< Protects Sector 104 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT105_OFS   ( 9)                            /*!< PROT105 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT105       ((uint32_t)0x00000200)          /*!< Protects Sector 105 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT106_OFS   (10)                            /*!< PROT106 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT106       ((uint32_t)0x00000400)          /*!< Protects Sector 106 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT107_OFS   (11)                            /*!< PROT107 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT107       ((uint32_t)0x00000800)          /*!< Protects Sector 107 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT108_OFS   (12)                            /*!< PROT108 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT108       ((uint32_t)0x00001000)          /*!< Protects Sector 108 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT109_OFS   (13)                            /*!< PROT109 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT109       ((uint32_t)0x00002000)          /*!< Protects Sector 109 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT110_OFS   (14)                            /*!< PROT110 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT110       ((uint32_t)0x00004000)          /*!< Protects Sector 110 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT111_OFS   (15)                            /*!< PROT111 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT111       ((uint32_t)0x00008000)          /*!< Protects Sector 111 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT112_OFS   (16)                            /*!< PROT112 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT112       ((uint32_t)0x00010000)          /*!< Protects Sector 112 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT113_OFS   (17)                            /*!< PROT113 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT113       ((uint32_t)0x00020000)          /*!< Protects Sector 113 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT114_OFS   (18)                            /*!< PROT114 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT114       ((uint32_t)0x00040000)          /*!< Protects Sector 114 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT115_OFS   (19)                            /*!< PROT115 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT115       ((uint32_t)0x00080000)          /*!< Protects Sector 115 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT116_OFS   (20)                            /*!< PROT116 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT116       ((uint32_t)0x00100000)          /*!< Protects Sector 116 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT117_OFS   (21)                            /*!< PROT117 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT117       ((uint32_t)0x00200000)          /*!< Protects Sector 117 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT118_OFS   (22)                            /*!< PROT118 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT118       ((uint32_t)0x00400000)          /*!< Protects Sector 118 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT119_OFS   (23)                            /*!< PROT119 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT119       ((uint32_t)0x00800000)          /*!< Protects Sector 119 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT120_OFS   (24)                            /*!< PROT120 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT120       ((uint32_t)0x01000000)          /*!< Protects Sector 120 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT121_OFS   (25)                            /*!< PROT121 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT121       ((uint32_t)0x02000000)          /*!< Protects Sector 121 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT122_OFS   (26)                            /*!< PROT122 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT122       ((uint32_t)0x04000000)          /*!< Protects Sector 122 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT123_OFS   (27)                            /*!< PROT123 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT123       ((uint32_t)0x08000000)          /*!< Protects Sector 123 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT124_OFS   (28)                            /*!< PROT124 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT124       ((uint32_t)0x10000000)          /*!< Protects Sector 124 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT125_OFS   (29)                            /*!< PROT125 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT125       ((uint32_t)0x20000000)          /*!< Protects Sector 125 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT126_OFS   (30)                            /*!< PROT126 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT126       ((uint32_t)0x40000000)          /*!< Protects Sector 126 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT127_OFS   (31)                            /*!< PROT127 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT3_PROT127       ((uint32_t)0x80000000)          /*!< Protects Sector 127 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT128_OFS   ( 0)                            /*!< PROT128 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT128       ((uint32_t)0x00000001)          /*!< Protects Sector 128 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT129_OFS   ( 1)                            /*!< PROT129 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT129       ((uint32_t)0x00000002)          /*!< Protects Sector 129 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT130_OFS   ( 2)                            /*!< PROT130 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT130       ((uint32_t)0x00000004)          /*!< Protects Sector 130 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT131_OFS   ( 3)                            /*!< PROT131 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT131       ((uint32_t)0x00000008)          /*!< Protects Sector 131 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT132_OFS   ( 4)                            /*!< PROT132 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT132       ((uint32_t)0x00000010)          /*!< Protects Sector 132 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT133_OFS   ( 5)                            /*!< PROT133 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT133       ((uint32_t)0x00000020)          /*!< Protects Sector 133 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT134_OFS   ( 6)                            /*!< PROT134 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT134       ((uint32_t)0x00000040)          /*!< Protects Sector 134 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT135_OFS   ( 7)                            /*!< PROT135 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT135       ((uint32_t)0x00000080)          /*!< Protects Sector 135 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT136_OFS   ( 8)                            /*!< PROT136 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT136       ((uint32_t)0x00000100)          /*!< Protects Sector 136 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT137_OFS   ( 9)                            /*!< PROT137 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT137       ((uint32_t)0x00000200)          /*!< Protects Sector 137 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT138_OFS   (10)                            /*!< PROT138 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT138       ((uint32_t)0x00000400)          /*!< Protects Sector 138 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT139_OFS   (11)                            /*!< PROT139 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT139       ((uint32_t)0x00000800)          /*!< Protects Sector 139 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT140_OFS   (12)                            /*!< PROT140 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT140       ((uint32_t)0x00001000)          /*!< Protects Sector 140 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT141_OFS   (13)                            /*!< PROT141 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT141       ((uint32_t)0x00002000)          /*!< Protects Sector 141 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT142_OFS   (14)                            /*!< PROT142 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT142       ((uint32_t)0x00004000)          /*!< Protects Sector 142 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT143_OFS   (15)                            /*!< PROT143 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT143       ((uint32_t)0x00008000)          /*!< Protects Sector 143 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT144_OFS   (16)                            /*!< PROT144 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT144       ((uint32_t)0x00010000)          /*!< Protects Sector 144 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT145_OFS   (17)                            /*!< PROT145 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT145       ((uint32_t)0x00020000)          /*!< Protects Sector 145 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT146_OFS   (18)                            /*!< PROT146 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT146       ((uint32_t)0x00040000)          /*!< Protects Sector 146 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT147_OFS   (19)                            /*!< PROT147 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT147       ((uint32_t)0x00080000)          /*!< Protects Sector 147 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT148_OFS   (20)                            /*!< PROT148 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT148       ((uint32_t)0x00100000)          /*!< Protects Sector 148 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT149_OFS   (21)                            /*!< PROT149 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT149       ((uint32_t)0x00200000)          /*!< Protects Sector 149 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT150_OFS   (22)                            /*!< PROT150 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT150       ((uint32_t)0x00400000)          /*!< Protects Sector 150 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT151_OFS   (23)                            /*!< PROT151 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT151       ((uint32_t)0x00800000)          /*!< Protects Sector 151 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT152_OFS   (24)                            /*!< PROT152 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT152       ((uint32_t)0x01000000)          /*!< Protects Sector 152 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT153_OFS   (25)                            /*!< PROT153 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT153       ((uint32_t)0x02000000)          /*!< Protects Sector 153 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT154_OFS   (26)                            /*!< PROT154 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT154       ((uint32_t)0x04000000)          /*!< Protects Sector 154 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT155_OFS   (27)                            /*!< PROT155 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT155       ((uint32_t)0x08000000)          /*!< Protects Sector 155 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT156_OFS   (28)                            /*!< PROT156 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT156       ((uint32_t)0x10000000)          /*!< Protects Sector 156 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT157_OFS   (29)                            /*!< PROT157 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT157       ((uint32_t)0x20000000)          /*!< Protects Sector 157 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT158_OFS   (30)                            /*!< PROT158 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT158       ((uint32_t)0x40000000)          /*!< Protects Sector 158 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT159_OFS   (31)                            /*!< PROT159 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT4_PROT159       ((uint32_t)0x80000000)          /*!< Protects Sector 159 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT160_OFS   ( 0)                            /*!< PROT160 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT160       ((uint32_t)0x00000001)          /*!< Protects Sector 160 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT161_OFS   ( 1)                            /*!< PROT161 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT161       ((uint32_t)0x00000002)          /*!< Protects Sector 161 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT162_OFS   ( 2)                            /*!< PROT162 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT162       ((uint32_t)0x00000004)          /*!< Protects Sector 162 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT163_OFS   ( 3)                            /*!< PROT163 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT163       ((uint32_t)0x00000008)          /*!< Protects Sector 163 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT164_OFS   ( 4)                            /*!< PROT164 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT164       ((uint32_t)0x00000010)          /*!< Protects Sector 164 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT165_OFS   ( 5)                            /*!< PROT165 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT165       ((uint32_t)0x00000020)          /*!< Protects Sector 165 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT166_OFS   ( 6)                            /*!< PROT166 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT166       ((uint32_t)0x00000040)          /*!< Protects Sector 166 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT167_OFS   ( 7)                            /*!< PROT167 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT167       ((uint32_t)0x00000080)          /*!< Protects Sector 167 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT168_OFS   ( 8)                            /*!< PROT168 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT168       ((uint32_t)0x00000100)          /*!< Protects Sector 168 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT169_OFS   ( 9)                            /*!< PROT169 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT169       ((uint32_t)0x00000200)          /*!< Protects Sector 169 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT170_OFS   (10)                            /*!< PROT170 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT170       ((uint32_t)0x00000400)          /*!< Protects Sector 170 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT171_OFS   (11)                            /*!< PROT171 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT171       ((uint32_t)0x00000800)          /*!< Protects Sector 171 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT172_OFS   (12)                            /*!< PROT172 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT172       ((uint32_t)0x00001000)          /*!< Protects Sector 172 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT173_OFS   (13)                            /*!< PROT173 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT173       ((uint32_t)0x00002000)          /*!< Protects Sector 173 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT174_OFS   (14)                            /*!< PROT174 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT174       ((uint32_t)0x00004000)          /*!< Protects Sector 174 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT175_OFS   (15)                            /*!< PROT175 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT175       ((uint32_t)0x00008000)          /*!< Protects Sector 175 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT176_OFS   (16)                            /*!< PROT176 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT176       ((uint32_t)0x00010000)          /*!< Protects Sector 176 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT177_OFS   (17)                            /*!< PROT177 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT177       ((uint32_t)0x00020000)          /*!< Protects Sector 177 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT178_OFS   (18)                            /*!< PROT178 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT178       ((uint32_t)0x00040000)          /*!< Protects Sector 178 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT179_OFS   (19)                            /*!< PROT179 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT179       ((uint32_t)0x00080000)          /*!< Protects Sector 179 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT180_OFS   (20)                            /*!< PROT180 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT180       ((uint32_t)0x00100000)          /*!< Protects Sector 180 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT181_OFS   (21)                            /*!< PROT181 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT181       ((uint32_t)0x00200000)          /*!< Protects Sector 181 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT182_OFS   (22)                            /*!< PROT182 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT182       ((uint32_t)0x00400000)          /*!< Protects Sector 182 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT183_OFS   (23)                            /*!< PROT183 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT183       ((uint32_t)0x00800000)          /*!< Protects Sector 183 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT184_OFS   (24)                            /*!< PROT184 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT184       ((uint32_t)0x01000000)          /*!< Protects Sector 184 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT185_OFS   (25)                            /*!< PROT185 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT185       ((uint32_t)0x02000000)          /*!< Protects Sector 185 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT186_OFS   (26)                            /*!< PROT186 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT186       ((uint32_t)0x04000000)          /*!< Protects Sector 186 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT187_OFS   (27)                            /*!< PROT187 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT187       ((uint32_t)0x08000000)          /*!< Protects Sector 187 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT188_OFS   (28)                            /*!< PROT188 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT188       ((uint32_t)0x10000000)          /*!< Protects Sector 188 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT189_OFS   (29)                            /*!< PROT189 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT189       ((uint32_t)0x20000000)          /*!< Protects Sector 189 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT190_OFS   (30)                            /*!< PROT190 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT190       ((uint32_t)0x40000000)          /*!< Protects Sector 190 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT191_OFS   (31)                            /*!< PROT191 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT5_PROT191       ((uint32_t)0x80000000)          /*!< Protects Sector 191 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT192_OFS   ( 0)                            /*!< PROT192 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT192       ((uint32_t)0x00000001)          /*!< Protects Sector 192 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT193_OFS   ( 1)                            /*!< PROT193 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT193       ((uint32_t)0x00000002)          /*!< Protects Sector 193 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT194_OFS   ( 2)                            /*!< PROT194 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT194       ((uint32_t)0x00000004)          /*!< Protects Sector 194 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT195_OFS   ( 3)                            /*!< PROT195 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT195       ((uint32_t)0x00000008)          /*!< Protects Sector 195 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT196_OFS   ( 4)                            /*!< PROT196 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT196       ((uint32_t)0x00000010)          /*!< Protects Sector 196 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT197_OFS   ( 5)                            /*!< PROT197 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT197       ((uint32_t)0x00000020)          /*!< Protects Sector 197 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT198_OFS   ( 6)                            /*!< PROT198 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT198       ((uint32_t)0x00000040)          /*!< Protects Sector 198 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT199_OFS   ( 7)                            /*!< PROT199 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT199       ((uint32_t)0x00000080)          /*!< Protects Sector 199 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT200_OFS   ( 8)                            /*!< PROT200 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT200       ((uint32_t)0x00000100)          /*!< Protects Sector 200 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT201_OFS   ( 9)                            /*!< PROT201 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT201       ((uint32_t)0x00000200)          /*!< Protects Sector 201 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT202_OFS   (10)                            /*!< PROT202 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT202       ((uint32_t)0x00000400)          /*!< Protects Sector 202 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT203_OFS   (11)                            /*!< PROT203 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT203       ((uint32_t)0x00000800)          /*!< Protects Sector 203 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT204_OFS   (12)                            /*!< PROT204 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT204       ((uint32_t)0x00001000)          /*!< Protects Sector 204 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT205_OFS   (13)                            /*!< PROT205 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT205       ((uint32_t)0x00002000)          /*!< Protects Sector 205 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT206_OFS   (14)                            /*!< PROT206 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT206       ((uint32_t)0x00004000)          /*!< Protects Sector 206 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT207_OFS   (15)                            /*!< PROT207 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT207       ((uint32_t)0x00008000)          /*!< Protects Sector 207 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT208_OFS   (16)                            /*!< PROT208 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT208       ((uint32_t)0x00010000)          /*!< Protects Sector 208 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT209_OFS   (17)                            /*!< PROT209 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT209       ((uint32_t)0x00020000)          /*!< Protects Sector 209 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT210_OFS   (18)                            /*!< PROT210 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT210       ((uint32_t)0x00040000)          /*!< Protects Sector 210 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT211_OFS   (19)                            /*!< PROT211 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT211       ((uint32_t)0x00080000)          /*!< Protects Sector 211 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT212_OFS   (20)                            /*!< PROT212 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT212       ((uint32_t)0x00100000)          /*!< Protects Sector 212 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT213_OFS   (21)                            /*!< PROT213 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT213       ((uint32_t)0x00200000)          /*!< Protects Sector 213 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT214_OFS   (22)                            /*!< PROT214 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT214       ((uint32_t)0x00400000)          /*!< Protects Sector 214 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT215_OFS   (23)                            /*!< PROT215 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT215       ((uint32_t)0x00800000)          /*!< Protects Sector 215 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT216_OFS   (24)                            /*!< PROT216 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT216       ((uint32_t)0x01000000)          /*!< Protects Sector 216 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT217_OFS   (25)                            /*!< PROT217 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT217       ((uint32_t)0x02000000)          /*!< Protects Sector 217 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT218_OFS   (26)                            /*!< PROT218 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT218       ((uint32_t)0x04000000)          /*!< Protects Sector 218 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT219_OFS   (27)                            /*!< PROT219 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT219       ((uint32_t)0x08000000)          /*!< Protects Sector 219 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT220_OFS   (28)                            /*!< PROT220 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT220       ((uint32_t)0x10000000)          /*!< Protects Sector 220 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT221_OFS   (29)                            /*!< PROT221 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT221       ((uint32_t)0x20000000)          /*!< Protects Sector 221 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT222_OFS   (30)                            /*!< PROT222 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT222       ((uint32_t)0x40000000)          /*!< Protects Sector 222 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT223_OFS   (31)                            /*!< PROT223 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT6_PROT223       ((uint32_t)0x80000000)          /*!< Protects Sector 223 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT224_OFS   ( 0)                            /*!< PROT224 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT224       ((uint32_t)0x00000001)          /*!< Protects Sector 224 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT225_OFS   ( 1)                            /*!< PROT225 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT225       ((uint32_t)0x00000002)          /*!< Protects Sector 225 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT226_OFS   ( 2)                            /*!< PROT226 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT226       ((uint32_t)0x00000004)          /*!< Protects Sector 226 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT227_OFS   ( 3)                            /*!< PROT227 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT227       ((uint32_t)0x00000008)          /*!< Protects Sector 227 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT228_OFS   ( 4)                            /*!< PROT228 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT228       ((uint32_t)0x00000010)          /*!< Protects Sector 228 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT229_OFS   ( 5)                            /*!< PROT229 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT229       ((uint32_t)0x00000020)          /*!< Protects Sector 229 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT230_OFS   ( 6)                            /*!< PROT230 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT230       ((uint32_t)0x00000040)          /*!< Protects Sector 230 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT231_OFS   ( 7)                            /*!< PROT231 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT231       ((uint32_t)0x00000080)          /*!< Protects Sector 231 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT232_OFS   ( 8)                            /*!< PROT232 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT232       ((uint32_t)0x00000100)          /*!< Protects Sector 232 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT233_OFS   ( 9)                            /*!< PROT233 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT233       ((uint32_t)0x00000200)          /*!< Protects Sector 233 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT234_OFS   (10)                            /*!< PROT234 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT234       ((uint32_t)0x00000400)          /*!< Protects Sector 234 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT235_OFS   (11)                            /*!< PROT235 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT235       ((uint32_t)0x00000800)          /*!< Protects Sector 235 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT236_OFS   (12)                            /*!< PROT236 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT236       ((uint32_t)0x00001000)          /*!< Protects Sector 236 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT237_OFS   (13)                            /*!< PROT237 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT237       ((uint32_t)0x00002000)          /*!< Protects Sector 237 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT238_OFS   (14)                            /*!< PROT238 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT238       ((uint32_t)0x00004000)          /*!< Protects Sector 238 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT239_OFS   (15)                            /*!< PROT239 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT239       ((uint32_t)0x00008000)          /*!< Protects Sector 239 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT240_OFS   (16)                            /*!< PROT240 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT240       ((uint32_t)0x00010000)          /*!< Protects Sector 240 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT241_OFS   (17)                            /*!< PROT241 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT241       ((uint32_t)0x00020000)          /*!< Protects Sector 241 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT242_OFS   (18)                            /*!< PROT242 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT242       ((uint32_t)0x00040000)          /*!< Protects Sector 242 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT243_OFS   (19)                            /*!< PROT243 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT243       ((uint32_t)0x00080000)          /*!< Protects Sector 243 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT244_OFS   (20)                            /*!< PROT244 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT244       ((uint32_t)0x00100000)          /*!< Protects Sector 244 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT245_OFS   (21)                            /*!< PROT245 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT245       ((uint32_t)0x00200000)          /*!< Protects Sector 245 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT246_OFS   (22)                            /*!< PROT246 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT246       ((uint32_t)0x00400000)          /*!< Protects Sector 246 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT247_OFS   (23)                            /*!< PROT247 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT247       ((uint32_t)0x00800000)          /*!< Protects Sector 247 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT248_OFS   (24)                            /*!< PROT248 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT248       ((uint32_t)0x01000000)          /*!< Protects Sector 248 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT249_OFS   (25)                            /*!< PROT249 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT249       ((uint32_t)0x02000000)          /*!< Protects Sector 249 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT250_OFS   (26)                            /*!< PROT250 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT250       ((uint32_t)0x04000000)          /*!< Protects Sector 250 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT251_OFS   (27)                            /*!< PROT251 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT251       ((uint32_t)0x08000000)          /*!< Protects Sector 251 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT252_OFS   (28)                            /*!< PROT252 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT252       ((uint32_t)0x10000000)          /*!< Protects Sector 252 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT253_OFS   (29)                            /*!< PROT253 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT253       ((uint32_t)0x20000000)          /*!< Protects Sector 253 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT254_OFS   (30)                            /*!< PROT254 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT254       ((uint32_t)0x40000000)          /*!< Protects Sector 254 from program or erase */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT255_OFS   (31)                            /*!< PROT255 Bit Offset */
#define FLCTL_A_BANK0_MAIN_WEPROT7_PROT255       ((uint32_t)0x80000000)          /*!< Protects Sector 255 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT0_OFS     ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT0         ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT1_OFS     ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT1         ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT2_OFS     ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT2         ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT3_OFS     ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT3         ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT4_OFS     ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT4         ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT5_OFS     ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT5         ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT6_OFS     ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT6         ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT7_OFS     ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT7         ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT8_OFS     ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT8         ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT9_OFS     ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT9         ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT10_OFS    (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT10        ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT11_OFS    (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT11        ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT12_OFS    (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT12        ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT13_OFS    (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT13        ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT14_OFS    (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT14        ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT15_OFS    (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT15        ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT16_OFS    (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT16        ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT17_OFS    (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT17        ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT18_OFS    (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT18        ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT19_OFS    (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT19        ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT20_OFS    (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT20        ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT21_OFS    (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT21        ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT22_OFS    (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT22        ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT23_OFS    (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT23        ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT24_OFS    (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT24        ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT25_OFS    (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT25        ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT26_OFS    (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT26        ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT27_OFS    (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT27        ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT28_OFS    (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT28        ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT29_OFS    (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT29        ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT30_OFS    (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT30        ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT31_OFS    (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT0_PROT31        ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT32_OFS    ( 0)                            /*!< PROT32 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT32        ((uint32_t)0x00000001)          /*!< Protects Sector 32 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT33_OFS    ( 1)                            /*!< PROT33 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT33        ((uint32_t)0x00000002)          /*!< Protects Sector 33 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT34_OFS    ( 2)                            /*!< PROT34 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT34        ((uint32_t)0x00000004)          /*!< Protects Sector 34 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT35_OFS    ( 3)                            /*!< PROT35 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT35        ((uint32_t)0x00000008)          /*!< Protects Sector 35 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT36_OFS    ( 4)                            /*!< PROT36 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT36        ((uint32_t)0x00000010)          /*!< Protects Sector 36 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT37_OFS    ( 5)                            /*!< PROT37 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT37        ((uint32_t)0x00000020)          /*!< Protects Sector 37 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT38_OFS    ( 6)                            /*!< PROT38 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT38        ((uint32_t)0x00000040)          /*!< Protects Sector 38 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT39_OFS    ( 7)                            /*!< PROT39 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT39        ((uint32_t)0x00000080)          /*!< Protects Sector 39 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT40_OFS    ( 8)                            /*!< PROT40 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT40        ((uint32_t)0x00000100)          /*!< Protects Sector 40 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT41_OFS    ( 9)                            /*!< PROT41 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT41        ((uint32_t)0x00000200)          /*!< Protects Sector 41 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT42_OFS    (10)                            /*!< PROT42 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT42        ((uint32_t)0x00000400)          /*!< Protects Sector 42 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT43_OFS    (11)                            /*!< PROT43 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT43        ((uint32_t)0x00000800)          /*!< Protects Sector 43 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT44_OFS    (12)                            /*!< PROT44 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT44        ((uint32_t)0x00001000)          /*!< Protects Sector 44 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT45_OFS    (13)                            /*!< PROT45 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT45        ((uint32_t)0x00002000)          /*!< Protects Sector 45 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT46_OFS    (14)                            /*!< PROT46 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT46        ((uint32_t)0x00004000)          /*!< Protects Sector 46 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT47_OFS    (15)                            /*!< PROT47 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT47        ((uint32_t)0x00008000)          /*!< Protects Sector 47 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT48_OFS    (16)                            /*!< PROT48 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT48        ((uint32_t)0x00010000)          /*!< Protects Sector 48 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT49_OFS    (17)                            /*!< PROT49 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT49        ((uint32_t)0x00020000)          /*!< Protects Sector 49 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT50_OFS    (18)                            /*!< PROT50 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT50        ((uint32_t)0x00040000)          /*!< Protects Sector 50 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT51_OFS    (19)                            /*!< PROT51 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT51        ((uint32_t)0x00080000)          /*!< Protects Sector 51 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT52_OFS    (20)                            /*!< PROT52 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT52        ((uint32_t)0x00100000)          /*!< Protects Sector 52 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT53_OFS    (21)                            /*!< PROT53 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT53        ((uint32_t)0x00200000)          /*!< Protects Sector 53 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT54_OFS    (22)                            /*!< PROT54 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT54        ((uint32_t)0x00400000)          /*!< Protects Sector 54 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT55_OFS    (23)                            /*!< PROT55 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT55        ((uint32_t)0x00800000)          /*!< Protects Sector 55 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT56_OFS    (24)                            /*!< PROT56 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT56        ((uint32_t)0x01000000)          /*!< Protects Sector 56 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT57_OFS    (25)                            /*!< PROT57 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT57        ((uint32_t)0x02000000)          /*!< Protects Sector 57 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT58_OFS    (26)                            /*!< PROT58 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT58        ((uint32_t)0x04000000)          /*!< Protects Sector 58 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT59_OFS    (27)                            /*!< PROT59 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT59        ((uint32_t)0x08000000)          /*!< Protects Sector 59 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT60_OFS    (28)                            /*!< PROT60 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT60        ((uint32_t)0x10000000)          /*!< Protects Sector 60 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT61_OFS    (29)                            /*!< PROT61 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT61        ((uint32_t)0x20000000)          /*!< Protects Sector 61 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT62_OFS    (30)                            /*!< PROT62 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT62        ((uint32_t)0x40000000)          /*!< Protects Sector 62 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT63_OFS    (31)                            /*!< PROT63 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT1_PROT63        ((uint32_t)0x80000000)          /*!< Protects Sector 63 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT64_OFS    ( 0)                            /*!< PROT64 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT64        ((uint32_t)0x00000001)          /*!< Protects Sector 64 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT65_OFS    ( 1)                            /*!< PROT65 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT65        ((uint32_t)0x00000002)          /*!< Protects Sector 65 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT66_OFS    ( 2)                            /*!< PROT66 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT66        ((uint32_t)0x00000004)          /*!< Protects Sector 66 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT67_OFS    ( 3)                            /*!< PROT67 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT67        ((uint32_t)0x00000008)          /*!< Protects Sector 67 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT68_OFS    ( 4)                            /*!< PROT68 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT68        ((uint32_t)0x00000010)          /*!< Protects Sector 68 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT69_OFS    ( 5)                            /*!< PROT69 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT69        ((uint32_t)0x00000020)          /*!< Protects Sector 69 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT70_OFS    ( 6)                            /*!< PROT70 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT70        ((uint32_t)0x00000040)          /*!< Protects Sector 70 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT71_OFS    ( 7)                            /*!< PROT71 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT71        ((uint32_t)0x00000080)          /*!< Protects Sector 71 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT72_OFS    ( 8)                            /*!< PROT72 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT72        ((uint32_t)0x00000100)          /*!< Protects Sector 72 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT73_OFS    ( 9)                            /*!< PROT73 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT73        ((uint32_t)0x00000200)          /*!< Protects Sector 73 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT74_OFS    (10)                            /*!< PROT74 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT74        ((uint32_t)0x00000400)          /*!< Protects Sector 74 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT75_OFS    (11)                            /*!< PROT75 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT75        ((uint32_t)0x00000800)          /*!< Protects Sector 75 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT76_OFS    (12)                            /*!< PROT76 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT76        ((uint32_t)0x00001000)          /*!< Protects Sector 76 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT77_OFS    (13)                            /*!< PROT77 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT77        ((uint32_t)0x00002000)          /*!< Protects Sector 77 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT78_OFS    (14)                            /*!< PROT78 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT78        ((uint32_t)0x00004000)          /*!< Protects Sector 78 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT79_OFS    (15)                            /*!< PROT79 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT79        ((uint32_t)0x00008000)          /*!< Protects Sector 79 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT80_OFS    (16)                            /*!< PROT80 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT80        ((uint32_t)0x00010000)          /*!< Protects Sector 80 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT81_OFS    (17)                            /*!< PROT81 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT81        ((uint32_t)0x00020000)          /*!< Protects Sector 81 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT82_OFS    (18)                            /*!< PROT82 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT82        ((uint32_t)0x00040000)          /*!< Protects Sector 82 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT83_OFS    (19)                            /*!< PROT83 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT83        ((uint32_t)0x00080000)          /*!< Protects Sector 83 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT84_OFS    (20)                            /*!< PROT84 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT84        ((uint32_t)0x00100000)          /*!< Protects Sector 84 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT85_OFS    (21)                            /*!< PROT85 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT85        ((uint32_t)0x00200000)          /*!< Protects Sector 85 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT86_OFS    (22)                            /*!< PROT86 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT86        ((uint32_t)0x00400000)          /*!< Protects Sector 86 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT87_OFS    (23)                            /*!< PROT87 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT87        ((uint32_t)0x00800000)          /*!< Protects Sector 87 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT88_OFS    (24)                            /*!< PROT88 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT88        ((uint32_t)0x01000000)          /*!< Protects Sector 88 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT89_OFS    (25)                            /*!< PROT89 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT89        ((uint32_t)0x02000000)          /*!< Protects Sector 89 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT90_OFS    (26)                            /*!< PROT90 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT90        ((uint32_t)0x04000000)          /*!< Protects Sector 90 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT91_OFS    (27)                            /*!< PROT91 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT91        ((uint32_t)0x08000000)          /*!< Protects Sector 91 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT92_OFS    (28)                            /*!< PROT92 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT92        ((uint32_t)0x10000000)          /*!< Protects Sector 92 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT93_OFS    (29)                            /*!< PROT93 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT93        ((uint32_t)0x20000000)          /*!< Protects Sector 93 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT94_OFS    (30)                            /*!< PROT94 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT94        ((uint32_t)0x40000000)          /*!< Protects Sector 94 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT95_OFS    (31)                            /*!< PROT95 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT2_PROT95        ((uint32_t)0x80000000)          /*!< Protects Sector 95 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT96_OFS    ( 0)                            /*!< PROT96 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT96        ((uint32_t)0x00000001)          /*!< Protects Sector 96 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT97_OFS    ( 1)                            /*!< PROT97 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT97        ((uint32_t)0x00000002)          /*!< Protects Sector 97 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT98_OFS    ( 2)                            /*!< PROT98 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT98        ((uint32_t)0x00000004)          /*!< Protects Sector 98 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT99_OFS    ( 3)                            /*!< PROT99 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT99        ((uint32_t)0x00000008)          /*!< Protects Sector 99 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT100_OFS   ( 4)                            /*!< PROT100 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT100       ((uint32_t)0x00000010)          /*!< Protects Sector 100 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT101_OFS   ( 5)                            /*!< PROT101 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT101       ((uint32_t)0x00000020)          /*!< Protects Sector 101 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT102_OFS   ( 6)                            /*!< PROT102 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT102       ((uint32_t)0x00000040)          /*!< Protects Sector 102 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT103_OFS   ( 7)                            /*!< PROT103 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT103       ((uint32_t)0x00000080)          /*!< Protects Sector 103 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT104_OFS   ( 8)                            /*!< PROT104 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT104       ((uint32_t)0x00000100)          /*!< Protects Sector 104 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT105_OFS   ( 9)                            /*!< PROT105 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT105       ((uint32_t)0x00000200)          /*!< Protects Sector 105 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT106_OFS   (10)                            /*!< PROT106 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT106       ((uint32_t)0x00000400)          /*!< Protects Sector 106 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT107_OFS   (11)                            /*!< PROT107 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT107       ((uint32_t)0x00000800)          /*!< Protects Sector 107 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT108_OFS   (12)                            /*!< PROT108 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT108       ((uint32_t)0x00001000)          /*!< Protects Sector 108 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT109_OFS   (13)                            /*!< PROT109 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT109       ((uint32_t)0x00002000)          /*!< Protects Sector 109 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT110_OFS   (14)                            /*!< PROT110 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT110       ((uint32_t)0x00004000)          /*!< Protects Sector 110 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT111_OFS   (15)                            /*!< PROT111 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT111       ((uint32_t)0x00008000)          /*!< Protects Sector 111 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT112_OFS   (16)                            /*!< PROT112 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT112       ((uint32_t)0x00010000)          /*!< Protects Sector 112 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT113_OFS   (17)                            /*!< PROT113 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT113       ((uint32_t)0x00020000)          /*!< Protects Sector 113 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT114_OFS   (18)                            /*!< PROT114 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT114       ((uint32_t)0x00040000)          /*!< Protects Sector 114 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT115_OFS   (19)                            /*!< PROT115 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT115       ((uint32_t)0x00080000)          /*!< Protects Sector 115 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT116_OFS   (20)                            /*!< PROT116 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT116       ((uint32_t)0x00100000)          /*!< Protects Sector 116 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT117_OFS   (21)                            /*!< PROT117 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT117       ((uint32_t)0x00200000)          /*!< Protects Sector 117 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT118_OFS   (22)                            /*!< PROT118 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT118       ((uint32_t)0x00400000)          /*!< Protects Sector 118 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT119_OFS   (23)                            /*!< PROT119 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT119       ((uint32_t)0x00800000)          /*!< Protects Sector 119 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT120_OFS   (24)                            /*!< PROT120 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT120       ((uint32_t)0x01000000)          /*!< Protects Sector 120 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT121_OFS   (25)                            /*!< PROT121 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT121       ((uint32_t)0x02000000)          /*!< Protects Sector 121 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT122_OFS   (26)                            /*!< PROT122 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT122       ((uint32_t)0x04000000)          /*!< Protects Sector 122 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT123_OFS   (27)                            /*!< PROT123 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT123       ((uint32_t)0x08000000)          /*!< Protects Sector 123 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT124_OFS   (28)                            /*!< PROT124 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT124       ((uint32_t)0x10000000)          /*!< Protects Sector 124 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT125_OFS   (29)                            /*!< PROT125 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT125       ((uint32_t)0x20000000)          /*!< Protects Sector 125 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT126_OFS   (30)                            /*!< PROT126 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT126       ((uint32_t)0x40000000)          /*!< Protects Sector 126 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT127_OFS   (31)                            /*!< PROT127 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT3_PROT127       ((uint32_t)0x80000000)          /*!< Protects Sector 127 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT128_OFS   ( 0)                            /*!< PROT128 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT128       ((uint32_t)0x00000001)          /*!< Protects Sector 128 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT129_OFS   ( 1)                            /*!< PROT129 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT129       ((uint32_t)0x00000002)          /*!< Protects Sector 129 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT130_OFS   ( 2)                            /*!< PROT130 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT130       ((uint32_t)0x00000004)          /*!< Protects Sector 130 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT131_OFS   ( 3)                            /*!< PROT131 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT131       ((uint32_t)0x00000008)          /*!< Protects Sector 131 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT132_OFS   ( 4)                            /*!< PROT132 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT132       ((uint32_t)0x00000010)          /*!< Protects Sector 132 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT133_OFS   ( 5)                            /*!< PROT133 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT133       ((uint32_t)0x00000020)          /*!< Protects Sector 133 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT134_OFS   ( 6)                            /*!< PROT134 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT134       ((uint32_t)0x00000040)          /*!< Protects Sector 134 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT135_OFS   ( 7)                            /*!< PROT135 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT135       ((uint32_t)0x00000080)          /*!< Protects Sector 135 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT136_OFS   ( 8)                            /*!< PROT136 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT136       ((uint32_t)0x00000100)          /*!< Protects Sector 136 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT137_OFS   ( 9)                            /*!< PROT137 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT137       ((uint32_t)0x00000200)          /*!< Protects Sector 137 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT138_OFS   (10)                            /*!< PROT138 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT138       ((uint32_t)0x00000400)          /*!< Protects Sector 138 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT139_OFS   (11)                            /*!< PROT139 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT139       ((uint32_t)0x00000800)          /*!< Protects Sector 139 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT140_OFS   (12)                            /*!< PROT140 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT140       ((uint32_t)0x00001000)          /*!< Protects Sector 140 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT141_OFS   (13)                            /*!< PROT141 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT141       ((uint32_t)0x00002000)          /*!< Protects Sector 141 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT142_OFS   (14)                            /*!< PROT142 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT142       ((uint32_t)0x00004000)          /*!< Protects Sector 142 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT143_OFS   (15)                            /*!< PROT143 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT143       ((uint32_t)0x00008000)          /*!< Protects Sector 143 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT144_OFS   (16)                            /*!< PROT144 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT144       ((uint32_t)0x00010000)          /*!< Protects Sector 144 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT145_OFS   (17)                            /*!< PROT145 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT145       ((uint32_t)0x00020000)          /*!< Protects Sector 145 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT146_OFS   (18)                            /*!< PROT146 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT146       ((uint32_t)0x00040000)          /*!< Protects Sector 146 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT147_OFS   (19)                            /*!< PROT147 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT147       ((uint32_t)0x00080000)          /*!< Protects Sector 147 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT148_OFS   (20)                            /*!< PROT148 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT148       ((uint32_t)0x00100000)          /*!< Protects Sector 148 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT149_OFS   (21)                            /*!< PROT149 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT149       ((uint32_t)0x00200000)          /*!< Protects Sector 149 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT150_OFS   (22)                            /*!< PROT150 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT150       ((uint32_t)0x00400000)          /*!< Protects Sector 150 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT151_OFS   (23)                            /*!< PROT151 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT151       ((uint32_t)0x00800000)          /*!< Protects Sector 151 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT152_OFS   (24)                            /*!< PROT152 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT152       ((uint32_t)0x01000000)          /*!< Protects Sector 152 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT153_OFS   (25)                            /*!< PROT153 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT153       ((uint32_t)0x02000000)          /*!< Protects Sector 153 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT154_OFS   (26)                            /*!< PROT154 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT154       ((uint32_t)0x04000000)          /*!< Protects Sector 154 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT155_OFS   (27)                            /*!< PROT155 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT155       ((uint32_t)0x08000000)          /*!< Protects Sector 155 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT156_OFS   (28)                            /*!< PROT156 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT156       ((uint32_t)0x10000000)          /*!< Protects Sector 156 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT157_OFS   (29)                            /*!< PROT157 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT157       ((uint32_t)0x20000000)          /*!< Protects Sector 157 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT158_OFS   (30)                            /*!< PROT158 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT158       ((uint32_t)0x40000000)          /*!< Protects Sector 158 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT159_OFS   (31)                            /*!< PROT159 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT4_PROT159       ((uint32_t)0x80000000)          /*!< Protects Sector 159 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT160_OFS   ( 0)                            /*!< PROT160 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT160       ((uint32_t)0x00000001)          /*!< Protects Sector 160 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT161_OFS   ( 1)                            /*!< PROT161 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT161       ((uint32_t)0x00000002)          /*!< Protects Sector 161 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT162_OFS   ( 2)                            /*!< PROT162 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT162       ((uint32_t)0x00000004)          /*!< Protects Sector 162 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT163_OFS   ( 3)                            /*!< PROT163 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT163       ((uint32_t)0x00000008)          /*!< Protects Sector 163 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT164_OFS   ( 4)                            /*!< PROT164 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT164       ((uint32_t)0x00000010)          /*!< Protects Sector 164 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT165_OFS   ( 5)                            /*!< PROT165 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT165       ((uint32_t)0x00000020)          /*!< Protects Sector 165 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT166_OFS   ( 6)                            /*!< PROT166 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT166       ((uint32_t)0x00000040)          /*!< Protects Sector 166 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT167_OFS   ( 7)                            /*!< PROT167 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT167       ((uint32_t)0x00000080)          /*!< Protects Sector 167 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT168_OFS   ( 8)                            /*!< PROT168 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT168       ((uint32_t)0x00000100)          /*!< Protects Sector 168 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT169_OFS   ( 9)                            /*!< PROT169 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT169       ((uint32_t)0x00000200)          /*!< Protects Sector 169 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT170_OFS   (10)                            /*!< PROT170 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT170       ((uint32_t)0x00000400)          /*!< Protects Sector 170 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT171_OFS   (11)                            /*!< PROT171 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT171       ((uint32_t)0x00000800)          /*!< Protects Sector 171 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT172_OFS   (12)                            /*!< PROT172 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT172       ((uint32_t)0x00001000)          /*!< Protects Sector 172 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT173_OFS   (13)                            /*!< PROT173 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT173       ((uint32_t)0x00002000)          /*!< Protects Sector 173 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT174_OFS   (14)                            /*!< PROT174 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT174       ((uint32_t)0x00004000)          /*!< Protects Sector 174 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT175_OFS   (15)                            /*!< PROT175 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT175       ((uint32_t)0x00008000)          /*!< Protects Sector 175 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT176_OFS   (16)                            /*!< PROT176 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT176       ((uint32_t)0x00010000)          /*!< Protects Sector 176 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT177_OFS   (17)                            /*!< PROT177 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT177       ((uint32_t)0x00020000)          /*!< Protects Sector 177 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT178_OFS   (18)                            /*!< PROT178 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT178       ((uint32_t)0x00040000)          /*!< Protects Sector 178 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT179_OFS   (19)                            /*!< PROT179 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT179       ((uint32_t)0x00080000)          /*!< Protects Sector 179 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT180_OFS   (20)                            /*!< PROT180 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT180       ((uint32_t)0x00100000)          /*!< Protects Sector 180 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT181_OFS   (21)                            /*!< PROT181 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT181       ((uint32_t)0x00200000)          /*!< Protects Sector 181 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT182_OFS   (22)                            /*!< PROT182 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT182       ((uint32_t)0x00400000)          /*!< Protects Sector 182 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT183_OFS   (23)                            /*!< PROT183 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT183       ((uint32_t)0x00800000)          /*!< Protects Sector 183 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT184_OFS   (24)                            /*!< PROT184 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT184       ((uint32_t)0x01000000)          /*!< Protects Sector 184 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT185_OFS   (25)                            /*!< PROT185 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT185       ((uint32_t)0x02000000)          /*!< Protects Sector 185 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT186_OFS   (26)                            /*!< PROT186 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT186       ((uint32_t)0x04000000)          /*!< Protects Sector 186 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT187_OFS   (27)                            /*!< PROT187 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT187       ((uint32_t)0x08000000)          /*!< Protects Sector 187 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT188_OFS   (28)                            /*!< PROT188 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT188       ((uint32_t)0x10000000)          /*!< Protects Sector 188 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT189_OFS   (29)                            /*!< PROT189 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT189       ((uint32_t)0x20000000)          /*!< Protects Sector 189 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT190_OFS   (30)                            /*!< PROT190 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT190       ((uint32_t)0x40000000)          /*!< Protects Sector 190 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT191_OFS   (31)                            /*!< PROT191 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT5_PROT191       ((uint32_t)0x80000000)          /*!< Protects Sector 191 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT192_OFS   ( 0)                            /*!< PROT192 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT192       ((uint32_t)0x00000001)          /*!< Protects Sector 192 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT193_OFS   ( 1)                            /*!< PROT193 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT193       ((uint32_t)0x00000002)          /*!< Protects Sector 193 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT194_OFS   ( 2)                            /*!< PROT194 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT194       ((uint32_t)0x00000004)          /*!< Protects Sector 194 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT195_OFS   ( 3)                            /*!< PROT195 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT195       ((uint32_t)0x00000008)          /*!< Protects Sector 195 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT196_OFS   ( 4)                            /*!< PROT196 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT196       ((uint32_t)0x00000010)          /*!< Protects Sector 196 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT197_OFS   ( 5)                            /*!< PROT197 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT197       ((uint32_t)0x00000020)          /*!< Protects Sector 197 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT198_OFS   ( 6)                            /*!< PROT198 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT198       ((uint32_t)0x00000040)          /*!< Protects Sector 198 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT199_OFS   ( 7)                            /*!< PROT199 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT199       ((uint32_t)0x00000080)          /*!< Protects Sector 199 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT200_OFS   ( 8)                            /*!< PROT200 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT200       ((uint32_t)0x00000100)          /*!< Protects Sector 200 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT201_OFS   ( 9)                            /*!< PROT201 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT201       ((uint32_t)0x00000200)          /*!< Protects Sector 201 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT202_OFS   (10)                            /*!< PROT202 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT202       ((uint32_t)0x00000400)          /*!< Protects Sector 202 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT203_OFS   (11)                            /*!< PROT203 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT203       ((uint32_t)0x00000800)          /*!< Protects Sector 203 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT204_OFS   (12)                            /*!< PROT204 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT204       ((uint32_t)0x00001000)          /*!< Protects Sector 204 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT205_OFS   (13)                            /*!< PROT205 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT205       ((uint32_t)0x00002000)          /*!< Protects Sector 205 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT206_OFS   (14)                            /*!< PROT206 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT206       ((uint32_t)0x00004000)          /*!< Protects Sector 206 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT207_OFS   (15)                            /*!< PROT207 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT207       ((uint32_t)0x00008000)          /*!< Protects Sector 207 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT208_OFS   (16)                            /*!< PROT208 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT208       ((uint32_t)0x00010000)          /*!< Protects Sector 208 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT209_OFS   (17)                            /*!< PROT209 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT209       ((uint32_t)0x00020000)          /*!< Protects Sector 209 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT210_OFS   (18)                            /*!< PROT210 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT210       ((uint32_t)0x00040000)          /*!< Protects Sector 210 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT211_OFS   (19)                            /*!< PROT211 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT211       ((uint32_t)0x00080000)          /*!< Protects Sector 211 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT212_OFS   (20)                            /*!< PROT212 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT212       ((uint32_t)0x00100000)          /*!< Protects Sector 212 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT213_OFS   (21)                            /*!< PROT213 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT213       ((uint32_t)0x00200000)          /*!< Protects Sector 213 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT214_OFS   (22)                            /*!< PROT214 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT214       ((uint32_t)0x00400000)          /*!< Protects Sector 214 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT215_OFS   (23)                            /*!< PROT215 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT215       ((uint32_t)0x00800000)          /*!< Protects Sector 215 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT216_OFS   (24)                            /*!< PROT216 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT216       ((uint32_t)0x01000000)          /*!< Protects Sector 216 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT217_OFS   (25)                            /*!< PROT217 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT217       ((uint32_t)0x02000000)          /*!< Protects Sector 217 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT218_OFS   (26)                            /*!< PROT218 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT218       ((uint32_t)0x04000000)          /*!< Protects Sector 218 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT219_OFS   (27)                            /*!< PROT219 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT219       ((uint32_t)0x08000000)          /*!< Protects Sector 219 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT220_OFS   (28)                            /*!< PROT220 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT220       ((uint32_t)0x10000000)          /*!< Protects Sector 220 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT221_OFS   (29)                            /*!< PROT221 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT221       ((uint32_t)0x20000000)          /*!< Protects Sector 221 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT222_OFS   (30)                            /*!< PROT222 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT222       ((uint32_t)0x40000000)          /*!< Protects Sector 222 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT223_OFS   (31)                            /*!< PROT223 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT6_PROT223       ((uint32_t)0x80000000)          /*!< Protects Sector 223 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT224_OFS   ( 0)                            /*!< PROT224 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT224       ((uint32_t)0x00000001)          /*!< Protects Sector 224 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT225_OFS   ( 1)                            /*!< PROT225 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT225       ((uint32_t)0x00000002)          /*!< Protects Sector 225 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT226_OFS   ( 2)                            /*!< PROT226 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT226       ((uint32_t)0x00000004)          /*!< Protects Sector 226 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT227_OFS   ( 3)                            /*!< PROT227 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT227       ((uint32_t)0x00000008)          /*!< Protects Sector 227 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT228_OFS   ( 4)                            /*!< PROT228 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT228       ((uint32_t)0x00000010)          /*!< Protects Sector 228 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT229_OFS   ( 5)                            /*!< PROT229 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT229       ((uint32_t)0x00000020)          /*!< Protects Sector 229 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT230_OFS   ( 6)                            /*!< PROT230 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT230       ((uint32_t)0x00000040)          /*!< Protects Sector 230 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT231_OFS   ( 7)                            /*!< PROT231 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT231       ((uint32_t)0x00000080)          /*!< Protects Sector 231 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT232_OFS   ( 8)                            /*!< PROT232 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT232       ((uint32_t)0x00000100)          /*!< Protects Sector 232 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT233_OFS   ( 9)                            /*!< PROT233 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT233       ((uint32_t)0x00000200)          /*!< Protects Sector 233 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT234_OFS   (10)                            /*!< PROT234 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT234       ((uint32_t)0x00000400)          /*!< Protects Sector 234 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT235_OFS   (11)                            /*!< PROT235 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT235       ((uint32_t)0x00000800)          /*!< Protects Sector 235 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT236_OFS   (12)                            /*!< PROT236 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT236       ((uint32_t)0x00001000)          /*!< Protects Sector 236 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT237_OFS   (13)                            /*!< PROT237 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT237       ((uint32_t)0x00002000)          /*!< Protects Sector 237 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT238_OFS   (14)                            /*!< PROT238 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT238       ((uint32_t)0x00004000)          /*!< Protects Sector 238 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT239_OFS   (15)                            /*!< PROT239 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT239       ((uint32_t)0x00008000)          /*!< Protects Sector 239 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT240_OFS   (16)                            /*!< PROT240 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT240       ((uint32_t)0x00010000)          /*!< Protects Sector 240 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT241_OFS   (17)                            /*!< PROT241 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT241       ((uint32_t)0x00020000)          /*!< Protects Sector 241 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT242_OFS   (18)                            /*!< PROT242 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT242       ((uint32_t)0x00040000)          /*!< Protects Sector 242 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT243_OFS   (19)                            /*!< PROT243 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT243       ((uint32_t)0x00080000)          /*!< Protects Sector 243 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT244_OFS   (20)                            /*!< PROT244 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT244       ((uint32_t)0x00100000)          /*!< Protects Sector 244 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT245_OFS   (21)                            /*!< PROT245 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT245       ((uint32_t)0x00200000)          /*!< Protects Sector 245 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT246_OFS   (22)                            /*!< PROT246 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT246       ((uint32_t)0x00400000)          /*!< Protects Sector 246 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT247_OFS   (23)                            /*!< PROT247 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT247       ((uint32_t)0x00800000)          /*!< Protects Sector 247 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT248_OFS   (24)                            /*!< PROT248 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT248       ((uint32_t)0x01000000)          /*!< Protects Sector 248 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT249_OFS   (25)                            /*!< PROT249 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT249       ((uint32_t)0x02000000)          /*!< Protects Sector 249 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT250_OFS   (26)                            /*!< PROT250 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT250       ((uint32_t)0x04000000)          /*!< Protects Sector 250 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT251_OFS   (27)                            /*!< PROT251 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT251       ((uint32_t)0x08000000)          /*!< Protects Sector 251 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT252_OFS   (28)                            /*!< PROT252 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT252       ((uint32_t)0x10000000)          /*!< Protects Sector 252 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT253_OFS   (29)                            /*!< PROT253 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT253       ((uint32_t)0x20000000)          /*!< Protects Sector 253 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT254_OFS   (30)                            /*!< PROT254 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT254       ((uint32_t)0x40000000)          /*!< Protects Sector 254 from program or erase */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT255_OFS   (31)                            /*!< PROT255 Bit Offset */
#define FLCTL_A_BANK1_MAIN_WEPROT7_PROT255       ((uint32_t)0x80000000)          /*!< Protects Sector 255 from program or erase */
#define LCD_F_CTL_ON_OFS                         ( 0)                            /*!< LCDON Bit Offset */
#define LCD_F_CTL_ON                             ((uint32_t)0x00000001)          /*!< LCD on */
#define LCD_F_CTL_LP_OFS                         ( 1)                            /*!< LCDLP Bit Offset */
#define LCD_F_CTL_LP                             ((uint32_t)0x00000002)          /*!< LCD Low-power Waveform */
#define LCD_F_CTL_SON_OFS                        ( 2)                            /*!< LCDSON Bit Offset */
#define LCD_F_CTL_SON                            ((uint32_t)0x00000004)          /*!< LCD segments on */
#define LCD_F_CTL_MX_OFS                         ( 3)                            /*!< LCDMXx Bit Offset */
#define LCD_F_CTL_MX_MASK                        ((uint32_t)0x00000038)          /*!< LCDMXx Bit Mask */
#define LCD_F_CTL_MX0                            ((uint32_t)0x00000008)          /*!< MX Bit 0 */
#define LCD_F_CTL_MX1                            ((uint32_t)0x00000010)          /*!< MX Bit 1 */
#define LCD_F_CTL_MX2                            ((uint32_t)0x00000020)          /*!< MX Bit 2 */
#define LCD_F_CTL_MX_0                           ((uint32_t)0x00000000)          /*!< Static */
#define LCD_F_CTL_MX_1                           ((uint32_t)0x00000008)          /*!< 2-mux */
#define LCD_F_CTL_MX_2                           ((uint32_t)0x00000010)          /*!< 3-mux */
#define LCD_F_CTL_MX_3                           ((uint32_t)0x00000018)          /*!< 4-mux */
#define LCD_F_CTL_MX_4                           ((uint32_t)0x00000020)          /*!< 5-mux */
#define LCD_F_CTL_MX_5                           ((uint32_t)0x00000028)          /*!< 6-mux */
#define LCD_F_CTL_MX_6                           ((uint32_t)0x00000030)          /*!< 7-mux */
#define LCD_F_CTL_MX_7                           ((uint32_t)0x00000038)          /*!< 8-mux */
#define LCD_F_CTL_PRE_OFS                        ( 8)                            /*!< LCDPREx Bit Offset */
#define LCD_F_CTL_PRE_MASK                       ((uint32_t)0x00000700)          /*!< LCDPREx Bit Mask */
#define LCD_F_CTL_PRE0                           ((uint32_t)0x00000100)          /*!< PRE Bit 0 */
#define LCD_F_CTL_PRE1                           ((uint32_t)0x00000200)          /*!< PRE Bit 1 */
#define LCD_F_CTL_PRE2                           ((uint32_t)0x00000400)          /*!< PRE Bit 2 */
#define LCD_F_CTL_PRE_0                          ((uint32_t)0x00000000)          /*!< Divide by 1 */
#define LCD_F_CTL_PRE_1                          ((uint32_t)0x00000100)          /*!< Divide by 2 */
#define LCD_F_CTL_PRE_2                          ((uint32_t)0x00000200)          /*!< Divide by 4 */
#define LCD_F_CTL_PRE_3                          ((uint32_t)0x00000300)          /*!< Divide by 8 */
#define LCD_F_CTL_PRE_4                          ((uint32_t)0x00000400)          /*!< Divide by 16 */
#define LCD_F_CTL_PRE_5                          ((uint32_t)0x00000500)          /*!< Divide by 32 */
#define LCD_F_CTL_PRE_6                          ((uint32_t)0x00000600)          /*!< Reserved (defaults to divide by 32) */
#define LCD_F_CTL_PRE_7                          ((uint32_t)0x00000700)          /*!< Reserved (defaults to divide by 32) */
#define LCD_F_CTL_DIV_OFS                        (11)                            /*!< LCDDIVx Bit Offset */
#define LCD_F_CTL_DIV_MASK                       ((uint32_t)0x0000F800)          /*!< LCDDIVx Bit Mask */
#define LCD_F_CTL_DIV0                           ((uint32_t)0x00000800)          /*!< DIV Bit 0 */
#define LCD_F_CTL_DIV1                           ((uint32_t)0x00001000)          /*!< DIV Bit 1 */
#define LCD_F_CTL_DIV2                           ((uint32_t)0x00002000)          /*!< DIV Bit 2 */
#define LCD_F_CTL_DIV3                           ((uint32_t)0x00004000)          /*!< DIV Bit 3 */
#define LCD_F_CTL_DIV4                           ((uint32_t)0x00008000)          /*!< DIV Bit 4 */
#define LCD_F_CTL_DIV_0                          ((uint32_t)0x00000000)          /*!< Divide by 1 */
#define LCD_F_CTL_DIV_1                          ((uint32_t)0x00000800)          /*!< Divide by 2 */
#define LCD_F_CTL_DIV_2                          ((uint32_t)0x00001000)          /*!< Divide by 3 */
#define LCD_F_CTL_DIV_3                          ((uint32_t)0x00001800)          /*!< Divide by 4 */
#define LCD_F_CTL_DIV_4                          ((uint32_t)0x00002000)          /*!< Divide by 5 */
#define LCD_F_CTL_DIV_5                          ((uint32_t)0x00002800)          /*!< Divide by 6 */
#define LCD_F_CTL_DIV_6                          ((uint32_t)0x00003000)          /*!< Divide by 7 */
#define LCD_F_CTL_DIV_7                          ((uint32_t)0x00003800)          /*!< Divide by 8 */
#define LCD_F_CTL_DIV_8                          ((uint32_t)0x00004000)          /*!< Divide by 9 */
#define LCD_F_CTL_DIV_9                          ((uint32_t)0x00004800)          /*!< Divide by 10 */
#define LCD_F_CTL_DIV_10                         ((uint32_t)0x00005000)          /*!< Divide by 11 */
#define LCD_F_CTL_DIV_11                         ((uint32_t)0x00005800)          /*!< Divide by 12 */
#define LCD_F_CTL_DIV_12                         ((uint32_t)0x00006000)          /*!< Divide by 13 */
#define LCD_F_CTL_DIV_13                         ((uint32_t)0x00006800)          /*!< Divide by 14 */
#define LCD_F_CTL_DIV_14                         ((uint32_t)0x00007000)          /*!< Divide by 15 */
#define LCD_F_CTL_DIV_15                         ((uint32_t)0x00007800)          /*!< Divide by 16 */
#define LCD_F_CTL_DIV_16                         ((uint32_t)0x00008000)          /*!< Divide by 17 */
#define LCD_F_CTL_DIV_17                         ((uint32_t)0x00008800)          /*!< Divide by 18 */
#define LCD_F_CTL_DIV_18                         ((uint32_t)0x00009000)          /*!< Divide by 19 */
#define LCD_F_CTL_DIV_19                         ((uint32_t)0x00009800)          /*!< Divide by 20 */
#define LCD_F_CTL_DIV_20                         ((uint32_t)0x0000A000)          /*!< Divide by 21 */
#define LCD_F_CTL_DIV_21                         ((uint32_t)0x0000A800)          /*!< Divide by 22 */
#define LCD_F_CTL_DIV_22                         ((uint32_t)0x0000B000)          /*!< Divide by 23 */
#define LCD_F_CTL_DIV_23                         ((uint32_t)0x0000B800)          /*!< Divide by 24 */
#define LCD_F_CTL_DIV_24                         ((uint32_t)0x0000C000)          /*!< Divide by 25 */
#define LCD_F_CTL_DIV_25                         ((uint32_t)0x0000C800)          /*!< Divide by 26 */
#define LCD_F_CTL_DIV_26                         ((uint32_t)0x0000D000)          /*!< Divide by 27 */
#define LCD_F_CTL_DIV_27                         ((uint32_t)0x0000D800)          /*!< Divide by 28 */
#define LCD_F_CTL_DIV_28                         ((uint32_t)0x0000E000)          /*!< Divide by 29 */
#define LCD_F_CTL_DIV_29                         ((uint32_t)0x0000E800)          /*!< Divide by 30 */
#define LCD_F_CTL_DIV_30                         ((uint32_t)0x0000F000)          /*!< Divide by 31 */
#define LCD_F_CTL_DIV_31                         ((uint32_t)0x0000F800)          /*!< Divide by 32 */
#define LCD_F_CTL_SSEL_OFS                       (16)                            /*!< LCDSSEL Bit Offset */
#define LCD_F_CTL_SSEL_MASK                      ((uint32_t)0x00030000)          /*!< LCDSSEL Bit Mask */
#define LCD_F_CTL_SSEL0                          ((uint32_t)0x00010000)          /*!< SSEL Bit 0 */
#define LCD_F_CTL_SSEL1                          ((uint32_t)0x00020000)          /*!< SSEL Bit 1 */
#define LCD_F_CTL_SSEL_0                         ((uint32_t)0x00000000)          /*!< ACLK */
#define LCD_F_CTL_SSEL_1                         ((uint32_t)0x00010000)          /*!< VLOCLK */
#define LCD_F_CTL_SSEL_2                         ((uint32_t)0x00020000)          /*!< REFOCLK */
#define LCD_F_CTL_SSEL_3                         ((uint32_t)0x00030000)          /*!< LFXTCLK */
#define LCD_F_BMCTL_BLKMOD_OFS                   ( 0)                            /*!< LCDBLKMODx Bit Offset */
#define LCD_F_BMCTL_BLKMOD_MASK                  ((uint32_t)0x00000003)          /*!< LCDBLKMODx Bit Mask */
#define LCD_F_BMCTL_BLKMOD0                      ((uint32_t)0x00000001)          /*!< BLKMOD Bit 0 */
#define LCD_F_BMCTL_BLKMOD1                      ((uint32_t)0x00000002)          /*!< BLKMOD Bit 1 */
#define LCD_F_BMCTL_BLKMOD_0                     ((uint32_t)0x00000000)          /*!< Blinking disabled */
#define LCD_F_BMCTL_BLKMOD_1                     ((uint32_t)0x00000001)          /*!< Blinking of individual segments as enabled in blinking memory register  */
#define LCD_F_BMCTL_BLKMOD_2                     ((uint32_t)0x00000002)          /*!< Blinking of all segments */
#define LCD_F_BMCTL_BLKMOD_3                     ((uint32_t)0x00000003)          /*!< Switching between display contents as stored in LCDMx and LCDBMx memory  */
#define LCD_F_BMCTL_BLKPRE_OFS                   ( 2)                            /*!< LCDBLKPREx Bit Offset */
#define LCD_F_BMCTL_BLKPRE_MASK                  ((uint32_t)0x0000001C)          /*!< LCDBLKPREx Bit Mask */
#define LCD_F_BMCTL_BLKPRE0                      ((uint32_t)0x00000004)          /*!< BLKPRE Bit 0 */
#define LCD_F_BMCTL_BLKPRE1                      ((uint32_t)0x00000008)          /*!< BLKPRE Bit 1 */
#define LCD_F_BMCTL_BLKPRE2                      ((uint32_t)0x00000010)          /*!< BLKPRE Bit 2 */
#define LCD_F_BMCTL_BLKPRE_0                     ((uint32_t)0x00000000)          /*!< Divide by 512 */
#define LCD_F_BMCTL_BLKPRE_1                     ((uint32_t)0x00000004)          /*!< Divide by 1024 */
#define LCD_F_BMCTL_BLKPRE_2                     ((uint32_t)0x00000008)          /*!< Divide by 2048 */
#define LCD_F_BMCTL_BLKPRE_3                     ((uint32_t)0x0000000C)          /*!< Divide by 4096 */
#define LCD_F_BMCTL_BLKPRE_4                     ((uint32_t)0x00000010)          /*!< Divide by 8162 */
#define LCD_F_BMCTL_BLKPRE_5                     ((uint32_t)0x00000014)          /*!< Divide by 16384 */
#define LCD_F_BMCTL_BLKPRE_6                     ((uint32_t)0x00000018)          /*!< Divide by 32768 */
#define LCD_F_BMCTL_BLKPRE_7                     ((uint32_t)0x0000001C)          /*!< Divide by 65536 */
#define LCD_F_BMCTL_BLKDIV_OFS                   ( 5)                            /*!< LCDBLKDIVx Bit Offset */
#define LCD_F_BMCTL_BLKDIV_MASK                  ((uint32_t)0x000000E0)          /*!< LCDBLKDIVx Bit Mask */
#define LCD_F_BMCTL_BLKDIV0                      ((uint32_t)0x00000020)          /*!< BLKDIV Bit 0 */
#define LCD_F_BMCTL_BLKDIV1                      ((uint32_t)0x00000040)          /*!< BLKDIV Bit 1 */
#define LCD_F_BMCTL_BLKDIV2                      ((uint32_t)0x00000080)          /*!< BLKDIV Bit 2 */
#define LCD_F_BMCTL_BLKDIV_0                     ((uint32_t)0x00000000)          /*!< Divide by 1 */
#define LCD_F_BMCTL_BLKDIV_1                     ((uint32_t)0x00000020)          /*!< Divide by 2 */
#define LCD_F_BMCTL_BLKDIV_2                     ((uint32_t)0x00000040)          /*!< Divide by 3 */
#define LCD_F_BMCTL_BLKDIV_3                     ((uint32_t)0x00000060)          /*!< Divide by 4 */
#define LCD_F_BMCTL_BLKDIV_4                     ((uint32_t)0x00000080)          /*!< Divide by 5 */
#define LCD_F_BMCTL_BLKDIV_5                     ((uint32_t)0x000000A0)          /*!< Divide by 6 */
#define LCD_F_BMCTL_BLKDIV_6                     ((uint32_t)0x000000C0)          /*!< Divide by 7 */
#define LCD_F_BMCTL_BLKDIV_7                     ((uint32_t)0x000000E0)          /*!< Divide by 8 */
#define LCD_F_BMCTL_DISP_OFS                     (16)                            /*!< LCDDISP Bit Offset */
#define LCD_F_BMCTL_DISP                         ((uint32_t)0x00010000)          /*!< Select LCD memory registers for display */
#define LCD_F_BMCTL_CLRM_OFS                     (17)                            /*!< LCDCLRM Bit Offset */
#define LCD_F_BMCTL_CLRM                         ((uint32_t)0x00020000)          /*!< Clear LCD memory */
#define LCD_F_BMCTL_CLRBM_OFS                    (18)                            /*!< LCDCLRBM Bit Offset */
#define LCD_F_BMCTL_CLRBM                        ((uint32_t)0x00040000)          /*!< Clear LCD blinking memory */
#define LCD_F_VCTL_LCD2B_OFS                     ( 0)                            /*!< LCD2B Bit Offset */
#define LCD_F_VCTL_LCD2B                         ((uint32_t)0x00000001)          /*!< Bias select. */
#define LCD_F_VCTL_EXTBIAS_OFS                   ( 5)                            /*!< LCDEXTBIAS Bit Offset */
#define LCD_F_VCTL_EXTBIAS                       ((uint32_t)0x00000020)          /*!< V2 to V4 voltage select */
#define LCD_F_VCTL_R03EXT_OFS                    ( 6)                            /*!< R03EXT Bit Offset */
#define LCD_F_VCTL_R03EXT                        ((uint32_t)0x00000040)          /*!< V5 voltage select */
#define LCD_F_VCTL_REXT_OFS                      ( 7)                            /*!< LCDREXT Bit Offset */
#define LCD_F_VCTL_REXT                          ((uint32_t)0x00000080)          /*!< V2 to V4 voltage on external Rx3 pins */
#define LCD_F_PCTL0_S0_OFS                       ( 0)                            /*!< LCDS0 Bit Offset */
#define LCD_F_PCTL0_S0                           ((uint32_t)0x00000001)          /*!< LCD pin 0 enable */
#define LCD_F_PCTL0_S1_OFS                       ( 1)                            /*!< LCDS1 Bit Offset */
#define LCD_F_PCTL0_S1                           ((uint32_t)0x00000002)          /*!< LCD pin 1 enable */
#define LCD_F_PCTL0_S2_OFS                       ( 2)                            /*!< LCDS2 Bit Offset */
#define LCD_F_PCTL0_S2                           ((uint32_t)0x00000004)          /*!< LCD pin 2 enable */
#define LCD_F_PCTL0_S3_OFS                       ( 3)                            /*!< LCDS3 Bit Offset */
#define LCD_F_PCTL0_S3                           ((uint32_t)0x00000008)          /*!< LCD pin 3 enable */
#define LCD_F_PCTL0_S4_OFS                       ( 4)                            /*!< LCDS4 Bit Offset */
#define LCD_F_PCTL0_S4                           ((uint32_t)0x00000010)          /*!< LCD pin 4 enable */
#define LCD_F_PCTL0_S5_OFS                       ( 5)                            /*!< LCDS5 Bit Offset */
#define LCD_F_PCTL0_S5                           ((uint32_t)0x00000020)          /*!< LCD pin 5 enable */
#define LCD_F_PCTL0_S6_OFS                       ( 6)                            /*!< LCDS6 Bit Offset */
#define LCD_F_PCTL0_S6                           ((uint32_t)0x00000040)          /*!< LCD pin 6 enable */
#define LCD_F_PCTL0_S7_OFS                       ( 7)                            /*!< LCDS7 Bit Offset */
#define LCD_F_PCTL0_S7                           ((uint32_t)0x00000080)          /*!< LCD pin 7 enable */
#define LCD_F_PCTL0_S8_OFS                       ( 8)                            /*!< LCDS8 Bit Offset */
#define LCD_F_PCTL0_S8                           ((uint32_t)0x00000100)          /*!< LCD pin 8 enable */
#define LCD_F_PCTL0_S9_OFS                       ( 9)                            /*!< LCDS9 Bit Offset */
#define LCD_F_PCTL0_S9                           ((uint32_t)0x00000200)          /*!< LCD pin 9 enable */
#define LCD_F_PCTL0_S10_OFS                      (10)                            /*!< LCDS10 Bit Offset */
#define LCD_F_PCTL0_S10                          ((uint32_t)0x00000400)          /*!< LCD pin 10 enable */
#define LCD_F_PCTL0_S11_OFS                      (11)                            /*!< LCDS11 Bit Offset */
#define LCD_F_PCTL0_S11                          ((uint32_t)0x00000800)          /*!< LCD pin 11 enable */
#define LCD_F_PCTL0_S12_OFS                      (12)                            /*!< LCDS12 Bit Offset */
#define LCD_F_PCTL0_S12                          ((uint32_t)0x00001000)          /*!< LCD pin 12 enable */
#define LCD_F_PCTL0_S13_OFS                      (13)                            /*!< LCDS13 Bit Offset */
#define LCD_F_PCTL0_S13                          ((uint32_t)0x00002000)          /*!< LCD pin 13 enable */
#define LCD_F_PCTL0_S14_OFS                      (14)                            /*!< LCDS14 Bit Offset */
#define LCD_F_PCTL0_S14                          ((uint32_t)0x00004000)          /*!< LCD pin 14 enable */
#define LCD_F_PCTL0_S15_OFS                      (15)                            /*!< LCDS15 Bit Offset */
#define LCD_F_PCTL0_S15                          ((uint32_t)0x00008000)          /*!< LCD pin 15 enable */
#define LCD_F_PCTL0_S16_OFS                      (16)                            /*!< LCDS16 Bit Offset */
#define LCD_F_PCTL0_S16                          ((uint32_t)0x00010000)          /*!< LCD pin 16 enable */
#define LCD_F_PCTL0_S17_OFS                      (17)                            /*!< LCDS17 Bit Offset */
#define LCD_F_PCTL0_S17                          ((uint32_t)0x00020000)          /*!< LCD pin 17 enable */
#define LCD_F_PCTL0_S18_OFS                      (18)                            /*!< LCDS18 Bit Offset */
#define LCD_F_PCTL0_S18                          ((uint32_t)0x00040000)          /*!< LCD pin 18 enable */
#define LCD_F_PCTL0_S19_OFS                      (19)                            /*!< LCDS19 Bit Offset */
#define LCD_F_PCTL0_S19                          ((uint32_t)0x00080000)          /*!< LCD pin 19 enable */
#define LCD_F_PCTL0_S20_OFS                      (20)                            /*!< LCDS20 Bit Offset */
#define LCD_F_PCTL0_S20                          ((uint32_t)0x00100000)          /*!< LCD pin 20 enable */
#define LCD_F_PCTL0_S21_OFS                      (21)                            /*!< LCDS21 Bit Offset */
#define LCD_F_PCTL0_S21                          ((uint32_t)0x00200000)          /*!< LCD pin 21 enable */
#define LCD_F_PCTL0_S22_OFS                      (22)                            /*!< LCDS22 Bit Offset */
#define LCD_F_PCTL0_S22                          ((uint32_t)0x00400000)          /*!< LCD pin 22 enable */
#define LCD_F_PCTL0_S23_OFS                      (23)                            /*!< LCDS23 Bit Offset */
#define LCD_F_PCTL0_S23                          ((uint32_t)0x00800000)          /*!< LCD pin 23 enable */
#define LCD_F_PCTL0_S24_OFS                      (24)                            /*!< LCDS24 Bit Offset */
#define LCD_F_PCTL0_S24                          ((uint32_t)0x01000000)          /*!< LCD pin 24 enable */
#define LCD_F_PCTL0_S25_OFS                      (25)                            /*!< LCDS25 Bit Offset */
#define LCD_F_PCTL0_S25                          ((uint32_t)0x02000000)          /*!< LCD pin 25 enable */
#define LCD_F_PCTL0_S26_OFS                      (26)                            /*!< LCDS26 Bit Offset */
#define LCD_F_PCTL0_S26                          ((uint32_t)0x04000000)          /*!< LCD pin 26 enable */
#define LCD_F_PCTL0_S27_OFS                      (27)                            /*!< LCDS27 Bit Offset */
#define LCD_F_PCTL0_S27                          ((uint32_t)0x08000000)          /*!< LCD pin 27 enable */
#define LCD_F_PCTL0_S28_OFS                      (28)                            /*!< LCDS28 Bit Offset */
#define LCD_F_PCTL0_S28                          ((uint32_t)0x10000000)          /*!< LCD pin 28 enable */
#define LCD_F_PCTL0_S29_OFS                      (29)                            /*!< LCDS29 Bit Offset */
#define LCD_F_PCTL0_S29                          ((uint32_t)0x20000000)          /*!< LCD pin 29 enable */
#define LCD_F_PCTL0_S30_OFS                      (30)                            /*!< LCDS30 Bit Offset */
#define LCD_F_PCTL0_S30                          ((uint32_t)0x40000000)          /*!< LCD pin 30 enable */
#define LCD_F_PCTL0_S31_OFS                      (31)                            /*!< LCDS31 Bit Offset */
#define LCD_F_PCTL0_S31                          ((uint32_t)0x80000000)          /*!< LCD pin 31 enable */
#define LCD_F_PCTL1_S32_OFS                      ( 0)                            /*!< LCDS32 Bit Offset */
#define LCD_F_PCTL1_S32                          ((uint32_t)0x00000001)          /*!< LCD pin 32 enable */
#define LCD_F_PCTL1_S33_OFS                      ( 1)                            /*!< LCDS33 Bit Offset */
#define LCD_F_PCTL1_S33                          ((uint32_t)0x00000002)          /*!< LCD pin 33 enable */
#define LCD_F_PCTL1_S34_OFS                      ( 2)                            /*!< LCDS34 Bit Offset */
#define LCD_F_PCTL1_S34                          ((uint32_t)0x00000004)          /*!< LCD pin 34 enable */
#define LCD_F_PCTL1_S35_OFS                      ( 3)                            /*!< LCDS35 Bit Offset */
#define LCD_F_PCTL1_S35                          ((uint32_t)0x00000008)          /*!< LCD pin 35 enable */
#define LCD_F_PCTL1_S36_OFS                      ( 4)                            /*!< LCDS36 Bit Offset */
#define LCD_F_PCTL1_S36                          ((uint32_t)0x00000010)          /*!< LCD pin 36 enable */
#define LCD_F_PCTL1_S37_OFS                      ( 5)                            /*!< LCDS37 Bit Offset */
#define LCD_F_PCTL1_S37                          ((uint32_t)0x00000020)          /*!< LCD pin 37 enable */
#define LCD_F_PCTL1_S38_OFS                      ( 6)                            /*!< LCDS38 Bit Offset */
#define LCD_F_PCTL1_S38                          ((uint32_t)0x00000040)          /*!< LCD pin 38 enable */
#define LCD_F_PCTL1_S39_OFS                      ( 7)                            /*!< LCDS39 Bit Offset */
#define LCD_F_PCTL1_S39                          ((uint32_t)0x00000080)          /*!< LCD pin 39 enable */
#define LCD_F_PCTL1_S40_OFS                      ( 8)                            /*!< LCDS40 Bit Offset */
#define LCD_F_PCTL1_S40                          ((uint32_t)0x00000100)          /*!< LCD pin 40 enable */
#define LCD_F_PCTL1_S41_OFS                      ( 9)                            /*!< LCDS41 Bit Offset */
#define LCD_F_PCTL1_S41                          ((uint32_t)0x00000200)          /*!< LCD pin 41 enable */
#define LCD_F_PCTL1_S42_OFS                      (10)                            /*!< LCDS42 Bit Offset */
#define LCD_F_PCTL1_S42                          ((uint32_t)0x00000400)          /*!< LCD pin 42 enable */
#define LCD_F_PCTL1_S43_OFS                      (11)                            /*!< LCDS43 Bit Offset */
#define LCD_F_PCTL1_S43                          ((uint32_t)0x00000800)          /*!< LCD pin 43 enable */
#define LCD_F_PCTL1_S44_OFS                      (12)                            /*!< LCDS44 Bit Offset */
#define LCD_F_PCTL1_S44                          ((uint32_t)0x00001000)          /*!< LCD pin 44 enable */
#define LCD_F_PCTL1_S45_OFS                      (13)                            /*!< LCDS45 Bit Offset */
#define LCD_F_PCTL1_S45                          ((uint32_t)0x00002000)          /*!< LCD pin 45 enable */
#define LCD_F_PCTL1_S46_OFS                      (14)                            /*!< LCDS46 Bit Offset */
#define LCD_F_PCTL1_S46                          ((uint32_t)0x00004000)          /*!< LCD pin 46 enable */
#define LCD_F_PCTL1_S47_OFS                      (15)                            /*!< LCDS47 Bit Offset */
#define LCD_F_PCTL1_S47                          ((uint32_t)0x00008000)          /*!< LCD pin 47 enable */
#define LCD_F_PCTL1_S48_OFS                      (16)                            /*!< LCDS48 Bit Offset */
#define LCD_F_PCTL1_S48                          ((uint32_t)0x00010000)          /*!< LCD pin 48 enable */
#define LCD_F_PCTL1_S49_OFS                      (17)                            /*!< LCDS49 Bit Offset */
#define LCD_F_PCTL1_S49                          ((uint32_t)0x00020000)          /*!< LCD pin 49 enable */
#define LCD_F_PCTL1_S50_OFS                      (18)                            /*!< LCDS50 Bit Offset */
#define LCD_F_PCTL1_S50                          ((uint32_t)0x00040000)          /*!< LCD pin 50 enable */
#define LCD_F_PCTL1_S51_OFS                      (19)                            /*!< LCDS51 Bit Offset */
#define LCD_F_PCTL1_S51                          ((uint32_t)0x00080000)          /*!< LCD pin 51 enable */
#define LCD_F_PCTL1_S52_OFS                      (20)                            /*!< LCDS52 Bit Offset */
#define LCD_F_PCTL1_S52                          ((uint32_t)0x00100000)          /*!< LCD pin 52 enable */
#define LCD_F_PCTL1_S53_OFS                      (21)                            /*!< LCDS53 Bit Offset */
#define LCD_F_PCTL1_S53                          ((uint32_t)0x00200000)          /*!< LCD pin 53 enable */
#define LCD_F_PCTL1_S54_OFS                      (22)                            /*!< LCDS54 Bit Offset */
#define LCD_F_PCTL1_S54                          ((uint32_t)0x00400000)          /*!< LCD pin 54 enable */
#define LCD_F_PCTL1_S55_OFS                      (23)                            /*!< LCDS55 Bit Offset */
#define LCD_F_PCTL1_S55                          ((uint32_t)0x00800000)          /*!< LCD pin 55 enable */
#define LCD_F_PCTL1_S56_OFS                      (24)                            /*!< LCDS56 Bit Offset */
#define LCD_F_PCTL1_S56                          ((uint32_t)0x01000000)          /*!< LCD pin 56 enable */
#define LCD_F_PCTL1_S57_OFS                      (25)                            /*!< LCDS57 Bit Offset */
#define LCD_F_PCTL1_S57                          ((uint32_t)0x02000000)          /*!< LCD pin 57 enable */
#define LCD_F_PCTL1_S58_OFS                      (26)                            /*!< LCDS58 Bit Offset */
#define LCD_F_PCTL1_S58                          ((uint32_t)0x04000000)          /*!< LCD pin 58 enable */
#define LCD_F_PCTL1_S59_OFS                      (27)                            /*!< LCDS59 Bit Offset */
#define LCD_F_PCTL1_S59                          ((uint32_t)0x08000000)          /*!< LCD pin 59 enable */
#define LCD_F_PCTL1_S60_OFS                      (28)                            /*!< LCDS60 Bit Offset */
#define LCD_F_PCTL1_S60                          ((uint32_t)0x10000000)          /*!< LCD pin 60 enable */
#define LCD_F_PCTL1_S61_OFS                      (29)                            /*!< LCDS61 Bit Offset */
#define LCD_F_PCTL1_S61                          ((uint32_t)0x20000000)          /*!< LCD pin 61 enable */
#define LCD_F_PCTL1_S62_OFS                      (30)                            /*!< LCDS62 Bit Offset */
#define LCD_F_PCTL1_S62                          ((uint32_t)0x40000000)          /*!< LCD pin 62 enable */
#define LCD_F_PCTL1_S63_OFS                      (31)                            /*!< LCDS63 Bit Offset */
#define LCD_F_PCTL1_S63                          ((uint32_t)0x80000000)          /*!< LCD pin 63 enable */
#define LCD_F_CSSEL0_CSS0_OFS                    ( 0)                            /*!< LCDCSS0 Bit Offset */
#define LCD_F_CSSEL0_CSS0                        ((uint32_t)0x00000001)          /*!< L0 Com Seg select */
#define LCD_F_CSSEL0_CSS1_OFS                    ( 1)                            /*!< LCDCSS1 Bit Offset */
#define LCD_F_CSSEL0_CSS1                        ((uint32_t)0x00000002)          /*!< L1 Com Seg select */
#define LCD_F_CSSEL0_CSS2_OFS                    ( 2)                            /*!< LCDCSS2 Bit Offset */
#define LCD_F_CSSEL0_CSS2                        ((uint32_t)0x00000004)          /*!< L2 Com Seg select */
#define LCD_F_CSSEL0_CSS3_OFS                    ( 3)                            /*!< LCDCSS3 Bit Offset */
#define LCD_F_CSSEL0_CSS3                        ((uint32_t)0x00000008)          /*!< L3 Com Seg select */
#define LCD_F_CSSEL0_CSS4_OFS                    ( 4)                            /*!< LCDCSS4 Bit Offset */
#define LCD_F_CSSEL0_CSS4                        ((uint32_t)0x00000010)          /*!< L4 Com Seg select */
#define LCD_F_CSSEL0_CSS5_OFS                    ( 5)                            /*!< LCDCSS5 Bit Offset */
#define LCD_F_CSSEL0_CSS5                        ((uint32_t)0x00000020)          /*!< L5 Com Seg select */
#define LCD_F_CSSEL0_CSS6_OFS                    ( 6)                            /*!< LCDCSS6 Bit Offset */
#define LCD_F_CSSEL0_CSS6                        ((uint32_t)0x00000040)          /*!< L6 Com Seg select */
#define LCD_F_CSSEL0_CSS7_OFS                    ( 7)                            /*!< LCDCSS7 Bit Offset */
#define LCD_F_CSSEL0_CSS7                        ((uint32_t)0x00000080)          /*!< L7 Com Seg select */
#define LCD_F_CSSEL0_CSS8_OFS                    ( 8)                            /*!< LCDCSS8 Bit Offset */
#define LCD_F_CSSEL0_CSS8                        ((uint32_t)0x00000100)          /*!< L8 Com Seg select */
#define LCD_F_CSSEL0_CSS9_OFS                    ( 9)                            /*!< LCDCSS9 Bit Offset */
#define LCD_F_CSSEL0_CSS9                        ((uint32_t)0x00000200)          /*!< L9 Com Seg select */
#define LCD_F_CSSEL0_CSS10_OFS                   (10)                            /*!< LCDCSS10 Bit Offset */
#define LCD_F_CSSEL0_CSS10                       ((uint32_t)0x00000400)          /*!< L10 Com Seg select */
#define LCD_F_CSSEL0_CSS11_OFS                   (11)                            /*!< LCDCSS11 Bit Offset */
#define LCD_F_CSSEL0_CSS11                       ((uint32_t)0x00000800)          /*!< L11 Com Seg select */
#define LCD_F_CSSEL0_CSS12_OFS                   (12)                            /*!< LCDCSS12 Bit Offset */
#define LCD_F_CSSEL0_CSS12                       ((uint32_t)0x00001000)          /*!< L12 Com Seg select */
#define LCD_F_CSSEL0_CSS13_OFS                   (13)                            /*!< LCDCSS13 Bit Offset */
#define LCD_F_CSSEL0_CSS13                       ((uint32_t)0x00002000)          /*!< L13 Com Seg select */
#define LCD_F_CSSEL0_CSS14_OFS                   (14)                            /*!< LCDCSS14 Bit Offset */
#define LCD_F_CSSEL0_CSS14                       ((uint32_t)0x00004000)          /*!< L14 Com Seg select */
#define LCD_F_CSSEL0_CSS15_OFS                   (15)                            /*!< LCDCSS15 Bit Offset */
#define LCD_F_CSSEL0_CSS15                       ((uint32_t)0x00008000)          /*!< L15 Com Seg select */
#define LCD_F_CSSEL0_CSS16_OFS                   (16)                            /*!< LCDCSS16 Bit Offset */
#define LCD_F_CSSEL0_CSS16                       ((uint32_t)0x00010000)          /*!< L16 Com Seg select */
#define LCD_F_CSSEL0_CSS17_OFS                   (17)                            /*!< LCDCSS17 Bit Offset */
#define LCD_F_CSSEL0_CSS17                       ((uint32_t)0x00020000)          /*!< L17 Com Seg select */
#define LCD_F_CSSEL0_CSS18_OFS                   (18)                            /*!< LCDCSS18 Bit Offset */
#define LCD_F_CSSEL0_CSS18                       ((uint32_t)0x00040000)          /*!< L18 Com Seg select */
#define LCD_F_CSSEL0_CSS19_OFS                   (19)                            /*!< LCDCSS19 Bit Offset */
#define LCD_F_CSSEL0_CSS19                       ((uint32_t)0x00080000)          /*!< L19 Com Seg select */
#define LCD_F_CSSEL0_CSS20_OFS                   (20)                            /*!< LCDCSS20 Bit Offset */
#define LCD_F_CSSEL0_CSS20                       ((uint32_t)0x00100000)          /*!< L20 Com Seg select */
#define LCD_F_CSSEL0_CSS21_OFS                   (21)                            /*!< LCDCSS21 Bit Offset */
#define LCD_F_CSSEL0_CSS21                       ((uint32_t)0x00200000)          /*!< L21 Com Seg select */
#define LCD_F_CSSEL0_CSS22_OFS                   (22)                            /*!< LCDCSS22 Bit Offset */
#define LCD_F_CSSEL0_CSS22                       ((uint32_t)0x00400000)          /*!< L22 Com Seg select */
#define LCD_F_CSSEL0_CSS23_OFS                   (23)                            /*!< LCDCSS23 Bit Offset */
#define LCD_F_CSSEL0_CSS23                       ((uint32_t)0x00800000)          /*!< L23 Com Seg select */
#define LCD_F_CSSEL0_CSS24_OFS                   (24)                            /*!< LCDCSS24 Bit Offset */
#define LCD_F_CSSEL0_CSS24                       ((uint32_t)0x01000000)          /*!< L24 Com Seg select */
#define LCD_F_CSSEL0_CSS25_OFS                   (25)                            /*!< LCDCSS25 Bit Offset */
#define LCD_F_CSSEL0_CSS25                       ((uint32_t)0x02000000)          /*!< L25 Com Seg select */
#define LCD_F_CSSEL0_CSS26_OFS                   (26)                            /*!< LCDCSS26 Bit Offset */
#define LCD_F_CSSEL0_CSS26                       ((uint32_t)0x04000000)          /*!< L26 Com Seg select */
#define LCD_F_CSSEL0_CSS27_OFS                   (27)                            /*!< LCDCSS27 Bit Offset */
#define LCD_F_CSSEL0_CSS27                       ((uint32_t)0x08000000)          /*!< L27 Com Seg select */
#define LCD_F_CSSEL0_CSS28_OFS                   (28)                            /*!< LCDCSS28 Bit Offset */
#define LCD_F_CSSEL0_CSS28                       ((uint32_t)0x10000000)          /*!< L28 Com Seg select */
#define LCD_F_CSSEL0_CSS29_OFS                   (29)                            /*!< LCDCSS29 Bit Offset */
#define LCD_F_CSSEL0_CSS29                       ((uint32_t)0x20000000)          /*!< L29 Com Seg select */
#define LCD_F_CSSEL0_CSS30_OFS                   (30)                            /*!< LCDCSS30 Bit Offset */
#define LCD_F_CSSEL0_CSS30                       ((uint32_t)0x40000000)          /*!< L30 Com Seg select */
#define LCD_F_CSSEL0_CSS31_OFS                   (31)                            /*!< LCDCSS31 Bit Offset */
#define LCD_F_CSSEL0_CSS31                       ((uint32_t)0x80000000)          /*!< L31 Com Seg select */
#define LCD_F_CSSEL1_CSS32_OFS                   ( 0)                            /*!< LCDCSS32 Bit Offset */
#define LCD_F_CSSEL1_CSS32                       ((uint32_t)0x00000001)          /*!< L32 Com Seg select */
#define LCD_F_CSSEL1_CSS33_OFS                   ( 1)                            /*!< LCDCSS33 Bit Offset */
#define LCD_F_CSSEL1_CSS33                       ((uint32_t)0x00000002)          /*!< L33 Com Seg select */
#define LCD_F_CSSEL1_CSS34_OFS                   ( 2)                            /*!< LCDCSS34 Bit Offset */
#define LCD_F_CSSEL1_CSS34                       ((uint32_t)0x00000004)          /*!< L34 Com Seg select */
#define LCD_F_CSSEL1_CSS35_OFS                   ( 3)                            /*!< LCDCSS35 Bit Offset */
#define LCD_F_CSSEL1_CSS35                       ((uint32_t)0x00000008)          /*!< L35 Com Seg select */
#define LCD_F_CSSEL1_CSS36_OFS                   ( 4)                            /*!< LCDCSS36 Bit Offset */
#define LCD_F_CSSEL1_CSS36                       ((uint32_t)0x00000010)          /*!< L36 Com Seg select */
#define LCD_F_CSSEL1_CSS37_OFS                   ( 5)                            /*!< LCDCSS37 Bit Offset */
#define LCD_F_CSSEL1_CSS37                       ((uint32_t)0x00000020)          /*!< L37 Com Seg select */
#define LCD_F_CSSEL1_CSS38_OFS                   ( 6)                            /*!< LCDCSS38 Bit Offset */
#define LCD_F_CSSEL1_CSS38                       ((uint32_t)0x00000040)          /*!< L38 Com Seg select */
#define LCD_F_CSSEL1_CSS39_OFS                   ( 7)                            /*!< LCDCSS39 Bit Offset */
#define LCD_F_CSSEL1_CSS39                       ((uint32_t)0x00000080)          /*!< L39 Com Seg select */
#define LCD_F_CSSEL1_CSS40_OFS                   ( 8)                            /*!< LCDCSS40 Bit Offset */
#define LCD_F_CSSEL1_CSS40                       ((uint32_t)0x00000100)          /*!< L40 Com Seg select */
#define LCD_F_CSSEL1_CSS41_OFS                   ( 9)                            /*!< LCDCSS41 Bit Offset */
#define LCD_F_CSSEL1_CSS41                       ((uint32_t)0x00000200)          /*!< L41 Com Seg select */
#define LCD_F_CSSEL1_CSS42_OFS                   (10)                            /*!< LCDCSS42 Bit Offset */
#define LCD_F_CSSEL1_CSS42                       ((uint32_t)0x00000400)          /*!< L42 Com Seg select */
#define LCD_F_CSSEL1_CSS43_OFS                   (11)                            /*!< LCDCSS43 Bit Offset */
#define LCD_F_CSSEL1_CSS43                       ((uint32_t)0x00000800)          /*!< L43 Com Seg select */
#define LCD_F_CSSEL1_CSS44_OFS                   (12)                            /*!< LCDCSS44 Bit Offset */
#define LCD_F_CSSEL1_CSS44                       ((uint32_t)0x00001000)          /*!< L44 Com Seg select */
#define LCD_F_CSSEL1_CSS45_OFS                   (13)                            /*!< LCDCSS45 Bit Offset */
#define LCD_F_CSSEL1_CSS45                       ((uint32_t)0x00002000)          /*!< L45 Com Seg select */
#define LCD_F_CSSEL1_CSS46_OFS                   (14)                            /*!< LCDCSS46 Bit Offset */
#define LCD_F_CSSEL1_CSS46                       ((uint32_t)0x00004000)          /*!< L46 Com Seg select */
#define LCD_F_CSSEL1_CSS47_OFS                   (15)                            /*!< LCDCSS47 Bit Offset */
#define LCD_F_CSSEL1_CSS47                       ((uint32_t)0x00008000)          /*!< L47 Com Seg select */
#define LCD_F_CSSEL1_CSS48_OFS                   (16)                            /*!< LCDCSS48 Bit Offset */
#define LCD_F_CSSEL1_CSS48                       ((uint32_t)0x00010000)          /*!< L48 Com Seg select */
#define LCD_F_CSSEL1_CSS49_OFS                   (17)                            /*!< LCDCSS49 Bit Offset */
#define LCD_F_CSSEL1_CSS49                       ((uint32_t)0x00020000)          /*!< L49 Com Seg select */
#define LCD_F_CSSEL1_CSS50_OFS                   (18)                            /*!< LCDCSS50 Bit Offset */
#define LCD_F_CSSEL1_CSS50                       ((uint32_t)0x00040000)          /*!< L50 Com Seg select */
#define LCD_F_CSSEL1_CSS51_OFS                   (19)                            /*!< LCDCSS51 Bit Offset */
#define LCD_F_CSSEL1_CSS51                       ((uint32_t)0x00080000)          /*!< L51 Com Seg select */
#define LCD_F_CSSEL1_CSS52_OFS                   (20)                            /*!< LCDCSS52 Bit Offset */
#define LCD_F_CSSEL1_CSS52                       ((uint32_t)0x00100000)          /*!< L52 Com Seg select */
#define LCD_F_CSSEL1_CSS53_OFS                   (21)                            /*!< LCDCSS53 Bit Offset */
#define LCD_F_CSSEL1_CSS53                       ((uint32_t)0x00200000)          /*!< L53 Com Seg select */
#define LCD_F_CSSEL1_CSS54_OFS                   (22)                            /*!< LCDCSS54 Bit Offset */
#define LCD_F_CSSEL1_CSS54                       ((uint32_t)0x00400000)          /*!< L54 Com Seg select */
#define LCD_F_CSSEL1_CSS55_OFS                   (23)                            /*!< LCDCSS55 Bit Offset */
#define LCD_F_CSSEL1_CSS55                       ((uint32_t)0x00800000)          /*!< L55 Com Seg select */
#define LCD_F_CSSEL1_CSS56_OFS                   (24)                            /*!< LCDCSS56 Bit Offset */
#define LCD_F_CSSEL1_CSS56                       ((uint32_t)0x01000000)          /*!< L56 Com Seg select */
#define LCD_F_CSSEL1_CSS57_OFS                   (25)                            /*!< LCDCSS57 Bit Offset */
#define LCD_F_CSSEL1_CSS57                       ((uint32_t)0x02000000)          /*!< L57 Com Seg select */
#define LCD_F_CSSEL1_CSS58_OFS                   (26)                            /*!< LCDCSS58 Bit Offset */
#define LCD_F_CSSEL1_CSS58                       ((uint32_t)0x04000000)          /*!< L58 Com Seg select */
#define LCD_F_CSSEL1_CSS59_OFS                   (27)                            /*!< LCDCSS59 Bit Offset */
#define LCD_F_CSSEL1_CSS59                       ((uint32_t)0x08000000)          /*!< L59 Com Seg select */
#define LCD_F_CSSEL1_CSS60_OFS                   (28)                            /*!< LCDCSS60 Bit Offset */
#define LCD_F_CSSEL1_CSS60                       ((uint32_t)0x10000000)          /*!< L60 Com Seg select */
#define LCD_F_CSSEL1_CSS61_OFS                   (29)                            /*!< LCDCSS61 Bit Offset */
#define LCD_F_CSSEL1_CSS61                       ((uint32_t)0x20000000)          /*!< L61 Com Seg select */
#define LCD_F_CSSEL1_CSS62_OFS                   (30)                            /*!< LCDCSS62 Bit Offset */
#define LCD_F_CSSEL1_CSS62                       ((uint32_t)0x40000000)          /*!< L62 Com Seg select */
#define LCD_F_CSSEL1_CSS63_OFS                   (31)                            /*!< LCDCSS63 Bit Offset */
#define LCD_F_CSSEL1_CSS63                       ((uint32_t)0x80000000)          /*!< L63 Com Seg select */
#define LCD_F_ANMCTL_ANMEN_OFS                   ( 0)                            /*!< LCDANMEN Bit Offset */
#define LCD_F_ANMCTL_ANMEN                       ((uint32_t)0x00000001)          /*!< Enable Animation */
#define LCD_F_ANMCTL_ANMSTP_OFS                  ( 1)                            /*!< LCDANMSTP Bit Offset */
#define LCD_F_ANMCTL_ANMSTP_MASK                 ((uint32_t)0x0000000E)          /*!< LCDANMSTP Bit Mask */
#define LCD_F_ANMCTL_ANMSTP0                     ((uint32_t)0x00000002)          /*!< ANMSTP Bit 0 */
#define LCD_F_ANMCTL_ANMSTP1                     ((uint32_t)0x00000004)          /*!< ANMSTP Bit 1 */
#define LCD_F_ANMCTL_ANMSTP2                     ((uint32_t)0x00000008)          /*!< ANMSTP Bit 2 */
#define LCD_F_ANMCTL_ANMSTP_0                    ((uint32_t)0x00000000)          /*!< T0 */
#define LCD_F_ANMCTL_ANMSTP_1                    ((uint32_t)0x00000002)          /*!< T0 to T1 */
#define LCD_F_ANMCTL_ANMSTP_2                    ((uint32_t)0x00000004)          /*!< T0 to T2 */
#define LCD_F_ANMCTL_ANMSTP_3                    ((uint32_t)0x00000006)          /*!< T0 to T3 */
#define LCD_F_ANMCTL_ANMSTP_4                    ((uint32_t)0x00000008)          /*!< T0 to T4 */
#define LCD_F_ANMCTL_ANMSTP_5                    ((uint32_t)0x0000000A)          /*!< T0 to T5 */
#define LCD_F_ANMCTL_ANMSTP_6                    ((uint32_t)0x0000000C)          /*!< T0 to T6 */
#define LCD_F_ANMCTL_ANMSTP_7                    ((uint32_t)0x0000000E)          /*!< T0 to T7 */
#define LCD_F_ANMCTL_ANMCLR_OFS                  ( 7)                            /*!< LCDANMCLR Bit Offset */
#define LCD_F_ANMCTL_ANMCLR                      ((uint32_t)0x00000080)          /*!< Clear Animation Memory */
#define LCD_F_ANMCTL_ANMPRE_OFS                  (16)                            /*!< LCDANMPREx Bit Offset */
#define LCD_F_ANMCTL_ANMPRE_MASK                 ((uint32_t)0x00070000)          /*!< LCDANMPREx Bit Mask */
#define LCD_F_ANMCTL_ANMPRE0                     ((uint32_t)0x00010000)          /*!< ANMPRE Bit 0 */
#define LCD_F_ANMCTL_ANMPRE1                     ((uint32_t)0x00020000)          /*!< ANMPRE Bit 1 */
#define LCD_F_ANMCTL_ANMPRE2                     ((uint32_t)0x00040000)          /*!< ANMPRE Bit 2 */
#define LCD_F_ANMCTL_ANMPRE_0                    ((uint32_t)0x00000000)          /*!< Divide by 512 */
#define LCD_F_ANMCTL_ANMPRE_1                    ((uint32_t)0x00010000)          /*!< Divide by 1024 */
#define LCD_F_ANMCTL_ANMPRE_2                    ((uint32_t)0x00020000)          /*!< Divide by 2048 */
#define LCD_F_ANMCTL_ANMPRE_3                    ((uint32_t)0x00030000)          /*!< Divide by 4096 */
#define LCD_F_ANMCTL_ANMPRE_4                    ((uint32_t)0x00040000)          /*!< Divide by 8162 */
#define LCD_F_ANMCTL_ANMPRE_5                    ((uint32_t)0x00050000)          /*!< Divide by 16384 */
#define LCD_F_ANMCTL_ANMPRE_6                    ((uint32_t)0x00060000)          /*!< Divide by 32768 */
#define LCD_F_ANMCTL_ANMPRE_7                    ((uint32_t)0x00070000)          /*!< Divide by 65536 */
#define LCD_F_ANMCTL_ANMDIV_OFS                  (19)                            /*!< LCDANMDIVx Bit Offset */
#define LCD_F_ANMCTL_ANMDIV_MASK                 ((uint32_t)0x00380000)          /*!< LCDANMDIVx Bit Mask */
#define LCD_F_ANMCTL_ANMDIV0                     ((uint32_t)0x00080000)          /*!< ANMDIV Bit 0 */
#define LCD_F_ANMCTL_ANMDIV1                     ((uint32_t)0x00100000)          /*!< ANMDIV Bit 1 */
#define LCD_F_ANMCTL_ANMDIV2                     ((uint32_t)0x00200000)          /*!< ANMDIV Bit 2 */
#define LCD_F_ANMCTL_ANMDIV_0                    ((uint32_t)0x00000000)          /*!< Divide by 1 */
#define LCD_F_ANMCTL_ANMDIV_1                    ((uint32_t)0x00080000)          /*!< Divide by 2 */
#define LCD_F_ANMCTL_ANMDIV_2                    ((uint32_t)0x00100000)          /*!< Divide by 3 */
#define LCD_F_ANMCTL_ANMDIV_3                    ((uint32_t)0x00180000)          /*!< Divide by 4 */
#define LCD_F_ANMCTL_ANMDIV_4                    ((uint32_t)0x00200000)          /*!< Divide by 5 */
#define LCD_F_ANMCTL_ANMDIV_5                    ((uint32_t)0x00280000)          /*!< Divide by 6 */
#define LCD_F_ANMCTL_ANMDIV_6                    ((uint32_t)0x00300000)          /*!< Divide by 7 */
#define LCD_F_ANMCTL_ANMDIV_7                    ((uint32_t)0x00380000)          /*!< Divide by 8 */
#define LCD_F_IE_BLKOFFIE_OFS                    ( 1)                            /*!< LCDBLKOFFIE Bit Offset */
#define LCD_F_IE_BLKOFFIE                        ((uint32_t)0x00000002)          /*!< LCD Blink, segments off interrupt enable */
#define LCD_F_IE_BLKONIE_OFS                     ( 2)                            /*!< LCDBLKONIE Bit Offset */
#define LCD_F_IE_BLKONIE                         ((uint32_t)0x00000004)          /*!< LCD Blink, segments on interrupt enable */
#define LCD_F_IE_FRMIE_OFS                       ( 3)                            /*!< LCDFRMIE Bit Offset */
#define LCD_F_IE_FRMIE                           ((uint32_t)0x00000008)          /*!< LCD Frame interrupt enable */
#define LCD_F_IE_ANMSTPIE_OFS                    ( 8)                            /*!< LCDANMSTPIE Bit Offset */
#define LCD_F_IE_ANMSTPIE                        ((uint32_t)0x00000100)          /*!< LCD Animation step interrupt enable */
#define LCD_F_IE_ANMLOOPIE_OFS                   ( 9)                            /*!< LCDANMLOOPIE Bit Offset */
#define LCD_F_IE_ANMLOOPIE                       ((uint32_t)0x00000200)          /*!< LCD Animation loop interrupt enable */
#define LCD_F_IFG_BLKOFFIFG_OFS                  ( 1)                            /*!< LCDBLKOFFIFG Bit Offset */
#define LCD_F_IFG_BLKOFFIFG                      ((uint32_t)0x00000002)          /*!< LCD Blink, segments off interrupt flag */
#define LCD_F_IFG_BLKONIFG_OFS                   ( 2)                            /*!< LCDBLKONIFG Bit Offset */
#define LCD_F_IFG_BLKONIFG                       ((uint32_t)0x00000004)          /*!< LCD Blink, segments on interrupt flag */
#define LCD_F_IFG_FRMIFG_OFS                     ( 3)                            /*!< LCDFRMIFG Bit Offset */
#define LCD_F_IFG_FRMIFG                         ((uint32_t)0x00000008)          /*!< LCD Frame interrupt flag */
#define LCD_F_IFG_ANMSTPIFG_OFS                  ( 8)                            /*!< LCDANMSTPIFG Bit Offset */
#define LCD_F_IFG_ANMSTPIFG                      ((uint32_t)0x00000100)          /*!< LCD Animation step interrupt flag */
#define LCD_F_IFG_ANMLOOPIFG_OFS                 ( 9)                            /*!< LCDANMLOOPIFG Bit Offset */
#define LCD_F_IFG_ANMLOOPIFG                     ((uint32_t)0x00000200)          /*!< LCD Animation loop interrupt flag */
#define LCD_F_SETIFG_SETLCDBLKOFFIFG_OFS         ( 1)                            /*!< SETLCDBLKOFFIFG Bit Offset */
#define LCD_F_SETIFG_SETLCDBLKOFFIFG             ((uint32_t)0x00000002)          /*!< Sets LCDBLKOFFIFG */
#define LCD_F_SETIFG_SETLCDBLKONIFG_OFS          ( 2)                            /*!< SETLCDBLKONIFG Bit Offset */
#define LCD_F_SETIFG_SETLCDBLKONIFG              ((uint32_t)0x00000004)          /*!< Sets LCDBLKONIFG */
#define LCD_F_SETIFG_SETLCDFRMIFG_OFS            ( 3)                            /*!< SETLCDFRMIFG Bit Offset */
#define LCD_F_SETIFG_SETLCDFRMIFG                ((uint32_t)0x00000008)          /*!< Sets LCDFRMIFG */
#define LCD_F_SETIFG_SETLCDANMSTPIFG_OFS         ( 8)                            /*!< SETLCDANMSTPIFG Bit Offset */
#define LCD_F_SETIFG_SETLCDANMSTPIFG             ((uint32_t)0x00000100)          /*!< Sets LCDANMSTPIFG */
#define LCD_F_SETIFG_SETLCDANMLOOPIFG_OFS        ( 9)                            /*!< SETLCDANMLOOPIFG Bit Offset */
#define LCD_F_SETIFG_SETLCDANMLOOPIFG            ((uint32_t)0x00000200)          /*!< Sets LCDANMLOOPIFG */
#define LCD_F_CLRIFG_CLRLCDBLKOFFIFG_OFS         ( 1)                            /*!< CLRLCDBLKOFFIFG Bit Offset */
#define LCD_F_CLRIFG_CLRLCDBLKOFFIFG             ((uint32_t)0x00000002)          /*!< Clears LCDBLKOFFIFG */
#define LCD_F_CLRIFG_CLRLCDBLKONIFG_OFS          ( 2)                            /*!< CLRLCDBLKONIFG Bit Offset */
#define LCD_F_CLRIFG_CLRLCDBLKONIFG              ((uint32_t)0x00000004)          /*!< Clears LCDBLKONIFG */
#define LCD_F_CLRIFG_CLRLCDFRMIFG_OFS            ( 3)                            /*!< CLRLCDFRMIFG Bit Offset */
#define LCD_F_CLRIFG_CLRLCDFRMIFG                ((uint32_t)0x00000008)          /*!< Clears LCDFRMIFG */
#define LCD_F_CLRIFG_CLRLCDANMSTPIFG_OFS         ( 8)                            /*!< CLRLCDANMSTPIFG Bit Offset */
#define LCD_F_CLRIFG_CLRLCDANMSTPIFG             ((uint32_t)0x00000100)          /*!< Clears LCDANMSTPIFG */
#define LCD_F_CLRIFG_CLRLCDANMLOOPIFG_OFS        ( 9)                            /*!< CLRLCDANMLOOPIFG Bit Offset */
#define LCD_F_CLRIFG_CLRLCDANMLOOPIFG            ((uint32_t)0x00000200)          /*!< Clears LCDANMLOOPIFG */
#define SYSCTL_A_REBOOT_CTL_REBOOT_OFS           ( 0)                            /*!< REBOOT Bit Offset */
#define SYSCTL_A_REBOOT_CTL_REBOOT               ((uint32_t)0x00000001)          /*!< Write 1 initiates a Reboot of the device */
#define SYSCTL_A_REBOOT_CTL_WKEY_OFS             ( 8)                            /*!< WKEY Bit Offset */
#define SYSCTL_A_REBOOT_CTL_WKEY_MASK            ((uint32_t)0x0000FF00)          /*!< WKEY Bit Mask */
#define SYSCTL_A_NMI_CTLSTAT_CS_SRC_OFS          ( 0)                            /*!< CS_SRC Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_CS_SRC              ((uint32_t)0x00000001)          /*!< CS interrupt as a source of NMI */
#define SYSCTL_A_NMI_CTLSTAT_PSS_SRC_OFS         ( 1)                            /*!< PSS_SRC Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_PSS_SRC             ((uint32_t)0x00000002)          /*!< PSS interrupt as a source of NMI */
#define SYSCTL_A_NMI_CTLSTAT_PCM_SRC_OFS         ( 2)                            /*!< PCM_SRC Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_PCM_SRC             ((uint32_t)0x00000004)          /*!< PCM interrupt as a source of NMI */
#define SYSCTL_A_NMI_CTLSTAT_PIN_SRC_OFS         ( 3)                            /*!< PIN_SRC Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_PIN_SRC             ((uint32_t)0x00000008)          
#define SYSCTL_A_NMI_CTLSTAT_CS_FLG_OFS          (16)                            /*!< CS_FLG Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_CS_FLG              ((uint32_t)0x00010000)          /*!< CS interrupt was the source of NMI */
#define SYSCTL_A_NMI_CTLSTAT_PSS_FLG_OFS         (17)                            /*!< PSS_FLG Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_PSS_FLG             ((uint32_t)0x00020000)          /*!< PSS interrupt was the source of NMI */
#define SYSCTL_A_NMI_CTLSTAT_PCM_FLG_OFS         (18)                            /*!< PCM_FLG Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_PCM_FLG             ((uint32_t)0x00040000)          /*!< PCM interrupt was the source of NMI */
#define SYSCTL_A_NMI_CTLSTAT_PIN_FLG_OFS         (19)                            /*!< PIN_FLG Bit Offset */
#define SYSCTL_A_NMI_CTLSTAT_PIN_FLG             ((uint32_t)0x00080000)          /*!< RSTn/NMI pin was the source of NMI */
#define SYSCTL_A_WDTRESET_CTL_TIMEOUT_OFS        ( 0)                            /*!< TIMEOUT Bit Offset */
#define SYSCTL_A_WDTRESET_CTL_TIMEOUT            ((uint32_t)0x00000001)          /*!< WDT timeout reset type */
#define SYSCTL_A_WDTRESET_CTL_VIOLATION_OFS      ( 1)                            /*!< VIOLATION Bit Offset */
#define SYSCTL_A_WDTRESET_CTL_VIOLATION          ((uint32_t)0x00000002)          /*!< WDT password violation reset type */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_0_OFS     ( 0)                            /*!< HALT_T16_0 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_0         ((uint32_t)0x00000001)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_1_OFS     ( 1)                            /*!< HALT_T16_1 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_1         ((uint32_t)0x00000002)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_2_OFS     ( 2)                            /*!< HALT_T16_2 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_2         ((uint32_t)0x00000004)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_3_OFS     ( 3)                            /*!< HALT_T16_3 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_T16_3         ((uint32_t)0x00000008)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_T32_0_OFS     ( 4)                            /*!< HALT_T32_0 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_T32_0         ((uint32_t)0x00000010)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA0_OFS      ( 5)                            /*!< HALT_eUA0 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA0          ((uint32_t)0x00000020)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA1_OFS      ( 6)                            /*!< HALT_eUA1 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA1          ((uint32_t)0x00000040)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA2_OFS      ( 7)                            /*!< HALT_eUA2 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA2          ((uint32_t)0x00000080)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA3_OFS      ( 8)                            /*!< HALT_eUA3 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUA3          ((uint32_t)0x00000100)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB0_OFS      ( 9)                            /*!< HALT_eUB0 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB0          ((uint32_t)0x00000200)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB1_OFS      (10)                            /*!< HALT_eUB1 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB1          ((uint32_t)0x00000400)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB2_OFS      (11)                            /*!< HALT_eUB2 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB2          ((uint32_t)0x00000800)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB3_OFS      (12)                            /*!< HALT_eUB3 Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_EUB3          ((uint32_t)0x00001000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_ADC_OFS       (13)                            /*!< HALT_ADC Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_ADC           ((uint32_t)0x00002000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_WDT_OFS       (14)                            /*!< HALT_WDT Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_WDT           ((uint32_t)0x00004000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_DMA_OFS       (15)                            /*!< HALT_DMA Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_DMA           ((uint32_t)0x00008000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_PERIHALT_CTL_HALT_LCD_OFS       (16)                            /*!< HALT_LCD Bit Offset */
#define SYSCTL_A_PERIHALT_CTL_HALT_LCD           ((uint32_t)0x00010000)          /*!< Freezes IP operation when CPU is halted */
#define SYSCTL_A_DIO_GLTFLT_CTL_GLTCH_EN_OFS     ( 0)                            /*!< GLTCH_EN Bit Offset */
#define SYSCTL_A_DIO_GLTFLT_CTL_GLTCH_EN         ((uint32_t)0x00000001)          /*!< Glitch filter enable */
#define SYSCTL_A_SECDATA_UNLOCK_UNLKEY_OFS       ( 0)                            /*!< UNLKEY Bit Offset */
#define SYSCTL_A_SECDATA_UNLOCK_UNLKEY_MASK      ((uint32_t)0x0000FFFF)          /*!< UNLKEY Bit Mask */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK0_EN_OFS    ( 0)                            /*!< BNK0_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK0_EN        ((uint32_t)0x00000001)          /*!< When 1, enables Bank0 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK1_EN_OFS    ( 1)                            /*!< BNK1_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK1_EN        ((uint32_t)0x00000002)          /*!< When 1, enables Bank1 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK2_EN_OFS    ( 2)                            /*!< BNK2_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK2_EN        ((uint32_t)0x00000004)          /*!< When 1, enables Bank2 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK3_EN_OFS    ( 3)                            /*!< BNK3_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK3_EN        ((uint32_t)0x00000008)          /*!< When 1, enables Bank3 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK4_EN_OFS    ( 4)                            /*!< BNK4_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK4_EN        ((uint32_t)0x00000010)          /*!< When 1, enables Bank4 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK5_EN_OFS    ( 5)                            /*!< BNK5_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK5_EN        ((uint32_t)0x00000020)          /*!< When 1, enables Bank5 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK6_EN_OFS    ( 6)                            /*!< BNK6_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK6_EN        ((uint32_t)0x00000040)          /*!< When 1, enables Bank6 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK7_EN_OFS    ( 7)                            /*!< BNK7_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK7_EN        ((uint32_t)0x00000080)          /*!< When 1, enables Bank7 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK8_EN_OFS    ( 8)                            /*!< BNK8_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK8_EN        ((uint32_t)0x00000100)          /*!< When 1, enables Bank8 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK9_EN_OFS    ( 9)                            /*!< BNK9_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK9_EN        ((uint32_t)0x00000200)          /*!< When 1, enables Bank9 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK10_EN_OFS   (10)                            /*!< BNK10_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK10_EN       ((uint32_t)0x00000400)          /*!< When 1, enables Bank10 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK11_EN_OFS   (11)                            /*!< BNK11_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK11_EN       ((uint32_t)0x00000800)          /*!< When 1, enables Bank11 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK12_EN_OFS   (12)                            /*!< BNK12_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK12_EN       ((uint32_t)0x00001000)          /*!< When 1, enables Bank12 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK13_EN_OFS   (13)                            /*!< BNK13_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK13_EN       ((uint32_t)0x00002000)          /*!< When 1, enables Bank13 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK14_EN_OFS   (14)                            /*!< BNK14_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK14_EN       ((uint32_t)0x00004000)          /*!< When 1, enables Bank14 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK15_EN_OFS   (15)                            /*!< BNK15_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK15_EN       ((uint32_t)0x00008000)          /*!< When 1, enables Bank15 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK16_EN_OFS   (16)                            /*!< BNK16_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK16_EN       ((uint32_t)0x00010000)          /*!< When 1, enables Bank16 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK17_EN_OFS   (17)                            /*!< BNK17_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK17_EN       ((uint32_t)0x00020000)          /*!< When 1, enables Bank17 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK18_EN_OFS   (18)                            /*!< BNK18_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK18_EN       ((uint32_t)0x00040000)          /*!< When 1, enables Bank18 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK19_EN_OFS   (19)                            /*!< BNK19_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK19_EN       ((uint32_t)0x00080000)          /*!< When 1, enables Bank19 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK20_EN_OFS   (20)                            /*!< BNK20_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK20_EN       ((uint32_t)0x00100000)          /*!< When 1, enables Bank20 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK21_EN_OFS   (21)                            /*!< BNK21_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK21_EN       ((uint32_t)0x00200000)          /*!< When 1, enables Bank21 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK22_EN_OFS   (22)                            /*!< BNK22_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK22_EN       ((uint32_t)0x00400000)          /*!< When 1, enables Bank22 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK23_EN_OFS   (23)                            /*!< BNK23_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK23_EN       ((uint32_t)0x00800000)          /*!< When 1, enables Bank23 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK24_EN_OFS   (24)                            /*!< BNK24_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK24_EN       ((uint32_t)0x01000000)          /*!< When 1, enables Bank24 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK25_EN_OFS   (25)                            /*!< BNK25_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK25_EN       ((uint32_t)0x02000000)          /*!< When 1, enables Bank25 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK26_EN_OFS   (26)                            /*!< BNK26_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK26_EN       ((uint32_t)0x04000000)          /*!< When 1, enables Bank26 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK27_EN_OFS   (27)                            /*!< BNK27_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK27_EN       ((uint32_t)0x08000000)          /*!< When 1, enables Bank27 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK28_EN_OFS   (28)                            /*!< BNK28_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK28_EN       ((uint32_t)0x10000000)          /*!< When 1, enables Bank28 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK29_EN_OFS   (29)                            /*!< BNK29_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK29_EN       ((uint32_t)0x20000000)          /*!< When 1, enables Bank29 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK30_EN_OFS   (30)                            /*!< BNK30_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK30_EN       ((uint32_t)0x40000000)          /*!< When 1, enables Bank30 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK31_EN_OFS   (31)                            /*!< BNK31_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL0_BNK31_EN       ((uint32_t)0x80000000)          /*!< When 1, enables Bank31 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK32_EN_OFS   ( 0)                            /*!< BNK32_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK32_EN       ((uint32_t)0x00000001)          /*!< When 1, enables Bank32 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK33_EN_OFS   ( 1)                            /*!< BNK33_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK33_EN       ((uint32_t)0x00000002)          /*!< When 1, enables Bank33 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK34_EN_OFS   ( 2)                            /*!< BNK34_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK34_EN       ((uint32_t)0x00000004)          /*!< When 1, enables Bank34 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK35_EN_OFS   ( 3)                            /*!< BNK35_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK35_EN       ((uint32_t)0x00000008)          /*!< When 1, enables Bank35 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK36_EN_OFS   ( 4)                            /*!< BNK36_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK36_EN       ((uint32_t)0x00000010)          /*!< When 1, enables Bank36 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK37_EN_OFS   ( 5)                            /*!< BNK37_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK37_EN       ((uint32_t)0x00000020)          /*!< When 1, enables Bank37 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK38_EN_OFS   ( 6)                            /*!< BNK38_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK38_EN       ((uint32_t)0x00000040)          /*!< When 1, enables Bank38 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK39_EN_OFS   ( 7)                            /*!< BNK39_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK39_EN       ((uint32_t)0x00000080)          /*!< When 1, enables Bank39 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK40_EN_OFS   ( 8)                            /*!< BNK40_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK40_EN       ((uint32_t)0x00000100)          /*!< When 1, enables Bank40 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK41_EN_OFS   ( 9)                            /*!< BNK41_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK41_EN       ((uint32_t)0x00000200)          /*!< When 1, enables Bank41 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK42_EN_OFS   (10)                            /*!< BNK42_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK42_EN       ((uint32_t)0x00000400)          /*!< When 1, enables Bank42 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK43_EN_OFS   (11)                            /*!< BNK43_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK43_EN       ((uint32_t)0x00000800)          /*!< When 1, enables Bank43 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK44_EN_OFS   (12)                            /*!< BNK44_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK44_EN       ((uint32_t)0x00001000)          /*!< When 1, enables Bank44 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK45_EN_OFS   (13)                            /*!< BNK45_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK45_EN       ((uint32_t)0x00002000)          /*!< When 1, enables Bank45 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK46_EN_OFS   (14)                            /*!< BNK46_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK46_EN       ((uint32_t)0x00004000)          /*!< When 1, enables Bank46 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK47_EN_OFS   (15)                            /*!< BNK47_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK47_EN       ((uint32_t)0x00008000)          /*!< When 1, enables Bank47 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK48_EN_OFS   (16)                            /*!< BNK48_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK48_EN       ((uint32_t)0x00010000)          /*!< When 1, enables Bank48 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK49_EN_OFS   (17)                            /*!< BNK49_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK49_EN       ((uint32_t)0x00020000)          /*!< When 1, enables Bank49 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK50_EN_OFS   (18)                            /*!< BNK50_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK50_EN       ((uint32_t)0x00040000)          /*!< When 1, enables Bank50 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK51_EN_OFS   (19)                            /*!< BNK51_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK51_EN       ((uint32_t)0x00080000)          /*!< When 1, enables Bank51 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK52_EN_OFS   (20)                            /*!< BNK52_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK52_EN       ((uint32_t)0x00100000)          /*!< When 1, enables Bank52 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK53_EN_OFS   (21)                            /*!< BNK53_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK53_EN       ((uint32_t)0x00200000)          /*!< When 1, enables Bank53 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK54_EN_OFS   (22)                            /*!< BNK54_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK54_EN       ((uint32_t)0x00400000)          /*!< When 1, enables Bank54 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK55_EN_OFS   (23)                            /*!< BNK55_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK55_EN       ((uint32_t)0x00800000)          /*!< When 1, enables Bank55 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK56_EN_OFS   (24)                            /*!< BNK56_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK56_EN       ((uint32_t)0x01000000)          /*!< When 1, enables Bank56 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK57_EN_OFS   (25)                            /*!< BNK57_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK57_EN       ((uint32_t)0x02000000)          /*!< When 1, enables Bank57 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK58_EN_OFS   (26)                            /*!< BNK58_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK58_EN       ((uint32_t)0x04000000)          /*!< When 1, enables Bank58 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK59_EN_OFS   (27)                            /*!< BNK59_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK59_EN       ((uint32_t)0x08000000)          /*!< When 1, enables Bank59 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK60_EN_OFS   (28)                            /*!< BNK60_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK60_EN       ((uint32_t)0x10000000)          /*!< When 1, enables Bank60 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK61_EN_OFS   (29)                            /*!< BNK61_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK61_EN       ((uint32_t)0x20000000)          /*!< When 1, enables Bank61 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK62_EN_OFS   (30)                            /*!< BNK62_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK62_EN       ((uint32_t)0x40000000)          /*!< When 1, enables Bank62 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK63_EN_OFS   (31)                            /*!< BNK63_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL1_BNK63_EN       ((uint32_t)0x80000000)          /*!< When 1, enables Bank63 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK64_EN_OFS   ( 0)                            /*!< BNK64_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK64_EN       ((uint32_t)0x00000001)          /*!< When 1, enables Bank64 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK65_EN_OFS   ( 1)                            /*!< BNK65_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK65_EN       ((uint32_t)0x00000002)          /*!< When 1, enables Bank65 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK66_EN_OFS   ( 2)                            /*!< BNK66_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK66_EN       ((uint32_t)0x00000004)          /*!< When 1, enables Bank66 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK67_EN_OFS   ( 3)                            /*!< BNK67_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK67_EN       ((uint32_t)0x00000008)          /*!< When 1, enables Bank67 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK68_EN_OFS   ( 4)                            /*!< BNK68_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK68_EN       ((uint32_t)0x00000010)          /*!< When 1, enables Bank68 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK69_EN_OFS   ( 5)                            /*!< BNK69_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK69_EN       ((uint32_t)0x00000020)          /*!< When 1, enables Bank69 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK70_EN_OFS   ( 6)                            /*!< BNK70_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK70_EN       ((uint32_t)0x00000040)          /*!< When 1, enables Bank70 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK71_EN_OFS   ( 7)                            /*!< BNK71_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK71_EN       ((uint32_t)0x00000080)          /*!< When 1, enables Bank71 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK72_EN_OFS   ( 8)                            /*!< BNK72_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK72_EN       ((uint32_t)0x00000100)          /*!< When 1, enables Bank72 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK73_EN_OFS   ( 9)                            /*!< BNK73_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK73_EN       ((uint32_t)0x00000200)          /*!< When 1, enables Bank73 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK74_EN_OFS   (10)                            /*!< BNK74_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK74_EN       ((uint32_t)0x00000400)          /*!< When 1, enables Bank74 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK75_EN_OFS   (11)                            /*!< BNK75_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK75_EN       ((uint32_t)0x00000800)          /*!< When 1, enables Bank75 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK76_EN_OFS   (12)                            /*!< BNK76_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK76_EN       ((uint32_t)0x00001000)          /*!< When 1, enables Bank76 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK77_EN_OFS   (13)                            /*!< BNK77_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK77_EN       ((uint32_t)0x00002000)          /*!< When 1, enables Bank77 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK78_EN_OFS   (14)                            /*!< BNK78_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK78_EN       ((uint32_t)0x00004000)          /*!< When 1, enables Bank78 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK79_EN_OFS   (15)                            /*!< BNK79_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK79_EN       ((uint32_t)0x00008000)          /*!< When 1, enables Bank79 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK80_EN_OFS   (16)                            /*!< BNK80_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK80_EN       ((uint32_t)0x00010000)          /*!< When 1, enables Bank80 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK81_EN_OFS   (17)                            /*!< BNK81_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK81_EN       ((uint32_t)0x00020000)          /*!< When 1, enables Bank81 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK82_EN_OFS   (18)                            /*!< BNK82_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK82_EN       ((uint32_t)0x00040000)          /*!< When 1, enables Bank82 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK83_EN_OFS   (19)                            /*!< BNK83_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK83_EN       ((uint32_t)0x00080000)          /*!< When 1, enables Bank83 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK84_EN_OFS   (20)                            /*!< BNK84_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK84_EN       ((uint32_t)0x00100000)          /*!< When 1, enables Bank84 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK85_EN_OFS   (21)                            /*!< BNK85_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK85_EN       ((uint32_t)0x00200000)          /*!< When 1, enables Bank85 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK86_EN_OFS   (22)                            /*!< BNK86_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK86_EN       ((uint32_t)0x00400000)          /*!< When 1, enables Bank86 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK87_EN_OFS   (23)                            /*!< BNK87_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK87_EN       ((uint32_t)0x00800000)          /*!< When 1, enables Bank87 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK88_EN_OFS   (24)                            /*!< BNK88_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK88_EN       ((uint32_t)0x01000000)          /*!< When 1, enables Bank88 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK89_EN_OFS   (25)                            /*!< BNK89_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK89_EN       ((uint32_t)0x02000000)          /*!< When 1, enables Bank89 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK90_EN_OFS   (26)                            /*!< BNK90_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK90_EN       ((uint32_t)0x04000000)          /*!< When 1, enables Bank90 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK91_EN_OFS   (27)                            /*!< BNK91_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK91_EN       ((uint32_t)0x08000000)          /*!< When 1, enables Bank91 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK92_EN_OFS   (28)                            /*!< BNK92_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK92_EN       ((uint32_t)0x10000000)          /*!< When 1, enables Bank92 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK93_EN_OFS   (29)                            /*!< BNK93_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK93_EN       ((uint32_t)0x20000000)          /*!< When 1, enables Bank93 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK94_EN_OFS   (30)                            /*!< BNK94_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK94_EN       ((uint32_t)0x40000000)          /*!< When 1, enables Bank94 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK95_EN_OFS   (31)                            /*!< BNK95_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL2_BNK95_EN       ((uint32_t)0x80000000)          /*!< When 1, enables Bank95 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK96_EN_OFS   ( 0)                            /*!< BNK96_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK96_EN       ((uint32_t)0x00000001)          /*!< When 1, enables Bank96 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK97_EN_OFS   ( 1)                            /*!< BNK97_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK97_EN       ((uint32_t)0x00000002)          /*!< When 1, enables Bank97 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK98_EN_OFS   ( 2)                            /*!< BNK98_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK98_EN       ((uint32_t)0x00000004)          /*!< When 1, enables Bank98 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK99_EN_OFS   ( 3)                            /*!< BNK99_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK99_EN       ((uint32_t)0x00000008)          /*!< When 1, enables Bank99 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK100_EN_OFS  ( 4)                            /*!< BNK100_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK100_EN      ((uint32_t)0x00000010)          /*!< When 1, enables Bank100 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK101_EN_OFS  ( 5)                            /*!< BNK101_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK101_EN      ((uint32_t)0x00000020)          /*!< When 1, enables Bank101 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK102_EN_OFS  ( 6)                            /*!< BNK102_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK102_EN      ((uint32_t)0x00000040)          /*!< When 1, enables Bank102 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK103_EN_OFS  ( 7)                            /*!< BNK103_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK103_EN      ((uint32_t)0x00000080)          /*!< When 1, enables Bank103 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK104_EN_OFS  ( 8)                            /*!< BNK104_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK104_EN      ((uint32_t)0x00000100)          /*!< When 1, enables Bank104 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK105_EN_OFS  ( 9)                            /*!< BNK105_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK105_EN      ((uint32_t)0x00000200)          /*!< When 1, enables Bank105 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK106_EN_OFS  (10)                            /*!< BNK106_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK106_EN      ((uint32_t)0x00000400)          /*!< When 1, enables Bank106 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK107_EN_OFS  (11)                            /*!< BNK107_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK107_EN      ((uint32_t)0x00000800)          /*!< When 1, enables Bank107 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK108_EN_OFS  (12)                            /*!< BNK108_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK108_EN      ((uint32_t)0x00001000)          /*!< When 1, enables Bank108 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK109_EN_OFS  (13)                            /*!< BNK109_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK109_EN      ((uint32_t)0x00002000)          /*!< When 1, enables Bank109 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK110_EN_OFS  (14)                            /*!< BNK110_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK110_EN      ((uint32_t)0x00004000)          /*!< When 1, enables Bank110 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK111_EN_OFS  (15)                            /*!< BNK111_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK111_EN      ((uint32_t)0x00008000)          /*!< When 1, enables Bank111 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK112_EN_OFS  (16)                            /*!< BNK112_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK112_EN      ((uint32_t)0x00010000)          /*!< When 1, enables Bank112 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK113_EN_OFS  (17)                            /*!< BNK113_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK113_EN      ((uint32_t)0x00020000)          /*!< When 1, enables Bank113 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK114_EN_OFS  (18)                            /*!< BNK114_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK114_EN      ((uint32_t)0x00040000)          /*!< When 1, enables Bank114 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK115_EN_OFS  (19)                            /*!< BNK115_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK115_EN      ((uint32_t)0x00080000)          /*!< When 1, enables Bank115 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK116_EN_OFS  (20)                            /*!< BNK116_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK116_EN      ((uint32_t)0x00100000)          /*!< When 1, enables Bank116 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK117_EN_OFS  (21)                            /*!< BNK117_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK117_EN      ((uint32_t)0x00200000)          /*!< When 1, enables Bank117 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK118_EN_OFS  (22)                            /*!< BNK118_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK118_EN      ((uint32_t)0x00400000)          /*!< When 1, enables Bank118 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK119_EN_OFS  (23)                            /*!< BNK119_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK119_EN      ((uint32_t)0x00800000)          /*!< When 1, enables Bank119 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK120_EN_OFS  (24)                            /*!< BNK120_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK120_EN      ((uint32_t)0x01000000)          /*!< When 1, enables Bank120 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK121_EN_OFS  (25)                            /*!< BNK121_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK121_EN      ((uint32_t)0x02000000)          /*!< When 1, enables Bank121 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK122_EN_OFS  (26)                            /*!< BNK122_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK122_EN      ((uint32_t)0x04000000)          /*!< When 1, enables Bank122 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK123_EN_OFS  (27)                            /*!< BNK123_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK123_EN      ((uint32_t)0x08000000)          /*!< When 1, enables Bank123 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK124_EN_OFS  (28)                            /*!< BNK124_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK124_EN      ((uint32_t)0x10000000)          /*!< When 1, enables Bank124 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK125_EN_OFS  (29)                            /*!< BNK125_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK125_EN      ((uint32_t)0x20000000)          /*!< When 1, enables Bank125 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK126_EN_OFS  (30)                            /*!< BNK126_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK126_EN      ((uint32_t)0x40000000)          /*!< When 1, enables Bank126 of the SRAM */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK127_EN_OFS  (31)                            /*!< BNK127_EN Bit Offset */
#define SYSCTL_A_SRAM_BANKEN_CTL3_BNK127_EN      ((uint32_t)0x80000000)          /*!< When 1, enables Bank127 of the SRAM */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK0_EN_OFS    ( 0)                            /*!< BLK0_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK0_EN        ((uint32_t)0x00000001)          /*!< Block0 is always retained in LPM3, LPM4 and LPM3.5 modes of operation */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK1_EN_OFS    ( 1)                            /*!< BLK1_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK1_EN        ((uint32_t)0x00000002)          /*!< When 1, Block1 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK2_EN_OFS    ( 2)                            /*!< BLK2_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK2_EN        ((uint32_t)0x00000004)          /*!< When 1, Block2 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK3_EN_OFS    ( 3)                            /*!< BLK3_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK3_EN        ((uint32_t)0x00000008)          /*!< When 1, Block3 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK4_EN_OFS    ( 4)                            /*!< BLK4_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK4_EN        ((uint32_t)0x00000010)          /*!< When 1, Block4 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK5_EN_OFS    ( 5)                            /*!< BLK5_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK5_EN        ((uint32_t)0x00000020)          /*!< When 1, Block5 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK6_EN_OFS    ( 6)                            /*!< BLK6_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK6_EN        ((uint32_t)0x00000040)          /*!< When 1, Block6 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK7_EN_OFS    ( 7)                            /*!< BLK7_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK7_EN        ((uint32_t)0x00000080)          /*!< When 1, Block7 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK8_EN_OFS    ( 8)                            /*!< BLK8_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK8_EN        ((uint32_t)0x00000100)          /*!< When 1, Block8 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK9_EN_OFS    ( 9)                            /*!< BLK9_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK9_EN        ((uint32_t)0x00000200)          /*!< When 1, Block9 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK10_EN_OFS   (10)                            /*!< BLK10_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK10_EN       ((uint32_t)0x00000400)          /*!< When 1, Block10 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK11_EN_OFS   (11)                            /*!< BLK11_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK11_EN       ((uint32_t)0x00000800)          /*!< When 1, Block11 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK12_EN_OFS   (12)                            /*!< BLK12_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK12_EN       ((uint32_t)0x00001000)          /*!< When 1, Block12 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK13_EN_OFS   (13)                            /*!< BLK13_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK13_EN       ((uint32_t)0x00002000)          /*!< When 1, Block13 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK14_EN_OFS   (14)                            /*!< BLK14_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK14_EN       ((uint32_t)0x00004000)          /*!< When 1, Block14 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK15_EN_OFS   (15)                            /*!< BLK15_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK15_EN       ((uint32_t)0x00008000)          /*!< When 1, Block15 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK16_EN_OFS   (16)                            /*!< BLK16_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK16_EN       ((uint32_t)0x00010000)          /*!< When 1, Block16 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK17_EN_OFS   (17)                            /*!< BLK17_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK17_EN       ((uint32_t)0x00020000)          /*!< When 1, Block17 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK18_EN_OFS   (18)                            /*!< BLK18_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK18_EN       ((uint32_t)0x00040000)          /*!< When 1, Block18 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK19_EN_OFS   (19)                            /*!< BLK19_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK19_EN       ((uint32_t)0x00080000)          /*!< When 1, Block19 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK20_EN_OFS   (20)                            /*!< BLK20_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK20_EN       ((uint32_t)0x00100000)          /*!< When 1, Block20 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK21_EN_OFS   (21)                            /*!< BLK21_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK21_EN       ((uint32_t)0x00200000)          /*!< When 1, Block21 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK22_EN_OFS   (22)                            /*!< BLK22_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK22_EN       ((uint32_t)0x00400000)          /*!< When 1, Block22 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK23_EN_OFS   (23)                            /*!< BLK23_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK23_EN       ((uint32_t)0x00800000)          /*!< When 1, Block23 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK24_EN_OFS   (24)                            /*!< BLK24_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK24_EN       ((uint32_t)0x01000000)          /*!< When 1, Block24 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK25_EN_OFS   (25)                            /*!< BLK25_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK25_EN       ((uint32_t)0x02000000)          /*!< When 1, Block25 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK26_EN_OFS   (26)                            /*!< BLK26_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK26_EN       ((uint32_t)0x04000000)          /*!< When 1, Block26 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK27_EN_OFS   (27)                            /*!< BLK27_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK27_EN       ((uint32_t)0x08000000)          /*!< When 1, Block27 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK28_EN_OFS   (28)                            /*!< BLK28_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK28_EN       ((uint32_t)0x10000000)          /*!< When 1, Block28 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK29_EN_OFS   (29)                            /*!< BLK29_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK29_EN       ((uint32_t)0x20000000)          /*!< When 1, Block29 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK30_EN_OFS   (30)                            /*!< BLK30_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK30_EN       ((uint32_t)0x40000000)          /*!< When 1, Block30 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK31_EN_OFS   (31)                            /*!< BLK31_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL0_BLK31_EN       ((uint32_t)0x80000000)          /*!< When 1, Block31 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK32_EN_OFS   ( 0)                            /*!< BLK32_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK32_EN       ((uint32_t)0x00000001)          /*!< When 1, Block32 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK33_EN_OFS   ( 1)                            /*!< BLK33_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK33_EN       ((uint32_t)0x00000002)          /*!< When 1, Block33 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK34_EN_OFS   ( 2)                            /*!< BLK34_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK34_EN       ((uint32_t)0x00000004)          /*!< When 1, Block34 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK35_EN_OFS   ( 3)                            /*!< BLK35_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK35_EN       ((uint32_t)0x00000008)          /*!< When 1, Block35 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK36_EN_OFS   ( 4)                            /*!< BLK36_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK36_EN       ((uint32_t)0x00000010)          /*!< When 1, Block36 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK37_EN_OFS   ( 5)                            /*!< BLK37_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK37_EN       ((uint32_t)0x00000020)          /*!< When 1, Block37 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK38_EN_OFS   ( 6)                            /*!< BLK38_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK38_EN       ((uint32_t)0x00000040)          /*!< When 1, Block38 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK39_EN_OFS   ( 7)                            /*!< BLK39_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK39_EN       ((uint32_t)0x00000080)          /*!< When 1, Block39 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK40_EN_OFS   ( 8)                            /*!< BLK40_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK40_EN       ((uint32_t)0x00000100)          /*!< When 1, Block40 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK41_EN_OFS   ( 9)                            /*!< BLK41_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK41_EN       ((uint32_t)0x00000200)          /*!< When 1, Block41 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK42_EN_OFS   (10)                            /*!< BLK42_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK42_EN       ((uint32_t)0x00000400)          /*!< When 1, Block42 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK43_EN_OFS   (11)                            /*!< BLK43_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK43_EN       ((uint32_t)0x00000800)          /*!< When 1, Block43 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK44_EN_OFS   (12)                            /*!< BLK44_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK44_EN       ((uint32_t)0x00001000)          /*!< When 1, Block44 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK45_EN_OFS   (13)                            /*!< BLK45_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK45_EN       ((uint32_t)0x00002000)          /*!< When 1, Block45 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK46_EN_OFS   (14)                            /*!< BLK46_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK46_EN       ((uint32_t)0x00004000)          /*!< When 1, Block46 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK47_EN_OFS   (15)                            /*!< BLK47_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK47_EN       ((uint32_t)0x00008000)          /*!< When 1, Block47 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK48_EN_OFS   (16)                            /*!< BLK48_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK48_EN       ((uint32_t)0x00010000)          /*!< When 1, Block48 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK49_EN_OFS   (17)                            /*!< BLK49_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK49_EN       ((uint32_t)0x00020000)          /*!< When 1, Block49 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK50_EN_OFS   (18)                            /*!< BLK50_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK50_EN       ((uint32_t)0x00040000)          /*!< When 1, Block50 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK51_EN_OFS   (19)                            /*!< BLK51_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK51_EN       ((uint32_t)0x00080000)          /*!< When 1, Block51 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK52_EN_OFS   (20)                            /*!< BLK52_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK52_EN       ((uint32_t)0x00100000)          /*!< When 1, Block52 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK53_EN_OFS   (21)                            /*!< BLK53_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK53_EN       ((uint32_t)0x00200000)          /*!< When 1, Block53 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK54_EN_OFS   (22)                            /*!< BLK54_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK54_EN       ((uint32_t)0x00400000)          /*!< When 1, Block54 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK55_EN_OFS   (23)                            /*!< BLK55_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK55_EN       ((uint32_t)0x00800000)          /*!< When 1, Block55 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK56_EN_OFS   (24)                            /*!< BLK56_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK56_EN       ((uint32_t)0x01000000)          /*!< When 1, Block56 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK57_EN_OFS   (25)                            /*!< BLK57_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK57_EN       ((uint32_t)0x02000000)          /*!< When 1, Block57 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK58_EN_OFS   (26)                            /*!< BLK58_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK58_EN       ((uint32_t)0x04000000)          /*!< When 1, Block58 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK59_EN_OFS   (27)                            /*!< BLK59_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK59_EN       ((uint32_t)0x08000000)          /*!< When 1, Block59 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK60_EN_OFS   (28)                            /*!< BLK60_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK60_EN       ((uint32_t)0x10000000)          /*!< When 1, Block60 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK61_EN_OFS   (29)                            /*!< BLK61_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK61_EN       ((uint32_t)0x20000000)          /*!< When 1, Block61 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK62_EN_OFS   (30)                            /*!< BLK62_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK62_EN       ((uint32_t)0x40000000)          /*!< When 1, Block62 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK63_EN_OFS   (31)                            /*!< BLK63_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL1_BLK63_EN       ((uint32_t)0x80000000)          /*!< When 1, Block63 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK64_EN_OFS   ( 0)                            /*!< BLK64_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK64_EN       ((uint32_t)0x00000001)          /*!< When 1, Block64 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK65_EN_OFS   ( 1)                            /*!< BLK65_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK65_EN       ((uint32_t)0x00000002)          /*!< When 1, Block65 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK66_EN_OFS   ( 2)                            /*!< BLK66_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK66_EN       ((uint32_t)0x00000004)          /*!< When 1, Block66 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK67_EN_OFS   ( 3)                            /*!< BLK67_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK67_EN       ((uint32_t)0x00000008)          /*!< When 1, Block67 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK68_EN_OFS   ( 4)                            /*!< BLK68_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK68_EN       ((uint32_t)0x00000010)          /*!< When 1, Block68 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK69_EN_OFS   ( 5)                            /*!< BLK69_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK69_EN       ((uint32_t)0x00000020)          /*!< When 1, Block69 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK70_EN_OFS   ( 6)                            /*!< BLK70_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK70_EN       ((uint32_t)0x00000040)          /*!< When 1, Block70 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK71_EN_OFS   ( 7)                            /*!< BLK71_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK71_EN       ((uint32_t)0x00000080)          /*!< When 1, Block71 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK72_EN_OFS   ( 8)                            /*!< BLK72_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK72_EN       ((uint32_t)0x00000100)          /*!< When 1, Block72 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK73_EN_OFS   ( 9)                            /*!< BLK73_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK73_EN       ((uint32_t)0x00000200)          /*!< When 1, Block73 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK74_EN_OFS   (10)                            /*!< BLK74_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK74_EN       ((uint32_t)0x00000400)          /*!< When 1, Block74 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK75_EN_OFS   (11)                            /*!< BLK75_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK75_EN       ((uint32_t)0x00000800)          /*!< When 1, Block75 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK76_EN_OFS   (12)                            /*!< BLK76_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK76_EN       ((uint32_t)0x00001000)          /*!< When 1, Block76 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK77_EN_OFS   (13)                            /*!< BLK77_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK77_EN       ((uint32_t)0x00002000)          /*!< When 1, Block77 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK78_EN_OFS   (14)                            /*!< BLK78_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK78_EN       ((uint32_t)0x00004000)          /*!< When 1, Block78 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK79_EN_OFS   (15)                            /*!< BLK79_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK79_EN       ((uint32_t)0x00008000)          /*!< When 1, Block79 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK80_EN_OFS   (16)                            /*!< BLK80_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK80_EN       ((uint32_t)0x00010000)          /*!< When 1, Block80 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK81_EN_OFS   (17)                            /*!< BLK81_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK81_EN       ((uint32_t)0x00020000)          /*!< When 1, Block81 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK82_EN_OFS   (18)                            /*!< BLK82_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK82_EN       ((uint32_t)0x00040000)          /*!< When 1, Block82 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK83_EN_OFS   (19)                            /*!< BLK83_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK83_EN       ((uint32_t)0x00080000)          /*!< When 1, Block83 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK84_EN_OFS   (20)                            /*!< BLK84_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK84_EN       ((uint32_t)0x00100000)          /*!< When 1, Block84 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK85_EN_OFS   (21)                            /*!< BLK85_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK85_EN       ((uint32_t)0x00200000)          /*!< When 1, Block85 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK86_EN_OFS   (22)                            /*!< BLK86_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK86_EN       ((uint32_t)0x00400000)          /*!< When 1, Block86 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK87_EN_OFS   (23)                            /*!< BLK87_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK87_EN       ((uint32_t)0x00800000)          /*!< When 1, Block87 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK88_EN_OFS   (24)                            /*!< BLK88_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK88_EN       ((uint32_t)0x01000000)          /*!< When 1, Block88 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK89_EN_OFS   (25)                            /*!< BLK89_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK89_EN       ((uint32_t)0x02000000)          /*!< When 1, Block89 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK90_EN_OFS   (26)                            /*!< BLK90_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK90_EN       ((uint32_t)0x04000000)          /*!< When 1, Block90 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK91_EN_OFS   (27)                            /*!< BLK91_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK91_EN       ((uint32_t)0x08000000)          /*!< When 1, Block91 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK92_EN_OFS   (28)                            /*!< BLK92_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK92_EN       ((uint32_t)0x10000000)          /*!< When 1, Block92 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK93_EN_OFS   (29)                            /*!< BLK93_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK93_EN       ((uint32_t)0x20000000)          /*!< When 1, Block93 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK94_EN_OFS   (30)                            /*!< BLK94_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK94_EN       ((uint32_t)0x40000000)          /*!< When 1, Block94 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK95_EN_OFS   (31)                            /*!< BLK95_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL2_BLK95_EN       ((uint32_t)0x80000000)          /*!< When 1, Block95 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK96_EN_OFS   ( 0)                            /*!< BLK96_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK96_EN       ((uint32_t)0x00000001)          /*!< When 1, Block96 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK97_EN_OFS   ( 1)                            /*!< BLK97_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK97_EN       ((uint32_t)0x00000002)          /*!< When 1, Block97 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK98_EN_OFS   ( 2)                            /*!< BLK98_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK98_EN       ((uint32_t)0x00000004)          /*!< When 1, Block98 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK99_EN_OFS   ( 3)                            /*!< BLK99_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK99_EN       ((uint32_t)0x00000008)          /*!< When 1, Block99 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK100_EN_OFS  ( 4)                            /*!< BLK100_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK100_EN      ((uint32_t)0x00000010)          /*!< When 1, Block100 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK101_EN_OFS  ( 5)                            /*!< BLK101_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK101_EN      ((uint32_t)0x00000020)          /*!< When 1, Block101 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK102_EN_OFS  ( 6)                            /*!< BLK102_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK102_EN      ((uint32_t)0x00000040)          /*!< When 1, Block102 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK103_EN_OFS  ( 7)                            /*!< BLK103_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK103_EN      ((uint32_t)0x00000080)          /*!< When 1, Block103 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK104_EN_OFS  ( 8)                            /*!< BLK104_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK104_EN      ((uint32_t)0x00000100)          /*!< When 1, Block104 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK105_EN_OFS  ( 9)                            /*!< BLK105_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK105_EN      ((uint32_t)0x00000200)          /*!< When 1, Block105 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK106_EN_OFS  (10)                            /*!< BLK106_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK106_EN      ((uint32_t)0x00000400)          /*!< When 1, Block106 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK107_EN_OFS  (11)                            /*!< BLK107_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK107_EN      ((uint32_t)0x00000800)          /*!< When 1, Block107 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK108_EN_OFS  (12)                            /*!< BLK108_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK108_EN      ((uint32_t)0x00001000)          /*!< When 1, Block108 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK109_EN_OFS  (13)                            /*!< BLK109_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK109_EN      ((uint32_t)0x00002000)          /*!< When 1, Block109 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK110_EN_OFS  (14)                            /*!< BLK110_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK110_EN      ((uint32_t)0x00004000)          /*!< When 1, Block110 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK111_EN_OFS  (15)                            /*!< BLK111_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK111_EN      ((uint32_t)0x00008000)          /*!< When 1, Block111 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK112_EN_OFS  (16)                            /*!< BLK112_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK112_EN      ((uint32_t)0x00010000)          /*!< When 1, Block112 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK113_EN_OFS  (17)                            /*!< BLK113_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK113_EN      ((uint32_t)0x00020000)          /*!< When 1, Block113 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK114_EN_OFS  (18)                            /*!< BLK114_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK114_EN      ((uint32_t)0x00040000)          /*!< When 1, Block114 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK115_EN_OFS  (19)                            /*!< BLK115_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK115_EN      ((uint32_t)0x00080000)          /*!< When 1, Block115 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK116_EN_OFS  (20)                            /*!< BLK116_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK116_EN      ((uint32_t)0x00100000)          /*!< When 1, Block116 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK117_EN_OFS  (21)                            /*!< BLK117_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK117_EN      ((uint32_t)0x00200000)          /*!< When 1, Block117 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK118_EN_OFS  (22)                            /*!< BLK118_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK118_EN      ((uint32_t)0x00400000)          /*!< When 1, Block118 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK119_EN_OFS  (23)                            /*!< BLK119_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK119_EN      ((uint32_t)0x00800000)          /*!< When 1, Block119 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK120_EN_OFS  (24)                            /*!< BLK120_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK120_EN      ((uint32_t)0x01000000)          /*!< When 1, Block120 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK121_EN_OFS  (25)                            /*!< BLK121_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK121_EN      ((uint32_t)0x02000000)          /*!< When 1, Block121 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK122_EN_OFS  (26)                            /*!< BLK122_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK122_EN      ((uint32_t)0x04000000)          /*!< When 1, Block122 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK123_EN_OFS  (27)                            /*!< BLK123_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK123_EN      ((uint32_t)0x08000000)          /*!< When 1, Block123 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK124_EN_OFS  (28)                            /*!< BLK124_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK124_EN      ((uint32_t)0x10000000)          /*!< When 1, Block124 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK125_EN_OFS  (29)                            /*!< BLK125_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK125_EN      ((uint32_t)0x20000000)          /*!< When 1, Block125 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK126_EN_OFS  (30)                            /*!< BLK126_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK126_EN      ((uint32_t)0x40000000)          /*!< When 1, Block126 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK127_EN_OFS  (31)                            /*!< BLK127_EN Bit Offset */
#define SYSCTL_A_SRAM_BLKRET_CTL3_BLK127_EN      ((uint32_t)0x80000000)          /*!< When 1, Block127 of the SRAM is retained in LPM3 and LPM4 */
#define SYSCTL_A_SRAM_STAT_BNKEN_RDY_OFS         ( 0)                            /*!< BNKEN_RDY Bit Offset */
#define SYSCTL_A_SRAM_STAT_BNKEN_RDY             ((uint32_t)0x00000001)          /*!< When 1, indicates SRAM is ready for access and banks can be  */
#define SYSCTL_A_SRAM_STAT_BLKRET_RDY_OFS        ( 1)                            /*!< BLKRET_RDY Bit Offset */
#define SYSCTL_A_SRAM_STAT_BLKRET_RDY            ((uint32_t)0x00000002)          /*!< When 1, indicates SRAM is ready for access and blocks can be  */
#define SYSCTL_A_MASTER_UNLOCK_UNLKEY_OFS        ( 0)                            /*!< UNLKEY Bit Offset */
#define SYSCTL_A_MASTER_UNLOCK_UNLKEY_MASK       ((uint32_t)0x0000FFFF)          /*!< UNLKEY Bit Mask */
#define SYSCTL_A_RESET_REQ_POR_OFS               ( 0)                            /*!< POR Bit Offset */
#define SYSCTL_A_RESET_REQ_POR                   ((uint32_t)0x00000001)          /*!< Generate POR */
#define SYSCTL_A_RESET_REQ_REBOOT_OFS            ( 1)                            /*!< REBOOT Bit Offset */
#define SYSCTL_A_RESET_REQ_REBOOT                ((uint32_t)0x00000002)          /*!< Generate Reboot_Reset */
#define SYSCTL_A_RESET_REQ_WKEY_OFS              ( 8)                            /*!< WKEY Bit Offset */
#define SYSCTL_A_RESET_REQ_WKEY_MASK             ((uint32_t)0x0000FF00)          /*!< WKEY Bit Mask */
#define SYSCTL_A_RESET_STATOVER_SOFT_OFS         ( 0)                            /*!< SOFT Bit Offset */
#define SYSCTL_A_RESET_STATOVER_SOFT             ((uint32_t)0x00000001)          /*!< Indicates if SOFT Reset is active */
#define SYSCTL_A_RESET_STATOVER_HARD_OFS         ( 1)                            /*!< HARD Bit Offset */
#define SYSCTL_A_RESET_STATOVER_HARD             ((uint32_t)0x00000002)          /*!< Indicates if HARD Reset is active */
#define SYSCTL_A_RESET_STATOVER_REBOOT_OFS       ( 2)                            /*!< REBOOT Bit Offset */
#define SYSCTL_A_RESET_STATOVER_REBOOT           ((uint32_t)0x00000004)          /*!< Indicates if Reboot Reset is active */
#define SYSCTL_A_RESET_STATOVER_SOFT_OVER_OFS    ( 8)                            /*!< SOFT_OVER Bit Offset */
#define SYSCTL_A_RESET_STATOVER_SOFT_OVER        ((uint32_t)0x00000100)          /*!< SOFT_Reset overwrite request */
#define SYSCTL_A_RESET_STATOVER_HARD_OVER_OFS    ( 9)                            /*!< HARD_OVER Bit Offset */
#define SYSCTL_A_RESET_STATOVER_HARD_OVER        ((uint32_t)0x00000200)          /*!< HARD_Reset overwrite request */
#define SYSCTL_A_RESET_STATOVER_RBT_OVER_OFS     (10)                            /*!< RBT_OVER Bit Offset */
#define SYSCTL_A_RESET_STATOVER_RBT_OVER         ((uint32_t)0x00000400)          /*!< Reboot Reset overwrite request */
#define SYSCTL_A_CSYS_MASTER_UNLOCK_UNLKEY_VAL  ((uint32_t)0x0000695A)          /*!< Unlock key value which when written, determines if accesses to other CPU_SYS register */
#define SYSCTL_A_REBOOT_CTL_WKEY_VAL            ((uint32_t)0x00006900)          /*!< Key value to validate write to bit 0 */
#define SYSCTL_A_BOOT_CTL_WKEY_VAL              ((uint32_t)0x00006900)          /*!< Key value to validate write to bit 0 */
#define SYSCTL_A_ETW_CTL_WKEY_VAL               ((uint32_t)0x00006900)          /*!< Key value to validate write to bit 0 */
#define SYSCTL_A_SECDATA_UNLOCK_KEY_VAL         ((uint32_t)0x0000695A)          /*!< Unlock Key value, which requests for secure data region to be unlocked for data access */


/******************************************************************************
* BSL                                                                         *
******************************************************************************/
#define BSL_DEFAULT_PARAM                        ((uint32_t)0xFC48FFFF)          /*!< I2C slave address = 0x48, Interface selection = Auto */
#define BSL_API_TABLE_ADDR                       ((uint32_t)0x00202000)          /*!< Address of BSL API table */
#define BSL_ENTRY_FUNCTION                       (*((uint32_t *)BSL_API_TABLE_ADDR))

#define BSL_AUTO_INTERFACE                       ((uint32_t)0x0000E0000)         /*!< Auto detect interface */
#define BSL_UART_INTERFACE                       ((uint32_t)0x0000C0000)         /*!< UART interface */
#define BSL_SPI_INTERFACE                        ((uint32_t)0x0000A0000)         /*!< SPI interface */
#define BSL_I2C_INTERFACE                        ((uint32_t)0x000080000)         /*!< I2C interface */

#define BSL_INVOKE(x)                            ((void (*)())BSL_ENTRY_FUNCTION)((uint32_t) x) /*!< Invoke the BSL with parameters */


/******************************************************************************
* Mailbox struct legacy definition                                            *
******************************************************************************/
#define FLASH_MAILBOX_Type                    FL_BOOTOVER_MAILBOX_Type

/******************************************************************************
* Device Unlock Support                                                       *
******************************************************************************/
/* unlock the device by:
 *   Load SYSCTL_SECDATA_UNLOCK register address into R0
 *   Load SYSCTL_SECDATA_UNLOCK unlock key into R1
 *   Write the unlock key to the SYSCTL_SECDATA_UNLOCK register
 */
#define UNLOCK_DEVICE\
    __asm("  MOVW.W          R0, #0x3040");\
    __asm("  MOVT.W          R0, #0xE004");\
    __asm("  MOVW.W          R1, #0x695A");\
    __asm("  MOVT.W          R1, #0x0000");\
    __asm("  STR             R1, [R0]");

/******************************************************************************
*
* The following are values that can be used to choose the command that will be
* run by the boot code. Perform a logical OR of these settings to create your
* general parameter command.
*
******************************************************************************/
#define COMMAND_FACTORY_RESET                    ((uint32_t)0x00010000)
#define COMMAND_BSL_CONFIG                       ((uint32_t)0x00020000)
#define COMMAND_JTAG_SWD_LOCK_SECEN              ((uint32_t)0x00080000)
#define COMMAND_SEC_ZONE0_EN                     ((uint32_t)0x00100000)
#define COMMAND_SEC_ZONE1_EN                     ((uint32_t)0x00200000)
#define COMMAND_SEC_ZONE2_EN                     ((uint32_t)0x00400000)
#define COMMAND_SEC_ZONE3_EN                     ((uint32_t)0x00800000)
#define COMMAND_SEC_ZONE0_UPDATE                 ((uint32_t)0x01000000)
#define COMMAND_SEC_ZONE1_UPDATE                 ((uint32_t)0x02000000)
#define COMMAND_SEC_ZONE2_UPDATE                 ((uint32_t)0x04000000)
#define COMMAND_SEC_ZONE3_UPDATE                 ((uint32_t)0x08000000)
#define COMMAND_JTAG_SWD_LOCK_ENC_UPDATE         ((uint32_t)0x10000000)
#define COMMAND_NONE                             ((uint32_t)0xFFFFFFFF)

/******************************************************************************
*
* The following are values that can be used to configure the BSL. Perform a
* logical OR of these settings to create your BSL parameter.
*
******************************************************************************/
#define BSL_CONFIG_HW_INVOKE                     ((uint32_t)0x70000000)

#define BSL_CONFIG_HW_INVOKE_PORT1               ((uint32_t)0x00000000)
#define BSL_CONFIG_HW_INVOKE_PORT2               ((uint32_t)0x00000001)
#define BSL_CONFIG_HW_INVOKE_PORT3               ((uint32_t)0x00000002)

#define BSL_CONFIG_HW_INVOKE_PIN0                ((uint32_t)0x00000000)
#define BSL_CONFIG_HW_INVOKE_PIN1                ((uint32_t)0x00000010)
#define BSL_CONFIG_HW_INVOKE_PIN2                ((uint32_t)0x00000020)
#define BSL_CONFIG_HW_INVOKE_PIN3                ((uint32_t)0x00000030)
#define BSL_CONFIG_HW_INVOKE_PIN4                ((uint32_t)0x00000040)
#define BSL_CONFIG_HW_INVOKE_PIN5                ((uint32_t)0x00000050)
#define BSL_CONFIG_HW_INVOKE_PIN6                ((uint32_t)0x00000060)
#define BSL_CONFIG_HW_INVOKE_PIN7                ((uint32_t)0x00000070)

#define BSL_CONFIG_HW_INVOKE_PIN_LOW             ((uint32_t)0x00000000)
#define BSL_CONFIG_HW_INVOKE_PIN_HIGH            ((uint32_t)0x00001000)

#define BSL_CONFIG_INTERFACE_I2C                 ((uint32_t)0x00008000)
#define BSL_CONFIG_INTERFACE_SPI                 ((uint32_t)0x0000A000)
#define BSL_CONFIG_INTERFACE_UART                ((uint32_t)0x0000C000)
#define BSL_CONFIG_INTERFACE_AUTO                ((uint32_t)0x0000E000)

#define BSL_CONFIG_I2C_ADD_OFFSET                (16)


/******************************************************************************
* ULP Advisor                                                                 *
******************************************************************************/
#ifdef __TI_ARM__
#pragma ULP_PORT_CONFIG(1,DIR={0x40004C04,8},OUT={0x40004C02,8},SEL1={0x40004C0A,8},SEL2={0x40004C0C,8})
#pragma ULP_PORT_CONFIG(2,DIR={0x40004C05,8},OUT={0x40004C03,8},SEL1={0x40004C0B,8},SEL2={0x40004C0D,8})
#pragma ULP_PORT_CONFIG(3,DIR={0x40004C24,8},OUT={0x40004C22,8},SEL1={0x40004C2A,8},SEL2={0x40004C2C,8})
#pragma ULP_PORT_CONFIG(4,DIR={0x40004C25,8},OUT={0x40004C23,8},SEL1={0x40004C2B,8},SEL2={0x40004C2D,8})
#pragma ULP_PORT_CONFIG(5,DIR={0x40004C44,8},OUT={0x40004C42,8},SEL1={0x40004C4A,8},SEL2={0x40004C4C,8})
#pragma ULP_PORT_CONFIG(6,DIR={0x40004C45,8},OUT={0x40004C43,8},SEL1={0x40004C4B,8},SEL2={0x40004C4D,8})
#pragma ULP_PORT_CONFIG(7,DIR={0x40004C64,8},OUT={0x40004C62,8},SEL1={0x40004C6A,8},SEL2={0x40004C6C,8})
#pragma ULP_PORT_CONFIG(8,DIR={0x40004C65,8},OUT={0x40004C63,8},SEL1={0x40004C6B,8},SEL2={0x40004C6D,8})
#pragma ULP_PORT_CONFIG(9,DIR={0x40004C84,8},OUT={0x40004C82,8},SEL1={0x40004C8A,8},SEL2={0x40004C8C,8})
#pragma ULP_PORT_CONFIG(10,DIR={0x40004C85,8},OUT={0x40004C83,8},SEL1={0x40004C8B,8},SEL2={0x40004C8D,8})
#endif


#ifdef __cplusplus
}
#endif

#endif /* __MSP432P4XX_H__ */

