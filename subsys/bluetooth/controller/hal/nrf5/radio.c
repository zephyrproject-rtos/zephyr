/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <arch/arm/cortex_m/cmsis.h>

#include "util/mem.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll_sw/pdu.h"

#if defined(CONFIG_SOC_SERIES_NRF51X)
#define RADIO_PDU_LEN_MAX (BIT(5) - 1)
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#define RADIO_PDU_LEN_MAX (BIT(8) - 1)
#else
#error "Platform not defined."
#endif

static radio_isr_fp sfp_radio_isr;

void isr_radio(void)
{
	if (sfp_radio_isr) {
		sfp_radio_isr();
	}
}

void radio_isr_set(radio_isr_fp fp_radio_isr)
{
	sfp_radio_isr = fp_radio_isr;	/* atomic assignment of 32-bit word */

	NRF_RADIO->INTENSET = (0 |
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
	NRF_GPIO->DIRSET = BIT(CONFIG_BT_CTLR_GPIO_PA_PIN);
#if defined(CONFIG_BT_CTLR_GPIO_PA_POL_INV)
	NRF_GPIO->OUTSET = BIT(CONFIG_BT_CTLR_GPIO_PA_PIN);
#else
	NRF_GPIO->OUTCLR = BIT(CONFIG_BT_CTLR_GPIO_PA_PIN);
#endif
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	NRF_GPIO->DIRSET = BIT(CONFIG_BT_CTLR_GPIO_LNA_PIN);

	radio_gpio_lna_off();
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_SOC_SERIES_NRF52X)
	struct {
		u32_t volatile reserved_0[0x5a0 >> 2];
		u32_t volatile bridge_type;
		u32_t volatile reserved_1[((0xe00 - 0x5a0) >> 2) - 1];
		struct {
			u32_t volatile CPU0;
			u32_t volatile SPIS1;
			u32_t volatile RADIO;
			u32_t volatile ECB;
			u32_t volatile CCM;
			u32_t volatile AAR;
			u32_t volatile SAADC;
			u32_t volatile UARTE;
			u32_t volatile SERIAL0;
			u32_t volatile SERIAL2;
			u32_t volatile NFCT;
			u32_t volatile I2S;
			u32_t volatile PDM;
			u32_t volatile PWM;
		} RAMPRI;
	} volatile *NRF_AMLI = (void volatile *)0x40000000UL;

	NRF_AMLI->RAMPRI.CPU0    = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.SPIS1   = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.RADIO   = 0x00000000UL;
	NRF_AMLI->RAMPRI.ECB     = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.CCM     = 0x00000000UL;
	NRF_AMLI->RAMPRI.AAR     = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.SAADC   = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.UARTE   = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.SERIAL0 = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.SERIAL2 = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.NFCT    = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.I2S     = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.PDM     = 0xFFFFFFFFUL;
	NRF_AMLI->RAMPRI.PWM     = 0xFFFFFFFFUL;
#endif /* CONFIG_SOC_SERIES_NRF52X */
}

void radio_reset(void)
{
	irq_disable(RADIO_IRQn);

	NRF_RADIO->POWER =
	    ((RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos) &
	     RADIO_POWER_POWER_Msk);
	NRF_RADIO->POWER =
	    ((RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos) &
	     RADIO_POWER_POWER_Msk);
}

void radio_phy_set(u8_t phy, u8_t flags)
{
	u32_t mode;

	switch (phy) {
	case BIT(0):
	default:
		mode = RADIO_MODE_MODE_Ble_1Mbit;

#if defined(CONFIG_SOC_NRF52840)
		/* Workaround: nRF52840 Engineering A Errata ID 164 */
		*(volatile u32_t *)0x4000173c &= ~0x80000000;
#endif /* CONFIG_SOC_NRF52840 */
		break;

#if defined(CONFIG_SOC_SERIES_NRF51X)
	case BIT(1):
		mode = RADIO_MODE_MODE_Nrf_2Mbit;
		break;

#elif defined(CONFIG_SOC_SERIES_NRF52X)
	case BIT(1):
		mode = RADIO_MODE_MODE_Ble_2Mbit;

#if defined(CONFIG_SOC_NRF52840)
		/* Workaround: nRF52840 Engineering A Errata ID 164 */
		*(volatile u32_t *)0x4000173c &= ~0x80000000;
#endif /* CONFIG_SOC_NRF52840 */
		break;

#if defined(CONFIG_SOC_NRF52840)
	case BIT(2):
		if (flags & 0x01) {
			mode = RADIO_MODE_MODE_Ble_LR125Kbit;
		} else {
			mode = RADIO_MODE_MODE_Ble_LR500Kbit;
		}

		/* Workaround: nRF52840 Engineering A Errata ID 164 */
		*(volatile u32_t *)0x4000173c |= 0x80000000;
		*(volatile u32_t *)0x4000173c =
			((*(volatile u32_t *)0x4000173c) & 0xFFFFFF00) |
			0x5C;
		break;
#endif /* CONFIG_SOC_NRF52840 */
#endif /* CONFIG_SOC_SERIES_NRF52X */
	}

	NRF_RADIO->MODE = (mode << RADIO_MODE_MODE_Pos) & RADIO_MODE_MODE_Msk;

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
	NRF_RADIO->MODECNF0 |= (RADIO_MODECNF0_RU_Fast <<
				RADIO_MODECNF0_RU_Pos) &
			       RADIO_MODECNF0_RU_Msk;
#endif /* CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
}

