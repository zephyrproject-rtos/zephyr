/**
 * \file
 *
 * \brief Header file for SAMD20E14
 *
 * Copyright (c) 2017 Microchip Technology Inc.
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

#ifndef _SAMD20E14_
#define _SAMD20E14_

/**
 * \ingroup SAMD20_definitions
 * \addtogroup SAMD20E14_definitions SAMD20E14 definitions
 * This file defines all structures and symbols for SAMD20E14:
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
/**  CMSIS DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */
/** \defgroup SAMD20E14_cmsis CMSIS Definitions */
/*@{*/

/** Interrupt Number Definition */
typedef enum IRQn
{
  /******  Cortex-M0+ Processor Exceptions Numbers ******************************/
  NonMaskableInt_IRQn      = -14,/**<  2 Non Maskable Interrupt                 */
  HardFault_IRQn           = -13,/**<  3 Cortex-M0+ Hard Fault Interrupt        */
  SVCall_IRQn              = -5, /**< 11 Cortex-M0+ SV Call Interrupt           */
  PendSV_IRQn              = -2, /**< 14 Cortex-M0+ Pend SV Interrupt           */
  SysTick_IRQn             = -1, /**< 15 Cortex-M0+ System Tick Interrupt       */
  /******  SAMD20E14-specific Interrupt Numbers ***********************/
  PM_IRQn                  =  0, /**<  0 SAMD20E14 Power Manager (PM) */
  SYSCTRL_IRQn             =  1, /**<  1 SAMD20E14 System Control (SYSCTRL) */
  WDT_IRQn                 =  2, /**<  2 SAMD20E14 Watchdog Timer (WDT) */
  RTC_IRQn                 =  3, /**<  3 SAMD20E14 Real-Time Counter (RTC) */
  EIC_IRQn                 =  4, /**<  4 SAMD20E14 External Interrupt Controller (EIC) */
  NVMCTRL_IRQn             =  5, /**<  5 SAMD20E14 Non-Volatile Memory Controller (NVMCTRL) */
  EVSYS_IRQn               =  6, /**<  6 SAMD20E14 Event System Interface (EVSYS) */
  SERCOM0_IRQn             =  7, /**<  7 SAMD20E14 Serial Communication Interface 0 (SERCOM0) */
  SERCOM1_IRQn             =  8, /**<  8 SAMD20E14 Serial Communication Interface 1 (SERCOM1) */
  SERCOM2_IRQn             =  9, /**<  9 SAMD20E14 Serial Communication Interface 2 (SERCOM2) */
  SERCOM3_IRQn             = 10, /**< 10 SAMD20E14 Serial Communication Interface 3 (SERCOM3) */
  TC0_IRQn                 = 13, /**< 13 SAMD20E14 Basic Timer Counter 0 (TC0) */
  TC1_IRQn                 = 14, /**< 14 SAMD20E14 Basic Timer Counter 1 (TC1) */
  TC2_IRQn                 = 15, /**< 15 SAMD20E14 Basic Timer Counter 2 (TC2) */
  TC3_IRQn                 = 16, /**< 16 SAMD20E14 Basic Timer Counter 3 (TC3) */
  TC4_IRQn                 = 17, /**< 17 SAMD20E14 Basic Timer Counter 4 (TC4) */
  TC5_IRQn                 = 18, /**< 18 SAMD20E14 Basic Timer Counter 5 (TC5) */
  ADC_IRQn                 = 21, /**< 21 SAMD20E14 Analog Digital Converter (ADC) */
  AC_IRQn                  = 22, /**< 22 SAMD20E14 Analog Comparators (AC) */
  DAC_IRQn                 = 23, /**< 23 SAMD20E14 Digital Analog Converter (DAC) */
  PTC_IRQn                 = 24, /**< 24 SAMD20E14 Peripheral Touch Controller (PTC) */

  PERIPH_COUNT_IRQn        = 25  /**< Number of peripheral IDs */
} IRQn_Type;

