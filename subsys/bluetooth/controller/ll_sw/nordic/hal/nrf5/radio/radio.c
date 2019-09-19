/*
 * Copyright (c) 2016 - 2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/dlist.h>
#include <sys/mempool_base.h>
#include <toolchain.h>

#include "util/mem.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "ll_sw/pdu.h"
#include "radio_nrf5.h"

#include <nrfx/hal/nrf_radio.h>
#include <nrfx/hal/nrf_rtc.h>
#include <nrfx/hal/nrf_ccm.h>

#if defined(CONFIG_SOC_SERIES_NRF51X)
#define RADIO_PDU_LEN_MAX (BIT(5) - 1)
#elif defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define RADIO_PDU_LEN_MAX (BIT(8) - 1)
#else
#error "Platform not defined."
#endif

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
#if ((CONFIG_BT_CTLR_GPIO_PA_PIN) > 31)
#define NRF_GPIO_PA     NRF_P1
#define NRF_GPIO_PA_PIN ((CONFIG_BT_CTLR_GPIO_PA_PIN) - 32)
#else
#define NRF_GPIO_PA     NRF_GPIO
#define NRF_GPIO_PA_PIN CONFIG_BT_CTLR_GPIO_PA_PIN
#endif
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
#if ((CONFIG_BT_CTLR_GPIO_LNA_PIN) > 31)
#define NRF_GPIO_LNA     NRF_P1
#define NRF_GPIO_LNA_PIN ((CONFIG_BT_CTLR_GPIO_LNA_PIN) - 32)
#else
#define NRF_GPIO_LNA     NRF_GPIO
#define NRF_GPIO_LNA_PIN CONFIG_BT_CTLR_GPIO_LNA_PIN
#endif
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

/* The following two constants are used in nrfx_glue.h for marking these PPI
 * channels and groups as occupied and thus unavailable to other modules.
 */
const u32_t z_bt_ctlr_used_nrf_ppi_channels = HAL_USED_PPI_CHANNELS;
const u32_t z_bt_ctlr_used_nrf_ppi_groups   = HAL_USED_PPI_GROUPS;

static radio_isr_cb_t isr_cb;
static void           *isr_cb_param;

void isr_radio(void)
{
	if (radio_has_disabled()) {
		isr_cb(isr_cb_param);
	}
}

void radio_isr_set(radio_isr_cb_t cb, void *param)
{
	irq_disable(RADIO_IRQn);

	isr_cb_param = param;
	isr_cb = cb;

	nrf_radio_int_enable(0 |
				/* RADIO_INTENSET_READY_Msk |
				 * RADIO_INTENSET_ADDRESS_Msk |
				 * RADIO_INTENSET_PAYLOAD_Msk |
				 * RADIO_INTENSET_END_Msk |
				 */
				RADIO_INTENSET_DISABLED_Msk
				/* | RADIO_INTENSET_RSSIEND_Msk |
				 */
	    );

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	irq_enable(RADIO_IRQn);
}

void radio_setup(void)
{
#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	NRF_GPIO_PA->DIRSET = BIT(NRF_GPIO_PA_PIN);
#if defined(CONFIG_BT_CTLR_GPIO_PA_POL_INV)
	NRF_GPIO_PA->OUTSET = BIT(NRF_GPIO_PA_PIN);
#else
	NRF_GPIO_PA->OUTCLR = BIT(NRF_GPIO_PA_PIN);
#endif
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	NRF_GPIO_LNA->DIRSET = BIT(NRF_GPIO_LNA_PIN);

	radio_gpio_lna_off();
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

	hal_radio_ram_prio_setup();
}

void radio_reset(void)
{
	irq_disable(RADIO_IRQn);

	nrf_radio_power_set((RADIO_POWER_POWER_Disabled
			    << RADIO_POWER_POWER_Pos) & RADIO_POWER_POWER_Msk);
	nrf_radio_power_set((RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos)
			    & RADIO_POWER_POWER_Msk);

	hal_radio_reset();

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	hal_radio_sw_switch_ppi_group_setup();
#endif
}

void radio_phy_set(u8_t phy, u8_t flags)
{
	u32_t mode;

	mode = hal_radio_phy_mode_get(phy, flags);

	NRF_RADIO->MODE = (mode << RADIO_MODE_MODE_Pos) & RADIO_MODE_MODE_Msk;

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
	NRF_RADIO->MODECNF0 |= (RADIO_MODECNF0_RU_Fast <<
				RADIO_MODECNF0_RU_Pos) &
			       RADIO_MODECNF0_RU_Msk;
#endif /* CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
}

