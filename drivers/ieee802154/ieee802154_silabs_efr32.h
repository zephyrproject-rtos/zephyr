/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ieee802154_silabs_efr32.h - Silabs EFR32 802.15.4 driver */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_SILABS_EFR32_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_SILABS_EFR32_H_

#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/ieee802154_frame.h>

#include <zephyr/sys/atomic.h>

#include <sl_rail.h>
#include <sl_rail_ieee802154.h>
#include <sl_rail_util_compatible_pa.h>

/* PHY layer constants */
#define SL_802154_PHR_BYTES 1
#define SL_802154_SHR_BYTES 5 /* 4 bytes of preamble, 1 byte sync-word */

#define SL_802154_EUI64_BYTES 8

#define SL_802154_CCM_NONCE_BYTES (SL_802154_EUI64_BYTES + sizeof(uint32_t) + sizeof(uint8_t))

/* Link-layer security: keys and frame counter (SiSDK radio_security / RAIL security).
 * Stored so that otPlatRadioSetMacKey / SetMacFrameCounter succeed. The stack
 * builds secured frames and passes them to tx(); we do not encrypt in the driver
 * unless RAIL security APIs are used. Key array is terminated by key_value == NULL.
 */
enum sl_802154_mac_key_type {
	SL_802154_MAC_KEY_PREVIOUS,
	SL_802154_MAC_KEY_CURRENT,
	SL_802154_MAC_KEY_NEXT,
	SL_802154_MAC_KEY_COUNT
};

#define SYMBOLS_PER_ENERGY_READING 8
#define QUARTER_DBM_IN_DBM         4

struct sl_802154_energy_scan_data {

	/* Callback handler of the currently ongoing energy scan.
	 * It shall be NULL if energy scan is not in progress.
	 */
	energy_scan_done_cb_t energy_scan_done;

	/* Current energy scan frame counter */
	uint32_t frame_counter;

	/* Max energy scan frame counter value */
	uint32_t frame_counter_max;

	/* Max RSSI reading */
	int8_t max_reading;

	/* Energy scan state */
	bool in_progress;
};

struct sl_802154_sec_key_buf {
	uint8_t key_value[IEEE802154_KEY_MAX_LEN];
	uint8_t key_id;
};

struct sl_802154_mac_data {

	struct ieee802154_key sec_keys[SL_802154_MAC_KEY_COUNT];

	uint32_t ack_fc;

	uint32_t frame_counter;

	struct sl_802154_sec_key_buf sec_key_storage[SL_802154_MAC_KEY_COUNT];

	uint8_t ack_key_id;

	bool em_pending_data;

	bool ack_fpb;

	bool ack_seb;
};

struct sl_802154_ack_ie_data {

	uint16_t short_addr;

	uint8_t ext_addr[IEEE802154_EXT_ADDR_LENGTH];

	/* Packed (alignment 1) - placed last so any alignment padding
	 * for the enclosing struct ends up after this field, not before.
	 */
	struct ieee802154_header_ie link_metrics_header_ie;
};

/* RAIL buffers: use SDK builtin for RX; provide our own TX FIFO (no builtin).
 * If sl_rail_init fails with INVALID_PARAMETER, init with minimal config (buffers
 * 0/NULL) and set RX/TX via sl_rail_set_* after init.
 */
#define SL_802154_RAIL_TX_FIFO_BYTES ((IEEE802154_MAX_PHY_PACKET_SIZE) + (SL_802154_PHR_BYTES))
#define SL_802154_RAIL_TX_FIFO_ALIGN_COUNT \
	DIV_ROUND_UP(SL_802154_RAIL_TX_FIFO_BYTES, sizeof(sl_rail_fifo_buffer_align_t))

struct sl_802154_radio_data {
	sl_rail_fifo_buffer_align_t rail_tx_fifo[SL_802154_RAIL_TX_FIFO_ALIGN_COUNT];

	sl_rail_handle_t rail_handle;

	const sl_rail_ieee802154_config_t rail_ieee802154_config;

	/* Timer for energy scan operations */
	sl_rail_multi_timer_t rail_timer;

	atomic_t state;

	/* The TX power in deci-dBm. */
	sl_rail_tx_power_t txpwr;

