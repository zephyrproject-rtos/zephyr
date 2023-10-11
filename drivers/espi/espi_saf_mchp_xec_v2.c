/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi_saf_v2

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/logging/log.h>

#include "espi_mchp_xec_v2.h"
#include "espi_utils.h"
LOG_MODULE_REGISTER(espi_saf, CONFIG_ESPI_LOG_LEVEL);

/* common clock control device node for all Microchip XEC chips */
#define MCHP_XEC_CLOCK_CONTROL_NODE	DT_NODELABEL(pcr)

/* SAF EC Portal read/write flash access limited to 1-64 bytes */
#define MAX_SAF_ECP_BUFFER_SIZE 64ul

/* 1 second maximum for flash operations */
#define MAX_SAF_FLASH_TIMEOUT 125000ul /* 1000ul */

#define MAX_SAF_FLASH_TIMEOUT_MS 1000ul

/* 64 bytes @ 24MHz quad is approx. 6 us */
#define SAF_WAIT_INTERVAL 8

/* After 8 wait intervals yield */
#define SAF_YIELD_THRESHOLD 64

/* Get QMSPI 0 encoded GIRQ information */
#define XEC_QMSPI_ENC_GIRQ						\
	DT_PROP_BY_IDX(DT_INST(0, microchip_xec_qmspi_ldma), girqs, 0)

#define XEC_QMSPI_GIRQ MCHP_XEC_ECIA_GIRQ(XEC_QMSPI_ENC_GIRQ)
#define XEC_QMSPI_GIRQ_POS MCHP_XEC_ECIA_GIRQ_POS(XEC_QMSPI_ENC_GIRQ)

#define XEC_SAF_DONE_ENC_GIRQ DT_INST_PROP_BY_IDX(0, girqs, 0)
#define XEC_SAF_ERR_ENC_GIRQ DT_INST_PROP_BY_IDX(0, girqs, 1)

#define XEC_SAF_DONE_GIRQ MCHP_XEC_ECIA_GIRQ(XEC_SAF_DONE_ENC_GIRQ)
#define XEC_SAF_DONE_GIRQ_POS MCHP_XEC_ECIA_GIRQ_POS(XEC_SAF_ERR_ENC_GIRQ)

/*
 * SAF configuration from Device Tree
 * SAF controller register block base address
 * QMSPI controller register block base address
 * SAF communications register block base address
 * Flash STATUS1 poll timeout in 32KHz periods
 * Flash consecutive read timeout in units of 20 ns
 * Delay before first Poll-1 command after suspend in 20 ns units
 * Hold off suspend for this interval if erase or program in 32KHz periods.
 * Add delay between Poll STATUS1 commands in 20 ns units.
 */
struct espi_saf_xec_config {
	struct mchp_espi_saf * const saf_base;
	struct qmspi_regs * const qmspi_base;
	struct mchp_espi_saf_comm * const saf_comm_base;
	struct espi_iom_regs * const iom_base;
	void (*irq_config_func)(void);
	uint32_t poll_timeout;
	uint32_t consec_rd_timeout;
	uint32_t sus_chk_delay;
	uint16_t sus_rsm_interval;
	uint16_t poll_interval;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	uint8_t irq_info_size;
	uint8_t rsvd1;
	const struct espi_xec_irq_info *irq_info_list;
};

struct espi_saf_xec_data {
	struct k_sem ecp_lock;
	uint32_t hwstatus;
	sys_slist_t callbacks;
};

/* EC portal local flash r/w buffer */
static uint32_t slave_mem[MAX_SAF_ECP_BUFFER_SIZE];

/*
 * @brief eSPI SAF configuration
 */

static inline void mchp_saf_cs_descr_wr(struct mchp_espi_saf *regs, uint8_t cs,
					uint32_t val)
{
	regs->SAF_CS_OP[cs].OP_DESCR = val;
}

static inline void mchp_saf_poll2_mask_wr(struct mchp_espi_saf *regs, uint8_t cs,
					  uint16_t val)
{
	LOG_DBG("%s cs: %d mask %x", __func__, cs, val);
	if (cs == 0) {
		regs->SAF_CS0_CFG_P2M = val;
	} else {
		regs->SAF_CS1_CFG_P2M = val;
	}
}

static inline void mchp_saf_cm_prefix_wr(struct mchp_espi_saf *regs, uint8_t cs,
					 uint16_t val)
{
	if (cs == 0) {
		regs->SAF_CS0_CM_PRF = val;
	} else {
		regs->SAF_CS1_CM_PRF = val;
	}
}

/*
 * Initialize SAF flash protection regions.
 * SAF HW implements 17 protection regions.
 * At least one protection region must be configured to allow
 * EC access to the local flash through the EC Portal.
 * Each protection region is composed of 4 32-bit registers
 * Start bits[19:0] = bits[31:12] region start address (4KB boundaries)
 * Limit bits[19:0] = bits[31:12] region limit address (4KB boundaries)
 * Write protect b[7:0] = masters[7:0] allow write/erase. 1=allowed
 * Read protetc b[7:0] = masters[7:0] allow read. 1=allowed
 *
 * This routine configures protection region 0 for full flash array
 * address range and read-write-erase for all masters.
 * This routine must be called AFTER the flash configuration size/limit and
 * threshold registers have been programmed.
 *
 * POR default values:
 * Start = 0x7ffff
 * Limit = 0
 * Write Prot = 0x01 Master 0 always granted write/erase
 * Read Prot = 0x01 Master 0 always granted read
 *
 * Sample code configures PR[0]
 * Start = 0
 * Limit = 0x7ffff
 * WR = 0xFF
 * RD = 0xFF
 */
