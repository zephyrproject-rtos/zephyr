/***************************************************************************//**
* \file cy8c6136bzi_f34.h
*
* \brief
* CY8C6136BZI-F34 device header
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

#ifndef _CY8C6136BZI_F34_H_
#define _CY8C6136BZI_F34_H_

/**
* \addtogroup group_device CY8C6136BZI-F34
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
  /* CY8C6136BZI-F34 Peripheral Interrupt Numbers */
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
  pass_interrupt_ctbs_IRQn          =  23,      /*!<  23 [DeepSleep] CTBm Interrupt (all CTBms) */
  bless_interrupt_IRQn              =  24,      /*!<  24 [DeepSleep] Bluetooth Radio interrupt */
  cpuss_interrupts_ipc_0_IRQn       =  25,      /*!<  25 [DeepSleep] CPUSS Inter Process Communication Interrupt #0 */
  cpuss_interrupts_ipc_1_IRQn       =  26,      /*!<  26 [DeepSleep] CPUSS Inter Process Communication Interrupt #1 */
  cpuss_interrupts_ipc_2_IRQn       =  27,      /*!<  27 [DeepSleep] CPUSS Inter Process Communication Interrupt #2 */
  cpuss_interrupts_ipc_3_IRQn       =  28,      /*!<  28 [DeepSleep] CPUSS Inter Process Communication Interrupt #3 */
  cpuss_interrupts_ipc_4_IRQn       =  29,      /*!<  29 [DeepSleep] CPUSS Inter Process Communication Interrupt #4 */
  cpuss_interrupts_ipc_5_IRQn       =  30,      /*!<  30 [DeepSleep] CPUSS Inter Process Communication Interrupt #5 */
  cpuss_interrupts_ipc_6_IRQn       =  31,      /*!<  31 [DeepSleep] CPUSS Inter Process Communication Interrupt #6 */
  cpuss_interrupts_ipc_7_IRQn       =  32,      /*!<  32 [DeepSleep] CPUSS Inter Process Communication Interrupt #7 */
  cpuss_interrupts_ipc_8_IRQn       =  33,      /*!<  33 [DeepSleep] CPUSS Inter Process Communication Interrupt #8 */
  cpuss_interrupts_ipc_9_IRQn       =  34,      /*!<  34 [DeepSleep] CPUSS Inter Process Communication Interrupt #9 */
  cpuss_interrupts_ipc_10_IRQn      =  35,      /*!<  35 [DeepSleep] CPUSS Inter Process Communication Interrupt #10 */
  cpuss_interrupts_ipc_11_IRQn      =  36,      /*!<  36 [DeepSleep] CPUSS Inter Process Communication Interrupt #11 */
  cpuss_interrupts_ipc_12_IRQn      =  37,      /*!<  37 [DeepSleep] CPUSS Inter Process Communication Interrupt #12 */
  cpuss_interrupts_ipc_13_IRQn      =  38,      /*!<  38 [DeepSleep] CPUSS Inter Process Communication Interrupt #13 */
  cpuss_interrupts_ipc_14_IRQn      =  39,      /*!<  39 [DeepSleep] CPUSS Inter Process Communication Interrupt #14 */
  cpuss_interrupts_ipc_15_IRQn      =  40,      /*!<  40 [DeepSleep] CPUSS Inter Process Communication Interrupt #15 */
  scb_0_interrupt_IRQn              =  41,      /*!<  41 [Active] Serial Communication Block #0 */
  scb_1_interrupt_IRQn              =  42,      /*!<  42 [Active] Serial Communication Block #1 */
  scb_2_interrupt_IRQn              =  43,      /*!<  43 [Active] Serial Communication Block #2 */
  scb_3_interrupt_IRQn              =  44,      /*!<  44 [Active] Serial Communication Block #3 */
  scb_4_interrupt_IRQn              =  45,      /*!<  45 [Active] Serial Communication Block #4 */
  scb_5_interrupt_IRQn              =  46,      /*!<  46 [Active] Serial Communication Block #5 */
  scb_6_interrupt_IRQn              =  47,      /*!<  47 [Active] Serial Communication Block #6 */
  scb_7_interrupt_IRQn              =  48,      /*!<  48 [Active] Serial Communication Block #7 */
  csd_interrupt_IRQn                =  49,      /*!<  49 [Active] CSD (Capsense) interrupt */
  cpuss_interrupts_dw0_0_IRQn       =  50,      /*!<  50 [Active] CPUSS DataWire #0, Channel #0 */
  cpuss_interrupts_dw0_1_IRQn       =  51,      /*!<  51 [Active] CPUSS DataWire #0, Channel #1 */
  cpuss_interrupts_dw0_2_IRQn       =  52,      /*!<  52 [Active] CPUSS DataWire #0, Channel #2 */
  cpuss_interrupts_dw0_3_IRQn       =  53,      /*!<  53 [Active] CPUSS DataWire #0, Channel #3 */
  cpuss_interrupts_dw0_4_IRQn       =  54,      /*!<  54 [Active] CPUSS DataWire #0, Channel #4 */
  cpuss_interrupts_dw0_5_IRQn       =  55,      /*!<  55 [Active] CPUSS DataWire #0, Channel #5 */
  cpuss_interrupts_dw0_6_IRQn       =  56,      /*!<  56 [Active] CPUSS DataWire #0, Channel #6 */
  cpuss_interrupts_dw0_7_IRQn       =  57,      /*!<  57 [Active] CPUSS DataWire #0, Channel #7 */
  cpuss_interrupts_dw0_8_IRQn       =  58,      /*!<  58 [Active] CPUSS DataWire #0, Channel #8 */
  cpuss_interrupts_dw0_9_IRQn       =  59,      /*!<  59 [Active] CPUSS DataWire #0, Channel #9 */
  cpuss_interrupts_dw0_10_IRQn      =  60,      /*!<  60 [Active] CPUSS DataWire #0, Channel #10 */
  cpuss_interrupts_dw0_11_IRQn      =  61,      /*!<  61 [Active] CPUSS DataWire #0, Channel #11 */
  cpuss_interrupts_dw0_12_IRQn      =  62,      /*!<  62 [Active] CPUSS DataWire #0, Channel #12 */
  cpuss_interrupts_dw0_13_IRQn      =  63,      /*!<  63 [Active] CPUSS DataWire #0, Channel #13 */
  cpuss_interrupts_dw0_14_IRQn      =  64,      /*!<  64 [Active] CPUSS DataWire #0, Channel #14 */
  cpuss_interrupts_dw0_15_IRQn      =  65,      /*!<  65 [Active] CPUSS DataWire #0, Channel #15 */
  cpuss_interrupts_dw1_0_IRQn       =  66,      /*!<  66 [Active] CPUSS DataWire #1, Channel #0 */
  cpuss_interrupts_dw1_1_IRQn       =  67,      /*!<  67 [Active] CPUSS DataWire #1, Channel #1 */
  cpuss_interrupts_dw1_2_IRQn       =  68,      /*!<  68 [Active] CPUSS DataWire #1, Channel #2 */
  cpuss_interrupts_dw1_3_IRQn       =  69,      /*!<  69 [Active] CPUSS DataWire #1, Channel #3 */
  cpuss_interrupts_dw1_4_IRQn       =  70,      /*!<  70 [Active] CPUSS DataWire #1, Channel #4 */
  cpuss_interrupts_dw1_5_IRQn       =  71,      /*!<  71 [Active] CPUSS DataWire #1, Channel #5 */
  cpuss_interrupts_dw1_6_IRQn       =  72,      /*!<  72 [Active] CPUSS DataWire #1, Channel #6 */
  cpuss_interrupts_dw1_7_IRQn       =  73,      /*!<  73 [Active] CPUSS DataWire #1, Channel #7 */
  cpuss_interrupts_dw1_8_IRQn       =  74,      /*!<  74 [Active] CPUSS DataWire #1, Channel #8 */
  cpuss_interrupts_dw1_9_IRQn       =  75,      /*!<  75 [Active] CPUSS DataWire #1, Channel #9 */
  cpuss_interrupts_dw1_10_IRQn      =  76,      /*!<  76 [Active] CPUSS DataWire #1, Channel #10 */
  cpuss_interrupts_dw1_11_IRQn      =  77,      /*!<  77 [Active] CPUSS DataWire #1, Channel #11 */
  cpuss_interrupts_dw1_12_IRQn      =  78,      /*!<  78 [Active] CPUSS DataWire #1, Channel #12 */
  cpuss_interrupts_dw1_13_IRQn      =  79,      /*!<  79 [Active] CPUSS DataWire #1, Channel #13 */
  cpuss_interrupts_dw1_14_IRQn      =  80,      /*!<  80 [Active] CPUSS DataWire #1, Channel #14 */
  cpuss_interrupts_dw1_15_IRQn      =  81,      /*!<  81 [Active] CPUSS DataWire #1, Channel #15 */
  cpuss_interrupts_fault_0_IRQn     =  82,      /*!<  82 [Active] CPUSS Fault Structure Interrupt #0 */
  cpuss_interrupts_fault_1_IRQn     =  83,      /*!<  83 [Active] CPUSS Fault Structure Interrupt #1 */
  cpuss_interrupt_crypto_IRQn       =  84,      /*!<  84 [Active] CRYPTO Accelerator Interrupt */
  cpuss_interrupt_fm_IRQn           =  85,      /*!<  85 [Active] FLASH Macro Interrupt */
  cpuss_interrupts_cm0_cti_0_IRQn   =  86,      /*!<  86 [Active] CM0+ CTI #0 */
  cpuss_interrupts_cm0_cti_1_IRQn   =  87,      /*!<  87 [Active] CM0+ CTI #1 */
  cpuss_interrupts_cm4_cti_0_IRQn   =  88,      /*!<  88 [Active] CM4 CTI #0 */
  cpuss_interrupts_cm4_cti_1_IRQn   =  89,      /*!<  89 [Active] CM4 CTI #1 */
  tcpwm_0_interrupts_0_IRQn         =  90,      /*!<  90 [Active] TCPWM #0, Counter #0 */
  tcpwm_0_interrupts_1_IRQn         =  91,      /*!<  91 [Active] TCPWM #0, Counter #1 */
  tcpwm_0_interrupts_2_IRQn         =  92,      /*!<  92 [Active] TCPWM #0, Counter #2 */
  tcpwm_0_interrupts_3_IRQn         =  93,      /*!<  93 [Active] TCPWM #0, Counter #3 */
  tcpwm_0_interrupts_4_IRQn         =  94,      /*!<  94 [Active] TCPWM #0, Counter #4 */
  tcpwm_0_interrupts_5_IRQn         =  95,      /*!<  95 [Active] TCPWM #0, Counter #5 */
  tcpwm_0_interrupts_6_IRQn         =  96,      /*!<  96 [Active] TCPWM #0, Counter #6 */
  tcpwm_0_interrupts_7_IRQn         =  97,      /*!<  97 [Active] TCPWM #0, Counter #7 */
  tcpwm_1_interrupts_0_IRQn         =  98,      /*!<  98 [Active] TCPWM #1, Counter #0 */
  tcpwm_1_interrupts_1_IRQn         =  99,      /*!<  99 [Active] TCPWM #1, Counter #1 */
  tcpwm_1_interrupts_2_IRQn         = 100,      /*!< 100 [Active] TCPWM #1, Counter #2 */
  tcpwm_1_interrupts_3_IRQn         = 101,      /*!< 101 [Active] TCPWM #1, Counter #3 */
  tcpwm_1_interrupts_4_IRQn         = 102,      /*!< 102 [Active] TCPWM #1, Counter #4 */
  tcpwm_1_interrupts_5_IRQn         = 103,      /*!< 103 [Active] TCPWM #1, Counter #5 */
  tcpwm_1_interrupts_6_IRQn         = 104,      /*!< 104 [Active] TCPWM #1, Counter #6 */
  tcpwm_1_interrupts_7_IRQn         = 105,      /*!< 105 [Active] TCPWM #1, Counter #7 */
  tcpwm_1_interrupts_8_IRQn         = 106,      /*!< 106 [Active] TCPWM #1, Counter #8 */
  tcpwm_1_interrupts_9_IRQn         = 107,      /*!< 107 [Active] TCPWM #1, Counter #9 */
  tcpwm_1_interrupts_10_IRQn        = 108,      /*!< 108 [Active] TCPWM #1, Counter #10 */
  tcpwm_1_interrupts_11_IRQn        = 109,      /*!< 109 [Active] TCPWM #1, Counter #11 */
  tcpwm_1_interrupts_12_IRQn        = 110,      /*!< 110 [Active] TCPWM #1, Counter #12 */
  tcpwm_1_interrupts_13_IRQn        = 111,      /*!< 111 [Active] TCPWM #1, Counter #13 */
  tcpwm_1_interrupts_14_IRQn        = 112,      /*!< 112 [Active] TCPWM #1, Counter #14 */
  tcpwm_1_interrupts_15_IRQn        = 113,      /*!< 113 [Active] TCPWM #1, Counter #15 */
  tcpwm_1_interrupts_16_IRQn        = 114,      /*!< 114 [Active] TCPWM #1, Counter #16 */
  tcpwm_1_interrupts_17_IRQn        = 115,      /*!< 115 [Active] TCPWM #1, Counter #17 */
  tcpwm_1_interrupts_18_IRQn        = 116,      /*!< 116 [Active] TCPWM #1, Counter #18 */
  tcpwm_1_interrupts_19_IRQn        = 117,      /*!< 117 [Active] TCPWM #1, Counter #19 */
  tcpwm_1_interrupts_20_IRQn        = 118,      /*!< 118 [Active] TCPWM #1, Counter #20 */
  tcpwm_1_interrupts_21_IRQn        = 119,      /*!< 119 [Active] TCPWM #1, Counter #21 */
  tcpwm_1_interrupts_22_IRQn        = 120,      /*!< 120 [Active] TCPWM #1, Counter #22 */
  tcpwm_1_interrupts_23_IRQn        = 121,      /*!< 121 [Active] TCPWM #1, Counter #23 */
  udb_interrupts_0_IRQn             = 122,      /*!< 122 [Active] UDB Interrupt #0 */
  udb_interrupts_1_IRQn             = 123,      /*!< 123 [Active] UDB Interrupt #1 */
  udb_interrupts_2_IRQn             = 124,      /*!< 124 [Active] UDB Interrupt #2 */
  udb_interrupts_3_IRQn             = 125,      /*!< 125 [Active] UDB Interrupt #3 */
  udb_interrupts_4_IRQn             = 126,      /*!< 126 [Active] UDB Interrupt #4 */
  udb_interrupts_5_IRQn             = 127,      /*!< 127 [Active] UDB Interrupt #5 */
  udb_interrupts_6_IRQn             = 128,      /*!< 128 [Active] UDB Interrupt #6 */
  udb_interrupts_7_IRQn             = 129,      /*!< 129 [Active] UDB Interrupt #7 */
  udb_interrupts_8_IRQn             = 130,      /*!< 130 [Active] UDB Interrupt #8 */
  udb_interrupts_9_IRQn             = 131,      /*!< 131 [Active] UDB Interrupt #9 */
  udb_interrupts_10_IRQn            = 132,      /*!< 132 [Active] UDB Interrupt #10 */
  udb_interrupts_11_IRQn            = 133,      /*!< 133 [Active] UDB Interrupt #11 */
  udb_interrupts_12_IRQn            = 134,      /*!< 134 [Active] UDB Interrupt #12 */
  udb_interrupts_13_IRQn            = 135,      /*!< 135 [Active] UDB Interrupt #13 */
  udb_interrupts_14_IRQn            = 136,      /*!< 136 [Active] UDB Interrupt #14 */
  udb_interrupts_15_IRQn            = 137,      /*!< 137 [Active] UDB Interrupt #15 */
  pass_interrupt_sar_IRQn           = 138,      /*!< 138 [Active] SAR ADC interrupt */
  audioss_interrupt_i2s_IRQn        = 139,      /*!< 139 [Active] I2S Audio interrupt */
  audioss_interrupt_pdm_IRQn        = 140,      /*!< 140 [Active] PDM/PCM Audio interrupt */
  profile_interrupt_IRQn            = 141,      /*!< 141 [Active] Energy Profiler interrupt */
  smif_interrupt_IRQn               = 142,      /*!< 142 [Active] Serial Memory Interface interrupt */
  usb_interrupt_hi_IRQn             = 143,      /*!< 143 [Active] USB Interrupt */
  usb_interrupt_med_IRQn            = 144,      /*!< 144 [Active] USB Interrupt */
  usb_interrupt_lo_IRQn             = 145,      /*!< 145 [Active] USB Interrupt */
  pass_interrupt_dacs_IRQn          = 146,      /*!< 146 [Active] Consolidated interrrupt for all DACs */
  unconnected_IRQn                  = 240       /*!< 240 Unconnected */
} IRQn_Type;


