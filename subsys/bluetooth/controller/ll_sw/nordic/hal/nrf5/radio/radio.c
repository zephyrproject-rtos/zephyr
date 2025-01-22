/*
 * Copyright (c) 2016 - 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>

#include <nrfx_gpiote.h>

#include "util/mem.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/cntr.h"
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

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
static const nrfx_gpiote_t gpiote_palna = NRFX_GPIOTE_INSTANCE(
	NRF_DT_GPIOTE_INST(FEM_NODE, HAL_RADIO_GPIO_PA_PROP));
static uint8_t gpiote_ch_palna;

BUILD_ASSERT(NRF_DT_GPIOTE_INST(FEM_NODE, HAL_RADIO_GPIO_PA_PROP) ==
	     NRF_DT_GPIOTE_INST(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP),
	HAL_RADIO_GPIO_PA_PROP_NAME " and " HAL_RADIO_GPIO_LNA_PROP_NAME
	" GPIOs must use the same GPIOTE instance.");
#endif

#if defined(HAL_RADIO_FEM_IS_NRF21540)
static const nrfx_gpiote_t gpiote_pdn = NRFX_GPIOTE_INSTANCE(
	NRF_DT_GPIOTE_INST(FEM_NODE, pdn_gpios));
static uint8_t gpiote_ch_pdn;
#endif

/* These headers require the above gpiote-related variables to be declared. */
#if defined(PPI_PRESENT)
#include "radio_nrf5_ppi_gpiote.h"
#elif defined(DPPI_PRESENT)
#include "radio_nrf5_dppi_gpiote.h"
#endif

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
	irq_disable(HAL_RADIO_IRQn);

	isr_cb_param = param;
	isr_cb = cb;

	nrf_radio_int_enable(NRF_RADIO, HAL_RADIO_INTENSET_DISABLED_Msk);

	NVIC_ClearPendingIRQ(HAL_RADIO_IRQn);
	irq_enable(HAL_RADIO_IRQn);
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

	hal_radio_ram_prio_setup();
}

void radio_reset(void)
{
	irq_disable(HAL_RADIO_IRQn);

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

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->TIMING = (RADIO_TIMING_RU_Legacy << RADIO_TIMING_RU_Pos) &
			    RADIO_TIMING_RU_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->TIMING = (RADIO_TIMING_RU_Fast << RADIO_TIMING_RU_Pos) &
			    RADIO_TIMING_RU_Msk;
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	NRF_POWER->TASKS_CONSTLAT = 1U;
#endif /* CONFIG_SOC_COMPATIBLE_NRF54LX */

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

#if !defined(CONFIG_SOC_SERIES_NRF51X) && !defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
	NRF_RADIO->MODECNF0 = ((RADIO_MODECNF0_DTX_Center <<
				RADIO_MODECNF0_DTX_Pos) &
			       RADIO_MODECNF0_DTX_Msk) |
			      ((RADIO_MODECNF0_RU_Fast <<
				RADIO_MODECNF0_RU_Pos) &
			       RADIO_MODECNF0_RU_Msk);
#else /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
	NRF_RADIO->MODECNF0 = (RADIO_MODECNF0_DTX_Center <<
			       RADIO_MODECNF0_DTX_Pos) &
			      RADIO_MODECNF0_DTX_Msk;
#endif /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
#endif /* !CONFIG_SOC_SERIES_NRF51X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
}

void radio_tx_power_set(int8_t power)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	uint32_t value;

	value = hal_radio_tx_power_value(power);
	NRF_RADIO->TXPOWER = value;

#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
	uint32_t value;

	/* NOTE: TXPOWER register only accepts upto 0dBm, hence use the HAL
	 * floor value for the TXPOWER register. Permit +3dBm by using high
	 * voltage being set for radio.
	 */
	value = hal_radio_tx_power_floor(power);
	NRF_RADIO->TXPOWER = value;
	hal_radio_tx_power_high_voltage_set(power);

#else /* !CONFIG_SOC_COMPATIBLE_NRF53X  && !CONFIG_SOC_COMPATIBLE_NRF54LX */

	/* NOTE: valid value range is passed by Kconfig define. */
	NRF_RADIO->TXPOWER = (uint32_t)power;

#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
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
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X)
	return RADIO_TXPOWER_TXPOWER_Pos3dBm;

#else /* !CONFIG_SOC_COMPATIBLE_NRF53X */
	return (int8_t)hal_radio_tx_power_max_get();

#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X */
}

int8_t radio_tx_power_floor(int8_t power)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X)
	/* NOTE: TXPOWER register only accepts upto 0dBm, +3dBm permitted by
	 * use of high voltage being set for radio when TXPOWER register is set.
	 */
	if (power >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos3dBm;
	}
#endif /* CONFIG_SOC_COMPATIBLE_NRF53X */

	return (int8_t)hal_radio_tx_power_floor(power);
}

void radio_freq_chan_set(uint32_t chan)
{
	NRF_RADIO->FREQUENCY = chan;
}

void radio_whiten_iv_set(uint32_t iv)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#if defined(RADIO_DATAWHITEIV_DATAWHITEIV_Msk)
	NRF_RADIO->DATAWHITEIV = HAL_RADIO_RESET_VALUE_DATAWHITE | iv;
#else /* !RADIO_DATAWHITEIV_DATAWHITEIV_Msk */
	NRF_RADIO->DATAWHITE = HAL_RADIO_RESET_VALUE_DATAWHITE | iv;
#endif /* !RADIO_DATAWHITEIV_DATAWHITEIV_Msk */
#else /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
	nrf_radio_datawhiteiv_set(NRF_RADIO, iv);
#endif /* !CONFIG_SOC_COMPATIBLE_NRF54LX */

	NRF_RADIO->PCNF1 &= ~RADIO_PCNF1_WHITEEN_Msk;
	NRF_RADIO->PCNF1 |= ((1UL) << RADIO_PCNF1_WHITEEN_Pos) &
			    RADIO_PCNF1_WHITEEN_Msk;
}

void radio_aa_set(const uint8_t *aa)
{
	NRF_RADIO->TXADDRESS =
	    (((0UL) << RADIO_TXADDRESS_TXADDRESS_Pos) &
	     RADIO_TXADDRESS_TXADDRESS_Msk);
	NRF_RADIO->RXADDRESSES =
	    ((RADIO_RXADDRESSES_ADDR0_Enabled) << RADIO_RXADDRESSES_ADDR0_Pos);
	NRF_RADIO->PREFIX0 = aa[3];
	NRF_RADIO->BASE0 = (((uint32_t) aa[2]) << 24) | (aa[1] << 16) | (aa[0] << 8);
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
	defined(CONFIG_SOC_COMPATIBLE_NRF53X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
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
	if ((pdu_type == RADIO_PKT_CONF_PDU_TYPE_DC) ||
	    (pdu_type == RADIO_PKT_CONF_PDU_TYPE_BIS) ||
	    (pdu_type == RADIO_PKT_CONF_PDU_TYPE_CIS)) {
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
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
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
#endif /* CONFIG_SOC_COMPATIBLE_NRF53X || CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
}

void radio_tx_enable(void)
{
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
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
#endif /* CONFIG_SOC_COMPATIBLE_NRF53X || CONFIG_SOC_COMPATIBLE_NRF54LX */
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
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);
#if defined(CONFIG_BT_CTLR_DF_SUPPORT) && !defined(CONFIG_ZTEST)
	/* Clear it only for SoCs supporting DF extension */
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_PHYEND);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CTEPRESENT);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_BCMATCH);
#endif /* CONFIG_BT_CTLR_DF_SUPPORT && !CONFIG_ZTEST */
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_RATEBOOST);
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
}

