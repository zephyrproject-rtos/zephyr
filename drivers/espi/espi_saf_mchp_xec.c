/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2020 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi_saf

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/logging/log.h>

#include "espi_utils.h"
LOG_MODULE_REGISTER(espi_saf, CONFIG_ESPI_LOG_LEVEL);

/* SAF EC Portal read/write flash access limited to 1-64 bytes */
#define MAX_SAF_ECP_BUFFER_SIZE 64ul

/* 1 second maximum for flash operations */
#define MAX_SAF_FLASH_TIMEOUT 125000ul /* 1000ul */

/* 64 bytes @ 24MHz quad is approx. 6 us */
#define SAF_WAIT_INTERVAL 8

/* After 8 wait intervals yield */
#define SAF_YIELD_THRESHOLD 64

struct espi_isr {
	uint32_t girq_bit;
	void (*the_isr)(const struct device *dev);
};

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
	uintptr_t saf_base_addr;
	uintptr_t qmspi_base_addr;
	uintptr_t saf_comm_base_addr;
	uint32_t poll_timeout;
	uint32_t consec_rd_timeout;
	uint32_t sus_chk_delay;
	uint16_t sus_rsm_interval;
	uint16_t poll_interval;
};

struct espi_saf_xec_data {
	sys_slist_t callbacks;
	struct k_sem ecp_lock;
	uint32_t hwstatus;
};

/* EC portal local flash r/w buffer */
static uint32_t slave_mem[MAX_SAF_ECP_BUFFER_SIZE];

/*
 * @brief eSPI SAF configuration
 */

static inline void mchp_saf_cs_descr_wr(MCHP_SAF_HW_REGS *regs, uint8_t cs,
					uint32_t val)
{
	regs->SAF_CS_OP[cs].OP_DESCR = val;
}

static inline void mchp_saf_poll2_mask_wr(MCHP_SAF_HW_REGS *regs, uint8_t cs,
					  uint16_t val)
{
	LOG_DBG("%s cs: %d mask %x", __func__, cs, val);
	if (cs == 0) {
		regs->SAF_CS0_CFG_P2M = val;
	} else {
		regs->SAF_CS1_CFG_P2M = val;
	}
}

static inline void mchp_saf_cm_prefix_wr(MCHP_SAF_HW_REGS *regs, uint8_t cs,
					 uint16_t val)
{
	if (cs == 0) {
		regs->SAF_CS0_CM_PRF = val;
	} else {
		regs->SAF_CS1_CM_PRF = val;
	}
}

