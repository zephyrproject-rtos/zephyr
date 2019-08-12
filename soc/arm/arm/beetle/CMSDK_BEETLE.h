/*
 * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is derivative of CMSIS V5.00 ARMCM3.h
 */


#ifndef CMSDK_BEETLE_H
#define CMSDK_BEETLE_H

#ifdef __cplusplus
 extern "C" {
#endif


/* -------------------------  Interrupt Number Definition  ------------------------ */

typedef enum IRQn
{
/* -------------------  Cortex-M3 Processor Exceptions Numbers  ------------------- */
  NonMaskableInt_IRQn           = -14,        /*  2 Non Maskable Interrupt          */
  HardFault_IRQn                = -13,        /*  3 HardFault Interrupt             */
  MemoryManagement_IRQn         = -12,        /*  4 Memory Management Interrupt     */
  BusFault_IRQn                 = -11,        /*  5 Bus Fault Interrupt             */
  UsageFault_IRQn               = -10,        /*  6 Usage Fault Interrupt           */
  SVCall_IRQn                   =  -5,        /* 11 SV Call Interrupt               */
  DebugMonitor_IRQn             =  -4,        /* 12 Debug Monitor Interrupt         */
  PendSV_IRQn                   =  -2,        /* 14 Pend SV Interrupt               */
  SysTick_IRQn                  =  -1,        /* 15 System Tick Interrupt           */

/* ---------------------  CMSDK_BEETLE Specific Interrupt Numbers  ---------------- */
  UART0_IRQn                    = 0,       /* UART 0 RX and TX Combined Interrupt   */
  Spare_IRQn                    = 1,       /* Undefined                             */
  UART1_IRQn                    = 2,       /* UART 1 RX and TX Combined Interrupt   */
  I2C0_IRQn                     = 3,       /* I2C 0 Interrupt                       */
  I2C1_IRQn                     = 4,       /* I2C 1 Interrupt                       */
  RTC_IRQn                      = 5,       /* RTC Interrupt                         */
  PORT0_ALL_IRQn                = 6,       /* GPIO Port 0 combined Interrupt        */
  PORT1_ALL_IRQn                = 7,       /* GPIO Port 1 combined Interrupt        */
  TIMER0_IRQn                   = 8,       /* TIMER 0 Interrupt                     */
  TIMER1_IRQn                   = 9,       /* TIMER 1 Interrupt                     */
  DUALTIMER_IRQn                = 10,      /* Dual Timer Interrupt                  */
  SPI0_IRQn                     = 11,      /* SPI 0 Interrupt                       */
  UARTOVF_IRQn                  = 12,      /* UART 0,1,2 Overflow Interrupt         */
  SPI1_IRQn                     = 13,      /* SPI 1 Interrupt                       */
  QSPI_IRQn                     = 14,      /* QUAD SPI Interrupt                    */
  DMA_IRQn                      = 15,      /* Reserved for DMA Interrup		    */
  PORT0_0_IRQn                  = 16,      /* All P0 I/O pins used as irq source    */
  PORT0_1_IRQn                  = 17,      /* There are 16 pins in total            */
  PORT0_2_IRQn                  = 18,
  PORT0_3_IRQn                  = 19,
  PORT0_4_IRQn                  = 20,
  PORT0_5_IRQn                  = 21,
  PORT0_6_IRQn                  = 22,
  PORT0_7_IRQn                  = 23,
  PORT0_8_IRQn                  = 24,
  PORT0_9_IRQn                  = 25,
  PORT0_10_IRQn                 = 26,
  PORT0_11_IRQn                 = 27,
  PORT0_12_IRQn                 = 28,
  PORT0_13_IRQn                 = 29,
  PORT0_14_IRQn                 = 30,
  PORT0_15_IRQn                 = 31,
  SYSERROR_IRQn                 = 32,      /* System Error Interrupt                */
  EFLASH_IRQn                   = 33,      /* Embedded Flash Interrupt              */
  LLCC_TXCMD_EMPTY_IRQn         = 34,      /* t.b.a                                 */
  LLCC_TXEVT_EMPTY_IRQn         = 35,      /* t.b.a                                 */
  LLCC_TXDMAH_DONE_IRQn         = 36,      /* t.b.a                                 */
  LLCC_TXDMAL_DONE_IRQn         = 37,      /* t.b.a                                 */
  LLCC_RXCMD_VALID_IRQn         = 38,      /* t.b.a                                 */
  LLCC_RXEVT_VALID_IRQn         = 39,      /* t.b.a                                 */
  LLCC_RXDMAH_DONE_IRQn         = 40,      /* t.b.a                                 */
  LLCC_RXDMAL_DONE_IRQn         = 41,      /* t.b.a                                 */
  PORT2_ALL_IRQn                = 42,      /* GPIO Port 2 combined Interrupt        */
  PORT3_ALL_IRQn                = 43,      /* GPIO Port 3 combined Interrupt        */
  TRNG_IRQn                     = 44,      /* Random number generator Interrupt     */
} IRQn_Type;


/* ================================================================================ */
/* ================      Processor and Core Peripheral Section     ================ */
/* ================================================================================ */

/* --------  Configuration of the Cortex-M3 Processor and Core Peripherals  ------- */
#define __CM3_REV                 0x0201U   /* Core revision r2p1 */
#define __MPU_PRESENT             1         /* MPU present */
#define __VTOR_PRESENT            1         /* VTOR present or not */
#define __NVIC_PRIO_BITS          3         /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0         /* Set to 1 if different SysTick Config is used */

#ifdef __cplusplus
}
#endif

#include <core_cm3.h>                         /* Processor and core peripherals                  */

#endif  /* CMSDK_BEETLE_H */