uint32_t radio_is_ready(void)
{
	return (NRF_RADIO->EVENTS_READY != 0);
}

uint32_t radio_is_address(void)
{
	return (NRF_RADIO->EVENTS_ADDRESS != 0);
}

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
static uint32_t last_pdu_end_latency_us;
static uint32_t last_pdu_end_us;

static void last_pdu_end_us_init(uint32_t latency_us)
{
	last_pdu_end_latency_us = latency_us;
	last_pdu_end_us = 0U;
}

uint32_t radio_is_done(void)
{
	if (NRF_RADIO->HAL_RADIO_TRX_EVENTS_END != 0) {
		/* On packet END event increment last packet end time value.
		 * Note: this depends on the function being called exactly once
		 * in the ISR function.
		 */
		last_pdu_end_us += EVENT_TIMER->CC[HAL_EVENT_TIMER_TRX_END_CC_OFFSET];
		return 1;
	} else {
		return 0;
	}
}

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
uint32_t radio_is_done(void)
{
	return (NRF_RADIO->HAL_RADIO_TRX_EVENTS_END != 0);
}
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

uint32_t radio_is_tx_done(void)
{
	if (IS_ENABLED(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)) {
		return radio_is_done();
	} else {
		return 1U;
	}
}

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
	nrf_radio_crc_configure(NRF_RADIO,
				RADIO_CRCCNF_LEN_Three,
				NRF_RADIO_CRC_ADDR_SKIP,
				polynomial);
	NRF_RADIO->CRCINIT = iv;
}

uint32_t radio_crc_is_valid(void)
{
	return (NRF_RADIO->CRCSTATUS != 0);
}

/* Note: Only 3 bytes (PDU_EM_LL_SIZE_MAX) is required for empty PDU
 *       transmission, but in case of Radio ISR latency if rx packet pointer
 *       is not setup then Radio DMA will use previously assigned buffer which
 *       can be this empty PDU buffer. Radio DMA will overrun this buffer and
 *       cause memory corruption. Any detection of ISR latency will not happen
 *       if the ISR function pointer in RAM is corrupted by this overrun.
 *       Increasing ISR latencies in OS and CPU usage in the ULL_HIGH priority
 *       if it is same as LLL priority in Controller implementation then it is
 *       making it tight to execute Controller code in the tIFS between Tx-Rx
 *       PDU's Radio ISRs.
 */
static uint8_t MALIGN(4) _pkt_empty[MAX(HAL_RADIO_PDU_LEN_MAX,
					PDU_EM_LL_SIZE_MAX)];
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
 * PDU buffer. Note this buffer will be used to store whole PDUs, not just the BIG control payload.
 */
static uint8_t pkt_big_ctrl[offsetof(struct pdu_bis, payload) + sizeof(struct pdu_big_ctrl)];

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

	hal_radio_sw_switch_setup(sw_tifs_toggle);

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

			hal_radio_b2b_txen_on_sw_switch(cc, ppi);
		} else {
			/* If RX PHY is LE Coded, calculate for S8 coding.
			 * Assumption being, S8 has higher delay.
			 */
			delay = HAL_RADIO_NS2US_ROUND(
			    hal_radio_tx_ready_delay_ns_get(phy_next,
							    flags_next) +
			    hal_radio_rx_chain_delay_ns_get(phy_curr, 1));

			hal_radio_txen_on_sw_switch(cc, ppi);
		}

#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		if (dir_curr == SW_SWITCH_RX && end_evt_delay_en == END_EVT_DELAY_ENABLED &&
		    !(phy_curr & PHY_CODED)) {
			nrf_timer_cc_set(SW_SWITCH_TIMER, phyend_delay_cc,
					 SW_SWITCH_TIMER->CC[cc] - RADIO_EVENTS_PHYEND_DELAY_US);
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
		uint8_t ppi_en = HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(sw_tifs_toggle);
		uint8_t ppi_dis = HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(sw_tifs_toggle);
		uint8_t cc_s2 = SW_SWITCH_TIMER_S2_EVTS_COMP(sw_tifs_toggle);

		if (dir_curr == SW_SWITCH_RX && (phy_curr & PHY_CODED)) {
			/* Switching to TX after RX on LE Coded PHY. */
			uint32_t delay_s2;
			uint32_t new_cc_s2_value;

			/* Calculate assuming reception on S2 coding scheme. */
			delay_s2 = HAL_RADIO_NS2US_ROUND(
				hal_radio_tx_ready_delay_ns_get(phy_next,
								flags_next) +
				hal_radio_rx_chain_delay_ns_get(phy_curr, 0));

			new_cc_s2_value = SW_SWITCH_TIMER->CC[cc];

			if (delay_s2 < new_cc_s2_value) {
				new_cc_s2_value -= delay_s2;
			} else {
				new_cc_s2_value = 1;
			}
			nrf_timer_cc_set(SW_SWITCH_TIMER, cc_s2,
					 new_cc_s2_value);

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
				hal_radio_sw_switch_coded_config_clear(ppi_en, ppi_dis, cc_s2,
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

			hal_radio_rxen_on_sw_switch(cc, ppi);
		} else {
			delay = HAL_RADIO_NS2US_CEIL(
				hal_radio_rx_ready_delay_ns_get(phy_next,
								flags_next) +
				hal_radio_rx_chain_delay_ns_get(phy_curr,
								flags_curr)) +
				(EVENT_CLOCK_JITTER_US << 1);

			hal_radio_b2b_rxen_on_sw_switch(cc, ppi);
		}


#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		hal_radio_sw_switch_phyend_delay_compensation_config_clear(radio_enable_ppi,
									   phyend_delay_cc);
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
		if (1) {
			uint8_t ppi_en = HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(sw_tifs_toggle);
			uint8_t ppi_dis = HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(sw_tifs_toggle);
			uint8_t cc_s2 = SW_SWITCH_TIMER_S2_EVTS_COMP(sw_tifs_toggle);

			hal_radio_sw_switch_coded_config_clear(ppi_en, ppi_dis, cc_s2,
							       sw_tifs_toggle);
			hal_radio_sw_switch_disable_group_clear(ppi_dis, cc, sw_tifs_toggle);
		}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}

	if (delay < SW_SWITCH_TIMER->CC[cc]) {
		nrf_timer_cc_set(SW_SWITCH_TIMER,
				 cc,
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
#if !defined(CONFIG_SOC_COMPATIBLE_NRF53X) && !defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	/* The function is not called for nRF5340 single timer configuration because
	 * HAL_SW_SWITCH_TIMER_CLEAR_PPI is equal to HAL_RADIO_END_TIME_CAPTURE_PPI,
	 * so channel is already enabled.
	 */
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_RADIO_END_TIME_CAPTURE_PPI));
#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	sw_tifs_toggle += 1U;
	sw_tifs_toggle &= 1U;
}
#endif /* CONFIG_BT_CTLR_TIFS_HW */

