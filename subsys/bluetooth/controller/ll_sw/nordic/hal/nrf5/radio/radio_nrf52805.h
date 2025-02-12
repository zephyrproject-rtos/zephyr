/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Use the NRF_RTC instance for coarse radio event scheduling */
#define NRF_RTC NRF_RTC0

/* HAL abstraction of event timer prescaler value */
#define HAL_EVENT_TIMER_PRESCALER_VALUE 4U

/* NRF Radio HW timing constants
 * - provided in US and NS (for higher granularity)
 * - based on empirical measurements and sniffer logs
 */

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_FAST_NS 41050 /* 40.1 + 0.95 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NS 141050 /*140.1 + 0.95*/
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
/* 129 + 0.95 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS 129950
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_FAST_NS 40000 /* 40.1 - 0.1 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NS 144500 /* 144.6 - 0.1 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
/* 129 - 0.1 */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS 128900
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_FAST_NS 40300 /* 40.1 + 0.2 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NS 140300 /*140.1 + 0.2*/
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
/* 129 + 0.2 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS 129200
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_FAST_NS 40300 /* 40.1 + 0.2 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NS 144800 /* 144.6 + 0.2 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
/* 129 + 0.2 */
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS 129200
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL(\
		HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS)

#define HAL_RADIO_NRF52805_TX_CHAIN_DELAY_NS     600 /* 0.6 */
#define HAL_RADIO_NRF52805_TX_CHAIN_DELAY_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_TX_CHAIN_DELAY_NS)

#define HAL_RADIO_NRF52805_RX_CHAIN_DELAY_1M_NS  9400 /* 9.4 */
#define HAL_RADIO_NRF52805_RX_CHAIN_DELAY_1M_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_RX_CHAIN_DELAY_1M_NS)

#define HAL_RADIO_NRF52805_RX_CHAIN_DELAY_2M_NS  5450 /* 5.0 + 0.45 */
#define HAL_RADIO_NRF52805_RX_CHAIN_DELAY_2M_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF52805_RX_CHAIN_DELAY_2M_NS)

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_FAST_US
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_FAST_NS

#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_FAST_US
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_FAST_NS

#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_FAST_US
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_FAST_NS

#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_FAST_US
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_FAST_NS

#else /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */
#if defined(CONFIG_BT_CTLR_TIFS_HW)
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_US
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NS

#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_US
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NS

#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_US
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NS

#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_US
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NS

#else /* !CONFIG_BT_CTLR_TIFS_HW */
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
#endif /* !CONFIG_BT_CTLR_RADIO_ENABLE_FAST */

/* HAL abstraction of Radio bitfields */
#define HAL_RADIO_INTENSET_DISABLED_Msk         RADIO_INTENSET_DISABLED_Msk
#define HAL_RADIO_SHORTS_TRX_END_DISABLE_Msk    RADIO_SHORTS_END_DISABLE_Msk
#define HAL_RADIO_SHORTS_TRX_PHYEND_DISABLE_Msk RADIO_SHORTS_PHYEND_DISABLE_Msk

/* HAL abstraction of Radio IRQ number */
#define HAL_RADIO_IRQn                          RADIO_IRQn

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
	struct {
		uint32_t volatile reserved_0[0x5a0 >> 2];
		uint32_t volatile bridge_type;
		uint32_t volatile reserved_1[((0xe00 - 0x5a0) >> 2) - 1];
		struct {
			uint32_t volatile CPU0;
			uint32_t volatile SPIS1;
			uint32_t volatile RADIO;
			uint32_t volatile ECB;
			uint32_t volatile CCM;
			uint32_t volatile AAR;
			uint32_t volatile SAADC;
			uint32_t volatile UARTE;
			uint32_t volatile SERIAL0;
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
	return RADIO_TXPOWER_TXPOWER_Pos4dBm;
}

static inline uint32_t hal_radio_tx_power_floor(int8_t tx_power_lvl)
{
	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos4dBm;
	}

	if (tx_power_lvl >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm) {
		return RADIO_TXPOWER_TXPOWER_Pos3dBm;
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
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_US;
	}
}

static inline uint32_t hal_radio_rx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_US;
	}
}

static inline uint32_t hal_radio_tx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	return HAL_RADIO_NRF52805_TX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_rx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52805_RX_CHAIN_DELAY_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF52805_RX_CHAIN_DELAY_2M_US;
	}
}

static inline uint32_t hal_radio_tx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF52805_TXEN_TXIDLE_TX_2M_NS;
	}
}

static inline uint32_t hal_radio_rx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF52805_RXEN_RXIDLE_RX_2M_NS;
	}
}

static inline uint32_t hal_radio_tx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	return HAL_RADIO_NRF52805_TX_CHAIN_DELAY_US;
}

static inline uint32_t hal_radio_rx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	ARG_UNUSED(flags);

	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF52805_RX_CHAIN_DELAY_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF52805_RX_CHAIN_DELAY_2M_NS;
	}
}
