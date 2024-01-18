/* ieee802154_nrf5.h - nRF5 802.15.4 driver */

/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_NRF5_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_NRF5_H_

#include <zephyr/net/ieee802154_radio.h>

#define NRF5_PHR_LENGTH   (1)

struct nrf5_802154_rx_frame {
	void *fifo_reserved; /* 1st word reserved for use by fifo. */
	uint8_t *psdu; /* Pointer to a received frame. */
	uint64_t time; /* RX timestamp. */
	uint8_t lqi; /* Last received frame LQI value. */
	int8_t rssi; /* Last received frame RSSI value. */
	bool ack_fpb; /* FPB value in ACK sent for the received frame. */
	bool ack_seb; /* SEB value in ACK sent for the received frame. */
};

struct nrf5_802154_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

	/* RX thread stack. */
	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_NRF5_RX_STACK_SIZE);

	/* RX thread control block. */
	struct k_thread rx_thread;

	/* RX fifo queue. */
	struct k_fifo rx_fifo;

	/* Buffers for passing received frame pointers and data to the
	 * RX thread via rx_fifo object.
	 */
	struct nrf5_802154_rx_frame rx_frames[CONFIG_NRF_802154_RX_BUFFERS];

	/* Frame pending bit value in ACK sent for the last received frame. */
	bool last_frame_ack_fpb;

	/* Security Enabled bit value in ACK sent for the last received frame. */
	bool last_frame_ack_seb;

	/* CCA complete semaphore. Unlocked when CCA is complete. */
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
	uint8_t tx_psdu[NRF5_PHR_LENGTH + IEEE802154_MAX_PHY_PACKET_SIZE];

	/* TX result, updated in radio transmit callbacks. */
	uint8_t tx_result;

	/* A buffer for the received ACK frame. psdu pointer be NULL if no
	 * ACK was requested/received.
	 */
	struct nrf5_802154_rx_frame ack_frame;

	/* Callback handler of the currently ongoing energy scan.
	 * It shall be NULL if energy scan is not in progress.
	 */
	energy_scan_done_cb_t energy_scan_done;

	/* Callback handler to notify of any important radio events.
	 * Can be NULL if event notification is not needed.
	 */
	ieee802154_event_cb_t event_handler;

	/* Capabilities of the network interface. */
	enum ieee802154_hw_caps capabilities;

	/* Indicates if currently processed TX frame is secured. */
	bool tx_frame_is_secured;

	/* Indicates if currently processed TX frame has dynamic data updated. */
	bool tx_frame_mac_hdr_rdy;

#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	/* The maximum number of extra CCA attempts to be performed before transmission. */
	uint8_t max_extra_cca_attempts;
#endif

	/* The TX power in dBm. */
	int8_t txpwr;

	/* The radio channel. */
	uint8_t channel;

#if defined(CONFIG_NRF_802154_SER_HOST) && defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	/* The last configured value of CSL period in units of 10 symbols. */
	uint32_t csl_period;

	/* The last configured value of CSL phase time in nanoseconds. */
	net_time_t csl_rx_time;
#endif /* CONFIG_NRF_802154_SER_HOST && CONFIG_IEEE802154_CSL_ENDPOINT */

	/* Indicates if RxOnWhenIdle mode is enabled. */
	bool rx_on_when_idle;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_NRF5_H_ */