void radio_switch_complete_and_rx(uint8_t phy_rx)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_RXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk;

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
			    NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk;

	sw_switch(SW_SWITCH_RX, SW_SWITCH_TX, phy_rx, flags_rx, phy_tx, flags_tx,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_with_delay_compensation_and_tx(
	uint8_t phy_rx, uint8_t flags_rx, uint8_t phy_tx, uint8_t flags_tx,
	enum radio_end_evt_delay_state end_delay_en)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk;

	sw_switch(SW_SWITCH_RX, SW_SWITCH_TX, phy_rx, flags_rx, phy_tx, flags_tx, end_delay_en);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_b2b_tx(uint8_t phy_curr, uint8_t flags_curr,
				      uint8_t phy_next, uint8_t flags_next)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk;

	sw_switch(SW_SWITCH_TX, SW_SWITCH_TX, phy_curr, flags_curr, phy_next, flags_next,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_b2b_rx(uint8_t phy_curr, uint8_t flags_curr,
				      uint8_t phy_next, uint8_t flags_next)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
			    NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_RXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk;

	sw_switch(SW_SWITCH_RX, SW_SWITCH_RX, phy_curr, flags_curr, phy_next, flags_next,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_b2b_tx_disable(void)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk);
#else /* CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk);
	hal_radio_sw_switch_b2b_tx_disable(sw_tifs_toggle);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_b2b_rx_disable(void)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk);
#else /* CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk);
	hal_radio_sw_switch_b2b_rx_disable(sw_tifs_toggle);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_switch_complete_and_disable(void)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk);
#else /* CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_TRX_END_DISABLE_Msk);
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
#if defined(CONFIG_SOC_SERIES_NRF51X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF52X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)
	     RADIO_SHORTS_DISABLED_RSSISTOP_Msk |
#endif
	     0);
}

uint32_t radio_rssi_get(void)
{
	return NRF_RADIO->RSSISAMPLE;
}

void radio_rssi_status_reset(void)
{
#if defined(CONFIG_SOC_SERIES_NRF51X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF52X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_RSSIEND);
#endif
}

uint32_t radio_rssi_is_ready(void)
{
#if defined(CONFIG_SOC_SERIES_NRF51X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF52X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)
	return (NRF_RADIO->EVENTS_RSSIEND != 0);
#else
	return 1U;
#endif
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
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DEVMATCH);
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
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_BCMATCH);
}

uint32_t radio_bc_has_match(void)
{
	return (NRF_RADIO->EVENTS_BCMATCH != 0);
}

#if defined(CONFIG_BT_CTLR_RADIO_TIMER_ISR)
static radio_isr_cb_t isr_radio_tmr_cb;
static void           *isr_radio_tmr_cb_param;

void isr_radio_tmr(void)
{
	irq_disable(TIMER0_IRQn);
	nrf_timer_int_disable(EVENT_TIMER, TIMER_INTENSET_COMPARE2_Msk);
	nrf_timer_event_clear(EVENT_TIMER, HAL_EVENT_TIMER_DEFERRED_TX_EVENT);

	isr_radio_tmr_cb(isr_radio_tmr_cb_param);
}

uint32_t radio_tmr_isr_set(uint32_t start_us, radio_isr_cb_t cb, void *param)
{
	irq_disable(TIMER0_IRQn);

	isr_radio_tmr_cb_param = param;
	isr_radio_tmr_cb = cb;

	/* start_us could be the current count in the timer */
	uint32_t now_us = start_us;

	/* Setup timer compare while determining the latency in doing so */
	do {
		/* Set start to be, now plus the determined latency */
		start_us = (now_us << 1) - start_us;

		/* Setup compare event with min. 1 us offset */
		nrf_timer_event_clear(EVENT_TIMER, HAL_EVENT_TIMER_DEFERRED_TX_EVENT);
		nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_DEFERRED_TRX_CC_OFFSET,
				 start_us + 1U);

		/* Capture the current time */
		nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);

		now_us = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	} while ((now_us > start_us) &&
		 (EVENT_TIMER->EVENTS_COMPARE[HAL_EVENT_TIMER_DEFERRED_TRX_CC_OFFSET] == 0U));

	nrf_timer_int_enable(EVENT_TIMER, TIMER_INTENSET_COMPARE2_Msk);

	NVIC_ClearPendingIRQ(TIMER0_IRQn);

	irq_enable(TIMER0_IRQn);

	return start_us + 1U;
}
#endif /* CONFIG_BT_CTLR_RADIO_TIMER_ISR */

void radio_tmr_status_reset(void)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_event_disable(NRF_RTC, RTC_EVTENCLR_COMPARE2_Msk);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
	hal_trigger_crypt_ppi_disable();
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */

	hal_radio_nrf_ppi_channels_disable(
			BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI) |
			BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI) |
			BIT(HAL_EVENT_TIMER_START_PPI) |
			BIT(HAL_RADIO_READY_TIME_CAPTURE_PPI) |
			BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) |
			BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI) |
			BIT(HAL_RADIO_END_TIME_CAPTURE_PPI) |
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
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
#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
			BIT(HAL_TRIGGER_CRYPT_PPI) |
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */
			0);
}

void radio_tmr_tx_status_reset(void)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_event_disable(NRF_RTC, RTC_EVTENCLR_COMPARE2_Msk);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
	hal_trigger_crypt_ppi_disable();
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */

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
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
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
#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
			BIT(HAL_TRIGGER_CRYPT_PPI) |
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */
			0);
}

void radio_tmr_rx_status_reset(void)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_event_disable(NRF_RTC, RTC_EVTENCLR_COMPARE2_Msk);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
	hal_trigger_crypt_ppi_disable();
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */

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
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
			BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
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
#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
			BIT(HAL_TRIGGER_CRYPT_PPI) |
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */
			0);
}

void radio_tmr_tx_enable(void)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#else /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
#if (HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI)
	hal_radio_enable_on_tick_ppi_config_and_enable(1U);
#endif /* HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI */
#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
}

void radio_tmr_rx_enable(void)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#else /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
#if (HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI)
	hal_radio_enable_on_tick_ppi_config_and_enable(0U);
#endif /* HAL_RADIO_ENABLE_TX_ON_TICK_PPI == HAL_RADIO_ENABLE_RX_ON_TICK_PPI */
#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
}

void radio_tmr_tx_disable(void)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_TXEN);
#else /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
}

