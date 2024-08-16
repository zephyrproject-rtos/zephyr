/*
 * Copyright (c) 2023-2024 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atm34_ieee802154

#define LOG_MODULE_NAME ieee802154_atm34
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/ieee802154_radio.h>
#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#endif
#include <zephyr/random/random.h>

#include "arch.h"
#include "ble_driver.h"
#include "eui.h"
#include "radio_hal_154.h"
#include "radio_req_154.h"
#include "radio_hal_frc.h"

#define RX_GIVE_IRQn BLE_ISOTS_0_IRQn
#define TX_GIVE_IRQn BLE_ISOTS_1_IRQn

/* Bitfield to determine which operations are currently active. If the LOCKED
 * bit is set, an operation is ongoing that does not allow other operations,
 * and radio actions must return EIO. Other than the locked bit, the state
 * operates as a normal enum. */
enum {
	IEEE802154_ATM34_IDLE = 0,
	IEEE802154_ATM34_RX_RUNNING,
	IEEE802154_ATM34_CCW_RUNNING,
	IEEE802154_ATM34_RADIO_LOCKED = (1 << 7),
} ieee802154_atm34_radio_state;

#define PENDING_ENTRIES 4

struct long_pend_entry {
	uint64_t addr;
	bool status;
};

struct short_pend_entry {
	uint16_t addr;
	bool status;
};

struct ieee802154_atm34_data {
	uint8_t mac[8];
	struct net_if *iface;

	ieee802154_event_cb_t event_handler;

	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_ATM34_RX_STACK_SIZE);
	uint8_t rx_buffer[MAX_154_PACKET_LEN];
	struct k_thread rx_thread;

	/*
	 * Overall radio state of driver per external start/stop interface.
	 */
	atomic_t state; // Holds an ieee802154_atm34_radio_state (enum with lock bit)
	uint16_t set_channel;

	/*
	 * State and management of rx_thread
	 */
	bool volatile rx_enable;	// Should rx_thread keep receiving packets?
	uint32_t rx_start_time;
	uint32_t rx_duration;
	uint16_t rx_channel;
	struct k_sem rx_done;		// Wait until rx_thread paused
	struct k_sem rx_pause;		// Used to pause rx_thread

	/*
	 * Use a single priority for all operations
	 */
	atm_mac_mgr_priority_t priority;

	/*
	 * Coordinate rx and completion callback
	 */
	struct k_sem rx_wait;
	atm_mac_status_t volatile rx_status;
	atm_mac_154_rx_complete_info_t volatile rx_info;

	/*
	 * Coordinate tx and completion callback
	 */
	struct k_sem tx_wait;
	atm_mac_status_t volatile tx_status;
	atm_mac_154_tx_complete_info_t volatile tx_info;

	/*
	 * Coordinate ed scan and completion callback
	 */
	energy_scan_done_cb_t energy_scan_done;

	/*
	 * Lookup tables for data pending.  FIXME: linear search for now.
	 */
	struct long_pend_entry long_pending[PENDING_ENTRIES];
	struct short_pend_entry short_pending[PENDING_ENTRIES];
};

static struct ieee802154_atm34_data data;

static void ieee802154_atm34_radio_iface_init(struct net_if *iface)
{
	read_eui64(data.mac);

	net_if_set_link_addr(iface, data.mac, sizeof(data.mac), NET_LINK_IEEE802154);

	data.iface = iface;

	ieee802154_init(iface);
}

static enum ieee802154_hw_caps ieee802154_atm34_radio_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	// We could support TXTIME with minimal changes, but there is no good way to test this
	// unless we are a CSL transmitter. We do not have radio driver support for TX_SEC.
	return IEEE802154_HW_ENERGY_SCAN |
#ifdef CONFIG_IEEE802154_ATM34_AUTO_CRC
	       IEEE802154_HW_FCS |
#endif
	       IEEE802154_HW_FILTER | IEEE802154_HW_PROMISC | IEEE802154_HW_CSMA |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_RETRANSMISSION | IEEE802154_HW_RX_TX_ACK |
	       IEEE802154_HW_SLEEP_TO_TX | IEEE802154_HW_RXTIME;
}

static int ieee802154_atm34_radio_cca(const struct device *dev)
{
	ARG_UNUSED(dev);

	__ASSERT(false, "CCA not implemented as a standalone operation");

	return 0;
}

static int ieee802154_atm34_radio_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);

	if (channel < ATM_MAC_154_MIN_CHANNEL) {
		return -ENOTSUP;
	}
	if (channel > ATM_MAC_154_MAX_CHANNEL) {
		return -EINVAL;
	}

	data.set_channel = channel;
	atm_req_154_set_channel(atm_154_iface, channel);
	return 0;
}

