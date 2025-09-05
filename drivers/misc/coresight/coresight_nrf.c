/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdbool.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <nrf_ironside/tdd.h>
#include <uicr/uicr.h>

#include "coresight_arm.h"

#define DT_DRV_COMPAT nordic_coresight_nrf

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cs_trace, CONFIG_CORESIGHT_NRF_LOG_LEVEL);

/* CTI Channel Bitmasks */
#define CTI_CH_TPIU_FLUSH_REQ_OFFSET (1)

#define TS_CLOCKRATE (CONFIG_CORESIGHT_NRF_TDD_CLOCK_FREQUENCY / CONFIG_CORESIGHT_NRF_TSGEN_CLK_DIV)

enum coresight_nrf_mode {
	CORESIGHT_NRF_MODE_UNCONFIGURED,
	CORESIGHT_NRF_MODE_STM_TPIU,
	CORESIGHT_NRF_MODE_STM_ETR,
};

struct coresight_nrf_config {
	enum coresight_nrf_mode mode;
	const struct pinctrl_dev_config *pcfg;
};

static void nrf_tsgen_init(void)
{
	mem_addr_t tsgen = DT_REG_ADDR(DT_NODELABEL(tsgen));

	coresight_unlock(tsgen);

	sys_write32(TS_CLOCKRATE, tsgen + TSGEN_CNTFID0_OFFSET);
	sys_write32(TSGEN_CNTCR_EN_Msk, tsgen + TSGEN_CNTCR_OFFSET);

	coresight_lock(tsgen);

	LOG_INF("CoreSight Host TSGEN initialized with clockrate %u", TS_CLOCKRATE);
}

static void nrf_cti_for_tpiu_init(void)
{
	mem_addr_t cti210 = DT_REG_ADDR(DT_NODELABEL(cti210));

	coresight_unlock(cti210);

	/* Connect CTI channel to TPIU formatter flushin */
	sys_write32(BIT(CTI_CH_TPIU_FLUSH_REQ_OFFSET), cti210 + CTI_CTIOUTEN0_OFFSET);
	sys_write32(BIT(CTI_CH_TPIU_FLUSH_REQ_OFFSET), cti210 + CTI_CTIGATE_OFFSET);
	sys_write32(CTI_CTICONTROL_GLBEN_Msk, cti210 + CTI_CTICONTROL_OFFSET);

	coresight_lock(cti210);

	LOG_INF("CoreSight Host CTI initialized");
}

static void nrf_tpiu_init(void)
{
	mem_addr_t tpiu = DT_REG_ADDR(DT_NODELABEL(tpiu));

	coresight_unlock(tpiu);

	sys_write32(BIT((CONFIG_CORESIGHT_NRF_TPIU_PORTSIZE - 1)), tpiu + TPIU_CSPSR_OFFSET);

	/* Continuous formatting  */
	if (IS_ENABLED(CONFIG_CORESIGHT_NRF_TPIU_FFCR_TRIG)) {
		sys_write32((TPIU_FFCR_ENFCONT_Msk | TPIU_FFCR_FONFLIN_Msk | TPIU_FFCR_ENFTC_Msk),
			    tpiu + TPIU_FFCR_OFFSET);
	} else {
		sys_write32((TPIU_FFCR_ENFCONT_Msk | TPIU_FFCR_ENFTC_Msk), tpiu + TPIU_FFCR_OFFSET);
	}

	sys_write32(CONFIG_CORESIGHT_NRF_TPIU_SYNC_FRAME_COUNT, tpiu + TPIU_FSCR_OFFSET);

	coresight_lock(tpiu);

	LOG_INF("CoreSight Host TPIU initialized");
}

static void nrf_etr_init(uintptr_t buf, size_t buf_word_len)
{
	mem_addr_t etr = DT_REG_ADDR(DT_NODELABEL(etr));

	coresight_unlock(etr);

	sys_write32(buf_word_len, etr + ETR_RSZ_OFFSET);
	sys_write32(buf, etr + ETR_RWP_OFFSET);
	sys_write32(buf, etr + ETR_DBALO_OFFSET);
	sys_write32(0UL, etr + ETR_DBAHI_OFFSET);
	sys_write32(ETR_FFCR_ENFT_Msk, etr + ETR_FFCR_OFFSET); /* Enable formatting. Normal */
	sys_write32(ETR_MODE_MODE_CIRCULARBUF, etr + ETR_MODE_OFFSET); /* Circular buffer mode */
	sys_write32(ETR_CTL_TRACECAPTEN_Msk, etr + ETR_CTL_OFFSET);

	coresight_lock(etr);

	LOG_INF("Coresight Host ETR initialized");
}

