/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi_saf_v2

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "mec_espi_taf_regs.h"
#include "espi_utils.h"
LOG_MODULE_REGISTER(espi_saf, CONFIG_ESPI_LOG_LEVEL);

/* common clock control device node for all Microchip XEC chips */
#define MCHP_XEC_CLOCK_CONTROL_NODE DT_NODELABEL(pcr)

/* SAF EC Portal read/write flash access limited to 1-64 bytes */
#define MAX_SAF_ECP_BUFFER_SIZE 64ul

/* 1 second maximum for flash operations */
#define MAX_SAF_FLASH_TIMEOUT 125000ul /* 1000ul */

#define MAX_SAF_FLASH_TIMEOUT_MS 1000ul

/* 64 bytes @ 24MHz quad is approx. 6 us */
#define SAF_WAIT_INTERVAL 8

/* After 8 wait intervals yield */
#define SAF_YIELD_THRESHOLD 64

/* Delay after TAF activation in microseconds */
#define TAF_ACTIVATION_DELAY_US 1000U

/* Get QMSPI 0 encoded GIRQ information */
#define XEC_QMSPI_ENC_GIRQ DT_PROP_BY_IDX(DT_INST(0, microchip_xec_qmspi_ldma), girqs, 0)

#define XEC_QMSPI_GIRQ     MCHP_XEC_ECIA_GIRQ(XEC_QMSPI_ENC_GIRQ)
#define XEC_QMSPI_GIRQ_POS MCHP_XEC_ECIA_GIRQ_POS(XEC_QMSPI_ENC_GIRQ)

#define XEC_SAF_DONE_ENC_GIRQ DT_INST_PROP_BY_IDX(0, girqs, 0)
#define XEC_SAF_ERR_ENC_GIRQ  DT_INST_PROP_BY_IDX(0, girqs, 1)

#define XEC_SAF_DONE_GIRQ     MCHP_XEC_ECIA_GIRQ(XEC_SAF_DONE_ENC_GIRQ)
#define XEC_SAF_DONE_GIRQ_POS MCHP_XEC_ECIA_GIRQ_POS(XEC_SAF_ERR_ENC_GIRQ)

/* fallback QSPI frequency */
#define XEC_TAF_QSPI_DFLT_FREQ MHZ(24)
/* fallback QSPI clock polarity and sampling phases */
#define XEC_TAF_QSPI_DFLT_CPHA XEC_QSPI_SPI_MODE_0

/*
 * TAF configuration from Device Tree
 * TAF controller register block base address
 * QMSPI controller register block base address
 * TAF communications register block base address
 * Flash STATUS1 poll timeout in 32KHz periods
 * Flash consecutive read timeout in units of 20 ns
 * Delay before first Poll-1 command after suspend in 20 ns units
 * Hold off suspend for this interval if erase or program in 32KHz periods.
 * Add delay between Poll STATUS1 commands in 20 ns units.
 */
struct espi_taf_xec_config {
	mm_reg_t taf_base;
	mm_reg_t taf_comm_base;
	mm_reg_t espi_ioc_base;
	mm_reg_t qspi_base;
	volatile void *ecp_mem;
	void (*irq_config_func)(void);
	uint32_t poll_timeout;
	uint32_t consec_rd_timeout;
	uint32_t sus_chk_delay;
	uint16_t sus_rsm_interval;
	uint16_t poll_interval;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t girq_bmon;
	uint8_t girq_bmon_pos;
	uint8_t enc_pcr;
};

struct espi_taf_xec_data {
	struct k_mutex ecp_lock;
	struct k_sem sync;
	uint32_t ecp_cmd;
	uint32_t ecp_hwstatus;
	uint32_t hwstatus;
	sys_slist_t callbacks;
};

static void clear_ecp_mem(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;

	if (xcfg->ecp_mem == NULL) {
		return;
	}

	if ((IS_ALIGNED(xcfg->ecp_mem, 4) == true) && ((MAX_SAF_ECP_BUFFER_SIZE % 4U) == 0)) {
		volatile uint32_t *p = (volatile uint32_t *)xcfg->ecp_mem;

		for (size_t n = 0; n < (MAX_SAF_ECP_BUFFER_SIZE / 4U); n++) {
			*p++ = 0;
		}
	} else {
		volatile uint8_t *p = (volatile uint8_t *)xcfg->ecp_mem;

		for (size_t n = 0; n < MAX_SAF_ECP_BUFFER_SIZE; n++) {
			*p++ = 0;
		}
	}
}

static size_t pkt_to_ecp_mem(const struct device *dev, struct espi_saf_packet *pkt)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	volatile uint8_t *ecp_mem = xcfg->ecp_mem;
	const uint8_t *src = NULL;
	size_t n = 0, nmax = 0;

	if ((ecp_mem == NULL) || (pkt == NULL) || (pkt->buf == NULL) || (pkt->len == 0)) {
		return 0;
	}

	src = pkt->buf;
	nmax = MIN(pkt->len, MAX_SAF_ECP_BUFFER_SIZE);

	while (n < nmax) {
		*ecp_mem++ = *src++;
		n++;
	}

	return n;
}

static size_t ecp_mem_to_pkt(const struct device *dev, struct espi_saf_packet *pkt)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	volatile uint8_t *ecp_mem = xcfg->ecp_mem;
	uint8_t *dest = NULL;
	size_t n = 0, nmax = 0;

	if ((ecp_mem == NULL) || (pkt == NULL) || (pkt->buf == NULL) || (pkt->len == 0)) {
		return 0;
	}

	dest = pkt->buf;
	nmax = MIN(pkt->len, MAX_SAF_ECP_BUFFER_SIZE);

	while (n < nmax) {
		*dest++ = *ecp_mem++;
		n++;
	}

	return n;
}

/* Program bit mask used to mask out status bits returned by flash read status operation */
static inline void mchp_taf_poll2_mask_wr(mm_reg_t taf_base, uint8_t cs, uint16_t val)
{
	uint32_t rval = 0, msk = 0;

	LOG_DBG("%s cs: %d mask %x", __func__, cs, val);

	if (cs == 0) {
		msk = XEC_TAFS_CFG_CS_POLL2_MASK_CS0_MSK;
	} else if (cs == 1U) {
		msk = XEC_TAFS_CFG_CS_POLL2_MASK_CS1_MSK;
	} else {
		return;
	}

	rval = xec_tafs_field_prep(msk, (uint32_t)val);
	soc_mmcr_mask_set(taf_base + 0, rval, msk);
}

/* Program the continuous mode entry opcode and optional prefix byte for the flash connect to
 * chip select cs.
 */