typedef struct _DeviceVectors
{
  /* Stack pointer */
  void* pvStack;

  /* Cortex-M handlers */
  void* pfnReset_Handler;
  void* pfnNMI_Handler;
  void* pfnHardFault_Handler;
  void* pvReservedM12;
  void* pvReservedM11;
  void* pvReservedM10;
  void* pvReservedM9;
  void* pvReservedM8;
  void* pvReservedM7;
  void* pvReservedM6;
  void* pfnSVC_Handler;
  void* pvReservedM4;
  void* pvReservedM3;
  void* pfnPendSV_Handler;
  void* pfnSysTick_Handler;

  /* Peripheral handlers */
  void* pfnPM_Handler;                    /*  0 Power Manager */
  void* pfnSYSCTRL_Handler;               /*  1 System Control */
  void* pfnWDT_Handler;                   /*  2 Watchdog Timer */
  void* pfnRTC_Handler;                   /*  3 Real-Time Counter */
  void* pfnEIC_Handler;                   /*  4 External Interrupt Controller */
  void* pfnNVMCTRL_Handler;               /*  5 Non-Volatile Memory Controller */
  void* pfnEVSYS_Handler;                 /*  6 Event System Interface */
  void* pfnSERCOM0_Handler;               /*  7 Serial Communication Interface 0 */
  void* pfnSERCOM1_Handler;               /*  8 Serial Communication Interface 1 */
  void* pfnSERCOM2_Handler;               /*  9 Serial Communication Interface 2 */
  void* pfnSERCOM3_Handler;               /* 10 Serial Communication Interface 3 */
  void* pvReserved11;
  void* pvReserved12;
  void* pfnTC0_Handler;                   /* 13 Basic Timer Counter 0 */
  void* pfnTC1_Handler;                   /* 14 Basic Timer Counter 1 */
  void* pfnTC2_Handler;                   /* 15 Basic Timer Counter 2 */
  void* pfnTC3_Handler;                   /* 16 Basic Timer Counter 3 */
  void* pfnTC4_Handler;                   /* 17 Basic Timer Counter 4 */
  void* pfnTC5_Handler;                   /* 18 Basic Timer Counter 5 */
  void* pvReserved19;
  void* pvReserved20;
  void* pfnADC_Handler;                   /* 21 Analog Digital Converter */
  void* pfnAC_Handler;                    /* 22 Analog Comparators */
  void* pfnDAC_Handler;                   /* 23 Digital Analog Converter */
  void* pfnPTC_Handler;                   /* 24 Peripheral Touch Controller */
} DeviceVectors;

/* Cortex-M0+ processor handlers */
void Reset_Handler               ( void );
void NMI_Handler                 ( void );
void HardFault_Handler           ( void );
void SVC_Handler                 ( void );
void PendSV_Handler              ( void );
void SysTick_Handler             ( void );

/* Peripherals handlers */
void PM_Handler                  ( void );
void SYSCTRL_Handler             ( void );
void WDT_Handler                 ( void );
void RTC_Handler                 ( void );
void EIC_Handler                 ( void );
void NVMCTRL_Handler             ( void );
void EVSYS_Handler               ( void );
void SERCOM0_Handler             ( void );
void SERCOM1_Handler             ( void );
void SERCOM2_Handler             ( void );
void SERCOM3_Handler             ( void );
void TC0_Handler                 ( void );
void TC1_Handler                 ( void );
void TC2_Handler                 ( void );
void TC3_Handler                 ( void );
void TC4_Handler                 ( void );
void TC5_Handler                 ( void );
void ADC_Handler                 ( void );
void AC_Handler                  ( void );
void DAC_Handler                 ( void );
void PTC_Handler                 ( void );

/*
 * \brief Configuration of the Cortex-M0+ Processor and Core Peripherals
 */

#define LITTLE_ENDIAN          1        
#define __CM0PLUS_REV          1         /*!< Core revision r0p1 */
#define __MPU_PRESENT          0         /*!< MPU present or not */
#define __NVIC_PRIO_BITS       2         /*!< Number of bits used for Priority Levels */
#define __VTOR_PRESENT         1         /*!< VTOR present or not */
#define __Vendor_SysTickConfig 0         /*!< Set to 1 if different SysTick Config is used */

/**
 * \brief CMSIS includes
 */

#include <core_cm0plus.h>
#if !defined DONT_USE_CMSIS_INIT
#include "system_samd20.h"
#endif /* DONT_USE_CMSIS_INIT */

/*@}*/

