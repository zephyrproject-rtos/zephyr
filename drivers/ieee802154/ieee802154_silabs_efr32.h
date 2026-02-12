/* ieee802154_silabs_efr32.h - Silabs EFR32 802.15.4 driver */

/*
 * Copyright (c) 2026 Silicon Laboratories
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_SILABS_EFR32_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_SILABS_EFR32_H_

#include <zephyr/net/ieee802154_radio.h>

#include <sl_rail.h>
#include <sl_rail_ieee802154.h>
#include <sl_rail_util_compatible_pa.h>

#if CONFIG_OPENTHREAD_MULTIPLE_INSTANCE
#define RADIO_INTERFACE_COUNT CONFIG_OPENTHREAD_MULTIPLE_INSTANCE_NUM
#else
#define RADIO_INTERFACE_COUNT 1
#endif // CONFIG_OPENTHREAD_MULTIPLE_INSTANCE

// PHY layer constants
#define PHY_HEADER_SIZE 1
#define SHR_SIZE 5 // 4 bytes of preamble, 1 byte sync-word

#define IEEE802154_DSN_OFFSET         (2)

#define SILABS_EFR32_FCF_FRAME_PENDING  BIT(4)
#define SILABS_EFR32_FCF_ACK_REQUIRED  BIT(5)
#define SILABS_EFR32_FCF_FRAME_PENDING_SET_IN_OUTGOING_ACK  BIT(7)

/* Link-layer security: keys and frame counter (SiSDK radio_security / RAIL security).
 * Stored so that otPlatRadioSetMacKey / SetMacFrameCounter succeed. The stack
 * builds secured frames and passes them to tx(); we do not encrypt in the driver
 * unless RAIL security APIs are used. Key array is terminated by key_value == NULL.
 */
#define SILABS_SEC_KEY_SLOTS  3

// <o SL_802154_RADIO_PRIO_BACKGROUND_RX_VALUE> Background RX priority
// <1-255:1>
// <i> Default: 255
// <i> The 802.15.4 background RX priority. Background RX is the general, passive receive state when the device is not transmitting anything
#define SL_802154_RADIO_PRIO_BACKGROUND_RX_VALUE 255

struct silabs_efr32_802154_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

	/* Current channel (11–26 for 2.4 GHz O-QPSK). Set by set_channel(). */
	uint16_t current_channel;

	/* Filter state (maps to otPlatRadioSet* in PAL). Applied by filter(). */
	uint16_t pan_id;
	uint16_t short_addr;
	uint8_t ext_addr[8];

	/* CCA complete semaphore. Unlocked when CCA is complete. */
	struct k_sem cca_wait;

	/* CCA result. Holds information whether channel is free or not. */
	bool channel_free;

	/* TX synchronization semaphore. Unlocked when frame has been
	 * sent or send procedure failed.
	 */
	struct k_sem tx_wait;

	/* ACK synchronization semaphore. Unlocked when ACK has been
	 * received or reception procedure failed.
	 */
	 struct k_sem ack_wait;

	/* TX buffer. First byte is PHR (length), remaining bytes are
	 * MPDU data.
	 */
	uint8_t tx_psdu[1 + IEEE802154_MAX_PHY_PACKET_SIZE];

	/* TX result, updated in radio transmit callbacks. */
	uint8_t tx_result;

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

	bool is_src_match_enabled;

	/* Promiscuous mode requested by OpenThread. */
	bool promiscuous;

	/* CSL sample time and period. */
	uint32_t csl_sample_time;
	uint32_t csl_period;

	/* Frame counter. */
	uint32_t frame_counter;

	/* The TX power in dBm. */
	sl_rail_tx_power_t txpwr;

	/* RX buffer. */
	uint8_t rx_psdu[IEEE802154_MAX_PHY_PACKET_SIZE];

	bool pan_coordinator;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_SILABS_EFR32_H_ */