static inline void mchp_taf_cm_prefix_wr(mm_reg_t taf_base, uint8_t cs, uint16_t val)
{
	uint32_t rval = 0, msk = 0;

	if (cs == 0) {
		msk = XEC_TAFS_CONT_PREFIX_CS0_OP_MSK | XEC_TAFS_CONT_PREFIX_CS0_DAT_MSK;
	} else if (cs == 1U) {
		msk = XEC_TAFS_CONT_PREFIX_CS1_OP_MSK | XEC_TAFS_CONT_PREFIX_CS1_DAT_MSK;
	} else {
		return;
	}

	rval = xec_tafs_field_prep(msk, (uint32_t)val);
	soc_mmcr_mask_set(taf_base + XEC_TAFS_CONT_PREFIX_CFG_OFS, rval, msk);
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
 * Limit = 0x7ffffversation
 * WR = 0xFF
 * RD = 0xFF
 */
static void taf_protection_regions_init(mm_reg_t tafb)
{
	uint32_t lim0 = 0;
	LOG_DBG("%s", __func__);

	/* Get the flash size limit register value and convert to 4KB units since
	 * the protection registers work on 4KB boundaries
	 */
	lim0 = sys_read32(tafb + XEC_TAFS_CFG_SIZE_LIM_OFS);
	lim0 >>= 12;

	sys_write32(0, tafb + XEC_TAFS_PROT_RG_START_OFS(0));
	sys_write32(lim0, tafb + XEC_TAFS_PROT_RG_LIMIT_OFS(0));
	sys_write32(XEC_TAF_CID_ALL_MSK, tafb + XEC_TAFS_PROT_RG_WR_OFS(0));
	sys_write32(XEC_TAF_CID_ALL_MSK, tafb + XEC_TAFS_PROT_RG_RD_OFS(0));

	for (size_t n = 1U; n < XEC_TAFS_PROT_RG_MAX_REGIONS; n++) {
		sys_write32(XEC_TAF_PROT_RG_START_DFLT, tafb + XEC_TAFS_PROT_RG_START_OFS(n));
		sys_write32(XEC_TAF_PROT_RG_LIMIT_DFLT, tafb + XEC_TAFS_PROT_RG_LIMIT_OFS(n));
		sys_write32(0, tafb + XEC_TAFS_PROT_RG_WR_OFS(n));
		sys_write32(0, tafb + XEC_TAFS_PROT_RG_RD_OFS(n));

		LOG_DBG("PROT[%u] STA  0x%x", n, sys_read32(tafb + XEC_TAFS_PROT_RG_START_OFS(n)));
		LOG_DBG("PROT[%u] LIM  0x%x", n, sys_read32(tafb + XEC_TAFS_PROT_RG_LIMIT_OFS(n)));
		LOG_DBG("PROT[%u] WBM  0x%x", n, sys_read32(tafb + XEC_TAFS_PROT_RG_WR_OFS(n)));
		LOG_DBG("PROT[%u] RBM  0x%x", n, sys_read32(tafb + XEC_TAFS_PROT_RG_RD_OFS(n)));
	}
}

static int qmspi_freq_div(uint32_t freqhz, uint32_t *fdiv)
{
	uint32_t core_clock = soc_core_clock_get();

	if (fdiv == NULL) {
		return -EINVAL;
	}

	if (freqhz > core_clock) {
		return -EDOM;
	}

	if (freqhz < (core_clock / 0x10000U)) {
		return -EDOM;
	}

	*fdiv = core_clock / freqhz;

	return 0u;
}

static int qmspi_freq_div_from_mhz(uint32_t freqmhz, uint32_t *fdiv)
{
	uint32_t freqhz = freqmhz * 1000000u;

	return qmspi_freq_div(freqhz, fdiv);
}

static void qspi_soft_reset(mm_reg_t qbase)
{
	uint32_t reg_save_restore[4] = {0};

	reg_save_restore[0] = sys_read32(qbase + XEC_QSPI_CSTM_OFS);
	reg_save_restore[1] = sys_read32(qbase + XEC_QSPI_TAPS_OFS);
	reg_save_restore[2] = sys_read32(qbase + XEC_QSPI_TAPS_ADJ_OFS);
	reg_save_restore[3] = sys_read32(qbase + XEC_QSPI_TAPS_CR2_OFS);

	sys_set_bit(qbase + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_SRST_POS);
	while (sys_test_bit(qbase + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_SRST_POS) != 0) {
	}

	sys_write32(reg_save_restore[0], qbase + XEC_QSPI_CSTM_OFS);
	sys_write32(reg_save_restore[1], qbase + XEC_QSPI_TAPS_OFS);
	sys_write32(reg_save_restore[2], qbase + XEC_QSPI_TAPS_ADJ_OFS);
	sys_write32(reg_save_restore[3], qbase + XEC_QSPI_TAPS_CR2_OFS);
}

#define XEC_QSPI_LD_FLAG_UPDATE_NEXT_POS 0
/* TODO implement last of series has 0 next ?
 * or change flags to bit map where a 1 bit means set next
 * and 0 means 0 next? Then we have no unchanged.
 */
static int qspi_load_descrs(mm_reg_t qbase, uint8_t start_descr_idx, uint8_t num_descrs,
			    const uint32_t *descrs, uint32_t flags)
{
	uint32_t d = 0;
	uint16_t idx = 0, n = 0;

	if (((uint16_t)start_descr_idx + (uint16_t)num_descrs) > XEC_QSPI_MAX_DESCR_IDX) {
		return -EINVAL;
	}

	idx = start_descr_idx;
	while (n < num_descrs) {
		d = descrs[n];
		if ((flags & BIT(XEC_QSPI_LD_FLAG_UPDATE_NEXT_POS)) != 0) {
			d &= ~(XEC_QSPI_DR_ND_MSK);
			d |= XEC_QSPI_DR_ND_SET(idx + 1U);
		}
		sys_write32(d, qbase + XEC_QSPI_DESCR_OFS(idx));
		idx++;
		n++;
	}

	return 0;
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
 * ISSUE:
 * If driver for QMSPI implements PM mode where it disables itself and pins after
 * each transaction then this code will not work.
 * Future enhancement should implement QSPI TAF configuration using custom SPI/MSPI driver
 * APIs. The SPI/MSPI driver can then reconfigure its controller and pins.
 */
static int taf_qmspi_init(const struct espi_taf_xec_config *xcfg, const struct espi_saf_cfg *cfg)
{
	mm_reg_t taf_base = xcfg->taf_base;
	mm_reg_t qbase = xcfg->qspi_base;
	const struct espi_saf_hw_cfg *hwcfg = &cfg->hwcfg;
	uint32_t qmode = 0, qfdiv = 0, rval = 0;

	qmode = sys_read32(qbase + XEC_QSPI_MODE_OFS);

	if ((qmode & BIT(XEC_QSPI_MODE_ACTV_POS)) != 0) {
		/* save clock-phase bits and clock divider */
		qmode &= (XEC_QSPI_MODE_CP_MSK | XEC_QSPI_MODE_CK_DIV_MSK);
	} else {
		/* This path expects SPI flash devices are Quad enabled if TAF is
		 * configured for quad SPI operation
		 */
		if (qmspi_freq_div_from_mhz(XEC_TAF_QSPI_DFLT_FREQ, &qfdiv) != 0) {
			LOG_ERR("Bad default QMSPI freq");
			return -EINVAL;
		}

		qmode = XEC_QSPI_MODE_CK_DIV_SET(qfdiv);
		qmode |= XEC_QSPI_MODE_CP_SET(XEC_TAF_QSPI_DFLT_CPHA);
	}

	/* reset QMSPI */
	qspi_soft_reset(qbase);

	soc_ecia_girq_ctrl(XEC_QSPI0_GIRQ, XEC_QSPI0_GIRQ_POS, MCHP_MEC_ECIA_GIRQ_DIS);
	soc_ecia_girq_status_clear(XEC_QSPI0_GIRQ, XEC_QSPI0_GIRQ_POS);

	rval = (BIT(XEC_QSPI_IFCR_WP_HI_POS) | BIT(XEC_QSPI_IFCR_WP_OUT_EN_POS) |
		BIT(XEC_QSPI_IFCR_HOLD_HI_POS) | BIT(XEC_QSPI_IFCR_HOLD_OUT_EN_POS));
	sys_write32(rval, qbase + XEC_QSPI_IFCR_OFS);

	/* load descriptors and don't update each descriptors next field. We expect these
	 * descriptors to have their next fields set by the application.
	 */
	qspi_load_descrs(qbase, MCHP_SAF_CM_EXIT_START_DESCR, MCHP_SAF_NUM_GENERIC_DESCR,
			 hwcfg->generic_descr, 0);

	/* TAF HW uses QMSPI interrupt signal */
	sys_set_bit(qbase + XEC_QSPI_IER_OFS, XEC_QSPI_IER_XFR_DONE_POS);

	/* Turn on QSPI TAF mode an re-activate QSPI */
	qmode |= BIT(XEC_QSPI_MODE_TAF_POS) | BIT(XEC_QSPI_MODE_ACTV_POS);

	if ((hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_CPHA) != 0) {
		qmode &= ~(XEC_QSPI_MODE_CP_MSK);
		qmode |= XEC_QSPI_MODE_CP_SET((uint32_t)hwcfg->qmspi_cpha);
	}

	/* Copy QMSPI frequency divider into TAF CS0 and CS1 QMSPI frequency
	 * dividers. TAF HW uses CS0/CS1 divider register fields to overwrite
	 * QMSPI frequency divider in the QMSPI.Mode register. Later we will update
	 * TAF CS0/CS1 SPI frequency dividers based on flash configuration.
	 */
	qfdiv = XEC_QSPI_MODE_CK_DIV_GET(qmode);
	rval = xec_tafs_field_prep(XEC_TAFS_QSPI_CLK_DIV_READ_MSK, qfdiv) |
	       xec_tafs_field_prep(XEC_TAFS_QSPI_CLK_DIV_REST_MSK, qfdiv);
	sys_write32(rval, taf_base + XEC_TAFS_QSPI_CLK_DIV_CS0_OFS);
	sys_write32(rval, taf_base + XEC_TAFS_QSPI_CLK_DIV_CS1_OFS);

	/* TAF attached flashes require different chip select timing value */
	if ((hwcfg->flags & MCHP_SAF_HW_CFG_FLAG_CSTM) != 0) {
		sys_write32(hwcfg->qmspi_cs_timing, qbase + XEC_QSPI_CSTM_OFS);
	}

	/* TAF uses TX LDMA channel 0 in non-descriptor mode.
	 * TAF HW writes QMSPI.Control and TX LDMA channel 0 registers
	 * to transmit opcode, address, and data. We must configure the TX LDMA channel 0
	 * control register.
	 */
	rval = BIT(XEC_QSPI_LDMA_CHX_CR_EN_POS) | BIT(XEC_QSPI_LDMA_CHX_CR_RSE_POS) |
	       XEC_QSPI_LDMA_CHX_CR_SZ_SET(XEC_QSPI_LDMA_CHX_CR_SZ_4B);

	sys_write32(rval, qbase + XEC_QSPI_LDMA_CHX_CR_OFS(XEC_QSPI_LDMA_TX_CH0));

	/* enable QMSPI RX and TX LDMA in the mode register */
	qmode |= BIT(XEC_QSPI_MODE_LD_RX_EN_POS) | BIT(XEC_QSPI_MODE_LD_TX_EN_POS);

	sys_write32(qmode, qbase + XEC_QSPI_MODE_OFS);

	return 0;
}

/*
 * Registers at offsets:
 * TAF Poll timeout @ 0x194.  Hard coded to 0x28000. Default value = 0.
 *	recommended value = 0x28000 32KHz clocks (5 seconds). b[17:0]
 * TAF Poll interval @ 0x198.  Hard coded to 0
 *	Default value = 0. Recommended = 0. b[15:0]
 * TAF Suspend/Resume Interval @ 0x19c.  Hard coded to 0x8
 *	Default value = 0x01. Min time erase/prog in 32KHz units.
 * TAF Consecutive Read Timeout @ 0x1a0. Hard coded to 0x2. b[15:0]
 *	Units of MCLK. Recommend < 20us. b[19:0]
 * TAF Suspend Check Delay @ 0x1ac. Not touched.
 *	Default = 0. Recommend = 20us. Units = MCLK. b[19:0]
 */
static void taf_flash_timing_init(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;

	LOG_DBG("%s\n", __func__);

	sys_write32(xcfg->poll_timeout, tb + XEC_TAFS_TMO_TOTAL_POLL_OFS);
	sys_write32(xcfg->poll_interval, tb + XEC_TAFS_TMO_POLL_HOLDOFF_OFS);
	sys_write32(xcfg->sus_rsm_interval, tb + XEC_TAFS_TMO_RESUSPEND_OFS);
	sys_write32(xcfg->consec_rd_timeout, tb + XEC_TAFS_TMO_READ_WHILE_SUSP_OFS);
	sys_write32(xcfg->sus_chk_delay, tb + XEC_TAFS_TMO_DELAY_SUSP_CHECK_OFS);

	LOG_DBG("TAF TMO POLL %x\n", sys_read32(tb + XEC_TAFS_TMO_TOTAL_POLL_OFS));
	LOG_DBG("TAF TMO POLLH %x\n", sys_read32(tb + XEC_TAFS_TMO_POLL_HOLDOFF_OFS));
	LOG_DBG("TAF TMO RESUSP %x\n", sys_read32(tb + XEC_TAFS_TMO_RESUSPEND_OFS));
	LOG_DBG("TAF TMO RD SUSP %x\n", sys_read32(tb + XEC_TAFS_TMO_READ_WHILE_SUSP_OFS));
	LOG_DBG("TAF TMO CHK DLY %x\n", sys_read32(tb + XEC_TAFS_TMO_DELAY_SUSP_CHECK_OFS));
}

/*
 * Disable DnX bypass feature.
 * Allow access to all regions of the flash array during DnX.
 */
static void taf_dnx_bypass_init(mm_reg_t taf_base)
{
	sys_write32(0, taf_base + XEC_TAFS_DNX_PROT_BYP_OFS);
	sys_write32(UINT32_MAX, taf_base + XEC_TAFS_DNX_PROT_BYP_OFS);
}

/*
 * Bitmap of flash erase size from 1KB up to 128KB.
 * eSPI SAF specification requires 4KB erase support.
 * MCHP SAF supports 4KB, 32KB, and 64KB.
 * Only report 32KB and 64KB to Host if supported by both
 * flash devices.
 */
static int taf_init_erase_block_size(const struct device *dev, const struct espi_saf_cfg *cfg)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t iocb = xcfg->espi_ioc_base;
	struct espi_saf_flash_cfg *fcfg = cfg->flash_cfgs;
	uint32_t opb = fcfg->opb;
	uint8_t erase_bitmap = BIT(XEC_ESPI_IOC_TAF_ERBSZ_4KB_POS);

	LOG_DBG("%s\n", __func__);

	if (cfg->nflash_devices > 1) {
		fcfg++;
		opb &= fcfg->opb;
	}

	if ((opb & MCHP_SAF_CS_OPB_ER0_MASK) == 0) {
		/* One or both do not support 4KB erase! */
		return -EINVAL;
	}

	if ((opb & MCHP_SAF_CS_OPB_ER1_MASK) != 0) {
		erase_bitmap |= BIT(XEC_ESPI_IOC_TAF_ERBSZ_32KB_POS);
	}

	if ((opb & MCHP_SAF_CS_OPB_ER2_MASK) != 0) {
		erase_bitmap |= BIT(XEC_ESPI_IOC_TAF_ERBSZ_64KB_POS);
	}

	sys_write8(erase_bitmap, iocb + XEC_ESPI_IOC_TAF_ERBSZ_OFS);

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
static void taf_flash_misc_cfg(const struct device *dev, uint8_t cs,
			       const struct espi_saf_flash_cfg *fcfg)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	uint32_t d = 0, v = 0;

	d = sys_read32(tb + XEC_TAFS_CFG_MISC_OFS);

	v = MCHP_SAF_FL_CFG_MISC_CS0_CPE;
	if (cs != 0) {
		v = MCHP_SAF_FL_CFG_MISC_CS1_CPE;
	}

	/* Does this flash device require a prefix for continuous mode? */
	if (fcfg->cont_prefix != 0) {
		d |= v;
	} else {
		d &= ~v;
	}

	v = MCHP_SAF_FL_CFG_MISC_CS0_4BM;
	if (cs != 0) {
		v = MCHP_SAF_FL_CFG_MISC_CS1_4BM;
	}

	/* Use 32-bit addressing for this flash device? */
	if ((fcfg->flags & MCHP_FLASH_FLAG_ADDR32) != 0) {
		d |= v;
	} else {
		d &= ~v;
	}

	sys_write32(d, tb + XEC_TAFS_CFG_MISC_OFS);

	LOG_DBG("%s TAF Flash Misc Cfg: %x", __func__, d);
}

static void taf_flash_pd_cfg(const struct device *dev, uint8_t cs,
			     const struct espi_saf_flash_cfg *fcfg)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	uint32_t pdval = 0, msk = 0;

	if (cs == 0) {
		msk = BIT(XEC_TAFS_DP_CR_CS0_PD_EN_POS) |
		      BIT(XEC_TAFS_PD_CR_CS0_WK_ON_EC_ACTV_EN_POS);

		if ((fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS0_EN) != 0) {
			pdval |= BIT(XEC_TAFS_DP_CR_CS0_PD_EN_POS);
		}
		if ((fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS0_EC_WK_EN) != 0) {
			pdval |= BIT(XEC_TAFS_PD_CR_CS0_WK_ON_EC_ACTV_EN_POS);
		}
	} else {
		msk = BIT(XEC_TAFS_PD_CR_CS1_PD_EN_POS) |
		      BIT(XEC_TAFS_PD_CR_CS1_WK_ON_EC_ACTV_EN_POS);

		if ((fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS1_EN) != 0) {
			pdval |= BIT(XEC_TAFS_PD_CR_CS1_PD_EN_POS);
		}
		if ((fcfg->flags & MCHP_FLASH_FLAG_V2_PD_CS1_EC_WK_EN) != 0) {
			pdval |= BIT(XEC_TAFS_PD_CR_CS1_WK_ON_EC_ACTV_EN_POS);
		}
	}

	soc_mmcr_mask_set(tb + XEC_TAFS_PD_CR_OFS, pdval, msk);
}