static int ieee802154_atm34_radio_filter(const struct device *dev,
					 bool set,
					 enum ieee802154_filter_type type,
					 const struct ieee802154_filter *filter)
{
	ARG_UNUSED(dev);

	if (!set) {
		return -ENOTSUP;
	}

	// FIXME: check endianess of filter->*_addr
	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		atm_req_154_set_long_addr(atm_154_iface, *(uint64_t *)filter->ieee_addr);
		return 0;
	}
	if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		atm_req_154_set_short_addr(atm_154_iface, filter->short_addr);
		return 0;
	}
	if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		atm_req_154_set_pan_id(atm_154_iface, filter->pan_id);
		return 0;
	}

	return -ENOTSUP;
}

static int ieee802154_atm34_radio_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);

	atm_req_154_set_tx_power(atm_154_iface, dbm);
	return 0;
}

static net_time_t ieee802154_atm34_radio_get_time(const struct device *dev)
{
	ARG_UNUSED(dev);

	atm_mac_lock_sync();
	net_time_t current_time = ((net_time_t)atm_mac_frc_get_current_time()) * NSEC_PER_USEC;
	atm_mac_unlock();
	return current_time;
}

static uint8_t ieee802154_atm34_radio_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_ATM34_DELAY_TRX_ACC;
}

static bool ieee802154_atm34_cb_rx_long_addr_pend(uint64_t address)
{
	for (int i = 0; i < PENDING_ENTRIES; i++) {
		if (!data.long_pending[i].status) {
			continue;
		}
		if (data.long_pending[i].addr == address) {
			return true;
		}
	}

	return false;
}

static bool ieee802154_atm34_cb_rx_short_addr_pend(uint16_t address)
{
	for (int i = 0; i < PENDING_ENTRIES; i++) {
		if (!data.short_pending[i].status) {
			continue;
		}
		if (data.short_pending[i].addr == address) {
			return true;
		}
	}

	return false;
}

ISR_DIRECT_DECLARE(ieee802154_atm34_rx_give)
{
	k_sem_give(&data.rx_wait);
	return 1;
}

/*
 * @note: Always called from zero-latency ATLC_IRQn, so kernel calls are not permitted.
 */
static void ieee802154_atm34_cb_rx_complete(atm_mac_status_t status,
					    atm_mac_154_rx_complete_info_t const *info)
{
	data.rx_status = status;
	data.rx_info = *info;
	NVIC_SetPendingIRQ(RX_GIVE_IRQn);
}

static void ieee802154_atm34_rx_good_packet(void)
{
	uint8_t pkt_len = ATM_MAC_154_FRAME_LEN_READ(data.rx_buffer[ATM_MAC_154_LENGTH_OFFSET]);
	LOG_DBG("Caught packet (Len:%u LQI:%u RSSI:%d)", pkt_len, data.rx_info.lqi,
		data.rx_info.rssi);
	LOG_HEXDUMP_DBG(data.rx_buffer + ATM_MAC_154_PHR_LEN, pkt_len, "rx");
#ifdef CONFIG_IEEE802154_ATM34_AUTO_CRC
	if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) || IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
		pkt_len += ATM_MAC_154_FCS_LEN;
	}
#endif

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(data.iface, pkt_len,
							   AF_UNSPEC, 0, K_FOREVER);
	if (!pkt) {
		LOG_ERR("No free packet available.");
		return;
	}

	if (net_pkt_write(pkt, data.rx_buffer + ATM_MAC_154_PHR_LEN, pkt_len)) {
		LOG_ERR("Packet dropped by NET write");
		net_pkt_unref(pkt);
		return;
	}

	net_pkt_set_ieee802154_lqi(pkt, data.rx_info.lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, data.rx_info.rssi);
	net_pkt_set_ieee802154_ack_fpb(pkt, data.rx_info.fp_set);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	net_pkt_set_timestamp_ns(pkt, ((net_time_t)data.rx_info.timestamp) * NSEC_PER_USEC);
#endif

	if (net_recv_data(data.iface, pkt) < 0) {
		LOG_ERR("Packet dropped by NET stack");
		net_pkt_unref(pkt);
		return;
	}
}

