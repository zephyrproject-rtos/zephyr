/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */
#include <stddef.h>
#include <stdbool.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <nrf_ironside/tdd.h>
#include <uicr/uicr.h>

#include "nrf_coresight_fixups.h"

#define DT_DRV_COMPAT nordic_nrf_coresight

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cs_trace, CONFIG_NRF_CORESIGHT_LOG_LEVEL);

#define FIRMWARE_CLAIM_MASK BIT(CONFIG_NRF_CORESIGHT_FW_CLAIM_BIT)

/* CTI Channel Bitmasks */
#define CTI_CH_ETB_FORMATTER_STOP_OFFSET (0)
#define CTI_CH_TPIU_FLUSH_REQ_OFFSET (1)

/* STM Bitmasks and Offsets */
#define STM_STMTCSR_TRACEID_MASK (0x7F)
#define STM_STMHEMCR_EN_OFFSET (0)
#define STM_STMTCSR_EN_OFFSET (0)
#define STM_STMTCSR_TSEN_OFFSET (1)
#define STM_STMTCSR_TRACEID_OFFSET (16)

#define TS_CLOCKRATE                                                                               \
	(CONFIG_NRF_CORESIGHT_TDD_CLOCK_FREQUENCY / CONFIG_NRF_CORESIGHT_TSGEN_CLK_DIV)

/* Common key used to unlock ARM CoreSight peripherals. */
#define CORESIGHT_UNLOCK_KEY (0xC5ACCE55UL)

/* Lock bit offset in CoreSight Lock Status Register (LSR) */
#define CORESIGHT_LSR_LOCKED_OFFSET (1)

enum nrf_coresight_mode {
	NRF_CORESIGHT_MODE_STM_TPIU,
	NRF_CORESIGHT_MODE_STM_ETR,
};


struct nrf_coresight_config {
	enum nrf_coresight_mode mode;
	const struct pinctrl_dev_config *pcfg;
};


/* Check if a Lock Status Register (LSR) denotes a locked or unlocked state for a
 * CoreSight peripheral
 */
static inline bool lsr_is_locked(const volatile uint32_t *lsr)
{
	return (*lsr & (1 << CORESIGHT_LSR_LOCKED_OFFSET)) != 0;
}

static int unlock_periph(void *dev)
{
	/* All Coresight peripherals share a common subset of Coresight registers at the same
	 * offset, which include LSR/LAR.
	 */
	NRF_STM_Type *const cs_periph = (NRF_STM_Type *)dev;

	int exit_cnt = CONFIG_NRF_CORESIGHT_RETRY_COUNT;

	if (!lsr_is_locked(&cs_periph->LSR)) {
		return 0;
	}

	do {
		cs_periph->LAR = CORESIGHT_UNLOCK_KEY;
	} while ((exit_cnt-- > 0) && lsr_is_locked(&cs_periph->LSR));

	return lsr_is_locked(&cs_periph->LSR) ? -EBUSY : 0;
}

static int lock_periph(void *dev)
{
	/* All Coresight peripherals share a common subset of Coresight registers at the same
	 * offset, which include LSR/LAR.
	 */
	NRF_STM_Type *const cs_periph = (NRF_STM_Type *)dev;

	cs_periph->LAR = 0;

	return lsr_is_locked(&cs_periph->LSR) ? -EBUSY : 0;
}

static uint32_t periph_claim_get(void *dev)
{
	NRF_ATBFUNNEL_Type *const cs_periph = (NRF_ATBFUNNEL_Type *)dev;

	return cs_periph->CLAIMCLR;
}

static void periph_claim_set(void *dev, uint32_t mask)
{
	NRF_ATBFUNNEL_Type *const cs_periph = (NRF_ATBFUNNEL_Type *)dev;

	cs_periph->CLAIMSET = mask;
}

static bool periph_is_configured_externally(void *addr)
{
	return (periph_claim_get(addr) & ~FIRMWARE_CLAIM_MASK) != 0;
}