/* Configure SAF per chip select QMSPI clock dividers.
 * SAF HW implements two QMSP clock divider registers per chip select:
 * Each divider register is composed of two 16-bit fields:
 *   b[15:0] = QMSPI clock divider for SPI read
 *   b[31:16] = QMSPI clock divider for all other SPI commands
 */
static int taf_flash_freq_cfg(const struct device *dev, uint8_t cs,
			      const struct espi_saf_flash_cfg *fcfg)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	uint32_t fmhz = 0, fdiv = 0, taf_qclk = 0;

	if (cs == 0) {
		taf_qclk = sys_read32(tb + XEC_TAFS_QSPI_CLK_DIV_CS0_OFS);
	} else {
		taf_qclk = sys_read32(tb + XEC_TAFS_QSPI_CLK_DIV_CS1_OFS);
	}

	fmhz = fcfg->rd_freq_mhz;
	if (fmhz != 0) {
		fdiv = 0u;
		if (qmspi_freq_div_from_mhz(fmhz, &fdiv) != 0) {
			LOG_ERR("%s TAF CLKDIV CS0 bad freq MHz %u", __func__, fmhz);
			return -EIO;
		}

		if (fdiv != 0) {
			taf_qclk &= ~(XEC_TAFS_QSPI_CLK_DIV_READ_MSK);
			taf_qclk |= xec_tafs_field_prep(XEC_TAFS_QSPI_CLK_DIV_READ_MSK, fdiv);
		}
	}

	fmhz = fcfg->freq_mhz;
	if (fmhz != 0) {
		fdiv = 0u;
		if (qmspi_freq_div_from_mhz(fmhz, &fdiv) != 0) {
			LOG_ERR("%s TAF CLKDIV CS1 bad freq MHz %u", __func__, fmhz);
			return -EIO;
		}

		if (fdiv != 0) {
			taf_qclk &= ~(XEC_TAFS_QSPI_CLK_DIV_REST_MSK);
			taf_qclk |= xec_tafs_field_prep(XEC_TAFS_QSPI_CLK_DIV_REST_MSK, fdiv);
		}
	}

	if (cs == 0) {
		sys_write32(taf_qclk, tb + XEC_TAFS_QSPI_CLK_DIV_CS0_OFS);
	} else {
		sys_write32(taf_qclk, tb + XEC_TAFS_QSPI_CLK_DIV_CS1_OFS);
	}

	return 0;
}

