/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ELAN_EM32_CLOCK_H__
#define __ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ELAN_EM32_CLOCK_H__

/* Clock Source */
#define EM32_CLK_SRC_IRCLOW    0x00
#define EM32_CLK_SRC_IRCHIGH   0x01
#define EM32_CLK_SRC_EXTERNAL1 0x20

/* Clock Frequency Source */
#define EM32_CLK_FREQ_IRCLOW12   0x00
#define EM32_CLK_FREQ_IRCLOW16   0x01
#define EM32_CLK_FREQ_IRCLOW20   0x02
#define EM32_CLK_FREQ_IRCLOW24   0x03
#define EM32_CLK_FREQ_IRCLOW28   0x04
#define EM32_CLK_FREQ_IRCLOW32   0x05
#define EM32_CLK_FREQ_IRCHIGH64  0x11
#define EM32_CLK_FREQ_IRCHIGH80  0x12
#define EM32_CLK_FREQ_IRCHIGH96  0x13
#define EM32_CLK_FREQ_IRCHIGH112 0x14
#define EM32_CLK_FREQ_IRCHIGH128 0x15
#define EM32_CLK_FREQ_IRCHIGH96Q 0x16

/* AHB PreScaler */
#define EM32_AHB_CLK_DIV1   0x00
#define EM32_AHB_CLK_DIV2   0x01
#define EM32_AHB_CLK_DIV4   0x02
#define EM32_AHB_CLK_DIV8   0x03
#define EM32_AHB_CLK_DIV16  0x04
#define EM32_AHB_CLK_DIV32  0x05
#define EM32_AHB_CLK_DIV64  0x06
#define EM32_AHB_CLK_DIV128 0x07

/* Clock Gating ID */
#define EM32_GATE_HCLKG_DMA             0x00
#define EM32_GATE_HCLKG_GPIOA           0x01
#define EM32_GATE_HCLKG_GPIOB           0x02
#define EM32_GATE_PCLKG_LPC             0x03
#define EM32_GATE_HCLKG_7816_1          0x04
#define EM32_GATE_HCLKG_7816_2          0x05
#define EM32_GATE_HCLKG_ENCRYPT         0x06
#define EM32_GATE_PCLKG_USART           0x07
#define EM32_GATE_PCLKG_TMR1            0x08
#define EM32_GATE_PCLKG_TMR2            0x09
#define EM32_GATE_PCLKG_TMR3            0x0A
#define EM32_GATE_PCLKG_TMR4            0x0B
#define EM32_GATE_PCLKG_UART1           0x0C
#define EM32_GATE_PCLKG_UART2           0x0D
#define EM32_GATE_PCLKG_RVD1            0x0E
#define EM32_GATE_HCLKG_ESPI1           0x0F
#define EM32_GATE_PCLKG_SSP2            0x10
#define EM32_GATE_PCLKG_I2C1            0x11
#define EM32_GATE_PCLKG_I2C2            0x12
#define EM32_GATE_PCLKG_PWM             0x13
#define EM32_GATE_PCLKG_RVD2            0x14
#define EM32_GATE_PCLKG_UDC             0x15
#define EM32_GATE_PCLKG_ATRIM           0x16
#define EM32_GATE_PCLKG_RTC             0x17
#define EM32_GATE_PCLKG_BKP             0x18
#define EM32_GATE_PCLKG_DWG             0x19
#define EM32_GATE_PCLKG_PWR             0x1A
#define EM32_GATE_PCLKG_CACHE           0x1B
#define EM32_GATE_PCLKG_AIP             0x1C
#define EM32_GATE_PCLKG_ECC             0x1D
#define EM32_GATE_PCLKG_TRNG            0x1E
#define EM32_GATE_HCLKG_EXTSPI          0x1F
#define EM32_GATE_HCLKG_GHM_ACC1        0x20
#define EM32_GATE_HCLKG_GHM_ACC2        0x21
#define EM32_GATE_HCLKG_GHM_ACC3        0x22
#define EM32_GATE_HCLKF_GHM_IP          0x23
#define EM32_GATE_HCLKF_FLASH_BIST      0x24
#define EM32_GATE_HCLKF_GHM_RANSAC      0x25
#define EM32_GATE_HCLKF_SWSPI           0x26
#define EM32_GATE_HCLKF_GHM_DOUBLE      0x27
#define EM32_GATE_HCLKF_GHM_DISTINGUISH 0x28
#define EM32_GATE_HCLKF_GHM_LSE         0x29
#define EM32_GATE_HCLKF_GHM_SAD         0x2A
#define EM32_GATE_HCLKF_GHM_M2D         0x2B
#define EM32_GATE_PCLKG_SSP1            0x30
#define EM32_GATE_NONE                  0xFFFE
#define EM32_GATE_PCLKG_ALL             0xFFFF

#endif /* __ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ELAN_EM32_CLOCK_H__ */