static void ieee802154_atm34_rx_packet(const struct device *dev)
{
	k_sem_reset(&data.rx_wait);
	data.rx_status = ATM_MAC_154_RX_COMPLETE_STATUS_STOPPED;

	if (data.rx_channel && (data.rx_channel != data.set_channel)) {
		atm_req_154_set_channel(atm_154_iface, data.rx_channel);
	}

	// FIXME: ATLC won't sleep while waiting for rx_start_time because atm_mac lock is held
	atm_req_154_receive_packet(atm_154_iface, data.rx_buffer, data.rx_start_time,
				   data.rx_duration, data.priority);

	// Wait for rx_complete or rx_stop
	k_sem_take(&data.rx_wait, K_FOREVER);

	if (data.rx_channel && (data.rx_channel != data.set_channel)) {
		atm_req_154_set_channel(atm_154_iface, data.set_channel);
	}

	enum ieee802154_rx_fail_reason reason;
	switch (data.rx_status) {
	case ATM_MAC_154_RX_COMPLETE_STATUS_SUCCESS:
		ieee802154_atm34_rx_good_packet();
		return;
	case ATM_MAC_154_RX_COMPLETE_STATUS_FAIL_TIMEOUT:
	case ATM_MAC_154_RX_COMPLETE_STATUS_FAIL_PAST:
		reason = IEEE802154_RX_FAIL_NOT_RECEIVED;
		break;
	case ATM_MAC_154_RX_COMPLETE_STATUS_STOPPED:
		return;
	case ATM_MAC_154_RX_COMPLETE_STATUS_FAIL:
	default:
		reason = IEEE802154_RX_FAIL_OTHER;
		break;
	}

	if (data.event_handler) {
		data.event_handler(dev, IEEE802154_EVENT_RX_FAILED, &reason);
	}
}

static void ieee802154_atm34_rx_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	for (;;) {
		while (!data.rx_enable) {
			LOG_DBG("rx pause");
			k_sem_give(&data.rx_done);
			k_sem_take(&data.rx_pause, K_FOREVER);
		}
		LOG_DBG("rx start");
		ieee802154_atm34_rx_packet(dev);
	}
}

static void ieee802154_atm34_rx_enable(void)
{
	if (data.rx_enable) {
		return;
	}

	data.rx_enable = true;
	k_sem_give(&data.rx_pause);
}

static int ieee802154_atm34_radio_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	atomic_val_t radio_state = atomic_get(&data.state);
	if (radio_state & IEEE802154_ATM34_RADIO_LOCKED) {
		LOG_ERR("start: locked state %ld", radio_state);
		return -EIO;
		// If the radio is unlocked, radio_state acts as a normal enum
	} else if (radio_state == IEEE802154_ATM34_IDLE) {
		// No error - will start RX below
	} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
		return -EALREADY;
	} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		// End CCW below before starting RX
	} else {
		LOG_ERR("start: unknown state %ld", radio_state);
		return -EIO;
	}

	atomic_val_t rx_radio_state = IEEE802154_ATM34_RADIO_LOCKED | IEEE802154_ATM34_RX_RUNNING;
	if (!atomic_cas(&data.state, radio_state, rx_radio_state)) {
		LOG_ERR("start: state changed from %ld to %ld", radio_state, data.state);
		return -EIO;
	}

	if (radio_state == IEEE802154_ATM34_IDLE) {
		atm_mac_lock_sync();
	} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		atm_req_154_activate_carrier_wave(atm_154_iface, false);
	}

	data.rx_start_time = 0;
	data.rx_duration = 0;
	data.rx_channel = 0;
	ieee802154_atm34_rx_enable();

	if (!atomic_cas(&data.state, rx_radio_state, IEEE802154_ATM34_RX_RUNNING)) {
		__ASSERT(false, "start: locked state changed from %ld to %ld", rx_radio_state,
			 data.state);
	}
	LOG_INF("Receive started (channel:%d)", atm_req_154_get_channel(atm_154_iface));
	return 0;
}

static void ieee802154_atm34_rx_stop(void)
{
	if (data.rx_enable) {
		k_sem_reset(&data.rx_done);
		data.rx_enable = false;

		// Abort any operation in flight
		atm_req_154_stop(atm_154_iface);
		k_sem_give(&data.rx_wait);

		// Wait for rx_thread to pause
		k_sem_take(&data.rx_done, K_FOREVER);
	}

	// Fully clear state
	atm_req_154_stop(atm_154_iface);
}

static int ieee802154_atm34_radio_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	atomic_val_t radio_state = atomic_get(&data.state);

	if (radio_state & IEEE802154_ATM34_RADIO_LOCKED) {
		LOG_ERR("stop: locked state %ld", radio_state);
		return -EIO;
		// If the radio is unlocked, radio_state acts as a normal enum
	} else if (radio_state == IEEE802154_ATM34_IDLE) {
		return -EALREADY;
	} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
		// No error - will stop RX below
	} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		// No error - will stop CCW below
	} else {
		LOG_ERR("stop: unknown state %ld", radio_state);
		return -EIO;
	}

	atomic_val_t stop_radio_state = IEEE802154_ATM34_RADIO_LOCKED | IEEE802154_ATM34_IDLE;
	if (!atomic_cas(&data.state, radio_state, stop_radio_state)) {
		LOG_ERR("stop: state changed from %ld to %ld", radio_state, data.state);
		return -EIO;
	}

	if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		atm_req_154_activate_carrier_wave(atm_154_iface, false);
	}

	ieee802154_atm34_rx_stop();

	if (!atomic_cas(&data.state, stop_radio_state, IEEE802154_ATM34_IDLE)) {
		__ASSERT(false, "stop: locked state changed from %ld to %ld", stop_radio_state,
			 data.state);
	}
	atm_mac_unlock();
	if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		LOG_INF("ccw stopped");
	} else {
		LOG_INF("stopped");
	}
	return 0;
}

