/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_
#define ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_

#include <stdint.h>
#include <xtensa/config/core-isa.h>

/* L2 Local Memory Management */

/* These registers are for the L2 memory control and status. */
#define DFL2MM_REG 0x71d00

struct mtl_l2mm {
	uint32_t l2mcap;
	uint32_t l2mpat;
	uint32_t l2mecap;
	uint32_t l2mecs;
	uint32_t l2hsbpmptr;
	uint32_t l2usbpmptr;
	uint32_t l2usbmrpptr;
	uint32_t l2ucmrpptr;
	uint32_t l2ucmrpdptr;
};

#define MTL_L2MM ((volatile struct mtl_l2mm *)DFL2MM_REG)

/* DfL2MCAP */
struct mtl_l2mcap {
	uint32_t l2hss  : 8;
	uint32_t l2uss  : 4;
	uint32_t l2hsbs : 4;
	uint32_t l2hs2s : 8;
	uint32_t l2usbs : 5;
	uint32_t l2se   : 1;
	uint32_t el2se  : 1;
	uint32_t rsvd32 : 1;
};

#define MTL_L2MCAP ((volatile struct mtl_l2mcap *)DFL2MM_REG)

static inline uint32_t mtl_hpsram_get_bank_count(void)
{
	return MTL_L2MCAP->l2hss;
}

static inline uint32_t mtl_lpsram_get_bank_count(void)
{
	return MTL_L2MCAP->l2uss;
}

struct mtl_hpsram_regs {
	/** @brief power gating control */
	uint8_t HSxPGCTL;
	/** @brief retention mode control */
	uint8_t HSxRMCTL;
	uint8_t reserved[2];
	/** @brief power gating status */
	uint8_t HSxPGISTS;
	uint8_t reserved1[3];
};

/* These registers are for the L2 HP SRAM bank power management control and status.*/
#define L2HSBPM_REG					0x17A800
#define L2HSBPM_REG_SIZE			0x0008

#define HPSRAM_REGS(block_idx)		((volatile struct mtl_hpsram_regs *const) \
	(L2HSBPM_REG + L2HSBPM_REG_SIZE * (block_idx)))

#endif /* ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_ */