static void configure_atb_funnel(NRF_ATBFUNNEL_Type *funnel, uint32_t ch_mask)
{
	unlock_periph(funnel);

	if (!periph_is_configured_externally(funnel)) {
		const uint32_t funnel_hold_time = (((CONFIG_NRF_CORESIGHT_ATBFUNNEL_HOLD_TIME - 1)
						    << ATBFUNNEL_CTRLREG_HT_Pos) &
						   ATBFUNNEL_CTRLREG_HT_Msk);

		funnel->CTRLREG = (funnel_hold_time | (ch_mask & 0xFF));
	} else {
		funnel->CTRLREG = funnel->CTRLREG | (ch_mask & 0xFF);
	}

	periph_claim_set(funnel, FIRMWARE_CLAIM_MASK);
	lock_periph(funnel);
}

static void configure_atb_replicator(NRF_ATBREPLICATOR_Type *replicator, uint32_t ch0_filter,
				     uint32_t ch1_filter)
{
	unlock_periph(replicator);

	if (!periph_is_configured_externally(replicator)) {
		replicator->IDFILTER0 = ch0_filter;
		replicator->IDFILTER1 = ch1_filter;
	} else {
		replicator->IDFILTER0 = replicator->IDFILTER0 & ch0_filter;
		replicator->IDFILTER1 = replicator->IDFILTER1 & ch1_filter;
	}

	periph_claim_set(replicator, FIRMWARE_CLAIM_MASK);
	lock_periph(replicator);
}

static void cti_init(bool use_etb, bool use_tpiu)
{
	NRF_CTI_Type *const cti0 = NRF_CTI211;

	if (use_etb) {
		unlock_periph(cti0);

		/* Configure ETB formatter stop */
		if (!periph_is_configured_externally(cti0)) {
			cti0->CTIOUTEN[0] = BIT(CTI_CH_ETB_FORMATTER_STOP_OFFSET);
			cti0->CTIGATE = BIT(CTI_CH_ETB_FORMATTER_STOP_OFFSET);
		} else {
			cti0->CTIOUTEN[0] =
				cti0->CTIOUTEN[0] | BIT(CTI_CH_ETB_FORMATTER_STOP_OFFSET);
			cti0->CTIGATE = cti0->CTIGATE | BIT(CTI_CH_ETB_FORMATTER_STOP_OFFSET);
		}
		cti0->CTICONTROL = 1;

		periph_claim_set(cti0, FIRMWARE_CLAIM_MASK);
		lock_periph(cti0);
	}

	if (use_tpiu && IS_ENABLED(CONFIG_NRF_CORESIGHT_TPIU_FFCR_TRIG)) {
		/* Configure CTI1 for TPIU flush */
		NRF_CTI_Type *const cti1 = NRF_CTI210;

		unlock_periph(cti1);

		/* Connect CTI channel to TPIU formatter flushin */
		if (!periph_is_configured_externally(cti1)) {
			cti1->CTIOUTEN[0] = BIT(CTI_CH_TPIU_FLUSH_REQ_OFFSET);
			cti1->CTIGATE = BIT(CTI_CH_TPIU_FLUSH_REQ_OFFSET);
		} else {
			cti1->CTIOUTEN[0] = cti1->CTIOUTEN[0] | BIT(CTI_CH_TPIU_FLUSH_REQ_OFFSET);
			cti1->CTIGATE = cti1->CTIGATE | BIT(CTI_CH_TPIU_FLUSH_REQ_OFFSET);
		}
		cti1->CTICONTROL = 1;

		periph_claim_set(cti1, FIRMWARE_CLAIM_MASK);
		lock_periph(cti1);
	}

	LOG_INF("CoreSight Host CTI initialized");
}

static void ts_gen_init(void)
{
	NRF_TSGEN_Type *const tsgen = NRF_TSGEN;

	if (!periph_is_configured_externally(tsgen)) {
		tsgen->CNTFID0 = TS_CLOCKRATE;

		/* Enable timestamp generator */
		tsgen->CNTCR = 1;

		LOG_INF("CoreSight Host TSGEN initialized with clockrate %u", TS_CLOCKRATE);
	} else {
		LOG_INF("CoreSight Host TSGEN is initialized externally.");
	}
}