void radio_tmr_rx_disable(void)
{
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_RXEN);
#else /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* !CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX */
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
	uint32_t remainder_us;

	/* Convert jitter to positive offset remainder in microseconds */
	hal_ticker_remove_jitter(&ticks_start, &remainder);
	remainder_us = remainder;

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* When using single timer for software tIFS switching, ensure that
	 * the timer compare value is large enough to consider radio ISR
	 * latency so that the ISR is able to disable the PPI/DPPI that again
	 * could trigger the TXEN/RXEN task.
	 * The timer is cleared on Radio End and if the PPI/DPPI is not disabled
	 * by the Radio ISR, the compare will trigger again.
	 */
	uint32_t latency_ticks;
	uint32_t latency_us;

	latency_us = MAX(remainder_us, HAL_RADIO_ISR_LATENCY_MAX_US) - remainder_us;
	latency_ticks = HAL_TICKER_US_TO_TICKS(latency_us);
	ticks_start -= latency_ticks;
	remainder_us += latency_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);
	EVENT_TIMER->MODE = 0;
	EVENT_TIMER->PRESCALER = HAL_EVENT_TIMER_PRESCALER_VALUE;
	EVENT_TIMER->BITMODE = 2;	/* 24 - bit */

	nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_TRX_CC_OFFSET, remainder_us);

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	uint32_t cntr_l, cntr_h, cntr_h_overflow, stale;

	/* Disable capture/compare */
	nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);

	/* NOTE: We are going to use TASKS_CAPTURE to read current
	 *       SYSCOUNTER H and L, so that COMPARE registers can be set
	 *       considering that we need to set H compare value too.
	 */

	/* Read current syscounter value */
	do {
		cntr_h = nrf_grtc_sys_counter_high_get(NRF_GRTC);
		cntr_l = nrf_grtc_sys_counter_low_get(NRF_GRTC);
		cntr_h_overflow = nrf_grtc_sys_counter_high_get(NRF_GRTC);
	} while ((cntr_h & GRTC_SYSCOUNTER_SYSCOUNTERH_BUSY_Msk) ||
		 (cntr_h_overflow & GRTC_SYSCOUNTER_SYSCOUNTERH_OVERFLOW_Msk));

	/* Set a stale value in capture value */
	stale = cntr_l - 1U;
	NRF_GRTC->CC[HAL_CNTR_GRTC_CC_IDX_RADIO].CCL = stale;

	/* Trigger a capture */
	nrf_grtc_task_trigger(NRF_GRTC, (NRF_GRTC_TASK_CAPTURE_0 +
					 (HAL_CNTR_GRTC_CC_IDX_RADIO * sizeof(uint32_t))));

	/* Wait to get a new L value */
	do {
		cntr_l = NRF_GRTC->CC[HAL_CNTR_GRTC_CC_IDX_RADIO].CCL;
	} while (cntr_l == stale);

	/* Read H value */
	cntr_h = NRF_GRTC->CC[HAL_CNTR_GRTC_CC_IDX_RADIO].CCH;

	/* NOTE: HERE, we have cntr_h and cntr_l in sync. */

	/* Handle rollover between current and expected value */
	if (ticks_start < cntr_l) {
		cntr_h++;
	}

	/* Clear compare event, if any */
	nrf_grtc_event_clear(NRF_GRTC, HAL_CNTR_GRTC_EVENT_COMPARE_RADIO);

	/* Set compare register values */
	nrf_grtc_sys_counter_cc_set(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO,
				    ((((uint64_t)cntr_h & GRTC_CC_CCH_CCH_Msk) << 32) |
				     ticks_start));

	/* Enable compare */
	nrf_grtc_sys_counter_compare_event_enable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);

#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_cc_set(NRF_RTC, 2, ticks_start);
	nrf_rtc_event_enable(NRF_RTC, RTC_EVTENSET_COMPARE2_Msk);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

	hal_event_timer_start_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_EVENT_TIMER_START_PPI));

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	last_pdu_end_us_init(latency_us);

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_CLEAR);
	SW_SWITCH_TIMER->MODE = 0;
	SW_SWITCH_TIMER->PRESCALER = HAL_EVENT_TIMER_PRESCALER_VALUE;
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

	return remainder_us;
}

uint32_t radio_tmr_start_tick(uint8_t trx, uint32_t ticks_start)
{
	uint32_t remainder_us;

	/* Setup compare event with min. 1 us offset */
	remainder_us = 1U;

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* When using single timer for software tIFS switching, ensure that
	 * the timer compare value is large enough to consider radio ISR
	 * latency so that the ISR is able to disable the PPI/DPPI that again
	 * could trigger the TXEN/RXEN task.
	 * The timer is cleared on Radio End and if the PPI/DPPI is not disabled
	 * by the Radio ISR, the compare will trigger again.
	 */
	uint32_t latency_ticks;
	uint32_t latency_us;

	latency_us = MAX(remainder_us, HAL_RADIO_ISR_LATENCY_MAX_US) - remainder_us;
	latency_ticks = HAL_TICKER_US_TO_TICKS(latency_us);
	ticks_start -= latency_ticks;
	remainder_us += latency_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);

	nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_TRX_CC_OFFSET, remainder_us);

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	uint32_t cntr_l, cntr_h, cntr_h_overflow, stale;

	/* Disable capture/compare */
	nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);

	/* NOTE: We are going to use TASKS_CAPTURE to read current
	 *       SYSCOUNTER H and L, so that COMPARE registers can be set
	 *       considering that we need to set H compare value too.
	 */

	/* Read current syscounter value */
	do {
		cntr_h = nrf_grtc_sys_counter_high_get(NRF_GRTC);
		cntr_l = nrf_grtc_sys_counter_low_get(NRF_GRTC);
		cntr_h_overflow = nrf_grtc_sys_counter_high_get(NRF_GRTC);
	} while ((cntr_h & GRTC_SYSCOUNTER_SYSCOUNTERH_BUSY_Msk) ||
		 (cntr_h_overflow & GRTC_SYSCOUNTER_SYSCOUNTERH_OVERFLOW_Msk));

	/* Set a stale value in capture value */
	stale = cntr_l - 1U;
	NRF_GRTC->CC[HAL_CNTR_GRTC_CC_IDX_RADIO].CCL = stale;

	/* Trigger a capture */
	nrf_grtc_task_trigger(NRF_GRTC, (NRF_GRTC_TASK_CAPTURE_0 +
					 (HAL_CNTR_GRTC_CC_IDX_RADIO * sizeof(uint32_t))));

	/* Wait to get a new L value */
	do {
		cntr_l = NRF_GRTC->CC[HAL_CNTR_GRTC_CC_IDX_RADIO].CCL;
	} while (cntr_l == stale);

	/* Read H value */
	cntr_h = NRF_GRTC->CC[HAL_CNTR_GRTC_CC_IDX_RADIO].CCH;

	/* NOTE: HERE, we have cntr_h and cntr_l in sync. */

	/* Handle rollover between current and expected value */
	if (ticks_start < cntr_l) {
		cntr_h++;
	}

	/* Clear compare event, if any */
	nrf_grtc_event_clear(NRF_GRTC, HAL_CNTR_GRTC_EVENT_COMPARE_RADIO);

	/* Set compare register values */
	nrf_grtc_sys_counter_cc_set(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO,
				    ((((uint64_t)cntr_h & GRTC_CC_CCH_CCH_Msk) << 32) |
				     ticks_start));

	/* Enable compare */
	nrf_grtc_sys_counter_compare_event_enable(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);