void radio_tx_power_set(u32_t power)
{
	/* NOTE: valid value range is passed by Kconfig define. */
	NRF_RADIO->TXPOWER = power;
}

void radio_tx_power_max_set(void)
{
	NRF_RADIO->TXPOWER = hal_radio_tx_power_max_get();
}

void radio_freq_chan_set(u32_t chan)
{
	NRF_RADIO->FREQUENCY = chan;
}

void radio_whiten_iv_set(u32_t iv)
{
	NRF_RADIO->DATAWHITEIV = iv;

	NRF_RADIO->PCNF1 &= ~RADIO_PCNF1_WHITEEN_Msk;
	NRF_RADIO->PCNF1 |= ((1UL) << RADIO_PCNF1_WHITEEN_Pos) &
			    RADIO_PCNF1_WHITEEN_Msk;
}

void radio_aa_set(u8_t *aa)
{
	NRF_RADIO->TXADDRESS =
	    (((0UL) << RADIO_TXADDRESS_TXADDRESS_Pos) &
	     RADIO_TXADDRESS_TXADDRESS_Msk);
	NRF_RADIO->RXADDRESSES =
	    ((RADIO_RXADDRESSES_ADDR0_Enabled) << RADIO_RXADDRESSES_ADDR0_Pos);
	NRF_RADIO->PREFIX0 = aa[3];
	NRF_RADIO->BASE0 = (aa[2] << 24) | (aa[1] << 16) | (aa[0] << 8);
}

void radio_pkt_configure(u8_t bits_len, u8_t max_len, u8_t flags)
{
	u8_t dc = flags & 0x01; /* Adv or Data channel */
	u32_t extra;
	u8_t phy;

#if defined(CONFIG_SOC_SERIES_NRF51X)
	ARG_UNUSED(phy);

	extra = 0U;

	/* nRF51 supports only 27 byte PDU when using h/w CCM for encryption. */
	if (!IS_ENABLED(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR) && dc) {
		bits_len = 5U;
	}
#elif defined(CONFIG_SOC_COMPATIBLE_NRF52X)
	extra = 0U;

	phy = (flags >> 1) & 0x07; /* phy */
	switch (phy) {
	case BIT(0):
	default:
		extra |= (RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos) &
			 RADIO_PCNF0_PLEN_Msk;
		break;

	case BIT(1):
		extra |= (RADIO_PCNF0_PLEN_16bit << RADIO_PCNF0_PLEN_Pos) &
			 RADIO_PCNF0_PLEN_Msk;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	case BIT(2):
		extra |= (RADIO_PCNF0_PLEN_LongRange << RADIO_PCNF0_PLEN_Pos) &
			 RADIO_PCNF0_PLEN_Msk;
		extra |= (2UL << RADIO_PCNF0_CILEN_Pos) & RADIO_PCNF0_CILEN_Msk;
		extra |= (3UL << RADIO_PCNF0_TERMLEN_Pos) &
			 RADIO_PCNF0_TERMLEN_Msk;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}

	/* To use same Data Channel PDU structure with nRF5 specific overhead
	 * byte, include the S1 field in radio packet configuration.
	 */
	if (dc) {
		extra |= (RADIO_PCNF0_S1INCL_Include <<
			  RADIO_PCNF0_S1INCL_Pos) & RADIO_PCNF0_S1INCL_Msk;
	}
#endif /* CONFIG_SOC_COMPATIBLE_NRF52X */

	NRF_RADIO->PCNF0 = (((1UL) << RADIO_PCNF0_S0LEN_Pos) &
			    RADIO_PCNF0_S0LEN_Msk) |
			   ((((u32_t)bits_len) << RADIO_PCNF0_LFLEN_Pos) &
			    RADIO_PCNF0_LFLEN_Msk) |
			   ((((u32_t)8-bits_len) << RADIO_PCNF0_S1LEN_Pos) &
			    RADIO_PCNF0_S1LEN_Msk) |
			   extra;

	NRF_RADIO->PCNF1 &= ~(RADIO_PCNF1_MAXLEN_Msk | RADIO_PCNF1_STATLEN_Msk |
			      RADIO_PCNF1_BALEN_Msk | RADIO_PCNF1_ENDIAN_Msk);
	NRF_RADIO->PCNF1 |= ((((u32_t)max_len) << RADIO_PCNF1_MAXLEN_Pos) &
			     RADIO_PCNF1_MAXLEN_Msk) |
			    (((0UL) << RADIO_PCNF1_STATLEN_Pos) &
			     RADIO_PCNF1_STATLEN_Msk) |
			    (((3UL) << RADIO_PCNF1_BALEN_Pos) &
			     RADIO_PCNF1_BALEN_Msk) |
			    (((RADIO_PCNF1_ENDIAN_Little) <<
			      RADIO_PCNF1_ENDIAN_Pos) &
			     RADIO_PCNF1_ENDIAN_Msk);
}