static void tpiu_init(void)
{
	NRF_TPIU_Type *const tpiu = NRF_TPIU;

	unlock_periph(tpiu);

	/* TPIU portsize to 4 */
	tpiu->CURRENTPORTSIZE = BIT((CONFIG_NRF_CORESIGHT_TPIU_PORTSIZE - 1));

	/* Continuous formatting  */
	if (IS_ENABLED(CONFIG_NRF_CORESIGHT_TPIU_FFCR_TRIG)) {
		tpiu->FFCR = tpiu->FFCR |
			     (TPIU_FFCR_ENFCONT_Msk | TPIU_FFCR_FONFLIN_Msk | TPIU_FFCR_ENFTC_Msk);
	} else {
		tpiu->FFCR = tpiu->FFCR | TPIU_FFCR_ENFCONT_Msk | TPIU_FFCR_ENFTC_Msk;
	}

	tpiu->FSCR = CONFIG_NRF_CORESIGHT_TPIU_SYNC_FRAME_COUNT;

	periph_claim_set(tpiu, FIRMWARE_CLAIM_MASK);
	lock_periph(tpiu);

	LOG_INF("CoreSight Host TPIU initialized");
}

static void etr_init(uintptr_t buf, size_t buf_word_len)
{
	unlock_periph(NRF_ETR);

	NRF_ETR->RSZ = buf_word_len;
	NRF_ETR->RWP = buf;
	NRF_ETR->DBALO = buf;
	NRF_ETR->DBAHI = 0UL;
	NRF_ETR->FFCR = 1UL; /* Enable formatting. Normal */
	NRF_ETR->MODE = ETR_MODE_MODE_CIRCULARBUF; /* Circular buffer mode */
	NRF_ETR->CTL = 1UL;

	periph_claim_set(NRF_ETR, FIRMWARE_CLAIM_MASK);
	lock_periph(NRF_ETR);

	LOG_INF("Coresight Host ETR initialized");
}

static void stm_init(bool enable_hwevents)
{
	NRF_STM_Type *const stm = NRF_STM;

	/* Acquire access to stm configuration */
	unlock_periph(stm);

	/* Auto flush */
	stm->STMAUXCR = 1;

	stm->STMTSFREQR = TS_CLOCKRATE;
	__DSB();

	stm->STMSYNCR = (CONFIG_NRF_CORESIGHT_STM_SYNC_BYTE_COUNT & 0xFFF);
	__DSB();

	/* Enable all channels */
	stm->STMSPER = 0xFFFFFFFF;
	__DSB();

	if (enable_hwevents) {
		stm->STMHEER = 0xFFFFFFFF; /* All hardware events enabled */
		stm->STMHEMCR = (1 << STM_STMHEMCR_EN_OFFSET);
		__DSB();
	}

	/* Set traceid and enable STM tracing with timestamps */
	stm->STMTCSR = ((CONFIG_NRF_CORESIGHT_ATB_TRACE_ID_STM_GLOBAL & STM_STMTCSR_TRACEID_MASK)
			<< STM_STMTCSR_TRACEID_OFFSET) |
		       (1 << STM_STMTCSR_EN_OFFSET) | (1 << STM_STMTCSR_TSEN_OFFSET);
	__DSB();

	periph_claim_set(stm, FIRMWARE_CLAIM_MASK);
	lock_periph(stm);

	LOG_INF("CoreSight STM initialized with clockrate %u", TS_CLOCKRATE);
}

#define ATBFUNNEL_ALLOW_ALL 0xFF
#define ATBFUNNEL_ALLOW_NOTHING 0x0

#define ATBREPLICATOR_FORWARD_NOTHING 0xFFFFFFFF
#define ATBREPLICATOR_FORWARD_ALL 0x0