	uint8_t csma_ca_backoffs;

	bool rail_initialized;
};

/* Short and extended addresses used for software frame-pending (FPB).
 *
 * This structure may be accessed concurrently with IRQ context: it is
 * written from the driver's (thread-level) configure path and read
 * from the RAIL callback path when handling incoming frames and ACK
 * frame-pending.
 */
struct sl_802154_source_match_data {
	uint16_t short_addrs[CONFIG_IEEE802154_SILABS_EFR32_SRC_MATCH_SHORT_MAX];

	uint8_t ext_addrs[CONFIG_IEEE802154_SILABS_EFR32_SRC_MATCH_EXT_MAX]
			 [IEEE802154_EXT_ADDR_LENGTH];

	uint8_t short_addr_count;

	uint8_t ext_addr_count;
};

struct sl_802154_config {
	void (*irq_config_func)(const struct device *dev);
	/* HFXO worst-case accuracy in ± ppm. Always counted toward
	 * get_sch_acc because the radio's awake-time timing reference
	 * (RAIL/protimer) is derived from HFXO.
	 */
	uint16_t hfxo_accuracy_ppm;
	/* Sleep-clock (LFXO and similar) worst-case accuracy in ± ppm.
	 * Added to the HFXO value in get_sch_acc only when the device
	 * may sleep (CONFIG_OPENTHREAD_MTD_SED), to account for the
	 * drift that accumulates while sleeping off the low-frequency
	 * oscillator.
	 */
	uint16_t sleep_clock_accuracy_ppm;
};

struct sl_802154_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* TX synchronization semaphore. Unlocked when frame has been
	 * sent or send procedure failed.
	 */
	struct k_sem tx_wait;

	/* ACK synchronization semaphore. Unlocked when ACK has been
	 * received or reception procedure failed.
	 */
	struct k_sem ack_wait;

	/* Callback handler to notify of any important radio events.
	 * Can be NULL if event notification is not needed.
	 */
	ieee802154_event_cb_t event_handler;

	/* Parsed MHR structures */
	struct ieee802154_mhr tx_mhr;
	struct ieee802154_mhr rx_mhr;
	struct ieee802154_mhr enh_ack_mhr;

	struct sl_802154_radio_data radio_data;

	struct sl_802154_mac_data mac_data;

	struct sl_802154_energy_scan_data ed_scan_data;

	struct sl_802154_ack_ie_data ack_ie;

	struct sl_802154_source_match_data match_data;

	/* Capabilities of the network interface. */
	enum ieee802154_hw_caps capabilities;

	/* TX completion: errno to return from tx() (0, -ENOMSG, -EBUSY, -EIO).
	 * Set in events callback.
	 */
	int tx_errno;

	/* Current channel (11-26 for 2.4 GHz O-QPSK). Set by set_channel(). */
	uint16_t current_channel;

	/* Filter state (maps to otPlatRadioSet* in PAL). Applied by filter(). */
	uint16_t pan_id;
	uint16_t short_addr;

	uint8_t ext_addr[IEEE802154_EXT_ADDR_LENGTH];

	/* TX buffer: MPDU for security processing. */
	uint8_t tx_buffer[IEEE802154_MAX_PHY_PACKET_SIZE];

	/* RX buffer. */
	uint8_t rx_buffer[IEEE802154_MAX_PHY_PACKET_SIZE];

	/* Enhanced ACK buffer. */
	uint8_t enh_ack_buffer[IEEE802154_MAX_PHY_PACKET_SIZE];

	/* Key IDs for slots 0=prev, 1=curr, 2=next (set from IEEE802154_CONFIG_MAC_KEYS). */
	uint8_t key_id_prev;
	uint8_t key_id_current;
	uint8_t key_id_next;

	bool is_src_match_enabled;

	/* Promiscuous mode requested by OpenThread. */
	bool promiscuous;

	bool pan_coordinator;
};

/* Provided by the Simplicity SDK OpenThread blob, or by blob_stubs.c when
 * CONFIG_BUILD_ONLY_NO_BLOBS is set.
 */
void sl_openthread_init(void);

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_SILABS_EFR32_H_ */
