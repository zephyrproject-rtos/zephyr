/*
 * Copyright (c) 2016 - 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/dlist.h>
#include <zephyr/toolchain.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <soc.h>

#include <hal/nrf_rtc.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_ccm.h>
#include <hal/nrf_aar.h>

#include "util/mem.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/radio_df.h"
#include "hal/ticker.h"

#include "ll_sw/pdu_df.h"
#include "lll/pdu_vendor.h"
#include "ll_sw/pdu.h"

#include "radio_internal.h"

/* Converts the GPIO controller in a FEM property's GPIO specification
 * to its nRF register map pointer.
 *
 * Make sure to use NRF_DT_CHECK_GPIO_CTLR_IS_SOC to check the GPIO
 * controller has the right compatible wherever you use this.
 */
#define NRF_FEM_GPIO(prop) \
	((NRF_GPIO_Type *)DT_REG_ADDR(DT_GPIO_CTLR(FEM_NODE, prop)))

/* Converts GPIO specification to a PSEL value. */
#define NRF_FEM_PSEL(prop) NRF_DT_GPIOS_TO_PSEL(FEM_NODE, prop)

/* Check if GPIO flags are active low. */
#define ACTIVE_LOW(flags) ((flags) & GPIO_ACTIVE_LOW)

/* Check if GPIO flags contain unsupported values. */
#define BAD_FLAGS(flags) ((flags) & ~GPIO_ACTIVE_LOW)

/* GPIOTE OUTINIT setting for a pin's inactive level, from its
 * devicetree flags.
 */
#define OUTINIT_INACTIVE(flags)			\
	(ACTIVE_LOW(flags) ?				\
	 GPIOTE_CONFIG_OUTINIT_High :			\
	 GPIOTE_CONFIG_OUTINIT_Low)

#if defined(FEM_NODE)
BUILD_ASSERT(!HAL_RADIO_GPIO_PA_OFFSET_MISSING,
	     "fem node " DT_NODE_PATH(FEM_NODE) " has property "
	     HAL_RADIO_GPIO_PA_PROP_NAME " set, so you must also set "
	     HAL_RADIO_GPIO_PA_OFFSET_PROP_NAME);

BUILD_ASSERT(!HAL_RADIO_GPIO_LNA_OFFSET_MISSING,
	     "fem node " DT_NODE_PATH(FEM_NODE) " has property "
	     HAL_RADIO_GPIO_LNA_PROP_NAME " set, so you must also set "
	     HAL_RADIO_GPIO_LNA_OFFSET_PROP_NAME);
#endif	/* FEM_NODE */

/*
 * "Manual" conversions of devicetree values to register bits. We
 * can't use the Zephyr GPIO API here, so we need this extra
 * boilerplate.
 */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
#define NRF_GPIO_PA       NRF_FEM_GPIO(HAL_RADIO_GPIO_PA_PROP)
#define NRF_GPIO_PA_PIN   DT_GPIO_PIN(FEM_NODE, HAL_RADIO_GPIO_PA_PROP)
#define NRF_GPIO_PA_FLAGS DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_PA_PROP)
#define NRF_GPIO_PA_PSEL  NRF_FEM_PSEL(HAL_RADIO_GPIO_PA_PROP)
NRF_DT_CHECK_GPIO_CTLR_IS_SOC(FEM_NODE, HAL_RADIO_GPIO_PA_PROP,
			      HAL_RADIO_GPIO_PA_PROP_NAME);
BUILD_ASSERT(!BAD_FLAGS(NRF_GPIO_PA_FLAGS),
	     "fem node " DT_NODE_PATH(FEM_NODE) " has invalid GPIO flags in "
	     HAL_RADIO_GPIO_PA_PROP_NAME
	     "; only GPIO_ACTIVE_LOW or GPIO_ACTIVE_HIGH are supported");
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
#define NRF_GPIO_LNA       NRF_FEM_GPIO(HAL_RADIO_GPIO_LNA_PROP)
#define NRF_GPIO_LNA_PIN   DT_GPIO_PIN(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP)
#define NRF_GPIO_LNA_FLAGS DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP)
#define NRF_GPIO_LNA_PSEL  NRF_FEM_PSEL(HAL_RADIO_GPIO_LNA_PROP)
NRF_DT_CHECK_GPIO_CTLR_IS_SOC(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP,
			      HAL_RADIO_GPIO_LNA_PROP_NAME);
BUILD_ASSERT(!BAD_FLAGS(NRF_GPIO_LNA_FLAGS),
	     "fem node " DT_NODE_PATH(FEM_NODE) " has invalid GPIO flags in "
	     HAL_RADIO_GPIO_LNA_PROP_NAME
	     "; only GPIO_ACTIVE_LOW or GPIO_ACTIVE_HIGH are supported");
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(HAL_RADIO_FEM_IS_NRF21540)

#if DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios)
#define NRF_GPIO_PDN        NRF_FEM_GPIO(pdn_gpios)
#define NRF_GPIO_PDN_PIN    DT_GPIO_PIN(FEM_NODE, pdn_gpios)
#define NRF_GPIO_PDN_FLAGS  DT_GPIO_FLAGS(FEM_NODE, pdn_gpios)
#define NRF_GPIO_PDN_PSEL   NRF_FEM_PSEL(pdn_gpios)
#define NRF_GPIO_PDN_OFFSET DT_PROP(FEM_NODE, pdn_settle_time_us)
NRF_DT_CHECK_GPIO_CTLR_IS_SOC(FEM_NODE, pdn_gpios, "pdn-gpios");
#endif	/* DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios) */

/* CSN is special because it comes from the spi-if property. */
#if defined(HAL_RADIO_FEM_NRF21540_HAS_CSN)
#define NRF_GPIO_CSN_CTLR  DT_SPI_DEV_CS_GPIOS_CTLR(FEM_SPI_DEV_NODE)
#define NRF_GPIO_CSN       ((NRF_GPIO_Type *)DT_REG_ADDR(NRF_GPIO_CSN_CTLR))
#define NRF_GPIO_CSN_PIN   DT_SPI_DEV_CS_GPIOS_PIN(FEM_SPI_DEV_NODE)
#define NRF_GPIO_CSN_FLAGS DT_SPI_DEV_CS_GPIOS_FLAGS(FEM_SPI_DEV_NODE)
#define NRF_GPIO_CSN_PSEL  (NRF_GPIO_CSN_PIN + \
			    (DT_PROP(NRF_GPIO_CSN_CTLR, port) << 5))
BUILD_ASSERT(DT_NODE_HAS_COMPAT(NRF_GPIO_CSN_CTLR, nordic_nrf_gpio),
	     "fem node " DT_NODE_PATH(FEM_NODE) " has a spi-if property, "
	     " but the chip select pin is not on the SoC. Check cs-gpios in "
	     DT_NODE_PATH(DT_BUS(FEM_SPI_DEV_NODE)));
#endif	/* HAL_RADIO_FEM_NRF21540_HAS_CSN */

#endif	/* HAL_RADIO_FEM_IS_NRF21540 */

/* CTEINLINE S0_MASK for periodic advertising PUDs. It allows to accept all types of extended
 * advertising PDUs to have CTE included.
 */
#define DF_S0_ALLOW_ALL_PER_ADV_PDU 0x0
/* CTEINLINE S0_MASK for data channel PDUs. It points to CP bit in S0 byte to check if is it set
 * to 0x1. In that is true then S1 byte (CTEInfo) is considered as present in a PDU.
 */
#define DF_S0_MASK_CP_BIT_IN_DATA_CHANNEL_PDU 0x20

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

	nrf_radio_int_enable(NRF_RADIO,
			     0 |
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
#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	NRF_GPIO_PA->DIRSET = BIT(NRF_GPIO_PA_PIN);
	if (ACTIVE_LOW(NRF_GPIO_PA_FLAGS)) {
		NRF_GPIO_PA->OUTSET = BIT(NRF_GPIO_PA_PIN);
	} else {
		NRF_GPIO_PA->OUTCLR = BIT(NRF_GPIO_PA_PIN);
	}
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	NRF_GPIO_LNA->DIRSET = BIT(NRF_GPIO_LNA_PIN);

	radio_gpio_lna_off();
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(NRF_GPIO_PDN_PIN)
	NRF_GPIO_PDN->DIRSET = BIT(NRF_GPIO_PDN_PIN);
	if (ACTIVE_LOW(NRF_GPIO_PDN_FLAGS)) {
		NRF_GPIO_PDN->OUTSET = BIT(NRF_GPIO_PDN_PIN);
	} else {
		NRF_GPIO_PDN->OUTCLR = BIT(NRF_GPIO_PDN_PIN);
	}
#endif /* NRF_GPIO_PDN_PIN */

#if defined(NRF_GPIO_CSN_PIN)
	NRF_GPIO_CSN->DIRSET = BIT(NRF_GPIO_CSN_PIN);
	if (ACTIVE_LOW(NRF_GPIO_CSN_FLAGS)) {
		NRF_GPIO_CSN->OUTSET = BIT(NRF_GPIO_CSN_PIN);
	} else {
		NRF_GPIO_CSN->OUTCLR = BIT(NRF_GPIO_CSN_PIN);
	}
#endif /* NRF_GPIO_CSN_PIN */

	hal_radio_ram_prio_setup();
}

