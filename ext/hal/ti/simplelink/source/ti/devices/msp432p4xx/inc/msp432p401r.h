/******************************************************************************
*
* Copyright (C) 2012 - 2017 Texas Instruments Incorporated - http://www.ti.com/
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
* MSP432P401R Register Definitions
*
* This file includes CMSIS compliant component and register definitions
*
* For legacy components the definitions that are compatible with MSP430 code,
* are included with msp432p401r_classic.h
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

#ifndef __MSP432P401R_H__
#define __MSP432P401R_H__

/* Use standard integer types with explicit width */
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define __MSP432_HEADER_VERSION__ 3202

/* Remap MSP432 intrinsics to ARM equivalents */
#include "msp_compatibility.h"

/******************************************************************************
* include MSP430 legacy definitions to make porting of code from MSP430       *
* code base easier                                                            *
* With fully CMSIS compliant code, NO_MSP_CLASSIC_DEFINES may be defined in   *
* your project to omit including the classic defines                          *
******************************************************************************/
#ifndef NO_MSP_CLASSIC_DEFINES
#include "msp432p401r_classic.h"
#endif


#ifndef __CMSIS_CONFIG__
#define __CMSIS_CONFIG__

/** @addtogroup MSP432P401R_Definitions MSP432P401R Definitions
  This file defines all structures and symbols for MSP432P401R:
    - components and registers
    - peripheral base address
    - peripheral ID
    - Peripheral definitions
  @{
*/

/******************************************************************************
*                Processor and Core Peripherals                               *
******************************************************************************/
/** @addtogroup MSP432P401R_CMSIS Device CMSIS Definitions
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
  PORT6_IRQn                  = 40      /* 56 Port6 Interrupt           */
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

/* Definitions to show that specific ports are available */

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


/*@}*/ /* end of group MSP432P401R_CMSIS */

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
/** @addtogroup MSP432P401R_MemoryMap MSP432P401R Memory Mapping
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


/*@}*/ /* end of group MSP432P401R_MemoryMap */

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
/** @addtogroup MSP432P401R_Peripherals MSP432P401R Peripherals
  MSP432P401R Device Specific Peripheral registers structures
  @{
*/

#if defined ( __CC_ARM )
#pragma anon_unions
#endif


/******************************************************************************
* ADC14 Registers
******************************************************************************/
/** @addtogroup ADC14 MSP432P401R (ADC14)
  @{
*/
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

/*@}*/ /* end of group ADC14 */


/******************************************************************************
* AES256 Registers
******************************************************************************/
/** @addtogroup AES256 MSP432P401R (AES256)
  @{
*/
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

/*@}*/ /* end of group AES256 */


/******************************************************************************
* CAPTIO Registers
******************************************************************************/
/** @addtogroup CAPTIO MSP432P401R (CAPTIO)
  @{
*/
typedef struct {
       uint16_t RESERVED0[7];
  __IO uint16_t CTL;                                                             /*!< Capacitive Touch IO x Control Register */
} CAPTIO_Type;

/*@}*/ /* end of group CAPTIO */


/******************************************************************************
* COMP_E Registers
******************************************************************************/
/** @addtogroup COMP_E MSP432P401R (COMP_E)
  @{
*/
typedef struct {
  __IO uint16_t CTL0;                                                            /*!< Comparator Control Register 0 */
  __IO uint16_t CTL1;                                                            /*!< Comparator Control Register 1 */
  __IO uint16_t CTL2;                                                            /*!< Comparator Control Register 2 */
  __IO uint16_t CTL3;                                                            /*!< Comparator Control Register 3 */
       uint16_t RESERVED0[2];
  __IO uint16_t INT;                                                             /*!< Comparator Interrupt Control Register */
  __I  uint16_t IV;                                                              /*!< Comparator Interrupt Vector Word Register */
} COMP_E_Type;

/*@}*/ /* end of group COMP_E */


/******************************************************************************
* CRC32 Registers
******************************************************************************/
/** @addtogroup CRC32 MSP432P401R (CRC32)
  @{
*/
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

/*@}*/ /* end of group CRC32 */


/******************************************************************************
* CS Registers
******************************************************************************/
/** @addtogroup CS MSP432P401R (CS)
  @{
*/
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

/*@}*/ /* end of group CS */


/******************************************************************************
* DIO Registers
******************************************************************************/
/** @addtogroup DIO MSP432P401R (DIO)
  @{
*/
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

/*@}*/ /* end of group MSP432P401R_DIO */


/******************************************************************************
* DMA Registers
******************************************************************************/
/** @addtogroup DMA MSP432P401R (DMA)
  @{
*/
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

/*@}*/ /* end of group DMA */


/******************************************************************************
* EUSCI_A Registers
******************************************************************************/
/** @addtogroup EUSCI_A MSP432P401R (EUSCI_A)
  @{
*/
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

/*@}*/ /* end of group EUSCI_A */

/** @addtogroup EUSCI_A_SPI MSP432P401R (EUSCI_A_SPI)
  @{
*/
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

/*@}*/ /* end of group EUSCI_A_SPI */


/******************************************************************************
* EUSCI_B Registers
******************************************************************************/
/** @addtogroup EUSCI_B MSP432P401R (EUSCI_B)
  @{
*/
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

/*@}*/ /* end of group EUSCI_B */

/** @addtogroup EUSCI_B_SPI MSP432P401R (EUSCI_B_SPI)
  @{
*/
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

/*@}*/ /* end of group EUSCI_B_SPI */


/******************************************************************************
* FLCTL Registers
******************************************************************************/
/** @addtogroup FLCTL MSP432P401R (FLCTL)
  @{
*/
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

/*@}*/ /* end of group FLCTL */


/******************************************************************************
* FL_BOOTOVER_MAILBOX Registers
******************************************************************************/
/** @addtogroup SEC_ZONE_PARAMS MSP432P401R (FL_BOOTOVER_MAILBOX)
  @{
*/
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

/*@}*/ /* end of group SEC_ZONE_PARAMS */

/** @addtogroup SEC_ZONE_UPDATE MSP432P401R (FL_BOOTOVER_MAILBOX)
  @{
*/
typedef struct {
  __IO uint32_t SEC_ZONE_PAYLOADADDR;                                            /*!< Start address where the payload is loaded in the device. */
  __IO uint32_t SEC_ZONE_PAYLOADLEN;                                             /*!< Length of the payload in bytes. */
  __IO uint32_t SEC_ZONE_UPDATE_ACK;                                             /*!< Acknowledgment for the IP Protected Secure Zone Update Command */
       uint32_t RESERVED0;
} SEC_ZONE_UPDATE_Type;

/*@}*/ /* end of group SEC_ZONE_UPDATE */

/** @addtogroup FL_BOOTOVER_MAILBOX MSP432P401R (FL_BOOTOVER_MAILBOX)
  @{
*/
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

/*@}*/ /* end of group FL_BOOTOVER_MAILBOX */


/******************************************************************************
* PCM Registers
******************************************************************************/
/** @addtogroup PCM MSP432P401R (PCM)
  @{
*/
typedef struct {
  __IO uint32_t CTL0;                                                            /*!< Control 0 Register */
  __IO uint32_t CTL1;                                                            /*!< Control 1 Register */
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
  __I  uint32_t IFG;                                                             /*!< Interrupt Flag Register */
  __O  uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
} PCM_Type;

/*@}*/ /* end of group PCM */


/******************************************************************************
* PMAP Registers
******************************************************************************/
/** @addtogroup PMAP MSP432P401R (PMAP)
  @{
*/
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

/*@}*/ /* end of group PMAP */


/******************************************************************************
* PSS Registers
******************************************************************************/
/** @addtogroup PSS MSP432P401R (PSS)
  @{
*/
typedef struct {
  __IO uint32_t KEY;                                                             /*!< Key Register */
  __IO uint32_t CTL0;                                                            /*!< Control 0 Register */
       uint32_t RESERVED0[11];
  __IO uint32_t IE;                                                              /*!< Interrupt Enable Register */
  __I  uint32_t IFG;                                                             /*!< Interrupt Flag Register */
  __IO uint32_t CLRIFG;                                                          /*!< Clear Interrupt Flag Register */
} PSS_Type;

/*@}*/ /* end of group PSS */


/******************************************************************************
* REF_A Registers
******************************************************************************/
/** @addtogroup REF_A MSP432P401R (REF_A)
  @{
*/
typedef struct {
  __IO uint16_t CTL0;                                                            /*!< REF Control Register 0 */
} REF_A_Type;

/*@}*/ /* end of group REF_A */


/******************************************************************************
* RSTCTL Registers
******************************************************************************/
/** @addtogroup RSTCTL MSP432P401R (RSTCTL)
  @{
*/
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

/*@}*/ /* end of group RSTCTL */


/******************************************************************************
* RTC_C Registers
******************************************************************************/
/** @addtogroup RTC_C MSP432P401R (RTC_C)
  @{
*/
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

/*@}*/ /* end of group RTC_C */

/** @addtogroup RTC_C_BCD MSP432P401R (RTC_C_BCD)
  @{
*/
typedef struct {
       uint16_t RESERVED0[8];
  __IO uint16_t TIM0;                                                            /*!< Real-Time Clock Seconds, Minutes Register - BCD Format */
  __IO uint16_t TIM1;                                                            /*!< Real-Time Clock Hour, Day of Week - BCD Format */
  __IO uint16_t DATE;                                                            /*!< Real-Time Clock Date - BCD Format */
  __IO uint16_t YEAR;                                                            /*!< Real-Time Clock Year Register - BCD Format */
  __IO uint16_t AMINHR;                                                          /*!< Real-Time Clock Minutes, Hour Alarm - BCD Format */
  __IO uint16_t ADOWDAY;                                                         /*!< Real-Time Clock Day of Week, Day of Month Alarm - BCD Format */
} RTC_C_BCD_Type;

/*@}*/ /* end of group RTC_C_BCD */


/******************************************************************************
* SYSCTL Registers
******************************************************************************/
/** @addtogroup SYSCTL MSP432P401R (SYSCTL)
  @{
*/
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
  __IO uint32_t MASTER_UNLOCK;                                                   /*!< Master Unlock Register */
  __IO uint32_t BOOTOVER_REQ[2];                                                 /*!< Boot Override Request Register */
  __IO uint32_t BOOTOVER_ACK;                                                    /*!< Boot Override Acknowledge Register */
  __IO uint32_t RESET_REQ;                                                       /*!< Reset Request Register */
  __IO uint32_t RESET_STATOVER;                                                  /*!< Reset Status and Override Register */
       uint32_t RESERVED7[2];
  __I  uint32_t SYSTEM_STAT;                                                     /*!< System Status Register */
} SYSCTL_Boot_Type;

/*@}*/ /* end of group SYSCTL */


/******************************************************************************
* Timer32 Registers
******************************************************************************/
/** @addtogroup Timer32 MSP432P401R (Timer32)
  @{
*/
typedef struct {
  __IO uint32_t LOAD;                                                            /*!< Timer Load Register */
  __I  uint32_t VALUE;                                                           /*!< Timer Current Value Register */
  __IO uint32_t CONTROL;                                                         /*!< Timer Control Register */
  __O  uint32_t INTCLR;                                                          /*!< Timer Interrupt Clear Register */
  __I  uint32_t RIS;                                                             /*!< Timer Raw Interrupt Status Register */
  __I  uint32_t MIS;                                                             /*!< Timer Interrupt Status Register */
  __IO uint32_t BGLOAD;                                                          /*!< Timer Background Load Register */
} Timer32_Type;

/*@}*/ /* end of group Timer32 */


/******************************************************************************
* Timer_A Registers
******************************************************************************/
/** @addtogroup Timer_A MSP432P401R (Timer_A)
  @{
*/
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

/*@}*/ /* end of group Timer_A */


/******************************************************************************
* TLV Registers
******************************************************************************/
/** @addtogroup TLV MSP432P401R (TLV)
  @{
*/
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

/*@}*/ /* end of group TLV */


/******************************************************************************
* WDT_A Registers
******************************************************************************/
/** @addtogroup WDT_A MSP432P401R (WDT_A)
  @{
*/
typedef struct {
       uint16_t RESERVED0[6];
  __IO uint16_t CTL;                                                             /*!< Watchdog Timer Control Register */
} WDT_A_Type;

/*@}*/ /* end of group WDT_A */


#if defined ( __CC_ARM )
#pragma no_anon_unions
#endif

/*@}*/ /* end of group MSP432P401R_Peripherals */

/******************************************************************************
* Peripheral declaration                                                      *
******************************************************************************/
/** @addtogroup MSP432P401R_PeripheralDecl MSP432P401R Peripheral Declaration
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


/*@}*/ /* end of group MSP432P401R_PeripheralDecl */

/*@}*/ /* end of group MSP432P401R_Definitions */

#endif /* __CMSIS_CONFIG__ */

/******************************************************************************
* Peripheral register control bits                                            *
******************************************************************************/

/******************************************************************************
* ADC14 Bits
******************************************************************************/
/* ADC14_CTL0[SC] Bits */
#define ADC14_CTL0_SC_OFS                        ( 0)                            /*!< ADC14SC Bit Offset */
#define ADC14_CTL0_SC                            ((uint32_t)0x00000001)          /*!< ADC14 start conversion */
/* ADC14_CTL0[ENC] Bits */
#define ADC14_CTL0_ENC_OFS                       ( 1)                            /*!< ADC14ENC Bit Offset */
#define ADC14_CTL0_ENC                           ((uint32_t)0x00000002)          /*!< ADC14 enable conversion */
/* ADC14_CTL0[ON] Bits */
#define ADC14_CTL0_ON_OFS                        ( 4)                            /*!< ADC14ON Bit Offset */
#define ADC14_CTL0_ON                            ((uint32_t)0x00000010)          /*!< ADC14 on */
/* ADC14_CTL0[MSC] Bits */
#define ADC14_CTL0_MSC_OFS                       ( 7)                            /*!< ADC14MSC Bit Offset */
#define ADC14_CTL0_MSC                           ((uint32_t)0x00000080)          /*!< ADC14 multiple sample and conversion */
/* ADC14_CTL0[SHT0] Bits */
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
/* ADC14_CTL0[SHT1] Bits */
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
/* ADC14_CTL0[BUSY] Bits */
#define ADC14_CTL0_BUSY_OFS                      (16)                            /*!< ADC14BUSY Bit Offset */
#define ADC14_CTL0_BUSY                          ((uint32_t)0x00010000)          /*!< ADC14 busy */
/* ADC14_CTL0[CONSEQ] Bits */
#define ADC14_CTL0_CONSEQ_OFS                    (17)                            /*!< ADC14CONSEQ Bit Offset */
#define ADC14_CTL0_CONSEQ_MASK                   ((uint32_t)0x00060000)          /*!< ADC14CONSEQ Bit Mask */
#define ADC14_CTL0_CONSEQ0                       ((uint32_t)0x00020000)          /*!< CONSEQ Bit 0 */
#define ADC14_CTL0_CONSEQ1                       ((uint32_t)0x00040000)          /*!< CONSEQ Bit 1 */
#define ADC14_CTL0_CONSEQ_0                      ((uint32_t)0x00000000)          /*!< Single-channel, single-conversion */
#define ADC14_CTL0_CONSEQ_1                      ((uint32_t)0x00020000)          /*!< Sequence-of-channels */
#define ADC14_CTL0_CONSEQ_2                      ((uint32_t)0x00040000)          /*!< Repeat-single-channel */
#define ADC14_CTL0_CONSEQ_3                      ((uint32_t)0x00060000)          /*!< Repeat-sequence-of-channels */
/* ADC14_CTL0[SSEL] Bits */
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
/* ADC14_CTL0[DIV] Bits */
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
/* ADC14_CTL0[ISSH] Bits */
#define ADC14_CTL0_ISSH_OFS                      (25)                            /*!< ADC14ISSH Bit Offset */
#define ADC14_CTL0_ISSH                          ((uint32_t)0x02000000)          /*!< ADC14 invert signal sample-and-hold */
/* ADC14_CTL0[SHP] Bits */
#define ADC14_CTL0_SHP_OFS                       (26)                            /*!< ADC14SHP Bit Offset */
#define ADC14_CTL0_SHP                           ((uint32_t)0x04000000)          /*!< ADC14 sample-and-hold pulse-mode select */
/* ADC14_CTL0[SHS] Bits */
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
/* ADC14_CTL0[PDIV] Bits */
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
/* ADC14_CTL1[PWRMD] Bits */
#define ADC14_CTL1_PWRMD_OFS                     ( 0)                            /*!< ADC14PWRMD Bit Offset */
#define ADC14_CTL1_PWRMD_MASK                    ((uint32_t)0x00000003)          /*!< ADC14PWRMD Bit Mask */
#define ADC14_CTL1_PWRMD0                        ((uint32_t)0x00000001)          /*!< PWRMD Bit 0 */
#define ADC14_CTL1_PWRMD1                        ((uint32_t)0x00000002)          /*!< PWRMD Bit 1 */
#define ADC14_CTL1_PWRMD_0                       ((uint32_t)0x00000000)          /*!< Regular power mode for use with any resolution setting. Sample rate can be  */
                                                                                 /* up to 1 Msps. */
#define ADC14_CTL1_PWRMD_2                       ((uint32_t)0x00000002)          /*!< Low-power mode for 12-bit, 10-bit, and 8-bit resolution settings. Sample  */
                                                                                 /* rate must not exceed 200 ksps. */
/* ADC14_CTL1[REFBURST] Bits */
#define ADC14_CTL1_REFBURST_OFS                  ( 2)                            /*!< ADC14REFBURST Bit Offset */
#define ADC14_CTL1_REFBURST                      ((uint32_t)0x00000004)          /*!< ADC14 reference buffer burst */
/* ADC14_CTL1[DF] Bits */
#define ADC14_CTL1_DF_OFS                        ( 3)                            /*!< ADC14DF Bit Offset */
#define ADC14_CTL1_DF                            ((uint32_t)0x00000008)          /*!< ADC14 data read-back format */
/* ADC14_CTL1[RES] Bits */
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
/* ADC14_CTL1[CSTARTADD] Bits */
#define ADC14_CTL1_CSTARTADD_OFS                 (16)                            /*!< ADC14CSTARTADD Bit Offset */
#define ADC14_CTL1_CSTARTADD_MASK                ((uint32_t)0x001F0000)          /*!< ADC14CSTARTADD Bit Mask */
/* ADC14_CTL1[BATMAP] Bits */
#define ADC14_CTL1_BATMAP_OFS                    (22)                            /*!< ADC14BATMAP Bit Offset */
#define ADC14_CTL1_BATMAP                        ((uint32_t)0x00400000)          /*!< Controls 1/2 AVCC ADC input channel selection */
/* ADC14_CTL1[TCMAP] Bits */
#define ADC14_CTL1_TCMAP_OFS                     (23)                            /*!< ADC14TCMAP Bit Offset */
#define ADC14_CTL1_TCMAP                         ((uint32_t)0x00800000)          /*!< Controls temperature sensor ADC input channel selection */
/* ADC14_CTL1[CH0MAP] Bits */
#define ADC14_CTL1_CH0MAP_OFS                    (24)                            /*!< ADC14CH0MAP Bit Offset */
#define ADC14_CTL1_CH0MAP                        ((uint32_t)0x01000000)          /*!< Controls internal channel 0 selection to ADC input channel MAX-2 */
/* ADC14_CTL1[CH1MAP] Bits */
#define ADC14_CTL1_CH1MAP_OFS                    (25)                            /*!< ADC14CH1MAP Bit Offset */
#define ADC14_CTL1_CH1MAP                        ((uint32_t)0x02000000)          /*!< Controls internal channel 1 selection to ADC input channel MAX-3 */
/* ADC14_CTL1[CH2MAP] Bits */
#define ADC14_CTL1_CH2MAP_OFS                    (26)                            /*!< ADC14CH2MAP Bit Offset */
#define ADC14_CTL1_CH2MAP                        ((uint32_t)0x04000000)          /*!< Controls internal channel 2 selection to ADC input channel MAX-4 */
/* ADC14_CTL1[CH3MAP] Bits */
#define ADC14_CTL1_CH3MAP_OFS                    (27)                            /*!< ADC14CH3MAP Bit Offset */
#define ADC14_CTL1_CH3MAP                        ((uint32_t)0x08000000)          /*!< Controls internal channel 3 selection to ADC input channel MAX-5 */
/* ADC14_LO0[LO0] Bits */
#define ADC14_LO0_LO0_OFS                        ( 0)                            /*!< ADC14LO0 Bit Offset */
#define ADC14_LO0_LO0_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14LO0 Bit Mask */
/* ADC14_HI0[HI0] Bits */
#define ADC14_HI0_HI0_OFS                        ( 0)                            /*!< ADC14HI0 Bit Offset */
#define ADC14_HI0_HI0_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14HI0 Bit Mask */
/* ADC14_LO1[LO1] Bits */
#define ADC14_LO1_LO1_OFS                        ( 0)                            /*!< ADC14LO1 Bit Offset */
#define ADC14_LO1_LO1_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14LO1 Bit Mask */
/* ADC14_HI1[HI1] Bits */
#define ADC14_HI1_HI1_OFS                        ( 0)                            /*!< ADC14HI1 Bit Offset */
#define ADC14_HI1_HI1_MASK                       ((uint32_t)0x0000FFFF)          /*!< ADC14HI1 Bit Mask */
/* ADC14_MCTLN[INCH] Bits */
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
/* ADC14_MCTLN[EOS] Bits */
#define ADC14_MCTLN_EOS_OFS                      ( 7)                            /*!< ADC14EOS Bit Offset */
#define ADC14_MCTLN_EOS                          ((uint32_t)0x00000080)          /*!< End of sequence */
/* ADC14_MCTLN[VRSEL] Bits */
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
/* ADC14_MCTLN[DIF] Bits */
#define ADC14_MCTLN_DIF_OFS                      (13)                            /*!< ADC14DIF Bit Offset */
#define ADC14_MCTLN_DIF                          ((uint32_t)0x00002000)          /*!< Differential mode */
/* ADC14_MCTLN[WINC] Bits */
#define ADC14_MCTLN_WINC_OFS                     (14)                            /*!< ADC14WINC Bit Offset */
#define ADC14_MCTLN_WINC                         ((uint32_t)0x00004000)          /*!< Comparator window enable */
/* ADC14_MCTLN[WINCTH] Bits */
#define ADC14_MCTLN_WINCTH_OFS                   (15)                            /*!< ADC14WINCTH Bit Offset */
#define ADC14_MCTLN_WINCTH                       ((uint32_t)0x00008000)          /*!< Window comparator threshold register selection */
/* ADC14_MEMN[CONVRES] Bits */
#define ADC14_MEMN_CONVRES_OFS                   ( 0)                            /*!< Conversion_Results Bit Offset */
#define ADC14_MEMN_CONVRES_MASK                  ((uint32_t)0x0000FFFF)          /*!< Conversion_Results Bit Mask */
/* ADC14_IER0[IE0] Bits */
#define ADC14_IER0_IE0_OFS                       ( 0)                            /*!< ADC14IE0 Bit Offset */
#define ADC14_IER0_IE0                           ((uint32_t)0x00000001)          /*!< Interrupt enable */
/* ADC14_IER0[IE1] Bits */
#define ADC14_IER0_IE1_OFS                       ( 1)                            /*!< ADC14IE1 Bit Offset */
#define ADC14_IER0_IE1                           ((uint32_t)0x00000002)          /*!< Interrupt enable */
/* ADC14_IER0[IE2] Bits */
#define ADC14_IER0_IE2_OFS                       ( 2)                            /*!< ADC14IE2 Bit Offset */
#define ADC14_IER0_IE2                           ((uint32_t)0x00000004)          /*!< Interrupt enable */
/* ADC14_IER0[IE3] Bits */
#define ADC14_IER0_IE3_OFS                       ( 3)                            /*!< ADC14IE3 Bit Offset */
#define ADC14_IER0_IE3                           ((uint32_t)0x00000008)          /*!< Interrupt enable */
/* ADC14_IER0[IE4] Bits */
#define ADC14_IER0_IE4_OFS                       ( 4)                            /*!< ADC14IE4 Bit Offset */
#define ADC14_IER0_IE4                           ((uint32_t)0x00000010)          /*!< Interrupt enable */
/* ADC14_IER0[IE5] Bits */
#define ADC14_IER0_IE5_OFS                       ( 5)                            /*!< ADC14IE5 Bit Offset */
#define ADC14_IER0_IE5                           ((uint32_t)0x00000020)          /*!< Interrupt enable */
/* ADC14_IER0[IE6] Bits */
#define ADC14_IER0_IE6_OFS                       ( 6)                            /*!< ADC14IE6 Bit Offset */
#define ADC14_IER0_IE6                           ((uint32_t)0x00000040)          /*!< Interrupt enable */
/* ADC14_IER0[IE7] Bits */
#define ADC14_IER0_IE7_OFS                       ( 7)                            /*!< ADC14IE7 Bit Offset */
#define ADC14_IER0_IE7                           ((uint32_t)0x00000080)          /*!< Interrupt enable */
/* ADC14_IER0[IE8] Bits */
#define ADC14_IER0_IE8_OFS                       ( 8)                            /*!< ADC14IE8 Bit Offset */
#define ADC14_IER0_IE8                           ((uint32_t)0x00000100)          /*!< Interrupt enable */
/* ADC14_IER0[IE9] Bits */
#define ADC14_IER0_IE9_OFS                       ( 9)                            /*!< ADC14IE9 Bit Offset */
#define ADC14_IER0_IE9                           ((uint32_t)0x00000200)          /*!< Interrupt enable */
/* ADC14_IER0[IE10] Bits */
#define ADC14_IER0_IE10_OFS                      (10)                            /*!< ADC14IE10 Bit Offset */
#define ADC14_IER0_IE10                          ((uint32_t)0x00000400)          /*!< Interrupt enable */
/* ADC14_IER0[IE11] Bits */
#define ADC14_IER0_IE11_OFS                      (11)                            /*!< ADC14IE11 Bit Offset */
#define ADC14_IER0_IE11                          ((uint32_t)0x00000800)          /*!< Interrupt enable */
/* ADC14_IER0[IE12] Bits */
#define ADC14_IER0_IE12_OFS                      (12)                            /*!< ADC14IE12 Bit Offset */
#define ADC14_IER0_IE12                          ((uint32_t)0x00001000)          /*!< Interrupt enable */
/* ADC14_IER0[IE13] Bits */
#define ADC14_IER0_IE13_OFS                      (13)                            /*!< ADC14IE13 Bit Offset */
#define ADC14_IER0_IE13                          ((uint32_t)0x00002000)          /*!< Interrupt enable */
/* ADC14_IER0[IE14] Bits */
#define ADC14_IER0_IE14_OFS                      (14)                            /*!< ADC14IE14 Bit Offset */
#define ADC14_IER0_IE14                          ((uint32_t)0x00004000)          /*!< Interrupt enable */
/* ADC14_IER0[IE15] Bits */
#define ADC14_IER0_IE15_OFS                      (15)                            /*!< ADC14IE15 Bit Offset */
#define ADC14_IER0_IE15                          ((uint32_t)0x00008000)          /*!< Interrupt enable */
/* ADC14_IER0[IE16] Bits */
#define ADC14_IER0_IE16_OFS                      (16)                            /*!< ADC14IE16 Bit Offset */
#define ADC14_IER0_IE16                          ((uint32_t)0x00010000)          /*!< Interrupt enable */
/* ADC14_IER0[IE17] Bits */
#define ADC14_IER0_IE17_OFS                      (17)                            /*!< ADC14IE17 Bit Offset */
#define ADC14_IER0_IE17                          ((uint32_t)0x00020000)          /*!< Interrupt enable */
/* ADC14_IER0[IE19] Bits */
#define ADC14_IER0_IE19_OFS                      (19)                            /*!< ADC14IE19 Bit Offset */
#define ADC14_IER0_IE19                          ((uint32_t)0x00080000)          /*!< Interrupt enable */
/* ADC14_IER0[IE18] Bits */
#define ADC14_IER0_IE18_OFS                      (18)                            /*!< ADC14IE18 Bit Offset */
#define ADC14_IER0_IE18                          ((uint32_t)0x00040000)          /*!< Interrupt enable */
/* ADC14_IER0[IE20] Bits */
#define ADC14_IER0_IE20_OFS                      (20)                            /*!< ADC14IE20 Bit Offset */
#define ADC14_IER0_IE20                          ((uint32_t)0x00100000)          /*!< Interrupt enable */
/* ADC14_IER0[IE21] Bits */
#define ADC14_IER0_IE21_OFS                      (21)                            /*!< ADC14IE21 Bit Offset */
#define ADC14_IER0_IE21                          ((uint32_t)0x00200000)          /*!< Interrupt enable */
/* ADC14_IER0[IE22] Bits */
#define ADC14_IER0_IE22_OFS                      (22)                            /*!< ADC14IE22 Bit Offset */
#define ADC14_IER0_IE22                          ((uint32_t)0x00400000)          /*!< Interrupt enable */
/* ADC14_IER0[IE23] Bits */
#define ADC14_IER0_IE23_OFS                      (23)                            /*!< ADC14IE23 Bit Offset */
#define ADC14_IER0_IE23                          ((uint32_t)0x00800000)          /*!< Interrupt enable */
/* ADC14_IER0[IE24] Bits */
#define ADC14_IER0_IE24_OFS                      (24)                            /*!< ADC14IE24 Bit Offset */
#define ADC14_IER0_IE24                          ((uint32_t)0x01000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE25] Bits */
#define ADC14_IER0_IE25_OFS                      (25)                            /*!< ADC14IE25 Bit Offset */
#define ADC14_IER0_IE25                          ((uint32_t)0x02000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE26] Bits */
#define ADC14_IER0_IE26_OFS                      (26)                            /*!< ADC14IE26 Bit Offset */
#define ADC14_IER0_IE26                          ((uint32_t)0x04000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE27] Bits */
#define ADC14_IER0_IE27_OFS                      (27)                            /*!< ADC14IE27 Bit Offset */
#define ADC14_IER0_IE27                          ((uint32_t)0x08000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE28] Bits */
#define ADC14_IER0_IE28_OFS                      (28)                            /*!< ADC14IE28 Bit Offset */
#define ADC14_IER0_IE28                          ((uint32_t)0x10000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE29] Bits */
#define ADC14_IER0_IE29_OFS                      (29)                            /*!< ADC14IE29 Bit Offset */
#define ADC14_IER0_IE29                          ((uint32_t)0x20000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE30] Bits */
#define ADC14_IER0_IE30_OFS                      (30)                            /*!< ADC14IE30 Bit Offset */
#define ADC14_IER0_IE30                          ((uint32_t)0x40000000)          /*!< Interrupt enable */
/* ADC14_IER0[IE31] Bits */
#define ADC14_IER0_IE31_OFS                      (31)                            /*!< ADC14IE31 Bit Offset */
#define ADC14_IER0_IE31                          ((uint32_t)0x80000000)          /*!< Interrupt enable */
/* ADC14_IER1[INIE] Bits */
#define ADC14_IER1_INIE_OFS                      ( 1)                            /*!< ADC14INIE Bit Offset */
#define ADC14_IER1_INIE                          ((uint32_t)0x00000002)          /*!< Interrupt enable for ADC14MEMx within comparator window */
/* ADC14_IER1[LOIE] Bits */
#define ADC14_IER1_LOIE_OFS                      ( 2)                            /*!< ADC14LOIE Bit Offset */
#define ADC14_IER1_LOIE                          ((uint32_t)0x00000004)          /*!< Interrupt enable for ADC14MEMx below comparator window */
/* ADC14_IER1[HIIE] Bits */
#define ADC14_IER1_HIIE_OFS                      ( 3)                            /*!< ADC14HIIE Bit Offset */
#define ADC14_IER1_HIIE                          ((uint32_t)0x00000008)          /*!< Interrupt enable for ADC14MEMx above comparator window */
/* ADC14_IER1[OVIE] Bits */
#define ADC14_IER1_OVIE_OFS                      ( 4)                            /*!< ADC14OVIE Bit Offset */
#define ADC14_IER1_OVIE                          ((uint32_t)0x00000010)          /*!< ADC14MEMx overflow-interrupt enable */
/* ADC14_IER1[TOVIE] Bits */
#define ADC14_IER1_TOVIE_OFS                     ( 5)                            /*!< ADC14TOVIE Bit Offset */
#define ADC14_IER1_TOVIE                         ((uint32_t)0x00000020)          /*!< ADC14 conversion-time-overflow interrupt enable */
/* ADC14_IER1[RDYIE] Bits */
#define ADC14_IER1_RDYIE_OFS                     ( 6)                            /*!< ADC14RDYIE Bit Offset */
#define ADC14_IER1_RDYIE                         ((uint32_t)0x00000040)          /*!< ADC14 local buffered reference ready interrupt enable */
/* ADC14_IFGR0[IFG0] Bits */
#define ADC14_IFGR0_IFG0_OFS                     ( 0)                            /*!< ADC14IFG0 Bit Offset */
#define ADC14_IFGR0_IFG0                         ((uint32_t)0x00000001)          /*!< ADC14MEM0 interrupt flag */
/* ADC14_IFGR0[IFG1] Bits */
#define ADC14_IFGR0_IFG1_OFS                     ( 1)                            /*!< ADC14IFG1 Bit Offset */
#define ADC14_IFGR0_IFG1                         ((uint32_t)0x00000002)          /*!< ADC14MEM1 interrupt flag */
/* ADC14_IFGR0[IFG2] Bits */
#define ADC14_IFGR0_IFG2_OFS                     ( 2)                            /*!< ADC14IFG2 Bit Offset */
#define ADC14_IFGR0_IFG2                         ((uint32_t)0x00000004)          /*!< ADC14MEM2 interrupt flag */
/* ADC14_IFGR0[IFG3] Bits */
#define ADC14_IFGR0_IFG3_OFS                     ( 3)                            /*!< ADC14IFG3 Bit Offset */
#define ADC14_IFGR0_IFG3                         ((uint32_t)0x00000008)          /*!< ADC14MEM3 interrupt flag */
/* ADC14_IFGR0[IFG4] Bits */
#define ADC14_IFGR0_IFG4_OFS                     ( 4)                            /*!< ADC14IFG4 Bit Offset */
#define ADC14_IFGR0_IFG4                         ((uint32_t)0x00000010)          /*!< ADC14MEM4 interrupt flag */
/* ADC14_IFGR0[IFG5] Bits */
#define ADC14_IFGR0_IFG5_OFS                     ( 5)                            /*!< ADC14IFG5 Bit Offset */
#define ADC14_IFGR0_IFG5                         ((uint32_t)0x00000020)          /*!< ADC14MEM5 interrupt flag */
/* ADC14_IFGR0[IFG6] Bits */
#define ADC14_IFGR0_IFG6_OFS                     ( 6)                            /*!< ADC14IFG6 Bit Offset */
#define ADC14_IFGR0_IFG6                         ((uint32_t)0x00000040)          /*!< ADC14MEM6 interrupt flag */
/* ADC14_IFGR0[IFG7] Bits */
#define ADC14_IFGR0_IFG7_OFS                     ( 7)                            /*!< ADC14IFG7 Bit Offset */
#define ADC14_IFGR0_IFG7                         ((uint32_t)0x00000080)          /*!< ADC14MEM7 interrupt flag */
/* ADC14_IFGR0[IFG8] Bits */
#define ADC14_IFGR0_IFG8_OFS                     ( 8)                            /*!< ADC14IFG8 Bit Offset */
#define ADC14_IFGR0_IFG8                         ((uint32_t)0x00000100)          /*!< ADC14MEM8 interrupt flag */
/* ADC14_IFGR0[IFG9] Bits */
#define ADC14_IFGR0_IFG9_OFS                     ( 9)                            /*!< ADC14IFG9 Bit Offset */
#define ADC14_IFGR0_IFG9                         ((uint32_t)0x00000200)          /*!< ADC14MEM9 interrupt flag */
/* ADC14_IFGR0[IFG10] Bits */
#define ADC14_IFGR0_IFG10_OFS                    (10)                            /*!< ADC14IFG10 Bit Offset */
#define ADC14_IFGR0_IFG10                        ((uint32_t)0x00000400)          /*!< ADC14MEM10 interrupt flag */
/* ADC14_IFGR0[IFG11] Bits */
#define ADC14_IFGR0_IFG11_OFS                    (11)                            /*!< ADC14IFG11 Bit Offset */
#define ADC14_IFGR0_IFG11                        ((uint32_t)0x00000800)          /*!< ADC14MEM11 interrupt flag */
/* ADC14_IFGR0[IFG12] Bits */
#define ADC14_IFGR0_IFG12_OFS                    (12)                            /*!< ADC14IFG12 Bit Offset */
#define ADC14_IFGR0_IFG12                        ((uint32_t)0x00001000)          /*!< ADC14MEM12 interrupt flag */
/* ADC14_IFGR0[IFG13] Bits */
#define ADC14_IFGR0_IFG13_OFS                    (13)                            /*!< ADC14IFG13 Bit Offset */
#define ADC14_IFGR0_IFG13                        ((uint32_t)0x00002000)          /*!< ADC14MEM13 interrupt flag */
/* ADC14_IFGR0[IFG14] Bits */
#define ADC14_IFGR0_IFG14_OFS                    (14)                            /*!< ADC14IFG14 Bit Offset */
#define ADC14_IFGR0_IFG14                        ((uint32_t)0x00004000)          /*!< ADC14MEM14 interrupt flag */
/* ADC14_IFGR0[IFG15] Bits */
#define ADC14_IFGR0_IFG15_OFS                    (15)                            /*!< ADC14IFG15 Bit Offset */
#define ADC14_IFGR0_IFG15                        ((uint32_t)0x00008000)          /*!< ADC14MEM15 interrupt flag */
/* ADC14_IFGR0[IFG16] Bits */
#define ADC14_IFGR0_IFG16_OFS                    (16)                            /*!< ADC14IFG16 Bit Offset */
#define ADC14_IFGR0_IFG16                        ((uint32_t)0x00010000)          /*!< ADC14MEM16 interrupt flag */
/* ADC14_IFGR0[IFG17] Bits */
#define ADC14_IFGR0_IFG17_OFS                    (17)                            /*!< ADC14IFG17 Bit Offset */
#define ADC14_IFGR0_IFG17                        ((uint32_t)0x00020000)          /*!< ADC14MEM17 interrupt flag */
/* ADC14_IFGR0[IFG18] Bits */
#define ADC14_IFGR0_IFG18_OFS                    (18)                            /*!< ADC14IFG18 Bit Offset */
#define ADC14_IFGR0_IFG18                        ((uint32_t)0x00040000)          /*!< ADC14MEM18 interrupt flag */
/* ADC14_IFGR0[IFG19] Bits */
#define ADC14_IFGR0_IFG19_OFS                    (19)                            /*!< ADC14IFG19 Bit Offset */
#define ADC14_IFGR0_IFG19                        ((uint32_t)0x00080000)          /*!< ADC14MEM19 interrupt flag */
/* ADC14_IFGR0[IFG20] Bits */
#define ADC14_IFGR0_IFG20_OFS                    (20)                            /*!< ADC14IFG20 Bit Offset */
#define ADC14_IFGR0_IFG20                        ((uint32_t)0x00100000)          /*!< ADC14MEM20 interrupt flag */
/* ADC14_IFGR0[IFG21] Bits */
#define ADC14_IFGR0_IFG21_OFS                    (21)                            /*!< ADC14IFG21 Bit Offset */
#define ADC14_IFGR0_IFG21                        ((uint32_t)0x00200000)          /*!< ADC14MEM21 interrupt flag */
/* ADC14_IFGR0[IFG22] Bits */
#define ADC14_IFGR0_IFG22_OFS                    (22)                            /*!< ADC14IFG22 Bit Offset */
#define ADC14_IFGR0_IFG22                        ((uint32_t)0x00400000)          /*!< ADC14MEM22 interrupt flag */
/* ADC14_IFGR0[IFG23] Bits */
#define ADC14_IFGR0_IFG23_OFS                    (23)                            /*!< ADC14IFG23 Bit Offset */
#define ADC14_IFGR0_IFG23                        ((uint32_t)0x00800000)          /*!< ADC14MEM23 interrupt flag */
/* ADC14_IFGR0[IFG24] Bits */
#define ADC14_IFGR0_IFG24_OFS                    (24)                            /*!< ADC14IFG24 Bit Offset */
#define ADC14_IFGR0_IFG24                        ((uint32_t)0x01000000)          /*!< ADC14MEM24 interrupt flag */
/* ADC14_IFGR0[IFG25] Bits */
#define ADC14_IFGR0_IFG25_OFS                    (25)                            /*!< ADC14IFG25 Bit Offset */
#define ADC14_IFGR0_IFG25                        ((uint32_t)0x02000000)          /*!< ADC14MEM25 interrupt flag */
/* ADC14_IFGR0[IFG26] Bits */
#define ADC14_IFGR0_IFG26_OFS                    (26)                            /*!< ADC14IFG26 Bit Offset */
#define ADC14_IFGR0_IFG26                        ((uint32_t)0x04000000)          /*!< ADC14MEM26 interrupt flag */
/* ADC14_IFGR0[IFG27] Bits */
#define ADC14_IFGR0_IFG27_OFS                    (27)                            /*!< ADC14IFG27 Bit Offset */
#define ADC14_IFGR0_IFG27                        ((uint32_t)0x08000000)          /*!< ADC14MEM27 interrupt flag */
/* ADC14_IFGR0[IFG28] Bits */
#define ADC14_IFGR0_IFG28_OFS                    (28)                            /*!< ADC14IFG28 Bit Offset */
#define ADC14_IFGR0_IFG28                        ((uint32_t)0x10000000)          /*!< ADC14MEM28 interrupt flag */
/* ADC14_IFGR0[IFG29] Bits */
#define ADC14_IFGR0_IFG29_OFS                    (29)                            /*!< ADC14IFG29 Bit Offset */
#define ADC14_IFGR0_IFG29                        ((uint32_t)0x20000000)          /*!< ADC14MEM29 interrupt flag */
/* ADC14_IFGR0[IFG30] Bits */
#define ADC14_IFGR0_IFG30_OFS                    (30)                            /*!< ADC14IFG30 Bit Offset */
#define ADC14_IFGR0_IFG30                        ((uint32_t)0x40000000)          /*!< ADC14MEM30 interrupt flag */
/* ADC14_IFGR0[IFG31] Bits */
#define ADC14_IFGR0_IFG31_OFS                    (31)                            /*!< ADC14IFG31 Bit Offset */
#define ADC14_IFGR0_IFG31                        ((uint32_t)0x80000000)          /*!< ADC14MEM31 interrupt flag */
/* ADC14_IFGR1[INIFG] Bits */
#define ADC14_IFGR1_INIFG_OFS                    ( 1)                            /*!< ADC14INIFG Bit Offset */
#define ADC14_IFGR1_INIFG                        ((uint32_t)0x00000002)          /*!< Interrupt flag for ADC14MEMx within comparator window */
/* ADC14_IFGR1[LOIFG] Bits */
#define ADC14_IFGR1_LOIFG_OFS                    ( 2)                            /*!< ADC14LOIFG Bit Offset */
#define ADC14_IFGR1_LOIFG                        ((uint32_t)0x00000004)          /*!< Interrupt flag for ADC14MEMx below comparator window */
/* ADC14_IFGR1[HIIFG] Bits */
#define ADC14_IFGR1_HIIFG_OFS                    ( 3)                            /*!< ADC14HIIFG Bit Offset */
#define ADC14_IFGR1_HIIFG                        ((uint32_t)0x00000008)          /*!< Interrupt flag for ADC14MEMx above comparator window */
/* ADC14_IFGR1[OVIFG] Bits */
#define ADC14_IFGR1_OVIFG_OFS                    ( 4)                            /*!< ADC14OVIFG Bit Offset */
#define ADC14_IFGR1_OVIFG                        ((uint32_t)0x00000010)          /*!< ADC14MEMx overflow interrupt flag */
/* ADC14_IFGR1[TOVIFG] Bits */
#define ADC14_IFGR1_TOVIFG_OFS                   ( 5)                            /*!< ADC14TOVIFG Bit Offset */
#define ADC14_IFGR1_TOVIFG                       ((uint32_t)0x00000020)          /*!< ADC14 conversion time overflow interrupt flag */
/* ADC14_IFGR1[RDYIFG] Bits */
#define ADC14_IFGR1_RDYIFG_OFS                   ( 6)                            /*!< ADC14RDYIFG Bit Offset */
#define ADC14_IFGR1_RDYIFG                       ((uint32_t)0x00000040)          /*!< ADC14 local buffered reference ready interrupt flag */
/* ADC14_CLRIFGR0[CLRIFG0] Bits */
#define ADC14_CLRIFGR0_CLRIFG0_OFS               ( 0)                            /*!< CLRADC14IFG0 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG0                   ((uint32_t)0x00000001)          /*!< clear ADC14IFG0 */
/* ADC14_CLRIFGR0[CLRIFG1] Bits */
#define ADC14_CLRIFGR0_CLRIFG1_OFS               ( 1)                            /*!< CLRADC14IFG1 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG1                   ((uint32_t)0x00000002)          /*!< clear ADC14IFG1 */
/* ADC14_CLRIFGR0[CLRIFG2] Bits */
#define ADC14_CLRIFGR0_CLRIFG2_OFS               ( 2)                            /*!< CLRADC14IFG2 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG2                   ((uint32_t)0x00000004)          /*!< clear ADC14IFG2 */
/* ADC14_CLRIFGR0[CLRIFG3] Bits */
#define ADC14_CLRIFGR0_CLRIFG3_OFS               ( 3)                            /*!< CLRADC14IFG3 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG3                   ((uint32_t)0x00000008)          /*!< clear ADC14IFG3 */
/* ADC14_CLRIFGR0[CLRIFG4] Bits */
#define ADC14_CLRIFGR0_CLRIFG4_OFS               ( 4)                            /*!< CLRADC14IFG4 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG4                   ((uint32_t)0x00000010)          /*!< clear ADC14IFG4 */
/* ADC14_CLRIFGR0[CLRIFG5] Bits */
#define ADC14_CLRIFGR0_CLRIFG5_OFS               ( 5)                            /*!< CLRADC14IFG5 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG5                   ((uint32_t)0x00000020)          /*!< clear ADC14IFG5 */
/* ADC14_CLRIFGR0[CLRIFG6] Bits */
#define ADC14_CLRIFGR0_CLRIFG6_OFS               ( 6)                            /*!< CLRADC14IFG6 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG6                   ((uint32_t)0x00000040)          /*!< clear ADC14IFG6 */
/* ADC14_CLRIFGR0[CLRIFG7] Bits */
#define ADC14_CLRIFGR0_CLRIFG7_OFS               ( 7)                            /*!< CLRADC14IFG7 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG7                   ((uint32_t)0x00000080)          /*!< clear ADC14IFG7 */
/* ADC14_CLRIFGR0[CLRIFG8] Bits */
#define ADC14_CLRIFGR0_CLRIFG8_OFS               ( 8)                            /*!< CLRADC14IFG8 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG8                   ((uint32_t)0x00000100)          /*!< clear ADC14IFG8 */
/* ADC14_CLRIFGR0[CLRIFG9] Bits */
#define ADC14_CLRIFGR0_CLRIFG9_OFS               ( 9)                            /*!< CLRADC14IFG9 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG9                   ((uint32_t)0x00000200)          /*!< clear ADC14IFG9 */
/* ADC14_CLRIFGR0[CLRIFG10] Bits */
#define ADC14_CLRIFGR0_CLRIFG10_OFS              (10)                            /*!< CLRADC14IFG10 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG10                  ((uint32_t)0x00000400)          /*!< clear ADC14IFG10 */
/* ADC14_CLRIFGR0[CLRIFG11] Bits */
#define ADC14_CLRIFGR0_CLRIFG11_OFS              (11)                            /*!< CLRADC14IFG11 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG11                  ((uint32_t)0x00000800)          /*!< clear ADC14IFG11 */
/* ADC14_CLRIFGR0[CLRIFG12] Bits */
#define ADC14_CLRIFGR0_CLRIFG12_OFS              (12)                            /*!< CLRADC14IFG12 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG12                  ((uint32_t)0x00001000)          /*!< clear ADC14IFG12 */
/* ADC14_CLRIFGR0[CLRIFG13] Bits */
#define ADC14_CLRIFGR0_CLRIFG13_OFS              (13)                            /*!< CLRADC14IFG13 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG13                  ((uint32_t)0x00002000)          /*!< clear ADC14IFG13 */
/* ADC14_CLRIFGR0[CLRIFG14] Bits */
#define ADC14_CLRIFGR0_CLRIFG14_OFS              (14)                            /*!< CLRADC14IFG14 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG14                  ((uint32_t)0x00004000)          /*!< clear ADC14IFG14 */
/* ADC14_CLRIFGR0[CLRIFG15] Bits */
#define ADC14_CLRIFGR0_CLRIFG15_OFS              (15)                            /*!< CLRADC14IFG15 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG15                  ((uint32_t)0x00008000)          /*!< clear ADC14IFG15 */
/* ADC14_CLRIFGR0[CLRIFG16] Bits */
#define ADC14_CLRIFGR0_CLRIFG16_OFS              (16)                            /*!< CLRADC14IFG16 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG16                  ((uint32_t)0x00010000)          /*!< clear ADC14IFG16 */
/* ADC14_CLRIFGR0[CLRIFG17] Bits */
#define ADC14_CLRIFGR0_CLRIFG17_OFS              (17)                            /*!< CLRADC14IFG17 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG17                  ((uint32_t)0x00020000)          /*!< clear ADC14IFG17 */
/* ADC14_CLRIFGR0[CLRIFG18] Bits */
#define ADC14_CLRIFGR0_CLRIFG18_OFS              (18)                            /*!< CLRADC14IFG18 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG18                  ((uint32_t)0x00040000)          /*!< clear ADC14IFG18 */
/* ADC14_CLRIFGR0[CLRIFG19] Bits */
#define ADC14_CLRIFGR0_CLRIFG19_OFS              (19)                            /*!< CLRADC14IFG19 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG19                  ((uint32_t)0x00080000)          /*!< clear ADC14IFG19 */
/* ADC14_CLRIFGR0[CLRIFG20] Bits */
#define ADC14_CLRIFGR0_CLRIFG20_OFS              (20)                            /*!< CLRADC14IFG20 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG20                  ((uint32_t)0x00100000)          /*!< clear ADC14IFG20 */
/* ADC14_CLRIFGR0[CLRIFG21] Bits */
#define ADC14_CLRIFGR0_CLRIFG21_OFS              (21)                            /*!< CLRADC14IFG21 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG21                  ((uint32_t)0x00200000)          /*!< clear ADC14IFG21 */
/* ADC14_CLRIFGR0[CLRIFG22] Bits */
#define ADC14_CLRIFGR0_CLRIFG22_OFS              (22)                            /*!< CLRADC14IFG22 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG22                  ((uint32_t)0x00400000)          /*!< clear ADC14IFG22 */
/* ADC14_CLRIFGR0[CLRIFG23] Bits */
#define ADC14_CLRIFGR0_CLRIFG23_OFS              (23)                            /*!< CLRADC14IFG23 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG23                  ((uint32_t)0x00800000)          /*!< clear ADC14IFG23 */
/* ADC14_CLRIFGR0[CLRIFG24] Bits */
#define ADC14_CLRIFGR0_CLRIFG24_OFS              (24)                            /*!< CLRADC14IFG24 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG24                  ((uint32_t)0x01000000)          /*!< clear ADC14IFG24 */
/* ADC14_CLRIFGR0[CLRIFG25] Bits */
#define ADC14_CLRIFGR0_CLRIFG25_OFS              (25)                            /*!< CLRADC14IFG25 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG25                  ((uint32_t)0x02000000)          /*!< clear ADC14IFG25 */
/* ADC14_CLRIFGR0[CLRIFG26] Bits */
#define ADC14_CLRIFGR0_CLRIFG26_OFS              (26)                            /*!< CLRADC14IFG26 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG26                  ((uint32_t)0x04000000)          /*!< clear ADC14IFG26 */
/* ADC14_CLRIFGR0[CLRIFG27] Bits */
#define ADC14_CLRIFGR0_CLRIFG27_OFS              (27)                            /*!< CLRADC14IFG27 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG27                  ((uint32_t)0x08000000)          /*!< clear ADC14IFG27 */
/* ADC14_CLRIFGR0[CLRIFG28] Bits */
#define ADC14_CLRIFGR0_CLRIFG28_OFS              (28)                            /*!< CLRADC14IFG28 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG28                  ((uint32_t)0x10000000)          /*!< clear ADC14IFG28 */
/* ADC14_CLRIFGR0[CLRIFG29] Bits */
#define ADC14_CLRIFGR0_CLRIFG29_OFS              (29)                            /*!< CLRADC14IFG29 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG29                  ((uint32_t)0x20000000)          /*!< clear ADC14IFG29 */
/* ADC14_CLRIFGR0[CLRIFG30] Bits */
#define ADC14_CLRIFGR0_CLRIFG30_OFS              (30)                            /*!< CLRADC14IFG30 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG30                  ((uint32_t)0x40000000)          /*!< clear ADC14IFG30 */
/* ADC14_CLRIFGR0[CLRIFG31] Bits */
#define ADC14_CLRIFGR0_CLRIFG31_OFS              (31)                            /*!< CLRADC14IFG31 Bit Offset */
#define ADC14_CLRIFGR0_CLRIFG31                  ((uint32_t)0x80000000)          /*!< clear ADC14IFG31 */
/* ADC14_CLRIFGR1[CLRINIFG] Bits */
#define ADC14_CLRIFGR1_CLRINIFG_OFS              ( 1)                            /*!< CLRADC14INIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRINIFG                  ((uint32_t)0x00000002)          /*!< clear ADC14INIFG */
/* ADC14_CLRIFGR1[CLRLOIFG] Bits */
#define ADC14_CLRIFGR1_CLRLOIFG_OFS              ( 2)                            /*!< CLRADC14LOIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRLOIFG                  ((uint32_t)0x00000004)          /*!< clear ADC14LOIFG */
/* ADC14_CLRIFGR1[CLRHIIFG] Bits */
#define ADC14_CLRIFGR1_CLRHIIFG_OFS              ( 3)                            /*!< CLRADC14HIIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRHIIFG                  ((uint32_t)0x00000008)          /*!< clear ADC14HIIFG */
/* ADC14_CLRIFGR1[CLROVIFG] Bits */
#define ADC14_CLRIFGR1_CLROVIFG_OFS              ( 4)                            /*!< CLRADC14OVIFG Bit Offset */
#define ADC14_CLRIFGR1_CLROVIFG                  ((uint32_t)0x00000010)          /*!< clear ADC14OVIFG */
/* ADC14_CLRIFGR1[CLRTOVIFG] Bits */
#define ADC14_CLRIFGR1_CLRTOVIFG_OFS             ( 5)                            /*!< CLRADC14TOVIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRTOVIFG                 ((uint32_t)0x00000020)          /*!< clear ADC14TOVIFG */
/* ADC14_CLRIFGR1[CLRRDYIFG] Bits */
#define ADC14_CLRIFGR1_CLRRDYIFG_OFS             ( 6)                            /*!< CLRADC14RDYIFG Bit Offset */
#define ADC14_CLRIFGR1_CLRRDYIFG                 ((uint32_t)0x00000040)          /*!< clear ADC14RDYIFG */

/******************************************************************************
* AES256 Bits
******************************************************************************/
/* AES256_CTL0[OP] Bits */
#define AES256_CTL0_OP_OFS                       ( 0)                            /*!< AESOPx Bit Offset */
#define AES256_CTL0_OP_MASK                      ((uint16_t)0x0003)              /*!< AESOPx Bit Mask */
#define AES256_CTL0_OP0                          ((uint16_t)0x0001)              /*!< OP Bit 0 */
#define AES256_CTL0_OP1                          ((uint16_t)0x0002)              /*!< OP Bit 1 */
#define AES256_CTL0_OP_0                         ((uint16_t)0x0000)              /*!< Encryption */
#define AES256_CTL0_OP_1                         ((uint16_t)0x0001)              /*!< Decryption. The provided key is the same key used for encryption */
#define AES256_CTL0_OP_2                         ((uint16_t)0x0002)              /*!< Generate first round key required for decryption */
#define AES256_CTL0_OP_3                         ((uint16_t)0x0003)              /*!< Decryption. The provided key is the first round key required for decryption */
/* AES256_CTL0[KL] Bits */
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
/* AES256_CTL0[CM] Bits */
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
/* AES256_CTL0[SWRST] Bits */
#define AES256_CTL0_SWRST_OFS                    ( 7)                            /*!< AESSWRST Bit Offset */
#define AES256_CTL0_SWRST                        ((uint16_t)0x0080)              /*!< AES software reset */
/* AES256_CTL0[RDYIFG] Bits */
#define AES256_CTL0_RDYIFG_OFS                   ( 8)                            /*!< AESRDYIFG Bit Offset */
#define AES256_CTL0_RDYIFG                       ((uint16_t)0x0100)              /*!< AES ready interrupt flag */
/* AES256_CTL0[ERRFG] Bits */
#define AES256_CTL0_ERRFG_OFS                    (11)                            /*!< AESERRFG Bit Offset */
#define AES256_CTL0_ERRFG                        ((uint16_t)0x0800)              /*!< AES error flag */
/* AES256_CTL0[RDYIE] Bits */
#define AES256_CTL0_RDYIE_OFS                    (12)                            /*!< AESRDYIE Bit Offset */
#define AES256_CTL0_RDYIE                        ((uint16_t)0x1000)              /*!< AES ready interrupt enable */
/* AES256_CTL0[CMEN] Bits */
#define AES256_CTL0_CMEN_OFS                     (15)                            /*!< AESCMEN Bit Offset */
#define AES256_CTL0_CMEN                         ((uint16_t)0x8000)              /*!< AES cipher mode enable */
/* AES256_CTL1[BLKCNT] Bits */
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
/* AES256_STAT[BUSY] Bits */
#define AES256_STAT_BUSY_OFS                     ( 0)                            /*!< AESBUSY Bit Offset */
#define AES256_STAT_BUSY                         ((uint16_t)0x0001)              /*!< AES accelerator module busy */
/* AES256_STAT[KEYWR] Bits */
#define AES256_STAT_KEYWR_OFS                    ( 1)                            /*!< AESKEYWR Bit Offset */
#define AES256_STAT_KEYWR                        ((uint16_t)0x0002)              /*!< All 16 bytes written to AESAKEY */
/* AES256_STAT[DINWR] Bits */
#define AES256_STAT_DINWR_OFS                    ( 2)                            /*!< AESDINWR Bit Offset */
#define AES256_STAT_DINWR                        ((uint16_t)0x0004)              /*!< All 16 bytes written to AESADIN, AESAXDIN or AESAXIN */
/* AES256_STAT[DOUTRD] Bits */
#define AES256_STAT_DOUTRD_OFS                   ( 3)                            /*!< AESDOUTRD Bit Offset */
#define AES256_STAT_DOUTRD                       ((uint16_t)0x0008)              /*!< All 16 bytes read from AESADOUT */
/* AES256_STAT[KEYCNT] Bits */
#define AES256_STAT_KEYCNT_OFS                   ( 4)                            /*!< AESKEYCNTx Bit Offset */
#define AES256_STAT_KEYCNT_MASK                  ((uint16_t)0x00F0)              /*!< AESKEYCNTx Bit Mask */
#define AES256_STAT_KEYCNT0                      ((uint16_t)0x0010)              /*!< KEYCNT Bit 0 */
#define AES256_STAT_KEYCNT1                      ((uint16_t)0x0020)              /*!< KEYCNT Bit 1 */
#define AES256_STAT_KEYCNT2                      ((uint16_t)0x0040)              /*!< KEYCNT Bit 2 */
#define AES256_STAT_KEYCNT3                      ((uint16_t)0x0080)              /*!< KEYCNT Bit 3 */
/* AES256_STAT[DINCNT] Bits */
#define AES256_STAT_DINCNT_OFS                   ( 8)                            /*!< AESDINCNTx Bit Offset */
#define AES256_STAT_DINCNT_MASK                  ((uint16_t)0x0F00)              /*!< AESDINCNTx Bit Mask */
#define AES256_STAT_DINCNT0                      ((uint16_t)0x0100)              /*!< DINCNT Bit 0 */
#define AES256_STAT_DINCNT1                      ((uint16_t)0x0200)              /*!< DINCNT Bit 1 */
#define AES256_STAT_DINCNT2                      ((uint16_t)0x0400)              /*!< DINCNT Bit 2 */
#define AES256_STAT_DINCNT3                      ((uint16_t)0x0800)              /*!< DINCNT Bit 3 */
/* AES256_STAT[DOUTCNT] Bits */
#define AES256_STAT_DOUTCNT_OFS                  (12)                            /*!< AESDOUTCNTx Bit Offset */
#define AES256_STAT_DOUTCNT_MASK                 ((uint16_t)0xF000)              /*!< AESDOUTCNTx Bit Mask */
#define AES256_STAT_DOUTCNT0                     ((uint16_t)0x1000)              /*!< DOUTCNT Bit 0 */
#define AES256_STAT_DOUTCNT1                     ((uint16_t)0x2000)              /*!< DOUTCNT Bit 1 */
#define AES256_STAT_DOUTCNT2                     ((uint16_t)0x4000)              /*!< DOUTCNT Bit 2 */
#define AES256_STAT_DOUTCNT3                     ((uint16_t)0x8000)              /*!< DOUTCNT Bit 3 */
/* AES256_KEY[KEY0] Bits */
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
/* AES256_KEY[KEY1] Bits */
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
/* AES256_DIN[DIN0] Bits */
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
/* AES256_DIN[DIN1] Bits */
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
/* AES256_DOUT[DOUT0] Bits */
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
/* AES256_DOUT[DOUT1] Bits */
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
/* AES256_XDIN[XDIN0] Bits */
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
/* AES256_XDIN[XDIN1] Bits */
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
/* AES256_XIN[XIN0] Bits */
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
/* AES256_XIN[XIN1] Bits */
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

/******************************************************************************
* CAPTIO Bits
******************************************************************************/
/* CAPTIO_CTL[PISEL] Bits */
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
/* CAPTIO_CTL[POSEL] Bits */
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
/* CAPTIO_CTL[EN] Bits */
#define CAPTIO_CTL_EN_OFS                        ( 8)                            /*!< CAPTIOEN Bit Offset */
#define CAPTIO_CTL_EN                            ((uint16_t)0x0100)              /*!< Capacitive Touch IO enable */
/* CAPTIO_CTL[STATE] Bits */
#define CAPTIO_CTL_STATE_OFS                     ( 9)                            /*!< CAPTIOSTATE Bit Offset */
#define CAPTIO_CTL_STATE                         ((uint16_t)0x0200)              /*!< Capacitive Touch IO state */

/******************************************************************************
* COMP_E Bits
******************************************************************************/
/* COMP_E_CTL0[IPSEL] Bits */
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
/* COMP_E_CTL0[IPEN] Bits */
#define COMP_E_CTL0_IPEN_OFS                     ( 7)                            /*!< CEIPEN Bit Offset */
#define COMP_E_CTL0_IPEN                         ((uint16_t)0x0080)              /*!< Channel input enable for the V+ terminal */
/* COMP_E_CTL0[IMSEL] Bits */
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
/* COMP_E_CTL0[IMEN] Bits */
#define COMP_E_CTL0_IMEN_OFS                     (15)                            /*!< CEIMEN Bit Offset */
#define COMP_E_CTL0_IMEN                         ((uint16_t)0x8000)              /*!< Channel input enable for the - terminal */
/* COMP_E_CTL1[OUT] Bits */
#define COMP_E_CTL1_OUT_OFS                      ( 0)                            /*!< CEOUT Bit Offset */
#define COMP_E_CTL1_OUT                          ((uint16_t)0x0001)              /*!< Comparator output value */
/* COMP_E_CTL1[OUTPOL] Bits */
#define COMP_E_CTL1_OUTPOL_OFS                   ( 1)                            /*!< CEOUTPOL Bit Offset */
#define COMP_E_CTL1_OUTPOL                       ((uint16_t)0x0002)              /*!< Comparator output polarity */
/* COMP_E_CTL1[F] Bits */
#define COMP_E_CTL1_F_OFS                        ( 2)                            /*!< CEF Bit Offset */
#define COMP_E_CTL1_F                            ((uint16_t)0x0004)              /*!< Comparator output filter */
/* COMP_E_CTL1[IES] Bits */
#define COMP_E_CTL1_IES_OFS                      ( 3)                            /*!< CEIES Bit Offset */
#define COMP_E_CTL1_IES                          ((uint16_t)0x0008)              /*!< Interrupt edge select for CEIIFG and CEIFG */
/* COMP_E_CTL1[SHORT] Bits */
#define COMP_E_CTL1_SHORT_OFS                    ( 4)                            /*!< CESHORT Bit Offset */
#define COMP_E_CTL1_SHORT                        ((uint16_t)0x0010)              /*!< Input short */
/* COMP_E_CTL1[EX] Bits */
#define COMP_E_CTL1_EX_OFS                       ( 5)                            /*!< CEEX Bit Offset */
#define COMP_E_CTL1_EX                           ((uint16_t)0x0020)              /*!< Exchange */
/* COMP_E_CTL1[FDLY] Bits */
#define COMP_E_CTL1_FDLY_OFS                     ( 6)                            /*!< CEFDLY Bit Offset */
#define COMP_E_CTL1_FDLY_MASK                    ((uint16_t)0x00C0)              /*!< CEFDLY Bit Mask */
#define COMP_E_CTL1_FDLY0                        ((uint16_t)0x0040)              /*!< FDLY Bit 0 */
#define COMP_E_CTL1_FDLY1                        ((uint16_t)0x0080)              /*!< FDLY Bit 1 */
#define COMP_E_CTL1_FDLY_0                       ((uint16_t)0x0000)              /*!< Typical filter delay of TBD (450) ns */
#define COMP_E_CTL1_FDLY_1                       ((uint16_t)0x0040)              /*!< Typical filter delay of TBD (900) ns */
#define COMP_E_CTL1_FDLY_2                       ((uint16_t)0x0080)              /*!< Typical filter delay of TBD (1800) ns */
#define COMP_E_CTL1_FDLY_3                       ((uint16_t)0x00C0)              /*!< Typical filter delay of TBD (3600) ns */
/* COMP_E_CTL1[PWRMD] Bits */
#define COMP_E_CTL1_PWRMD_OFS                    ( 8)                            /*!< CEPWRMD Bit Offset */
#define COMP_E_CTL1_PWRMD_MASK                   ((uint16_t)0x0300)              /*!< CEPWRMD Bit Mask */
#define COMP_E_CTL1_PWRMD0                       ((uint16_t)0x0100)              /*!< PWRMD Bit 0 */
#define COMP_E_CTL1_PWRMD1                       ((uint16_t)0x0200)              /*!< PWRMD Bit 1 */
#define COMP_E_CTL1_PWRMD_0                      ((uint16_t)0x0000)              /*!< High-speed mode */
#define COMP_E_CTL1_PWRMD_1                      ((uint16_t)0x0100)              /*!< Normal mode */
#define COMP_E_CTL1_PWRMD_2                      ((uint16_t)0x0200)              /*!< Ultra-low power mode */
/* COMP_E_CTL1[ON] Bits */
#define COMP_E_CTL1_ON_OFS                       (10)                            /*!< CEON Bit Offset */
#define COMP_E_CTL1_ON                           ((uint16_t)0x0400)              /*!< Comparator On */
/* COMP_E_CTL1[MRVL] Bits */
#define COMP_E_CTL1_MRVL_OFS                     (11)                            /*!< CEMRVL Bit Offset */
#define COMP_E_CTL1_MRVL                         ((uint16_t)0x0800)              /*!< This bit is valid of CEMRVS is set to 1 */
/* COMP_E_CTL1[MRVS] Bits */
#define COMP_E_CTL1_MRVS_OFS                     (12)                            /*!< CEMRVS Bit Offset */
#define COMP_E_CTL1_MRVS                         ((uint16_t)0x1000)              
/* COMP_E_CTL2[REF0] Bits */
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
/* COMP_E_CTL2[RSEL] Bits */
#define COMP_E_CTL2_RSEL_OFS                     ( 5)                            /*!< CERSEL Bit Offset */
#define COMP_E_CTL2_RSEL                         ((uint16_t)0x0020)              /*!< Reference select */
/* COMP_E_CTL2[RS] Bits */
#define COMP_E_CTL2_RS_OFS                       ( 6)                            /*!< CERS Bit Offset */
#define COMP_E_CTL2_RS_MASK                      ((uint16_t)0x00C0)              /*!< CERS Bit Mask */
#define COMP_E_CTL2_RS0                          ((uint16_t)0x0040)              /*!< RS Bit 0 */
#define COMP_E_CTL2_RS1                          ((uint16_t)0x0080)              /*!< RS Bit 1 */
#define COMP_E_CTL2_RS_0                         ((uint16_t)0x0000)              /*!< No current is drawn by the reference circuitry */
#define COMP_E_CTL2_RS_1                         ((uint16_t)0x0040)              /*!< VCC applied to the resistor ladder */
#define COMP_E_CTL2_RS_2                         ((uint16_t)0x0080)              /*!< Shared reference voltage applied to the resistor ladder */
#define COMP_E_CTL2_RS_3                         ((uint16_t)0x00C0)              /*!< Shared reference voltage supplied to V(CREF). Resistor ladder is off */
/* COMP_E_CTL2[REF1] Bits */
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
/* COMP_E_CTL2[REFL] Bits */
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
/* COMP_E_CTL2[REFACC] Bits */
#define COMP_E_CTL2_REFACC_OFS                   (15)                            /*!< CEREFACC Bit Offset */
#define COMP_E_CTL2_REFACC                       ((uint16_t)0x8000)              /*!< Reference accuracy */
/* COMP_E_CTL3[PD0] Bits */
#define COMP_E_CTL3_PD0_OFS                      ( 0)                            /*!< CEPD0 Bit Offset */
#define COMP_E_CTL3_PD0                          ((uint16_t)0x0001)              /*!< Port disable */
/* COMP_E_CTL3[PD1] Bits */
#define COMP_E_CTL3_PD1_OFS                      ( 1)                            /*!< CEPD1 Bit Offset */
#define COMP_E_CTL3_PD1                          ((uint16_t)0x0002)              /*!< Port disable */
/* COMP_E_CTL3[PD2] Bits */
#define COMP_E_CTL3_PD2_OFS                      ( 2)                            /*!< CEPD2 Bit Offset */
#define COMP_E_CTL3_PD2                          ((uint16_t)0x0004)              /*!< Port disable */
/* COMP_E_CTL3[PD3] Bits */
#define COMP_E_CTL3_PD3_OFS                      ( 3)                            /*!< CEPD3 Bit Offset */
#define COMP_E_CTL3_PD3                          ((uint16_t)0x0008)              /*!< Port disable */
/* COMP_E_CTL3[PD4] Bits */
#define COMP_E_CTL3_PD4_OFS                      ( 4)                            /*!< CEPD4 Bit Offset */
#define COMP_E_CTL3_PD4                          ((uint16_t)0x0010)              /*!< Port disable */
/* COMP_E_CTL3[PD5] Bits */
#define COMP_E_CTL3_PD5_OFS                      ( 5)                            /*!< CEPD5 Bit Offset */
#define COMP_E_CTL3_PD5                          ((uint16_t)0x0020)              /*!< Port disable */
/* COMP_E_CTL3[PD6] Bits */
#define COMP_E_CTL3_PD6_OFS                      ( 6)                            /*!< CEPD6 Bit Offset */
#define COMP_E_CTL3_PD6                          ((uint16_t)0x0040)              /*!< Port disable */
/* COMP_E_CTL3[PD7] Bits */
#define COMP_E_CTL3_PD7_OFS                      ( 7)                            /*!< CEPD7 Bit Offset */
#define COMP_E_CTL3_PD7                          ((uint16_t)0x0080)              /*!< Port disable */
/* COMP_E_CTL3[PD8] Bits */
#define COMP_E_CTL3_PD8_OFS                      ( 8)                            /*!< CEPD8 Bit Offset */
#define COMP_E_CTL3_PD8                          ((uint16_t)0x0100)              /*!< Port disable */
/* COMP_E_CTL3[PD9] Bits */
#define COMP_E_CTL3_PD9_OFS                      ( 9)                            /*!< CEPD9 Bit Offset */
#define COMP_E_CTL3_PD9                          ((uint16_t)0x0200)              /*!< Port disable */
/* COMP_E_CTL3[PD10] Bits */
#define COMP_E_CTL3_PD10_OFS                     (10)                            /*!< CEPD10 Bit Offset */
#define COMP_E_CTL3_PD10                         ((uint16_t)0x0400)              /*!< Port disable */
/* COMP_E_CTL3[PD11] Bits */
#define COMP_E_CTL3_PD11_OFS                     (11)                            /*!< CEPD11 Bit Offset */
#define COMP_E_CTL3_PD11                         ((uint16_t)0x0800)              /*!< Port disable */
/* COMP_E_CTL3[PD12] Bits */
#define COMP_E_CTL3_PD12_OFS                     (12)                            /*!< CEPD12 Bit Offset */
#define COMP_E_CTL3_PD12                         ((uint16_t)0x1000)              /*!< Port disable */
/* COMP_E_CTL3[PD13] Bits */
#define COMP_E_CTL3_PD13_OFS                     (13)                            /*!< CEPD13 Bit Offset */
#define COMP_E_CTL3_PD13                         ((uint16_t)0x2000)              /*!< Port disable */
/* COMP_E_CTL3[PD14] Bits */
#define COMP_E_CTL3_PD14_OFS                     (14)                            /*!< CEPD14 Bit Offset */
#define COMP_E_CTL3_PD14                         ((uint16_t)0x4000)              /*!< Port disable */
/* COMP_E_CTL3[PD15] Bits */
#define COMP_E_CTL3_PD15_OFS                     (15)                            /*!< CEPD15 Bit Offset */
#define COMP_E_CTL3_PD15                         ((uint16_t)0x8000)              /*!< Port disable */
/* COMP_E_INT[IFG] Bits */
#define COMP_E_INT_IFG_OFS                       ( 0)                            /*!< CEIFG Bit Offset */
#define COMP_E_INT_IFG                           ((uint16_t)0x0001)              /*!< Comparator output interrupt flag */
/* COMP_E_INT[IIFG] Bits */
#define COMP_E_INT_IIFG_OFS                      ( 1)                            /*!< CEIIFG Bit Offset */
#define COMP_E_INT_IIFG                          ((uint16_t)0x0002)              /*!< Comparator output inverted interrupt flag */
/* COMP_E_INT[RDYIFG] Bits */
#define COMP_E_INT_RDYIFG_OFS                    ( 4)                            /*!< CERDYIFG Bit Offset */
#define COMP_E_INT_RDYIFG                        ((uint16_t)0x0010)              /*!< Comparator ready interrupt flag */
/* COMP_E_INT[IE] Bits */
#define COMP_E_INT_IE_OFS                        ( 8)                            /*!< CEIE Bit Offset */
#define COMP_E_INT_IE                            ((uint16_t)0x0100)              /*!< Comparator output interrupt enable */
/* COMP_E_INT[IIE] Bits */
#define COMP_E_INT_IIE_OFS                       ( 9)                            /*!< CEIIE Bit Offset */
#define COMP_E_INT_IIE                           ((uint16_t)0x0200)              /*!< Comparator output interrupt enable inverted polarity */
/* COMP_E_INT[RDYIE] Bits */
#define COMP_E_INT_RDYIE_OFS                     (12)                            /*!< CERDYIE Bit Offset */
#define COMP_E_INT_RDYIE                         ((uint16_t)0x1000)              /*!< Comparator ready interrupt enable */

/******************************************************************************
* COREDEBUG Bits
******************************************************************************/


/******************************************************************************
* CRC32 Bits
******************************************************************************/

/******************************************************************************
* CS Bits
******************************************************************************/
/* CS_KEY[KEY] Bits */
#define CS_KEY_KEY_OFS                           ( 0)                            /*!< CSKEY Bit Offset */
#define CS_KEY_KEY_MASK                          ((uint32_t)0x0000FFFF)          /*!< CSKEY Bit Mask */
/* CS_CTL0[DCOTUNE] Bits */
#define CS_CTL0_DCOTUNE_OFS                      ( 0)                            /*!< DCOTUNE Bit Offset */
#define CS_CTL0_DCOTUNE_MASK                     ((uint32_t)0x000003FF)          /*!< DCOTUNE Bit Mask */
/* CS_CTL0[DCORSEL] Bits */
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
/* CS_CTL0[DCORES] Bits */
#define CS_CTL0_DCORES_OFS                       (22)                            /*!< DCORES Bit Offset */
#define CS_CTL0_DCORES                           ((uint32_t)0x00400000)          /*!< Enables the DCO external resistor mode */
/* CS_CTL0[DCOEN] Bits */
#define CS_CTL0_DCOEN_OFS                        (23)                            /*!< DCOEN Bit Offset */
#define CS_CTL0_DCOEN                            ((uint32_t)0x00800000)          /*!< Enables the DCO oscillator */
/* CS_CTL1[SELM] Bits */
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
/* CS_CTL1[SELS] Bits */
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
/* CS_CTL1[SELA] Bits */
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
/* CS_CTL1[SELB] Bits */
#define CS_CTL1_SELB_OFS                         (12)                            /*!< SELB Bit Offset */
#define CS_CTL1_SELB                             ((uint32_t)0x00001000)          /*!< Selects the BCLK source */
/* CS_CTL1[DIVM] Bits */
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
/* CS_CTL1[DIVHS] Bits */
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
/* CS_CTL1[DIVA] Bits */
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
/* CS_CTL1[DIVS] Bits */
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
/* CS_CTL2[LFXTDRIVE] Bits */
#define CS_CTL2_LFXTDRIVE_OFS                    ( 0)                            /*!< LFXTDRIVE Bit Offset */
#define CS_CTL2_LFXTDRIVE_MASK                   ((uint32_t)0x00000003)          /*!< LFXTDRIVE Bit Mask */
#define CS_CTL2_LFXTDRIVE0                       ((uint32_t)0x00000001)          /*!< LFXTDRIVE Bit 0 */
#define CS_CTL2_LFXTDRIVE1                       ((uint32_t)0x00000002)          /*!< LFXTDRIVE Bit 1 */
#define CS_CTL2_LFXTDRIVE_0                      ((uint32_t)0x00000000)          /*!< Lowest drive strength and current consumption LFXT oscillator. */
#define CS_CTL2_LFXTDRIVE_1                      ((uint32_t)0x00000001)          /*!< Increased drive strength LFXT oscillator. */
#define CS_CTL2_LFXTDRIVE_2                      ((uint32_t)0x00000002)          /*!< Increased drive strength LFXT oscillator. */
#define CS_CTL2_LFXTDRIVE_3                      ((uint32_t)0x00000003)          /*!< Maximum drive strength and maximum current consumption LFXT oscillator. */
/* CS_CTL2[LFXT_EN] Bits */
#define CS_CTL2_LFXT_EN_OFS                      ( 8)                            /*!< LFXT_EN Bit Offset */
#define CS_CTL2_LFXT_EN                          ((uint32_t)0x00000100)          /*!< Turns on the LFXT oscillator regardless if used as a clock resource */
/* CS_CTL2[LFXTBYPASS] Bits */
#define CS_CTL2_LFXTBYPASS_OFS                   ( 9)                            /*!< LFXTBYPASS Bit Offset */
#define CS_CTL2_LFXTBYPASS                       ((uint32_t)0x00000200)          /*!< LFXT bypass select */
/* CS_CTL2[HFXTDRIVE] Bits */
#define CS_CTL2_HFXTDRIVE_OFS                    (16)                            /*!< HFXTDRIVE Bit Offset */
#define CS_CTL2_HFXTDRIVE                        ((uint32_t)0x00010000)          /*!< HFXT oscillator drive selection */
/* CS_CTL2[HFXTFREQ] Bits */
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
/* CS_CTL2[HFXT_EN] Bits */
#define CS_CTL2_HFXT_EN_OFS                      (24)                            /*!< HFXT_EN Bit Offset */
#define CS_CTL2_HFXT_EN                          ((uint32_t)0x01000000)          /*!< Turns on the HFXT oscillator regardless if used as a clock resource */
/* CS_CTL2[HFXTBYPASS] Bits */
#define CS_CTL2_HFXTBYPASS_OFS                   (25)                            /*!< HFXTBYPASS Bit Offset */
#define CS_CTL2_HFXTBYPASS                       ((uint32_t)0x02000000)          /*!< HFXT bypass select */
/* CS_CTL3[FCNTLF] Bits */
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
/* CS_CTL3[RFCNTLF] Bits */
#define CS_CTL3_RFCNTLF_OFS                      ( 2)                            /*!< RFCNTLF Bit Offset */
#define CS_CTL3_RFCNTLF                          ((uint32_t)0x00000004)          /*!< Reset start fault counter for LFXT */
/* CS_CTL3[FCNTLF_EN] Bits */
#define CS_CTL3_FCNTLF_EN_OFS                    ( 3)                            /*!< FCNTLF_EN Bit Offset */
#define CS_CTL3_FCNTLF_EN                        ((uint32_t)0x00000008)          /*!< Enable start fault counter for LFXT */
/* CS_CTL3[FCNTHF] Bits */
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
/* CS_CTL3[RFCNTHF] Bits */
#define CS_CTL3_RFCNTHF_OFS                      ( 6)                            /*!< RFCNTHF Bit Offset */
#define CS_CTL3_RFCNTHF                          ((uint32_t)0x00000040)          /*!< Reset start fault counter for HFXT */
/* CS_CTL3[FCNTHF_EN] Bits */
#define CS_CTL3_FCNTHF_EN_OFS                    ( 7)                            /*!< FCNTHF_EN Bit Offset */
#define CS_CTL3_FCNTHF_EN                        ((uint32_t)0x00000080)          /*!< Enable start fault counter for HFXT */
/* CS_CLKEN[ACLK_EN] Bits */
#define CS_CLKEN_ACLK_EN_OFS                     ( 0)                            /*!< ACLK_EN Bit Offset */
#define CS_CLKEN_ACLK_EN                         ((uint32_t)0x00000001)          /*!< ACLK system clock conditional request enable */
/* CS_CLKEN[MCLK_EN] Bits */
#define CS_CLKEN_MCLK_EN_OFS                     ( 1)                            /*!< MCLK_EN Bit Offset */
#define CS_CLKEN_MCLK_EN                         ((uint32_t)0x00000002)          /*!< MCLK system clock conditional request enable */
/* CS_CLKEN[HSMCLK_EN] Bits */
#define CS_CLKEN_HSMCLK_EN_OFS                   ( 2)                            /*!< HSMCLK_EN Bit Offset */
#define CS_CLKEN_HSMCLK_EN                       ((uint32_t)0x00000004)          /*!< HSMCLK system clock conditional request enable */
/* CS_CLKEN[SMCLK_EN] Bits */
#define CS_CLKEN_SMCLK_EN_OFS                    ( 3)                            /*!< SMCLK_EN Bit Offset */
#define CS_CLKEN_SMCLK_EN                        ((uint32_t)0x00000008)          /*!< SMCLK system clock conditional request enable */
/* CS_CLKEN[VLO_EN] Bits */
#define CS_CLKEN_VLO_EN_OFS                      ( 8)                            /*!< VLO_EN Bit Offset */
#define CS_CLKEN_VLO_EN                          ((uint32_t)0x00000100)          /*!< Turns on the VLO oscillator */
/* CS_CLKEN[REFO_EN] Bits */
#define CS_CLKEN_REFO_EN_OFS                     ( 9)                            /*!< REFO_EN Bit Offset */
#define CS_CLKEN_REFO_EN                         ((uint32_t)0x00000200)          /*!< Turns on the REFO oscillator */
/* CS_CLKEN[MODOSC_EN] Bits */
#define CS_CLKEN_MODOSC_EN_OFS                   (10)                            /*!< MODOSC_EN Bit Offset */
#define CS_CLKEN_MODOSC_EN                       ((uint32_t)0x00000400)          /*!< Turns on the MODOSC oscillator */
/* CS_CLKEN[REFOFSEL] Bits */
#define CS_CLKEN_REFOFSEL_OFS                    (15)                            /*!< REFOFSEL Bit Offset */
#define CS_CLKEN_REFOFSEL                        ((uint32_t)0x00008000)          /*!< Selects REFO nominal frequency */
/* CS_STAT[DCO_ON] Bits */
#define CS_STAT_DCO_ON_OFS                       ( 0)                            /*!< DCO_ON Bit Offset */
#define CS_STAT_DCO_ON                           ((uint32_t)0x00000001)          /*!< DCO status */
/* CS_STAT[DCOBIAS_ON] Bits */
#define CS_STAT_DCOBIAS_ON_OFS                   ( 1)                            /*!< DCOBIAS_ON Bit Offset */
#define CS_STAT_DCOBIAS_ON                       ((uint32_t)0x00000002)          /*!< DCO bias status */
/* CS_STAT[HFXT_ON] Bits */
#define CS_STAT_HFXT_ON_OFS                      ( 2)                            /*!< HFXT_ON Bit Offset */
#define CS_STAT_HFXT_ON                          ((uint32_t)0x00000004)          /*!< HFXT status */
/* CS_STAT[MODOSC_ON] Bits */
#define CS_STAT_MODOSC_ON_OFS                    ( 4)                            /*!< MODOSC_ON Bit Offset */
#define CS_STAT_MODOSC_ON                        ((uint32_t)0x00000010)          /*!< MODOSC status */
/* CS_STAT[VLO_ON] Bits */
#define CS_STAT_VLO_ON_OFS                       ( 5)                            /*!< VLO_ON Bit Offset */
#define CS_STAT_VLO_ON                           ((uint32_t)0x00000020)          /*!< VLO status */
/* CS_STAT[LFXT_ON] Bits */
#define CS_STAT_LFXT_ON_OFS                      ( 6)                            /*!< LFXT_ON Bit Offset */
#define CS_STAT_LFXT_ON                          ((uint32_t)0x00000040)          /*!< LFXT status */
/* CS_STAT[REFO_ON] Bits */
#define CS_STAT_REFO_ON_OFS                      ( 7)                            /*!< REFO_ON Bit Offset */
#define CS_STAT_REFO_ON                          ((uint32_t)0x00000080)          /*!< REFO status */
/* CS_STAT[ACLK_ON] Bits */
#define CS_STAT_ACLK_ON_OFS                      (16)                            /*!< ACLK_ON Bit Offset */
#define CS_STAT_ACLK_ON                          ((uint32_t)0x00010000)          /*!< ACLK system clock status */
/* CS_STAT[MCLK_ON] Bits */
#define CS_STAT_MCLK_ON_OFS                      (17)                            /*!< MCLK_ON Bit Offset */
#define CS_STAT_MCLK_ON                          ((uint32_t)0x00020000)          /*!< MCLK system clock status */
/* CS_STAT[HSMCLK_ON] Bits */
#define CS_STAT_HSMCLK_ON_OFS                    (18)                            /*!< HSMCLK_ON Bit Offset */
#define CS_STAT_HSMCLK_ON                        ((uint32_t)0x00040000)          /*!< HSMCLK system clock status */
/* CS_STAT[SMCLK_ON] Bits */
#define CS_STAT_SMCLK_ON_OFS                     (19)                            /*!< SMCLK_ON Bit Offset */
#define CS_STAT_SMCLK_ON                         ((uint32_t)0x00080000)          /*!< SMCLK system clock status */
/* CS_STAT[MODCLK_ON] Bits */
#define CS_STAT_MODCLK_ON_OFS                    (20)                            /*!< MODCLK_ON Bit Offset */
#define CS_STAT_MODCLK_ON                        ((uint32_t)0x00100000)          /*!< MODCLK system clock status */
/* CS_STAT[VLOCLK_ON] Bits */
#define CS_STAT_VLOCLK_ON_OFS                    (21)                            /*!< VLOCLK_ON Bit Offset */
#define CS_STAT_VLOCLK_ON                        ((uint32_t)0x00200000)          /*!< VLOCLK system clock status */
/* CS_STAT[LFXTCLK_ON] Bits */
#define CS_STAT_LFXTCLK_ON_OFS                   (22)                            /*!< LFXTCLK_ON Bit Offset */
#define CS_STAT_LFXTCLK_ON                       ((uint32_t)0x00400000)          /*!< LFXTCLK system clock status */
/* CS_STAT[REFOCLK_ON] Bits */
#define CS_STAT_REFOCLK_ON_OFS                   (23)                            /*!< REFOCLK_ON Bit Offset */
#define CS_STAT_REFOCLK_ON                       ((uint32_t)0x00800000)          /*!< REFOCLK system clock status */
/* CS_STAT[ACLK_READY] Bits */
#define CS_STAT_ACLK_READY_OFS                   (24)                            /*!< ACLK_READY Bit Offset */
#define CS_STAT_ACLK_READY                       ((uint32_t)0x01000000)          /*!< ACLK Ready status */
/* CS_STAT[MCLK_READY] Bits */
#define CS_STAT_MCLK_READY_OFS                   (25)                            /*!< MCLK_READY Bit Offset */
#define CS_STAT_MCLK_READY                       ((uint32_t)0x02000000)          /*!< MCLK Ready status */
/* CS_STAT[HSMCLK_READY] Bits */
#define CS_STAT_HSMCLK_READY_OFS                 (26)                            /*!< HSMCLK_READY Bit Offset */
#define CS_STAT_HSMCLK_READY                     ((uint32_t)0x04000000)          /*!< HSMCLK Ready status */
/* CS_STAT[SMCLK_READY] Bits */
#define CS_STAT_SMCLK_READY_OFS                  (27)                            /*!< SMCLK_READY Bit Offset */
#define CS_STAT_SMCLK_READY                      ((uint32_t)0x08000000)          /*!< SMCLK Ready status */
/* CS_STAT[BCLK_READY] Bits */
#define CS_STAT_BCLK_READY_OFS                   (28)                            /*!< BCLK_READY Bit Offset */
#define CS_STAT_BCLK_READY                       ((uint32_t)0x10000000)          /*!< BCLK Ready status */
/* CS_IE[LFXTIE] Bits */
#define CS_IE_LFXTIE_OFS                         ( 0)                            /*!< LFXTIE Bit Offset */
#define CS_IE_LFXTIE                             ((uint32_t)0x00000001)          /*!< LFXT oscillator fault flag interrupt enable */
/* CS_IE[HFXTIE] Bits */
#define CS_IE_HFXTIE_OFS                         ( 1)                            /*!< HFXTIE Bit Offset */
#define CS_IE_HFXTIE                             ((uint32_t)0x00000002)          /*!< HFXT oscillator fault flag interrupt enable */
/* CS_IE[DCOR_OPNIE] Bits */
#define CS_IE_DCOR_OPNIE_OFS                     ( 6)                            /*!< DCOR_OPNIE Bit Offset */
#define CS_IE_DCOR_OPNIE                         ((uint32_t)0x00000040)          /*!< DCO external resistor open circuit fault flag interrupt enable. */
/* CS_IE[FCNTLFIE] Bits */
#define CS_IE_FCNTLFIE_OFS                       ( 8)                            /*!< FCNTLFIE Bit Offset */
#define CS_IE_FCNTLFIE                           ((uint32_t)0x00000100)          /*!< Start fault counter interrupt enable LFXT */
/* CS_IE[FCNTHFIE] Bits */
#define CS_IE_FCNTHFIE_OFS                       ( 9)                            /*!< FCNTHFIE Bit Offset */
#define CS_IE_FCNTHFIE                           ((uint32_t)0x00000200)          /*!< Start fault counter interrupt enable HFXT */
/* CS_IFG[LFXTIFG] Bits */
#define CS_IFG_LFXTIFG_OFS                       ( 0)                            /*!< LFXTIFG Bit Offset */
#define CS_IFG_LFXTIFG                           ((uint32_t)0x00000001)          /*!< LFXT oscillator fault flag */
/* CS_IFG[HFXTIFG] Bits */
#define CS_IFG_HFXTIFG_OFS                       ( 1)                            /*!< HFXTIFG Bit Offset */
#define CS_IFG_HFXTIFG                           ((uint32_t)0x00000002)          /*!< HFXT oscillator fault flag */
/* CS_IFG[DCOR_SHTIFG] Bits */
#define CS_IFG_DCOR_SHTIFG_OFS                   ( 5)                            /*!< DCOR_SHTIFG Bit Offset */
#define CS_IFG_DCOR_SHTIFG                       ((uint32_t)0x00000020)          /*!< DCO external resistor short circuit fault flag. */
/* CS_IFG[DCOR_OPNIFG] Bits */
#define CS_IFG_DCOR_OPNIFG_OFS                   ( 6)                            /*!< DCOR_OPNIFG Bit Offset */
#define CS_IFG_DCOR_OPNIFG                       ((uint32_t)0x00000040)          /*!< DCO external resistor open circuit fault flag. */
/* CS_IFG[FCNTLFIFG] Bits */
#define CS_IFG_FCNTLFIFG_OFS                     ( 8)                            /*!< FCNTLFIFG Bit Offset */
#define CS_IFG_FCNTLFIFG                         ((uint32_t)0x00000100)          /*!< Start fault counter interrupt flag LFXT */
/* CS_IFG[FCNTHFIFG] Bits */
#define CS_IFG_FCNTHFIFG_OFS                     ( 9)                            /*!< FCNTHFIFG Bit Offset */
#define CS_IFG_FCNTHFIFG                         ((uint32_t)0x00000200)          /*!< Start fault counter interrupt flag HFXT */
/* CS_CLRIFG[CLR_LFXTIFG] Bits */
#define CS_CLRIFG_CLR_LFXTIFG_OFS                ( 0)                            /*!< CLR_LFXTIFG Bit Offset */
#define CS_CLRIFG_CLR_LFXTIFG                    ((uint32_t)0x00000001)          /*!< Clear LFXT oscillator fault interrupt flag */
/* CS_CLRIFG[CLR_HFXTIFG] Bits */
#define CS_CLRIFG_CLR_HFXTIFG_OFS                ( 1)                            /*!< CLR_HFXTIFG Bit Offset */
#define CS_CLRIFG_CLR_HFXTIFG                    ((uint32_t)0x00000002)          /*!< Clear HFXT oscillator fault interrupt flag */
/* CS_CLRIFG[CLR_DCOR_OPNIFG] Bits */
#define CS_CLRIFG_CLR_DCOR_OPNIFG_OFS            ( 6)                            /*!< CLR_DCOR_OPNIFG Bit Offset */
#define CS_CLRIFG_CLR_DCOR_OPNIFG                ((uint32_t)0x00000040)          /*!< Clear DCO external resistor open circuit fault interrupt flag. */
/* CS_CLRIFG[CLR_FCNTLFIFG] Bits */
#define CS_CLRIFG_CLR_FCNTLFIFG_OFS              ( 8)                            /*!< CLR_FCNTLFIFG Bit Offset */
#define CS_CLRIFG_CLR_FCNTLFIFG                  ((uint32_t)0x00000100)          /*!< Start fault counter clear interrupt flag LFXT */
/* CS_CLRIFG[CLR_FCNTHFIFG] Bits */
#define CS_CLRIFG_CLR_FCNTHFIFG_OFS              ( 9)                            /*!< CLR_FCNTHFIFG Bit Offset */
#define CS_CLRIFG_CLR_FCNTHFIFG                  ((uint32_t)0x00000200)          /*!< Start fault counter clear interrupt flag HFXT */
/* CS_SETIFG[SET_LFXTIFG] Bits */
#define CS_SETIFG_SET_LFXTIFG_OFS                ( 0)                            /*!< SET_LFXTIFG Bit Offset */
#define CS_SETIFG_SET_LFXTIFG                    ((uint32_t)0x00000001)          /*!< Set LFXT oscillator fault interrupt flag */
/* CS_SETIFG[SET_HFXTIFG] Bits */
#define CS_SETIFG_SET_HFXTIFG_OFS                ( 1)                            /*!< SET_HFXTIFG Bit Offset */
#define CS_SETIFG_SET_HFXTIFG                    ((uint32_t)0x00000002)          /*!< Set HFXT oscillator fault interrupt flag */
/* CS_SETIFG[SET_DCOR_OPNIFG] Bits */
#define CS_SETIFG_SET_DCOR_OPNIFG_OFS            ( 6)                            /*!< SET_DCOR_OPNIFG Bit Offset */
#define CS_SETIFG_SET_DCOR_OPNIFG                ((uint32_t)0x00000040)          /*!< Set DCO external resistor open circuit fault interrupt flag. */
/* CS_SETIFG[SET_FCNTHFIFG] Bits */
#define CS_SETIFG_SET_FCNTHFIFG_OFS              ( 9)                            /*!< SET_FCNTHFIFG Bit Offset */
#define CS_SETIFG_SET_FCNTHFIFG                  ((uint32_t)0x00000200)          /*!< Start fault counter set interrupt flag HFXT */
/* CS_SETIFG[SET_FCNTLFIFG] Bits */
#define CS_SETIFG_SET_FCNTLFIFG_OFS              ( 8)                            /*!< SET_FCNTLFIFG Bit Offset */
#define CS_SETIFG_SET_FCNTLFIFG                  ((uint32_t)0x00000100)          /*!< Start fault counter set interrupt flag LFXT */
/* CS_DCOERCAL0[DCO_TCCAL] Bits */
#define CS_DCOERCAL0_DCO_TCCAL_OFS               ( 0)                            /*!< DCO_TCCAL Bit Offset */
#define CS_DCOERCAL0_DCO_TCCAL_MASK              ((uint32_t)0x00000003)          /*!< DCO_TCCAL Bit Mask */
/* CS_DCOERCAL0[DCO_FCAL_RSEL04] Bits */
#define CS_DCOERCAL0_DCO_FCAL_RSEL04_OFS         (16)                            /*!< DCO_FCAL_RSEL04 Bit Offset */
#define CS_DCOERCAL0_DCO_FCAL_RSEL04_MASK        ((uint32_t)0x03FF0000)          /*!< DCO_FCAL_RSEL04 Bit Mask */
/* CS_DCOERCAL1[DCO_FCAL_RSEL5] Bits */
#define CS_DCOERCAL1_DCO_FCAL_RSEL5_OFS          ( 0)                            /*!< DCO_FCAL_RSEL5 Bit Offset */
#define CS_DCOERCAL1_DCO_FCAL_RSEL5_MASK         ((uint32_t)0x000003FF)          /*!< DCO_FCAL_RSEL5 Bit Mask */
/* Pre-defined bitfield values */
#define CS_KEY_VAL                               ((uint32_t)0x0000695A)          /*!< CS control key value */

/******************************************************************************
* DIO Bits
******************************************************************************/
/* DIO_IV[IV] Bits */
#define DIO_PORT_IV_OFS                          ( 0)                            /*!< DIO Port IV Bit Offset */
#define DIO_PORT_IV_MASK                         ((uint16_t)0x001F)              /*!< DIO Port IV Bit Mask */
#define DIO_PORT_IV0                             ((uint16_t)0x0001)              /*!< DIO Port IV Bit 0 */
#define DIO_PORT_IV1                             ((uint16_t)0x0002)              /*!< DIO Port IV Bit 1 */
#define DIO_PORT_IV2                             ((uint16_t)0x0004)              /*!< DIO Port IV Bit 2 */
#define DIO_PORT_IV3                             ((uint16_t)0x0008)              /*!< DIO Port IV Bit 3 */
#define DIO_PORT_IV4                             ((uint16_t)0x0010)              /*!< DIO Port IV Bit 4 */
#define DIO_PORT_IV_0                            ((uint16_t)0x0000)              /*!< No interrupt pending */
#define DIO_PORT_IV_2                            ((uint16_t)0x0002)              /*!< Interrupt Source: Port x.0 interrupt; Interrupt Flag: IFG0; Interrupt  */
                                                                                 /* Priority: Highest */
#define DIO_PORT_IV_4                            ((uint16_t)0x0004)              /*!< Interrupt Source: Port x.1 interrupt; Interrupt Flag: IFG1 */
#define DIO_PORT_IV_6                            ((uint16_t)0x0006)              /*!< Interrupt Source: Port x.2 interrupt; Interrupt Flag: IFG2 */
#define DIO_PORT_IV_8                            ((uint16_t)0x0008)              /*!< Interrupt Source: Port x.3 interrupt; Interrupt Flag: IFG3 */
#define DIO_PORT_IV_10                           ((uint16_t)0x000A)              /*!< Interrupt Source: Port x.4 interrupt; Interrupt Flag: IFG4 */
#define DIO_PORT_IV_12                           ((uint16_t)0x000C)              /*!< Interrupt Source: Port x.5 interrupt; Interrupt Flag: IFG5 */
#define DIO_PORT_IV_14                           ((uint16_t)0x000E)              /*!< Interrupt Source: Port x.6 interrupt; Interrupt Flag: IFG6 */
#define DIO_PORT_IV_16                           ((uint16_t)0x0010)              /*!< Interrupt Source: Port x.7 interrupt; Interrupt Flag: IFG7; Interrupt  */
                                                                                 /* Priority: Lowest */
#define DIO_PORT_IV__NONE                        ((uint16_t)0x0000)              /*!< No interrupt pending */
#define DIO_PORT_IV__IFG0                        ((uint16_t)0x0002)              /*!< Interrupt Source: Port x.0 interrupt; Interrupt Flag: IFG0; Interrupt  */
                                                                                 /* Priority: Highest */
#define DIO_PORT_IV__IFG1                        ((uint16_t)0x0004)              /*!< Interrupt Source: Port x.1 interrupt; Interrupt Flag: IFG1 */
#define DIO_PORT_IV__IFG2                        ((uint16_t)0x0006)              /*!< Interrupt Source: Port x.2 interrupt; Interrupt Flag: IFG2 */
#define DIO_PORT_IV__IFG3                        ((uint16_t)0x0008)              /*!< Interrupt Source: Port x.3 interrupt; Interrupt Flag: IFG3 */
#define DIO_PORT_IV__IFG4                        ((uint16_t)0x000A)              /*!< Interrupt Source: Port x.4 interrupt; Interrupt Flag: IFG4 */
#define DIO_PORT_IV__IFG5                        ((uint16_t)0x000C)              /*!< Interrupt Source: Port x.5 interrupt; Interrupt Flag: IFG5 */
#define DIO_PORT_IV__IFG6                        ((uint16_t)0x000E)              /*!< Interrupt Source: Port x.6 interrupt; Interrupt Flag: IFG6 */
#define DIO_PORT_IV__IFG7                        ((uint16_t)0x0010)              /*!< Interrupt Source: Port x.7 interrupt; Interrupt Flag: IFG7; Interrupt  */
                                                                                 /* Priority: Lowest */


/******************************************************************************
* DMA Bits
******************************************************************************/
/* DMA_DEVICE_CFG[NUM_DMA_CHANNELS] Bits */
#define DMA_DEVICE_CFG_NUM_DMA_CHANNELS_OFS      ( 0)                            /*!< NUM_DMA_CHANNELS Bit Offset */
#define DMA_DEVICE_CFG_NUM_DMA_CHANNELS_MASK     ((uint32_t)0x000000FF)          /*!< NUM_DMA_CHANNELS Bit Mask */
/* DMA_DEVICE_CFG[NUM_SRC_PER_CHANNEL] Bits */
#define DMA_DEVICE_CFG_NUM_SRC_PER_CHANNEL_OFS   ( 8)                            /*!< NUM_SRC_PER_CHANNEL Bit Offset */
#define DMA_DEVICE_CFG_NUM_SRC_PER_CHANNEL_MASK  ((uint32_t)0x0000FF00)          /*!< NUM_SRC_PER_CHANNEL Bit Mask */
/* DMA_SW_CHTRIG[CH0] Bits */
#define DMA_SW_CHTRIG_CH0_OFS                    ( 0)                            /*!< CH0 Bit Offset */
#define DMA_SW_CHTRIG_CH0                        ((uint32_t)0x00000001)          /*!< Write 1, triggers DMA_CHANNEL0 */
/* DMA_SW_CHTRIG[CH1] Bits */
#define DMA_SW_CHTRIG_CH1_OFS                    ( 1)                            /*!< CH1 Bit Offset */
#define DMA_SW_CHTRIG_CH1                        ((uint32_t)0x00000002)          /*!< Write 1, triggers DMA_CHANNEL1 */
/* DMA_SW_CHTRIG[CH2] Bits */
#define DMA_SW_CHTRIG_CH2_OFS                    ( 2)                            /*!< CH2 Bit Offset */
#define DMA_SW_CHTRIG_CH2                        ((uint32_t)0x00000004)          /*!< Write 1, triggers DMA_CHANNEL2 */
/* DMA_SW_CHTRIG[CH3] Bits */
#define DMA_SW_CHTRIG_CH3_OFS                    ( 3)                            /*!< CH3 Bit Offset */
#define DMA_SW_CHTRIG_CH3                        ((uint32_t)0x00000008)          /*!< Write 1, triggers DMA_CHANNEL3 */
/* DMA_SW_CHTRIG[CH4] Bits */
#define DMA_SW_CHTRIG_CH4_OFS                    ( 4)                            /*!< CH4 Bit Offset */
#define DMA_SW_CHTRIG_CH4                        ((uint32_t)0x00000010)          /*!< Write 1, triggers DMA_CHANNEL4 */
/* DMA_SW_CHTRIG[CH5] Bits */
#define DMA_SW_CHTRIG_CH5_OFS                    ( 5)                            /*!< CH5 Bit Offset */
#define DMA_SW_CHTRIG_CH5                        ((uint32_t)0x00000020)          /*!< Write 1, triggers DMA_CHANNEL5 */
/* DMA_SW_CHTRIG[CH6] Bits */
#define DMA_SW_CHTRIG_CH6_OFS                    ( 6)                            /*!< CH6 Bit Offset */
#define DMA_SW_CHTRIG_CH6                        ((uint32_t)0x00000040)          /*!< Write 1, triggers DMA_CHANNEL6 */
/* DMA_SW_CHTRIG[CH7] Bits */
#define DMA_SW_CHTRIG_CH7_OFS                    ( 7)                            /*!< CH7 Bit Offset */
#define DMA_SW_CHTRIG_CH7                        ((uint32_t)0x00000080)          /*!< Write 1, triggers DMA_CHANNEL7 */
/* DMA_SW_CHTRIG[CH8] Bits */
#define DMA_SW_CHTRIG_CH8_OFS                    ( 8)                            /*!< CH8 Bit Offset */
#define DMA_SW_CHTRIG_CH8                        ((uint32_t)0x00000100)          /*!< Write 1, triggers DMA_CHANNEL8 */
/* DMA_SW_CHTRIG[CH9] Bits */
#define DMA_SW_CHTRIG_CH9_OFS                    ( 9)                            /*!< CH9 Bit Offset */
#define DMA_SW_CHTRIG_CH9                        ((uint32_t)0x00000200)          /*!< Write 1, triggers DMA_CHANNEL9 */
/* DMA_SW_CHTRIG[CH10] Bits */
#define DMA_SW_CHTRIG_CH10_OFS                   (10)                            /*!< CH10 Bit Offset */
#define DMA_SW_CHTRIG_CH10                       ((uint32_t)0x00000400)          /*!< Write 1, triggers DMA_CHANNEL10 */
/* DMA_SW_CHTRIG[CH11] Bits */
#define DMA_SW_CHTRIG_CH11_OFS                   (11)                            /*!< CH11 Bit Offset */
#define DMA_SW_CHTRIG_CH11                       ((uint32_t)0x00000800)          /*!< Write 1, triggers DMA_CHANNEL11 */
/* DMA_SW_CHTRIG[CH12] Bits */
#define DMA_SW_CHTRIG_CH12_OFS                   (12)                            /*!< CH12 Bit Offset */
#define DMA_SW_CHTRIG_CH12                       ((uint32_t)0x00001000)          /*!< Write 1, triggers DMA_CHANNEL12 */
/* DMA_SW_CHTRIG[CH13] Bits */
#define DMA_SW_CHTRIG_CH13_OFS                   (13)                            /*!< CH13 Bit Offset */
#define DMA_SW_CHTRIG_CH13                       ((uint32_t)0x00002000)          /*!< Write 1, triggers DMA_CHANNEL13 */
/* DMA_SW_CHTRIG[CH14] Bits */
#define DMA_SW_CHTRIG_CH14_OFS                   (14)                            /*!< CH14 Bit Offset */
#define DMA_SW_CHTRIG_CH14                       ((uint32_t)0x00004000)          /*!< Write 1, triggers DMA_CHANNEL14 */
/* DMA_SW_CHTRIG[CH15] Bits */
#define DMA_SW_CHTRIG_CH15_OFS                   (15)                            /*!< CH15 Bit Offset */
#define DMA_SW_CHTRIG_CH15                       ((uint32_t)0x00008000)          /*!< Write 1, triggers DMA_CHANNEL15 */
/* DMA_SW_CHTRIG[CH16] Bits */
#define DMA_SW_CHTRIG_CH16_OFS                   (16)                            /*!< CH16 Bit Offset */
#define DMA_SW_CHTRIG_CH16                       ((uint32_t)0x00010000)          /*!< Write 1, triggers DMA_CHANNEL16 */
/* DMA_SW_CHTRIG[CH17] Bits */
#define DMA_SW_CHTRIG_CH17_OFS                   (17)                            /*!< CH17 Bit Offset */
#define DMA_SW_CHTRIG_CH17                       ((uint32_t)0x00020000)          /*!< Write 1, triggers DMA_CHANNEL17 */
/* DMA_SW_CHTRIG[CH18] Bits */
#define DMA_SW_CHTRIG_CH18_OFS                   (18)                            /*!< CH18 Bit Offset */
#define DMA_SW_CHTRIG_CH18                       ((uint32_t)0x00040000)          /*!< Write 1, triggers DMA_CHANNEL18 */
/* DMA_SW_CHTRIG[CH19] Bits */
#define DMA_SW_CHTRIG_CH19_OFS                   (19)                            /*!< CH19 Bit Offset */
#define DMA_SW_CHTRIG_CH19                       ((uint32_t)0x00080000)          /*!< Write 1, triggers DMA_CHANNEL19 */
/* DMA_SW_CHTRIG[CH20] Bits */
#define DMA_SW_CHTRIG_CH20_OFS                   (20)                            /*!< CH20 Bit Offset */
#define DMA_SW_CHTRIG_CH20                       ((uint32_t)0x00100000)          /*!< Write 1, triggers DMA_CHANNEL20 */
/* DMA_SW_CHTRIG[CH21] Bits */
#define DMA_SW_CHTRIG_CH21_OFS                   (21)                            /*!< CH21 Bit Offset */
#define DMA_SW_CHTRIG_CH21                       ((uint32_t)0x00200000)          /*!< Write 1, triggers DMA_CHANNEL21 */
/* DMA_SW_CHTRIG[CH22] Bits */
#define DMA_SW_CHTRIG_CH22_OFS                   (22)                            /*!< CH22 Bit Offset */
#define DMA_SW_CHTRIG_CH22                       ((uint32_t)0x00400000)          /*!< Write 1, triggers DMA_CHANNEL22 */
/* DMA_SW_CHTRIG[CH23] Bits */
#define DMA_SW_CHTRIG_CH23_OFS                   (23)                            /*!< CH23 Bit Offset */
#define DMA_SW_CHTRIG_CH23                       ((uint32_t)0x00800000)          /*!< Write 1, triggers DMA_CHANNEL23 */
/* DMA_SW_CHTRIG[CH24] Bits */
#define DMA_SW_CHTRIG_CH24_OFS                   (24)                            /*!< CH24 Bit Offset */
#define DMA_SW_CHTRIG_CH24                       ((uint32_t)0x01000000)          /*!< Write 1, triggers DMA_CHANNEL24 */
/* DMA_SW_CHTRIG[CH25] Bits */
#define DMA_SW_CHTRIG_CH25_OFS                   (25)                            /*!< CH25 Bit Offset */
#define DMA_SW_CHTRIG_CH25                       ((uint32_t)0x02000000)          /*!< Write 1, triggers DMA_CHANNEL25 */
/* DMA_SW_CHTRIG[CH26] Bits */
#define DMA_SW_CHTRIG_CH26_OFS                   (26)                            /*!< CH26 Bit Offset */
#define DMA_SW_CHTRIG_CH26                       ((uint32_t)0x04000000)          /*!< Write 1, triggers DMA_CHANNEL26 */
/* DMA_SW_CHTRIG[CH27] Bits */
#define DMA_SW_CHTRIG_CH27_OFS                   (27)                            /*!< CH27 Bit Offset */
#define DMA_SW_CHTRIG_CH27                       ((uint32_t)0x08000000)          /*!< Write 1, triggers DMA_CHANNEL27 */
/* DMA_SW_CHTRIG[CH28] Bits */
#define DMA_SW_CHTRIG_CH28_OFS                   (28)                            /*!< CH28 Bit Offset */
#define DMA_SW_CHTRIG_CH28                       ((uint32_t)0x10000000)          /*!< Write 1, triggers DMA_CHANNEL28 */
/* DMA_SW_CHTRIG[CH29] Bits */
#define DMA_SW_CHTRIG_CH29_OFS                   (29)                            /*!< CH29 Bit Offset */
#define DMA_SW_CHTRIG_CH29                       ((uint32_t)0x20000000)          /*!< Write 1, triggers DMA_CHANNEL29 */
/* DMA_SW_CHTRIG[CH30] Bits */
#define DMA_SW_CHTRIG_CH30_OFS                   (30)                            /*!< CH30 Bit Offset */
#define DMA_SW_CHTRIG_CH30                       ((uint32_t)0x40000000)          /*!< Write 1, triggers DMA_CHANNEL30 */
/* DMA_SW_CHTRIG[CH31] Bits */
#define DMA_SW_CHTRIG_CH31_OFS                   (31)                            /*!< CH31 Bit Offset */
#define DMA_SW_CHTRIG_CH31                       ((uint32_t)0x80000000)          /*!< Write 1, triggers DMA_CHANNEL31 */
/* DMA_CHN_SRCCFG[DMA_SRC] Bits */
#define DMA_CHN_SRCCFG_DMA_SRC_OFS               ( 0)                            /*!< DMA_SRC Bit Offset */
#define DMA_CHN_SRCCFG_DMA_SRC_MASK              ((uint32_t)0x000000FF)          /*!< DMA_SRC Bit Mask */
/* DMA_INT1_SRCCFG[INT_SRC] Bits */
#define DMA_INT1_SRCCFG_INT_SRC_OFS              ( 0)                            /*!< INT_SRC Bit Offset */
#define DMA_INT1_SRCCFG_INT_SRC_MASK             ((uint32_t)0x0000001F)          /*!< INT_SRC Bit Mask */
/* DMA_INT1_SRCCFG[EN] Bits */
#define DMA_INT1_SRCCFG_EN_OFS                   ( 5)                            /*!< EN Bit Offset */
#define DMA_INT1_SRCCFG_EN                       ((uint32_t)0x00000020)          /*!< Enables DMA_INT1 mapping */
/* DMA_INT2_SRCCFG[INT_SRC] Bits */
#define DMA_INT2_SRCCFG_INT_SRC_OFS              ( 0)                            /*!< INT_SRC Bit Offset */
#define DMA_INT2_SRCCFG_INT_SRC_MASK             ((uint32_t)0x0000001F)          /*!< INT_SRC Bit Mask */
/* DMA_INT2_SRCCFG[EN] Bits */
#define DMA_INT2_SRCCFG_EN_OFS                   ( 5)                            /*!< EN Bit Offset */
#define DMA_INT2_SRCCFG_EN                       ((uint32_t)0x00000020)          /*!< Enables DMA_INT2 mapping */
/* DMA_INT3_SRCCFG[INT_SRC] Bits */
#define DMA_INT3_SRCCFG_INT_SRC_OFS              ( 0)                            /*!< INT_SRC Bit Offset */
#define DMA_INT3_SRCCFG_INT_SRC_MASK             ((uint32_t)0x0000001F)          /*!< INT_SRC Bit Mask */
/* DMA_INT3_SRCCFG[EN] Bits */
#define DMA_INT3_SRCCFG_EN_OFS                   ( 5)                            /*!< EN Bit Offset */
#define DMA_INT3_SRCCFG_EN                       ((uint32_t)0x00000020)          /*!< Enables DMA_INT3 mapping */
/* DMA_INT0_SRCFLG[CH0] Bits */
#define DMA_INT0_SRCFLG_CH0_OFS                  ( 0)                            /*!< CH0 Bit Offset */
#define DMA_INT0_SRCFLG_CH0                      ((uint32_t)0x00000001)          /*!< Channel 0 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH1] Bits */
#define DMA_INT0_SRCFLG_CH1_OFS                  ( 1)                            /*!< CH1 Bit Offset */
#define DMA_INT0_SRCFLG_CH1                      ((uint32_t)0x00000002)          /*!< Channel 1 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH2] Bits */
#define DMA_INT0_SRCFLG_CH2_OFS                  ( 2)                            /*!< CH2 Bit Offset */
#define DMA_INT0_SRCFLG_CH2                      ((uint32_t)0x00000004)          /*!< Channel 2 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH3] Bits */
#define DMA_INT0_SRCFLG_CH3_OFS                  ( 3)                            /*!< CH3 Bit Offset */
#define DMA_INT0_SRCFLG_CH3                      ((uint32_t)0x00000008)          /*!< Channel 3 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH4] Bits */
#define DMA_INT0_SRCFLG_CH4_OFS                  ( 4)                            /*!< CH4 Bit Offset */
#define DMA_INT0_SRCFLG_CH4                      ((uint32_t)0x00000010)          /*!< Channel 4 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH5] Bits */
#define DMA_INT0_SRCFLG_CH5_OFS                  ( 5)                            /*!< CH5 Bit Offset */
#define DMA_INT0_SRCFLG_CH5                      ((uint32_t)0x00000020)          /*!< Channel 5 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH6] Bits */
#define DMA_INT0_SRCFLG_CH6_OFS                  ( 6)                            /*!< CH6 Bit Offset */
#define DMA_INT0_SRCFLG_CH6                      ((uint32_t)0x00000040)          /*!< Channel 6 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH7] Bits */
#define DMA_INT0_SRCFLG_CH7_OFS                  ( 7)                            /*!< CH7 Bit Offset */
#define DMA_INT0_SRCFLG_CH7                      ((uint32_t)0x00000080)          /*!< Channel 7 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH8] Bits */
#define DMA_INT0_SRCFLG_CH8_OFS                  ( 8)                            /*!< CH8 Bit Offset */
#define DMA_INT0_SRCFLG_CH8                      ((uint32_t)0x00000100)          /*!< Channel 8 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH9] Bits */
#define DMA_INT0_SRCFLG_CH9_OFS                  ( 9)                            /*!< CH9 Bit Offset */
#define DMA_INT0_SRCFLG_CH9                      ((uint32_t)0x00000200)          /*!< Channel 9 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH10] Bits */
#define DMA_INT0_SRCFLG_CH10_OFS                 (10)                            /*!< CH10 Bit Offset */
#define DMA_INT0_SRCFLG_CH10                     ((uint32_t)0x00000400)          /*!< Channel 10 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH11] Bits */
#define DMA_INT0_SRCFLG_CH11_OFS                 (11)                            /*!< CH11 Bit Offset */
#define DMA_INT0_SRCFLG_CH11                     ((uint32_t)0x00000800)          /*!< Channel 11 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH12] Bits */
#define DMA_INT0_SRCFLG_CH12_OFS                 (12)                            /*!< CH12 Bit Offset */
#define DMA_INT0_SRCFLG_CH12                     ((uint32_t)0x00001000)          /*!< Channel 12 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH13] Bits */
#define DMA_INT0_SRCFLG_CH13_OFS                 (13)                            /*!< CH13 Bit Offset */
#define DMA_INT0_SRCFLG_CH13                     ((uint32_t)0x00002000)          /*!< Channel 13 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH14] Bits */
#define DMA_INT0_SRCFLG_CH14_OFS                 (14)                            /*!< CH14 Bit Offset */
#define DMA_INT0_SRCFLG_CH14                     ((uint32_t)0x00004000)          /*!< Channel 14 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH15] Bits */
#define DMA_INT0_SRCFLG_CH15_OFS                 (15)                            /*!< CH15 Bit Offset */
#define DMA_INT0_SRCFLG_CH15                     ((uint32_t)0x00008000)          /*!< Channel 15 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH16] Bits */
#define DMA_INT0_SRCFLG_CH16_OFS                 (16)                            /*!< CH16 Bit Offset */
#define DMA_INT0_SRCFLG_CH16                     ((uint32_t)0x00010000)          /*!< Channel 16 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH17] Bits */
#define DMA_INT0_SRCFLG_CH17_OFS                 (17)                            /*!< CH17 Bit Offset */
#define DMA_INT0_SRCFLG_CH17                     ((uint32_t)0x00020000)          /*!< Channel 17 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH18] Bits */
#define DMA_INT0_SRCFLG_CH18_OFS                 (18)                            /*!< CH18 Bit Offset */
#define DMA_INT0_SRCFLG_CH18                     ((uint32_t)0x00040000)          /*!< Channel 18 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH19] Bits */
#define DMA_INT0_SRCFLG_CH19_OFS                 (19)                            /*!< CH19 Bit Offset */
#define DMA_INT0_SRCFLG_CH19                     ((uint32_t)0x00080000)          /*!< Channel 19 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH20] Bits */
#define DMA_INT0_SRCFLG_CH20_OFS                 (20)                            /*!< CH20 Bit Offset */
#define DMA_INT0_SRCFLG_CH20                     ((uint32_t)0x00100000)          /*!< Channel 20 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH21] Bits */
#define DMA_INT0_SRCFLG_CH21_OFS                 (21)                            /*!< CH21 Bit Offset */
#define DMA_INT0_SRCFLG_CH21                     ((uint32_t)0x00200000)          /*!< Channel 21 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH22] Bits */
#define DMA_INT0_SRCFLG_CH22_OFS                 (22)                            /*!< CH22 Bit Offset */
#define DMA_INT0_SRCFLG_CH22                     ((uint32_t)0x00400000)          /*!< Channel 22 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH23] Bits */
#define DMA_INT0_SRCFLG_CH23_OFS                 (23)                            /*!< CH23 Bit Offset */
#define DMA_INT0_SRCFLG_CH23                     ((uint32_t)0x00800000)          /*!< Channel 23 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH24] Bits */
#define DMA_INT0_SRCFLG_CH24_OFS                 (24)                            /*!< CH24 Bit Offset */
#define DMA_INT0_SRCFLG_CH24                     ((uint32_t)0x01000000)          /*!< Channel 24 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH25] Bits */
#define DMA_INT0_SRCFLG_CH25_OFS                 (25)                            /*!< CH25 Bit Offset */
#define DMA_INT0_SRCFLG_CH25                     ((uint32_t)0x02000000)          /*!< Channel 25 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH26] Bits */
#define DMA_INT0_SRCFLG_CH26_OFS                 (26)                            /*!< CH26 Bit Offset */
#define DMA_INT0_SRCFLG_CH26                     ((uint32_t)0x04000000)          /*!< Channel 26 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH27] Bits */
#define DMA_INT0_SRCFLG_CH27_OFS                 (27)                            /*!< CH27 Bit Offset */
#define DMA_INT0_SRCFLG_CH27                     ((uint32_t)0x08000000)          /*!< Channel 27 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH28] Bits */
#define DMA_INT0_SRCFLG_CH28_OFS                 (28)                            /*!< CH28 Bit Offset */
#define DMA_INT0_SRCFLG_CH28                     ((uint32_t)0x10000000)          /*!< Channel 28 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH29] Bits */
#define DMA_INT0_SRCFLG_CH29_OFS                 (29)                            /*!< CH29 Bit Offset */
#define DMA_INT0_SRCFLG_CH29                     ((uint32_t)0x20000000)          /*!< Channel 29 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH30] Bits */
#define DMA_INT0_SRCFLG_CH30_OFS                 (30)                            /*!< CH30 Bit Offset */
#define DMA_INT0_SRCFLG_CH30                     ((uint32_t)0x40000000)          /*!< Channel 30 was the source of DMA_INT0 */
/* DMA_INT0_SRCFLG[CH31] Bits */
#define DMA_INT0_SRCFLG_CH31_OFS                 (31)                            /*!< CH31 Bit Offset */
#define DMA_INT0_SRCFLG_CH31                     ((uint32_t)0x80000000)          /*!< Channel 31 was the source of DMA_INT0 */
/* DMA_INT0_CLRFLG[CH0] Bits */
#define DMA_INT0_CLRFLG_CH0_OFS                  ( 0)                            /*!< CH0 Bit Offset */
#define DMA_INT0_CLRFLG_CH0                      ((uint32_t)0x00000001)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH1] Bits */
#define DMA_INT0_CLRFLG_CH1_OFS                  ( 1)                            /*!< CH1 Bit Offset */
#define DMA_INT0_CLRFLG_CH1                      ((uint32_t)0x00000002)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH2] Bits */
#define DMA_INT0_CLRFLG_CH2_OFS                  ( 2)                            /*!< CH2 Bit Offset */
#define DMA_INT0_CLRFLG_CH2                      ((uint32_t)0x00000004)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH3] Bits */
#define DMA_INT0_CLRFLG_CH3_OFS                  ( 3)                            /*!< CH3 Bit Offset */
#define DMA_INT0_CLRFLG_CH3                      ((uint32_t)0x00000008)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH4] Bits */
#define DMA_INT0_CLRFLG_CH4_OFS                  ( 4)                            /*!< CH4 Bit Offset */
#define DMA_INT0_CLRFLG_CH4                      ((uint32_t)0x00000010)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH5] Bits */
#define DMA_INT0_CLRFLG_CH5_OFS                  ( 5)                            /*!< CH5 Bit Offset */
#define DMA_INT0_CLRFLG_CH5                      ((uint32_t)0x00000020)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH6] Bits */
#define DMA_INT0_CLRFLG_CH6_OFS                  ( 6)                            /*!< CH6 Bit Offset */
#define DMA_INT0_CLRFLG_CH6                      ((uint32_t)0x00000040)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH7] Bits */
#define DMA_INT0_CLRFLG_CH7_OFS                  ( 7)                            /*!< CH7 Bit Offset */
#define DMA_INT0_CLRFLG_CH7                      ((uint32_t)0x00000080)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH8] Bits */
#define DMA_INT0_CLRFLG_CH8_OFS                  ( 8)                            /*!< CH8 Bit Offset */
#define DMA_INT0_CLRFLG_CH8                      ((uint32_t)0x00000100)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH9] Bits */
#define DMA_INT0_CLRFLG_CH9_OFS                  ( 9)                            /*!< CH9 Bit Offset */
#define DMA_INT0_CLRFLG_CH9                      ((uint32_t)0x00000200)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH10] Bits */
#define DMA_INT0_CLRFLG_CH10_OFS                 (10)                            /*!< CH10 Bit Offset */
#define DMA_INT0_CLRFLG_CH10                     ((uint32_t)0x00000400)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH11] Bits */
#define DMA_INT0_CLRFLG_CH11_OFS                 (11)                            /*!< CH11 Bit Offset */
#define DMA_INT0_CLRFLG_CH11                     ((uint32_t)0x00000800)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH12] Bits */
#define DMA_INT0_CLRFLG_CH12_OFS                 (12)                            /*!< CH12 Bit Offset */
#define DMA_INT0_CLRFLG_CH12                     ((uint32_t)0x00001000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH13] Bits */
#define DMA_INT0_CLRFLG_CH13_OFS                 (13)                            /*!< CH13 Bit Offset */
#define DMA_INT0_CLRFLG_CH13                     ((uint32_t)0x00002000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH14] Bits */
#define DMA_INT0_CLRFLG_CH14_OFS                 (14)                            /*!< CH14 Bit Offset */
#define DMA_INT0_CLRFLG_CH14                     ((uint32_t)0x00004000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH15] Bits */
#define DMA_INT0_CLRFLG_CH15_OFS                 (15)                            /*!< CH15 Bit Offset */
#define DMA_INT0_CLRFLG_CH15                     ((uint32_t)0x00008000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH16] Bits */
#define DMA_INT0_CLRFLG_CH16_OFS                 (16)                            /*!< CH16 Bit Offset */
#define DMA_INT0_CLRFLG_CH16                     ((uint32_t)0x00010000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH17] Bits */
#define DMA_INT0_CLRFLG_CH17_OFS                 (17)                            /*!< CH17 Bit Offset */
#define DMA_INT0_CLRFLG_CH17                     ((uint32_t)0x00020000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH18] Bits */
#define DMA_INT0_CLRFLG_CH18_OFS                 (18)                            /*!< CH18 Bit Offset */
#define DMA_INT0_CLRFLG_CH18                     ((uint32_t)0x00040000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH19] Bits */
#define DMA_INT0_CLRFLG_CH19_OFS                 (19)                            /*!< CH19 Bit Offset */
#define DMA_INT0_CLRFLG_CH19                     ((uint32_t)0x00080000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH20] Bits */
#define DMA_INT0_CLRFLG_CH20_OFS                 (20)                            /*!< CH20 Bit Offset */
#define DMA_INT0_CLRFLG_CH20                     ((uint32_t)0x00100000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH21] Bits */
#define DMA_INT0_CLRFLG_CH21_OFS                 (21)                            /*!< CH21 Bit Offset */
#define DMA_INT0_CLRFLG_CH21                     ((uint32_t)0x00200000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH22] Bits */
#define DMA_INT0_CLRFLG_CH22_OFS                 (22)                            /*!< CH22 Bit Offset */
#define DMA_INT0_CLRFLG_CH22                     ((uint32_t)0x00400000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH23] Bits */
#define DMA_INT0_CLRFLG_CH23_OFS                 (23)                            /*!< CH23 Bit Offset */
#define DMA_INT0_CLRFLG_CH23                     ((uint32_t)0x00800000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH24] Bits */
#define DMA_INT0_CLRFLG_CH24_OFS                 (24)                            /*!< CH24 Bit Offset */
#define DMA_INT0_CLRFLG_CH24                     ((uint32_t)0x01000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH25] Bits */
#define DMA_INT0_CLRFLG_CH25_OFS                 (25)                            /*!< CH25 Bit Offset */
#define DMA_INT0_CLRFLG_CH25                     ((uint32_t)0x02000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH26] Bits */
#define DMA_INT0_CLRFLG_CH26_OFS                 (26)                            /*!< CH26 Bit Offset */
#define DMA_INT0_CLRFLG_CH26                     ((uint32_t)0x04000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH27] Bits */
#define DMA_INT0_CLRFLG_CH27_OFS                 (27)                            /*!< CH27 Bit Offset */
#define DMA_INT0_CLRFLG_CH27                     ((uint32_t)0x08000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH28] Bits */
#define DMA_INT0_CLRFLG_CH28_OFS                 (28)                            /*!< CH28 Bit Offset */
#define DMA_INT0_CLRFLG_CH28                     ((uint32_t)0x10000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH29] Bits */
#define DMA_INT0_CLRFLG_CH29_OFS                 (29)                            /*!< CH29 Bit Offset */
#define DMA_INT0_CLRFLG_CH29                     ((uint32_t)0x20000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH30] Bits */
#define DMA_INT0_CLRFLG_CH30_OFS                 (30)                            /*!< CH30 Bit Offset */
#define DMA_INT0_CLRFLG_CH30                     ((uint32_t)0x40000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_INT0_CLRFLG[CH31] Bits */
#define DMA_INT0_CLRFLG_CH31_OFS                 (31)                            /*!< CH31 Bit Offset */
#define DMA_INT0_CLRFLG_CH31                     ((uint32_t)0x80000000)          /*!< Clear corresponding DMA_INT0_SRCFLG_REG */
/* DMA_STAT[MASTEN] Bits */
#define DMA_STAT_MASTEN_OFS                      ( 0)                            /*!< MASTEN Bit Offset */
#define DMA_STAT_MASTEN                          ((uint32_t)0x00000001)          
/* DMA_STAT[STATE] Bits */
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
/* DMA_STAT[DMACHANS] Bits */
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
/* DMA_STAT[TESTSTAT] Bits */
#define DMA_STAT_TESTSTAT_OFS                    (28)                            /*!< TESTSTAT Bit Offset */
#define DMA_STAT_TESTSTAT_MASK                   ((uint32_t)0xF0000000)          /*!< TESTSTAT Bit Mask */
#define DMA_STAT_TESTSTAT0                       ((uint32_t)0x10000000)          /*!< TESTSTAT Bit 0 */
#define DMA_STAT_TESTSTAT1                       ((uint32_t)0x20000000)          /*!< TESTSTAT Bit 1 */
#define DMA_STAT_TESTSTAT2                       ((uint32_t)0x40000000)          /*!< TESTSTAT Bit 2 */
#define DMA_STAT_TESTSTAT3                       ((uint32_t)0x80000000)          /*!< TESTSTAT Bit 3 */
#define DMA_STAT_TESTSTAT_0                      ((uint32_t)0x00000000)          /*!< Controller does not include the integration test logic */
#define DMA_STAT_TESTSTAT_1                      ((uint32_t)0x10000000)          /*!< Controller includes the integration test logic */
/* DMA_CFG[MASTEN] Bits */
#define DMA_CFG_MASTEN_OFS                       ( 0)                            /*!< MASTEN Bit Offset */
#define DMA_CFG_MASTEN                           ((uint32_t)0x00000001)          
/* DMA_CFG[CHPROTCTRL] Bits */
#define DMA_CFG_CHPROTCTRL_OFS                   ( 5)                            /*!< CHPROTCTRL Bit Offset */
#define DMA_CFG_CHPROTCTRL_MASK                  ((uint32_t)0x000000E0)          /*!< CHPROTCTRL Bit Mask */
/* DMA_CTLBASE[ADDR] Bits */
#define DMA_CTLBASE_ADDR_OFS                     ( 5)                            /*!< ADDR Bit Offset */
#define DMA_CTLBASE_ADDR_MASK                    ((uint32_t)0xFFFFFFE0)          /*!< ADDR Bit Mask */
/* DMA_ERRCLR[ERRCLR] Bits */
#define DMA_ERRCLR_ERRCLR_OFS                    ( 0)                            /*!< ERRCLR Bit Offset */
#define DMA_ERRCLR_ERRCLR                        ((uint32_t)0x00000001)          
/* DMA channel definitions and memory structure alignment */
#define __MCU_NUM_DMA_CHANNELS__                8
#define DMA_CHANNEL_CONTROL_STRUCT_SIZE         0x10
#define DMA_CONTROL_MEMORY_ALIGNMENT            (__MCU_NUM_DMA_CHANNELS__ * DMA_CHANNEL_CONTROL_STRUCT_SIZE)

/* UDMA_STAT Control Bits */
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

/* UDMA_CFG Control Bits */
#define UDMA_CFG_MASTEN                         ((uint32_t)0x00000001)           /*!< Controller Master Enable */

/* UDMA_CTLBASE Control Bits */
#define UDMA_CTLBASE_ADDR_M                     ((uint32_t)0xFFFFFC00)           /*!< Channel Control Base Address */
#define UDMA_CTLBASE_ADDR_S                     (10)

/* UDMA_ALTBASE Control Bits */
#define UDMA_ALTBASE_ADDR_M                     ((uint32_t)0xFFFFFFFF)           /*!< Alternate Channel Address Pointer */
#define UDMA_ALTBASE_ADDR_S                     ( 0)

/* UDMA_WAITSTAT Control Bits */
#define UDMA_WAITSTAT_WAITREQ_M                 ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Wait Status */

/* UDMA_SWREQ Control Bits */
#define UDMA_SWREQ_M                            ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Software Request */

/* UDMA_USEBURSTSET Control Bits */
#define UDMA_USEBURSTSET_SET_M                  ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Useburst Set */

/* UDMA_USEBURSTCLR Control Bits */
#define UDMA_USEBURSTCLR_CLR_M                  ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Useburst Clear */

/* UDMA_REQMASKSET Control Bits */
#define UDMA_REQMASKSET_SET_M                   ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Request Mask Set */

/* UDMA_REQMASKCLR Control Bits */
#define UDMA_REQMASKCLR_CLR_M                   ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Request Mask Clear */

/* UDMA_ENASET Control Bits */
#define UDMA_ENASET_SET_M                       ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Enable Set */

/* UDMA_ENACLR Control Bits */
#define UDMA_ENACLR_CLR_M                       ((uint32_t)0xFFFFFFFF)           /*!< Clear Channel [n] Enable Clear */

/* UDMA_ALTSET Control Bits */
#define UDMA_ALTSET_SET_M                       ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Alternate Set */

/* UDMA_ALTCLR Control Bits */
#define UDMA_ALTCLR_CLR_M                       ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Alternate Clear */

/* UDMA_PRIOSET Control Bits */
#define UDMA_PRIOSET_SET_M                      ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Priority Set */

/* UDMA_PRIOCLR Control Bits */
#define UDMA_PRIOCLR_CLR_M                      ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Priority Clear */

/* UDMA_ERRCLR Control Bits */
#define UDMA_ERRCLR_ERRCLR                      ((uint32_t)0x00000001)           /*!< uDMA Bus Error Status */

/* UDMA_CHASGN Control Bits */
#define UDMA_CHASGN_M                           ((uint32_t)0xFFFFFFFF)           /*!< Channel [n] Assignment Select */
#define UDMA_CHASGN_PRIMARY                     ((uint32_t)0x00000000)           /*!< Use the primary channel assignment */
#define UDMA_CHASGN_SECONDARY                   ((uint32_t)0x00000001)           /*!< Use the secondary channel assignment */

/* Micro Direct Memory Access (uDMA) offsets */
#define UDMA_O_SRCENDP                          ((uint32_t)0x00000000)           /*!< DMA Channel Source Address End Pointer */
#define UDMA_O_DSTENDP                          ((uint32_t)0x00000004)           /*!< DMA Channel Destination Address End Pointer */
#define UDMA_O_CHCTL                            ((uint32_t)0x00000008)           /*!< DMA Channel Control Word */

/* UDMA_O_SRCENDP Control Bits */
#define UDMA_SRCENDP_ADDR_M                     ((uint32_t)0xFFFFFFFF)           /*!< Source Address End Pointer */
#define UDMA_SRCENDP_ADDR_S                     ( 0)

/* UDMA_O_DSTENDP Control Bits */
#define UDMA_DSTENDP_ADDR_M                     ((uint32_t)0xFFFFFFFF)           /*!< Destination Address End Pointer */
#define UDMA_DSTENDP_ADDR_S                     ( 0)

/* UDMA_O_CHCTL Control Bits */
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


/******************************************************************************
* DWT Bits
******************************************************************************/


/******************************************************************************
* EUSCI_A Bits
******************************************************************************/
/* EUSCI_A_CTLW0[SWRST] Bits */
#define EUSCI_A_CTLW0_SWRST_OFS                  ( 0)                            /*!< UCSWRST Bit Offset */
#define EUSCI_A_CTLW0_SWRST                      ((uint16_t)0x0001)              /*!< Software reset enable */
/* EUSCI_A_CTLW0[TXBRK] Bits */
#define EUSCI_A_CTLW0_TXBRK_OFS                  ( 1)                            /*!< UCTXBRK Bit Offset */
#define EUSCI_A_CTLW0_TXBRK                      ((uint16_t)0x0002)              /*!< Transmit break */
/* EUSCI_A_CTLW0[TXADDR] Bits */
#define EUSCI_A_CTLW0_TXADDR_OFS                 ( 2)                            /*!< UCTXADDR Bit Offset */
#define EUSCI_A_CTLW0_TXADDR                     ((uint16_t)0x0004)              /*!< Transmit address */
/* EUSCI_A_CTLW0[DORM] Bits */
#define EUSCI_A_CTLW0_DORM_OFS                   ( 3)                            /*!< UCDORM Bit Offset */
#define EUSCI_A_CTLW0_DORM                       ((uint16_t)0x0008)              /*!< Dormant */
/* EUSCI_A_CTLW0[BRKIE] Bits */
#define EUSCI_A_CTLW0_BRKIE_OFS                  ( 4)                            /*!< UCBRKIE Bit Offset */
#define EUSCI_A_CTLW0_BRKIE                      ((uint16_t)0x0010)              /*!< Receive break character interrupt enable */
/* EUSCI_A_CTLW0[RXEIE] Bits */
#define EUSCI_A_CTLW0_RXEIE_OFS                  ( 5)                            /*!< UCRXEIE Bit Offset */
#define EUSCI_A_CTLW0_RXEIE                      ((uint16_t)0x0020)              /*!< Receive erroneous-character interrupt enable */
/* EUSCI_A_CTLW0[SSEL] Bits */
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
/* EUSCI_A_CTLW0[SYNC] Bits */
#define EUSCI_A_CTLW0_SYNC_OFS                   ( 8)                            /*!< UCSYNC Bit Offset */
#define EUSCI_A_CTLW0_SYNC                       ((uint16_t)0x0100)              /*!< Synchronous mode enable */
/* EUSCI_A_CTLW0[MODE] Bits */
#define EUSCI_A_CTLW0_MODE_OFS                   ( 9)                            /*!< UCMODE Bit Offset */
#define EUSCI_A_CTLW0_MODE_MASK                  ((uint16_t)0x0600)              /*!< UCMODE Bit Mask */
#define EUSCI_A_CTLW0_MODE0                      ((uint16_t)0x0200)              /*!< MODE Bit 0 */
#define EUSCI_A_CTLW0_MODE1                      ((uint16_t)0x0400)              /*!< MODE Bit 1 */
#define EUSCI_A_CTLW0_MODE_0                     ((uint16_t)0x0000)              /*!< UART mode */
#define EUSCI_A_CTLW0_MODE_1                     ((uint16_t)0x0200)              /*!< Idle-line multiprocessor mode */
#define EUSCI_A_CTLW0_MODE_2                     ((uint16_t)0x0400)              /*!< Address-bit multiprocessor mode */
#define EUSCI_A_CTLW0_MODE_3                     ((uint16_t)0x0600)              /*!< UART mode with automatic baud-rate detection */
/* EUSCI_A_CTLW0[SPB] Bits */
#define EUSCI_A_CTLW0_SPB_OFS                    (11)                            /*!< UCSPB Bit Offset */
#define EUSCI_A_CTLW0_SPB                        ((uint16_t)0x0800)              /*!< Stop bit select */
/* EUSCI_A_CTLW0[SEVENBIT] Bits */
#define EUSCI_A_CTLW0_SEVENBIT_OFS               (12)                            /*!< UC7BIT Bit Offset */
#define EUSCI_A_CTLW0_SEVENBIT                   ((uint16_t)0x1000)              /*!< Character length */
/* EUSCI_A_CTLW0[MSB] Bits */
#define EUSCI_A_CTLW0_MSB_OFS                    (13)                            /*!< UCMSB Bit Offset */
#define EUSCI_A_CTLW0_MSB                        ((uint16_t)0x2000)              /*!< MSB first select */
/* EUSCI_A_CTLW0[PAR] Bits */
#define EUSCI_A_CTLW0_PAR_OFS                    (14)                            /*!< UCPAR Bit Offset */
#define EUSCI_A_CTLW0_PAR                        ((uint16_t)0x4000)              /*!< Parity select */
/* EUSCI_A_CTLW0[PEN] Bits */
#define EUSCI_A_CTLW0_PEN_OFS                    (15)                            /*!< UCPEN Bit Offset */
#define EUSCI_A_CTLW0_PEN                        ((uint16_t)0x8000)              /*!< Parity enable */
/* EUSCI_A_CTLW0[STEM] Bits */
#define EUSCI_A_CTLW0_STEM_OFS                   ( 1)                            /*!< UCSTEM Bit Offset */
#define EUSCI_A_CTLW0_STEM                       ((uint16_t)0x0002)              /*!< STE mode select in master mode. */
/* EUSCI_A_CTLW0[MST] Bits */
#define EUSCI_A_CTLW0_MST_OFS                    (11)                            /*!< UCMST Bit Offset */
#define EUSCI_A_CTLW0_MST                        ((uint16_t)0x0800)              /*!< Master mode select */
/* EUSCI_A_CTLW0[CKPL] Bits */
#define EUSCI_A_CTLW0_CKPL_OFS                   (14)                            /*!< UCCKPL Bit Offset */
#define EUSCI_A_CTLW0_CKPL                       ((uint16_t)0x4000)              /*!< Clock polarity select */
/* EUSCI_A_CTLW0[CKPH] Bits */
#define EUSCI_A_CTLW0_CKPH_OFS                   (15)                            /*!< UCCKPH Bit Offset */
#define EUSCI_A_CTLW0_CKPH                       ((uint16_t)0x8000)              /*!< Clock phase select */
/* EUSCI_A_CTLW1[GLIT] Bits */
#define EUSCI_A_CTLW1_GLIT_OFS                   ( 0)                            /*!< UCGLIT Bit Offset */
#define EUSCI_A_CTLW1_GLIT_MASK                  ((uint16_t)0x0003)              /*!< UCGLIT Bit Mask */
#define EUSCI_A_CTLW1_GLIT0                      ((uint16_t)0x0001)              /*!< GLIT Bit 0 */
#define EUSCI_A_CTLW1_GLIT1                      ((uint16_t)0x0002)              /*!< GLIT Bit 1 */
#define EUSCI_A_CTLW1_GLIT_0                     ((uint16_t)0x0000)              /*!< Approximately 2 ns (equivalent of 1 delay element) */
#define EUSCI_A_CTLW1_GLIT_1                     ((uint16_t)0x0001)              /*!< Approximately 50 ns */
#define EUSCI_A_CTLW1_GLIT_2                     ((uint16_t)0x0002)              /*!< Approximately 100 ns */
#define EUSCI_A_CTLW1_GLIT_3                     ((uint16_t)0x0003)              /*!< Approximately 200 ns */
/* EUSCI_A_MCTLW[OS16] Bits */
#define EUSCI_A_MCTLW_OS16_OFS                   ( 0)                            /*!< UCOS16 Bit Offset */
#define EUSCI_A_MCTLW_OS16                       ((uint16_t)0x0001)              /*!< Oversampling mode enabled */
/* EUSCI_A_MCTLW[BRF] Bits */
#define EUSCI_A_MCTLW_BRF_OFS                    ( 4)                            /*!< UCBRF Bit Offset */
#define EUSCI_A_MCTLW_BRF_MASK                   ((uint16_t)0x00F0)              /*!< UCBRF Bit Mask */
/* EUSCI_A_MCTLW[BRS] Bits */
#define EUSCI_A_MCTLW_BRS_OFS                    ( 8)                            /*!< UCBRS Bit Offset */
#define EUSCI_A_MCTLW_BRS_MASK                   ((uint16_t)0xFF00)              /*!< UCBRS Bit Mask */
/* EUSCI_A_STATW[BUSY] Bits */
#define EUSCI_A_STATW_BUSY_OFS                   ( 0)                            /*!< UCBUSY Bit Offset */
#define EUSCI_A_STATW_BUSY                       ((uint16_t)0x0001)              /*!< eUSCI_A busy */
/* EUSCI_A_STATW[ADDR_IDLE] Bits */
#define EUSCI_A_STATW_ADDR_IDLE_OFS              ( 1)                            /*!< UCADDR_UCIDLE Bit Offset */
#define EUSCI_A_STATW_ADDR_IDLE                  ((uint16_t)0x0002)              /*!< Address received / Idle line detected */
/* EUSCI_A_STATW[RXERR] Bits */
#define EUSCI_A_STATW_RXERR_OFS                  ( 2)                            /*!< UCRXERR Bit Offset */
#define EUSCI_A_STATW_RXERR                      ((uint16_t)0x0004)              /*!< Receive error flag */
/* EUSCI_A_STATW[BRK] Bits */
#define EUSCI_A_STATW_BRK_OFS                    ( 3)                            /*!< UCBRK Bit Offset */
#define EUSCI_A_STATW_BRK                        ((uint16_t)0x0008)              /*!< Break detect flag */
/* EUSCI_A_STATW[PE] Bits */
#define EUSCI_A_STATW_PE_OFS                     ( 4)                            /*!< UCPE Bit Offset */
#define EUSCI_A_STATW_PE                         ((uint16_t)0x0010)              
/* EUSCI_A_STATW[OE] Bits */
#define EUSCI_A_STATW_OE_OFS                     ( 5)                            /*!< UCOE Bit Offset */
#define EUSCI_A_STATW_OE                         ((uint16_t)0x0020)              /*!< Overrun error flag */
/* EUSCI_A_STATW[FE] Bits */
#define EUSCI_A_STATW_FE_OFS                     ( 6)                            /*!< UCFE Bit Offset */
#define EUSCI_A_STATW_FE                         ((uint16_t)0x0040)              /*!< Framing error flag */
/* EUSCI_A_STATW[LISTEN] Bits */
#define EUSCI_A_STATW_LISTEN_OFS                 ( 7)                            /*!< UCLISTEN Bit Offset */
#define EUSCI_A_STATW_LISTEN                     ((uint16_t)0x0080)              /*!< Listen enable */
/* EUSCI_A_STATW[SPI_BUSY] Bits */
#define EUSCI_A_STATW_SPI_BUSY_OFS               ( 0)                            /*!< UCBUSY Bit Offset */
#define EUSCI_A_STATW_SPI_BUSY                   ((uint16_t)0x0001)              /*!< eUSCI_A busy */
/* EUSCI_A_RXBUF[RXBUF] Bits */
#define EUSCI_A_RXBUF_RXBUF_OFS                  ( 0)                            /*!< UCRXBUF Bit Offset */
#define EUSCI_A_RXBUF_RXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCRXBUF Bit Mask */
/* EUSCI_A_TXBUF[TXBUF] Bits */
#define EUSCI_A_TXBUF_TXBUF_OFS                  ( 0)                            /*!< UCTXBUF Bit Offset */
#define EUSCI_A_TXBUF_TXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCTXBUF Bit Mask */
/* EUSCI_A_ABCTL[ABDEN] Bits */
#define EUSCI_A_ABCTL_ABDEN_OFS                  ( 0)                            /*!< UCABDEN Bit Offset */
#define EUSCI_A_ABCTL_ABDEN                      ((uint16_t)0x0001)              /*!< Automatic baud-rate detect enable */
/* EUSCI_A_ABCTL[BTOE] Bits */
#define EUSCI_A_ABCTL_BTOE_OFS                   ( 2)                            /*!< UCBTOE Bit Offset */
#define EUSCI_A_ABCTL_BTOE                       ((uint16_t)0x0004)              /*!< Break time out error */
/* EUSCI_A_ABCTL[STOE] Bits */
#define EUSCI_A_ABCTL_STOE_OFS                   ( 3)                            /*!< UCSTOE Bit Offset */
#define EUSCI_A_ABCTL_STOE                       ((uint16_t)0x0008)              /*!< Synch field time out error */
/* EUSCI_A_ABCTL[DELIM] Bits */
#define EUSCI_A_ABCTL_DELIM_OFS                  ( 4)                            /*!< UCDELIM Bit Offset */
#define EUSCI_A_ABCTL_DELIM_MASK                 ((uint16_t)0x0030)              /*!< UCDELIM Bit Mask */
#define EUSCI_A_ABCTL_DELIM0                     ((uint16_t)0x0010)              /*!< DELIM Bit 0 */
#define EUSCI_A_ABCTL_DELIM1                     ((uint16_t)0x0020)              /*!< DELIM Bit 1 */
#define EUSCI_A_ABCTL_DELIM_0                    ((uint16_t)0x0000)              /*!< 1 bit time */
#define EUSCI_A_ABCTL_DELIM_1                    ((uint16_t)0x0010)              /*!< 2 bit times */
#define EUSCI_A_ABCTL_DELIM_2                    ((uint16_t)0x0020)              /*!< 3 bit times */
#define EUSCI_A_ABCTL_DELIM_3                    ((uint16_t)0x0030)              /*!< 4 bit times */
/* EUSCI_A_IRCTL[IREN] Bits */
#define EUSCI_A_IRCTL_IREN_OFS                   ( 0)                            /*!< UCIREN Bit Offset */
#define EUSCI_A_IRCTL_IREN                       ((uint16_t)0x0001)              /*!< IrDA encoder/decoder enable */
/* EUSCI_A_IRCTL[IRTXCLK] Bits */
#define EUSCI_A_IRCTL_IRTXCLK_OFS                ( 1)                            /*!< UCIRTXCLK Bit Offset */
#define EUSCI_A_IRCTL_IRTXCLK                    ((uint16_t)0x0002)              /*!< IrDA transmit pulse clock select */
/* EUSCI_A_IRCTL[IRTXPL] Bits */
#define EUSCI_A_IRCTL_IRTXPL_OFS                 ( 2)                            /*!< UCIRTXPL Bit Offset */
#define EUSCI_A_IRCTL_IRTXPL_MASK                ((uint16_t)0x00FC)              /*!< UCIRTXPL Bit Mask */
/* EUSCI_A_IRCTL[IRRXFE] Bits */
#define EUSCI_A_IRCTL_IRRXFE_OFS                 ( 8)                            /*!< UCIRRXFE Bit Offset */
#define EUSCI_A_IRCTL_IRRXFE                     ((uint16_t)0x0100)              /*!< IrDA receive filter enabled */
/* EUSCI_A_IRCTL[IRRXPL] Bits */
#define EUSCI_A_IRCTL_IRRXPL_OFS                 ( 9)                            /*!< UCIRRXPL Bit Offset */
#define EUSCI_A_IRCTL_IRRXPL                     ((uint16_t)0x0200)              /*!< IrDA receive input UCAxRXD polarity */
/* EUSCI_A_IRCTL[IRRXFL] Bits */
#define EUSCI_A_IRCTL_IRRXFL_OFS                 (10)                            /*!< UCIRRXFL Bit Offset */
#define EUSCI_A_IRCTL_IRRXFL_MASK                ((uint16_t)0x3C00)              /*!< UCIRRXFL Bit Mask */
/* EUSCI_A_IE[RXIE] Bits */
#define EUSCI_A_IE_RXIE_OFS                      ( 0)                            /*!< UCRXIE Bit Offset */
#define EUSCI_A_IE_RXIE                          ((uint16_t)0x0001)              /*!< Receive interrupt enable */
/* EUSCI_A_IE[TXIE] Bits */
#define EUSCI_A_IE_TXIE_OFS                      ( 1)                            /*!< UCTXIE Bit Offset */
#define EUSCI_A_IE_TXIE                          ((uint16_t)0x0002)              /*!< Transmit interrupt enable */
/* EUSCI_A_IE[STTIE] Bits */
#define EUSCI_A_IE_STTIE_OFS                     ( 2)                            /*!< UCSTTIE Bit Offset */
#define EUSCI_A_IE_STTIE                         ((uint16_t)0x0004)              /*!< Start bit interrupt enable */
/* EUSCI_A_IE[TXCPTIE] Bits */
#define EUSCI_A_IE_TXCPTIE_OFS                   ( 3)                            /*!< UCTXCPTIE Bit Offset */
#define EUSCI_A_IE_TXCPTIE                       ((uint16_t)0x0008)              /*!< Transmit complete interrupt enable */
/* EUSCI_A_IFG[RXIFG] Bits */
#define EUSCI_A_IFG_RXIFG_OFS                    ( 0)                            /*!< UCRXIFG Bit Offset */
#define EUSCI_A_IFG_RXIFG                        ((uint16_t)0x0001)              /*!< Receive interrupt flag */
/* EUSCI_A_IFG[TXIFG] Bits */
#define EUSCI_A_IFG_TXIFG_OFS                    ( 1)                            /*!< UCTXIFG Bit Offset */
#define EUSCI_A_IFG_TXIFG                        ((uint16_t)0x0002)              /*!< Transmit interrupt flag */
/* EUSCI_A_IFG[STTIFG] Bits */
#define EUSCI_A_IFG_STTIFG_OFS                   ( 2)                            /*!< UCSTTIFG Bit Offset */
#define EUSCI_A_IFG_STTIFG                       ((uint16_t)0x0004)              /*!< Start bit interrupt flag */
/* EUSCI_A_IFG[TXCPTIFG] Bits */
#define EUSCI_A_IFG_TXCPTIFG_OFS                 ( 3)                            /*!< UCTXCPTIFG Bit Offset */
#define EUSCI_A_IFG_TXCPTIFG                     ((uint16_t)0x0008)              /*!< Transmit ready interrupt enable */
/* legacy definitions for backward compatibility to version 2100 */
#define EUSCI_A__RXIE_OFS                        EUSCI_A_IE_RXIE_OFS             /*!< UCRXIE Bit Offset */
#define EUSCI_A__RXIE                            EUSCI_A_IE_RXIE                 /*!< Receive interrupt enable */
#define EUSCI_A__TXIE_OFS                        EUSCI_A_IE_TXIE_OFS             /*!< UCTXIE Bit Offset */
#define EUSCI_A__TXIE                            EUSCI_A_IE_TXIE                 /*!< Transmit interrupt enable */


/******************************************************************************
* EUSCI_B Bits
******************************************************************************/
/* EUSCI_B_CTLW0[SWRST] Bits */
#define EUSCI_B_CTLW0_SWRST_OFS                  ( 0)                            /*!< UCSWRST Bit Offset */
#define EUSCI_B_CTLW0_SWRST                      ((uint16_t)0x0001)              /*!< Software reset enable */
/* EUSCI_B_CTLW0[TXSTT] Bits */
#define EUSCI_B_CTLW0_TXSTT_OFS                  ( 1)                            /*!< UCTXSTT Bit Offset */
#define EUSCI_B_CTLW0_TXSTT                      ((uint16_t)0x0002)              /*!< Transmit START condition in master mode */
/* EUSCI_B_CTLW0[TXSTP] Bits */
#define EUSCI_B_CTLW0_TXSTP_OFS                  ( 2)                            /*!< UCTXSTP Bit Offset */
#define EUSCI_B_CTLW0_TXSTP                      ((uint16_t)0x0004)              /*!< Transmit STOP condition in master mode */
/* EUSCI_B_CTLW0[TXNACK] Bits */
#define EUSCI_B_CTLW0_TXNACK_OFS                 ( 3)                            /*!< UCTXNACK Bit Offset */
#define EUSCI_B_CTLW0_TXNACK                     ((uint16_t)0x0008)              /*!< Transmit a NACK */
/* EUSCI_B_CTLW0[TR] Bits */
#define EUSCI_B_CTLW0_TR_OFS                     ( 4)                            /*!< UCTR Bit Offset */
#define EUSCI_B_CTLW0_TR                         ((uint16_t)0x0010)              /*!< Transmitter/receiver */
/* EUSCI_B_CTLW0[TXACK] Bits */
#define EUSCI_B_CTLW0_TXACK_OFS                  ( 5)                            /*!< UCTXACK Bit Offset */
#define EUSCI_B_CTLW0_TXACK                      ((uint16_t)0x0020)              /*!< Transmit ACK condition in slave mode */
/* EUSCI_B_CTLW0[SSEL] Bits */
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
/* EUSCI_B_CTLW0[SYNC] Bits */
#define EUSCI_B_CTLW0_SYNC_OFS                   ( 8)                            /*!< UCSYNC Bit Offset */
#define EUSCI_B_CTLW0_SYNC                       ((uint16_t)0x0100)              /*!< Synchronous mode enable */
/* EUSCI_B_CTLW0[MODE] Bits */
#define EUSCI_B_CTLW0_MODE_OFS                   ( 9)                            /*!< UCMODE Bit Offset */
#define EUSCI_B_CTLW0_MODE_MASK                  ((uint16_t)0x0600)              /*!< UCMODE Bit Mask */
#define EUSCI_B_CTLW0_MODE0                      ((uint16_t)0x0200)              /*!< MODE Bit 0 */
#define EUSCI_B_CTLW0_MODE1                      ((uint16_t)0x0400)              /*!< MODE Bit 1 */
#define EUSCI_B_CTLW0_MODE_0                     ((uint16_t)0x0000)              /*!< 3-pin SPI */
#define EUSCI_B_CTLW0_MODE_1                     ((uint16_t)0x0200)              /*!< 4-pin SPI (master or slave enabled if STE = 1) */
#define EUSCI_B_CTLW0_MODE_2                     ((uint16_t)0x0400)              /*!< 4-pin SPI (master or slave enabled if STE = 0) */
#define EUSCI_B_CTLW0_MODE_3                     ((uint16_t)0x0600)              /*!< I2C mode */
/* EUSCI_B_CTLW0[MST] Bits */
#define EUSCI_B_CTLW0_MST_OFS                    (11)                            /*!< UCMST Bit Offset */
#define EUSCI_B_CTLW0_MST                        ((uint16_t)0x0800)              /*!< Master mode select */
/* EUSCI_B_CTLW0[MM] Bits */
#define EUSCI_B_CTLW0_MM_OFS                     (13)                            /*!< UCMM Bit Offset */
#define EUSCI_B_CTLW0_MM                         ((uint16_t)0x2000)              /*!< Multi-master environment select */
/* EUSCI_B_CTLW0[SLA10] Bits */
#define EUSCI_B_CTLW0_SLA10_OFS                  (14)                            /*!< UCSLA10 Bit Offset */
#define EUSCI_B_CTLW0_SLA10                      ((uint16_t)0x4000)              /*!< Slave addressing mode select */
/* EUSCI_B_CTLW0[A10] Bits */
#define EUSCI_B_CTLW0_A10_OFS                    (15)                            /*!< UCA10 Bit Offset */
#define EUSCI_B_CTLW0_A10                        ((uint16_t)0x8000)              /*!< Own addressing mode select */
/* EUSCI_B_CTLW0[STEM] Bits */
#define EUSCI_B_CTLW0_STEM_OFS                   ( 1)                            /*!< UCSTEM Bit Offset */
#define EUSCI_B_CTLW0_STEM                       ((uint16_t)0x0002)              /*!< STE mode select in master mode. */
/* EUSCI_B_CTLW0[SEVENBIT] Bits */
#define EUSCI_B_CTLW0_SEVENBIT_OFS               (12)                            /*!< UC7BIT Bit Offset */
#define EUSCI_B_CTLW0_SEVENBIT                   ((uint16_t)0x1000)              /*!< Character length */
/* EUSCI_B_CTLW0[MSB] Bits */
#define EUSCI_B_CTLW0_MSB_OFS                    (13)                            /*!< UCMSB Bit Offset */
#define EUSCI_B_CTLW0_MSB                        ((uint16_t)0x2000)              /*!< MSB first select */
/* EUSCI_B_CTLW0[CKPL] Bits */
#define EUSCI_B_CTLW0_CKPL_OFS                   (14)                            /*!< UCCKPL Bit Offset */
#define EUSCI_B_CTLW0_CKPL                       ((uint16_t)0x4000)              /*!< Clock polarity select */
/* EUSCI_B_CTLW0[CKPH] Bits */
#define EUSCI_B_CTLW0_CKPH_OFS                   (15)                            /*!< UCCKPH Bit Offset */
#define EUSCI_B_CTLW0_CKPH                       ((uint16_t)0x8000)              /*!< Clock phase select */
/* EUSCI_B_CTLW1[GLIT] Bits */
#define EUSCI_B_CTLW1_GLIT_OFS                   ( 0)                            /*!< UCGLIT Bit Offset */
#define EUSCI_B_CTLW1_GLIT_MASK                  ((uint16_t)0x0003)              /*!< UCGLIT Bit Mask */
#define EUSCI_B_CTLW1_GLIT0                      ((uint16_t)0x0001)              /*!< GLIT Bit 0 */
#define EUSCI_B_CTLW1_GLIT1                      ((uint16_t)0x0002)              /*!< GLIT Bit 1 */
#define EUSCI_B_CTLW1_GLIT_0                     ((uint16_t)0x0000)              /*!< 50 ns */
#define EUSCI_B_CTLW1_GLIT_1                     ((uint16_t)0x0001)              /*!< 25 ns */
#define EUSCI_B_CTLW1_GLIT_2                     ((uint16_t)0x0002)              /*!< 12.5 ns */
#define EUSCI_B_CTLW1_GLIT_3                     ((uint16_t)0x0003)              /*!< 6.25 ns */
/* EUSCI_B_CTLW1[ASTP] Bits */
#define EUSCI_B_CTLW1_ASTP_OFS                   ( 2)                            /*!< UCASTP Bit Offset */
#define EUSCI_B_CTLW1_ASTP_MASK                  ((uint16_t)0x000C)              /*!< UCASTP Bit Mask */
#define EUSCI_B_CTLW1_ASTP0                      ((uint16_t)0x0004)              /*!< ASTP Bit 0 */
#define EUSCI_B_CTLW1_ASTP1                      ((uint16_t)0x0008)              /*!< ASTP Bit 1 */
#define EUSCI_B_CTLW1_ASTP_0                     ((uint16_t)0x0000)              /*!< No automatic STOP generation. The STOP condition is generated after the user  */
                                                                                 /* sets the UCTXSTP bit. The value in UCBxTBCNT is a don't care. */
#define EUSCI_B_CTLW1_ASTP_1                     ((uint16_t)0x0004)              /*!< UCBCNTIFG is set with the byte counter reaches the threshold defined in  */
                                                                                 /* UCBxTBCNT */
#define EUSCI_B_CTLW1_ASTP_2                     ((uint16_t)0x0008)              /*!< A STOP condition is generated automatically after the byte counter value  */
                                                                                 /* reached UCBxTBCNT. UCBCNTIFG is set with the byte counter reaching the */
                                                                                 /* threshold */
/* EUSCI_B_CTLW1[SWACK] Bits */
#define EUSCI_B_CTLW1_SWACK_OFS                  ( 4)                            /*!< UCSWACK Bit Offset */
#define EUSCI_B_CTLW1_SWACK                      ((uint16_t)0x0010)              /*!< SW or HW ACK control */
/* EUSCI_B_CTLW1[STPNACK] Bits */
#define EUSCI_B_CTLW1_STPNACK_OFS                ( 5)                            /*!< UCSTPNACK Bit Offset */
#define EUSCI_B_CTLW1_STPNACK                    ((uint16_t)0x0020)              /*!< ACK all master bytes */
/* EUSCI_B_CTLW1[CLTO] Bits */
#define EUSCI_B_CTLW1_CLTO_OFS                   ( 6)                            /*!< UCCLTO Bit Offset */
#define EUSCI_B_CTLW1_CLTO_MASK                  ((uint16_t)0x00C0)              /*!< UCCLTO Bit Mask */
#define EUSCI_B_CTLW1_CLTO0                      ((uint16_t)0x0040)              /*!< CLTO Bit 0 */
#define EUSCI_B_CTLW1_CLTO1                      ((uint16_t)0x0080)              /*!< CLTO Bit 1 */
#define EUSCI_B_CTLW1_CLTO_0                     ((uint16_t)0x0000)              /*!< Disable clock low timeout counter */
#define EUSCI_B_CTLW1_CLTO_1                     ((uint16_t)0x0040)              /*!< 135 000 SYSCLK cycles (approximately 28 ms) */
#define EUSCI_B_CTLW1_CLTO_2                     ((uint16_t)0x0080)              /*!< 150 000 SYSCLK cycles (approximately 31 ms) */
#define EUSCI_B_CTLW1_CLTO_3                     ((uint16_t)0x00C0)              /*!< 165 000 SYSCLK cycles (approximately 34 ms) */
/* EUSCI_B_CTLW1[ETXINT] Bits */
#define EUSCI_B_CTLW1_ETXINT_OFS                 ( 8)                            /*!< UCETXINT Bit Offset */
#define EUSCI_B_CTLW1_ETXINT                     ((uint16_t)0x0100)              /*!< Early UCTXIFG0 */
/* EUSCI_B_STATW[BBUSY] Bits */
#define EUSCI_B_STATW_BBUSY_OFS                  ( 4)                            /*!< UCBBUSY Bit Offset */
#define EUSCI_B_STATW_BBUSY                      ((uint16_t)0x0010)              /*!< Bus busy */
/* EUSCI_B_STATW[GC] Bits */
#define EUSCI_B_STATW_GC_OFS                     ( 5)                            /*!< UCGC Bit Offset */
#define EUSCI_B_STATW_GC                         ((uint16_t)0x0020)              /*!< General call address received */
/* EUSCI_B_STATW[SCLLOW] Bits */
#define EUSCI_B_STATW_SCLLOW_OFS                 ( 6)                            /*!< UCSCLLOW Bit Offset */
#define EUSCI_B_STATW_SCLLOW                     ((uint16_t)0x0040)              /*!< SCL low */
/* EUSCI_B_STATW[BCNT] Bits */
#define EUSCI_B_STATW_BCNT_OFS                   ( 8)                            /*!< UCBCNT Bit Offset */
#define EUSCI_B_STATW_BCNT_MASK                  ((uint16_t)0xFF00)              /*!< UCBCNT Bit Mask */
/* EUSCI_B_STATW[SPI_BUSY] Bits */
#define EUSCI_B_STATW_SPI_BUSY_OFS               ( 0)                            /*!< UCBUSY Bit Offset */
#define EUSCI_B_STATW_SPI_BUSY                   ((uint16_t)0x0001)              /*!< eUSCI_B busy */
/* EUSCI_B_STATW[OE] Bits */
#define EUSCI_B_STATW_OE_OFS                     ( 5)                            /*!< UCOE Bit Offset */
#define EUSCI_B_STATW_OE                         ((uint16_t)0x0020)              /*!< Overrun error flag */
/* EUSCI_B_STATW[FE] Bits */
#define EUSCI_B_STATW_FE_OFS                     ( 6)                            /*!< UCFE Bit Offset */
#define EUSCI_B_STATW_FE                         ((uint16_t)0x0040)              /*!< Framing error flag */
/* EUSCI_B_STATW[LISTEN] Bits */
#define EUSCI_B_STATW_LISTEN_OFS                 ( 7)                            /*!< UCLISTEN Bit Offset */
#define EUSCI_B_STATW_LISTEN                     ((uint16_t)0x0080)              /*!< Listen enable */
/* EUSCI_B_TBCNT[TBCNT] Bits */
#define EUSCI_B_TBCNT_TBCNT_OFS                  ( 0)                            /*!< UCTBCNT Bit Offset */
#define EUSCI_B_TBCNT_TBCNT_MASK                 ((uint16_t)0x00FF)              /*!< UCTBCNT Bit Mask */
/* EUSCI_B_RXBUF[RXBUF] Bits */
#define EUSCI_B_RXBUF_RXBUF_OFS                  ( 0)                            /*!< UCRXBUF Bit Offset */
#define EUSCI_B_RXBUF_RXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCRXBUF Bit Mask */
/* EUSCI_B_TXBUF[TXBUF] Bits */
#define EUSCI_B_TXBUF_TXBUF_OFS                  ( 0)                            /*!< UCTXBUF Bit Offset */
#define EUSCI_B_TXBUF_TXBUF_MASK                 ((uint16_t)0x00FF)              /*!< UCTXBUF Bit Mask */
/* EUSCI_B_I2COA0[I2COA0] Bits */
#define EUSCI_B_I2COA0_I2COA0_OFS                ( 0)                            /*!< I2COA0 Bit Offset */
#define EUSCI_B_I2COA0_I2COA0_MASK               ((uint16_t)0x03FF)              /*!< I2COA0 Bit Mask */
/* EUSCI_B_I2COA0[OAEN] Bits */
#define EUSCI_B_I2COA0_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA0_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
/* EUSCI_B_I2COA0[GCEN] Bits */
#define EUSCI_B_I2COA0_GCEN_OFS                  (15)                            /*!< UCGCEN Bit Offset */
#define EUSCI_B_I2COA0_GCEN                      ((uint16_t)0x8000)              /*!< General call response enable */
/* EUSCI_B_I2COA1[I2COA1] Bits */
#define EUSCI_B_I2COA1_I2COA1_OFS                ( 0)                            /*!< I2COA1 Bit Offset */
#define EUSCI_B_I2COA1_I2COA1_MASK               ((uint16_t)0x03FF)              /*!< I2COA1 Bit Mask */
/* EUSCI_B_I2COA1[OAEN] Bits */
#define EUSCI_B_I2COA1_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA1_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
/* EUSCI_B_I2COA2[I2COA2] Bits */
#define EUSCI_B_I2COA2_I2COA2_OFS                ( 0)                            /*!< I2COA2 Bit Offset */
#define EUSCI_B_I2COA2_I2COA2_MASK               ((uint16_t)0x03FF)              /*!< I2COA2 Bit Mask */
/* EUSCI_B_I2COA2[OAEN] Bits */
#define EUSCI_B_I2COA2_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA2_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
/* EUSCI_B_I2COA3[I2COA3] Bits */
#define EUSCI_B_I2COA3_I2COA3_OFS                ( 0)                            /*!< I2COA3 Bit Offset */
#define EUSCI_B_I2COA3_I2COA3_MASK               ((uint16_t)0x03FF)              /*!< I2COA3 Bit Mask */
/* EUSCI_B_I2COA3[OAEN] Bits */
#define EUSCI_B_I2COA3_OAEN_OFS                  (10)                            /*!< UCOAEN Bit Offset */
#define EUSCI_B_I2COA3_OAEN                      ((uint16_t)0x0400)              /*!< Own Address enable register */
/* EUSCI_B_ADDRX[ADDRX] Bits */
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
/* EUSCI_B_ADDMASK[ADDMASK] Bits */
#define EUSCI_B_ADDMASK_ADDMASK_OFS              ( 0)                            /*!< ADDMASK Bit Offset */
#define EUSCI_B_ADDMASK_ADDMASK_MASK             ((uint16_t)0x03FF)              /*!< ADDMASK Bit Mask */
/* EUSCI_B_I2CSA[I2CSA] Bits */
#define EUSCI_B_I2CSA_I2CSA_OFS                  ( 0)                            /*!< I2CSA Bit Offset */
#define EUSCI_B_I2CSA_I2CSA_MASK                 ((uint16_t)0x03FF)              /*!< I2CSA Bit Mask */
/* EUSCI_B_IE[RXIE0] Bits */
#define EUSCI_B_IE_RXIE0_OFS                     ( 0)                            /*!< UCRXIE0 Bit Offset */
#define EUSCI_B_IE_RXIE0                         ((uint16_t)0x0001)              /*!< Receive interrupt enable 0 */
/* EUSCI_B_IE[TXIE0] Bits */
#define EUSCI_B_IE_TXIE0_OFS                     ( 1)                            /*!< UCTXIE0 Bit Offset */
#define EUSCI_B_IE_TXIE0                         ((uint16_t)0x0002)              /*!< Transmit interrupt enable 0 */
/* EUSCI_B_IE[STTIE] Bits */
#define EUSCI_B_IE_STTIE_OFS                     ( 2)                            /*!< UCSTTIE Bit Offset */
#define EUSCI_B_IE_STTIE                         ((uint16_t)0x0004)              /*!< START condition interrupt enable */
/* EUSCI_B_IE[STPIE] Bits */
#define EUSCI_B_IE_STPIE_OFS                     ( 3)                            /*!< UCSTPIE Bit Offset */
#define EUSCI_B_IE_STPIE                         ((uint16_t)0x0008)              /*!< STOP condition interrupt enable */
/* EUSCI_B_IE[ALIE] Bits */
#define EUSCI_B_IE_ALIE_OFS                      ( 4)                            /*!< UCALIE Bit Offset */
#define EUSCI_B_IE_ALIE                          ((uint16_t)0x0010)              /*!< Arbitration lost interrupt enable */
/* EUSCI_B_IE[NACKIE] Bits */
#define EUSCI_B_IE_NACKIE_OFS                    ( 5)                            /*!< UCNACKIE Bit Offset */
#define EUSCI_B_IE_NACKIE                        ((uint16_t)0x0020)              /*!< Not-acknowledge interrupt enable */
/* EUSCI_B_IE[BCNTIE] Bits */
#define EUSCI_B_IE_BCNTIE_OFS                    ( 6)                            /*!< UCBCNTIE Bit Offset */
#define EUSCI_B_IE_BCNTIE                        ((uint16_t)0x0040)              /*!< Byte counter interrupt enable */
/* EUSCI_B_IE[CLTOIE] Bits */
#define EUSCI_B_IE_CLTOIE_OFS                    ( 7)                            /*!< UCCLTOIE Bit Offset */
#define EUSCI_B_IE_CLTOIE                        ((uint16_t)0x0080)              /*!< Clock low timeout interrupt enable */
/* EUSCI_B_IE[RXIE1] Bits */
#define EUSCI_B_IE_RXIE1_OFS                     ( 8)                            /*!< UCRXIE1 Bit Offset */
#define EUSCI_B_IE_RXIE1                         ((uint16_t)0x0100)              /*!< Receive interrupt enable 1 */
/* EUSCI_B_IE[TXIE1] Bits */
#define EUSCI_B_IE_TXIE1_OFS                     ( 9)                            /*!< UCTXIE1 Bit Offset */
#define EUSCI_B_IE_TXIE1                         ((uint16_t)0x0200)              /*!< Transmit interrupt enable 1 */
/* EUSCI_B_IE[RXIE2] Bits */
#define EUSCI_B_IE_RXIE2_OFS                     (10)                            /*!< UCRXIE2 Bit Offset */
#define EUSCI_B_IE_RXIE2                         ((uint16_t)0x0400)              /*!< Receive interrupt enable 2 */
/* EUSCI_B_IE[TXIE2] Bits */
#define EUSCI_B_IE_TXIE2_OFS                     (11)                            /*!< UCTXIE2 Bit Offset */
#define EUSCI_B_IE_TXIE2                         ((uint16_t)0x0800)              /*!< Transmit interrupt enable 2 */
/* EUSCI_B_IE[RXIE3] Bits */
#define EUSCI_B_IE_RXIE3_OFS                     (12)                            /*!< UCRXIE3 Bit Offset */
#define EUSCI_B_IE_RXIE3                         ((uint16_t)0x1000)              /*!< Receive interrupt enable 3 */
/* EUSCI_B_IE[TXIE3] Bits */
#define EUSCI_B_IE_TXIE3_OFS                     (13)                            /*!< UCTXIE3 Bit Offset */
#define EUSCI_B_IE_TXIE3                         ((uint16_t)0x2000)              /*!< Transmit interrupt enable 3 */
/* EUSCI_B_IE[BIT9IE] Bits */
#define EUSCI_B_IE_BIT9IE_OFS                    (14)                            /*!< UCBIT9IE Bit Offset */
#define EUSCI_B_IE_BIT9IE                        ((uint16_t)0x4000)              /*!< Bit position 9 interrupt enable */
/* EUSCI_B_IE[RXIE] Bits */
#define EUSCI_B_IE_RXIE_OFS                      ( 0)                            /*!< UCRXIE Bit Offset */
#define EUSCI_B_IE_RXIE                          ((uint16_t)0x0001)              /*!< Receive interrupt enable */
/* EUSCI_B_IE[TXIE] Bits */
#define EUSCI_B_IE_TXIE_OFS                      ( 1)                            /*!< UCTXIE Bit Offset */
#define EUSCI_B_IE_TXIE                          ((uint16_t)0x0002)              /*!< Transmit interrupt enable */
/* EUSCI_B_IFG[RXIFG0] Bits */
#define EUSCI_B_IFG_RXIFG0_OFS                   ( 0)                            /*!< UCRXIFG0 Bit Offset */
#define EUSCI_B_IFG_RXIFG0                       ((uint16_t)0x0001)              /*!< eUSCI_B receive interrupt flag 0 */
/* EUSCI_B_IFG[TXIFG0] Bits */
#define EUSCI_B_IFG_TXIFG0_OFS                   ( 1)                            /*!< UCTXIFG0 Bit Offset */
#define EUSCI_B_IFG_TXIFG0                       ((uint16_t)0x0002)              /*!< eUSCI_B transmit interrupt flag 0 */
/* EUSCI_B_IFG[STTIFG] Bits */
#define EUSCI_B_IFG_STTIFG_OFS                   ( 2)                            /*!< UCSTTIFG Bit Offset */
#define EUSCI_B_IFG_STTIFG                       ((uint16_t)0x0004)              /*!< START condition interrupt flag */
/* EUSCI_B_IFG[STPIFG] Bits */
#define EUSCI_B_IFG_STPIFG_OFS                   ( 3)                            /*!< UCSTPIFG Bit Offset */
#define EUSCI_B_IFG_STPIFG                       ((uint16_t)0x0008)              /*!< STOP condition interrupt flag */
/* EUSCI_B_IFG[ALIFG] Bits */
#define EUSCI_B_IFG_ALIFG_OFS                    ( 4)                            /*!< UCALIFG Bit Offset */
#define EUSCI_B_IFG_ALIFG                        ((uint16_t)0x0010)              /*!< Arbitration lost interrupt flag */
/* EUSCI_B_IFG[NACKIFG] Bits */
#define EUSCI_B_IFG_NACKIFG_OFS                  ( 5)                            /*!< UCNACKIFG Bit Offset */
#define EUSCI_B_IFG_NACKIFG                      ((uint16_t)0x0020)              /*!< Not-acknowledge received interrupt flag */
/* EUSCI_B_IFG[BCNTIFG] Bits */
#define EUSCI_B_IFG_BCNTIFG_OFS                  ( 6)                            /*!< UCBCNTIFG Bit Offset */
#define EUSCI_B_IFG_BCNTIFG                      ((uint16_t)0x0040)              /*!< Byte counter interrupt flag */
/* EUSCI_B_IFG[CLTOIFG] Bits */
#define EUSCI_B_IFG_CLTOIFG_OFS                  ( 7)                            /*!< UCCLTOIFG Bit Offset */
#define EUSCI_B_IFG_CLTOIFG                      ((uint16_t)0x0080)              /*!< Clock low timeout interrupt flag */
/* EUSCI_B_IFG[RXIFG1] Bits */
#define EUSCI_B_IFG_RXIFG1_OFS                   ( 8)                            /*!< UCRXIFG1 Bit Offset */
#define EUSCI_B_IFG_RXIFG1                       ((uint16_t)0x0100)              /*!< eUSCI_B receive interrupt flag 1 */
/* EUSCI_B_IFG[TXIFG1] Bits */
#define EUSCI_B_IFG_TXIFG1_OFS                   ( 9)                            /*!< UCTXIFG1 Bit Offset */
#define EUSCI_B_IFG_TXIFG1                       ((uint16_t)0x0200)              /*!< eUSCI_B transmit interrupt flag 1 */
/* EUSCI_B_IFG[RXIFG2] Bits */
#define EUSCI_B_IFG_RXIFG2_OFS                   (10)                            /*!< UCRXIFG2 Bit Offset */
#define EUSCI_B_IFG_RXIFG2                       ((uint16_t)0x0400)              /*!< eUSCI_B receive interrupt flag 2 */
/* EUSCI_B_IFG[TXIFG2] Bits */
#define EUSCI_B_IFG_TXIFG2_OFS                   (11)                            /*!< UCTXIFG2 Bit Offset */
#define EUSCI_B_IFG_TXIFG2                       ((uint16_t)0x0800)              /*!< eUSCI_B transmit interrupt flag 2 */
/* EUSCI_B_IFG[RXIFG3] Bits */
#define EUSCI_B_IFG_RXIFG3_OFS                   (12)                            /*!< UCRXIFG3 Bit Offset */
#define EUSCI_B_IFG_RXIFG3                       ((uint16_t)0x1000)              /*!< eUSCI_B receive interrupt flag 3 */
/* EUSCI_B_IFG[TXIFG3] Bits */
#define EUSCI_B_IFG_TXIFG3_OFS                   (13)                            /*!< UCTXIFG3 Bit Offset */
#define EUSCI_B_IFG_TXIFG3                       ((uint16_t)0x2000)              /*!< eUSCI_B transmit interrupt flag 3 */
/* EUSCI_B_IFG[BIT9IFG] Bits */
#define EUSCI_B_IFG_BIT9IFG_OFS                  (14)                            /*!< UCBIT9IFG Bit Offset */
#define EUSCI_B_IFG_BIT9IFG                      ((uint16_t)0x4000)              /*!< Bit position 9 interrupt flag */
/* EUSCI_B_IFG[RXIFG] Bits */
#define EUSCI_B_IFG_RXIFG_OFS                    ( 0)                            /*!< UCRXIFG Bit Offset */
#define EUSCI_B_IFG_RXIFG                        ((uint16_t)0x0001)              /*!< Receive interrupt flag */
/* EUSCI_B_IFG[TXIFG] Bits */
#define EUSCI_B_IFG_TXIFG_OFS                    ( 1)                            /*!< UCTXIFG Bit Offset */
#define EUSCI_B_IFG_TXIFG                        ((uint16_t)0x0002)              /*!< Transmit interrupt flag */
/* legacy definitions for backward compatibility to version 2100 */
#define EUSCI_B__RXIE_OFS                        EUSCI_B_IE_RXIE_OFS             /*!< UCRXIE Bit Offset */
#define EUSCI_B__RXIE                            EUSCI_B_IE_RXIE                 /*!< Receive interrupt enable */
#define EUSCI_B__TXIE_OFS                        EUSCI_B_IE_TXIE_OFS             /*!< UCTXIE Bit Offset */
#define EUSCI_B__TXIE                            EUSCI_B_IE_TXIE                 /*!< Transmit interrupt enable */


/******************************************************************************
* FLCTL Bits
******************************************************************************/
/* FLCTL_POWER_STAT[PSTAT] Bits */
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
/* FLCTL_POWER_STAT[LDOSTAT] Bits */
#define FLCTL_POWER_STAT_LDOSTAT_OFS             ( 3)                            /*!< LDOSTAT Bit Offset */
#define FLCTL_POWER_STAT_LDOSTAT                 ((uint32_t)0x00000008)          /*!< PSS FLDO GOOD status */
/* FLCTL_POWER_STAT[VREFSTAT] Bits */
#define FLCTL_POWER_STAT_VREFSTAT_OFS            ( 4)                            /*!< VREFSTAT Bit Offset */
#define FLCTL_POWER_STAT_VREFSTAT                ((uint32_t)0x00000010)          /*!< PSS VREF stable status */
/* FLCTL_POWER_STAT[IREFSTAT] Bits */
#define FLCTL_POWER_STAT_IREFSTAT_OFS            ( 5)                            /*!< IREFSTAT Bit Offset */
#define FLCTL_POWER_STAT_IREFSTAT                ((uint32_t)0x00000020)          /*!< PSS IREF stable status */
/* FLCTL_POWER_STAT[TRIMSTAT] Bits */
#define FLCTL_POWER_STAT_TRIMSTAT_OFS            ( 6)                            /*!< TRIMSTAT Bit Offset */
#define FLCTL_POWER_STAT_TRIMSTAT                ((uint32_t)0x00000040)          /*!< PSS trim done status */
/* FLCTL_POWER_STAT[RD_2T] Bits */
#define FLCTL_POWER_STAT_RD_2T_OFS               ( 7)                            /*!< RD_2T Bit Offset */
#define FLCTL_POWER_STAT_RD_2T                   ((uint32_t)0x00000080)          /*!< Indicates if Flash is being accessed in 2T mode */
/* FLCTL_BANK0_RDCTL[RD_MODE] Bits */
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
/* FLCTL_BANK0_RDCTL[BUFI] Bits */
#define FLCTL_BANK0_RDCTL_BUFI_OFS               ( 4)                            /*!< BUFI Bit Offset */
#define FLCTL_BANK0_RDCTL_BUFI                   ((uint32_t)0x00000010)          /*!< Enables read buffering feature for instruction fetches to this Bank */
/* FLCTL_BANK0_RDCTL[BUFD] Bits */
#define FLCTL_BANK0_RDCTL_BUFD_OFS               ( 5)                            /*!< BUFD Bit Offset */
#define FLCTL_BANK0_RDCTL_BUFD                   ((uint32_t)0x00000020)          /*!< Enables read buffering feature for data reads to this Bank */
/* FLCTL_BANK0_RDCTL[WAIT] Bits */
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
/* FLCTL_BANK0_RDCTL[RD_MODE_STATUS] Bits */
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
/* FLCTL_BANK1_RDCTL[RD_MODE] Bits */
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
/* FLCTL_BANK1_RDCTL[BUFI] Bits */
#define FLCTL_BANK1_RDCTL_BUFI_OFS               ( 4)                            /*!< BUFI Bit Offset */
#define FLCTL_BANK1_RDCTL_BUFI                   ((uint32_t)0x00000010)          /*!< Enables read buffering feature for instruction fetches to this Bank */
/* FLCTL_BANK1_RDCTL[BUFD] Bits */
#define FLCTL_BANK1_RDCTL_BUFD_OFS               ( 5)                            /*!< BUFD Bit Offset */
#define FLCTL_BANK1_RDCTL_BUFD                   ((uint32_t)0x00000020)          /*!< Enables read buffering feature for data reads to this Bank */
/* FLCTL_BANK1_RDCTL[RD_MODE_STATUS] Bits */
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
/* FLCTL_BANK1_RDCTL[WAIT] Bits */
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
/* FLCTL_RDBRST_CTLSTAT[START] Bits */
#define FLCTL_RDBRST_CTLSTAT_START_OFS           ( 0)                            /*!< START Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_START               ((uint32_t)0x00000001)          /*!< Start of burst/compare operation */
/* FLCTL_RDBRST_CTLSTAT[MEM_TYPE] Bits */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_OFS        ( 1)                            /*!< MEM_TYPE Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_MASK       ((uint32_t)0x00000006)          /*!< MEM_TYPE Bit Mask */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE0           ((uint32_t)0x00000002)          /*!< MEM_TYPE Bit 0 */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE1           ((uint32_t)0x00000004)          /*!< MEM_TYPE Bit 1 */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_0          ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_1          ((uint32_t)0x00000002)          /*!< Information Memory */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_2          ((uint32_t)0x00000004)          /*!< Reserved */
#define FLCTL_RDBRST_CTLSTAT_MEM_TYPE_3          ((uint32_t)0x00000006)          /*!< Engineering Memory */
/* FLCTL_RDBRST_CTLSTAT[STOP_FAIL] Bits */
#define FLCTL_RDBRST_CTLSTAT_STOP_FAIL_OFS       ( 3)                            /*!< STOP_FAIL Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_STOP_FAIL           ((uint32_t)0x00000008)          /*!< Terminate burst/compare operation */
/* FLCTL_RDBRST_CTLSTAT[DATA_CMP] Bits */
#define FLCTL_RDBRST_CTLSTAT_DATA_CMP_OFS        ( 4)                            /*!< DATA_CMP Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_DATA_CMP            ((uint32_t)0x00000010)          /*!< Data pattern used for comparison against memory read data */
/* FLCTL_RDBRST_CTLSTAT[TEST_EN] Bits */
#define FLCTL_RDBRST_CTLSTAT_TEST_EN_OFS         ( 6)                            /*!< TEST_EN Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_TEST_EN             ((uint32_t)0x00000040)          /*!< Enable comparison against test data compare registers */
/* FLCTL_RDBRST_CTLSTAT[BRST_STAT] Bits */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_OFS       (16)                            /*!< BRST_STAT Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_MASK      ((uint32_t)0x00030000)          /*!< BRST_STAT Bit Mask */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT0          ((uint32_t)0x00010000)          /*!< BRST_STAT Bit 0 */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT1          ((uint32_t)0x00020000)          /*!< BRST_STAT Bit 1 */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_0         ((uint32_t)0x00000000)          /*!< Idle */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_1         ((uint32_t)0x00010000)          /*!< Burst/Compare START bit written, but operation pending */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_2         ((uint32_t)0x00020000)          /*!< Burst/Compare in progress */
#define FLCTL_RDBRST_CTLSTAT_BRST_STAT_3         ((uint32_t)0x00030000)          /*!< Burst complete (status of completed burst remains in this state unless  */
                                                                                 /* explicitly cleared by SW) */
/* FLCTL_RDBRST_CTLSTAT[CMP_ERR] Bits */
#define FLCTL_RDBRST_CTLSTAT_CMP_ERR_OFS         (18)                            /*!< CMP_ERR Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_CMP_ERR             ((uint32_t)0x00040000)          /*!< Burst/Compare Operation encountered atleast one data */
/* FLCTL_RDBRST_CTLSTAT[ADDR_ERR] Bits */
#define FLCTL_RDBRST_CTLSTAT_ADDR_ERR_OFS        (19)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_ADDR_ERR            ((uint32_t)0x00080000)          /*!< Burst/Compare Operation was terminated due to access to */
/* FLCTL_RDBRST_CTLSTAT[CLR_STAT] Bits */
#define FLCTL_RDBRST_CTLSTAT_CLR_STAT_OFS        (23)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_RDBRST_CTLSTAT_CLR_STAT            ((uint32_t)0x00800000)          /*!< Clear status bits 19-16 of this register */
/* FLCTL_RDBRST_STARTADDR[START_ADDRESS] Bits */
#define FLCTL_RDBRST_STARTADDR_START_ADDRESS_OFS ( 0)                            /*!< START_ADDRESS Bit Offset */
#define FLCTL_RDBRST_STARTADDR_START_ADDRESS_MASK ((uint32_t)0x001FFFFF)          /*!< START_ADDRESS Bit Mask */
/* FLCTL_RDBRST_LEN[BURST_LENGTH] Bits */
#define FLCTL_RDBRST_LEN_BURST_LENGTH_OFS        ( 0)                            /*!< BURST_LENGTH Bit Offset */
#define FLCTL_RDBRST_LEN_BURST_LENGTH_MASK       ((uint32_t)0x001FFFFF)          /*!< BURST_LENGTH Bit Mask */
/* FLCTL_RDBRST_FAILADDR[FAIL_ADDRESS] Bits */
#define FLCTL_RDBRST_FAILADDR_FAIL_ADDRESS_OFS   ( 0)                            /*!< FAIL_ADDRESS Bit Offset */
#define FLCTL_RDBRST_FAILADDR_FAIL_ADDRESS_MASK  ((uint32_t)0x001FFFFF)          /*!< FAIL_ADDRESS Bit Mask */
/* FLCTL_RDBRST_FAILCNT[FAIL_COUNT] Bits */
#define FLCTL_RDBRST_FAILCNT_FAIL_COUNT_OFS      ( 0)                            /*!< FAIL_COUNT Bit Offset */
#define FLCTL_RDBRST_FAILCNT_FAIL_COUNT_MASK     ((uint32_t)0x0001FFFF)          /*!< FAIL_COUNT Bit Mask */
/* FLCTL_PRG_CTLSTAT[ENABLE] Bits */
#define FLCTL_PRG_CTLSTAT_ENABLE_OFS             ( 0)                            /*!< ENABLE Bit Offset */
#define FLCTL_PRG_CTLSTAT_ENABLE                 ((uint32_t)0x00000001)          /*!< Master control for all word program operations */
/* FLCTL_PRG_CTLSTAT[MODE] Bits */
#define FLCTL_PRG_CTLSTAT_MODE_OFS               ( 1)                            /*!< MODE Bit Offset */
#define FLCTL_PRG_CTLSTAT_MODE                   ((uint32_t)0x00000002)          /*!< Write mode */
/* FLCTL_PRG_CTLSTAT[VER_PRE] Bits */
#define FLCTL_PRG_CTLSTAT_VER_PRE_OFS            ( 2)                            /*!< VER_PRE Bit Offset */
#define FLCTL_PRG_CTLSTAT_VER_PRE                ((uint32_t)0x00000004)          /*!< Controls automatic pre program verify operations */
/* FLCTL_PRG_CTLSTAT[VER_PST] Bits */
#define FLCTL_PRG_CTLSTAT_VER_PST_OFS            ( 3)                            /*!< VER_PST Bit Offset */
#define FLCTL_PRG_CTLSTAT_VER_PST                ((uint32_t)0x00000008)          /*!< Controls automatic post program verify operations */
/* FLCTL_PRG_CTLSTAT[STATUS] Bits */
#define FLCTL_PRG_CTLSTAT_STATUS_OFS             (16)                            /*!< STATUS Bit Offset */
#define FLCTL_PRG_CTLSTAT_STATUS_MASK            ((uint32_t)0x00030000)          /*!< STATUS Bit Mask */
#define FLCTL_PRG_CTLSTAT_STATUS0                ((uint32_t)0x00010000)          /*!< STATUS Bit 0 */
#define FLCTL_PRG_CTLSTAT_STATUS1                ((uint32_t)0x00020000)          /*!< STATUS Bit 1 */
#define FLCTL_PRG_CTLSTAT_STATUS_0               ((uint32_t)0x00000000)          /*!< Idle (no program operation currently active) */
#define FLCTL_PRG_CTLSTAT_STATUS_1               ((uint32_t)0x00010000)          /*!< Single word program operation triggered, but pending */
#define FLCTL_PRG_CTLSTAT_STATUS_2               ((uint32_t)0x00020000)          /*!< Single word program in progress */
#define FLCTL_PRG_CTLSTAT_STATUS_3               ((uint32_t)0x00030000)          /*!< Reserved (Idle) */
/* FLCTL_PRG_CTLSTAT[BNK_ACT] Bits */
#define FLCTL_PRG_CTLSTAT_BNK_ACT_OFS            (18)                            /*!< BNK_ACT Bit Offset */
#define FLCTL_PRG_CTLSTAT_BNK_ACT                ((uint32_t)0x00040000)          /*!< Bank active */
/* FLCTL_PRGBRST_CTLSTAT[START] Bits */
#define FLCTL_PRGBRST_CTLSTAT_START_OFS          ( 0)                            /*!< START Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_START              ((uint32_t)0x00000001)          /*!< Trigger start of burst program operation */
/* FLCTL_PRGBRST_CTLSTAT[TYPE] Bits */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_OFS           ( 1)                            /*!< TYPE Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_MASK          ((uint32_t)0x00000006)          /*!< TYPE Bit Mask */
#define FLCTL_PRGBRST_CTLSTAT_TYPE0              ((uint32_t)0x00000002)          /*!< TYPE Bit 0 */
#define FLCTL_PRGBRST_CTLSTAT_TYPE1              ((uint32_t)0x00000004)          /*!< TYPE Bit 1 */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_0             ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_1             ((uint32_t)0x00000002)          /*!< Information Memory */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_2             ((uint32_t)0x00000004)          /*!< Reserved */
#define FLCTL_PRGBRST_CTLSTAT_TYPE_3             ((uint32_t)0x00000006)          /*!< Engineering Memory */
/* FLCTL_PRGBRST_CTLSTAT[LEN] Bits */
#define FLCTL_PRGBRST_CTLSTAT_LEN_OFS            ( 3)                            /*!< LEN Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_LEN_MASK           ((uint32_t)0x00000038)          /*!< LEN Bit Mask */
#define FLCTL_PRGBRST_CTLSTAT_LEN0               ((uint32_t)0x00000008)          /*!< LEN Bit 0 */
#define FLCTL_PRGBRST_CTLSTAT_LEN1               ((uint32_t)0x00000010)          /*!< LEN Bit 1 */
#define FLCTL_PRGBRST_CTLSTAT_LEN2               ((uint32_t)0x00000020)          /*!< LEN Bit 2 */
#define FLCTL_PRGBRST_CTLSTAT_LEN_0              ((uint32_t)0x00000000)          /*!< No burst operation */
#define FLCTL_PRGBRST_CTLSTAT_LEN_1              ((uint32_t)0x00000008)          /*!< 1 word burst of 128 bits, starting with address in the  */
                                                                                 /* FLCTL_PRGBRST_STARTADDR Register */
#define FLCTL_PRGBRST_CTLSTAT_LEN_2              ((uint32_t)0x00000010)          /*!< 2*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
                                                                                 /* Register */
#define FLCTL_PRGBRST_CTLSTAT_LEN_3              ((uint32_t)0x00000018)          /*!< 3*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
                                                                                 /* Register */
#define FLCTL_PRGBRST_CTLSTAT_LEN_4              ((uint32_t)0x00000020)          /*!< 4*128 bits burst write, starting with address in the FLCTL_PRGBRST_STARTADDR  */
                                                                                 /* Register */
/* FLCTL_PRGBRST_CTLSTAT[AUTO_PRE] Bits */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PRE_OFS       ( 6)                            /*!< AUTO_PRE Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PRE           ((uint32_t)0x00000040)          /*!< Auto-Verify operation before the Burst Program */
/* FLCTL_PRGBRST_CTLSTAT[AUTO_PST] Bits */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PST_OFS       ( 7)                            /*!< AUTO_PST Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_AUTO_PST           ((uint32_t)0x00000080)          /*!< Auto-Verify operation after the Burst Program */
/* FLCTL_PRGBRST_CTLSTAT[BURST_STATUS] Bits */
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
                                                                                 /* explicitly cleared by SW) */
/* FLCTL_PRGBRST_CTLSTAT[PRE_ERR] Bits */
#define FLCTL_PRGBRST_CTLSTAT_PRE_ERR_OFS        (19)                            /*!< PRE_ERR Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_PRE_ERR            ((uint32_t)0x00080000)          /*!< Burst Operation encountered preprogram auto-verify errors */
/* FLCTL_PRGBRST_CTLSTAT[PST_ERR] Bits */
#define FLCTL_PRGBRST_CTLSTAT_PST_ERR_OFS        (20)                            /*!< PST_ERR Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_PST_ERR            ((uint32_t)0x00100000)          /*!< Burst Operation encountered postprogram auto-verify errors */
/* FLCTL_PRGBRST_CTLSTAT[ADDR_ERR] Bits */
#define FLCTL_PRGBRST_CTLSTAT_ADDR_ERR_OFS       (21)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_ADDR_ERR           ((uint32_t)0x00200000)          /*!< Burst Operation was terminated due to attempted program of reserved memory */
/* FLCTL_PRGBRST_CTLSTAT[CLR_STAT] Bits */
#define FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS       (23)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_PRGBRST_CTLSTAT_CLR_STAT           ((uint32_t)0x00800000)          /*!< Clear status bits 21-16 of this register */
/* FLCTL_PRGBRST_STARTADDR[START_ADDRESS] Bits */
#define FLCTL_PRGBRST_STARTADDR_START_ADDRESS_OFS ( 0)                            /*!< START_ADDRESS Bit Offset */
#define FLCTL_PRGBRST_STARTADDR_START_ADDRESS_MASK ((uint32_t)0x003FFFFF)          /*!< START_ADDRESS Bit Mask */
/* FLCTL_ERASE_CTLSTAT[START] Bits */
#define FLCTL_ERASE_CTLSTAT_START_OFS            ( 0)                            /*!< START Bit Offset */
#define FLCTL_ERASE_CTLSTAT_START                ((uint32_t)0x00000001)          /*!< Start of Erase operation */
/* FLCTL_ERASE_CTLSTAT[MODE] Bits */
#define FLCTL_ERASE_CTLSTAT_MODE_OFS             ( 1)                            /*!< MODE Bit Offset */
#define FLCTL_ERASE_CTLSTAT_MODE                 ((uint32_t)0x00000002)          /*!< Erase mode selected by application */
/* FLCTL_ERASE_CTLSTAT[TYPE] Bits */
#define FLCTL_ERASE_CTLSTAT_TYPE_OFS             ( 2)                            /*!< TYPE Bit Offset */
#define FLCTL_ERASE_CTLSTAT_TYPE_MASK            ((uint32_t)0x0000000C)          /*!< TYPE Bit Mask */
#define FLCTL_ERASE_CTLSTAT_TYPE0                ((uint32_t)0x00000004)          /*!< TYPE Bit 0 */
#define FLCTL_ERASE_CTLSTAT_TYPE1                ((uint32_t)0x00000008)          /*!< TYPE Bit 1 */
#define FLCTL_ERASE_CTLSTAT_TYPE_0               ((uint32_t)0x00000000)          /*!< Main Memory */
#define FLCTL_ERASE_CTLSTAT_TYPE_1               ((uint32_t)0x00000004)          /*!< Information Memory */
#define FLCTL_ERASE_CTLSTAT_TYPE_2               ((uint32_t)0x00000008)          /*!< Reserved */
#define FLCTL_ERASE_CTLSTAT_TYPE_3               ((uint32_t)0x0000000C)          /*!< Engineering Memory */
/* FLCTL_ERASE_CTLSTAT[STATUS] Bits */
#define FLCTL_ERASE_CTLSTAT_STATUS_OFS           (16)                            /*!< STATUS Bit Offset */
#define FLCTL_ERASE_CTLSTAT_STATUS_MASK          ((uint32_t)0x00030000)          /*!< STATUS Bit Mask */
#define FLCTL_ERASE_CTLSTAT_STATUS0              ((uint32_t)0x00010000)          /*!< STATUS Bit 0 */
#define FLCTL_ERASE_CTLSTAT_STATUS1              ((uint32_t)0x00020000)          /*!< STATUS Bit 1 */
#define FLCTL_ERASE_CTLSTAT_STATUS_0             ((uint32_t)0x00000000)          /*!< Idle (no program operation currently active) */
#define FLCTL_ERASE_CTLSTAT_STATUS_1             ((uint32_t)0x00010000)          /*!< Erase operation triggered to START but pending */
#define FLCTL_ERASE_CTLSTAT_STATUS_2             ((uint32_t)0x00020000)          /*!< Erase operation in progress */
#define FLCTL_ERASE_CTLSTAT_STATUS_3             ((uint32_t)0x00030000)          /*!< Erase operation completed (status of completed erase remains in this state  */
                                                                                 /* unless explicitly cleared by SW) */
/* FLCTL_ERASE_CTLSTAT[ADDR_ERR] Bits */
#define FLCTL_ERASE_CTLSTAT_ADDR_ERR_OFS         (18)                            /*!< ADDR_ERR Bit Offset */
#define FLCTL_ERASE_CTLSTAT_ADDR_ERR             ((uint32_t)0x00040000)          /*!< Erase Operation was terminated due to attempted erase of reserved memory  */
                                                                                 /* address */
/* FLCTL_ERASE_CTLSTAT[CLR_STAT] Bits */
#define FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS         (19)                            /*!< CLR_STAT Bit Offset */
#define FLCTL_ERASE_CTLSTAT_CLR_STAT             ((uint32_t)0x00080000)          /*!< Clear status bits 18-16 of this register */
/* FLCTL_ERASE_SECTADDR[SECT_ADDRESS] Bits */
#define FLCTL_ERASE_SECTADDR_SECT_ADDRESS_OFS    ( 0)                            /*!< SECT_ADDRESS Bit Offset */
#define FLCTL_ERASE_SECTADDR_SECT_ADDRESS_MASK   ((uint32_t)0x003FFFFF)          /*!< SECT_ADDRESS Bit Mask */
/* FLCTL_BANK0_INFO_WEPROT[PROT0] Bits */
#define FLCTL_BANK0_INFO_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK0_INFO_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
/* FLCTL_BANK0_INFO_WEPROT[PROT1] Bits */
#define FLCTL_BANK0_INFO_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK0_INFO_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT0] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT1] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT2] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT2_OFS        ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT2            ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT3] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT3_OFS        ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT3            ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT4] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT4_OFS        ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT4            ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT5] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT5_OFS        ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT5            ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT6] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT6_OFS        ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT6            ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT7] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT7_OFS        ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT7            ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT8] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT8_OFS        ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT8            ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT9] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT9_OFS        ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT9            ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT10] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT10_OFS       (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT10           ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT11] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT11_OFS       (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT11           ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT12] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT12_OFS       (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT12           ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT13] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT13_OFS       (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT13           ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT14] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT14_OFS       (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT14           ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT15] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT15_OFS       (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT15           ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT16] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT16_OFS       (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT16           ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT17] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT17_OFS       (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT17           ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT18] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT18_OFS       (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT18           ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT19] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT19_OFS       (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT19           ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT20] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT20_OFS       (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT20           ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT21] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT21_OFS       (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT21           ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT22] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT22_OFS       (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT22           ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT23] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT23_OFS       (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT23           ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT24] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT24_OFS       (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT24           ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT25] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT25_OFS       (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT25           ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT26] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT26_OFS       (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT26           ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT27] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT27_OFS       (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT27           ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT28] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT28_OFS       (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT28           ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT29] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT29_OFS       (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT29           ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT30] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT30_OFS       (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT30           ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase */
/* FLCTL_BANK0_MAIN_WEPROT[PROT31] Bits */
#define FLCTL_BANK0_MAIN_WEPROT_PROT31_OFS       (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_BANK0_MAIN_WEPROT_PROT31           ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase */
/* FLCTL_BANK1_INFO_WEPROT[PROT0] Bits */
#define FLCTL_BANK1_INFO_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK1_INFO_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase operations */
/* FLCTL_BANK1_INFO_WEPROT[PROT1] Bits */
#define FLCTL_BANK1_INFO_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK1_INFO_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT0] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT0_OFS        ( 0)                            /*!< PROT0 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT0            ((uint32_t)0x00000001)          /*!< Protects Sector 0 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT1] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT1_OFS        ( 1)                            /*!< PROT1 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT1            ((uint32_t)0x00000002)          /*!< Protects Sector 1 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT2] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT2_OFS        ( 2)                            /*!< PROT2 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT2            ((uint32_t)0x00000004)          /*!< Protects Sector 2 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT3] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT3_OFS        ( 3)                            /*!< PROT3 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT3            ((uint32_t)0x00000008)          /*!< Protects Sector 3 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT4] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT4_OFS        ( 4)                            /*!< PROT4 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT4            ((uint32_t)0x00000010)          /*!< Protects Sector 4 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT5] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT5_OFS        ( 5)                            /*!< PROT5 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT5            ((uint32_t)0x00000020)          /*!< Protects Sector 5 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT6] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT6_OFS        ( 6)                            /*!< PROT6 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT6            ((uint32_t)0x00000040)          /*!< Protects Sector 6 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT7] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT7_OFS        ( 7)                            /*!< PROT7 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT7            ((uint32_t)0x00000080)          /*!< Protects Sector 7 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT8] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT8_OFS        ( 8)                            /*!< PROT8 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT8            ((uint32_t)0x00000100)          /*!< Protects Sector 8 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT9] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT9_OFS        ( 9)                            /*!< PROT9 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT9            ((uint32_t)0x00000200)          /*!< Protects Sector 9 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT10] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT10_OFS       (10)                            /*!< PROT10 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT10           ((uint32_t)0x00000400)          /*!< Protects Sector 10 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT11] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT11_OFS       (11)                            /*!< PROT11 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT11           ((uint32_t)0x00000800)          /*!< Protects Sector 11 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT12] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT12_OFS       (12)                            /*!< PROT12 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT12           ((uint32_t)0x00001000)          /*!< Protects Sector 12 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT13] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT13_OFS       (13)                            /*!< PROT13 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT13           ((uint32_t)0x00002000)          /*!< Protects Sector 13 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT14] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT14_OFS       (14)                            /*!< PROT14 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT14           ((uint32_t)0x00004000)          /*!< Protects Sector 14 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT15] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT15_OFS       (15)                            /*!< PROT15 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT15           ((uint32_t)0x00008000)          /*!< Protects Sector 15 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT16] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT16_OFS       (16)                            /*!< PROT16 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT16           ((uint32_t)0x00010000)          /*!< Protects Sector 16 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT17] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT17_OFS       (17)                            /*!< PROT17 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT17           ((uint32_t)0x00020000)          /*!< Protects Sector 17 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT18] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT18_OFS       (18)                            /*!< PROT18 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT18           ((uint32_t)0x00040000)          /*!< Protects Sector 18 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT19] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT19_OFS       (19)                            /*!< PROT19 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT19           ((uint32_t)0x00080000)          /*!< Protects Sector 19 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT20] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT20_OFS       (20)                            /*!< PROT20 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT20           ((uint32_t)0x00100000)          /*!< Protects Sector 20 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT21] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT21_OFS       (21)                            /*!< PROT21 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT21           ((uint32_t)0x00200000)          /*!< Protects Sector 21 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT22] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT22_OFS       (22)                            /*!< PROT22 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT22           ((uint32_t)0x00400000)          /*!< Protects Sector 22 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT23] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT23_OFS       (23)                            /*!< PROT23 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT23           ((uint32_t)0x00800000)          /*!< Protects Sector 23 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT24] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT24_OFS       (24)                            /*!< PROT24 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT24           ((uint32_t)0x01000000)          /*!< Protects Sector 24 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT25] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT25_OFS       (25)                            /*!< PROT25 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT25           ((uint32_t)0x02000000)          /*!< Protects Sector 25 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT26] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT26_OFS       (26)                            /*!< PROT26 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT26           ((uint32_t)0x04000000)          /*!< Protects Sector 26 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT27] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT27_OFS       (27)                            /*!< PROT27 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT27           ((uint32_t)0x08000000)          /*!< Protects Sector 27 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT28] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT28_OFS       (28)                            /*!< PROT28 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT28           ((uint32_t)0x10000000)          /*!< Protects Sector 28 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT29] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT29_OFS       (29)                            /*!< PROT29 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT29           ((uint32_t)0x20000000)          /*!< Protects Sector 29 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT30] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT30_OFS       (30)                            /*!< PROT30 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT30           ((uint32_t)0x40000000)          /*!< Protects Sector 30 from program or erase operations */
/* FLCTL_BANK1_MAIN_WEPROT[PROT31] Bits */
#define FLCTL_BANK1_MAIN_WEPROT_PROT31_OFS       (31)                            /*!< PROT31 Bit Offset */
#define FLCTL_BANK1_MAIN_WEPROT_PROT31           ((uint32_t)0x80000000)          /*!< Protects Sector 31 from program or erase operations */
/* FLCTL_BMRK_CTLSTAT[I_BMRK] Bits */
#define FLCTL_BMRK_CTLSTAT_I_BMRK_OFS            ( 0)                            /*!< I_BMRK Bit Offset */
#define FLCTL_BMRK_CTLSTAT_I_BMRK                ((uint32_t)0x00000001)          
/* FLCTL_BMRK_CTLSTAT[D_BMRK] Bits */
#define FLCTL_BMRK_CTLSTAT_D_BMRK_OFS            ( 1)                            /*!< D_BMRK Bit Offset */
#define FLCTL_BMRK_CTLSTAT_D_BMRK                ((uint32_t)0x00000002)          
/* FLCTL_BMRK_CTLSTAT[CMP_EN] Bits */
#define FLCTL_BMRK_CTLSTAT_CMP_EN_OFS            ( 2)                            /*!< CMP_EN Bit Offset */
#define FLCTL_BMRK_CTLSTAT_CMP_EN                ((uint32_t)0x00000004)          
/* FLCTL_BMRK_CTLSTAT[CMP_SEL] Bits */
#define FLCTL_BMRK_CTLSTAT_CMP_SEL_OFS           ( 3)                            /*!< CMP_SEL Bit Offset */
#define FLCTL_BMRK_CTLSTAT_CMP_SEL               ((uint32_t)0x00000008)          
/* FLCTL_IFG[RDBRST] Bits */
#define FLCTL_IFG_RDBRST_OFS                     ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_IFG_RDBRST                         ((uint32_t)0x00000001)          
/* FLCTL_IFG[AVPRE] Bits */
#define FLCTL_IFG_AVPRE_OFS                      ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_IFG_AVPRE                          ((uint32_t)0x00000002)          
/* FLCTL_IFG[AVPST] Bits */
#define FLCTL_IFG_AVPST_OFS                      ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_IFG_AVPST                          ((uint32_t)0x00000004)          
/* FLCTL_IFG[PRG] Bits */
#define FLCTL_IFG_PRG_OFS                        ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_IFG_PRG                            ((uint32_t)0x00000008)          
/* FLCTL_IFG[PRGB] Bits */
#define FLCTL_IFG_PRGB_OFS                       ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_IFG_PRGB                           ((uint32_t)0x00000010)          
/* FLCTL_IFG[ERASE] Bits */
#define FLCTL_IFG_ERASE_OFS                      ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_IFG_ERASE                          ((uint32_t)0x00000020)          
/* FLCTL_IFG[BMRK] Bits */
#define FLCTL_IFG_BMRK_OFS                       ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_IFG_BMRK                           ((uint32_t)0x00000100)          
/* FLCTL_IFG[PRG_ERR] Bits */
#define FLCTL_IFG_PRG_ERR_OFS                    ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_IFG_PRG_ERR                        ((uint32_t)0x00000200)          
/* FLCTL_IE[RDBRST] Bits */
#define FLCTL_IE_RDBRST_OFS                      ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_IE_RDBRST                          ((uint32_t)0x00000001)          
/* FLCTL_IE[AVPRE] Bits */
#define FLCTL_IE_AVPRE_OFS                       ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_IE_AVPRE                           ((uint32_t)0x00000002)          
/* FLCTL_IE[AVPST] Bits */
#define FLCTL_IE_AVPST_OFS                       ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_IE_AVPST                           ((uint32_t)0x00000004)          
/* FLCTL_IE[PRG] Bits */
#define FLCTL_IE_PRG_OFS                         ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_IE_PRG                             ((uint32_t)0x00000008)          
/* FLCTL_IE[PRGB] Bits */
#define FLCTL_IE_PRGB_OFS                        ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_IE_PRGB                            ((uint32_t)0x00000010)          
/* FLCTL_IE[ERASE] Bits */
#define FLCTL_IE_ERASE_OFS                       ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_IE_ERASE                           ((uint32_t)0x00000020)          
/* FLCTL_IE[BMRK] Bits */
#define FLCTL_IE_BMRK_OFS                        ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_IE_BMRK                            ((uint32_t)0x00000100)          
/* FLCTL_IE[PRG_ERR] Bits */
#define FLCTL_IE_PRG_ERR_OFS                     ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_IE_PRG_ERR                         ((uint32_t)0x00000200)          
/* FLCTL_CLRIFG[RDBRST] Bits */
#define FLCTL_CLRIFG_RDBRST_OFS                  ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_CLRIFG_RDBRST                      ((uint32_t)0x00000001)          
/* FLCTL_CLRIFG[AVPRE] Bits */
#define FLCTL_CLRIFG_AVPRE_OFS                   ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_CLRIFG_AVPRE                       ((uint32_t)0x00000002)          
/* FLCTL_CLRIFG[AVPST] Bits */
#define FLCTL_CLRIFG_AVPST_OFS                   ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_CLRIFG_AVPST                       ((uint32_t)0x00000004)          
/* FLCTL_CLRIFG[PRG] Bits */
#define FLCTL_CLRIFG_PRG_OFS                     ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_CLRIFG_PRG                         ((uint32_t)0x00000008)          
/* FLCTL_CLRIFG[PRGB] Bits */
#define FLCTL_CLRIFG_PRGB_OFS                    ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_CLRIFG_PRGB                        ((uint32_t)0x00000010)          
/* FLCTL_CLRIFG[ERASE] Bits */
#define FLCTL_CLRIFG_ERASE_OFS                   ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_CLRIFG_ERASE                       ((uint32_t)0x00000020)          
/* FLCTL_CLRIFG[BMRK] Bits */
#define FLCTL_CLRIFG_BMRK_OFS                    ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_CLRIFG_BMRK                        ((uint32_t)0x00000100)          
/* FLCTL_CLRIFG[PRG_ERR] Bits */
#define FLCTL_CLRIFG_PRG_ERR_OFS                 ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_CLRIFG_PRG_ERR                     ((uint32_t)0x00000200)          
/* FLCTL_SETIFG[RDBRST] Bits */
#define FLCTL_SETIFG_RDBRST_OFS                  ( 0)                            /*!< RDBRST Bit Offset */
#define FLCTL_SETIFG_RDBRST                      ((uint32_t)0x00000001)          
/* FLCTL_SETIFG[AVPRE] Bits */
#define FLCTL_SETIFG_AVPRE_OFS                   ( 1)                            /*!< AVPRE Bit Offset */
#define FLCTL_SETIFG_AVPRE                       ((uint32_t)0x00000002)          
/* FLCTL_SETIFG[AVPST] Bits */
#define FLCTL_SETIFG_AVPST_OFS                   ( 2)                            /*!< AVPST Bit Offset */
#define FLCTL_SETIFG_AVPST                       ((uint32_t)0x00000004)          
/* FLCTL_SETIFG[PRG] Bits */
#define FLCTL_SETIFG_PRG_OFS                     ( 3)                            /*!< PRG Bit Offset */
#define FLCTL_SETIFG_PRG                         ((uint32_t)0x00000008)          
/* FLCTL_SETIFG[PRGB] Bits */
#define FLCTL_SETIFG_PRGB_OFS                    ( 4)                            /*!< PRGB Bit Offset */
#define FLCTL_SETIFG_PRGB                        ((uint32_t)0x00000010)          
/* FLCTL_SETIFG[ERASE] Bits */
#define FLCTL_SETIFG_ERASE_OFS                   ( 5)                            /*!< ERASE Bit Offset */
#define FLCTL_SETIFG_ERASE                       ((uint32_t)0x00000020)          
/* FLCTL_SETIFG[BMRK] Bits */
#define FLCTL_SETIFG_BMRK_OFS                    ( 8)                            /*!< BMRK Bit Offset */
#define FLCTL_SETIFG_BMRK                        ((uint32_t)0x00000100)          
/* FLCTL_SETIFG[PRG_ERR] Bits */
#define FLCTL_SETIFG_PRG_ERR_OFS                 ( 9)                            /*!< PRG_ERR Bit Offset */
#define FLCTL_SETIFG_PRG_ERR                     ((uint32_t)0x00000200)          
/* FLCTL_READ_TIMCTL[SETUP] Bits */
#define FLCTL_READ_TIMCTL_SETUP_OFS              ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_READ_TIMCTL_SETUP_MASK             ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_READ_TIMCTL[IREF_BOOST1] Bits */
#define FLCTL_READ_TIMCTL_IREF_BOOST1_OFS        (12)                            /*!< IREF_BOOST1 Bit Offset */
#define FLCTL_READ_TIMCTL_IREF_BOOST1_MASK       ((uint32_t)0x0000F000)          /*!< IREF_BOOST1 Bit Mask */
/* FLCTL_READ_TIMCTL[SETUP_LONG] Bits */
#define FLCTL_READ_TIMCTL_SETUP_LONG_OFS         (16)                            /*!< SETUP_LONG Bit Offset */
#define FLCTL_READ_TIMCTL_SETUP_LONG_MASK        ((uint32_t)0x00FF0000)          /*!< SETUP_LONG Bit Mask */
/* FLCTL_READMARGIN_TIMCTL[SETUP] Bits */
#define FLCTL_READMARGIN_TIMCTL_SETUP_OFS        ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_READMARGIN_TIMCTL_SETUP_MASK       ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_PRGVER_TIMCTL[SETUP] Bits */
#define FLCTL_PRGVER_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_PRGVER_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_PRGVER_TIMCTL[ACTIVE] Bits */
#define FLCTL_PRGVER_TIMCTL_ACTIVE_OFS           ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_PRGVER_TIMCTL_ACTIVE_MASK          ((uint32_t)0x00000F00)          /*!< ACTIVE Bit Mask */
/* FLCTL_PRGVER_TIMCTL[HOLD] Bits */
#define FLCTL_PRGVER_TIMCTL_HOLD_OFS             (12)                            /*!< HOLD Bit Offset */
#define FLCTL_PRGVER_TIMCTL_HOLD_MASK            ((uint32_t)0x0000F000)          /*!< HOLD Bit Mask */
/* FLCTL_ERSVER_TIMCTL[SETUP] Bits */
#define FLCTL_ERSVER_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_ERSVER_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_LKGVER_TIMCTL[SETUP] Bits */
#define FLCTL_LKGVER_TIMCTL_SETUP_OFS            ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_LKGVER_TIMCTL_SETUP_MASK           ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_PROGRAM_TIMCTL[SETUP] Bits */
#define FLCTL_PROGRAM_TIMCTL_SETUP_OFS           ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_PROGRAM_TIMCTL_SETUP_MASK          ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_PROGRAM_TIMCTL[ACTIVE] Bits */
#define FLCTL_PROGRAM_TIMCTL_ACTIVE_OFS          ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_PROGRAM_TIMCTL_ACTIVE_MASK         ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
/* FLCTL_PROGRAM_TIMCTL[HOLD] Bits */
#define FLCTL_PROGRAM_TIMCTL_HOLD_OFS            (28)                            /*!< HOLD Bit Offset */
#define FLCTL_PROGRAM_TIMCTL_HOLD_MASK           ((uint32_t)0xF0000000)          /*!< HOLD Bit Mask */
/* FLCTL_ERASE_TIMCTL[SETUP] Bits */
#define FLCTL_ERASE_TIMCTL_SETUP_OFS             ( 0)                            /*!< SETUP Bit Offset */
#define FLCTL_ERASE_TIMCTL_SETUP_MASK            ((uint32_t)0x000000FF)          /*!< SETUP Bit Mask */
/* FLCTL_ERASE_TIMCTL[ACTIVE] Bits */
#define FLCTL_ERASE_TIMCTL_ACTIVE_OFS            ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_ERASE_TIMCTL_ACTIVE_MASK           ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */
/* FLCTL_ERASE_TIMCTL[HOLD] Bits */
#define FLCTL_ERASE_TIMCTL_HOLD_OFS              (28)                            /*!< HOLD Bit Offset */
#define FLCTL_ERASE_TIMCTL_HOLD_MASK             ((uint32_t)0xF0000000)          /*!< HOLD Bit Mask */
/* FLCTL_MASSERASE_TIMCTL[BOOST_ACTIVE] Bits */
#define FLCTL_MASSERASE_TIMCTL_BOOST_ACTIVE_OFS  ( 0)                            /*!< BOOST_ACTIVE Bit Offset */
#define FLCTL_MASSERASE_TIMCTL_BOOST_ACTIVE_MASK ((uint32_t)0x000000FF)          /*!< BOOST_ACTIVE Bit Mask */
/* FLCTL_MASSERASE_TIMCTL[BOOST_HOLD] Bits */
#define FLCTL_MASSERASE_TIMCTL_BOOST_HOLD_OFS    ( 8)                            /*!< BOOST_HOLD Bit Offset */
#define FLCTL_MASSERASE_TIMCTL_BOOST_HOLD_MASK   ((uint32_t)0x0000FF00)          /*!< BOOST_HOLD Bit Mask */
/* FLCTL_BURSTPRG_TIMCTL[ACTIVE] Bits */
#define FLCTL_BURSTPRG_TIMCTL_ACTIVE_OFS         ( 8)                            /*!< ACTIVE Bit Offset */
#define FLCTL_BURSTPRG_TIMCTL_ACTIVE_MASK        ((uint32_t)0x0FFFFF00)          /*!< ACTIVE Bit Mask */

/******************************************************************************
* FL_BOOTOVER_MAILBOX Bits
******************************************************************************/

/******************************************************************************
* FPB Bits
******************************************************************************/


/******************************************************************************
* FPU Bits
******************************************************************************/


/******************************************************************************
* ITM Bits
******************************************************************************/


/******************************************************************************
* MPU Bits
******************************************************************************/

/* Pre-defined bitfield values */

/*  MPU_RASR_SIZE Bitfield Bits */
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

/*  MPU_RASR_AP Bitfield Bits */
#define MPU_RASR_AP_PRV_NO_USR_NO                ((uint32_t)0x00000000)          /*!< Privileged permissions: No access. User permissions: No access. */
#define MPU_RASR_AP_PRV_RW_USR_NO                ((uint32_t)0x01000000)          /*!< Privileged permissions: Read-write. User permissions: No access. */
#define MPU_RASR_AP_PRV_RW_USR_RO                ((uint32_t)0x02000000)          /*!< Privileged permissions: Read-write. User permissions: Read-only. */
#define MPU_RASR_AP_PRV_RW_USR_RW                ((uint32_t)0x03000000)          /*!< Privileged permissions: Read-write. User permissions: Read-write. */
#define MPU_RASR_AP_PRV_RO_USR_NO                ((uint32_t)0x05000000)          /*!< Privileged permissions: Read-only. User permissions: No access. */
#define MPU_RASR_AP_PRV_RO_USR_RO                ((uint32_t)0x06000000)          /*!< Privileged permissions: Read-only. User permissions: Read-only. */

/* MPU_RASR_XN Bitfield Bits */
#define MPU_RASR_AP_EXEC                         ((uint32_t)0x00000000)          /*!< Instruction access enabled */
#define MPU_RASR_AP_NOEXEC                       ((uint32_t)0x10000000)          /*!< Instruction access disabled */


/******************************************************************************
* NVIC Bits
******************************************************************************/

/* NVIC_IPR0[NVIC_IPR0_PRI_0] Bits */
#define NVIC_IPR0_PRI_0_OFS                      ( 0)                            /*!< PRI_0 Offset */
#define NVIC_IPR0_PRI_0_M                        ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR0[NVIC_IPR0_PRI_1] Bits */
#define NVIC_IPR0_PRI_1_OFS                      ( 8)                            /*!< PRI_1 Offset */
#define NVIC_IPR0_PRI_1_M                        ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR0[NVIC_IPR0_PRI_2] Bits */
#define NVIC_IPR0_PRI_2_OFS                      (16)                            /*!< PRI_2 Offset */
#define NVIC_IPR0_PRI_2_M                        ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR0[NVIC_IPR0_PRI_3] Bits */
#define NVIC_IPR0_PRI_3_OFS                      (24)                            /*!< PRI_3 Offset */
#define NVIC_IPR0_PRI_3_M                        ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR1[NVIC_IPR1_PRI_4] Bits */
#define NVIC_IPR1_PRI_4_OFS                      ( 0)                            /*!< PRI_4 Offset */
#define NVIC_IPR1_PRI_4_M                        ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR1[NVIC_IPR1_PRI_5] Bits */
#define NVIC_IPR1_PRI_5_OFS                      ( 8)                            /*!< PRI_5 Offset */
#define NVIC_IPR1_PRI_5_M                        ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR1[NVIC_IPR1_PRI_6] Bits */
#define NVIC_IPR1_PRI_6_OFS                      (16)                            /*!< PRI_6 Offset */
#define NVIC_IPR1_PRI_6_M                        ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR1[NVIC_IPR1_PRI_7] Bits */
#define NVIC_IPR1_PRI_7_OFS                      (24)                            /*!< PRI_7 Offset */
#define NVIC_IPR1_PRI_7_M                        ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR2[NVIC_IPR2_PRI_8] Bits */
#define NVIC_IPR2_PRI_8_OFS                      ( 0)                            /*!< PRI_8 Offset */
#define NVIC_IPR2_PRI_8_M                        ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR2[NVIC_IPR2_PRI_9] Bits */
#define NVIC_IPR2_PRI_9_OFS                      ( 8)                            /*!< PRI_9 Offset */
#define NVIC_IPR2_PRI_9_M                        ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR2[NVIC_IPR2_PRI_10] Bits */
#define NVIC_IPR2_PRI_10_OFS                     (16)                            /*!< PRI_10 Offset */
#define NVIC_IPR2_PRI_10_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR2[NVIC_IPR2_PRI_11] Bits */
#define NVIC_IPR2_PRI_11_OFS                     (24)                            /*!< PRI_11 Offset */
#define NVIC_IPR2_PRI_11_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR3[NVIC_IPR3_PRI_12] Bits */
#define NVIC_IPR3_PRI_12_OFS                     ( 0)                            /*!< PRI_12 Offset */
#define NVIC_IPR3_PRI_12_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR3[NVIC_IPR3_PRI_13] Bits */
#define NVIC_IPR3_PRI_13_OFS                     ( 8)                            /*!< PRI_13 Offset */
#define NVIC_IPR3_PRI_13_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR3[NVIC_IPR3_PRI_14] Bits */
#define NVIC_IPR3_PRI_14_OFS                     (16)                            /*!< PRI_14 Offset */
#define NVIC_IPR3_PRI_14_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR3[NVIC_IPR3_PRI_15] Bits */
#define NVIC_IPR3_PRI_15_OFS                     (24)                            /*!< PRI_15 Offset */
#define NVIC_IPR3_PRI_15_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR4[NVIC_IPR4_PRI_16] Bits */
#define NVIC_IPR4_PRI_16_OFS                     ( 0)                            /*!< PRI_16 Offset */
#define NVIC_IPR4_PRI_16_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR4[NVIC_IPR4_PRI_17] Bits */
#define NVIC_IPR4_PRI_17_OFS                     ( 8)                            /*!< PRI_17 Offset */
#define NVIC_IPR4_PRI_17_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR4[NVIC_IPR4_PRI_18] Bits */
#define NVIC_IPR4_PRI_18_OFS                     (16)                            /*!< PRI_18 Offset */
#define NVIC_IPR4_PRI_18_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR4[NVIC_IPR4_PRI_19] Bits */
#define NVIC_IPR4_PRI_19_OFS                     (24)                            /*!< PRI_19 Offset */
#define NVIC_IPR4_PRI_19_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR5[NVIC_IPR5_PRI_20] Bits */
#define NVIC_IPR5_PRI_20_OFS                     ( 0)                            /*!< PRI_20 Offset */
#define NVIC_IPR5_PRI_20_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR5[NVIC_IPR5_PRI_21] Bits */
#define NVIC_IPR5_PRI_21_OFS                     ( 8)                            /*!< PRI_21 Offset */
#define NVIC_IPR5_PRI_21_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR5[NVIC_IPR5_PRI_22] Bits */
#define NVIC_IPR5_PRI_22_OFS                     (16)                            /*!< PRI_22 Offset */
#define NVIC_IPR5_PRI_22_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR5[NVIC_IPR5_PRI_23] Bits */
#define NVIC_IPR5_PRI_23_OFS                     (24)                            /*!< PRI_23 Offset */
#define NVIC_IPR5_PRI_23_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR6[NVIC_IPR6_PRI_24] Bits */
#define NVIC_IPR6_PRI_24_OFS                     ( 0)                            /*!< PRI_24 Offset */
#define NVIC_IPR6_PRI_24_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR6[NVIC_IPR6_PRI_25] Bits */
#define NVIC_IPR6_PRI_25_OFS                     ( 8)                            /*!< PRI_25 Offset */
#define NVIC_IPR6_PRI_25_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR6[NVIC_IPR6_PRI_26] Bits */
#define NVIC_IPR6_PRI_26_OFS                     (16)                            /*!< PRI_26 Offset */
#define NVIC_IPR6_PRI_26_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR6[NVIC_IPR6_PRI_27] Bits */
#define NVIC_IPR6_PRI_27_OFS                     (24)                            /*!< PRI_27 Offset */
#define NVIC_IPR6_PRI_27_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR7[NVIC_IPR7_PRI_28] Bits */
#define NVIC_IPR7_PRI_28_OFS                     ( 0)                            /*!< PRI_28 Offset */
#define NVIC_IPR7_PRI_28_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR7[NVIC_IPR7_PRI_29] Bits */
#define NVIC_IPR7_PRI_29_OFS                     ( 8)                            /*!< PRI_29 Offset */
#define NVIC_IPR7_PRI_29_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR7[NVIC_IPR7_PRI_30] Bits */
#define NVIC_IPR7_PRI_30_OFS                     (16)                            /*!< PRI_30 Offset */
#define NVIC_IPR7_PRI_30_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR7[NVIC_IPR7_PRI_31] Bits */
#define NVIC_IPR7_PRI_31_OFS                     (24)                            /*!< PRI_31 Offset */
#define NVIC_IPR7_PRI_31_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR8[NVIC_IPR8_PRI_32] Bits */
#define NVIC_IPR8_PRI_32_OFS                     ( 0)                            /*!< PRI_32 Offset */
#define NVIC_IPR8_PRI_32_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR8[NVIC_IPR8_PRI_33] Bits */
#define NVIC_IPR8_PRI_33_OFS                     ( 8)                            /*!< PRI_33 Offset */
#define NVIC_IPR8_PRI_33_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR8[NVIC_IPR8_PRI_34] Bits */
#define NVIC_IPR8_PRI_34_OFS                     (16)                            /*!< PRI_34 Offset */
#define NVIC_IPR8_PRI_34_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR8[NVIC_IPR8_PRI_35] Bits */
#define NVIC_IPR8_PRI_35_OFS                     (24)                            /*!< PRI_35 Offset */
#define NVIC_IPR8_PRI_35_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR9[NVIC_IPR9_PRI_36] Bits */
#define NVIC_IPR9_PRI_36_OFS                     ( 0)                            /*!< PRI_36 Offset */
#define NVIC_IPR9_PRI_36_M                       ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR9[NVIC_IPR9_PRI_37] Bits */
#define NVIC_IPR9_PRI_37_OFS                     ( 8)                            /*!< PRI_37 Offset */
#define NVIC_IPR9_PRI_37_M                       ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR9[NVIC_IPR9_PRI_38] Bits */
#define NVIC_IPR9_PRI_38_OFS                     (16)                            /*!< PRI_38 Offset */
#define NVIC_IPR9_PRI_38_M                       ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR9[NVIC_IPR9_PRI_39] Bits */
#define NVIC_IPR9_PRI_39_OFS                     (24)                            /*!< PRI_39 Offset */
#define NVIC_IPR9_PRI_39_M                       ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR10[NVIC_IPR10_PRI_40] Bits */
#define NVIC_IPR10_PRI_40_OFS                    ( 0)                            /*!< PRI_40 Offset */
#define NVIC_IPR10_PRI_40_M                      ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR10[NVIC_IPR10_PRI_41] Bits */
#define NVIC_IPR10_PRI_41_OFS                    ( 8)                            /*!< PRI_41 Offset */
#define NVIC_IPR10_PRI_41_M                      ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR10[NVIC_IPR10_PRI_42] Bits */
#define NVIC_IPR10_PRI_42_OFS                    (16)                            /*!< PRI_42 Offset */
#define NVIC_IPR10_PRI_42_M                      ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR10[NVIC_IPR10_PRI_43] Bits */
#define NVIC_IPR10_PRI_43_OFS                    (24)                            /*!< PRI_43 Offset */
#define NVIC_IPR10_PRI_43_M                      ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR11[NVIC_IPR11_PRI_44] Bits */
#define NVIC_IPR11_PRI_44_OFS                    ( 0)                            /*!< PRI_44 Offset */
#define NVIC_IPR11_PRI_44_M                      ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR11[NVIC_IPR11_PRI_45] Bits */
#define NVIC_IPR11_PRI_45_OFS                    ( 8)                            /*!< PRI_45 Offset */
#define NVIC_IPR11_PRI_45_M                      ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR11[NVIC_IPR11_PRI_46] Bits */
#define NVIC_IPR11_PRI_46_OFS                    (16)                            /*!< PRI_46 Offset */
#define NVIC_IPR11_PRI_46_M                      ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR11[NVIC_IPR11_PRI_47] Bits */
#define NVIC_IPR11_PRI_47_OFS                    (24)                            /*!< PRI_47 Offset */
#define NVIC_IPR11_PRI_47_M                      ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR12[NVIC_IPR12_PRI_48] Bits */
#define NVIC_IPR12_PRI_48_OFS                    ( 0)                            /*!< PRI_48 Offset */
#define NVIC_IPR12_PRI_48_M                      ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR12[NVIC_IPR12_PRI_49] Bits */
#define NVIC_IPR12_PRI_49_OFS                    ( 8)                            /*!< PRI_49 Offset */
#define NVIC_IPR12_PRI_49_M                      ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR12[NVIC_IPR12_PRI_50] Bits */
#define NVIC_IPR12_PRI_50_OFS                    (16)                            /*!< PRI_50 Offset */
#define NVIC_IPR12_PRI_50_M                      ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR12[NVIC_IPR12_PRI_51] Bits */
#define NVIC_IPR12_PRI_51_OFS                    (24)                            /*!< PRI_51 Offset */
#define NVIC_IPR12_PRI_51_M                      ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR13[NVIC_IPR13_PRI_52] Bits */
#define NVIC_IPR13_PRI_52_OFS                    ( 0)                            /*!< PRI_52 Offset */
#define NVIC_IPR13_PRI_52_M                      ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR13[NVIC_IPR13_PRI_53] Bits */
#define NVIC_IPR13_PRI_53_OFS                    ( 8)                            /*!< PRI_53 Offset */
#define NVIC_IPR13_PRI_53_M                      ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR13[NVIC_IPR13_PRI_54] Bits */
#define NVIC_IPR13_PRI_54_OFS                    (16)                            /*!< PRI_54 Offset */
#define NVIC_IPR13_PRI_54_M                      ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR13[NVIC_IPR13_PRI_55] Bits */
#define NVIC_IPR13_PRI_55_OFS                    (24)                            /*!< PRI_55 Offset */
#define NVIC_IPR13_PRI_55_M                      ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR14[NVIC_IPR14_PRI_56] Bits */
#define NVIC_IPR14_PRI_56_OFS                    ( 0)                            /*!< PRI_56 Offset */
#define NVIC_IPR14_PRI_56_M                      ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR14[NVIC_IPR14_PRI_57] Bits */
#define NVIC_IPR14_PRI_57_OFS                    ( 8)                            /*!< PRI_57 Offset */
#define NVIC_IPR14_PRI_57_M                      ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR14[NVIC_IPR14_PRI_58] Bits */
#define NVIC_IPR14_PRI_58_OFS                    (16)                            /*!< PRI_58 Offset */
#define NVIC_IPR14_PRI_58_M                      ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR14[NVIC_IPR14_PRI_59] Bits */
#define NVIC_IPR14_PRI_59_OFS                    (24)                            /*!< PRI_59 Offset */
#define NVIC_IPR14_PRI_59_M                      ((uint32_t)0xff000000)          /*  */
/* NVIC_IPR15[NVIC_IPR15_PRI_60] Bits */
#define NVIC_IPR15_PRI_60_OFS                    ( 0)                            /*!< PRI_60 Offset */
#define NVIC_IPR15_PRI_60_M                      ((uint32_t)0x000000ff)          /*  */
/* NVIC_IPR15[NVIC_IPR15_PRI_61] Bits */
#define NVIC_IPR15_PRI_61_OFS                    ( 8)                            /*!< PRI_61 Offset */
#define NVIC_IPR15_PRI_61_M                      ((uint32_t)0x0000ff00)          /*  */
/* NVIC_IPR15[NVIC_IPR15_PRI_62] Bits */
#define NVIC_IPR15_PRI_62_OFS                    (16)                            /*!< PRI_62 Offset */
#define NVIC_IPR15_PRI_62_M                      ((uint32_t)0x00ff0000)          /*  */
/* NVIC_IPR15[NVIC_IPR15_PRI_63] Bits */
#define NVIC_IPR15_PRI_63_OFS                    (24)                            /*!< PRI_63 Offset */
#define NVIC_IPR15_PRI_63_M                      ((uint32_t)0xff000000)          /*  */


/******************************************************************************
* PCM Bits
******************************************************************************/
/* PCM_CTL0[AMR] Bits */
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
/* PCM_CTL0[LPMR] Bits */
#define PCM_CTL0_LPMR_OFS                        ( 4)                            /*!< LPMR Bit Offset */
#define PCM_CTL0_LPMR_MASK                       ((uint32_t)0x000000F0)          /*!< LPMR Bit Mask */
#define PCM_CTL0_LPMR0                           ((uint32_t)0x00000010)          /*!< LPMR Bit 0 */
#define PCM_CTL0_LPMR1                           ((uint32_t)0x00000020)          /*!< LPMR Bit 1 */
#define PCM_CTL0_LPMR2                           ((uint32_t)0x00000040)          /*!< LPMR Bit 2 */
#define PCM_CTL0_LPMR3                           ((uint32_t)0x00000080)          /*!< LPMR Bit 3 */
#define PCM_CTL0_LPMR_0                          ((uint32_t)0x00000000)          /*!< LPM3. Core voltage setting is similar to the mode from which LPM3 is  */
                                                                                 /* entered. */
#define PCM_CTL0_LPMR_10                         ((uint32_t)0x000000A0)          /*!< LPM3.5. Core voltage setting 0. */
#define PCM_CTL0_LPMR_12                         ((uint32_t)0x000000C0)          /*!< LPM4.5 */
#define PCM_CTL0_LPMR__LPM3                      ((uint32_t)0x00000000)          /*!< LPM3. Core voltage setting is similar to the mode from which LPM3 is  */
                                                                                 /* entered. */
#define PCM_CTL0_LPMR__LPM35                     ((uint32_t)0x000000A0)          /*!< LPM3.5. Core voltage setting 0. */
#define PCM_CTL0_LPMR__LPM45                     ((uint32_t)0x000000C0)          /*!< LPM4.5 */
/* PCM_CTL0[CPM] Bits */
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
/* PCM_CTL0[KEY] Bits */
#define PCM_CTL0_KEY_OFS                         (16)                            /*!< PCMKEY Bit Offset */
#define PCM_CTL0_KEY_MASK                        ((uint32_t)0xFFFF0000)          /*!< PCMKEY Bit Mask */
/* PCM_CTL1[LOCKLPM5] Bits */
#define PCM_CTL1_LOCKLPM5_OFS                    ( 0)                            /*!< LOCKLPM5 Bit Offset */
#define PCM_CTL1_LOCKLPM5                        ((uint32_t)0x00000001)          /*!< Lock LPM5 */
/* PCM_CTL1[LOCKBKUP] Bits */
#define PCM_CTL1_LOCKBKUP_OFS                    ( 1)                            /*!< LOCKBKUP Bit Offset */
#define PCM_CTL1_LOCKBKUP                        ((uint32_t)0x00000002)          /*!< Lock Backup */
/* PCM_CTL1[FORCE_LPM_ENTRY] Bits */
#define PCM_CTL1_FORCE_LPM_ENTRY_OFS             ( 2)                            /*!< FORCE_LPM_ENTRY Bit Offset */
#define PCM_CTL1_FORCE_LPM_ENTRY                 ((uint32_t)0x00000004)          /*!< Force LPM entry */
/* PCM_CTL1[PMR_BUSY] Bits */
#define PCM_CTL1_PMR_BUSY_OFS                    ( 8)                            /*!< PMR_BUSY Bit Offset */
#define PCM_CTL1_PMR_BUSY                        ((uint32_t)0x00000100)          /*!< Power mode request busy flag */
/* PCM_CTL1[KEY] Bits */
#define PCM_CTL1_KEY_OFS                         (16)                            /*!< PCMKEY Bit Offset */
#define PCM_CTL1_KEY_MASK                        ((uint32_t)0xFFFF0000)          /*!< PCMKEY Bit Mask */
/* PCM_IE[LPM_INVALID_TR_IE] Bits */
#define PCM_IE_LPM_INVALID_TR_IE_OFS             ( 0)                            /*!< LPM_INVALID_TR_IE Bit Offset */
#define PCM_IE_LPM_INVALID_TR_IE                 ((uint32_t)0x00000001)          /*!< LPM invalid transition interrupt enable */
/* PCM_IE[LPM_INVALID_CLK_IE] Bits */
#define PCM_IE_LPM_INVALID_CLK_IE_OFS            ( 1)                            /*!< LPM_INVALID_CLK_IE Bit Offset */
#define PCM_IE_LPM_INVALID_CLK_IE                ((uint32_t)0x00000002)          /*!< LPM invalid clock interrupt enable */
/* PCM_IE[AM_INVALID_TR_IE] Bits */
#define PCM_IE_AM_INVALID_TR_IE_OFS              ( 2)                            /*!< AM_INVALID_TR_IE Bit Offset */
#define PCM_IE_AM_INVALID_TR_IE                  ((uint32_t)0x00000004)          /*!< Active mode invalid transition interrupt enable */
/* PCM_IE[DCDC_ERROR_IE] Bits */
#define PCM_IE_DCDC_ERROR_IE_OFS                 ( 6)                            /*!< DCDC_ERROR_IE Bit Offset */
#define PCM_IE_DCDC_ERROR_IE                     ((uint32_t)0x00000040)          /*!< DC-DC error interrupt enable */
/* PCM_IFG[LPM_INVALID_TR_IFG] Bits */
#define PCM_IFG_LPM_INVALID_TR_IFG_OFS           ( 0)                            /*!< LPM_INVALID_TR_IFG Bit Offset */
#define PCM_IFG_LPM_INVALID_TR_IFG               ((uint32_t)0x00000001)          /*!< LPM invalid transition flag */
/* PCM_IFG[LPM_INVALID_CLK_IFG] Bits */
#define PCM_IFG_LPM_INVALID_CLK_IFG_OFS          ( 1)                            /*!< LPM_INVALID_CLK_IFG Bit Offset */
#define PCM_IFG_LPM_INVALID_CLK_IFG              ((uint32_t)0x00000002)          /*!< LPM invalid clock flag */
/* PCM_IFG[AM_INVALID_TR_IFG] Bits */
#define PCM_IFG_AM_INVALID_TR_IFG_OFS            ( 2)                            /*!< AM_INVALID_TR_IFG Bit Offset */
#define PCM_IFG_AM_INVALID_TR_IFG                ((uint32_t)0x00000004)          /*!< Active mode invalid transition flag */
/* PCM_IFG[DCDC_ERROR_IFG] Bits */
#define PCM_IFG_DCDC_ERROR_IFG_OFS               ( 6)                            /*!< DCDC_ERROR_IFG Bit Offset */
#define PCM_IFG_DCDC_ERROR_IFG                   ((uint32_t)0x00000040)          /*!< DC-DC error flag */
/* PCM_CLRIFG[CLR_LPM_INVALID_TR_IFG] Bits */
#define PCM_CLRIFG_CLR_LPM_INVALID_TR_IFG_OFS    ( 0)                            /*!< CLR_LPM_INVALID_TR_IFG Bit Offset */
#define PCM_CLRIFG_CLR_LPM_INVALID_TR_IFG        ((uint32_t)0x00000001)          /*!< Clear LPM invalid transition flag */
/* PCM_CLRIFG[CLR_LPM_INVALID_CLK_IFG] Bits */
#define PCM_CLRIFG_CLR_LPM_INVALID_CLK_IFG_OFS   ( 1)                            /*!< CLR_LPM_INVALID_CLK_IFG Bit Offset */
#define PCM_CLRIFG_CLR_LPM_INVALID_CLK_IFG       ((uint32_t)0x00000002)          /*!< Clear LPM invalid clock flag */
/* PCM_CLRIFG[CLR_AM_INVALID_TR_IFG] Bits */
#define PCM_CLRIFG_CLR_AM_INVALID_TR_IFG_OFS     ( 2)                            /*!< CLR_AM_INVALID_TR_IFG Bit Offset */
#define PCM_CLRIFG_CLR_AM_INVALID_TR_IFG         ((uint32_t)0x00000004)          /*!< Clear active mode invalid transition flag */
/* PCM_CLRIFG[CLR_DCDC_ERROR_IFG] Bits */
#define PCM_CLRIFG_CLR_DCDC_ERROR_IFG_OFS        ( 6)                            /*!< CLR_DCDC_ERROR_IFG Bit Offset */
#define PCM_CLRIFG_CLR_DCDC_ERROR_IFG            ((uint32_t)0x00000040)          /*!< Clear DC-DC error flag */
/* Pre-defined bitfield values */
#define PCM_CTL0_KEY_VAL                         ((uint32_t)0x695A0000)          /*!< PCM key value */
#define PCM_CTL1_KEY_VAL                         ((uint32_t)0x695A0000)          /*!< PCM key value */


/******************************************************************************
* PMAP Bits
******************************************************************************/
/* PMAP_CTL[LOCKED] Bits */
#define PMAP_CTL_LOCKED_OFS                      ( 0)                            /*!< PMAPLOCKED Bit Offset */
#define PMAP_CTL_LOCKED                          ((uint16_t)0x0001)              /*!< Port mapping lock bit */
/* PMAP_CTL[PRECFG] Bits */
#define PMAP_CTL_PRECFG_OFS                      ( 1)                            /*!< PMAPRECFG Bit Offset */
#define PMAP_CTL_PRECFG                          ((uint16_t)0x0002)              /*!< Port mapping reconfiguration control bit */
/* Pre-defined bitfield values */
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


/******************************************************************************
* PSS Bits
******************************************************************************/
/* PSS_KEY[KEY] Bits */
#define PSS_KEY_KEY_OFS                          ( 0)                            /*!< PSSKEY Bit Offset */
#define PSS_KEY_KEY_MASK                         ((uint32_t)0x0000FFFF)          /*!< PSSKEY Bit Mask */
/* PSS_CTL0[SVSMHOFF] Bits */
#define PSS_CTL0_SVSMHOFF_OFS                    ( 0)                            /*!< SVSMHOFF Bit Offset */
#define PSS_CTL0_SVSMHOFF                        ((uint32_t)0x00000001)          /*!< SVSM high-side off */
/* PSS_CTL0[SVSMHLP] Bits */
#define PSS_CTL0_SVSMHLP_OFS                     ( 1)                            /*!< SVSMHLP Bit Offset */
#define PSS_CTL0_SVSMHLP                         ((uint32_t)0x00000002)          /*!< SVSM high-side low power normal performance mode */
/* PSS_CTL0[SVSMHS] Bits */
#define PSS_CTL0_SVSMHS_OFS                      ( 2)                            /*!< SVSMHS Bit Offset */
#define PSS_CTL0_SVSMHS                          ((uint32_t)0x00000004)          /*!< Supply supervisor or monitor selection for the high-side */
/* PSS_CTL0[SVSMHTH] Bits */
#define PSS_CTL0_SVSMHTH_OFS                     ( 3)                            /*!< SVSMHTH Bit Offset */
#define PSS_CTL0_SVSMHTH_MASK                    ((uint32_t)0x00000038)          /*!< SVSMHTH Bit Mask */
/* PSS_CTL0[SVMHOE] Bits */
#define PSS_CTL0_SVMHOE_OFS                      ( 6)                            /*!< SVMHOE Bit Offset */
#define PSS_CTL0_SVMHOE                          ((uint32_t)0x00000040)          /*!< SVSM high-side output enable */
/* PSS_CTL0[SVMHOUTPOLAL] Bits */
#define PSS_CTL0_SVMHOUTPOLAL_OFS                ( 7)                            /*!< SVMHOUTPOLAL Bit Offset */
#define PSS_CTL0_SVMHOUTPOLAL                    ((uint32_t)0x00000080)          /*!< SVMHOUT pin polarity active low */
/* PSS_CTL0[DCDC_FORCE] Bits */
#define PSS_CTL0_DCDC_FORCE_OFS                  (10)                            /*!< DCDC_FORCE Bit Offset */
#define PSS_CTL0_DCDC_FORCE                      ((uint32_t)0x00000400)          /*!< Force DC-DC regulator operation */
/* PSS_CTL0[VCORETRAN] Bits */
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
/* PSS_IE[SVSMHIE] Bits */
#define PSS_IE_SVSMHIE_OFS                       ( 1)                            /*!< SVSMHIE Bit Offset */
#define PSS_IE_SVSMHIE                           ((uint32_t)0x00000002)          /*!< High-side SVSM interrupt enable */
/* PSS_IFG[SVSMHIFG] Bits */
#define PSS_IFG_SVSMHIFG_OFS                     ( 1)                            /*!< SVSMHIFG Bit Offset */
#define PSS_IFG_SVSMHIFG                         ((uint32_t)0x00000002)          /*!< High-side SVSM interrupt flag */
/* PSS_CLRIFG[CLRSVSMHIFG] Bits */
#define PSS_CLRIFG_CLRSVSMHIFG_OFS               ( 1)                            /*!< CLRSVSMHIFG Bit Offset */
#define PSS_CLRIFG_CLRSVSMHIFG                   ((uint32_t)0x00000002)          /*!< SVSMH clear interrupt flag */
/* Pre-defined bitfield values */
#define PSS_KEY_KEY_VAL                           ((uint32_t)0x0000695A)          /*!< PSS control key value */


/******************************************************************************
* REF_A Bits
******************************************************************************/
/* REF_A_CTL0[ON] Bits */
#define REF_A_CTL0_ON_OFS                        ( 0)                            /*!< REFON Bit Offset */
#define REF_A_CTL0_ON                            ((uint16_t)0x0001)              /*!< Reference enable */
/* REF_A_CTL0[OUT] Bits */
#define REF_A_CTL0_OUT_OFS                       ( 1)                            /*!< REFOUT Bit Offset */
#define REF_A_CTL0_OUT                           ((uint16_t)0x0002)              /*!< Reference output buffer */
/* REF_A_CTL0[TCOFF] Bits */
#define REF_A_CTL0_TCOFF_OFS                     ( 3)                            /*!< REFTCOFF Bit Offset */
#define REF_A_CTL0_TCOFF                         ((uint16_t)0x0008)              /*!< Temperature sensor disabled */
/* REF_A_CTL0[VSEL] Bits */
#define REF_A_CTL0_VSEL_OFS                      ( 4)                            /*!< REFVSEL Bit Offset */
#define REF_A_CTL0_VSEL_MASK                     ((uint16_t)0x0030)              /*!< REFVSEL Bit Mask */
#define REF_A_CTL0_VSEL0                         ((uint16_t)0x0010)              /*!< VSEL Bit 0 */
#define REF_A_CTL0_VSEL1                         ((uint16_t)0x0020)              /*!< VSEL Bit 1 */
#define REF_A_CTL0_VSEL_0                        ((uint16_t)0x0000)              /*!< 1.2 V available when reference requested or REFON = 1 */
#define REF_A_CTL0_VSEL_1                        ((uint16_t)0x0010)              /*!< 1.45 V available when reference requested or REFON = 1 */
#define REF_A_CTL0_VSEL_3                        ((uint16_t)0x0030)              /*!< 2.5 V available when reference requested or REFON = 1 */
/* REF_A_CTL0[GENOT] Bits */
#define REF_A_CTL0_GENOT_OFS                     ( 6)                            /*!< REFGENOT Bit Offset */
#define REF_A_CTL0_GENOT                         ((uint16_t)0x0040)              /*!< Reference generator one-time trigger */
/* REF_A_CTL0[BGOT] Bits */
#define REF_A_CTL0_BGOT_OFS                      ( 7)                            /*!< REFBGOT Bit Offset */
#define REF_A_CTL0_BGOT                          ((uint16_t)0x0080)              /*!< Bandgap and bandgap buffer one-time trigger */
/* REF_A_CTL0[GENACT] Bits */
#define REF_A_CTL0_GENACT_OFS                    ( 8)                            /*!< REFGENACT Bit Offset */
#define REF_A_CTL0_GENACT                        ((uint16_t)0x0100)              /*!< Reference generator active */
/* REF_A_CTL0[BGACT] Bits */
#define REF_A_CTL0_BGACT_OFS                     ( 9)                            /*!< REFBGACT Bit Offset */
#define REF_A_CTL0_BGACT                         ((uint16_t)0x0200)              /*!< Reference bandgap active */
/* REF_A_CTL0[GENBUSY] Bits */
#define REF_A_CTL0_GENBUSY_OFS                   (10)                            /*!< REFGENBUSY Bit Offset */
#define REF_A_CTL0_GENBUSY                       ((uint16_t)0x0400)              /*!< Reference generator busy */
/* REF_A_CTL0[BGMODE] Bits */
#define REF_A_CTL0_BGMODE_OFS                    (11)                            /*!< BGMODE Bit Offset */
#define REF_A_CTL0_BGMODE                        ((uint16_t)0x0800)              /*!< Bandgap mode */
/* REF_A_CTL0[GENRDY] Bits */
#define REF_A_CTL0_GENRDY_OFS                    (12)                            /*!< REFGENRDY Bit Offset */
#define REF_A_CTL0_GENRDY                        ((uint16_t)0x1000)              /*!< Variable reference voltage ready status */
/* REF_A_CTL0[BGRDY] Bits */
#define REF_A_CTL0_BGRDY_OFS                     (13)                            /*!< REFBGRDY Bit Offset */
#define REF_A_CTL0_BGRDY                         ((uint16_t)0x2000)              /*!< Buffered bandgap voltage ready status */

/******************************************************************************
* RSTCTL Bits
******************************************************************************/
/* RSTCTL_RESET_REQ[SOFT_REQ] Bits */
#define RSTCTL_RESET_REQ_SOFT_REQ_OFS            ( 0)                            /*!< SOFT_REQ Bit Offset */
#define RSTCTL_RESET_REQ_SOFT_REQ                ((uint32_t)0x00000001)          /*!< Soft Reset request */
/* RSTCTL_RESET_REQ[HARD_REQ] Bits */
#define RSTCTL_RESET_REQ_HARD_REQ_OFS            ( 1)                            /*!< HARD_REQ Bit Offset */
#define RSTCTL_RESET_REQ_HARD_REQ                ((uint32_t)0x00000002)          /*!< Hard Reset request */
/* RSTCTL_RESET_REQ[RSTKEY] Bits */
#define RSTCTL_RESET_REQ_RSTKEY_OFS              ( 8)                            /*!< RSTKEY Bit Offset */
#define RSTCTL_RESET_REQ_RSTKEY_MASK             ((uint32_t)0x0000FF00)          /*!< RSTKEY Bit Mask */
/* RSTCTL_HARDRESET_STAT[SRC0] Bits */
#define RSTCTL_HARDRESET_STAT_SRC0_OFS           ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC0               ((uint32_t)0x00000001)          /*!< Indicates that SRC0 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC1] Bits */
#define RSTCTL_HARDRESET_STAT_SRC1_OFS           ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC1               ((uint32_t)0x00000002)          /*!< Indicates that SRC1 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC2] Bits */
#define RSTCTL_HARDRESET_STAT_SRC2_OFS           ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC2               ((uint32_t)0x00000004)          /*!< Indicates that SRC2 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC3] Bits */
#define RSTCTL_HARDRESET_STAT_SRC3_OFS           ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC3               ((uint32_t)0x00000008)          /*!< Indicates that SRC3 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC4] Bits */
#define RSTCTL_HARDRESET_STAT_SRC4_OFS           ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC4               ((uint32_t)0x00000010)          /*!< Indicates that SRC4 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC5] Bits */
#define RSTCTL_HARDRESET_STAT_SRC5_OFS           ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC5               ((uint32_t)0x00000020)          /*!< Indicates that SRC5 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC6] Bits */
#define RSTCTL_HARDRESET_STAT_SRC6_OFS           ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC6               ((uint32_t)0x00000040)          /*!< Indicates that SRC6 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC7] Bits */
#define RSTCTL_HARDRESET_STAT_SRC7_OFS           ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC7               ((uint32_t)0x00000080)          /*!< Indicates that SRC7 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC8] Bits */
#define RSTCTL_HARDRESET_STAT_SRC8_OFS           ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC8               ((uint32_t)0x00000100)          /*!< Indicates that SRC8 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC9] Bits */
#define RSTCTL_HARDRESET_STAT_SRC9_OFS           ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC9               ((uint32_t)0x00000200)          /*!< Indicates that SRC9 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC10] Bits */
#define RSTCTL_HARDRESET_STAT_SRC10_OFS          (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC10              ((uint32_t)0x00000400)          /*!< Indicates that SRC10 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC11] Bits */
#define RSTCTL_HARDRESET_STAT_SRC11_OFS          (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC11              ((uint32_t)0x00000800)          /*!< Indicates that SRC11 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC12] Bits */
#define RSTCTL_HARDRESET_STAT_SRC12_OFS          (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC12              ((uint32_t)0x00001000)          /*!< Indicates that SRC12 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC13] Bits */
#define RSTCTL_HARDRESET_STAT_SRC13_OFS          (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC13              ((uint32_t)0x00002000)          /*!< Indicates that SRC13 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC14] Bits */
#define RSTCTL_HARDRESET_STAT_SRC14_OFS          (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC14              ((uint32_t)0x00004000)          /*!< Indicates that SRC14 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_STAT[SRC15] Bits */
#define RSTCTL_HARDRESET_STAT_SRC15_OFS          (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_HARDRESET_STAT_SRC15              ((uint32_t)0x00008000)          /*!< Indicates that SRC15 was the source of the Hard Reset */
/* RSTCTL_HARDRESET_CLR[SRC0] Bits */
#define RSTCTL_HARDRESET_CLR_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC1] Bits */
#define RSTCTL_HARDRESET_CLR_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC2] Bits */
#define RSTCTL_HARDRESET_CLR_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC3] Bits */
#define RSTCTL_HARDRESET_CLR_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC4] Bits */
#define RSTCTL_HARDRESET_CLR_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC5] Bits */
#define RSTCTL_HARDRESET_CLR_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC6] Bits */
#define RSTCTL_HARDRESET_CLR_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC7] Bits */
#define RSTCTL_HARDRESET_CLR_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC8] Bits */
#define RSTCTL_HARDRESET_CLR_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC9] Bits */
#define RSTCTL_HARDRESET_CLR_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC10] Bits */
#define RSTCTL_HARDRESET_CLR_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC11] Bits */
#define RSTCTL_HARDRESET_CLR_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC12] Bits */
#define RSTCTL_HARDRESET_CLR_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC13] Bits */
#define RSTCTL_HARDRESET_CLR_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC14] Bits */
#define RSTCTL_HARDRESET_CLR_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HARDRESET_STAT */
/* RSTCTL_HARDRESET_CLR[SRC15] Bits */
#define RSTCTL_HARDRESET_CLR_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_HARDRESET_CLR_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_HRDRESETSTAT_REG */
/* RSTCTL_HARDRESET_SET[SRC0] Bits */
#define RSTCTL_HARDRESET_SET_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC1] Bits */
#define RSTCTL_HARDRESET_SET_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC2] Bits */
#define RSTCTL_HARDRESET_SET_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC3] Bits */
#define RSTCTL_HARDRESET_SET_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC4] Bits */
#define RSTCTL_HARDRESET_SET_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC5] Bits */
#define RSTCTL_HARDRESET_SET_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC6] Bits */
#define RSTCTL_HARDRESET_SET_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC7] Bits */
#define RSTCTL_HARDRESET_SET_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC8] Bits */
#define RSTCTL_HARDRESET_SET_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC9] Bits */
#define RSTCTL_HARDRESET_SET_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC10] Bits */
#define RSTCTL_HARDRESET_SET_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC11] Bits */
#define RSTCTL_HARDRESET_SET_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC12] Bits */
#define RSTCTL_HARDRESET_SET_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC13] Bits */
#define RSTCTL_HARDRESET_SET_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC14] Bits */
#define RSTCTL_HARDRESET_SET_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_HARDRESET_SET[SRC15] Bits */
#define RSTCTL_HARDRESET_SET_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_HARDRESET_SET_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_HARDRESET_STAT (and  */
                                                                                 /* initiates a Hard Reset) */
/* RSTCTL_SOFTRESET_STAT[SRC0] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC0_OFS           ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC0               ((uint32_t)0x00000001)          /*!< If 1, indicates that SRC0 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC1] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC1_OFS           ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC1               ((uint32_t)0x00000002)          /*!< If 1, indicates that SRC1 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC2] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC2_OFS           ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC2               ((uint32_t)0x00000004)          /*!< If 1, indicates that SRC2 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC3] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC3_OFS           ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC3               ((uint32_t)0x00000008)          /*!< If 1, indicates that SRC3 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC4] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC4_OFS           ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC4               ((uint32_t)0x00000010)          /*!< If 1, indicates that SRC4 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC5] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC5_OFS           ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC5               ((uint32_t)0x00000020)          /*!< If 1, indicates that SRC5 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC6] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC6_OFS           ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC6               ((uint32_t)0x00000040)          /*!< If 1, indicates that SRC6 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC7] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC7_OFS           ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC7               ((uint32_t)0x00000080)          /*!< If 1, indicates that SRC7 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC8] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC8_OFS           ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC8               ((uint32_t)0x00000100)          /*!< If 1, indicates that SRC8 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC9] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC9_OFS           ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC9               ((uint32_t)0x00000200)          /*!< If 1, indicates that SRC9 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC10] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC10_OFS          (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC10              ((uint32_t)0x00000400)          /*!< If 1, indicates that SRC10 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC11] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC11_OFS          (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC11              ((uint32_t)0x00000800)          /*!< If 1, indicates that SRC11 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC12] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC12_OFS          (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC12              ((uint32_t)0x00001000)          /*!< If 1, indicates that SRC12 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC13] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC13_OFS          (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC13              ((uint32_t)0x00002000)          /*!< If 1, indicates that SRC13 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC14] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC14_OFS          (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC14              ((uint32_t)0x00004000)          /*!< If 1, indicates that SRC14 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_STAT[SRC15] Bits */
#define RSTCTL_SOFTRESET_STAT_SRC15_OFS          (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_SOFTRESET_STAT_SRC15              ((uint32_t)0x00008000)          /*!< If 1, indicates that SRC15 was the source of the Soft Reset */
/* RSTCTL_SOFTRESET_CLR[SRC0] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC1] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC2] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC3] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC4] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC5] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC6] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC7] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC8] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC9] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC10] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC11] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC12] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC13] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC14] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_CLR[SRC15] Bits */
#define RSTCTL_SOFTRESET_CLR_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_SOFTRESET_CLR_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 clears the corresponding bit in the RSTCTL_SOFTRESET_STAT */
/* RSTCTL_SOFTRESET_SET[SRC0] Bits */
#define RSTCTL_SOFTRESET_SET_SRC0_OFS            ( 0)                            /*!< SRC0 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC0                ((uint32_t)0x00000001)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC1] Bits */
#define RSTCTL_SOFTRESET_SET_SRC1_OFS            ( 1)                            /*!< SRC1 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC1                ((uint32_t)0x00000002)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC2] Bits */
#define RSTCTL_SOFTRESET_SET_SRC2_OFS            ( 2)                            /*!< SRC2 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC2                ((uint32_t)0x00000004)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC3] Bits */
#define RSTCTL_SOFTRESET_SET_SRC3_OFS            ( 3)                            /*!< SRC3 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC3                ((uint32_t)0x00000008)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC4] Bits */
#define RSTCTL_SOFTRESET_SET_SRC4_OFS            ( 4)                            /*!< SRC4 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC4                ((uint32_t)0x00000010)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC5] Bits */
#define RSTCTL_SOFTRESET_SET_SRC5_OFS            ( 5)                            /*!< SRC5 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC5                ((uint32_t)0x00000020)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC6] Bits */
#define RSTCTL_SOFTRESET_SET_SRC6_OFS            ( 6)                            /*!< SRC6 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC6                ((uint32_t)0x00000040)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC7] Bits */
#define RSTCTL_SOFTRESET_SET_SRC7_OFS            ( 7)                            /*!< SRC7 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC7                ((uint32_t)0x00000080)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC8] Bits */
#define RSTCTL_SOFTRESET_SET_SRC8_OFS            ( 8)                            /*!< SRC8 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC8                ((uint32_t)0x00000100)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC9] Bits */
#define RSTCTL_SOFTRESET_SET_SRC9_OFS            ( 9)                            /*!< SRC9 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC9                ((uint32_t)0x00000200)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC10] Bits */
#define RSTCTL_SOFTRESET_SET_SRC10_OFS           (10)                            /*!< SRC10 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC10               ((uint32_t)0x00000400)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC11] Bits */
#define RSTCTL_SOFTRESET_SET_SRC11_OFS           (11)                            /*!< SRC11 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC11               ((uint32_t)0x00000800)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC12] Bits */
#define RSTCTL_SOFTRESET_SET_SRC12_OFS           (12)                            /*!< SRC12 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC12               ((uint32_t)0x00001000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC13] Bits */
#define RSTCTL_SOFTRESET_SET_SRC13_OFS           (13)                            /*!< SRC13 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC13               ((uint32_t)0x00002000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC14] Bits */
#define RSTCTL_SOFTRESET_SET_SRC14_OFS           (14)                            /*!< SRC14 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC14               ((uint32_t)0x00004000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_SOFTRESET_SET[SRC15] Bits */
#define RSTCTL_SOFTRESET_SET_SRC15_OFS           (15)                            /*!< SRC15 Bit Offset */
#define RSTCTL_SOFTRESET_SET_SRC15               ((uint32_t)0x00008000)          /*!< Write 1 sets the corresponding bit in the RSTCTL_SOFTRESET_STAT (and  */
                                                                                 /* initiates a Soft Reset) */
/* RSTCTL_PSSRESET_STAT[SVSMH] Bits */
#define RSTCTL_PSSRESET_STAT_SVSMH_OFS           ( 1)                            /*!< SVSMH Bit Offset */
#define RSTCTL_PSSRESET_STAT_SVSMH               ((uint32_t)0x00000002)          /*!< Indicates if POR was caused by an SVSMH trip condition int the PSS */
/* RSTCTL_PSSRESET_STAT[BGREF] Bits */
#define RSTCTL_PSSRESET_STAT_BGREF_OFS           ( 2)                            /*!< BGREF Bit Offset */
#define RSTCTL_PSSRESET_STAT_BGREF               ((uint32_t)0x00000004)          /*!< Indicates if POR was caused by a BGREF not okay condition in the PSS */
/* RSTCTL_PSSRESET_STAT[VCCDET] Bits */
#define RSTCTL_PSSRESET_STAT_VCCDET_OFS          ( 3)                            /*!< VCCDET Bit Offset */
#define RSTCTL_PSSRESET_STAT_VCCDET              ((uint32_t)0x00000008)          /*!< Indicates if POR was caused by a VCCDET trip condition in the PSS */
/* RSTCTL_PSSRESET_CLR[CLR] Bits */
#define RSTCTL_PSSRESET_CLR_CLR_OFS              ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_PSSRESET_CLR_CLR                  ((uint32_t)0x00000001)          /*!< Write 1 clears all PSS Reset Flags in the RSTCTL_PSSRESET_STAT */
/* RSTCTL_PCMRESET_STAT[LPM35] Bits */
#define RSTCTL_PCMRESET_STAT_LPM35_OFS           ( 0)                            /*!< LPM35 Bit Offset */
#define RSTCTL_PCMRESET_STAT_LPM35               ((uint32_t)0x00000001)          /*!< Indicates if POR was caused by PCM due to an exit from LPM3.5 */
/* RSTCTL_PCMRESET_STAT[LPM45] Bits */
#define RSTCTL_PCMRESET_STAT_LPM45_OFS           ( 1)                            /*!< LPM45 Bit Offset */
#define RSTCTL_PCMRESET_STAT_LPM45               ((uint32_t)0x00000002)          /*!< Indicates if POR was caused by PCM due to an exit from LPM4.5 */
/* RSTCTL_PCMRESET_CLR[CLR] Bits */
#define RSTCTL_PCMRESET_CLR_CLR_OFS              ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_PCMRESET_CLR_CLR                  ((uint32_t)0x00000001)          /*!< Write 1 clears all PCM Reset Flags in the RSTCTL_PCMRESET_STAT */
/* RSTCTL_PINRESET_STAT[RSTNMI] Bits */
#define RSTCTL_PINRESET_STAT_RSTNMI_OFS          ( 0)                            /*!< RSTNMI Bit Offset */
#define RSTCTL_PINRESET_STAT_RSTNMI              ((uint32_t)0x00000001)          /*!< POR was caused by RSTn/NMI pin based reset event */
/* RSTCTL_PINRESET_CLR[CLR] Bits */
#define RSTCTL_PINRESET_CLR_CLR_OFS              ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_PINRESET_CLR_CLR                  ((uint32_t)0x00000001)          /*!< Write 1 clears the RSTn/NMI Pin Reset Flag in RSTCTL_PINRESET_STAT */
/* RSTCTL_REBOOTRESET_STAT[REBOOT] Bits */
#define RSTCTL_REBOOTRESET_STAT_REBOOT_OFS       ( 0)                            /*!< REBOOT Bit Offset */
#define RSTCTL_REBOOTRESET_STAT_REBOOT           ((uint32_t)0x00000001)          /*!< Indicates if Reboot reset was caused by the SYSCTL module. */
/* RSTCTL_REBOOTRESET_CLR[CLR] Bits */
#define RSTCTL_REBOOTRESET_CLR_CLR_OFS           ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_REBOOTRESET_CLR_CLR               ((uint32_t)0x00000001)          /*!< Write 1 clears the Reboot Reset Flag in RSTCTL_REBOOTRESET_STAT */
/* RSTCTL_CSRESET_STAT[DCOR_SHT] Bits */
#define RSTCTL_CSRESET_STAT_DCOR_SHT_OFS         ( 0)                            /*!< DCOR_SHT Bit Offset */
#define RSTCTL_CSRESET_STAT_DCOR_SHT             ((uint32_t)0x00000001)          /*!< Indicates if POR was caused by DCO short circuit fault in the external  */
                                                                                 /* resistor mode */
/* RSTCTL_CSRESET_CLR[CLR] Bits */
#define RSTCTL_CSRESET_CLR_CLR_OFS               ( 0)                            /*!< CLR Bit Offset */
#define RSTCTL_CSRESET_CLR_CLR                   ((uint32_t)0x00000001)          /*!< Write 1 clears the DCOR_SHT Flag in RSTCTL_CSRESET_STAT as well as  */
                                                                                 /* DCOR_SHTIFG flag in CSIFG register of clock system */
/* Pre-defined bitfield values */
#define RSTCTL_RESETREQ_RSTKEY_VAL                 ((uint32_t)0x00006900)          /*!< Key value to enable writes to bits 1-0 */


/******************************************************************************
* RTC_C Bits
******************************************************************************/
/* RTC_C_CTL0[RDYIFG] Bits */
#define RTC_C_CTL0_RDYIFG_OFS                    ( 0)                            /*!< RTCRDYIFG Bit Offset */
#define RTC_C_CTL0_RDYIFG                        ((uint16_t)0x0001)              /*!< Real-time clock ready interrupt flag */
/* RTC_C_CTL0[AIFG] Bits */
#define RTC_C_CTL0_AIFG_OFS                      ( 1)                            /*!< RTCAIFG Bit Offset */
#define RTC_C_CTL0_AIFG                          ((uint16_t)0x0002)              /*!< Real-time clock alarm interrupt flag */
/* RTC_C_CTL0[TEVIFG] Bits */
#define RTC_C_CTL0_TEVIFG_OFS                    ( 2)                            /*!< RTCTEVIFG Bit Offset */
#define RTC_C_CTL0_TEVIFG                        ((uint16_t)0x0004)              /*!< Real-time clock time event interrupt flag */
/* RTC_C_CTL0[OFIFG] Bits */
#define RTC_C_CTL0_OFIFG_OFS                     ( 3)                            /*!< RTCOFIFG Bit Offset */
#define RTC_C_CTL0_OFIFG                         ((uint16_t)0x0008)              /*!< 32-kHz crystal oscillator fault interrupt flag */
/* RTC_C_CTL0[RDYIE] Bits */
#define RTC_C_CTL0_RDYIE_OFS                     ( 4)                            /*!< RTCRDYIE Bit Offset */
#define RTC_C_CTL0_RDYIE                         ((uint16_t)0x0010)              /*!< Real-time clock ready interrupt enable */
/* RTC_C_CTL0[AIE] Bits */
#define RTC_C_CTL0_AIE_OFS                       ( 5)                            /*!< RTCAIE Bit Offset */
#define RTC_C_CTL0_AIE                           ((uint16_t)0x0020)              /*!< Real-time clock alarm interrupt enable */
/* RTC_C_CTL0[TEVIE] Bits */
#define RTC_C_CTL0_TEVIE_OFS                     ( 6)                            /*!< RTCTEVIE Bit Offset */
#define RTC_C_CTL0_TEVIE                         ((uint16_t)0x0040)              /*!< Real-time clock time event interrupt enable */
/* RTC_C_CTL0[OFIE] Bits */
#define RTC_C_CTL0_OFIE_OFS                      ( 7)                            /*!< RTCOFIE Bit Offset */
#define RTC_C_CTL0_OFIE                          ((uint16_t)0x0080)              /*!< 32-kHz crystal oscillator fault interrupt enable */
/* RTC_C_CTL0[KEY] Bits */
#define RTC_C_CTL0_KEY_OFS                       ( 8)                            /*!< RTCKEY Bit Offset */
#define RTC_C_CTL0_KEY_MASK                      ((uint16_t)0xFF00)              /*!< RTCKEY Bit Mask */
/* RTC_C_CTL13[TEV] Bits */
#define RTC_C_CTL13_TEV_OFS                      ( 0)                            /*!< RTCTEV Bit Offset */
#define RTC_C_CTL13_TEV_MASK                     ((uint16_t)0x0003)              /*!< RTCTEV Bit Mask */
#define RTC_C_CTL13_TEV0                         ((uint16_t)0x0001)              /*!< TEV Bit 0 */
#define RTC_C_CTL13_TEV1                         ((uint16_t)0x0002)              /*!< TEV Bit 1 */
#define RTC_C_CTL13_TEV_0                        ((uint16_t)0x0000)              /*!< Minute changed */
#define RTC_C_CTL13_TEV_1                        ((uint16_t)0x0001)              /*!< Hour changed */
#define RTC_C_CTL13_TEV_2                        ((uint16_t)0x0002)              /*!< Every day at midnight (00:00) */
#define RTC_C_CTL13_TEV_3                        ((uint16_t)0x0003)              /*!< Every day at noon (12:00) */
/* RTC_C_CTL13[SSEL] Bits */
#define RTC_C_CTL13_SSEL_OFS                     ( 2)                            /*!< RTCSSEL Bit Offset */
#define RTC_C_CTL13_SSEL_MASK                    ((uint16_t)0x000C)              /*!< RTCSSEL Bit Mask */
#define RTC_C_CTL13_SSEL0                        ((uint16_t)0x0004)              /*!< SSEL Bit 0 */
#define RTC_C_CTL13_SSEL1                        ((uint16_t)0x0008)              /*!< SSEL Bit 1 */
#define RTC_C_CTL13_SSEL_0                       ((uint16_t)0x0000)              /*!< BCLK */
#define RTC_C_CTL13_SSEL__BCLK                   ((uint16_t)0x0000)              /*!< BCLK */
/* RTC_C_CTL13[RDY] Bits */
#define RTC_C_CTL13_RDY_OFS                      ( 4)                            /*!< RTCRDY Bit Offset */
#define RTC_C_CTL13_RDY                          ((uint16_t)0x0010)              /*!< Real-time clock ready */
/* RTC_C_CTL13[MODE] Bits */
#define RTC_C_CTL13_MODE_OFS                     ( 5)                            /*!< RTCMODE Bit Offset */
#define RTC_C_CTL13_MODE                         ((uint16_t)0x0020)              
/* RTC_C_CTL13[HOLD] Bits */
#define RTC_C_CTL13_HOLD_OFS                     ( 6)                            /*!< RTCHOLD Bit Offset */
#define RTC_C_CTL13_HOLD                         ((uint16_t)0x0040)              /*!< Real-time clock hold */
/* RTC_C_CTL13[BCD] Bits */
#define RTC_C_CTL13_BCD_OFS                      ( 7)                            /*!< RTCBCD Bit Offset */
#define RTC_C_CTL13_BCD                          ((uint16_t)0x0080)              /*!< Real-time clock BCD select */
/* RTC_C_CTL13[CALF] Bits */
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
/* RTC_C_OCAL[OCAL] Bits */
#define RTC_C_OCAL_OCAL_OFS                      ( 0)                            /*!< RTCOCAL Bit Offset */
#define RTC_C_OCAL_OCAL_MASK                     ((uint16_t)0x00FF)              /*!< RTCOCAL Bit Mask */
/* RTC_C_OCAL[OCALS] Bits */
#define RTC_C_OCAL_OCALS_OFS                     (15)                            /*!< RTCOCALS Bit Offset */
#define RTC_C_OCAL_OCALS                         ((uint16_t)0x8000)              /*!< Real-time clock offset error calibration sign */
/* RTC_C_TCMP[TCMPx] Bits */
#define RTC_C_TCMP_TCMPX_OFS                     ( 0)                            /*!< RTCTCMP Bit Offset */
#define RTC_C_TCMP_TCMPX_MASK                    ((uint16_t)0x00FF)              /*!< RTCTCMP Bit Mask */
/* RTC_C_TCMP[TCOK] Bits */
#define RTC_C_TCMP_TCOK_OFS                      (13)                            /*!< RTCTCOK Bit Offset */
#define RTC_C_TCMP_TCOK                          ((uint16_t)0x2000)              /*!< Real-time clock temperature compensation write OK */
/* RTC_C_TCMP[TCRDY] Bits */
#define RTC_C_TCMP_TCRDY_OFS                     (14)                            /*!< RTCTCRDY Bit Offset */
#define RTC_C_TCMP_TCRDY                         ((uint16_t)0x4000)              /*!< Real-time clock temperature compensation ready */
/* RTC_C_TCMP[TCMPS] Bits */
#define RTC_C_TCMP_TCMPS_OFS                     (15)                            /*!< RTCTCMPS Bit Offset */
#define RTC_C_TCMP_TCMPS                         ((uint16_t)0x8000)              /*!< Real-time clock temperature compensation sign */
/* RTC_C_PS0CTL[RT0PSIFG] Bits */
#define RTC_C_PS0CTL_RT0PSIFG_OFS                ( 0)                            /*!< RT0PSIFG Bit Offset */
#define RTC_C_PS0CTL_RT0PSIFG                    ((uint16_t)0x0001)              /*!< Prescale timer 0 interrupt flag */
/* RTC_C_PS0CTL[RT0PSIE] Bits */
#define RTC_C_PS0CTL_RT0PSIE_OFS                 ( 1)                            /*!< RT0PSIE Bit Offset */
#define RTC_C_PS0CTL_RT0PSIE                     ((uint16_t)0x0002)              /*!< Prescale timer 0 interrupt enable */
/* RTC_C_PS0CTL[RT0IP] Bits */
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
/* RTC_C_PS1CTL[RT1PSIFG] Bits */
#define RTC_C_PS1CTL_RT1PSIFG_OFS                ( 0)                            /*!< RT1PSIFG Bit Offset */
#define RTC_C_PS1CTL_RT1PSIFG                    ((uint16_t)0x0001)              /*!< Prescale timer 1 interrupt flag */
/* RTC_C_PS1CTL[RT1PSIE] Bits */
#define RTC_C_PS1CTL_RT1PSIE_OFS                 ( 1)                            /*!< RT1PSIE Bit Offset */
#define RTC_C_PS1CTL_RT1PSIE                     ((uint16_t)0x0002)              /*!< Prescale timer 1 interrupt enable */
/* RTC_C_PS1CTL[RT1IP] Bits */
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
/* RTC_C_PS[RT0PS] Bits */
#define RTC_C_PS_RT0PS_OFS                       ( 0)                            /*!< RT0PS Bit Offset */
#define RTC_C_PS_RT0PS_MASK                      ((uint16_t)0x00FF)              /*!< RT0PS Bit Mask */
/* RTC_C_PS[RT1PS] Bits */
#define RTC_C_PS_RT1PS_OFS                       ( 8)                            /*!< RT1PS Bit Offset */
#define RTC_C_PS_RT1PS_MASK                      ((uint16_t)0xFF00)              /*!< RT1PS Bit Mask */
/* RTC_C_TIM0[SEC] Bits */
#define RTC_C_TIM0_SEC_OFS                       ( 0)                            /*!< Seconds Bit Offset */
#define RTC_C_TIM0_SEC_MASK                      ((uint16_t)0x003F)              /*!< Seconds Bit Mask */
/* RTC_C_TIM0[MIN] Bits */
#define RTC_C_TIM0_MIN_OFS                       ( 8)                            /*!< Minutes Bit Offset */
#define RTC_C_TIM0_MIN_MASK                      ((uint16_t)0x3F00)              /*!< Minutes Bit Mask */
/* RTC_C_TIM0[SEC_LD] Bits */
#define RTC_C_TIM0_SEC_LD_OFS                    ( 0)                            /*!< SecondsLowDigit Bit Offset */
#define RTC_C_TIM0_SEC_LD_MASK                   ((uint16_t)0x000F)              /*!< SecondsLowDigit Bit Mask */
/* RTC_C_TIM0[SEC_HD] Bits */
#define RTC_C_TIM0_SEC_HD_OFS                    ( 4)                            /*!< SecondsHighDigit Bit Offset */
#define RTC_C_TIM0_SEC_HD_MASK                   ((uint16_t)0x0070)              /*!< SecondsHighDigit Bit Mask */
/* RTC_C_TIM0[MIN_LD] Bits */
#define RTC_C_TIM0_MIN_LD_OFS                    ( 8)                            /*!< MinutesLowDigit Bit Offset */
#define RTC_C_TIM0_MIN_LD_MASK                   ((uint16_t)0x0F00)              /*!< MinutesLowDigit Bit Mask */
/* RTC_C_TIM0[MIN_HD] Bits */
#define RTC_C_TIM0_MIN_HD_OFS                    (12)                            /*!< MinutesHighDigit Bit Offset */
#define RTC_C_TIM0_MIN_HD_MASK                   ((uint16_t)0x7000)              /*!< MinutesHighDigit Bit Mask */
/* RTC_C_TIM1[HOUR] Bits */
#define RTC_C_TIM1_HOUR_OFS                      ( 0)                            /*!< Hours Bit Offset */
#define RTC_C_TIM1_HOUR_MASK                     ((uint16_t)0x001F)              /*!< Hours Bit Mask */
/* RTC_C_TIM1[DOW] Bits */
#define RTC_C_TIM1_DOW_OFS                       ( 8)                            /*!< DayofWeek Bit Offset */
#define RTC_C_TIM1_DOW_MASK                      ((uint16_t)0x0700)              /*!< DayofWeek Bit Mask */
/* RTC_C_TIM1[HOUR_LD] Bits */
#define RTC_C_TIM1_HOUR_LD_OFS                   ( 0)                            /*!< HoursLowDigit Bit Offset */
#define RTC_C_TIM1_HOUR_LD_MASK                  ((uint16_t)0x000F)              /*!< HoursLowDigit Bit Mask */
/* RTC_C_TIM1[HOUR_HD] Bits */
#define RTC_C_TIM1_HOUR_HD_OFS                   ( 4)                            /*!< HoursHighDigit Bit Offset */
#define RTC_C_TIM1_HOUR_HD_MASK                  ((uint16_t)0x0030)              /*!< HoursHighDigit Bit Mask */
/* RTC_C_DATE[DAY] Bits */
#define RTC_C_DATE_DAY_OFS                       ( 0)                            /*!< Day Bit Offset */
#define RTC_C_DATE_DAY_MASK                      ((uint16_t)0x001F)              /*!< Day Bit Mask */
/* RTC_C_DATE[MON] Bits */
#define RTC_C_DATE_MON_OFS                       ( 8)                            /*!< Month Bit Offset */
#define RTC_C_DATE_MON_MASK                      ((uint16_t)0x0F00)              /*!< Month Bit Mask */
/* RTC_C_DATE[DAY_LD] Bits */
#define RTC_C_DATE_DAY_LD_OFS                    ( 0)                            /*!< DayLowDigit Bit Offset */
#define RTC_C_DATE_DAY_LD_MASK                   ((uint16_t)0x000F)              /*!< DayLowDigit Bit Mask */
/* RTC_C_DATE[DAY_HD] Bits */
#define RTC_C_DATE_DAY_HD_OFS                    ( 4)                            /*!< DayHighDigit Bit Offset */
#define RTC_C_DATE_DAY_HD_MASK                   ((uint16_t)0x0030)              /*!< DayHighDigit Bit Mask */
/* RTC_C_DATE[MON_LD] Bits */
#define RTC_C_DATE_MON_LD_OFS                    ( 8)                            /*!< MonthLowDigit Bit Offset */
#define RTC_C_DATE_MON_LD_MASK                   ((uint16_t)0x0F00)              /*!< MonthLowDigit Bit Mask */
/* RTC_C_DATE[MON_HD] Bits */
#define RTC_C_DATE_MON_HD_OFS                    (12)                            /*!< MonthHighDigit Bit Offset */
#define RTC_C_DATE_MON_HD                        ((uint16_t)0x1000)              /*!< Month  high digit (0 or 1) */
/* RTC_C_YEAR[YEAR_LB] Bits */
#define RTC_C_YEAR_YEAR_LB_OFS                   ( 0)                            /*!< YearLowByte Bit Offset */
#define RTC_C_YEAR_YEAR_LB_MASK                  ((uint16_t)0x00FF)              /*!< YearLowByte Bit Mask */
/* RTC_C_YEAR[YEAR_HB] Bits */
#define RTC_C_YEAR_YEAR_HB_OFS                   ( 8)                            /*!< YearHighByte Bit Offset */
#define RTC_C_YEAR_YEAR_HB_MASK                  ((uint16_t)0x0F00)              /*!< YearHighByte Bit Mask */
/* RTC_C_YEAR[YEAR] Bits */
#define RTC_C_YEAR_YEAR_OFS                      ( 0)                            /*!< Year Bit Offset */
#define RTC_C_YEAR_YEAR_MASK                     ((uint16_t)0x000F)              /*!< Year Bit Mask */
/* RTC_C_YEAR[DEC] Bits */
#define RTC_C_YEAR_DEC_OFS                       ( 4)                            /*!< Decade Bit Offset */
#define RTC_C_YEAR_DEC_MASK                      ((uint16_t)0x00F0)              /*!< Decade Bit Mask */
/* RTC_C_YEAR[CENT_LD] Bits */
#define RTC_C_YEAR_CENT_LD_OFS                   ( 8)                            /*!< CenturyLowDigit Bit Offset */
#define RTC_C_YEAR_CENT_LD_MASK                  ((uint16_t)0x0F00)              /*!< CenturyLowDigit Bit Mask */
/* RTC_C_YEAR[CENT_HD] Bits */
#define RTC_C_YEAR_CENT_HD_OFS                   (12)                            /*!< CenturyHighDigit Bit Offset */
#define RTC_C_YEAR_CENT_HD_MASK                  ((uint16_t)0x7000)              /*!< CenturyHighDigit Bit Mask */
/* RTC_C_AMINHR[MIN] Bits */
#define RTC_C_AMINHR_MIN_OFS                     ( 0)                            /*!< Minutes Bit Offset */
#define RTC_C_AMINHR_MIN_MASK                    ((uint16_t)0x003F)              /*!< Minutes Bit Mask */
/* RTC_C_AMINHR[MINAE] Bits */
#define RTC_C_AMINHR_MINAE_OFS                   ( 7)                            /*!< MINAE Bit Offset */
#define RTC_C_AMINHR_MINAE                       ((uint16_t)0x0080)              /*!< Alarm enable */
/* RTC_C_AMINHR[HOUR] Bits */
#define RTC_C_AMINHR_HOUR_OFS                    ( 8)                            /*!< Hours Bit Offset */
#define RTC_C_AMINHR_HOUR_MASK                   ((uint16_t)0x1F00)              /*!< Hours Bit Mask */
/* RTC_C_AMINHR[HOURAE] Bits */
#define RTC_C_AMINHR_HOURAE_OFS                  (15)                            /*!< HOURAE Bit Offset */
#define RTC_C_AMINHR_HOURAE                      ((uint16_t)0x8000)              /*!< Alarm enable */
/* RTC_C_AMINHR[MIN_LD] Bits */
#define RTC_C_AMINHR_MIN_LD_OFS                  ( 0)                            /*!< MinutesLowDigit Bit Offset */
#define RTC_C_AMINHR_MIN_LD_MASK                 ((uint16_t)0x000F)              /*!< MinutesLowDigit Bit Mask */
/* RTC_C_AMINHR[MIN_HD] Bits */
#define RTC_C_AMINHR_MIN_HD_OFS                  ( 4)                            /*!< MinutesHighDigit Bit Offset */
#define RTC_C_AMINHR_MIN_HD_MASK                 ((uint16_t)0x0070)              /*!< MinutesHighDigit Bit Mask */
/* RTC_C_AMINHR[HOUR_LD] Bits */
#define RTC_C_AMINHR_HOUR_LD_OFS                 ( 8)                            /*!< HoursLowDigit Bit Offset */
#define RTC_C_AMINHR_HOUR_LD_MASK                ((uint16_t)0x0F00)              /*!< HoursLowDigit Bit Mask */
/* RTC_C_AMINHR[HOUR_HD] Bits */
#define RTC_C_AMINHR_HOUR_HD_OFS                 (12)                            /*!< HoursHighDigit Bit Offset */
#define RTC_C_AMINHR_HOUR_HD_MASK                ((uint16_t)0x3000)              /*!< HoursHighDigit Bit Mask */
/* RTC_C_ADOWDAY[DOW] Bits */
#define RTC_C_ADOWDAY_DOW_OFS                    ( 0)                            /*!< DayofWeek Bit Offset */
#define RTC_C_ADOWDAY_DOW_MASK                   ((uint16_t)0x0007)              /*!< DayofWeek Bit Mask */
/* RTC_C_ADOWDAY[DOWAE] Bits */
#define RTC_C_ADOWDAY_DOWAE_OFS                  ( 7)                            /*!< DOWAE Bit Offset */
#define RTC_C_ADOWDAY_DOWAE                      ((uint16_t)0x0080)              /*!< Alarm enable */
/* RTC_C_ADOWDAY[DAY] Bits */
#define RTC_C_ADOWDAY_DAY_OFS                    ( 8)                            /*!< DayofMonth Bit Offset */
#define RTC_C_ADOWDAY_DAY_MASK                   ((uint16_t)0x1F00)              /*!< DayofMonth Bit Mask */
/* RTC_C_ADOWDAY[DAYAE] Bits */
#define RTC_C_ADOWDAY_DAYAE_OFS                  (15)                            /*!< DAYAE Bit Offset */
#define RTC_C_ADOWDAY_DAYAE                      ((uint16_t)0x8000)              /*!< Alarm enable */
/* RTC_C_ADOWDAY[DAY_LD] Bits */
#define RTC_C_ADOWDAY_DAY_LD_OFS                 ( 8)                            /*!< DayLowDigit Bit Offset */
#define RTC_C_ADOWDAY_DAY_LD_MASK                ((uint16_t)0x0F00)              /*!< DayLowDigit Bit Mask */
/* RTC_C_ADOWDAY[DAY_HD] Bits */
#define RTC_C_ADOWDAY_DAY_HD_OFS                 (12)                            /*!< DayHighDigit Bit Offset */
#define RTC_C_ADOWDAY_DAY_HD_MASK                ((uint16_t)0x3000)              /*!< DayHighDigit Bit Mask */
/* Pre-defined bitfield values */
#define RTC_C_KEY                                 ((uint16_t)0xA500)              /*!< RTC_C Key Value for RTC_C write access */
#define RTC_C_KEY_H                               ((uint16_t)0x00A5)              /*!< RTC_C Key Value for RTC_C write access */
#define RTC_C_KEY_VAL                             ((uint16_t)0xA500)              /*!< RTC_C Key Value for RTC_C write access */


/******************************************************************************
* SCB Bits
******************************************************************************/
/* SCB_PFR0[STATE0] Bits */
#define SCB_PFR0_STATE0_OFS                      ( 0)                            /*!< STATE0 Bit Offset */
#define SCB_PFR0_STATE0_MASK                     ((uint32_t)0x0000000F)          /*!< STATE0 Bit Mask */
#define SCB_PFR0_STATE00                         ((uint32_t)0x00000001)          /*!< STATE0 Bit 0 */
#define SCB_PFR0_STATE01                         ((uint32_t)0x00000002)          /*!< STATE0 Bit 1 */
#define SCB_PFR0_STATE02                         ((uint32_t)0x00000004)          /*!< STATE0 Bit 2 */
#define SCB_PFR0_STATE03                         ((uint32_t)0x00000008)          /*!< STATE0 Bit 3 */
#define SCB_PFR0_STATE0_0                        ((uint32_t)0x00000000)          /*!< no ARM encoding */
#define SCB_PFR0_STATE0_1                        ((uint32_t)0x00000001)          /*!< N/A */
/* SCB_PFR0[STATE1] Bits */
#define SCB_PFR0_STATE1_OFS                      ( 4)                            /*!< STATE1 Bit Offset */
#define SCB_PFR0_STATE1_MASK                     ((uint32_t)0x000000F0)          /*!< STATE1 Bit Mask */
#define SCB_PFR0_STATE10                         ((uint32_t)0x00000010)          /*!< STATE1 Bit 0 */
#define SCB_PFR0_STATE11                         ((uint32_t)0x00000020)          /*!< STATE1 Bit 1 */
#define SCB_PFR0_STATE12                         ((uint32_t)0x00000040)          /*!< STATE1 Bit 2 */
#define SCB_PFR0_STATE13                         ((uint32_t)0x00000080)          /*!< STATE1 Bit 3 */
#define SCB_PFR0_STATE1_0                        ((uint32_t)0x00000000)          /*!< N/A */
#define SCB_PFR0_STATE1_1                        ((uint32_t)0x00000010)          /*!< N/A */
#define SCB_PFR0_STATE1_2                        ((uint32_t)0x00000020)          /*!< Thumb-2 encoding with the 16-bit basic instructions plus 32-bit Buncond/BL  */
                                                                                 /* but no other 32-bit basic instructions (Note non-basic 32-bit instructions */
                                                                                 /* can be added using the appropriate instruction attribute, but other 32-bit */
                                                                                 /* basic instructions cannot.) */
#define SCB_PFR0_STATE1_3                        ((uint32_t)0x00000030)          /*!< Thumb-2 encoding with all Thumb-2 basic instructions */
/* SCB_PFR1[MICROCONTROLLER_PROGRAMMERS_MODEL] Bits */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_OFS ( 8)                            /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit Offset */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_MASK ((uint32_t)0x00000F00)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit Mask */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL0 ((uint32_t)0x00000100)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 0 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL1 ((uint32_t)0x00000200)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 1 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL2 ((uint32_t)0x00000400)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 2 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL3 ((uint32_t)0x00000800)          /*!< MICROCONTROLLER_PROGRAMMERS_MODEL Bit 3 */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_0 ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_2 ((uint32_t)0x00000200)          /*!< two-stack support */
/* SCB_DFR0[MICROCONTROLLER_DEBUG_MODEL] Bits */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_OFS (20)                            /*!< MICROCONTROLLER_DEBUG_MODEL Bit Offset */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_MASK ((uint32_t)0x00F00000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit Mask */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL0    ((uint32_t)0x00100000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 0 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL1    ((uint32_t)0x00200000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 1 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL2    ((uint32_t)0x00400000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 2 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL3    ((uint32_t)0x00800000)          /*!< MICROCONTROLLER_DEBUG_MODEL Bit 3 */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_0   ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_DFR0_MICROCONTROLLER_DEBUG_MODEL_1   ((uint32_t)0x00100000)          /*!< Microcontroller debug v1 (ITMv1, DWTv1, optional ETM) */
/* SCB_MMFR0[PMSA_SUPPORT] Bits */
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
/* SCB_MMFR0[CACHE_COHERENCE_SUPPORT] Bits */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_OFS    ( 8)                            /*!< CACHE_COHERENCE_SUPPORT Bit Offset */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_MASK   ((uint32_t)0x00000F00)          /*!< CACHE_COHERENCE_SUPPORT Bit Mask */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT0       ((uint32_t)0x00000100)          /*!< CACHE_COHERENCE_SUPPORT Bit 0 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT1       ((uint32_t)0x00000200)          /*!< CACHE_COHERENCE_SUPPORT Bit 1 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT2       ((uint32_t)0x00000400)          /*!< CACHE_COHERENCE_SUPPORT Bit 2 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT3       ((uint32_t)0x00000800)          /*!< CACHE_COHERENCE_SUPPORT Bit 3 */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_0      ((uint32_t)0x00000000)          /*!< no shared support */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_1      ((uint32_t)0x00000100)          /*!< partial-inner-shared coherency (coherency amongst some - but not all - of  */
                                                                                 /* the entities within an inner-coherent domain) */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_2      ((uint32_t)0x00000200)          /*!< full-inner-shared coherency (coherency amongst all of the entities within an  */
                                                                                 /* inner-coherent domain) */
#define SCB_MMFR0_CACHE_COHERENCE_SUPPORT_3      ((uint32_t)0x00000300)          /*!< full coherency (coherency amongst all of the entities) */
/* SCB_MMFR0[OUTER_NON_SHARABLE_SUPPORT] Bits */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_OFS (12)                            /*!< OUTER_NON_SHARABLE_SUPPORT Bit Offset */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_MASK ((uint32_t)0x0000F000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit Mask */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT0    ((uint32_t)0x00001000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 0 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT1    ((uint32_t)0x00002000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 1 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT2    ((uint32_t)0x00004000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 2 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT3    ((uint32_t)0x00008000)          /*!< OUTER_NON_SHARABLE_SUPPORT Bit 3 */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_0   ((uint32_t)0x00000000)          /*!< Outer non-sharable not supported */
#define SCB_MMFR0_OUTER_NON_SHARABLE_SUPPORT_1   ((uint32_t)0x00001000)          /*!< Outer sharable supported */
/* SCB_MMFR0[AUILIARY_REGISTER_SUPPORT] Bits */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_OFS  (20)                            /*!< AUXILIARY_REGISTER_SUPPORT Bit Offset */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_MASK ((uint32_t)0x00F00000)          /*!< AUXILIARY_REGISTER_SUPPORT Bit Mask */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT0     ((uint32_t)0x00100000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 0 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT1     ((uint32_t)0x00200000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 1 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT2     ((uint32_t)0x00400000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 2 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT3     ((uint32_t)0x00800000)          /*!< AUILIARY_REGISTER_SUPPORT Bit 3 */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_0    ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_MMFR0_AUILIARY_REGISTER_SUPPORT_1    ((uint32_t)0x00100000)          /*!< Auxiliary control register */
/* SCB_MMFR2[WAIT_FOR_INTERRUPT_STALLING] Bits */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_OFS (24)                            /*!< WAIT_FOR_INTERRUPT_STALLING Bit Offset */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_MASK ((uint32_t)0x0F000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit Mask */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING0   ((uint32_t)0x01000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 0 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING1   ((uint32_t)0x02000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 1 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING2   ((uint32_t)0x04000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 2 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING3   ((uint32_t)0x08000000)          /*!< WAIT_FOR_INTERRUPT_STALLING Bit 3 */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_0  ((uint32_t)0x00000000)          /*!< not supported */
#define SCB_MMFR2_WAIT_FOR_INTERRUPT_STALLING_1  ((uint32_t)0x01000000)          /*!< wait for interrupt supported */
/* SCB_ISAR0[BITCOUNT_INSTRS] Bits */
#define SCB_ISAR0_BITCOUNT_INSTRS_OFS            ( 4)                            /*!< BITCOUNT_INSTRS Bit Offset */
#define SCB_ISAR0_BITCOUNT_INSTRS_MASK           ((uint32_t)0x000000F0)          /*!< BITCOUNT_INSTRS Bit Mask */
#define SCB_ISAR0_BITCOUNT_INSTRS0               ((uint32_t)0x00000010)          /*!< BITCOUNT_INSTRS Bit 0 */
#define SCB_ISAR0_BITCOUNT_INSTRS1               ((uint32_t)0x00000020)          /*!< BITCOUNT_INSTRS Bit 1 */
#define SCB_ISAR0_BITCOUNT_INSTRS2               ((uint32_t)0x00000040)          /*!< BITCOUNT_INSTRS Bit 2 */
#define SCB_ISAR0_BITCOUNT_INSTRS3               ((uint32_t)0x00000080)          /*!< BITCOUNT_INSTRS Bit 3 */
#define SCB_ISAR0_BITCOUNT_INSTRS_0              ((uint32_t)0x00000000)          /*!< no bit-counting instructions present */
#define SCB_ISAR0_BITCOUNT_INSTRS_1              ((uint32_t)0x00000010)          /*!< adds CLZ */
/* SCB_ISAR0[BITFIELD_INSTRS] Bits */
#define SCB_ISAR0_BITFIELD_INSTRS_OFS            ( 8)                            /*!< BITFIELD_INSTRS Bit Offset */
#define SCB_ISAR0_BITFIELD_INSTRS_MASK           ((uint32_t)0x00000F00)          /*!< BITFIELD_INSTRS Bit Mask */
#define SCB_ISAR0_BITFIELD_INSTRS0               ((uint32_t)0x00000100)          /*!< BITFIELD_INSTRS Bit 0 */
#define SCB_ISAR0_BITFIELD_INSTRS1               ((uint32_t)0x00000200)          /*!< BITFIELD_INSTRS Bit 1 */
#define SCB_ISAR0_BITFIELD_INSTRS2               ((uint32_t)0x00000400)          /*!< BITFIELD_INSTRS Bit 2 */
#define SCB_ISAR0_BITFIELD_INSTRS3               ((uint32_t)0x00000800)          /*!< BITFIELD_INSTRS Bit 3 */
#define SCB_ISAR0_BITFIELD_INSTRS_0              ((uint32_t)0x00000000)          /*!< no bitfield instructions present */
#define SCB_ISAR0_BITFIELD_INSTRS_1              ((uint32_t)0x00000100)          /*!< adds BFC, BFI, SBFX, UBFX */
/* SCB_ISAR0[CMPBRANCH_INSTRS] Bits */
#define SCB_ISAR0_CMPBRANCH_INSTRS_OFS           (12)                            /*!< CMPBRANCH_INSTRS Bit Offset */
#define SCB_ISAR0_CMPBRANCH_INSTRS_MASK          ((uint32_t)0x0000F000)          /*!< CMPBRANCH_INSTRS Bit Mask */
#define SCB_ISAR0_CMPBRANCH_INSTRS0              ((uint32_t)0x00001000)          /*!< CMPBRANCH_INSTRS Bit 0 */
#define SCB_ISAR0_CMPBRANCH_INSTRS1              ((uint32_t)0x00002000)          /*!< CMPBRANCH_INSTRS Bit 1 */
#define SCB_ISAR0_CMPBRANCH_INSTRS2              ((uint32_t)0x00004000)          /*!< CMPBRANCH_INSTRS Bit 2 */
#define SCB_ISAR0_CMPBRANCH_INSTRS3              ((uint32_t)0x00008000)          /*!< CMPBRANCH_INSTRS Bit 3 */
#define SCB_ISAR0_CMPBRANCH_INSTRS_0             ((uint32_t)0x00000000)          /*!< no combined compare-and-branch instructions present */
#define SCB_ISAR0_CMPBRANCH_INSTRS_1             ((uint32_t)0x00001000)          /*!< adds CB{N}Z */
/* SCB_ISAR0[COPROC_INSTRS] Bits */
#define SCB_ISAR0_COPROC_INSTRS_OFS              (16)                            /*!< COPROC_INSTRS Bit Offset */
#define SCB_ISAR0_COPROC_INSTRS_MASK             ((uint32_t)0x000F0000)          /*!< COPROC_INSTRS Bit Mask */
#define SCB_ISAR0_COPROC_INSTRS0                 ((uint32_t)0x00010000)          /*!< COPROC_INSTRS Bit 0 */
#define SCB_ISAR0_COPROC_INSTRS1                 ((uint32_t)0x00020000)          /*!< COPROC_INSTRS Bit 1 */
#define SCB_ISAR0_COPROC_INSTRS2                 ((uint32_t)0x00040000)          /*!< COPROC_INSTRS Bit 2 */
#define SCB_ISAR0_COPROC_INSTRS3                 ((uint32_t)0x00080000)          /*!< COPROC_INSTRS Bit 3 */
#define SCB_ISAR0_COPROC_INSTRS_0                ((uint32_t)0x00000000)          /*!< no coprocessor support, other than for separately attributed architectures  */
                                                                                 /* such as CP15 or VFP */
#define SCB_ISAR0_COPROC_INSTRS_1                ((uint32_t)0x00010000)          /*!< adds generic CDP, LDC, MCR, MRC, STC */
#define SCB_ISAR0_COPROC_INSTRS_2                ((uint32_t)0x00020000)          /*!< adds generic CDP2, LDC2, MCR2, MRC2, STC2 */
#define SCB_ISAR0_COPROC_INSTRS_3                ((uint32_t)0x00030000)          /*!< adds generic MCRR, MRRC */
#define SCB_ISAR0_COPROC_INSTRS_4                ((uint32_t)0x00040000)          /*!< adds generic MCRR2, MRRC2 */
/* SCB_ISAR0[DEBUG_INSTRS] Bits */
#define SCB_ISAR0_DEBUG_INSTRS_OFS               (20)                            /*!< DEBUG_INSTRS Bit Offset */
#define SCB_ISAR0_DEBUG_INSTRS_MASK              ((uint32_t)0x00F00000)          /*!< DEBUG_INSTRS Bit Mask */
#define SCB_ISAR0_DEBUG_INSTRS0                  ((uint32_t)0x00100000)          /*!< DEBUG_INSTRS Bit 0 */
#define SCB_ISAR0_DEBUG_INSTRS1                  ((uint32_t)0x00200000)          /*!< DEBUG_INSTRS Bit 1 */
#define SCB_ISAR0_DEBUG_INSTRS2                  ((uint32_t)0x00400000)          /*!< DEBUG_INSTRS Bit 2 */
#define SCB_ISAR0_DEBUG_INSTRS3                  ((uint32_t)0x00800000)          /*!< DEBUG_INSTRS Bit 3 */
#define SCB_ISAR0_DEBUG_INSTRS_0                 ((uint32_t)0x00000000)          /*!< no debug instructions present */
#define SCB_ISAR0_DEBUG_INSTRS_1                 ((uint32_t)0x00100000)          /*!< adds BKPT */
/* SCB_ISAR0[DIVIDE_INSTRS] Bits */
#define SCB_ISAR0_DIVIDE_INSTRS_OFS              (24)                            /*!< DIVIDE_INSTRS Bit Offset */
#define SCB_ISAR0_DIVIDE_INSTRS_MASK             ((uint32_t)0x0F000000)          /*!< DIVIDE_INSTRS Bit Mask */
#define SCB_ISAR0_DIVIDE_INSTRS0                 ((uint32_t)0x01000000)          /*!< DIVIDE_INSTRS Bit 0 */
#define SCB_ISAR0_DIVIDE_INSTRS1                 ((uint32_t)0x02000000)          /*!< DIVIDE_INSTRS Bit 1 */
#define SCB_ISAR0_DIVIDE_INSTRS2                 ((uint32_t)0x04000000)          /*!< DIVIDE_INSTRS Bit 2 */
#define SCB_ISAR0_DIVIDE_INSTRS3                 ((uint32_t)0x08000000)          /*!< DIVIDE_INSTRS Bit 3 */
#define SCB_ISAR0_DIVIDE_INSTRS_0                ((uint32_t)0x00000000)          /*!< no divide instructions present */
#define SCB_ISAR0_DIVIDE_INSTRS_1                ((uint32_t)0x01000000)          /*!< adds SDIV, UDIV (v1 quotient only result) */
/* SCB_ISAR1[ETEND_INSRS] Bits */
#define SCB_ISAR1_ETEND_INSRS_OFS                (12)                            /*!< EXTEND_INSRS Bit Offset */
#define SCB_ISAR1_ETEND_INSRS_MASK               ((uint32_t)0x0000F000)          /*!< EXTEND_INSRS Bit Mask */
#define SCB_ISAR1_ETEND_INSRS0                   ((uint32_t)0x00001000)          /*!< ETEND_INSRS Bit 0 */
#define SCB_ISAR1_ETEND_INSRS1                   ((uint32_t)0x00002000)          /*!< ETEND_INSRS Bit 1 */
#define SCB_ISAR1_ETEND_INSRS2                   ((uint32_t)0x00004000)          /*!< ETEND_INSRS Bit 2 */
#define SCB_ISAR1_ETEND_INSRS3                   ((uint32_t)0x00008000)          /*!< ETEND_INSRS Bit 3 */
#define SCB_ISAR1_ETEND_INSRS_0                  ((uint32_t)0x00000000)          /*!< no scalar (i.e. non-SIMD) sign/zero-extend instructions present */
#define SCB_ISAR1_ETEND_INSRS_1                  ((uint32_t)0x00001000)          /*!< adds SXTB, SXTH, UXTB, UXTH */
#define SCB_ISAR1_ETEND_INSRS_2                  ((uint32_t)0x00002000)          /*!< N/A */
/* SCB_ISAR1[IFTHEN_INSTRS] Bits */
#define SCB_ISAR1_IFTHEN_INSTRS_OFS              (16)                            /*!< IFTHEN_INSTRS Bit Offset */
#define SCB_ISAR1_IFTHEN_INSTRS_MASK             ((uint32_t)0x000F0000)          /*!< IFTHEN_INSTRS Bit Mask */
#define SCB_ISAR1_IFTHEN_INSTRS0                 ((uint32_t)0x00010000)          /*!< IFTHEN_INSTRS Bit 0 */
#define SCB_ISAR1_IFTHEN_INSTRS1                 ((uint32_t)0x00020000)          /*!< IFTHEN_INSTRS Bit 1 */
#define SCB_ISAR1_IFTHEN_INSTRS2                 ((uint32_t)0x00040000)          /*!< IFTHEN_INSTRS Bit 2 */
#define SCB_ISAR1_IFTHEN_INSTRS3                 ((uint32_t)0x00080000)          /*!< IFTHEN_INSTRS Bit 3 */
#define SCB_ISAR1_IFTHEN_INSTRS_0                ((uint32_t)0x00000000)          /*!< IT instructions not present */
#define SCB_ISAR1_IFTHEN_INSTRS_1                ((uint32_t)0x00010000)          /*!< adds IT instructions (and IT bits in PSRs) */
/* SCB_ISAR1[IMMEDIATE_INSTRS] Bits */
#define SCB_ISAR1_IMMEDIATE_INSTRS_OFS           (20)                            /*!< IMMEDIATE_INSTRS Bit Offset */
#define SCB_ISAR1_IMMEDIATE_INSTRS_MASK          ((uint32_t)0x00F00000)          /*!< IMMEDIATE_INSTRS Bit Mask */
#define SCB_ISAR1_IMMEDIATE_INSTRS0              ((uint32_t)0x00100000)          /*!< IMMEDIATE_INSTRS Bit 0 */
#define SCB_ISAR1_IMMEDIATE_INSTRS1              ((uint32_t)0x00200000)          /*!< IMMEDIATE_INSTRS Bit 1 */
#define SCB_ISAR1_IMMEDIATE_INSTRS2              ((uint32_t)0x00400000)          /*!< IMMEDIATE_INSTRS Bit 2 */
#define SCB_ISAR1_IMMEDIATE_INSTRS3              ((uint32_t)0x00800000)          /*!< IMMEDIATE_INSTRS Bit 3 */
#define SCB_ISAR1_IMMEDIATE_INSTRS_0             ((uint32_t)0x00000000)          /*!< no special immediate-generating instructions present */
#define SCB_ISAR1_IMMEDIATE_INSTRS_1             ((uint32_t)0x00100000)          /*!< adds ADDW, MOVW, MOVT, SUBW */
/* SCB_ISAR1[INTERWORK_INSTRS] Bits */
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
/* SCB_ISAR2[LOADSTORE_INSTRS] Bits */
#define SCB_ISAR2_LOADSTORE_INSTRS_OFS           ( 0)                            /*!< LOADSTORE_INSTRS Bit Offset */
#define SCB_ISAR2_LOADSTORE_INSTRS_MASK          ((uint32_t)0x0000000F)          /*!< LOADSTORE_INSTRS Bit Mask */
#define SCB_ISAR2_LOADSTORE_INSTRS0              ((uint32_t)0x00000001)          /*!< LOADSTORE_INSTRS Bit 0 */
#define SCB_ISAR2_LOADSTORE_INSTRS1              ((uint32_t)0x00000002)          /*!< LOADSTORE_INSTRS Bit 1 */
#define SCB_ISAR2_LOADSTORE_INSTRS2              ((uint32_t)0x00000004)          /*!< LOADSTORE_INSTRS Bit 2 */
#define SCB_ISAR2_LOADSTORE_INSTRS3              ((uint32_t)0x00000008)          /*!< LOADSTORE_INSTRS Bit 3 */
#define SCB_ISAR2_LOADSTORE_INSTRS_0             ((uint32_t)0x00000000)          /*!< no additional normal load/store instructions present */
#define SCB_ISAR2_LOADSTORE_INSTRS_1             ((uint32_t)0x00000001)          /*!< adds LDRD/STRD */
/* SCB_ISAR2[MEMHINT_INSTRS] Bits */
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
/* SCB_ISAR2[MULTIACCESSINT_INSTRS] Bits */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_OFS      ( 8)                            /*!< MULTIACCESSINT_INSTRS Bit Offset */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_MASK     ((uint32_t)0x00000F00)          /*!< MULTIACCESSINT_INSTRS Bit Mask */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS0         ((uint32_t)0x00000100)          /*!< MULTIACCESSINT_INSTRS Bit 0 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS1         ((uint32_t)0x00000200)          /*!< MULTIACCESSINT_INSTRS Bit 1 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS2         ((uint32_t)0x00000400)          /*!< MULTIACCESSINT_INSTRS Bit 2 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS3         ((uint32_t)0x00000800)          /*!< MULTIACCESSINT_INSTRS Bit 3 */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_0        ((uint32_t)0x00000000)          /*!< the (LDM/STM) instructions are non-interruptible */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_1        ((uint32_t)0x00000100)          /*!< the (LDM/STM) instructions are restartable */
#define SCB_ISAR2_MULTIACCESSINT_INSTRS_2        ((uint32_t)0x00000200)          /*!< the (LDM/STM) instructions are continuable */
/* SCB_ISAR2[MULT_INSTRS] Bits */
#define SCB_ISAR2_MULT_INSTRS_OFS                (12)                            /*!< MULT_INSTRS Bit Offset */
#define SCB_ISAR2_MULT_INSTRS_MASK               ((uint32_t)0x0000F000)          /*!< MULT_INSTRS Bit Mask */
#define SCB_ISAR2_MULT_INSTRS0                   ((uint32_t)0x00001000)          /*!< MULT_INSTRS Bit 0 */
#define SCB_ISAR2_MULT_INSTRS1                   ((uint32_t)0x00002000)          /*!< MULT_INSTRS Bit 1 */
#define SCB_ISAR2_MULT_INSTRS2                   ((uint32_t)0x00004000)          /*!< MULT_INSTRS Bit 2 */
#define SCB_ISAR2_MULT_INSTRS3                   ((uint32_t)0x00008000)          /*!< MULT_INSTRS Bit 3 */
#define SCB_ISAR2_MULT_INSTRS_0                  ((uint32_t)0x00000000)          /*!< only MUL present */
#define SCB_ISAR2_MULT_INSTRS_1                  ((uint32_t)0x00001000)          /*!< adds MLA */
#define SCB_ISAR2_MULT_INSTRS_2                  ((uint32_t)0x00002000)          /*!< adds MLS */
/* SCB_ISAR2[MULTS_INSTRS] Bits */
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
/* SCB_ISAR2[MULTU_INSTRS] Bits */
#define SCB_ISAR2_MULTU_INSTRS_OFS               (20)                            /*!< MULTU_INSTRS Bit Offset */
#define SCB_ISAR2_MULTU_INSTRS_MASK              ((uint32_t)0x00F00000)          /*!< MULTU_INSTRS Bit Mask */
#define SCB_ISAR2_MULTU_INSTRS0                  ((uint32_t)0x00100000)          /*!< MULTU_INSTRS Bit 0 */
#define SCB_ISAR2_MULTU_INSTRS1                  ((uint32_t)0x00200000)          /*!< MULTU_INSTRS Bit 1 */
#define SCB_ISAR2_MULTU_INSTRS2                  ((uint32_t)0x00400000)          /*!< MULTU_INSTRS Bit 2 */
#define SCB_ISAR2_MULTU_INSTRS3                  ((uint32_t)0x00800000)          /*!< MULTU_INSTRS Bit 3 */
#define SCB_ISAR2_MULTU_INSTRS_0                 ((uint32_t)0x00000000)          /*!< no unsigned multiply instructions present */
#define SCB_ISAR2_MULTU_INSTRS_1                 ((uint32_t)0x00100000)          /*!< adds UMULL, UMLAL */
#define SCB_ISAR2_MULTU_INSTRS_2                 ((uint32_t)0x00200000)          /*!< N/A */
/* SCB_ISAR2[REVERSAL_INSTRS] Bits */
#define SCB_ISAR2_REVERSAL_INSTRS_OFS            (28)                            /*!< REVERSAL_INSTRS Bit Offset */
#define SCB_ISAR2_REVERSAL_INSTRS_MASK           ((uint32_t)0xF0000000)          /*!< REVERSAL_INSTRS Bit Mask */
#define SCB_ISAR2_REVERSAL_INSTRS0               ((uint32_t)0x10000000)          /*!< REVERSAL_INSTRS Bit 0 */
#define SCB_ISAR2_REVERSAL_INSTRS1               ((uint32_t)0x20000000)          /*!< REVERSAL_INSTRS Bit 1 */
#define SCB_ISAR2_REVERSAL_INSTRS2               ((uint32_t)0x40000000)          /*!< REVERSAL_INSTRS Bit 2 */
#define SCB_ISAR2_REVERSAL_INSTRS3               ((uint32_t)0x80000000)          /*!< REVERSAL_INSTRS Bit 3 */
#define SCB_ISAR2_REVERSAL_INSTRS_0              ((uint32_t)0x00000000)          /*!< no reversal instructions present */
#define SCB_ISAR2_REVERSAL_INSTRS_1              ((uint32_t)0x10000000)          /*!< adds REV, REV16, REVSH */
#define SCB_ISAR2_REVERSAL_INSTRS_2              ((uint32_t)0x20000000)          /*!< adds RBIT */
/* SCB_ISAR3[SATRUATE_INSTRS] Bits */
#define SCB_ISAR3_SATRUATE_INSTRS_OFS            ( 0)                            /*!< SATRUATE_INSTRS Bit Offset */
#define SCB_ISAR3_SATRUATE_INSTRS_MASK           ((uint32_t)0x0000000F)          /*!< SATRUATE_INSTRS Bit Mask */
#define SCB_ISAR3_SATRUATE_INSTRS0               ((uint32_t)0x00000001)          /*!< SATRUATE_INSTRS Bit 0 */
#define SCB_ISAR3_SATRUATE_INSTRS1               ((uint32_t)0x00000002)          /*!< SATRUATE_INSTRS Bit 1 */
#define SCB_ISAR3_SATRUATE_INSTRS2               ((uint32_t)0x00000004)          /*!< SATRUATE_INSTRS Bit 2 */
#define SCB_ISAR3_SATRUATE_INSTRS3               ((uint32_t)0x00000008)          /*!< SATRUATE_INSTRS Bit 3 */
#define SCB_ISAR3_SATRUATE_INSTRS_0              ((uint32_t)0x00000000)          /*!< no non-SIMD saturate instructions present */
#define SCB_ISAR3_SATRUATE_INSTRS_1              ((uint32_t)0x00000001)          /*!< N/A */
/* SCB_ISAR3[SIMD_INSTRS] Bits */
#define SCB_ISAR3_SIMD_INSTRS_OFS                ( 4)                            /*!< SIMD_INSTRS Bit Offset */
#define SCB_ISAR3_SIMD_INSTRS_MASK               ((uint32_t)0x000000F0)          /*!< SIMD_INSTRS Bit Mask */
#define SCB_ISAR3_SIMD_INSTRS0                   ((uint32_t)0x00000010)          /*!< SIMD_INSTRS Bit 0 */
#define SCB_ISAR3_SIMD_INSTRS1                   ((uint32_t)0x00000020)          /*!< SIMD_INSTRS Bit 1 */
#define SCB_ISAR3_SIMD_INSTRS2                   ((uint32_t)0x00000040)          /*!< SIMD_INSTRS Bit 2 */
#define SCB_ISAR3_SIMD_INSTRS3                   ((uint32_t)0x00000080)          /*!< SIMD_INSTRS Bit 3 */
#define SCB_ISAR3_SIMD_INSTRS_0                  ((uint32_t)0x00000000)          /*!< no SIMD instructions present */
#define SCB_ISAR3_SIMD_INSTRS_1                  ((uint32_t)0x00000010)          /*!< adds SSAT, USAT (and the Q flag in the PSRs) */
#define SCB_ISAR3_SIMD_INSTRS_3                  ((uint32_t)0x00000030)          /*!< N/A */
/* SCB_ISAR3[SVC_INSTRS] Bits */
#define SCB_ISAR3_SVC_INSTRS_OFS                 ( 8)                            /*!< SVC_INSTRS Bit Offset */
#define SCB_ISAR3_SVC_INSTRS_MASK                ((uint32_t)0x00000F00)          /*!< SVC_INSTRS Bit Mask */
#define SCB_ISAR3_SVC_INSTRS0                    ((uint32_t)0x00000100)          /*!< SVC_INSTRS Bit 0 */
#define SCB_ISAR3_SVC_INSTRS1                    ((uint32_t)0x00000200)          /*!< SVC_INSTRS Bit 1 */
#define SCB_ISAR3_SVC_INSTRS2                    ((uint32_t)0x00000400)          /*!< SVC_INSTRS Bit 2 */
#define SCB_ISAR3_SVC_INSTRS3                    ((uint32_t)0x00000800)          /*!< SVC_INSTRS Bit 3 */
#define SCB_ISAR3_SVC_INSTRS_0                   ((uint32_t)0x00000000)          /*!< no SVC (SWI) instructions present */
#define SCB_ISAR3_SVC_INSTRS_1                   ((uint32_t)0x00000100)          /*!< adds SVC (SWI) */
/* SCB_ISAR3[SYNCPRIM_INSTRS] Bits */
#define SCB_ISAR3_SYNCPRIM_INSTRS_OFS            (12)                            /*!< SYNCPRIM_INSTRS Bit Offset */
#define SCB_ISAR3_SYNCPRIM_INSTRS_MASK           ((uint32_t)0x0000F000)          /*!< SYNCPRIM_INSTRS Bit Mask */
#define SCB_ISAR3_SYNCPRIM_INSTRS0               ((uint32_t)0x00001000)          /*!< SYNCPRIM_INSTRS Bit 0 */
#define SCB_ISAR3_SYNCPRIM_INSTRS1               ((uint32_t)0x00002000)          /*!< SYNCPRIM_INSTRS Bit 1 */
#define SCB_ISAR3_SYNCPRIM_INSTRS2               ((uint32_t)0x00004000)          /*!< SYNCPRIM_INSTRS Bit 2 */
#define SCB_ISAR3_SYNCPRIM_INSTRS3               ((uint32_t)0x00008000)          /*!< SYNCPRIM_INSTRS Bit 3 */
#define SCB_ISAR3_SYNCPRIM_INSTRS_0              ((uint32_t)0x00000000)          /*!< no synchronization primitives present */
#define SCB_ISAR3_SYNCPRIM_INSTRS_1              ((uint32_t)0x00001000)          /*!< adds LDREX, STREX */
#define SCB_ISAR3_SYNCPRIM_INSTRS_2              ((uint32_t)0x00002000)          /*!< adds LDREXB, LDREXH, LDREXD, STREXB, STREXH, STREXD, CLREX(N/A) */
/* SCB_ISAR3[TABBRANCH_INSTRS] Bits */
#define SCB_ISAR3_TABBRANCH_INSTRS_OFS           (16)                            /*!< TABBRANCH_INSTRS Bit Offset */
#define SCB_ISAR3_TABBRANCH_INSTRS_MASK          ((uint32_t)0x000F0000)          /*!< TABBRANCH_INSTRS Bit Mask */
#define SCB_ISAR3_TABBRANCH_INSTRS0              ((uint32_t)0x00010000)          /*!< TABBRANCH_INSTRS Bit 0 */
#define SCB_ISAR3_TABBRANCH_INSTRS1              ((uint32_t)0x00020000)          /*!< TABBRANCH_INSTRS Bit 1 */
#define SCB_ISAR3_TABBRANCH_INSTRS2              ((uint32_t)0x00040000)          /*!< TABBRANCH_INSTRS Bit 2 */
#define SCB_ISAR3_TABBRANCH_INSTRS3              ((uint32_t)0x00080000)          /*!< TABBRANCH_INSTRS Bit 3 */
#define SCB_ISAR3_TABBRANCH_INSTRS_0             ((uint32_t)0x00000000)          /*!< no table-branch instructions present */
#define SCB_ISAR3_TABBRANCH_INSTRS_1             ((uint32_t)0x00010000)          /*!< adds TBB, TBH */
/* SCB_ISAR3[THUMBCOPY_INSTRS] Bits */
#define SCB_ISAR3_THUMBCOPY_INSTRS_OFS           (20)                            /*!< THUMBCOPY_INSTRS Bit Offset */
#define SCB_ISAR3_THUMBCOPY_INSTRS_MASK          ((uint32_t)0x00F00000)          /*!< THUMBCOPY_INSTRS Bit Mask */
#define SCB_ISAR3_THUMBCOPY_INSTRS0              ((uint32_t)0x00100000)          /*!< THUMBCOPY_INSTRS Bit 0 */
#define SCB_ISAR3_THUMBCOPY_INSTRS1              ((uint32_t)0x00200000)          /*!< THUMBCOPY_INSTRS Bit 1 */
#define SCB_ISAR3_THUMBCOPY_INSTRS2              ((uint32_t)0x00400000)          /*!< THUMBCOPY_INSTRS Bit 2 */
#define SCB_ISAR3_THUMBCOPY_INSTRS3              ((uint32_t)0x00800000)          /*!< THUMBCOPY_INSTRS Bit 3 */
#define SCB_ISAR3_THUMBCOPY_INSTRS_0             ((uint32_t)0x00000000)          /*!< Thumb MOV(register) instruction does not allow low reg -> low reg */
#define SCB_ISAR3_THUMBCOPY_INSTRS_1             ((uint32_t)0x00100000)          /*!< adds Thumb MOV(register) low reg -> low reg and the CPY alias */
/* SCB_ISAR3[TRUENOP_INSTRS] Bits */
#define SCB_ISAR3_TRUENOP_INSTRS_OFS             (24)                            /*!< TRUENOP_INSTRS Bit Offset */
#define SCB_ISAR3_TRUENOP_INSTRS_MASK            ((uint32_t)0x0F000000)          /*!< TRUENOP_INSTRS Bit Mask */
#define SCB_ISAR3_TRUENOP_INSTRS0                ((uint32_t)0x01000000)          /*!< TRUENOP_INSTRS Bit 0 */
#define SCB_ISAR3_TRUENOP_INSTRS1                ((uint32_t)0x02000000)          /*!< TRUENOP_INSTRS Bit 1 */
#define SCB_ISAR3_TRUENOP_INSTRS2                ((uint32_t)0x04000000)          /*!< TRUENOP_INSTRS Bit 2 */
#define SCB_ISAR3_TRUENOP_INSTRS3                ((uint32_t)0x08000000)          /*!< TRUENOP_INSTRS Bit 3 */
#define SCB_ISAR3_TRUENOP_INSTRS_0               ((uint32_t)0x00000000)          /*!< true NOP instructions not present - that is, NOP instructions with no  */
                                                                                 /* register dependencies */
#define SCB_ISAR3_TRUENOP_INSTRS_1               ((uint32_t)0x01000000)          /*!< adds "true NOP", and the capability of additional "NOP compatible hints" */
/* SCB_ISAR4[UNPRIV_INSTRS] Bits */
#define SCB_ISAR4_UNPRIV_INSTRS_OFS              ( 0)                            /*!< UNPRIV_INSTRS Bit Offset */
#define SCB_ISAR4_UNPRIV_INSTRS_MASK             ((uint32_t)0x0000000F)          /*!< UNPRIV_INSTRS Bit Mask */
#define SCB_ISAR4_UNPRIV_INSTRS0                 ((uint32_t)0x00000001)          /*!< UNPRIV_INSTRS Bit 0 */
#define SCB_ISAR4_UNPRIV_INSTRS1                 ((uint32_t)0x00000002)          /*!< UNPRIV_INSTRS Bit 1 */
#define SCB_ISAR4_UNPRIV_INSTRS2                 ((uint32_t)0x00000004)          /*!< UNPRIV_INSTRS Bit 2 */
#define SCB_ISAR4_UNPRIV_INSTRS3                 ((uint32_t)0x00000008)          /*!< UNPRIV_INSTRS Bit 3 */
#define SCB_ISAR4_UNPRIV_INSTRS_0                ((uint32_t)0x00000000)          /*!< no "T variant" instructions exist */
#define SCB_ISAR4_UNPRIV_INSTRS_1                ((uint32_t)0x00000001)          /*!< adds LDRBT, LDRT, STRBT, STRT */
#define SCB_ISAR4_UNPRIV_INSTRS_2                ((uint32_t)0x00000002)          /*!< adds LDRHT, LDRSBT, LDRSHT, STRHT */
/* SCB_ISAR4[WITHSHIFTS_INSTRS] Bits */
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
/* SCB_ISAR4[WRITEBACK_INSTRS] Bits */
#define SCB_ISAR4_WRITEBACK_INSTRS_OFS           ( 8)                            /*!< WRITEBACK_INSTRS Bit Offset */
#define SCB_ISAR4_WRITEBACK_INSTRS_MASK          ((uint32_t)0x00000F00)          /*!< WRITEBACK_INSTRS Bit Mask */
#define SCB_ISAR4_WRITEBACK_INSTRS0              ((uint32_t)0x00000100)          /*!< WRITEBACK_INSTRS Bit 0 */
#define SCB_ISAR4_WRITEBACK_INSTRS1              ((uint32_t)0x00000200)          /*!< WRITEBACK_INSTRS Bit 1 */
#define SCB_ISAR4_WRITEBACK_INSTRS2              ((uint32_t)0x00000400)          /*!< WRITEBACK_INSTRS Bit 2 */
#define SCB_ISAR4_WRITEBACK_INSTRS3              ((uint32_t)0x00000800)          /*!< WRITEBACK_INSTRS Bit 3 */
#define SCB_ISAR4_WRITEBACK_INSTRS_0             ((uint32_t)0x00000000)          /*!< only non-writeback addressing modes present, except that  */
                                                                                 /* LDMIA/STMDB/PUSH/POP instructions support writeback addressing. */
#define SCB_ISAR4_WRITEBACK_INSTRS_1             ((uint32_t)0x00000100)          /*!< adds all currently-defined writeback addressing modes (ARMv7, Thumb-2) */
/* SCB_ISAR4[BARRIER_INSTRS] Bits */
#define SCB_ISAR4_BARRIER_INSTRS_OFS             (16)                            /*!< BARRIER_INSTRS Bit Offset */
#define SCB_ISAR4_BARRIER_INSTRS_MASK            ((uint32_t)0x000F0000)          /*!< BARRIER_INSTRS Bit Mask */
#define SCB_ISAR4_BARRIER_INSTRS0                ((uint32_t)0x00010000)          /*!< BARRIER_INSTRS Bit 0 */
#define SCB_ISAR4_BARRIER_INSTRS1                ((uint32_t)0x00020000)          /*!< BARRIER_INSTRS Bit 1 */
#define SCB_ISAR4_BARRIER_INSTRS2                ((uint32_t)0x00040000)          /*!< BARRIER_INSTRS Bit 2 */
#define SCB_ISAR4_BARRIER_INSTRS3                ((uint32_t)0x00080000)          /*!< BARRIER_INSTRS Bit 3 */
#define SCB_ISAR4_BARRIER_INSTRS_0               ((uint32_t)0x00000000)          /*!< no barrier instructions supported */
#define SCB_ISAR4_BARRIER_INSTRS_1               ((uint32_t)0x00010000)          /*!< adds DMB, DSB, ISB barrier instructions */
/* SCB_ISAR4[SYNCPRIM_INSTRS_FRAC] Bits */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_OFS       (20)                            /*!< SYNCPRIM_INSTRS_FRAC Bit Offset */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_MASK      ((uint32_t)0x00F00000)          /*!< SYNCPRIM_INSTRS_FRAC Bit Mask */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC0          ((uint32_t)0x00100000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 0 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC1          ((uint32_t)0x00200000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 1 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC2          ((uint32_t)0x00400000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 2 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC3          ((uint32_t)0x00800000)          /*!< SYNCPRIM_INSTRS_FRAC Bit 3 */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_0         ((uint32_t)0x00000000)          /*!< no additional support */
#define SCB_ISAR4_SYNCPRIM_INSTRS_FRAC_3         ((uint32_t)0x00300000)          /*!< adds CLREX, LDREXB, STREXB, LDREXH, STREXH */
/* SCB_ISAR4[PSR_M_INSTRS] Bits */
#define SCB_ISAR4_PSR_M_INSTRS_OFS               (24)                            /*!< PSR_M_INSTRS Bit Offset */
#define SCB_ISAR4_PSR_M_INSTRS_MASK              ((uint32_t)0x0F000000)          /*!< PSR_M_INSTRS Bit Mask */
#define SCB_ISAR4_PSR_M_INSTRS0                  ((uint32_t)0x01000000)          /*!< PSR_M_INSTRS Bit 0 */
#define SCB_ISAR4_PSR_M_INSTRS1                  ((uint32_t)0x02000000)          /*!< PSR_M_INSTRS Bit 1 */
#define SCB_ISAR4_PSR_M_INSTRS2                  ((uint32_t)0x04000000)          /*!< PSR_M_INSTRS Bit 2 */
#define SCB_ISAR4_PSR_M_INSTRS3                  ((uint32_t)0x08000000)          /*!< PSR_M_INSTRS Bit 3 */
#define SCB_ISAR4_PSR_M_INSTRS_0                 ((uint32_t)0x00000000)          /*!< instructions not present */
#define SCB_ISAR4_PSR_M_INSTRS_1                 ((uint32_t)0x01000000)          /*!< adds CPS, MRS, and MSR instructions (M-profile forms) */
/* SCB_CPACR[CP11] Bits */
#define SCB_CPACR_CP11_OFS                       (22)                            /*!< CP11 Bit Offset */
#define SCB_CPACR_CP11_MASK                      ((uint32_t)0x00C00000)          /*!< CP11 Bit Mask */
/* SCB_CPACR[CP10] Bits */
#define SCB_CPACR_CP10_OFS                       (20)                            /*!< CP10 Bit Offset */
#define SCB_CPACR_CP10_MASK                      ((uint32_t)0x00300000)          /*!< CP10 Bit Mask */
/* SCB_SHPR1[SCB_SHPR1_PRI_4] Bits */
#define SCB_SHPR1_PRI_4_OFS                      ( 0)                            /*!< PRI_4 Offset */
#define SCB_SHPR1_PRI_4_M                        ((uint32_t)0x000000ff)          /*  */
/* SCB_SHPR1[SCB_SHPR1_PRI_5] Bits */
#define SCB_SHPR1_PRI_5_OFS                      ( 8)                            /*!< PRI_5 Offset */
#define SCB_SHPR1_PRI_5_M                        ((uint32_t)0x0000ff00)          /*  */
/* SCB_SHPR1[SCB_SHPR1_PRI_6] Bits */
#define SCB_SHPR1_PRI_6_OFS                      (16)                            /*!< PRI_6 Offset */
#define SCB_SHPR1_PRI_6_M                        ((uint32_t)0x00ff0000)          /*  */
/* SCB_SHPR1[SCB_SHPR1_PRI_7] Bits */
#define SCB_SHPR1_PRI_7_OFS                      (24)                            /*!< PRI_7 Offset */
#define SCB_SHPR1_PRI_7_M                        ((uint32_t)0xff000000)          /*  */
/* SCB_SHPR2[SCB_SHPR2_PRI_8] Bits */
#define SCB_SHPR2_PRI_8_OFS                      ( 0)                            /*!< PRI_8 Offset */
#define SCB_SHPR2_PRI_8_M                        ((uint32_t)0x000000ff)          /*  */
/* SCB_SHPR2[SCB_SHPR2_PRI_9] Bits */
#define SCB_SHPR2_PRI_9_OFS                      ( 8)                            /*!< PRI_9 Offset */
#define SCB_SHPR2_PRI_9_M                        ((uint32_t)0x0000ff00)          /*  */
/* SCB_SHPR2[SCB_SHPR2_PRI_10] Bits */
#define SCB_SHPR2_PRI_10_OFS                     (16)                            /*!< PRI_10 Offset */
#define SCB_SHPR2_PRI_10_M                       ((uint32_t)0x00ff0000)          /*  */
/* SCB_SHPR2[SCB_SHPR2_PRI_11] Bits */
#define SCB_SHPR2_PRI_11_OFS                     (24)                            /*!< PRI_11 Offset */
#define SCB_SHPR2_PRI_11_M                       ((uint32_t)0xff000000)          /*  */
/* SCB_SHPR3[SCB_SHPR3_PRI_12] Bits */
#define SCB_SHPR3_PRI_12_OFS                     ( 0)                            /*!< PRI_12 Offset */
#define SCB_SHPR3_PRI_12_M                       ((uint32_t)0x000000ff)          /*  */
/* SCB_SHPR3[SCB_SHPR3_PRI_13] Bits */
#define SCB_SHPR3_PRI_13_OFS                     ( 8)                            /*!< PRI_13 Offset */
#define SCB_SHPR3_PRI_13_M                       ((uint32_t)0x0000ff00)          /*  */
/* SCB_SHPR3[SCB_SHPR3_PRI_14] Bits */
#define SCB_SHPR3_PRI_14_OFS                     (16)                            /*!< PRI_14 Offset */
#define SCB_SHPR3_PRI_14_M                       ((uint32_t)0x00ff0000)          /*  */
/* SCB_SHPR3[SCB_SHPR3_PRI_15] Bits */
#define SCB_SHPR3_PRI_15_OFS                     (24)                            /*!< PRI_15 Offset */
#define SCB_SHPR3_PRI_15_M                       ((uint32_t)0xff000000)          /*  */

/* SCB_CFSR[SCB_CFSR_IACCVIOL] Bits */
#define SCB_CFSR_IACCVIOL_OFS                    ( 0)                            /*!< IACCVIOL Offset */
#define SCB_CFSR_IACCVIOL                        ((uint32_t)0x00000001)          /*  */
/* SCB_CFSR[SCB_CFSR_DACCVIOL] Bits */
#define SCB_CFSR_DACCVIOL_OFS                    ( 1)                            /*!< DACCVIOL Offset */
#define SCB_CFSR_DACCVIOL                        ((uint32_t)0x00000002)          /*  */
/* SCB_CFSR[SCB_CFSR_MUNSTKERR] Bits */
#define SCB_CFSR_MUNSTKERR_OFS                   ( 3)                            /*!< MUNSTKERR Offset */
#define SCB_CFSR_MUNSTKERR                       ((uint32_t)0x00000008)          /*  */
/* SCB_CFSR[SCB_CFSR_MSTKERR] Bits */
#define SCB_CFSR_MSTKERR_OFS                     ( 4)                            /*!< MSTKERR Offset */
#define SCB_CFSR_MSTKERR                         ((uint32_t)0x00000010)          /*  */
/* SCB_CFSR[SCB_CFSR_MMARVALID] Bits */
#define SCB_CFSR_MMARVALID_OFS                   ( 7)                            /*!< MMARVALID Offset */
#define SCB_CFSR_MMARVALID                       ((uint32_t)0x00000080)          /*  */
/* SCB_CFSR[SCB_CFSR_IBUSERR] Bits */
#define SCB_CFSR_IBUSERR_OFS                     ( 8)                            /*!< IBUSERR Offset */
#define SCB_CFSR_IBUSERR                         ((uint32_t)0x00000100)          /*  */
/* SCB_CFSR[SCB_CFSR_PRECISERR] Bits */
#define SCB_CFSR_PRECISERR_OFS                   ( 9)                            /*!< PRECISERR Offset */
#define SCB_CFSR_PRECISERR                       ((uint32_t)0x00000200)          /*  */
/* SCB_CFSR[SCB_CFSR_IMPRECISERR] Bits */
#define SCB_CFSR_IMPRECISERR_OFS                 (10)                            /*!< IMPRECISERR Offset */
#define SCB_CFSR_IMPRECISERR                     ((uint32_t)0x00000400)          /*  */
/* SCB_CFSR[SCB_CFSR_UNSTKERR] Bits */
#define SCB_CFSR_UNSTKERR_OFS                    (11)                            /*!< UNSTKERR Offset */
#define SCB_CFSR_UNSTKERR                        ((uint32_t)0x00000800)          /*  */
/* SCB_CFSR[SCB_CFSR_STKERR] Bits */
#define SCB_CFSR_STKERR_OFS                      (12)                            /*!< STKERR Offset */
#define SCB_CFSR_STKERR                          ((uint32_t)0x00001000)          /*  */
/* SCB_CFSR[SCB_CFSR_BFARVALID] Bits */
#define SCB_CFSR_BFARVALID_OFS                   (15)                            /*!< BFARVALID Offset */
#define SCB_CFSR_BFARVALID                       ((uint32_t)0x00008000)          /*  */
/* SCB_CFSR[SCB_CFSR_UNDEFINSTR] Bits */
#define SCB_CFSR_UNDEFINSTR_OFS                  (16)                            /*!< UNDEFINSTR Offset */
#define SCB_CFSR_UNDEFINSTR                      ((uint32_t)0x00010000)          /*  */
/* SCB_CFSR[SCB_CFSR_INVSTATE] Bits */
#define SCB_CFSR_INVSTATE_OFS                    (17)                            /*!< INVSTATE Offset */
#define SCB_CFSR_INVSTATE                        ((uint32_t)0x00020000)          /*  */
/* SCB_CFSR[SCB_CFSR_INVPC] Bits */
#define SCB_CFSR_INVPC_OFS                       (18)                            /*!< INVPC Offset */
#define SCB_CFSR_INVPC                           ((uint32_t)0x00040000)          /*  */
/* SCB_CFSR[SCB_CFSR_NOCP] Bits */
#define SCB_CFSR_NOCP_OFS                        (19)                            /*!< NOCP Offset */
#define SCB_CFSR_NOCP                            ((uint32_t)0x00080000)          /*  */
/* SCB_CFSR[SCB_CFSR_UNALIGNED] Bits */
#define SCB_CFSR_UNALIGNED_OFS                   (24)                            /*!< UNALIGNED Offset */
#define SCB_CFSR_UNALIGNED                       ((uint32_t)0x01000000)          /*  */
/* SCB_CFSR[SCB_CFSR_DIVBYZERO] Bits */
#define SCB_CFSR_DIVBYZERO_OFS                   (25)                            /*!< DIVBYZERO Offset */
#define SCB_CFSR_DIVBYZERO                       ((uint32_t)0x02000000)          /*  */
/* SCB_CFSR[SCB_CFSR_MLSPERR] Bits */
#define SCB_CFSR_MLSPERR_OFS                     ( 5)                            /*!< MLSPERR Offset */
#define SCB_CFSR_MLSPERR                         ((uint32_t)0x00000020)          /*  */
/* SCB_CFSR[SCB_CFSR_LSPERR] Bits */
#define SCB_CFSR_LSPERR_OFS                      (13)                            /*!< LSPERR Offset */
#define SCB_CFSR_LSPERR                          ((uint32_t)0x00002000)          /*  */


/******************************************************************************
* SCNSCB Bits
******************************************************************************/


/******************************************************************************
* SYSCTL Bits
******************************************************************************/
/* SYSCTL_REBOOT_CTL[REBOOT] Bits */
#define SYSCTL_REBOOT_CTL_REBOOT_OFS             ( 0)                            /*!< REBOOT Bit Offset */
#define SYSCTL_REBOOT_CTL_REBOOT                 ((uint32_t)0x00000001)          /*!< Write 1 initiates a Reboot of the device */
/* SYSCTL_REBOOT_CTL[WKEY] Bits */
#define SYSCTL_REBOOT_CTL_WKEY_OFS               ( 8)                            /*!< WKEY Bit Offset */
#define SYSCTL_REBOOT_CTL_WKEY_MASK              ((uint32_t)0x0000FF00)          /*!< WKEY Bit Mask */
/* SYSCTL_NMI_CTLSTAT[CS_SRC] Bits */
#define SYSCTL_NMI_CTLSTAT_CS_SRC_OFS            ( 0)                            /*!< CS_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_CS_SRC                ((uint32_t)0x00000001)          /*!< CS interrupt as a source of NMI */
/* SYSCTL_NMI_CTLSTAT[PSS_SRC] Bits */
#define SYSCTL_NMI_CTLSTAT_PSS_SRC_OFS           ( 1)                            /*!< PSS_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PSS_SRC               ((uint32_t)0x00000002)          /*!< PSS interrupt as a source of NMI */
/* SYSCTL_NMI_CTLSTAT[PCM_SRC] Bits */
#define SYSCTL_NMI_CTLSTAT_PCM_SRC_OFS           ( 2)                            /*!< PCM_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PCM_SRC               ((uint32_t)0x00000004)          /*!< PCM interrupt as a source of NMI */
/* SYSCTL_NMI_CTLSTAT[PIN_SRC] Bits */
#define SYSCTL_NMI_CTLSTAT_PIN_SRC_OFS           ( 3)                            /*!< PIN_SRC Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PIN_SRC               ((uint32_t)0x00000008)          
/* SYSCTL_NMI_CTLSTAT[CS_FLG] Bits */
#define SYSCTL_NMI_CTLSTAT_CS_FLG_OFS            (16)                            /*!< CS_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_CS_FLG                ((uint32_t)0x00010000)          /*!< CS interrupt was the source of NMI */
/* SYSCTL_NMI_CTLSTAT[PSS_FLG] Bits */
#define SYSCTL_NMI_CTLSTAT_PSS_FLG_OFS           (17)                            /*!< PSS_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PSS_FLG               ((uint32_t)0x00020000)          /*!< PSS interrupt was the source of NMI */
/* SYSCTL_NMI_CTLSTAT[PCM_FLG] Bits */
#define SYSCTL_NMI_CTLSTAT_PCM_FLG_OFS           (18)                            /*!< PCM_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PCM_FLG               ((uint32_t)0x00040000)          /*!< PCM interrupt was the source of NMI */
/* SYSCTL_NMI_CTLSTAT[PIN_FLG] Bits */
#define SYSCTL_NMI_CTLSTAT_PIN_FLG_OFS           (19)                            /*!< PIN_FLG Bit Offset */
#define SYSCTL_NMI_CTLSTAT_PIN_FLG               ((uint32_t)0x00080000)          /*!< RSTn/NMI pin was the source of NMI */
/* SYSCTL_WDTRESET_CTL[TIMEOUT] Bits */
#define SYSCTL_WDTRESET_CTL_TIMEOUT_OFS          ( 0)                            /*!< TIMEOUT Bit Offset */
#define SYSCTL_WDTRESET_CTL_TIMEOUT              ((uint32_t)0x00000001)          /*!< WDT timeout reset type */
/* SYSCTL_WDTRESET_CTL[VIOLATION] Bits */
#define SYSCTL_WDTRESET_CTL_VIOLATION_OFS        ( 1)                            /*!< VIOLATION Bit Offset */
#define SYSCTL_WDTRESET_CTL_VIOLATION            ((uint32_t)0x00000002)          /*!< WDT password violation reset type */
/* SYSCTL_PERIHALT_CTL[HALT_T16_0] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_T16_0_OFS       ( 0)                            /*!< HALT_T16_0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_0           ((uint32_t)0x00000001)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_T16_1] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_T16_1_OFS       ( 1)                            /*!< HALT_T16_1 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_1           ((uint32_t)0x00000002)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_T16_2] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_T16_2_OFS       ( 2)                            /*!< HALT_T16_2 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_2           ((uint32_t)0x00000004)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_T16_3] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_T16_3_OFS       ( 3)                            /*!< HALT_T16_3 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T16_3           ((uint32_t)0x00000008)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_T32_0] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_T32_0_OFS       ( 4)                            /*!< HALT_T32_0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_T32_0           ((uint32_t)0x00000010)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUA0] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUA0_OFS        ( 5)                            /*!< HALT_eUA0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA0            ((uint32_t)0x00000020)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUA1] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUA1_OFS        ( 6)                            /*!< HALT_eUA1 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA1            ((uint32_t)0x00000040)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUA2] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUA2_OFS        ( 7)                            /*!< HALT_eUA2 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA2            ((uint32_t)0x00000080)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUA3] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUA3_OFS        ( 8)                            /*!< HALT_eUA3 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUA3            ((uint32_t)0x00000100)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUB0] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUB0_OFS        ( 9)                            /*!< HALT_eUB0 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB0            ((uint32_t)0x00000200)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUB1] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUB1_OFS        (10)                            /*!< HALT_eUB1 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB1            ((uint32_t)0x00000400)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUB2] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUB2_OFS        (11)                            /*!< HALT_eUB2 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB2            ((uint32_t)0x00000800)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_eUB3] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_EUB3_OFS        (12)                            /*!< HALT_eUB3 Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_EUB3            ((uint32_t)0x00001000)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_ADC] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_ADC_OFS         (13)                            /*!< HALT_ADC Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_ADC             ((uint32_t)0x00002000)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_WDT] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_WDT_OFS         (14)                            /*!< HALT_WDT Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_WDT             ((uint32_t)0x00004000)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_PERIHALT_CTL[HALT_DMA] Bits */
#define SYSCTL_PERIHALT_CTL_HALT_DMA_OFS         (15)                            /*!< HALT_DMA Bit Offset */
#define SYSCTL_PERIHALT_CTL_HALT_DMA             ((uint32_t)0x00008000)          /*!< Freezes IP operation when CPU is halted */
/* SYSCTL_SRAM_BANKEN[BNK0_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK0_EN_OFS           ( 0)                            /*!< BNK0_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK0_EN               ((uint32_t)0x00000001)          /*!< SRAM Bank0 enable */
/* SYSCTL_SRAM_BANKEN[BNK1_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK1_EN_OFS           ( 1)                            /*!< BNK1_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK1_EN               ((uint32_t)0x00000002)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[BNK2_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK2_EN_OFS           ( 2)                            /*!< BNK2_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK2_EN               ((uint32_t)0x00000004)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[BNK3_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK3_EN_OFS           ( 3)                            /*!< BNK3_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK3_EN               ((uint32_t)0x00000008)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[BNK4_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK4_EN_OFS           ( 4)                            /*!< BNK4_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK4_EN               ((uint32_t)0x00000010)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[BNK5_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK5_EN_OFS           ( 5)                            /*!< BNK5_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK5_EN               ((uint32_t)0x00000020)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[BNK6_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK6_EN_OFS           ( 6)                            /*!< BNK6_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK6_EN               ((uint32_t)0x00000040)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[BNK7_EN] Bits */
#define SYSCTL_SRAM_BANKEN_BNK7_EN_OFS           ( 7)                            /*!< BNK7_EN Bit Offset */
#define SYSCTL_SRAM_BANKEN_BNK7_EN               ((uint32_t)0x00000080)          /*!< SRAM Bank1 enable */
/* SYSCTL_SRAM_BANKEN[SRAM_RDY] Bits */
#define SYSCTL_SRAM_BANKEN_SRAM_RDY_OFS          (16)                            /*!< SRAM_RDY Bit Offset */
#define SYSCTL_SRAM_BANKEN_SRAM_RDY              ((uint32_t)0x00010000)          /*!< SRAM ready */
/* SYSCTL_SRAM_BANKRET[BNK0_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK0_RET_OFS         ( 0)                            /*!< BNK0_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK0_RET             ((uint32_t)0x00000001)          /*!< Bank0 retention */
/* SYSCTL_SRAM_BANKRET[BNK1_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK1_RET_OFS         ( 1)                            /*!< BNK1_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK1_RET             ((uint32_t)0x00000002)          /*!< Bank1 retention */
/* SYSCTL_SRAM_BANKRET[BNK2_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK2_RET_OFS         ( 2)                            /*!< BNK2_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK2_RET             ((uint32_t)0x00000004)          /*!< Bank2 retention */
/* SYSCTL_SRAM_BANKRET[BNK3_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK3_RET_OFS         ( 3)                            /*!< BNK3_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK3_RET             ((uint32_t)0x00000008)          /*!< Bank3 retention */
/* SYSCTL_SRAM_BANKRET[BNK4_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK4_RET_OFS         ( 4)                            /*!< BNK4_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK4_RET             ((uint32_t)0x00000010)          /*!< Bank4 retention */
/* SYSCTL_SRAM_BANKRET[BNK5_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK5_RET_OFS         ( 5)                            /*!< BNK5_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK5_RET             ((uint32_t)0x00000020)          /*!< Bank5 retention */
/* SYSCTL_SRAM_BANKRET[BNK6_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK6_RET_OFS         ( 6)                            /*!< BNK6_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK6_RET             ((uint32_t)0x00000040)          /*!< Bank6 retention */
/* SYSCTL_SRAM_BANKRET[BNK7_RET] Bits */
#define SYSCTL_SRAM_BANKRET_BNK7_RET_OFS         ( 7)                            /*!< BNK7_RET Bit Offset */
#define SYSCTL_SRAM_BANKRET_BNK7_RET             ((uint32_t)0x00000080)          /*!< Bank7 retention */
/* SYSCTL_SRAM_BANKRET[SRAM_RDY] Bits */
#define SYSCTL_SRAM_BANKRET_SRAM_RDY_OFS         (16)                            /*!< SRAM_RDY Bit Offset */
#define SYSCTL_SRAM_BANKRET_SRAM_RDY             ((uint32_t)0x00010000)          /*!< SRAM ready */
/* SYSCTL_DIO_GLTFLT_CTL[GLTCH_EN] Bits */
#define SYSCTL_DIO_GLTFLT_CTL_GLTCH_EN_OFS       ( 0)                            /*!< GLTCH_EN Bit Offset */
#define SYSCTL_DIO_GLTFLT_CTL_GLTCH_EN           ((uint32_t)0x00000001)          /*!< Glitch filter enable */
/* SYSCTL_SECDATA_UNLOCK[UNLKEY] Bits */
#define SYSCTL_SECDATA_UNLOCK_UNLKEY_OFS         ( 0)                            /*!< UNLKEY Bit Offset */
#define SYSCTL_SECDATA_UNLOCK_UNLKEY_MASK        ((uint32_t)0x0000FFFF)          /*!< UNLKEY Bit Mask */
/* SYSCTL_MASTER_UNLOCK[UNLKEY] Bits */
#define SYSCTL_MASTER_UNLOCK_UNLKEY_OFS          ( 0)                            /*!< UNLKEY Bit Offset */
#define SYSCTL_MASTER_UNLOCK_UNLKEY_MASK         ((uint32_t)0x0000FFFF)          /*!< UNLKEY Bit Mask */
/* SYSCTL_RESET_REQ[POR] Bits */
#define SYSCTL_RESET_REQ_POR_OFS                 ( 0)                            /*!< POR Bit Offset */
#define SYSCTL_RESET_REQ_POR                     ((uint32_t)0x00000001)          /*!< Generate POR */
/* SYSCTL_RESET_REQ[REBOOT] Bits */
#define SYSCTL_RESET_REQ_REBOOT_OFS              ( 1)                            /*!< REBOOT Bit Offset */
#define SYSCTL_RESET_REQ_REBOOT                  ((uint32_t)0x00000002)          /*!< Generate Reboot_Reset */
/* SYSCTL_RESET_REQ[WKEY] Bits */
#define SYSCTL_RESET_REQ_WKEY_OFS                ( 8)                            /*!< WKEY Bit Offset */
#define SYSCTL_RESET_REQ_WKEY_MASK               ((uint32_t)0x0000FF00)          /*!< WKEY Bit Mask */
/* SYSCTL_RESET_STATOVER[SOFT] Bits */
#define SYSCTL_RESET_STATOVER_SOFT_OFS           ( 0)                            /*!< SOFT Bit Offset */
#define SYSCTL_RESET_STATOVER_SOFT               ((uint32_t)0x00000001)          /*!< Indicates if SOFT Reset is active */
/* SYSCTL_RESET_STATOVER[HARD] Bits */
#define SYSCTL_RESET_STATOVER_HARD_OFS           ( 1)                            /*!< HARD Bit Offset */
#define SYSCTL_RESET_STATOVER_HARD               ((uint32_t)0x00000002)          /*!< Indicates if HARD Reset is active */
/* SYSCTL_RESET_STATOVER[REBOOT] Bits */
#define SYSCTL_RESET_STATOVER_REBOOT_OFS         ( 2)                            /*!< REBOOT Bit Offset */
#define SYSCTL_RESET_STATOVER_REBOOT             ((uint32_t)0x00000004)          /*!< Indicates if Reboot Reset is active */
/* SYSCTL_RESET_STATOVER[SOFT_OVER] Bits */
#define SYSCTL_RESET_STATOVER_SOFT_OVER_OFS      ( 8)                            /*!< SOFT_OVER Bit Offset */
#define SYSCTL_RESET_STATOVER_SOFT_OVER          ((uint32_t)0x00000100)          /*!< SOFT_Reset overwrite request */
/* SYSCTL_RESET_STATOVER[HARD_OVER] Bits */
#define SYSCTL_RESET_STATOVER_HARD_OVER_OFS      ( 9)                            /*!< HARD_OVER Bit Offset */
#define SYSCTL_RESET_STATOVER_HARD_OVER          ((uint32_t)0x00000200)          /*!< HARD_Reset overwrite request */
/* SYSCTL_RESET_STATOVER[RBT_OVER] Bits */
#define SYSCTL_RESET_STATOVER_RBT_OVER_OFS       (10)                            /*!< RBT_OVER Bit Offset */
#define SYSCTL_RESET_STATOVER_RBT_OVER           ((uint32_t)0x00000400)          /*!< Reboot Reset overwrite request */
/* Pre-defined bitfield values */
#define SYSCTL_REBOOT_CTL_WKEY_VAL              ((uint32_t)0x00006900)          /*!< Key value to enable writes to bit 0 */
                                                                                /* cleared */


/******************************************************************************
* SYSTICK Bits
******************************************************************************/

/******************************************************************************
* Timer32 Bits
******************************************************************************/
/* TIMER32_CONTROL[ONESHOT] Bits */
#define TIMER32_CONTROL_ONESHOT_OFS              ( 0)                            /*!< ONESHOT Bit Offset */
#define TIMER32_CONTROL_ONESHOT                  ((uint32_t)0x00000001)          /*!< Selects one-shot or wrapping counter mode */
/* TIMER32_CONTROL[SIZE] Bits */
#define TIMER32_CONTROL_SIZE_OFS                 ( 1)                            /*!< SIZE Bit Offset */
#define TIMER32_CONTROL_SIZE                     ((uint32_t)0x00000002)          /*!< Selects 16 or 32 bit counter operation */
/* TIMER32_CONTROL[PRESCALE] Bits */
#define TIMER32_CONTROL_PRESCALE_OFS             ( 2)                            /*!< PRESCALE Bit Offset */
#define TIMER32_CONTROL_PRESCALE_MASK            ((uint32_t)0x0000000C)          /*!< PRESCALE Bit Mask */
#define TIMER32_CONTROL_PRESCALE0                ((uint32_t)0x00000004)          /*!< PRESCALE Bit 0 */
#define TIMER32_CONTROL_PRESCALE1                ((uint32_t)0x00000008)          /*!< PRESCALE Bit 1 */
#define TIMER32_CONTROL_PRESCALE_0               ((uint32_t)0x00000000)          /*!< 0 stages of prescale, clock is divided by 1 */
#define TIMER32_CONTROL_PRESCALE_1               ((uint32_t)0x00000004)          /*!< 4 stages of prescale, clock is divided by 16 */
#define TIMER32_CONTROL_PRESCALE_2               ((uint32_t)0x00000008)          /*!< 8 stages of prescale, clock is divided by 256 */
/* TIMER32_CONTROL[IE] Bits */
#define TIMER32_CONTROL_IE_OFS                   ( 5)                            /*!< IE Bit Offset */
#define TIMER32_CONTROL_IE                       ((uint32_t)0x00000020)          /*!< Interrupt enable bit */
/* TIMER32_CONTROL[MODE] Bits */
#define TIMER32_CONTROL_MODE_OFS                 ( 6)                            /*!< MODE Bit Offset */
#define TIMER32_CONTROL_MODE                     ((uint32_t)0x00000040)          /*!< Mode bit */
/* TIMER32_CONTROL[ENABLE] Bits */
#define TIMER32_CONTROL_ENABLE_OFS               ( 7)                            /*!< ENABLE Bit Offset */
#define TIMER32_CONTROL_ENABLE                   ((uint32_t)0x00000080)
/* TIMER32_RIS[RAW_IFG] Bits */
#define TIMER32_RIS_RAW_IFG_OFS                  ( 0)                            /*!< RAW_IFG Bit Offset */
#define TIMER32_RIS_RAW_IFG                      ((uint32_t)0x00000001)          /*!< Raw interrupt status */
/* TIMER32_MIS[IFG] Bits */
#define TIMER32_MIS_IFG_OFS                      ( 0)                            /*!< IFG Bit Offset */
#define TIMER32_MIS_IFG                          ((uint32_t)0x00000001)          /*!< Enabled interrupt status */



/******************************************************************************
* TIMER_A Bits
******************************************************************************/
/* TIMER_A_CTL[IFG] Bits */
#define TIMER_A_CTL_IFG_OFS                      ( 0)                            /*!< TAIFG Bit Offset */
#define TIMER_A_CTL_IFG                          ((uint16_t)0x0001)              /*!< TimerA interrupt flag */
/* TIMER_A_CTL[IE] Bits */
#define TIMER_A_CTL_IE_OFS                       ( 1)                            /*!< TAIE Bit Offset */
#define TIMER_A_CTL_IE                           ((uint16_t)0x0002)              /*!< TimerA interrupt enable */
/* TIMER_A_CTL[CLR] Bits */
#define TIMER_A_CTL_CLR_OFS                      ( 2)                            /*!< TACLR Bit Offset */
#define TIMER_A_CTL_CLR                          ((uint16_t)0x0004)              /*!< TimerA clear */
/* TIMER_A_CTL[MC] Bits */
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
/* TIMER_A_CTL[ID] Bits */
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
/* TIMER_A_CTL[SSEL] Bits */
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
/* TIMER_A_CCTLN[CCIFG] Bits */
#define TIMER_A_CCTLN_CCIFG_OFS                  ( 0)                            /*!< CCIFG Bit Offset */
#define TIMER_A_CCTLN_CCIFG                      ((uint16_t)0x0001)              /*!< Capture/compare interrupt flag */
/* TIMER_A_CCTLN[COV] Bits */
#define TIMER_A_CCTLN_COV_OFS                    ( 1)                            /*!< COV Bit Offset */
#define TIMER_A_CCTLN_COV                        ((uint16_t)0x0002)              /*!< Capture overflow */
/* TIMER_A_CCTLN[OUT] Bits */
#define TIMER_A_CCTLN_OUT_OFS                    ( 2)                            /*!< OUT Bit Offset */
#define TIMER_A_CCTLN_OUT                        ((uint16_t)0x0004)              /*!< Output */
/* TIMER_A_CCTLN[CCI] Bits */
#define TIMER_A_CCTLN_CCI_OFS                    ( 3)                            /*!< CCI Bit Offset */
#define TIMER_A_CCTLN_CCI                        ((uint16_t)0x0008)              /*!< Capture/compare input */
/* TIMER_A_CCTLN[CCIE] Bits */
#define TIMER_A_CCTLN_CCIE_OFS                   ( 4)                            /*!< CCIE Bit Offset */
#define TIMER_A_CCTLN_CCIE                       ((uint16_t)0x0010)              /*!< Capture/compare interrupt enable */
/* TIMER_A_CCTLN[OUTMOD] Bits */
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
/* TIMER_A_CCTLN[CAP] Bits */
#define TIMER_A_CCTLN_CAP_OFS                    ( 8)                            /*!< CAP Bit Offset */
#define TIMER_A_CCTLN_CAP                        ((uint16_t)0x0100)              /*!< Capture mode */
/* TIMER_A_CCTLN[SCCI] Bits */
#define TIMER_A_CCTLN_SCCI_OFS                   (10)                            /*!< SCCI Bit Offset */
#define TIMER_A_CCTLN_SCCI                       ((uint16_t)0x0400)              /*!< Synchronized capture/compare input */
/* TIMER_A_CCTLN[SCS] Bits */
#define TIMER_A_CCTLN_SCS_OFS                    (11)                            /*!< SCS Bit Offset */
#define TIMER_A_CCTLN_SCS                        ((uint16_t)0x0800)              /*!< Synchronize capture source */
/* TIMER_A_CCTLN[CCIS] Bits */
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
/* TIMER_A_CCTLN[CM] Bits */
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
/* TIMER_A_EX0[IDEX] Bits */
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

/******************************************************************************
* TLV Bits
******************************************************************************/
/******************************************************************************
* TLV table start and TLV tags                                                *
******************************************************************************/
#define TLV_START_ADDR                    (TLV_BASE + 0x0004)                    /*!< Start Address of the TLV structure */

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


/******************************************************************************
* TPIU Bits
******************************************************************************/


/******************************************************************************
* WDT_A Bits
******************************************************************************/
/* WDT_A_CTL[IS] Bits */
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
/* WDT_A_CTL[CNTCL] Bits */
#define WDT_A_CTL_CNTCL_OFS                      ( 3)                            /*!< WDTCNTCL Bit Offset */
#define WDT_A_CTL_CNTCL                          ((uint16_t)0x0008)              /*!< Watchdog timer counter clear */
/* WDT_A_CTL[TMSEL] Bits */
#define WDT_A_CTL_TMSEL_OFS                      ( 4)                            /*!< WDTTMSEL Bit Offset */
#define WDT_A_CTL_TMSEL                          ((uint16_t)0x0010)              /*!< Watchdog timer mode select */
/* WDT_A_CTL[SSEL] Bits */
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
/* WDT_A_CTL[HOLD] Bits */
#define WDT_A_CTL_HOLD_OFS                       ( 7)                            /*!< WDTHOLD Bit Offset */
#define WDT_A_CTL_HOLD                           ((uint16_t)0x0080)              /*!< Watchdog timer hold */
/* WDT_A_CTL[PW] Bits */
#define WDT_A_CTL_PW_OFS                         ( 8)                            /*!< WDTPW Bit Offset */
#define WDT_A_CTL_PW_MASK                        ((uint16_t)0xFF00)              /*!< WDTPW Bit Mask */
/* Pre-defined bitfield values */
#define WDT_A_CTL_PW                              ((uint16_t)0x5A00)              /*!< WDT Key Value for WDT write access */


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

#endif /* __MSP432P401R_H__ */

