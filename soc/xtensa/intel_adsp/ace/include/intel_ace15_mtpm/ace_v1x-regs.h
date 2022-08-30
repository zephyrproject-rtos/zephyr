/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_
#define ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_

#include <stdint.h>
#include <xtensa/config/core-isa.h>

/* Value used as delay when waiting for hw register state change. */
#define HW_STATE_CHECK_DELAY 256

/* Boot / recovery capability function registers */
struct dfdspbrcp {
	struct {
		uint32_t cap;
		uint32_t ctl;
	} capctl[3];
	uint32_t unused0[10];
	struct {
		uint32_t brcap;
		uint32_t wdtcs;
		uint32_t wdtipptr;
		uint32_t unused1;
		uint32_t bctl;
		uint32_t baddr;
		uint32_t battr;
		uint32_t unused2;
	} bootctl[3];
};

/* These registers are added by Intel outside of the Tensilica Core
 * for boot / recovery control, such as boot path, watch dog timer etc.
 */
#define DFDSPBRCP_REG				0x178d00

#define DFDSPBRCP_CTL_SPA			BIT(0)
#define DFDSPBRCP_CTL_CPA			BIT(8)
#define DFDSPBRCP_BCTL_BYPROM		BIT(0)
#define DFDSPBRCP_BCTL_WAITIPCG		BIT(16)
#define DFDSPBRCP_BCTL_WAITIPPG		BIT(17)

#define DFDSPBRCP_BATTR_LPSCTL_RESTORE_BOOT		BIT(12)
#define DFDSPBRCP_BATTR_LPSCTL_HP_CLOCK_BOOT	BIT(13)
#define DFDSPBRCP_BATTR_LPSCTL_LP_CLOCK_BOOT	BIT(14)
#define DFDSPBRCP_BATTR_LPSCTL_L1_MIN_WAY		BIT(15)
#define DFDSPBRCP_BATTR_LPSCTL_BATTR_SLAVE_CORE	BIT(16)

#define DFDSPBRCP_WDT_RESUME		BIT(8)
#define DFDSPBRCP_WDT_RESTART_COMMAND	0x76

#define DFDSPBRCP (*(volatile struct dfdspbrcp *)DFDSPBRCP_REG)

/* Power control */
#define PWRCTL_REG 0x71b90

struct ace_pwrctl {
	uint16_t wpdsphpxpg : 3;
	uint16_t rsvd3      : 1;
	uint16_t wphstpg    : 1;
	uint16_t rsvd5      : 1;
	uint16_t wphubhppg  : 1;
	uint16_t wpdspulppg : 1;
	uint16_t wpioxpg    : 2;
	uint16_t rsvd11     : 2;
	uint16_t wpmlpg     : 1;
	uint16_t rsvd14     : 2;
	uint16_t phubulppg  : 1;
};

#define ACE_PWRCTL ((volatile struct ace_pwrctl *)PWRCTL_REG)

#define PWRSTS_REG 0x71b92

struct ace_pwrsts {
	uint16_t dsphpxpgs : 4;
	uint16_t hstpgs    : 1;
	uint16_t rsvd5     : 1;
	uint16_t hubhppgs  : 1;
	uint16_t dspulppgs : 1;
	uint16_t ioxpgs    : 4;
	uint16_t mlpgs     : 2;
	uint16_t rsvd14    : 1;
	uint16_t hubulppgs : 1;
};

#define ACE_PWRSTS ((volatile struct ace_pwrsts *)PWRSTS_REG)

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