void radio_reset(void)
{
	irq_disable(RADIO_IRQn);

	/* nRF SoC generic radio reset/initializations
	 * Note: Only registers whose bits are partially modified across
	 *       functions are assigned back the power-on reset values.
	 *       Ignore other registers for reset which will have all bits
	 *       explicitly assigned by functions in this file.
	 */
	NRF_RADIO->PCNF1 = HAL_RADIO_RESET_VALUE_PCNF1;

#if defined(CONFIG_BT_CTLR_DF) && !defined(CONFIG_ZTEST)
	radio_df_reset();
#endif /* CONFIG_BT_CTLR_DF && !CONFIG_ZTEST */

	/* nRF SoC specific reset/initializations, if any */
	hal_radio_reset();

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	hal_radio_sw_switch_ppi_group_setup();
#endif

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	hal_palna_ppi_setup();
#endif
#if defined(HAL_RADIO_FEM_IS_NRF21540)
	hal_fem_ppi_setup();
#endif
}

void radio_stop(void)
{
	/* nRF SoC specific radio stop/cleanup, if any */
	hal_radio_stop();

	/* nRF SoC generic radio stop/cleanup
	 * TODO: Initialize NRF_RADIO registers used by Controller to power-on
	 *       reset values.
	 *       This is required in case NRF_RADIO is share/used between
	 *       Bluetooth radio events by application defined radio protocols.
	 *       The application too shall restore the registers it uses to the
	 *       power-on reset values once it has stopped using the radio.
	 *
	 *       Registers used for Bluetooth Low Energy Controller:
	 *       - MODE
	 *       - MODECNF0
	 *       - TXPOWER
	 *       - FREQUENCY
	 *       - DATAWHITEIV
	 *       - PCNF0
	 *       - PCNF1
	 *       - TXADDRESS
	 *       - RXADDRESSES
	 *       - PREFIX0
	 *       - BASE0
	 *       - PACKETPTR
	 *       - CRCCNF
	 *       - CRCPOLY
	 *       - CRCINIT
	 *       - DAB
	 *       - DAP
	 *       - DACNF
	 *       - BCC
	 *       - TIFS
	 *       - SHORTS
	 *
	 *       Additional registers used for Direction Finding feature:
	 *       - SWITCHPATTERN
	 *       - DFEMODE
	 *       - CTEINLINECONF
	 *       - DFECTRL1
	 *       - DEFCTRL2
	 *       - CLEARPATTERN
	 *       - PSEL.DFEGPIO[n]
	 *       - DFEPACKET.PTR
	 */
}

void radio_phy_set(uint8_t phy, uint8_t flags)
{
	uint32_t mode;

	mode = hal_radio_phy_mode_get(phy, flags);

	NRF_RADIO->MODE = (mode << RADIO_MODE_MODE_Pos) & RADIO_MODE_MODE_Msk;

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
	NRF_RADIO->MODECNF0 = ((RADIO_MODECNF0_DTX_Center <<
				RADIO_MODECNF0_DTX_Pos) &
			       RADIO_MODECNF0_DTX_Msk) |
			      ((RADIO_MODECNF0_RU_Fast <<
				RADIO_MODECNF0_RU_Pos) &
			       RADIO_MODECNF0_RU_Msk);
#else /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
#if !defined(CONFIG_SOC_SERIES_NRF51X)
	NRF_RADIO->MODECNF0 = (RADIO_MODECNF0_DTX_Center <<
			       RADIO_MODECNF0_DTX_Pos) &
			      RADIO_MODECNF0_DTX_Msk;
#endif /* !CONFIG_SOC_SERIES_NRF51X */
#endif /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
}

void radio_tx_power_set(int8_t power)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	uint32_t value;

	/* NOTE: TXPOWER register only accepts upto 0dBm, hence use the HAL
	 * floor value for the TXPOWER register. Permit +3dBm by using high
	 * voltage being set for radio.
	 */
	value = hal_radio_tx_power_floor(power);
	NRF_RADIO->TXPOWER = value;
	hal_radio_tx_power_high_voltage_set(power);

#else /* !CONFIG_SOC_SERIES_NRF53X */

	/* NOTE: valid value range is passed by Kconfig define. */
	NRF_RADIO->TXPOWER = (uint32_t)power;

#endif /* !CONFIG_SOC_SERIES_NRF53X */
}

void radio_tx_power_max_set(void)
{
	int8_t power;

	power = radio_tx_power_max_get();
	radio_tx_power_set(power);
}

int8_t radio_tx_power_min_get(void)
{
	return (int8_t)hal_radio_tx_power_min_get();
}

int8_t radio_tx_power_max_get(void)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	return RADIO_TXPOWER_TXPOWER_Pos3dBm;

#else /* !CONFIG_SOC_SERIES_NRF53X */
	return (int8_t)hal_radio_tx_power_max_get();

#endif /* !CONFIG_SOC_SERIES_NRF53X */
}

int8_t radio_tx_power_floor(int8_t power)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* NOTE: TXPOWER register only accepts upto 0dBm, +3dBm permitted by
	 * use of high voltage being set for radio when TXPOWER register is set.
	 */
	if (power >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos3dBm;
	}
#endif /* CONFIG_SOC_SERIES_NRF53X */

	return (int8_t)hal_radio_tx_power_floor(power);
}

void radio_freq_chan_set(uint32_t chan)
{
	NRF_RADIO->FREQUENCY = chan;
}

void radio_whiten_iv_set(uint32_t iv)
{
	NRF_RADIO->DATAWHITEIV = iv;

	NRF_RADIO->PCNF1 &= ~RADIO_PCNF1_WHITEEN_Msk;
	NRF_RADIO->PCNF1 |= ((1UL) << RADIO_PCNF1_WHITEEN_Pos) &
			    RADIO_PCNF1_WHITEEN_Msk;
}

void radio_aa_set(uint8_t *aa)
{
	NRF_RADIO->TXADDRESS =
	    (((0UL) << RADIO_TXADDRESS_TXADDRESS_Pos) &
	     RADIO_TXADDRESS_TXADDRESS_Msk);
	NRF_RADIO->RXADDRESSES =
	    ((RADIO_RXADDRESSES_ADDR0_Enabled) << RADIO_RXADDRESSES_ADDR0_Pos);
	NRF_RADIO->PREFIX0 = aa[3];
	NRF_RADIO->BASE0 = (aa[2] << 24) | (aa[1] << 16) | (aa[0] << 8);
}

