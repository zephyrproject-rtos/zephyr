/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2019 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <hal/nrf_vreqctrl.h>

/* NRF Radio HW timing constants
 * - provided in US and NS (for higher granularity)
 * - based on the timings configured in the HW models, which are based
 *   on the product specification
 * - Note that this timings are approx. the same as in the real HW,
 *   but tend to be rounded to the nearest microsecond
 */

/* Override EVENT_TIMER_ID from 4 to 0, as nRF5340 does not have 4 timer
 * instances.
 */
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
#undef EVENT_TIMER_ID
#define EVENT_TIMER_ID 0

#undef EVENT_TIMER
#define EVENT_TIMER    _CONCAT(NRF_TIMER, EVENT_TIMER_ID)

#undef SW_SWITCH_TIMER
#define SW_SWITCH_TIMER EVENT_TIMER
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_NS 41000
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NS 141000
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
 /* 129.5 + 0.8 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS 130000
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_NS 40000
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NS 140000
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS 129000
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_NS 40000
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NS 140000
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
/* 129.5 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS 129000
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_NS 40000
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NS 140000
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
/* 129.5 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS 129000
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS)

#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_NS     1000
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_TX_CHAIN_DELAY_NS)

#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_US  9
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_NS  9000
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_US  5
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_NS  5000

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_NS

#else /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
#if defined(CONFIG_BT_CTLR_TIFS_HW)
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NS

#else /* !CONFIG_BT_CTLR_TIFS_HW */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS

#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */

/* nRF5340 supports +3dBm Tx Power using high voltage request, define +3dBm
 * value for Controller use.
 */
#ifndef RADIO_TXPOWER_TXPOWER_Pos3dBm
#define RADIO_TXPOWER_TXPOWER_Pos3dBm (0x03UL)
#endif

/* SoC specific NRF_RADIO power-on reset value. Refer to Product Specification,
 * RADIO Registers section for the documented reset values.
 *
 * NOTE: Only implementation used values defined here.
 *       In the future if MDK or nRFx header include these, use them instead.
 */
#define HAL_RADIO_RESET_VALUE_DFEMODE       0x00000000UL
#define HAL_RADIO_RESET_VALUE_CTEINLINECONF 0x00002800UL

static inline void hal_radio_tx_power_high_voltage_clear(void);

static inline void hal_radio_reset(void)
{
}

static inline void hal_radio_stop(void)
{
	/* If +3dBm Tx power was used, then turn off high voltage when radio not
	 * used.
	 */
	hal_radio_tx_power_high_voltage_clear();
}

static inline void hal_radio_ram_prio_setup(void)
{
}

static inline uint32_t hal_radio_phy_mode_get(uint8_t phy, uint8_t flags)
{
	uint32_t mode;

	switch (phy) {
	case BIT(0):
	default:
		mode = RADIO_MODE_MODE_Ble_1Mbit;
		break;

	case BIT(1):
		mode = RADIO_MODE_MODE_Ble_2Mbit;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			mode = RADIO_MODE_MODE_Ble_LR125Kbit;
		} else {
			mode = RADIO_MODE_MODE_Ble_LR500Kbit;
		}
		break;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}

	return mode;
}

static inline uint32_t hal_radio_tx_power_max_get(void)
{
	return RADIO_TXPOWER_TXPOWER_0dBm;
}

static inline uint32_t hal_radio_tx_power_min_get(void)
{
	return RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

static inline uint32_t hal_radio_tx_power_floor(int8_t tx_power_lvl)
{
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_0dBm) {
		return RADIO_TXPOWER_TXPOWER_0dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg1dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg1dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg2dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg2dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg3dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg3dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg4dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg5dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg5dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg6dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg6dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg7dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg7dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg8dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg12dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg16dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg20dBm;
	}

	/* Note: The -30 dBm power level is deprecated so ignore it! */
	return RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

static inline void hal_radio_tx_power_high_voltage_set(int8_t tx_power_lvl)
{
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm) {
		nrf_vreqctrl_radio_high_voltage_set(NRF_VREQCTRL, true);
	}
}

static inline void hal_radio_tx_power_high_voltage_clear(void)
{
	nrf_vreqctrl_radio_high_voltage_set(NRF_VREQCTRL, false);
}

static inline uint32_t hal_radio_tx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US;
	}
}

static inline uint32_t hal_radio_rx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US;
	}
}

static inline uint32_t hal_radio_tx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_rx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_US;
	}
}

static inline uint32_t hal_radio_tx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS;
	}
}

static inline uint32_t hal_radio_rx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS;
	}
}

static inline uint32_t hal_radio_tx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_NS;
}

static inline uint32_t hal_radio_rx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_NS;

	}
}
