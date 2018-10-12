/***************************************************************************//**
* \file psoc6a2m_124bga.h
*
* \brief
* PSoC6A2M_124BGA device header
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _PSOC6A2M_124BGA_H_
#define _PSOC6A2M_124BGA_H_

/**
* \addtogroup group_device PSoC6A2M_124BGA
* \{
*/

/**
* \addtogroup Configuration_of_CMSIS
* \{
*/

/*******************************************************************************
*                         Interrupt Number Definition
*******************************************************************************/

typedef enum {
#if ((defined(__GNUC__)        && (__ARM_ARCH == 6) && (__ARM_ARCH_6M__ == 1)) || \
     (defined(__ICCARM__)      && (__CORE__ == __ARM6M__)) || \
     (defined(__ARMCC_VERSION) && (__TARGET_ARCH_THUMB == 3)) || \
     (defined(__ghs__)         && defined(__CORE_CORTEXM0PLUS__)))
  /* ARM Cortex-M0+ Core Interrupt Numbers */
  Reset_IRQn                        = -15,      /*!< -15 Reset Vector, invoked on Power up and warm reset */
  NonMaskableInt_IRQn               = -14,      /*!< -14 Non maskable Interrupt, cannot be stopped or preempted */
  HardFault_IRQn                    = -13,      /*!< -13 Hard Fault, all classes of Fault */
  SVCall_IRQn                       =  -5,      /*!<  -5 System Service Call via SVC instruction */
  PendSV_IRQn                       =  -2,      /*!<  -2 Pendable request for system service */
  SysTick_IRQn                      =  -1,      /*!<  -1 System Tick Timer */
  /* PSoC6A2M_124BGA User Interrupt Numbers */
  CPUIntIdx0_IRQn                   =   0,      /*!<   0 [DeepSleep] CPU User Interrupt #0 */
  CPUIntIdx1_IRQn                   =   1,      /*!<   1 [DeepSleep] CPU User Interrupt #1 */
  CPUIntIdx2_IRQn                   =   2,      /*!<   2 [DeepSleep] CPU User Interrupt #2 */
  CPUIntIdx3_IRQn                   =   3,      /*!<   3 [DeepSleep] CPU User Interrupt #3 */
  CPUIntIdx4_IRQn                   =   4,      /*!<   4 [DeepSleep] CPU User Interrupt #4 */
  CPUIntIdx5_IRQn                   =   5,      /*!<   5 [DeepSleep] CPU User Interrupt #5 */
  CPUIntIdx6_IRQn                   =   6,      /*!<   6 [DeepSleep] CPU User Interrupt #6 */
  CPUIntIdx7_IRQn                   =   7,      /*!<   7 [DeepSleep] CPU User Interrupt #7 */
  /* PSoC6A2M_124BGA Internal SW Interrupt Numbers */
  Internal0_IRQn                    =   8,      /*!<   8 [Active] Internal SW Interrupt #0 */
  Internal1_IRQn                    =   9,      /*!<   9 [Active] Internal SW Interrupt #1 */
  Internal2_IRQn                    =  10,      /*!<  10 [Active] Internal SW Interrupt #2 */
  Internal3_IRQn                    =  11,      /*!<  11 [Active] Internal SW Interrupt #3 */
  Internal4_IRQn                    =  12,      /*!<  12 [Active] Internal SW Interrupt #4 */
  Internal5_IRQn                    =  13,      /*!<  13 [Active] Internal SW Interrupt #5 */
  Internal6_IRQn                    =  14,      /*!<  14 [Active] Internal SW Interrupt #6 */
  Internal7_IRQn                    =  15,      /*!<  15 [Active] Internal SW Interrupt #7 */