static void nrf_stm_init(void)
{
	mem_addr_t stm = DT_REG_ADDR(DT_NODELABEL(stm));

	/* Acquire access to stm configuration */
	coresight_unlock(stm);

	/* Auto flush */
	sys_write32(1, stm + STM_STMAUXCR_OFFSET);

	sys_write32(TS_CLOCKRATE, stm + STM_STMTSFREQR_OFFSET);

	sys_write32((CONFIG_CORESIGHT_NRF_STM_SYNC_BYTE_COUNT & 0xFFF), stm + STM_STMSYNCR_OFFSET);

	/* Enable all channels */
	sys_write32(0xFFFFFFFF, stm + STM_STMSPER_OFFSET);

	if (IS_ENABLED(CONFIG_CORESIGHT_NRF_STM_HWEVENTS)) {
		sys_write32(0xFFFFFFFF, stm + STM_STMHEER_OFFSET);
		sys_write32((1 << STM_STMHEMCR_EN_Pos), stm + STM_STMHEMCR_OFFSET);
	}

	/* Set traceid and enable STM tracing with timestamps */
	sys_write32(((CONFIG_CORESIGHT_NRF_ATB_TRACE_ID_STM_GLOBAL & STM_STMTCSR_TRACEID_Msk)
		     << STM_STMTCSR_TRACEID_Pos) |
			    (1 << STM_STMTCSR_EN_Pos) | (1 << STM_STMTCSR_TSEN_Pos),
		    stm + STM_STMTCSR_OFFSET);

	coresight_lock(stm);

	LOG_INF("CoreSight STM initialized with clockrate %u", TS_CLOCKRATE);
}

/**
 * @brief Configure ATB Funnel peripheral
 *
 * @param funnel_addr Base address of the ATB Funnel
 * @param ch_mask Channel mask to enable
 * @param hold_time Hold time value for the funnel
 * @param claim_mask Claim mask for the funnel
 */
static void nrf_atbfunnel_init(mem_addr_t funnel_addr, uint32_t ch_mask)
{
	coresight_unlock(funnel_addr);

	const uint32_t funnel_hold_time =
		(((CONFIG_CORESIGHT_NRF_ATBFUNNEL_HOLD_TIME - 1) << ATBFUNNEL_CTRLREG_HT_Pos) &
		 ATBFUNNEL_CTRLREG_HT_Msk);
	*(volatile uint32_t *)(funnel_addr + ATBFUNNEL_CTRLREG_OFFSET) =
		(funnel_hold_time | (ch_mask & 0xFF));

	coresight_lock(funnel_addr);
}

/**
 * @brief Configure ATB Replicator peripheral
 *
 * @param replicator_addr Base address of the ATB Replicator
 * @param ch0_filter Channel 0 filter value
 * @param ch1_filter Channel 1 filter value
 * @param claim_mask Claim mask for the replicator
 */
static void nrf_atbreplicator_init(mem_addr_t replicator_addr, uint32_t ch0_filter,
				   uint32_t ch1_filter)
{
	coresight_unlock(replicator_addr);

	*(volatile uint32_t *)(replicator_addr + ATBREPLICATOR_IDFILTER0_OFFSET) = ch0_filter;
	*(volatile uint32_t *)(replicator_addr + ATBREPLICATOR_IDFILTER1_OFFSET) = ch1_filter;

	coresight_lock(replicator_addr);
}

int coresight_nrf_init_etr(uintptr_t buf, size_t buf_word_len)
{
	mem_addr_t atbfunnel210 = DT_REG_ADDR(DT_NODELABEL(atbfunnel210));
	mem_addr_t atbfunnel211 = DT_REG_ADDR(DT_NODELABEL(atbfunnel211));
	mem_addr_t atbfunnel212 = DT_REG_ADDR(DT_NODELABEL(atbfunnel212));
	mem_addr_t atbfunnel213 = DT_REG_ADDR(DT_NODELABEL(atbfunnel213));
	mem_addr_t atbreplicator210 = DT_REG_ADDR(DT_NODELABEL(atbreplicator210));
	mem_addr_t atbreplicator211 = DT_REG_ADDR(DT_NODELABEL(atbreplicator211));
	mem_addr_t atbreplicator212 = DT_REG_ADDR(DT_NODELABEL(atbreplicator212));
	mem_addr_t atbreplicator213 = DT_REG_ADDR(DT_NODELABEL(atbreplicator213));

	nrf_atbfunnel_init(atbfunnel210, ATBFUNNEL_CTRLREG_ENS_NONE);
	nrf_atbfunnel_init(atbfunnel211, ATBFUNNEL_CTRLREG_ENS_ALL);
	nrf_atbfunnel_init(atbfunnel212, ATBFUNNEL_CTRLREG_ENS_ALL);
	nrf_atbfunnel_init(atbfunnel213, ATBFUNNEL_CTRLREG_ENS_ALL);

	nrf_atbreplicator_init(atbreplicator210, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);
	nrf_atbreplicator_init(atbreplicator211, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);
	nrf_atbreplicator_init(atbreplicator212, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);
	nrf_atbreplicator_init(atbreplicator213, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);

	nrf_tsgen_init();
	nrf_etr_init(buf, buf_word_len);
	nrf_stm_init();

	return 0;
}

