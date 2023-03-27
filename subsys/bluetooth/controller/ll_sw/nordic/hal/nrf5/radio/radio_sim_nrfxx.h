/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This header needs lots of types and macros, instead of relaying on
 * good inclusion order let's pull them through soc.h
 */
#include <soc.h>

/* NRF Radio HW timing constants
 * - provided in US and NS (for higher granularity)
 * - based on empirical measurements and sniffer logs
 */

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_FAST_NS 41000
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NS 141000
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS 130000
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_FAST_NS 40000
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NS 140000
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS 129000
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_FAST_NS 40000
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NS 140000
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS 129000
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_FAST_NS 40000
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NS 140000
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS 129000
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL(\
		HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS)

#define HAL_RADIO_NRF52833_TX_CHAIN_DELAY_NS     1000
#define HAL_RADIO_NRF52833_TX_CHAIN_DELAY_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_TX_CHAIN_DELAY_NS)

#define HAL_RADIO_NRF52833_RX_CHAIN_DELAY_1M_NS  9000
#define HAL_RADIO_NRF52833_RX_CHAIN_DELAY_1M_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_RX_CHAIN_DELAY_1M_NS)

#define HAL_RADIO_NRF52833_RX_CHAIN_DELAY_2M_NS  5000
#define HAL_RADIO_NRF52833_RX_CHAIN_DELAY_2M_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52833_RX_CHAIN_DELAY_2M_NS)

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_FAST_US
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_FAST_NS

#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_FAST_US
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_FAST_NS

#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_FAST_US
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_FAST_NS

#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_FAST_US
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_FAST_NS

#else /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
#if defined(CONFIG_BT_CTLR_TIFS_HW)
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_US
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NS

#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_US
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NS

#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_US
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NS

#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_US
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NS

#else /* !CONFIG_BT_CTLR_TIFS_HW */
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
#undef EVENT_TIMER
#define EVENT_TIMER NRF_TIMER0
#define SW_SWITCH_TIMER EVENT_TIMER
#define SW_SWITCH_TIMER_EVTS_COMP_BASE 0
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#define SW_SWITCH_TIMER NRF_TIMER1
#define SW_SWITCH_TIMER_EVTS_COMP_BASE 0
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

static inline void hal_radio_reset(void)
{
	/* TODO: Add any required setup for each radio event
	 */
}

static inline void hal_radio_stop(void)
{
	/* TODO: Add any required cleanup of actions taken in hal_radio_reset()
	 */
}

static inline void hal_radio_ram_prio_setup(void)
{

}

static inline uint32_t hal_radio_phy_mode_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);
	uint32_t mode;

	switch (phy) {
	case BIT(0):
	default:
		mode = RADIO_MODE_MODE_Ble_1Mbit;
		break;

	case BIT(1):
		mode = RADIO_MODE_MODE_Ble_2Mbit;
		break;
	}

	return mode;
}

static inline uint32_t hal_radio_tx_power_min_get(void)
{
	return RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

static inline uint32_t hal_radio_tx_power_max_get(void)
{
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
	return RADIO_TXPOWER_TXPOWER_Pos4dBm;
#else
	return RADIO_TXPOWER_TXPOWER_0dBm;
#endif
}

static inline uint32_t hal_radio_tx_power_floor(int8_t tx_power_lvl)
{
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos4dBm;
	}
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos3dBm;
	}
#endif
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_0dBm) {
		return RADIO_TXPOWER_TXPOWER_0dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg4dBm;
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

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg30dBm) {
		return RADIO_TXPOWER_TXPOWER_Neg30dBm;
	}

	return RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

static inline uint32_t hal_radio_tx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_US;
	}
}

static inline uint32_t hal_radio_rx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_US;
	}
}

static inline uint32_t hal_radio_tx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	return HAL_RADIO_NRF52833_TX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_rx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52833_RX_CHAIN_DELAY_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF52833_RX_CHAIN_DELAY_2M_US;
	}
}

static inline uint32_t hal_radio_tx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF52833_TXEN_TXIDLE_TX_2M_NS;
	}
}

static inline uint32_t hal_radio_rx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF52833_RXEN_RXIDLE_RX_2M_NS;
	}
}

static inline uint32_t hal_radio_tx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	return HAL_RADIO_NRF52833_TX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_rx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52833_RX_CHAIN_DELAY_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF52833_RX_CHAIN_DELAY_2M_NS;
	}
}