/* ************************************************************************** */
/**  SOFTWARE PERIPHERAL API DEFINITION FOR SAMD20E14 */
/* ************************************************************************** */
/** \defgroup SAMD20E14_api Peripheral Software API */
/*@{*/

#include "component/ac.h"
#include "component/adc.h"
#include "component/dac.h"
#include "component/dsu.h"
#include "component/eic.h"
#include "component/evsys.h"
#include "component/gclk.h"
#include "component/nvmctrl.h"
#include "component/pac.h"
#include "component/pm.h"
#include "component/port.h"
#include "component/rtc.h"
#include "component/sercom.h"
#include "component/sysctrl.h"
#include "component/tc.h"
#include "component/wdt.h"
/*@}*/

/* ************************************************************************** */
/**  REGISTERS ACCESS DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */
/** \defgroup SAMD20E14_reg Registers Access Definitions */
/*@{*/

#include "instance/ac.h"
#include "instance/adc.h"
#include "instance/dac.h"
#include "instance/dsu.h"
#include "instance/eic.h"
#include "instance/evsys.h"
#include "instance/gclk.h"
#include "instance/nvmctrl.h"
#include "instance/pac0.h"
#include "instance/pac1.h"
#include "instance/pac2.h"
#include "instance/pm.h"
#include "instance/port.h"
#include "instance/rtc.h"
#include "instance/sercom0.h"
#include "instance/sercom1.h"
#include "instance/sercom2.h"
#include "instance/sercom3.h"
#include "instance/sysctrl.h"
#include "instance/tc0.h"
#include "instance/tc1.h"
#include "instance/tc2.h"
#include "instance/tc3.h"
#include "instance/tc4.h"
#include "instance/tc5.h"
#include "instance/wdt.h"
/*@}*/

/* ************************************************************************** */
/**  PERIPHERAL ID DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */
/** \defgroup SAMD20E14_id Peripheral Ids Definitions */
/*@{*/

// Peripheral instances on HPB0 bridge
#define ID_PAC0           0 /**< \brief Peripheral Access Controller 0 (PAC0) */
#define ID_PM             1 /**< \brief Power Manager (PM) */
#define ID_SYSCTRL        2 /**< \brief System Control (SYSCTRL) */
#define ID_GCLK           3 /**< \brief Generic Clock Generator (GCLK) */
#define ID_WDT            4 /**< \brief Watchdog Timer (WDT) */
#define ID_RTC            5 /**< \brief Real-Time Counter (RTC) */
#define ID_EIC            6 /**< \brief External Interrupt Controller (EIC) */

// Peripheral instances on HPB1 bridge
#define ID_PAC1          32 /**< \brief Peripheral Access Controller 1 (PAC1) */
#define ID_DSU           33 /**< \brief Device Service Unit (DSU) */
#define ID_NVMCTRL       34 /**< \brief Non-Volatile Memory Controller (NVMCTRL) */
#define ID_PORT          35 /**< \brief Port Module (PORT) */

// Peripheral instances on HPB2 bridge
#define ID_PAC2          64 /**< \brief Peripheral Access Controller 2 (PAC2) */
#define ID_EVSYS         65 /**< \brief Event System Interface (EVSYS) */
#define ID_SERCOM0       66 /**< \brief Serial Communication Interface 0 (SERCOM0) */
#define ID_SERCOM1       67 /**< \brief Serial Communication Interface 1 (SERCOM1) */
#define ID_SERCOM2       68 /**< \brief Serial Communication Interface 2 (SERCOM2) */
#define ID_SERCOM3       69 /**< \brief Serial Communication Interface 3 (SERCOM3) */
#define ID_TC0           72 /**< \brief Basic Timer Counter 0 (TC0) */
#define ID_TC1           73 /**< \brief Basic Timer Counter 1 (TC1) */
#define ID_TC2           74 /**< \brief Basic Timer Counter 2 (TC2) */
#define ID_TC3           75 /**< \brief Basic Timer Counter 3 (TC3) */
#define ID_TC4           76 /**< \brief Basic Timer Counter 4 (TC4) */
#define ID_TC5           77 /**< \brief Basic Timer Counter 5 (TC5) */
#define ID_ADC           80 /**< \brief Analog Digital Converter (ADC) */
#define ID_AC            81 /**< \brief Analog Comparators (AC) */
#define ID_DAC           82 /**< \brief Digital Analog Converter (DAC) */
#define ID_PTC           83 /**< \brief Peripheral Touch Controller (PTC) */