#else
  /* ARM Cortex-M4 Core Interrupt Numbers */
  Reset_IRQn                        = -15,      /*!< -15 Reset Vector, invoked on Power up and warm reset */
  NonMaskableInt_IRQn               = -14,      /*!< -14 Non maskable Interrupt, cannot be stopped or preempted */
  HardFault_IRQn                    = -13,      /*!< -13 Hard Fault, all classes of Fault */
  MemoryManagement_IRQn             = -12,      /*!< -12 Memory Management, MPU mismatch, including Access Violation and No Match */
  BusFault_IRQn                     = -11,      /*!< -11 Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory related Fault */
  UsageFault_IRQn                   = -10,      /*!< -10 Usage Fault, i.e. Undef Instruction, Illegal State Transition */
  SVCall_IRQn                       =  -5,      /*!<  -5 System Service Call via SVC instruction */
  DebugMonitor_IRQn                 =  -4,      /*!<  -4 Debug Monitor */
  PendSV_IRQn                       =  -2,      /*!<  -2 Pendable request for system service */
  SysTick_IRQn                      =  -1,      /*!<  -1 System Tick Timer */
  /* PSoC6A2M_124BGA Peripheral Interrupt Numbers */
  ioss_interrupts_gpio_0_IRQn       =   0,      /*!<   0 [DeepSleep] GPIO Port Interrupt #0 */
  ioss_interrupts_gpio_1_IRQn       =   1,      /*!<   1 [DeepSleep] GPIO Port Interrupt #1 */
  ioss_interrupts_gpio_2_IRQn       =   2,      /*!<   2 [DeepSleep] GPIO Port Interrupt #2 */
  ioss_interrupts_gpio_3_IRQn       =   3,      /*!<   3 [DeepSleep] GPIO Port Interrupt #3 */
  ioss_interrupts_gpio_4_IRQn       =   4,      /*!<   4 [DeepSleep] GPIO Port Interrupt #4 */
  ioss_interrupts_gpio_5_IRQn       =   5,      /*!<   5 [DeepSleep] GPIO Port Interrupt #5 */
  ioss_interrupts_gpio_6_IRQn       =   6,      /*!<   6 [DeepSleep] GPIO Port Interrupt #6 */
  ioss_interrupts_gpio_7_IRQn       =   7,      /*!<   7 [DeepSleep] GPIO Port Interrupt #7 */
  ioss_interrupts_gpio_8_IRQn       =   8,      /*!<   8 [DeepSleep] GPIO Port Interrupt #8 */
  ioss_interrupts_gpio_9_IRQn       =   9,      /*!<   9 [DeepSleep] GPIO Port Interrupt #9 */
  ioss_interrupts_gpio_10_IRQn      =  10,      /*!<  10 [DeepSleep] GPIO Port Interrupt #10 */
  ioss_interrupts_gpio_11_IRQn      =  11,      /*!<  11 [DeepSleep] GPIO Port Interrupt #11 */
  ioss_interrupts_gpio_12_IRQn      =  12,      /*!<  12 [DeepSleep] GPIO Port Interrupt #12 */
  ioss_interrupts_gpio_13_IRQn      =  13,      /*!<  13 [DeepSleep] GPIO Port Interrupt #13 */
  ioss_interrupts_gpio_14_IRQn      =  14,      /*!<  14 [DeepSleep] GPIO Port Interrupt #14 */
  ioss_interrupt_gpio_IRQn          =  15,      /*!<  15 [DeepSleep] GPIO All Ports */
  ioss_interrupt_vdd_IRQn           =  16,      /*!<  16 [DeepSleep] GPIO Supply Detect Interrupt */
  lpcomp_interrupt_IRQn             =  17,      /*!<  17 [DeepSleep] Low Power Comparator Interrupt */
  scb_8_interrupt_IRQn              =  18,      /*!<  18 [DeepSleep] Serial Communication Block #8 (DeepSleep capable) */
  srss_interrupt_mcwdt_0_IRQn       =  19,      /*!<  19 [DeepSleep] Multi Counter Watchdog Timer interrupt */
  srss_interrupt_mcwdt_1_IRQn       =  20,      /*!<  20 [DeepSleep] Multi Counter Watchdog Timer interrupt */
  srss_interrupt_backup_IRQn        =  21,      /*!<  21 [DeepSleep] Backup domain interrupt */
  srss_interrupt_IRQn               =  22,      /*!<  22 [DeepSleep] Other combined Interrupts for SRSS (LVD, WDT, CLKCAL) */
  cpuss_interrupts_ipc_0_IRQn       =  23,      /*!<  23 [DeepSleep] CPUSS Inter Process Communication Interrupt #0 */
  cpuss_interrupts_ipc_1_IRQn       =  24,      /*!<  24 [DeepSleep] CPUSS Inter Process Communication Interrupt #1 */
  cpuss_interrupts_ipc_2_IRQn       =  25,      /*!<  25 [DeepSleep] CPUSS Inter Process Communication Interrupt #2 */
  cpuss_interrupts_ipc_3_IRQn       =  26,      /*!<  26 [DeepSleep] CPUSS Inter Process Communication Interrupt #3 */
  cpuss_interrupts_ipc_4_IRQn       =  27,      /*!<  27 [DeepSleep] CPUSS Inter Process Communication Interrupt #4 */
  cpuss_interrupts_ipc_5_IRQn       =  28,      /*!<  28 [DeepSleep] CPUSS Inter Process Communication Interrupt #5 */
  cpuss_interrupts_ipc_6_IRQn       =  29,      /*!<  29 [DeepSleep] CPUSS Inter Process Communication Interrupt #6 */
  cpuss_interrupts_ipc_7_IRQn       =  30,      /*!<  30 [DeepSleep] CPUSS Inter Process Communication Interrupt #7 */
  cpuss_interrupts_ipc_8_IRQn       =  31,      /*!<  31 [DeepSleep] CPUSS Inter Process Communication Interrupt #8 */
  cpuss_interrupts_ipc_9_IRQn       =  32,      /*!<  32 [DeepSleep] CPUSS Inter Process Communication Interrupt #9 */
  cpuss_interrupts_ipc_10_IRQn      =  33,      /*!<  33 [DeepSleep] CPUSS Inter Process Communication Interrupt #10 */
  cpuss_interrupts_ipc_11_IRQn      =  34,      /*!<  34 [DeepSleep] CPUSS Inter Process Communication Interrupt #11 */
  cpuss_interrupts_ipc_12_IRQn      =  35,      /*!<  35 [DeepSleep] CPUSS Inter Process Communication Interrupt #12 */
  cpuss_interrupts_ipc_13_IRQn      =  36,      /*!<  36 [DeepSleep] CPUSS Inter Process Communication Interrupt #13 */
  cpuss_interrupts_ipc_14_IRQn      =  37,      /*!<  37 [DeepSleep] CPUSS Inter Process Communication Interrupt #14 */
  cpuss_interrupts_ipc_15_IRQn      =  38,      /*!<  38 [DeepSleep] CPUSS Inter Process Communication Interrupt #15 */
  scb_0_interrupt_IRQn              =  39,      /*!<  39 [Active] Serial Communication Block #0 */
  scb_1_interrupt_IRQn              =  40,      /*!<  40 [Active] Serial Communication Block #1 */
  scb_2_interrupt_IRQn              =  41,      /*!<  41 [Active] Serial Communication Block #2 */
  scb_3_interrupt_IRQn              =  42,      /*!<  42 [Active] Serial Communication Block #3 */
  scb_4_interrupt_IRQn              =  43,      /*!<  43 [Active] Serial Communication Block #4 */
  scb_5_interrupt_IRQn              =  44,      /*!<  44 [Active] Serial Communication Block #5 */
  scb_6_interrupt_IRQn              =  45,      /*!<  45 [Active] Serial Communication Block #6 */
  scb_7_interrupt_IRQn              =  46,      /*!<  46 [Active] Serial Communication Block #7 */
  scb_9_interrupt_IRQn              =  47,      /*!<  47 [Active] Serial Communication Block #9 */
  scb_10_interrupt_IRQn             =  48,      /*!<  48 [Active] Serial Communication Block #10 */
  scb_11_interrupt_IRQn             =  49,      /*!<  49 [Active] Serial Communication Block #11 */
  scb_12_interrupt_IRQn             =  50,      /*!<  50 [Active] Serial Communication Block #12 */
  csd_interrupt_IRQn                =  51,      /*!<  51 [Active] CSD (Capsense) interrupt */
  cpuss_interrupts_dmac_0_IRQn      =  52,      /*!<  52 [Active] CPUSS DMAC, Channel #0 */
  cpuss_interrupts_dmac_1_IRQn      =  53,      /*!<  53 [Active] CPUSS DMAC, Channel #1 */
  cpuss_interrupts_dmac_2_IRQn      =  54,      /*!<  54 [Active] CPUSS DMAC, Channel #2 */
  cpuss_interrupts_dmac_3_IRQn      =  55,      /*!<  55 [Active] CPUSS DMAC, Channel #3 */
  cpuss_interrupts_dw0_0_IRQn       =  56,      /*!<  56 [Active] CPUSS DataWire #0, Channel #0 */
  cpuss_interrupts_dw0_1_IRQn       =  57,      /*!<  57 [Active] CPUSS DataWire #0, Channel #1 */
  cpuss_interrupts_dw0_2_IRQn       =  58,      /*!<  58 [Active] CPUSS DataWire #0, Channel #2 */
  cpuss_interrupts_dw0_3_IRQn       =  59,      /*!<  59 [Active] CPUSS DataWire #0, Channel #3 */
  cpuss_interrupts_dw0_4_IRQn       =  60,      /*!<  60 [Active] CPUSS DataWire #0, Channel #4 */
  cpuss_interrupts_dw0_5_IRQn       =  61,      /*!<  61 [Active] CPUSS DataWire #0, Channel #5 */
  cpuss_interrupts_dw0_6_IRQn       =  62,      /*!<  62 [Active] CPUSS DataWire #0, Channel #6 */
  cpuss_interrupts_dw0_7_IRQn       =  63,      /*!<  63 [Active] CPUSS DataWire #0, Channel #7 */
  cpuss_interrupts_dw0_8_IRQn       =  64,      /*!<  64 [Active] CPUSS DataWire #0, Channel #8 */
  cpuss_interrupts_dw0_9_IRQn       =  65,      /*!<  65 [Active] CPUSS DataWire #0, Channel #9 */
  cpuss_interrupts_dw0_10_IRQn      =  66,      /*!<  66 [Active] CPUSS DataWire #0, Channel #10 */
  cpuss_interrupts_dw0_11_IRQn      =  67,      /*!<  67 [Active] CPUSS DataWire #0, Channel #11 */
  cpuss_interrupts_dw0_12_IRQn      =  68,      /*!<  68 [Active] CPUSS DataWire #0, Channel #12 */
  cpuss_interrupts_dw0_13_IRQn      =  69,      /*!<  69 [Active] CPUSS DataWire #0, Channel #13 */
  cpuss_interrupts_dw0_14_IRQn      =  70,      /*!<  70 [Active] CPUSS DataWire #0, Channel #14 */
  cpuss_interrupts_dw0_15_IRQn      =  71,      /*!<  71 [Active] CPUSS DataWire #0, Channel #15 */
  cpuss_interrupts_dw0_16_IRQn      =  72,      /*!<  72 [Active] CPUSS DataWire #0, Channel #16 */
  cpuss_interrupts_dw0_17_IRQn      =  73,      /*!<  73 [Active] CPUSS DataWire #0, Channel #17 */
  cpuss_interrupts_dw0_18_IRQn      =  74,      /*!<  74 [Active] CPUSS DataWire #0, Channel #18 */
  cpuss_interrupts_dw0_19_IRQn      =  75,      /*!<  75 [Active] CPUSS DataWire #0, Channel #19 */
  cpuss_interrupts_dw0_20_IRQn      =  76,      /*!<  76 [Active] CPUSS DataWire #0, Channel #20 */
  cpuss_interrupts_dw0_21_IRQn      =  77,      /*!<  77 [Active] CPUSS DataWire #0, Channel #21 */
  cpuss_interrupts_dw0_22_IRQn      =  78,      /*!<  78 [Active] CPUSS DataWire #0, Channel #22 */
  cpuss_interrupts_dw0_23_IRQn      =  79,      /*!<  79 [Active] CPUSS DataWire #0, Channel #23 */
  cpuss_interrupts_dw0_24_IRQn      =  80,      /*!<  80 [Active] CPUSS DataWire #0, Channel #24 */
  cpuss_interrupts_dw0_25_IRQn      =  81,      /*!<  81 [Active] CPUSS DataWire #0, Channel #25 */
  cpuss_interrupts_dw0_26_IRQn      =  82,      /*!<  82 [Active] CPUSS DataWire #0, Channel #26 */
  cpuss_interrupts_dw0_27_IRQn      =  83,      /*!<  83 [Active] CPUSS DataWire #0, Channel #27 */
  cpuss_interrupts_dw0_28_IRQn      =  84,      /*!<  84 [Active] CPUSS DataWire #0, Channel #28 */
  cpuss_interrupts_dw1_0_IRQn       =  85,      /*!<  85 [Active] CPUSS DataWire #1, Channel #0 */
  cpuss_interrupts_dw1_1_IRQn       =  86,      /*!<  86 [Active] CPUSS DataWire #1, Channel #1 */
  cpuss_interrupts_dw1_2_IRQn       =  87,      /*!<  87 [Active] CPUSS DataWire #1, Channel #2 */
  cpuss_interrupts_dw1_3_IRQn       =  88,      /*!<  88 [Active] CPUSS DataWire #1, Channel #3 */
  cpuss_interrupts_dw1_4_IRQn       =  89,      /*!<  89 [Active] CPUSS DataWire #1, Channel #4 */
  cpuss_interrupts_dw1_5_IRQn       =  90,      /*!<  90 [Active] CPUSS DataWire #1, Channel #5 */
  cpuss_interrupts_dw1_6_IRQn       =  91,      /*!<  91 [Active] CPUSS DataWire #1, Channel #6 */
  cpuss_interrupts_dw1_7_IRQn       =  92,      /*!<  92 [Active] CPUSS DataWire #1, Channel #7 */
  cpuss_interrupts_dw1_8_IRQn       =  93,      /*!<  93 [Active] CPUSS DataWire #1, Channel #8 */
  cpuss_interrupts_dw1_9_IRQn       =  94,      /*!<  94 [Active] CPUSS DataWire #1, Channel #9 */
  cpuss_interrupts_dw1_10_IRQn      =  95,      /*!<  95 [Active] CPUSS DataWire #1, Channel #10 */
  cpuss_interrupts_dw1_11_IRQn      =  96,      /*!<  96 [Active] CPUSS DataWire #1, Channel #11 */
  cpuss_interrupts_dw1_12_IRQn      =  97,      /*!<  97 [Active] CPUSS DataWire #1, Channel #12 */
  cpuss_interrupts_dw1_13_IRQn      =  98,      /*!<  98 [Active] CPUSS DataWire #1, Channel #13 */
  cpuss_interrupts_dw1_14_IRQn      =  99,      /*!<  99 [Active] CPUSS DataWire #1, Channel #14 */
  cpuss_interrupts_dw1_15_IRQn      = 100,      /*!< 100 [Active] CPUSS DataWire #1, Channel #15 */
  cpuss_interrupts_dw1_16_IRQn      = 101,      /*!< 101 [Active] CPUSS DataWire #1, Channel #16 */
  cpuss_interrupts_dw1_17_IRQn      = 102,      /*!< 102 [Active] CPUSS DataWire #1, Channel #17 */
  cpuss_interrupts_dw1_18_IRQn      = 103,      /*!< 103 [Active] CPUSS DataWire #1, Channel #18 */
  cpuss_interrupts_dw1_19_IRQn      = 104,      /*!< 104 [Active] CPUSS DataWire #1, Channel #19 */
  cpuss_interrupts_dw1_20_IRQn      = 105,      /*!< 105 [Active] CPUSS DataWire #1, Channel #20 */
  cpuss_interrupts_dw1_21_IRQn      = 106,      /*!< 106 [Active] CPUSS DataWire #1, Channel #21 */
  cpuss_interrupts_dw1_22_IRQn      = 107,      /*!< 107 [Active] CPUSS DataWire #1, Channel #22 */
  cpuss_interrupts_dw1_23_IRQn      = 108,      /*!< 108 [Active] CPUSS DataWire #1, Channel #23 */
  cpuss_interrupts_dw1_24_IRQn      = 109,      /*!< 109 [Active] CPUSS DataWire #1, Channel #24 */
  cpuss_interrupts_dw1_25_IRQn      = 110,      /*!< 110 [Active] CPUSS DataWire #1, Channel #25 */
  cpuss_interrupts_dw1_26_IRQn      = 111,      /*!< 111 [Active] CPUSS DataWire #1, Channel #26 */
  cpuss_interrupts_dw1_27_IRQn      = 112,      /*!< 112 [Active] CPUSS DataWire #1, Channel #27 */
  cpuss_interrupts_dw1_28_IRQn      = 113,      /*!< 113 [Active] CPUSS DataWire #1, Channel #28 */
  cpuss_interrupts_fault_0_IRQn     = 114,      /*!< 114 [Active] CPUSS Fault Structure Interrupt #0 */
  cpuss_interrupts_fault_1_IRQn     = 115,      /*!< 115 [Active] CPUSS Fault Structure Interrupt #1 */
  cpuss_interrupt_crypto_IRQn       = 116,      /*!< 116 [Active] CRYPTO Accelerator Interrupt */
  cpuss_interrupt_fm_IRQn           = 117,      /*!< 117 [Active] FLASH Macro Interrupt */
  cpuss_interrupts_cm4_fp_IRQn      = 118,      /*!< 118 [Active] Floating Point operation fault */
  cpuss_interrupts_cm0_cti_0_IRQn   = 119,      /*!< 119 [Active] CM0+ CTI #0 */
  cpuss_interrupts_cm0_cti_1_IRQn   = 120,      /*!< 120 [Active] CM0+ CTI #1 */
  cpuss_interrupts_cm4_cti_0_IRQn   = 121,      /*!< 121 [Active] CM4 CTI #0 */
  cpuss_interrupts_cm4_cti_1_IRQn   = 122,      /*!< 122 [Active] CM4 CTI #1 */
  tcpwm_0_interrupts_0_IRQn         = 123,      /*!< 123 [Active] TCPWM #0, Counter #0 */
  tcpwm_0_interrupts_1_IRQn         = 124,      /*!< 124 [Active] TCPWM #0, Counter #1 */
  tcpwm_0_interrupts_2_IRQn         = 125,      /*!< 125 [Active] TCPWM #0, Counter #2 */
  tcpwm_0_interrupts_3_IRQn         = 126,      /*!< 126 [Active] TCPWM #0, Counter #3 */
  tcpwm_0_interrupts_4_IRQn         = 127,      /*!< 127 [Active] TCPWM #0, Counter #4 */
  tcpwm_0_interrupts_5_IRQn         = 128,      /*!< 128 [Active] TCPWM #0, Counter #5 */
  tcpwm_0_interrupts_6_IRQn         = 129,      /*!< 129 [Active] TCPWM #0, Counter #6 */
  tcpwm_0_interrupts_7_IRQn         = 130,      /*!< 130 [Active] TCPWM #0, Counter #7 */
  tcpwm_1_interrupts_0_IRQn         = 131,      /*!< 131 [Active] TCPWM #1, Counter #0 */
  tcpwm_1_interrupts_1_IRQn         = 132,      /*!< 132 [Active] TCPWM #1, Counter #1 */
  tcpwm_1_interrupts_2_IRQn         = 133,      /*!< 133 [Active] TCPWM #1, Counter #2 */
  tcpwm_1_interrupts_3_IRQn         = 134,      /*!< 134 [Active] TCPWM #1, Counter #3 */
  tcpwm_1_interrupts_4_IRQn         = 135,      /*!< 135 [Active] TCPWM #1, Counter #4 */
  tcpwm_1_interrupts_5_IRQn         = 136,      /*!< 136 [Active] TCPWM #1, Counter #5 */
  tcpwm_1_interrupts_6_IRQn         = 137,      /*!< 137 [Active] TCPWM #1, Counter #6 */
  tcpwm_1_interrupts_7_IRQn         = 138,      /*!< 138 [Active] TCPWM #1, Counter #7 */
  tcpwm_1_interrupts_8_IRQn         = 139,      /*!< 139 [Active] TCPWM #1, Counter #8 */
  tcpwm_1_interrupts_9_IRQn         = 140,      /*!< 140 [Active] TCPWM #1, Counter #9 */
  tcpwm_1_interrupts_10_IRQn        = 141,      /*!< 141 [Active] TCPWM #1, Counter #10 */
  tcpwm_1_interrupts_11_IRQn        = 142,      /*!< 142 [Active] TCPWM #1, Counter #11 */
  tcpwm_1_interrupts_12_IRQn        = 143,      /*!< 143 [Active] TCPWM #1, Counter #12 */
  tcpwm_1_interrupts_13_IRQn        = 144,      /*!< 144 [Active] TCPWM #1, Counter #13 */
  tcpwm_1_interrupts_14_IRQn        = 145,      /*!< 145 [Active] TCPWM #1, Counter #14 */
  tcpwm_1_interrupts_15_IRQn        = 146,      /*!< 146 [Active] TCPWM #1, Counter #15 */
  tcpwm_1_interrupts_16_IRQn        = 147,      /*!< 147 [Active] TCPWM #1, Counter #16 */
  tcpwm_1_interrupts_17_IRQn        = 148,      /*!< 148 [Active] TCPWM #1, Counter #17 */
  tcpwm_1_interrupts_18_IRQn        = 149,      /*!< 149 [Active] TCPWM #1, Counter #18 */
  tcpwm_1_interrupts_19_IRQn        = 150,      /*!< 150 [Active] TCPWM #1, Counter #19 */
  tcpwm_1_interrupts_20_IRQn        = 151,      /*!< 151 [Active] TCPWM #1, Counter #20 */
  tcpwm_1_interrupts_21_IRQn        = 152,      /*!< 152 [Active] TCPWM #1, Counter #21 */
  tcpwm_1_interrupts_22_IRQn        = 153,      /*!< 153 [Active] TCPWM #1, Counter #22 */
  tcpwm_1_interrupts_23_IRQn        = 154,      /*!< 154 [Active] TCPWM #1, Counter #23 */
  pass_interrupt_sar_IRQn           = 155,      /*!< 155 [Active] SAR ADC interrupt */
  audioss_0_interrupt_i2s_IRQn      = 156,      /*!< 156 [Active] I2S0 Audio interrupt */
  audioss_0_interrupt_pdm_IRQn      = 157,      /*!< 157 [Active] PDM0/PCM0 Audio interrupt */
  audioss_1_interrupt_i2s_IRQn      = 158,      /*!< 158 [Active] I2S1 Audio interrupt */
  profile_interrupt_IRQn            = 159,      /*!< 159 [Active] Energy Profiler interrupt */
  smif_interrupt_IRQn               = 160,      /*!< 160 [Active] Serial Memory Interface interrupt */
  usb_interrupt_hi_IRQn             = 161,      /*!< 161 [Active] USB Interrupt */
  usb_interrupt_med_IRQn            = 162,      /*!< 162 [Active] USB Interrupt */
  usb_interrupt_lo_IRQn             = 163,      /*!< 163 [Active] USB Interrupt */
  sdhc_0_interrupt_wakeup_IRQn      = 164,      /*!< 164 [Active] SDIO wakeup interrupt for mxsdhc */
  sdhc_0_interrupt_general_IRQn     = 165,      /*!< 165 [Active] Consolidated interrupt for mxsdhc for everything else */
  sdhc_1_interrupt_wakeup_IRQn      = 166,      /*!< 166 [Active] EEMC wakeup interrupt for mxsdhc, not used */
  sdhc_1_interrupt_general_IRQn     = 167,      /*!< 167 [Active] Consolidated interrupt for mxsdhc for everything else */
  unconnected_IRQn                  = 240       /*!< 240 Unconnected */
#endif
} IRQn_Type;


#if ((defined(__GNUC__)        && (__ARM_ARCH == 6) && (__ARM_ARCH_6M__ == 1)) || \
     (defined(__ICCARM__)      && (__CORE__ == __ARM6M__)) || \
     (defined(__ARMCC_VERSION) && (__TARGET_ARCH_THUMB == 3)) || \
     (defined(__ghs__)         && defined(__CORE_CORTEXM0PLUS__)))