static int ieee802154_atm34_radio_continuous_carrier(const struct device *dev)
{
	ARG_UNUSED(dev);

	atomic_val_t radio_state = atomic_get(&data.state);

	if (radio_state & IEEE802154_ATM34_RADIO_LOCKED) {
		LOG_ERR("stop: locked state %ld", radio_state);
		return -EIO;
		// If the radio is unlocked, radio_state acts as a normal enum
	} else if (radio_state == IEEE802154_ATM34_IDLE) {
		// No error - will start CCW below
	} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
		// No error - will stop RX and start CCW below
	} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		return -EALREADY;
	} else {
		LOG_ERR("ccw: unknown state %ld", radio_state);
		return -EIO;
	}

	atomic_val_t ccw_radio_state = IEEE802154_ATM34_RADIO_LOCKED | IEEE802154_ATM34_CCW_RUNNING;
	if (!atomic_cas(&data.state, radio_state, ccw_radio_state)) {
		LOG_ERR("ccw: state changed from %ld to %ld", radio_state, data.state);
		return -EIO;
	}

	if (radio_state == IEEE802154_ATM34_IDLE) {
		atm_mac_lock_sync();
	} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
		ieee802154_atm34_rx_stop();
	}

	atm_req_154_activate_carrier_wave(atm_154_iface, true);
	uint8_t channel = atm_req_154_get_channel(atm_154_iface);

	if (!atomic_cas(&data.state, ccw_radio_state, IEEE802154_ATM34_CCW_RUNNING)) {
		__ASSERT(false, "stop: locked state changed from %ld to %ld", ccw_radio_state,
			 data.state);
	}
	LOG_INF("Continuous carrier wave transmission started (channel:%d)", channel);
	return 0;
}

static int ieee802154_atm34_handle_ack(void)
{
	uint8_t ack_len = ATM_MAC_154_FRAME_LEN_READ(data.tx_info.ack_buffer[0]);
	LOG_DBG("Caught ack (Len:%u LQI:%u RSSI:%d)", ack_len, data.tx_info.ack_lqi,
		data.tx_info.ack_rssi);
	LOG_HEXDUMP_DBG(data.tx_info.ack_buffer + ATM_MAC_154_PHR_LEN, ack_len, "ack");

	struct net_pkt *ack_pkt = net_pkt_rx_alloc_with_buffer(data.iface, ack_len, AF_UNSPEC, 0,
							       K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		return -ENOMEM;
	}

	/*
	 * Upper layers expect the frame to start at the MAC header, skip the
	 * PHY header (1 byte).
	 */
	if (net_pkt_write(ack_pkt, data.tx_info.ack_buffer + ATM_MAC_154_PHR_LEN, ack_len) < 0) {
		LOG_ERR("Failed to write to a packet.");
		net_pkt_unref(ack_pkt);
		return -ENOMEM;
	}

	net_pkt_set_ieee802154_lqi(ack_pkt, data.tx_info.ack_lqi);
	net_pkt_set_ieee802154_rssi_dbm(ack_pkt, data.tx_info.ack_rssi);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	net_pkt_set_timestamp_ns(ack_pkt, ((net_time_t)data.tx_info.ack_timestamp) * NSEC_PER_USEC);
#endif

	net_pkt_cursor_init(ack_pkt);

	if (ieee802154_handle_ack(data.iface, ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled - releasing.");
	}

	net_pkt_unref(ack_pkt);
	return 0;
}

ISR_DIRECT_DECLARE(ieee802154_atm34_tx_give)
{
	k_sem_give(&data.tx_wait);
	return 1;
}

/*
 * @note: Always called from zero-latency ATLC_IRQn, so kernel calls are not permitted.
 */
static void ieee802154_atm34_cb_tx_complete(atm_mac_status_t status,
					    atm_mac_154_tx_complete_info_t const *info)
{
	data.tx_status = status;
	data.tx_info = *info;
	NVIC_SetPendingIRQ(TX_GIVE_IRQn);
}

BUILD_ASSERT((int)IEEE802154_TX_MODE_DIRECT ==
	(int)ATM_MAC_154_TX_CSMA_MODE_DISABLED, "tx_mode/csma_mode mismatch");
BUILD_ASSERT((int)IEEE802154_TX_MODE_CCA ==
	(int)ATM_MAC_154_TX_CSMA_MODE_CCA_ONLY, "tx_mode/csma_mode mismatch");
BUILD_ASSERT((int)IEEE802154_TX_MODE_CSMA_CA ==
	(int)ATM_MAC_154_TX_CSMA_MODE_ENABLED, "tx_mode/csma_mode mismatch");

static int ieee802154_atm34_radio_tx(const struct device *dev, enum ieee802154_tx_mode tx_mode,
				     struct net_pkt *pkt, struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	uint8_t payload_len = frag->len;
	uint8_t *payload = frag->data;

	if (payload_len > IEEE802154_MTU) {
		LOG_ERR("Payload too large: %d", payload_len);
		return -EMSGSIZE;
	}

#ifndef CONFIG_IEEE802154_ATM34_AUTO_CRC
	if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) || IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
		payload_len += ATM_MAC_154_FCS_LEN;
	}