#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_cc_set(NRF_RTC, 2, ticks_start);
	nrf_rtc_event_enable(NRF_RTC, RTC_EVTENSET_COMPARE2_Msk);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

	hal_event_timer_start_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_EVENT_TIMER_START_PPI));

	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	last_pdu_end_us_init(latency_us);
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
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
#endif /* CONFIG_SOC_COMPATIBLE_NRF53X || CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	return remainder_us;
}

uint32_t radio_tmr_start_us(uint8_t trx, uint32_t start_us)
{
	hal_radio_enable_on_tick_ppi_config_and_enable(trx);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* As timer is reset on every radio end, remove the accumulated
	 * last_pdu_end_us in the given start_us.
	 */
	start_us -= last_pdu_end_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#if defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
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
#endif /* CONFIG_SOC_COMPATIBLE_NRF53X || CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

	/* start_us could be the current count in the timer */
	uint32_t now_us = start_us;
	uint32_t actual_us;

	/* Setup timer compare while determining the latency in doing so */
	do {
		/* Set start to be, now plus the determined latency */
		start_us = (now_us << 1) - start_us;

		/* Setup compare event with min. 1 us offset */
		actual_us = start_us + 1U;

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
		/* When using single timer for software tIFS switching, ensure that
		 * the timer compare value is large enough to consider radio ISR
		 * latency so that the ISR is able to disable the PPI/DPPI that again
		 * could trigger the TXEN/RXEN task.
		 * The timer is cleared on Radio End and if the PPI/DPPI is not disabled
		 * by the Radio ISR, the compare will trigger again.
		 */
		uint32_t latency_us;

		latency_us = MAX(actual_us, HAL_RADIO_ISR_LATENCY_MAX_US) - actual_us;
		actual_us += latency_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

		nrf_timer_event_clear(EVENT_TIMER, HAL_EVENT_TIMER_TRX_EVENT);
		nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_TRX_CC_OFFSET, actual_us);

		/* Capture the current time */
		nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);

		now_us = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];
	} while ((now_us > start_us) &&
		 (EVENT_TIMER->EVENTS_COMPARE[HAL_EVENT_TIMER_TRX_CC_OFFSET] == 0U));

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	actual_us += last_pdu_end_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	return actual_us;
}

uint32_t radio_tmr_start_now(uint8_t trx)
{
	uint32_t start_us;

	/* PPI/DPPI configuration will be done in radio_tmr_start_us() */

	/* Capture the current time */
	nrf_timer_task_trigger(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_TASK);
	start_us = EVENT_TIMER->CC[HAL_EVENT_TIMER_SAMPLE_CC_OFFSET];

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* As timer is reset on every radio end, add the accumulated
	 * last_pdu_end_us to the captured current time.
	 */
	start_us += last_pdu_end_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	/* Setup radio start at current time */
	start_us = radio_tmr_start_us(trx, start_us);

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* Remove the single timer start latency used to mitigate use of too
	 * small compare register value. Thus, start_us returned be always
	 * the value corresponding to the captured radio ready timestamp.
	 * This is used in the calculation of aux_offset in subsequent
	 * ADV_EXT_IND PDUs.
	 */
	start_us -= last_pdu_end_latency_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	return start_us;
}

uint32_t radio_tmr_start_get(void)
{
	uint32_t start_ticks;

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	uint64_t cc;

	cc = nrf_grtc_sys_counter_cc_get(NRF_GRTC, HAL_CNTR_GRTC_CC_IDX_RADIO);

	start_ticks = cc & 0xffffffffUL;

#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	start_ticks = nrf_rtc_cc_get(NRF_RTC, 2);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

	return start_ticks;
}

uint32_t radio_tmr_start_latency_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return last_pdu_end_latency_us;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return 0U;
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

void radio_tmr_stop(void)
{
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_STOP);
#if defined(TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Msk)
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_SHUTDOWN);
#endif /* TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Msk */

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_STOP);
#if defined(TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Msk)
	nrf_timer_task_trigger(SW_SWITCH_TIMER, NRF_TIMER_TASK_SHUTDOWN);
#endif /* TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Msk */
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	NRF_POWER->TASKS_LOWPWR = 1U;
#endif /* CONFIG_SOC_COMPATIBLE_NRF54LX */
}

void radio_tmr_hcto_configure(uint32_t hcto_us)
{
	nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_HCTO_CC_OFFSET, hcto_us);

	hal_radio_recv_timeout_cancel_ppi_config();
	hal_radio_disable_on_hcto_ppi_config();
	hal_radio_nrf_ppi_channels_enable(
		BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) |
		BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI));
}

void radio_tmr_hcto_configure_abs(uint32_t hcto_from_start_us)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	/* As timer is reset on every radio end, remove the accumulated
	 * last_pdu_end_us in the given hcto_us.
	 */
	hcto_from_start_us -= last_pdu_end_us;
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

	radio_tmr_hcto_configure(hcto_from_start_us);
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
	return EVENT_TIMER->CC[HAL_EVENT_TIMER_HCTO_CC_OFFSET];
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
	return EVENT_TIMER->CC[HAL_EVENT_TIMER_TRX_CC_OFFSET];
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
#if (!defined(CONFIG_SOC_COMPATIBLE_NRF53X) && !defined(CONFIG_SOC_COMPATIBLE_NRF54LX)) || \
	((defined(CONFIG_SOC_COMPATIBLE_NRF53X) || defined(CONFIG_SOC_COMPATIBLE_NRF54LX)) && \
	 !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER))
	hal_radio_end_time_capture_ppi_config();
	hal_radio_nrf_ppi_channels_enable(BIT(HAL_RADIO_END_TIME_CAPTURE_PPI));
#endif /* (!CONFIG_SOC_COMPATIBLE_NRF53X && !CONFIG_SOC_COMPATIBLE_NRF54LX) ||
	* ((CONFIG_SOC_COMPATIBLE_NRF53X || CONFIG_SOC_COMPATIBLE_NRF54LX) &&
	*  !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	*/
}

uint32_t radio_tmr_end_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return last_pdu_end_us;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return EVENT_TIMER->CC[HAL_EVENT_TIMER_TRX_END_CC_OFFSET];
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
}

