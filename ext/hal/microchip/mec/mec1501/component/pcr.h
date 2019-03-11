/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
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

/** @file pcr.h
 *MEC1501 Power Control Reset definitions
 */
/** @defgroup MEC1501 Peripherals PCR
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _PCR_H
#define _PCR_H

#define PCR_BASE_ADDR           0x40080100ul

#define PCR_SYS_SLP_CTRL_ADDR   (PCR_BASE_ADDR)
#define PCR_SYS_CLK_CTRL_ADDR   (PCR_BASE_ADDR + 0x04)
#define PCR_SLOW_CLK_CTRL_ADDR  (PCR_BASE_ADDR + 0x08)
#define PCR_OSC_ID_ADDR         (PCR_BASE_ADDR + 0x0C)
#define PCR_PRS_ADDR            (PCR_BASE_ADDR + 0x10)
#define PCR_PR_CTRL_ADDR        (PCR_BASE_ADDR + 0x14)
#define PCR_SYS_RESET_ADDR      (PCR_BASE_ADDR + 0x18)
#define PCR_PKE_CLK_CTRL_ADDR   (PCR_BASE_ADDR + 0x1C)
#define PCR_SLP_EN0_ADDR        (PCR_BASE_ADDR + 0x30)
#define PCR_CLK_REQ0_ADDR       (PCR_BASE_ADDR + 0x50)
#define PCR_SLP_EN1_ADDR        (PCR_BASE_ADDR + 0x34)
#define PCR_CLK_REQ1_ADDR       (PCR_BASE_ADDR + 0x54)
#define PCR_SLP_EN2_ADDR        (PCR_BASE_ADDR + 0x38)
#define PCR_CLK_REQ2_ADDR       (PCR_BASE_ADDR + 0x58)
#define PCR_SLP_EN3_ADDR        (PCR_BASE_ADDR + 0x3C)
#define PCR_CLK_REQ3_ADDR       (PCR_BASE_ADDR + 0x5C)
#define PCR_SLP_EN4_ADDR        (PCR_BASE_ADDR + 0x40)
#define PCR_CLK_REQ4_ADDR       (PCR_BASE_ADDR + 0x60)

#define PCR_SLP_EN_ADDR(n)       (PCR_BASE_ADDR + 0x30 + ((n) << 2))
#define PCR_CLK_REQ_ADDR(n)      (PCR_BASE_ADDR + 0x50 + ((n) << 2))

#define PCR_SLEEP_EN    (1u)
#define PCR_SLEEP_DIS   (0u)

/*
 * MEC1501 PCR implements multiple SLP_EN, CLR_REQ, and RST_EN registers.
 */
#define MAX_PCR_SCR_REGS    5

/*
 * VTR Powered PCR registers
 */

#define PCR_SLP(bitpos)     (1ul << (bitpos))

/*
 * PCR System Sleep Control
 */
#define PCR_SYS_SLP_CTRL_OFS            0x00
#define PCR_SYS_SLP_CTRL_MASK           0x09
#define PCR_SYS_SLP_CTRL_SLP_LIGHT      (0 << 0)
#define PCR_SYS_SLP_CTRL_SLP_HEAVY      (1 << 0)
#define PCR_SYS_SLP_CTRL_SLP_ALL        (1 << 3)

/*
 * PCR Process Clock Control
 * Divides 48MHz clock to ARM Cortex-M4 core including
 * SysTick and NVIC.
 */
#define PCR_PROC_CLK_CTRL_OFS           0x04
#define PCR_PROC_CLK_CTRL_MASK          0xFF
#define PCR_PROC_CLK_CTRL_48MHZ         0x01
#define PCR_PROC_CLK_CTRL_16MHZ         0x03
#define PCR_PROC_CLK_CTRL_12MHZ         0x04
#define PCR_PROC_CLK_CTRL_4MHZ          0x10
#define PCR_PROC_CLK_CTRL_1MHZ          0x30

/*
 * PCR Slow Clock Control
 * Clock divicder for 100KHz clock domain
 */
#define PCR_SLOW_CLK_CTRL_OFS           0x08
#define PCR_SLOW_CLK_CTRL_MASK          0x3FF
#define PCR_SLOW_CLK_CTRL_100KHZ        0x1E0

/*
 * PCR Oscillator ID register (Read-Only)
 */
#define PCR_OSC_ID_OFS                  0x0C
#define PCR_OSC_ID_MASK                 0x1FF
#define PCR_OSC_ID_PLL_LOCK             (1u << 8)

/*
 * PCR Power Reset Status Register
 */