static void saf_protection_regions_init(struct mchp_espi_saf *regs)
{
	LOG_DBG("%s", __func__);

	for (size_t n = 0; n < MCHP_ESPI_SAF_PR_MAX; n++) {
		if (n == 0) {
			regs->SAF_PROT_RG[0].START = 0U;
			regs->SAF_PROT_RG[0].LIMIT =
				regs->SAF_FL_CFG_SIZE_LIM >> 12;
			regs->SAF_PROT_RG[0].WEBM = MCHP_SAF_MSTR_ALL;
			regs->SAF_PROT_RG[0].RDBM = MCHP_SAF_MSTR_ALL;
		} else {
			regs->SAF_PROT_RG[n].START =
				MCHP_SAF_PROT_RG_START_DFLT;
			regs->SAF_PROT_RG[n].LIMIT =
				MCHP_SAF_PROT_RG_LIMIT_DFLT;
			regs->SAF_PROT_RG[n].WEBM = 0U;
			regs->SAF_PROT_RG[n].RDBM = 0U;
		}

		LOG_DBG("PROT[%d] START %x", n, regs->SAF_PROT_RG[n].START);
		LOG_DBG("PROT[%d] LIMIT %x", n, regs->SAF_PROT_RG[n].LIMIT);
		LOG_DBG("PROT[%d] WEBM %x", n, regs->SAF_PROT_RG[n].WEBM);
		LOG_DBG("PROT[%d] RDBM %x", n, regs->SAF_PROT_RG[n].RDBM);
	}
}

static int qmspi_freq_div(uint32_t freqhz, uint32_t *fdiv)
{
	clock_control_subsys_t clkss =
		(clock_control_subsys_t)(MCHP_XEC_PCR_CLK_PERIPH_FAST);
	uint32_t clk = 0u;

	if (!fdiv) {
		return -EINVAL;
	}

	if (clock_control_get_rate(DEVICE_DT_GET(MCHP_XEC_CLOCK_CONTROL_NODE),
				    (clock_control_subsys_t)clkss, &clk)) {
		return -EIO;
	}

	*fdiv = 0u; /* maximum divider = 0x10000 */
	if (freqhz) {
		*fdiv = clk / freqhz;
	}

	return 0u;
}

static int qmspi_freq_div_from_mhz(uint32_t freqmhz, uint32_t *fdiv)
{
	uint32_t freqhz = freqmhz * 1000000u;

	return qmspi_freq_div(freqhz, fdiv);
}

/*
 * Take over and re-initialize QMSPI for use by SAF HW engine.
 * When SAF is activated, QMSPI registers are controlled by SAF
 * HW engine. CPU no longer has access to QMSPI registers.
 * 1. Save QMSPI driver frequency divider, SPI signalling mode, and
 *    chip select timing.
 * 2. Put QMSPI controller in a known state by performing a soft reset.
 * 3. Clear QMSPI GIRQ status
 * 4. Configure QMSPI interface control for SAF.
 * 5. Load flash device independent (generic) descriptors.
 * 6. Enable transfer done interrupt in QMSPI
 * 7. Enable QMSPI SAF mode
 * 8. If user configuration overrides frequency, signalling mode,
 *    or chip select timing derive user values.
 * 9. Program QMSPI MODE and CSTIM registers with activate set.
 */
static int saf_qmspi_init(const struct espi_saf_xec_config *xcfg,
			  const struct espi_saf_cfg *cfg)
{
	uint32_t qmode, qfdiv, cstim, n;
	struct qmspi_regs * const qregs = xcfg->qmspi_base;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	const struct espi_saf_hw_cfg *hwcfg = &cfg->hwcfg;

	qmode = qregs->MODE;
	if (!(qmode & MCHP_QMSPI_M_ACTIVATE)) {
		return -EAGAIN;
	}

	qmode = qregs->MODE & (MCHP_QMSPI_M_FDIV_MASK | MCHP_QMSPI_M_SIG_MASK);
	cstim = qregs->CSTM;
	qregs->MODE = MCHP_QMSPI_M_SRST;
	qregs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	mchp_soc_ecia_girq_src_dis(XEC_QMSPI_GIRQ, XEC_QMSPI_GIRQ_POS);
	mchp_soc_ecia_girq_src_clr(XEC_QMSPI_GIRQ, XEC_QMSPI_GIRQ_POS);

	qregs->IFCTRL =
		(MCHP_QMSPI_IFC_WP_OUT_HI | MCHP_QMSPI_IFC_WP_OUT_EN |
		 MCHP_QMSPI_IFC_HOLD_OUT_HI | MCHP_QMSPI_IFC_HOLD_OUT_EN);

	for (n = 0; n < MCHP_SAF_NUM_GENERIC_DESCR; n++) {
		qregs->DESCR[MCHP_SAF_CM_EXIT_START_DESCR + n] =
			hwcfg->generic_descr[n];
	}

	/* SAF HW uses QMSPI interrupt signal */
	qregs->IEN = MCHP_QMSPI_IEN_XFR_DONE;

	qmode |= (MCHP_QMSPI_M_SAF_DMA_MODE_EN | MCHP_QMSPI_M_CS0 |
		  MCHP_QMSPI_M_ACTIVATE);

	if (hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_CPHA) {
		qmode = (qmode & ~(MCHP_QMSPI_M_SIG_MASK)) |
			((hwcfg->qmspi_cpha << MCHP_QMSPI_M_SIG_POS) &
			 MCHP_QMSPI_M_SIG_MASK);
	}


	/* Copy QMSPI frequency divider into SAF CS0 and CS1 QMSPI frequency
	 * dividers. SAF HW uses CS0/CS1 divider register fields to overwrite
	 * QMSPI frequency divider in QMSPI.Mode register. Later we will update
	 * SAF CS0/CS1 SPI frequency dividers based on flash configuration.
	 */
	qfdiv = (qmode & MCHP_QMSPI_M_FDIV_MASK) >> MCHP_QMSPI_M_FDIV_POS;
	qfdiv = qfdiv | (qfdiv << 16); /* read and rest clock dividers */
	regs->SAF_CLKDIV_CS0 = qfdiv;
	regs->SAF_CLKDIV_CS1 = qfdiv;

	if (hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_CSTM) {
		cstim = hwcfg->qmspi_cs_timing;
	}

	/* MEC172x SAF uses TX LDMA channel 0 in non-descriptor mode.
	 * SAF HW writes QMSPI.Control and TX LDMA channel 0 registers
	 * to transmit opcode, address, and data. We configure must
	 * configure TX LDMA channel 0 control register. We believe SAF
	 * HW will set bit[6] to 1.
	 */
	qregs->LDTX[0].CTRL = MCHP_QMSPI_LDC_EN | MCHP_QMSPI_LDC_RS_EN | MCHP_QMSPI_LDC_ASZ_4;

	qmode |= MCHP_QMSPI_M_LDMA_RX_EN | MCHP_QMSPI_M_LDMA_TX_EN;

	qregs->MODE = qmode;
	qregs->CSTM = cstim;

	return 0;
}