void radio_pkt_configure(uint8_t bits_len, uint8_t max_len, uint8_t flags)
{
	const uint8_t pdu_type = RADIO_PKT_CONF_PDU_TYPE_GET(flags); /* Adv or Data channel */
	uint8_t bits_s1;
	uint32_t extra;
	uint8_t phy;

#if defined(CONFIG_SOC_SERIES_NRF51X)
	ARG_UNUSED(phy);

	extra = 0U;

	/* nRF51 supports only 27 byte PDU when using h/w CCM for encryption. */
	if (!IS_ENABLED(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR) &&
	    pdu_type == RADIO_PKT_CONF_PDU_TYPE_DC) {
		bits_len = RADIO_PKT_CONF_LENGTH_5BIT;
	}
	bits_s1 = RADIO_PKT_CONF_LENGTH_8BIT - bits_len;

#elif defined(CONFIG_SOC_COMPATIBLE_NRF52X) || \
	defined(CONFIG_SOC_SERIES_NRF53X)
	extra = 0U;

	phy = RADIO_PKT_CONF_PHY_GET(flags);
	switch (phy) {
	case PHY_1M:
	default:
		extra |= (RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos) &
			 RADIO_PCNF0_PLEN_Msk;
		break;

	case PHY_2M:
		extra |= (RADIO_PCNF0_PLEN_16bit << RADIO_PCNF0_PLEN_Pos) &
			 RADIO_PCNF0_PLEN_Msk;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	case PHY_CODED:
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
	if (pdu_type == RADIO_PKT_CONF_PDU_TYPE_DC) {
		extra |= (RADIO_PCNF0_S1INCL_Include <<
			  RADIO_PCNF0_S1INCL_Pos) & RADIO_PCNF0_S1INCL_Msk;
#if defined(CONFIG_BT_CTLR_DF)
		if (RADIO_PKT_CONF_CTE_GET(flags) == RADIO_PKT_CONF_CTE_ENABLED) {
			bits_s1 = RADIO_PKT_CONF_S1_8BIT;
		} else
#endif /* CONFIG_BT_CTLR_DF */
		{
			bits_s1 = 0U;
		}
	} else {
		bits_s1 = 0U;
	}
#endif /* CONFIG_SOC_COMPATIBLE_NRF52X */

	NRF_RADIO->PCNF0 =
		(((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |
		((((uint32_t)bits_len) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk) |
		((((uint32_t)bits_s1) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) | extra;

	NRF_RADIO->PCNF1 &= ~(RADIO_PCNF1_MAXLEN_Msk | RADIO_PCNF1_STATLEN_Msk |
			      RADIO_PCNF1_BALEN_Msk | RADIO_PCNF1_ENDIAN_Msk);
	NRF_RADIO->PCNF1 |=
		((((uint32_t)max_len) << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk) |
		(((0UL) << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk) |
		(((3UL) << RADIO_PCNF1_BALEN_Pos) & RADIO_PCNF1_BALEN_Msk) |
		(((RADIO_PCNF1_ENDIAN_Little) << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk);
}

void radio_pkt_rx_set(void *rx_packet)
{
	NRF_RADIO->PACKETPTR = (uint32_t)rx_packet;
}

void radio_pkt_tx_set(void *tx_packet)
{
	NRF_RADIO->PACKETPTR = (uint32_t)tx_packet;
}

uint32_t radio_tx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return hal_radio_tx_ready_delay_us_get(phy, flags);
}

uint32_t radio_tx_chain_delay_get(uint8_t phy, uint8_t flags)
{
	return hal_radio_tx_chain_delay_us_get(phy, flags);
}

uint32_t radio_rx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return hal_radio_rx_ready_delay_us_get(phy, flags);
}

uint32_t radio_rx_chain_delay_get(uint8_t phy, uint8_t flags)
{
	return hal_radio_rx_chain_delay_us_get(phy, flags);
}

void radio_rx_enable(void)
{
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* NOTE: Timer clear DPPI configuration is needed only for nRF53
	 *       because of calls to radio_disable() and
	 *       radio_switch_complete_and_disable() inside a radio event call
	 *       hal_radio_sw_switch_disable(), which in the case of nRF53
	 *       cancels the task subscription.
	 */
	/* FIXME: hal_sw_switch_timer_clear_ppi_config() sets both task and
	 *        event. Consider a new interface to only set the task, or
	 *        change the design to not clear task subscription inside a
	 *        radio event but when the radio event is done.
	 */
	hal_sw_switch_timer_clear_ppi_config();
#endif /* CONFIG_SOC_SERIES_NRF53X */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
}

void radio_tx_enable(void)
{
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* NOTE: Timer clear DPPI configuration is needed only for nRF53
	 *       because of calls to radio_disable() and
	 *       radio_switch_complete_and_disable() inside a radio event call
	 *       hal_radio_sw_switch_disable(), which in the case of nRF53
	 *       cancels the task subscription.
	 */
	/* FIXME: hal_sw_switch_timer_clear_ppi_config() sets both task and
	 *        event. Consider a new interface to only set the task, or
	 *        change the design to not clear task subscription inside a
	 *        radio event but when the radio event is done.
	 */
	hal_sw_switch_timer_clear_ppi_config();
#endif /* CONFIG_SOC_SERIES_NRF53X */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_TXEN);
}

void radio_disable(void)
{
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	hal_radio_sw_switch_cleanup();
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	NRF_RADIO->SHORTS = 0;
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
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
#if defined(CONFIG_BT_CTLR_DF_SUPPORT) && !defined(CONFIG_ZTEST)
	/* Clear it only for SoCs supporting DF extension */
	NRF_RADIO->EVENTS_PHYEND = 0;
	NRF_RADIO->EVENTS_CTEPRESENT = 0;
	NRF_RADIO->EVENTS_BCMATCH = 0;
#endif /* CONFIG_BT_CTLR_DF_SUPPORT && !CONFIG_ZTEST */
	NRF_RADIO->EVENTS_DISABLED = 0;
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	NRF_RADIO->EVENTS_RATEBOOST = 0;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
}

uint32_t radio_is_ready(void)
{
	return (NRF_RADIO->EVENTS_READY != 0);
}

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
static uint32_t last_pdu_end_us;

uint32_t radio_is_done(void)
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
uint32_t radio_is_done(void)
{
	return (NRF_RADIO->NRF_RADIO_TXRX_END_EVENT != 0);
}
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

uint32_t radio_has_disabled(void)
{
	return (NRF_RADIO->EVENTS_DISABLED != 0);
}

uint32_t radio_is_idle(void)
{
	return (NRF_RADIO->STATE == 0);
}

void radio_crc_configure(uint32_t polynomial, uint32_t iv)
{
	NRF_RADIO->CRCCNF =
	    (((RADIO_CRCCNF_SKIPADDR_Skip) << RADIO_CRCCNF_SKIPADDR_Pos) &
	     RADIO_CRCCNF_SKIPADDR_Msk) |
	    (((RADIO_CRCCNF_LEN_Three) << RADIO_CRCCNF_LEN_Pos) &
	       RADIO_CRCCNF_LEN_Msk);
	NRF_RADIO->CRCPOLY = polynomial;
	NRF_RADIO->CRCINIT = iv;
}

uint32_t radio_crc_is_valid(void)
{
	return (NRF_RADIO->CRCSTATUS != 0);
}

static uint8_t MALIGN(4) _pkt_empty[PDU_EM_LL_SIZE_MAX];
static uint8_t MALIGN(4) _pkt_scratch[MAX((HAL_RADIO_PDU_LEN_MAX + 3),
				       PDU_AC_LL_SIZE_MAX)];

void *radio_pkt_empty_get(void)
{
	return _pkt_empty;
}

void *radio_pkt_scratch_get(void)
{
	return _pkt_scratch;
}

#if defined(CONFIG_SOC_NRF52832) && \
	defined(CONFIG_BT_CTLR_LE_ENC) && \
	defined(HAL_RADIO_PDU_LEN_MAX) && \
	(!defined(CONFIG_BT_CTLR_DATA_LENGTH_MAX) || \
	 (CONFIG_BT_CTLR_DATA_LENGTH_MAX < (HAL_RADIO_PDU_LEN_MAX - 4)))
static uint8_t MALIGN(4) _pkt_decrypt[MAX((HAL_RADIO_PDU_LEN_MAX + 3),
				       PDU_AC_LL_SIZE_MAX)];

void *radio_pkt_decrypt_get(void)
{
	return _pkt_decrypt;
}
#elif !defined(HAL_RADIO_PDU_LEN_MAX)
#error "Undefined HAL_RADIO_PDU_LEN_MAX."
#endif

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_SYNC_ISO)
/* Dedicated Rx PDU Buffer for Control PDU independent of node_rx with BIS Data
 * PDU buffer
 */
static uint8_t pkt_big_ctrl[sizeof(struct pdu_big_ctrl)];

void *radio_pkt_big_ctrl_get(void)
{
	return pkt_big_ctrl;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_SYNC_ISO */

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
static uint8_t sw_tifs_toggle;
/**
 * @brief Implementation of Radio operation software switch.
 *
 * In case the switch is from RX to TX or from RX to RX and CTEINLINE is enabled then PDU end event
 * (EVENTS_PHYEND) may be delayed after actual PDU end. The delay occurs when received PDU does not
 * include CTE. To maintain TIFS the delay must be compensated.
 * To handle that, two timer EVENTS_COMPARE are prepared: without and without delay compensation.
 * If EVENTS_CTEPRESENT is fired then EVENTS_COMPARE for delayed EVENTS_PHYEND event is cancelled.
 * In other case EVENTS_COMPARE for delayed compensation will timeout first and disable group of
 * PPIs related with the Radio operation switch.
 * Enable of end event compensation is controller by @p end_evt_delay_en.
 *
 * @param dir_curr         Current direction the Radio is working: SW_SWITCH_TX or SW_SWITCH_RX
 * @param dir_next         Next direction the Radio is preparing for: SW_SWITCH_TX or SW_SWITCH_RX
 * @param phy_curr         PHY the Radio is working on.
 * @param flags_curr       Flags related with current PHY, the Radio is working on.
 * @param phy_next         Next PHY the Radio is preparing for.
 * @param flags_next       Flags related with next PHY, the Radio is preparing for.
 * @param end_evt_delay_en Enable end event delay compensation for TIFS after switch from current
 *                         direction to next direction.
 */
void sw_switch(uint8_t dir_curr, uint8_t dir_next, uint8_t phy_curr, uint8_t flags_curr,
	       uint8_t phy_next, uint8_t flags_next,
	       enum radio_end_evt_delay_state end_evt_delay_en)
{
	uint8_t ppi = HAL_SW_SWITCH_RADIO_ENABLE_PPI(sw_tifs_toggle);
	uint8_t cc = SW_SWITCH_TIMER_EVTS_COMP(sw_tifs_toggle);
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
	uint8_t phyend_delay_cc =
		SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_EVTS_COMP(sw_tifs_toggle);
	uint8_t radio_enable_ppi =
		HAL_SW_SWITCH_RADIO_ENABLE_PHYEND_DELAY_COMPENSATION_PPI(sw_tifs_toggle);
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
	uint32_t delay;

	hal_radio_sw_switch_setup(cc, ppi, sw_tifs_toggle);

	/* NOTE: As constants are passed to dir_curr and dir_next, the
	 *       compiler should optimize out the redundant code path
	 *       during the optimization.
	 */
	if (dir_next == SW_SWITCH_TX) {
		/* TX */

		/* Calculate delay with respect to current and next PHY.
		 */
		if (dir_curr == SW_SWITCH_TX) {
			delay = HAL_RADIO_NS2US_ROUND(
			    hal_radio_tx_ready_delay_ns_get(phy_next,
							    flags_next) +
			    hal_radio_tx_chain_delay_ns_get(phy_curr,
							    flags_curr));

			hal_radio_b2b_txen_on_sw_switch(ppi);
		} else {
			/* If RX PHY is LE Coded, calculate for S8 coding.
			 * Assumption being, S8 has higher delay.
			 */
			delay = HAL_RADIO_NS2US_ROUND(
			    hal_radio_tx_ready_delay_ns_get(phy_next,
							    flags_next) +
			    hal_radio_rx_chain_delay_ns_get(phy_curr, 1));

			hal_radio_txen_on_sw_switch(ppi);
		}

#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		if (dir_curr == SW_SWITCH_RX && end_evt_delay_en == END_EVT_DELAY_ENABLED &&
		    !(phy_curr & PHY_CODED)) {
			SW_SWITCH_TIMER->CC[phyend_delay_cc] =
				SW_SWITCH_TIMER->CC[cc] - RADIO_EVENTS_PHYEND_DELAY_US;
			if (delay < SW_SWITCH_TIMER->CC[cc]) {
				nrf_timer_cc_set(SW_SWITCH_TIMER, phyend_delay_cc,
						 (SW_SWITCH_TIMER->CC[phyend_delay_cc] - delay));
			} else {
				nrf_timer_cc_set(SW_SWITCH_TIMER, phyend_delay_cc, 1);
			}

			hal_radio_sw_switch_phyend_delay_compensation_config_set(radio_enable_ppi,
										 phyend_delay_cc);
		} else {
			hal_radio_sw_switch_phyend_delay_compensation_config_clear(radio_enable_ppi,
										   phyend_delay_cc);
		}
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
		uint8_t ppi_en =
			HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(sw_tifs_toggle);
		uint8_t ppi_dis =
			HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(sw_tifs_toggle);

		if (dir_curr == SW_SWITCH_RX && (phy_curr & PHY_CODED)) {
			/* Switching to TX after RX on LE Coded PHY. */

			uint8_t cc_s2 =
			    SW_SWITCH_TIMER_S2_EVTS_COMP(sw_tifs_toggle);

			uint32_t delay_s2;

			/* Calculate assuming reception on S2 coding scheme. */
			delay_s2 = HAL_RADIO_NS2US_ROUND(
				hal_radio_tx_ready_delay_ns_get(phy_next,
								flags_next) +
				hal_radio_rx_chain_delay_ns_get(phy_curr, 0));

			SW_SWITCH_TIMER->CC[cc_s2] = SW_SWITCH_TIMER->CC[cc];

			if (delay_s2 < SW_SWITCH_TIMER->CC[cc_s2]) {
				SW_SWITCH_TIMER->CC[cc_s2] -= delay_s2;
			} else {
				SW_SWITCH_TIMER->CC[cc_s2] = 1;
			}

			/* Setup the Tx start for S2 using a dedicated compare,
			 * setup a PPI to disable PPI group on that compare
			 * event, and then importantly setup a capture PPI to
			 * disable the Tx start for S8 on RATEBOOST event.
			 */
			hal_radio_sw_switch_coded_tx_config_set(ppi_en, ppi_dis,
				cc_s2, sw_tifs_toggle);

		} else {
			/* Switching to TX after RX on LE 1M/2M PHY.
			 *
			 * NOTE: PHYEND delay compensation and switching between Coded S2 and S8 PHY
			 *       may not happen at once. PHYEND delay may not happen when Code PHY
			 *       is used. Both functionalities use the same EVENTS_COMPARE channels,
			 *       hence when PHYEND delay is applied, coded config clear may not be
			 *       called.
			 *
			 * TODO: This has to be refactored. It is temporarily implemented this way
			 *       because the code is very fragile and hard to debug.
			 */
			if (end_evt_delay_en != END_EVT_DELAY_ENABLED) {
				hal_radio_sw_switch_coded_config_clear(ppi_en, ppi_dis, cc,
								       sw_tifs_toggle);
			}

			hal_radio_sw_switch_disable_group_clear(ppi_dis, cc, sw_tifs_toggle);
		}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	} else {
		/* RX */

		/* Calculate delay with respect to current and next PHY. */
		if (dir_curr) {
			delay = HAL_RADIO_NS2US_CEIL(
				hal_radio_rx_ready_delay_ns_get(phy_next,
								flags_next) +
				hal_radio_tx_chain_delay_ns_get(phy_curr,
								flags_curr)) +
				(EVENT_CLOCK_JITTER_US << 1);

			hal_radio_rxen_on_sw_switch(ppi);
		} else {
			delay = HAL_RADIO_NS2US_CEIL(
				hal_radio_rx_ready_delay_ns_get(phy_next,
								flags_next) +
				hal_radio_rx_chain_delay_ns_get(phy_curr,
								flags_curr)) +
				(EVENT_CLOCK_JITTER_US << 1);

			hal_radio_b2b_rxen_on_sw_switch(ppi);
		}


#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		hal_radio_sw_switch_phyend_delay_compensation_config_clear(radio_enable_ppi,
									   phyend_delay_cc);
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
		if (1) {
			uint8_t ppi_en = HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(
						sw_tifs_toggle);
			uint8_t ppi_dis = HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(
						sw_tifs_toggle);

			hal_radio_sw_switch_coded_config_clear(ppi_en,
				ppi_dis, cc, sw_tifs_toggle);
			hal_radio_sw_switch_disable_group_clear(ppi_dis, cc, sw_tifs_toggle);
		}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}

	if (delay < SW_SWITCH_TIMER->CC[cc]) {
		nrf_timer_cc_set(SW_SWITCH_TIMER, cc,
				 (SW_SWITCH_TIMER->CC[cc] - delay));
	} else {
		nrf_timer_cc_set(SW_SWITCH_TIMER, cc, 1);
	}

	hal_radio_nrf_ppi_channels_enable(BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
				BIT(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI));

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* NOTE: For single timer configuration nRF5340 shares the DPPI channel being
	 *       triggered by Radio End for End time capture and sw_switch DPPI channel toggling
	 *       hence always need to capture End time. Or when using single event timer since
	 *       the timer is cleared on Radio End, we always need to capture the Radio End
	 *       time-stamp.
	 */
	hal_radio_end_time_capture_ppi_config();
#if !defined(CONFIG_SOC_SERIES_NRF53X)
	/* The function is not called for nRF5340 single timer configuration because
	 * HAL_SW_SWITCH_TIMER_CLEAR_PPI is equal to HAL_RADIO_END_TIME_CAPTURE_PPI,
	 * so channel is already enabled.
	 */
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_RADIO_END_TIME_CAPTURE_PPI));
#endif /* !CONFIG_SOC_SERIES_NRF53X */
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	sw_tifs_toggle += 1U;
	sw_tifs_toggle &= 1U;
}
#endif /* CONFIG_BT_CTLR_TIFS_HW */

void radio_switch_complete_and_rx(uint8_t phy_rx)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_RXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE;

	/* NOTE: As Tx chain delays are negligible constant values (~1 us)
	 *	 across nRF5x radios, sw_switch assumes the 1M chain delay for
	 *       calculations.
	 */
	sw_switch(SW_SWITCH_TX, SW_SWITCH_RX, SW_SWITCH_PHY_1M, SW_SWITCH_FLAGS_DONTCARE, phy_rx,
		  SW_SWITCH_FLAGS_DONTCARE, END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_tx(uint8_t phy_rx, uint8_t flags_rx,
				  uint8_t phy_tx, uint8_t flags_tx)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE;

	sw_switch(SW_SWITCH_RX, SW_SWITCH_TX, phy_rx, flags_rx, phy_tx, flags_tx,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_with_delay_compensation_and_tx(
	uint8_t phy_rx, uint8_t flags_rx, uint8_t phy_tx, uint8_t flags_tx,
	enum radio_end_evt_delay_state end_delay_en)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE;

	sw_switch(SW_SWITCH_RX, SW_SWITCH_TX, phy_rx, flags_rx, phy_tx, flags_tx, end_delay_en);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_b2b_tx(uint8_t phy_curr, uint8_t flags_curr,
				      uint8_t phy_next, uint8_t flags_next)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE;

	sw_switch(SW_SWITCH_TX, SW_SWITCH_TX, phy_curr, flags_curr, phy_next, flags_next,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_b2b_rx(uint8_t phy_curr, uint8_t flags_curr,
				      uint8_t phy_next, uint8_t flags_next)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_RXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE;

	sw_switch(SW_SWITCH_RX, SW_SWITCH_RX, phy_curr, flags_curr, phy_next, flags_next,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_disable(void)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk);
#else /* CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE);
	hal_radio_sw_switch_disable();
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

uint8_t radio_phy_flags_rx_get(void)
{
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	return (NRF_RADIO->EVENTS_RATEBOOST) ? 0U : 1U;
#else /* !CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
	return 0;
#endif /* !CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#else /* !CONFIG_BT_CTLR_PHY_CODED */
	return 0;
#endif /* !CONFIG_BT_CTLR_PHY_CODED */
}

void radio_rssi_measure(void)
{
	NRF_RADIO->SHORTS |=
	    (RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
	     RADIO_SHORTS_DISABLED_RSSISTOP_Msk);
}

uint32_t radio_rssi_get(void)
{
	return NRF_RADIO->RSSISAMPLE;
}

void radio_rssi_status_reset(void)
{
	NRF_RADIO->EVENTS_RSSIEND = 0;
}

uint32_t radio_rssi_is_ready(void)
{
	return (NRF_RADIO->EVENTS_RSSIEND != 0);
}

void radio_filter_configure(uint8_t bitmask_enable, uint8_t bitmask_addr_type,
			    uint8_t *bdaddr)
{
	uint8_t index;

	for (index = 0U; index < 8; index++) {
		NRF_RADIO->DAB[index] = ((uint32_t)bdaddr[3] << 24) |
			((uint32_t)bdaddr[2] << 16) |
			((uint32_t)bdaddr[1] << 8) |
			bdaddr[0];
		NRF_RADIO->DAP[index] = ((uint32_t)bdaddr[5] << 8) | bdaddr[4];
		bdaddr += 6;
	}

	NRF_RADIO->DACNF = ((uint32_t)bitmask_addr_type << 8) | bitmask_enable;
}

void radio_filter_disable(void)
{
	NRF_RADIO->DACNF &= ~(0x000000FF);
}

void radio_filter_status_reset(void)
{
	NRF_RADIO->EVENTS_DEVMATCH = 0;
}

uint32_t radio_filter_has_match(void)
{
	return (NRF_RADIO->EVENTS_DEVMATCH != 0);
}

uint32_t radio_filter_match_get(void)
{
	return NRF_RADIO->DAI;
}

void radio_bc_configure(uint32_t n)
{
	NRF_RADIO->BCC = n;
	NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_BCSTART_Msk;
}

void radio_bc_status_reset(void)
{
	NRF_RADIO->EVENTS_BCMATCH = 0;
}

uint32_t radio_bc_has_match(void)
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
#if defined(DPPI_PRESENT)
			BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
#endif /* DPPI_PRESENT */
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
			BIT(HAL_TRIGGER_RATEOVERRIDE_PPI) |
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
			BIT(HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI) |
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
			BIT(HAL_TRIGGER_CRYPT_DELAY_PPI) |
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
			BIT(HAL_TRIGGER_CRYPT_PPI));
}

void radio_tmr_tx_status_reset(void)
{
	nrf_rtc_event_disable(NRF_RTC0, RTC_EVTENCLR_COMPARE2_Msk);

	hal_radio_nrf_ppi_channels_disable(
#if (HAL_RADIO_ENABLE_TX_ON_TICK_PPI != HAL_RADIO_ENABLE_RX_ON_TICK_PPI) && \
	!defined(DPPI_PRESENT)
			BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI) |
#endif /* (HAL_RADIO_ENABLE_TX_ON_TICK_PPI !=
	*  HAL_RADIO_ENABLE_RX_ON_TICK_PPI) && !DPPI_PRESENT
	*/
			BIT(HAL_EVENT_TIMER_START_PPI) |
			BIT(HAL_RADIO_READY_TIME_CAPTURE_PPI) |
			BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) |
			BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI) |
			BIT(HAL_RADIO_END_TIME_CAPTURE_PPI) |
#if defined(DPPI_PRESENT)
			BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
#endif /* DPPI_PRESENT */
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
			BIT(HAL_TRIGGER_RATEOVERRIDE_PPI) |
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
			BIT(HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI) |
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
			BIT(HAL_TRIGGER_CRYPT_DELAY_PPI) |
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
			BIT(HAL_TRIGGER_CRYPT_PPI));
}