/*******************************************************************************
*                    Processor and Core Peripheral Section
*******************************************************************************/

/* Configuration of the ARM Cortex-M4 Processor and Core Peripherals */
#define __CM4_REV                       0x0001U /*!< CM4 Core Revision */
#define __NVIC_PRIO_BITS                3       /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig          0       /*!< Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT                  1       /*!< Set to 1 if CPU supports Vector Table Offset Register */
#define __MPU_PRESENT                   1       /*!< MPU present or not */
#define __FPU_PRESENT                   1       /*!< FPU present or not */
#define __CM0P_PRESENT                  0       /*!< CM0P present or not */

/** \} Configuration_of_CMSIS */

/* Memory Blocks */
#define CY_ROM_BASE                     0x00000000UL
#define CY_ROM_SIZE                     0x00020000UL
#define CY_SRAM0_BASE                   0x08000000UL
#define CY_SRAM0_SIZE                   0x00020000UL
#define CY_FLASH_BASE                   0x10000000UL
#define CY_FLASH_SIZE                   0x00080000UL
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
#define CY_IP_MXSCB_INSTANCES           9u
#define CY_IP_MXSCB_VERSION             1u
#define CY_IP_MXPERI                    1u
#define CY_IP_MXPERI_INSTANCES          1u
#define CY_IP_MXPERI_VERSION            1u
#define CY_IP_MXPERI_TR                 1u
#define CY_IP_MXPERI_TR_INSTANCES       1u
#define CY_IP_MXPERI_TR_VERSION         1u
#define CY_IP_M4CPUSS                   1u
#define CY_IP_M4CPUSS_INSTANCES         1u
#define CY_IP_M4CPUSS_VERSION           1u
#define CY_IP_M4CPUSS_DMA               1u
#define CY_IP_M4CPUSS_DMA_INSTANCES     2u
#define CY_IP_M4CPUSS_DMA_VERSION       1u
#define CY_IP_MXAUDIOSS                 1u
#define CY_IP_MXAUDIOSS_INSTANCES       1u
#define CY_IP_MXAUDIOSS_VERSION         1u
#define CY_IP_MXLPCOMP                  1u
#define CY_IP_MXLPCOMP_INSTANCES        1u
#define CY_IP_MXLPCOMP_VERSION          1u
#define CY_IP_MXS40PASS                 1u
#define CY_IP_MXS40PASS_INSTANCES       1u
#define CY_IP_MXS40PASS_VERSION         1u
#define CY_IP_MXS40PASS_SAR             1u
#define CY_IP_MXS40PASS_SAR_INSTANCES   16u
#define CY_IP_MXS40PASS_SAR_VERSION     1u
#define CY_IP_MXS40PASS_CTDAC           1u
#define CY_IP_MXS40PASS_CTDAC_INSTANCES 1u
#define CY_IP_MXS40PASS_CTDAC_VERSION   1u
#define CY_IP_MXS40PASS_CTB             1u
#define CY_IP_MXS40PASS_CTB_INSTANCES   1u
#define CY_IP_MXS40PASS_CTB_VERSION     1u
#define CY_IP_MXSMIF                    1u
#define CY_IP_MXSMIF_INSTANCES          1u
#define CY_IP_MXSMIF_VERSION            1u
#define CY_IP_MXUSBFS                   1u
#define CY_IP_MXUSBFS_INSTANCES         1u
#define CY_IP_MXUSBFS_VERSION           1u
#define CY_IP_MXS40IOSS                 1u
#define CY_IP_MXS40IOSS_INSTANCES       1u
#define CY_IP_MXS40IOSS_VERSION         1u
#define CY_IP_MXEFUSE                   1u
#define CY_IP_MXEFUSE_INSTANCES         1u
#define CY_IP_MXEFUSE_VERSION           1u
#define CY_IP_MXUDB                     1u
#define CY_IP_MXUDB_INSTANCES           1u
#define CY_IP_MXUDB_VERSION             1u
#define CY_IP_MXPROFILE                 1u
#define CY_IP_MXPROFILE_INSTANCES       1u
#define CY_IP_MXPROFILE_VERSION         1u

