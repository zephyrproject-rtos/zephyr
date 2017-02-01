/**
 * \file
 *
 * \brief Header file for ATSAME70N20
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
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

#ifndef _SAME70N20_H_
#define _SAME70N20_H_

/** \addtogroup SAME70N20_definitions SAME70N20 definitions
  This file defines all structures and symbols for SAME70N20:
    - registers and bitfields
    - peripheral base address
    - peripheral ID
    - PIO definitions
 *  @{
 */

#ifdef __cplusplus
 extern "C" {
#endif

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#include <stdint.h>
/* IO definitions (access restrictions to peripheral registers) */
/** \defgroup Atmel_glob_defs Atmel Global Defines

    <strong>IO Type Qualifiers</strong> are used
    \li to specify the access to peripheral variables.
    \li for automatic generation of peripheral register debug information.

    \remark
    CMSIS core has a syntax that differs from this using i.e. __I, __O, or __IO followed by 'uint<size>_t' respective types.
    Default the header files will follow the CMSIS core syntax.
 *  @{
 */

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

#if !defined(_UL)
/* Macros that deal with adding suffixes to integer literal constants for C/C++ */
#define _U(x) x ## U    /**< C code: Unsigned integer literal constant value */
#define _L(x) x ## L    /**< C code: Long integer literal constant value */
#define _UL(x) x ## UL  /**< C code: Unsigned Long integer literal constant value */
#endif /* !defined(UL) */
#else /* Assembler */
#define CAST(type, value) (value) /**< Pointer Type Conversion Macro for Assembler */
#define REG_ACCESS(type, address) (address) /**< Assembly code: Register address */
#if !defined(_UL)
#define _U(x) x    /**< Assembler: Unsigned integer literal constant value */
#define _L(x) x    /**< Assembler: Long integer literal constant value */
#define _UL(x) x   /**< Assembler: Unsigned Long integer literal constant value */
#endif /* !defined(UL) */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

/** @}  end of Atmel Global Defines */

/** \addtogroup SAME70N20_cmsis CMSIS Definitions
 *  @{
 */
/* ************************************************************************** */
/*   CMSIS DEFINITIONS FOR SAME70N20 */
/* ************************************************************************** */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** Interrupt Number Definition */
typedef enum IRQn
{
/******  CORTEX-M7 Processor Exceptions Numbers ******************************/
  NonMaskableInt_IRQn       = -14, /**< 2   Non Maskable Interrupt               */
  HardFault_IRQn            = -13, /**< 3   HardFault Interrupt                  */
  MemoryManagement_IRQn     = -12, /**< 4   Cortex-M7 Memory Management Interrupt  */
  BusFault_IRQn             = -11, /**< 5   Cortex-M7 Bus Fault Interrupt        */
  UsageFault_IRQn           = -10, /**< 6   Cortex-M7 Usage Fault Interrupt      */
  SVCall_IRQn               = -5 , /**< 11  Cortex-M7 SV Call Interrupt          */
  DebugMonitor_IRQn         = -4 , /**< 12  Cortex-M7 Debug Monitor Interrupt    */
  PendSV_IRQn               = -2 , /**< 14  Cortex-M7 Pend SV Interrupt          */
  SysTick_IRQn              = -1 , /**< 15  Cortex-M7 System Tick Interrupt      */
/******  SAME70N20 specific Interrupt Numbers ***********************************/
  SUPC_IRQn                 = 0  , /**< 0   SAME70N20 Supply Controller (SUPC)  */
  RSTC_IRQn                 = 1  , /**< 1   SAME70N20 Reset Controller (RSTC)   */
  RTC_IRQn                  = 2  , /**< 2   SAME70N20 Real-time Clock (RTC)     */
  RTT_IRQn                  = 3  , /**< 3   SAME70N20 Real-time Timer (RTT)     */
  WDT_IRQn                  = 4  , /**< 4   SAME70N20 Watchdog Timer (WDT)      */
  PMC_IRQn                  = 5  , /**< 5   SAME70N20 Power Management Controller (PMC) */
  EFC_IRQn                  = 6  , /**< 6   SAME70N20 Embedded Flash Controller (EFC) */
  UART0_IRQn                = 7  , /**< 7   SAME70N20 Universal Asynchronous Receiver Transmitter (UART0) */
  UART1_IRQn                = 8  , /**< 8   SAME70N20 Universal Asynchronous Receiver Transmitter (UART1) */
  PIOA_IRQn                 = 10 , /**< 10  SAME70N20 Parallel Input/Output Controller (PIOA) */
  PIOB_IRQn                 = 11 , /**< 11  SAME70N20 Parallel Input/Output Controller (PIOB) */
  USART0_IRQn               = 13 , /**< 13  SAME70N20 Universal Synchronous Asynchronous Receiver Transmitter (USART0) */
  USART1_IRQn               = 14 , /**< 14  SAME70N20 Universal Synchronous Asynchronous Receiver Transmitter (USART1) */
  USART2_IRQn               = 15 , /**< 15  SAME70N20 Universal Synchronous Asynchronous Receiver Transmitter (USART2) */
  PIOD_IRQn                 = 16 , /**< 16  SAME70N20 Parallel Input/Output Controller (PIOD) */
  HSMCI_IRQn                = 18 , /**< 18  SAME70N20 High Speed MultiMedia Card Interface (HSMCI) */
  TWIHS0_IRQn               = 19 , /**< 19  SAME70N20 Two-wire Interface High Speed (TWIHS0) */
  TWIHS1_IRQn               = 20 , /**< 20  SAME70N20 Two-wire Interface High Speed (TWIHS1) */
  SPI0_IRQn                 = 21 , /**< 21  SAME70N20 Serial Peripheral Interface (SPI0) */
  SSC_IRQn                  = 22 , /**< 22  SAME70N20 Synchronous Serial Controller (SSC) */
  TC0_IRQn                  = 23 , /**< 23  SAME70N20 Timer Counter (TC0)       */
  TC1_IRQn                  = 24 , /**< 24  SAME70N20 Timer Counter (TC0)       */
  TC2_IRQn                  = 25 , /**< 25  SAME70N20 Timer Counter (TC0)       */
  AFEC0_IRQn                = 29 , /**< 29  SAME70N20 Analog Front-End Controller (AFEC0) */
  DACC_IRQn                 = 30 , /**< 30  SAME70N20 Digital-to-Analog Converter Controller (DACC) */
  PWM0_IRQn                 = 31 , /**< 31  SAME70N20 Pulse Width Modulation Controller (PWM0) */
  ICM_IRQn                  = 32 , /**< 32  SAME70N20 Integrity Check Monitor (ICM) */
  ACC_IRQn                  = 33 , /**< 33  SAME70N20 Analog Comparator Controller (ACC) */
  USBHS_IRQn                = 34 , /**< 34  SAME70N20 USB High-Speed Interface (USBHS) */
  MCAN0_IRQn                = 35 , /**< 35  SAME70N20 Controller Area Network (MCAN0) */
  MCAN1_IRQn                = 37 , /**< 37  SAME70N20 Controller Area Network (MCAN1) */
  GMAC_IRQn                 = 39 , /**< 39  SAME70N20 Gigabit Ethernet MAC (GMAC) */
  AFEC1_IRQn                = 40 , /**< 40  SAME70N20 Analog Front-End Controller (AFEC1) */
  TWIHS2_IRQn               = 41 , /**< 41  SAME70N20 Two-wire Interface High Speed (TWIHS2) */
  SPI1_IRQn                 = 42 , /**< 42  SAME70N20 Serial Peripheral Interface (SPI1) */
  QSPI_IRQn                 = 43 , /**< 43  SAME70N20 Quad Serial Peripheral Interface (QSPI) */
  UART2_IRQn                = 44 , /**< 44  SAME70N20 Universal Asynchronous Receiver Transmitter (UART2) */
  UART3_IRQn                = 45 , /**< 45  SAME70N20 Universal Asynchronous Receiver Transmitter (UART3) */
  UART4_IRQn                = 46 , /**< 46  SAME70N20 Universal Asynchronous Receiver Transmitter (UART4) */
  TC9_IRQn                  = 50 , /**< 50  SAME70N20 Timer Counter (TC3)       */
  TC10_IRQn                 = 51 , /**< 51  SAME70N20 Timer Counter (TC3)       */
  TC11_IRQn                 = 52 , /**< 52  SAME70N20 Timer Counter (TC3)       */
  AES_IRQn                  = 56 , /**< 56  SAME70N20 Advanced Encryption Standard (AES) */
  TRNG_IRQn                 = 57 , /**< 57  SAME70N20 True Random Number Generator (TRNG) */
  XDMAC_IRQn                = 58 , /**< 58  SAME70N20 Extensible DMA Controller (XDMAC) */
  ISI_IRQn                  = 59 , /**< 59  SAME70N20 Image Sensor Interface (ISI) */
  PWM1_IRQn                 = 60 , /**< 60  SAME70N20 Pulse Width Modulation Controller (PWM1) */
  RSWDT_IRQn                = 63 , /**< 63  SAME70N20 Reinforced Safety Watchdog Timer (RSWDT) */

  PERIPH_COUNT_IRQn        = 64  /**< Number of peripheral IDs */
} IRQn_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct _DeviceVectors
{
  /* Stack pointer */
  void* pvStack;
  /* Cortex-M handlers */
  void* pfnReset_Handler;                        /* 0   Reset handler                        */
  void* pfnNMI_Handler;                          /* 2   Non Maskable Interrupt               */
  void* pfnHardFault_Handler;                    /* 3   HardFault Interrupt                  */
  void* pfnMemManage_Handler;                    /* 4   Cortex-M7 Memory Management Interrupt  */
  void* pfnBusFault_Handler;                     /* 5   Cortex-M7 Bus Fault Interrupt        */
  void* pfnUsageFault_Handler;                   /* 6   Cortex-M7 Usage Fault Interrupt      */
  void* pfnReserved1_Handler;
  void* pfnReserved2_Handler;
  void* pfnReserved3_Handler;
  void* pfnReserved4_Handler;
  void* pfnSVC_Handler;                          /* 11  Cortex-M7 SV Call Interrupt          */
  void* pfnDebugMon_Handler;                     /* 12  Cortex-M7 Debug Monitor Interrupt    */
  void* pfnReserved5_Handler;
  void* pfnPendSV_Handler;                       /* 14  Cortex-M7 Pend SV Interrupt          */
  void* pfnSysTick_Handler;                      /* 15  Cortex-M7 System Tick Interrupt      */

  /* Peripheral handlers */
  void* pfnSUPC_Handler;                         /* 0   SAME70N20 Supply Controller (SUPC) */
  void* pfnRSTC_Handler;                         /* 1   SAME70N20 Reset Controller (RSTC) */
  void* pfnRTC_Handler;                          /* 2   SAME70N20 Real-time Clock (RTC) */
  void* pfnRTT_Handler;                          /* 3   SAME70N20 Real-time Timer (RTT) */
  void* pfnWDT_Handler;                          /* 4   SAME70N20 Watchdog Timer (WDT) */
  void* pfnPMC_Handler;                          /* 5   SAME70N20 Power Management Controller (PMC) */
  void* pfnEFC_Handler;                          /* 6   SAME70N20 Embedded Flash Controller (EFC) */
  void* pfnUART0_Handler;                        /* 7   SAME70N20 Universal Asynchronous Receiver Transmitter (UART0) */
  void* pfnUART1_Handler;                        /* 8   SAME70N20 Universal Asynchronous Receiver Transmitter (UART1) */
  void* pvReserved9;
  void* pfnPIOA_Handler;                         /* 10  SAME70N20 Parallel Input/Output Controller (PIOA) */
  void* pfnPIOB_Handler;                         /* 11  SAME70N20 Parallel Input/Output Controller (PIOB) */
  void* pvReserved12;
  void* pfnUSART0_Handler;                       /* 13  SAME70N20 Universal Synchronous Asynchronous Receiver Transmitter (USART0) */
  void* pfnUSART1_Handler;                       /* 14  SAME70N20 Universal Synchronous Asynchronous Receiver Transmitter (USART1) */
  void* pfnUSART2_Handler;                       /* 15  SAME70N20 Universal Synchronous Asynchronous Receiver Transmitter (USART2) */
  void* pfnPIOD_Handler;                         /* 16  SAME70N20 Parallel Input/Output Controller (PIOD) */
  void* pvReserved17;
  void* pfnHSMCI_Handler;                        /* 18  SAME70N20 High Speed MultiMedia Card Interface (HSMCI) */
  void* pfnTWIHS0_Handler;                       /* 19  SAME70N20 Two-wire Interface High Speed (TWIHS0) */
  void* pfnTWIHS1_Handler;                       /* 20  SAME70N20 Two-wire Interface High Speed (TWIHS1) */
  void* pfnSPI0_Handler;                         /* 21  SAME70N20 Serial Peripheral Interface (SPI0) */
  void* pfnSSC_Handler;                          /* 22  SAME70N20 Synchronous Serial Controller (SSC) */
  void* pfnTC0_Handler;                          /* 23  SAME70N20 Timer Counter (TC0)  */
  void* pfnTC1_Handler;                          /* 24  SAME70N20 Timer Counter (TC0)  */
  void* pfnTC2_Handler;                          /* 25  SAME70N20 Timer Counter (TC0)  */
  void* pvReserved26;
  void* pvReserved27;
  void* pvReserved28;
  void* pfnAFEC0_Handler;                        /* 29  SAME70N20 Analog Front-End Controller (AFEC0) */
  void* pfnDACC_Handler;                         /* 30  SAME70N20 Digital-to-Analog Converter Controller (DACC) */
  void* pfnPWM0_Handler;                         /* 31  SAME70N20 Pulse Width Modulation Controller (PWM0) */
  void* pfnICM_Handler;                          /* 32  SAME70N20 Integrity Check Monitor (ICM) */
  void* pfnACC_Handler;                          /* 33  SAME70N20 Analog Comparator Controller (ACC) */
  void* pfnUSBHS_Handler;                        /* 34  SAME70N20 USB High-Speed Interface (USBHS) */
  void* pfnMCAN0_Handler;                        /* 35  SAME70N20 Controller Area Network (MCAN0) */
  void* pvReserved36;
  void* pfnMCAN1_Handler;                        /* 37  SAME70N20 Controller Area Network (MCAN1) */
  void* pvReserved38;
  void* pfnGMAC_Handler;                         /* 39  SAME70N20 Gigabit Ethernet MAC (GMAC) */
  void* pfnAFEC1_Handler;                        /* 40  SAME70N20 Analog Front-End Controller (AFEC1) */
  void* pfnTWIHS2_Handler;                       /* 41  SAME70N20 Two-wire Interface High Speed (TWIHS2) */
  void* pfnSPI1_Handler;                         /* 42  SAME70N20 Serial Peripheral Interface (SPI1) */
  void* pfnQSPI_Handler;                         /* 43  SAME70N20 Quad Serial Peripheral Interface (QSPI) */
  void* pfnUART2_Handler;                        /* 44  SAME70N20 Universal Asynchronous Receiver Transmitter (UART2) */
  void* pfnUART3_Handler;                        /* 45  SAME70N20 Universal Asynchronous Receiver Transmitter (UART3) */
  void* pfnUART4_Handler;                        /* 46  SAME70N20 Universal Asynchronous Receiver Transmitter (UART4) */
  void* pvReserved47;
  void* pvReserved48;
  void* pvReserved49;
  void* pfnTC9_Handler;                          /* 50  SAME70N20 Timer Counter (TC3)  */
  void* pfnTC10_Handler;                         /* 51  SAME70N20 Timer Counter (TC3)  */
  void* pfnTC11_Handler;                         /* 52  SAME70N20 Timer Counter (TC3)  */
  void* pvReserved53;
  void* pvReserved54;
  void* pvReserved55;
  void* pfnAES_Handler;                          /* 56  SAME70N20 Advanced Encryption Standard (AES) */
  void* pfnTRNG_Handler;                         /* 57  SAME70N20 True Random Number Generator (TRNG) */
  void* pfnXDMAC_Handler;                        /* 58  SAME70N20 Extensible DMA Controller (XDMAC) */
  void* pfnISI_Handler;                          /* 59  SAME70N20 Image Sensor Interface (ISI) */
  void* pfnPWM1_Handler;                         /* 60  SAME70N20 Pulse Width Modulation Controller (PWM1) */
  void* pvReserved61;
  void* pvReserved62;
  void* pfnRSWDT_Handler;                        /* 63  SAME70N20 Reinforced Safety Watchdog Timer (RSWDT) */
} DeviceVectors;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if !defined DONT_USE_PREDEFINED_CORE_HANDLERS

/* CORTEX-M7 core handlers */
void Reset_Handler        ( void );
void NMI_Handler          ( void );
void HardFault_Handler    ( void );
void MemManage_Handler    ( void );
void BusFault_Handler     ( void );
void UsageFault_Handler   ( void );
void SVC_Handler          ( void );
void DebugMon_Handler     ( void );
void PendSV_Handler       ( void );
void SysTick_Handler      ( void );
#endif /* DONT_USE_PREDEFINED_CORE_HANDLERS */

#if !defined DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

/* Peripherals handlers */
void ACC_Handler              ( void );
void AES_Handler              ( void );
void AFEC0_Handler            ( void );
void AFEC1_Handler            ( void );
void DACC_Handler             ( void );
void EFC_Handler              ( void );
void GMAC_Handler             ( void );
void HSMCI_Handler            ( void );
void ICM_Handler              ( void );
void ISI_Handler              ( void );
void MCAN0_Handler            ( void );
void MCAN1_Handler            ( void );
void PIOA_Handler             ( void );
void PIOB_Handler             ( void );
void PIOD_Handler             ( void );
void PMC_Handler              ( void );
void PWM0_Handler             ( void );
void PWM1_Handler             ( void );
void QSPI_Handler             ( void );
void RSTC_Handler             ( void );
void RSWDT_Handler            ( void );
void RTC_Handler              ( void );
void RTT_Handler              ( void );
void SPI0_Handler             ( void );
void SPI1_Handler             ( void );
void SSC_Handler              ( void );
void SUPC_Handler             ( void );
void TC0_Handler              ( void );
void TC10_Handler             ( void );
void TC11_Handler             ( void );
void TC1_Handler              ( void );
void TC2_Handler              ( void );
void TC9_Handler              ( void );
void TRNG_Handler             ( void );
void TWIHS0_Handler           ( void );
void TWIHS1_Handler           ( void );
void TWIHS2_Handler           ( void );
void UART0_Handler            ( void );
void UART1_Handler            ( void );
void UART2_Handler            ( void );
void UART3_Handler            ( void );
void UART4_Handler            ( void );
void USART0_Handler           ( void );
void USART1_Handler           ( void );
void USART2_Handler           ( void );
void USBHS_Handler            ( void );
void WDT_Handler              ( void );
void XDMAC_Handler            ( void );
#endif /* DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/*
 * \brief Configuration of the CORTEX-M7 Processor and Core Peripherals
 */

#define __DTCM_PRESENT          1
#define __FPU_DP                1
#define __Vendor_SysTickConfig  0
#define __CM7_REV               0
#define __MPU_PRESENT           1
#define NUM_IRQ                 68
#define LITTLE_ENDIAN           1
#define __DCACHE_PRESENT        1
#define __ICACHE_PRESENT        1
#define __ITCM_PRESENT          1
#define __FPU_PRESENT           1
#define __NVIC_PRIO_BITS        3

/*
 * \brief CMSIS includes
 */
#include <core_cm7.h>
#if !defined DONT_USE_CMSIS_INIT
#include "system_same70.h"
#endif /* DONT_USE_CMSIS_INIT */

/** @}  end of SAME70N20_cmsis CMSIS Definitions */

/** \defgroup SAME70N20_api Peripheral Software API
 *  @{
 */

/* ************************************************************************** */
/**  SOFTWARE PERIPHERAL API DEFINITION FOR SAME70N20 */
/* ************************************************************************** */
#include "component/acc.h"
#include "component/aes.h"
#include "component/afec.h"
#include "component/chipid.h"
#include "component/dacc.h"
#include "component/efc.h"
#include "component/gmac.h"
#include "component/gpbr.h"
#include "component/hsmci.h"
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
#include "component/spi.h"
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

/** \defgroup SAME70N20_reg Registers Access Definitions
 *  @{
 */

/* ************************************************************************** */
/*   REGISTER ACCESS DEFINITIONS FOR SAME70N20 */
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
#include "instance/hsmci.h"
#include "instance/icm.h"
#include "instance/isi.h"
#include "instance/matrix.h"
#include "instance/mcan0.h"
#include "instance/mcan1.h"
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
#include "instance/spi0.h"
#include "instance/spi1.h"
#include "instance/ssc.h"
#include "instance/supc.h"
#include "instance/tc0.h"
#include "instance/tc3.h"
#include "instance/trng.h"
#include "instance/twihs0.h"
#include "instance/twihs1.h"
#include "instance/twihs2.h"
#include "instance/uart0.h"
#include "instance/uart1.h"
#include "instance/uart2.h"
#include "instance/uart3.h"
#include "instance/uart4.h"
#include "instance/usart0.h"
#include "instance/usart1.h"
#include "instance/usart2.h"
#include "instance/usbhs.h"
#include "instance/utmi.h"
#include "instance/wdt.h"
#include "instance/xdmac.h"
/** @}  end of Registers Access Definitions */

/** \addtogroup SAME70N20_id Peripheral Ids Definitions
 *  @{
 */

/* ************************************************************************** */
/*  PERIPHERAL ID DEFINITIONS FOR SAME70N20 */
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
#define ID_USART2       ( 15) /**< \brief Universal Synchronous Asynchronous Receiver Transmitter (USART2) */
#define ID_PIOD         ( 16) /**< \brief Parallel Input/Output Controller (PIOD) */
#define ID_HSMCI        ( 18) /**< \brief High Speed MultiMedia Card Interface (HSMCI) */
#define ID_TWIHS0       ( 19) /**< \brief Two-wire Interface High Speed (TWIHS0) */
#define ID_TWIHS1       ( 20) /**< \brief Two-wire Interface High Speed (TWIHS1) */
#define ID_SPI0         ( 21) /**< \brief Serial Peripheral Interface (SPI0) */
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
#define ID_MCAN1        ( 37) /**< \brief Controller Area Network (MCAN1) */
#define ID_GMAC         ( 39) /**< \brief Gigabit Ethernet MAC (GMAC) */
#define ID_AFEC1        ( 40) /**< \brief Analog Front-End Controller (AFEC1) */
#define ID_TWIHS2       ( 41) /**< \brief Two-wire Interface High Speed (TWIHS2) */
#define ID_SPI1         ( 42) /**< \brief Serial Peripheral Interface (SPI1) */
#define ID_QSPI         ( 43) /**< \brief Quad Serial Peripheral Interface (QSPI) */
#define ID_UART2        ( 44) /**< \brief Universal Asynchronous Receiver Transmitter (UART2) */
#define ID_UART3        ( 45) /**< \brief Universal Asynchronous Receiver Transmitter (UART3) */
#define ID_UART4        ( 46) /**< \brief Universal Asynchronous Receiver Transmitter (UART4) */
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

/** \addtogroup legacy_SAME70N20_id Legacy Peripheral Ids Definitions
 *  @{
 */

/* ************************************************************************** */
/*  LEGACY PERIPHERAL ID DEFINITIONS FOR SAME70N20 */
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

/** \addtogroup SAME70N20_base Peripheral Base Address Definitions
 *  @{
 */

/* ************************************************************************** */
/*   BASE ADDRESS DEFINITIONS FOR SAME70N20 */
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
#define HSMCI                  (0x40000000)                   /**< \brief (HSMCI     ) Base Address */
#define ICM                    (0x40048000)                   /**< \brief (ICM       ) Base Address */
#define ISI                    (0x4004C000)                   /**< \brief (ISI       ) Base Address */
#define MATRIX                 (0x40088000)                   /**< \brief (MATRIX    ) Base Address */
#define MCAN0                  (0x40030000)                   /**< \brief (MCAN0     ) Base Address */
#define MCAN1                  (0x40034000)                   /**< \brief (MCAN1     ) Base Address */
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
#define SPI0                   (0x40008000)                   /**< \brief (SPI0      ) Base Address */
#define SPI1                   (0x40058000)                   /**< \brief (SPI1      ) Base Address */
#define SSC                    (0x40004000)                   /**< \brief (SSC       ) Base Address */
#define SUPC                   (0x400E1810)                   /**< \brief (SUPC      ) Base Address */
#define TC0                    (0x4000C000)                   /**< \brief (TC0       ) Base Address */
#define TC3                    (0x40054000)                   /**< \brief (TC3       ) Base Address */
#define TRNG                   (0x40070000)                   /**< \brief (TRNG      ) Base Address */
#define TWIHS0                 (0x40018000)                   /**< \brief (TWIHS0    ) Base Address */
#define TWIHS1                 (0x4001C000)                   /**< \brief (TWIHS1    ) Base Address */
#define TWIHS2                 (0x40060000)                   /**< \brief (TWIHS2    ) Base Address */
#define UART0                  (0x400E0800)                   /**< \brief (UART0     ) Base Address */
#define UART1                  (0x400E0A00)                   /**< \brief (UART1     ) Base Address */
#define UART2                  (0x400E1A00)                   /**< \brief (UART2     ) Base Address */
#define UART3                  (0x400E1C00)                   /**< \brief (UART3     ) Base Address */
#define UART4                  (0x400E1E00)                   /**< \brief (UART4     ) Base Address */
#define USART0                 (0x40024000)                   /**< \brief (USART0    ) Base Address */
#define USART1                 (0x40028000)                   /**< \brief (USART1    ) Base Address */
#define USART2                 (0x4002C000)                   /**< \brief (USART2    ) Base Address */
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

#define HSMCI                  ((Hsmci *)0x40000000U)         /**< \brief (HSMCI     ) Base Address */
#define HSMCI_INST_NUM         1                              /**< \brief (HSMCI     ) Number of instances */
#define HSMCI_INSTS            { HSMCI }                      /**< \brief (HSMCI     ) Instances List */

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
#define MCAN1                  ((Mcan *)0x40034000U)          /**< \brief (MCAN1     ) Base Address */
#define MCAN_INST_NUM          2                              /**< \brief (MCAN      ) Number of instances */
#define MCAN_INSTS             { MCAN0, MCAN1 }               /**< \brief (MCAN      ) Instances List */

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

#define SPI0                   ((Spi *)0x40008000U)           /**< \brief (SPI0      ) Base Address */
#define SPI1                   ((Spi *)0x40058000U)           /**< \brief (SPI1      ) Base Address */
#define SPI_INST_NUM           2                              /**< \brief (SPI       ) Number of instances */
#define SPI_INSTS              { SPI0, SPI1 }                 /**< \brief (SPI       ) Instances List */

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
#define TWIHS2                 ((Twihs *)0x40060000U)         /**< \brief (TWIHS2    ) Base Address */
#define TWIHS_INST_NUM         3                              /**< \brief (TWIHS     ) Number of instances */
#define TWIHS_INSTS            { TWIHS0, TWIHS1, TWIHS2 }     /**< \brief (TWIHS     ) Instances List */

#define UART0                  ((Uart *)0x400E0800U)          /**< \brief (UART0     ) Base Address */
#define UART1                  ((Uart *)0x400E0A00U)          /**< \brief (UART1     ) Base Address */
#define UART2                  ((Uart *)0x400E1A00U)          /**< \brief (UART2     ) Base Address */
#define UART3                  ((Uart *)0x400E1C00U)          /**< \brief (UART3     ) Base Address */
#define UART4                  ((Uart *)0x400E1E00U)          /**< \brief (UART4     ) Base Address */
#define UART_INST_NUM          5                              /**< \brief (UART      ) Number of instances */
#define UART_INSTS             { UART0, UART1, UART2, UART3, UART4 } /**< \brief (UART      ) Instances List */

#define USART0                 ((Usart *)0x40024000U)         /**< \brief (USART0    ) Base Address */
#define USART1                 ((Usart *)0x40028000U)         /**< \brief (USART1    ) Base Address */
#define USART2                 ((Usart *)0x4002C000U)         /**< \brief (USART2    ) Base Address */
#define USART_INST_NUM         3                              /**< \brief (USART     ) Number of instances */
#define USART_INSTS            { USART0, USART1, USART2 }     /**< \brief (USART     ) Instances List */

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

/** \addtogroup SAME70N20_pio Peripheral Pio Definitions
 *  @{
 */

/* ************************************************************************** */
/*   PIO DEFINITIONS FOR SAME70N20*/
/* ************************************************************************** */
#include "pio/same70n20.h"
/** @}  end of Peripheral Pio Definitions */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/* ************************************************************************** */
/*   MEMORY MAPPING DEFINITIONS FOR SAME70N20*/
/* ************************************************************************** */

#define PERIPHERALS_SIZE         (0x20000000U)       /* 524288kB Memory segment type: io */
#define SYSTEM_SIZE              (0x10000000U)       /* 262144kB Memory segment type: io */
#define QSPIMEM_SIZE             (0x20000000U)       /* 524288kB Memory segment type: other */
#define AXIMX_SIZE               (0x00100000U)       /* 1024kB Memory segment type: other */
#define ITCM_SIZE                (0x00200000U)       /* 2048kB Memory segment type: other */
#define IFLASH_SIZE              (0x00100000U)       /* 1024kB Memory segment type: flash */
#define IFLASH_PAGE_SIZE         (       512U)
#define IFLASH_NB_OF_PAGES       (      2048U)

#define IROM_SIZE                (0x00004000U)       /*   16kB Memory segment type: rom */
#define DTCM_SIZE                (0x00020000U)       /*  128kB Memory segment type: other */
#define IRAM_SIZE                (0x00060000U)       /*  384kB Memory segment type: ram */
#define EBI_CS0_SIZE             (0x01000000U)       /* 16384kB Memory segment type: other */
#define EBI_CS1_SIZE             (0x01000000U)       /* 16384kB Memory segment type: other */
#define EBI_CS2_SIZE             (0x01000000U)       /* 16384kB Memory segment type: other */
#define EBI_CS3_SIZE             (0x01000000U)       /* 16384kB Memory segment type: other */
#define SDRAM_CS_SIZE            (0x10000000U)       /* 262144kB Memory segment type: other */

#define PERIPHERALS_ADDR         (0x40000000U)       /**< PERIPHERALS base address (type: io)*/
#define SYSTEM_ADDR              (0xe0000000U)       /**< SYSTEM base address (type: io)*/
#define QSPIMEM_ADDR             (0x80000000U)       /**< QSPIMEM base address (type: other)*/
#define AXIMX_ADDR               (0xa0000000U)       /**< AXIMX base address (type: other)*/
#define ITCM_ADDR                (0x00000000U)       /**< ITCM base address (type: other)*/
#define IFLASH_ADDR              (0x00400000U)       /**< IFLASH base address (type: flash)*/
#define IROM_ADDR                (0x00800000U)       /**< IROM base address (type: rom)*/
#define DTCM_ADDR                (0x20000000U)       /**< DTCM base address (type: other)*/
#define IRAM_ADDR                (0x20400000U)       /**< IRAM base address (type: ram)*/
#define EBI_CS0_ADDR             (0x60000000U)       /**< EBI_CS0 base address (type: other)*/
#define EBI_CS1_ADDR             (0x61000000U)       /**< EBI_CS1 base address (type: other)*/
#define EBI_CS2_ADDR             (0x62000000U)       /**< EBI_CS2 base address (type: other)*/
#define EBI_CS3_ADDR             (0x63000000U)       /**< EBI_CS3 base address (type: other)*/
#define SDRAM_CS_ADDR            (0x70000000U)       /**< SDRAM_CS base address (type: other)*/

/* ************************************************************************** */
/**  DEVICE SIGNATURES FOR SAME70N20 */
/* ************************************************************************** */
#define JTAGID                   (0X05B3D03FUL)
#define CHIP_JTAGID              (0X05B3D03FUL)
#define CHIP_CIDR                (0XA1020C00UL)
#define CHIP_EXID                (0X00000001UL)

/* ************************************************************************** */
/**  ELECTRICAL DEFINITIONS FOR SAME70N20 */
/* ************************************************************************** */
#define CHIP_FREQ_SLCK_RC_MIN          (20000UL)       
#define CHIP_FREQ_SLCK_RC              (32000UL)       
#define CHIP_FREQ_SLCK_RC_MAX          (44000UL)       
#define CHIP_FREQ_MAINCK_RC_4MHZ       (4000000UL)     
#define CHIP_FREQ_MAINCK_RC_8MHZ       (8000000UL)     
#define CHIP_FREQ_MAINCK_RC_12MHZ      (12000000UL)    
#define CHIP_FREQ_CPU_MAX              (120000000UL)   
#define CHIP_FREQ_XTAL_32K             (32768UL)       
#define CHIP_FREQ_XTAL_12M             (12000000UL)    
#define CHIP_FREQ_FWS_0                (20000000UL)    /**< \brief Maximum operating frequency when FWS is 0*/
#define CHIP_FREQ_FWS_1                (40000000UL)    /**< \brief Maximum operating frequency when FWS is 1*/
#define CHIP_FREQ_FWS_2                (60000000UL)    /**< \brief Maximum operating frequency when FWS is 2*/
#define CHIP_FREQ_FWS_3                (80000000UL)    /**< \brief Maximum operating frequency when FWS is 3*/
#define CHIP_FREQ_FWS_4                (100000000UL)   /**< \brief Maximum operating frequency when FWS is 4*/
#define CHIP_FREQ_FWS_5                (123000000UL)   /**< \brief Maximum operating frequency when FWS is 5*/

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#ifdef __cplusplus
}
#endif

/** @}  end of SAME70N20 definitions */


#endif /* _SAME70N20_H_ */