void radio_tmr_rx_status_reset(void)
{
	nrf_rtc_event_disable(NRF_RTC0, RTC_EVTENCLR_COMPARE2_Msk);

	hal_radio_nrf_ppi_channels_disable(
#if (HAL_RADIO_ENABLE_TX_ON_TICK_PPI != HAL_RADIO_ENABLE_RX_ON_TICK_PPI) && \
	!defined(DPPI_PRESENT)
			BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI) |
#endif /* (HAL_RADIO_ENABLE_TX_ON_TICK_PPI !=
	*  HAL_RADIO_ENABLE_RX_ON_TICK_PPI) && !DPPI_PRESENT
	*/
			BIT(HAL_EVENT_TIMER_START_PPI) |
			BIT(HAL_RADIO_READY_TIME_CAPTURE_PPI) |
			BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) |
			BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI) |
			BIT(HAL_RADIO_END_TIME_CAPTURE_PPI) |
#if defined(DPPI_PRESENT)
			BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
#endif /* DPPI_PRESENT */
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
			BIT(HAL_TRIGGER_RATEOVERRIDE_PPI) |
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
			BIT(HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI) |
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
			BIT(HAL_TRIGGER_CRYPT_DELAY_PPI) |
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
			BIT(HAL_TRIGGER_CRYPT_PPI));
}

