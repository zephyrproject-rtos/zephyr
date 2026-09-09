/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * xHCI platform quirks (xhci_hcd.quirks / xhci-plat devicetree properties).
 */

#ifndef ZEPHYR_USB_XHCI_PLAT_H
#define ZEPHYR_USB_XHCI_PLAT_H

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/** Bulk EP recovery after CLEAR_FEATURE(HALT): Configure EP drop+add. */
#define XHCI_QUIRK_BULK_FORCE_DROP_ADD BIT(0)
/** Bulk IN uses driver-owned bounce buffers (integrated IP DMA safety). */
#define XHCI_QUIRK_BULK_IN_STAGING     BIT(1)
/** Mirror Configure Endpoint output contexts when HC skips writeback. */
#define XHCI_QUIRK_CFG_EP_CTX_MIRROR   BIT(2)
/** DWC3 host init write-64-hi-lo-quirk (reserved). */
#define XHCI_QUIRK_WRITE_64_HI_LO      BIT(3)
/** DWC3 host init xhci-sg-trb-cache-size-quirk (reserved). */
#define XHCI_QUIRK_SG_TRB_CACHE_SIZE   BIT(4)

struct xhci_plat_config {
	uint32_t quirks;
};

static inline bool xhci_plat_quirk(const struct xhci_plat_config *plat, uint32_t quirk)
{
	return plat != NULL && (plat->quirks & quirk) != 0U;
}

#define _XHCI_WRAPPER_OR_DEFAULT(n, val)                                                           \
	COND_CODE_1(DT_INST_REG_HAS_NAME(n, usb2_wrapper), (val), (0U))

#define _XHCI_QUIRK_FROM_DT_OR_INTEGRATED(n, prop, flag)                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop), (flag), (_XHCI_WRAPPER_OR_DEFAULT(n, flag)))

#define _XHCI_QUIRK_FROM_DT_ONLY(n, prop, flag)                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop), (flag), (0U))

#define _XHCI_PLAT_QUIRKS(n)                                                                       \
	(_XHCI_QUIRK_FROM_DT_OR_INTEGRATED(n, zephyr_xhci_bulk_force_drop_add,                     \
					   XHCI_QUIRK_BULK_FORCE_DROP_ADD) |                       \
	 _XHCI_QUIRK_FROM_DT_OR_INTEGRATED(n, zephyr_xhci_bulk_in_staging,                         \
					   XHCI_QUIRK_BULK_IN_STAGING) |                           \
	 _XHCI_QUIRK_FROM_DT_OR_INTEGRATED(n, zephyr_xhci_cfg_ep_ctx_mirror,                       \
					   XHCI_QUIRK_CFG_EP_CTX_MIRROR) |                         \
	 _XHCI_QUIRK_FROM_DT_ONLY(n, write_64_hi_lo_quirk, XHCI_QUIRK_WRITE_64_HI_LO) |            \
	 _XHCI_QUIRK_FROM_DT_ONLY(n, xhci_sg_trb_cache_size_quirk, XHCI_QUIRK_SG_TRB_CACHE_SIZE) | \
	 _XHCI_WRAPPER_OR_DEFAULT(n, (XHCI_QUIRK_WRITE_64_HI_LO | XHCI_QUIRK_SG_TRB_CACHE_SIZE)))

/** Populate struct xhci_plat_config from snps,dwc3 instance @a n. */
#define XHCI_PLAT_CONFIG_INIT(n)                                                                   \
	{                                                                                          \
		.quirks = _XHCI_PLAT_QUIRKS(n),                                                    \
	}

#endif /* ZEPHYR_USB_XHCI_PLAT_H */
