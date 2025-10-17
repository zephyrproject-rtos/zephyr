/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SYNA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SYNA_CLOCK_H_

/* Bit locations in GLOBAL_CLK_ENABLE1/2 registers */

#define SYNA_IMG_CAM_CLK  0
#define SYNA_GPIO_DB_CLK  1
#define SYNA_AXI_CLK      2
#define SYNA_NPU_CLK      3
#define SYNA_DMA0_CLK     4
#define SYNA_SD0_AXI_CLK  5
#define SYNA_SD1_AXI_CLK  6
#define SYNA_XSPI_AXI_CLK 7
#define SYNA_USB_ACLK     8
#define SYNA_IMG_SYS_CLK  9
#define SYNA_IMG_CFG_CLK  10
#define SYNA_IMG_APB_CLK  11
#define SYNA_DMA1_CLK     12
#define SYNA_PVT_APB_CLK  13

#define SYNA_XSPI_CFG_CLK      32
#define SYNA_SD0_CFG_CLK       33
#define SYNA_SD1_CFG_CLK       34
#define SYNA_USB_CFG_CLK       35
#define SYNA_SPI_MSTR_CFG_CLK  36
#define SYNA_SPI_SLV_CFG_CLK   37
#define SYNA_I3C0_CFG_CLK      38
#define SYNA_I3C1_CFG_CLK      39
#define SYNA_UART0_CFG_CLK     40
#define SYNA_UART1_CFG_CLK     41
#define SYNA_I2C0_MSTR_CFG_CLK 42
#define SYNA_I2C1_MSTR_CFG_CLK 43
#define SYNA_I2C_SLV_CFG_CLK   44
#define SYNA_GPIO_CFG_CLK      45
#define SYNA_I2S_CFG_CLK       46
#define SYNA_I2S_TX0_PCLK      47
#define SYNA_I2S_TX1_PCLK      48
#define SYNA_I2S_TX2_PCLK      49
#define SYNA_I2S_TX3_PCLK      50
#define SYNA_I2S_RX0_PCLK      51
#define SYNA_I2S_RX1_PCLK      52
#define SYNA_I2S_RX2_PCLK      53
#define SYNA_I2S_RX3_PCLK      54
#define SYNA_I2S_SWIRE_CFG_CLK 55

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SYNA_CLOCK_H_ */
