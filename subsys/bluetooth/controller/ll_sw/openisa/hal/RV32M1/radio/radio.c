/*
 * Copyright (c) 2016 - 2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019 - 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/toolchain.h>
#include <zephyr/irq.h>
#include <errno.h>

#include "util/mem.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll_sw/pdu.h"

#include "fsl_xcvr.h"
#include "hal/cntr.h"
#include "hal/ticker.h"
#include "hal/swi.h"
#include "fsl_cau3_ble.h"	/* must be after irq.h */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_openisa_radio
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static radio_isr_cb_t isr_cb;
static void *isr_cb_param;

#define RADIO_AESCCM_HDR_MASK 0xE3 /* AES-CCM: NESN, SN, MD bits masked to 0 */
#define RADIO_PDU_LEN_MAX (BIT(8) - 1)
#define BYTES_TO_USEC(bytes, bits_per_usec)	\
		((bytes) * 8 >> (__builtin_ffs(bits_per_usec) - 1))

/* us values */
#define MIN_CMD_TIME 10		/* Minimum interval for a delayed radio cmd */
#define RX_MARGIN 8
#define TX_MARGIN 0
#define RX_WTMRK 5		/* (AA + PDU header) - 1 */
#define AA_OVHD_1MBPS 27	/* AA playback overhead for 1 Mbps PHY */
#define AA_OVHD_2MBPS 10	/* AA playback overhead for 2 Mbps PHY */
#define RX_OVHD 32		/* Rx overhead */

#define PB_RX 544	/* half of PB (packet buffer) */

/* The PDU in packet buffer starts after the Access Address which is 4 octets */
#define PB_RX_PDU (PB_RX + 2)	/* Rx PDU offset (in halfwords) in PB */
#define PB_TX_PDU 2		/* Tx PDU offset (in halfwords) in packet
				 * buffer
				 */

#define RADIO_ACTIVE_MASK 0x1fff
#define RADIO_DISABLE_TMR 4		/* us */

/* Delay needed in order to enter Manual DSM.
 * Must be at least 4 ticks ahead of DSM_TIMER (from RM).
 */
#define DSM_ENTER_DELAY_TICKS 6

/* Delay needed in order to exit Manual DSM.
 * Should be after radio_tmr_start() (2 ticks after lll_clk_on()).
 * But less that 1.5ms (EVENT_OVERHEAD_XTAL_US) (ULL to LLL time offset).
 * Must be at least 4 ticks ahead of DSM_TIMER (from RM).
 */
#define DSM_EXIT_DELAY_TICKS 30

/* Mask to determine the state of DSM machine */
#define MAN_DSM_ON (RSIM_DSM_CONTROL_MAN_DEEP_SLEEP_STATUS_MASK \
		| RSIM_DSM_CONTROL_DSM_MAN_READY_MASK \
		| RSIM_DSM_CONTROL_MAN_SLEEP_REQUEST_MASK) \

static uint32_t dsm_ref; /* DSM reference counter */

static uint8_t delayed_radio_start;
static uint8_t delayed_trx;
static uint32_t delayed_ticks_start;
static uint32_t delayed_remainder;
static uint8_t delayed_radio_stop;
static uint32_t delayed_hcto;

static uint32_t rtc_start;
static uint32_t rtc_diff_start_us;

static uint32_t tmr_aa;		/* AA (Access Address) timestamp saved value */
static uint32_t tmr_aa_save;	/* save AA timestamp */
static uint32_t tmr_ready;		/* radio ready for Tx/Rx timestamp */
static uint32_t tmr_end;		/* Tx/Rx end timestamp saved value */
static uint32_t tmr_end_save;	/* save Tx/Rx end timestamp */
static uint32_t tmr_tifs;

static uint32_t rx_wu;
static uint32_t tx_wu;

static uint8_t phy_mode;		/* Current PHY mode (DR_1MBPS or DR_2MBPS) */
static uint8_t bits_per_usec;	/* This saves the # of bits per usec,
				 * depending on the PHY mode
				 */
static uint8_t phy_aa_ovhd;	/* This saves the AA overhead, depending on the
				 * PHY mode
				 */

static uint32_t isr_tmr_aa;
static uint32_t isr_tmr_end;
static uint32_t isr_latency;
static uint32_t next_wu;
static uint32_t next_radio_cmd;

static uint32_t radio_trx;
static uint32_t force_bad_crc;
static uint32_t skip_hcto;

static uint8_t *rx_pkt_ptr;
static uint32_t payload_max_size;

static uint8_t MALIGN(4) _pkt_empty[PDU_EM_LL_SIZE_MAX];
static uint8_t MALIGN(4) _pkt_scratch[
			((RADIO_PDU_LEN_MAX + 3) > PDU_AC_LL_SIZE_MAX) ?
			(RADIO_PDU_LEN_MAX + 3) : PDU_AC_LL_SIZE_MAX];

static int8_t rssi;

static struct {
	union {
		uint64_t counter;
		uint8_t bytes[CAU3_AES_BLOCK_SIZE - 1 - 2];
	} nonce;	/* used by the B0 format but not in-situ */
	struct pdu_data *rx_pkt_out;
	struct pdu_data *rx_pkt_in;
	uint8_t auth_mic_valid;
	uint8_t empty_pdu_rxed;
} ctx_ccm;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#define RPA_NO_IRK_MATCH 0xFF	/* No IRK match in AR table */

static struct {
	uint8_t ar_enable;
	uint32_t irk_idx;
} radio_ar_ctx = {0U, RPA_NO_IRK_MATCH};
#endif /* CONFIG_BT_CTLR_PRIVACY */

static void tmp_cb(void *param)
{
	uint32_t tmr = GENFSK->EVENT_TMR & GENFSK_EVENT_TMR_EVENT_TMR_MASK;
	uint32_t t2 = GENFSK->T2_CMP & GENFSK_T2_CMP_T2_CMP_MASK;

	isr_latency = (tmr - t2) & GENFSK_EVENT_TMR_EVENT_TMR_MASK; /* 24bit */
	/* Mark as done */
	*(uint32_t *)param = 1;
}

static void get_isr_latency(void)
{
	volatile uint32_t tmp = 0;

	radio_isr_set(tmp_cb, (void *)&tmp);

	/* Reset TMR to zero */
	GENFSK->EVENT_TMR = 0x1000000;

	radio_disable();
	while (!tmp) {
	}
	irq_disable(LL_RADIO_IRQn_2nd_lvl);
}