uint32_t radio_tmr_tifs_base_get(void)
{
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
	return 0U;
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
	return radio_tmr_end_get();
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
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
	nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_SAMPLE_CC_OFFSET, cc);

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
int radio_gpio_pa_lna_init(void)
{
#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	if (nrfx_gpiote_channel_alloc(&gpiote_palna, &gpiote_ch_palna) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if defined(NRF_GPIO_PDN_PIN)
	if (nrfx_gpiote_channel_alloc(&gpiote_pdn, &gpiote_ch_pdn) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

	return 0;
}

void radio_gpio_pa_lna_deinit(void)
{
#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	(void)nrfx_gpiote_channel_free(&gpiote_palna, gpiote_ch_palna);
#endif

#if defined(NRF_GPIO_PDN_PIN)
	(void)nrfx_gpiote_channel_free(&gpiote_pdn, gpiote_ch_pdn);
#endif
}

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
void radio_gpio_pa_setup(void)
{
	gpiote_palna.p_reg->CONFIG[gpiote_ch_palna] =
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
#endif
}
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
void radio_gpio_lna_setup(void)
{
	gpiote_palna.p_reg->CONFIG[gpiote_ch_palna] =
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
#endif
}

void radio_gpio_pdn_setup(void)
{
	/* Note: the pdn-gpios property is optional. */
#if defined(NRF_GPIO_PDN)
	gpiote_pdn.p_reg->CONFIG[gpiote_ch_pdn] =
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
	nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_PA_LNA_CC_OFFSET, trx_us);
#if defined(HAL_RADIO_FEM_IS_NRF21540) && DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios)
	nrf_timer_cc_set(EVENT_TIMER, HAL_EVENT_TIMER_PA_LNA_PDN_CC_OFFSET,
			 (trx_us - NRF_GPIO_PDN_OFFSET));
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
	gpiote_palna.p_reg->CONFIG[gpiote_ch_palna] = 0;
	gpiote_pdn.p_reg->CONFIG[gpiote_ch_pdn] = 0;
#else
	hal_radio_nrf_ppi_channels_disable(BIT(HAL_ENABLE_PALNA_PPI) |
					   BIT(HAL_DISABLE_PALNA_PPI));
	gpiote_palna.p_reg->CONFIG[gpiote_ch_palna] = 0;
#endif
}
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN || HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
struct ccm_job_ptr {
	void *ptr;
	struct {
		uint32_t length:24;
		uint32_t attribute:8;
	} __packed;
} __packed;

#define CCM_JOB_PTR_ATTRIBUTE_ALEN  11U
#define CCM_JOB_PTR_ATTRIBUTE_MLEN  12U
#define CCM_JOB_PTR_ATTRIBUTE_ADATA 13U
#define CCM_JOB_PTR_ATTRIBUTE_MDATA 14U

static struct {
	uint16_t in_alen;
	uint16_t in_mlen;
	uint8_t  in_mlen_msb;
	uint8_t  out_mlen_msb;
	uint16_t out_alen;
	struct ccm_job_ptr in[6];
	struct ccm_job_ptr out[6];
} ccm_job;

#else /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
static uint8_t MALIGN(4) _ccm_scratch[(HAL_RADIO_PDU_LEN_MAX - 4) + 16];
#endif /* !CONFIG_SOC_COMPATIBLE_NRF54LX */

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_SYNC_ISO)
static void *radio_ccm_ext_rx_pkt_set(struct ccm *cnf, uint8_t phy, uint8_t pdu_type, void *pkt)
{
	uint32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;

	mode = (CCM_MODE_MODE_Decryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	/* Enable CCM Protocol Mode BLE */
	mode |= (CCM_MODE_PROTOCOL_Ble << CCM_MODE_PROTOCOL_Pos) &
		CCM_MODE_PROTOCOL_Msk;

	/* Enable CCM MAC Length 4 bytes */
	mode |= (CCM_MODE_MACLEN_M4 << CCM_MODE_MACLEN_Pos) &
		CCM_MODE_MACLEN_Msk;

#elif !defined(CONFIG_SOC_SERIES_NRF51X)
	/* Enable CCM support for 8-bit length field PDUs. */
	mode |= (CCM_MODE_LENGTH_Extended << CCM_MODE_LENGTH_Pos) &
		CCM_MODE_LENGTH_Msk;
#endif /* !CONFIG_SOC_SERIES_NRF51X */

	/* Select CCM data rate based on current PHY in use. */
	switch (phy) {
	default:
	case PHY_1M:
#if !defined(CONFIG_SOC_SERIES_NRF51X)
		mode |= (CCM_MODE_DATARATE_1Mbit <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;
#endif /* !CONFIG_SOC_SERIES_NRF51X */

		if (false) {

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
		} else if (pdu_type == RADIO_PKT_CONF_PDU_TYPE_DC) {
			/* When direction finding CTE receive feature is enabled then on-the-fly
			 * PDU parsing for CTEInfo is always done. In such situation, the CCM
			 * TASKS_CRYPT must be started with short delay. That give the Radio time
			 * to store received bits in shared memory.
			 */
			radio_bc_configure(CCM_TASKS_CRYPT_DELAY_BITS);
			radio_bc_status_reset();
			hal_trigger_crypt_by_bcmatch_ppi_config();
			hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_DELAY_PPI));
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
		} else {
			hal_trigger_crypt_ppi_config();
			hal_radio_nrf_ppi_channels_enable(BIT(HAL_TRIGGER_CRYPT_PPI));
		}
		break;

	case PHY_2M:
#if !defined(CONFIG_SOC_SERIES_NRF51X)
		mode |= (CCM_MODE_DATARATE_2Mbit <<
			 CCM_MODE_DATARATE_Pos) &
			CCM_MODE_DATARATE_Msk;
#endif /* !CONFIG_SOC_SERIES_NRF51X */

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

	NRF_CCM->MODE = mode;

#if defined(CONFIG_HAS_HW_NRF_CCM_HEADERMASK) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#if defined(CONFIG_HAS_HW_NRF_CCM_HEADERMASK)
#define ADATAMASK HEADERMASK
#endif /* CONFIG_HAS_HW_NRF_CCM_HEADERMASK */
	switch (pdu_type) {
	case RADIO_PKT_CONF_PDU_TYPE_BIS:
		NRF_CCM->ADATAMASK = 0xC3; /* mask CSSN and CSTF */
		break;
	case RADIO_PKT_CONF_PDU_TYPE_CIS:
		NRF_CCM->ADATAMASK = 0xA3; /* mask SN, NESN, CIE and NPI */
		break;
	default:
		/* Using default reset value of ADATAMASK */
		NRF_CCM->ADATAMASK = 0xE3; /* mask SN, NESN and MD */
		break;
	}
#if defined(CONFIG_HAS_HW_NRF_CCM_HEADERMASK)
#undef ADATAMASK
#endif /* CONFIG_HAS_HW_NRF_CCM_HEADERMASK */
#endif /* CONFIG_HAS_HW_NRF_CCM_HEADERMASK ||
	* CONFIG_SOC_COMPATIBLE_NRF54LX
	*/

#if !defined(CONFIG_SOC_SERIES_NRF51X) && \
	!defined(CONFIG_SOC_NRF52832) && \
	!defined(CONFIG_SOC_COMPATIBLE_NRF54LX) && \
	(!defined(CONFIG_BT_CTLR_DATA_LENGTH_MAX) || \
	 (CONFIG_BT_CTLR_DATA_LENGTH_MAX < ((HAL_RADIO_PDU_LEN_MAX) - 4U)))
	const uint8_t max_len = (NRF_RADIO->PCNF1 & RADIO_PCNF1_MAXLEN_Msk) >>
				RADIO_PCNF1_MAXLEN_Pos;

	/* MAXPACKETSIZE value 0x001B (27) - 0x00FB (251) bytes */
	NRF_CCM->MAXPACKETSIZE = max_len - 4U;
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	/* Configure the CCM key, nonce and pointers */
	NRF_CCM->KEY.VALUE[3] = sys_get_be32(&cnf->key[0]);
	NRF_CCM->KEY.VALUE[2] = sys_get_be32(&cnf->key[4]);
	NRF_CCM->KEY.VALUE[1] = sys_get_be32(&cnf->key[8]);
	NRF_CCM->KEY.VALUE[0] = sys_get_be32(&cnf->key[12]);

	NRF_CCM->NONCE.VALUE[3] = ((uint8_t *)&cnf->counter)[0];
	NRF_CCM->NONCE.VALUE[2] = sys_get_be32(&((uint8_t *)&cnf->counter)[1]) |
				  (cnf->direction << 7);
	NRF_CCM->NONCE.VALUE[1] = sys_get_be32(&cnf->iv[0]);
	NRF_CCM->NONCE.VALUE[0] = sys_get_be32(&cnf->iv[4]);

	const uint8_t mlen = (NRF_RADIO->PCNF1 & RADIO_PCNF1_MAXLEN_Msk) >>
			     RADIO_PCNF1_MAXLEN_Pos;

	const uint8_t alen = sizeof(uint8_t);

	ccm_job.in_alen = alen;
	ccm_job.in[0].ptr = &ccm_job.in_alen;
	ccm_job.in[0].length = sizeof(ccm_job.in_alen);
	ccm_job.in[0].attribute = CCM_JOB_PTR_ATTRIBUTE_ALEN;

	ccm_job.in[1].ptr = (void *)((uint8_t *)_pkt_scratch + 1U);
	ccm_job.in[1].length = sizeof(uint8_t);
	ccm_job.in[1].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.in_mlen_msb = 0U;
	ccm_job.in[2].ptr = &ccm_job.in_mlen_msb;
	ccm_job.in[2].length = sizeof(ccm_job.in_mlen_msb);
	ccm_job.in[2].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.in[3].ptr = (void *)((uint8_t *)_pkt_scratch + 0U);
	ccm_job.in[3].length = sizeof(uint8_t);
	ccm_job.in[3].attribute = CCM_JOB_PTR_ATTRIBUTE_ADATA;

	ccm_job.in[4].ptr = (void *)((uint8_t *)_pkt_scratch + 3U);
	ccm_job.in[4].length = mlen;
	ccm_job.in[4].attribute = CCM_JOB_PTR_ATTRIBUTE_MDATA;

	ccm_job.in[5].ptr = NULL;
	ccm_job.in[5].length = 0U;
	ccm_job.in[5].attribute = 0U;

	ccm_job.out[0].ptr = &ccm_job.out_alen;
	ccm_job.out[0].length = sizeof(ccm_job.out_alen);
	ccm_job.out[0].attribute = CCM_JOB_PTR_ATTRIBUTE_ALEN;

	ccm_job.out[1].ptr = (void *)((uint8_t *)pkt + 1U);
	ccm_job.out[1].length = sizeof(uint8_t);
	ccm_job.out[1].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.out[2].ptr = &ccm_job.out_mlen_msb;
	ccm_job.out[2].length = sizeof(ccm_job.out_mlen_msb);
	ccm_job.out[2].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.out[3].ptr = (void *)((uint8_t *)pkt + 0U);
	ccm_job.out[3].length = sizeof(uint8_t);
	ccm_job.out[3].attribute = CCM_JOB_PTR_ATTRIBUTE_ADATA;

	ccm_job.out[4].ptr = (void *)((uint8_t *)pkt + 3U);
	ccm_job.out[4].length = mlen - sizeof(uint32_t);
	ccm_job.out[4].attribute = CCM_JOB_PTR_ATTRIBUTE_MDATA;

	ccm_job.out[5].ptr = NULL;
	ccm_job.out[5].length = 0U;
	ccm_job.out[5].attribute = 0U;

	NRF_CCM->IN.PTR = (uint32_t)ccm_job.in;
	NRF_CCM->OUT.PTR = (uint32_t)ccm_job.out;

	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_END);
	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_ERROR);

