/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_gpio.h>
#include <hal/ccm.h>

#include "ll_sw/pdu.h"

#include "radio_nrf5.h"
#include "radio.h"
#include "radio_df.h"
#include "radio_internal.h"

/* Devicetree node identifier for the radio node. */
#define RADIO_NODE DT_NODELABEL(radio)

/* Value to set for unconnected antenna GPIO pins. */
#define DFE_PSEL_NOT_SET 0xFF
/* Number of PSEL_DFEGPIO[n] registers in the radio peripheral. */
#define MAX_DFE_GPIO 8
/* Run a macro 'fn' on each available DFE GPIO index, from 0 to
 * MAX_DFE_GPIO-1, with the given parenthesized separator.
 */
#define FOR_EACH_DFE_GPIO(fn, sep) \
	FOR_EACH(fn, sep, 0, 1, 2, 3, 4, 5, 6, 7)

/* Index of antenna id in antenna switching pattern used for GUARD and REFERENCE period */
#define GUARD_REF_ANTENNA_PATTERN_IDX 0U

/* Direction Finding antenna matrix configuration */
struct df_ant_cfg {
	uint8_t ant_num;
	/* Selection of GPIOs to be used to switch antennas by Radio */
	uint8_t dfe_gpio[MAX_DFE_GPIO];
};

#define DFE_GPIO_PSEL(idx)					  \
	NRF_DT_GPIOS_TO_PSEL_OR(RADIO_NODE, dfegpio##idx##_gpios, \
				DFE_PSEL_NOT_SET)

#define DFE_GPIO_PIN_DISCONNECT (RADIO_PSEL_DFEGPIO_CONNECT_Disconnected << \
				 RADIO_PSEL_DFEGPIO_CONNECT_Pos)

#define HAS_DFE_GPIO(idx) DT_NODE_HAS_PROP(RADIO_NODE, dfegpio##idx##_gpios)

/* The number of dfegpio[n]-gpios properties which are set. */
#define DFE_GPIO_NUM (FOR_EACH_DFE_GPIO(HAS_DFE_GPIO, (+)))

/* The minimum number of antennas required to enable antenna switching. */
#define MIN_ANTENNA_NUM 2

/* The maximum number of antennas supported by the number of
 * dfegpio[n]-gpios properties which are set.
 */
#if (DFE_GPIO_NUM > 0)
#define MAX_ANTENNA_NUM BIT(DFE_GPIO_NUM)
#else
#define MAX_ANTENNA_NUM 0
#endif

uint8_t radio_df_pdu_antenna_switch_pattern_get(void)
{
	return PDU_ANTENNA;
}

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) || \
	defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)

/*
 * Check that we have an antenna switch pattern for the DFE idle
 * state. (In DFE idle state, the radio peripheral transmits or
 * receives PDUs.)
 */

#define HAS_PDU_ANTENNA DT_NODE_HAS_PROP(RADIO_NODE, dfe_pdu_antenna)

BUILD_ASSERT(HAS_PDU_ANTENNA,
	     "Missing antenna pattern used to select antenna for PDU Tx "
	     "during the DFE Idle state. "
	     "Set the dfe-pdu-antenna devicetree property.");

void radio_df_ant_switch_pattern_set(const uint8_t *patterns, uint8_t len)
{
	/* SWITCHPATTERN is like a moving pointer to an underlying buffer.
	 * Each write stores a value and moves the pointer to new free position.
	 * When read it returns number of stored elements since last write to
	 * CLEARPATTERN. There is no need to use a subscript operator.
	 *
	 * Some storage entries in the buffer has special purpose for DFE
	 * extension in radio:
	 * - SWITCHPATTERN[0] for idle period (PDU Tx/Rx),
	 * - SWITCHPATTERN[1] for guard and reference period,
	 * - SWITCHPATTERN[2] and following for switch-sampling slots.
	 * Due to that in SWITCHPATTER[0] there is stored a pattern provided by
	 * DTS property dfe_pdu_antenna. This limits number of supported antenna
	 * switch patterns by one.
	 */
	NRF_RADIO->SWITCHPATTERN = PDU_ANTENNA;
	for (uint8_t idx = 0; idx < len; ++idx) {
		NRF_RADIO->SWITCHPATTERN = patterns[idx];
	}

	/* Store antenna id used for GUARD and REFERENCE period at the end of SWITCHPATTERN buffer.
	 * It is required to apply reference antenna id when user provided switchpattern is
	 * exhausted.
	 * Maximum length of the switch pattern provided to this function is at maximum lower by one
	 * than capacity of SWITCHPATTERN buffer. Hence there is always space for reference antenna
	 * id after end of switch pattern.
	 */
	NRF_RADIO->SWITCHPATTERN = patterns[GUARD_REF_ANTENNA_PATTERN_IDX];
}