static uint32_t radio_tmr_start_hlp(uint8_t trx, uint32_t ticks_start, uint32_t remainder);
static void radio_tmr_hcto_configure_hlp(uint32_t hcto);
static void radio_config_after_wake(void)
{
	if (!delayed_radio_start) {
		delayed_radio_stop = 0;
		return;
	}

	delayed_radio_start = 0;
	radio_tmr_start_hlp(delayed_trx, delayed_ticks_start,
				delayed_remainder);

	if (delayed_radio_stop) {
		delayed_radio_stop = 0;

		/* Adjust time out as remainder was in radio_tmr_start_hlp() */
		delayed_hcto += rtc_diff_start_us;
		radio_tmr_hcto_configure_hlp(delayed_hcto);
	}
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
static void ar_execute(void *pkt)
{
	struct pdu_adv *pdu_adv = (struct pdu_adv *)pkt;
	bt_addr_t *rpa = (bt_addr_t *)&pdu_adv->payload[0];

	/* Perform address resolution when TxAdd=1 and address is resolvable */
	if (pdu_adv->tx_addr && BT_ADDR_IS_RPA(rpa)) {
		uint32_t *hash, *prand;
		status_t status;

		/* Use pointers to avoid breaking strict aliasing */
		hash = (uint32_t *)(&rpa->val[0]);
		prand = (uint32_t *)(&rpa->val[3]);

		/* CAUv3 needs hash & prand in le format, right-justified */
		status = CAU3_RPAtableSearch(CAU3, (*prand & 0xFFFFFF),
					(*hash & 0xFFFFFF),
					&radio_ar_ctx.irk_idx,
					kCAU3_TaskDoneEvent);
		if (status != kStatus_Success) {
			radio_ar_ctx.irk_idx = RPA_NO_IRK_MATCH;
			BT_ERR("CAUv3 RPA table search failed %d", status);
			return;
		}
	}

	radio_ar_ctx.ar_enable = 0U;
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

static void pkt_rx(void)
{
	uint32_t len, idx;
	uint16_t *rxb = (uint16_t *)rx_pkt_ptr, tmp;
	volatile uint16_t *pb = &GENFSK->PACKET_BUFFER[PB_RX_PDU];
	volatile const uint32_t *sts = &GENFSK->XCVR_STS;

	/* payload length */
	len = (GENFSK->XCVR_CTRL & GENFSK_XCVR_CTRL_LENGTH_EXT_MASK) >>
			GENFSK_XCVR_CTRL_LENGTH_EXT_SHIFT;

	if (len > payload_max_size) {
		/* Unexpected size */
		force_bad_crc = 1;
		next_radio_cmd = 0;
		while (*sts & GENFSK_XCVR_STS_RX_IN_PROGRESS_MASK) {
		}
		return;
	}

	/* For Data Physical Channel PDU,
	 * assume no CTEInfo field, CP=0 => 2 bytes header
	 */
	/* Add PDU header size */
	len += 2;

	/* Add to AA time, PDU + CRC time */
	isr_tmr_end = isr_tmr_aa + BYTES_TO_USEC(len + 3, bits_per_usec);

	/* If not enough time for warmup after we copy the PDU from
	 * packet buffer, send delayed command now
	 */
	if (next_radio_cmd) {
		/* Start Rx/Tx in TIFS */
		idx = isr_tmr_end + tmr_tifs - next_wu;
		GENFSK->XCVR_CTRL = next_radio_cmd;
		GENFSK->T1_CMP = GENFSK_T1_CMP_T1_CMP(idx) |
				 GENFSK_T1_CMP_T1_CMP_EN(1);
		next_radio_cmd = 0;
	}

	/* Can't rely on data read from packet buffer while in Rx */
	/* Wait for Rx to finish */
	while (*sts & GENFSK_XCVR_STS_RX_IN_PROGRESS_MASK) {
	}

	if (ctx_ccm.rx_pkt_out) {
		*(uint16_t *)ctx_ccm.rx_pkt_out = pb[0];
		if (len < CAU3_BLE_MIC_SIZE) {
			ctx_ccm.rx_pkt_out = 0;
			ctx_ccm.rx_pkt_in = 0;
			ctx_ccm.empty_pdu_rxed = 1;
		}
	}

	/* Copy the PDU */
	for (idx = 0; idx < len / 2; idx++) {
		rxb[idx] = pb[idx];
	}

	/* Copy last byte */
	if (len & 0x1) {
		tmp = pb[len / 2];
		rx_pkt_ptr[len - 1] =  ((uint8_t *)&tmp)[0];
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Perform address resolution on this Rx */
	if (radio_ar_ctx.ar_enable) {
		ar_execute(rxb);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	force_bad_crc = 0;
}

#define IRQ_MASK ~(GENFSK_IRQ_CTRL_T2_IRQ_MASK | \
		   GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_MASK | \
		   GENFSK_IRQ_CTRL_TX_IRQ_MASK | \
		   GENFSK_IRQ_CTRL_WAKE_IRQ_MASK)
void isr_radio(void *arg)
{
	ARG_UNUSED(arg);

	uint32_t tmr = GENFSK->EVENT_TMR & GENFSK_EVENT_TMR_EVENT_TMR_MASK;
	uint32_t irq = GENFSK->IRQ_CTRL;
	uint32_t valid = 0;
	/* We need to check for a valid IRQ source.
	 * In theory, we could get to this ISR after the IRQ source was cleared.
	 * This could happen due to the way LLL interacts with IRQs
	 * (radio_isr_set(), radio_disable()) and their propagation path:
	 * GENFSK -> INTMUX(does not latch pending source interrupts)
	 * INTMUX -> EVENT_UNIT
	 */

	if (irq & GENFSK_IRQ_CTRL_WAKE_IRQ_MASK) {
		/* Clear pending interrupts */
		GENFSK->IRQ_CTRL &= 0xffffffff;

		/* Disable DSM_TIMER */
		RSIM->DSM_CONTROL &= ~RSIM_DSM_CONTROL_DSM_TIMER_EN(1);

		radio_config_after_wake();
		return;
	}

	if (irq & GENFSK_IRQ_CTRL_TX_IRQ_MASK) {
		valid = 1;
		GENFSK->IRQ_CTRL &= (IRQ_MASK | GENFSK_IRQ_CTRL_TX_IRQ_MASK);
		GENFSK->T1_CMP &= ~GENFSK_T1_CMP_T1_CMP_EN_MASK;

		isr_tmr_end = tmr - isr_latency;
		if (tmr_end_save) {
			tmr_end = isr_tmr_end;
		}
		radio_trx = 1;
	}

	if (irq & GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_MASK) {
		valid = 1;
		/* Disable Rx timeout */
		/* 0b1010..RX Cancel -- Cancels pending RX events but do not
		 *			abort a RX-in-progress
		 */
		GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0xa);
		GENFSK->T2_CMP &= ~GENFSK_T2_CMP_T2_CMP_EN_MASK;

		GENFSK->IRQ_CTRL &= (IRQ_MASK |
				     GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_MASK);
		GENFSK->T1_CMP &= ~GENFSK_T1_CMP_T1_CMP_EN_MASK;

		/* Fix reported AA time */
		isr_tmr_aa = GENFSK->TIMESTAMP - phy_aa_ovhd;

		if (tmr_aa_save) {
			tmr_aa = isr_tmr_aa;
		}
		/* Copy the PDU as it arrives, calculates Rx end */
		pkt_rx();
		if (tmr_end_save) {
			tmr_end = isr_tmr_end;	/* from pkt_rx() */
		}
		radio_trx = 1;
		rssi = (GENFSK->XCVR_STS & GENFSK_XCVR_STS_RSSI_MASK) >>
					GENFSK_XCVR_STS_RSSI_SHIFT;
	}

	if (irq & GENFSK_IRQ_CTRL_T2_IRQ_MASK) {
		valid = 1;
		GENFSK->IRQ_CTRL &= (IRQ_MASK | GENFSK_IRQ_CTRL_T2_IRQ_MASK);
		/* Disable both comparators */
		GENFSK->T1_CMP &= ~GENFSK_T1_CMP_T1_CMP_EN_MASK;
		GENFSK->T2_CMP &= ~GENFSK_T2_CMP_T2_CMP_EN_MASK;
	}

	if (radio_trx && next_radio_cmd) {
		/* Start Rx/Tx in TIFS */
		tmr = isr_tmr_end + tmr_tifs - next_wu;
		GENFSK->XCVR_CTRL = next_radio_cmd;
		GENFSK->T1_CMP = GENFSK_T1_CMP_T1_CMP(tmr) |
				 GENFSK_T1_CMP_T1_CMP_EN(1);
		next_radio_cmd = 0;
	}

	if (valid) {
		isr_cb(isr_cb_param);
	}
}

void radio_isr_set(radio_isr_cb_t cb, void *param)
{
	irq_disable(LL_RADIO_IRQn_2nd_lvl);
	irq_disable(LL_RADIO_IRQn);

	isr_cb_param = param;
	isr_cb = cb;

	/* Clear pending interrupts */
	GENFSK->IRQ_CTRL &= 0xffffffff;
	EVENT_UNIT->INTPTPENDCLEAR = (uint32_t)(1U << LL_RADIO_IRQn);

	irq_enable(LL_RADIO_IRQn);
	irq_enable(LL_RADIO_IRQn_2nd_lvl);
}

#define DISABLE_HPMCAL

#ifdef DISABLE_HPMCAL
#define WU_OPTIM            26  /* 34: quite ok, 36 few ok */
#define USE_FIXED_HPMCAL    563

static void hpmcal_disable(void)
{
#ifdef USE_FIXED_HPMCAL
	uint32_t hpmcal = USE_FIXED_HPMCAL;
#else
	uint32_t hpmcal_vals[40];
	uint32_t hpmcal;
	int i;

	GENFSK->TX_POWER = GENFSK_TX_POWER_TX_POWER(1);

	/* TX warm-up at Channel Frequency = 2.44GHz */
	for (i = 0; i < ARRAY_SIZE(hpmcal_vals); i++) {
		GENFSK->CHANNEL_NUM = 2402 - 2360 + 2 * i;

		/* Reset TMR to zero */
		GENFSK->EVENT_TMR = 0x1000000;

		/* TX Start Now */
		GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0x1);

		while ((GENFSK->EVENT_TMR & 0xffffff) < 1000)
			;

		/* Abort All */
		GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0xb);

		/* Wait for XCVR to become idle. */
		while (GENFSK->XCVR_CTRL & GENFSK_XCVR_CTRL_XCVR_BUSY_MASK)
			;

		hpmcal_vals[i] = (XCVR_PLL_DIG->HPMCAL_CTRL &
				XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MASK) >>
				XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_SHIFT;
	}

	hpmcal = hpmcal_vals[20];
#endif

	XCVR_PLL_DIG->HPMCAL_CTRL = (XCVR_PLL_DIG->HPMCAL_CTRL &
			~XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL_MASK) +
			XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL(hpmcal) +
			XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE_MASK +
			0x00000000;

	/* Move the sigma_delta_en signal to be 1us after pll_dig_en */
	int pll_dig_en  = (XCVR_TSM->TIMING34 &
			XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI_MASK) >>
			XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI_SHIFT;

	XCVR_TSM->TIMING38 = (XCVR_TSM->TIMING38 &
			~XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI_MASK) +
			XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI(pll_dig_en+1);
			/* sigma_delta_en */

	XCVR_TSM->TIMING19   -= B1(WU_OPTIM); /* sy_pd_filter_charge_en */
	XCVR_TSM->TIMING24   -= B1(WU_OPTIM); /* sy_divn_cal_en */
	XCVR_TSM->TIMING13   -= B1(WU_OPTIM); /* sy_vco_autotune_en */
	XCVR_TSM->TIMING17   -= B0(WU_OPTIM); /* sy_lo_tx_buf_en */
	XCVR_TSM->TIMING26   -= B0(WU_OPTIM); /* tx_pa_en */
	XCVR_TSM->TIMING35   -= B0(WU_OPTIM); /* tx_dig_en */
	XCVR_TSM->TIMING14   -= B0(WU_OPTIM); /* sy_pd_cycle_slip_ld_ft_en */

	XCVR_TSM->END_OF_SEQ -= B1(WU_OPTIM) + B0(WU_OPTIM);
}
#endif