/* busy wait or yield until we have SAF interrupt support */
static int xec_saf_spin_yield(int *counter)
{
	*counter = *counter + 1;

	if (*counter > MAX_SAF_FLASH_TIMEOUT) {
		return -ETIMEDOUT;
	}

	if (*counter > SAF_YIELD_THRESHOLD) {
		k_yield();
	} else {
		k_busy_wait(SAF_WAIT_INTERVAL);
	}

	return 0;
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
static void saf_protection_regions_init(MCHP_SAF_HW_REGS *regs)
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

static uint32_t qmspi_freq_div(uint32_t freqhz)
{
	uint32_t fdiv;

	if (freqhz < (MCHP_QMSPI_MIN_FREQ_KHZ * 1000U)) {
		fdiv = 0U; /* freq divider field -> 256 */
	} else if (freqhz >= (MCHP_QMSPI_MAX_FREQ_KHZ * 1000U)) {
		fdiv = 1U;
	} else {
		/* truncation produces next higher integer frequency */
		fdiv = MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ / freqhz;
	}

	fdiv &= MCHP_QMSPI_M_FDIV_MASK0;
	fdiv <<= MCHP_QMSPI_M_FDIV_POS;

	return fdiv;
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
	uint32_t qmode, cstim, n;
	QMSPI_Type *regs = (QMSPI_Type *)xcfg->qmspi_base_addr;
	const struct espi_saf_hw_cfg *hwcfg = &cfg->hwcfg;

	qmode = regs->MODE;
	if (!(qmode & MCHP_QMSPI_M_ACTIVATE)) {
		return -EAGAIN;
	}

	qmode = regs->MODE & (MCHP_QMSPI_M_FDIV_MASK | MCHP_QMSPI_M_SIG_MASK);
	cstim = regs->CSTM;
	regs->MODE = MCHP_QMSPI_M_SRST;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	MCHP_GIRQ_ENCLR(MCHP_QMSPI_GIRQ_NUM) = MCHP_QMSPI_GIRQ_VAL;
	MCHP_GIRQ_SRC(MCHP_QMSPI_GIRQ_NUM) = MCHP_QMSPI_GIRQ_VAL;

	regs->IFCTRL =
		(MCHP_QMSPI_IFC_WP_OUT_HI | MCHP_QMSPI_IFC_WP_OUT_EN |
		 MCHP_QMSPI_IFC_HOLD_OUT_HI | MCHP_QMSPI_IFC_HOLD_OUT_EN);

	for (n = 0; n < MCHP_SAF_NUM_GENERIC_DESCR; n++) {
		regs->DESCR[MCHP_SAF_CM_EXIT_START_DESCR + n] =
			hwcfg->generic_descr[n];
	}

	regs->IEN = MCHP_QMSPI_IEN_XFR_DONE;

	qmode |= (MCHP_QMSPI_M_SAF_DMA_MODE_EN | MCHP_QMSPI_M_CS0 |
		  MCHP_QMSPI_M_ACTIVATE);

	if (hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_CPHA) {
		qmode = (qmode & ~(MCHP_QMSPI_M_SIG_MASK)) |
			((hwcfg->qmspi_cpha << MCHP_QMSPI_M_SIG_POS) &
			 MCHP_QMSPI_M_SIG_MASK);
	}

	if (hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_FREQ) {
		qmode = (qmode & ~(MCHP_QMSPI_M_FDIV_MASK)) |
			qmspi_freq_div(hwcfg->qmspi_freq_hz);
	}

	if (hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_CSTM) {
		cstim = hwcfg->qmspi_cs_timing;
	}

	regs->MODE = qmode;
	regs->CSTM = cstim;

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
static void saf_flash_timing_init(MCHP_SAF_HW_REGS *regs,
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
static void saf_dnx_bypass_init(MCHP_SAF_HW_REGS *regs)
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
static int saf_init_erase_block_size(const struct espi_saf_cfg *cfg)
{
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

	ESPI_CAP_REGS->FC_SERBZ = erase_bitmap;

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
static void saf_flash_misc_cfg(MCHP_SAF_HW_REGS *regs, uint8_t cs,
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
 */
static void saf_flash_cfg(const struct device *dev,
			  const struct espi_saf_flash_cfg *fcfg, uint8_t cs)
{
	uint32_t d, did;
	const struct espi_saf_xec_config *xcfg = dev->config;
	MCHP_SAF_HW_REGS *regs = (MCHP_SAF_HW_REGS *)xcfg->saf_base_addr;
	QMSPI_Type *qregs = (QMSPI_Type *)xcfg->qmspi_base_addr;

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
}

static const uint32_t tag_map_dflt[MCHP_ESPI_SAF_TAGMAP_MAX] = {
	MCHP_SAF_TAG_MAP0_DFLT, MCHP_SAF_TAG_MAP1_DFLT, MCHP_SAF_TAG_MAP2_DFLT
};

static void saf_tagmap_init(MCHP_SAF_HW_REGS *regs,
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

	const struct espi_saf_xec_config *xcfg = dev->config;
	MCHP_SAF_HW_REGS *regs = (MCHP_SAF_HW_REGS *)xcfg->saf_base_addr;
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

	/* flash device connected to CS0 required */
	totalsz = fcfg->flashsz;
	regs->SAF_FL_CFG_THRH = totalsz;
	saf_flash_cfg(dev, fcfg, 0);

	/* optional second flash device connected to CS1 */
	if (cfg->nflash_devices > 1) {
		fcfg++;
		totalsz += fcfg->flashsz;
	}
	/* Program CS1 configuration (same as CS0 if only one device) */
	saf_flash_cfg(dev, fcfg, 1);

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

	ret = saf_init_erase_block_size(cfg);
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
		MCHP_SAF_COMM_MODE_REG |= MCHP_SAF_COMM_MODE_PF_EN;
	} else {
		MCHP_SAF_COMM_MODE_REG &= ~(MCHP_SAF_COMM_MODE_PF_EN);
	}

	LOG_DBG("%s SAF_FL_CFG_MISC: %x", __func__, regs->SAF_FL_CFG_MISC);
	LOG_DBG("%s Aft MCHP_SAF_COMM_MODE_REG: %x", __func__,
		MCHP_SAF_COMM_MODE_REG);

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

	const struct espi_saf_xec_config *xcfg = dev->config;
	MCHP_SAF_HW_REGS *regs = (MCHP_SAF_HW_REGS *)xcfg->saf_base_addr;

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
	const struct espi_saf_xec_config *cfg = dev->config;
	MCHP_SAF_HW_REGS *regs = (MCHP_SAF_HW_REGS *)cfg->saf_base_addr;

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

static uint32_t get_erase_size_encoding(uint32_t erase_size)
{
	uint8_t supsz = ESPI_CAP_REGS->FC_SERBZ;

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
 * EC access (read/erase/write) to SAF attached flash array
 * cmd  0 = read
 *	1 = write
 *	2 = erase
 */
static int saf_ecp_access(const struct device *dev,
			  struct espi_saf_packet *pckt, uint8_t cmd)
{
	uint32_t err_mask, n;
	int rc, counter;
	struct espi_saf_xec_data *xdat = dev->data;
	const struct espi_saf_xec_config *cfg = dev->config;
	MCHP_SAF_HW_REGS *regs = (MCHP_SAF_HW_REGS *)cfg->saf_base_addr;

	counter = 0;
	err_mask = MCHP_SAF_ECP_STS_ERR_MASK;

	LOG_DBG("%s", __func__);

	if (!(regs->SAF_FL_CFG_MISC & MCHP_SAF_FL_CFG_MISC_SAF_EN)) {
		LOG_ERR("SAF is disabled");
		return -EIO;
	}

	if (regs->SAF_ECP_BUSY & MCHP_SAF_ECP_BUSY) {
		LOG_ERR("SAF EC Portal is busy");
		return -EBUSY;
	}

	if ((cmd == MCHP_SAF_ECP_CMD_CTYPE_READ0) ||
	    (cmd == MCHP_SAF_ECP_CMD_CTYPE_WRITE0)) {
		rc = check_ecp_access_size(pckt->len);
		if (rc) {
			LOG_ERR("SAF EC Portal size out of bounds");
			return rc;
		}

		if (cmd == MCHP_SAF_ECP_CMD_CTYPE_WRITE0) {
			memcpy(slave_mem, pckt->buf, pckt->len);
		}

		n = pckt->len;
	} else if (cmd == MCHP_SAF_ECP_CMD_CTYPE_ERASE0) {
		n = get_erase_size_encoding(pckt->len);
		if (n == 0xffffffff) {
			LOG_ERR("SAF EC Portal unsupported erase size");
			return -EAGAIN;
		}
	} else {
		LOG_ERR("SAF EC Portal bad cmd");
		return -EAGAIN;
	}

	LOG_DBG("%s params val done", __func__);

	k_sem_take(&xdat->ecp_lock, K_FOREVER);

	regs->SAF_ECP_INTEN = 0;
	regs->SAF_ECP_STATUS = 0xffffffff;

	/*
	 * TODO - Force SAF Done interrupt disabled until we have support
	 * from eSPI driver.
	 */
	MCHP_GIRQ_ENCLR(MCHP_SAF_GIRQ) = MCHP_SAF_GIRQ_ECP_DONE_BIT;
	MCHP_GIRQ_SRC(MCHP_SAF_GIRQ) = MCHP_SAF_GIRQ_ECP_DONE_BIT;

	regs->SAF_ECP_FLAR = pckt->flash_addr;
	regs->SAF_ECP_BFAR = (uint32_t)&slave_mem[0];

	regs->SAF_ECP_CMD =
		MCHP_SAF_ECP_CMD_PUT_FLASH_NP |
		((uint32_t)cmd << MCHP_SAF_ECP_CMD_CTYPE_POS) |
		((n << MCHP_SAF_ECP_CMD_LEN_POS) & MCHP_SAF_ECP_CMD_LEN_MASK);

	/* TODO when interrupts are available enable here */
	regs->SAF_ECP_START = MCHP_SAF_ECP_START;

	/* TODO
	 * ISR is in eSPI driver. Use polling until eSPI driver has been
	 * modified to provide callback for GIRQ19 SAF ECP Done.
	 */
	rc = 0;
	xdat->hwstatus = regs->SAF_ECP_STATUS;
	while (!(xdat->hwstatus & MCHP_SAF_ECP_STS_DONE)) {
		rc = xec_saf_spin_yield(&counter);
		if (rc < 0) {
			goto ecp_exit;
		}
		xdat->hwstatus = regs->SAF_ECP_STATUS;
	}

	/* clear hardware status and check for errors */
	regs->SAF_ECP_STATUS = xdat->hwstatus;
	if (xdat->hwstatus & MCHP_SAF_ECP_STS_ERR_MASK) {
		rc = -EIO;
		goto ecp_exit;
	}

	if (cmd == MCHP_SAF_ECP_CMD_CTYPE_READ0) {
		memcpy(pckt->buf, slave_mem, pckt->len);
	}

ecp_exit:
	k_sem_give(&xdat->ecp_lock);

	return rc;
}

/* Flash read using SAF EC Portal */
static int saf_xec_flash_read(const struct device *dev,
			      struct espi_saf_packet *pckt)
{
	LOG_DBG("%s", __func__);
	return saf_ecp_access(dev, pckt, MCHP_SAF_ECP_CMD_CTYPE_READ0);
}

/* Flash write using SAF EC Portal */
static int saf_xec_flash_write(const struct device *dev,
			       struct espi_saf_packet *pckt)
{
	return saf_ecp_access(dev, pckt, MCHP_SAF_ECP_CMD_CTYPE_WRITE0);
}

/* Flash erase using SAF EC Portal */
static int saf_xec_flash_erase(const struct device *dev,
			       struct espi_saf_packet *pckt)
{
	return saf_ecp_access(dev, pckt, MCHP_SAF_ECP_CMD_CTYPE_ERASE0);
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
	const struct espi_saf_xec_config *cfg;
	MCHP_SAF_HW_REGS *regs;

	if (dev == NULL) {
		return -EINVAL;
	}

	cfg = dev->config;
	regs = (MCHP_SAF_HW_REGS *)cfg->saf_base_addr;

	regs->SAF_FL_CFG_MISC |= MCHP_SAF_FL_CFG_MISC_SAF_EN;

	return 0;
}

static int espi_saf_xec_init(const struct device *dev);

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

static struct espi_saf_xec_data espi_saf_xec_data;

static const struct espi_saf_xec_config espi_saf_xec_config = {
	.saf_base_addr = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.qmspi_base_addr = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.saf_comm_base_addr = DT_INST_REG_ADDR_BY_IDX(0, 2),
	.poll_timeout = DT_INST_PROP_OR(inst, poll_timeout,
					MCHP_SAF_FLASH_POLL_TIMEOUT),
	.consec_rd_timeout = DT_INST_PROP_OR(
		inst, consec_rd_timeout, MCHP_SAF_FLASH_CONSEC_READ_TIMEOUT),
	.sus_chk_delay = DT_INST_PROP_OR(inst, sus_chk_delay,
					 MCHP_SAF_FLASH_SUS_CHK_DELAY),
	.sus_rsm_interval = DT_INST_PROP_OR(inst, sus_rsm_interval,
					    MCHP_SAF_FLASH_SUS_RSM_INTERVAL),
	.poll_interval = DT_INST_PROP_OR(inst, poll_interval,
					 MCHP_SAF_FLASH_POLL_INTERVAL),
};

DEVICE_DT_INST_DEFINE(0, &espi_saf_xec_init, NULL,
		      &espi_saf_xec_data, &espi_saf_xec_config, POST_KERNEL,
		      CONFIG_ESPI_TAF_INIT_PRIORITY, &espi_saf_xec_driver_api);

static int espi_saf_xec_init(const struct device *dev)
{
	struct espi_saf_xec_data *data = dev->data;

	/* ungate SAF clocks by disabling PCR sleep enable */
	mchp_pcr_periph_slp_ctrl(PCR_ESPI_SAF, MCHP_PCR_SLEEP_DIS);

	/* reset the SAF block */
	mchp_pcr_periph_reset(PCR_ESPI_SAF);

	/* Configure the channels and its capabilities based on build config */
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_GBL_CAP0_FC_SUPP;
	ESPI_CAP_REGS->FC_CAP &= ~(MCHP_ESPI_FC_CAP_SHARE_MASK);
	ESPI_CAP_REGS->FC_CAP |= MCHP_ESPI_FC_CAP_SHARE_MAF_SAF;

	k_sem_init(&data->ecp_lock, 1, 1);

	return 0;
}
