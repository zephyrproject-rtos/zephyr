/*
 * Copyright (c) 2024 A Labs GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_ESP32_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_ESP32_H_

#include <esp_ieee802154_types.h>

#include <zephyr/net/ieee802154_radio.h>

struct esp32_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

	/* CCA complete semaphore. Unlocked when CCA is complete. */
	struct k_sem cca_wait;

	/* CCA result. Holds information whether channel is free or not. */
	bool channel_free;

	bool is_started;

	/* TX synchronization semaphore. Unlocked when frame has been
	 * sent or send procedure failed.
	 */
	struct k_sem tx_wait;

	/*
	 * ACK frame (stored until esp_ieee802154_receive_handle_done is called)
	 * First byte is frame length (PHR), followed by payload (PSDU)
	 */
	const uint8_t *ack_frame;

	/* A buffer for the received ACK frame. psdu pointer be NULL if no
	 * ACK was requested/received.
	 */
	esp_ieee802154_frame_info_t *ack_frame_info;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_ESP32_H_ */