void radio_setup(void)
{
	XCVR_Reset();

#if defined(CONFIG_BT_CTLR_PRIVACY) || defined(CONFIG_BT_CTLR_LE_ENC)
	CAU3_Init(CAU3);
#endif /* CONFIG_BT_CTLR_PRIVACY || CONFIG_BT_CTLR_LE_ENC */

	XCVR_Init(GFSK_BT_0p5_h_0p5, DR_1MBPS);
	XCVR_SetXtalTrim(41);

#ifdef DISABLE_HPMCAL
	hpmcal_disable();
#endif

	/* Enable Deep Sleep Mode for GENFSK */
	XCVR_MISC->XCVR_CTRL |= XCVR_CTRL_XCVR_CTRL_MAN_DSM_SEL(2);

	/* Enable the CRC as it is disabled by default after reset */
	XCVR_MISC->CRCW_CFG |= XCVR_CTRL_CRCW_CFG_CRCW_EN(1);

	/* Assign Radio #0 Interrupt to GENERIC_FSK */
	XCVR_MISC->XCVR_CTRL |= XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL(3);

	phy_mode = GENFSK->BITRATE = DR_1MBPS;
	bits_per_usec = 1;
	phy_aa_ovhd = AA_OVHD_1MBPS;

	/*
	 * Split the buffer in equal parts: first half for Tx,
	 * second half for Rx
	 */
	GENFSK->PB_PARTITION = GENFSK_PB_PARTITION_PB_PARTITION(PB_RX);

	/* Get warmup times. They are used in TIFS calculations */
	rx_wu = (XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK)
				>> XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;
	tx_wu = (XCVR_TSM->END_OF_SEQ & XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK)
				>> XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT;

	/* IRQ config, clear pending interrupts */
	irq_disable(LL_RADIO_IRQn_2nd_lvl);
	GENFSK->IRQ_CTRL = 0xffffffff;
	GENFSK->IRQ_CTRL = GENFSK_IRQ_CTRL_GENERIC_FSK_IRQ_EN(1) |
			   GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_EN(1) |
			   GENFSK_IRQ_CTRL_TX_IRQ_EN(1) |
			   GENFSK_IRQ_CTRL_T2_IRQ_EN(1) |
			   GENFSK_IRQ_CTRL_WAKE_IRQ_EN(1);

	/* Disable Rx recycle */
	GENFSK->IRQ_CTRL |= GENFSK_IRQ_CTRL_CRC_IGNORE(1);
	GENFSK->WHITEN_SZ_THR |= GENFSK_WHITEN_SZ_THR_REC_BAD_PKT(1);

	/* Turn radio on to measure ISR latency */
	if (radio_is_off()) {
		dsm_ref = 0;
		radio_wake();
		while (radio_is_off()) {
		}
	} else {
		dsm_ref = 1;
	}

	get_isr_latency();

	/* Turn radio off */
	radio_sleep();
}

void radio_reset(void)
{
	irq_disable(LL_RADIO_IRQn_2nd_lvl);
	/* Vega radio is never disabled therefore doesn't require resetting */
}

void radio_phy_set(uint8_t phy, uint8_t flags)
{
	int err = 0;
	ARG_UNUSED(flags);

	/* This function sets one of two modes:
	 * - BLE 1 Mbps
	 * - BLE 2 Mbps
	 * Coded BLE is not supported by VEGA
	 */
	switch (phy) {
	case BIT(0):
	default:
		if (phy_mode == DR_1MBPS) {
			break;
		}

		err = XCVR_ChangeMode(GFSK_BT_0p5_h_0p5, DR_1MBPS);
		if (err) {
			BT_ERR("Failed to change PHY to 1 Mbps");
			BT_ASSERT(0);
		}

		phy_mode = GENFSK->BITRATE = DR_1MBPS;
		bits_per_usec = 1;
		phy_aa_ovhd = AA_OVHD_1MBPS;

		break;

	case BIT(1):
		if (phy_mode == DR_2MBPS) {
			break;
		}

		err = XCVR_ChangeMode(GFSK_BT_0p5_h_0p5, DR_2MBPS);
		if (err) {
			BT_ERR("Failed to change PHY to 2 Mbps");
			BT_ASSERT(0);
		}

		phy_mode = GENFSK->BITRATE = DR_2MBPS;
		bits_per_usec = 2;
		phy_aa_ovhd = AA_OVHD_2MBPS;

		break;
	}
}