void radio_pkt_rx_set(void *rx_packet)
{
	NRF_RADIO->PACKETPTR = (u32_t)rx_packet;
}

void radio_pkt_tx_set(void *tx_packet)
{
	NRF_RADIO->PACKETPTR = (u32_t)tx_packet;
}

u32_t radio_tx_ready_delay_get(u8_t phy, u8_t flags)
{
	return hal_radio_tx_ready_delay_us_get(phy, flags);
}

u32_t radio_tx_chain_delay_get(u8_t phy, u8_t flags)
{
	return hal_radio_tx_chain_delay_us_get(phy, flags);
}

u32_t radio_rx_ready_delay_get(u8_t phy, u8_t flags)
{
	return hal_radio_rx_ready_delay_us_get(phy, flags);
}

u32_t radio_rx_chain_delay_get(u8_t phy, u8_t flags)
{
	return hal_radio_rx_chain_delay_us_get(phy, flags);
}

void radio_rx_enable(void)
{
	nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
}

void radio_tx_enable(void)
{
	nrf_radio_task_trigger(NRF_RADIO_TASK_TXEN);
}

void radio_disable(void)
{
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	hal_radio_nrf_ppi_channels_disable(BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
				 BIT(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI));
	hal_radio_nrf_ppi_group_disable(SW_SWITCH_TIMER_TASK_GROUP(0));
	hal_radio_nrf_ppi_group_disable(SW_SWITCH_TIMER_TASK_GROUP(1));
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	NRF_RADIO->SHORTS = 0;
	nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
}

void radio_status_reset(void)
{
	/* NOTE: Only EVENTS_* registers read (checked) by software needs reset
	 *       between Radio IRQs. In PPI use, irrespective of stored EVENT_*
	 *       register value, PPI task will be triggered. Hence, other
	 *       EVENT_* registers are not reset to save code and CPU time.
	 */
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
}

u32_t radio_is_ready(void)
{
	return (NRF_RADIO->EVENTS_READY != 0);
}

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
static u32_t last_pdu_end_us;

u32_t radio_is_done(void)
{
	if (NRF_RADIO->EVENTS_END != 0) {
		/* On packet END event increment last packet end time value.
		 * Note: this depends on the function being called exactly once
		 * in the ISR function.
		 */
		last_pdu_end_us += EVENT_TIMER->CC[2];
		return 1;
	} else {
		return 0;
	}
}

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
u32_t radio_is_done(void)
{
	return (NRF_RADIO->EVENTS_END != 0);
}
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

u32_t radio_has_disabled(void)
{
	return (NRF_RADIO->EVENTS_DISABLED != 0);
}

u32_t radio_is_idle(void)
{
	return (NRF_RADIO->STATE == 0);
}

void radio_crc_configure(u32_t polynomial, u32_t iv)
{
	NRF_RADIO->CRCCNF =
	    (((RADIO_CRCCNF_SKIPADDR_Skip) << RADIO_CRCCNF_SKIPADDR_Pos) &
	     RADIO_CRCCNF_SKIPADDR_Msk) |
	    (((RADIO_CRCCNF_LEN_Three) << RADIO_CRCCNF_LEN_Pos) &
	       RADIO_CRCCNF_LEN_Msk);
	NRF_RADIO->CRCPOLY = polynomial;
	NRF_RADIO->CRCINIT = iv;
}

u32_t radio_crc_is_valid(void)
{
	return (NRF_RADIO->CRCSTATUS != 0);
}

static u8_t MALIGN(4) _pkt_empty[PDU_EM_SIZE_MAX];
static u8_t MALIGN(4) _pkt_scratch[
			((RADIO_PDU_LEN_MAX + 3) > PDU_AC_SIZE_MAX) ?
			(RADIO_PDU_LEN_MAX + 3) : PDU_AC_SIZE_MAX];