#define PCR_PRS_OFS                     0x10
#define PCR_PRS_MASK                    0xCEC
#define PCR_PRS_VCC_PWRGD_STATE_RO      (1u << 2)
#define PCR_PRS_HOST_RESET_STATE_RO     (1u << 3)
#define PCR_PRS_VBAT_RST_RWC            (1u << 5)
#define PCR_PRS_VTR_RST_RWC             (1u << 6)
#define PCR_PRS_JTAG_RST_RWC            (1u << 7)
#define PCR_PRS_32K_ACTIVE_RO           (1u << 10)
#define PCR_PRS_LPC_ESPI_CLK_ACTIVE_RO  (1u << 11)

/*
 * PCR Power Reset Control Register
 */
#define PCR_PR_CTRL_OFS                 0x14
#define PCR_PR_CTRL_MASK                0x101
#define PCR_PR_CTRL_PWR_INV             (1 << 0)
#define PCR_PR_CTRL_USE_ESPI_PLTRST     (0 << 8)
#define PCR_PR_CTRL_USE_LRESET          (1 << 8)

/*
 * PCR System Reset Register
 */
#define PCR_SYS_RESET_OFS               0x18
#define PCR_SYS_RESET_MASK              0x100
#define PCR_SYS_RESET_NOW               (1 << 8)

/*
 * PCR PKE Clock Register
 */
#define PCR_PKE_CLK_CTRL_OFS            0x1C
#define PCR_PKE_CLK_CTRL_MASK           0x03
#define PCR_PKE_CLK_CTRL_96M            0x00
#define PCR_PKE_CLK_CTRL_48M            0x03

/*
 * Sleep Enable Reg 0       (Offset +30h)
 * Clock Required Reg 0     (Offset +50h)
 * Reset Enable Reg 3       (Offset +70h)
 */
#define PCR0_JTAG_STAP_POS      0u

/*
 * Sleep Enable Reg 1       (Offset +34h)
 * Clock Required Reg 1     (Offset +54h)
 * Reset Enable Reg 1       (Offset +74h)
 */
#define PCR1_ECIA_POS           0u
#define PCR1_PECI_POS           1u
#define PCR1_TACH0_POS          2u
#define PCR1_PWM0_POS           4u
#define PCR1_PMC_POS            5u
#define PCR1_DMA_POS            6u
#define PCR1_TFDP_POS           7u
#define PCR1_CPU_POS            8u
#define PCR1_WDT_POS            9u
#define PCR1_I2CSMB0_POS        10u
#define PCR1_TACH1_POS          11u
#define PCR1_TACH2_POS          12u
#define PCR1_TACH3_POS          13u
#define PCR1_PWM1_POS           20u
#define PCR1_PWM2_POS           21u
#define PCR1_PWM3_POS           22u
#define PCR1_PWM4_POS           23u
#define PCR1_PWM5_POS           24u
#define PCR1_PWM6_POS           25u
#define PCR1_PWM7_POS           26u
#define PCR1_PWM8_POS           27u
#define PCR1_ECS_POS            29u
#define PCR1_BTMR0_POS          30u
#define PCR1_BTMR1_POS          31u

/*
 * Sleep Enable Reg 2       (Offset +38h)
 * Clock Required Reg 2     (Offset +58h)
 * Reset Enable Reg 2       (Offset +78h)
 */
#define PCR2_EMI0_POS           0u
#define PCR2_UART0_POS          1u
#define PCR2_UART1_POS          2u
#define PCR2_GCFG_POS           12u
#define PCR2_ACPI_EC0_POS       13u
#define PCR2_ACPI_EC1_POS       14u
#define PCR2_ACPI_PM1_POS       15u
#define PCR2_EM8042_POS         16u
#define PCR2_MBOX_POS           17u
#define PCR2_RTC_POS            18u
#define PCR2_ESPI_POS           19u
#define PCR2_SCR32_POS          20u
#define PCR2_ACPI_EC2_POS       21u
#define PCR2_ACPI_EC3_POS       22u
#define PCR2_PORT80CAP0_POS     25u
#define PCR2_PORT80CAP1_POS     26u
#define PCR2_ESPI_SAF_POS       27u
#define PCR2_UART2_POS          28u

/*
 * Sleep Enable Reg 3       (Offset +3Ch)
 * Clock Required Reg 3     (Offset +5Ch)
 * Reset Enable Reg 3       (Offset +7Ch)
 */