/*
 * Program flash device specific TAF and QMSPI registers.
 *
 * CS0 OpA @ 0x4c or CS1 OpA @ 0x5C
 * CS0 OpB @ 0x50 or CS1 OpB @ 0x60
 * CS0 OpC @ 0x54 or CS1 OpC @ 0x64
 * Poll 2 Mask @ 0x1a4
 * Continuous Prefix @ 0x1b0
 * CS0: QMSPI descriptors 0-5 or CS1 QMSPI descriptors 6-11
 * CS0 Descrs @ 0x58 or CS1 Descrs @ 0x68
 * TAF CS0 QMSPI frequency dividers (read/all other) commands
 * TAF CS1 QMSPI frequency dividers (read/all other) commands
 */
const uint16_t taf_opcode_ofs[XEC_TAFS_MAX_CHIP_SELECTS][4] = {
	{XEC_TAFS_CFG_CS0_OPCODE_A_OFS, XEC_TAFS_CFG_CS0_OPCODE_B_OFS,
	 XEC_TAFS_CFG_CS0_OPCODE_C_OFS, XEC_TAFS_CFG_CS0_DESCR_OFS},
	{XEC_TAFS_CFG_CS1_OPCODE_A_OFS, XEC_TAFS_CFG_CS1_OPCODE_B_OFS,
	 XEC_TAFS_CFG_CS1_OPCODE_C_OFS, XEC_TAFS_CFG_CS1_DESCR_OFS},
};

