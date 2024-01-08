/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
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

/* NRF Radio HW timing constants
 * - provided in US and NS (for higher granularity)
 * - based on empirical measurements and sniffer logs
 */

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_NS 40900 /*40.1 + 0.8*/
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NS 140900 /*140.1 + 0.8*/
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
 /* 129.5 + 0.8 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS 130300
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_NS 40000 /* 40.1 - 0.1 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NS 144900 /* 145 - 0.1 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
/* 129.5 - 0.1 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS 129400
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE CODED PHY [S2].
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_FAST_NS 42300 /* 40.1 + 2.2 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_FAST_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY [S2].
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NS 132200 /* 130 + 2.2 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY [S2].
 */
/* 129.5 + 2.2 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NO_HW_TIFS_NS 131700
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NO_HW_TIFS_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with fast Radio ramp-up mode)
 * in microseconds for LE CODED PHY [S8].
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_FAST_NS 42300 /* 40.1 + 2.2 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_FAST_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_FAST_NS)
/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY [S8].
 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NS 121800 /*119.6 + 2.2*/
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_US \
	HAL_RADIO_NS2US_ROUND(HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NS)

/* TXEN->TXIDLE + TXIDLE->TX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY [S8].
 */
 /* 129.5 + 2.2 */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NO_HW_TIFS_NS 131700
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_ROUND( \
		HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_NS 40300 /* 40.1 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 1M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NS 140300 /*140.1 + 0.2*/
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 1M PHY.
 */
/* 129.5 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS 129700
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_NS 40300 /* 40.1 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE 2M PHY.
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NS 144800 /*144.6 + 0.2*/
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE 2M PHY.
 */
/* 129.5 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS 129700
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE Coded PHY [S2].
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_FAST_NS 40300 /* 40.1 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE Coded PHY [S2].
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NS 130200 /* 130 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode
 * and no HW TIFS auto-switch) in microseconds for LE Coded PHY [S2].
 */
/* 129.5 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NO_HW_TIFS_NS 129700
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NO_HW_TIFS_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with fast Radio ramp-up mode)
 * in microseconds for LE Coded PHY [S8].
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_FAST_NS 40300 /* 40.1 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_FAST_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_FAST_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode)
 * in microseconds for LE Coded PHY [S8].
 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NS 120200 /* 120.0 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_US \
	HAL_RADIO_NS2US_CEIL(HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NS)

/* RXEN->RXIDLE + RXIDLE->RX (with default Radio ramp-up mode and
 * no HW TIFS auto-switch) in microseconds for LE Coded PHY [S8].
 */
/* 129.5 + 0.2 */
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NO_HW_TIFS_NS 129700
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NO_HW_TIFS_US \
	HAL_RADIO_NS2US_CEIL( \
		HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NO_HW_TIFS_NS)

#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_1M_US  1 /* ceil(0.6) */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_1M_NS  600 /* 0.6 */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_2M_US  1 /* ceil(0.6) */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_2M_NS  600 /* 0.6 */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S2_US  1 /* ceil(0.6) */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S2_NS  600 /* 0.6 */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S8_US  1 /* ceil(0.6) */
#define HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S8_NS  600 /* 0.6 */

#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_US  10 /* ceil(9.4) */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_NS  9400 /* 9.4 */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_US  5 /* ceil(5.0) */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_NS  5000 /* 5.0 */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S2_US  25 /* ceil(19.6) */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S2_NS  24600 /* 19.6 */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S8_US  30 /* ceil(29.6) */
#define HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S8_NS  29600 /* 29.6 */

#if defined(CONFIG_BT_CTLR_RADIO_ENABLE_FAST)
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_FAST_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_FAST_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_FAST_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_FAST_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_FAST_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_FAST_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_FAST_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_FAST_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_FAST_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_FAST_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_FAST_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_FAST_NS

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

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NS

#else /* !CONFIG_BT_CTLR_TIFS_HW */
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_US \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_NS \
	HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_DEFAULT_NO_HW_TIFS_NS

#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_US \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NO_HW_TIFS_US
#define HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_NS \
	HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_DEFAULT_NO_HW_TIFS_NS
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
	/* TODO: Add any required setup for each radio event
	 */
}

static inline void hal_radio_stop(void)
{
	/* If +3dBm Tx power was used, then turn off high voltage when radio not
	 * used.
	 */
	hal_radio_tx_power_high_voltage_clear();

	/* TODO: Add any required cleanup of actions taken in hal_radio_reset()
	 */
}

static inline void hal_radio_ram_prio_setup(void)
{
	/* TODO */
}

static inline uint32_t hal_radio_phy_mode_get(uint8_t phy, uint8_t flags)
{
	uint32_t mode;

	switch (phy) {
	case BIT(0):
	default:
		mode = RADIO_MODE_MODE_Ble_1Mbit;

		/* Workaround: nRF5340 Revision 1 Errata 117 */
		*((volatile uint32_t *)0x41008588) =
			*((volatile uint32_t *)0x01FF0080); /* non-2M mode */
		break;

	case BIT(1):
		mode = RADIO_MODE_MODE_Ble_2Mbit;

		/* Workaround: nRF5340 Revision 1 Errata 117 */
		*((volatile uint32_t *)0x41008588) =
			*((volatile uint32_t *)0x01FF0084); /* 2M mode */
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			mode = RADIO_MODE_MODE_Ble_LR125Kbit;
		} else {
			mode = RADIO_MODE_MODE_Ble_LR500Kbit;
		}

		/* Workaround: nRF5340 Revision 1 Errata 117 */
		*((volatile uint32_t *)0x41008588) =
			*((volatile uint32_t *)0x01FF0080); /* non-2M mode */
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
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_US;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_US;
		} else {
			return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_US;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_rx_ready_delay_us_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_US;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_US;
		} else {
			return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_US;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_tx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_2M_US;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S8_US;
		} else {
			return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S2_US;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_rx_chain_delay_us_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_US;
	case BIT(1):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_US;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S8_US;
		} else {
			return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S2_US;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_tx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_2M_NS;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S8_NS;
		} else {
			return HAL_RADIO_NRF5340_TXEN_TXIDLE_TX_S2_NS;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_rx_ready_delay_ns_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_2M_NS;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S8_NS;
		} else {
			return HAL_RADIO_NRF5340_RXEN_RXIDLE_RX_S2_NS;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_tx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_2M_NS;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S8_NS;
		} else {
			return HAL_RADIO_NRF5340_TX_CHAIN_DELAY_S2_NS;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}

static inline uint32_t hal_radio_rx_chain_delay_ns_get(uint8_t phy, uint8_t flags)
{
	switch (phy) {
	default:
	case BIT(0):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_1M_NS;
	case BIT(1):
		return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_2M_NS;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case BIT(2):
		if (flags & 0x01) {
			return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S8_NS;
		} else {
			return HAL_RADIO_NRF5340_RX_CHAIN_DELAY_S2_NS;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	}
}