/* PSoC6A2M_124BGA interrupts that can be routed to the CM0+ NVIC */
typedef enum {
  ioss_interrupts_gpio_0_IRQn       =   0,      /*!<   0 [DeepSleep] GPIO Port Interrupt #0 */
  ioss_interrupts_gpio_1_IRQn       =   1,      /*!<   1 [DeepSleep] GPIO Port Interrupt #1 */
  ioss_interrupts_gpio_2_IRQn       =   2,      /*!<   2 [DeepSleep] GPIO Port Interrupt #2 */
  ioss_interrupts_gpio_3_IRQn       =   3,      /*!<   3 [DeepSleep] GPIO Port Interrupt #3 */
  ioss_interrupts_gpio_4_IRQn       =   4,      /*!<   4 [DeepSleep] GPIO Port Interrupt #4 */
  ioss_interrupts_gpio_5_IRQn       =   5,      /*!<   5 [DeepSleep] GPIO Port Interrupt #5 */
  ioss_interrupts_gpio_6_IRQn       =   6,      /*!<   6 [DeepSleep] GPIO Port Interrupt #6 */
  ioss_interrupts_gpio_7_IRQn       =   7,      /*!<   7 [DeepSleep] GPIO Port Interrupt #7 */
  ioss_interrupts_gpio_8_IRQn       =   8,      /*!<   8 [DeepSleep] GPIO Port Interrupt #8 */
  ioss_interrupts_gpio_9_IRQn       =   9,      /*!<   9 [DeepSleep] GPIO Port Interrupt #9 */
  ioss_interrupts_gpio_10_IRQn      =  10,      /*!<  10 [DeepSleep] GPIO Port Interrupt #10 */
  ioss_interrupts_gpio_11_IRQn      =  11,      /*!<  11 [DeepSleep] GPIO Port Interrupt #11 */
  ioss_interrupts_gpio_12_IRQn      =  12,      /*!<  12 [DeepSleep] GPIO Port Interrupt #12 */
  ioss_interrupts_gpio_13_IRQn      =  13,      /*!<  13 [DeepSleep] GPIO Port Interrupt #13 */
  ioss_interrupts_gpio_14_IRQn      =  14,      /*!<  14 [DeepSleep] GPIO Port Interrupt #14 */
  ioss_interrupt_gpio_IRQn          =  15,      /*!<  15 [DeepSleep] GPIO All Ports */
  ioss_interrupt_vdd_IRQn           =  16,      /*!<  16 [DeepSleep] GPIO Supply Detect Interrupt */
  lpcomp_interrupt_IRQn             =  17,      /*!<  17 [DeepSleep] Low Power Comparator Interrupt */
  scb_8_interrupt_IRQn              =  18,      /*!<  18 [DeepSleep] Serial Communication Block #8 (DeepSleep capable) */
  srss_interrupt_mcwdt_0_IRQn       =  19,      /*!<  19 [DeepSleep] Multi Counter Watchdog Timer interrupt */
  srss_interrupt_mcwdt_1_IRQn       =  20,      /*!<  20 [DeepSleep] Multi Counter Watchdog Timer interrupt */
  srss_interrupt_backup_IRQn        =  21,      /*!<  21 [DeepSleep] Backup domain interrupt */
  srss_interrupt_IRQn               =  22,      /*!<  22 [DeepSleep] Other combined Interrupts for SRSS (LVD, WDT, CLKCAL) */
  cpuss_interrupts_ipc_0_IRQn       =  23,      /*!<  23 [DeepSleep] CPUSS Inter Process Communication Interrupt #0 */
  cpuss_interrupts_ipc_1_IRQn       =  24,      /*!<  24 [DeepSleep] CPUSS Inter Process Communication Interrupt #1 */
  cpuss_interrupts_ipc_2_IRQn       =  25,      /*!<  25 [DeepSleep] CPUSS Inter Process Communication Interrupt #2 */
  cpuss_interrupts_ipc_3_IRQn       =  26,      /*!<  26 [DeepSleep] CPUSS Inter Process Communication Interrupt #3 */
  cpuss_interrupts_ipc_4_IRQn       =  27,      /*!<  27 [DeepSleep] CPUSS Inter Process Communication Interrupt #4 */
  cpuss_interrupts_ipc_5_IRQn       =  28,      /*!<  28 [DeepSleep] CPUSS Inter Process Communication Interrupt #5 */
  cpuss_interrupts_ipc_6_IRQn       =  29,      /*!<  29 [DeepSleep] CPUSS Inter Process Communication Interrupt #6 */
  cpuss_interrupts_ipc_7_IRQn       =  30,      /*!<  30 [DeepSleep] CPUSS Inter Process Communication Interrupt #7 */
  cpuss_interrupts_ipc_8_IRQn       =  31,      /*!<  31 [DeepSleep] CPUSS Inter Process Communication Interrupt #8 */
  cpuss_interrupts_ipc_9_IRQn       =  32,      /*!<  32 [DeepSleep] CPUSS Inter Process Communication Interrupt #9 */
  cpuss_interrupts_ipc_10_IRQn      =  33,      /*!<  33 [DeepSleep] CPUSS Inter Process Communication Interrupt #10 */
  cpuss_interrupts_ipc_11_IRQn      =  34,      /*!<  34 [DeepSleep] CPUSS Inter Process Communication Interrupt #11 */
  cpuss_interrupts_ipc_12_IRQn      =  35,      /*!<  35 [DeepSleep] CPUSS Inter Process Communication Interrupt #12 */
  cpuss_interrupts_ipc_13_IRQn      =  36,      /*!<  36 [DeepSleep] CPUSS Inter Process Communication Interrupt #13 */
  cpuss_interrupts_ipc_14_IRQn      =  37,      /*!<  37 [DeepSleep] CPUSS Inter Process Communication Interrupt #14 */
  cpuss_interrupts_ipc_15_IRQn      =  38,      /*!<  38 [DeepSleep] CPUSS Inter Process Communication Interrupt #15 */
  scb_0_interrupt_IRQn              =  39,      /*!<  39 [Active] Serial Communication Block #0 */
  scb_1_interrupt_IRQn              =  40,      /*!<  40 [Active] Serial Communication Block #1 */
  scb_2_interrupt_IRQn              =  41,      /*!<  41 [Active] Serial Communication Block #2 */
  scb_3_interrupt_IRQn              =  42,      /*!<  42 [Active] Serial Communication Block #3 */
  scb_4_interrupt_IRQn              =  43,      /*!<  43 [Active] Serial Communication Block #4 */
  scb_5_interrupt_IRQn              =  44,      /*!<  44 [Active] Serial Communication Block #5 */
  scb_6_interrupt_IRQn              =  45,      /*!<  45 [Active] Serial Communication Block #6 */
  scb_7_interrupt_IRQn              =  46,      /*!<  46 [Active] Serial Communication Block #7 */
  scb_9_interrupt_IRQn              =  47,      /*!<  47 [Active] Serial Communication Block #9 */
  scb_10_interrupt_IRQn             =  48,      /*!<  48 [Active] Serial Communication Block #10 */
  scb_11_interrupt_IRQn             =  49,      /*!<  49 [Active] Serial Communication Block #11 */
  scb_12_interrupt_IRQn             =  50,      /*!<  50 [Active] Serial Communication Block #12 */
  csd_interrupt_IRQn                =  51,      /*!<  51 [Active] CSD (Capsense) interrupt */
  cpuss_interrupts_dmac_0_IRQn      =  52,      /*!<  52 [Active] CPUSS DMAC, Channel #0 */
  cpuss_interrupts_dmac_1_IRQn      =  53,      /*!<  53 [Active] CPUSS DMAC, Channel #1 */
  cpuss_interrupts_dmac_2_IRQn      =  54,      /*!<  54 [Active] CPUSS DMAC, Channel #2 */
  cpuss_interrupts_dmac_3_IRQn      =  55,      /*!<  55 [Active] CPUSS DMAC, Channel #3 */
  cpuss_interrupts_dw0_0_IRQn       =  56,      /*!<  56 [Active] CPUSS DataWire #0, Channel #0 */
  cpuss_interrupts_dw0_1_IRQn       =  57,      /*!<  57 [Active] CPUSS DataWire #0, Channel #1 */
  cpuss_interrupts_dw0_2_IRQn       =  58,      /*!<  58 [Active] CPUSS DataWire #0, Channel #2 */
  cpuss_interrupts_dw0_3_IRQn       =  59,      /*!<  59 [Active] CPUSS DataWire #0, Channel #3 */
  cpuss_interrupts_dw0_4_IRQn       =  60,      /*!<  60 [Active] CPUSS DataWire #0, Channel #4 */
  cpuss_interrupts_dw0_5_IRQn       =  61,      /*!<  61 [Active] CPUSS DataWire #0, Channel #5 */
  cpuss_interrupts_dw0_6_IRQn       =  62,      /*!<  62 [Active] CPUSS DataWire #0, Channel #6 */
  cpuss_interrupts_dw0_7_IRQn       =  63,      /*!<  63 [Active] CPUSS DataWire #0, Channel #7 */
  cpuss_interrupts_dw0_8_IRQn       =  64,      /*!<  64 [Active] CPUSS DataWire #0, Channel #8 */
  cpuss_interrupts_dw0_9_IRQn       =  65,      /*!<  65 [Active] CPUSS DataWire #0, Channel #9 */
  cpuss_interrupts_dw0_10_IRQn      =  66,      /*!<  66 [Active] CPUSS DataWire #0, Channel #10 */
  cpuss_interrupts_dw0_11_IRQn      =  67,      /*!<  67 [Active] CPUSS DataWire #0, Channel #11 */
  cpuss_interrupts_dw0_12_IRQn      =  68,      /*!<  68 [Active] CPUSS DataWire #0, Channel #12 */
  cpuss_interrupts_dw0_13_IRQn      =  69,      /*!<  69 [Active] CPUSS DataWire #0, Channel #13 */
  cpuss_interrupts_dw0_14_IRQn      =  70,      /*!<  70 [Active] CPUSS DataWire #0, Channel #14 */
  cpuss_interrupts_dw0_15_IRQn      =  71,      /*!<  71 [Active] CPUSS DataWire #0, Channel #15 */
  cpuss_interrupts_dw0_16_IRQn      =  72,      /*!<  72 [Active] CPUSS DataWire #0, Channel #16 */
  cpuss_interrupts_dw0_17_IRQn      =  73,      /*!<  73 [Active] CPUSS DataWire #0, Channel #17 */
  cpuss_interrupts_dw0_18_IRQn      =  74,      /*!<  74 [Active] CPUSS DataWire #0, Channel #18 */
  cpuss_interrupts_dw0_19_IRQn      =  75,      /*!<  75 [Active] CPUSS DataWire #0, Channel #19 */
  cpuss_interrupts_dw0_20_IRQn      =  76,      /*!<  76 [Active] CPUSS DataWire #0, Channel #20 */
  cpuss_interrupts_dw0_21_IRQn      =  77,      /*!<  77 [Active] CPUSS DataWire #0, Channel #21 */
  cpuss_interrupts_dw0_22_IRQn      =  78,      /*!<  78 [Active] CPUSS DataWire #0, Channel #22 */
  cpuss_interrupts_dw0_23_IRQn      =  79,      /*!<  79 [Active] CPUSS DataWire #0, Channel #23 */
  cpuss_interrupts_dw0_24_IRQn      =  80,      /*!<  80 [Active] CPUSS DataWire #0, Channel #24 */
  cpuss_interrupts_dw0_25_IRQn      =  81,      /*!<  81 [Active] CPUSS DataWire #0, Channel #25 */
  cpuss_interrupts_dw0_26_IRQn      =  82,      /*!<  82 [Active] CPUSS DataWire #0, Channel #26 */
  cpuss_interrupts_dw0_27_IRQn      =  83,      /*!<  83 [Active] CPUSS DataWire #0, Channel #27 */
  cpuss_interrupts_dw0_28_IRQn      =  84,      /*!<  84 [Active] CPUSS DataWire #0, Channel #28 */
  cpuss_interrupts_dw1_0_IRQn       =  85,      /*!<  85 [Active] CPUSS DataWire #1, Channel #0 */
  cpuss_interrupts_dw1_1_IRQn       =  86,      /*!<  86 [Active] CPUSS DataWire #1, Channel #1 */
  cpuss_interrupts_dw1_2_IRQn       =  87,      /*!<  87 [Active] CPUSS DataWire #1, Channel #2 */
  cpuss_interrupts_dw1_3_IRQn       =  88,      /*!<  88 [Active] CPUSS DataWire #1, Channel #3 */
  cpuss_interrupts_dw1_4_IRQn       =  89,      /*!<  89 [Active] CPUSS DataWire #1, Channel #4 */
  cpuss_interrupts_dw1_5_IRQn       =  90,      /*!<  90 [Active] CPUSS DataWire #1, Channel #5 */
  cpuss_interrupts_dw1_6_IRQn       =  91,      /*!<  91 [Active] CPUSS DataWire #1, Channel #6 */
  cpuss_interrupts_dw1_7_IRQn       =  92,      /*!<  92 [Active] CPUSS DataWire #1, Channel #7 */
  cpuss_interrupts_dw1_8_IRQn       =  93,      /*!<  93 [Active] CPUSS DataWire #1, Channel #8 */
  cpuss_interrupts_dw1_9_IRQn       =  94,      /*!<  94 [Active] CPUSS DataWire #1, Channel #9 */
  cpuss_interrupts_dw1_10_IRQn      =  95,      /*!<  95 [Active] CPUSS DataWire #1, Channel #10 */
  cpuss_interrupts_dw1_11_IRQn      =  96,      /*!<  96 [Active] CPUSS DataWire #1, Channel #11 */
  cpuss_interrupts_dw1_12_IRQn      =  97,      /*!<  97 [Active] CPUSS DataWire #1, Channel #12 */
  cpuss_interrupts_dw1_13_IRQn      =  98,      /*!<  98 [Active] CPUSS DataWire #1, Channel #13 */
  cpuss_interrupts_dw1_14_IRQn      =  99,      /*!<  99 [Active] CPUSS DataWire #1, Channel #14 */
  cpuss_interrupts_dw1_15_IRQn      = 100,      /*!< 100 [Active] CPUSS DataWire #1, Channel #15 */
  cpuss_interrupts_dw1_16_IRQn      = 101,      /*!< 101 [Active] CPUSS DataWire #1, Channel #16 */
  cpuss_interrupts_dw1_17_IRQn      = 102,      /*!< 102 [Active] CPUSS DataWire #1, Channel #17 */
  cpuss_interrupts_dw1_18_IRQn      = 103,      /*!< 103 [Active] CPUSS DataWire #1, Channel #18 */
  cpuss_interrupts_dw1_19_IRQn      = 104,      /*!< 104 [Active] CPUSS DataWire #1, Channel #19 */
  cpuss_interrupts_dw1_20_IRQn      = 105,      /*!< 105 [Active] CPUSS DataWire #1, Channel #20 */
  cpuss_interrupts_dw1_21_IRQn      = 106,      /*!< 106 [Active] CPUSS DataWire #1, Channel #21 */
  cpuss_interrupts_dw1_22_IRQn      = 107,      /*!< 107 [Active] CPUSS DataWire #1, Channel #22 */
  cpuss_interrupts_dw1_23_IRQn      = 108,      /*!< 108 [Active] CPUSS DataWire #1, Channel #23 */
  cpuss_interrupts_dw1_24_IRQn      = 109,      /*!< 109 [Active] CPUSS DataWire #1, Channel #24 */
  cpuss_interrupts_dw1_25_IRQn      = 110,      /*!< 110 [Active] CPUSS DataWire #1, Channel #25 */
  cpuss_interrupts_dw1_26_IRQn      = 111,      /*!< 111 [Active] CPUSS DataWire #1, Channel #26 */
  cpuss_interrupts_dw1_27_IRQn      = 112,      /*!< 112 [Active] CPUSS DataWire #1, Channel #27 */
  cpuss_interrupts_dw1_28_IRQn      = 113,      /*!< 113 [Active] CPUSS DataWire #1, Channel #28 */
  cpuss_interrupts_fault_0_IRQn     = 114,      /*!< 114 [Active] CPUSS Fault Structure Interrupt #0 */
  cpuss_interrupts_fault_1_IRQn     = 115,      /*!< 115 [Active] CPUSS Fault Structure Interrupt #1 */
  cpuss_interrupt_crypto_IRQn       = 116,      /*!< 116 [Active] CRYPTO Accelerator Interrupt */
  cpuss_interrupt_fm_IRQn           = 117,      /*!< 117 [Active] FLASH Macro Interrupt */
  cpuss_interrupts_cm4_fp_IRQn      = 118,      /*!< 118 [Active] Floating Point operation fault */
  cpuss_interrupts_cm0_cti_0_IRQn   = 119,      /*!< 119 [Active] CM0+ CTI #0 */
  cpuss_interrupts_cm0_cti_1_IRQn   = 120,      /*!< 120 [Active] CM0+ CTI #1 */
  cpuss_interrupts_cm4_cti_0_IRQn   = 121,      /*!< 121 [Active] CM4 CTI #0 */
  cpuss_interrupts_cm4_cti_1_IRQn   = 122,      /*!< 122 [Active] CM4 CTI #1 */
  tcpwm_0_interrupts_0_IRQn         = 123,      /*!< 123 [Active] TCPWM #0, Counter #0 */
  tcpwm_0_interrupts_1_IRQn         = 124,      /*!< 124 [Active] TCPWM #0, Counter #1 */
  tcpwm_0_interrupts_2_IRQn         = 125,      /*!< 125 [Active] TCPWM #0, Counter #2 */
  tcpwm_0_interrupts_3_IRQn         = 126,      /*!< 126 [Active] TCPWM #0, Counter #3 */
  tcpwm_0_interrupts_4_IRQn         = 127,      /*!< 127 [Active] TCPWM #0, Counter #4 */
  tcpwm_0_interrupts_5_IRQn         = 128,      /*!< 128 [Active] TCPWM #0, Counter #5 */
  tcpwm_0_interrupts_6_IRQn         = 129,      /*!< 129 [Active] TCPWM #0, Counter #6 */
  tcpwm_0_interrupts_7_IRQn         = 130,      /*!< 130 [Active] TCPWM #0, Counter #7 */
  tcpwm_1_interrupts_0_IRQn         = 131,      /*!< 131 [Active] TCPWM #1, Counter #0 */
  tcpwm_1_interrupts_1_IRQn         = 132,      /*!< 132 [Active] TCPWM #1, Counter #1 */
  tcpwm_1_interrupts_2_IRQn         = 133,      /*!< 133 [Active] TCPWM #1, Counter #2 */
  tcpwm_1_interrupts_3_IRQn         = 134,      /*!< 134 [Active] TCPWM #1, Counter #3 */
  tcpwm_1_interrupts_4_IRQn         = 135,      /*!< 135 [Active] TCPWM #1, Counter #4 */
  tcpwm_1_interrupts_5_IRQn         = 136,      /*!< 136 [Active] TCPWM #1, Counter #5 */
  tcpwm_1_interrupts_6_IRQn         = 137,      /*!< 137 [Active] TCPWM #1, Counter #6 */
  tcpwm_1_interrupts_7_IRQn         = 138,      /*!< 138 [Active] TCPWM #1, Counter #7 */
  tcpwm_1_interrupts_8_IRQn         = 139,      /*!< 139 [Active] TCPWM #1, Counter #8 */
  tcpwm_1_interrupts_9_IRQn         = 140,      /*!< 140 [Active] TCPWM #1, Counter #9 */
  tcpwm_1_interrupts_10_IRQn        = 141,      /*!< 141 [Active] TCPWM #1, Counter #10 */
  tcpwm_1_interrupts_11_IRQn        = 142,      /*!< 142 [Active] TCPWM #1, Counter #11 */
  tcpwm_1_interrupts_12_IRQn        = 143,      /*!< 143 [Active] TCPWM #1, Counter #12 */
  tcpwm_1_interrupts_13_IRQn        = 144,      /*!< 144 [Active] TCPWM #1, Counter #13 */
  tcpwm_1_interrupts_14_IRQn        = 145,      /*!< 145 [Active] TCPWM #1, Counter #14 */
  tcpwm_1_interrupts_15_IRQn        = 146,      /*!< 146 [Active] TCPWM #1, Counter #15 */
  tcpwm_1_interrupts_16_IRQn        = 147,      /*!< 147 [Active] TCPWM #1, Counter #16 */
  tcpwm_1_interrupts_17_IRQn        = 148,      /*!< 148 [Active] TCPWM #1, Counter #17 */
  tcpwm_1_interrupts_18_IRQn        = 149,      /*!< 149 [Active] TCPWM #1, Counter #18 */
  tcpwm_1_interrupts_19_IRQn        = 150,      /*!< 150 [Active] TCPWM #1, Counter #19 */
  tcpwm_1_interrupts_20_IRQn        = 151,      /*!< 151 [Active] TCPWM #1, Counter #20 */
  tcpwm_1_interrupts_21_IRQn        = 152,      /*!< 152 [Active] TCPWM #1, Counter #21 */
  tcpwm_1_interrupts_22_IRQn        = 153,      /*!< 153 [Active] TCPWM #1, Counter #22 */
  tcpwm_1_interrupts_23_IRQn        = 154,      /*!< 154 [Active] TCPWM #1, Counter #23 */
  pass_interrupt_sar_IRQn           = 155,      /*!< 155 [Active] SAR ADC interrupt */
  audioss_0_interrupt_i2s_IRQn      = 156,      /*!< 156 [Active] I2S0 Audio interrupt */
  audioss_0_interrupt_pdm_IRQn      = 157,      /*!< 157 [Active] PDM0/PCM0 Audio interrupt */
  audioss_1_interrupt_i2s_IRQn      = 158,      /*!< 158 [Active] I2S1 Audio interrupt */
  profile_interrupt_IRQn            = 159,      /*!< 159 [Active] Energy Profiler interrupt */
  smif_interrupt_IRQn               = 160,      /*!< 160 [Active] Serial Memory Interface interrupt */
  usb_interrupt_hi_IRQn             = 161,      /*!< 161 [Active] USB Interrupt */
  usb_interrupt_med_IRQn            = 162,      /*!< 162 [Active] USB Interrupt */
  usb_interrupt_lo_IRQn             = 163,      /*!< 163 [Active] USB Interrupt */
  sdhc_0_interrupt_wakeup_IRQn      = 164,      /*!< 164 [Active] SDIO wakeup interrupt for mxsdhc */
  sdhc_0_interrupt_general_IRQn     = 165,      /*!< 165 [Active] Consolidated interrupt for mxsdhc for everything else */
  sdhc_1_interrupt_wakeup_IRQn      = 166,      /*!< 166 [Active] EEMC wakeup interrupt for mxsdhc, not used */
  sdhc_1_interrupt_general_IRQn     = 167,      /*!< 167 [Active] Consolidated interrupt for mxsdhc for everything else */
  disconnected_IRQn                 = 240       /*!< 240 Disconnected */
} cy_en_intr_t;