void *radio_pkt_empty_get(void)
{
	return _pkt_empty;
}

void *radio_pkt_scratch_get(void)
{
	return _pkt_scratch;
}

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
static u8_t sw_tifs_toggle;

static void sw_switch(u8_t dir, u8_t phy_curr, u8_t flags_curr, u8_t phy_next,
		      u8_t flags_next)
{
	u8_t ppi = HAL_SW_SWITCH_RADIO_ENABLE_PPI(sw_tifs_toggle);
	u8_t cc = SW_SWITCH_TIMER_EVTS_COMP(sw_tifs_toggle);
	u32_t delay;

	hal_radio_sw_switch_setup(cc, ppi, sw_tifs_toggle);

	if (dir) {
		/* TX */

		/* Calculate delay with respect to current (RX) and next
		 * (TX) PHY. If RX PHY is LE Coded, assume S8 coding scheme.
		 */
		delay = HAL_RADIO_NS2US_ROUND(
		    hal_radio_tx_ready_delay_ns_get(phy_next, flags_next) +
		    hal_radio_rx_chain_delay_ns_get(phy_curr, 1));

		hal_radio_txen_on_sw_switch(ppi);

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
		u8_t ppi_en =
		    HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(sw_tifs_toggle);
		u8_t ppi_dis =
			HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(
			    sw_tifs_toggle);

		if (phy_curr & BIT(2)) {
			/* Switching to TX after RX on LE Coded PHY. */

			u8_t cc_s2 =
			    SW_SWITCH_TIMER_S2_EVTS_COMP(sw_tifs_toggle);

			u32_t delay_s2;

			/* Calculate assuming reception on S2 coding scheme. */
			delay_s2 = HAL_RADIO_NS2US_ROUND(
				hal_radio_tx_ready_delay_ns_get(
					phy_next, flags_next) +
				hal_radio_rx_chain_delay_ns_get(phy_curr, 0));

			SW_SWITCH_TIMER->CC[cc_s2] =
				SW_SWITCH_TIMER->CC[cc];

			if (delay_s2 < SW_SWITCH_TIMER->CC[cc_s2]) {
				SW_SWITCH_TIMER->CC[cc_s2] -= delay_s2;
			} else {
				SW_SWITCH_TIMER->CC[cc_s2] = 1;
			}

			hal_radio_sw_switch_coded_tx_config_set(ppi_en, ppi_dis,
				cc_s2, sw_tifs_toggle);
		} else {
			/* Switching to TX after RX on LE 1M/2M PHY */

			hal_radio_sw_switch_coded_config_clear(ppi_en,
				ppi_dis, cc, sw_tifs_toggle);
		}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	} else {
		/* RX */
		delay = HAL_RADIO_NS2US_CEIL(
			hal_radio_rx_ready_delay_ns_get(phy_next, flags_next) -
			hal_radio_tx_chain_delay_ns_get(phy_curr, flags_curr)) +
			4; /* 4us as +/- active jitter */

		hal_radio_rxen_on_sw_switch(ppi);

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
		if (1) {
			u8_t ppi_en =
				HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(
					sw_tifs_toggle);
			u8_t ppi_dis =
				HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(
					sw_tifs_toggle);

			hal_radio_sw_switch_coded_config_clear(ppi_en,
				ppi_dis, cc, sw_tifs_toggle);
		}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}

	if (delay <
		SW_SWITCH_TIMER->CC[cc]) {
		nrf_timer_cc_write(SW_SWITCH_TIMER, cc,
				   SW_SWITCH_TIMER->CC[cc] - delay);
	} else {
		nrf_timer_cc_write(SW_SWITCH_TIMER, cc, 1);
	}

	hal_radio_nrf_ppi_channels_enable(BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
				BIT(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI));

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* Since the event timer is cleared on END, we
	 * always need to capture the PDU END time-stamp.
	 */
	radio_tmr_end_capture();
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	sw_tifs_toggle += 1U;
	sw_tifs_toggle &= 1;
}
#endif /* CONFIG_BT_CTLR_TIFS_HW */