void radio_tmr_tx_enable(void)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
#else /* !CONFIG_SOC_SERIES_NRF53X */
#if (HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI)
	hal_radio_enable_on_tick_ppi_config_and_enable(1U);
#endif /* HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI */
#endif /* !CONFIG_SOC_SERIES_NRF53X */
}

void radio_tmr_rx_enable(void)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
#else /* !CONFIG_SOC_SERIES_NRF53X */
#if (HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI)
	hal_radio_enable_on_tick_ppi_config_and_enable(0U);
#endif /* HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI */
#endif /* !CONFIG_SOC_SERIES_NRF53X */
}

void radio_tmr_tx_disable(void)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_TXEN);
#else /* !CONFIG_SOC_SERIES_NRF53X */
#endif /* !CONFIG_SOC_SERIES_NRF53X */
}

void radio_tmr_rx_disable(void)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_RXEN);
#else /* !CONFIG_SOC_SERIES_NRF53X */
#endif /* !CONFIG_SOC_SERIES_NRF53X */
}

void radio_tmr_tifs_set(uint32_t tifs)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->TIFS = tifs;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	nrf_timer_cc_set(SW_SWITCH_TIMER,
			 SW_SWITCH_TIMER_EVTS_COMP(sw_tifs_toggle), tifs);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