static int taf_flash_cfg(const struct device *dev, const struct espi_saf_flash_cfg *fcfg,
			 uint8_t cs)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	mm_reg_t qb = xcfg->qspi_base;
	int rc = 0;
	uint8_t start_didx = 0;

	LOG_DBG("%s cs=%u", __func__, cs);

	if (cs >= XEC_TAFS_MAX_CHIP_SELECTS) {
		return -EINVAL;
	}

	sys_write32(fcfg->opa, tb + (mm_reg_t)taf_opcode_ofs[cs][0]);
	sys_write32(fcfg->opb, tb + (mm_reg_t)taf_opcode_ofs[cs][1]);
	sys_write32(fcfg->opc, tb + (mm_reg_t)taf_opcode_ofs[cs][2]);
	sys_write32(fcfg->cs_cfg_descr_ids, tb + (mm_reg_t)taf_opcode_ofs[cs][3]);

	start_didx = MCHP_SAF_QMSPI_CS0_START_DESCR;
	if (cs != 0) {
		start_didx = MCHP_SAF_QMSPI_CS1_START_DESCR;
	}

	rc = qspi_load_descrs(qb, start_didx, MCHP_SAF_QMSPI_NUM_FLASH_DESCR, fcfg->descr,
			      BIT(XEC_QSPI_LD_FLAG_UPDATE_NEXT_POS));
	if (rc != 0) {
		LOG_ERR("Flash at CS[%u] bad QMSPI descriptors", cs);
		return rc;
	}

	mchp_taf_poll2_mask_wr(tb, cs, fcfg->poll2_mask);
	mchp_taf_cm_prefix_wr(tb, cs, fcfg->cont_prefix);
	taf_flash_misc_cfg(dev, cs, fcfg);
	taf_flash_pd_cfg(dev, cs, fcfg);

	return taf_flash_freq_cfg(dev, cs, fcfg);
}

static const uint32_t tag_map_dflt[MCHP_ESPI_TAF_TAGMAP_MAX] = {
	MCHP_TAF_TAG_MAP0_DFLT, MCHP_TAF_TAG_MAP1_DFLT, MCHP_TAF_TAG_MAP2_DFLT};

static void taf_tagmap_init(mm_reg_t taf_base, const struct espi_saf_cfg *cfg)
{
	const struct espi_saf_hw_cfg *hwcfg = &cfg->hwcfg;
	uint32_t tagm_ofs = XEC_TAFS_TAG_MAP0_OFS;
	uint32_t tagm_val = 0;

	for (size_t n = 0; n < MCHP_ESPI_SAF_TAGMAP_MAX; n++) {
		tagm_val = tag_map_dflt[n];

		if (hwcfg->tag_map[n] & MCHP_SAF_HW_CFG_TAGMAP_USE) {
			tagm_val = hwcfg->tag_map[n];
		}

		sys_write32(tagm_val, taf_base + tagm_ofs);

		LOG_DBG("TAF TAGM[%u]=0x%x", n, sys_read32(taf_base + tagm_ofs));

		tagm_ofs += 4U;
	}
}

#define TAF_QSPI_LDMA_CHAN_CR_VAL                                                                  \
	(BIT(XEC_QSPI_LDMA_CHX_CR_EN_POS) | BIT(XEC_QSPI_LDMA_CHX_CR_RSE_POS) |                    \
	 XEC_QSPI_LDMA_CHX_CR_SZ_SET(XEC_QSPI_LDMA_CHX_CR_SZ_4B))

static void taf_qmspi_ldma_cfg(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t qb = xcfg->qspi_base;
	uint32_t d = 0, n = 0, temp = 0, chan = 0;

	sys_clear_bit(qb + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_ACTV_POS);

	for (n = 0; n < XEC_QSPI_MAX_DESCR_IDX; n++) {
		d = sys_read32(qb + XEC_QSPI_DESCR_OFS(n));

		temp = XEC_QSPI_CR_TXM_GET(d);
		if (temp != XEC_QSPI_CR_TXM_DIS) {
			chan = XEC_QSPI_CR_TXDMA_GET(d);
			if (chan != XEC_QSPI_CR_TXDMA_DIS) {
				/* this descriptor uses TX DMA, set bit */
				sys_set_bit(qb + XEC_QSPI_LDMA_TX_EN_OFS, n);
				/* chan is 1-based add last RX index to get TX index */
				chan += XEC_QSPI_LDMA_RX_CH2;
				sys_write32(TAF_QSPI_LDMA_CHAN_CR_VAL,
					    qb + XEC_QSPI_LDMA_CHX_CR_OFS(chan));
			}
		}

		if ((d & BIT(XEC_QSPI_CR_RX_EN_POS)) != 0) {
			chan = XEC_QSPI_CR_RXDMA_GET(d);
			if (chan != XEC_QSPI_CR_RXDMA_DIS) {
				sys_set_bit(qb + XEC_QSPI_LDMA_RX_EN_OFS, n);
				/* chan is 1-based. Decrement to get 0 based */
				chan--;
				sys_write32(TAF_QSPI_LDMA_CHAN_CR_VAL,
					    qb + XEC_QSPI_LDMA_CHX_CR_OFS(chan));
			}
		}
	}

	sys_set_bit(qb + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_ACTV_POS);
}

/* API */
static bool espi_taf_xec_v2_channel_ready(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;

	if (sys_test_bit(xcfg->taf_base + XEC_TAFS_CFG_MISC_OFS,
			 XEC_TAFS_CFG_MISC_TAF_MODE_EN_POS) != 0) {
		return true;
	}

	return false;
}

/*
 * Configure SAF and QMSPI for SAF operation based upon the
 * number and characteristics of local SPI flash devices.
 * NOTE: SAF is configured but not activated. SAF should be
 * activated only when eSPI master sends Flash Channel enable
 * message with MAF/SAF select flag.
 */