void radio_switch_complete_and_rx(u8_t phy_rx)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_RXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk;
	sw_switch(0, 0, 0, phy_rx, 0);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_tx(u8_t phy_rx, u8_t flags_rx, u8_t phy_tx,
				  u8_t flags_tx)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk;
	sw_switch(1, phy_rx, flags_rx, phy_tx, flags_tx);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_disable(void)
{
	NRF_RADIO->SHORTS =
	    (RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	hal_radio_sw_switch_disable();
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_rssi_measure(void)
{
	NRF_RADIO->SHORTS |=
	    (RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
	     RADIO_SHORTS_DISABLED_RSSISTOP_Msk);
}

u32_t radio_rssi_get(void)
{
	return NRF_RADIO->RSSISAMPLE;
}

void radio_rssi_status_reset(void)
{
	NRF_RADIO->EVENTS_RSSIEND = 0;
}

u32_t radio_rssi_is_ready(void)
{
	return (NRF_RADIO->EVENTS_RSSIEND != 0);
}

void radio_filter_configure(u8_t bitmask_enable, u8_t bitmask_addr_type,
			    u8_t *bdaddr)
{
	u8_t index;

	for (index = 0U; index < 8; index++) {
		NRF_RADIO->DAB[index] = ((u32_t)bdaddr[3] << 24) |
			((u32_t)bdaddr[2] << 16) |
			((u32_t)bdaddr[1] << 8) |
			bdaddr[0];
		NRF_RADIO->DAP[index] = ((u32_t)bdaddr[5] << 8) | bdaddr[4];
		bdaddr += 6;
	}

	NRF_RADIO->DACNF = ((u32_t)bitmask_addr_type << 8) | bitmask_enable;
}

void radio_filter_disable(void)
{
	NRF_RADIO->DACNF &= ~(0x000000FF);
}

void radio_filter_status_reset(void)
{
	NRF_RADIO->EVENTS_DEVMATCH = 0;
}

u32_t radio_filter_has_match(void)
{
	return (NRF_RADIO->EVENTS_DEVMATCH != 0);
}

u32_t radio_filter_match_get(void)
{
	return NRF_RADIO->DAI;
}

void radio_bc_configure(u32_t n)
{
	nrf_radio_bcc_set(n);
	NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_BCSTART_Msk;
}

void radio_bc_status_reset(void)
{
	NRF_RADIO->EVENTS_BCMATCH = 0;
}

u32_t radio_bc_has_match(void)
{
	return (NRF_RADIO->EVENTS_BCMATCH != 0);
}

void radio_tmr_status_reset(void)
{
	nrf_rtc_event_disable(NRF_RTC0, RTC_EVTENCLR_COMPARE2_Msk);

	hal_radio_nrf_ppi_channels_disable(
			BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI) |
			BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI) |
			BIT(HAL_EVENT_TIMER_START_PPI) |
			BIT(HAL_RADIO_READY_TIME_CAPTURE_PPI) |
			BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) |
			BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI) |
			BIT(HAL_RADIO_END_TIME_CAPTURE_PPI) |
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
			BIT(HAL_TRIGGER_RATEOVERRIDE_PPI) |
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			BIT(HAL_TRIGGER_CRYPT_PPI));
}

void radio_tmr_tifs_set(u32_t tifs)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->TIFS = tifs;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	nrf_timer_cc_write(SW_SWITCH_TIMER,
			   SW_SWITCH_TIMER_EVTS_COMP(sw_tifs_toggle), tifs);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

u32_t radio_tmr_start(u8_t trx, u32_t ticks_start, u32_t remainder)
{
	if ((!(remainder / 1000000UL)) || (remainder & 0x80000000)) {
		ticks_start--;
		remainder += 30517578UL;
	}
	remainder /= 1000000UL;

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);
	EVENT_TIMER->MODE = 0;
	EVENT_TIMER->PRESCALER = 4;
	EVENT_TIMER->BITMODE = 2;	/* 24 - bit */

	nrf_timer_cc_write(EVENT_TIMER, 0, remainder);

	nrf_rtc_cc_set(NRF_RTC0, 2, ticks_start);
	nrf_rtc_event_enable(NRF_RTC0, RTC_EVTENSET_COMPARE2_Msk);

	hal_event_timer_start_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_EVENT_TIMER_START_PPI));

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	last_pdu_end_us = 0U;

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_CLEAR);
	SW_SWITCH_TIMER->MODE = 0;
	SW_SWITCH_TIMER->PRESCALER = 4;
	SW_SWITCH_TIMER->BITMODE = 0; /* 16 bit */
	/* FIXME: start alongwith EVENT_TIMER, to save power */
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_START);
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	hal_sw_switch_timer_clear_ppi_config();