uint32_t radio_tmr_start(uint8_t trx, uint32_t ticks_start, uint32_t remainder)
{
	hal_ticker_remove_jitter(&ticks_start, &remainder);

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);
	EVENT_TIMER->MODE = 0;
	EVENT_TIMER->PRESCALER = 4;
	EVENT_TIMER->BITMODE = 2;	/* 24 - bit */

	nrf_timer_cc_set(EVENT_TIMER, 0, remainder);

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
	/* FIXME: start along with EVENT_TIMER, to save power */
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_START);
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	hal_sw_switch_timer_clear_ppi_config();

#if !defined(CONFIG_BT_CTLR_PHY_CODED) || \
	!defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	/* Software switch group's disable PPI can be configured one time here
	 * at timer setup when only 1M and/or 2M is supported.
	 */
	hal_radio_group_task_disable_ppi_setup();

#else /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
	/* Software switch group's disable PPI needs to be configured at every
	 * sw_switch() as they depend on the actual PHYs used in TX/RX mode.
	 */
#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	return remainder;
}

uint32_t radio_tmr_start_tick(uint8_t trx, uint32_t tick)
{
	uint32_t remainder_us;

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);

	/* Setup compare event with min. 1 us offset */
	remainder_us = 1;
	nrf_timer_cc_set(EVENT_TIMER, 0, remainder_us);

	nrf_rtc_cc_set(NRF_RTC0, 2, tick);
	nrf_rtc_event_enable(NRF_RTC0, RTC_EVTENSET_COMPARE2_Msk);

	hal_event_timer_start_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_EVENT_TIMER_START_PPI));

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	last_pdu_end_us = 0U;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* NOTE: Timer clear DPPI configuration is needed only for nRF53
	 *       because of calls to radio_disable() and
	 *       radio_switch_complete_and_disable() inside a radio event call
	 *       hal_radio_sw_switch_disable(), which in the case of nRF53
	 *       cancels the task subscription.
	 */
	/* FIXME: hal_sw_switch_timer_clear_ppi_config() sets both task and
	 *        event. Consider a new interface to only set the task, or
	 *        change the design to not clear task subscription inside a
	 *        radio event but when the radio event is done.
	 */
	hal_sw_switch_timer_clear_ppi_config();
#endif /* CONFIG_SOC_SERIES_NRF53X */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	return remainder_us;
}

uint32_t radio_tmr_start_us(uint8_t trx, uint32_t start_us)
{
	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	last_pdu_end_us = 0U;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* NOTE: Timer clear DPPI configuration is needed only for nRF53
	 *       because of calls to radio_disable() and
	 *       radio_switch_complete_and_disable() inside a radio event call
	 *       hal_radio_sw_switch_disable(), which in the case of nRF53
	 *       cancels the task subscription.
	 */
	/* FIXME: hal_sw_switch_timer_clear_ppi_config() sets both task and
	 *        event. Consider a new interface to only set the task, or
	 *        change the design to not clear task subscription inside a
	 *        radio event but when the radio event is done.
	 */
	hal_sw_switch_timer_clear_ppi_config();
#endif /* CONFIG_SOC_SERIES_NRF53X */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	/* start_us could be the current count in the timer */
	uint32_t now_us = start_us;

	/* Setup PPI while determining the latency in doing so */
	do {
		/* Set start to be, now plus the determined latency */
		start_us = (now_us << 1) - start_us;

		/* Setup compare event with min. 1 us offset */
		EVENT_TIMER->EVENTS_COMPARE[0] = 0U;
		nrf_timer_cc_set(EVENT_TIMER, 0, start_us + 1U);

		/* Capture the current time */
		nrf_timer_task_trigger(EVENT_TIMER,
				       HAL_EVENT_TIMER_SAMPLE_TASK);

		now_us = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	} while ((now_us > start_us) && (EVENT_TIMER->EVENTS_COMPARE[0] == 0U));

	return start_us + 1U;
}

uint32_t radio_tmr_start_now(uint8_t trx)
{
	uint32_t start_us;

	/* PPI/DPPI configuration will be done in radio_tmr_start_us() */

	/* Capture the current time */
	nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);
	start_us = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];

	/* Setup radio start at current time */
	start_us = radio_tmr_start_us(trx, start_us);

	return start_us;
}

uint32_t radio_tmr_start_get(void)
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

void radio_tmr_hcto_configure(uint32_t hcto)
{
	nrf_timer_cc_set(EVENT_TIMER, 1, hcto);

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

uint32_t radio_tmr_aa_get(void)
{
	return EVENT_TIMER->CC[1];
}

static uint32_t radio_tmr_aa;

void radio_tmr_aa_save(uint32_t aa)
{
	radio_tmr_aa = aa;
}

uint32_t radio_tmr_aa_restore(void)
{
	/* NOTE: we dont need to restore for now, but return the saved value. */
	return radio_tmr_aa;
}

uint32_t radio_tmr_ready_get(void)
{
	return EVENT_TIMER->CC[0];
}

static uint32_t radio_tmr_ready;

void radio_tmr_ready_save(uint32_t ready)
{
	radio_tmr_ready = ready;
}

uint32_t radio_tmr_ready_restore(void)
{
	return radio_tmr_ready;
}

void radio_tmr_end_capture(void)
{
	/* NOTE: nRF5340 for single timer configuration shares the DPPI channel being triggered
	 *       by Radio End for End time capture and sw_switch DPPI channel toggling hence
	 *       always need to capture End time. Hence, the below code is present in
	 *       hal_sw_switch_timer_clear_ppi_config() and sw_switch(). There is no need to
	 *       configure the channel again in this function.
	 */
#if !defined(CONFIG_SOC_SERIES_NRF53X) ||                                                          \
	(defined(CONFIG_SOC_SERIES_NRF53X) && !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER))
	hal_radio_end_time_capture_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_RADIO_END_TIME_CAPTURE_PPI));
#endif /* !CONFIG_SOC_SERIES_NRF53X ||
	* (CONFIG_SOC_SERIES_NRF53X && !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	*/
}

uint32_t radio_tmr_end_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return last_pdu_end_us;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return EVENT_TIMER->CC[2];
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

uint32_t radio_tmr_tifs_base_get(void)
{
	return radio_tmr_end_get();
}

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
static uint32_t tmr_sample_val;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

void radio_tmr_sample(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	uint32_t cc;

	cc = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);

	tmr_sample_val = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET] = cc;

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

uint32_t radio_tmr_sample_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return tmr_sample_val;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || \
	defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
void radio_gpio_pa_setup(void)
{
	NRF_GPIOTE->CONFIG[HAL_PALNA_GPIOTE_CHAN] =
		(GPIOTE_CONFIG_MODE_Task <<
		 GPIOTE_CONFIG_MODE_Pos) |
		(NRF_GPIO_PA_PSEL <<
		 GPIOTE_CONFIG_PSEL_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle <<
		 GPIOTE_CONFIG_POLARITY_Pos) |
		(OUTINIT_INACTIVE(NRF_GPIO_PA_FLAGS) <<
		 GPIOTE_CONFIG_OUTINIT_Pos);

#if defined(HAL_RADIO_FEM_IS_NRF21540)
	hal_pa_ppi_setup();
	radio_gpio_pdn_setup();
	radio_gpio_csn_setup();
#endif
}
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
void radio_gpio_lna_setup(void)
{
	NRF_GPIOTE->CONFIG[HAL_PALNA_GPIOTE_CHAN] =
		(GPIOTE_CONFIG_MODE_Task <<
		 GPIOTE_CONFIG_MODE_Pos) |
		(NRF_GPIO_LNA_PSEL <<
		 GPIOTE_CONFIG_PSEL_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle <<
		 GPIOTE_CONFIG_POLARITY_Pos) |
		(OUTINIT_INACTIVE(NRF_GPIO_LNA_FLAGS) <<
		 GPIOTE_CONFIG_OUTINIT_Pos);

#if defined(HAL_RADIO_FEM_IS_NRF21540)
	hal_lna_ppi_setup();
	radio_gpio_pdn_setup();
	radio_gpio_csn_setup();
#endif
}