static int espi_taf_xec_v2_config(const struct device *dev, const struct espi_saf_cfg *cfg)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	mm_reg_t tcommb = xcfg->taf_comm_base;
	const struct espi_saf_hw_cfg *hwcfg = NULL;
	const struct espi_saf_flash_cfg *fcfg = NULL;
	uint32_t totalsz = 0;
	uint32_t u = 0;
	int ret = 0;

	LOG_DBG("%s", __func__);

	if (cfg == NULL) {
		return -EINVAL;
	}

	hwcfg = &cfg->hwcfg;
	fcfg = cfg->flash_cfgs;

	if ((fcfg == NULL) || (cfg->nflash_devices == 0U) ||
	    (cfg->nflash_devices > XEC_TAFS_MAX_FLASH_DEVICES)) {
		return -EINVAL;
	}

	if (espi_taf_xec_v2_channel_ready(dev) == true) {
		return -EAGAIN;
	}

	taf_qmspi_init(xcfg, cfg);

	/* Not masking out any flash status bits returned by OP2 status poll */
	sys_write32(0, tb + XEC_TAFS_CFG_CS_POLL2_MASK_OFS);

	/* Program QMSPI start descriptor for exit continuous mode,
	 * polling 1 read status, and polling 2 read status.
	 */
	u = XEC_TAFS_CFG_GD_EXCM_SET(XEC_TAFS_CFG_GD_EXCM_STD_VAL) |
	    XEC_TAFS_CFG_GD_POLL1_SET(XEC_TAFG_CFG_GD_POLL1_STD_VAL) |
	    XEC_TAFS_CFG_GD_POLL2_SET(XEC_TAFS_CFG_GD_POLL2_STD_VAL);

	sys_write32(u, tb + XEC_TAFS_CFG_GD_OFS);

	/* global flash power down activity counter and interval time */
	sys_write32(hwcfg->flash_pd_timeout, tb + XEC_TAFS_ACTV_CNTR_RELOAD_OFS);
	sys_write32(hwcfg->flash_pd_min_interval, tb + XEC_TAFS_PD_TMO_POWER_DOWN_UP_OFS);

	/* flash device connected to CS0 required */
	totalsz = fcfg->flashsz;
	sys_write32(totalsz, tb + XEC_TAFS_CFG_THRLD_OFS);

	ret = taf_flash_cfg(dev, fcfg, 0);
	if (ret != 0) {
		return ret;
	}

	/* optional second flash device connected to CS1 */
	if (cfg->nflash_devices > 1) {
		fcfg++;
		totalsz += fcfg->flashsz;
	}

	/* Program CS1 configuration (same as CS0 if only one device) */
	ret = taf_flash_cfg(dev, fcfg, 1);
	if (ret) {
		return ret;
	}

	if (totalsz == 0) {
		return -EAGAIN;
	}

	sys_write32((totalsz - 1U), tb + XEC_TAFS_CFG_SIZE_LIM_OFS);

	LOG_DBG("TAF Flash Cfg Threshold = %x Size Limit = %x",
		sys_read32(tb + XEC_TAFS_CFG_THRLD_OFS),
		sys_read32(tb + XEC_TAFS_CFG_SIZE_LIM_OFS));

	taf_tagmap_init(tb, cfg);

	taf_protection_regions_init(tb);

	taf_dnx_bypass_init(tb);

	taf_flash_timing_init(dev);

	ret = taf_init_erase_block_size(dev, cfg);
	if (ret != 0) {
		LOG_ERR("SAF Config bad flash erase config");
		return ret;
	}

	/* Default or expedited prefetch? */
	u = XEC_TAFS_CFG_MISC_PREFETCH_OPT_SET(XEC_TAFS_CFG_MISC_PREFETCH_OPT_CANON);
	if ((cfg->hwcfg.flags & MCHP_SAF_HW_CFG_FLAG_PFEXP) != 0) {
		u = XEC_TAFS_CFG_MISC_PREFETCH_OPT_SET(XEC_TAFS_CFG_MISC_PREFETCH_OPT_EXPEDITE);
	}

	soc_mmcr_mask_set(tb + XEC_TAFS_CFG_MISC_OFS, u, XEC_TAFS_CFG_MISC_PREFETCH_OPT_EN_MSK);

	/* enable prefetch ? */
	if ((cfg->hwcfg.flags & MCHP_SAF_HW_CFG_FLAG_PFEN) != 0) {
		sys_set_bit(tcommb + XEC_TAFS_COMM_MODE_OFS, XEC_TAFS_COMM_MODE_PREFETCH_EN);
	} else {
		sys_clear_bit(tcommb + XEC_TAFS_COMM_MODE_OFS, XEC_TAFS_COMM_MODE_PREFETCH_EN);
	}

	LOG_DBG("%s SAF_FL_CFG_MISC: %x", __func__, sys_read32(tb + XEC_TAFS_CFG_MISC_OFS));
	LOG_DBG("%s TAF CommMode: %x", __func__, sys_read32(tcommb + XEC_TAFS_COMM_MODE_OFS));

	taf_qmspi_ldma_cfg(dev);

	return 0;
}

/* API */
static int espi_taf_xec_v2_set_pr(const struct device *dev, const struct espi_saf_protection *pr)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	const struct espi_saf_pr *preg = NULL;
	uint32_t start_addr = 0, lim_addr = 0, wrmap = 0, rdmap = 0;
	size_t nr = 0;

	if ((pr == NULL) || (pr->nregions >= MCHP_ESPI_SAF_PR_MAX)) {
		return -EINVAL;
	}

	if (espi_taf_xec_v2_channel_ready(dev) == true) {
		return -EAGAIN;
	}

	preg = pr->pregions;
	nr = pr->nregions;

	while (nr != 0) {
		uint32_t rr = preg->pr_num;

		if (rr >= XEC_TAFS_PROT_RG_MAX_REGIONS) {
			return -EINVAL;
		}

		start_addr = XEC_TAF_PROT_RG_START_DFLT;
		lim_addr = 0;
		wrmap = 0;
		rdmap = 0;

		if ((preg->flags & MCHP_SAF_PR_FLAG_ENABLE) != 0) {
			if (!IS_ALIGNED(preg->start, KB(4))) {
				LOG_ERR("Set PR: region[%u] start not 4KB aligned", rr);
				return -EINVAL;
			}

			start_addr = preg->start >> 12U; /* 4KB alignment */
			lim_addr = (preg->start + preg->size - 1U) >> 12U;
			wrmap = preg->master_bm_we;
			rdmap = preg->master_bm_rd;
		}

		/* NOTE: If previously locked writes have no effect */
		sys_write32(start_addr, tb + XEC_TAFS_PROT_RG_START_OFS(rr));
		sys_write32(lim_addr, tb + XEC_TAFS_PROT_RG_LIMIT_OFS(rr));
		sys_write32(wrmap, tb + XEC_TAFS_PROT_RG_WR_OFS(rr));
		sys_write32(rdmap, tb + XEC_TAFS_PROT_RG_RD_OFS(rr));

		if ((preg->flags & MCHP_SAF_PR_FLAG_LOCK) != 0) {
			sys_set_bit(tb + XEC_TAFS_PROT_LOCK_OFS, rr);
		}

		preg++;
		nr--;
	}

	return 0;
}

/* Current TAF hardware erase operations use the ECP command register length
 * field to encode the erase size.
 * 4KB  = 0
 * 32KB = 1
 * 64KB = 2
 * All other encoding are invalid. The attached flash must support one of these
 * erase sizes.
 * The eSPI target has a register with a bit map of support erase sizes.
 * We updated that register when the application passed us the flash parameters
 * during driver configuration.
 */
static int get_erase_size_encoding(const struct device *dev, uint32_t erase_size,
				   uint32_t *encoding)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	uint8_t ebsz = sys_read8(xcfg->espi_ioc_base + XEC_ESPI_IOC_TAF_ERBSZ_OFS);

	LOG_DBG("%s\n", __func__);

	if (encoding == NULL) {
		return -EINVAL;
	}

	if (((ebsz & BIT(XEC_ESPI_IOC_TAF_ERBSZ_4KB_POS)) != 0) && (erase_size == KB(4))) {
		*encoding = XEC_TAFS_ECP_CMD_LEN_ERASE_4KB;
	} else if (((ebsz & BIT(XEC_ESPI_IOC_TAF_ERBSZ_32KB_POS)) != 0) && (erase_size == KB(32))) {
		*encoding = XEC_TAFS_ECP_CMD_LEN_ERASE_32KB;
	} else if (((ebsz & BIT(XEC_ESPI_IOC_TAF_ERBSZ_64KB_POS)) != 0) && (erase_size == KB(64))) {
		*encoding = XEC_TAFS_ECP_CMD_LEN_ERASE_64KB;
	} else {
		return -EBADR;
	}

	return 0;
}

static int check_ecp_access_size(uint32_t reqlen)
{
	if ((reqlen < MCHP_SAF_ECP_CMD_RW_LEN_MIN) || (reqlen > MCHP_SAF_ECP_CMD_RW_LEN_MAX)) {
		return -EAGAIN;
	}

	return 0;
}

static int ecp_rw(const struct device *dev, struct espi_saf_packet *pkt, uint8_t cmd)
{
	struct espi_taf_xec_data *const xdat = dev->data;
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	uint32_t ecp_cmd = XEC_TAFS_ECP_CTYPE_READ;
	int rc = check_ecp_access_size(pkt->len);

	if (rc != 0) {
		LOG_ERR("TAF EC Portal request size out of bounds");
		return rc;
	}

	if (cmd == MCHP_SAF_ECP_CMD_WRITE) {
		ecp_cmd = XEC_TAFS_ECP_CTYPE_WRITE;
		pkt_to_ecp_mem(dev, pkt);
	}

	sys_write32(pkt->flash_addr, tb + XEC_TAFS_ECP_FLASH_ADDR_OFS);
	sys_write32((uint32_t)xcfg->ecp_mem, tb + XEC_TAFS_ECP_MBUF_ADDR_OFS);

	ecp_cmd = xec_tafs_field_prep(XEC_TAFS_ECP_CMD_CTYPE_MSK, ecp_cmd);
	ecp_cmd |= xec_tafs_field_prep(XEC_TAFS_ECP_CMD_PUT_MSK, XEC_TAFS_ECP_CMD_PUT_FLASH_NP);
	ecp_cmd |= xec_tafs_field_prep(XEC_TAFS_ECP_CMD_LEN_MSK, pkt->len);

	xdat->ecp_cmd = ecp_cmd;

	return 0;
}