/*
 * Check that the number of antennas has been set, and that enough
 * pins are configured to represent each pattern for the given number
 * of antennas.
 */

#define HAS_ANTENNA_NUM DT_NODE_HAS_PROP(RADIO_NODE, dfe_antenna_num)

BUILD_ASSERT(HAS_ANTENNA_NUM,
	     "You must set the dfe-antenna-num property in the radio node "
	     "to enable antenna switching.");

#define ANTENNA_NUM DT_PROP_OR(RADIO_NODE, dfe_antenna_num, 0)

BUILD_ASSERT(!HAS_ANTENNA_NUM || (ANTENNA_NUM <= MAX_ANTENNA_NUM),
	     "Insufficient number of GPIO pins configured. "
	     "Set more dfegpio[n]-gpios properties.");
BUILD_ASSERT(!HAS_ANTENNA_NUM || (ANTENNA_NUM >= MIN_ANTENNA_NUM),
	     "Insufficient number of antennas provided. "
	     "Increase the dfe-antenna-num property.");

/*
 * Check that each dfegpio[n]-gpios property has a zero flags cell.
 */

#define ASSERT_DFE_GPIO_FLAGS_ARE_ZERO(idx)				   \
	BUILD_ASSERT(DT_GPIO_FLAGS(RADIO_NODE, dfegpio##idx##_gpios) == 0, \
		     "The flags cell in each dfegpio[n]-gpios "		   \
		     "property must be zero.")

FOR_EACH_DFE_GPIO(ASSERT_DFE_GPIO_FLAGS_ARE_ZERO, (;));

/* Stores the dfegpio[n]-gpios property values.
 */
const static struct df_ant_cfg ant_cfg = {
	.ant_num = ANTENNA_NUM,
	.dfe_gpio = { FOR_EACH_DFE_GPIO(DFE_GPIO_PSEL, (,)) }
};

/* @brief Function configures Radio with information about GPIO pins that may be
 *        used to drive antenna switching during CTE Tx/RX.
 *
 * Sets up DF related PSEL.DFEGPIO registers to give possibility to Radio
 * to drive antennas switches.
 *
 */
void radio_df_ant_switching_pin_sel_cfg(void)
{
	uint8_t pin_sel;

	for (uint8_t idx = 0; idx < MAX_DFE_GPIO; ++idx) {
		pin_sel = ant_cfg.dfe_gpio[idx];

		if (pin_sel != DFE_PSEL_NOT_SET) {
			nrf_radio_dfe_pattern_pin_set(NRF_RADIO,
						      pin_sel,
						      idx);
		} else {
			nrf_radio_dfe_pattern_pin_set(NRF_RADIO,
						      DFE_GPIO_PIN_DISCONNECT,
						      idx);
		}
	}
}

#if defined(CONFIG_BT_CTLR_DF_INIT_ANT_SEL_GPIOS)
/* @brief Function configures GPIO pins that will be used by Direction Finding
 *        Extension for antenna switching.
 *
 * Function configures antenna selection GPIO pins in GPIO peripheral.
 * Also sets pin outputs to match state in SWITCHPATTERN[0] that is used
 * to enable antenna for PDU Rx/Tx. That has to prevent glitches when
 * DFE is powered off after end of transmission.
 *
 * @return	Zero in case of success, other value in case of failure.
 */
void radio_df_ant_switching_gpios_cfg(void)
{
	uint8_t pin_sel;

	for (uint8_t idx = 0; idx < MAX_DFE_GPIO; ++idx) {
		pin_sel = ant_cfg.dfe_gpio[idx];
		if (pin_sel != DFE_PSEL_NOT_SET) {
			nrf_gpio_cfg_output(pin_sel);

			if (BIT(idx) & PDU_ANTENNA) {
				nrf_gpio_pin_set(pin_sel);
			} else {
				nrf_gpio_pin_clear(pin_sel);
			}
		}
	}
}
#endif /* CONFIG_BT_CTLR_DF_INIT_ANT_SEL_GPIOS */
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_TX || CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

/* @brief Function provides number of available antennas for Direction Finding.
 *
 * The number of antennas is hardware defined. It is provided via devicetree.
 *
 * If antenna switching is not enabled then there must be a single antenna
 * responsible for PDU reception and transmission.
 *
 * @return	Number of available antennas.
 */
uint8_t radio_df_ant_num_get(void)
{
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) || \
	defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	return ant_cfg.ant_num;
#else
	return 1U;
#endif
}