void radio_tx_power_set(uint32_t power)
{
	ARG_UNUSED(power);

	/* tx_power_level should only have the values 1, 2, 4, 6, ... , 62.
	 * Any value odd value (1, 3, ...) is not permitted and will result in
	 * undefined behavior.
	 * Those value have an associated db level that was not found in the
	 * documentation.
	 * Because of these inconsistencies for the moment this
	 * function sets the power level to a known value.
	 */
	uint32_t tx_power_level = 62;

	GENFSK->TX_POWER = GENFSK_TX_POWER_TX_POWER(tx_power_level);
}

void radio_tx_power_max_set(void)
{
	printk("%s\n", __func__);
}

void radio_freq_chan_set(uint32_t chan)
{
	/*
	 * The channel number for vega radio is computed
	 * as 2360 + ch_num [in MHz]
	 * The LLL expects the channel number to be computed as 2400 + ch_num.
	 * Therefore a compensation of 40 MHz has been provided.
	 */
	GENFSK->CHANNEL_NUM = GENFSK_CHANNEL_NUM_CHANNEL_NUM(40 + chan);
}

#define GENFSK_BLE_WHITEN_START			1	/* after H0 */
#define GENFSK_BLE_WHITEN_END			1	/* at the end of CRC */
#define GENFSK_BLE_WHITEN_POLY_TYPE		0	/* Galois poly type */
#define GENFSK_BLE_WHITEN_SIZE			7	/* poly order */
#define GENFSK_BLE_WHITEN_POLY			0x04

void radio_whiten_iv_set(uint32_t iv)
{
	GENFSK->WHITEN_CFG &= ~(GENFSK_WHITEN_CFG_WHITEN_START_MASK |
				GENFSK_WHITEN_CFG_WHITEN_END_MASK |
				GENFSK_WHITEN_CFG_WHITEN_B4_CRC_MASK |
				GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE_MASK |
				GENFSK_WHITEN_CFG_WHITEN_REF_IN_MASK |
				GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT_MASK |
				GENFSK_WHITEN_CFG_WHITEN_SIZE_MASK |
				GENFSK_WHITEN_CFG_MANCHESTER_EN_MASK |
				GENFSK_WHITEN_CFG_MANCHESTER_INV_MASK |
				GENFSK_WHITEN_CFG_MANCHESTER_START_MASK |
				GENFSK_WHITEN_CFG_WHITEN_INIT_MASK);

	GENFSK->WHITEN_CFG |=
	    GENFSK_WHITEN_CFG_WHITEN_START(GENFSK_BLE_WHITEN_START) |
	    GENFSK_WHITEN_CFG_WHITEN_END(GENFSK_BLE_WHITEN_END) |
	    GENFSK_WHITEN_CFG_WHITEN_B4_CRC(0) |
	    GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE(GENFSK_BLE_WHITEN_POLY_TYPE) |
	    GENFSK_WHITEN_CFG_WHITEN_REF_IN(0) |
	    GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT(0) |
	    GENFSK_WHITEN_CFG_WHITEN_SIZE(GENFSK_BLE_WHITEN_SIZE) |
	    GENFSK_WHITEN_CFG_MANCHESTER_EN(0) |
	    GENFSK_WHITEN_CFG_MANCHESTER_INV(0) |
	    GENFSK_WHITEN_CFG_MANCHESTER_START(0) |
	    GENFSK_WHITEN_CFG_WHITEN_INIT(iv | 0x40);

	GENFSK->WHITEN_POLY =
		GENFSK_WHITEN_POLY_WHITEN_POLY(GENFSK_BLE_WHITEN_POLY);

	GENFSK->WHITEN_SZ_THR &= ~GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR_MASK;
	GENFSK->WHITEN_SZ_THR |= GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR(0);
}

void radio_aa_set(uint8_t *aa)
{
	/* Configure Access Address detection using NETWORK ADDRESS 0 */
	GENFSK->NTW_ADR_0 = *((uint32_t *)aa);
	GENFSK->NTW_ADR_CTRL &= ~(GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ_MASK |
				  GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0_MASK);
	GENFSK->NTW_ADR_CTRL |= GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ(3) |
				GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0(0);

	GENFSK->NTW_ADR_CTRL |= (uint32_t) ((1 << 0) <<
			GENFSK_NTW_ADR_CTRL_NTW_ADR_EN_SHIFT);

	/*
	 * The Access Address must be written in the packet buffer
	 * in front of the PDU
	 */
	GENFSK->PACKET_BUFFER[0] = (aa[1] << 8) + aa[0];
	GENFSK->PACKET_BUFFER[1] = (aa[3] << 8) + aa[2];
}

#define GENFSK_BLE_CRC_SZ	3 /* 3 bytes */
#define GENFSK_BLE_PREAMBLE_SZ	0 /* 1 byte of preamble, depends on PHY type */
#define GENFSK_BLE_LEN_BIT_ORD	0 /* LSB */
#define GENFSK_BLE_SYNC_ADDR_SZ	3 /* 4 bytes, Access Address */
#define GENFSK_BLE_LEN_ADJ_SZ	GENFSK_BLE_CRC_SZ /* adjust length with CRC
						   * size
						   */
#define GENFSK_BLE_H0_SZ	8 /* 8 bits */

void radio_pkt_configure(uint8_t bits_len, uint8_t max_len, uint8_t flags)
{
	ARG_UNUSED(flags);

	payload_max_size = max_len;

	GENFSK->XCVR_CFG &= ~GENFSK_XCVR_CFG_PREAMBLE_SZ_MASK;
	GENFSK->XCVR_CFG |=
		GENFSK_XCVR_CFG_PREAMBLE_SZ(GENFSK_BLE_PREAMBLE_SZ);

	GENFSK->PACKET_CFG &= ~(GENFSK_PACKET_CFG_LENGTH_SZ_MASK |
			GENFSK_PACKET_CFG_LENGTH_BIT_ORD_MASK |
			GENFSK_PACKET_CFG_SYNC_ADDR_SZ_MASK |
			GENFSK_PACKET_CFG_LENGTH_ADJ_MASK |
			GENFSK_PACKET_CFG_H0_SZ_MASK |
			GENFSK_PACKET_CFG_H1_SZ_MASK);

	GENFSK->PACKET_CFG |= GENFSK_PACKET_CFG_LENGTH_SZ(bits_len) |
		GENFSK_PACKET_CFG_LENGTH_BIT_ORD(GENFSK_BLE_LEN_BIT_ORD) |
		GENFSK_PACKET_CFG_SYNC_ADDR_SZ(GENFSK_BLE_SYNC_ADDR_SZ) |
		GENFSK_PACKET_CFG_LENGTH_ADJ(GENFSK_BLE_LEN_ADJ_SZ) |
		GENFSK_PACKET_CFG_H0_SZ(GENFSK_BLE_H0_SZ) |
		GENFSK_PACKET_CFG_H1_SZ((8 - bits_len));

	GENFSK->H0_CFG &= ~(GENFSK_H0_CFG_H0_MASK_MASK |
			    GENFSK_H0_CFG_H0_MATCH_MASK);
	GENFSK->H0_CFG |= GENFSK_H0_CFG_H0_MASK(0) |
			  GENFSK_H0_CFG_H0_MATCH(0);

	GENFSK->H1_CFG &= ~(GENFSK_H1_CFG_H1_MASK_MASK |
			    GENFSK_H1_CFG_H1_MATCH_MASK);
	GENFSK->H1_CFG |= GENFSK_H1_CFG_H1_MASK(0) |
			  GENFSK_H1_CFG_H1_MATCH(0);

	/* set Rx watermark to AA + PDU header */
	GENFSK->RX_WATERMARK = GENFSK_RX_WATERMARK_RX_WATERMARK(RX_WTMRK);
}