static int ecp_erase(const struct device *dev, struct espi_saf_packet *pkt)
{
	struct espi_taf_xec_data *const xdat = dev->data;
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	uint32_t cmd = 0, encoded_erase_sz = 0;
	int rc = get_erase_size_encoding(dev, pkt->len, &encoded_erase_sz);

	if (rc != 0) {
		return rc;
	}

	sys_write32(pkt->flash_addr, tb + XEC_TAFS_ECP_FLASH_ADDR_OFS);
	sys_write32((uint32_t)xcfg->ecp_mem, tb + XEC_TAFS_ECP_MBUF_ADDR_OFS);

	cmd = xec_tafs_field_prep(XEC_TAFS_ECP_CMD_CTYPE_MSK, XEC_TAFS_ECP_CTYPE_ERASE);
	cmd |= xec_tafs_field_prep(XEC_TAFS_ECP_CMD_PUT_MSK, XEC_TAFS_ECP_CMD_PUT_FLASH_NP);
	cmd |= xec_tafs_field_prep(XEC_TAFS_ECP_CMD_LEN_MSK, encoded_erase_sz);

	xdat->ecp_cmd = cmd;

	return 0;
}

/*
 * Application can send a flash read, erase, write operation to the attached flash array.
 * NOTE: If the Host eSPI controller has an on-going flash access the EC Portal hardware
 * will stall waiting for the Host access to finish.
 */
static int taf_v2_ecp_access(const struct device *dev, struct espi_saf_packet *pkt, uint8_t cmd)
{
	struct espi_taf_xec_data *const xdat = dev->data;
	const struct espi_taf_xec_config *xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;
	int rc = 0;

	LOG_DBG("%s", __func__);

	if (pkt == NULL) {
		return -EINVAL;
	}

	if (espi_taf_xec_v2_channel_ready(dev) == false) {
		LOG_ERR("TAF is disabled");
		return -EIO;
	}

	if (sys_test_bit(tb + XEC_TAFS_ECP_BUSY_OFS, XEC_TAFS_ECP_BUSY_EC0_POS) != 0) {
		LOG_ERR("TAF EC Portal is busy");
		return -EBUSY;
	}

	k_mutex_lock(&xdat->ecp_lock, K_FOREVER);

	if ((cmd == XEC_TAFS_ECP_CTYPE_READ) || (cmd == XEC_TAFS_ECP_CTYPE_WRITE)) {
		rc = ecp_rw(dev, pkt, cmd);
	} else if (cmd == MCHP_SAF_ECP_CMD_ERASE) {
		rc = ecp_erase(dev, pkt);
	} else {
		rc = -ENOTSUP;
	}

	if (rc != 0) {
		LOG_ERR("TAF EC Port bad cmd");
		goto ecp_acc_exit;
	}

	LOG_DBG("%s ECP FlashAddr=0x%x", __func__, sys_read32(tb + XEC_TAFS_ECP_FLASH_ADDR_OFS));
	LOG_DBG("%s ECP MemAddr=0x%x", __func__, sys_read32(tb + XEC_TAFS_ECP_MBUF_ADDR_OFS));
	LOG_DBG("%s ECP Cmd=0x%x", __func__, xdat->ecp_cmd);

	sys_write32(0, tb + XEC_TAFS_ECP_IER_OFS);
	sys_write32(XEC_TAFS_ECP_SR_MSK, tb + XEC_TAFS_ECP_SR_OFS);
	soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);

	/* NOTE: errors cause Done to be set along with error status bits */
	sys_set_bit(tb + XEC_TAFS_ECP_IER_OFS, XEC_TAFS_ECP_IER_DONE_EN_POS);

	sys_write32(xdat->ecp_cmd, tb + XEC_TAFS_ECP_CMD_OFS);

	xdat->ecp_hwstatus = 0;
	sys_set_bit(tb + XEC_TAFS_ECP_START_OFS, XEC_TAFS_ECP_START_EN_POS);

	rc = k_sem_take(&xdat->sync, K_MSEC(MAX_SAF_FLASH_TIMEOUT_MS));
	if (rc == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		rc = -ETIMEDOUT;
		goto ecp_acc_exit;
	}

	LOG_DBG("%s wake on semaphore", __func__);

	/* ISR saved and cleared HW status */
	if ((xdat->ecp_hwstatus & XEC_TAFS_ECP_SR_ERR_MSK) != 0) {
		LOG_ERR("%s error 0x%x", __func__, xdat->ecp_hwstatus);
		rc = -EIO;
		goto ecp_acc_exit;
	}

	if (cmd == XEC_TAFS_ECP_CTYPE_READ) {
		ecp_mem_to_pkt(dev, pkt);
	}

ecp_acc_exit:
	clear_ecp_mem(dev);
	k_mutex_unlock(&xdat->ecp_lock);

	return rc;
}

/* Flash read using SAF EC Portal */
static int taf_xec_v2_flash_read(const struct device *dev, struct espi_saf_packet *pckt)
{
	return taf_v2_ecp_access(dev, pckt, XEC_TAFS_ECP_CTYPE_READ);
}

/* Flash write using SAF EC Portal */
static int taf_xec_v2_flash_write(const struct device *dev, struct espi_saf_packet *pckt)
{
	return taf_v2_ecp_access(dev, pckt, XEC_TAFS_ECP_CTYPE_WRITE);
}

/* Flash erase using SAF EC Portal */
static int taf_xec_v2_flash_erase(const struct device *dev, struct espi_saf_packet *pckt)
{
	return taf_v2_ecp_access(dev, pckt, XEC_TAFS_ECP_CTYPE_ERASE);
}