#include "psoc6able2_config.h"
#include "gpio_psoc6able2_124_bga.h"

#define CY_DEVICE_PSOC6ABLE2
#define CY_SILICON_ID                   0xE2142100UL
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

#define PERI_BASE                               0x40010000UL
#define PERI_PPU_GR_MMIO0_BASE                  0x40015000UL
#define PERI_PPU_GR_MMIO1_BASE                  0x40015040UL
#define PERI_PPU_GR_MMIO2_BASE                  0x40015080UL
#define PERI_PPU_GR_MMIO3_BASE                  0x400150C0UL
#define PERI_PPU_GR_MMIO4_BASE                  0x40015100UL
#define PERI_PPU_GR_MMIO6_BASE                  0x40015180UL
#define PERI_PPU_GR_MMIO9_BASE                  0x40015240UL
#define PERI_PPU_GR_MMIO10_BASE                 0x40015280UL
#define PERI_GR_PPU_SL_PERI_GR1_BASE            0x40100000UL
#define PERI_GR_PPU_SL_CRYPTO_BASE              0x40100040UL
#define PERI_GR_PPU_SL_PERI_GR2_BASE            0x40200000UL
#define PERI_GR_PPU_SL_CPUSS_BASE               0x40200040UL
#define PERI_GR_PPU_SL_FAULT_BASE               0x40200080UL
#define PERI_GR_PPU_SL_IPC_BASE                 0x402000C0UL
#define PERI_GR_PPU_SL_PROT_BASE                0x40200100UL
#define PERI_GR_PPU_SL_FLASHC_BASE              0x40200140UL
#define PERI_GR_PPU_SL_SRSS_BASE                0x40200180UL
#define PERI_GR_PPU_SL_BACKUP_BASE              0x402001C0UL
#define PERI_GR_PPU_SL_DW0_BASE                 0x40200200UL
#define PERI_GR_PPU_SL_DW1_BASE                 0x40200240UL
#define PERI_GR_PPU_SL_EFUSE_BASE               0x40200300UL
#define PERI_GR_PPU_SL_PROFILE_BASE             0x40200340UL
#define PERI_GR_PPU_RG_IPC_STRUCT0_BASE         0x40201000UL
#define PERI_GR_PPU_RG_IPC_STRUCT1_BASE         0x40201040UL
#define PERI_GR_PPU_RG_IPC_STRUCT2_BASE         0x40201080UL
#define PERI_GR_PPU_RG_IPC_STRUCT3_BASE         0x402010C0UL
#define PERI_GR_PPU_RG_IPC_STRUCT4_BASE         0x40201100UL
#define PERI_GR_PPU_RG_IPC_STRUCT5_BASE         0x40201140UL
#define PERI_GR_PPU_RG_IPC_STRUCT6_BASE         0x40201180UL
#define PERI_GR_PPU_RG_IPC_STRUCT7_BASE         0x402011C0UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT0_BASE    0x40201200UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT1_BASE    0x40201240UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT2_BASE    0x40201280UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT3_BASE    0x402012C0UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT4_BASE    0x40201300UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT5_BASE    0x40201340UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT6_BASE    0x40201380UL
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT7_BASE    0x402013C0UL
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT0_BASE   0x40201400UL
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT1_BASE   0x40201440UL
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT2_BASE   0x40201480UL
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT3_BASE   0x402014C0UL
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT0_BASE   0x40201500UL
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT1_BASE   0x40201540UL
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT2_BASE   0x40201580UL
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT3_BASE   0x402015C0UL
#define PERI_GR_PPU_RG_SMPU_BASE                0x40201600UL
#define PERI_GR_PPU_RG_MPU_CM0P_BASE            0x40201640UL
#define PERI_GR_PPU_RG_MPU_CRYPTO_BASE          0x40201680UL
#define PERI_GR_PPU_RG_MPU_CM4_BASE             0x402016C0UL
#define PERI_GR_PPU_RG_MPU_TC_BASE              0x40201700UL
#define PERI_GR_PPU_SL_PERI_GR3_BASE            0x40300000UL
#define PERI_GR_PPU_SL_HSIOM_BASE               0x40300040UL
#define PERI_GR_PPU_SL_GPIO_BASE                0x40300080UL
#define PERI_GR_PPU_SL_SMARTIO_BASE             0x403000C0UL
#define PERI_GR_PPU_SL_UDB_BASE                 0x40300100UL
#define PERI_GR_PPU_SL_LPCOMP_BASE              0x40300140UL
#define PERI_GR_PPU_SL_CSD_BASE                 0x40300180UL
#define PERI_GR_PPU_SL_TCPWM0_BASE              0x40300200UL
#define PERI_GR_PPU_SL_TCPWM1_BASE              0x40300240UL
#define PERI_GR_PPU_SL_LCD_BASE                 0x40300280UL
#define PERI_GR_PPU_SL_BLE_BASE                 0x403002C0UL
#define PERI_GR_PPU_SL_USBFS_BASE               0x40300300UL
#define PERI_GR_PPU_SL_PERI_GR4_BASE            0x40400000UL
#define PERI_GR_PPU_SL_SMIF_BASE                0x40400080UL
#define PERI_GR_PPU_SL_PERI_GR6_BASE            0x40600000UL
#define PERI_GR_PPU_SL_SCB0_BASE                0x40600040UL
#define PERI_GR_PPU_SL_SCB1_BASE                0x40600080UL
#define PERI_GR_PPU_SL_SCB2_BASE                0x406000C0UL
#define PERI_GR_PPU_SL_SCB3_BASE                0x40600100UL
#define PERI_GR_PPU_SL_SCB4_BASE                0x40600140UL
#define PERI_GR_PPU_SL_SCB5_BASE                0x40600180UL
#define PERI_GR_PPU_SL_SCB6_BASE                0x406001C0UL
#define PERI_GR_PPU_SL_SCB7_BASE                0x40600200UL
#define PERI_GR_PPU_SL_SCB8_BASE                0x40600240UL
#define PERI_GR_PPU_SL_PERI_GR9_BASE            0x41000000UL
#define PERI_GR_PPU_SL_PASS_BASE                0x41000040UL
#define PERI_GR_PPU_SL_PERI_GR10_BASE           0x42A00000UL
#define PERI_GR_PPU_SL_I2S_BASE                 0x42A00040UL
#define PERI_GR_PPU_SL_PDM_BASE                 0x42A00080UL
#define PERI                                    ((PERI_Type*) PERI_BASE)                                          /* 0x40010000 */
#define PERI_GR0                                ((PERI_GR_Type*) &PERI->GR[0])                                    /* 0x40010000 */
#define PERI_GR1                                ((PERI_GR_Type*) &PERI->GR[1])                                    /* 0x40010040 */
#define PERI_GR2                                ((PERI_GR_Type*) &PERI->GR[2])                                    /* 0x40010080 */
#define PERI_GR3                                ((PERI_GR_Type*) &PERI->GR[3])                                    /* 0x400100C0 */
#define PERI_GR4                                ((PERI_GR_Type*) &PERI->GR[4])                                    /* 0x40010100 */
#define PERI_GR6                                ((PERI_GR_Type*) &PERI->GR[6])                                    /* 0x40010180 */
#define PERI_GR9                                ((PERI_GR_Type*) &PERI->GR[9])                                    /* 0x40010240 */
#define PERI_GR10                               ((PERI_GR_Type*) &PERI->GR[10])                                   /* 0x40010280 */
#define PERI_TR_GR0                             ((PERI_TR_GR_Type*) &PERI->TR_GR[0])                              /* 0x40012000 */
#define PERI_TR_GR1                             ((PERI_TR_GR_Type*) &PERI->TR_GR[1])                              /* 0x40012200 */
#define PERI_TR_GR2                             ((PERI_TR_GR_Type*) &PERI->TR_GR[2])                              /* 0x40012400 */
#define PERI_TR_GR3                             ((PERI_TR_GR_Type*) &PERI->TR_GR[3])                              /* 0x40012600 */
#define PERI_TR_GR4                             ((PERI_TR_GR_Type*) &PERI->TR_GR[4])                              /* 0x40012800 */
#define PERI_TR_GR5                             ((PERI_TR_GR_Type*) &PERI->TR_GR[5])                              /* 0x40012A00 */
#define PERI_TR_GR6                             ((PERI_TR_GR_Type*) &PERI->TR_GR[6])                              /* 0x40012C00 */
#define PERI_TR_GR7                             ((PERI_TR_GR_Type*) &PERI->TR_GR[7])                              /* 0x40012E00 */
#define PERI_TR_GR8                             ((PERI_TR_GR_Type*) &PERI->TR_GR[8])                              /* 0x40013000 */
#define PERI_TR_GR9                             ((PERI_TR_GR_Type*) &PERI->TR_GR[9])                              /* 0x40013200 */
#define PERI_TR_GR10                            ((PERI_TR_GR_Type*) &PERI->TR_GR[10])                             /* 0x40013400 */
#define PERI_TR_GR11                            ((PERI_TR_GR_Type*) &PERI->TR_GR[11])                             /* 0x40013600 */
#define PERI_TR_GR12                            ((PERI_TR_GR_Type*) &PERI->TR_GR[12])                             /* 0x40013800 */
#define PERI_TR_GR13                            ((PERI_TR_GR_Type*) &PERI->TR_GR[13])                             /* 0x40013A00 */
#define PERI_TR_GR14                            ((PERI_TR_GR_Type*) &PERI->TR_GR[14])                             /* 0x40013C00 */
#define PERI_PPU_PR0                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[0])                            /* 0x40014000 */
#define PERI_PPU_PR1                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[1])                            /* 0x40014040 */
#define PERI_PPU_PR2                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[2])                            /* 0x40014080 */
#define PERI_PPU_PR3                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[3])                            /* 0x400140C0 */
#define PERI_PPU_PR4                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[4])                            /* 0x40014100 */
#define PERI_PPU_PR5                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[5])                            /* 0x40014140 */
#define PERI_PPU_PR6                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[6])                            /* 0x40014180 */
#define PERI_PPU_PR7                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[7])                            /* 0x400141C0 */
#define PERI_PPU_PR8                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[8])                            /* 0x40014200 */
#define PERI_PPU_PR9                            ((PERI_PPU_PR_Type*) &PERI->PPU_PR[9])                            /* 0x40014240 */
#define PERI_PPU_PR10                           ((PERI_PPU_PR_Type*) &PERI->PPU_PR[10])                           /* 0x40014280 */
#define PERI_PPU_PR11                           ((PERI_PPU_PR_Type*) &PERI->PPU_PR[11])                           /* 0x400142C0 */
#define PERI_PPU_PR12                           ((PERI_PPU_PR_Type*) &PERI->PPU_PR[12])                           /* 0x40014300 */
#define PERI_PPU_PR13                           ((PERI_PPU_PR_Type*) &PERI->PPU_PR[13])                           /* 0x40014340 */
#define PERI_PPU_PR14                           ((PERI_PPU_PR_Type*) &PERI->PPU_PR[14])                           /* 0x40014380 */
#define PERI_PPU_PR15                           ((PERI_PPU_PR_Type*) &PERI->PPU_PR[15])                           /* 0x400143C0 */
#define PERI_PPU_GR0                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[0])                            /* 0x40015000 */
#define PERI_PPU_GR1                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[1])                            /* 0x40015040 */
#define PERI_PPU_GR2                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[2])                            /* 0x40015080 */
#define PERI_PPU_GR3                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[3])                            /* 0x400150C0 */
#define PERI_PPU_GR4                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[4])                            /* 0x40015100 */
#define PERI_PPU_GR6                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[6])                            /* 0x40015180 */
#define PERI_PPU_GR9                            ((PERI_PPU_GR_Type*) &PERI->PPU_GR[9])                            /* 0x40015240 */
#define PERI_PPU_GR10                           ((PERI_PPU_GR_Type*) &PERI->PPU_GR[10])                           /* 0x40015280 */
#define PERI_PPU_GR_MMIO0                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO0_BASE)                      /* 0x40015000 */
#define PERI_PPU_GR_MMIO1                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO1_BASE)                      /* 0x40015040 */
#define PERI_PPU_GR_MMIO2                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO2_BASE)                      /* 0x40015080 */
#define PERI_PPU_GR_MMIO3                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO3_BASE)                      /* 0x400150C0 */
#define PERI_PPU_GR_MMIO4                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO4_BASE)                      /* 0x40015100 */
#define PERI_PPU_GR_MMIO6                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO6_BASE)                      /* 0x40015180 */
#define PERI_PPU_GR_MMIO9                       ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO9_BASE)                      /* 0x40015240 */
#define PERI_PPU_GR_MMIO10                      ((PERI_PPU_GR_Type*) PERI_PPU_GR_MMIO10_BASE)                     /* 0x40015280 */
#define PERI_GR_PPU_SL_PERI_GR1                 ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR1_BASE)             /* 0x40100000 */
#define PERI_GR_PPU_SL_CRYPTO                   ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_CRYPTO_BASE)               /* 0x40100040 */
#define PERI_GR_PPU_SL_PERI_GR2                 ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR2_BASE)             /* 0x40200000 */
#define PERI_GR_PPU_SL_CPUSS                    ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_CPUSS_BASE)                /* 0x40200040 */
#define PERI_GR_PPU_SL_FAULT                    ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_FAULT_BASE)                /* 0x40200080 */
#define PERI_GR_PPU_SL_IPC                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_IPC_BASE)                  /* 0x402000C0 */
#define PERI_GR_PPU_SL_PROT                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PROT_BASE)                 /* 0x40200100 */
#define PERI_GR_PPU_SL_FLASHC                   ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_FLASHC_BASE)               /* 0x40200140 */
#define PERI_GR_PPU_SL_SRSS                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SRSS_BASE)                 /* 0x40200180 */
#define PERI_GR_PPU_SL_BACKUP                   ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_BACKUP_BASE)               /* 0x402001C0 */
#define PERI_GR_PPU_SL_DW0                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_DW0_BASE)                  /* 0x40200200 */
#define PERI_GR_PPU_SL_DW1                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_DW1_BASE)                  /* 0x40200240 */
#define PERI_GR_PPU_SL_EFUSE                    ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_EFUSE_BASE)                /* 0x40200300 */
#define PERI_GR_PPU_SL_PROFILE                  ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PROFILE_BASE)              /* 0x40200340 */
#define PERI_GR_PPU_RG_IPC_STRUCT0              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT0_BASE)          /* 0x40201000 */
#define PERI_GR_PPU_RG_IPC_STRUCT1              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT1_BASE)          /* 0x40201040 */
#define PERI_GR_PPU_RG_IPC_STRUCT2              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT2_BASE)          /* 0x40201080 */
#define PERI_GR_PPU_RG_IPC_STRUCT3              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT3_BASE)          /* 0x402010C0 */
#define PERI_GR_PPU_RG_IPC_STRUCT4              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT4_BASE)          /* 0x40201100 */
#define PERI_GR_PPU_RG_IPC_STRUCT5              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT5_BASE)          /* 0x40201140 */
#define PERI_GR_PPU_RG_IPC_STRUCT6              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT6_BASE)          /* 0x40201180 */
#define PERI_GR_PPU_RG_IPC_STRUCT7              ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_STRUCT7_BASE)          /* 0x402011C0 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT0         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT0_BASE)     /* 0x40201200 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT1         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT1_BASE)     /* 0x40201240 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT2         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT2_BASE)     /* 0x40201280 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT3         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT3_BASE)     /* 0x402012C0 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT4         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT4_BASE)     /* 0x40201300 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT5         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT5_BASE)     /* 0x40201340 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT6         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT6_BASE)     /* 0x40201380 */
#define PERI_GR_PPU_RG_IPC_INTR_STRUCT7         ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_IPC_INTR_STRUCT7_BASE)     /* 0x402013C0 */
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT0        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW0_DW_CH_STRUCT0_BASE)    /* 0x40201400 */
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT1        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW0_DW_CH_STRUCT1_BASE)    /* 0x40201440 */
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT2        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW0_DW_CH_STRUCT2_BASE)    /* 0x40201480 */
#define PERI_GR_PPU_RG_DW0_DW_CH_STRUCT3        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW0_DW_CH_STRUCT3_BASE)    /* 0x402014C0 */
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT0        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW1_DW_CH_STRUCT0_BASE)    /* 0x40201500 */
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT1        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW1_DW_CH_STRUCT1_BASE)    /* 0x40201540 */
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT2        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW1_DW_CH_STRUCT2_BASE)    /* 0x40201580 */
#define PERI_GR_PPU_RG_DW1_DW_CH_STRUCT3        ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_DW1_DW_CH_STRUCT3_BASE)    /* 0x402015C0 */
#define PERI_GR_PPU_RG_SMPU                     ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_SMPU_BASE)                 /* 0x40201600 */
#define PERI_GR_PPU_RG_MPU_CM0P                 ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_MPU_CM0P_BASE)             /* 0x40201640 */
#define PERI_GR_PPU_RG_MPU_CRYPTO               ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_MPU_CRYPTO_BASE)           /* 0x40201680 */
#define PERI_GR_PPU_RG_MPU_CM4                  ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_MPU_CM4_BASE)              /* 0x402016C0 */
#define PERI_GR_PPU_RG_MPU_TC                   ((PERI_GR_PPU_RG_Type*) PERI_GR_PPU_RG_MPU_TC_BASE)               /* 0x40201700 */
#define PERI_GR_PPU_SL_PERI_GR3                 ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR3_BASE)             /* 0x40300000 */
#define PERI_GR_PPU_SL_HSIOM                    ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_HSIOM_BASE)                /* 0x40300040 */
#define PERI_GR_PPU_SL_GPIO                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_GPIO_BASE)                 /* 0x40300080 */
#define PERI_GR_PPU_SL_SMARTIO                  ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SMARTIO_BASE)              /* 0x403000C0 */
#define PERI_GR_PPU_SL_UDB                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_UDB_BASE)                  /* 0x40300100 */
#define PERI_GR_PPU_SL_LPCOMP                   ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_LPCOMP_BASE)               /* 0x40300140 */
#define PERI_GR_PPU_SL_CSD                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_CSD_BASE)                  /* 0x40300180 */
#define PERI_GR_PPU_SL_TCPWM0                   ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_TCPWM0_BASE)               /* 0x40300200 */
#define PERI_GR_PPU_SL_TCPWM1                   ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_TCPWM1_BASE)               /* 0x40300240 */
#define PERI_GR_PPU_SL_LCD                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_LCD_BASE)                  /* 0x40300280 */
#define PERI_GR_PPU_SL_BLE                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_BLE_BASE)                  /* 0x403002C0 */
#define PERI_GR_PPU_SL_USBFS                    ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_USBFS_BASE)                /* 0x40300300 */
#define PERI_GR_PPU_SL_PERI_GR4                 ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR4_BASE)             /* 0x40400000 */
#define PERI_GR_PPU_SL_SMIF                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SMIF_BASE)                 /* 0x40400080 */
#define PERI_GR_PPU_SL_PERI_GR6                 ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR6_BASE)             /* 0x40600000 */
#define PERI_GR_PPU_SL_SCB0                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB0_BASE)                 /* 0x40600040 */
#define PERI_GR_PPU_SL_SCB1                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB1_BASE)                 /* 0x40600080 */
#define PERI_GR_PPU_SL_SCB2                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB2_BASE)                 /* 0x406000C0 */
#define PERI_GR_PPU_SL_SCB3                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB3_BASE)                 /* 0x40600100 */
#define PERI_GR_PPU_SL_SCB4                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB4_BASE)                 /* 0x40600140 */
#define PERI_GR_PPU_SL_SCB5                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB5_BASE)                 /* 0x40600180 */
#define PERI_GR_PPU_SL_SCB6                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB6_BASE)                 /* 0x406001C0 */
#define PERI_GR_PPU_SL_SCB7                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB7_BASE)                 /* 0x40600200 */
#define PERI_GR_PPU_SL_SCB8                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_SCB8_BASE)                 /* 0x40600240 */
#define PERI_GR_PPU_SL_PERI_GR9                 ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR9_BASE)             /* 0x41000000 */
#define PERI_GR_PPU_SL_PASS                     ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PASS_BASE)                 /* 0x41000040 */
#define PERI_GR_PPU_SL_PERI_GR10                ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PERI_GR10_BASE)            /* 0x42A00000 */
#define PERI_GR_PPU_SL_I2S                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_I2S_BASE)                  /* 0x42A00040 */
#define PERI_GR_PPU_SL_PDM                      ((PERI_GR_PPU_SL_Type*) PERI_GR_PPU_SL_PDM_BASE)                  /* 0x42A00080 */