static inline void radio_df_mode_set(uint8_t mode)
{
	NRF_RADIO->DFEMODE &= ~RADIO_DFEMODE_DFEOPMODE_Msk;
	NRF_RADIO->DFEMODE |= ((mode << RADIO_DFEMODE_DFEOPMODE_Pos)
			       & RADIO_DFEMODE_DFEOPMODE_Msk);
}

void radio_df_mode_set_aoa(void)
{
	radio_df_mode_set(NRF_RADIO_DFE_OP_MODE_AOA);
}

void radio_df_mode_set_aod(void)
{
	radio_df_mode_set(NRF_RADIO_DFE_OP_MODE_AOD);
}

static void radio_df_cte_inline_set_disabled(void)
{
	NRF_RADIO->CTEINLINECONF &= ~RADIO_CTEINLINECONF_CTEINLINECTRLEN_Msk;
	NRF_RADIO->CTEINLINECONF |= ((RADIO_CTEINLINECONF_CTEINLINECTRLEN_Disabled <<
				      RADIO_CTEINLINECONF_CTEINLINECTRLEN_Pos)
				     & RADIO_CTEINLINECONF_CTEINLINECTRLEN_Msk);
}

static inline void radio_df_ctrl_set(uint8_t cte_len,
				     uint8_t switch_spacing,
				     uint8_t sample_spacing,
				     uint8_t phy)
{
	uint16_t sample_offset;
	uint32_t conf;

	/* Complete setup is done on purpose, to be sure that there isn't left
	 * any unexpected state in the register.
	 */
	conf = ((((uint32_t)cte_len << RADIO_DFECTRL1_NUMBEROF8US_Pos) &
				       RADIO_DFECTRL1_NUMBEROF8US_Msk) |
		((uint32_t)RADIO_DFECTRL1_DFEINEXTENSION_CRC <<
			RADIO_DFECTRL1_DFEINEXTENSION_Pos) |
		((uint32_t)switch_spacing << RADIO_DFECTRL1_TSWITCHSPACING_Pos) |
		((uint32_t)NRF_RADIO_DFECTRL_SAMPLE_SPACING_1US <<
			RADIO_DFECTRL1_TSAMPLESPACINGREF_Pos) |
		((uint32_t)NRF_RADIO_DFECTRL_SAMPLE_TYPE_IQ <<
			RADIO_DFECTRL1_SAMPLETYPE_Pos) |
		((uint32_t)sample_spacing << RADIO_DFECTRL1_TSAMPLESPACING_Pos) |
		(((uint32_t)0 << RADIO_DFECTRL1_AGCBACKOFFGAIN_Pos) &
				 RADIO_DFECTRL1_AGCBACKOFFGAIN_Msk));

	NRF_RADIO->DFECTRL1 = conf;

	switch (phy) {
	case PHY_1M:
		if (switch_spacing == RADIO_DFECTRL1_TSWITCHSPACING_2us) {
			sample_offset = CONFIG_BT_CTLR_DF_SAMPLE_OFFSET_PHY_1M_SAMPLING_1US;
		} else if (switch_spacing == RADIO_DFECTRL1_TSWITCHSPACING_4us) {
			sample_offset = CONFIG_BT_CTLR_DF_SAMPLE_OFFSET_PHY_1M_SAMPLING_2US;
		} else {
			sample_offset = 0;
		}
		break;
	case PHY_2M:
		if (switch_spacing == RADIO_DFECTRL1_TSWITCHSPACING_2us) {
			sample_offset = CONFIG_BT_CTLR_DF_SAMPLE_OFFSET_PHY_2M_SAMPLING_1US;
		} else if (switch_spacing == RADIO_DFECTRL1_TSWITCHSPACING_4us) {
			sample_offset = CONFIG_BT_CTLR_DF_SAMPLE_OFFSET_PHY_2M_SAMPLING_2US;
		} else {
			sample_offset = 0;
		}
		break;
	case PHY_LEGACY:
	default:
		/* If phy is set to legacy, the function is called in TX context and actual value
		 * does not matter, hence it is set to default zero.
		 */
		sample_offset = 0;
	}

	conf = ((((uint32_t)sample_offset << RADIO_DFECTRL2_TSAMPLEOFFSET_Pos) &
				       RADIO_DFECTRL2_TSAMPLEOFFSET_Msk) |
		(((uint32_t)CONFIG_BT_CTLR_DF_SWITCH_OFFSET << RADIO_DFECTRL2_TSWITCHOFFSET_Pos) &
				       RADIO_DFECTRL2_TSWITCHOFFSET_Msk));

	NRF_RADIO->DFECTRL2 = conf;
}