#define PCR3_HDMI_CEC_POS           1u
#define PCR3_ADC_POS                3u
#define PCR3_PS20_POS               5u
#define PCR3_PS21_POS               6u
#define PCR3_HTMR0_POS              10u
#define PCR3_KEYSCAN_POS            11u
#define PCR3_I2CSMB1_POS            13u
#define PCR3_I2CSMB2_POS            14u
#define PCR3_I2CSMB3_POS            15u
#define PCR3_LED0_POS               16u
#define PCR3_LED1_POS               17u
#define PCR3_LED2_POS               18u
#define PCR3_I2CSMB4_POS            20u
#define PCR3_BTMR4_POS              23u
#define PCR3_BTMR5_POS              24u
#define PCR3_PKE_POS                26u
#define PCR3_RNG_POS                27u
#define PCR3_AESH_POS               28u
#define PCR3_HTMR1_POS              29u
#define PCR3_CCT_POS                30u

/*
 * Sleep Enable Reg 4       (Offset +40h)
 * Clock Required Reg 4     (Offset +60h)
 * Reset Enable Reg 4       (Offset +80h)
 */
#define PCR4_RTMR_POS           6u
#define PCR4_QMSPI_POS          8u
#define PCR4_I2C0_POS           10u
#define PCR4_I2C1_POS           11u
#define PCR4_I2C3_POS           12u
#define PCR4_SPISLV_POS         16u

/* Reset Enable Lock        (Offset +84h) */
#define PCR_RSTEN_UNLOCK    0xA6382D4C
#define PCR_RSTEN_LOCK      0xA6382D4D


/* =====================================================================*/
/* ================          PCR                       ================ */
/* =====================================================================*/

/**
  * @brief Power Control Reset (PCR)
  */
#define PCR_MAX_SLPEN_IDX   5

typedef struct
{           /*!< (@ 0x40080100) PCR Structure   */
    __IOM uint32_t SYS_SLP_CTRL;            /*!< (@ 0x0000) System Sleep Control */
    __IOM uint32_t PROC_CLK_CTRL;           /*!< (@ 0x0004) Processor Clock Control */
    __IOM uint32_t SLOW_CLK_CTRL;           /*!< (@ 0x0008) Slow Clock Control */
    __IOM uint32_t OSC_ID;                  /*!< (@ 0x000C) Processor Clock Control */
    __IOM uint32_t PWR_RST_STS;             /*!< (@ 0x0010) Power Reset Status */
    __IOM uint32_t PWR_RST_CTRL;            /*!< (@ 0x0014) Power Reset Control */
    __IOM uint32_t SYS_RST;                 /*!< (@ 0x0018) System Reset */
    __IOM uint32_t TEST1C;
    __IOM uint32_t TEST20;
          uint8_t  RSVD1[12];
    __IOM uint32_t SLP_EN0;                 /*!< (@ 0x0030) Sleep Enable 0 */
    __IOM uint32_t SLP_EN1;                 /*!< (@ 0x0034) Sleep Enable 1 */
    __IOM uint32_t SLP_EN2;                 /*!< (@ 0x0038) Sleep Enable 2 */
    __IOM uint32_t SLP_EN3;                 /*!< (@ 0x003C) Sleep Enable 3 */
    __IOM uint32_t SLP_EN4;                 /*!< (@ 0x0040) Sleep Enable 4 */
          uint8_t  RSVD2[12];
    __IOM uint32_t CLK_REQ0;                /*!< (@ 0x0050) Clock Required 0 (RO) */
    __IOM uint32_t CLK_REQ1;                /*!< (@ 0x0054) Clock Required 1 (RO) */
    __IOM uint32_t CLK_REQ2;                /*!< (@ 0x0058) Clock Required 2 (RO) */
    __IOM uint32_t CLK_REQ3;                /*!< (@ 0x005C) Clock Required 3 (RO) */
    __IOM uint32_t CLK_REQ4;                /*!< (@ 0x0060) Clock Required 4 (RO) */
          uint8_t  RSVD3[12];
    __IOM uint32_t RST_EN0;                 /*!< (@ 0x0070) Peripheral Reset 0 */
    __IOM uint32_t RST_EN1;                 /*!< (@ 0x0074) Peripheral Reset 1 */
    __IOM uint32_t RST_EN2;                 /*!< (@ 0x0078) Peripheral Reset 2 */
    __IOM uint32_t RST_EN3;                 /*!< (@ 0x007C) Peripheral Reset 3 */
    __IOM uint32_t RST_EN4;                 /*!< (@ 0x0080) Peripheral Reset 4 */
    __IOM uint32_t RST_EN_LOCK;             /*!< (@ 0x0084) Peripheral Lock */
} PCR_Type;


#endif // #ifndef _DEFS_H
/* end pcr.h */
/**   @}
 */