/*******************************************************************************
*                                    CPUSS
*******************************************************************************/

#define CPUSS_BASE                              0x40210000UL
#define CPUSS                                   ((CPUSS_Type*) CPUSS_BASE)                                        /* 0x40210000 */

/*******************************************************************************
*                                    FAULT
*******************************************************************************/

#define FAULT_BASE                              0x40220000UL
#define FAULT                                   ((FAULT_Type*) FAULT_BASE)                                        /* 0x40220000 */
#define FAULT_STRUCT0                           ((FAULT_STRUCT_Type*) &FAULT->STRUCT[0])                          /* 0x40220000 */
#define FAULT_STRUCT1                           ((FAULT_STRUCT_Type*) &FAULT->STRUCT[1])                          /* 0x40220100 */

/*******************************************************************************
*                                     IPC
*******************************************************************************/

#define IPC_BASE                                0x40230000UL
#define IPC                                     ((IPC_Type*) IPC_BASE)                                            /* 0x40230000 */
#define IPC_STRUCT0                             ((IPC_STRUCT_Type*) &IPC->STRUCT[0])                              /* 0x40230000 */
#define IPC_STRUCT1                             ((IPC_STRUCT_Type*) &IPC->STRUCT[1])                              /* 0x40230020 */
#define IPC_STRUCT2                             ((IPC_STRUCT_Type*) &IPC->STRUCT[2])                              /* 0x40230040 */
#define IPC_STRUCT3                             ((IPC_STRUCT_Type*) &IPC->STRUCT[3])                              /* 0x40230060 */
#define IPC_STRUCT4                             ((IPC_STRUCT_Type*) &IPC->STRUCT[4])                              /* 0x40230080 */
#define IPC_STRUCT5                             ((IPC_STRUCT_Type*) &IPC->STRUCT[5])                              /* 0x402300A0 */
#define IPC_STRUCT6                             ((IPC_STRUCT_Type*) &IPC->STRUCT[6])                              /* 0x402300C0 */
#define IPC_STRUCT7                             ((IPC_STRUCT_Type*) &IPC->STRUCT[7])                              /* 0x402300E0 */
#define IPC_STRUCT8                             ((IPC_STRUCT_Type*) &IPC->STRUCT[8])                              /* 0x40230100 */
#define IPC_STRUCT9                             ((IPC_STRUCT_Type*) &IPC->STRUCT[9])                              /* 0x40230120 */
#define IPC_STRUCT10                            ((IPC_STRUCT_Type*) &IPC->STRUCT[10])                             /* 0x40230140 */
#define IPC_STRUCT11                            ((IPC_STRUCT_Type*) &IPC->STRUCT[11])                             /* 0x40230160 */
#define IPC_STRUCT12                            ((IPC_STRUCT_Type*) &IPC->STRUCT[12])                             /* 0x40230180 */
#define IPC_STRUCT13                            ((IPC_STRUCT_Type*) &IPC->STRUCT[13])                             /* 0x402301A0 */
#define IPC_STRUCT14                            ((IPC_STRUCT_Type*) &IPC->STRUCT[14])                             /* 0x402301C0 */
#define IPC_STRUCT15                            ((IPC_STRUCT_Type*) &IPC->STRUCT[15])                             /* 0x402301E0 */
#define IPC_INTR_STRUCT0                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[0])                    /* 0x40231000 */
#define IPC_INTR_STRUCT1                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[1])                    /* 0x40231020 */
#define IPC_INTR_STRUCT2                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[2])                    /* 0x40231040 */
#define IPC_INTR_STRUCT3                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[3])                    /* 0x40231060 */
#define IPC_INTR_STRUCT4                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[4])                    /* 0x40231080 */
#define IPC_INTR_STRUCT5                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[5])                    /* 0x402310A0 */
#define IPC_INTR_STRUCT6                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[6])                    /* 0x402310C0 */
#define IPC_INTR_STRUCT7                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[7])                    /* 0x402310E0 */
#define IPC_INTR_STRUCT8                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[8])                    /* 0x40231100 */
#define IPC_INTR_STRUCT9                        ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[9])                    /* 0x40231120 */
#define IPC_INTR_STRUCT10                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[10])                   /* 0x40231140 */
#define IPC_INTR_STRUCT11                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[11])                   /* 0x40231160 */
#define IPC_INTR_STRUCT12                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[12])                   /* 0x40231180 */
#define IPC_INTR_STRUCT13                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[13])                   /* 0x402311A0 */
#define IPC_INTR_STRUCT14                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[14])                   /* 0x402311C0 */
#define IPC_INTR_STRUCT15                       ((IPC_INTR_STRUCT_Type*) &IPC->INTR_STRUCT[15])                   /* 0x402311E0 */

