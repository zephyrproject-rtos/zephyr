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

#undef ETR_MODE_MODE_CIRCULARBUF

#include "coresight_arm.h"

#define DT_DRV_COMPAT nordic_coresight_nrf

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cs_trace, CONFIG_DEBUG_CORESIGHT_NRF_LOG_LEVEL);

#define CTI_CH_TPIU_FLUSH_REQ_OFFSET (1)

#define TS_CLOCKRATE                                                                               \
	(DT_PROP(DT_NODELABEL(hsfll200), clock_frequency) /                                        \
	 CONFIG_DEBUG_CORESIGHT_NRF_TSGEN_CLK_DIV)

#define ATBREPLICATOR_IDFILTER_FORWARD_STM                                                         \
	BIT(CONFIG_DEBUG_CORESIGHT_NRF_ATB_TRACE_ID_STM_GLOBAL >> 4)
#define ATBFUNNEL211_STM_ENS_MASK BIT(2)

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

	sys_write32(BIT((CONFIG_DEBUG_CORESIGHT_NRF_TPIU_PORTSIZE - 1)), tpiu + TPIU_CSPSR_OFFSET);

	/* Continuous formatting  */
	if (IS_ENABLED(CONFIG_DEBUG_CORESIGHT_NRF_TPIU_FFCR_TRIG)) {
		sys_write32((TPIU_FFCR_ENFCONT_Msk | TPIU_FFCR_FONFLIN_Msk | TPIU_FFCR_ENFTC_Msk),
			    tpiu + TPIU_FFCR_OFFSET);
	} else {
		sys_write32((TPIU_FFCR_ENFCONT_Msk | TPIU_FFCR_ENFTC_Msk), tpiu + TPIU_FFCR_OFFSET);
	}

	sys_write32(CONFIG_DEBUG_CORESIGHT_NRF_TPIU_SYNC_FRAME_COUNT, tpiu + TPIU_FSCR_OFFSET);

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
	sys_write32(ETR_FFCR_ENFT_Msk, etr + ETR_FFCR_OFFSET);
	sys_write32(ETR_MODE_MODE_CIRCULARBUF, etr + ETR_MODE_OFFSET);
	sys_write32(ETR_CTL_TRACECAPTEN_Msk, etr + ETR_CTL_OFFSET);

	coresight_lock(etr);

	LOG_INF("Coresight Host ETR initialized");
}

static void nrf_stm_init(void)
{
	mem_addr_t stm = DT_REG_ADDR(DT_NODELABEL(stm));

	coresight_unlock(stm);

	sys_write32(1, stm + STM_STMAUXCR_OFFSET);

	sys_write32(TS_CLOCKRATE, stm + STM_STMTSFREQR_OFFSET);

	sys_write32((CONFIG_DEBUG_CORESIGHT_NRF_STM_SYNC_BYTE_COUNT & 0xFFF),
		    stm + STM_STMSYNCR_OFFSET);

	sys_write32(0xFFFFFFFF, stm + STM_STMSPER_OFFSET);

	if (IS_ENABLED(CONFIG_DEBUG_CORESIGHT_NRF_STM_HWEVENTS)) {
		sys_write32(0xFFFFFFFF, stm + STM_STMHEER_OFFSET);
		sys_write32((1 << STM_STMHEMCR_EN_Pos), stm + STM_STMHEMCR_OFFSET);
	}

	sys_write32(((CONFIG_DEBUG_CORESIGHT_NRF_ATB_TRACE_ID_STM_GLOBAL & STM_STMTCSR_TRACEID_Msk)
		     << STM_STMTCSR_TRACEID_Pos) |
			    (1 << STM_STMTCSR_EN_Pos) | (1 << STM_STMTCSR_TSEN_Pos),
		    stm + STM_STMTCSR_OFFSET);

	coresight_lock(stm);

	LOG_INF("CoreSight STM initialized with clockrate %u", TS_CLOCKRATE);
}

static void nrf_atbfunnel_init(mem_addr_t funnel_addr, uint32_t enable_set_mask)
{
	coresight_unlock(funnel_addr);

	uint32_t ctrlreg_old = sys_read32(funnel_addr + ATBFUNNEL_CTRLREG_OFFSET);

	const uint32_t funnel_hold_time = (((CONFIG_DEBUG_CORESIGHT_NRF_ATBFUNNEL_HOLD_TIME - 1)
					    << ATBFUNNEL_CTRLREG_HT_Pos) &
					   ATBFUNNEL_CTRLREG_HT_Msk);

	uint32_t ctrlreg_new = (ctrlreg_old & ~ATBFUNNEL_CTRLREG_HT_Msk) | funnel_hold_time |
			       (enable_set_mask & 0xFF);

	sys_write32(ctrlreg_new, funnel_addr + ATBFUNNEL_CTRLREG_OFFSET);

	coresight_lock(funnel_addr);
}

