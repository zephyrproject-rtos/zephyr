/*
 * Copyright (c) 2024 A Labs GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_ESP32_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_ESP32_H_

#include <esp_ieee802154_types.h>

#include <zephyr/net/ieee802154_radio.h>

struct ieee802154_esp32_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

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
	uint8_t tx_psdu[1 + IEEE802154_MAX_PHY_PACKET_SIZE];

	/*
	 * ACK frame (stored until esp_ieee802154_receive_handle_done is called)
	 * First byte is frame length (PHR), followed by payload (PSDU)
	 */
	const uint8_t *ack_frame;

	/* A buffer for the received ACK frame. psdu pointer be NULL if no
	 * ACK was requested/received.
	 */
	esp_ieee802154_frame_info_t *ack_frame_info;

	/* Callback handler of the currently ongoing energy scan.
	 * It shall be NULL if energy scan is not in progress.
	 */
	energy_scan_done_cb_t energy_scan_done;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_ESP32_H_ */