/*******************************************************************************
*                                     PROT
*******************************************************************************/

#define PROT_BASE                               0x40240000UL
#define PROT                                    ((PROT_Type*) PROT_BASE)                                          /* 0x40240000 */
#define PROT_SMPU_SMPU_STRUCT0                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[0])        /* 0x40242000 */
#define PROT_SMPU_SMPU_STRUCT1                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[1])        /* 0x40242040 */
#define PROT_SMPU_SMPU_STRUCT2                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[2])        /* 0x40242080 */
#define PROT_SMPU_SMPU_STRUCT3                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[3])        /* 0x402420C0 */
#define PROT_SMPU_SMPU_STRUCT4                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[4])        /* 0x40242100 */
#define PROT_SMPU_SMPU_STRUCT5                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[5])        /* 0x40242140 */
#define PROT_SMPU_SMPU_STRUCT6                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[6])        /* 0x40242180 */
#define PROT_SMPU_SMPU_STRUCT7                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[7])        /* 0x402421C0 */
#define PROT_SMPU_SMPU_STRUCT8                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[8])        /* 0x40242200 */
#define PROT_SMPU_SMPU_STRUCT9                  ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[9])        /* 0x40242240 */
#define PROT_SMPU_SMPU_STRUCT10                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[10])       /* 0x40242280 */
#define PROT_SMPU_SMPU_STRUCT11                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[11])       /* 0x402422C0 */
#define PROT_SMPU_SMPU_STRUCT12                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[12])       /* 0x40242300 */
#define PROT_SMPU_SMPU_STRUCT13                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[13])       /* 0x40242340 */
#define PROT_SMPU_SMPU_STRUCT14                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[14])       /* 0x40242380 */
#define PROT_SMPU_SMPU_STRUCT15                 ((PROT_SMPU_SMPU_STRUCT_Type*) &PROT->SMPU.SMPU_STRUCT[15])       /* 0x402423C0 */
#define PROT_SMPU                               ((PROT_SMPU_Type*) &PROT->SMPU)                                   /* 0x40240000 */
#define PROT_MPU1_MPU_STRUCT0                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[0])       /* 0x40244600 */
#define PROT_MPU1_MPU_STRUCT1                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[1])       /* 0x40244620 */
#define PROT_MPU1_MPU_STRUCT2                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[2])       /* 0x40244640 */
#define PROT_MPU1_MPU_STRUCT3                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[3])       /* 0x40244660 */
#define PROT_MPU1_MPU_STRUCT4                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[4])       /* 0x40244680 */
#define PROT_MPU1_MPU_STRUCT5                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[5])       /* 0x402446A0 */
#define PROT_MPU1_MPU_STRUCT6                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[6])       /* 0x402446C0 */
#define PROT_MPU1_MPU_STRUCT7                   ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[1].MPU_STRUCT[7])       /* 0x402446E0 */
#define PROT_MPU15_MPU_STRUCT0                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[0])      /* 0x40247E00 */
#define PROT_MPU15_MPU_STRUCT1                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[1])      /* 0x40247E20 */
#define PROT_MPU15_MPU_STRUCT2                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[2])      /* 0x40247E40 */
#define PROT_MPU15_MPU_STRUCT3                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[3])      /* 0x40247E60 */
#define PROT_MPU15_MPU_STRUCT4                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[4])      /* 0x40247E80 */
#define PROT_MPU15_MPU_STRUCT5                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[5])      /* 0x40247EA0 */
#define PROT_MPU15_MPU_STRUCT6                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[6])      /* 0x40247EC0 */
#define PROT_MPU15_MPU_STRUCT7                  ((PROT_MPU_MPU_STRUCT_Type*) &PROT->CYMPU[15].MPU_STRUCT[7])      /* 0x40247EE0 */
#define PROT_MPU0                               ((PROT_MPU_Type*) &PROT->CYMPU[0])                                /* 0x40244000 */
#define PROT_MPU1                               ((PROT_MPU_Type*) &PROT->CYMPU[1])                                /* 0x40244400 */
#define PROT_MPU2                               ((PROT_MPU_Type*) &PROT->CYMPU[2])                                /* 0x40244800 */
#define PROT_MPU3                               ((PROT_MPU_Type*) &PROT->CYMPU[3])                                /* 0x40244C00 */
#define PROT_MPU4                               ((PROT_MPU_Type*) &PROT->CYMPU[4])                                /* 0x40245000 */
#define PROT_MPU5                               ((PROT_MPU_Type*) &PROT->CYMPU[5])                                /* 0x40245400 */
#define PROT_MPU6                               ((PROT_MPU_Type*) &PROT->CYMPU[6])                                /* 0x40245800 */
#define PROT_MPU7                               ((PROT_MPU_Type*) &PROT->CYMPU[7])                                /* 0x40245C00 */
#define PROT_MPU8                               ((PROT_MPU_Type*) &PROT->CYMPU[8])                                /* 0x40246000 */
#define PROT_MPU9                               ((PROT_MPU_Type*) &PROT->CYMPU[9])                                /* 0x40246400 */
#define PROT_MPU10                              ((PROT_MPU_Type*) &PROT->CYMPU[10])                               /* 0x40246800 */
#define PROT_MPU11                              ((PROT_MPU_Type*) &PROT->CYMPU[11])                               /* 0x40246C00 */
#define PROT_MPU12                              ((PROT_MPU_Type*) &PROT->CYMPU[12])                               /* 0x40247000 */
#define PROT_MPU13                              ((PROT_MPU_Type*) &PROT->CYMPU[13])                               /* 0x40247400 */
#define PROT_MPU14                              ((PROT_MPU_Type*) &PROT->CYMPU[14])                               /* 0x40247800 */
#define PROT_MPU15                              ((PROT_MPU_Type*) &PROT->CYMPU[15])                               /* 0x40247C00 */