static void nrf_atbreplicator_init(mem_addr_t replicator_addr, uint32_t filter, bool ch0_allow,
				   bool ch1_allow)
{
	coresight_unlock(replicator_addr);

	uint32_t ch0_current = sys_read32(replicator_addr + ATBREPLICATOR_IDFILTER0_OFFSET);
	uint32_t ch1_current = sys_read32(replicator_addr + ATBREPLICATOR_IDFILTER1_OFFSET);

	if (ch0_allow) {
		sys_write32(ch0_current & ~filter,
			    replicator_addr + ATBREPLICATOR_IDFILTER0_OFFSET);
	} else {
		sys_write32(ch0_current | filter, replicator_addr + ATBREPLICATOR_IDFILTER0_OFFSET);
	}

	if (ch1_allow) {
		sys_write32(ch1_current & ~filter,
			    replicator_addr + ATBREPLICATOR_IDFILTER1_OFFSET);
	} else {
		sys_write32(ch1_current | filter, replicator_addr + ATBREPLICATOR_IDFILTER1_OFFSET);
	}

	coresight_lock(replicator_addr);
}

static int coresight_nrf_init_stm_etr(uintptr_t buf, size_t buf_word_len)
{
	mem_addr_t atbfunnel211 = DT_REG_ADDR(DT_NODELABEL(atbfunnel211));
	mem_addr_t atbreplicator210 = DT_REG_ADDR(DT_NODELABEL(atbreplicator210));
	mem_addr_t atbreplicator213 = DT_REG_ADDR(DT_NODELABEL(atbreplicator213));

	nrf_atbfunnel_init(atbfunnel211, ATBFUNNEL211_STM_ENS_MASK);
	nrf_atbreplicator_init(atbreplicator210, ATBREPLICATOR_IDFILTER_FORWARD_STM, false, true);
	nrf_atbreplicator_init(atbreplicator213, ATBREPLICATOR_IDFILTER_FORWARD_STM, false, true);

	nrf_tsgen_init();
	nrf_etr_init(buf, buf_word_len);
	nrf_stm_init();

	return 0;
}

static int coresight_nrf_init_stm_tpiu(void)
{
	mem_addr_t atbfunnel211 = DT_REG_ADDR(DT_NODELABEL(atbfunnel211));
	mem_addr_t atbreplicator210 = DT_REG_ADDR(DT_NODELABEL(atbreplicator210));
	mem_addr_t atbreplicator213 = DT_REG_ADDR(DT_NODELABEL(atbreplicator213));

	nrf_atbfunnel_init(atbfunnel211, ATBFUNNEL211_STM_ENS_MASK);
	nrf_atbreplicator_init(atbreplicator210, ATBREPLICATOR_IDFILTER_FORWARD_STM, false, true);
	nrf_atbreplicator_init(atbreplicator213, ATBREPLICATOR_IDFILTER_FORWARD_STM, true, false);

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
		return coresight_nrf_init_stm_tpiu();
	}
	case CORESIGHT_NRF_MODE_STM_ETR: {
		uintptr_t etr_buffer = DT_REG_ADDR(DT_NODELABEL(etr_buffer));
		size_t buf_word_len = DT_REG_SIZE(DT_NODELABEL(etr_buffer)) / sizeof(uint32_t);

		return coresight_nrf_init_stm_etr(etr_buffer, buf_word_len);
	}
	default: {
		LOG_ERR("Unsupported Coresight mode");
		return -ENOTSUP;
	}
	}
	return 0;
}

#define DEBUG_CORESIGHT_NRF_INIT_PRIORITY UTIL_INC(CONFIG_NRF_IRONSIDE_CALL_INIT_PRIORITY)

#define CORESIGHT_NRF_INST(inst)                                                                   \
	COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(inst, 0),                                      \
		    (PINCTRL_DT_INST_DEFINE(inst);), ())       \
                                                                                                   \
	static struct coresight_nrf_config coresight_nrf_cfg_##inst = {                            \
		.mode = _CONCAT(CORESIGHT_NRF_MODE_,                                               \
				DT_STRING_UPPER_TOKEN(DT_DRV_INST(inst), mode)),                   \
		.pcfg = COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(inst, 0),                     \
				    (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)),               \
				    (NULL)) };                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, coresight_nrf_init, NULL, NULL, &coresight_nrf_cfg_##inst,     \
			      POST_KERNEL, DEBUG_CORESIGHT_NRF_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CORESIGHT_NRF_INST)