#if !defined(CONFIG_BT_CTLR_PHY_CODED) || \
	!defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)

	hal_radio_group_task_disable_ppi_setup();

#else /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
	/* PPI setup needs to be configured at every sw_switch()
	 * as they depend on the actual PHYs used in TX/RX mode.
	 */
#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	return remainder;
}

u32_t radio_tmr_start_tick(u8_t trx, u32_t tick)
{
	u32_t remainder_us;

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);

	/* Setup compare event with min. 1 us offset */
	remainder_us = 1;
	nrf_timer_cc_write(EVENT_TIMER, 0, remainder_us);

	nrf_rtc_cc_set(NRF_RTC0, 2, tick);
	nrf_rtc_event_enable(NRF_RTC0, RTC_EVTENSET_COMPARE2_Msk);

	hal_event_timer_start_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_EVENT_TIMER_START_PPI));

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	last_pdu_end_us = 0U;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	return remainder_us;
}

void radio_tmr_start_us(u8_t trx, u32_t us)
{
	nrf_timer_cc_write(EVENT_TIMER, 0, us);

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);
}

u32_t radio_tmr_start_now(u8_t trx)
{
	u32_t now, start;

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

	/* Capture the current time */
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CAPTURE1);
	now = EVENT_TIMER->CC[1];
	start = now;

	/* Setup PPI while determining the latency in doing so */
	do {
		/* Set start to be, now plus the determined latency */
		start = (now << 1) - start;

		/* Setup compare event with min. 1 us offset */
		nrf_timer_cc_write(EVENT_TIMER, 0, start + 1);

		/* Capture the current time */
		nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CAPTURE1);

		now = EVENT_TIMER->CC[1];
	} while (now > start);

	return start;
}

u32_t radio_tmr_start_get(void)
{
	return nrf_rtc_cc_get(NRF_RTC0, 2);
}

void radio_tmr_stop(void)
{
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_SHUTDOWN);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_SHUTDOWN);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_tmr_hcto_configure(u32_t hcto)
{
	nrf_timer_cc_write(EVENT_TIMER, 1, hcto);

	hal_radio_recv_timeout_cancel_ppi_config();
	hal_radio_disable_on_hcto_ppi_config();
	hal_radio_nrf_ppi_channels_enable(
		BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) |
		BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI));
}

void radio_tmr_aa_capture(void)
{
	hal_radio_ready_time_capture_ppi_config();
	hal_radio_recv_timeout_cancel_ppi_config();
	hal_radio_nrf_ppi_channels_enable(
		BIT(HAL_RADIO_READY_TIME_CAPTURE_PPI) |
		BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI));
}

u32_t radio_tmr_aa_get(void)
{
	return EVENT_TIMER->CC[1];
}

static u32_t radio_tmr_aa;

void radio_tmr_aa_save(u32_t aa)
{
	radio_tmr_aa = aa;
}

u32_t radio_tmr_aa_restore(void)
{
	/* NOTE: we dont need to restore for now, but return the saved value. */
	return radio_tmr_aa;
}

u32_t radio_tmr_ready_get(void)
{
	return EVENT_TIMER->CC[0];
}

void radio_tmr_end_capture(void)
{
	hal_radio_end_time_capture_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_RADIO_END_TIME_CAPTURE_PPI));
}

u32_t radio_tmr_end_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return last_pdu_end_us;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return EVENT_TIMER->CC[2];
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

u32_t radio_tmr_tifs_base_get(void)
{
	return radio_tmr_end_get();
}

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
static u32_t tmr_sample_val;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

void radio_tmr_sample(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	u32_t cc;

	cc = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);

	tmr_sample_val = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET] = cc;

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

u32_t radio_tmr_sample_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return tmr_sample_val;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN) || \
    defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
