/* ieee802154_nrf5.h - nRF5 802.15.4 driver */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_NRF5_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_NRF5_H_

#include "nrf_802154_config.h"

#define NRF5_FCS_LENGTH   (2)
#define NRF5_PSDU_LENGTH  (125)
#define NRF5_PHR_LENGTH   (1)

struct nrf5_802154_rx_frame {
	void *fifo_reserved; /* 1st word reserved for use by fifo. */
	u8_t *psdu; /* Pointer to a received frame. */
	u8_t lqi; /* Last received frame LQI value. */
	s8_t rssi; /* Last received frame RSSI value. */
};

struct nrf5_802154_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	u8_t mac[8];

	/* RX thread stack. */
	K_THREAD_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_NRF5_RX_STACK_SIZE);

	/* RX thread control block. */
	struct k_thread rx_thread;

	/* RX fifo queue. */
	struct k_fifo rx_fifo;

	/* Buffers for passing received frame pointers and data to the
	 * RX thread via rx_fifo object.
	 */
	struct nrf5_802154_rx_frame rx_frames[NRF_802154_RX_BUFFERS];

	/* CCA complete sempahore. Unlocked when CCA is complete. */
	struct k_sem cca_wait;

	/* CCA result. Holds information whether channel is free or not. */
	bool channel_free;

	/* TX synchronization semaphore. Unlocked when frame has been
	 * sent or send procedure failed.
	 */
	struct k_sem tx_wait;

	/* TX buffer. First byte is PHR (length), remaining bytes are
	 * MPDU data.
	 */
	u8_t tx_psdu[NRF5_PHR_LENGTH + NRF5_PSDU_LENGTH + NRF5_FCS_LENGTH];

	/* TX result, updated in radio transmit callbacks. */
	u8_t tx_result;

	/* A pointer to the received ACK frame. May be NULL if no ACK was
	 * requested/received.
	 */
	u8_t *ack;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_NRF5_H_ */
