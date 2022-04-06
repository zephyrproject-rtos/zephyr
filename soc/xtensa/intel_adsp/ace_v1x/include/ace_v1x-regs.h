/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_
#define ZEPHYR_SOC_INTEL_ADSP_ACE_v1x_REGS_H_

#include <stdint.h>
#include <xtensa/config/core-isa.h>

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

/* Low priority interrupt indices */
#define MTL_INTL_HIPC		0
#define MTL_INTL_SBIPC		1
#define MTL_INTL_ML			2
#define MTL_INTL_IDCA		3
#define MTL_INTL_LPVML		4
#define MTL_INTL_SHA		5
#define MTL_INTL_L1L2M		6
#define MTL_INTL_I2S		7
#define MTL_INTL_DMIC		8
#define MTL_INTL_SNDW		9
#define MTL_INTL_TTS		10
#define MTL_INTL_WDT		11
#define MTL_INTL_HDAHIDMA	12
#define MTL_INTL_HDAHODMA	13
#define MTL_INTL_HDALIDMA	14
#define MTL_INTL_HDALODMA	15
#define MTL_INTL_I3C		16
#define MTL_INTL_GPDMA		17
#define MTL_INTL_PWM		18
#define MTL_INTL_I2C		19
#define MTL_INTL_SPI		20
#define MTL_INTL_UART		21
#define MTL_INTL_GPIO		22
#define MTL_INTL_UAOL		23
#define MTL_INTL_IDCB		24
#define MTL_INTL_DCW		25
#define MTL_INTL_DTF		26
#define MTL_INTL_FLV		27
#define MTL_INTL_DPDMA		28

/* Device interrupt control for the low priority interrupts.  It
 * provides per-core masking and status checking: MTL_DINT is an array
 * of these structs, one per core.  The state is in the bottom bits,
 * indexed by MTL_INTL_*.  Note that some of these use more than one
 * bit to discriminate sources (e.g. TTS's bits 0-2 are
 * timestamp/comparator0/comparator1).  It seems safe to write all 1's
 * to the short to "just enable everything", but drivers should
 * probably implement proper logic.
 *
 * Note that this block is independent of the Designware controller
 * that manages the shared IRQ.  Interrupts need to unmasked in both
 * in order to be delivered to software.  Per simulator source code,
 * this is "upstream" of DW: an interrupt will not be latched into the
 * status registers of the DW controller unless the IE bits here are
 * set.  That seems unlikely to correctly capture the hardware
 * behavior (it would mean that the DW controller was being
 * independently triggered multiple times by each core!).  Beware.
 *
 * Enable an interrupt for a core with e.g.:
 *
 *    MTL_DINT[core_id].ie[MTL_INTL_TTS] = 0xffff;
 */
struct mtl_dint {
	uint16_t ie[32];  /* enable */
	uint16_t is[32];  /* status (potentially masked by ie)   */
	uint16_t irs[32]; /* "raw" status (hardware input state) */
	uint32_t unused[16];
};

/* This register enables (1) or disable (0) the interrupt of
 * each host inter-processor communication capability instance in a single register.
 */
#define DXHIPCIE_REG 0x78840

#define MTL_DINT ((volatile struct mtl_dint *)DXHIPCIE_REG)

/* Convert between IRQ_CONNECT() numbers and MTL_INTL_* interrupts */
#define MTL_IRQ_TO_ZEPHYR(n)   (XCHAL_NUM_INTERRUPTS + (n))
#define MTL_IRQ_FROM_ZEPHYR(n) ((n) - XCHAL_NUM_INTERRUPTS)

/* MTL also has per-core instantiations of a Synopsys interrupt
 * controller.  These inputs (with the same indices as MTL_INTL_*
 * above) are downstream of the DINT layer, and must be independently
 * masked/enabled.  The core Zephyr intc_dw driver unfortunately
 * doesn't understand this kind of MP implementation.  Note also that
 * as instantiated (there are only 28 sources), the high 32 bit
 * registers don't exist and aren't named here.  Access via e.g.:
 *
 *     ACE_INTC[core_id].inten |= interrupt_bit;
 */
struct ace_intc {
	uint32_t inten;
	uint32_t unused0;
	uint32_t intmask;
	uint32_t unused1;
	uint32_t intforce;
	uint32_t unused2;
	uint32_t rawstatus;
	uint32_t unused3;
	uint32_t status;
	uint32_t unused4;
	uint32_t maskstatus;
	uint32_t unused5;
	uint32_t finalstatus;
	uint32_t unused6;
	uint32_t vector;
	uint32_t unused7[33];
	uint32_t fiq_inten;
	uint32_t fiq_intmask;
	uint32_t fiq_intforce;
	uint32_t fiq_rawstatus;
	uint32_t fiq_status;
	uint32_t fiq_finalstatus;
	uint32_t plevel;
	uint32_t unused8;
	uint32_t dsp_ictl_version_id;
	uint32_t unused9[199];
};

#define ACE_INTC ((volatile struct ace_intc *)DT_REG_ADDR(DT_NODELABEL(ace_intc)))

#define ACE_INTC_IRQ DT_IRQN(DT_NODELABEL(ace_intc))

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