/*******************************************************************************
*                                    FLASHC
*******************************************************************************/

#define FLASHC_BASE                             0x40250000UL
#define FLASHC                                  ((FLASHC_Type*) FLASHC_BASE)                                      /* 0x40250000 */
#define FLASHC_FM_CTL                           ((FLASHC_FM_CTL_Type*) &FLASHC->FM_CTL)                           /* 0x4025F000 */

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
#define DW1_BASE                                0x40281000UL
#define DW0                                     ((DW_Type*) DW0_BASE)                                             /* 0x40280000 */
#define DW1                                     ((DW_Type*) DW1_BASE)                                             /* 0x40281000 */
#define DW0_CH_STRUCT0                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[0])                         /* 0x40280800 */
#define DW0_CH_STRUCT1                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[1])                         /* 0x40280820 */
#define DW0_CH_STRUCT2                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[2])                         /* 0x40280840 */
#define DW0_CH_STRUCT3                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[3])                         /* 0x40280860 */
#define DW0_CH_STRUCT4                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[4])                         /* 0x40280880 */
#define DW0_CH_STRUCT5                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[5])                         /* 0x402808A0 */
#define DW0_CH_STRUCT6                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[6])                         /* 0x402808C0 */
#define DW0_CH_STRUCT7                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[7])                         /* 0x402808E0 */
#define DW0_CH_STRUCT8                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[8])                         /* 0x40280900 */
#define DW0_CH_STRUCT9                          ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[9])                         /* 0x40280920 */
#define DW0_CH_STRUCT10                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[10])                        /* 0x40280940 */
#define DW0_CH_STRUCT11                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[11])                        /* 0x40280960 */
#define DW0_CH_STRUCT12                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[12])                        /* 0x40280980 */
#define DW0_CH_STRUCT13                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[13])                        /* 0x402809A0 */
#define DW0_CH_STRUCT14                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[14])                        /* 0x402809C0 */
#define DW0_CH_STRUCT15                         ((DW_CH_STRUCT_Type*) &DW0->CH_STRUCT[15])                        /* 0x402809E0 */
#define DW1_CH_STRUCT0                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[0])                         /* 0x40281800 */
#define DW1_CH_STRUCT1                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[1])                         /* 0x40281820 */
#define DW1_CH_STRUCT2                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[2])                         /* 0x40281840 */
#define DW1_CH_STRUCT3                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[3])                         /* 0x40281860 */
#define DW1_CH_STRUCT4                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[4])                         /* 0x40281880 */
#define DW1_CH_STRUCT5                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[5])                         /* 0x402818A0 */
#define DW1_CH_STRUCT6                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[6])                         /* 0x402818C0 */
#define DW1_CH_STRUCT7                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[7])                         /* 0x402818E0 */
#define DW1_CH_STRUCT8                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[8])                         /* 0x40281900 */
#define DW1_CH_STRUCT9                          ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[9])                         /* 0x40281920 */
#define DW1_CH_STRUCT10                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[10])                        /* 0x40281940 */
#define DW1_CH_STRUCT11                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[11])                        /* 0x40281960 */
#define DW1_CH_STRUCT12                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[12])                        /* 0x40281980 */
#define DW1_CH_STRUCT13                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[13])                        /* 0x402819A0 */
#define DW1_CH_STRUCT14                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[14])                        /* 0x402819C0 */
#define DW1_CH_STRUCT15                         ((DW_CH_STRUCT_Type*) &DW1->CH_STRUCT[15])                        /* 0x402819E0 */

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