#define ID_PERIPH_COUNT  84 /**< \brief Max number of peripheral IDs */
/*@}*/

/* ************************************************************************** */
/**  BASE ADDRESS DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */
/** \defgroup SAMD20E14_base Peripheral Base Address Definitions */
/*@{*/

#if defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)
#define AC                            (0x42004400) /**< \brief (AC) APB Base Address */
#define ADC                           (0x42004000) /**< \brief (ADC) APB Base Address */
#define DAC                           (0x42004800) /**< \brief (DAC) APB Base Address */
#define DSU                           (0x41002000) /**< \brief (DSU) APB Base Address */
#define EIC                           (0x40001800) /**< \brief (EIC) APB Base Address */
#define EVSYS                         (0x42000400) /**< \brief (EVSYS) APB Base Address */
#define GCLK                          (0x40000C00) /**< \brief (GCLK) APB Base Address */
#define NVMCTRL                       (0x41004000) /**< \brief (NVMCTRL) APB Base Address */
#define NVMCTRL_CAL                   (0x00800000) /**< \brief (NVMCTRL) CAL Base Address */
#define NVMCTRL_LOCKBIT               (0x00802000) /**< \brief (NVMCTRL) LOCKBIT Base Address */
#define NVMCTRL_OTP1                  (0x00806000) /**< \brief (NVMCTRL) OTP1 Base Address */
#define NVMCTRL_OTP2                  (0x00806008) /**< \brief (NVMCTRL) OTP2 Base Address */
#define NVMCTRL_OTP4                  (0x00806020) /**< \brief (NVMCTRL) OTP4 Base Address */
#define NVMCTRL_TEMP_LOG              (0x00806030) /**< \brief (NVMCTRL) TEMP_LOG Base Address */
#define NVMCTRL_USER                  (0x00804000) /**< \brief (NVMCTRL) USER Base Address */
#define PAC0                          (0x40000000) /**< \brief (PAC0) APB Base Address */
#define PAC1                          (0x41000000) /**< \brief (PAC1) APB Base Address */
#define PAC2                          (0x42000000) /**< \brief (PAC2) APB Base Address */
#define PM                            (0x40000400) /**< \brief (PM) APB Base Address */
#define PORT                          (0x41004400) /**< \brief (PORT) APB Base Address */
#define PORT_IOBUS                    (0x60000000) /**< \brief (PORT) IOBUS Base Address */
#define PTC                           (0x42004C00) /**< \brief (PTC) APB Base Address */
#define RTC                           (0x40001400) /**< \brief (RTC) APB Base Address */
#define SERCOM0                       (0x42000800) /**< \brief (SERCOM0) APB Base Address */
#define SERCOM1                       (0x42000C00) /**< \brief (SERCOM1) APB Base Address */
#define SERCOM2                       (0x42001000) /**< \brief (SERCOM2) APB Base Address */
#define SERCOM3                       (0x42001400) /**< \brief (SERCOM3) APB Base Address */
#define SYSCTRL                       (0x40000800) /**< \brief (SYSCTRL) APB Base Address */
#define TC0                           (0x42002000) /**< \brief (TC0) APB Base Address */
#define TC1                           (0x42002400) /**< \brief (TC1) APB Base Address */
#define TC2                           (0x42002800) /**< \brief (TC2) APB Base Address */
#define TC3                           (0x42002C00) /**< \brief (TC3) APB Base Address */
#define TC4                           (0x42003000) /**< \brief (TC4) APB Base Address */
#define TC5                           (0x42003400) /**< \brief (TC5) APB Base Address */
#define WDT                           (0x40001000) /**< \brief (WDT) APB Base Address */
#else
#define AC                ((Ac       *)0x42004400UL) /**< \brief (AC) APB Base Address */
#define AC_INST_NUM       1                          /**< \brief (AC) Number of instances */
#define AC_INSTS          { AC }                     /**< \brief (AC) Instances List */

#define ADC               ((Adc      *)0x42004000UL) /**< \brief (ADC) APB Base Address */
#define ADC_INST_NUM      1                          /**< \brief (ADC) Number of instances */
#define ADC_INSTS         { ADC }                    /**< \brief (ADC) Instances List */