#endif

/*******************************************************************************
*                    Processor and Core Peripheral Section
*******************************************************************************/

#if ((defined(__GNUC__)        && (__ARM_ARCH == 6) && (__ARM_ARCH_6M__ == 1)) || \
     (defined(__ICCARM__)      && (__CORE__ == __ARM6M__)) || \
     (defined(__ARMCC_VERSION) && (__TARGET_ARCH_THUMB == 3)) || \
     (defined(__ghs__)         && defined(__CORE_CORTEXM0PLUS__)))

/* Configuration of the ARM Cortex-M0+ Processor and Core Peripherals */
#define __CM0PLUS_REV                   0x0001U /*!< CM0PLUS Core Revision */
#define __NVIC_PRIO_BITS                2       /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig          0       /*!< Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT                  1       /*!< Set to 1 if CPU supports Vector Table Offset Register */
#define __MPU_PRESENT                   1       /*!< MPU present or not */

/** \} Configuration_of_CMSIS */

/* Memory Blocks */
#define CY_ROM_BASE                     0x00000000UL
#define CY_ROM_SIZE                     0x00010000UL
#define CY_SRAM0_BASE                   0x08000000UL
#define CY_SRAM0_SIZE                   0x00080000UL
#define CY_SRAM1_BASE                   0x08080000UL
#define CY_SRAM1_SIZE                   0x00040000UL
#define CY_SRAM2_BASE                   0x080C0000UL
#define CY_SRAM2_SIZE                   0x00040000UL
#define CY_FLASH_BASE                   0x10000000UL
#define CY_FLASH_SIZE                   0x00200000UL
#define CY_EM_EEPROM_BASE               0x14000000UL
#define CY_EM_EEPROM_SIZE               0x00008000UL
#define CY_XIP_BASE                     0x18000000UL
#define CY_XIP_SIZE                     0x08000000UL
#define CY_SFLASH_BASE                  0x16000000UL
#define CY_SFLASH_SIZE                  0x00008000UL
#define CY_EFUSE_BASE                   0x402C0800UL
#define CY_EFUSE_SIZE                   0x00000200UL

#include "core_cm0plus.h"                       /*!< ARM Cortex-M0+ processor and core peripherals */
#include "system_psoc6.h"                       /*!< PSoC 6 System */

#else

/* Configuration of the ARM Cortex-M4 Processor and Core Peripherals */
#define __CM4_REV                       0x0001U /*!< CM4 Core Revision */
#define __NVIC_PRIO_BITS                3       /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig          0       /*!< Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT                  1       /*!< Set to 1 if CPU supports Vector Table Offset Register */
#define __MPU_PRESENT                   1       /*!< MPU present or not */
#define __FPU_PRESENT                   1       /*!< FPU present or not */
#define __CM0P_PRESENT                  1       /*!< CM0P present or not */

/** \} Configuration_of_CMSIS */

/* Memory Blocks */
#define CY_ROM_BASE                     0x00000000UL
#define CY_ROM_SIZE                     0x00010000UL
#define CY_SRAM0_BASE                   0x08000000UL
#define CY_SRAM0_SIZE                   0x00080000UL
#define CY_SRAM1_BASE                   0x08080000UL
#define CY_SRAM1_SIZE                   0x00040000UL
#define CY_SRAM2_BASE                   0x080C0000UL
#define CY_SRAM2_SIZE                   0x00040000UL
#define CY_FLASH_BASE                   0x10000000UL
#define CY_FLASH_SIZE                   0x00200000UL
#define CY_EM_EEPROM_BASE               0x14000000UL
#define CY_EM_EEPROM_SIZE               0x00008000UL
#define CY_XIP_BASE                     0x18000000UL
#define CY_XIP_SIZE                     0x08000000UL
#define CY_SFLASH_BASE                  0x16000000UL
#define CY_SFLASH_SIZE                  0x00008000UL
#define CY_EFUSE_BASE                   0x402C0800UL
#define CY_EFUSE_SIZE                   0x00000200UL

#include "core_cm4.h"                           /*!< ARM Cortex-M4 processor and core peripherals */
#include "system_psoc6.h"                       /*!< PSoC 6 System */

#endif

/* IP List */
#define CY_IP_MXTCPWM                   1u
#define CY_IP_MXTCPWM_INSTANCES         2u
#define CY_IP_MXTCPWM_VERSION           1u
#define CY_IP_MXCSDV2                   1u
#define CY_IP_MXCSDV2_INSTANCES         1u
#define CY_IP_MXCSDV2_VERSION           1u
#define CY_IP_MXLCD                     1u
#define CY_IP_MXLCD_INSTANCES           1u
#define CY_IP_MXLCD_VERSION             1u
#define CY_IP_MXS40SRSS                 1u
#define CY_IP_MXS40SRSS_INSTANCES       1u
#define CY_IP_MXS40SRSS_VERSION         1u
#define CY_IP_MXS40SRSS_RTC             1u
#define CY_IP_MXS40SRSS_RTC_INSTANCES   1u
#define CY_IP_MXS40SRSS_RTC_VERSION     1u
#define CY_IP_MXS40SRSS_MCWDT           1u
#define CY_IP_MXS40SRSS_MCWDT_INSTANCES 2u
#define CY_IP_MXS40SRSS_MCWDT_VERSION   1u
#define CY_IP_MXSCB                     1u
#define CY_IP_MXSCB_INSTANCES           13u
#define CY_IP_MXSCB_VERSION             1u
#define CY_IP_MXPERI                    1u
#define CY_IP_MXPERI_INSTANCES          1u
#define CY_IP_MXPERI_VERSION            2u
#define CY_IP_MXPERI_TR                 1u
#define CY_IP_MXPERI_TR_INSTANCES       1u
#define CY_IP_MXPERI_TR_VERSION         2u
#define CY_IP_M4CPUSS                   1u
#define CY_IP_M4CPUSS_INSTANCES         1u
#define CY_IP_M4CPUSS_VERSION           2u
#define CY_IP_M4CPUSS_DMAC              1u
#define CY_IP_M4CPUSS_DMAC_INSTANCES    1u
#define CY_IP_M4CPUSS_DMAC_VERSION      2u
#define CY_IP_M4CPUSS_DMA               1u
#define CY_IP_M4CPUSS_DMA_INSTANCES     2u
#define CY_IP_M4CPUSS_DMA_VERSION       2u
#define CY_IP_MXCRYPTO                  1u
#define CY_IP_MXCRYPTO_INSTANCES        1u
#define CY_IP_MXCRYPTO_VERSION          2u
#define CY_IP_MXSDHC                    1u
#define CY_IP_MXSDHC_INSTANCES          2u
#define CY_IP_MXSDHC_VERSION            1u
#define CY_IP_MXAUDIOSS                 1u
#define CY_IP_MXAUDIOSS_INSTANCES       2u
#define CY_IP_MXAUDIOSS_VERSION         1u
#define CY_IP_MXLPCOMP                  1u
#define CY_IP_MXLPCOMP_INSTANCES        1u
#define CY_IP_MXLPCOMP_VERSION          1u
#define CY_IP_MXSMIF                    1u
#define CY_IP_MXSMIF_INSTANCES          1u
#define CY_IP_MXSMIF_VERSION            1u
#define CY_IP_MXS40PASS                 1u
#define CY_IP_MXS40PASS_INSTANCES       1u
#define CY_IP_MXS40PASS_VERSION         1u
#define CY_IP_MXS40PASS_SAR             1u
#define CY_IP_MXS40PASS_SAR_INSTANCES   16u
#define CY_IP_MXS40PASS_SAR_VERSION     1u
#define CY_IP_MXS40IOSS                 1u
#define CY_IP_MXS40IOSS_INSTANCES       1u
#define CY_IP_MXS40IOSS_VERSION         2u
#define CY_IP_MXEFUSE                   1u
#define CY_IP_MXEFUSE_INSTANCES         1u
#define CY_IP_MXEFUSE_VERSION           1u
#define CY_IP_MXPROFILE                 1u
#define CY_IP_MXPROFILE_INSTANCES       1u
#define CY_IP_MXPROFILE_VERSION         1u

