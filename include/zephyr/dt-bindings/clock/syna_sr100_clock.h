/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief List clock subsystem IDs for Synaptics SR100 family.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SYNA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SYNA_CLOCK_H_

/** Clock gating helpers */
/** @brief clock gating helper for XSPI */
#define XSPI_CORE_CLK_CTRL      0x30
/** @brief clock gating helper for SD0 */
#define SD0_CORE_CLK_CTRL       0x38
/** @brief clock gating helper for SD1 */
#define SD1_CORE_CLK_CTRL       0x3c
/** @brief clock gating helper for SPI master */
#define SPI_MSTR_SSI_CLK_CTRL   0x40
/** @brief clock gating helper for SPI slave */
#define SPI_SLV_SSI_CLK_CTRL    0x44
/** @brief clock gating helper for I3C0 */
#define I3C0_CORE_CLK_CTRL      0x48
/** @brief clock gating helper for I3C1 */
#define I3C1_CORE_CLK_CTRL      0x4c
/** @brief clock gating helper for UART0 */
#define UART0_CORE_CLK_CTRL     0x50
/** @brief clock gating helper for UART1 */
#define UART1_CORE_CLK_CTRL     0x54
/** @brief clock gating helper for I2C0 master */
#define I2C0_MSTR_CORE_CLK_CTRL 0x58
/** @brief clock gating helper for I2C1 master */
#define I2C1_MSTR_CORE_CLK_CTRL 0x5c
/** @brief clock gating helper for I2C slave */
#define I2C_SLV_CORE_CLK_CTRL   0x60
/** @brief clock gating helper for USB */
#define USB2_CORE_CLK_CTRL      0x64

/** Bit locations in GLOBAL_CLK_ENABLE1/2 registers */
/** @brief bit shift for clock gating */
#define CGL_REG 16
/** @brief bit shift for AXI clocks */
#define AXI_ID  8

/** Device domain clock selection */
/** @brief Camera clock */
#define SYNA_IMG_CAM_CLK 0
/** @brief GPIO Debounce clock */
#define SYNA_GPIO_DB_CLK 1
/** @brief AXI clock */
#define SYNA_AXI_CLK     2
/** @brief NPU clock */
#define SYNA_NPU_CLK     3
/** @brief DMA0 clock */
#define SYNA_DMA0_CLK    4
/** @brief Image Processing clock (sys) */
#define SYNA_IMG_SYS_CLK 9
/** @brief Image Processing clock (cfg) */
#define SYNA_IMG_CFG_CLK 10
/** @brief Image Processing clock (apb) */
#define SYNA_IMG_APB_CLK 11
/** @brief DMA1 clock */
#define SYNA_DMA1_CLK    12
/** @brief Process, Voltage, and Temperature clock */
#define SYNA_PVT_APB_CLK 13

/** @brief xSPI clock */
#define SYNA_XSPI_CLK      ((XSPI_CORE_CLK_CTRL << CGL_REG) | (7 << AXI_ID) | 32)
/** @brief USB clock */
#define SYNA_USB_CLK       ((USB2_CORE_CLK_CTRL << CGL_REG) | (8 << AXI_ID) | 35)
/** @brief SD0 clock */
#define SYNA_SD0_CLK       ((SD0_CORE_CLK_CTRL << CGL_REG) | (5 << AXI_ID) | 33)
/** @brief SD1 clock */
#define SYNA_SD1_CLK       ((SD1_CORE_CLK_CTRL << CGL_REG) | (6 << AXI_ID) | 34)
/** @brief SPI master clock */
#define SYNA_SPI_MSTR_CLK  ((SPI_MSTR_SSI_CLK_CTRL << CGL_REG) | 36)
/** @brief SPI slave clock */
#define SYNA_SPI_SLV_CLK   ((SPI_SLV_SSI_CLK_CTRL << CGL_REG) | 37)
/** @brief I3C0 clock */
#define SYNA_I3C0_CLK      ((I3C0_CORE_CLK_CTRL << CGL_REG) | 38)
/** @brief I3C1 clock */
#define SYNA_I3C1_CLK      ((I3C1_CORE_CLK_CTRL << CGL_REG) | 39)
/** @brief UART0 clock */
#define SYNA_UART0_CLK     ((UART0_CORE_CLK_CTRL << CGL_REG) | 40)
/** @brief UART1 clock */
#define SYNA_UART1_CLK     ((UART1_CORE_CLK_CTRL << CGL_REG) | 41)
/** @brief I2C0 master clock */
#define SYNA_I2C0_MSTR_CLK ((I2C0_MSTR_CORE_CLK_CTRL << CGL_REG) | 42)
/** @brief I2C1 master  clock */
#define SYNA_I2C1_MSTR_CLK ((I2C1_MSTR_CORE_CLK_CTRL << CGL_REG) | 43)
/** @brief I2C slave  clock */
#define SYNA_I2C_SLV_CLK   ((I2C_SLV_CORE_CLK_CTRL << CGL_REG) | 44)

/** @brief GPIO clock */
#define SYNA_GPIO_CFG_CLK      45
/** @brief I2S clock */
#define SYNA_I2S_CFG_CLK       46
/** @brief I2S TX0 clock */
#define SYNA_I2S_TX0_PCLK      47
/** @brief I2S TX1 clock */
#define SYNA_I2S_TX1_PCLK      48
/** @brief I2S TX2 clock */
#define SYNA_I2S_TX2_PCLK      49
/** @brief I2S TX3 clock */
#define SYNA_I2S_TX3_PCLK      50
/** @brief I2S RX0 clock */
#define SYNA_I2S_RX0_PCLK      51
/** @brief I2S RX1 clock */
#define SYNA_I2S_RX1_PCLK      52
/** @brief I2S RX2 clock */
#define SYNA_I2S_RX2_PCLK      53
/** @brief I2S RX3 clock */
#define SYNA_I2S_RX3_PCLK      54
/** @brief I2S SWIRE clock */
#define SYNA_I2S_SWIRE_CFG_CLK 55

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SYNA_CLOCK_H_ */