void radio_df_cte_tx_aod_2us_set(uint8_t cte_len)
{
	/* Sample spacing does not matter for AoD Tx. It is set to value
	 * that is in DFECTRL1 register after reset. That is done instead of
	 * adding conditions on the value and masking of the field before
	 * storing configuration in the register. Also values in DFECTRL2,
	 * that depend on PHY, are irrelevant for AoD Tx, hence use of
	 * PHY_LEGACY here.
	 */
	radio_df_ctrl_set(cte_len, RADIO_DFECTRL1_TSWITCHSPACING_2us,
			  RADIO_DFECTRL1_TSAMPLESPACING_2us, PHY_LEGACY);
}

void radio_df_cte_tx_aod_4us_set(uint8_t cte_len)
{
	/* Sample spacing does not matter for AoD Tx. It is set to value
	 * that is in DFECTRL1 register after reset. That is done instead of
	 * adding conditions on the value and masking of the field before
	 * storing configuration in the register. Also values in DFECTRL2,
	 * that depend on PHY, are irrelevant for AoD Tx, hence use of
	 * PHY_LEGACY here.
	 */
	radio_df_ctrl_set(cte_len, RADIO_DFECTRL1_TSWITCHSPACING_4us,
			  RADIO_DFECTRL1_TSAMPLESPACING_2us, PHY_LEGACY);
}

void radio_df_cte_tx_aoa_set(uint8_t cte_len)
{
	/* Switch and sample spacing does not matter for AoA Tx. It is set to
	 * value that is in DFECTRL1 register after reset. That is done instead
	 * of adding conditions on the value and masking of the field before
	 * storing configuration in the register. Also values in DFECTRL2,
	 * that depend on PHY, are irrelevant for AoA Tx, hence use of
	 * PHY_LEGACY here.
	 */
	radio_df_ctrl_set(cte_len, RADIO_DFECTRL1_TSWITCHSPACING_4us,
			  RADIO_DFECTRL1_TSAMPLESPACING_2us, PHY_LEGACY);
}

void radio_df_cte_rx_2us_switching(bool cte_info_in_s1, uint8_t phy)
{
	/* BT spec requires single sample for a single switching slot, so
	 * spacing for slot and samples is the same.
	 * CTE duration is used only when CTEINLINE config is disabled.
	 */
	radio_df_ctrl_set(0, RADIO_DFECTRL1_TSWITCHSPACING_2us,
			  RADIO_DFECTRL1_TSAMPLESPACING_2us, phy);
	radio_df_cte_inline_set_enabled(cte_info_in_s1);
}

void radio_df_cte_rx_4us_switching(bool cte_info_in_s1, uint8_t phy)
{
	/* BT spec requires single sample for a single switching slot, so
	 * spacing for slot and samples is the same.
	 * CTE duration is used only when CTEINLINE config is disabled.
	 */
	radio_df_ctrl_set(0, RADIO_DFECTRL1_TSWITCHSPACING_4us,
			  RADIO_DFECTRL1_TSAMPLESPACING_4us, phy);
	radio_df_cte_inline_set_enabled(cte_info_in_s1);
}

void radio_df_ant_switch_pattern_clear(void)
{
	NRF_RADIO->CLEARPATTERN = RADIO_CLEARPATTERN_CLEARPATTERN_Clear;
}

void radio_df_reset(void)
{
	radio_df_mode_set(RADIO_DFEMODE_DFEOPMODE_Disabled);
	radio_df_cte_inline_set_disabled();
	radio_df_ant_switch_pattern_clear();
}

void radio_switch_complete_and_phy_end_b2b_tx(uint8_t phy_curr, uint8_t flags_curr,
					      uint8_t phy_next, uint8_t flags_next)
{
#if defined(CONFIG_BT_CTLR_TIFS_HW)
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |
			    RADIO_SHORTS_DISABLED_TXEN_Msk;
#else /* !CONFIG_BT_CTLR_TIFS_HW */
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | NRF_RADIO_SHORTS_PDU_END_DISABLE;
	sw_switch(SW_SWITCH_TX, SW_SWITCH_TX, phy_curr, flags_curr, phy_next, flags_next,
		  END_EVT_DELAY_DISABLED);
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}

void radio_df_iq_data_packet_set(uint8_t *buffer, size_t len)
{
	nrf_radio_dfe_buffer_set(NRF_RADIO, (uint32_t *)buffer, len);
}

uint32_t radio_df_iq_samples_amount_get(void)
{
	return nrf_radio_dfe_amount_get(NRF_RADIO);
}

uint8_t radio_df_cte_status_get(void)
{
	return NRF_RADIO->CTESTATUS;
}

bool radio_df_cte_ready(void)
{
	return (NRF_RADIO->EVENTS_CTEPRESENT != 0);
}