#define HSIOM_BASE                              0x40310000UL
#define HSIOM                                   ((HSIOM_Type*) HSIOM_BASE)                                        /* 0x40310000 */
#define HSIOM_PRT0                              ((HSIOM_PRT_Type*) &HSIOM->PRT[0])                                /* 0x40310000 */
#define HSIOM_PRT1                              ((HSIOM_PRT_Type*) &HSIOM->PRT[1])                                /* 0x40310010 */
#define HSIOM_PRT2                              ((HSIOM_PRT_Type*) &HSIOM->PRT[2])                                /* 0x40310020 */
#define HSIOM_PRT3                              ((HSIOM_PRT_Type*) &HSIOM->PRT[3])                                /* 0x40310030 */
#define HSIOM_PRT4                              ((HSIOM_PRT_Type*) &HSIOM->PRT[4])                                /* 0x40310040 */
#define HSIOM_PRT5                              ((HSIOM_PRT_Type*) &HSIOM->PRT[5])                                /* 0x40310050 */
#define HSIOM_PRT6                              ((HSIOM_PRT_Type*) &HSIOM->PRT[6])                                /* 0x40310060 */
#define HSIOM_PRT7                              ((HSIOM_PRT_Type*) &HSIOM->PRT[7])                                /* 0x40310070 */
#define HSIOM_PRT8                              ((HSIOM_PRT_Type*) &HSIOM->PRT[8])                                /* 0x40310080 */
#define HSIOM_PRT9                              ((HSIOM_PRT_Type*) &HSIOM->PRT[9])                                /* 0x40310090 */
#define HSIOM_PRT10                             ((HSIOM_PRT_Type*) &HSIOM->PRT[10])                               /* 0x403100A0 */
#define HSIOM_PRT11                             ((HSIOM_PRT_Type*) &HSIOM->PRT[11])                               /* 0x403100B0 */
#define HSIOM_PRT12                             ((HSIOM_PRT_Type*) &HSIOM->PRT[12])                               /* 0x403100C0 */
#define HSIOM_PRT13                             ((HSIOM_PRT_Type*) &HSIOM->PRT[13])                               /* 0x403100D0 */
#define HSIOM_PRT14                             ((HSIOM_PRT_Type*) &HSIOM->PRT[14])                               /* 0x403100E0 */

/*******************************************************************************
*                                     GPIO
*******************************************************************************/

#define GPIO_BASE                               0x40320000UL
#define GPIO                                    ((GPIO_Type*) GPIO_BASE)                                          /* 0x40320000 */
#define GPIO_PRT0                               ((GPIO_PRT_Type*) &GPIO->PRT[0])                                  /* 0x40320000 */
#define GPIO_PRT1                               ((GPIO_PRT_Type*) &GPIO->PRT[1])                                  /* 0x40320080 */
#define GPIO_PRT2                               ((GPIO_PRT_Type*) &GPIO->PRT[2])                                  /* 0x40320100 */
#define GPIO_PRT3                               ((GPIO_PRT_Type*) &GPIO->PRT[3])                                  /* 0x40320180 */
#define GPIO_PRT4                               ((GPIO_PRT_Type*) &GPIO->PRT[4])                                  /* 0x40320200 */
#define GPIO_PRT5                               ((GPIO_PRT_Type*) &GPIO->PRT[5])                                  /* 0x40320280 */
#define GPIO_PRT6                               ((GPIO_PRT_Type*) &GPIO->PRT[6])                                  /* 0x40320300 */
#define GPIO_PRT7                               ((GPIO_PRT_Type*) &GPIO->PRT[7])                                  /* 0x40320380 */
#define GPIO_PRT8                               ((GPIO_PRT_Type*) &GPIO->PRT[8])                                  /* 0x40320400 */
#define GPIO_PRT9                               ((GPIO_PRT_Type*) &GPIO->PRT[9])                                  /* 0x40320480 */
#define GPIO_PRT10                              ((GPIO_PRT_Type*) &GPIO->PRT[10])                                 /* 0x40320500 */
#define GPIO_PRT11                              ((GPIO_PRT_Type*) &GPIO->PRT[11])                                 /* 0x40320580 */
#define GPIO_PRT12                              ((GPIO_PRT_Type*) &GPIO->PRT[12])                                 /* 0x40320600 */
#define GPIO_PRT13                              ((GPIO_PRT_Type*) &GPIO->PRT[13])                                 /* 0x40320680 */
#define GPIO_PRT14                              ((GPIO_PRT_Type*) &GPIO->PRT[14])                                 /* 0x40320700 */

/*******************************************************************************
*                                   SMARTIO
*******************************************************************************/

#define SMARTIO_BASE                            0x40330000UL
#define SMARTIO                                 ((SMARTIO_Type*) SMARTIO_BASE)                                    /* 0x40330000 */
#define SMARTIO_PRT8                            ((SMARTIO_PRT_Type*) &SMARTIO->PRT[8])                            /* 0x40330800 */
#define SMARTIO_PRT9                            ((SMARTIO_PRT_Type*) &SMARTIO->PRT[9])                            /* 0x40330900 */

/*******************************************************************************
*                                     UDB
*******************************************************************************/