void radio_tx_power_set(u32_t power)
{
	/* TODO map power to h/w values. */
	NRF_RADIO->TXPOWER = power;
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

	extra = 0;

	/* nRF51 supports only 27 byte PDU when using h/w CCM for encryption. */
	if (!IS_ENABLED(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR) && dc) {
		bits_len = 5;
	}
#elif defined(CONFIG_SOC_SERIES_NRF52X)
	extra = 0;

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

#if defined(CONFIG_SOC_NRF52840)
	case BIT(2):
		extra |= (RADIO_PCNF0_PLEN_LongRange << RADIO_PCNF0_PLEN_Pos) &
			 RADIO_PCNF0_PLEN_Msk;
		extra |= (2UL << RADIO_PCNF0_CILEN_Pos) & RADIO_PCNF0_CILEN_Msk;
		extra |= (3UL << RADIO_PCNF0_TERMLEN_Pos) &
			 RADIO_PCNF0_TERMLEN_Msk;
		break;
#endif /* CONFIG_SOC_NRF52840 */
	}

	/* To use same Data Channel PDU structure with nRF5 specific overhead
	 * byte, include the S1 field in radio packet configuration.
	 */
	if (dc) {
		extra |= (RADIO_PCNF0_S1INCL_Include <<
			  RADIO_PCNF0_S1INCL_Pos) & RADIO_PCNF0_S1INCL_Msk;
	}
#endif /* CONFIG_SOC_SERIES_NRF52X */

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
	/* Return TXEN->TXIDLE + TXIDLE->TX */

#if defined(CONFIG_SOC_SERIES_NRF51X)
	return 140;
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
	return 40;
#elif defined(CONFIG_SOC_NRF52840)
	switch (phy) {
	default:
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	case BIT(0):
		return 141; /* floor(140.1 + 1.6) */
	case BIT(1):
		return 146; /* floor(145 + 1) */
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	case BIT(0):
	case BIT(1):
		return 131; /* floor(129.5 + 1.6) */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
	case BIT(2):
		if (flags & 0x01) {
			return 121; /* floor(119.6 + 2.2) */
		} else {
			return 132; /* floor(130 + 2.2) */
		}
	}
#else /* !CONFIG_SOC_NRF52840 */
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	return 140;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	return 131; /* floor(129.5 + 1.6) */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* !CONFIG_SOC_NRF52840 */
#endif /* CONFIG_SOC_SERIES_NRF52X */
}

u32_t radio_tx_chain_delay_get(u8_t phy, u8_t flags)
{
#if defined(CONFIG_SOC_SERIES_NRF51X)
	return 1; /* ceil(1) */
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#if defined(CONFIG_SOC_NRF52840)
	switch (phy) {
	default:
	case BIT(0):
	case BIT(1):
		return 1; /* ceil(0.6) */
	case BIT(2):
		if (flags & 0x01) {
			return 1; /* TODO: different within packet */
		} else {
			return 1; /* TODO: different within packet */
		}
	}
#else /* !CONFIG_SOC_NRF52840 */
	return 1; /* ceil(0.6) */
#endif /* !CONFIG_SOC_NRF52840 */
#endif /* CONFIG_SOC_SERIES_NRF52X */
}

