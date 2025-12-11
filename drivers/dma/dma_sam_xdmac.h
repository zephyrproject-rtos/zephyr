/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Direct Memory Access (XDMAC) driver.
 */

#ifndef ZEPHYR_DRIVERS_DMA_DMA_SAM_XDMAC_H_
#define ZEPHYR_DRIVERS_DMA_DMA_SAM_XDMAC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* XDMA_MBR_UBC */
#define XDMA_UBC_NDE (0x1u << 24)
#define   XDMA_UBC_NDE_FETCH_DIS (0x0u << 24)
#define   XDMA_UBC_NDE_FETCH_EN  (0x1u << 24)
#define XDMA_UBC_NSEN (0x1u << 25)
#define   XDMA_UBC_NSEN_UNCHANGED (0x0u << 25)
#define   XDMA_UBC_NSEN_UPDATED (0x1u << 25)
#define XDMA_UBC_NDEN (0x1u << 26)
#define   XDMA_UBC_NDEN_UNCHANGED (0x0u << 26)
#define   XDMA_UBC_NDEN_UPDATED (0x1u << 26)
#define XDMA_UBC_NVIEW_SHIFT 27
#define   XDMA_UBC_NVIEW_MASK (0x3u << XDMA_UBC_NVIEW_SHIFT)
#define   XDMA_UBC_NVIEW_NDV0 (0x0u << XDMA_UBC_NVIEW_SHIFT)
#define   XDMA_UBC_NVIEW_NDV1 (0x1u << XDMA_UBC_NVIEW_SHIFT)
#define   XDMA_UBC_NVIEW_NDV2 (0x2u << XDMA_UBC_NVIEW_SHIFT)
#define   XDMA_UBC_NVIEW_NDV3 (0x3u << XDMA_UBC_NVIEW_SHIFT)

/** DMA channel configuration parameters */
struct sam_xdmac_channel_config {
	/** Configuration Register */
	uint32_t cfg;
	/** Data Stride / Memory Set Pattern Register */
	uint32_t ds_msp;
	/** Source Microblock Stride */
	uint32_t sus;
	/** Destination Microblock Stride */
	uint32_t dus;
	/** Channel Interrupt Enable */
	uint32_t cie;
};

/** DMA transfer configuration parameters */
struct sam_xdmac_transfer_config {
	/** Microblock length */
	uint32_t ublen;
	/** Source Address */
	uint32_t sa;
	/** Destination Address */
	uint32_t da;
	/** Block length (The length of the block is (blen+1) microblocks) */
	uint32_t blen;
	/** Next descriptor address */
	uint32_t nda;
	/** Next descriptor configuration */
	uint32_t ndc;
};

/** DMA Master transfer linked list view 0 structure */
struct sam_xdmac_linked_list_desc_view0 {
	/** Next Descriptor Address */
	uint32_t mbr_nda;
	/** Microblock Control */
	uint32_t mbr_ubc;
	/** Transfer Address */
	uint32_t mbr_ta;
};

/** DMA Master transfer linked list view 1 structure */
struct sam_xdmac_linked_list_desc_view1 {
	/** Next Descriptor Address */
	uint32_t mbr_nda;
	/** Microblock Control */
	uint32_t mbr_ubc;
	/** Source Address */
	uint32_t mbr_sa;
	/** Destination Address */
	uint32_t mbr_da;
};

/** DMA Master transfer linked list view 2 structure */
struct sam_xdmac_linked_list_desc_view2 {
	/** Next Descriptor Address */
	uint32_t mbr_nda;
	/** Microblock Control */
	uint32_t mbr_ubc;
	/** Source Address */
	uint32_t mbr_sa;
	/** Destination Address */
	uint32_t mbr_da;
	/** Configuration Register */
	uint32_t mbr_cfg;
};

/** DMA Master transfer linked list view 3 structure */
struct sam_xdmac_linked_list_desc_view3 {
	/** Next Descriptor Address */
	uint32_t mbr_nda;
	/** Microblock Control */
	uint32_t mbr_ubc;
	/** Source Address */
	uint32_t mbr_sa;
	/** Destination Address */
	uint32_t mbr_da;
	/** Configuration Register */
	uint32_t mbr_cfg;
	/** Block Control */
	uint32_t mbr_bc;
	/** Data Stride */
	uint32_t mbr_ds;
	/** Source Microblock Stride */
	uint32_t mbr_sus;
	/** Destination Microblock Stride */
	uint32_t mbr_dus;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_DMA_DMA_SAM_XDMAC_H_ */