#else /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
	NRF_CCM->CNFPTR = (uint32_t)cnf;
	NRF_CCM->INPTR = (uint32_t)_pkt_scratch;
	NRF_CCM->OUTPTR = (uint32_t)pkt;
	NRF_CCM->SCRATCHPTR = (uint32_t)_ccm_scratch;
	NRF_CCM->SHORTS = 0;

	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_ENDKSGEN);
	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_END);
	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_ERROR);

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_KSGEN);
#endif /* !CONFIG_SOC_COMPATIBLE_NRF54LX */

	return _pkt_scratch;
}

void *radio_ccm_rx_pkt_set(struct ccm *cnf, uint8_t phy, void *pkt)
{
	return radio_ccm_ext_rx_pkt_set(cnf, phy, RADIO_PKT_CONF_PDU_TYPE_DC, pkt);
}

void *radio_ccm_iso_rx_pkt_set(struct ccm *cnf, uint8_t phy, uint8_t pdu_type, void *pkt)
{
	return radio_ccm_ext_rx_pkt_set(cnf, phy, pdu_type, pkt);
}
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_SYNC_ISO */

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_ADV_ISO)
static void *radio_ccm_ext_tx_pkt_set(struct ccm *cnf, uint8_t pdu_type, void *pkt)
{
	uint32_t mode;

	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Disabled;
	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled;

	mode = (CCM_MODE_MODE_Encryption << CCM_MODE_MODE_Pos) &
	       CCM_MODE_MODE_Msk;

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF53X)
	/* Enable CCM support for 8-bit length field PDUs. */
	mode |= (CCM_MODE_LENGTH_Extended << CCM_MODE_LENGTH_Pos) &
		CCM_MODE_LENGTH_Msk;

	/* NOTE: use fastest data rate as tx data needs to be prepared before
	 * radio Tx on any PHY.
	 */
	mode |= (CCM_MODE_DATARATE_2Mbit << CCM_MODE_DATARATE_Pos) &
		CCM_MODE_DATARATE_Msk;

#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	/* Enable CCM Protocol Mode BLE */
	mode |= (CCM_MODE_PROTOCOL_Ble << CCM_MODE_PROTOCOL_Pos) &
		CCM_MODE_PROTOCOL_Msk;

	/* NOTE: use fastest data rate as tx data needs to be prepared before
	 * radio Tx on any PHY.
	 */
	mode |= (CCM_MODE_DATARATE_4Mbit << CCM_MODE_DATARATE_Pos) &
		CCM_MODE_DATARATE_Msk;

	/* Enable CCM MAC Length 4 bytes */
	mode |= (CCM_MODE_MACLEN_M4 << CCM_MODE_MACLEN_Pos) &
		CCM_MODE_MACLEN_Msk;
#endif

	NRF_CCM->MODE = mode;

#if defined(CONFIG_HAS_HW_NRF_CCM_HEADERMASK) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#if defined(CONFIG_HAS_HW_NRF_CCM_HEADERMASK)
#define ADATAMASK HEADERMASK
#endif /* CONFIG_HAS_HW_NRF_CCM_HEADERMASK */
	switch (pdu_type) {
	case RADIO_PKT_CONF_PDU_TYPE_BIS:
		NRF_CCM->ADATAMASK = 0xC3; /* mask CSSN and CSTF */
		break;
	case RADIO_PKT_CONF_PDU_TYPE_CIS:
		NRF_CCM->ADATAMASK = 0xA3; /* mask SN, NESN, CIE and NPI */
		break;
	default:
		/* Using default reset value of ADATAMASK */
		NRF_CCM->ADATAMASK = 0xE3; /* mask SN, NESN and MD */
		break;
	}
