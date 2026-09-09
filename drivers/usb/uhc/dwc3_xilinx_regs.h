/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xilinx/AMD PS USB2_REGS wrapper (Versal @ 0xFF9D0000; ZynqMP uses a similar block).
 */

#ifndef ZEPHYR_USB_DWC3_XILINX_REGS_H
#define ZEPHYR_USB_DWC3_XILINX_REGS_H

#include <zephyr/sys/util.h>

/* Offsets from USB2_REGS base (Versal PS-PMC USB2_REGS). */
#define XILINX_USB2_PHY_RST       0x001c
#define XILINX_USB2_PORT          0x0034
#define XILINX_USB2_JITTER_ADJUST 0x0038
#define XILINX_USB2_COHERENCY     0x0044
#define XILINX_USB2_XHC_BME       0x0048
#define XILINX_USB2_IR_STATUS     0x0064

#define XILINX_USB2_PHY_RST_CHK_BIT BIT(0)
#define XILINX_USB2_COHERENCY_ROUTE BIT(0)

#define XILINX_USB2_JITTER_FLADJ_MASK GENMASK(5, 0)

#define XILINX_USB2_IR_HOST_SYS_ERR BIT(1)
#define XILINX_USB2_IR_ADDR_DEC_ERR BIT(0)

/* snps,quirk-frame-length-adjustment default on integrated wrappers (125 us @ 30 MHz). */
#define XILINX_USB2_GFLADJ_30MHZ_DEFAULT 0x20U

/* Legacy Versal names (same register map). */
#define VERSAL_USB2_PHY_RST              XILINX_USB2_PHY_RST
#define VERSAL_USB2_JITTER_ADJUST        XILINX_USB2_JITTER_ADJUST
#define VERSAL_USB2_COHERENCY            XILINX_USB2_COHERENCY
#define VERSAL_USB2_PHY_RST_CHK_BIT      XILINX_USB2_PHY_RST_CHK_BIT
#define VERSAL_USB2_COHERENCY_ROUTE      XILINX_USB2_COHERENCY_ROUTE
#define VERSAL_USB2_JITTER_FLADJ_MASK    XILINX_USB2_JITTER_FLADJ_MASK
#define VERSAL_USB2_GFLADJ_30MHZ_DEFAULT XILINX_USB2_GFLADJ_30MHZ_DEFAULT

#endif /* ZEPHYR_USB_DWC3_XILINX_REGS_H */
