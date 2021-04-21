/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <devicetree.h>
#include <sys/util_macro.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_gpio.h>

#include "radio_nrf5.h"
#include "radio_df.h"

/* @brief Minimum antennas number required if antenna switching is enabled */
#define DF_ANT_NUM_MIN 2
/* @brief Value that used to check if PDU antenna pattern is not set */
#define DF_PDU_ANT_NOT_SET 0xFF
/* @brief Value to set antenna GPIO pin as not connected */
#define DF_GPIO_PIN_NOT_SET 0xFF
/* @brief Number of PSEL_DFEGPIO registers in Radio peripheral */
#define DF_PSEL_GPIO_NUM 8
/* @brief Maximum index number related with count of PSEL_DFEGPIO registers in
 *        Radio peripheral. It is used in macros only e.g. UTIL_LISTIFY that
 *        evaluate indices in an inclusive way.
 */
#define DF_PSEL_GPIO_NUM_MAX_IDX 7

/* @brief Direction Finding antenna matrix configuration */
struct df_ant_cfg {
	uint8_t ant_num;
	/* Selection of GPIOs to be used to switch antennas by Radio */
	uint8_t dfe_gpio[DF_PSEL_GPIO_NUM];
};

#define RADIO DT_NODELABEL(radio)
#define DFE_GPIO_PSEL(idx)                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(RADIO, dfegpio##idx##_gpios),        \
		   (NRF_DT_GPIOS_TO_PSEL(RADIO, dfegpio##idx##_gpios)),   \
		   (DF_GPIO_PIN_NOT_SET))

#define DFE_GPIO_PIN_DISCONNECT (RADIO_PSEL_DFEGPIO_CONNECT_Disconnected << \
				 RADIO_PSEL_DFEGPIO_CONNECT_Pos)

#define COUNT_GPIO(idx, _) + DT_NODE_HAS_PROP(RADIO, dfegpio##idx##_gpios)
#define DFE_GPIO_NUM        (UTIL_LISTIFY(DF_PSEL_GPIO_NUM, COUNT_GPIO, _))

/* DFE_GPIO_NUM_IS_ZERO is required to correctly compile COND_CODE_1 in
 * DFE_GPIO_ALLOWED_ANT_NUM macro. DFE_GPIO_NUM does not expand to literal 1
 * So it is not possible to use it as a conditional in COND_CODE_1 argument.
 */
#if (DFE_GPIO_NUM > 0)
#define DFE_GPIO_NUM_IS_ZERO 1
#else
#define DFE_GPIO_NUM_IS_ZERO EMPTY
#endif

#define DFE_GPIO_ALLOWED_ANT_NUM COND_CODE_1(DFE_GPIO_NUM_IS_ZERO,       \
					     (BIT(DFE_GPIO_NUM)), (0))

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) || \
	defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)

/* Check if there is enough pins configured to represent each pattern
 * for given antennas number.
 */
BUILD_ASSERT((DT_PROP(RADIO, dfe_antenna_num) <= DFE_GPIO_ALLOWED_ANT_NUM), "Insufficient "
	     "number of GPIO pins configured.");
BUILD_ASSERT((DT_PROP(RADIO, dfe_antenna_num) >= DF_ANT_NUM_MIN), "Insufficient "
	     "number of antennas provided.");
BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_CTLR_DF_INIT_ANT_SEL_GPIOS) ||
	     (DT_PROP(RADIO, dfe_pdu_antenna) != DF_PDU_ANT_NOT_SET), "Missing "
	     "antenna pattern used to select antenna for PDU Tx.");

/* Check if dfegpio#-gios property has flag cell set to zero */
#define DFE_GPIO_PIN_FLAGS(idx) (DT_GPIO_FLAGS(RADIO, dfegpio##idx##_gpios))
#define DFE_GPIO_PIN_IS_FLAG_ZERO(idx)                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(RADIO, dfegpio##idx##_gpios),           \
		    (BUILD_ASSERT((DFE_GPIO_PIN_FLAGS(idx) == 0),            \
				  "Flags value of GPIO pin property must be" \
				  "zero.")),                                 \
		    (EMPTY))

#define DFE_GPIO_PIN_LIST(idx, _) idx,
FOR_EACH(DFE_GPIO_PIN_IS_FLAG_ZERO, (;),
	UTIL_LISTIFY(DF_PSEL_GPIO_NUM_MAX_IDX, DFE_GPIO_PIN_LIST))

