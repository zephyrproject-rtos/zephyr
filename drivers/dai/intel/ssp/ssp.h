/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_SSP_H__
#define __INTEL_DAI_DRIVER_SSP_H__

#define SSP_IP_VER_1_0 0x10000 /* cAVS */
#define SSP_IP_VER_1_5 0x10500 /* ACE15 */
#define SSP_IP_VER_2_0 0x20000 /* ACE20 */
#define SSP_IP_VER_3_0 0x30000 /* ACE30 */
#define SSP_IP_VER_4_0 0x40000 /* ACE40 */

/* SSP IP version defined by CONFIG_SOC*/
#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_CAVS)
#define SSP_IP_VER SSP_IP_VER_1_0
#elif defined(CONFIG_SOC_INTEL_ACE15_MTPM)
#define SSP_IP_VER SSP_IP_VER_1_5
#elif defined(CONFIG_SOC_INTEL_ACE20_LNL)
#define SSP_IP_VER SSP_IP_VER_2_0
#elif defined(CONFIG_SOC_INTEL_ACE30)
#define SSP_IP_VER SSP_IP_VER_3_0
#elif defined(CONFIG_SOC_INTEL_ACE40)
#define SSP_IP_VER SSP_IP_VER_4_0
#else
#error "Unknown SSP IP"
#endif

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

#if defined(CONFIG_SOC_INTEL_ACE15_MTPM) || defined(CONFIG_SOC_SERIES_INTEL_ADSP_CAVS)
#include "ssp_regs_v1.h"
#elif defined(CONFIG_SOC_INTEL_ACE20_LNL)
#include "ssp_regs_v2.h"
#elif defined(CONFIG_SOC_INTEL_ACE30) || defined(CONFIG_SOC_INTEL_ACE40)
#include "ssp_regs_v3.h"
#else
#error "Missing ssp definitions"
#endif

#if SSP_IP_VER == SSP_IP_VER_1_0
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

#if SSP_IP_VER == SSP_IP_VER_1_0
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
	uint32_t ssp_index;
	int acquire_count;
	bool is_initialized;
	bool is_power_en;
	uint32_t base;
	uint32_t ip_base;
	uint32_t shim_base;
#if SSP_IP_VER > SSP_IP_VER_1_5
	uint32_t hdamlssp_base;
	uint32_t i2svss_base;
#endif
#if SSP_IP_VER >= SSP_IP_VER_1_5
	uint32_t link_clock;
#endif
	int irq;
	const char *irq_name;
	uint32_t flags;
	struct dai_intel_ssp_plat_fifo_data fifo[2];
	struct dai_intel_ssp_mn *mn_inst;
	struct dai_intel_ssp_freq_table *ftable;
	uint32_t *fsources;
	uint32_t clk_active;
	struct dai_intel_ipc3_ssp_params params;
};

struct dai_intel_ssp_pdata {
	uint32_t sscr0;
	uint32_t sscr1;
	uint32_t psp;
	struct dai_config config;
	struct dai_properties props;
};

struct dai_intel_ssp {
	uint32_t dai_index;
	uint32_t ssp_index;
	uint32_t tdm_slot_group;
	uint32_t state[2];
	struct k_spinlock lock;	/**< locking mechanism */
	int sref;		/**< simple ref counter, guarded by lock */
	struct dai_intel_ssp_plat_data *ssp_plat_data;
	void *priv_data;
};

#endif