int nrf_coresight_init_etr(uintptr_t buf, size_t buf_word_len) {
	configure_atb_funnel(NRF_ATBFUNNEL210, ATBFUNNEL_ALLOW_NOTHING);
	configure_atb_funnel(NRF_ATBFUNNEL211, ATBFUNNEL_ALLOW_ALL);
	configure_atb_funnel(NRF_ATBFUNNEL212, ATBFUNNEL_ALLOW_ALL);
	configure_atb_funnel(NRF_ATBFUNNEL213, ATBFUNNEL_ALLOW_ALL);

	configure_atb_replicator(NRF_ATBREPLICATOR210, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);
	configure_atb_replicator(NRF_ATBREPLICATOR211, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);
	configure_atb_replicator(NRF_ATBREPLICATOR212, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);
	configure_atb_replicator(NRF_ATBREPLICATOR213, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);

	ts_gen_init();
	cti_init(false, true);
	etr_init(buf, buf_word_len);
	stm_init(false);

	return 0;
}

int nrf_coresight_init_tpiu(void)
{
	configure_atb_funnel(NRF_ATBFUNNEL210, ATBFUNNEL_ALLOW_NOTHING);
	configure_atb_funnel(NRF_ATBFUNNEL211, ATBFUNNEL_ALLOW_ALL);
	configure_atb_funnel(NRF_ATBFUNNEL212, ATBFUNNEL_ALLOW_ALL);
	configure_atb_funnel(NRF_ATBFUNNEL213, ATBFUNNEL_ALLOW_ALL);

	configure_atb_replicator(NRF_ATBREPLICATOR210, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);
	configure_atb_replicator(NRF_ATBREPLICATOR211, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);
	configure_atb_replicator(NRF_ATBREPLICATOR212, ATBREPLICATOR_FORWARD_NOTHING,
				ATBREPLICATOR_FORWARD_ALL);
	configure_atb_replicator(NRF_ATBREPLICATOR213, ATBREPLICATOR_FORWARD_ALL,
				ATBREPLICATOR_FORWARD_NOTHING);

	ts_gen_init();
	cti_init(false, true);
	tpiu_init();
	stm_init(false);

	return 0;
}

static int nrf_coresight_init(const struct device *dev) {
	int err;
	struct nrf_coresight_config *cfg = (struct nrf_coresight_config *)dev->config;

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
	case NRF_CORESIGHT_MODE_STM_TPIU: {
		return nrf_coresight_init_tpiu();
	}
	case NRF_CORESIGHT_MODE_STM_ETR: {
		uintptr_t etr_buffer = (DT_REG_ADDR(DT_NODELABEL(etr_buffer)));
		size_t buf_word_len = DT_REG_SIZE(DT_NODELABEL(etr_buffer)) / sizeof(uint32_t);
		return nrf_coresight_init_etr(etr_buffer, buf_word_len);
	}
	default: {
		LOG_ERR("Unsupported Coresight mode");
		return -ENOTSUP;
	}
	}
    return 0;
}

#define NRF_CORESIGHT_INIT_PRIORITY UTIL_INC(CONFIG_NRF_IRONSIDE_CALL_INIT_PRIORITY)

#define NRF_CORESIGHT_INST(inst)                                                                  \
	COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(inst, 0), (PINCTRL_DT_INST_DEFINE(inst);), ())       \
																								   \
	static struct nrf_coresight_config nrf_coresight_cfg_##inst = {                           \
		.mode = _CONCAT(NRF_CORESIGHT_MODE_, DT_STRING_UPPER_TOKEN(DT_DRV_INST(inst), mode)), \
		.pcfg = COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(inst, 0),                             \
				    (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)), (NULL))               \
	};                                                                                         \
																								   \
	DEVICE_DT_INST_DEFINE(inst, nrf_coresight_init, NULL, NULL, &nrf_coresight_cfg_##inst,      \
			      POST_KERNEL, NRF_CORESIGHT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NRF_CORESIGHT_INST)