void radio_pkt_rx_set(void *rx_packet)
{
	rx_pkt_ptr = rx_packet;
}

void radio_pkt_tx_set(void *tx_packet)
{
	/*
	 * The GENERIC_FSK software must program the TX buffer
	 * before commanding a TX operation, and must not access the RAM during
	 * the transmission.
	 */
	uint16_t *pkt = tx_packet;
	uint32_t cnt = 0, pkt_len = (((uint8_t *)tx_packet)[1] + 1) / 2 + 1;
	volatile uint16_t *pkt_buffer = &GENFSK->PACKET_BUFFER[PB_TX_PDU];

	for (; cnt < pkt_len; cnt++) {
		pkt_buffer[cnt] = pkt[cnt];
	}
}

uint32_t radio_tx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return tx_wu;
}

uint32_t radio_tx_chain_delay_get(uint8_t phy, uint8_t flags)
{
	return 0;
}

uint32_t radio_rx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return rx_wu;
}

uint32_t radio_rx_chain_delay_get(uint8_t phy, uint8_t flags)
{
	/* RX_WTMRK = AA + PDU header, but AA time is already accounted for */
	/* PDU header (assume 2 bytes) => 16us, depends on PHY type */
	/* 2 * RX_OVHD = RX_WATERMARK_IRQ time - TIMESTAMP - isr_latency */
	/* The rest is Rx margin that for now isn't well defined */
	return BYTES_TO_USEC(2, bits_per_usec) + 2 * RX_OVHD +
					RX_MARGIN + isr_latency +
					RX_OVHD;
}

void radio_rx_enable(void)
{
	/* Wait for idle state */
	while (GENFSK->XCVR_STS & RADIO_ACTIVE_MASK) {
	}

	/* 0b0101..RX Start Now */
	GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0x5);
}

void radio_tx_enable(void)
{
	/* Wait for idle state */
	while (GENFSK->XCVR_STS & RADIO_ACTIVE_MASK) {
	}

	/* 0b0001..TX Start Now */
	GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0x1);
}

void radio_disable(void)
{
	/* Disable both comparators */
	GENFSK->T1_CMP = 0;
	GENFSK->T2_CMP = 0;

	/*
	 * 0b1011..Abort All - Cancels all pending events and abort any
	 * sequence-in-progress
	 */
	GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0xb);

	/* Wait for idle state */
	while (GENFSK->XCVR_STS & RADIO_ACTIVE_MASK) {
	}

	/* Clear pending interrupts */
	GENFSK->IRQ_CTRL &= 0xffffffff;
	EVENT_UNIT->INTPTPENDCLEAR = (uint32_t)(1U << LL_RADIO_IRQn);

	next_radio_cmd = 0;
	radio_trx = 0;

	/* generate T2 interrupt to get into isr_radio() */
	uint32_t tmr = GENFSK->EVENT_TMR + RADIO_DISABLE_TMR;

	GENFSK->T2_CMP = GENFSK_T2_CMP_T2_CMP(tmr) | GENFSK_T2_CMP_T2_CMP_EN(1);
}

void radio_status_reset(void)
{
	radio_trx = 0;
}

uint32_t radio_is_ready(void)
{
	/* Always false. LLL expects the radio not to be in idle/Tx/Rx state */
	return 0;
}

uint32_t radio_is_done(void)
{
	return radio_trx;
}

uint32_t radio_has_disabled(void)
{
	/* Not used */
	return 0;
}

uint32_t radio_is_idle(void)
{
	/* Vega radio is never disabled */
	return 1;
}

#define GENFSK_BLE_CRC_START_BYTE	4 /* After Access Address */
#define GENFSK_BLE_CRC_BYTE_ORD		0 /* LSB */

void radio_crc_configure(uint32_t polynomial, uint32_t iv)
{
	/* printk("%s poly: %08x, iv: %08x\n", __func__, polynomial, iv); */

	GENFSK->CRC_CFG &= ~(GENFSK_CRC_CFG_CRC_SZ_MASK |
			     GENFSK_CRC_CFG_CRC_START_BYTE_MASK |
			     GENFSK_CRC_CFG_CRC_REF_IN_MASK |
			     GENFSK_CRC_CFG_CRC_REF_OUT_MASK |
			     GENFSK_CRC_CFG_CRC_BYTE_ORD_MASK);
	GENFSK->CRC_CFG |= GENFSK_CRC_CFG_CRC_SZ(GENFSK_BLE_CRC_SZ) |
		GENFSK_CRC_CFG_CRC_START_BYTE(GENFSK_BLE_CRC_START_BYTE) |
		GENFSK_CRC_CFG_CRC_REF_IN(0) |
		GENFSK_CRC_CFG_CRC_REF_OUT(0) |
		GENFSK_CRC_CFG_CRC_BYTE_ORD(GENFSK_BLE_CRC_BYTE_ORD);

	GENFSK->CRC_INIT = (iv << ((4U - GENFSK_BLE_CRC_SZ) << 3));
	GENFSK->CRC_POLY = (polynomial << ((4U - GENFSK_BLE_CRC_SZ) << 3));
	GENFSK->CRC_XOR_OUT = 0;

	/*
	 * Enable CRC in hardware; this is already done at startup but we do it
	 * here anyways just to be sure.
	 */
	GENFSK->XCVR_CFG &= ~GENFSK_XCVR_CFG_SW_CRC_EN_MASK;
}

uint32_t radio_crc_is_valid(void)
{
	if (force_bad_crc)
		return 0;

	uint32_t radio_crc = (GENFSK->XCVR_STS & GENFSK_XCVR_STS_CRC_VALID_MASK) >>
						GENFSK_XCVR_STS_CRC_VALID_SHIFT;
	return radio_crc;
}

void *radio_pkt_empty_get(void)
{
	return _pkt_empty;
}

void *radio_pkt_scratch_get(void)
{
	return _pkt_scratch;
}

void radio_switch_complete_and_rx(uint8_t phy_rx)
{
	/*  0b0110..RX Start @ T1 Timer Compare Match (EVENT_TMR = T1_CMP) */
	next_radio_cmd = GENFSK_XCVR_CTRL_SEQCMD(0x6);

	/* the margin is used to account for any overhead in radio switching */
	next_wu = rx_wu + RX_MARGIN;
}

void radio_switch_complete_and_tx(uint8_t phy_rx, uint8_t flags_rx, uint8_t phy_tx,
				  uint8_t flags_tx)
{
	/*  0b0010..TX Start @ T1 Timer Compare Match (EVENT_TMR = T1_CMP) */
	next_radio_cmd = GENFSK_XCVR_CTRL_SEQCMD(0x2);

	/* the margin is used to account for any overhead in radio switching */
	next_wu = tx_wu + TX_MARGIN;
}

void radio_switch_complete_and_disable(void)
{
	next_radio_cmd = 0;
}

void radio_rssi_measure(void)
{
	rssi = 0;
}

uint32_t radio_rssi_get(void)
{
	return (uint32_t)-rssi;
}

void radio_rssi_status_reset(void)
{
}

uint32_t radio_rssi_is_ready(void)
{
	return (rssi != 0);
}

void radio_filter_configure(uint8_t bitmask_enable, uint8_t bitmask_addr_type,
			    uint8_t *bdaddr)
{
	/* printk("%s\n", __func__); */
}