#include "psoc6a2m_config.h"
#include "gpio_psoc6a2m_124_bga.h"

#define CY_DEVICE_PSOC6A2M
#define CY_SILICON_ID                   0xE0002102UL
#define CY_HF_CLK_MAX_FREQ              150000000UL

#define CPUSS_FLASHC_PA_SIZE_LOG2       0x7UL

/*******************************************************************************
*                                    SFLASH
*******************************************************************************/

#define SFLASH_BASE                             0x16000000UL
#define SFLASH                                  ((SFLASH_Type*) SFLASH_BASE)                                      /* 0x16000000 */

/*******************************************************************************
*                                     PERI
*******************************************************************************/

#define PERI_BASE                               0x40000000UL
#define PERI                                    ((PERI_Type*) PERI_BASE)                                          /* 0x40000000 */
#define PERI_GR0                                ((PERI_GR_Type*) &PERI->GR[0])                                    /* 0x40004000 */
#define PERI_GR1                                ((PERI_GR_Type*) &PERI->GR[1])                                    /* 0x40004020 */
#define PERI_GR2                                ((PERI_GR_Type*) &PERI->GR[2])                                    /* 0x40004040 */
#define PERI_GR3                                ((PERI_GR_Type*) &PERI->GR[3])                                    /* 0x40004060 */
#define PERI_GR4                                ((PERI_GR_Type*) &PERI->GR[4])                                    /* 0x40004080 */
#define PERI_GR6                                ((PERI_GR_Type*) &PERI->GR[6])                                    /* 0x400040C0 */
#define PERI_GR9                                ((PERI_GR_Type*) &PERI->GR[9])                                    /* 0x40004120 */
#define PERI_GR10                               ((PERI_GR_Type*) &PERI->GR[10])                                   /* 0x40004140 */
#define PERI_TR_GR0                             ((PERI_TR_GR_Type*) &PERI->TR_GR[0])                              /* 0x40008000 */
#define PERI_TR_GR1                             ((PERI_TR_GR_Type*) &PERI->TR_GR[1])                              /* 0x40008400 */
#define PERI_TR_GR2                             ((PERI_TR_GR_Type*) &PERI->TR_GR[2])                              /* 0x40008800 */
#define PERI_TR_GR3                             ((PERI_TR_GR_Type*) &PERI->TR_GR[3])                              /* 0x40008C00 */
#define PERI_TR_GR4                             ((PERI_TR_GR_Type*) &PERI->TR_GR[4])                              /* 0x40009000 */
#define PERI_TR_GR5                             ((PERI_TR_GR_Type*) &PERI->TR_GR[5])                              /* 0x40009400 */
#define PERI_TR_GR6                             ((PERI_TR_GR_Type*) &PERI->TR_GR[6])                              /* 0x40009800 */
#define PERI_TR_GR7                             ((PERI_TR_GR_Type*) &PERI->TR_GR[7])                              /* 0x40009C00 */
#define PERI_TR_GR8                             ((PERI_TR_GR_Type*) &PERI->TR_GR[8])                              /* 0x4000A000 */
#define PERI_TR_GR9                             ((PERI_TR_GR_Type*) &PERI->TR_GR[9])                              /* 0x4000A400 */
#define PERI_TR_1TO1_GR0                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[0])                    /* 0x4000C000 */
#define PERI_TR_1TO1_GR1                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[1])                    /* 0x4000C400 */
#define PERI_TR_1TO1_GR2                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[2])                    /* 0x4000C800 */
#define PERI_TR_1TO1_GR3                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[3])                    /* 0x4000CC00 */
#define PERI_TR_1TO1_GR4                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[4])                    /* 0x4000D000 */
#define PERI_TR_1TO1_GR5                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[5])                    /* 0x4000D400 */
#define PERI_TR_1TO1_GR6                        ((PERI_TR_1TO1_GR_Type*) &PERI->TR_1TO1_GR[6])                    /* 0x4000D800 */

/*******************************************************************************
*                                   PERI_MS
*******************************************************************************/