static int espi_taf_xec_v2_manage_callback(const struct device *dev, struct espi_callback *callback,
					   bool set)
{
	struct espi_taf_xec_data *const data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_taf_xec_v2_activate(const struct device *dev)
{
	const struct espi_taf_xec_config *const xcfg = dev->config;
	mm_reg_t tb = xcfg->taf_base;

	/* clear any eSPI bus monitor or portal status */
	sys_write32(XEC_TAFS_ECP_SR_MSK, tb + XEC_TAFS_ECP_SR_OFS);
	sys_write32(XEC_TAFS_ESPI_ERR_MSK, tb + XEC_TAFS_ESPI_BMON_SR_OFS);

	soc_ecia_girq_status_clear(xcfg->girq_bmon, xcfg->girq_bmon_pos);

	/* Activate TAF. Hardware locks QMSPI. QMSPI registers become inaccessible by the EC */
	sys_set_bit(tb + XEC_TAFS_CFG_MISC_OFS, XEC_TAFS_CFG_MISC_TAF_MODE_EN_POS);

	/* enable interrupts in bus monitor */
	sys_write32(XEC_TAFS_ESPI_ERR_MSK, tb + XEC_TAFS_ESPI_BMON_IER_OFS);

	/* delay for hardware to sort itself out */
	k_busy_wait(TAF_ACTIVATION_DELAY_US);

	return 0;
}

static void espi_xec_taf_v2_done_isr(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	struct espi_taf_xec_data *const data = dev->data;
	mm_reg_t tb = xcfg->taf_base;
	struct espi_event evt = {0};
	uint32_t ecp_status = sys_read32(tb + XEC_TAFS_ECP_SR_OFS);

	sys_write32(0, tb + XEC_TAFS_ECP_IER_OFS);
	sys_write32(ecp_status, tb + XEC_TAFS_ECP_SR_OFS);
	soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);

	data->ecp_hwstatus = ecp_status;

	LOG_DBG("SAF Done ISR: status=0x%x", ecp_status);

	evt.evt_type = ESPI_BUS_TAF_NOTIFICATION;
	evt.evt_details = ESPI_TAF_EVENT_EC0_REQUEST;
	evt.evt_data = ecp_status;

	/* Signal the driver API that our hardware is done */
	k_sem_give(&data->sync);
	/* relase the API lock in case the application callback wants to start another transfer */
	k_mutex_unlock(&data->ecp_lock);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

/* Interrupt handler for Host TAF requests completing with or without errors.
 * The application may register a callback to see these events.
 */
static void espi_taf_xec_v2_busmon_isr(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	struct espi_taf_xec_data *const data = dev->data;
	mm_reg_t tb = xcfg->taf_base;
	struct espi_event evt = {0};
	uint32_t mon_status = sys_read32(tb + XEC_TAFS_ESPI_BMON_SR_OFS);

	sys_write32(mon_status, tb + XEC_TAFS_ESPI_BMON_SR_OFS);
	soc_ecia_girq_status_clear(xcfg->girq_bmon, xcfg->girq_bmon_pos);

	data->hwstatus = mon_status;

	evt.evt_type = ESPI_BUS_TAF_NOTIFICATION;
	evt.evt_details = ESPI_TAF_EVENT_HOST_REQUEST;
	evt.evt_data = mon_status;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static DEVICE_API(espi_saf, espi_taf_xec_v2_driver_api) = {
	.config = espi_taf_xec_v2_config,
	.set_protection_regions = espi_taf_xec_v2_set_pr,
	.activate = espi_taf_xec_v2_activate,
	.get_channel_status = espi_taf_xec_v2_channel_ready,
	.flash_read = taf_xec_v2_flash_read,
	.flash_write = taf_xec_v2_flash_write,
	.flash_erase = taf_xec_v2_flash_erase,
	.manage_callback = espi_taf_xec_v2_manage_callback,
};

static int espi_taf_xec_v2_init(const struct device *dev)
{
	const struct espi_taf_xec_config *xcfg = dev->config;
	struct espi_taf_xec_data *const data = dev->data;
	struct xec_espi_cap_regs *const cap_regs =
		(struct xec_espi_cap_regs *)(xcfg->espi_ioc_base + MCHP_ESPI_IO_CAP_OFS);
	uint8_t temp8 = 0;

	/* ungate SAF clocks by disabling PCR sleep enable */
	soc_xec_pcr_sleep_en_clear(xcfg->enc_pcr);

	/* Configure the channels and its capabilities based on build config */
	cap_regs->CAP0 |= MCHP_ESPI_GBL_CAP0_FC_SUPP;
	temp8 = cap_regs->CAPFC & ~(MCHP_ESPI_FC_CAP_SHARE_MASK);
	cap_regs->CAPFC = temp8 | MCHP_ESPI_FC_CAP_SHARE_CAF_TAF;

	if (xcfg->irq_config_func != NULL) {
		xcfg->irq_config_func();
		soc_ecia_girq_ctrl(xcfg->girq, xcfg->girq_pos, MCHP_MEC_ECIA_GIRQ_EN);
		soc_ecia_girq_ctrl(xcfg->girq_bmon, xcfg->girq_bmon_pos, MCHP_MEC_ECIA_GIRQ_EN);
	}

	k_mutex_init(&data->ecp_lock);
	k_sem_init(&data->sync, 0, 1);

	return 0;
}

#define XEC_TAF_DT_GIRQ(inst, pos)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, pos))
#define XEC_TAF_DT_GIRQ_POS(inst, pos) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, pos))

#define ESPI_TAF_XEC_DEVICE(n)                                                                     \
	static volatile __aligned(4) uint8_t ecp_mem##n[MAX_SAF_ECP_BUFFER_SIZE];                  \
	static struct espi_taf_xec_data espi_taf_xec_drv_data##n;                                  \
	static void espi_taf_xec_connect_irqs##n(void)                                             \
	{                                                                                          \
		/* TAF data transfer done */                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    espi_xec_taf_v2_done_isr, DEVICE_DT_INST_GET(n), 0);                   \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
		/* TAF Bus Monitor */                                                              \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq), DT_INST_IRQ_BY_IDX(n, 1, priority),     \
			    espi_taf_xec_v2_busmon_isr, DEVICE_DT_INST_GET(n), 0);                 \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));                                         \
	}                                                                                          \
	static const struct espi_taf_xec_config espi_taf_xec_drv_cfg##n = {                        \
		.taf_base = (mm_reg_t)(DT_INST_REG_ADDR_BY_IDX(n, 0)),                             \
		.taf_comm_base = (mm_reg_t)(DT_INST_REG_ADDR_BY_IDX(n, 2)),                        \
		.espi_ioc_base = (mm_reg_t)(DT_REG_ADDR_BY_NAME(DT_INST_PARENT(n), io)),           \
		.qspi_base = (mm_reg_t)(DT_INST_REG_ADDR_BY_IDX(n, 1)),                            \
		.ecp_mem = (void *)ecp_mem##n,                                                     \
		.irq_config_func = espi_taf_xec_connect_irqs##n,                                   \
		.poll_timeout = DT_INST_PROP_OR(n, poll_timeout, MCHP_SAF_FLASH_POLL_TIMEOUT),     \
		.consec_rd_timeout =                                                               \
			DT_INST_PROP_OR(n, consec_rd_timeout, MCHP_SAF_FLASH_CONSEC_READ_TIMEOUT), \
		.sus_chk_delay = DT_INST_PROP_OR(n, sus_chk_delay, MCHP_SAF_FLASH_SUS_CHK_DELAY),  \
		.sus_rsm_interval =                                                                \
			DT_INST_PROP_OR(n, sus_rsm_interval, MCHP_SAF_FLASH_SUS_RSM_INTERVAL),     \
		.poll_interval = DT_INST_PROP_OR(n, poll_interval, MCHP_SAF_FLASH_POLL_INTERVAL),  \
		.girq = XEC_TAF_DT_GIRQ(n, 0),                                                     \
		.girq_pos = XEC_TAF_DT_GIRQ_POS(n, 0),                                             \
		.girq_bmon = XEC_TAF_DT_GIRQ(n, 1),                                                \
		.girq_bmon_pos = XEC_TAF_DT_GIRQ_POS(n, 1),                                        \
		.enc_pcr = DT_INST_PROP(n, pcr_scr),                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(0, &espi_taf_xec_v2_init, NULL, &espi_taf_xec_drv_data##n,           \
			      &espi_taf_xec_drv_cfg##n, POST_KERNEL,                               \
			      CONFIG_ESPI_TAF_INIT_PRIORITY, &espi_taf_xec_v2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESPI_TAF_XEC_DEVICE)