/*
 * Registers at offsets:
 * SAF Poll timeout @ 0x194.  Hard coded to 0x28000. Default value = 0.
 *	recommended value = 0x28000 32KHz clocks (5 seconds). b[17:0]
 * SAF Poll interval @ 0x198.  Hard coded to 0
 *	Default value = 0. Recommended = 0. b[15:0]
 * SAF Suspend/Resume Interval @ 0x19c.  Hard coded to 0x8
 *	Default value = 0x01. Min time erase/prog in 32KHz units.
 * SAF Consecutive Read Timeout @ 0x1a0. Hard coded to 0x2. b[15:0]
 *	Units of MCLK. Recommend < 20us. b[19:0]
 * SAF Suspend Check Delay @ 0x1ac. Not touched.
 *	Default = 0. Recommend = 20us. Units = MCLK. b[19:0]
 */
static void saf_flash_timing_init(struct mchp_espi_saf * const regs,
				  const struct espi_saf_xec_config *cfg)
{
	LOG_DBG("%s\n", __func__);
	regs->SAF_POLL_TMOUT = cfg->poll_timeout;
	regs->SAF_POLL_INTRVL = cfg->poll_interval;
	regs->SAF_SUS_RSM_INTRVL = cfg->sus_rsm_interval;
	regs->SAF_CONSEC_RD_TMOUT = cfg->consec_rd_timeout;
	regs->SAF_SUS_CHK_DLY = cfg->sus_chk_delay;
	LOG_DBG("SAF_POLL_TMOUT %x\n", regs->SAF_POLL_TMOUT);
	LOG_DBG("SAF_POLL_INTRVL %x\n", regs->SAF_POLL_INTRVL);
	LOG_DBG("SAF_SUS_RSM_INTRVL %x\n", regs->SAF_SUS_RSM_INTRVL);
	LOG_DBG("SAF_CONSEC_RD_TMOUT %x\n", regs->SAF_CONSEC_RD_TMOUT);
	LOG_DBG("SAF_SUS_CHK_DLY %x\n", regs->SAF_SUS_CHK_DLY);
}

/*
 * Disable DnX bypass feature.
 */
static void saf_dnx_bypass_init(struct mchp_espi_saf * const regs)
{
	regs->SAF_DNX_PROT_BYP = 0;
	regs->SAF_DNX_PROT_BYP = 0xffffffff;
}

/*
 * Bitmap of flash erase size from 1KB up to 128KB.
 * eSPI SAF specification requires 4KB erase support.
 * MCHP SAF supports 4KB, 32KB, and 64KB.
 * Only report 32KB and 64KB to Host if supported by both
 * flash devices.
 */
static int saf_init_erase_block_size(const struct device *dev, const struct espi_saf_cfg *cfg)
{
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct espi_iom_regs * const espi_iom = xcfg->iom_base;
	struct espi_saf_flash_cfg *fcfg = cfg->flash_cfgs;
	uint32_t opb = fcfg->opb;
	uint8_t erase_bitmap = MCHP_ESPI_SERASE_SZ_4K;

	LOG_DBG("%s\n", __func__);

	if (cfg->nflash_devices > 1) {
		fcfg++;
		opb &= fcfg->opb;
	}

	if ((opb & MCHP_SAF_CS_OPB_ER0_MASK) == 0) {
		/* One or both do not support 4KB erase! */
		return -EINVAL;
	}

	if (opb & MCHP_SAF_CS_OPB_ER1_MASK) {
		erase_bitmap |= MCHP_ESPI_SERASE_SZ_32K;
	}

	if (opb & MCHP_SAF_CS_OPB_ER2_MASK) {
		erase_bitmap |= MCHP_ESPI_SERASE_SZ_64K;
	}

	espi_iom->SAFEBS = erase_bitmap;

	return 0;
}

/*
 * Set the continuous mode prefix and 4-byte address mode bits
 * based upon the flash configuration information.
 * Updates:
 * SAF Flash Config Poll2 Mask @ 0x1A4
 * SAF Flash Config Special Mode @ 0x1B0
 * SAF Flash Misc Config @ 0x38
 */
static void saf_flash_misc_cfg(struct mchp_espi_saf * const regs, uint8_t cs,
			       const struct espi_saf_flash_cfg *fcfg)
{
	uint32_t d, v;

	d = regs->SAF_FL_CFG_MISC;

	v = MCHP_SAF_FL_CFG_MISC_CS0_CPE;
	if (cs) {
		v = MCHP_SAF_FL_CFG_MISC_CS1_CPE;
	}

	/* Does this flash device require a prefix for continuous mode? */
	if (fcfg->cont_prefix != 0) {
		d |= v;
	} else {
		d &= ~v;
	}

	v = MCHP_SAF_FL_CFG_MISC_CS0_4BM;
	if (cs) {
		v = MCHP_SAF_FL_CFG_MISC_CS1_4BM;
	}

	/* Use 32-bit addressing for this flash device? */
	if (fcfg->flags & MCHP_FLASH_FLAG_ADDR32) {
		d |= v;
	} else {
		d &= ~v;
	}

	regs->SAF_FL_CFG_MISC = d;
	LOG_DBG("%s SAF_FL_CFG_MISC: %x", __func__, d);
}

static void saf_flash_pd_cfg(struct mchp_espi_saf * const regs, uint8_t cs,
			     const struct espi_saf_flash_cfg *fcfg)
{
	uint32_t pdval = 0u;
	uint32_t msk = 0u;

	if (cs == 0) {
		msk = BIT(SAF_PWRDN_CTRL_CS0_PD_EN_POS) | BIT(SAF_PWRDN_CTRL_CS0_PD_EN_POS);
		if (fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS0_EN) {
			pdval |= BIT(SAF_PWRDN_CTRL_CS0_PD_EN_POS);
		}
		if (fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS0_EC_WK_EN) {
			pdval |= BIT(SAF_PWRDN_CTRL_CS0_WPA_EN_POS);
		}
	} else {
		msk = BIT(SAF_PWRDN_CTRL_CS1_PD_EN_POS) | BIT(SAF_PWRDN_CTRL_CS1_PD_EN_POS);
		if (fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS1_EN) {
			pdval |= BIT(SAF_PWRDN_CTRL_CS1_PD_EN_POS);
		}
		if (fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS1_EC_WK_EN) {
			pdval |= BIT(SAF_PWRDN_CTRL_CS1_PD_EN_POS);
		}
	}

	regs->SAF_PWRDN_CTRL = (regs->SAF_PWRDN_CTRL & ~msk) | pdval;
}