#define PERI_MS_BASE                            0x40010000UL
#define PERI_MS                                 ((PERI_MS_Type*) PERI_MS_BASE)                                    /* 0x40010000 */
#define PERI_MS_PPU_PR0                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[0])                      /* 0x40010000 */
#define PERI_MS_PPU_PR1                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[1])                      /* 0x40010040 */
#define PERI_MS_PPU_PR2                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[2])                      /* 0x40010080 */
#define PERI_MS_PPU_PR3                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[3])                      /* 0x400100C0 */
#define PERI_MS_PPU_PR4                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[4])                      /* 0x40010100 */
#define PERI_MS_PPU_PR5                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[5])                      /* 0x40010140 */
#define PERI_MS_PPU_PR6                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[6])                      /* 0x40010180 */
#define PERI_MS_PPU_PR7                         ((PERI_MS_PPU_PR_Type*) &PERI_MS->PPU_PR[7])                      /* 0x400101C0 */
#define PERI_MS_PPU_FX0                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[0])                      /* 0x40010800 */
#define PERI_MS_PPU_FX1                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[1])                      /* 0x40010840 */
#define PERI_MS_PPU_FX2                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[2])                      /* 0x40010880 */
#define PERI_MS_PPU_FX3                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[3])                      /* 0x400108C0 */
#define PERI_MS_PPU_FX4                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[4])                      /* 0x40010900 */
#define PERI_MS_PPU_FX5                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[5])                      /* 0x40010940 */
#define PERI_MS_PPU_FX6                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[6])                      /* 0x40010980 */
#define PERI_MS_PPU_FX7                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[7])                      /* 0x400109C0 */
#define PERI_MS_PPU_FX8                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[8])                      /* 0x40010A00 */
#define PERI_MS_PPU_FX9                         ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[9])                      /* 0x40010A40 */
#define PERI_MS_PPU_FX10                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[10])                     /* 0x40010A80 */
#define PERI_MS_PPU_FX11                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[11])                     /* 0x40010AC0 */
#define PERI_MS_PPU_FX12                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[12])                     /* 0x40010B00 */
#define PERI_MS_PPU_FX13                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[13])                     /* 0x40010B40 */
#define PERI_MS_PPU_FX14                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[14])                     /* 0x40010B80 */
#define PERI_MS_PPU_FX15                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[15])                     /* 0x40010BC0 */
#define PERI_MS_PPU_FX16                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[16])                     /* 0x40010C00 */
#define PERI_MS_PPU_FX17                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[17])                     /* 0x40010C40 */
#define PERI_MS_PPU_FX18                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[18])                     /* 0x40010C80 */
#define PERI_MS_PPU_FX19                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[19])                     /* 0x40010CC0 */
#define PERI_MS_PPU_FX20                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[20])                     /* 0x40010D00 */
#define PERI_MS_PPU_FX21                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[21])                     /* 0x40010D40 */
#define PERI_MS_PPU_FX22                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[22])                     /* 0x40010D80 */
#define PERI_MS_PPU_FX23                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[23])                     /* 0x40010DC0 */
#define PERI_MS_PPU_FX24                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[24])                     /* 0x40010E00 */
#define PERI_MS_PPU_FX25                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[25])                     /* 0x40010E40 */
#define PERI_MS_PPU_FX26                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[26])                     /* 0x40010E80 */
#define PERI_MS_PPU_FX27                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[27])                     /* 0x40010EC0 */
#define PERI_MS_PPU_FX28                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[28])                     /* 0x40010F00 */
#define PERI_MS_PPU_FX29                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[29])                     /* 0x40010F40 */
#define PERI_MS_PPU_FX30                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[30])                     /* 0x40010F80 */
#define PERI_MS_PPU_FX31                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[31])                     /* 0x40010FC0 */
#define PERI_MS_PPU_FX32                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[32])                     /* 0x40011000 */
#define PERI_MS_PPU_FX33                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[33])                     /* 0x40011040 */
#define PERI_MS_PPU_FX34                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[34])                     /* 0x40011080 */
#define PERI_MS_PPU_FX35                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[35])                     /* 0x400110C0 */
#define PERI_MS_PPU_FX36                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[36])                     /* 0x40011100 */
#define PERI_MS_PPU_FX37                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[37])                     /* 0x40011140 */
#define PERI_MS_PPU_FX38                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[38])                     /* 0x40011180 */
#define PERI_MS_PPU_FX39                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[39])                     /* 0x400111C0 */
#define PERI_MS_PPU_FX40                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[40])                     /* 0x40011200 */
#define PERI_MS_PPU_FX41                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[41])                     /* 0x40011240 */
#define PERI_MS_PPU_FX42                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[42])                     /* 0x40011280 */
#define PERI_MS_PPU_FX43                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[43])                     /* 0x400112C0 */
#define PERI_MS_PPU_FX44                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[44])                     /* 0x40011300 */
#define PERI_MS_PPU_FX45                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[45])                     /* 0x40011340 */
#define PERI_MS_PPU_FX46                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[46])                     /* 0x40011380 */
#define PERI_MS_PPU_FX47                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[47])                     /* 0x400113C0 */
#define PERI_MS_PPU_FX48                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[48])                     /* 0x40011400 */
#define PERI_MS_PPU_FX49                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[49])                     /* 0x40011440 */
#define PERI_MS_PPU_FX50                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[50])                     /* 0x40011480 */
#define PERI_MS_PPU_FX51                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[51])                     /* 0x400114C0 */
#define PERI_MS_PPU_FX52                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[52])                     /* 0x40011500 */
#define PERI_MS_PPU_FX53                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[53])                     /* 0x40011540 */
#define PERI_MS_PPU_FX54                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[54])                     /* 0x40011580 */
#define PERI_MS_PPU_FX55                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[55])                     /* 0x400115C0 */
#define PERI_MS_PPU_FX56                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[56])                     /* 0x40011600 */
#define PERI_MS_PPU_FX57                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[57])                     /* 0x40011640 */
#define PERI_MS_PPU_FX58                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[58])                     /* 0x40011680 */
#define PERI_MS_PPU_FX59                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[59])                     /* 0x400116C0 */
#define PERI_MS_PPU_FX60                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[60])                     /* 0x40011700 */
#define PERI_MS_PPU_FX61                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[61])                     /* 0x40011740 */
#define PERI_MS_PPU_FX62                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[62])                     /* 0x40011780 */
#define PERI_MS_PPU_FX63                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[63])                     /* 0x400117C0 */
#define PERI_MS_PPU_FX64                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[64])                     /* 0x40011800 */
#define PERI_MS_PPU_FX65                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[65])                     /* 0x40011840 */
#define PERI_MS_PPU_FX66                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[66])                     /* 0x40011880 */
#define PERI_MS_PPU_FX67                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[67])                     /* 0x400118C0 */
#define PERI_MS_PPU_FX68                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[68])                     /* 0x40011900 */
#define PERI_MS_PPU_FX69                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[69])                     /* 0x40011940 */
#define PERI_MS_PPU_FX70                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[70])                     /* 0x40011980 */
#define PERI_MS_PPU_FX71                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[71])                     /* 0x400119C0 */
#define PERI_MS_PPU_FX72                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[72])                     /* 0x40011A00 */
#define PERI_MS_PPU_FX73                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[73])                     /* 0x40011A40 */
#define PERI_MS_PPU_FX74                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[74])                     /* 0x40011A80 */
#define PERI_MS_PPU_FX75                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[75])                     /* 0x40011AC0 */
#define PERI_MS_PPU_FX76                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[76])                     /* 0x40011B00 */
#define PERI_MS_PPU_FX77                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[77])                     /* 0x40011B40 */
#define PERI_MS_PPU_FX78                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[78])                     /* 0x40011B80 */
#define PERI_MS_PPU_FX79                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[79])                     /* 0x40011BC0 */
#define PERI_MS_PPU_FX80                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[80])                     /* 0x40011C00 */
#define PERI_MS_PPU_FX81                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[81])                     /* 0x40011C40 */
#define PERI_MS_PPU_FX82                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[82])                     /* 0x40011C80 */
#define PERI_MS_PPU_FX83                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[83])                     /* 0x40011CC0 */
#define PERI_MS_PPU_FX84                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[84])                     /* 0x40011D00 */
#define PERI_MS_PPU_FX85                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[85])                     /* 0x40011D40 */
#define PERI_MS_PPU_FX86                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[86])                     /* 0x40011D80 */
#define PERI_MS_PPU_FX87                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[87])                     /* 0x40011DC0 */
#define PERI_MS_PPU_FX88                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[88])                     /* 0x40011E00 */
#define PERI_MS_PPU_FX89                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[89])                     /* 0x40011E40 */
#define PERI_MS_PPU_FX90                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[90])                     /* 0x40011E80 */
#define PERI_MS_PPU_FX91                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[91])                     /* 0x40011EC0 */
#define PERI_MS_PPU_FX92                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[92])                     /* 0x40011F00 */
#define PERI_MS_PPU_FX93                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[93])                     /* 0x40011F40 */
#define PERI_MS_PPU_FX94                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[94])                     /* 0x40011F80 */
#define PERI_MS_PPU_FX95                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[95])                     /* 0x40011FC0 */
#define PERI_MS_PPU_FX96                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[96])                     /* 0x40012000 */
#define PERI_MS_PPU_FX97                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[97])                     /* 0x40012040 */
#define PERI_MS_PPU_FX98                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[98])                     /* 0x40012080 */
#define PERI_MS_PPU_FX99                        ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[99])                     /* 0x400120C0 */
#define PERI_MS_PPU_FX100                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[100])                    /* 0x40012100 */
#define PERI_MS_PPU_FX101                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[101])                    /* 0x40012140 */
#define PERI_MS_PPU_FX102                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[102])                    /* 0x40012180 */
#define PERI_MS_PPU_FX103                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[103])                    /* 0x400121C0 */
#define PERI_MS_PPU_FX104                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[104])                    /* 0x40012200 */
#define PERI_MS_PPU_FX105                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[105])                    /* 0x40012240 */
#define PERI_MS_PPU_FX106                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[106])                    /* 0x40012280 */
#define PERI_MS_PPU_FX107                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[107])                    /* 0x400122C0 */
#define PERI_MS_PPU_FX108                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[108])                    /* 0x40012300 */
#define PERI_MS_PPU_FX109                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[109])                    /* 0x40012340 */
#define PERI_MS_PPU_FX110                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[110])                    /* 0x40012380 */
#define PERI_MS_PPU_FX111                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[111])                    /* 0x400123C0 */
#define PERI_MS_PPU_FX112                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[112])                    /* 0x40012400 */
#define PERI_MS_PPU_FX113                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[113])                    /* 0x40012440 */
#define PERI_MS_PPU_FX114                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[114])                    /* 0x40012480 */
#define PERI_MS_PPU_FX115                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[115])                    /* 0x400124C0 */
#define PERI_MS_PPU_FX116                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[116])                    /* 0x40012500 */
#define PERI_MS_PPU_FX117                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[117])                    /* 0x40012540 */
#define PERI_MS_PPU_FX118                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[118])                    /* 0x40012580 */
#define PERI_MS_PPU_FX119                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[119])                    /* 0x400125C0 */
#define PERI_MS_PPU_FX120                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[120])                    /* 0x40012600 */
#define PERI_MS_PPU_FX121                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[121])                    /* 0x40012640 */
#define PERI_MS_PPU_FX122                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[122])                    /* 0x40012680 */
#define PERI_MS_PPU_FX123                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[123])                    /* 0x400126C0 */
#define PERI_MS_PPU_FX124                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[124])                    /* 0x40012700 */
#define PERI_MS_PPU_FX125                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[125])                    /* 0x40012740 */
#define PERI_MS_PPU_FX126                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[126])                    /* 0x40012780 */
#define PERI_MS_PPU_FX127                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[127])                    /* 0x400127C0 */
#define PERI_MS_PPU_FX128                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[128])                    /* 0x40012800 */
#define PERI_MS_PPU_FX129                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[129])                    /* 0x40012840 */
#define PERI_MS_PPU_FX130                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[130])                    /* 0x40012880 */
#define PERI_MS_PPU_FX131                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[131])                    /* 0x400128C0 */
#define PERI_MS_PPU_FX132                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[132])                    /* 0x40012900 */
#define PERI_MS_PPU_FX133                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[133])                    /* 0x40012940 */
#define PERI_MS_PPU_FX134                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[134])                    /* 0x40012980 */
#define PERI_MS_PPU_FX135                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[135])                    /* 0x400129C0 */
#define PERI_MS_PPU_FX136                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[136])                    /* 0x40012A00 */
#define PERI_MS_PPU_FX137                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[137])                    /* 0x40012A40 */
#define PERI_MS_PPU_FX138                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[138])                    /* 0x40012A80 */
#define PERI_MS_PPU_FX139                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[139])                    /* 0x40012AC0 */
#define PERI_MS_PPU_FX140                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[140])                    /* 0x40012B00 */
#define PERI_MS_PPU_FX141                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[141])                    /* 0x40012B40 */
#define PERI_MS_PPU_FX142                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[142])                    /* 0x40012B80 */
#define PERI_MS_PPU_FX143                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[143])                    /* 0x40012BC0 */
#define PERI_MS_PPU_FX144                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[144])                    /* 0x40012C00 */
#define PERI_MS_PPU_FX145                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[145])                    /* 0x40012C40 */
#define PERI_MS_PPU_FX146                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[146])                    /* 0x40012C80 */
#define PERI_MS_PPU_FX147                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[147])                    /* 0x40012CC0 */
#define PERI_MS_PPU_FX148                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[148])                    /* 0x40012D00 */
#define PERI_MS_PPU_FX149                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[149])                    /* 0x40012D40 */
#define PERI_MS_PPU_FX150                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[150])                    /* 0x40012D80 */
#define PERI_MS_PPU_FX151                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[151])                    /* 0x40012DC0 */
#define PERI_MS_PPU_FX152                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[152])                    /* 0x40012E00 */
#define PERI_MS_PPU_FX153                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[153])                    /* 0x40012E40 */
#define PERI_MS_PPU_FX154                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[154])                    /* 0x40012E80 */
#define PERI_MS_PPU_FX155                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[155])                    /* 0x40012EC0 */
#define PERI_MS_PPU_FX156                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[156])                    /* 0x40012F00 */
#define PERI_MS_PPU_FX157                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[157])                    /* 0x40012F40 */
#define PERI_MS_PPU_FX158                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[158])                    /* 0x40012F80 */
#define PERI_MS_PPU_FX159                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[159])                    /* 0x40012FC0 */
#define PERI_MS_PPU_FX160                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[160])                    /* 0x40013000 */
#define PERI_MS_PPU_FX161                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[161])                    /* 0x40013040 */
#define PERI_MS_PPU_FX162                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[162])                    /* 0x40013080 */
#define PERI_MS_PPU_FX163                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[163])                    /* 0x400130C0 */
#define PERI_MS_PPU_FX164                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[164])                    /* 0x40013100 */
#define PERI_MS_PPU_FX165                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[165])                    /* 0x40013140 */
#define PERI_MS_PPU_FX166                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[166])                    /* 0x40013180 */
#define PERI_MS_PPU_FX167                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[167])                    /* 0x400131C0 */
#define PERI_MS_PPU_FX168                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[168])                    /* 0x40013200 */
#define PERI_MS_PPU_FX169                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[169])                    /* 0x40013240 */
#define PERI_MS_PPU_FX170                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[170])                    /* 0x40013280 */
#define PERI_MS_PPU_FX171                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[171])                    /* 0x400132C0 */
#define PERI_MS_PPU_FX172                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[172])                    /* 0x40013300 */
#define PERI_MS_PPU_FX173                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[173])                    /* 0x40013340 */
#define PERI_MS_PPU_FX174                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[174])                    /* 0x40013380 */
#define PERI_MS_PPU_FX175                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[175])                    /* 0x400133C0 */
#define PERI_MS_PPU_FX176                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[176])                    /* 0x40013400 */
#define PERI_MS_PPU_FX177                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[177])                    /* 0x40013440 */
#define PERI_MS_PPU_FX178                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[178])                    /* 0x40013480 */
#define PERI_MS_PPU_FX179                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[179])                    /* 0x400134C0 */
#define PERI_MS_PPU_FX180                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[180])                    /* 0x40013500 */
#define PERI_MS_PPU_FX181                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[181])                    /* 0x40013540 */
#define PERI_MS_PPU_FX182                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[182])                    /* 0x40013580 */
#define PERI_MS_PPU_FX183                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[183])                    /* 0x400135C0 */
#define PERI_MS_PPU_FX184                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[184])                    /* 0x40013600 */
#define PERI_MS_PPU_FX185                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[185])                    /* 0x40013640 */
#define PERI_MS_PPU_FX186                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[186])                    /* 0x40013680 */
#define PERI_MS_PPU_FX187                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[187])                    /* 0x400136C0 */
#define PERI_MS_PPU_FX188                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[188])                    /* 0x40013700 */
#define PERI_MS_PPU_FX189                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[189])                    /* 0x40013740 */
#define PERI_MS_PPU_FX190                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[190])                    /* 0x40013780 */
#define PERI_MS_PPU_FX191                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[191])                    /* 0x400137C0 */
#define PERI_MS_PPU_FX192                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[192])                    /* 0x40013800 */
#define PERI_MS_PPU_FX193                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[193])                    /* 0x40013840 */
#define PERI_MS_PPU_FX194                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[194])                    /* 0x40013880 */
#define PERI_MS_PPU_FX195                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[195])                    /* 0x400138C0 */
#define PERI_MS_PPU_FX196                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[196])                    /* 0x40013900 */
#define PERI_MS_PPU_FX197                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[197])                    /* 0x40013940 */
#define PERI_MS_PPU_FX198                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[198])                    /* 0x40013980 */
#define PERI_MS_PPU_FX199                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[199])                    /* 0x400139C0 */
#define PERI_MS_PPU_FX200                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[200])                    /* 0x40013A00 */
#define PERI_MS_PPU_FX201                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[201])                    /* 0x40013A40 */
#define PERI_MS_PPU_FX202                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[202])                    /* 0x40013A80 */
#define PERI_MS_PPU_FX203                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[203])                    /* 0x40013AC0 */
#define PERI_MS_PPU_FX204                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[204])                    /* 0x40013B00 */
#define PERI_MS_PPU_FX205                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[205])                    /* 0x40013B40 */
#define PERI_MS_PPU_FX206                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[206])                    /* 0x40013B80 */
#define PERI_MS_PPU_FX207                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[207])                    /* 0x40013BC0 */
#define PERI_MS_PPU_FX208                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[208])                    /* 0x40013C00 */
#define PERI_MS_PPU_FX209                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[209])                    /* 0x40013C40 */
#define PERI_MS_PPU_FX210                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[210])                    /* 0x40013C80 */
#define PERI_MS_PPU_FX211                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[211])                    /* 0x40013CC0 */
#define PERI_MS_PPU_FX212                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[212])                    /* 0x40013D00 */
#define PERI_MS_PPU_FX213                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[213])                    /* 0x40013D40 */
#define PERI_MS_PPU_FX214                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[214])                    /* 0x40013D80 */
#define PERI_MS_PPU_FX215                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[215])                    /* 0x40013DC0 */
#define PERI_MS_PPU_FX216                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[216])                    /* 0x40013E00 */
#define PERI_MS_PPU_FX217                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[217])                    /* 0x40013E40 */
#define PERI_MS_PPU_FX218                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[218])                    /* 0x40013E80 */
#define PERI_MS_PPU_FX219                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[219])                    /* 0x40013EC0 */
#define PERI_MS_PPU_FX220                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[220])                    /* 0x40013F00 */
#define PERI_MS_PPU_FX221                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[221])                    /* 0x40013F40 */
#define PERI_MS_PPU_FX222                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[222])                    /* 0x40013F80 */
#define PERI_MS_PPU_FX223                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[223])                    /* 0x40013FC0 */
#define PERI_MS_PPU_FX224                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[224])                    /* 0x40014000 */
#define PERI_MS_PPU_FX225                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[225])                    /* 0x40014040 */
#define PERI_MS_PPU_FX226                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[226])                    /* 0x40014080 */
#define PERI_MS_PPU_FX227                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[227])                    /* 0x400140C0 */
#define PERI_MS_PPU_FX228                       ((PERI_MS_PPU_FX_Type*) &PERI_MS->PPU_FX[228])                    /* 0x40014100 */

/*******************************************************************************
*                                    CRYPTO
*******************************************************************************/

#define CRYPTO_BASE                             0x40100000UL
#define CRYPTO                                  ((CRYPTO_Type*) CRYPTO_BASE)                                      /* 0x40100000 */

/*******************************************************************************
*                                    CPUSS
*******************************************************************************/

#define CPUSS_BASE                              0x40200000UL
#define CPUSS                                   ((CPUSS_Type*) CPUSS_BASE)                                        /* 0x40200000 */

/*******************************************************************************
*                                    FAULT
*******************************************************************************/

#define FAULT_BASE                              0x40210000UL
#define FAULT                                   ((FAULT_Type*) FAULT_BASE)                                        /* 0x40210000 */
#define FAULT_STRUCT0                           ((FAULT_STRUCT_Type*) &FAULT->STRUCT[0])                          /* 0x40210000 */
#define FAULT_STRUCT1                           ((FAULT_STRUCT_Type*) &FAULT->STRUCT[1])                          /* 0x40210100 */

/*******************************************************************************
*                                     IPC
*******************************************************************************/

#define IPC_BASE                                0x40220000UL
#define IPC                                     ((IPC_Type*) IPC_BASE)                                            /* 0x40220000 */
#define IPC_STRUCT0                             ((IPC_STRUCT_Type*) &IPC->STRUCT[0])                              /* 0x40220000 */
#define IPC_STRUCT1                             ((IPC_STRUCT_Type*) &IPC->STRUCT[1])                              /* 0x40220020 */
#define IPC_STRUCT2                             ((IPC_STRUCT_Type*) &IPC->STRUCT[2])                              /* 0x40220040 */
#define IPC_STRUCT3                             ((IPC_STRUCT_Type*) &IPC->STRUCT[3])                              /* 0x40220060 */
#define IPC_STRUCT4                             ((IPC_STRUCT_Type*) &IPC->STRUCT[4])                              /* 0x40220080 */
#define IPC_STRUCT5                             ((IPC_STRUCT_Type*) &IPC->STRUCT[5])                              /* 0x402200A0 */
#define IPC_STRUCT6                             ((IPC_STRUCT_Type*) &IPC->STRUCT[6])                              /* 0x402200C0 */
#define IPC_STRUCT7                             ((IPC_STRUCT_Type*) &IPC->STRUCT[7])                              /* 0x402200E0 */
#define IPC_STRUCT8                             ((IPC_STRUCT_Type*) &IPC->STRUCT[8])                              /* 0x40220100 */
#define IPC_STRUCT9                             ((IPC_STRUCT_Type*) &IPC->STRUCT[9])                              /* 0x40220120 */
#define IPC_STRUCT10                            ((IPC_STRUCT_Type*) &IPC->STRUCT[10])                             /* 0x40220140 */
#define IPC_STRUCT11                            ((IPC_STRUCT_Type*) &IPC->STRUCT[11])                             /* 0x40220160 */
#define IPC_STRUCT12                            ((IPC_STRUCT_Type*) &IPC->STRUCT[12])                             /* 0x40220180 */
#define IPC_STRUCT13                            ((IPC_STRUCT_Type*) &IPC->STRUCT[13])                             /* 0x402201A0 */
#define IPC_STRUCT14                            ((IPC_STRUCT_Type*) &IPC->STRUCT[14])                             /* 0x402201C0 */
#define IPC_STRUCT15                            ((IPC_STRUCT_Type*) &IPC->STRUCT[15])                             /* 0x402201E0 */
#define IPC_INTR_STRUCT0                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[0])                    /* 0x40221000 */
#define IPC_INTR_STRUCT1                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[1])                    /* 0x40221020 */
#define IPC_INTR_STRUCT2                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[2])                    /* 0x40221040 */
#define IPC_INTR_STRUCT3                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[3])                    /* 0x40221060 */
#define IPC_INTR_STRUCT4                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[4])                    /* 0x40221080 */
#define IPC_INTR_STRUCT5                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[5])                    /* 0x402210A0 */
#define IPC_INTR_STRUCT6                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[6])                    /* 0x402210C0 */
#define IPC_INTR_STRUCT7                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[7])                    /* 0x402210E0 */
#define IPC_INTR_STRUCT8                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[8])                    /* 0x40221100 */
#define IPC_INTR_STRUCT9                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[9])                    /* 0x40221120 */
#define IPC_INTR_STRUCT10                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[10])                   /* 0x40221140 */
#define IPC_INTR_STRUCT11                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[11])                   /* 0x40221160 */
#define IPC_INTR_STRUCT12                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[12])                   /* 0x40221180 */
#define IPC_INTR_STRUCT13                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[13])                   /* 0x402211A0 */
#define IPC_INTR_STRUCT14                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[14])                   /* 0x402211C0 */
#define IPC_INTR_STRUCT15                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[15])                   /* 0x402211E0 */