u32_t radio_rx_ready_delay_get(u8_t phy)
{
	/* Return RXEN->RXIDLE + RXIDLE->RX */

#if defined(CONFIG_SOC_SERIES_NRF51X)
	return 138;
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
	return 40;
#elif defined(CONFIG_SOC_NRF52840)
	switch (phy) {
	default:
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	case BIT(0):
		return 141; /* ceil(140.1 + 0.2) */
	case BIT(1):
		return 145; /* ceil(144.6 + 0.2) */
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	case BIT(0):
	case BIT(1):
		return 130; /* ceil(129.5 + 0.2) */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
	case BIT(2):
		return 121; /* ceil(120 + 0.2) */
	}
#else /* !CONFIG_SOC_NRF52840 */
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	return 140;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	return 130; /* ceil(129.5 + 0.2) */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* !CONFIG_SOC_NRF52840 */
#endif /* CONFIG_SOC_SERIES_NRF52X */
}

u32_t radio_rx_chain_delay_get(u8_t phy, u8_t flags)
{
#if defined(CONFIG_SOC_SERIES_NRF51X)
	return 3; /* ceil(3) */
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#if defined(CONFIG_SOC_NRF52840)
	switch (phy) {
	default:
	case BIT(0):
		return 10; /* ceil(9.4) */
	case BIT(1):
		return 5; /* ceil(5) */
	case BIT(2):
		if (flags & 0x01) {
			return 30; /* ciel(29.6) */
		} else {
			return 20; /* ciel(19.6) */
		}
	}
#else /* !CONFIG_SOC_NRF52840 */
	switch (phy) {
	default:
	case BIT(0):
		return 10; /* ceil(9.4) */
	case BIT(1):
		return 5; /* ceil(5) */
	}
#endif /* !CONFIG_SOC_NRF52840 */
#endif /* CONFIG_SOC_SERIES_NRF52X */
}

void radio_rx_enable(void)
{
	NRF_RADIO->TASKS_RXEN = 1;
}

void radio_tx_enable(void)
{
	NRF_RADIO->TASKS_TXEN = 1;
}

void radio_disable(void)
{
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_PPI->CHENCLR = PPI_CHEN_CH7_Msk | PPI_CHEN_CH10_Msk;
	NRF_PPI->TASKS_CHG[0].DIS = 1;
	NRF_PPI->TASKS_CHG[1].DIS = 1;
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	NRF_RADIO->SHORTS = 0;
	NRF_RADIO->TASKS_DISABLE = 1;
}

void radio_status_reset(void)
{
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_ADDRESS = 0;
	NRF_RADIO->EVENTS_PAYLOAD = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
}

u32_t radio_is_ready(void)
{
	return (NRF_RADIO->EVENTS_READY != 0);
}

u32_t radio_is_done(void)
{
	return (NRF_RADIO->EVENTS_END != 0);
}

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
	u8_t ppi = 11 + sw_tifs_toggle;
	u32_t delay;

	NRF_TIMER1->EVENTS_COMPARE[sw_tifs_toggle] = 0;

	NRF_PPI->CH[10].EEP = (u32_t)&(NRF_RADIO->EVENTS_END);
	NRF_PPI->CH[10].TEP = (u32_t)&(NRF_PPI->TASKS_CHG[sw_tifs_toggle].EN);

	NRF_PPI->CH[ppi].EEP = (u32_t)
			       &(NRF_TIMER1->EVENTS_COMPARE[sw_tifs_toggle]);
	if (dir) {
		delay = radio_tx_ready_delay_get(phy_next, flags_next) +
			radio_rx_chain_delay_get(phy_curr, 1);

		NRF_PPI->CH[ppi].TEP = (u32_t)&(NRF_RADIO->TASKS_TXEN);
	} else {
		delay = radio_rx_ready_delay_get(phy_next) -
			radio_tx_chain_delay_get(phy_curr, flags_curr) +
			4; /* 4us as +/- active jitter */

		NRF_PPI->CH[ppi].TEP = (u32_t)&(NRF_RADIO->TASKS_RXEN);
	}

	if (delay < NRF_TIMER1->CC[sw_tifs_toggle]) {
		NRF_TIMER1->CC[sw_tifs_toggle] -= delay;
	} else {
		NRF_TIMER1->CC[sw_tifs_toggle] = 1;
	}

	NRF_PPI->CHENSET = PPI_CHEN_CH7_Msk | PPI_CHEN_CH10_Msk;

	sw_tifs_toggle += 1;
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
	NRF_PPI->CHENCLR = PPI_CHEN_CH7_Msk | PPI_CHEN_CH10_Msk;
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

	for (index = 0; index < 8; index++) {
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
	NRF_RADIO->EVENTS_DEVMISS = 0;
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
	NRF_RADIO->BCC = n;
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
	NRF_RTC0->EVTENCLR = RTC_EVTENCLR_COMPARE2_Msk;
	NRF_PPI->CHENCLR =
	    (PPI_CHEN_CH0_Msk | PPI_CHEN_CH1_Msk | PPI_CHEN_CH2_Msk |
	     PPI_CHEN_CH3_Msk | PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk |
	     PPI_CHEN_CH6_Msk | PPI_CHEN_CH13_Msk);
}