/* Configure SAF per chip select QMSPI clock dividers.
 * SAF HW implements two QMSP clock divider registers per chip select:
 * Each divider register is composed of two 16-bit fields:
 *   b[15:0] = QMSPI clock divider for SPI read
 *   b[31:16] = QMSPI clock divider for all other SPI commands
 */
static int saf_flash_freq_cfg(struct mchp_espi_saf * const regs, uint8_t cs,
			       const struct espi_saf_flash_cfg *fcfg)
{
	uint32_t fmhz, fdiv, saf_qclk;

	if (cs == 0) {
		saf_qclk = regs->SAF_CLKDIV_CS0;
	} else {
		saf_qclk = regs->SAF_CLKDIV_CS1;
	}

	fmhz = fcfg->rd_freq_mhz;
	if (fmhz) {
		fdiv = 0u;
		if (qmspi_freq_div_from_mhz(fmhz, &fdiv)) {
			LOG_ERR("%s SAF CLKDIV CS0 bad freq MHz %u",
				__func__, fmhz);
			return -EIO;
		}
		if (fdiv) {
			saf_qclk = (saf_qclk & ~SAF_CLKDIV_CS_MSK0) |
				   (fdiv & SAF_CLKDIV_CS_MSK0);
		}
	}

	fmhz = fcfg->freq_mhz;
	if (fmhz) {
		fdiv = 0u;
		if (qmspi_freq_div_from_mhz(fmhz, &fdiv)) {
			LOG_ERR("%s SAF CLKDIV CS1 bad freq MHz %u",
				__func__, fmhz);
			return -EIO;
		}
		if (fdiv) {
			saf_qclk &= ~(SAF_CLKDIV_CS_MSK0 << 16);
			saf_qclk |= (fdiv & SAF_CLKDIV_CS_MSK0) << 16;
		}
	}

	if (cs == 0) {
		regs->SAF_CLKDIV_CS0 = saf_qclk;
	} else {
		regs->SAF_CLKDIV_CS1 = saf_qclk;
	}

	return 0;
}

/*
 * Program flash device specific SAF and QMSPI registers.
 *
 * CS0 OpA @ 0x4c or CS1 OpA @ 0x5C
 * CS0 OpB @ 0x50 or CS1 OpB @ 0x60
 * CS0 OpC @ 0x54 or CS1 OpC @ 0x64
 * Poll 2 Mask @ 0x1a4
 * Continuous Prefix @ 0x1b0
 * CS0: QMSPI descriptors 0-5 or CS1 QMSPI descriptors 6-11
 * CS0 Descrs @ 0x58 or CS1 Descrs @ 0x68
 * SAF CS0 QMSPI frequency dividers (read/all other) commands
 * SAF CS1 QMSPI frequency dividers (read/all other) commands
 */
static int saf_flash_cfg(const struct device *dev,
			 const struct espi_saf_flash_cfg *fcfg, uint8_t cs)
{
	uint32_t d, did;
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	struct qmspi_regs * const qregs = xcfg->qmspi_base;

	LOG_DBG("%s cs=%u", __func__, cs);

	regs->SAF_CS_OP[cs].OPA = fcfg->opa;
	regs->SAF_CS_OP[cs].OPB = fcfg->opb;
	regs->SAF_CS_OP[cs].OPC = fcfg->opc;
	regs->SAF_CS_OP[cs].OP_DESCR = (uint32_t)fcfg->cs_cfg_descr_ids;

	did = MCHP_SAF_QMSPI_CS0_START_DESCR;
	if (cs != 0) {
		did = MCHP_SAF_QMSPI_CS1_START_DESCR;
	}

	for (size_t i = 0; i < MCHP_SAF_QMSPI_NUM_FLASH_DESCR; i++) {
		d = fcfg->descr[i] & ~(MCHP_QMSPI_C_NEXT_DESCR_MASK);
		d |= (((did + 1) << MCHP_QMSPI_C_NEXT_DESCR_POS) &
		      MCHP_QMSPI_C_NEXT_DESCR_MASK);
		qregs->DESCR[did++] = d;
	}

	mchp_saf_poll2_mask_wr(regs, cs, fcfg->poll2_mask);
	mchp_saf_cm_prefix_wr(regs, cs, fcfg->cont_prefix);
	saf_flash_misc_cfg(regs, cs, fcfg);
	saf_flash_pd_cfg(regs, cs, fcfg);

	return saf_flash_freq_cfg(regs, cs, fcfg);
}

static const uint32_t tag_map_dflt[MCHP_ESPI_SAF_TAGMAP_MAX] = {
	MCHP_SAF_TAG_MAP0_DFLT, MCHP_SAF_TAG_MAP1_DFLT, MCHP_SAF_TAG_MAP2_DFLT
};

static void saf_tagmap_init(struct mchp_espi_saf * const regs,
			    const struct espi_saf_cfg *cfg)
{
	const struct espi_saf_hw_cfg *hwcfg = &cfg->hwcfg;

	for (int i = 0; i < MCHP_ESPI_SAF_TAGMAP_MAX; i++) {
		if (hwcfg->tag_map[i] & MCHP_SAF_HW_CFG_TAGMAP_USE) {
			regs->SAF_TAG_MAP[i] = hwcfg->tag_map[i];
		} else {
			regs->SAF_TAG_MAP[i] = tag_map_dflt[i];
		}
	}

	LOG_DBG("SAF TAG0 %x", regs->SAF_TAG_MAP[0]);
	LOG_DBG("SAF TAG1 %x", regs->SAF_TAG_MAP[1]);
	LOG_DBG("SAF TAG2 %x", regs->SAF_TAG_MAP[2]);
}

#define SAF_QSPI_LDMA_CTRL						\
	(MCHP_QMSPI_LDC_EN | MCHP_QMSPI_LDC_RS_EN |			\
	 MCHP_QMSPI_LDC_ASZ_4)