void radio_gpio_pa_setup(void)
{
	/* NOTE: With GPIO Pins above 31, left shift of
	 *       CONFIG_BT_CTLR_GPIO_PA_PIN by GPIOTE_CONFIG_PSEL_Pos will
	 *       set the NRF_GPIOTE->CONFIG[n].PORT to 1 (P1 port).
	 */
	NRF_GPIOTE->CONFIG[CONFIG_BT_CTLR_PA_LNA_GPIOTE_CHAN] =
		(GPIOTE_CONFIG_MODE_Task <<
		 GPIOTE_CONFIG_MODE_Pos) |
		(CONFIG_BT_CTLR_GPIO_PA_PIN <<
		 GPIOTE_CONFIG_PSEL_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle <<
		 GPIOTE_CONFIG_POLARITY_Pos) |
#if defined(CONFIG_BT_CTLR_GPIO_PA_POL_INV)
		(GPIOTE_CONFIG_OUTINIT_High <<
		 GPIOTE_CONFIG_OUTINIT_Pos);
#else
		(GPIOTE_CONFIG_OUTINIT_Low <<
		 GPIOTE_CONFIG_OUTINIT_Pos);
#endif
}
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
void radio_gpio_lna_setup(void)
{
	/* NOTE: With GPIO Pins above 31, left shift of
	 *       CONFIG_BT_CTLR_GPIO_LNA_PIN by GPIOTE_CONFIG_PSEL_Pos will
	 *       set the NRF_GPIOTE->CONFIG[n].PORT to 1 (P1 port).
	 */
	NRF_GPIOTE->CONFIG[CONFIG_BT_CTLR_PA_LNA_GPIOTE_CHAN] =
		(GPIOTE_CONFIG_MODE_Task <<
		 GPIOTE_CONFIG_MODE_Pos) |
		(CONFIG_BT_CTLR_GPIO_LNA_PIN <<
		 GPIOTE_CONFIG_PSEL_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle <<
		 GPIOTE_CONFIG_POLARITY_Pos) |
#if defined(CONFIG_BT_CTLR_GPIO_LNA_POL_INV)
		(GPIOTE_CONFIG_OUTINIT_High <<
		 GPIOTE_CONFIG_OUTINIT_Pos);
#else
		(GPIOTE_CONFIG_OUTINIT_Low <<
		 GPIOTE_CONFIG_OUTINIT_Pos);
#endif
}

void radio_gpio_lna_on(void)
{
#if defined(CONFIG_BT_CTLR_GPIO_LNA_POL_INV)
	NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
#else
	NRF_GPIO_LNA->OUTSET = BIT(NRF_GPIO_LNA_PIN);
#endif
}

void radio_gpio_lna_off(void)
{
#if defined(CONFIG_BT_CTLR_GPIO_LNA_POL_INV)
	NRF_GPIO_LNA->OUTSET = BIT(NRF_GPIO_LNA_PIN);
#else
	NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
#endif
}
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

void radio_gpio_pa_lna_enable(u32_t trx_us)
{
	nrf_timer_cc_write(EVENT_TIMER, 2, trx_us);

	hal_enable_palna_ppi_config();
	hal_disable_palna_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_ENABLE_PALNA_PPI) |
				BIT(HAL_DISABLE_PALNA_PPI));
}

void radio_gpio_pa_lna_disable(void)
{
	hal_radio_nrf_ppi_channels_disable(BIT(HAL_ENABLE_PALNA_PPI) |
				 BIT(HAL_DISABLE_PALNA_PPI));
}
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN || CONFIG_BT_CTLR_GPIO_LNA_PIN */

static u8_t MALIGN(4) _ccm_scratch[(RADIO_PDU_LEN_MAX - 4) + 16];

