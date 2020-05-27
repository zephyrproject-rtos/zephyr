/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_BT_CTLR_TIFS_HW)
#define SW_SWITCH_TIMER NRF_TIMER1
#define SW_SWITCH_TIMER_EVTS_COMP_BASE 0
#define SW_SWITCH_TIMER_TASK_GROUP_BASE 0
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

/* TXEN->TXIDLE + TXIDLE->TX in microseconds. */
#define HAL_RADIO_NRF51_TXEN_TXIDLE_TX_US 140
#define HAL_RADIO_NRF51_TXEN_TXIDLE_TX_NS 140000
/* RXEN->RXIDLE + RXIDLE->RX in microseconds. */
#define HAL_RADIO_NRF51_RXEN_RXIDLE_RX_US 138
#define HAL_RADIO_NRF51_RXEN_RXIDLE_RX_NS 138000

#define HAL_RADIO_NRF51_TX_CHAIN_DELAY_US 1 /* ceil(1.0) */
#define HAL_RADIO_NRF51_TX_CHAIN_DELAY_NS 1000 /* 1.0 */
#define HAL_RADIO_NRF51_RX_CHAIN_DELAY_US 3 /* ceil(3.0) */
#define HAL_RADIO_NRF51_RX_CHAIN_DELAY_NS 3000 /* 3.0 */

static inline void hal_radio_reset(void)
{
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
		mode = RADIO_MODE_MODE_Nrf_2Mbit;
		break;
	}

	return mode;
}

static inline uint32_t hal_radio_tx_power_min_get(void)
{
	return RADIO_TXPOWER_TXPOWER_Neg30dBm;
}

static inline uint32_t hal_radio_tx_power_max_get(void)
{
	return RADIO_TXPOWER_TXPOWER_Pos4dBm;
}

static inline uint32_t hal_radio_tx_power_floor(int8_t tx_power_lvl)
{
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos4dBm;
	}

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

	/* Note: The -30 dBm power level is deprecated so ignore it! */
	return RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

static inline uint32_t hal_radio_tx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_TXEN_TXIDLE_TX_US;
}

static inline uint32_t hal_radio_rx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_RXEN_RXIDLE_RX_US;
}

static inline uint32_t hal_radio_tx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_TX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_rx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_RX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_tx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_TXEN_TXIDLE_TX_NS;
}

static inline uint32_t hal_radio_rx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_RXEN_RXIDLE_RX_NS;
}

static inline uint32_t hal_radio_tx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_TX_CHAIN_DELAY_NS;
}

static inline uint32_t hal_radio_rx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);
	return HAL_RADIO_NRF51_RX_CHAIN_DELAY_NS;
}
