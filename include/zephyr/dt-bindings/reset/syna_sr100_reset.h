/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief List reset subsystem IDs for Synaptics SR100 family.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SYNA_SR100_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SYNA_SR100_RESET_H_

#include <zephyr/sys/util_macro.h>

/** helpers for RST_REG values */
/** @brief Image Processing reset */
#define IMGPROC_RST   0x20
/** @brief NPU reset */
#define NPU_RST       0x30
/** @brief DMA0 reset */
#define DMA0_RST      0x40
/** @brief DMA1 reset */
#define DMA1_RST      0x48
/** @brief AXI reset */
#define AXI_RST       0x50
/** @brief reset field helper for APB */
#define APB_PERIF_RST 0x8C
/** @brief reset field helper for peripherals */
#define PERIF_RST     0x80

/** helpers for STI_REG values */
/** @brief sticky field helper for peripherals */
#define TOP_STICKY_RST 0x58
/** @brief sticky field helper for USB */
#define PER_STICKY_RST 0x88

/** register field helpers */
/** @brief bit shift for sticky field */
#define STI_REG  24
/** @brief bit mask for sticky field */
#define STI_MASK 21
/** @brief sticky bit */
#define STI_BIT  16
/** @brief bit shift for reset field */
#define RST_REG  8
/** @brief bit mask for reset field */
#define RST_MASK 5
/** @brief bit shift for reset bit */
#define RST_BIT  0

/** Device domain reset selection */
/** @brief Image Processing reset */
#define SYNA_IMGPRC_RST                                                                            \
	((TOP_STICKY_RST << STI_REG) | (1 << STI_BIT) | (IMGPROC_RST << RST_REG) | (0 << RST_BIT))
/** @brief NPU reset */
#define SYNA_NPU_RST                                                                               \
	((TOP_STICKY_RST << STI_REG) | (0 << STI_BIT) | (NPU_RST << RST_REG) | (0 << RST_BIT))
/** @brief DMA0 reset */
#define SYNA_DMA0_RST                                                                              \
	((TOP_STICKY_RST << STI_REG) | (2 << STI_BIT) | (DMA0_RST << RST_REG) | (0 << RST_BIT))
/** @brief DMA1 reset */
#define SYNA_DMA1_RST                                                                              \
	((TOP_STICKY_RST << STI_REG) | (5 << STI_BIT) | (DMA1_RST << RST_REG) | (0 << RST_BIT))
/** @brief AXI reset */
#define SYNA_AXI_RST   ((AXI_RST << RST_REG) | (0 << RST_BIT))
/** @brief OTP reset */
#define SYNA_OTP_RST   ((TOP_STICKY_RST << STI_REG) | (3 << STI_BIT))
/** @brief Clock Calibration reset */
#define SYNA_CALIB_RST ((TOP_STICKY_RST << STI_REG) | (6 << STI_BIT))
/** @brief USB reset */
#define SYNA_USB_RST                                                                               \
	((PER_STICKY_RST << STI_REG) | (0 << STI_BIT) | (7 << STI_MASK) | (PERIF_RST << RST_REG) | \
	 (4 << RST_BIT))
/** @brief UART0 reset */
#define SYNA_UART0_RST ((APB_PERIF_RST << RST_REG) | (0 << RST_BIT))
/** @brief UART1 reset */
#define SYNA_UART1_RST ((APB_PERIF_RST << RST_REG) | (1 << RST_BIT))
/** @brief I2C0 master reset */
#define SYNA_I2C0M_RST ((APB_PERIF_RST << RST_REG) | (2 << RST_BIT))
/** @brief I2C1 master reset */
#define SYNA_I2C1M_RST ((APB_PERIF_RST << RST_REG) | (3 << RST_BIT))
/** @brief I2C slave reset */
#define SYNA_I2CS_RST  ((APB_PERIF_RST << RST_REG) | (4 << RST_BIT))
/** @brief I3C0 reset */
#define SYNA_I3C0_RST  ((APB_PERIF_RST << RST_REG) | (5 << RST_BIT))
/** @brief I3C1 reset */
#define SYNA_I3C1_RST  ((APB_PERIF_RST << RST_REG) | (6 << RST_BIT))
/** @brief SPI master reset */
#define SYNA_SPIM_RST  ((APB_PERIF_RST << RST_REG) | (7 << RST_BIT))
/** @brief SPI slave reset */
#define SYNA_SPIS_RST  ((APB_PERIF_RST << RST_REG) | (8 << RST_BIT))
/** @brief GPIO reset */
#define SYNA_GPIO_RST  ((APB_PERIF_RST << RST_REG) | (9 << RST_BIT))
/** @brief Soundwire reset */
#define SYNA_SWIRE_RST ((APB_PERIF_RST << RST_REG) | (19 << RST_BIT))
/** @brief SD0 reset */
#define SYNA_SD0_RST   ((PERIF_RST << RST_REG) | (2 << RST_BIT))
/** @brief SD1 reset */
#define SYNA_SD1_RST   ((PERIF_RST << RST_REG) | (3 << RST_BIT))
/** @brief xSPI reset */
#define SYNA_XSPI_RST  ((PERIF_RST << RST_REG) | (5 << RST_BIT))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SYNA_SR100_RESET_H_ */