#if defined(CONFIG_HAS_HW_NRF_CCM_HEADERMASK)
#undef ADATAMASK
#endif /* CONFIG_HAS_HW_NRF_CCM_HEADERMASK */
#endif /* CONFIG_HAS_HW_NRF_CCM_HEADERMASK ||
	* CONFIG_SOC_COMPATIBLE_NRF54LX
	*/

#if !defined(CONFIG_SOC_SERIES_NRF51X) && \
	!defined(CONFIG_SOC_NRF52832) && \
	!defined(CONFIG_SOC_COMPATIBLE_NRF54LX) && \
	(!defined(CONFIG_BT_CTLR_DATA_LENGTH_MAX) || \
	 (CONFIG_BT_CTLR_DATA_LENGTH_MAX < ((HAL_RADIO_PDU_LEN_MAX) - 4U)))
	const uint8_t max_len = (NRF_RADIO->PCNF1 & RADIO_PCNF1_MAXLEN_Msk) >>
				RADIO_PCNF1_MAXLEN_Pos;

	/* MAXPACKETSIZE value 0x001B (27) - 0x00FB (251) bytes */
	NRF_CCM->MAXPACKETSIZE = max_len - 4U;
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	/* Configure the CCM key, nonce and pointers */
	NRF_CCM->KEY.VALUE[3] = sys_get_be32(&cnf->key[0]);
	NRF_CCM->KEY.VALUE[2] = sys_get_be32(&cnf->key[4]);
	NRF_CCM->KEY.VALUE[1] = sys_get_be32(&cnf->key[8]);
	NRF_CCM->KEY.VALUE[0] = sys_get_be32(&cnf->key[12]);

	NRF_CCM->NONCE.VALUE[3] = ((uint8_t *)&cnf->counter)[0];
	NRF_CCM->NONCE.VALUE[2] = sys_get_be32(&((uint8_t *)&cnf->counter)[1]) |
				  (cnf->direction << 7);
	NRF_CCM->NONCE.VALUE[1] = sys_get_be32(&cnf->iv[0]);
	NRF_CCM->NONCE.VALUE[0] = sys_get_be32(&cnf->iv[4]);

	const uint8_t alen = sizeof(uint8_t);

	ccm_job.in_alen = alen;
	ccm_job.in[0].ptr = &ccm_job.in_alen;
	ccm_job.in[0].length = sizeof(ccm_job.in_alen);
	ccm_job.in[0].attribute = CCM_JOB_PTR_ATTRIBUTE_ALEN;

	const uint8_t mlen = *((uint8_t *)pkt + 1U);

	ccm_job.in_mlen = mlen;
	ccm_job.in[1].ptr = &ccm_job.in_mlen;
	ccm_job.in[1].length = sizeof(ccm_job.in_mlen);
	ccm_job.in[1].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.in[2].ptr = (void *)((uint8_t *)pkt + 0U);
	ccm_job.in[2].length = alen;
	ccm_job.in[2].attribute = CCM_JOB_PTR_ATTRIBUTE_ADATA;

	ccm_job.in[3].ptr = (void *)((uint8_t *)pkt + 3U);
	ccm_job.in[3].length = mlen;
	ccm_job.in[3].attribute = CCM_JOB_PTR_ATTRIBUTE_MDATA;

	ccm_job.in[4].ptr = NULL;
	ccm_job.in[4].length = 0U;
	ccm_job.in[4].attribute = 0U;

	ccm_job.out[0].ptr = &ccm_job.out_alen;
	ccm_job.out[0].length = sizeof(ccm_job.out_alen);
	ccm_job.out[0].attribute = CCM_JOB_PTR_ATTRIBUTE_ALEN;

	ccm_job.out[1].ptr = (void *)((uint8_t *)_pkt_scratch + 1U);
	ccm_job.out[1].length = sizeof(uint8_t);
	ccm_job.out[1].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.out[2].ptr = &ccm_job.out_mlen_msb;
	ccm_job.out[2].length = sizeof(ccm_job.out_mlen_msb);
	ccm_job.out[2].attribute = CCM_JOB_PTR_ATTRIBUTE_MLEN;

	ccm_job.out[3].ptr = (void *)((uint8_t *)_pkt_scratch + 0U);
	ccm_job.out[3].length = sizeof(uint8_t);
	ccm_job.out[3].attribute = CCM_JOB_PTR_ATTRIBUTE_ADATA;

	ccm_job.out[4].ptr = (void *)((uint8_t *)_pkt_scratch + 3U);
	ccm_job.out[4].length = mlen + sizeof(uint32_t);
	ccm_job.out[4].attribute = CCM_JOB_PTR_ATTRIBUTE_MDATA;

	ccm_job.out[5].ptr = NULL;
	ccm_job.out[5].length = 0U;
	ccm_job.out[5].attribute = 0U;

	NRF_CCM->IN.PTR = (uint32_t)ccm_job.in;
	NRF_CCM->OUT.PTR = (uint32_t)ccm_job.out;

	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_END);
	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_ERROR);

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_START);

#else /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
	NRF_CCM->CNFPTR = (uint32_t)cnf;
	NRF_CCM->INPTR = (uint32_t)pkt;
	NRF_CCM->OUTPTR = (uint32_t)_pkt_scratch;
	NRF_CCM->SCRATCHPTR = (uint32_t)_ccm_scratch;
	NRF_CCM->SHORTS = CCM_SHORTS_ENDKSGEN_CRYPT_Msk;

	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_ENDKSGEN);
	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_END);
	nrf_ccm_event_clear(NRF_CCM, NRF_CCM_EVENT_ERROR);

	nrf_ccm_task_trigger(NRF_CCM, NRF_CCM_TASK_KSGEN);
#endif /* !CONFIG_SOC_COMPATIBLE_NRF54LX */

	return _pkt_scratch;
}

void *radio_ccm_tx_pkt_set(struct ccm *cnf, void *pkt)
{
	return radio_ccm_ext_tx_pkt_set(cnf, RADIO_PKT_CONF_PDU_TYPE_DC, pkt);
}

void *radio_ccm_iso_tx_pkt_set(struct ccm *cnf, uint8_t pdu_type, void *pkt)
{
	return radio_ccm_ext_tx_pkt_set(cnf, pdu_type, pkt);
}
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_ADV_ISO */

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

	nrf_aar_event_clear(NRF_AAR, NRF_AAR_EVENT_END);
	nrf_aar_event_clear(NRF_AAR, NRF_AAR_EVENT_RESOLVED);
	nrf_aar_event_clear(NRF_AAR, NRF_AAR_EVENT_NOTRESOLVED);

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

	nrf_aar_event_clear(NRF_AAR, NRF_AAR_EVENT_END);
	nrf_aar_event_clear(NRF_AAR, NRF_AAR_EVENT_RESOLVED);
	nrf_aar_event_clear(NRF_AAR, NRF_AAR_EVENT_NOTRESOLVED);

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
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */

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