#endif
	LOG_DBG("%p (%u)", (void *)payload, payload_len);
	LOG_HEXDUMP_DBG(payload, payload_len, "tx");

	atomic_val_t radio_state;
	atomic_val_t tx_radio_state;
	switch (tx_mode) {
	case IEEE802154_TX_MODE_DIRECT:
	case IEEE802154_TX_MODE_CCA:
	case IEEE802154_TX_MODE_CSMA_CA:
		radio_state = atomic_get(&data.state);
		if (radio_state & IEEE802154_ATM34_RADIO_LOCKED) {
			LOG_ERR("tx: locked state %ld", radio_state);
			return -EIO;
			// If the radio is unlocked, radio_state acts as a normal enum
		} else if (radio_state == IEEE802154_ATM34_IDLE) {
			// No error - will start RX below
		} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
			// End RX below before starting TX
		} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
			LOG_ERR("tx: unavailable durinng ccw %ld", radio_state);
			return -EIO;
		} else {
			LOG_ERR("tx: unknown state %ld", radio_state);
			return -EIO;
		}
		tx_radio_state = radio_state | IEEE802154_ATM34_RADIO_LOCKED;
		if (!atomic_cas(&data.state, radio_state, tx_radio_state)) {
			LOG_ERR("tx: state changed from %ld to %ld", radio_state, data.state);
			return -EIO;
		}

		if (radio_state == IEEE802154_ATM34_IDLE) {
			atm_mac_lock_sync();
		} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
			ieee802154_atm34_rx_stop();
		}
		k_sem_reset(&data.tx_wait);
		atm_req_154_transmit_packet_with_len(atm_154_iface, payload_len, payload,
						     (atm_mac_154_tx_csma_mode_t) tx_mode, false, 0,
						     data.priority);
		break;
	default:
		NET_ERR("TX mode %d not supported", tx_mode);
		return -ENOTSUP;
	}

	if (data.event_handler) {
		data.event_handler(dev, IEEE802154_EVENT_TX_STARTED, frag);
	}

	k_sem_take(&data.tx_wait, K_FOREVER);

	// Always enable RX at the end of TX
	ieee802154_atm34_rx_enable();

	if (!atomic_cas(&data.state, tx_radio_state, IEEE802154_ATM34_RX_RUNNING)) {
		__ASSERT(false, "tx: locked state changed from %ld to %ld", tx_radio_state,
			 data.state);
	}

	switch (data.tx_status) {
	case ATM_MAC_154_TX_COMPLETE_STATUS_SUCCESS:
		if (!data.tx_info.ack_received) {
			return 0;
		}
		return ieee802154_atm34_handle_ack();
	case ATM_MAC_154_TX_COMPLETE_STATUS_FAIL_CCA:
		return -EBUSY;
	case ATM_MAC_154_TX_COMPLETE_STATUS_FAIL_NO_ACK:
		return -ENOMSG;
	case ATM_MAC_154_TX_COMPLETE_STATUS_FAIL:
	default:
		return -EIO;
	}
}

static void ieee802154_atm34_cb_ed_complete(atm_mac_status_t status, int8_t rssi)
{
	LOG_DBG("Rssi: %d", rssi);

	atomic_val_t radio_state = atomic_get(&data.state);

	if (!(radio_state & IEEE802154_ATM34_RADIO_LOCKED)) {
		// Radio must be locked during ED
		LOG_ERR("ED complete: unknown state %ld", radio_state);
	}
	atomic_val_t post_ed_radio_state = radio_state & ~IEEE802154_ATM34_RADIO_LOCKED;
	if (post_ed_radio_state == IEEE802154_ATM34_RX_RUNNING) {
		ieee802154_atm34_rx_enable();
	}
	if (!atomic_cas(&data.state, radio_state, post_ed_radio_state)) {
		__ASSERT(false, "ed: locked state changed from %ld to %ld", radio_state,
			 data.state);
	}
	if (post_ed_radio_state == IEEE802154_ATM34_IDLE) {
		atm_mac_unlock();
	}

	if (!data.energy_scan_done) {
		return;
	}

	energy_scan_done_cb_t callback = data.energy_scan_done;
	data.energy_scan_done = NULL;
	callback(net_if_get_device(data.iface), rssi);
}

