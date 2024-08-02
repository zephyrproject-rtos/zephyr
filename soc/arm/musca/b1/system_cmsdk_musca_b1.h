/*
 * Copyright (c) 2017-2019 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is derivative of CMSIS V5.01 Device\_Template_Vendor\Vendor\Device\Include\Device.h
 */

#ifndef SYSTEM_CMSDK_MUSCA_B1_H
#define SYSTEM_CMSDK_MUSCA_B1_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================================================================== */
/* ================                                Interrupt Number Definition                                ================ */
/* =========================================================================================================================== */

typedef enum IRQn {
/* ===========================================  Core Specific Interrupt Numbers  ============================================= */
  NonMaskableInt_IRQn       = -14,              /*  -14 Non Maskable Interrupt                                                 */
  HardFault_IRQn            = -13,              /*  -13 HardFault Interrupt                                                    */
  MemoryManagement_IRQn     = -12,              /*  -12 Memory Management Interrupt                                            */
  BusFault_IRQn             = -11,              /*  -11 Bus Fault Interrupt                                                    */
  UsageFault_IRQn           = -10,              /*  -10 Usage Fault Interrupt                                                  */
  SecureFault_IRQn          =  -9,              /*  -9 Secure Fault Interrupt                                                  */
  SVCall_IRQn               =  -5,              /*  -5 SV Call Interrupt                                                       */
  DebugMonitor_IRQn         =  -4,              /*  -4 Debug Monitor Interrupt                                                 */
  PendSV_IRQn               =  -2,              /*  -2 Pend SV Interrupt                                                       */
  SysTick_IRQn              =  -1,              /*  -1 System Tick Interrupt                                                   */

/* ========================================  Musca Specific SSE-200 Interrupt Numbers  ====================================== */
  NS_WATCHDOG_RESET_IRQn    = 0,                 /*   Non-Secure Watchdog Reset Request Interrupt                              */
  NS_WATCHDOG_IRQn          = 1,                 /*   Non-Secure Watchdog Interrupt                                            */
  S32K_TIMER_IRQn           = 2,                 /*   S32K Timer Interrupt                                                     */
  TIMER0_IRQn               = 3,                 /*   CMSDK Timer 0 Interrupt                                                  */
  TIMER1_IRQn               = 4,                 /*   CMSDK Timer 1 Interrupt                                                  */
  DUALTIMER_IRQn            = 5,                 /*   CMSDK Dual Timer Interrupt                                               */
  MHU0_IRQn                 = 6,                 /*   Message Handling Unit 0 Interrupt                                        */
  MHU1_IRQn                 = 7,                 /*   Message Handling Unit 1 Interrupt                                        */
  CRYPTOCELL_IRQn           = 8,                 /*   CryptoCell-312 Interrupt                                                 */
  S_MPC_COMBINED_IRQn       = 9,                 /*   Secure Combined MPC Interrupt                                            */
  S_PPC_COMBINED_IRQn       = 10,                /*   Secure Combined PPC Interrupt                                            */
  S_MSC_COMBINED_IRQn       = 11,                /*   Secure Combined MSC Interrupt                                            */
  S_BRIDGE_ERR_IRQn         = 12,                /*   Secure Bridge Error Combined Interrupt                                   */
  I_CACHE_INV_ERR_IRQn      = 13,                /*   Instruction Cache Invalidation Interrupt                                 */
  /* Reserved               = 14,                     Reserved                                                                 */
  SYS_PPU_IRQn              = 15,                /*   System PPU Interrupt                                                     */
  CPU0_PPU_IRQn             = 16,                /*   CPU0 PPU Interrupt                                                       */
  CPU1_PPU_IRQn             = 17,                /*   CPU1 PPU Interrupt                                                       */
  CPU0_DGB_PPU_IRQn         = 18,                /*   CPU0 Debug PPU Interrupt                                                 */
  CPU1_DGB_PPU_IRQn         = 19,                /*   CPU1 Debug PPU Interrupt                                                 */
  CRYPTOCELL_PPU_IRQn       = 20,                /*   CryptoCell PPU Interrupt                                                 */
  /* Reserved               = 21,                     Reserved                                                                 */
  RAM0_PPU_IRQn             = 22,                /*   RAM 0 PPU Interrupt                                                      */
  RAM1_PPU_IRQn             = 23,                /*   RAM 1 PPU Interrupt                                                      */
  RAM2_PPU_IRQn             = 24,                /*   RAM 2 PPU Interrupt                                                      */
  RAM3_PPU_IRQn             = 25,                /*   RAM 3 PPU Interrupt                                                      */
  DEBUG_PPU_IRQn            = 26,                /*   Debug PPU Interrupt                                                      */
  /* Reserved               = 27,                     Reserved                                                                 */
  CPU0_CTI_IRQn             = 28,                /*   CPU0 CTI Interrupt                                                       */
  CPU1_CTI_IRQn             = 29,                /*   CPU1 CTI Interrupt                                                       */
  /* Reserved               = 30,                     Reserved                                                                 */
  /* Reserved               = 31,                     Reserved                                                                 */
/* ==========================================  Musca Specific Expansion Interrupt Numbers  =================================== */
  /* None                   = 32,                     Not used. Tied to 0                                                      */
  GpTimer_IRQn              = 33,                /*   General Purpose Timer Interrupt                                          */
  I2C0_IRQn                 = 34,                /*   I2C0 Interrupt                                                           */
  I2C1_IRQn                 = 35,                /*   I2C1 Interrupt                                                           */
  I2S_IRQn                  = 36,                /*   I2S Interrupt                                                            */
  SPI_IRQn                  = 37,                /*   SPI Interrupt                                                            */
  QSPI_IRQn                 = 38,                /*   QSPI Interrupt                                                           */
  UART0_Rx_IRQn             = 39,                /*   UART0 receive FIFO interrupt                                             */
  UART0_Tx_IRQn             = 40,                /*   UART0 transmit FIFO interrupt                                            */
  UART0_RxTimeout_IRQn      = 41,                /*   UART0 receive timeout interrupt                                          */
  UART0_ModemStatus_IRQn    = 42,                /*   UART0 modem status interrupt                                             */
  UART0_Error_IRQn          = 43,                /*   UART0 error interrupt                                                    */
  UART0_IRQn                = 44,                /*   UART0 interrupt                                                          */
  UART1_Rx_IRQn             = 45,                /*   UART1 receive FIFO interrupt                                             */
  UART1_Tx_IRQn             = 46,                /*   UART1 transmit FIFO interrupt                                            */
  UART1_RxTimeout_IRQn      = 47,                /*   UART1 receive timeout interrupt                                          */
  UART1_ModemStatus_IRQn    = 48,                /*   UART1 modem status interrupt                                             */
  UART1_Error_IRQn          = 49,                /*   UART1 error interrupt                                                    */
  UART1_IRQn                = 50,                /*   UART1 interrupt                                                          */
  GPIO_0_IRQn               = 51,                /*   GPIO 0 interrupt                                                         */
  GPIO_1_IRQn               = 52,                /*   GPIO 1 interrupt                                                         */
  GPIO_2_IRQn               = 53,                /*   GPIO 2 interrupt                                                         */
  GPIO_3_IRQn               = 54,                /*   GPIO 3 interrupt                                                         */
  GPIO_4_IRQn               = 55,                /*   GPIO 4 interrupt                                                         */
  GPIO_5_IRQn               = 56,                /*   GPIO 5 interrupt                                                         */
  GPIO_6_IRQn               = 57,                /*   GPIO 6 interrupt                                                         */
  GPIO_7_IRQn               = 58,                /*   GPIO 7 interrupt                                                         */
  GPIO_8_IRQn               = 59,                /*   GPIO 8 interrupt                                                         */
  GPIO_9_IRQn               = 60,                /*   GPIO 9 interrupt                                                         */
  GPIO_10_IRQn              = 61,                /*   GPIO 10 interrupt                                                        */
  GPIO_11_IRQn              = 62,                /*   GPIO 11 interrupt                                                        */
  GPIO_12_IRQn              = 63,                /*   GPIO 12 interrupt                                                        */
  GPIO_13_IRQn              = 64,                /*   GPIO 13 interrupt                                                        */
  GPIO_14_IRQn              = 65,                /*   GPIO 14 interrupt                                                        */
  GPIO_15_IRQn              = 66,                /*   GPIO 15 interrupt                                                        */
  Combined_IRQn             = 67,                /*   Combined interrupt                                                       */
  PVT_IRQn                  = 68,                /*   PVT sensor interrupt                                                     */
  /* Reserved               = 69,                     Reserved                                                                 */
  PWM_0_IRQn                = 70,                /*   PWM0 interrupt                                                           */
  RTC_IRQn                  = 71,                /*   RTC interrupt                                                            */
  GpTimer0_IRQn             = 72,                /*   General Purpose Timer0 Interrupt                                         */
  GpTimer1_IRQn             = 73,                /*   General Purpose Timer1 Interrupt                                         */
  PWM_1_IRQn                = 74,                /*   PWM1 interrupt                                                           */
  PWM_2_IRQn                = 75,                /*   PWM2 interrupt                                                           */
  IOMUX_IRQn                = 76,                /*   IOMUX interrupt                                                          */
} IRQn_Type;



/* =========================================================================================================================== */
/* ================                           Processor and Core Peripheral Section                           ================ */
/* =========================================================================================================================== */

/* --------  Configuration of the Cortex-M33 Processor and Core Peripherals  ------ */
#define __CM33_REV                0x0000U   /* Core revision r0p1 */
#define __SAUREGION_PRESENT       1U        /* SAU regions present */
#define __MPU_PRESENT             1U        /* MPU present */
#define __VTOR_PRESENT            1U        /* VTOR present */
#define __NVIC_PRIO_BITS          3U        /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0U        /* Set to 1 if different SysTick Config is used */

/* CORE 0 doesn't have FPU or DSP */
#define __FPU_PRESENT             0U        /* no FPU present */
#define __DSP_PRESENT             0U        /* no DSP extension present */

#ifdef __cplusplus
}
#endif

#include <core_cm33.h>                           /*!< ARM Cortex-M33 processor and core peripherals */

#endif  /* SYSTEM_CMSDK_MUSCA_B1_H */