/*******************************************************************************
*                                     PROT
*******************************************************************************/

#define PROT_BASE                               0x40230000UL
#define PROT                                    ((PROT_Type*) PROT_BASE)                                          /* 0x40230000 */
#define PROT_SMPU_SMPU_STRUCT0                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[0])        /* 0x40232000 */
#define PROT_SMPU_SMPU_STRUCT1                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[1])        /* 0x40232040 */
#define PROT_SMPU_SMPU_STRUCT2                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[2])        /* 0x40232080 */
#define PROT_SMPU_SMPU_STRUCT3                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[3])        /* 0x402320C0 */
#define PROT_SMPU_SMPU_STRUCT4                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[4])        /* 0x40232100 */
#define PROT_SMPU_SMPU_STRUCT5                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[5])        /* 0x40232140 */
#define PROT_SMPU_SMPU_STRUCT6                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[6])        /* 0x40232180 */
#define PROT_SMPU_SMPU_STRUCT7                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[7])        /* 0x402321C0 */
#define PROT_SMPU_SMPU_STRUCT8                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[8])        /* 0x40232200 */
#define PROT_SMPU_SMPU_STRUCT9                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[9])        /* 0x40232240 */
#define PROT_SMPU_SMPU_STRUCT10                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[10])       /* 0x40232280 */
#define PROT_SMPU_SMPU_STRUCT11                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[11])       /* 0x402322C0 */
#define PROT_SMPU_SMPU_STRUCT12                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[12])       /* 0x40232300 */
#define PROT_SMPU_SMPU_STRUCT13                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[13])       /* 0x40232340 */
#define PROT_SMPU_SMPU_STRUCT14                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[14])       /* 0x40232380 */
#define PROT_SMPU_SMPU_STRUCT15                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[15])       /* 0x402323C0 */
#define PROT_SMPU                               ((PROT_SMPU_Type*) &PROT->SMPU)                                   /* 0x40230000 */
#define PROT_MPU5_MPU_STRUCT0                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[0])       /* 0x40235600 */
#define PROT_MPU5_MPU_STRUCT1                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[1])       /* 0x40235620 */
#define PROT_MPU5_MPU_STRUCT2                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[2])       /* 0x40235640 */
#define PROT_MPU5_MPU_STRUCT3                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[3])       /* 0x40235660 */
#define PROT_MPU5_MPU_STRUCT4                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[4])       /* 0x40235680 */
#define PROT_MPU5_MPU_STRUCT5                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[5])       /* 0x402356A0 */
#define PROT_MPU5_MPU_STRUCT6                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[6])       /* 0x402356C0 */
#define PROT_MPU5_MPU_STRUCT7                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[5].MPU_STRUCT[7])       /* 0x402356E0 */
#define PROT_MPU6_MPU_STRUCT0                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[0])       /* 0x40235A00 */
#define PROT_MPU6_MPU_STRUCT1                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[1])       /* 0x40235A20 */
#define PROT_MPU6_MPU_STRUCT2                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[2])       /* 0x40235A40 */
#define PROT_MPU6_MPU_STRUCT3                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[3])       /* 0x40235A60 */
#define PROT_MPU6_MPU_STRUCT4                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[4])       /* 0x40235A80 */
#define PROT_MPU6_MPU_STRUCT5                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[5])       /* 0x40235AA0 */
#define PROT_MPU6_MPU_STRUCT6                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[6])       /* 0x40235AC0 */
#define PROT_MPU6_MPU_STRUCT7                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[6].MPU_STRUCT[7])       /* 0x40235AE0 */
#define PROT_MPU15_MPU_STRUCT0                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[0])      /* 0x40237E00 */
#define PROT_MPU15_MPU_STRUCT1                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[1])      /* 0x40237E20 */
#define PROT_MPU15_MPU_STRUCT2                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[2])      /* 0x40237E40 */
#define PROT_MPU15_MPU_STRUCT3                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[3])      /* 0x40237E60 */
#define PROT_MPU15_MPU_STRUCT4                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[4])      /* 0x40237E80 */
#define PROT_MPU15_MPU_STRUCT5                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[5])      /* 0x40237EA0 */
#define PROT_MPU15_MPU_STRUCT6                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[6])      /* 0x40237EC0 */
#define PROT_MPU15_MPU_STRUCT7                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[7])      /* 0x40237EE0 */
#define PROT_MPU0                               ((PROT_MPU_Type*) &PROT->CYMPU[0])                                /* 0x40234000 */
#define PROT_MPU1                               ((PROT_MPU_Type*) &PROT->CYMPU[1])                                /* 0x40234400 */
#define PROT_MPU2                               ((PROT_MPU_Type*) &PROT->CYMPU[2])                                /* 0x40234800 */
#define PROT_MPU3                               ((PROT_MPU_Type*) &PROT->CYMPU[3])                                /* 0x40234C00 */
#define PROT_MPU4                               ((PROT_MPU_Type*) &PROT->CYMPU[4])                                /* 0x40235000 */
#define PROT_MPU5                               ((PROT_MPU_Type*) &PROT->CYMPU[5])                                /* 0x40235400 */
#define PROT_MPU6                               ((PROT_MPU_Type*) &PROT->CYMPU[6])                                /* 0x40235800 */
#define PROT_MPU7                               ((PROT_MPU_Type*) &PROT->CYMPU[7])                                /* 0x40235C00 */
#define PROT_MPU8                               ((PROT_MPU_Type*) &PROT->CYMPU[8])                                /* 0x40236000 */
#define PROT_MPU9                               ((PROT_MPU_Type*) &PROT->CYMPU[9])                                /* 0x40236400 */
#define PROT_MPU10                              ((PROT_MPU_Type*) &PROT->CYMPU[10])                               /* 0x40236800 */
#define PROT_MPU11                              ((PROT_MPU_Type*) &PROT->CYMPU[11])                               /* 0x40236C00 */
#define PROT_MPU12                              ((PROT_MPU_Type*) &PROT->CYMPU[12])                               /* 0x40237000 */
#define PROT_MPU13                              ((PROT_MPU_Type*) &PROT->CYMPU[13])                               /* 0x40237400 */
#define PROT_MPU14                              ((PROT_MPU_Type*) &PROT->CYMPU[14])                               /* 0x40237800 */
#define PROT_MPU15                              ((PROT_MPU_Type*) &PROT->CYMPU[15])                               /* 0x40237C00 */

/*******************************************************************************
*                                    FLASHC
*******************************************************************************/

#define FLASHC_BASE                             0x40240000UL
#define FLASHC                                  ((FLASHC_Type*) FLASHC_BASE)                                      /* 0x40240000 */
#define FLASHC_FM_CTL                           ((FLASHC_FM_CTL_Type*) &FLASHC->FM_CTL)                           /* 0x4024F000 */

/*******************************************************************************
*                                     SRSS
*******************************************************************************/

#define SRSS_BASE                               0x40260000UL
#define SRSS                                    ((SRSS_Type*) SRSS_BASE)                                          /* 0x40260000 */
#define MCWDT_STRUCT0                           ((MCWDT_STRUCT_Type*) &SRSS->MCWDT_STRUCT[0])                     /* 0x40260200 */
#define MCWDT_STRUCT1                           ((MCWDT_STRUCT_Type*) &SRSS->MCWDT_STRUCT[1])                     /* 0x40260240 */

/*******************************************************************************
*                                    BACKUP
*******************************************************************************/

#define BACKUP_BASE                             0x40270000UL
#define BACKUP                                  ((BACKUP_Type*) BACKUP_BASE)                                      /* 0x40270000 */

/*******************************************************************************
*                                      DW
*******************************************************************************/

#define DW0_BASE                                0x40280000UL
#define DW1_BASE                                0x40290000UL
#define DW0                                     ((DW_Type*) DW0_BASE)                                             /* 0x40280000 */
#define DW1                                     ((DW_Type*) DW1_BASE)                                             /* 0x40290000 */
#define DW0_CH_STRUCT0                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[0])                         /* 0x40288000 */
#define DW0_CH_STRUCT1                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[1])                         /* 0x40288040 */
#define DW0_CH_STRUCT2                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[2])                         /* 0x40288080 */
#define DW0_CH_STRUCT3                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[3])                         /* 0x402880C0 */
#define DW0_CH_STRUCT4                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[4])                         /* 0x40288100 */
#define DW0_CH_STRUCT5                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[5])                         /* 0x40288140 */
#define DW0_CH_STRUCT6                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[6])                         /* 0x40288180 */
#define DW0_CH_STRUCT7                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[7])                         /* 0x402881C0 */
#define DW0_CH_STRUCT8                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[8])                         /* 0x40288200 */
#define DW0_CH_STRUCT9                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[9])                         /* 0x40288240 */
#define DW0_CH_STRUCT10                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[10])                        /* 0x40288280 */
#define DW0_CH_STRUCT11                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[11])                        /* 0x402882C0 */
#define DW0_CH_STRUCT12                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[12])                        /* 0x40288300 */
#define DW0_CH_STRUCT13                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[13])                        /* 0x40288340 */
#define DW0_CH_STRUCT14                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[14])                        /* 0x40288380 */
#define DW0_CH_STRUCT15                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[15])                        /* 0x402883C0 */
#define DW0_CH_STRUCT16                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[16])                        /* 0x40288400 */
#define DW0_CH_STRUCT17                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[17])                        /* 0x40288440 */
#define DW0_CH_STRUCT18                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[18])                        /* 0x40288480 */
#define DW0_CH_STRUCT19                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[19])                        /* 0x402884C0 */
#define DW0_CH_STRUCT20                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[20])                        /* 0x40288500 */
#define DW0_CH_STRUCT21                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[21])                        /* 0x40288540 */
#define DW0_CH_STRUCT22                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[22])                        /* 0x40288580 */
#define DW0_CH_STRUCT23                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[23])                        /* 0x402885C0 */
#define DW0_CH_STRUCT24                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[24])                        /* 0x40288600 */
#define DW0_CH_STRUCT25                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[25])                        /* 0x40288640 */
#define DW0_CH_STRUCT26                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[26])                        /* 0x40288680 */
#define DW0_CH_STRUCT27                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[27])                        /* 0x402886C0 */
#define DW0_CH_STRUCT28                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[28])                        /* 0x40288700 */
#define DW1_CH_STRUCT0                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[0])                         /* 0x40298000 */
#define DW1_CH_STRUCT1                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[1])                         /* 0x40298040 */
#define DW1_CH_STRUCT2                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[2])                         /* 0x40298080 */
#define DW1_CH_STRUCT3                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[3])                         /* 0x402980C0 */
#define DW1_CH_STRUCT4                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[4])                         /* 0x40298100 */
#define DW1_CH_STRUCT5                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[5])                         /* 0x40298140 */
#define DW1_CH_STRUCT6                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[6])                         /* 0x40298180 */
#define DW1_CH_STRUCT7                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[7])                         /* 0x402981C0 */
#define DW1_CH_STRUCT8                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[8])                         /* 0x40298200 */
#define DW1_CH_STRUCT9                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[9])                         /* 0x40298240 */
#define DW1_CH_STRUCT10                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[10])                        /* 0x40298280 */
#define DW1_CH_STRUCT11                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[11])                        /* 0x402982C0 */
#define DW1_CH_STRUCT12                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[12])                        /* 0x40298300 */
#define DW1_CH_STRUCT13                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[13])                        /* 0x40298340 */
#define DW1_CH_STRUCT14                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[14])                        /* 0x40298380 */
#define DW1_CH_STRUCT15                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[15])                        /* 0x402983C0 */
#define DW1_CH_STRUCT16                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[16])                        /* 0x40298400 */
#define DW1_CH_STRUCT17                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[17])                        /* 0x40298440 */
#define DW1_CH_STRUCT18                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[18])                        /* 0x40298480 */
#define DW1_CH_STRUCT19                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[19])                        /* 0x402984C0 */
#define DW1_CH_STRUCT20                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[20])                        /* 0x40298500 */
#define DW1_CH_STRUCT21                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[21])                        /* 0x40298540 */
#define DW1_CH_STRUCT22                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[22])                        /* 0x40298580 */
#define DW1_CH_STRUCT23                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[23])                        /* 0x402985C0 */
#define DW1_CH_STRUCT24                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[24])                        /* 0x40298600 */
#define DW1_CH_STRUCT25                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[25])                        /* 0x40298640 */
#define DW1_CH_STRUCT26                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[26])                        /* 0x40298680 */
#define DW1_CH_STRUCT27                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[27])                        /* 0x402986C0 */
#define DW1_CH_STRUCT28                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[28])                        /* 0x40298700 */

/*******************************************************************************
*                                     DMAC
*******************************************************************************/

#define DMAC_BASE                               0x402A0000UL
#define DMAC                                    ((DMAC_Type*) DMAC_BASE)                                          /* 0x402A0000 */
#define DMAC_CH0                                ((DMAC_CH_Type*) &DMAC->CH[0])                                    /* 0x402A1000 */
#define DMAC_CH1                                ((DMAC_CH_Type*) &DMAC->CH[1])                                    /* 0x402A1100 */
#define DMAC_CH2                                ((DMAC_CH_Type*) &DMAC->CH[2])                                    /* 0x402A1200 */
#define DMAC_CH3                                ((DMAC_CH_Type*) &DMAC->CH[3])                                    /* 0x402A1300 */

/*******************************************************************************
*                                    EFUSE
*******************************************************************************/

#define EFUSE_BASE                              0x402C0000UL
#define EFUSE                                   ((EFUSE_Type*) EFUSE_BASE)                                        /* 0x402C0000 */

/*******************************************************************************
*                                   PROFILE
*******************************************************************************/