int coresight_nrf_init_tpiu(void)
{
	mem_addr_t atbfunnel210 = DT_REG_ADDR(DT_NODELABEL(atbfunnel210));
	mem_addr_t atbfunnel211 = DT_REG_ADDR(DT_NODELABEL(atbfunnel211));
	mem_addr_t atbfunnel212 = DT_REG_ADDR(DT_NODELABEL(atbfunnel212));
	mem_addr_t atbfunnel213 = DT_REG_ADDR(DT_NODELABEL(atbfunnel213));
	mem_addr_t atbreplicator210 = DT_REG_ADDR(DT_NODELABEL(atbreplicator210));
	mem_addr_t atbreplicator211 = DT_REG_ADDR(DT_NODELABEL(atbreplicator211));
	mem_addr_t atbreplicator212 = DT_REG_ADDR(DT_NODELABEL(atbreplicator212));
	mem_addr_t atbreplicator213 = DT_REG_ADDR(DT_NODELABEL(atbreplicator213));

	nrf_atbfunnel_init(atbfunnel210, ATBFUNNEL_CTRLREG_ENS_NONE);
	nrf_atbfunnel_init(atbfunnel211, ATBFUNNEL_CTRLREG_ENS_ALL);
	nrf_atbfunnel_init(atbfunnel212, ATBFUNNEL_CTRLREG_ENS_ALL);
	nrf_atbfunnel_init(atbfunnel213, ATBFUNNEL_CTRLREG_ENS_ALL);

	nrf_atbreplicator_init(atbreplicator210, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);
	nrf_atbreplicator_init(atbreplicator211, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);
	nrf_atbreplicator_init(atbreplicator212, ATBREPLICATOR_IDFILTER_BLOCK_ALL,
			       ATBREPLICATOR_IDFILTER_FORWARD_ALL);
	nrf_atbreplicator_init(atbreplicator213, ATBREPLICATOR_IDFILTER_FORWARD_ALL,
			       ATBREPLICATOR_IDFILTER_BLOCK_ALL);

	nrf_tsgen_init();
	nrf_cti_for_tpiu_init();
	nrf_tpiu_init();
	nrf_stm_init();

	return 0;
}

static int coresight_nrf_init(const struct device *dev)
{
	int err;
	struct coresight_nrf_config *cfg = (struct coresight_nrf_config *)dev->config;

	if (cfg->pcfg) {
		err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			LOG_ERR("Failed to configure pins (%d)", err);
			return err;
		}
	}

	err = ironside_se_tdd_configure(IRONSIDE_SE_TDD_CONFIG_ON_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure TDD (%d)", err);
		return err;
	}

	switch (cfg->mode) {
	case CORESIGHT_NRF_MODE_UNCONFIGURED: {
		return 0;
	}
	case CORESIGHT_NRF_MODE_STM_TPIU: {
		return coresight_nrf_init_tpiu();
	}
	case CORESIGHT_NRF_MODE_STM_ETR: {
		uintptr_t etr_buffer = DT_REG_ADDR(DT_NODELABEL(etr_buffer));
		size_t buf_word_len = DT_REG_SIZE(DT_NODELABEL(etr_buffer)) / sizeof(uint32_t);

		return coresight_nrf_init_etr(etr_buffer, buf_word_len);
	}
	default: {
		LOG_ERR("Unsupported Coresight mode");
		return -ENOTSUP;
	}
	}
	return 0;
}

#define CORESIGHT_NRF_INIT_PRIORITY UTIL_INC(CONFIG_NRF_IRONSIDE_CALL_INIT_PRIORITY)

#define CORESIGHT_NRF_INST(inst)                                                   \
	COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(inst, 0),                                      \
		    (PINCTRL_DT_INST_DEFINE(inst);), ())                                   \
	                                                                                   \
	static struct coresight_nrf_config coresight_nrf_cfg_##inst = {                   \
		.mode = _CONCAT(CORESIGHT_NRF_MODE_,                                       \
				DT_STRING_UPPER_TOKEN(DT_DRV_INST(inst), mode)),           \
		.pcfg = COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(inst, 0),                     \
				    (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)), (NULL)) };        \
	                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, coresight_nrf_init, NULL, NULL,                       \
			      &coresight_nrf_cfg_##inst, POST_KERNEL,                   \
			      CORESIGHT_NRF_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CORESIGHT_NRF_INST)
