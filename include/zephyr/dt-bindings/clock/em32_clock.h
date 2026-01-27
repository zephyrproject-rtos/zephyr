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

/**
 * @defgroup em32_clock_gating_ids EM32 Clock Gating IDs
 *
 * Clock gating IDs for EM32 platform peripherals.
 * These IDs are used in DTS (Device Tree Source) to define which
 * peripherals have clock gating enabled for power management.
 * @{
 */

/* HCLK Gating IDs */

/** DMA HCLK gating ID */
#define EM32_GATE_HCLKG_DMA 0x00

/** GPIO Port A HCLK gating ID */
#define EM32_GATE_HCLKG_GPIOA 0x01

/** GPIO Port B HCLK gating ID */
#define EM32_GATE_HCLKG_GPIOB 0x02

/** ISO7816-1 Interface HCLK gating ID */
#define EM32_GATE_HCLKG_7816_1 0x04

/** ISO7816-2 Interface HCLK gating ID */
#define EM32_GATE_HCLKG_7816_2 0x05

/** Encryption Engine HCLK gating ID */
#define EM32_GATE_HCLKG_ENCRYPT 0x06

/** eSPI Interface HCLK gating ID */
#define EM32_GATE_HCLKG_ESPI1 0x0F

/** External SPI Interface HCLK gating ID */
#define EM32_GATE_HCLKG_EXTSPI 0x1F

/** GHM Accelerator 1 HCLK gating ID */
#define EM32_GATE_HCLKG_GHM_ACC1 0x20

/** GHM Accelerator 2 HCLK gating ID */
#define EM32_GATE_HCLKG_GHM_ACC2 0x21

/** GHM Accelerator 3 HCLK gating ID */
#define EM32_GATE_HCLKG_GHM_ACC3 0x22

/* HCLKF Gating IDs */

/** GHM IP HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_IP 0x23

/** Flash BIST HCLKF gating ID */
#define EM32_GATE_HCLKF_FLASH_BIST 0x24

/** GHM RANSAC HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_RANSAC 0x25

/** Software SPI HCLKF gating ID */
#define EM32_GATE_HCLKF_SWSPI 0x26

/** GHM Double Processing HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_DOUBLE 0x27

/** GHM Distinguish HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_DISTINGUISH 0x28

/** GHM Least Squares Estimation HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_LSE 0x29

/** GHM Sum of Absolute Differences HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_SAD 0x2A

/** GHM Motion-to-Depth HCLKF gating ID */
#define EM32_GATE_HCLKF_GHM_M2D 0x2B

/* PCLK Gating IDs */

/** LPC Controller PCLK gating ID */
#define EM32_GATE_PCLKG_LPC 0x03

/** USART PCLK gating ID */
#define EM32_GATE_PCLKG_USART 0x07

/** Timer 1 PCLK gating ID */
#define EM32_GATE_PCLKG_TMR1 0x08

/** Timer 2 PCLK gating ID */
#define EM32_GATE_PCLKG_TMR2 0x09

/** Timer 3 PCLK gating ID */
#define EM32_GATE_PCLKG_TMR3 0x0A

/** Timer 4 PCLK gating ID */
#define EM32_GATE_PCLKG_TMR4 0x0B

/** UART 1 PCLK gating ID */
#define EM32_GATE_PCLKG_UART1 0x0C

/** UART 2 PCLK gating ID */
#define EM32_GATE_PCLKG_UART2 0x0D

/** RVD 1 PCLK gating ID */
#define EM32_GATE_PCLKG_RVD1 0x0E

/** SSP 2 PCLK gating ID */
#define EM32_GATE_PCLKG_SSP2 0x10

/** I2C 1 PCLK gating ID */
#define EM32_GATE_PCLKG_I2C1 0x11

/** I2C 2 PCLK gating ID */
#define EM32_GATE_PCLKG_I2C2 0x12

/** PWM PCLK gating ID */
#define EM32_GATE_PCLKG_PWM 0x13

/** RVD 2 PCLK gating ID */
#define EM32_GATE_PCLKG_RVD2 0x14

/** USB Device Controller PCLK gating ID */
#define EM32_GATE_PCLKG_UDC 0x15

/** Analog Trim PCLK gating ID */
#define EM32_GATE_PCLKG_ATRIM 0x16

/** RTC PCLK gating ID */
#define EM32_GATE_PCLKG_RTC 0x17

/** Backup Register PCLK gating ID */
#define EM32_GATE_PCLKG_BKP 0x18

/** Debug/Watch Dog PCLK gating ID */
#define EM32_GATE_PCLKG_DWG 0x19

/** Power Management PCLK gating ID */
#define EM32_GATE_PCLKG_PWR 0x1A

/** Cache Controller PCLK gating ID */
#define EM32_GATE_PCLKG_CACHE 0x1B

/** AES IP PCLK gating ID */
#define EM32_GATE_PCLKG_AIP 0x1C

/** ECC Engine PCLK gating ID */
#define EM32_GATE_PCLKG_ECC 0x1D

/** TRNG PCLK gating ID */
#define EM32_GATE_PCLKG_TRNG 0x1E

/** SSP 1 PCLK gating ID */
#define EM32_GATE_PCLKG_SSP1 0x30

/* Special IDs */

/** No clock gating (invalid/reserved ID) */
#define EM32_GATE_NONE 0xFFFE

/** Enable all peripheral PCLK gates */
#define EM32_GATE_PCLKG_ALL 0xFFFF

/** @} */

#endif /* __ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ELAN_EM32_CLOCK_H__ */