void radio_tmr_tifs_set(u32_t tifs)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->TIFS = tifs;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_TIMER1->CC[sw_tifs_toggle] = tifs;
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

u32_t radio_tmr_start(u8_t trx, u32_t ticks_start, u32_t remainder)
{
	if ((!(remainder / 1000000UL)) || (remainder & 0x80000000)) {
		ticks_start--;
		remainder += 30517578UL;
	}
	remainder /= 1000000UL;

	NRF_TIMER0->TASKS_CLEAR = 1;
	NRF_TIMER0->MODE = 0;
	NRF_TIMER0->PRESCALER = 4;
	NRF_TIMER0->BITMODE = 2;	/* 24 - bit */

	NRF_TIMER0->CC[0] = remainder;
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;

	NRF_RTC0->CC[2] = ticks_start;
	NRF_RTC0->EVTENSET = RTC_EVTENSET_COMPARE2_Msk;
	NRF_RTC0->EVENTS_COMPARE[2] = 0;

	NRF_PPI->CH[1].EEP = (u32_t)&(NRF_RTC0->EVENTS_COMPARE[2]);
	NRF_PPI->CH[1].TEP = (u32_t)&(NRF_TIMER0->TASKS_START);
	NRF_PPI->CHENSET = PPI_CHEN_CH1_Msk;

	NRF_PPI->CH[0].EEP = (u32_t)&(NRF_TIMER0->EVENTS_COMPARE[0]);
	NRF_PPI->CH[0].TEP = (trx) ? (u32_t)&(NRF_RADIO->TASKS_TXEN) :
				     (u32_t)&(NRF_RADIO->TASKS_RXEN);
	NRF_PPI->CHENSET = PPI_CHEN_CH0_Msk;

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_TIMER1->TASKS_CLEAR = 1;
	NRF_TIMER1->MODE = 0;
	NRF_TIMER1->PRESCALER = 4;
	NRF_TIMER1->BITMODE = 0; /* 16 bit */
	NRF_TIMER1->TASKS_START = 1;

	NRF_PPI->CH[7].EEP = (u32_t)&(NRF_RADIO->EVENTS_END);
	NRF_PPI->CH[7].TEP = (u32_t)&(NRF_TIMER1->TASKS_CLEAR);

	NRF_PPI->CH[8].EEP = (u32_t)&(NRF_TIMER1->EVENTS_COMPARE[0]);
	NRF_PPI->CH[8].TEP = (u32_t)&(NRF_PPI->TASKS_CHG[0].DIS);

	NRF_PPI->CH[9].EEP = (u32_t)&(NRF_TIMER1->EVENTS_COMPARE[1]);
	NRF_PPI->CH[9].TEP = (u32_t)&(NRF_PPI->TASKS_CHG[1].DIS);

	NRF_PPI->CHG[0] = PPI_CHG_CH8_Msk | PPI_CHG_CH11_Msk;
	NRF_PPI->CHG[1] = PPI_CHG_CH9_Msk | PPI_CHG_CH12_Msk;
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	return remainder;
}

void radio_tmr_start_us(u8_t trx, u32_t us)
{
	NRF_TIMER0->CC[0] = us;
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;

	NRF_PPI->CH[0].EEP = (u32_t)&(NRF_TIMER0->EVENTS_COMPARE[0]);
	NRF_PPI->CH[0].TEP = (trx) ? (u32_t)&(NRF_RADIO->TASKS_TXEN) :
				     (u32_t)&(NRF_RADIO->TASKS_RXEN);
	NRF_PPI->CHENSET = PPI_CHEN_CH0_Msk;
}