void radio_gpio_pdn_setup(void)
{
	/* Note: the pdn-gpios property is optional. */
#if defined(NRF_GPIO_PDN)
	NRF_GPIOTE->CONFIG[HAL_PDN_GPIOTE_CHAN] =
		(GPIOTE_CONFIG_MODE_Task <<
		 GPIOTE_CONFIG_MODE_Pos) |
		(NRF_GPIO_PDN_PSEL <<
		 GPIOTE_CONFIG_PSEL_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle <<
		 GPIOTE_CONFIG_POLARITY_Pos) |
		(OUTINIT_INACTIVE(NRF_GPIO_PDN_FLAGS) <<
		 GPIOTE_CONFIG_OUTINIT_Pos);
#endif /* NRF_GPIO_PDN_PIN */
}

void radio_gpio_csn_setup(void)
{
	/* Note: the spi-if property is optional. */
#if defined(NRF_GPIO_CSN_PIN)
	NRF_GPIOTE->CONFIG[HAL_CSN_GPIOTE_CHAN] =
		(GPIOTE_CONFIG_MODE_Task <<
		 GPIOTE_CONFIG_MODE_Pos) |
		(NRF_GPIO_CSN_PSEL <<
		 GPIOTE_CONFIG_PSEL_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle <<
		 GPIOTE_CONFIG_POLARITY_Pos) |
		(OUTINIT_INACTIVE(NRF_GPIO_CSN_FLAGS) <<
		 GPIOTE_CONFIG_OUTINIT_Pos);
#endif /* NRF_GPIO_CSN_PIN */
}

void radio_gpio_lna_on(void)
{
	if (ACTIVE_LOW(NRF_GPIO_LNA_FLAGS)) {
		NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
	} else {
		NRF_GPIO_LNA->OUTSET = BIT(NRF_GPIO_LNA_PIN);
	}
}

void radio_gpio_lna_off(void)
{
	if (ACTIVE_LOW(NRF_GPIO_LNA_FLAGS)) {
		NRF_GPIO_LNA->OUTSET = BIT(NRF_GPIO_LNA_PIN);
	} else {
		NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
	}
}
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

void radio_gpio_pa_lna_enable(uint32_t trx_us)
{
	nrf_timer_cc_set(EVENT_TIMER, 2, trx_us);
#if defined(HAL_RADIO_FEM_IS_NRF21540) && DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios)
	nrf_timer_cc_set(EVENT_TIMER, 3, (trx_us - NRF_GPIO_PDN_OFFSET));
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_ENABLE_PALNA_PPI) |
					  BIT(HAL_DISABLE_PALNA_PPI) |
					  BIT(HAL_ENABLE_FEM_PPI) |
					  BIT(HAL_DISABLE_FEM_PPI));
#else
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_ENABLE_PALNA_PPI) |
					  BIT(HAL_DISABLE_PALNA_PPI));
#endif
}

void radio_gpio_pa_lna_disable(void)
{
#if defined(HAL_RADIO_FEM_IS_NRF21540)
	hal_radio_nrf_ppi_channels_disable(BIT(HAL_ENABLE_PALNA_PPI) |
					   BIT(HAL_DISABLE_PALNA_PPI) |
					   BIT(HAL_ENABLE_FEM_PPI) |
					   BIT(HAL_DISABLE_FEM_PPI));
	NRF_GPIOTE->CONFIG[HAL_PALNA_GPIOTE_CHAN] = 0;
	NRF_GPIOTE->CONFIG[HAL_PDN_GPIOTE_CHAN] = 0;
	NRF_GPIOTE->CONFIG[HAL_CSN_GPIOTE_CHAN] = 0;
#else
	hal_radio_nrf_ppi_channels_disable(BIT(HAL_ENABLE_PALNA_PPI) |
					   BIT(HAL_DISABLE_PALNA_PPI));
	NRF_GPIOTE->CONFIG[HAL_PALNA_GPIOTE_CHAN] = 0;
#endif
}
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN || HAL_RADIO_GPIO_HAVE_LNA_PIN */

static uint8_t MALIGN(4) _ccm_scratch[(HAL_RADIO_PDU_LEN_MAX - 4) + 16];

void *radio_ccm_rx_pkt_set(struct ccm *ccm, uint8_t phy, void *pkt)
{
	uint32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;
	mode = (CCM_MODE_MODE_Decryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;

#if !defined(CONFIG_SOC_SERIES_NRF51X)
	/* Enable CCM support for 8-bit length field PDUs. */
	mode |= (CCM_MODE_LENGTH_Extended << CCM_MODE_LENGTH_Pos) &
		CCM_MODE_LENGTH_Msk;

	/* Select CCM data rate based on current PHY in use. */
	switch (phy) {
	default:
	case PHY_1M:
		mode |= (CCM_MODE_DATARATE_1Mbit <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
		/* When direction finding CTE receive feature is enabled then on-the-fly PDU
		 * parsing for CTEInfo is always done. In such situation, the CCM TASKS_CRYPT
		 * must be started with short delay. That give the Radio time to store received bits
		 * in shared memory.
		 */
		radio_bc_configure(CCM_TASKS_CRYPT_DELAY_BITS);
		radio_bc_status_reset();
		hal_trigger_crypt_by_bcmatch_ppi_config();
		hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_DELAY_PPI));
#else
		hal_trigger_crypt_ppi_config();
		hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_PPI));
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
		break;

	case PHY_2M:
		mode |= (CCM_MODE_DATARATE_2Mbit <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;

		hal_trigger_crypt_ppi_config();
		hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_PPI));

		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	case PHY_CODED:
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

		hal_trigger_crypt_ppi_config();
		hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_PPI));

		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
#endif /* !CONFIG_SOC_SERIES_NRF51X */

#if !defined(CONFIG_SOC_SERIES_NRF51X) && \
	!defined(CONFIG_SOC_NRF52832) && \
	(!defined(CONFIG_BT_CTLR_DATA_LENGTH_MAX) || \
	 (CONFIG_BT_CTLR_DATA_LENGTH_MAX < ((HAL_RADIO_PDU_LEN_MAX) - 4U)))
	uint8_t max_len = (NRF_RADIO->PCNF1 & RADIO_PCNF1_MAXLEN_Msk) >>
			RADIO_PCNF1_MAXLEN_Pos;

	/* MAXPACKETSIZE value 0x001B (27) - 0x00FB (251) bytes */
	NRF_CCM->MAXPACKETSIZE = max_len - 4U;
#endif

	NRF_CCM->MODE = mode;
	NRF_CCM->CNFPTR = (uint32_t)ccm;
	NRF_CCM->INPTR = (uint32_t)_pkt_scratch;
	NRF_CCM->OUTPTR = (uint32_t)pkt;
	NRF_CCM->SCRATCHPTR = (uint32_t)_ccm_scratch;
	NRF_CCM->SHORTS = 0;
	NRF_CCM->EVENTS_ENDKSGEN = 0;
	NRF_CCM->EVENTS_ENDCRYPT = 0;
	NRF_CCM->EVENTS_ERROR = 0;

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_KSGEN);

	return _pkt_scratch;
}

void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt)
{
	uint32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;
	mode = (CCM_MODE_MODE_Encryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)
	/* Enable CCM support for 8-bit length field PDUs. */
	mode |= (CCM_MODE_LENGTH_Extended << CCM_MODE_LENGTH_Pos) &
		CCM_MODE_LENGTH_Msk;

	/* NOTE: use fastest data rate as tx data needs to be prepared before
	 * radio Tx on any PHY.
	 */
	mode |= (CCM_MODE_DATARATE_2Mbit << CCM_MODE_DATARATE_Pos) &
		CCM_MODE_DATARATE_Msk;
#endif

#if !defined(CONFIG_SOC_SERIES_NRF51X) && \
	!defined(CONFIG_SOC_NRF52832) && \
	(!defined(CONFIG_BT_CTLR_DATA_LENGTH_MAX) || \
	 (CONFIG_BT_CTLR_DATA_LENGTH_MAX < ((HAL_RADIO_PDU_LEN_MAX) - 4)))
	uint8_t max_len = (NRF_RADIO->PCNF1 & RADIO_PCNF1_MAXLEN_Msk) >>
			RADIO_PCNF1_MAXLEN_Pos;

	/* MAXPACKETSIZE value 0x001B (27) - 0x00FB (251) bytes */
	NRF_CCM->MAXPACKETSIZE = max_len - 4U;
#endif

	NRF_CCM->MODE = mode;
	NRF_CCM->CNFPTR = (uint32_t)ccm;
	NRF_CCM->INPTR = (uint32_t)pkt;
	NRF_CCM->OUTPTR = (uint32_t)_pkt_scratch;
	NRF_CCM->SCRATCHPTR = (uint32_t)_ccm_scratch;
	NRF_CCM->SHORTS = CCM_SHORTS_ENDKSGEN_CRYPT_Msk;
	NRF_CCM->EVENTS_ENDKSGEN = 0;
	NRF_CCM->EVENTS_ENDCRYPT = 0;
	NRF_CCM->EVENTS_ERROR = 0;

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_KSGEN);

	return _pkt_scratch;
}