#define DAC               ((Dac      *)0x42004800UL) /**< \brief (DAC) APB Base Address */
#define DAC_INST_NUM      1                          /**< \brief (DAC) Number of instances */
#define DAC_INSTS         { DAC }                    /**< \brief (DAC) Instances List */

#define DSU               ((Dsu      *)0x41002000UL) /**< \brief (DSU) APB Base Address */
#define DSU_INST_NUM      1                          /**< \brief (DSU) Number of instances */
#define DSU_INSTS         { DSU }                    /**< \brief (DSU) Instances List */

#define EIC               ((Eic      *)0x40001800UL) /**< \brief (EIC) APB Base Address */
#define EIC_INST_NUM      1                          /**< \brief (EIC) Number of instances */
#define EIC_INSTS         { EIC }                    /**< \brief (EIC) Instances List */

#define EVSYS             ((Evsys    *)0x42000400UL) /**< \brief (EVSYS) APB Base Address */
#define EVSYS_INST_NUM    1                          /**< \brief (EVSYS) Number of instances */
#define EVSYS_INSTS       { EVSYS }                  /**< \brief (EVSYS) Instances List */

#define GCLK              ((Gclk     *)0x40000C00UL) /**< \brief (GCLK) APB Base Address */
#define GCLK_INST_NUM     1                          /**< \brief (GCLK) Number of instances */
#define GCLK_INSTS        { GCLK }                   /**< \brief (GCLK) Instances List */

#define NVMCTRL           ((Nvmctrl  *)0x41004000UL) /**< \brief (NVMCTRL) APB Base Address */
#define NVMCTRL_CAL                   (0x00800000UL) /**< \brief (NVMCTRL) CAL Base Address */
#define NVMCTRL_LOCKBIT               (0x00802000UL) /**< \brief (NVMCTRL) LOCKBIT Base Address */
#define NVMCTRL_OTP1                  (0x00806000UL) /**< \brief (NVMCTRL) OTP1 Base Address */
#define NVMCTRL_OTP2                  (0x00806008UL) /**< \brief (NVMCTRL) OTP2 Base Address */
#define NVMCTRL_OTP4                  (0x00806020UL) /**< \brief (NVMCTRL) OTP4 Base Address */
#define NVMCTRL_TEMP_LOG              (0x00806030UL) /**< \brief (NVMCTRL) TEMP_LOG Base Address */
#define NVMCTRL_USER                  (0x00804000UL) /**< \brief (NVMCTRL) USER Base Address */
#define NVMCTRL_INST_NUM  1                          /**< \brief (NVMCTRL) Number of instances */
#define NVMCTRL_INSTS     { NVMCTRL }                /**< \brief (NVMCTRL) Instances List */

#define PAC0              ((Pac      *)0x40000000UL) /**< \brief (PAC0) APB Base Address */
#define PAC1              ((Pac      *)0x41000000UL) /**< \brief (PAC1) APB Base Address */
#define PAC2              ((Pac      *)0x42000000UL) /**< \brief (PAC2) APB Base Address */
#define PAC_INST_NUM      3                          /**< \brief (PAC) Number of instances */
#define PAC_INSTS         { PAC0, PAC1, PAC2 }       /**< \brief (PAC) Instances List */

#define PM                ((Pm       *)0x40000400UL) /**< \brief (PM) APB Base Address */
#define PM_INST_NUM       1                          /**< \brief (PM) Number of instances */
#define PM_INSTS          { PM }                     /**< \brief (PM) Instances List */

#define PORT              ((Port     *)0x41004400UL) /**< \brief (PORT) APB Base Address */
#define PORT_IOBUS        ((Port     *)0x60000000UL) /**< \brief (PORT) IOBUS Base Address */
#define PORT_INST_NUM     1                          /**< \brief (PORT) Number of instances */
#define PORT_INSTS        { PORT }                   /**< \brief (PORT) Instances List */
#define PORT_IOBUS_INST_NUM 1                          /**< \brief (PORT) Number of instances */
#define PORT_IOBUS_INSTS  { PORT_IOBUS }             /**< \brief (PORT) Instances List */