u32_t radio_tmr_start_now(u8_t trx)
{
	u32_t now, start;

	/* Setup PPI for Radio start */
	NRF_PPI->CH[0].EEP = (u32_t)&(NRF_TIMER0->EVENTS_COMPARE[0]);
	NRF_PPI->CH[0].TEP = (trx) ? (u32_t)&(NRF_RADIO->TASKS_TXEN) :
				     (u32_t)&(NRF_RADIO->TASKS_RXEN);
	NRF_PPI->CHENSET = PPI_CHEN_CH0_Msk;

	/* Capture the current time */
	NRF_TIMER0->TASKS_CAPTURE[1] = 1;
	now = NRF_TIMER0->CC[1];
	start = now;

	/* Setup PPI while determining the latency in doing so */
	do {
		/* Set start to be, now plus the determined latency */
		start = (now << 1) - start;

		/* Setup compare event with min. 1 us offset */
		NRF_TIMER0->CC[0] = start + 1;
		NRF_TIMER0->EVENTS_COMPARE[0] = 0;

		/* Capture the current time */
		NRF_TIMER0->TASKS_CAPTURE[1] = 1;
		now = NRF_TIMER0->CC[1];
	} while (now > start);

	return start;
}

void radio_tmr_stop(void)
{
	NRF_TIMER0->TASKS_STOP = 1;
	NRF_TIMER0->TASKS_SHUTDOWN = 1;

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_TIMER1->TASKS_STOP = 1;
	NRF_TIMER1->TASKS_SHUTDOWN = 1;
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_tmr_hcto_configure(u32_t hcto)
{
	NRF_TIMER0->CC[1] = hcto;
	NRF_TIMER0->EVENTS_COMPARE[1] = 0;

	NRF_PPI->CH[3].EEP = (u32_t)&(NRF_RADIO->EVENTS_ADDRESS);
	NRF_PPI->CH[3].TEP = (u32_t)&(NRF_TIMER0->TASKS_CAPTURE[1]);
	NRF_PPI->CH[4].EEP = (u32_t)&(NRF_TIMER0->EVENTS_COMPARE[1]);
	NRF_PPI->CH[4].TEP = (u32_t)&(NRF_RADIO->TASKS_DISABLE);
	NRF_PPI->CHENSET = (PPI_CHEN_CH3_Msk | PPI_CHEN_CH4_Msk);
}

void radio_tmr_aa_capture(void)
{
	NRF_PPI->CH[2].EEP = (u32_t)&(NRF_RADIO->EVENTS_READY);
	NRF_PPI->CH[2].TEP = (u32_t)&(NRF_TIMER0->TASKS_CAPTURE[0]);
	NRF_PPI->CH[3].EEP = (u32_t)&(NRF_RADIO->EVENTS_ADDRESS);
	NRF_PPI->CH[3].TEP = (u32_t)&(NRF_TIMER0->TASKS_CAPTURE[1]);
	NRF_PPI->CHENSET = (PPI_CHEN_CH2_Msk | PPI_CHEN_CH3_Msk);
}

u32_t radio_tmr_aa_get(void)
{
	return NRF_TIMER0->CC[1];
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
	return NRF_TIMER0->CC[0];
}

void radio_tmr_end_capture(void)
{
	NRF_PPI->CH[5].EEP = (u32_t)&(NRF_RADIO->EVENTS_END);
	NRF_PPI->CH[5].TEP = (u32_t)&(NRF_TIMER0->TASKS_CAPTURE[2]);
	NRF_PPI->CHENSET = PPI_CHEN_CH5_Msk;
}

u32_t radio_tmr_end_get(void)
{
	return NRF_TIMER0->CC[2];
}

void radio_tmr_sample(void)
{
	NRF_TIMER0->TASKS_CAPTURE[3] = 1;
}

u32_t radio_tmr_sample_get(void)
{
	return NRF_TIMER0->CC[3];
}

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN) || \
    defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
void radio_gpio_pa_setup(void)
{
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
	NRF_GPIO->OUTCLR = BIT(CONFIG_BT_CTLR_GPIO_LNA_PIN);
#else
	NRF_GPIO->OUTSET = BIT(CONFIG_BT_CTLR_GPIO_LNA_PIN);
#endif
}

void radio_gpio_lna_off(void)
{
#if defined(CONFIG_BT_CTLR_GPIO_LNA_POL_INV)
	NRF_GPIO->OUTSET = BIT(CONFIG_BT_CTLR_GPIO_LNA_PIN);
#else
	NRF_GPIO->OUTCLR = BIT(CONFIG_BT_CTLR_GPIO_LNA_PIN);
#endif
}
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

