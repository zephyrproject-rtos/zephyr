/**
 * \file
 *
 * \brief Header file for ATSAME70J20B
 *
 * Copyright (c) 2018 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

/* file generated from device description version 2018-01-24T14:00:00Z */
#ifndef _SAME70J20B_H_
#define _SAME70J20B_H_

/** \addtogroup SAME70J20B_definitions SAME70J20B definitions
  This file defines all structures and symbols for SAME70J20B:
    - registers and bitfields
    - peripheral base address
    - peripheral ID
    - PIO definitions
 *  @{
 */

#ifdef __cplusplus
 extern "C" {
#endif

/** \defgroup Atmel_glob_defs Atmel Global Defines

    <strong>IO Type Qualifiers</strong> are used
    \li to specify the access to peripheral variables.
    \li for automatic generation of peripheral register debug information.

    \remark
    CMSIS core has a syntax that differs from this using i.e. __I, __O, or __IO followed by 'uint<size>_t' respective types.
    Default the header files will follow the CMSIS core syntax.
 *  @{
 */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#include <stdint.h>

/* IO definitions (access restrictions to peripheral registers) */
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
typedef volatile       uint32_t WoReg8;  /**< Write only  8-bit register (volatile unsigned int) */
typedef volatile       uint32_t RwReg;   /**< Read-Write 32-bit register (volatile unsigned int) */
typedef volatile       uint16_t RwReg16; /**< Read-Write 16-bit register (volatile unsigned int) */
typedef volatile       uint8_t  RwReg8;  /**< Read-Write  8-bit register (volatile unsigned int) */

#define CAST(type, value) ((type *)(value)) /**< Pointer Type Conversion Macro for C/C++ */
#define REG_ACCESS(type, address) (*(type*)(address)) /**< C code: Register value */
#else /* Assembler */
#define CAST(type, value) (value) /**< Pointer Type Conversion Macro for Assembler */
#define REG_ACCESS(type, address) (address) /**< Assembly code: Register address */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !defined(SKIP_INTEGER_LITERALS)

#if defined(_U_) || defined(_L_) || defined(_UL_)
  #error "Integer Literals macros already defined elsewhere"
#endif

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/* Macros that deal with adding suffixes to integer literal constants for C/C++ */
#define _U_(x) x ## U    /**< C code: Unsigned integer literal constant value */
#define _L_(x) x ## L    /**< C code: Long integer literal constant value */
#define _UL_(x) x ## UL  /**< C code: Unsigned Long integer literal constant value */

#else /* Assembler */

#define _U_(x) x    /**< Assembler: Unsigned integer literal constant value */
#define _L_(x) x    /**< Assembler: Long integer literal constant value */
#define _UL_(x) x   /**< Assembler: Unsigned Long integer literal constant value */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* SKIP_INTEGER_LITERALS */
/** @}  end of Atmel Global Defines */

/** \addtogroup SAME70J20B_cmsis CMSIS Definitions
 *  @{
 */
/* ************************************************************************** */
/*   CMSIS DEFINITIONS FOR SAME70J20B */
/* ************************************************************************** */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** Interrupt Number Definition */
typedef enum IRQn
{
/******  CORTEX-M7 Processor Exceptions Numbers ******************************/
  Reset_IRQn                = -15, /**< 1   Reset Vector, invoked on Power up and warm reset  */
  NonMaskableInt_IRQn       = -14, /**< 2   Non maskable Interrupt, cannot be stopped or preempted  */
  HardFault_IRQn            = -13, /**< 3   Hard Fault, all classes of Fault     */
  MemoryManagement_IRQn     = -12, /**< 4   Memory Management, MPU mismatch, including Access Violation and No Match  */
  BusFault_IRQn             = -11, /**< 5   Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory related Fault  */
  UsageFault_IRQn           = -10, /**< 6   Usage Fault, i.e. Undef Instruction, Illegal State Transition  */
  SVCall_IRQn               = -5 , /**< 11  System Service Call via SVC instruction  */
  DebugMonitor_IRQn         = -4 , /**< 12  Debug Monitor                        */
  PendSV_IRQn               = -2 , /**< 14  Pendable request for system service  */
  SysTick_IRQn              = -1 , /**< 15  System Tick Timer                    */
/******  SAME70J20B specific Interrupt Numbers ***********************************/
  SUPC_IRQn                 = 0  , /**< 0   SAME70J20B Supply Controller (SUPC) */
  RSTC_IRQn                 = 1  , /**< 1   SAME70J20B Reset Controller (RSTC)  */
  RTC_IRQn                  = 2  , /**< 2   SAME70J20B Real-time Clock (RTC)    */
  RTT_IRQn                  = 3  , /**< 3   SAME70J20B Real-time Timer (RTT)    */
  WDT_IRQn                  = 4  , /**< 4   SAME70J20B Watchdog Timer (WDT)     */
  PMC_IRQn                  = 5  , /**< 5   SAME70J20B Power Management Controller (PMC) */
  EFC_IRQn                  = 6  , /**< 6   SAME70J20B Embedded Flash Controller (EFC) */
  UART0_IRQn                = 7  , /**< 7   SAME70J20B Universal Asynchronous Receiver Transmitter (UART0) */
  UART1_IRQn                = 8  , /**< 8   SAME70J20B Universal Asynchronous Receiver Transmitter (UART1) */
  PIOA_IRQn                 = 10 , /**< 10  SAME70J20B Parallel Input/Output Controller (PIOA) */
  PIOB_IRQn                 = 11 , /**< 11  SAME70J20B Parallel Input/Output Controller (PIOB) */
  USART0_IRQn               = 13 , /**< 13  SAME70J20B Universal Synchronous Asynchronous Receiver Transmitter (USART0) */
  USART1_IRQn               = 14 , /**< 14  SAME70J20B Universal Synchronous Asynchronous Receiver Transmitter (USART1) */
  PIOD_IRQn                 = 16 , /**< 16  SAME70J20B Parallel Input/Output Controller (PIOD) */
  TWIHS0_IRQn               = 19 , /**< 19  SAME70J20B Two-wire Interface High Speed (TWIHS0) */
  TWIHS1_IRQn               = 20 , /**< 20  SAME70J20B Two-wire Interface High Speed (TWIHS1) */
  SSC_IRQn                  = 22 , /**< 22  SAME70J20B Synchronous Serial Controller (SSC) */
  TC0_IRQn                  = 23 , /**< 23  SAME70J20B Timer Counter (TC0)      */
  TC1_IRQn                  = 24 , /**< 24  SAME70J20B Timer Counter (TC0)      */
  TC2_IRQn                  = 25 , /**< 25  SAME70J20B Timer Counter (TC0)      */
  AFEC0_IRQn                = 29 , /**< 29  SAME70J20B Analog Front-End Controller (AFEC0) */
  DACC_IRQn                 = 30 , /**< 30  SAME70J20B Digital-to-Analog Converter Controller (DACC) */
  PWM0_IRQn                 = 31 , /**< 31  SAME70J20B Pulse Width Modulation Controller (PWM0) */
  ICM_IRQn                  = 32 , /**< 32  SAME70J20B Integrity Check Monitor (ICM) */
  ACC_IRQn                  = 33 , /**< 33  SAME70J20B Analog Comparator Controller (ACC) */
  USBHS_IRQn                = 34 , /**< 34  SAME70J20B USB High-Speed Interface (USBHS) */
  MCAN0_INT0_IRQn           = 35 , /**< 35  SAME70J20B Controller Area Network (MCAN0) */
  MCAN0_INT1_IRQn           = 36 , /**< 36  SAME70J20B Controller Area Network (MCAN0) */
  GMAC_IRQn                 = 39 , /**< 39  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  AFEC1_IRQn                = 40 , /**< 40  SAME70J20B Analog Front-End Controller (AFEC1) */
  QSPI_IRQn                 = 43 , /**< 43  SAME70J20B Quad Serial Peripheral Interface (QSPI) */
  UART2_IRQn                = 44 , /**< 44  SAME70J20B Universal Asynchronous Receiver Transmitter (UART2) */
  TC9_IRQn                  = 50 , /**< 50  SAME70J20B Timer Counter (TC3)      */
  TC10_IRQn                 = 51 , /**< 51  SAME70J20B Timer Counter (TC3)      */
  TC11_IRQn                 = 52 , /**< 52  SAME70J20B Timer Counter (TC3)      */
  AES_IRQn                  = 56 , /**< 56  SAME70J20B Advanced Encryption Standard (AES) */
  TRNG_IRQn                 = 57 , /**< 57  SAME70J20B True Random Number Generator (TRNG) */
  XDMAC_IRQn                = 58 , /**< 58  SAME70J20B Extensible DMA Controller (XDMAC) */
  ISI_IRQn                  = 59 , /**< 59  SAME70J20B Image Sensor Interface (ISI) */
  PWM1_IRQn                 = 60 , /**< 60  SAME70J20B Pulse Width Modulation Controller (PWM1) */
  FPU_IRQn                  = 61 , /**< 61  SAME70J20B Floating Point Unit Registers (FPU) */
  RSWDT_IRQn                = 63 , /**< 63  SAME70J20B Reinforced Safety Watchdog Timer (RSWDT) */
  CCW_IRQn                  = 64 , /**< 64  SAME70J20B System Control Registers (SystemControl) */
  CCF_IRQn                  = 65 , /**< 65  SAME70J20B System Control Registers (SystemControl) */
  GMAC_Q1_IRQn              = 66 , /**< 66  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  GMAC_Q2_IRQn              = 67 , /**< 67  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  IXC_IRQn                  = 68 , /**< 68  SAME70J20B Floating Point Unit Registers (FPU) */
  GMAC_Q3_IRQn              = 71 , /**< 71  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  GMAC_Q4_IRQn              = 72 , /**< 72  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  GMAC_Q5_IRQn              = 73 , /**< 73  SAME70J20B Gigabit Ethernet MAC (GMAC) */

  PERIPH_COUNT_IRQn        = 74  /**< Number of peripheral IDs */
} IRQn_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct _DeviceVectors
{
  /* Stack pointer */
  void* pvStack;
  /* Cortex-M handlers */
  void* pfnReset_Handler;                        /* -15 Reset Vector, invoked on Power up and warm reset  */
  void* pfnNonMaskableInt_Handler;               /* -14 Non maskable Interrupt, cannot be stopped or preempted  */
  void* pfnHardFault_Handler;                    /* -13 Hard Fault, all classes of Fault     */
  void* pfnMemoryManagement_Handler;             /* -12 Memory Management, MPU mismatch, including Access Violation and No Match  */
  void* pfnBusFault_Handler;                     /* -11 Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory related Fault  */
  void* pfnUsageFault_Handler;                   /* -10 Usage Fault, i.e. Undef Instruction, Illegal State Transition  */
  void* pvReservedC9;
  void* pvReservedC8;
  void* pvReservedC7;
  void* pvReservedC6;
  void* pfnSVCall_Handler;                       /*  -5 System Service Call via SVC instruction  */
  void* pfnDebugMonitor_Handler;                 /*  -4 Debug Monitor                        */
  void* pvReservedC3;
  void* pfnPendSV_Handler;                       /*  -2 Pendable request for system service  */
  void* pfnSysTick_Handler;                      /*  -1 System Tick Timer                    */

  /* Peripheral handlers */
  void* pfnSUPC_Handler;                         /* 0   SAME70J20B Supply Controller (SUPC) */
  void* pfnRSTC_Handler;                         /* 1   SAME70J20B Reset Controller (RSTC) */
  void* pfnRTC_Handler;                          /* 2   SAME70J20B Real-time Clock (RTC) */
  void* pfnRTT_Handler;                          /* 3   SAME70J20B Real-time Timer (RTT) */
  void* pfnWDT_Handler;                          /* 4   SAME70J20B Watchdog Timer (WDT) */
  void* pfnPMC_Handler;                          /* 5   SAME70J20B Power Management Controller (PMC) */
  void* pfnEFC_Handler;                          /* 6   SAME70J20B Embedded Flash Controller (EFC) */
  void* pfnUART0_Handler;                        /* 7   SAME70J20B Universal Asynchronous Receiver Transmitter (UART0) */
  void* pfnUART1_Handler;                        /* 8   SAME70J20B Universal Asynchronous Receiver Transmitter (UART1) */
  void* pvReserved9;
  void* pfnPIOA_Handler;                         /* 10  SAME70J20B Parallel Input/Output Controller (PIOA) */
  void* pfnPIOB_Handler;                         /* 11  SAME70J20B Parallel Input/Output Controller (PIOB) */
  void* pvReserved12;
  void* pfnUSART0_Handler;                       /* 13  SAME70J20B Universal Synchronous Asynchronous Receiver Transmitter (USART0) */
  void* pfnUSART1_Handler;                       /* 14  SAME70J20B Universal Synchronous Asynchronous Receiver Transmitter (USART1) */
  void* pvReserved15;
  void* pfnPIOD_Handler;                         /* 16  SAME70J20B Parallel Input/Output Controller (PIOD) */
  void* pvReserved17;
  void* pvReserved18;
  void* pfnTWIHS0_Handler;                       /* 19  SAME70J20B Two-wire Interface High Speed (TWIHS0) */
  void* pfnTWIHS1_Handler;                       /* 20  SAME70J20B Two-wire Interface High Speed (TWIHS1) */
  void* pvReserved21;
  void* pfnSSC_Handler;                          /* 22  SAME70J20B Synchronous Serial Controller (SSC) */
  void* pfnTC0_Handler;                          /* 23  SAME70J20B Timer Counter (TC0) */
  void* pfnTC1_Handler;                          /* 24  SAME70J20B Timer Counter (TC0) */
  void* pfnTC2_Handler;                          /* 25  SAME70J20B Timer Counter (TC0) */
  void* pvReserved26;
  void* pvReserved27;
  void* pvReserved28;
  void* pfnAFEC0_Handler;                        /* 29  SAME70J20B Analog Front-End Controller (AFEC0) */
  void* pfnDACC_Handler;                         /* 30  SAME70J20B Digital-to-Analog Converter Controller (DACC) */
  void* pfnPWM0_Handler;                         /* 31  SAME70J20B Pulse Width Modulation Controller (PWM0) */
  void* pfnICM_Handler;                          /* 32  SAME70J20B Integrity Check Monitor (ICM) */
  void* pfnACC_Handler;                          /* 33  SAME70J20B Analog Comparator Controller (ACC) */
  void* pfnUSBHS_Handler;                        /* 34  SAME70J20B USB High-Speed Interface (USBHS) */
  void* pfnMCAN0_INT0_Handler;                   /* 35  SAME70J20B Controller Area Network (MCAN0) */
  void* pfnMCAN0_INT1_Handler;                   /* 36  SAME70J20B Controller Area Network (MCAN0) */
  void* pvReserved37;
  void* pvReserved38;
  void* pfnGMAC_Handler;                         /* 39  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  void* pfnAFEC1_Handler;                        /* 40  SAME70J20B Analog Front-End Controller (AFEC1) */
  void* pvReserved41;
  void* pvReserved42;
  void* pfnQSPI_Handler;                         /* 43  SAME70J20B Quad Serial Peripheral Interface (QSPI) */
  void* pfnUART2_Handler;                        /* 44  SAME70J20B Universal Asynchronous Receiver Transmitter (UART2) */
  void* pvReserved45;
  void* pvReserved46;
  void* pvReserved47;
  void* pvReserved48;
  void* pvReserved49;
  void* pfnTC9_Handler;                          /* 50  SAME70J20B Timer Counter (TC3) */
  void* pfnTC10_Handler;                         /* 51  SAME70J20B Timer Counter (TC3) */
  void* pfnTC11_Handler;                         /* 52  SAME70J20B Timer Counter (TC3) */
  void* pvReserved53;
  void* pvReserved54;
  void* pvReserved55;
  void* pfnAES_Handler;                          /* 56  SAME70J20B Advanced Encryption Standard (AES) */
  void* pfnTRNG_Handler;                         /* 57  SAME70J20B True Random Number Generator (TRNG) */
  void* pfnXDMAC_Handler;                        /* 58  SAME70J20B Extensible DMA Controller (XDMAC) */
  void* pfnISI_Handler;                          /* 59  SAME70J20B Image Sensor Interface (ISI) */
  void* pfnPWM1_Handler;                         /* 60  SAME70J20B Pulse Width Modulation Controller (PWM1) */
  void* pfnFPU_Handler;                          /* 61  SAME70J20B Floating Point Unit Registers (FPU) */
  void* pvReserved62;
  void* pfnRSWDT_Handler;                        /* 63  SAME70J20B Reinforced Safety Watchdog Timer (RSWDT) */
  void* pfnCCW_Handler;                          /* 64  SAME70J20B System Control Registers (SystemControl) */
  void* pfnCCF_Handler;                          /* 65  SAME70J20B System Control Registers (SystemControl) */
  void* pfnGMAC_Q1_Handler;                      /* 66  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  void* pfnGMAC_Q2_Handler;                      /* 67  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  void* pfnIXC_Handler;                          /* 68  SAME70J20B Floating Point Unit Registers (FPU) */
  void* pvReserved69;
  void* pvReserved70;
  void* pfnGMAC_Q3_Handler;                      /* 71  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  void* pfnGMAC_Q4_Handler;                      /* 72  SAME70J20B Gigabit Ethernet MAC (GMAC) */
  void* pfnGMAC_Q5_Handler;                      /* 73  SAME70J20B Gigabit Ethernet MAC (GMAC) */
} DeviceVectors;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if !defined DONT_USE_PREDEFINED_CORE_HANDLERS

/* CORTEX-M7 core handlers */
void Reset_Handler                 ( void );
void NonMaskableInt_Handler        ( void );
void HardFault_Handler             ( void );
void MemoryManagement_Handler      ( void );
void BusFault_Handler              ( void );
void UsageFault_Handler            ( void );
void SVCall_Handler                ( void );
void DebugMonitor_Handler          ( void );
void PendSV_Handler                ( void );
void SysTick_Handler               ( void );
#endif /* DONT_USE_PREDEFINED_CORE_HANDLERS */

#if !defined DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

/* Peripherals handlers */
void ACC_Handler                   ( void );
void AES_Handler                   ( void );
void AFEC0_Handler                 ( void );
void AFEC1_Handler                 ( void );
void CCF_Handler                   ( void );
void CCW_Handler                   ( void );
void DACC_Handler                  ( void );
void EFC_Handler                   ( void );
void FPU_Handler                   ( void );
void GMAC_Handler                  ( void );
void GMAC_Q1_Handler               ( void );
void GMAC_Q2_Handler               ( void );
void GMAC_Q3_Handler               ( void );
void GMAC_Q4_Handler               ( void );
void GMAC_Q5_Handler               ( void );
void ICM_Handler                   ( void );
void ISI_Handler                   ( void );
void IXC_Handler                   ( void );
void MCAN0_INT0_Handler            ( void );
void MCAN0_INT1_Handler            ( void );
void PIOA_Handler                  ( void );
void PIOB_Handler                  ( void );
void PIOD_Handler                  ( void );
void PMC_Handler                   ( void );
void PWM0_Handler                  ( void );
void PWM1_Handler                  ( void );
void QSPI_Handler                  ( void );
void RSTC_Handler                  ( void );
void RSWDT_Handler                 ( void );
void RTC_Handler                   ( void );
void RTT_Handler                   ( void );
void SSC_Handler                   ( void );
void SUPC_Handler                  ( void );
void TC0_Handler                   ( void );
void TC10_Handler                  ( void );
void TC11_Handler                  ( void );
void TC1_Handler                   ( void );
void TC2_Handler                   ( void );
void TC9_Handler                   ( void );
void TRNG_Handler                  ( void );
void TWIHS0_Handler                ( void );
void TWIHS1_Handler                ( void );
void UART0_Handler                 ( void );
void UART1_Handler                 ( void );
void UART2_Handler                 ( void );
void USART0_Handler                ( void );
void USART1_Handler                ( void );
void USBHS_Handler                 ( void );
void WDT_Handler                   ( void );
void XDMAC_Handler                 ( void );
#endif /* DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/*
 * \brief Configuration of the CORTEX-M7 Processor and Core Peripherals
 */

#define __CM7_REV                 0x0101 /**< CM7 Core Revision                                                         */
#define __NVIC_PRIO_BITS               3 /**< Number of Bits used for Priority Levels                                   */
#define __Vendor_SysTickConfig         0 /**< Set to 1 if different SysTick Config is used                              */
#define __MPU_PRESENT                  1 /**< MPU present or not                                                        */
#define __VTOR_PRESENT                 1 /**< Vector Table Offset Register present or not                               */
#define __FPU_PRESENT                  1 /**< FPU present or not                                                        */
#define __FPU_DP                       1 /**< Double Precision FPU                                                      */
#define __ICACHE_PRESENT               1 /**< Instruction Cache present                                                 */
#define __DCACHE_PRESENT               1 /**< Data Cache present                                                        */
#define __ITCM_PRESENT                 1 /**< Instruction TCM present                                                   */
#define __DTCM_PRESENT                 1 /**< Data TCM present                                                          */
#define __DEBUG_LVL                    1
#define __TRACE_LVL                    1
#define LITTLE_ENDIAN                  1
#define __ARCH_ARM                     1
#define __ARCH_ARM_CORTEX_M            1
#define __DEVICE_IS_SAM                1

/*
 * \brief CMSIS includes
 */
#include <core_cm7.h>
#if !defined DONT_USE_CMSIS_INIT
#include "system_same70.h"
#endif /* DONT_USE_CMSIS_INIT */

/** @}  end of SAME70J20B_cmsis CMSIS Definitions */

/** \defgroup SAME70J20B_api Peripheral Software API
 *  @{
 */

/* ************************************************************************** */
/**  SOFTWARE PERIPHERAL API DEFINITION FOR SAME70J20B */
/* ************************************************************************** */
#include "component/acc.h"
#include "component/aes.h"
#include "component/afec.h"
#include "component/chipid.h"
#include "component/dacc.h"
#include "component/efc.h"
#include "component/gmac.h"
#include "component/gpbr.h"
#include "component/icm.h"
#include "component/isi.h"
#include "component/matrix.h"
#include "component/mcan.h"
#include "component/pio.h"
#include "component/pmc.h"
#include "component/pwm.h"
#include "component/qspi.h"
#include "component/rstc.h"
#include "component/rswdt.h"
#include "component/rtc.h"
#include "component/rtt.h"
#include "component/ssc.h"
#include "component/supc.h"
#include "component/tc.h"
#include "component/trng.h"
#include "component/twihs.h"
#include "component/uart.h"
#include "component/usart.h"
#include "component/usbhs.h"
#include "component/utmi.h"
#include "component/wdt.h"
#include "component/xdmac.h"
/** @}  end of Peripheral Software API */

/** \defgroup SAME70J20B_reg Registers Access Definitions
 *  @{
 */

/* ************************************************************************** */
/*   REGISTER ACCESS DEFINITIONS FOR SAME70J20B */
/* ************************************************************************** */
#include "instance/acc.h"
#include "instance/aes.h"
#include "instance/afec0.h"
#include "instance/afec1.h"
#include "instance/chipid.h"
#include "instance/dacc.h"
#include "instance/efc.h"
#include "instance/gmac.h"
#include "instance/gpbr.h"
#include "instance/icm.h"
#include "instance/isi.h"
#include "instance/matrix.h"
#include "instance/mcan0.h"
#include "instance/pioa.h"
#include "instance/piob.h"
#include "instance/piod.h"
#include "instance/pmc.h"
#include "instance/pwm0.h"
#include "instance/pwm1.h"
#include "instance/qspi.h"
#include "instance/rstc.h"
#include "instance/rswdt.h"
#include "instance/rtc.h"
#include "instance/rtt.h"
#include "instance/ssc.h"
#include "instance/supc.h"
#include "instance/tc0.h"
#include "instance/tc3.h"
#include "instance/trng.h"
#include "instance/twihs0.h"
#include "instance/twihs1.h"
#include "instance/uart0.h"
#include "instance/uart1.h"
#include "instance/uart2.h"
#include "instance/usart0.h"
#include "instance/usart1.h"
#include "instance/usbhs.h"
#include "instance/utmi.h"
#include "instance/wdt.h"
#include "instance/xdmac.h"
/** @}  end of Registers Access Definitions */

/** \addtogroup SAME70J20B_id Peripheral Ids Definitions
 *  @{
 */

/* ************************************************************************** */
/*  PERIPHERAL ID DEFINITIONS FOR SAME70J20B */
/* ************************************************************************** */
#define ID_SUPC         (  0) /**< \brief Supply Controller (SUPC) */
#define ID_RSTC         (  1) /**< \brief Reset Controller (RSTC) */
#define ID_RTC          (  2) /**< \brief Real-time Clock (RTC) */
#define ID_RTT          (  3) /**< \brief Real-time Timer (RTT) */
#define ID_WDT          (  4) /**< \brief Watchdog Timer (WDT) */
#define ID_PMC          (  5) /**< \brief Power Management Controller (PMC) */
#define ID_EFC          (  6) /**< \brief Embedded Flash Controller (EFC) */
#define ID_UART0        (  7) /**< \brief Universal Asynchronous Receiver Transmitter (UART0) */
#define ID_UART1        (  8) /**< \brief Universal Asynchronous Receiver Transmitter (UART1) */
#define ID_PIOA         ( 10) /**< \brief Parallel Input/Output Controller (PIOA) */
#define ID_PIOB         ( 11) /**< \brief Parallel Input/Output Controller (PIOB) */
#define ID_USART0       ( 13) /**< \brief Universal Synchronous Asynchronous Receiver Transmitter (USART0) */
#define ID_USART1       ( 14) /**< \brief Universal Synchronous Asynchronous Receiver Transmitter (USART1) */
#define ID_PIOD         ( 16) /**< \brief Parallel Input/Output Controller (PIOD) */
#define ID_TWIHS0       ( 19) /**< \brief Two-wire Interface High Speed (TWIHS0) */
#define ID_TWIHS1       ( 20) /**< \brief Two-wire Interface High Speed (TWIHS1) */
#define ID_SSC          ( 22) /**< \brief Synchronous Serial Controller (SSC) */
#define ID_TC0_CHANNEL0 ( 23) /**< \brief Timer Counter (TC0_CHANNEL0) */
#define ID_TC0_CHANNEL1 ( 24) /**< \brief Timer Counter (TC0_CHANNEL1) */
#define ID_TC0_CHANNEL2 ( 25) /**< \brief Timer Counter (TC0_CHANNEL2) */
#define ID_AFEC0        ( 29) /**< \brief Analog Front-End Controller (AFEC0) */
#define ID_DACC         ( 30) /**< \brief Digital-to-Analog Converter Controller (DACC) */
#define ID_PWM0         ( 31) /**< \brief Pulse Width Modulation Controller (PWM0) */
#define ID_ICM          ( 32) /**< \brief Integrity Check Monitor (ICM) */
#define ID_ACC          ( 33) /**< \brief Analog Comparator Controller (ACC) */
#define ID_USBHS        ( 34) /**< \brief USB High-Speed Interface (USBHS) */
#define ID_MCAN0        ( 35) /**< \brief Controller Area Network (MCAN0) */
#define ID_GMAC         ( 39) /**< \brief Gigabit Ethernet MAC (GMAC) */
#define ID_AFEC1        ( 40) /**< \brief Analog Front-End Controller (AFEC1) */
#define ID_QSPI         ( 43) /**< \brief Quad Serial Peripheral Interface (QSPI) */
#define ID_UART2        ( 44) /**< \brief Universal Asynchronous Receiver Transmitter (UART2) */
#define ID_TC3_CHANNEL0 ( 50) /**< \brief Timer Counter (TC3_CHANNEL0) */
#define ID_TC3_CHANNEL1 ( 51) /**< \brief Timer Counter (TC3_CHANNEL1) */
#define ID_TC3_CHANNEL2 ( 52) /**< \brief Timer Counter (TC3_CHANNEL2) */
#define ID_AES          ( 56) /**< \brief Advanced Encryption Standard (AES) */
#define ID_TRNG         ( 57) /**< \brief True Random Number Generator (TRNG) */
#define ID_XDMAC        ( 58) /**< \brief Extensible DMA Controller (XDMAC) */
#define ID_ISI          ( 59) /**< \brief Image Sensor Interface (ISI) */
#define ID_PWM1         ( 60) /**< \brief Pulse Width Modulation Controller (PWM1) */
#define ID_RSWDT        ( 63) /**< \brief Reinforced Safety Watchdog Timer (RSWDT) */

#define ID_PERIPH_COUNT ( 64) /**< \brief Number of peripheral IDs */
/** @}  end of Peripheral Ids Definitions */

/** \addtogroup legacy_SAME70J20B_id Legacy Peripheral Ids Definitions
 *  @{
 */

/* ************************************************************************** */
/*  LEGACY PERIPHERAL ID DEFINITIONS FOR SAME70J20B */
/* ************************************************************************** */
#define ID_TC0                   TC0_INSTANCE_ID_CHANNEL0
#define ID_TC1                   TC0_INSTANCE_ID_CHANNEL1
#define ID_TC2                   TC0_INSTANCE_ID_CHANNEL2
#define ID_TC3                   TC1_INSTANCE_ID_CHANNEL0
#define ID_TC4                   TC1_INSTANCE_ID_CHANNEL1
#define ID_TC5                   TC1_INSTANCE_ID_CHANNEL2
#define ID_TC6                   TC2_INSTANCE_ID_CHANNEL0
#define ID_TC7                   TC2_INSTANCE_ID_CHANNEL1
#define ID_TC8                   TC2_INSTANCE_ID_CHANNEL2
#define ID_TC9                   TC3_INSTANCE_ID_CHANNEL0
#define ID_TC10                  TC3_INSTANCE_ID_CHANNEL1
#define ID_TC11                  TC3_INSTANCE_ID_CHANNEL2
/** @}  end of Legacy Peripheral Ids Definitions */

/** \addtogroup SAME70J20B_base Peripheral Base Address Definitions
 *  @{
 */

/* ************************************************************************** */
/*   BASE ADDRESS DEFINITIONS FOR SAME70J20B */
/* ************************************************************************** */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#define ACC                    (0x40044000)                   /**< \brief (ACC       ) Base Address */
#define AES                    (0x4006C000)                   /**< \brief (AES       ) Base Address */
#define AFEC0                  (0x4003C000)                   /**< \brief (AFEC0     ) Base Address */
#define AFEC1                  (0x40064000)                   /**< \brief (AFEC1     ) Base Address */
#define CHIPID                 (0x400E0940)                   /**< \brief (CHIPID    ) Base Address */
#define DACC                   (0x40040000)                   /**< \brief (DACC      ) Base Address */
#define EFC                    (0x400E0C00)                   /**< \brief (EFC       ) Base Address */
#define GMAC                   (0x40050000)                   /**< \brief (GMAC      ) Base Address */
#define GPBR                   (0x400E1890)                   /**< \brief (GPBR      ) Base Address */
#define ICM                    (0x40048000)                   /**< \brief (ICM       ) Base Address */
#define ISI                    (0x4004C000)                   /**< \brief (ISI       ) Base Address */
#define MATRIX                 (0x40088000)                   /**< \brief (MATRIX    ) Base Address */
#define MCAN0                  (0x40030000)                   /**< \brief (MCAN0     ) Base Address */
#define PIOA                   (0x400E0E00)                   /**< \brief (PIOA      ) Base Address */
#define PIOB                   (0x400E1000)                   /**< \brief (PIOB      ) Base Address */
#define PIOD                   (0x400E1400)                   /**< \brief (PIOD      ) Base Address */
#define PMC                    (0x400E0600)                   /**< \brief (PMC       ) Base Address */
#define PWM0                   (0x40020000)                   /**< \brief (PWM0      ) Base Address */
#define PWM1                   (0x4005C000)                   /**< \brief (PWM1      ) Base Address */
#define QSPI                   (0x4007C000)                   /**< \brief (QSPI      ) Base Address */
#define RSTC                   (0x400E1800)                   /**< \brief (RSTC      ) Base Address */
#define RSWDT                  (0x400E1900)                   /**< \brief (RSWDT     ) Base Address */
#define RTC                    (0x400E1860)                   /**< \brief (RTC       ) Base Address */
#define RTT                    (0x400E1830)                   /**< \brief (RTT       ) Base Address */
#define SSC                    (0x40004000)                   /**< \brief (SSC       ) Base Address */
#define SUPC                   (0x400E1810)                   /**< \brief (SUPC      ) Base Address */
#define TC0                    (0x4000C000)                   /**< \brief (TC0       ) Base Address */
#define TC3                    (0x40054000)                   /**< \brief (TC3       ) Base Address */
#define TRNG                   (0x40070000)                   /**< \brief (TRNG      ) Base Address */
#define TWIHS0                 (0x40018000)                   /**< \brief (TWIHS0    ) Base Address */
#define TWIHS1                 (0x4001C000)                   /**< \brief (TWIHS1    ) Base Address */
#define UART0                  (0x400E0800)                   /**< \brief (UART0     ) Base Address */
#define UART1                  (0x400E0A00)                   /**< \brief (UART1     ) Base Address */
#define UART2                  (0x400E1A00)                   /**< \brief (UART2     ) Base Address */
#define USART0                 (0x40024000)                   /**< \brief (USART0    ) Base Address */
#define USART1                 (0x40028000)                   /**< \brief (USART1    ) Base Address */
#define USBHS                  (0x40038000)                   /**< \brief (USBHS     ) Base Address */
#define UTMI                   (0x400E0400)                   /**< \brief (UTMI      ) Base Address */
#define WDT                    (0x400E1850)                   /**< \brief (WDT       ) Base Address */
#define XDMAC                  (0x40078000)                   /**< \brief (XDMAC     ) Base Address */

#else /* For C/C++ compiler */

#define ACC                    ((Acc *)0x40044000U)           /**< \brief (ACC       ) Base Address */
#define ACC_INST_NUM           1                              /**< \brief (ACC       ) Number of instances */
#define ACC_INSTS              { ACC }                        /**< \brief (ACC       ) Instances List */

#define AES                    ((Aes *)0x4006C000U)           /**< \brief (AES       ) Base Address */
#define AES_INST_NUM           1                              /**< \brief (AES       ) Number of instances */
#define AES_INSTS              { AES }                        /**< \brief (AES       ) Instances List */

#define AFEC0                  ((Afec *)0x4003C000U)          /**< \brief (AFEC0     ) Base Address */
#define AFEC1                  ((Afec *)0x40064000U)          /**< \brief (AFEC1     ) Base Address */
#define AFEC_INST_NUM          2                              /**< \brief (AFEC      ) Number of instances */
#define AFEC_INSTS             { AFEC0, AFEC1 }               /**< \brief (AFEC      ) Instances List */

#define CHIPID                 ((Chipid *)0x400E0940U)        /**< \brief (CHIPID    ) Base Address */
#define CHIPID_INST_NUM        1                              /**< \brief (CHIPID    ) Number of instances */
#define CHIPID_INSTS           { CHIPID }                     /**< \brief (CHIPID    ) Instances List */

#define DACC                   ((Dacc *)0x40040000U)          /**< \brief (DACC      ) Base Address */
#define DACC_INST_NUM          1                              /**< \brief (DACC      ) Number of instances */
#define DACC_INSTS             { DACC }                       /**< \brief (DACC      ) Instances List */

#define EFC                    ((Efc *)0x400E0C00U)           /**< \brief (EFC       ) Base Address */
#define EFC_INST_NUM           1                              /**< \brief (EFC       ) Number of instances */
#define EFC_INSTS              { EFC }                        /**< \brief (EFC       ) Instances List */

#define GMAC                   ((Gmac *)0x40050000U)          /**< \brief (GMAC      ) Base Address */
#define GMAC_INST_NUM          1                              /**< \brief (GMAC      ) Number of instances */
#define GMAC_INSTS             { GMAC }                       /**< \brief (GMAC      ) Instances List */

#define GPBR                   ((Gpbr *)0x400E1890U)          /**< \brief (GPBR      ) Base Address */
#define GPBR_INST_NUM          1                              /**< \brief (GPBR      ) Number of instances */
#define GPBR_INSTS             { GPBR }                       /**< \brief (GPBR      ) Instances List */

#define ICM                    ((Icm *)0x40048000U)           /**< \brief (ICM       ) Base Address */
#define ICM_INST_NUM           1                              /**< \brief (ICM       ) Number of instances */
#define ICM_INSTS              { ICM }                        /**< \brief (ICM       ) Instances List */

#define ISI                    ((Isi *)0x4004C000U)           /**< \brief (ISI       ) Base Address */
#define ISI_INST_NUM           1                              /**< \brief (ISI       ) Number of instances */
#define ISI_INSTS              { ISI }                        /**< \brief (ISI       ) Instances List */

#define MATRIX                 ((Matrix *)0x40088000U)        /**< \brief (MATRIX    ) Base Address */
#define MATRIX_INST_NUM        1                              /**< \brief (MATRIX    ) Number of instances */
#define MATRIX_INSTS           { MATRIX }                     /**< \brief (MATRIX    ) Instances List */

#define MCAN0                  ((Mcan *)0x40030000U)          /**< \brief (MCAN0     ) Base Address */
#define MCAN_INST_NUM          1                              /**< \brief (MCAN      ) Number of instances */
#define MCAN_INSTS             { MCAN0 }                      /**< \brief (MCAN      ) Instances List */

#define PIOA                   ((Pio *)0x400E0E00U)           /**< \brief (PIOA      ) Base Address */
#define PIOB                   ((Pio *)0x400E1000U)           /**< \brief (PIOB      ) Base Address */
#define PIOD                   ((Pio *)0x400E1400U)           /**< \brief (PIOD      ) Base Address */
#define PIO_INST_NUM           3                              /**< \brief (PIO       ) Number of instances */
#define PIO_INSTS              { PIOA, PIOB, PIOD }           /**< \brief (PIO       ) Instances List */

#define PMC                    ((Pmc *)0x400E0600U)           /**< \brief (PMC       ) Base Address */
#define PMC_INST_NUM           1                              /**< \brief (PMC       ) Number of instances */
#define PMC_INSTS              { PMC }                        /**< \brief (PMC       ) Instances List */

#define PWM0                   ((Pwm *)0x40020000U)           /**< \brief (PWM0      ) Base Address */
#define PWM1                   ((Pwm *)0x4005C000U)           /**< \brief (PWM1      ) Base Address */
#define PWM_INST_NUM           2                              /**< \brief (PWM       ) Number of instances */
#define PWM_INSTS              { PWM0, PWM1 }                 /**< \brief (PWM       ) Instances List */

#define QSPI                   ((Qspi *)0x4007C000U)          /**< \brief (QSPI      ) Base Address */
#define QSPI_INST_NUM          1                              /**< \brief (QSPI      ) Number of instances */
#define QSPI_INSTS             { QSPI }                       /**< \brief (QSPI      ) Instances List */

#define RSTC                   ((Rstc *)0x400E1800U)          /**< \brief (RSTC      ) Base Address */
#define RSTC_INST_NUM          1                              /**< \brief (RSTC      ) Number of instances */
#define RSTC_INSTS             { RSTC }                       /**< \brief (RSTC      ) Instances List */

#define RSWDT                  ((Rswdt *)0x400E1900U)         /**< \brief (RSWDT     ) Base Address */
#define RSWDT_INST_NUM         1                              /**< \brief (RSWDT     ) Number of instances */
#define RSWDT_INSTS            { RSWDT }                      /**< \brief (RSWDT     ) Instances List */

#define RTC                    ((Rtc *)0x400E1860U)           /**< \brief (RTC       ) Base Address */
#define RTC_INST_NUM           1                              /**< \brief (RTC       ) Number of instances */
#define RTC_INSTS              { RTC }                        /**< \brief (RTC       ) Instances List */

#define RTT                    ((Rtt *)0x400E1830U)           /**< \brief (RTT       ) Base Address */
#define RTT_INST_NUM           1                              /**< \brief (RTT       ) Number of instances */
#define RTT_INSTS              { RTT }                        /**< \brief (RTT       ) Instances List */

#define SSC                    ((Ssc *)0x40004000U)           /**< \brief (SSC       ) Base Address */
#define SSC_INST_NUM           1                              /**< \brief (SSC       ) Number of instances */
#define SSC_INSTS              { SSC }                        /**< \brief (SSC       ) Instances List */

#define SUPC                   ((Supc *)0x400E1810U)          /**< \brief (SUPC      ) Base Address */
#define SUPC_INST_NUM          1                              /**< \brief (SUPC      ) Number of instances */
#define SUPC_INSTS             { SUPC }                       /**< \brief (SUPC      ) Instances List */

#define TC0                    ((Tc *)0x4000C000U)            /**< \brief (TC0       ) Base Address */
#define TC3                    ((Tc *)0x40054000U)            /**< \brief (TC3       ) Base Address */
#define TC_INST_NUM            2                              /**< \brief (TC        ) Number of instances */
#define TC_INSTS               { TC0, TC3 }                   /**< \brief (TC        ) Instances List */

#define TRNG                   ((Trng *)0x40070000U)          /**< \brief (TRNG      ) Base Address */
#define TRNG_INST_NUM          1                              /**< \brief (TRNG      ) Number of instances */
#define TRNG_INSTS             { TRNG }                       /**< \brief (TRNG      ) Instances List */

#define TWIHS0                 ((Twihs *)0x40018000U)         /**< \brief (TWIHS0    ) Base Address */
#define TWIHS1                 ((Twihs *)0x4001C000U)         /**< \brief (TWIHS1    ) Base Address */
#define TWIHS_INST_NUM         2                              /**< \brief (TWIHS     ) Number of instances */
#define TWIHS_INSTS            { TWIHS0, TWIHS1 }             /**< \brief (TWIHS     ) Instances List */

#define UART0                  ((Uart *)0x400E0800U)          /**< \brief (UART0     ) Base Address */
#define UART1                  ((Uart *)0x400E0A00U)          /**< \brief (UART1     ) Base Address */
#define UART2                  ((Uart *)0x400E1A00U)          /**< \brief (UART2     ) Base Address */
#define UART_INST_NUM          3                              /**< \brief (UART      ) Number of instances */
#define UART_INSTS             { UART0, UART1, UART2 }        /**< \brief (UART      ) Instances List */

#define USART0                 ((Usart *)0x40024000U)         /**< \brief (USART0    ) Base Address */
#define USART1                 ((Usart *)0x40028000U)         /**< \brief (USART1    ) Base Address */
#define USART_INST_NUM         2                              /**< \brief (USART     ) Number of instances */
#define USART_INSTS            { USART0, USART1 }             /**< \brief (USART     ) Instances List */

#define USBHS                  ((Usbhs *)0x40038000U)         /**< \brief (USBHS     ) Base Address */
#define USBHS_INST_NUM         1                              /**< \brief (USBHS     ) Number of instances */
#define USBHS_INSTS            { USBHS }                      /**< \brief (USBHS     ) Instances List */

#define UTMI                   ((Utmi *)0x400E0400U)          /**< \brief (UTMI      ) Base Address */
#define UTMI_INST_NUM          1                              /**< \brief (UTMI      ) Number of instances */
#define UTMI_INSTS             { UTMI }                       /**< \brief (UTMI      ) Instances List */

#define WDT                    ((Wdt *)0x400E1850U)           /**< \brief (WDT       ) Base Address */
#define WDT_INST_NUM           1                              /**< \brief (WDT       ) Number of instances */
#define WDT_INSTS              { WDT }                        /**< \brief (WDT       ) Instances List */

#define XDMAC                  ((Xdmac *)0x40078000U)         /**< \brief (XDMAC     ) Base Address */
#define XDMAC_INST_NUM         1                              /**< \brief (XDMAC     ) Number of instances */
#define XDMAC_INSTS            { XDMAC }                      /**< \brief (XDMAC     ) Instances List */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/** @}  end of Peripheral Base Address Definitions */

/** \addtogroup SAME70J20B_pio Peripheral Pio Definitions
 *  @{
 */

/* ************************************************************************** */
/*   PIO DEFINITIONS FOR SAME70J20B*/
/* ************************************************************************** */
#include "pio/same70j20b.h"
/** @}  end of Peripheral Pio Definitions */

/* ************************************************************************** */
/*   MEMORY MAPPING DEFINITIONS FOR SAME70J20B*/
/* ************************************************************************** */

#define PERIPHERALS_SIZE         _U_(0x20000000)       /* 524288kB Memory segment type: io */
#define SYSTEM_SIZE              _U_(0x10000000)       /* 262144kB Memory segment type: io */
#define QSPIMEM_SIZE             _U_(0x20000000)       /* 524288kB Memory segment type: other */
#define AXIMX_SIZE               _U_(0x00100000)       /* 1024kB Memory segment type: other */
#define ITCM_SIZE                _U_(0x00200000)       /* 2048kB Memory segment type: other */
#define IFLASH_SIZE              _U_(0x00100000)       /* 1024kB Memory segment type: flash */
#define IFLASH_PAGE_SIZE         _U_(       512)
#define IFLASH_NB_OF_PAGES       _U_(      2048)

#define IROM_SIZE                _U_(0x00004000)       /*   16kB Memory segment type: rom */
#define DTCM_SIZE                _U_(0x00020000)       /*  128kB Memory segment type: other */
#define IRAM_SIZE                _U_(0x00060000)       /*  384kB Memory segment type: ram */
#define SDRAM_CS_SIZE            _U_(0x10000000)       /* 262144kB Memory segment type: other */

#define PERIPHERALS_ADDR         _U_(0x40000000)       /**< PERIPHERALS base address (type: io)*/
#define SYSTEM_ADDR              _U_(0xe0000000)       /**< SYSTEM base address (type: io)*/
#define QSPIMEM_ADDR             _U_(0x80000000)       /**< QSPIMEM base address (type: other)*/
#define AXIMX_ADDR               _U_(0xa0000000)       /**< AXIMX base address (type: other)*/
#define ITCM_ADDR                _U_(0x00000000)       /**< ITCM base address (type: other)*/
#define IFLASH_ADDR              _U_(0x00400000)       /**< IFLASH base address (type: flash)*/
#define IROM_ADDR                _U_(0x00800000)       /**< IROM base address (type: rom)*/
#define DTCM_ADDR                _U_(0x20000000)       /**< DTCM base address (type: other)*/
#define IRAM_ADDR                _U_(0x20400000)       /**< IRAM base address (type: ram)*/
#define SDRAM_CS_ADDR            _U_(0x70000000)       /**< SDRAM_CS base address (type: other)*/

/* ************************************************************************** */
/**  DEVICE SIGNATURES FOR SAME70J20B */
/* ************************************************************************** */
#define JTAGID                   _UL_(0X05B3D03F)
#define CHIP_JTAGID              _UL_(0X05B3D03F)
#define CHIP_CIDR                _UL_(0XA1020C01)
#define CHIP_EXID                _UL_(0X00000000)

/* ************************************************************************** */
/**  ELECTRICAL DEFINITIONS FOR SAME70J20B */
/* ************************************************************************** */
#define CHIP_FREQ_SLCK_RC_MIN          _UL_(20000)     
#define CHIP_FREQ_SLCK_RC              _UL_(32000)     /**< \brief Typical Slow Clock Internal RC frequency*/
#define CHIP_FREQ_SLCK_RC_MAX          _UL_(44000)     
#define CHIP_FREQ_MAINCK_RC_4MHZ       _UL_(4000000)   
#define CHIP_FREQ_MAINCK_RC_8MHZ       _UL_(8000000)   
#define CHIP_FREQ_MAINCK_RC_12MHZ      _UL_(12000000)  
#define CHIP_FREQ_CPU_MAX              _UL_(300000000) 
#define CHIP_FREQ_XTAL_32K             _UL_(32768)     
#define CHIP_FREQ_XTAL_12M             _UL_(12000000)  
#define CHIP_FREQ_FWS_0                _UL_(23000000)  /**< \brief Maximum operating frequency when FWS is 0*/
#define CHIP_FREQ_FWS_1                _UL_(46000000)  /**< \brief Maximum operating frequency when FWS is 1*/
#define CHIP_FREQ_FWS_2                _UL_(69000000)  /**< \brief Maximum operating frequency when FWS is 2*/
#define CHIP_FREQ_FWS_3                _UL_(92000000)  /**< \brief Maximum operating frequency when FWS is 3*/
#define CHIP_FREQ_FWS_4                _UL_(115000000) /**< \brief Maximum operating frequency when FWS is 4*/
#define CHIP_FREQ_FWS_5                _UL_(138000000) /**< \brief Maximum operating frequency when FWS is 5*/
#define CHIP_FREQ_FWS_6                _UL_(150000000) /**< \brief Maximum operating frequency when FWS is 6*/
#define CHIP_FREQ_FWS_NUMBER           _UL_(7)         /**< \brief Number of FWS ranges*/

#ifdef __cplusplus
}
#endif

/** @}  end of SAME70J20B definitions */


#endif /* _SAME70J20B_H_ */