static int ieee802154_atm34_radio_ed_scan(const struct device *dev, uint16_t duration,
					  energy_scan_done_cb_t done_cb)
{
	ARG_UNUSED(dev);

	if (data.energy_scan_done) {
		return -EALREADY;
	}
	atomic_val_t radio_state = atomic_get(&data.state);
	if (radio_state & IEEE802154_ATM34_RADIO_LOCKED) {
		LOG_ERR("ed: locked state %ld", radio_state);
		return -EIO;
		// If the radio is unlocked, radio_state acts as a normal enum
	} else if (radio_state == IEEE802154_ATM34_IDLE) {
		// No error - will start RX below
	} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
		// End RX below before starting TX
	} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
		LOG_ERR("ed: unavailable durinng ccw %ld", radio_state);
		return -EIO;
	} else {
		LOG_ERR("ed: unknown state %ld", radio_state);
		return -EIO;
	}

	// All ED scans lock the radio
	atomic_val_t ed_radio_state = radio_state | IEEE802154_ATM34_RADIO_LOCKED;
	if (!atomic_cas(&data.state, radio_state, ed_radio_state)) {
		LOG_ERR("ed: state changed from %ld to %ld", radio_state, data.state);
		return -EIO;
	}

	if (radio_state == IEEE802154_ATM34_IDLE) {
		atm_mac_lock_sync();
	} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
		ieee802154_atm34_rx_stop();
	}

	data.energy_scan_done = done_cb;
	atm_req_154_energy_detect(atm_154_iface, duration * USEC_PER_MSEC, data.priority);
	LOG_DBG("Energy detect started (channel:%d)", atm_req_154_get_channel(atm_154_iface));
	return 0;
}

static int long_pending_set(uint64_t addr)
{
	struct long_pend_entry *first_empty = NULL;

	for (int i = 0; i < PENDING_ENTRIES; i++) {
		if (data.long_pending[i].status) {
			if (data.long_pending[i].addr == addr) {
				return 0;
			}
			continue;
		}

		if (first_empty) {
			continue;
		}
		first_empty = &data.long_pending[i];
	}

	if (!first_empty) {
		return -ENOMEM;
	}
	first_empty->addr = addr;
	first_empty->status = true;
	return 0;
}

static int long_pending_clear(uint64_t addr)
{
	for (int i = 0; i < PENDING_ENTRIES; i++) {
		if (!data.long_pending[i].status) {
			continue;
		}

		if (data.long_pending[i].addr == addr) {
			data.long_pending[i].status = false;
			return 0;
		}
	}
	return -ENOENT;
}

static int long_pending_clear_all(void)
{
	for (int i = 0; i < PENDING_ENTRIES; i++) {
		data.long_pending[i].status = false;
	}
	return 0;
}

static int short_pending_set(uint16_t addr)
{
	struct short_pend_entry *first_empty = NULL;

	for (int i = 0; i < PENDING_ENTRIES; i++) {
		if (data.short_pending[i].status) {
			if (data.short_pending[i].addr == addr) {
				return 0;
			}
			continue;
		}

		if (first_empty) {
			continue;
		}
		first_empty = &data.short_pending[i];
	}

	if (!first_empty) {
		return -ENOMEM;
	}
	first_empty->addr = addr;
	first_empty->status = true;
	return 0;
}

static int short_pending_clear(uint16_t addr)
{
	for (int i = 0; i < PENDING_ENTRIES; i++) {
		if (!data.short_pending[i].status) {
			continue;
		}

		if (data.short_pending[i].addr == addr) {
			data.short_pending[i].status = false;
			return 0;
		}
	}
	return -ENOENT;
}

static int short_pending_clear_all(void)
{
	for (int i = 0; i < PENDING_ENTRIES; i++) {
		data.short_pending[i].status = false;
	}
	return 0;
}

static int ieee802154_atm34_set_ack_fpb(bool extended, uint8_t *addr)
{
	if (extended) {
		LOG_INF("Set ACK_FPB %016" PRIx64, *(uint64_t *)addr);
		return long_pending_set(*(uint64_t *)addr);
	}

	LOG_INF("Set ACK_FPB %#04" PRIx16, *(uint16_t *)addr);
	return short_pending_set(*(uint16_t *)addr);
}