void radio_gpio_pa_lna_enable(u32_t trx_us)
{
	NRF_TIMER0->CC[2] = trx_us;
	NRF_TIMER0->EVENTS_COMPARE[2] = 0;

	NRF_PPI->CH[14].EEP = (u32_t)&(NRF_TIMER0->EVENTS_COMPARE[2]);
	NRF_PPI->CH[14].TEP = (u32_t)
		&(NRF_GPIOTE->TASKS_OUT[CONFIG_BT_CTLR_PA_LNA_GPIOTE_CHAN]);

	NRF_PPI->CH[15].EEP = (u32_t)&(NRF_RADIO->EVENTS_DISABLED);
	NRF_PPI->CH[15].TEP = (u32_t)
		&(NRF_GPIOTE->TASKS_OUT[CONFIG_BT_CTLR_PA_LNA_GPIOTE_CHAN]);

	NRF_PPI->CHENSET = PPI_CHEN_CH14_Msk | PPI_CHEN_CH15_Msk;
}

void radio_gpio_pa_lna_disable(void)
{
	NRF_PPI->CHENCLR = PPI_CHEN_CH14_Msk | PPI_CHEN_CH15_Msk;
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
#if defined(CONFIG_SOC_SERIES_NRF52X)
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

#if defined(CONFIG_SOC_NRF52840)
	case BIT(2):
		mode |= (CCM_MODE_DATARATE_125Kbps <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;

		NRF_CCM->RATEOVERRIDE =
			(CCM_RATEOVERRIDE_RATEOVERRIDE_500Kbps <<
			 CCM_RATEOVERRIDE_RATEOVERRIDE_Pos) &
			CCM_RATEOVERRIDE_RATEOVERRIDE_Msk;

		NRF_PPI->CH[13].EEP = (u32_t)&(NRF_RADIO->EVENTS_RATEBOOST);
		NRF_PPI->CH[13].TEP = (u32_t)&(NRF_CCM->TASKS_RATEOVERRIDE);
		NRF_PPI->CHENSET = PPI_CHEN_CH13_Msk;
		break;
#endif /* CONFIG_SOC_NRF52840 */
	}
#endif
	NRF_CCM->MODE = mode;
	NRF_CCM->CNFPTR = (u32_t)ccm;
	NRF_CCM->INPTR = (u32_t)_pkt_scratch;
	NRF_CCM->OUTPTR = (u32_t)pkt;
	NRF_CCM->SCRATCHPTR = (u32_t)_ccm_scratch;
	NRF_CCM->SHORTS = 0;
	NRF_CCM->EVENTS_ENDKSGEN = 0;
	NRF_CCM->EVENTS_ENDCRYPT = 0;
	NRF_CCM->EVENTS_ERROR = 0;

	NRF_PPI->CH[6].EEP = (u32_t)&(NRF_RADIO->EVENTS_ADDRESS);
	NRF_PPI->CH[6].TEP = (u32_t)&(NRF_CCM->TASKS_CRYPT);
	NRF_PPI->CHENSET = PPI_CHEN_CH6_Msk;

	NRF_CCM->TASKS_KSGEN = 1;

	return _pkt_scratch;
}

void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt)
{
	u32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;
	mode = (CCM_MODE_MODE_Encryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;
#if defined(CONFIG_SOC_SERIES_NRF52X)
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
	NRF_CCM->EVENTS_ENDKSGEN = 0;
	NRF_CCM->EVENTS_ENDCRYPT = 0;
	NRF_CCM->EVENTS_ERROR = 0;

	NRF_CCM->TASKS_KSGEN = 1;

	return _pkt_scratch;
}

u32_t radio_ccm_is_done(void)
{
	NRF_CCM->INTENSET = CCM_INTENSET_ENDCRYPT_Msk;
	while (NRF_CCM->EVENTS_ENDCRYPT == 0) {
		__WFE();
		__SEV();
		__WFE();
	}
	NRF_CCM->INTENCLR = CCM_INTENCLR_ENDCRYPT_Msk;
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

	NRF_PPI->CH[6].EEP = (u32_t)&(NRF_RADIO->EVENTS_BCMATCH);
	NRF_PPI->CH[6].TEP = (u32_t)&(NRF_AAR->TASKS_START);
	NRF_PPI->CHENSET = PPI_CHEN_CH6_Msk;
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