static void saf_qmspi_ldma_cfg(const struct espi_saf_xec_config * const xcfg)
{
	struct qmspi_regs * const qregs = xcfg->qmspi_base;
	uint32_t qmode = qregs->MODE;
	uint32_t n, temp, chan;

	qregs->MODE = qmode & ~(MCHP_QMSPI_M_ACTIVATE);

	for (n = 0u; n < MCHP_QMSPI_MAX_DESCR; n++) {
		temp = qregs->DESCR[n];
		if (temp & MCHP_QMSPI_C_TX_MASK) {
			chan = (temp & MCHP_QMSPI_C_TX_DMA_MASK) >> MCHP_QMSPI_C_TX_DMA_POS;
			if (chan) { /* zero is disabled */
				chan--; /* register array index starts at 0 */
				qregs->LDMA_TX_DESCR_BM |= BIT(n);
				qregs->LDTX[chan].CTRL = SAF_QSPI_LDMA_CTRL;
			}
		}
		if (temp & MCHP_QMSPI_C_RX_EN) {
			chan = (temp & MCHP_QMSPI_C_RX_DMA_MASK) >> MCHP_QMSPI_C_RX_DMA_POS;
			if (chan) {
				chan--;
				qregs->LDMA_RX_DESCR_BM |= BIT(n);
				qregs->LDRX[chan].CTRL = SAF_QSPI_LDMA_CTRL;
			}
		}
	}

	qregs->MODE = qmode;
}

/*
 * Configure SAF and QMSPI for SAF operation based upon the
 * number and characteristics of local SPI flash devices.
 * NOTE: SAF is configured but not activated. SAF should be
 * activated only when eSPI master sends Flash Channel enable
 * message with MAF/SAF select flag.
 */
