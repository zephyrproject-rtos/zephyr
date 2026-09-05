/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xilinx/AMD integration for integrated Synopsys DWC3+xHCI blocks.
 * USB2 wrapper MMIO tuning (Versal, ZynqMP) and snps,* DT quirks.
 */

#ifndef ZEPHYR_USB_DWC3_XILINX_H
#define ZEPHYR_USB_DWC3_XILINX_H

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/** snps,dis-u2-susphy-quirk — do not enable U2 suspend-PHY. */
#define DWC3_QUIRK_DIS_U2_SUSPHY BIT(0)
/** snps,dis-u3-susphy-quirk — do not enable U3 suspend-PHY. */
#define DWC3_QUIRK_DIS_U3_SUSPHY BIT(1)

struct dwc3_xilinx_config {
	/** DWC3 IP quirks (DWC3_QUIRK_*), from snps,* device tree properties. */
	uint32_t dwc3_quirks;
	/**
	 * snps,quirk-frame-length-adjustment (GFLADJ 30 MHz field).
	 * 0 selects the integrated-wrapper default when wrapper tuning runs.
	 */
	uint32_t fladj;
	/** Busy-wait after Address Device (BSR=0); 0 disables. */
	uint8_t post_set_address_ms;
};

static inline bool dwc3_xilinx_wrapper_present(mm_reg_t usb2_wrapper_base)
{
	return usb2_wrapper_base != 0U;
}

static inline bool dwc3_xilinx_dwc3_quirk(const struct dwc3_xilinx_config *cfg, uint32_t quirk)
{
	return cfg != NULL && (cfg->dwc3_quirks & quirk) != 0U;
}

static inline uint8_t dwc3_xilinx_post_set_address_ms(const struct dwc3_xilinx_config *cfg)
{
	return cfg != NULL ? cfg->post_set_address_ms : 0U;
}

void dwc3_xilinx_pre_host_burst(mm_reg_t usb2_wrapper_base);
void dwc3_xilinx_host_tune_post(const struct dwc3_xilinx_config *cfg, mm_reg_t dwc3_base,
				mm_reg_t usb2_wrapper_base);

#define _DWC3_HAS_USB2_WRAPPER(n) DT_INST_REG_HAS_NAME(n, usb2_wrapper)

#define _DWC3_WRAPPER_OR_DEFAULT(n, val) COND_CODE_1(_DWC3_HAS_USB2_WRAPPER(n), (val), (0U))

#define _DWC3_QUIRK_FROM_DT_OR_INTEGRATED(n, prop, flag)                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop), (flag), (_DWC3_WRAPPER_OR_DEFAULT(n, flag)))

#define _DWC3_DWC3_QUIRKS(n)                                                                       \
	(_DWC3_QUIRK_FROM_DT_OR_INTEGRATED(n, snps_dis_u2_susphy_quirk,                            \
					   DWC3_QUIRK_DIS_U2_SUSPHY) |                             \
	 _DWC3_QUIRK_FROM_DT_OR_INTEGRATED(n, snps_dis_u3_susphy_quirk, DWC3_QUIRK_DIS_U3_SUSPHY))

#define _DWC3_FLAADJ_DEFAULT 0x20U

#define _DWC3_FLAADJ(n)                                                                            \
	DT_INST_PROP_OR(n, snps_quirk_frame_length_adjustment,                                     \
			(_DWC3_WRAPPER_OR_DEFAULT(n, _DWC3_FLAADJ_DEFAULT)))

#define _DWC3_POST_SET_ADDRESS_MS(n)                                                               \
	DT_INST_PROP_OR(n, zephyr_post_set_address_ms, (_DWC3_WRAPPER_OR_DEFAULT(n, 10U)))

/** Populate struct dwc3_xilinx_config from snps,dwc3 instance @a n. */
#define DWC3_XILINX_CONFIG_INIT(n)                                                                 \
	{                                                                                          \
		.dwc3_quirks = _DWC3_DWC3_QUIRKS(n),                                               \
		.fladj = _DWC3_FLAADJ(n),                                                          \
		.post_set_address_ms = _DWC3_POST_SET_ADDRESS_MS(n),                               \
	}

#endif /* ZEPHYR_USB_DWC3_XILINX_H */