void *radio_ccm_rx_pkt_set(struct ccm *ccm, u8_t phy, void *pkt)
{

	u32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;
	mode = (CCM_MODE_MODE_Decryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
	/* Enable CCM support for 8-bit length field PDUs. */
	mode |= (CCM_MODE_LENGTH_Extended << CCM_MODE_LENGTH_Pos) &
		CCM_MODE_LENGTH_Msk;

	/* Select CCM data rate based on current PHY in use. */
	switch (phy) {
	default:
	case BIT(0):
		mode |= (CCM_MODE_DATARATE_1Mbit <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;
		break;

	case BIT(1):
		mode |= (CCM_MODE_DATARATE_2Mbit <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	case BIT(2):
		mode |= (CCM_MODE_DATARATE_125Kbps <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;

		NRF_CCM->RATEOVERRIDE =
			(CCM_RATEOVERRIDE_RATEOVERRIDE_500Kbps <<
			 CCM_RATEOVERRIDE_RATEOVERRIDE_Pos) &
			CCM_RATEOVERRIDE_RATEOVERRIDE_Msk;

		hal_trigger_rateoverride_ppi_config();
		hal_radio_nrf_ppi_channels_enable(
			BIT(HAL_TRIGGER_RATEOVERRIDE_PPI));
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
#endif /* CONFIG_SOC_COMPATIBLE_NRF52X */

	NRF_CCM->MODE = mode;
	NRF_CCM->CNFPTR = (u32_t)ccm;
	NRF_CCM->INPTR = (u32_t)_pkt_scratch;
	NRF_CCM->OUTPTR = (u32_t)pkt;
	NRF_CCM->SCRATCHPTR = (u32_t)_ccm_scratch;
	NRF_CCM->SHORTS = 0;
	NRF_CCM->EVENTS_ENDCRYPT = 0;
	NRF_CCM->EVENTS_ERROR = 0;

	hal_trigger_crypt_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_PPI));

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_KSGEN);

	return _pkt_scratch;
}

void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt)
{
	u32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;
	mode = (CCM_MODE_MODE_Encryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
	/* Enable CCM support for 8-bit length field PDUs. */
	mode |= (CCM_MODE_LENGTH_Extended << CCM_MODE_LENGTH_Pos) &
		CCM_MODE_LENGTH_Msk;

	/* NOTE: use fastest data rate as tx data needs to be prepared before
	 * radio Tx on any PHY.
	 */
	mode |= (CCM_MODE_DATARATE_2Mbit << CCM_MODE_DATARATE_Pos) &
		CCM_MODE_DATARATE_Msk;
#endif
	NRF_CCM->MODE = mode;
	NRF_CCM->CNFPTR = (u32_t)ccm;
	NRF_CCM->INPTR = (u32_t)pkt;
	NRF_CCM->OUTPTR = (u32_t)_pkt_scratch;
	NRF_CCM->SCRATCHPTR = (u32_t)_ccm_scratch;
	NRF_CCM->SHORTS = CCM_SHORTS_ENDKSGEN_CRYPT_Msk;
	NRF_CCM->EVENTS_ENDCRYPT = 0;
	NRF_CCM->EVENTS_ERROR = 0;

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_KSGEN);

	return _pkt_scratch;
}

u32_t radio_ccm_is_done(void)
{
	nrf_ccm_int_enable(NRF_CCM, CCM_INTENSET_ENDCRYPT_Msk);
	while (NRF_CCM->EVENTS_ENDCRYPT == 0) {
		__WFE();
		__SEV();
		__WFE();
	}
	nrf_ccm_int_disable(NRF_CCM, CCM_INTENCLR_ENDCRYPT_Msk);
	NVIC_ClearPendingIRQ(CCM_AAR_IRQn);

	return (NRF_CCM->EVENTS_ERROR == 0);
}

u32_t radio_ccm_mic_is_valid(void)
{
	return (NRF_CCM->MICSTATUS != 0);
}

static u8_t MALIGN(4) _aar_scratch[3];

void radio_ar_configure(u32_t nirk, void *irk)
{
	NRF_AAR->ENABLE = (AAR_ENABLE_ENABLE_Enabled << AAR_ENABLE_ENABLE_Pos) &
			  AAR_ENABLE_ENABLE_Msk;
	NRF_AAR->NIRK = nirk;
	NRF_AAR->IRKPTR = (u32_t)irk;
	NRF_AAR->ADDRPTR = (u32_t)NRF_RADIO->PACKETPTR - 1;
	NRF_AAR->SCRATCHPTR = (u32_t)&_aar_scratch[0];

	NRF_AAR->EVENTS_END = 0;
	NRF_AAR->EVENTS_RESOLVED = 0;
	NRF_AAR->EVENTS_NOTRESOLVED = 0;

	radio_bc_configure(64);
	radio_bc_status_reset();

	hal_trigger_aar_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_AAR_PPI));
}

u32_t radio_ar_match_get(void)
{
	return NRF_AAR->STATUS;
}

void radio_ar_status_reset(void)
{
	radio_bc_status_reset();

	NRF_AAR->ENABLE = (AAR_ENABLE_ENABLE_Disabled << AAR_ENABLE_ENABLE_Pos) &
			  AAR_ENABLE_ENABLE_Msk;
}

u32_t radio_ar_has_match(void)
{
	return (radio_bc_has_match() &&
		NRF_AAR->EVENTS_END &&
		NRF_AAR->EVENTS_RESOLVED &&
		!NRF_AAR->EVENTS_NOTRESOLVED);
}