void radio_filter_disable(void)
{
	/* Nothing to do here */
}

void radio_filter_status_reset(void)
{
	/* printk("%s\n", __func__); */
}

uint32_t radio_filter_has_match(void)
{
	/* printk("%s\n", __func__); */
	return 0;
}

uint32_t radio_filter_match_get(void)
{
	/* printk("%s\n", __func__); */
	return 0;
}

void radio_bc_configure(uint32_t n)
{
	printk("%s\n", __func__);
}

void radio_bc_status_reset(void)
{
	printk("%s\n", __func__);
}

uint32_t radio_bc_has_match(void)
{
	printk("%s\n", __func__);
	return 0;
}

void radio_tmr_status_reset(void)
{
	tmr_aa_save = 0;
	tmr_end_save = 0;
}

void radio_tmr_tifs_set(uint32_t tifs)
{
	tmr_tifs = tifs;
}

/* Start the radio after ticks_start (ticks) + remainder (us) time */
static uint32_t radio_tmr_start_hlp(uint8_t trx, uint32_t ticks_start, uint32_t remainder)
{
	uint32_t radio_start_now_cmd = 0;

	/* Disable both comparators */
	GENFSK->T1_CMP = 0;
	GENFSK->T2_CMP = 0;

	/* Save it for later */
	rtc_start = ticks_start;

	/* Convert ticks to us and use just EVENT_TMR */
	rtc_diff_start_us = HAL_TICKER_TICKS_TO_US(rtc_start - cntr_cnt_get());

	skip_hcto = 0;
	if (rtc_diff_start_us > GENFSK_T1_CMP_T1_CMP_MASK) {
		/* ticks_start already passed. Don't start the radio */
		rtc_diff_start_us = 0;

		/* Ignore time out as well */
		skip_hcto = 1;
		return remainder;
	}
	remainder += rtc_diff_start_us;

	if (trx) {
		if (remainder <= MIN_CMD_TIME) {
			/* 0b0001..TX Start Now */
			radio_start_now_cmd = GENFSK_XCVR_CTRL_SEQCMD(0x1);
			remainder = 0;
		} else {
			/*
			 * 0b0010..TX Start @ T1 Timer Compare Match
			 *         (EVENT_TMR = T1_CMP)
			 */
			GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0x2);
			GENFSK->T1_CMP = GENFSK_T1_CMP_T1_CMP(remainder);
		}
		tmr_ready = remainder + tx_wu;
	} else {
		if (remainder <= MIN_CMD_TIME) {
			/* 0b0101..RX Start Now */
			radio_start_now_cmd = GENFSK_XCVR_CTRL_SEQCMD(0x5);
			remainder = 0;
		} else {
			/*
			 * 0b0110..RX Start @ T1 Timer Compare Match
			 *         (EVENT_TMR = T1_CMP)
			 */
			GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0x6);
			GENFSK->T1_CMP = GENFSK_T1_CMP_T1_CMP(remainder);
		}
		tmr_ready = remainder + rx_wu;
	}

	/*
	 * reset EVENT_TMR should be done after ticks_start.
	 * We converted ticks to us, so we reset it now.
	 * All tmr_* times are relative to EVENT_TMR reset.
	 * rtc_diff_start_us is used to adjust them
	 */
	GENFSK->EVENT_TMR = GENFSK_EVENT_TMR_EVENT_TMR_LD(1);

	if (radio_start_now_cmd) {
		/* trigger Rx/Tx Start Now */
		GENFSK->XCVR_CTRL = radio_start_now_cmd;
	} else {
		/* enable T1_CMP to trigger the SEQCMD */
		GENFSK->T1_CMP |= GENFSK_T1_CMP_T1_CMP_EN(1);
	}

	return remainder;
}

uint32_t radio_tmr_start(uint8_t trx, uint32_t ticks_start, uint32_t remainder)
{
	if ((!(remainder / 1000000UL)) || (remainder & 0x80000000)) {
		ticks_start--;
		remainder += 30517578UL;
	}
	remainder /= 1000000UL;

	if (radio_is_off()) {
		delayed_radio_start = 1;
		delayed_trx = trx;
		delayed_ticks_start = ticks_start;
		delayed_remainder = remainder;
		return remainder;
	}

	delayed_radio_start = 0;
	return radio_tmr_start_hlp(trx, ticks_start, remainder);
}

uint32_t radio_tmr_start_tick(uint8_t trx, uint32_t tick)
{
	/* Setup compare event with min. 1 us offset */
	uint32_t remainder_us = 1;

	return radio_tmr_start_hlp(trx, tick, remainder_us);
}

void radio_tmr_start_us(uint8_t trx, uint32_t us)
{
	printk("%s\n", __func__);
}

uint32_t radio_tmr_start_now(uint8_t trx)
{
	printk("%s\n", __func__);
	return 0;
}

uint32_t radio_tmr_start_get(void)
{
	return rtc_start;
}

void radio_tmr_stop(void)
{
}

static void radio_tmr_hcto_configure_hlp(uint32_t hcto)
{
	if (skip_hcto) {
		skip_hcto = 0;
		return;
	}

	/* 0b1001..RX Stop @ T2 Timer Compare Match (EVENT_TMR = T2_CMP) */
	GENFSK->XCVR_CTRL = GENFSK_XCVR_CTRL_SEQCMD(0x9);
	GENFSK->T2_CMP = GENFSK_T2_CMP_T2_CMP(hcto) |
			 GENFSK_T2_CMP_T2_CMP_EN(1);

}

/* Header completion time out */
void radio_tmr_hcto_configure(uint32_t hcto)
{
	if (delayed_radio_start) {
		delayed_radio_stop = 1;
		delayed_hcto = hcto;
		return;
	}

	delayed_radio_stop = 0;
	radio_tmr_hcto_configure_hlp(hcto);
}

void radio_tmr_aa_capture(void)
{
	tmr_aa_save = 1;
}

uint32_t radio_tmr_aa_get(void)
{
	return tmr_aa - rtc_diff_start_us;
}

static uint32_t radio_tmr_aa;

void radio_tmr_aa_save(uint32_t aa)
{
	radio_tmr_aa = aa;
}

uint32_t radio_tmr_aa_restore(void)
{
	return radio_tmr_aa;
}

uint32_t radio_tmr_ready_get(void)
{
	return tmr_ready - rtc_diff_start_us;
}

void radio_tmr_end_capture(void)
{
	tmr_end_save = 1;
}

uint32_t radio_tmr_end_get(void)
{
	return tmr_end - rtc_diff_start_us;
}

uint32_t radio_tmr_tifs_base_get(void)
{
	return radio_tmr_end_get() + rtc_diff_start_us;
}

void radio_tmr_sample(void)
{
	printk("%s\n", __func__);
}

uint32_t radio_tmr_sample_get(void)
{
	printk("%s\n", __func__);
	return 0;
}