static int espi_saf_xec_configuration(const struct device *dev,
				      const struct espi_saf_cfg *cfg)
{
	int ret = 0;
	uint32_t totalsz = 0;
	uint32_t u = 0;

	LOG_DBG("%s", __func__);

	if ((dev == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	struct mchp_espi_saf_comm * const comm_regs = xcfg->saf_comm_base;
	const struct espi_saf_hw_cfg *hwcfg = &cfg->hwcfg;
	const struct espi_saf_flash_cfg *fcfg = cfg->flash_cfgs;

	if ((fcfg == NULL) || (cfg->nflash_devices == 0U) ||
	    (cfg->nflash_devices > MCHP_SAF_MAX_FLASH_DEVICES)) {
		return -EINVAL;
	}

	if (regs->SAF_FL_CFG_MISC & MCHP_SAF_FL_CFG_MISC_SAF_EN) {
		return -EAGAIN;
	}

	saf_qmspi_init(xcfg, cfg);

	regs->SAF_CS0_CFG_P2M = 0;
	regs->SAF_CS1_CFG_P2M = 0;

	regs->SAF_FL_CFG_GEN_DESCR = MCHP_SAF_FL_CFG_GEN_DESCR_STD;

	/* global flash power down activity counter and interval time */
	regs->SAF_AC_RELOAD = hwcfg->flash_pd_timeout;
	regs->SAF_FL_PWR_TMOUT = hwcfg->flash_pd_min_interval;

	/* flash device connected to CS0 required */
	totalsz = fcfg->flashsz;
	regs->SAF_FL_CFG_THRH = totalsz;
	ret = saf_flash_cfg(dev, fcfg, 0);
	if (ret) {
		return ret;
	}

	/* optional second flash device connected to CS1 */
	if (cfg->nflash_devices > 1) {
		fcfg++;
		totalsz += fcfg->flashsz;
	}
	/* Program CS1 configuration (same as CS0 if only one device) */
	ret = saf_flash_cfg(dev, fcfg, 1);
	if (ret) {
		return ret;
	}

	if (totalsz == 0) {
		return -EAGAIN;
	}

	regs->SAF_FL_CFG_SIZE_LIM = totalsz - 1;

	LOG_DBG("SAF_FL_CFG_THRH = %x SAF_FL_CFG_SIZE_LIM = %x",
		regs->SAF_FL_CFG_THRH, regs->SAF_FL_CFG_SIZE_LIM);

	saf_tagmap_init(regs, cfg);

	saf_protection_regions_init(regs);

	saf_dnx_bypass_init(regs);

	saf_flash_timing_init(regs, xcfg);

	ret = saf_init_erase_block_size(dev, cfg);
	if (ret != 0) {
		LOG_ERR("SAF Config bad flash erase config");
		return ret;
	}

	/* Default or expedited prefetch? */
	u = MCHP_SAF_FL_CFG_MISC_PFOE_DFLT;
	if (cfg->hwcfg.flags & MCHP_SAF_HW_CFG_FLAG_PFEXP) {
		u = MCHP_SAF_FL_CFG_MISC_PFOE_EXP;
	}

	regs->SAF_FL_CFG_MISC =
		(regs->SAF_FL_CFG_MISC & ~(MCHP_SAF_FL_CFG_MISC_PFOE_MASK)) | u;

	/* enable prefetch ? */
	if (cfg->hwcfg.flags & MCHP_SAF_HW_CFG_FLAG_PFEN) {
		comm_regs->SAF_COMM_MODE |= MCHP_SAF_COMM_MODE_PF_EN;
	} else {
		comm_regs->SAF_COMM_MODE &= ~(MCHP_SAF_COMM_MODE_PF_EN);
	}

	LOG_DBG("%s SAF_FL_CFG_MISC: %x", __func__, regs->SAF_FL_CFG_MISC);
	LOG_DBG("%s Aft MCHP_SAF_COMM_MODE_REG: %x", __func__,
		comm_regs->SAF_COMM_MODE);

	saf_qmspi_ldma_cfg(xcfg);

	return 0;
}

static int espi_saf_xec_set_pr(const struct device *dev,
			       const struct espi_saf_protection *pr)
{
	if ((dev == NULL) || (pr == NULL)) {
		return -EINVAL;
	}

	if (pr->nregions >= MCHP_ESPI_SAF_PR_MAX) {
		return -EINVAL;
	}

	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct mchp_espi_saf * const regs = xcfg->saf_base;

	if (regs->SAF_FL_CFG_MISC & MCHP_SAF_FL_CFG_MISC_SAF_EN) {
		return -EAGAIN;
	}

	const struct espi_saf_pr *preg = pr->pregions;
	size_t n = pr->nregions;

	while (n--) {
		uint8_t regnum = preg->pr_num;

		if (regnum >= MCHP_ESPI_SAF_PR_MAX) {
			return -EINVAL;
		}

		/* NOTE: If previously locked writes have no effect */
		if (preg->flags & MCHP_SAF_PR_FLAG_ENABLE) {
			regs->SAF_PROT_RG[regnum].START = preg->start >> 12U;
			regs->SAF_PROT_RG[regnum].LIMIT =
				(preg->start + preg->size - 1U) >> 12U;
			regs->SAF_PROT_RG[regnum].WEBM = preg->master_bm_we;
			regs->SAF_PROT_RG[regnum].RDBM = preg->master_bm_rd;
		} else {
			regs->SAF_PROT_RG[regnum].START = 0x7FFFFU;
			regs->SAF_PROT_RG[regnum].LIMIT = 0U;
			regs->SAF_PROT_RG[regnum].WEBM = 0U;
			regs->SAF_PROT_RG[regnum].RDBM = 0U;
		}

		if (preg->flags & MCHP_SAF_PR_FLAG_LOCK) {
			regs->SAF_PROT_LOCK |= (1UL << regnum);
		}

		preg++;
	}

	return 0;
}

static bool espi_saf_xec_channel_ready(const struct device *dev)
{
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct mchp_espi_saf * const regs = xcfg->saf_base;

	if (regs->SAF_FL_CFG_MISC & MCHP_SAF_FL_CFG_MISC_SAF_EN) {
		return true;
	}

	return false;
}

/*
 * MCHP SAF hardware supports a range of flash block erase
 * sizes from 1KB to 128KB. The eSPI Host specification requires
 * 4KB must be supported. The MCHP SAF QMSPI HW interface only
 * supported three erase sizes. Most SPI flash devices chosen for
 * SAF support 4KB, 32KB, and 64KB.
 * Get flash erase sizes driver has configured from eSPI capabilities
 * registers. We assume driver flash tables have opcodes to match
 * capabilities configuration.
 * Check requested erase size is supported.
 */
struct erase_size_encoding {
	uint8_t hwbitpos;
	uint8_t encoding;
};

static const struct erase_size_encoding ersz_enc[] = {
	{ MCHP_ESPI_SERASE_SZ_4K_BITPOS, 0 },
	{ MCHP_ESPI_SERASE_SZ_32K_BITPOS, 1 },
	{ MCHP_ESPI_SERASE_SZ_64K_BITPOS, 2 }
};

#define SAF_ERASE_ENCODING_MAX_ENTRY                                           \
	(sizeof(ersz_enc) / sizeof(struct erase_size_encoding))

static uint32_t get_erase_size_encoding(const struct device *dev, uint32_t erase_size)
{
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct espi_iom_regs * const espi_iom = xcfg->iom_base;
	uint8_t supsz = espi_iom->SAFEBS;

	LOG_DBG("%s\n", __func__);
	for (int i = 0; i < SAF_ERASE_ENCODING_MAX_ENTRY; i++) {
		uint32_t sz = MCHP_ESPI_SERASE_SZ(ersz_enc[i].hwbitpos);

		if ((sz == erase_size) &&
		    (supsz & (1 << ersz_enc[i].hwbitpos))) {
			return ersz_enc[i].encoding;
		}
	}

	return 0xffffffffU;
}

static int check_ecp_access_size(uint32_t reqlen)
{
	if ((reqlen < MCHP_SAF_ECP_CMD_RW_LEN_MIN) ||
	    (reqlen > MCHP_SAF_ECP_CMD_RW_LEN_MAX)) {
		return -EAGAIN;
	}

	return 0;
}

/*
 * EC access to SAF atttached flash array
 * Allowed commands:
 * MCHP_SAF_ECP_CMD_READ(0x0), MCHP_SAF_ECP_CMD_WRITE(0x01),
 * MCHP_SAF_ECP_CMD_ERASE(0x02), MCHP_SAF_ECP_CMD_RPMC_OP1_CS0(0x03),
 * MCHP_SAF_ECP_CMD_RPMC_OP2_CS0(0x04), MCHP_SAF_ECP_CMD_RPMC_OP1_CS1(0x83),
 * MCHP_SAF_ECP_CMD_RPMC_OP2_CS1(0x84)
 */
static int saf_ecp_access(const struct device *dev,
			  struct espi_saf_packet *pckt, uint8_t cmd)
{
	uint32_t scmd, err_mask, n;
	int rc, counter;
	struct espi_saf_xec_data *xdat = dev->data;
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	const struct espi_xec_irq_info *safirq = &xcfg->irq_info_list[0];

	counter = 0;
	err_mask = MCHP_SAF_ECP_STS_ERR_MASK;

	LOG_DBG("%s", __func__);

	if (!(regs->SAF_FL_CFG_MISC & MCHP_SAF_FL_CFG_MISC_SAF_EN)) {
		LOG_ERR("SAF is disabled");
		return -EIO;
	}

	n = regs->SAF_ECP_BUSY;
	if (n & (MCHP_SAF_ECP_EC0_BUSY | MCHP_SAF_ECP_EC1_BUSY)) {
		LOG_ERR("SAF EC Portal is busy: 0x%08x", n);
		return -EBUSY;
	}

	switch (cmd) {
	case MCHP_SAF_ECP_CMD_READ:
	case MCHP_SAF_ECP_CMD_WRITE:
		rc = check_ecp_access_size(pckt->len);
		if (rc) {
			LOG_ERR("SAF EC Portal size out of bounds");
			return rc;
		}

		if (cmd == MCHP_SAF_ECP_CMD_WRITE) {
			memcpy(slave_mem, pckt->buf, pckt->len);
		}

		n = pckt->len;
		break;
	case MCHP_SAF_ECP_CMD_ERASE:
		n = get_erase_size_encoding(dev, pckt->len);
		if (n == UINT32_MAX) {
			LOG_ERR("SAF EC Portal unsupported erase size");
			return -EAGAIN;
		}
		break;
	case MCHP_SAF_ECP_CMD_RPMC_OP1_CS0:
	case MCHP_SAF_ECP_CMD_RPMC_OP2_CS0:
		rc = check_ecp_access_size(pckt->len);
		if (rc) {
			LOG_ERR("SAF EC Portal RPMC size out of bounds");
			return rc;
		}
		if (!(regs->SAF_CFG_CS0_OPD & SAF_CFG_CS_OPC_RPMC_OP2_MSK)) {
			LOG_ERR("SAF CS0 RPMC opcode not configured");
			return -EIO;
		}
		n = pckt->len;
		break;
	case MCHP_SAF_ECP_CMD_RPMC_OP1_CS1:
	case MCHP_SAF_ECP_CMD_RPMC_OP2_CS1:
		rc = check_ecp_access_size(pckt->len);
		if (rc) {
			LOG_ERR("SAF EC Portal RPMC size out of bounds");
			return rc;
		}
		if (!(regs->SAF_CFG_CS1_OPD & SAF_CFG_CS_OPC_RPMC_OP2_MSK)) {
			LOG_ERR("SAF CS1 RPMC opcode not configured");
			return -EIO;
		}
		n = pckt->len;
		break;
	default:
		LOG_ERR("SAF EC Portal bad cmd");
		return -EAGAIN;
	}

	LOG_DBG("%s params val done", __func__);

	regs->SAF_ECP_INTEN = 0;
	regs->SAF_ECP_STATUS = MCHP_SAF_ECP_STS_MASK;
	mchp_xec_ecia_girq_src_clr(safirq->gid, safirq->gpos);

	regs->SAF_ECP_INTEN = BIT(MCHP_SAF_ECP_INTEN_DONE_POS);

	regs->SAF_ECP_FLAR = pckt->flash_addr;
	regs->SAF_ECP_BFAR = (uint32_t)&slave_mem[0];

	scmd = MCHP_SAF_ECP_CMD_PUT_FLASH_NP |
		((uint32_t)cmd << MCHP_SAF_ECP_CMD_CTYPE_POS) |
		((n << MCHP_SAF_ECP_CMD_LEN_POS) & MCHP_SAF_ECP_CMD_LEN_MASK);

	LOG_DBG("%s ECP_FLAR=0x%x", __func__, regs->SAF_ECP_FLAR);
	LOG_DBG("%s ECP_BFAR=0x%x", __func__, regs->SAF_ECP_BFAR);
	LOG_DBG("%s ECP_CMD=0x%x", __func__, scmd);

	regs->SAF_ECP_CMD = scmd;
	regs->SAF_ECP_START = MCHP_SAF_ECP_START;

	rc = k_sem_take(&xdat->ecp_lock, K_MSEC(MAX_SAF_FLASH_TIMEOUT_MS));
	if (rc == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	LOG_DBG("%s wake on semaphore", __func__);

	n = regs->SAF_ECP_STATUS;
	/* clear hardware status and check for errors */
	if (n & err_mask) {
		regs->SAF_ECP_STATUS = n;
		LOG_ERR("%s error %x", __func__, n);
		return -EIO;
	}

	if (cmd == MCHP_SAF_ECP_CMD_READ) {
		memcpy(pckt->buf, slave_mem, pckt->len);
	}

	return rc;
}

/* Flash read using SAF EC Portal */
static int saf_xec_flash_read(const struct device *dev,
			      struct espi_saf_packet *pckt)
{
	LOG_DBG("%s", __func__);
	return saf_ecp_access(dev, pckt, MCHP_SAF_ECP_CMD_READ);
}

/* Flash write using SAF EC Portal */
static int saf_xec_flash_write(const struct device *dev,
			       struct espi_saf_packet *pckt)
{
	return saf_ecp_access(dev, pckt, MCHP_SAF_ECP_CMD_WRITE);
}

/* Flash erase using SAF EC Portal */
static int saf_xec_flash_erase(const struct device *dev,
			       struct espi_saf_packet *pckt)
{
	return saf_ecp_access(dev, pckt, MCHP_SAF_ECP_CMD_ERASE);
}

static int espi_saf_xec_manage_callback(const struct device *dev,
					struct espi_callback *callback,
					bool set)
{
	struct espi_saf_xec_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_saf_xec_activate(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	const struct espi_xec_irq_info *safirq = &xcfg->irq_info_list[1];

	regs->SAF_ESPI_MON_STATUS = MCHP_SAF_ESPI_MON_STS_IEN_MSK;
	mchp_xec_ecia_girq_src_clr(safirq->gid, safirq->gpos);

	regs->SAF_FL_CFG_MISC |= MCHP_SAF_FL_CFG_MISC_SAF_EN;
	regs->SAF_ESPI_MON_INTEN = (BIT(MCHP_SAF_ESPI_MON_STS_IEN_TMOUT_POS) |
				    BIT(MCHP_SAF_ESPI_MON_STS_IEN_OOR_POS) |
				    BIT(MCHP_SAF_ESPI_MON_STS_IEN_AV_POS) |
				    BIT(MCHP_SAF_ESPI_MON_STS_IEN_BND_4K_POS) |
				    BIT(MCHP_SAF_ESPI_MON_STS_IEN_ERSZ_POS));

	k_busy_wait(1000); /* TODO FIXME get estimate of time interval */

	return 0;
}

static void espi_saf_done_isr(const struct device *dev)
{
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct espi_saf_xec_data *data = dev->data;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	const struct espi_xec_irq_info *safirq = &xcfg->irq_info_list[0];
	uint32_t ecp_status = regs->SAF_ECP_STATUS;
	struct espi_event evt = { .evt_type = ESPI_BUS_SAF_NOTIFICATION,
				  .evt_details = BIT(0),
				  .evt_data = ecp_status };

	regs->SAF_ECP_INTEN = 0u;
	regs->SAF_ECP_STATUS = BIT(MCHP_SAF_ECP_STS_DONE_POS);
	mchp_xec_ecia_girq_src_clr(safirq->gid, safirq->gpos);

	data->hwstatus = ecp_status;

	LOG_DBG("SAF Done ISR: status=0x%x", ecp_status);

	espi_send_callbacks(&data->callbacks, dev, evt);

	k_sem_give(&data->ecp_lock);
}

static void espi_saf_err_isr(const struct device *dev)
{
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct espi_saf_xec_data *data = dev->data;
	struct mchp_espi_saf * const regs = xcfg->saf_base;
	const struct espi_xec_irq_info *safirq = &xcfg->irq_info_list[1];
	uint32_t mon_status = regs->SAF_ESPI_MON_STATUS;
	struct espi_event evt = { .evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				  .evt_details = BIT(7),
				  .evt_data = mon_status };

	regs->SAF_ESPI_MON_STATUS = mon_status;
	mchp_xec_ecia_girq_src_clr(safirq->gid, safirq->gpos);

	data->hwstatus = mon_status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static const struct espi_saf_driver_api espi_saf_xec_driver_api = {
	.config = espi_saf_xec_configuration,
	.set_protection_regions = espi_saf_xec_set_pr,
	.activate = espi_saf_xec_activate,
	.get_channel_status = espi_saf_xec_channel_ready,
	.flash_read = saf_xec_flash_read,
	.flash_write = saf_xec_flash_write,
	.flash_erase = saf_xec_flash_erase,
	.manage_callback = espi_saf_xec_manage_callback,
};

static int espi_saf_xec_init(const struct device *dev)
{
	const struct espi_saf_xec_config * const xcfg = dev->config;
	struct espi_saf_xec_data * const data = dev->data;
	struct espi_iom_regs * const espi_iom = xcfg->iom_base;

	/* ungate SAF clocks by disabling PCR sleep enable */
	z_mchp_xec_pcr_periph_sleep(xcfg->pcr_idx, xcfg->pcr_pos, 0);

	/* Configure the channels and its capabilities based on build config */
	espi_iom->CAP0 |= MCHP_ESPI_GBL_CAP0_FC_SUPP;
	espi_iom->CAPFC &= ~(MCHP_ESPI_FC_CAP_SHARE_MASK);
	espi_iom->CAPFC |= MCHP_ESPI_FC_CAP_SHARE_MAF_SAF;

	xcfg->irq_config_func();

	k_sem_init(&data->ecp_lock, 0, 1);

	return 0;
}


/* n = node-id, p = property, i = index */
#define XEC_SAF_IRQ_INFO(n, p, i)					    \
	{								    \
		.gid = MCHP_XEC_ECIA_GIRQ(DT_PROP_BY_IDX(n, p, i)),	    \
		.gpos = MCHP_XEC_ECIA_GIRQ_POS(DT_PROP_BY_IDX(n, p, i)),    \
		.anid = MCHP_XEC_ECIA_NVIC_AGGR(DT_PROP_BY_IDX(n, p, i)),   \
		.dnid = MCHP_XEC_ECIA_NVIC_DIRECT(DT_PROP_BY_IDX(n, p, i)), \
	},

#define ESPI_SAF_XEC_DEVICE(n)								\
											\
	static struct espi_saf_xec_data espisaf_xec_data_##n;				\
											\
	static void espi_saf_xec_connect_irqs_##n(void);				\
											\
	static const struct espi_xec_irq_info espi_saf_xec_irq_info_##n[] = {		\
		DT_INST_FOREACH_PROP_ELEM(n, girqs, XEC_SAF_IRQ_INFO)			\
	};										\
											\
	static const struct espi_saf_xec_config espisaf_xec_config_##n = {		\
		.saf_base = (struct mchp_espi_saf * const)(				\
					DT_INST_REG_ADDR_BY_IDX(n, 0)),			\
		.qmspi_base = (struct qmspi_regs * const)(				\
						DT_INST_REG_ADDR_BY_IDX(n, 1)),		\
		.saf_comm_base = (struct mchp_espi_saf_comm * const)(			\
							DT_INST_REG_ADDR_BY_IDX(n, 2)),	\
		.iom_base = (struct espi_iom_regs * const)(				\
					DT_REG_ADDR_BY_NAME(DT_INST_PARENT(n), io)),	\
		.poll_timeout = DT_INST_PROP_OR(n, poll_timeout,			\
						MCHP_SAF_FLASH_POLL_TIMEOUT),		\
		.consec_rd_timeout = DT_INST_PROP_OR(					\
			n, consec_rd_timeout, MCHP_SAF_FLASH_CONSEC_READ_TIMEOUT),	\
		.sus_chk_delay = DT_INST_PROP_OR(n, sus_chk_delay,			\
						 MCHP_SAF_FLASH_SUS_CHK_DELAY),		\
		.sus_rsm_interval = DT_INST_PROP_OR(n, sus_rsm_interval,		\
						    MCHP_SAF_FLASH_SUS_RSM_INTERVAL),	\
		.poll_interval = DT_INST_PROP_OR(n, poll_interval,			\
						 MCHP_SAF_FLASH_POLL_INTERVAL),		\
		.pcr_idx = DT_INST_PROP_BY_IDX(n, pcrs, 0),				\
		.pcr_pos = DT_INST_PROP_BY_IDX(n, pcrs, 1),				\
		.irq_config_func = espi_saf_xec_connect_irqs_##n,			\
		.irq_info_size = ARRAY_SIZE(espi_saf_xec_irq_info_##n),			\
		.irq_info_list = espi_saf_xec_irq_info_##n,				\
	};										\
	DEVICE_DT_INST_DEFINE(0, &espi_saf_xec_init, NULL,				\
				  &espisaf_xec_data_##n,				\
				  &espisaf_xec_config_##n, POST_KERNEL,			\
				  CONFIG_ESPI_SAF_INIT_PRIORITY,			\
				  &espi_saf_xec_driver_api);				\
											\
	static void espi_saf_xec_connect_irqs_##n(void)					\
	{										\
		uint8_t girq, gpos;							\
											\
		/* SAF Done */								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),				\
				DT_INST_IRQ_BY_IDX(n, 0, priority),			\
				espi_saf_done_isr,					\
				DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));				\
											\
		girq = MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(n, girqs, 0));		\
		gpos = MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(n, girqs, 0));	\
		mchp_xec_ecia_girq_src_en(girq, gpos);					\
											\
		/* SAF Error */								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),				\
				DT_INST_IRQ_BY_IDX(n, 1, priority),			\
				espi_saf_err_isr,					\
				DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));				\
											\
		girq = MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(n, girqs, 1));		\
		gpos = MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(n, girqs, 1));	\
		mchp_xec_ecia_girq_src_en(girq, gpos);					\
	}

DT_INST_FOREACH_STATUS_OKAY(ESPI_SAF_XEC_DEVICE)