#if DT_NODE_HAS_STATUS(RADIO, okay)
const static struct df_ant_cfg ant_cfg = {
	.ant_num = DT_PROP(RADIO, dfe_antenna_num),
	.dfe_gpio = {
		FOR_EACH(DFE_GPIO_PSEL, (,),
			 UTIL_LISTIFY(DF_PSEL_GPIO_NUM_MAX_IDX, DFE_GPIO_PIN_LIST))
	}
};
#else
#error "DF antenna switching feature requires dfe_antenna_num to be enabled in DTS"
#endif

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

	for (uint8_t idx = 0; idx < DF_PSEL_GPIO_NUM; ++idx) {
		pin_sel = ant_cfg.dfe_gpio[idx];

		if (pin_sel != DF_GPIO_PIN_NOT_SET) {
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

	for (uint8_t idx = 0; idx < DF_PSEL_GPIO_NUM; ++idx) {
		pin_sel = ant_cfg.dfe_gpio[idx];
		if (pin_sel != DF_GPIO_PIN_NOT_SET) {
			nrf_gpio_cfg_output(pin_sel);

			if (BIT(idx) & DT_PROP(RADIO, dfe_pdu_antenna)) {
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
 * @return	Number of available antennas.
 */
uint8_t radio_df_ant_num_get(void)
{
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) || \
	defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	return ant_cfg.ant_num;
#else
	return 0;
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

void radio_df_cte_inline_set(uint8_t enable)
{
	NRF_RADIO->CTEINLINECONF &= ~RADIO_CTEINLINECONF_CTEINLINECTRLEN_Msk;
	NRF_RADIO->CTEINLINECONF |= ((enable <<
				      RADIO_CTEINLINECONF_CTEINLINECTRLEN_Pos)
				     & RADIO_CTEINLINECONF_CTEINLINECTRLEN_Msk);
}

void radio_df_cte_length_set(uint8_t value)
{
	NRF_RADIO->DFECTRL1 &= ~RADIO_DFECTRL1_NUMBEROF8US_Msk;
	NRF_RADIO->DFECTRL1 |= ((value << RADIO_DFECTRL1_NUMBEROF8US_Pos)
				& RADIO_DFECTRL1_NUMBEROF8US_Msk);
}

void radio_df_ant_switch_pattern_clear(void)
{
	NRF_RADIO->CLEARPATTERN = RADIO_CLEARPATTERN_CLEARPATTERN_Clear;
}

static inline void radio_df_ant_switch_spacing_set(uint8_t spacing)
{
	NRF_RADIO->DFECTRL1 &= ~RADIO_DFECTRL1_TSWITCHSPACING_Msk;
	NRF_RADIO->DFECTRL1 |= ((spacing << RADIO_DFECTRL1_TSWITCHSPACING_Pos)
				& RADIO_DFECTRL1_TSWITCHSPACING_Msk);
}

void radio_df_ant_switch_spacing_set_2us(void)
{
	radio_df_ant_switch_spacing_set(RADIO_DFECTRL1_TSWITCHSPACING_2us);
}

void radio_df_ant_switch_spacing_set_4us(void)
{
	radio_df_ant_switch_spacing_set(RADIO_DFECTRL1_TSWITCHSPACING_4us);
}

void radio_df_ant_switch_pattern_set(uint8_t *patterns, uint8_t len)
{
	/* SWITCHPATTERN is like a moving pointer to underlying buffer.
	 * Each write stores a value and moves the pointer to new free position.
	 * When read it returns number of stored elements since last write to
	 * CLEARPATTERN. There is no need to use subscript operator.
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
	NRF_RADIO->SWITCHPATTERN = DT_PROP(RADIO, dfe_pdu_antenna);
	for (uint8_t idx = 0; idx < len; ++idx) {
		NRF_RADIO->SWITCHPATTERN = patterns[idx];
	}
}

void radio_df_reset(void)
{
	radio_df_mode_set(RADIO_DFEMODE_DFEOPMODE_Disabled);
	radio_df_cte_inline_set(RADIO_CTEINLINECONF_CTEINLINECTRLEN_Disabled);
	radio_df_ant_switch_pattern_clear();
}

void radio_switch_complete_and_phy_end_disable(void)
{
	NRF_RADIO->SHORTS =
	       (RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_PHYEND_DISABLE_Msk);

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
	hal_radio_sw_switch_disable();
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
}
