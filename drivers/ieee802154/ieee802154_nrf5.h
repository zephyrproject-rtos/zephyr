/* ieee802154_nrf5.h - nRF5 802.15.4 driver */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __IEEE802154_NRF5_H__
#define __IEEE802154_NRF5_H__

#include <linker/sections.h>
#include <atomic.h>

#define NRF5_FCS_LENGTH   (2)
#define NRF5_PSDU_LENGTH  (125)
#define NRF5_PHR_LENGTH   (1)

struct nrf5_802154_data {
	/* Pointer to the network interface. */
	struct net_if *iface;
	/* Pointer to a received frame. */
	u8_t *rx_psdu;
	/* TX buffer. First byte is PHR (length), remaining bytes are
	 * MPDU data.
	 */
	u8_t tx_psdu[NRF5_PHR_LENGTH + NRF5_PSDU_LENGTH];
	/* 802.15.4 HW address. */
	u8_t mac[8];
	/* RX thread stack. */
	K_THREAD_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_NRF5_RX_STACK_SIZE);
	/* RX thread control block */
	struct k_thread rx_thread;

	/* CCA complete sempahore. Unlocked when CCA is complete. */
	struct k_sem cca_wait;
	/* RX synchronization semaphore. Unlocked when frame has been
	 * received.
	 */
	struct k_sem rx_wait;
	/* TX synchronization semaphore. Unlocked when frame has been
	 * sent or CCA failed.
	 */
	struct k_sem tx_wait;
	/* TX result. Set to 1 on success, 0 otherwise. */
	bool tx_success;

	/* CCA channel energy. Unit as per 802.15.4-2006 specification. */
	s8_t channel_ed;

	/* TX power, in dBm, to be used when sending a frame. */
	s8_t txpower;
	/* 802.15.4 channel to be used when sending a frame. */
	u8_t channel;

	/* Last received frame LQI value. */
	u8_t lqi;
	/* Last received frame RSSI value. */
	s8_t rssi;
};

#endif /* __IEEE802154_NRF5_H__ */