uint32_t radio_ccm_is_done(void)
{
	nrf_ccm_int_enable(NRF_CCM, CCM_INTENSET_ENDCRYPT_Msk);
	while (NRF_CCM->EVENTS_ENDCRYPT == 0) {
		cpu_sleep();
	}
	nrf_ccm_int_disable(NRF_CCM, CCM_INTENCLR_ENDCRYPT_Msk);
	NVIC_ClearPendingIRQ(nrfx_get_irq_number(NRF_CCM));

	return (NRF_CCM->EVENTS_ERROR == 0);
}

uint32_t radio_ccm_mic_is_valid(void)
{
	return (NRF_CCM->MICSTATUS != 0);
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
static uint8_t MALIGN(4) _aar_scratch[3];

void radio_ar_configure(uint32_t nirk, void *irk, uint8_t flags)
{
	uint32_t addrptr;
	uint8_t bcc;
	uint8_t phy;

	/* Flags provide hint on how to setup AAR:
	 * ....Xb - legacy PDU
	 * ...X.b - extended PDU
	 * XXX..b = RX PHY
	 * 00000b = default case mapped to 00101b (legacy, 1M)
	 *
	 * If neither legacy not extended bit is set, legacy PDU is selected for
	 * 1M PHY and extended PDU otherwise.
	 */

	phy = flags >> 2;

	/* Check if extended PDU or non-1M and not legacy PDU */
	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	    ((flags & BIT(1)) || (!(flags & BIT(0)) && (phy > PHY_1M)))) {
		addrptr = NRF_RADIO->PACKETPTR + 1;
		bcc = 80;
	} else {
		addrptr = NRF_RADIO->PACKETPTR - 1;
		bcc = 64;
	}

	/* For Coded PHY adjust for CI and TERM1 */
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) && (phy == PHY_CODED)) {
		bcc += 5;
	}

	NRF_AAR->ENABLE = (AAR_ENABLE_ENABLE_Enabled << AAR_ENABLE_ENABLE_Pos) &
			  AAR_ENABLE_ENABLE_Msk;
	NRF_AAR->NIRK = nirk;
	NRF_AAR->IRKPTR = (uint32_t)irk;
	NRF_AAR->ADDRPTR = addrptr;
	NRF_AAR->SCRATCHPTR = (uint32_t)&_aar_scratch[0];

	NRF_AAR->EVENTS_END = 0;
	NRF_AAR->EVENTS_RESOLVED = 0;
	NRF_AAR->EVENTS_NOTRESOLVED = 0;

	radio_bc_configure(bcc);
	radio_bc_status_reset();

	hal_trigger_aar_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_AAR_PPI));
}

uint32_t radio_ar_match_get(void)
{
	return NRF_AAR->STATUS;
}

void radio_ar_status_reset(void)
{
	radio_bc_status_reset();

	NRF_AAR->ENABLE = (AAR_ENABLE_ENABLE_Disabled << AAR_ENABLE_ENABLE_Pos) &
			  AAR_ENABLE_ENABLE_Msk;

	hal_radio_nrf_ppi_channels_disable(BIT(HAL_TRIGGER_AAR_PPI));
}

uint32_t radio_ar_has_match(void)
{
	if (!radio_bc_has_match()) {
		return 0U;
	}

	nrf_aar_int_enable(NRF_AAR, AAR_INTENSET_END_Msk);

	while (NRF_AAR->EVENTS_END == 0U) {
		cpu_sleep();
	}

	nrf_aar_int_disable(NRF_AAR, AAR_INTENCLR_END_Msk);

	NVIC_ClearPendingIRQ(nrfx_get_irq_number(NRF_AAR));

	if (NRF_AAR->EVENTS_RESOLVED && !NRF_AAR->EVENTS_NOTRESOLVED) {
		return 1U;
	}

	return 0U;
}

uint8_t radio_ar_resolve(const uint8_t *addr)
{
	uint8_t retval;

	NRF_AAR->ENABLE = (AAR_ENABLE_ENABLE_Enabled << AAR_ENABLE_ENABLE_Pos) &
			  AAR_ENABLE_ENABLE_Msk;

	NRF_AAR->ADDRPTR = (uint32_t)addr - 3;

	NRF_AAR->EVENTS_END = 0;
	NRF_AAR->EVENTS_RESOLVED = 0;
	NRF_AAR->EVENTS_NOTRESOLVED = 0;

	NVIC_ClearPendingIRQ(nrfx_get_irq_number(NRF_AAR));

	nrf_aar_int_enable(NRF_AAR, AAR_INTENSET_END_Msk);

	nrf_aar_task_trigger(NRF_AAR, NRF_AAR_TASK_START);

	while (NRF_AAR->EVENTS_END == 0) {
		cpu_sleep();
	}

	nrf_aar_int_disable(NRF_AAR, AAR_INTENCLR_END_Msk);

	NVIC_ClearPendingIRQ(nrfx_get_irq_number(NRF_AAR));

	retval = (NRF_AAR->EVENTS_RESOLVED && !NRF_AAR->EVENTS_NOTRESOLVED) ?
		 1U : 0U;

	NRF_AAR->ENABLE = (AAR_ENABLE_ENABLE_Disabled << AAR_ENABLE_ENABLE_Pos) &
			  AAR_ENABLE_ENABLE_Msk;

	return retval;

}
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_DF_SUPPORT) && !defined(CONFIG_ZTEST)
/* @brief Function configures CTE inline register to start sampling of CTE
 *        according to information parsed from CTEInfo field of received PDU.
 *
 * @param[in] cte_info_in_s1    Informs where to expect CTEInfo field in PDU:
 *                              in S1 for data pdu, not in S1 for adv. PDU
 */
void radio_df_cte_inline_set_enabled(bool cte_info_in_s1)
{
	const nrf_radio_cteinline_conf_t inline_conf = {
		.enable = true,
		/* Indicates whether CTEInfo is in S1 byte or not. */
		.info_in_s1 = cte_info_in_s1,
		/* Enable or disable switching and sampling when CRC is not OK. */
#if defined(CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC)
		.err_handling = true,
#else
		.err_handling = false,
#endif /* CONFIG_BT_CTLR_DF_SAMPLE_CTE_FOR_PDU_WITH_BAD_CRC */
		/* Maximum range of CTE time. 20 * 8us according to BT spec.*/
		.time_range = NRF_RADIO_CTEINLINE_TIME_RANGE_20,
		/* Spacing between samples for 1us AoD or AoA is set to 2us. */
		.rx1us = NRF_RADIO_CTEINLINE_RX_MODE_2US,
		/* Spacing between samples for 2us AoD or AoA is set to 4us. */
		.rx2us = NRF_RADIO_CTEINLINE_RX_MODE_4US,
		/* S0 bit pattern to match all types of adv. PDUs or CP bit in conn PDU*/
		.s0_pattern = (cte_info_in_s1 ? DF_S0_MASK_CP_BIT_IN_DATA_CHANNEL_PDU :
							DF_S0_ALLOW_ALL_PER_ADV_PDU),
		/* S0 bit mask set to don't match any bit in SO octet or match CP bit in conn PDU */
		.s0_mask = (cte_info_in_s1 ? DF_S0_MASK_CP_BIT_IN_DATA_CHANNEL_PDU :
							DF_S0_ALLOW_ALL_PER_ADV_PDU)
	};

	nrf_radio_cteinline_configure(NRF_RADIO, &inline_conf);
}
#endif /* CONFIG_BT_CTLR_DF_SUPPORT && !CONFIG_ZTEST */