static int ieee802154_atm34_clear_ack_fpb(bool extended, uint8_t *addr)
{
	if (addr) {
		if (extended) {
			LOG_INF("Clear ACK_FPB %016" PRIx64, *(uint64_t *)addr);
			return long_pending_clear(*(uint64_t *)addr);
		}

		LOG_INF("Clear ACK_FPB %#04" PRIx16, *(uint16_t *)addr);
		return short_pending_clear(*(uint16_t *)addr);
	}

	LOG_INF("Clear ACK_FPB%s", extended ? " extended" : "");
	if (extended) {
		return long_pending_clear_all();
	}

	return short_pending_clear_all();
}

static int ieee802154_atm34_radio_configure(const struct device *dev,
					    enum ieee802154_config_type type,
					    const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);

	switch (type) {
	case IEEE802154_CONFIG_PROMISCUOUS:
		atm_req_154_disable_address_filtering(atm_154_iface, config->promiscuous);
		return 0;
	case IEEE802154_CONFIG_EVENT_HANDLER:
		data.event_handler = config->event_handler;
		return 0;
	case IEEE802154_CONFIG_PAN_COORDINATOR:
		atm_mac_154_device_mode_t mode =
			config->pan_coordinator ? ATM_MAC_154_MODE_PAN_COORDINATOR :
#if defined(CONFIG_OPENTHREAD_FTD)
						ATM_MAC_154_MODE_COORDINATOR;
#else
						ATM_MAC_154_MODE_DEVICE;
		if (config->pan_coordinator) {
			LOG_ERR("config: Cannot set MTD as PAN coordinator");
			return -EIO;
		}
#endif
		atm_req_154_set_device_mode(atm_154_iface, mode);
		return 0;
	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.enabled) {
			return ieee802154_atm34_set_ack_fpb(config->ack_fpb.extended,
							    config->ack_fpb.addr);
		}
		return ieee802154_atm34_clear_ack_fpb(config->ack_fpb.extended,
						      config->ack_fpb.addr);
#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	case IEEE802154_CONFIG_RX_SLOT:
		atomic_val_t radio_state = atomic_get(&data.state);

		if (radio_state & IEEE802154_ATM34_RADIO_LOCKED) {
			LOG_ERR("config: locked state %ld", radio_state);
			return -EIO;
			// If the radio is unlocked, radio_state acts as a normal enum
		} else if (radio_state == IEEE802154_ATM34_IDLE) {
			// No error - will start RX below
		} else if (radio_state == IEEE802154_ATM34_RX_RUNNING) {
			// End RX below before starting TX
		} else if (radio_state == IEEE802154_ATM34_CCW_RUNNING) {
			LOG_ERR("config: unavailable durinng ccw %ld", radio_state);
			return -EIO;
		} else {
			LOG_ERR("config: unknown state %ld", radio_state);
			return -EIO;
		}

		if ((config->rx_slot.start == -1) || !config->rx_slot.duration) {
			return 0;
		}

		atomic_val_t config_state = radio_state | IEEE802154_ATM34_RADIO_LOCKED;
		if (!atomic_cas(&data.state, radio_state, config_state)) {
			LOG_ERR("config: state changed from %ld to %ld", radio_state, data.state);
			return -EIO;
		}

		if (radio_state == IEEE802154_ATM34_IDLE) {
			atm_mac_lock_sync();
		}

		data.rx_start_time = config->rx_slot.start / NSEC_PER_USEC;
		data.rx_duration = config->rx_slot.duration / NSEC_PER_USEC;
		data.rx_channel = config->rx_slot.channel;
		ieee802154_atm34_rx_enable();

		if (!atomic_cas(&data.state, config_state, IEEE802154_ATM34_RX_RUNNING)) {
			__ASSERT(false, "config: locked state changed from %ld to %ld",
				 config_state, data.state);
		}
		LOG_INF("Receive @%" PRIu32 " for %" PRIu32 " (channel:%d)", data.rx_start_time,
			data.rx_duration, data.rx_channel);

		return 0;
	case IEEE802154_CONFIG_CSL_PERIOD:
		atm_req_154_set_csl_ie_period(atm_154_iface, config->csl_period);
		return 0;
	case IEEE802154_CONFIG_CSL_RX_TIME:
		// Used in conjunction with CSL period to calculate the CSL phase
		atm_req_154_set_csl_ie_rx_time(atm_154_iface, config->csl_rx_time / NSEC_PER_USEC);
		return 0;
#endif
	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		// The long address must be little endian when passed down
		uint64_t long_addr;
		sys_memcpy_swap(&long_addr, config->ack_ie.ext_addr, sizeof(long_addr));

		atm_req_154_enable_enhanced_ack(atm_154_iface, config->ack_ie.short_addr, long_addr,
						config->ack_ie.data, config->ack_ie.data_len);
		return 0;

	default:
		break;
	}

	LOG_INF("configure %d", type);

	/* configure not supported */
	return -ENOTSUP;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, ATM_MAC_154_MIN_CHANNEL,
					 ATM_MAC_154_MAX_CHANNEL);

