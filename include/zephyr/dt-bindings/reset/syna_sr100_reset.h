/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SYNA_SR100_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SYNA_SR100_RESET_H_

#include <zephyr/sys/util_macro.h>

#define IMGPROC_RST	0x20
#define NPU_RST		0x30
#define DMA0_RST	0x40
#define DMA1_RST	0x48
#define AXI_RST		0x50
#define TOP_STICKY_RST	0x58
#define PERIF_RST	0x80
#define PER_STICKY_RST	0x88
#define APB_PERIF_RST	0x8C

#define STI_REG		24
#define STI_MASK	21
#define STI_BIT		16
#define RST_REG		8
#define RST_MASK	5
#define RST_BIT		0

#define SYNA_IMGPRC_RST	((TOP_STICKY_RST << STI_REG) | (1 << STI_BIT) | \
			 (IMGPROC_RST << RST_REG) | (0 << RST_BIT))
#define SYNA_NPU_RST	((TOP_STICKY_RST << STI_REG) | (0 << STI_BIT) | \
			 (NPU_RST << RST_REG) | (0 << RST_BIT))
#define SYNA_DMA0_RST	((TOP_STICKY_RST << STI_REG) | (2 << STI_BIT) | \
			 (DMA0_RST << RST_REG) | (0 << RST_BIT))
#define SYNA_DMA1_RST	((TOP_STICKY_RST << STI_REG) | (5 << STI_BIT) | \
			 (DMA1_RST << RST_REG) | (0 << RST_BIT))
#define SYNA_AXI_RST	((AXI_RST << RST_REG) | (0 << RST_BIT))
#define SYNA_OTP_RST	((TOP_STICKY_RST << STI_REG) | (3 << STI_BIT))
#define SYNA_CALIB_RST	((TOP_STICKY_RST << STI_REG) | (6 << STI_BIT))
#define SYNA_USB_RST	((PER_STICKY_RST << STI_REG) | (0 << STI_BIT) | \
			 (7 << STI_MASK) | (PERIF_RST << RST_REG) | \
			 (4 << RST_BIT))
#define SYNA_UART0_RST	((APB_PERIF_RST << RST_REG) | (0 << RST_BIT))
#define SYNA_UART1_RST	((APB_PERIF_RST << RST_REG) | (1 << RST_BIT))
#define SYNA_I2C0M_RST	((APB_PERIF_RST << RST_REG) | (2 << RST_BIT))
#define SYNA_I2C1M_RST	((APB_PERIF_RST << RST_REG) | (3 << RST_BIT))
#define SYNA_I2CS_RST	((APB_PERIF_RST << RST_REG) | (4 << RST_BIT))
#define SYNA_I3C0_RST	((APB_PERIF_RST << RST_REG) | (5 << RST_BIT))
#define SYNA_I3C1_RST	((APB_PERIF_RST << RST_REG) | (6 << RST_BIT))
#define SYNA_SPIM_RST	((APB_PERIF_RST << RST_REG) | (7 << RST_BIT))
#define SYNA_SPIS_RST	((APB_PERIF_RST << RST_REG) | (8 << RST_BIT))
#define SYNA_GPIO_RST	((APB_PERIF_RST << RST_REG) | (9 << RST_BIT))
#define SYNA_SWIRE_RST	((APB_PERIF_RST << RST_REG) | (19 << RST_BIT))
#define SYNA_SD0_RST	((PERIF_RST << RST_REG) | (2 << RST_BIT))
#define SYNA_SD1_RST	((PERIF_RST << RST_REG) | (3 << RST_BIT))
#define SYNA_XSPI_RST	((PERIF_RST << RST_REG) | (5 << RST_BIT))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SYNA_SR100_RESET_H_ */