void *radio_ccm_rx_pkt_set_ut(struct ccm *ccm, uint8_t phy, void *pkt)
{
	/* Saved by LL as MSO to LSO in the ccm->key
	 * SK (LSO to MSO)
	 * :0x66:0xC6:0xC2:0x27:0x8E:0x3B:0x8E:0x05
	 * :0x3E:0x7E:0xA3:0x26:0x52:0x1B:0xAD:0x99
	 */
	uint8_t key_local[16] __aligned(4) = {
		0x99, 0xad, 0x1b, 0x52, 0x26, 0xa3, 0x7e, 0x3e,
		0x05, 0x8e, 0x3b, 0x8e, 0x27, 0xc2, 0xc6, 0x66
	};
	void *result;

	/* ccm.key[16] is stored in MSO format, as retrieved from e function */
	memcpy(ccm->key, key_local, sizeof(key_local));

	/* Input std sample data, vol 6, part C, ch 1 */
	_pkt_scratch[0] = 0x0f;
	_pkt_scratch[1] = 0x05;
	_pkt_scratch[2] = 0x9f; /* cleartext = 0x06*/
	_pkt_scratch[3] = 0xcd;
	_pkt_scratch[4] = 0xa7;
	_pkt_scratch[5] = 0xf4;
	_pkt_scratch[6] = 0x48;

	/* IV std sample data, vol 6, part C, ch 1, stored in LL in LSO format
	 * IV (LSO to MSO)      :0x24:0xAB:0xDC:0xBA:0xBE:0xBA:0xAF:0xDE
	 */
	ccm->iv[0] = 0x24;
	ccm->iv[1] = 0xAB;
	ccm->iv[2] = 0xDC;
	ccm->iv[3] = 0xBA;
	ccm->iv[4] = 0xBE;
	ccm->iv[5] = 0xBA;
	ccm->iv[6] = 0xAF;
	ccm->iv[7] = 0xDE;

	result = radio_ccm_rx_pkt_set(ccm, phy, pkt);
	radio_ccm_is_done();

	if (ctx_ccm.auth_mic_valid == 1 && ((uint8_t *)pkt)[2] == 0x06) {
		BT_INFO("Passed decrypt\n");
	} else {
		BT_INFO("Failed decrypt\n");
	}

	return result;
}

void *radio_ccm_rx_pkt_set(struct ccm *ccm, uint8_t phy, void *pkt)
{
	uint8_t key_local[16] __aligned(4);
	status_t status;
	cau3_handle_t handle = {
			.keySlot = kCAU3_KeySlot2,
			.taskDone = kCAU3_TaskDonePoll
	};
	ARG_UNUSED(phy);

	/* ccm.key[16] is stored in MSO format, as retrieved from e function */
	memcpy(key_local, ccm->key, sizeof(key_local));
	ctx_ccm.auth_mic_valid = 0;
	ctx_ccm.empty_pdu_rxed = 0;
	ctx_ccm.rx_pkt_in = (struct pdu_data *)_pkt_scratch;
	ctx_ccm.rx_pkt_out = (struct pdu_data *)pkt;
	ctx_ccm.nonce.counter = ccm->counter;	/* LSO to MSO, counter is LE */
	/* The directionBit set to 1 for Data Physical Chan PDUs sent by
	 * the central and set to 0 for Data Physical Chan PDUs sent by the
	 * peripheral
	 */
	ctx_ccm.nonce.bytes[4] |= ccm->direction << 7;
	memcpy(&ctx_ccm.nonce.bytes[5], ccm->iv, 8); /* LSO to MSO */

	/* Loads the key into CAU3's DMEM and expands the AES key schedule. */
	status = CAU3_AES_SetKey(CAU3, &handle, key_local, 16);
	if (status != kStatus_Success) {
		BT_ERR("CAUv3 AES key set failed %d", status);
		return NULL;
	}

	return _pkt_scratch;
}

void *radio_ccm_tx_pkt_set_ut(struct ccm *ccm, void *pkt)
{
	/* Clear:
	 * 06 1b 17 00 37 36 35 34 33 32 31 30 41 42 43
	 * 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51
	 */
	uint8_t data_in[29] = {
		0x06, 0x1b, 0x17, 0x00, 0x37, 0x36, 0x35, 0x34,
		0x33, 0x32, 0x31, 0x30, 0x41, 0x42, 0x43, 0x44,
		0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c,
		0x4d, 0x4e, 0x4f, 0x50, 0x51
	};
	/* LL_DATA2:
	 * 06 1f f3 88 81 e7 bd 94 c9 c3 69 b9 a6 68 46
	 * dd 47 86 aa 8c 39 ce 54 0d 0d ae 3a dc df 89 b9 60 88
	 */
	uint8_t data_ref_out[33] = {
		0x06, 0x1f, 0xf3, 0x88, 0x81, 0xe7, 0xbd, 0x94,
		0xc9, 0xc3, 0x69, 0xb9, 0xa6, 0x68, 0x46, 0xdd,
		0x47, 0x86, 0xaa, 0x8c, 0x39, 0xce, 0x54, 0x0d,
		0x0d, 0xae, 0x3a, 0xdc, 0xdf,
		0x89, 0xb9, 0x60, 0x88
	};
	/* Saved by LL as MSO to LSO in the ccm->key
	 * SK (LSO to MSO)
	 * :0x66:0xC6:0xC2:0x27:0x8E:0x3B:0x8E:0x05
	 * :0x3E:0x7E:0xA3:0x26:0x52:0x1B:0xAD:0x99
	 */
	uint8_t key_local[16] __aligned(4) = {
		0x99, 0xad, 0x1b, 0x52, 0x26, 0xa3, 0x7e, 0x3e,
		0x05, 0x8e, 0x3b, 0x8e, 0x27, 0xc2, 0xc6, 0x66
	};
	void *result;

	/* ccm.key[16] is stored in MSO format, as retrieved from e function */
	memcpy(ccm->key, key_local, sizeof(key_local));
	memcpy(pkt, data_in, sizeof(data_in));
	/* IV std sample data, vol 6, part C, ch 1, stored in LL in LSO format
	 * IV (LSO to MSO)      :0x24:0xAB:0xDC:0xBA:0xBE:0xBA:0xAF:0xDE
	 */
	ccm->iv[0] = 0x24;
	ccm->iv[1] = 0xAB;
	ccm->iv[2] = 0xDC;
	ccm->iv[3] = 0xBA;
	ccm->iv[4] = 0xBE;
	ccm->iv[5] = 0xBA;
	ccm->iv[6] = 0xAF;
	ccm->iv[7] = 0xDE;
	/* 4. Data packet2 (packet 1, S --> M) */
	ccm->counter = 1;
	ccm->direction = 0;

	result = radio_ccm_tx_pkt_set(ccm, pkt);

	if (memcmp(result, data_ref_out, sizeof(data_ref_out))) {
		BT_INFO("Failed encrypt\n");
	} else {
		BT_INFO("Passed encrypt\n");
	}

	return result;
}

void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt)
{
	uint8_t key_local[16] __aligned(4);
	uint8_t aad;
	uint8_t *auth_mic;
	status_t status;
	cau3_handle_t handle = {
			.keySlot = kCAU3_KeySlot2,
			.taskDone = kCAU3_TaskDonePoll
	};

	/* Test for Empty PDU and bypass encryption */
	if (((struct pdu_data *)pkt)->len == 0) {
		return pkt;
	}

	/* ccm.key[16] is stored in MSO format, as retrieved from e function */
	memcpy(key_local, ccm->key, sizeof(key_local));
	ctx_ccm.nonce.counter = ccm->counter;	/* LSO to MSO, counter is LE */
	/* The directionBit set to 1 for Data Physical Chan PDUs sent by
	 * the central and set to 0 for Data Physical Chan PDUs sent by the
	 * peripheral
	 */
	ctx_ccm.nonce.bytes[4] |= ccm->direction << 7;
	memcpy(&ctx_ccm.nonce.bytes[5], ccm->iv, 8); /* LSO to MSO */

	/* Loads the key into CAU3's DMEM and expands the AES key schedule. */
	status = CAU3_AES_SetKey(CAU3, &handle, key_local, 16);
	if (status != kStatus_Success) {
		BT_ERR("CAUv3 AES key set failed %d", status);
		return NULL;
	}

	auth_mic = _pkt_scratch + 2 + ((struct pdu_data *)pkt)->len;
	aad = *(uint8_t *)pkt & RADIO_AESCCM_HDR_MASK;

	status = CAU3_AES_CCM_EncryptTag(CAU3, &handle,
				 (uint8_t *)pkt + 2, ((struct pdu_data *)pkt)->len,
				 _pkt_scratch + 2,
				 ctx_ccm.nonce.bytes, 13,
				 &aad, 1, auth_mic, CAU3_BLE_MIC_SIZE);
	if (status != kStatus_Success) {
		BT_ERR("CAUv3 AES CCM decrypt failed %d", status);
		return 0;
	}

	_pkt_scratch[0] = *(uint8_t *)pkt;
	_pkt_scratch[1] = ((struct pdu_data *)pkt)->len + CAU3_BLE_MIC_SIZE;

	return _pkt_scratch;
}