static int ieee802154_atm34_radio_attr_get(const struct device *dev, enum ieee802154_attr attr,
					   struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

#ifndef CONFIG_OPENTHREAD_THREAD_PRIORITY
#define RX_THREAD_PRIO K_PRIO_COOP(2)
#elif defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define RX_THREAD_PRIO K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY + 1)
#else
#define RX_THREAD_PRIO K_PRIO_PREEMPT(CONFIG_OPENTHREAD_THREAD_PRIORITY + 1)
#endif

static int ieee802154_atm34_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	data.state = ATOMIC_INIT(IEEE802154_ATM34_IDLE);
	k_sem_init(&data.rx_done, 0, 1);
	k_sem_init(&data.rx_pause, 0, 1);
	k_sem_init(&data.rx_wait, 0, 1);
	k_sem_init(&data.tx_wait, 0, 1);

	IRQ_DIRECT_CONNECT(RX_GIVE_IRQn, IRQ_PRI_UI, ieee802154_atm34_rx_give, 0);
	irq_enable(RX_GIVE_IRQn);
	IRQ_DIRECT_CONNECT(TX_GIVE_IRQn, IRQ_PRI_UI, ieee802154_atm34_tx_give, 0);
	irq_enable(TX_GIVE_IRQn);

	atm_mac_lock_sync();

	atm_req_154_init(atm_154_iface);
#ifndef CONFIG_IEEE802154_ATM34_AUTO_CRC
	atm_req_154_disable_auto_crc(atm_154_iface, true);
#endif
	atm_req_154_set_device_mode(atm_154_iface, COND_CODE_1(CONFIG_OPENTHREAD_FTD,
		(ATM_MAC_154_MODE_COORDINATOR), (ATM_MAC_154_MODE_DEVICE)));
	atm_req_154_set_version(atm_154_iface, ATM_MAC_154_VERSION_THREAD_1_3_TL1);
#ifdef CONFIG_NET_L2_OPENTHREAD
	atm_req_154_set_min_csma_backoff_exponent(atm_154_iface, 0);
#endif
	atm_req_154_register_rx_long_addr_callback(atm_154_iface,
						   ieee802154_atm34_cb_rx_long_addr_pend);
	atm_req_154_register_rx_short_addr_callback(atm_154_iface,
						    ieee802154_atm34_cb_rx_short_addr_pend);
	atm_req_154_register_rx_complete_callback(atm_154_iface, ieee802154_atm34_cb_rx_complete);
	atm_req_154_register_tx_complete_callback(atm_154_iface, ieee802154_atm34_cb_tx_complete);
	atm_req_154_register_ed_complete_callback(atm_154_iface, ieee802154_atm34_cb_ed_complete);

	k_thread_create(&data.rx_thread, data.rx_stack,
			CONFIG_IEEE802154_ATM34_RX_STACK_SIZE,
			ieee802154_atm34_rx_thread, (void *)dev, NULL, NULL,
			RX_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(&data.rx_thread, "ieee802154_atm34");

	atm_mac_unlock();
	return 0;
}

static struct ieee802154_radio_api ieee802154_atm34_radio_api = {
	.iface_api.init = ieee802154_atm34_radio_iface_init,
	.get_capabilities = ieee802154_atm34_radio_get_capabilities,
	.cca = ieee802154_atm34_radio_cca,
	.set_channel = ieee802154_atm34_radio_set_channel,
	.filter = ieee802154_atm34_radio_filter,
	.set_txpower = ieee802154_atm34_radio_set_txpower,
	.start = ieee802154_atm34_radio_start,
	.stop = ieee802154_atm34_radio_stop,
	.continuous_carrier = ieee802154_atm34_radio_continuous_carrier,
	.tx = ieee802154_atm34_radio_tx,
	.ed_scan = ieee802154_atm34_radio_ed_scan,
	.get_time = ieee802154_atm34_radio_get_time,
	.get_sch_acc = ieee802154_atm34_radio_get_acc,
	.configure = ieee802154_atm34_radio_configure,
	.attr_get = ieee802154_atm34_radio_attr_get,
};

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU 125
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#endif

#if defined(CONFIG_NET_L2_IEEE802154) || defined(CONFIG_NET_L2_OPENTHREAD)
NET_DEVICE_DT_INST_DEFINE(0, ieee802154_atm34_init, NULL, &data, NULL,
			  80,
			  &ieee802154_atm34_radio_api, L2, L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, ieee802154_atm34_init, NULL, &data, NULL,
		      POST_KERNEL, 80,
		      &ieee802154_atm34_radio_api);
#endif