#define PTC               ((void     *)0x42004C00UL) /**< \brief (PTC) APB Base Address */
#define PTC_GCLK_ID       27
#define PTC_INST_NUM      1                          /**< \brief (PTC) Number of instances */
#define PTC_INSTS         { PTC }                    /**< \brief (PTC) Instances List */

#define RTC               ((Rtc      *)0x40001400UL) /**< \brief (RTC) APB Base Address */
#define RTC_INST_NUM      1                          /**< \brief (RTC) Number of instances */
#define RTC_INSTS         { RTC }                    /**< \brief (RTC) Instances List */

#define SERCOM0           ((Sercom   *)0x42000800UL) /**< \brief (SERCOM0) APB Base Address */
#define SERCOM1           ((Sercom   *)0x42000C00UL) /**< \brief (SERCOM1) APB Base Address */
#define SERCOM2           ((Sercom   *)0x42001000UL) /**< \brief (SERCOM2) APB Base Address */
#define SERCOM3           ((Sercom   *)0x42001400UL) /**< \brief (SERCOM3) APB Base Address */
#define SERCOM_INST_NUM   4                          /**< \brief (SERCOM) Number of instances */
#define SERCOM_INSTS      { SERCOM0, SERCOM1, SERCOM2, SERCOM3 } /**< \brief (SERCOM) Instances List */

#define SYSCTRL           ((Sysctrl  *)0x40000800UL) /**< \brief (SYSCTRL) APB Base Address */
#define SYSCTRL_INST_NUM  1                          /**< \brief (SYSCTRL) Number of instances */
#define SYSCTRL_INSTS     { SYSCTRL }                /**< \brief (SYSCTRL) Instances List */

#define TC0               ((Tc       *)0x42002000UL) /**< \brief (TC0) APB Base Address */
#define TC1               ((Tc       *)0x42002400UL) /**< \brief (TC1) APB Base Address */
#define TC2               ((Tc       *)0x42002800UL) /**< \brief (TC2) APB Base Address */
#define TC3               ((Tc       *)0x42002C00UL) /**< \brief (TC3) APB Base Address */
#define TC4               ((Tc       *)0x42003000UL) /**< \brief (TC4) APB Base Address */
#define TC5               ((Tc       *)0x42003400UL) /**< \brief (TC5) APB Base Address */
#define TC_INST_NUM       6                          /**< \brief (TC) Number of instances */
#define TC_INSTS          { TC0, TC1, TC2, TC3, TC4, TC5 } /**< \brief (TC) Instances List */

#define WDT               ((Wdt      *)0x40001000UL) /**< \brief (WDT) APB Base Address */
#define WDT_INST_NUM      1                          /**< \brief (WDT) Number of instances */
#define WDT_INSTS         { WDT }                    /**< \brief (WDT) Instances List */

#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/*@}*/

/* ************************************************************************** */
/**  PORT DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */
/** \defgroup SAMD20E14_port PORT Definitions */
/*@{*/

#include "pio/samd20e14.h"
/*@}*/

/* ************************************************************************** */
/**  MEMORY MAPPING DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */

#define FLASH_SIZE            _UL_(0x00004000) /* 16 kB */
#define FLASH_PAGE_SIZE       64
#define FLASH_NB_OF_PAGES     256
#define FLASH_USER_PAGE_SIZE  64
#define HRAMC0_SIZE           _UL_(0x00000800) /* 2 kB */

#define FLASH_ADDR            _UL_(0x00000000) /**< FLASH base address */
#define FLASH_USER_PAGE_ADDR  _UL_(0x00800000) /**< FLASH_USER_PAGE base address */
#define HRAMC0_ADDR           _UL_(0x20000000) /**< HRAMC0 base address */
#define HPB0_ADDR             _UL_(0x40000000) /**< HPB0 base address */
#define HPB1_ADDR             _UL_(0x41000000) /**< HPB1 base address */
#define HPB2_ADDR             _UL_(0x42000000) /**< HPB2 base address */
#define PPB_ADDR              _UL_(0xE0000000) /**< PPB base address */

#define DSU_DID_RESETVALUE    _UL_(0x1000140E)
#define PORT_GROUPS           1

/* ************************************************************************** */
/**  ELECTRICAL DEFINITIONS FOR SAMD20E14 */
/* ************************************************************************** */


#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* SAMD20E14_H */