#define PROFILE_BASE                            0x402D0000UL
#define PROFILE                                 ((PROFILE_Type*) PROFILE_BASE)                                    /* 0x402D0000 */
#define PROFILE_CNT_STRUCT0                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[0])              /* 0x402D0800 */
#define PROFILE_CNT_STRUCT1                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[1])              /* 0x402D0810 */
#define PROFILE_CNT_STRUCT2                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[2])              /* 0x402D0820 */
#define PROFILE_CNT_STRUCT3                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[3])              /* 0x402D0830 */
#define PROFILE_CNT_STRUCT4                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[4])              /* 0x402D0840 */
#define PROFILE_CNT_STRUCT5                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[5])              /* 0x402D0850 */
#define PROFILE_CNT_STRUCT6                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[6])              /* 0x402D0860 */
#define PROFILE_CNT_STRUCT7                     ((PROFILE_CNT_STRUCT_Type*) &PROFILE->CNT_STRUCT[7])              /* 0x402D0870 */

/*******************************************************************************
*                                    HSIOM
*******************************************************************************/

#define HSIOM_BASE                              0x40300000UL
#define HSIOM                                   ((HSIOM_Type*) HSIOM_BASE)                                        /* 0x40300000 */
#define HSIOM_PRT0                              ((HSIOM_PRT_Type*) &HSIOM->PRT[0])                                /* 0x40300000 */
#define HSIOM_PRT1                              ((HSIOM_PRT_Type*) &HSIOM->PRT[1])                                /* 0x40300010 */
#define HSIOM_PRT2                              ((HSIOM_PRT_Type*) &HSIOM->PRT[2])                                /* 0x40300020 */
#define HSIOM_PRT3                              ((HSIOM_PRT_Type*) &HSIOM->PRT[3])                                /* 0x40300030 */
#define HSIOM_PRT4                              ((HSIOM_PRT_Type*) &HSIOM->PRT[4])                                /* 0x40300040 */
#define HSIOM_PRT5                              ((HSIOM_PRT_Type*) &HSIOM->PRT[5])                                /* 0x40300050 */
#define HSIOM_PRT6                              ((HSIOM_PRT_Type*) &HSIOM->PRT[6])                                /* 0x40300060 */
#define HSIOM_PRT7                              ((HSIOM_PRT_Type*) &HSIOM->PRT[7])                                /* 0x40300070 */
#define HSIOM_PRT8                              ((HSIOM_PRT_Type*) &HSIOM->PRT[8])                                /* 0x40300080 */
#define HSIOM_PRT9                              ((HSIOM_PRT_Type*) &HSIOM->PRT[9])                                /* 0x40300090 */
#define HSIOM_PRT10                             ((HSIOM_PRT_Type*) &HSIOM->PRT[10])                               /* 0x403000A0 */
#define HSIOM_PRT11                             ((HSIOM_PRT_Type*) &HSIOM->PRT[11])                               /* 0x403000B0 */
#define HSIOM_PRT12                             ((HSIOM_PRT_Type*) &HSIOM->PRT[12])                               /* 0x403000C0 */
#define HSIOM_PRT13                             ((HSIOM_PRT_Type*) &HSIOM->PRT[13])                               /* 0x403000D0 */
#define HSIOM_PRT14                             ((HSIOM_PRT_Type*) &HSIOM->PRT[14])                               /* 0x403000E0 */

/*******************************************************************************
*                                     GPIO
*******************************************************************************/

#define GPIO_BASE                               0x40310000UL
#define GPIO                                    ((GPIO_Type*) GPIO_BASE)                                          /* 0x40310000 */
#define GPIO_PRT0                               ((GPIO_PRT_Type*) &GPIO->PRT[0])                                  /* 0x40310000 */
#define GPIO_PRT1                               ((GPIO_PRT_Type*) &GPIO->PRT[1])                                  /* 0x40310080 */
#define GPIO_PRT2                               ((GPIO_PRT_Type*) &GPIO->PRT[2])                                  /* 0x40310100 */
#define GPIO_PRT3                               ((GPIO_PRT_Type*) &GPIO->PRT[3])                                  /* 0x40310180 */
#define GPIO_PRT4                               ((GPIO_PRT_Type*) &GPIO->PRT[4])                                  /* 0x40310200 */
#define GPIO_PRT5                               ((GPIO_PRT_Type*) &GPIO->PRT[5])                                  /* 0x40310280 */
#define GPIO_PRT6                               ((GPIO_PRT_Type*) &GPIO->PRT[6])                                  /* 0x40310300 */
#define GPIO_PRT7                               ((GPIO_PRT_Type*) &GPIO->PRT[7])                                  /* 0x40310380 */
#define GPIO_PRT8                               ((GPIO_PRT_Type*) &GPIO->PRT[8])                                  /* 0x40310400 */
#define GPIO_PRT9                               ((GPIO_PRT_Type*) &GPIO->PRT[9])                                  /* 0x40310480 */
#define GPIO_PRT10                              ((GPIO_PRT_Type*) &GPIO->PRT[10])                                 /* 0x40310500 */
#define GPIO_PRT11                              ((GPIO_PRT_Type*) &GPIO->PRT[11])                                 /* 0x40310580 */
#define GPIO_PRT12                              ((GPIO_PRT_Type*) &GPIO->PRT[12])                                 /* 0x40310600 */
#define GPIO_PRT13                              ((GPIO_PRT_Type*) &GPIO->PRT[13])                                 /* 0x40310680 */
#define GPIO_PRT14                              ((GPIO_PRT_Type*) &GPIO->PRT[14])                                 /* 0x40310700 */

/*******************************************************************************
*                                   SMARTIO
*******************************************************************************/

#define SMARTIO_BASE                            0x40320000UL
#define SMARTIO                                 ((SMARTIO_Type*) SMARTIO_BASE)                                    /* 0x40320000 */
#define SMARTIO_PRT8                            ((SMARTIO_PRT_Type*) &SMARTIO->PRT[8])                            /* 0x40320800 */
#define SMARTIO_PRT9                            ((SMARTIO_PRT_Type*) &SMARTIO->PRT[9])                            /* 0x40320900 */

/*******************************************************************************
*                                    LPCOMP
*******************************************************************************/

#define LPCOMP_BASE                             0x40350000UL
#define LPCOMP                                  ((LPCOMP_Type*) LPCOMP_BASE)                                      /* 0x40350000 */

/*******************************************************************************
*                                     CSD
*******************************************************************************/

#define CSD0_BASE                               0x40360000UL
#define CSD0                                    ((CSD_Type*) CSD0_BASE)                                           /* 0x40360000 */

/*******************************************************************************
*                                    TCPWM
*******************************************************************************/

#define TCPWM0_BASE                             0x40380000UL
#define TCPWM1_BASE                             0x40390000UL
#define TCPWM0                                  ((TCPWM_Type*) TCPWM0_BASE)                                       /* 0x40380000 */
#define TCPWM1                                  ((TCPWM_Type*) TCPWM1_BASE)                                       /* 0x40390000 */
#define TCPWM0_CNT0                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[0])                               /* 0x40380100 */
#define TCPWM0_CNT1                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[1])                               /* 0x40380140 */
#define TCPWM0_CNT2                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[2])                               /* 0x40380180 */
#define TCPWM0_CNT3                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[3])                               /* 0x403801C0 */
#define TCPWM0_CNT4                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[4])                               /* 0x40380200 */
#define TCPWM0_CNT5                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[5])                               /* 0x40380240 */
#define TCPWM0_CNT6                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[6])                               /* 0x40380280 */
#define TCPWM0_CNT7                             ((TCPWM_CNT_Type*) &TCPWM0->CNT[7])                               /* 0x403802C0 */
#define TCPWM1_CNT0                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[0])                               /* 0x40390100 */
#define TCPWM1_CNT1                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[1])                               /* 0x40390140 */
#define TCPWM1_CNT2                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[2])                               /* 0x40390180 */
#define TCPWM1_CNT3                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[3])                               /* 0x403901C0 */
#define TCPWM1_CNT4                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[4])                               /* 0x40390200 */
#define TCPWM1_CNT5                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[5])                               /* 0x40390240 */
#define TCPWM1_CNT6                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[6])                               /* 0x40390280 */
#define TCPWM1_CNT7                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[7])                               /* 0x403902C0 */
#define TCPWM1_CNT8                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[8])                               /* 0x40390300 */
#define TCPWM1_CNT9                             ((TCPWM_CNT_Type*) &TCPWM1->CNT[9])                               /* 0x40390340 */
#define TCPWM1_CNT10                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[10])                              /* 0x40390380 */
#define TCPWM1_CNT11                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[11])                              /* 0x403903C0 */
#define TCPWM1_CNT12                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[12])                              /* 0x40390400 */
#define TCPWM1_CNT13                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[13])                              /* 0x40390440 */
#define TCPWM1_CNT14                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[14])                              /* 0x40390480 */
#define TCPWM1_CNT15                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[15])                              /* 0x403904C0 */
#define TCPWM1_CNT16                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[16])                              /* 0x40390500 */
#define TCPWM1_CNT17                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[17])                              /* 0x40390540 */
#define TCPWM1_CNT18                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[18])                              /* 0x40390580 */
#define TCPWM1_CNT19                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[19])                              /* 0x403905C0 */
#define TCPWM1_CNT20                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[20])                              /* 0x40390600 */
#define TCPWM1_CNT21                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[21])                              /* 0x40390640 */
#define TCPWM1_CNT22                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[22])                              /* 0x40390680 */
#define TCPWM1_CNT23                            ((TCPWM_CNT_Type*) &TCPWM1->CNT[23])                              /* 0x403906C0 */

/*******************************************************************************
*                                     LCD
*******************************************************************************/

#define LCD0_BASE                               0x403B0000UL
#define LCD0                                    ((LCD_Type*) LCD0_BASE)                                           /* 0x403B0000 */

/*******************************************************************************
*                                     SMIF
*******************************************************************************/

#define SMIF0_BASE                              0x40420000UL
#define SMIF0                                   ((SMIF_Type*) SMIF0_BASE)                                         /* 0x40420000 */
#define SMIF0_DEVICE0                           ((SMIF_DEVICE_Type*) &SMIF0->DEVICE[0])                           /* 0x40420800 */
#define SMIF0_DEVICE1                           ((SMIF_DEVICE_Type*) &SMIF0->DEVICE[1])                           /* 0x40420880 */
#define SMIF0_DEVICE2                           ((SMIF_DEVICE_Type*) &SMIF0->DEVICE[2])                           /* 0x40420900 */
#define SMIF0_DEVICE3                           ((SMIF_DEVICE_Type*) &SMIF0->DEVICE[3])                           /* 0x40420980 */

/*******************************************************************************
*                                     SDHC
*******************************************************************************/

#define SDHC0_BASE                              0x40460000UL
#define SDHC1_BASE                              0x40470000UL
#define SDHC0                                   ((SDHC_Type*) SDHC0_BASE)                                         /* 0x40460000 */
#define SDHC1                                   ((SDHC_Type*) SDHC1_BASE)                                         /* 0x40470000 */
#define SDHC0_WRAP                              ((SDHC_WRAP_Type*) &SDHC0->WRAP)                                  /* 0x40460000 */
#define SDHC1_WRAP                              ((SDHC_WRAP_Type*) &SDHC1->WRAP)                                  /* 0x40470000 */
#define SDHC0_CORE                              ((SDHC_CORE_Type*) &SDHC0->CORE)                                  /* 0x40461000 */
#define SDHC1_CORE                              ((SDHC_CORE_Type*) &SDHC1->CORE)                                  /* 0x40471000 */

/*******************************************************************************
*                                     SCB
*******************************************************************************/

#define SCB0_BASE                               0x40600000UL
#define SCB1_BASE                               0x40610000UL
#define SCB2_BASE                               0x40620000UL
#define SCB3_BASE                               0x40630000UL
#define SCB4_BASE                               0x40640000UL
#define SCB5_BASE                               0x40650000UL
#define SCB6_BASE                               0x40660000UL
#define SCB7_BASE                               0x40670000UL
#define SCB8_BASE                               0x40680000UL
#define SCB9_BASE                               0x40690000UL
#define SCB10_BASE                              0x406A0000UL
#define SCB11_BASE                              0x406B0000UL
#define SCB12_BASE                              0x406C0000UL
#define SCB0                                    ((CySCB_Type*) SCB0_BASE)                                         /* 0x40600000 */
#define SCB1                                    ((CySCB_Type*) SCB1_BASE)                                         /* 0x40610000 */
#define SCB2                                    ((CySCB_Type*) SCB2_BASE)                                         /* 0x40620000 */
#define SCB3                                    ((CySCB_Type*) SCB3_BASE)                                         /* 0x40630000 */
#define SCB4                                    ((CySCB_Type*) SCB4_BASE)                                         /* 0x40640000 */
#define SCB5                                    ((CySCB_Type*) SCB5_BASE)                                         /* 0x40650000 */
#define SCB6                                    ((CySCB_Type*) SCB6_BASE)                                         /* 0x40660000 */
#define SCB7                                    ((CySCB_Type*) SCB7_BASE)                                         /* 0x40670000 */
#define SCB8                                    ((CySCB_Type*) SCB8_BASE)                                         /* 0x40680000 */
#define SCB9                                    ((CySCB_Type*) SCB9_BASE)                                         /* 0x40690000 */
#define SCB10                                   ((CySCB_Type*) SCB10_BASE)                                        /* 0x406A0000 */
#define SCB11                                   ((CySCB_Type*) SCB11_BASE)                                        /* 0x406B0000 */
#define SCB12                                   ((CySCB_Type*) SCB12_BASE)                                        /* 0x406C0000 */

/*******************************************************************************
*                                     CTBM
*******************************************************************************/


/*******************************************************************************
*                                    CTDAC
*******************************************************************************/


/*******************************************************************************
*                                     SAR
*******************************************************************************/

#define SAR_BASE                                0x409D0000UL
#define SAR                                     ((SAR_Type*) SAR_BASE)                                            /* 0x409D0000 */

/*******************************************************************************
*                                     PASS
*******************************************************************************/

#define PASS_BASE                               0x409F0000UL
#define PASS                                    ((PASS_Type*) PASS_BASE)                                          /* 0x409F0000 */
#define PASS_AREF                               ((PASS_AREF_Type*) &PASS->AREF)                                   /* 0x409F0E00 */

/*******************************************************************************
*                                     PDM
*******************************************************************************/

#define PDM0_BASE                               0x40A00000UL
#define PDM0                                    ((PDM_Type*) PDM0_BASE)                                           /* 0x40A00000 */

/*******************************************************************************
*                                     I2S
*******************************************************************************/

#define I2S0_BASE                               0x40A10000UL
#define I2S1_BASE                               0x40A11000UL
#define I2S0                                    ((I2S_Type*) I2S0_BASE)                                           /* 0x40A10000 */
#define I2S1                                    ((I2S_Type*) I2S1_BASE)                                           /* 0x40A11000 */

/** \} PSoC6A2M_124BGA */

#endif /* _PSOC6A2M_124BGA_H_ */


/* [] END OF FILE */