uint32_t radio_ccm_is_done(void)
{
	status_t status;
	uint8_t *auth_mic;
	uint8_t aad;
	cau3_handle_t handle = {
			.keySlot = kCAU3_KeySlot2,
			.taskDone = kCAU3_TaskDonePoll
	};

	if (ctx_ccm.rx_pkt_in->len > CAU3_BLE_MIC_SIZE) {
		auth_mic = (uint8_t *)ctx_ccm.rx_pkt_in + 2 +
				ctx_ccm.rx_pkt_in->len - CAU3_BLE_MIC_SIZE;
		aad = *(uint8_t *)ctx_ccm.rx_pkt_in & RADIO_AESCCM_HDR_MASK;
		status = CAU3_AES_CCM_DecryptTag(CAU3, &handle,
				(uint8_t *)ctx_ccm.rx_pkt_in + 2,
				(uint8_t *)ctx_ccm.rx_pkt_out + 2,
				ctx_ccm.rx_pkt_in->len - CAU3_BLE_MIC_SIZE,
				ctx_ccm.nonce.bytes, 13,
				&aad, 1, auth_mic, CAU3_BLE_MIC_SIZE);
		if (status != kStatus_Success) {
			BT_ERR("CAUv3 AES CCM decrypt failed %d", status);
			return 0;
		}

		ctx_ccm.auth_mic_valid = handle.micPassed;
		ctx_ccm.rx_pkt_out->len -= 4;

	} else if (ctx_ccm.rx_pkt_in->len == 0) {
		/* Just copy input into output */
		*ctx_ccm.rx_pkt_out = *ctx_ccm.rx_pkt_in;
		ctx_ccm.auth_mic_valid = 1;
		ctx_ccm.empty_pdu_rxed = 1;
	} else {
		return 0;   /* length only allowed 0, not 1,2,3,4 */
	}

	return 1;
}

uint32_t radio_ccm_mic_is_valid(void)
{
	return ctx_ccm.auth_mic_valid;
}

uint32_t radio_ccm_is_available(void)
{
	return ctx_ccm.empty_pdu_rxed;
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
void radio_ar_configure(uint32_t nirk, void *irk)
{
	status_t status;
	uint8_t pirk[16];

	/* Initialize CAUv3 RPA table */
	status = CAU3_RPAtableInit(CAU3, kCAU3_TaskDoneEvent);
	if (kStatus_Success != status) {
		BT_ERR("CAUv3 RPA table init failed");
		return;
	}

	/* CAUv3 RPA table is limited to CONFIG_BT_CTLR_RL_SIZE entries */
	if (nirk > CONFIG_BT_CTLR_RL_SIZE) {
		BT_WARN("Max CAUv3 RPA table size is %d",
			CONFIG_BT_CTLR_RL_SIZE);
		nirk = CONFIG_BT_CTLR_RL_SIZE;
	}

	/* Insert RPA keys(IRK) in table */
	for (int i = 0; i < nirk; i++) {
		/* CAUv3 needs IRK in le format */
		sys_memcpy_swap(pirk, (uint8_t *)irk + i * 16, 16);
		status = CAU3_RPAtableInsertKey(CAU3, (uint32_t *)&pirk,
						kCAU3_TaskDoneEvent);
		if (kStatus_Success != status) {
			BT_ERR("CAUv3 RPA table insert failed");
			return;
		}
	}

	/* RPA Table was configured, it can be used for next Rx */
	radio_ar_ctx.ar_enable = 1U;
}

uint32_t radio_ar_match_get(void)
{
	return radio_ar_ctx.irk_idx;
}

void radio_ar_status_reset(void)
{
	radio_ar_ctx.ar_enable = 0U;
	radio_ar_ctx.irk_idx = RPA_NO_IRK_MATCH;
}

uint32_t radio_ar_has_match(void)
{
	return (radio_ar_ctx.irk_idx != RPA_NO_IRK_MATCH);
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

uint32_t radio_sleep(void)
{

	if (dsm_ref == 0) {
		return -EALREADY;
	}

	uint32_t localref = --dsm_ref;
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_HIGH_PRIO) && \
	(CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* TODO:
	 * These operations (decrement, check) should be atomic/critical section
	 * since we turn radio on/off from different contexts/ISRs (LLL, radio).
	 * It's fine for now as all the contexts have the same priority and
	 * don't preempt each other.
	 */
#else
#error "Missing atomic operation in radio_sleep()"
#endif
	if (localref == 0) {

		uint32_t status = (RSIM->DSM_CONTROL & MAN_DSM_ON);

		if (status) {
			/* Already in sleep mode */
			return 0;
		}

		/* Disable DSM_TIMER */
		RSIM->DSM_CONTROL &= ~RSIM_DSM_CONTROL_DSM_TIMER_EN(1);

		/* Get current DSM_TIMER value */
		uint32_t dsm_timer = RSIM->DSM_TIMER;

		/* Set Sleep time after DSM_ENTER_DELAY */
		RSIM->MAN_SLEEP = RSIM_MAN_SLEEP_MAN_SLEEP_TIME(dsm_timer +
				DSM_ENTER_DELAY_TICKS);

		/* Set Wake time to max, we use DSM early exit */
		RSIM->MAN_WAKE = RSIM_MAN_WAKE_MAN_WAKE_TIME(dsm_timer - 1);

		/* MAN wakeup request enable */
		RSIM->DSM_CONTROL |= RSIM_DSM_CONTROL_MAN_WAKEUP_REQUEST_EN(1);

		/* Enable DSM, sending a sleep request */
		GENFSK->DSM_CTRL |= GENFSK_DSM_CTRL_GEN_SLEEP_REQUEST(1);

		/* Enable DSM_TIMER */
		RSIM->DSM_CONTROL |= RSIM_DSM_CONTROL_DSM_TIMER_EN(1);
	}

	return 0;
}

uint32_t radio_wake(void)
{
	uint32_t localref = ++dsm_ref;
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_HIGH_PRIO) && \
	(CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* TODO:
	 * These operations (increment, check) should be atomic/critical section
	 * since we turn radio on/off from different contexts/ISRs (LLL, radio).
	 * It's fine for now as all the contexts have the same priority and
	 * don't preempt each other.
	 */
#else
#error "Missing atomic operation in radio_wake()"
#endif
	if (localref == 1) {

		uint32_t status = (RSIM->DSM_CONTROL & MAN_DSM_ON);

		if (!status) {
			/* Not in sleep mode */
			return 0;
		}

		/* Get current DSM_TIMER value */
		uint32_t dsm_timer = RSIM->DSM_TIMER;

		/* Set Wake time after DSM_ENTER_DELAY */
		RSIM->MAN_WAKE = RSIM_MAN_WAKE_MAN_WAKE_TIME(dsm_timer +
				DSM_EXIT_DELAY_TICKS);
	}

	return 0;
}

uint32_t radio_is_off(void)
{
	uint32_t status = (RSIM->DSM_CONTROL & MAN_DSM_ON);

	return status;
}