#define UDB_BASE                                0x40340000UL
#define UDB                                     ((UDB_Type*) UDB_BASE)                                            /* 0x40340000 */
#define UDB_WRKONE                              ((UDB_WRKONE_Type*) &UDB->WRKONE)                                 /* 0x40340000 */
#define UDB_WRKMULT                             ((UDB_WRKMULT_Type*) &UDB->WRKMULT)                               /* 0x40341000 */
#define UDB_UDBPAIR0_UDBSNG0                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[0].UDBSNG[0])           /* 0x40342000 */
#define UDB_UDBPAIR0_UDBSNG1                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[0].UDBSNG[1])           /* 0x40342080 */
#define UDB_UDBPAIR1_UDBSNG0                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[1].UDBSNG[0])           /* 0x40342200 */
#define UDB_UDBPAIR1_UDBSNG1                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[1].UDBSNG[1])           /* 0x40342280 */
#define UDB_UDBPAIR2_UDBSNG0                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[2].UDBSNG[0])           /* 0x40342400 */
#define UDB_UDBPAIR2_UDBSNG1                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[2].UDBSNG[1])           /* 0x40342480 */
#define UDB_UDBPAIR3_UDBSNG0                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[3].UDBSNG[0])           /* 0x40342600 */
#define UDB_UDBPAIR3_UDBSNG1                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[3].UDBSNG[1])           /* 0x40342680 */
#define UDB_UDBPAIR4_UDBSNG0                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[4].UDBSNG[0])           /* 0x40342800 */
#define UDB_UDBPAIR4_UDBSNG1                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[4].UDBSNG[1])           /* 0x40342880 */
#define UDB_UDBPAIR5_UDBSNG0                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[5].UDBSNG[0])           /* 0x40342A00 */
#define UDB_UDBPAIR5_UDBSNG1                    ((UDB_UDBPAIR_UDBSNG_Type*) &UDB->UDBPAIR[5].UDBSNG[1])           /* 0x40342A80 */
#define UDB_UDBPAIR0_ROUTE                      ((UDB_UDBPAIR_ROUTE_Type*) &UDB->UDBPAIR[0].ROUTE)                /* 0x40342100 */
#define UDB_UDBPAIR1_ROUTE                      ((UDB_UDBPAIR_ROUTE_Type*) &UDB->UDBPAIR[1].ROUTE)                /* 0x40342300 */
#define UDB_UDBPAIR2_ROUTE                      ((UDB_UDBPAIR_ROUTE_Type*) &UDB->UDBPAIR[2].ROUTE)                /* 0x40342500 */
#define UDB_UDBPAIR3_ROUTE                      ((UDB_UDBPAIR_ROUTE_Type*) &UDB->UDBPAIR[3].ROUTE)                /* 0x40342700 */
#define UDB_UDBPAIR4_ROUTE                      ((UDB_UDBPAIR_ROUTE_Type*) &UDB->UDBPAIR[4].ROUTE)                /* 0x40342900 */
#define UDB_UDBPAIR5_ROUTE                      ((UDB_UDBPAIR_ROUTE_Type*) &UDB->UDBPAIR[5].ROUTE)                /* 0x40342B00 */
#define UDB_UDBPAIR0                            ((UDB_UDBPAIR_Type*) &UDB->UDBPAIR[0])                            /* 0x40342000 */
#define UDB_UDBPAIR1                            ((UDB_UDBPAIR_Type*) &UDB->UDBPAIR[1])                            /* 0x40342200 */
#define UDB_UDBPAIR2                            ((UDB_UDBPAIR_Type*) &UDB->UDBPAIR[2])                            /* 0x40342400 */
#define UDB_UDBPAIR3                            ((UDB_UDBPAIR_Type*) &UDB->UDBPAIR[3])                            /* 0x40342600 */
#define UDB_UDBPAIR4                            ((UDB_UDBPAIR_Type*) &UDB->UDBPAIR[4])                            /* 0x40342800 */
#define UDB_UDBPAIR5                            ((UDB_UDBPAIR_Type*) &UDB->UDBPAIR[5])                            /* 0x40342A00 */
#define UDB_DSI0                                ((UDB_DSI_Type*) &UDB->DSI[0])                                    /* 0x40346000 */
#define UDB_DSI1                                ((UDB_DSI_Type*) &UDB->DSI[1])                                    /* 0x40346080 */
#define UDB_DSI2                                ((UDB_DSI_Type*) &UDB->DSI[2])                                    /* 0x40346100 */
#define UDB_DSI3                                ((UDB_DSI_Type*) &UDB->DSI[3])                                    /* 0x40346180 */
#define UDB_DSI4                                ((UDB_DSI_Type*) &UDB->DSI[4])                                    /* 0x40346200 */
#define UDB_DSI5                                ((UDB_DSI_Type*) &UDB->DSI[5])                                    /* 0x40346280 */
#define UDB_DSI6                                ((UDB_DSI_Type*) &UDB->DSI[6])                                    /* 0x40346300 */
#define UDB_DSI7                                ((UDB_DSI_Type*) &UDB->DSI[7])                                    /* 0x40346380 */
#define UDB_DSI8                                ((UDB_DSI_Type*) &UDB->DSI[8])                                    /* 0x40346400 */
#define UDB_DSI9                                ((UDB_DSI_Type*) &UDB->DSI[9])                                    /* 0x40346480 */
#define UDB_DSI10                               ((UDB_DSI_Type*) &UDB->DSI[10])                                   /* 0x40346500 */
#define UDB_DSI11                               ((UDB_DSI_Type*) &UDB->DSI[11])                                   /* 0x40346580 */
#define UDB_PA0                                 ((UDB_PA_Type*) &UDB->PA[0])                                      /* 0x40347000 */
#define UDB_PA1                                 ((UDB_PA_Type*) &UDB->PA[1])                                      /* 0x40347040 */
#define UDB_PA2                                 ((UDB_PA_Type*) &UDB->PA[2])                                      /* 0x40347080 */
#define UDB_PA3                                 ((UDB_PA_Type*) &UDB->PA[3])                                      /* 0x403470C0 */
#define UDB_PA4                                 ((UDB_PA_Type*) &UDB->PA[4])                                      /* 0x40347100 */
#define UDB_PA5                                 ((UDB_PA_Type*) &UDB->PA[5])                                      /* 0x40347140 */
#define UDB_PA6                                 ((UDB_PA_Type*) &UDB->PA[6])                                      /* 0x40347180 */
#define UDB_PA7                                 ((UDB_PA_Type*) &UDB->PA[7])                                      /* 0x403471C0 */
#define UDB_PA8                                 ((UDB_PA_Type*) &UDB->PA[8])                                      /* 0x40347200 */
#define UDB_PA9                                 ((UDB_PA_Type*) &UDB->PA[9])                                      /* 0x40347240 */
#define UDB_PA10                                ((UDB_PA_Type*) &UDB->PA[10])                                     /* 0x40347280 */
#define UDB_PA11                                ((UDB_PA_Type*) &UDB->PA[11])                                     /* 0x403472C0 */
#define UDB_BCTL                                ((UDB_BCTL_Type*) &UDB->BCTL)                                     /* 0x40347800 */
#define UDB_UDBIF                               ((UDB_UDBIF_Type*) &UDB->UDBIF)                                   /* 0x40347900 */

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
*                                    USBFS
*******************************************************************************/

#define USBFS0_BASE                             0x403F0000UL
#define USBFS0                                  ((USBFS_Type*) USBFS0_BASE)                                       /* 0x403F0000 */
#define USBFS0_USBDEV                           ((USBFS_USBDEV_Type*) &USBFS0->USBDEV)                            /* 0x403F0000 */
#define USBFS0_USBLPM                           ((USBFS_USBLPM_Type*) &USBFS0->USBLPM)                            /* 0x403F2000 */
#define USBFS0_USBHOST                          ((USBFS_USBHOST_Type*) &USBFS0->USBHOST)                          /* 0x403F4000 */

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
*                                     SCB
*******************************************************************************/

#define SCB0_BASE                               0x40610000UL
#define SCB1_BASE                               0x40620000UL
#define SCB2_BASE                               0x40630000UL
#define SCB3_BASE                               0x40640000UL
#define SCB4_BASE                               0x40650000UL
#define SCB5_BASE                               0x40660000UL
#define SCB6_BASE                               0x40670000UL
#define SCB7_BASE                               0x40680000UL
#define SCB8_BASE                               0x40690000UL
#define SCB0                                    ((CySCB_Type*) SCB0_BASE)                                         /* 0x40610000 */
#define SCB1                                    ((CySCB_Type*) SCB1_BASE)                                         /* 0x40620000 */
#define SCB2                                    ((CySCB_Type*) SCB2_BASE)                                         /* 0x40630000 */
#define SCB3                                    ((CySCB_Type*) SCB3_BASE)                                         /* 0x40640000 */
#define SCB4                                    ((CySCB_Type*) SCB4_BASE)                                         /* 0x40650000 */
#define SCB5                                    ((CySCB_Type*) SCB5_BASE)                                         /* 0x40660000 */
#define SCB6                                    ((CySCB_Type*) SCB6_BASE)                                         /* 0x40670000 */
#define SCB7                                    ((CySCB_Type*) SCB7_BASE)                                         /* 0x40680000 */
#define SCB8                                    ((CySCB_Type*) SCB8_BASE)                                         /* 0x40690000 */

/*******************************************************************************
*                                     CTBM
*******************************************************************************/

#define CTBM0_BASE                              0x41100000UL
#define CTBM0                                   ((CTBM_Type*) CTBM0_BASE)                                         /* 0x41100000 */

/*******************************************************************************
*                                    CTDAC
*******************************************************************************/

#define CTDAC0_BASE                             0x41140000UL
#define CTDAC0                                  ((CTDAC_Type*) CTDAC0_BASE)                                       /* 0x41140000 */

/*******************************************************************************
*                                     SAR
*******************************************************************************/

#define SAR_BASE                                0x411D0000UL
#define SAR                                     ((SAR_Type*) SAR_BASE)                                            /* 0x411D0000 */

/*******************************************************************************
*                                     PASS
*******************************************************************************/

#define PASS_BASE                               0x411F0000UL
#define PASS                                    ((PASS_Type*) PASS_BASE)                                          /* 0x411F0000 */
#define PASS_AREF                               ((PASS_AREF_Type*) &PASS->AREF)                                   /* 0x411F0E00 */

/*******************************************************************************
*                                     I2S
*******************************************************************************/

#define I2S0_BASE                               0x42A10000UL
#define I2S0                                    ((I2S_Type*) I2S0_BASE)                                           /* 0x42A10000 */

/*******************************************************************************
*                                     PDM
*******************************************************************************/

#define PDM0_BASE                               0x42A20000UL
#define PDM0                                    ((PDM_Type*) PDM0_BASE)                                           /* 0x42A20000 */


/* Backward compabitility definitions */
#define I2S                                     I2S0
#define PDM                                     PDM0

/** \} CY8C6136BZI-F34 */

#endif /* _CY8C6136BZI_F34_H_ */


/* [] END OF FILE */
