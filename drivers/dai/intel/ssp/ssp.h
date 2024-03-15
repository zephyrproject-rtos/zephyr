/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_SSP_H__
#define __INTEL_DAI_DRIVER_SSP_H__

#include <stdint.h>
#include <zephyr/drivers/dai.h>
#include "dai-params-intel-ipc3.h"
#include "dai-params-intel-ipc4.h"

#define DAI_INTEL_SSP_MASK(b_hi, b_lo)	\
	(((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL) << (b_lo))
#define DAI_INTEL_SSP_SET_BIT(b, x)		(((x) & 1) << (b))
#define DAI_INTEL_SSP_SET_BITS(b_hi, b_lo, x)	\
	(((x) & ((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL)) << (b_lo))
#define DAI_INTEL_SSP_GET_BIT(b, x) \
	(((x) & (1ULL << (b))) >> (b))
#define DAI_INTEL_SSP_GET_BITS(b_hi, b_lo, x) \
	(((x) & MASK(b_hi, b_lo)) >> (b_lo))
#define DAI_INTEL_SSP_IS_BIT_SET(reg, bit)	(((reg >> bit) & (0x1)) != 0)

/* ssp_freq array constants */
#define DAI_INTEL_SSP_NUM_FREQ			3
#define DAI_INTEL_SSP_MAX_FREQ_INDEX		(DAI_INTEL_SSP_NUM_FREQ - 1)
#define DAI_INTEL_SSP_DEFAULT_IDX		1

/* the SSP port fifo depth */
#define DAI_INTEL_SSP_FIFO_DEPTH		32

/* the watermark for the SSP fifo depth setting */
#define DAI_INTEL_SSP_FIFO_WATERMARK		8

/* minimal SSP port delay in cycles */
#define DAI_INTEL_SSP_PLATFORM_DELAY		1600
/* minimal SSP port delay in useconds */
#define DAI_INTEL_SSP_PLATFORM_DELAY_US		42
#define DAI_INTEL_SSP_PLATFORM_DEFAULT_DELAY	12
#define DAI_INTEL_SSP_DEFAULT_TRY_TIMES		8

/** \brief Number of SSP MCLKs available */
#define DAI_INTEL_SSP_NUM_MCLK			2

#define DAI_INTEL_SSP_CLOCK_XTAL_OSCILLATOR	0x0
#define DAI_INTEL_SSP_CLOCK_AUDIO_CARDINAL	0x1
#define DAI_INTEL_SSP_CLOCK_PLL_FIXED		0x2

/* SSP register offsets */
#define SSCR0			0x00
#define SSCR1			0x04
#define SSSR			0x08
#define SSITR			0x0C
#define SSDR			0x10
#define SSTO			0x28
#define SSPSP			0x2C
#define SSTSA			0x30
#define SSRSA			0x34
#define SSTSS			0x38
#define SSCR2			0x40

/* SSCR0 bits */
#define SSCR0_DSIZE(x)		DAI_INTEL_SSP_SET_BITS(3, 0, (x) - 1)
#define SSCR0_DSIZE_GET(x)	(((x) & DAI_INTEL_SSP_MASK(3, 0)) + 1)
#define SSCR0_FRF		DAI_INTEL_SSP_MASK(5, 4)
#define SSCR0_MOT		DAI_INTEL_SSP_SET_BITS(5, 4, 0)
#define SSCR0_TI		DAI_INTEL_SSP_SET_BITS(5, 4, 1)
#define SSCR0_NAT		DAI_INTEL_SSP_SET_BITS(5, 4, 2)
#define SSCR0_PSP		DAI_INTEL_SSP_SET_BITS(5, 4, 3)
#define SSCR0_ECS		BIT(6)
#define SSCR0_SSE		BIT(7)
#define SSCR0_SCR_MASK		DAI_INTEL_SSP_MASK(19, 8)
#define SSCR0_SCR(x)		DAI_INTEL_SSP_SET_BITS(19, 8, x)
#define SSCR0_EDSS		BIT(20)
#define SSCR0_NCS		BIT(21)
#define SSCR0_RIM		BIT(22)
#define SSCR0_TIM		BIT(23)
#define SSCR0_FRDC(x)		DAI_INTEL_SSP_SET_BITS(26, 24, (x) - 1)
#define SSCR0_FRDC_GET(x)	((((x) & DAI_INTEL_SSP_MASK(26, 24)) >> 24) + 1)
#define SSCR0_ACS		BIT(30)
#define SSCR0_MOD		BIT(31)

/* SSCR1 bits */
#define SSCR1_RIE		BIT(0)
#define SSCR1_TIE		BIT(1)
#define SSCR1_LBM		BIT(2)
#define SSCR1_SPO		BIT(3)
#define SSCR1_SPH		BIT(4)
#define SSCR1_MWDS		BIT(5)
#define SSCR1_TFT_MASK		DAI_INTEL_SSP_MASK(9, 6)
#define SSCR1_TFT(x)		DAI_INTEL_SSP_SET_BITS(9, 6, (x) - 1)
#define SSCR1_RFT_MASK		DAI_INTEL_SSP_MASK(13, 10)
#define SSCR1_RFT(x)		DAI_INTEL_SSP_SET_BITS(13, 10, (x) - 1)
#define SSCR1_EFWR		BIT(14)
#define SSCR1_STRF		BIT(15)
#define SSCR1_IFS		BIT(16)
#define SSCR1_PINTE		BIT(18)
#define SSCR1_TINTE		BIT(19)
#define SSCR1_RSRE		BIT(20)
#define SSCR1_TSRE		BIT(21)
#define SSCR1_TRAIL		BIT(22)
#define SSCR1_RWOT		BIT(23)
#define SSCR1_SFRMDIR		BIT(24)
#define SSCR1_SCLKDIR		BIT(25)
#define SSCR1_ECRB		BIT(26)
#define SSCR1_ECRA		BIT(27)
#define SSCR1_SCFR		BIT(28)
#define SSCR1_EBCEI		BIT(29)
#define SSCR1_TTE		BIT(30)
#define SSCR1_TTELP		BIT(31)

#define SSCR2_TURM1		BIT(1)
#define SSCR2_PSPSRWFDFD	BIT(3)
#define SSCR2_PSPSTWFDFD	BIT(4)
#define SSCR2_SDFD		BIT(14)
#define SSCR2_SDPM		BIT(16)
#define SSCR2_LJDFD		BIT(17)
#define SSCR2_MMRATF		BIT(18)
#define SSCR2_SMTATF		BIT(19)
#define SSCR2_SFRMEN		BIT(20)
#define SSCR2_ACIOLBS		BIT(21)

/* SSR bits */
#define SSSR_TNF		BIT(2)
#define SSSR_RNE		BIT(3)
#define SSSR_BSY		BIT(4)
#define SSSR_TFS		BIT(5)
#define SSSR_RFS		BIT(6)
#define SSSR_ROR		BIT(7)
#define SSSR_TUR		BIT(21)

/* SSPSP bits */
#define SSPSP_SCMODE(x)		DAI_INTEL_SSP_SET_BITS(1, 0, x)
#define SSPSP_SFRMP(x)		DAI_INTEL_SSP_SET_BIT(2, x)
#define SSPSP_ETDS		BIT(3)
#define SSPSP_STRTDLY(x)	DAI_INTEL_SSP_SET_BITS(6, 4, x)
#define SSPSP_DMYSTRT(x)	DAI_INTEL_SSP_SET_BITS(8, 7, x)
#define SSPSP_SFRMDLY(x)	DAI_INTEL_SSP_SET_BITS(15, 9, x)
#define SSPSP_SFRMWDTH(x)	DAI_INTEL_SSP_SET_BITS(21, 16, x)
#define SSPSP_DMYSTOP(x)	DAI_INTEL_SSP_SET_BITS(24, 23, x)
#define SSPSP_DMYSTOP_BITS	2
#define SSPSP_DMYSTOP_MASK	DAI_INTEL_SSP_MASK(SSPSP_DMYSTOP_BITS - 1, 0)
#define SSPSP_FSRT		BIT(25)
#define SSPSP_EDMYSTOP(x)	DAI_INTEL_SSP_SET_BITS(28, 26, x)

#define SSPSP2			0x44
#define SSPSP2_FEP_MASK		0xff

#define SSCR3			0x48
#define SSIOC			0x4C
#define SSP_REG_MAX		SSIOC

/* SSTSA bits */
#define SSTSA_SSTSA(x)		DAI_INTEL_SSP_SET_BITS(7, 0, x)
#define SSTSA_GET(x)		((x) & DAI_INTEL_SSP_MASK(7, 0))
#define SSTSA_TXEN		BIT(8)

/* SSRSA bits */
#define SSRSA_SSRSA(x)		DAI_INTEL_SSP_SET_BITS(7, 0, x)
#define SSRSA_GET(x)		((x) & DAI_INTEL_SSP_MASK(7, 0))
#define SSRSA_RXEN		BIT(8)

/* SSCR3 bits */
#define SSCR3_FRM_MST_EN	BIT(0)
#define SSCR3_I2S_MODE_EN	BIT(1)
#define SSCR3_I2S_FRM_POL(x)	DAI_INTEL_SSP_SET_BIT(2, x)
#define SSCR3_I2S_TX_SS_FIX_EN	BIT(3)
#define SSCR3_I2S_RX_SS_FIX_EN	BIT(4)
#define SSCR3_I2S_TX_EN		BIT(9)
#define SSCR3_I2S_RX_EN		BIT(10)
#define SSCR3_CLK_EDGE_SEL	BIT(12)
#define SSCR3_STRETCH_TX	BIT(14)
#define SSCR3_STRETCH_RX	BIT(15)
#define SSCR3_MST_CLK_EN	BIT(16)
#define SSCR3_SYN_FIX_EN	BIT(17)

/* SSCR4 bits */
#define SSCR4_TOT_FRM_PRD(x)	((x) << 7)

/* SSCR5 bits */
#define SSCR5_FRM_ASRT_CLOCKS(x)	(((x) - 1) << 1)
#define SSCR5_FRM_POLARITY(x)	DAI_INTEL_SSP_SET_BIT(0, x)

/* SFIFOTT bits */
#define SFIFOTT_TX(x)		((x) - 1)
#define SFIFOTT_RX(x)		(((x) - 1) << 16)

/* SFIFOL bits */
#define SFIFOL_TFL(x)		((x) & 0xFFFF)
#define SFIFOL_RFL(x)		((x) >> 16)

#define SSTSA_TSEN			BIT(8)
#define SSRSA_RSEN			BIT(8)

#define SSCR3_TFL_MASK	DAI_INTEL_SSP_MASK(5, 0)
#define SSCR3_RFL_MASK	DAI_INTEL_SSP_MASK(13, 8)
#define SSCR3_TFL_VAL(scr3_val)	(((scr3_val) >> 0) & DAI_INTEL_SSP_MASK(5, 0))
#define SSCR3_RFL_VAL(scr3_val)	(((scr3_val) >> 8) & DAI_INTEL_SSP_MASK(5, 0))
#define SSCR3_TX(x)	DAI_INTEL_SSP_SET_BITS(21, 16, (x) - 1)
#define SSCR3_RX(x)	DAI_INTEL_SSP_SET_BITS(29, 24, (x) - 1)

#define SSIOC_TXDPDEB	BIT(1)
#define SSIOC_SFCR	BIT(4)
#define SSIOC_SCOE	BIT(5)

/* For 8000 Hz rate one sample is transmitted within 125us */
#define DAI_INTEL_SSP_MAX_SEND_TIME_PER_SAMPLE 125

/* SSP flush retry counts maximum */
#define DAI_INTEL_SSP_RX_FLUSH_RETRY_MAX	16

#define SSP_CLK_MCLK_ES_REQ	BIT(0)
#define SSP_CLK_MCLK_ACTIVE	BIT(1)
#define SSP_CLK_BCLK_ES_REQ	BIT(2)
#define SSP_CLK_BCLK_ACTIVE	BIT(3)

#define I2SLCTL_OFFSET		0x04

#if defined(CONFIG_SOC_INTEL_ACE15_MTPM) || defined(CONFIG_SOC_SERIES_INTEL_ADSP_CAVS)
#define I2SLCTL_SPA(x)		BIT(0 + x)
#define I2SLCTL_CPA(x)		BIT(8 + x)
#elif defined(CONFIG_SOC_INTEL_ACE20_LNL)
#define I2SLCTL_OFLEN		BIT(4)
#define I2SLCTL_SPA(x)		BIT(16 + x)
#define I2SLCTL_CPA(x)		BIT(23 + x)
#define PCMS0CM_OFFSET		0x16
#define PCMS1CM_OFFSET		0x1A
#else
#error "Missing ssp definitions"
#endif

#define I2CLCTL_MLCS(x)		DAI_INTEL_SSP_SET_BITS(30, 27, x)
#define SHIM_CLKCTL		0x78
#define SHIM_CLKCTL_I2SFDCGB(x)		BIT(20 + x)
#define SHIM_CLKCTL_I2SEFDCGB(x)	BIT(18 + x)

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
/** \brief Offset of MCLK Divider Control Register. */
#define MN_MDIVCTRL 0x100

/** \brief Offset of MCLK Divider x Ratio Register. */
#define MN_MDIVR(x) (0x180 + (x) * 0x4)
#else
#define MN_MDIVCTRL 0x0
#define MN_MDIVR(x) (0x80 + (x) * 0x4)
#endif

/** \brief Enables the output of MCLK Divider. */
#define MN_MDIVCTRL_M_DIV_ENABLE(x) BIT(x)

/** \brief Bits for setting MCLK source clock. */
#define MCDSS(x)	DAI_INTEL_SSP_SET_BITS(17, 16, x)

/** \brief Offset of BCLK x M/N Divider M Value Register. */
#define MN_MDIV_M_VAL(x) (0x100 + (x) * 0x8 + 0x0)

/** \brief Offset of BCLK x M/N Divider N Value Register. */
#define MN_MDIV_N_VAL(x) (0x100 + (x) * 0x8 + 0x4)

/** \brief Bits for setting M/N source clock. */
#define MNDSS(x)	DAI_INTEL_SSP_SET_BITS(21, 20, x)

/** \brief Mask for clearing mclk and bclk source in MN_MDIVCTRL */
#define MN_SOURCE_CLKS_MASK 0x3

#if CONFIG_INTEL_MN
/** \brief BCLKs can be driven by multiple sources - M/N or XTAL directly.
 *	   Even in the case of M/N, the actual clock source can be XTAL,
 *	   Audio cardinal clock (24.576) or 96 MHz PLL.
 *	   The MN block is not really the source of clocks, but rather
 *	   an intermediate component.
 *	   Input for source is shared by all outputs coming from that source
 *	   and once it's in use, it can be adjusted only with dividers.
 *	   In order to change input, the source should not be in use, that's why
 *	   it's necessary to keep track of BCLKs sources to know when it's safe
 *	   to change shared input clock.
 */
enum bclk_source {
	MN_BCLK_SOURCE_NONE = 0, /**< port is not using any clock */
	MN_BCLK_SOURCE_MN, /**< port is using clock driven by M/N */
	MN_BCLK_SOURCE_XTAL, /**< port is using XTAL directly */
};
#endif

struct dai_intel_ssp_mn {
	uint32_t base;
	/**< keep track of which MCLKs are in use to know when it's safe to
	 * change shared clock
	 */
	int mclk_sources_ref[DAI_INTEL_SSP_NUM_MCLK];
	int mclk_rate[DAI_INTEL_SSP_NUM_MCLK];
	int mclk_source_clock;

#if CONFIG_INTEL_MN
	enum bclk_source bclk_sources[(CONFIG_DAI_INTEL_SSP_NUM_BASE +
				       CONFIG_DAI_INTEL_SSP_NUM_EXT)];
	int bclk_source_mn_clock;
#endif

	struct k_spinlock lock; /**< lock mechanism */
};

struct dai_intel_ssp_freq_table {
	uint32_t freq;
	uint32_t ticks_per_msec;
};

struct dai_intel_ssp_plat_fifo_data {
	uint32_t offset;
	uint32_t width;
	uint32_t depth;
	uint32_t watermark;
	uint32_t handshake;
};

struct dai_intel_ssp_plat_data {
	uint32_t base;
	uint32_t ip_base;
	uint32_t shim_base;
#ifdef CONFIG_SOC_INTEL_ACE20_LNL
	uint32_t hdamlssp_base;
	uint32_t i2svss_base;
#endif
	int irq;
	const char *irq_name;
	uint32_t flags;
	struct dai_intel_ssp_plat_fifo_data fifo[2];
	struct dai_intel_ssp_mn *mn_inst;
	struct dai_intel_ssp_freq_table *ftable;
	uint32_t *fsources;
};

struct dai_intel_ssp_pdata {
	uint32_t sscr0;
	uint32_t sscr1;
	uint32_t psp;
	uint32_t state[2];
	uint32_t clk_active;
	struct dai_config config;
	struct dai_properties props;
	struct dai_intel_ipc3_ssp_params params;
};

struct dai_intel_ssp {
	uint32_t index;		/**< index */
	struct k_spinlock lock;	/**< locking mechanism */
	int sref;		/**< simple ref counter, guarded by lock */
	struct dai_intel_ssp_plat_data plat_data;
	void *priv_data;
};

#endif
