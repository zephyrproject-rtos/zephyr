/* ieee802154_silabs_efr32.c - Silabs EFR32 802.15.4 driver */

/*
 * Copyright (c) 2026 Silicon Laboratories
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_efr32_ieee802154

#define LOG_MODULE_NAME ieee802154_silabs_efr32
#if defined(CONFIG_IEEE802154_SILABS_EFR32_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_SILABS_EFR32_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio_openthread.h>
#include <openthread/platform/radio.h>
#endif

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#include "ieee802154_silabs_efr32.h"

#include <em_system.h>

/* RAIL handle; init'd once in silabs_efr32_rail_init(). */
static sl_rail_handle_t s_rail_handle;
static bool s_rail_initialized;

/* TX completion: errno to return from tx() (0, -ENOMSG, -EBUSY, -EIO). Set in events callback. */
static int s_tx_errno;

static bool s_em_pending_data = false;

// TODO convert to Kconfig
#define OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER 0

/*
 * Override RAIL assert handler so we log and halt instead of spinning forever.
 * The prebuilt RAIL library calls sl_railcb_assert_failed(handle, errorCode) on
 * failed assertions; the default implementation loops indefinitely.
 */
static const char *silabs_rail_assert_code_str(sl_rail_assert_error_codes_t code)
{
	switch (code) {
	case 0:  return "APPENDED_INFO_MISSING";
	case 1:  return "RX_FIFO_BYTES";
	case 4:  return "BAD_PACKET_LENGTH";
	case 6:  return "UNEXPECTED_STATE_RX_FIFO";
	case 7:  return "UNEXPECTED_STATE_RXLEN_FIFO";
	case 8:  return "UNEXPECTED_STATE_TX_FIFO";
	case 9:  return "UNEXPECTED_STATE_TXACK_FIFO";
	case 29: return "NULL_HANDLE";
	case 31: return "NO_ACTIVE_CONFIG";
	case 45: return "NULL_PARAMETER";
	case 62: return "FAILED_TX_CRC_CONFIG";
	case 63: return "INVALID_PA_OPERATION";
	case 64: return "SEQ_INVALID_PA_SELECTED";
	case 70: return "FAILED_MULTITIMER_CORRUPT";
	case 74: return "SECURE_ACCESS_FAULT";
	default: return NULL;
	}
}

void sl_railcb_assert_failed(sl_rail_handle_t rail_handle,
	sl_rail_assert_error_codes_t error_code,
	int line)
{
	ARG_UNUSED(rail_handle);
	const char *code_str = silabs_rail_assert_code_str(error_code);

	if (code_str != NULL) {
		LOG_ERR("RAIL assert failed: %s (%u)", code_str, (unsigned int)error_code);
	} else {
		LOG_ERR("RAIL assert failed: error_code=%u (see RAIL_AssertErrorCodes_t)", (unsigned int)error_code);
	}
	log_flush();
	k_fatal_halt(K_ERR_CPU_EXCEPTION);
}

/* Event notification work: RAIL callback runs in ISR so we defer to thread. */
#define SILABS_EV_PENDING_TX_STARTED  (1U << 0)
#define SILABS_EV_PENDING_RX_FAILED   (1U << 1)
#define SILABS_EV_PENDING_RX_OFF      (1U << 2)
static atomic_t s_event_pending;
static enum ieee802154_rx_fail_reason s_rx_fail_reason;
static void silabs_efr32_event_work_handler(struct k_work *work);
static K_WORK_DEFINE(silabs_efr32_event_work, silabs_efr32_event_work_handler);
static const struct device *silabs_efr32_get_device(void);

/* Soft source match table (Thread: set Frame Pending in ACK for these addresses).
 * Add/clear via IEEE802154_CONFIG_ACK_FPB (otPlatRadioAddSrcMatch* / ClearSrcMatch*).
 * When SL_RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND fires, we look up the source
 * address and call sl_rail_ieee802154_toggle_frame_pending() so the immediate ACK
 * has the FP bit set (SiSDK radio.cpp handle_data_request_command).
 */
#define SILABS_SRC_MATCH_SHORT_MAX 16
#define SILABS_SRC_MATCH_EXT_MAX   16
static uint16_t s_src_match_short[SILABS_SRC_MATCH_SHORT_MAX];
static uint8_t  s_src_match_ext[SILABS_SRC_MATCH_EXT_MAX][8];
static uint8_t  s_src_match_short_count;
static uint8_t  s_src_match_ext_count;

static struct ieee802154_key s_sec_keys[SILABS_SEC_KEY_SLOTS];

/* Driver data; defined here so the events callback can reference it. */
static struct silabs_efr32_802154_data silabs_efr32_data;

static bool src_match_short_contains(uint16_t short_addr);
static bool src_match_ext_contains(const uint8_t *addr);

// only 2.4 GHz OQPSK is supported
#define SL_CHANNEL_MAX OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX
#define SL_CHANNEL_MIN OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN
#define SL_MAX_CHANNELS_SUPPORTED ((OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN) + 1)

void sl_openthread_init(void);

/*************************************************************************************************************************/
/// Radio Energy Scan
// Timer for energy scan operations
static sl_rail_multi_timer_t s_rail_timer;

/**
 * Initialize energy scan module.
 * No initialization needed for per-scan objects.
 */
 void sli_ot_energy_scan_init(void)
 {
	 // Initialize the timer to zero
	 memset(&s_rail_timer, 0, sizeof(s_rail_timer));
 
	 // Clean up any ongoing energy scans
	//  clearAllScans();
 }
 

// Check if a scan is in progress
/*
bool isScanInProgress(EnergyScan *scan)
{
    return (scan != NULL && scan->isInProgress());
}
*/

bool sli_ot_energy_scan_is_in_progress(void)
{
	return false;
}
/*
{
    // Check if any instance has a scan in progress
    for (instanceIndex_t i = 0; i < RADIO_INTERFACE_COUNT; i++)
    {
        if (isScanInProgress(sEnergyScans[i]))
        {
            return true;
        }
    }
    return false;
}
*/

/*************************************************************************************************************************/
/// Radio State Machine

#define RADIO_SCHEDULER_CHANNEL_SLIP_TIME (500000UL)
#define RADIO_PRIORITY_TX_MIN 100
#define RADIO_TIMING_CSMA_OVERHEAD_US 500
#define RADIO_TIMING_DEFAULT_BYTETIME_US 32   // only used if sl_rail_get_bit_rate returns 0
#define RADIO_TIMING_DEFAULT_SYMBOLTIME_US 16 // only used if sl_rail_get_symbol_rate returns 0
#define CCA_THRESHOLD_DEFAULT -75 // dBm - default for 2.4GHz 802.15.4

/* IEEE 802.15.4 FCF: ACK request is bit 5 of first FCF byte (MPDU byte 0 = tx_psdu[1]) */
#define SILABS_EFR32_FCF_ACK_REQUEST  BIT(5)
/* Frame type is bits 0..2 of first FCF byte (SiSDK: IEEE802154_FRAME_TYPE_MASK / _ACK / _DATA) */
#define SILABS_EFR32_FCF_FRAME_TYPE_MASK     ((uint16_t)0x0007U)
#define SILABS_EFR32_FCF_FRAME_TYPE_COMMAND  ((uint16_t)0x0003U)
#define SILABS_EFR32_FCF_FRAME_TYPE_ACK      ((uint16_t)0x0002U)
#define SILABS_EFR32_FCF_FRAME_TYPE_DATA     ((uint16_t)0x0001U)
/* DSN (Data Sequence Number) is first byte after 2-byte FCF in MPDU (SiSDK: IEEE802154_DSN_OFFSET) */
#define SILABS_EFR32_DSN_OFFSET  (2U)
#define SILABS_EFR32_FCF_OFFSET  (0U)

#define SILABS_RADIO_FLAG_NONE                 BIT(0)
#define SILABS_RADIO_FLAG_ONGOING_TX_DATA      BIT(1)
#define SILABS_RADIO_FLAG_ONGOING_TX_ACK       BIT(2)
#define SILABS_RADIO_FLAG_WAITING_FOR_ACK      BIT(3)

// Internal state variables
static atomic_t s_radio_state = ATOMIC_INIT(SILABS_RADIO_FLAG_NONE);

static inline void silabs_radio_state_set_flag(uint32_t flag, bool val)
{
    unsigned int key = irq_lock();
    uint32_t state = (uint32_t)atomic_get(&s_radio_state);
    state = val ? (state | flag) : (state & ~flag);
    atomic_set(&s_radio_state, (atomic_val_t)state);
    irq_unlock(key);
}

static inline bool silabs_radio_state_get_flag(uint32_t flag)
{
    unsigned int key = irq_lock();
    bool set = ((uint32_t)atomic_get(&s_radio_state) & flag) != 0U;
    irq_unlock(key);
    return set;
}

void silabs_radio_state_set_tx_ack_ongoing(bool ongoing)
{
    silabs_radio_state_set_flag(SILABS_RADIO_FLAG_ONGOING_TX_ACK, ongoing);
}

bool silabs_radio_state_is_tx_ack_ongoing(void)
{
    return silabs_radio_state_get_flag(SILABS_RADIO_FLAG_ONGOING_TX_ACK);
}

void silabs_radio_state_set_tx_data_ongoing(bool ongoing)
{
    silabs_radio_state_set_flag(SILABS_RADIO_FLAG_ONGOING_TX_DATA, ongoing);
}

bool silabs_radio_state_is_tx_data_ongoing(void)
{
    return silabs_radio_state_get_flag(SILABS_RADIO_FLAG_ONGOING_TX_DATA);
}

void silabs_radio_state_set_waiting_for_ack(bool waiting)
{
    silabs_radio_state_set_flag(SILABS_RADIO_FLAG_WAITING_FOR_ACK, waiting);
}

bool silabs_radio_state_is_waiting_for_ack(void)
{
    return silabs_radio_state_get_flag(SILABS_RADIO_FLAG_WAITING_FOR_ACK);
}

void silabs_radio_state_clear_tx_data_and_wait_for_ack(void)
{
    silabs_radio_state_set_flag(SILABS_RADIO_FLAG_ONGOING_TX_DATA | SILABS_RADIO_FLAG_WAITING_FOR_ACK, false);
}


static int skip_rx_packet_length_bytes(sl_rail_rx_packet_info_t *packet_info)
{
	if (packet_info->first_portion_bytes <= 0) {
		return -EINVAL;
	}

	packet_info->p_first_portion_data += PHY_HEADER_SIZE;
	packet_info->packet_bytes -= PHY_HEADER_SIZE;
	packet_info->first_portion_bytes -= PHY_HEADER_SIZE;

	return 0;
}

/*************************************************************************************************************************/
/// RAIL Helper Functions

/**
 * Convert a 32-bit microsecond counter (that wraps at 2^32) into a monotonic
 * 64-bit nanosecond value by counting wraps.
 *
 * @param now_us Current 32-bit usec reading from the hardware timer.
 * @return 64-bit nanoseconds = ((wrap_count << 32) | now_us) * 1000.
 *
 * @note Caller must serialize calls (e.g. irq_lock) if used from both ISR and
 *       thread, so wrap detection is consistent.
 */
static uint64_t us32_to_ns64(uint32_t now_us)
{
	static uint32_t prev_us;
	static uint64_t wraps;
	uint64_t us64;

	if (now_us < prev_us) {
		wraps++;
	}
	prev_us = now_us;
	us64 = ((uint64_t)wraps << 32) | (uint64_t)now_us;
	return us64 * (uint64_t)NSEC_PER_USEC;
}

static inline int yield_radio(void)
{
	sl_rail_status_t status = sl_rail_yield_radio(s_rail_handle);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	return 0;
}

static inline int idle_radio(void)
{
	sl_rail_status_t status = sl_rail_idle(s_rail_handle, SL_RAIL_IDLE, true);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	return 0;
}

static inline void set_radio_to_idle(void)
{
	if (sl_rail_get_radio_state(s_rail_handle) != SL_RAIL_RF_STATE_IDLE) {
		(void)idle_radio();
	}
}

/*************************************************************************************************************************/
/// Source match table helpers

/* Source match table helpers. config->ack_fpb.addr is little-endian. */
static int src_match_short_add(const uint8_t *addr_le)
{
	unsigned int key;
	key = irq_lock();
	uint16_t a = sys_get_le16(addr_le);
	for (uint8_t i = 0; i < s_src_match_short_count; i++) {
		if (s_src_match_short[i] == a) {
			irq_unlock(key);
			return 0; /* already present */
		}
	}
	if (s_src_match_short_count >= SILABS_SRC_MATCH_SHORT_MAX) {
		irq_unlock(key);
		return -ENOMEM;
	}
	s_src_match_short[s_src_match_short_count++] = a;
	irq_unlock(key);
	return 0;
}

static int src_match_short_remove(const uint8_t *addr_le)
{
	unsigned int key;
	key = irq_lock();
	uint16_t a = sys_get_le16(addr_le);
	for (uint8_t i = 0; i < s_src_match_short_count; i++) {
		if (s_src_match_short[i] == a) {
			s_src_match_short[i] = s_src_match_short[--s_src_match_short_count];
			irq_unlock(key);
			return 0;
		}
	}
	irq_unlock(key);
	return -ENOENT;
}

static void src_match_short_clear(void)
{
	unsigned int key;
	key = irq_lock();
	s_src_match_short_count = 0;
	irq_unlock(key);
}

static int src_match_ext_add(const uint8_t *addr)
{
	unsigned int key;
	key = irq_lock();
	for (uint8_t i = 0; i < s_src_match_ext_count; i++) {
		if (memcmp(s_src_match_ext[i], addr, 8) == 0) {
			irq_unlock(key);
			return 0; /* already present */
		}
	}
	if (s_src_match_ext_count >= SILABS_SRC_MATCH_EXT_MAX) {
		irq_unlock(key);
		return -ENOMEM;
	}
	memcpy(s_src_match_ext[s_src_match_ext_count++], addr, 8);
	irq_unlock(key);
	return 0;
}

static int src_match_ext_remove(const uint8_t *addr)
{
	unsigned int key;
	key = irq_lock();
	for (uint8_t i = 0; i < s_src_match_ext_count; i++) {
		if (memcmp(s_src_match_ext[i], addr, 8) == 0) {
			memcpy(s_src_match_ext[i], s_src_match_ext[s_src_match_ext_count - 1], 8);
			s_src_match_ext_count--;
			irq_unlock(key);
			return 0;
		}
	}
	irq_unlock(key);
	return -ENOENT;
}

static void src_match_ext_clear(void)
{
	unsigned int key;
	key = irq_lock();
	s_src_match_ext_count = 0;
	irq_unlock(key);
}

/* Used from DATA_REQUEST_COMMAND handler to decide if we set FP in the outgoing ACK. */
static bool src_match_short_contains(uint16_t short_addr)
{
	for (uint8_t i = 0; i < s_src_match_short_count; i++) {
		if (s_src_match_short[i] == short_addr) {
			return true;
		}
	}
	return false;
}

static bool src_match_ext_contains(const uint8_t *addr)
{
	for (uint8_t i = 0; i < s_src_match_ext_count; i++) {
		if (memcmp(s_src_match_ext[i], addr, 8) == 0) {
			return true;
		}
	}
	return false;
}


/*************************************************************************************************************************/
/// Radio Event Processsing

static uint8_t read_initial_packet_data(sl_rail_rx_packet_info_t *packet_info,
	uint8_t                   expected_data_bytes_max,
	uint8_t                   expected_data_bytes_min,
	uint8_t                  *buffer,
	uint8_t                   buffer_len)
{
	// Check if we have enough buffer
	if (buffer_len < expected_data_bytes_max) {
		// TODO handle assertion failure
		return 0;
	}

	// Read the packet info
	sl_rail_status_t status = sl_rail_get_rx_incoming_packet_info(s_rail_handle, packet_info);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		// TODO handle assertion failure
		return 0;
	}

	// We are trying to get the packet info of a packet before it is completely received.
	// We do this to evaluate the FP bit in response and add IEs to ACK if needed.
	// Check to see if we have received atleast minimum number of bytes requested.
	if (packet_info->packet_bytes < expected_data_bytes_min) {
		// TODO handle assertion failure
		return 0;
	}

	sl_rail_rx_packet_info_t adjusted_packet_info = *packet_info;

	// Only extract what we care about
	if (packet_info->packet_bytes > expected_data_bytes_max)
	{
		adjusted_packet_info.packet_bytes = expected_data_bytes_max;
		// Check if the initial portion of the packet received so far exceeds the max value requested.
		if (packet_info->first_portion_bytes >= expected_data_bytes_max)
		{
			// If we have received more, make sure to copy only the required bytes into the buffer.
			adjusted_packet_info.first_portion_bytes = expected_data_bytes_max;
			adjusted_packet_info.p_last_portion_data = NULL;
		}
	}

	// Copy number of bytes as indicated in `packet_info->first_portion_bytes` into the buffer.
	(void)sl_rail_copy_rx_packet(s_rail_handle, buffer, &adjusted_packet_info);
	// Put it back to packetBytes.
	return (uint8_t)adjusted_packet_info.packet_bytes;
}

static bool write_enhanced_ack(sl_rail_rx_packet_info_t *packet_info_for_enh_ack,
	uint32_t                  rxTimestamp,
	uint8_t                  *initial_pkt_read_bytes);

void handle_data_request_command(void)
{
#define MAX_EXPECTED_BYTES (2U + 2U + 1U) /* PHR + FCF + DSN */

	uint8_t                  pktOffset = PHY_HEADER_SIZE;
	sl_rail_rx_packet_info_t packet_info;

    // This callback occurs after the address fields of an incoming
    // ACK-requesting CMD or DATA frame have been received and we
    // can do a frame pending check.  We must also figure out what
    // kind of ACK is being requested -- Immediate or Enhanced.


    uint8_t initial_pkt_read_bytes = read_initial_packet_data(&packet_info, MAX_EXPECTED_BYTES, pktOffset + 2, silabs_efr32_data.rx_psdu, MAX_EXPECTED_BYTES);

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
	uint32_t                 rxCallbackTimestamp = sl_rail_get_time(s_rail_handle);
    if (write_enhanced_ack(&packet_info, rxCallbackTimestamp, &initial_pkt_read_bytes))
    {
        // We also return true above if there were failures in
        // generating an enhanced ACK.
        return;
    }
#endif

    // Calculate frame pending for immediate-ACK
    // If not, RAIL will send an immediate ACK, but we need to do FP lookup.
    sl_rail_status_t status = SL_RAIL_STATUS_NO_ERROR;

    // Check if we read the FCF, if not, set mac_fcf to 0
    uint16_t mac_fcf = (initial_pkt_read_bytes <= pktOffset) ? 0U : silabs_efr32_data.rx_psdu[pktOffset];

    bool frame_pending_set = false;

    if (silabs_efr32_data.is_src_match_enabled)
    {
		if (s_src_match_short_count > 0 || s_src_match_ext_count > 0) {
			sl_rail_ieee802154_address_t src_addr;

			if (sl_rail_ieee802154_get_address(s_rail_handle, &src_addr) == SL_RAIL_STATUS_NO_ERROR) {
				bool match = false;

				if (src_addr.address_length == SL_RAIL_IEEE802154_SHORT_ADDRESS) {
					match = src_match_short_contains(src_addr.short_address);
				} else if (src_addr.address_length == SL_RAIL_IEEE802154_LONG_ADDRESS) {
					match = src_match_ext_contains(src_addr.long_address);
				}
				if (match) {
					status = sl_rail_ieee802154_toggle_frame_pending(s_rail_handle);
					if (status != SL_RAIL_STATUS_NO_ERROR) {
						// TODO handle assertion failure
						return;
					}
					frame_pending_set = true;
				}
			}
		}
    }
    else if ((mac_fcf & SILABS_EFR32_FCF_FRAME_TYPE_MASK) != SILABS_EFR32_FCF_FRAME_TYPE_DATA)
    {
        status = sl_rail_ieee802154_toggle_frame_pending(s_rail_handle);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			// TODO handle assertion failure
			return;
		}
		frame_pending_set = true;
    }

    if (frame_pending_set)
    {
        // Store whether frame pending was set in the outgoing ACK in a reserved
        // bit of the MAC header (cleared later)

        if (skip_rx_packet_length_bytes(&packet_info) != 0) {
			// TODO handle assertion failure
			return;
		}
        uint8_t *mac_fcf_pointer =
            ((packet_info.first_portion_bytes == 0) ? packet_info.p_last_portion_data : packet_info.p_first_portion_data);
        *mac_fcf_pointer |= SILABS_EFR32_FCF_FRAME_PENDING_SET_IN_OUTGOING_ACK;
    }

    if (status == SL_RAIL_STATUS_INVALID_STATE)
    {
        LOG_WRN("Too late to modify outgoing FP");
    }
    else
    {
        // TODO handle assertion failures
		return;
    }
	// TODO
	return;
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
//------------------------------------------------------------------------------
// Radio implementation: Enhanced ACKs, CSL

// Return false if we should generate an immediate ACK
// Return true otherwise
static bool write_enhanced_ack(sl_rail_rx_packet_info_t *packet_info_for_enh_ack,
                                       uint32_t                  rxTimestamp,
                                       uint8_t                  *initial_pkt_read_bytes)
{
	// TODO implement
	return false;
}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

static inline void handle_tx_failed(void)
{
	silabs_radio_state_clear_tx_data_and_wait_for_ack();
	yield_radio();
}

static inline void handle_ack_timeout(void)
{
	__ASSERT_NO_MSG(silabs_radio_state_is_tx_data_ongoing());
	__ASSERT_NO_MSG(silabs_radio_state_is_waiting_for_ack());
	silabs_radio_state_set_tx_data_ongoing(false);
	silabs_radio_state_set_waiting_for_ack(false);
	s_tx_errno = -ENOMSG;
	yield_radio();
	s_em_pending_data = false;
}

static inline void handle_rx_failed(enum ieee802154_rx_fail_reason reason)
{
	s_rx_fail_reason = reason;
	atomic_or(&s_event_pending, SILABS_EV_PENDING_RX_FAILED);
	k_work_submit(&silabs_efr32_event_work);
}

static void handle_rx_ack(uint16_t length, sl_rail_rx_packet_details_t *packet_details)
{
	struct net_pkt *ack_pkt;

	/* allocate ack packet */
	ack_pkt = net_pkt_rx_alloc_with_buffer(silabs_efr32_data.iface, length, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		return;
	}

	/* update packet data */
	if (net_pkt_write(ack_pkt, silabs_efr32_data.rx_psdu, length) < 0) {
  		LOG_ERR("Failed to write to a packet.");
  		goto out;
	}

	/* update RSSI and LQI */
	net_pkt_set_ieee802154_lqi(ack_pkt, packet_details->lqi);
	net_pkt_set_ieee802154_rssi_dbm(ack_pkt, packet_details->rssi_dbm);
	// set timestamp of the ACK packet
	net_pkt_set_timestamp_ns(ack_pkt, us32_to_ns64(packet_details->time_received.packet_time));

	/* init net cursor */
	net_pkt_cursor_init(ack_pkt);

	/* handle ack */
	if (ieee802154_handle_ack(silabs_efr32_data.iface, ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled - releasing.");
	}

out:
	net_pkt_unref(ack_pkt);
	
}

static bool validate_pkt_timestamp(sl_rail_rx_packet_details_t *pPacketDetails, uint16_t packetLength)
{
	bool valid = true;
	// Get the timestamp when the SFD was received
	if (pPacketDetails->time_received.time_position == SL_RAIL_PACKET_TIME_INVALID) {
		valid = false;
	}

    // + PHY HEADER SIZE for PHY header
    // We would not need this if PHR is not included and we want the MHR
	pPacketDetails->time_received.total_packet_bytes = packetLength + PHY_HEADER_SIZE;
	sl_rail_status_t status = sl_rail_get_rx_time_sync_word_end(s_rail_handle, pPacketDetails);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		valid = false;
	}

	return valid;
}

/**
 * Called from RAIL events callback (ISR context) when RX_PACKET_RECEIVED fires.
 * RAIL does not hold packets once the callback returns; we must copy here.
 */
static void handle_rx_pkt(void)
{
	sl_rail_rx_packet_info_t packet_info;
	sl_rail_rx_packet_details_t packet_details;
	sl_rail_rx_packet_handle_t packet_handle;
	uint16_t packet_length;
	uint8_t mac_fcf;

	packet_handle = sl_rail_get_rx_packet_info(s_rail_handle, SL_RAIL_RX_PACKET_HANDLE_NEWEST, &packet_info);
	if (packet_handle == SL_RAIL_RX_PACKET_HANDLE_INVALID ||
	    packet_info.packet_status != SL_RAIL_RX_PACKET_READY_SUCCESS) {
		return;
	}

	if (sl_rail_get_rx_packet_details(s_rail_handle, packet_handle, &packet_details) != SL_RAIL_STATUS_NO_ERROR) {
		return;
	}

	/**
	 * RAIL's packetBytes includes the (1 or 2 byte) PHY header but not the 2-byte CRC.
	 * We want packet_length to match the PHY header length so we add 2 for CRC
	 * and subtract the PHY header size.
	 */
	packet_length = packet_info.packet_bytes + 2U - PHY_HEADER_SIZE;
	if (packet_length > IEEE802154_MAX_PHY_PACKET_SIZE || packet_length < 4U) {
		return;
	}
	if (packet_info.first_portion_bytes == 0) {
		return;
	}
	if (packet_length != packet_info.p_first_portion_data[0]) {
		return;
	}
	if (skip_rx_packet_length_bytes(&packet_info) != 0) {
		return;
	}
	if (!validate_pkt_timestamp(&packet_details, packet_length)) {
		return;
	}

	sl_rail_status_t status = sl_rail_copy_rx_packet(s_rail_handle, silabs_efr32_data.rx_psdu, &packet_info);

	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return;
	}

	mac_fcf = silabs_efr32_data.rx_psdu[0]; /* first FCF byte */

	if (packet_details.is_ack) {
		uint16_t tx_fcf = (uint16_t)silabs_efr32_data.tx_psdu[SILABS_EFR32_FCF_OFFSET] |
				  (uint16_t)(silabs_efr32_data.tx_psdu[SILABS_EFR32_FCF_OFFSET + 1] << 8);
		bool is_data_request = ((tx_fcf & SILABS_EFR32_FCF_FRAME_TYPE_MASK) ==
				       SILABS_EFR32_FCF_FRAME_TYPE_COMMAND);

		if ((mac_fcf & SILABS_EFR32_FCF_FRAME_TYPE_MASK) == SILABS_EFR32_FCF_FRAME_TYPE_ACK &&
		    ((silabs_efr32_data.tx_psdu[1] & SILABS_EFR32_FCF_ACK_REQUEST) != 0U) &&
		    silabs_efr32_data.rx_psdu[SILABS_EFR32_DSN_OFFSET] == silabs_efr32_data.tx_psdu[SILABS_EFR32_DSN_OFFSET]) {
			handle_rx_ack(packet_length, &packet_details);
			s_tx_errno = 0;
			silabs_radio_state_clear_tx_data_and_wait_for_ack();
			if (is_data_request && (mac_fcf & SILABS_EFR32_FCF_FRAME_PENDING) != 0) {
				s_em_pending_data = true;
			}
		}
		if (!is_data_request) {
			yield_radio();
		}
	} else {
		if (!silabs_efr32_data.promiscuous && packet_length < 4U) {
			return;
		}

		struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(silabs_efr32_data.iface, packet_length,
								  NET_AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("No pkt available");
			return;
		}
		if (net_pkt_write(pkt, silabs_efr32_data.rx_psdu, packet_length) < 0) {
			LOG_ERR("Failed to write to a packet.");
			net_pkt_unref(pkt);
			return;
		}
		net_pkt_set_ieee802154_lqi(pkt, packet_details.lqi);
		net_pkt_set_ieee802154_rssi_dbm(pkt, packet_details.rssi_dbm);
		net_pkt_set_timestamp_ns(pkt, us32_to_ns64(packet_details.time_received.packet_time));
		net_pkt_cursor_init(pkt);

		int net_status = net_recv_data(silabs_efr32_data.iface, pkt);
		if (net_status < 0) {
			LOG_ERR("RCV Packet dropped by NET stack: %d", net_status);
			net_pkt_unref(pkt);
			return;
		}

		if (mac_fcf & SILABS_EFR32_FCF_ACK_REQUIRED) {
			silabs_radio_state_set_tx_ack_ongoing(true);
		} else if (s_em_pending_data) {
			// We received a frame that does not require an ACK as result of a data
            // poll: we yield the radio here.
			(void)yield_radio();
			s_em_pending_data = false;
		}
	}
}

static inline void handle_scheduler_event(sl_rail_handle_t handle)
{
	sl_rail_scheduler_status_t scheduler_status;
    sl_rail_status_t           rail_status;
    sl_rail_get_scheduler_status(handle, &scheduler_status, &rail_status);
	if (scheduler_status != SL_RAIL_SCHEDULER_STATUS_NO_ERROR) {
		if (silabs_radio_state_is_tx_ack_ongoing()) {
			silabs_radio_state_set_tx_ack_ongoing(false);
		}
		// We were in the process of TXing a data frame, treat it as a CCA_FAIL.
		if (silabs_radio_state_is_tx_data_ongoing()) {
			s_tx_errno = -EBUSY;
			handle_tx_failed();

			// We are waiting for an ACK: we will never get the ACK we were waiting for.
			// We want to yield the radio only if the PACKET_SENT event
			// already fired.
			if (silabs_radio_state_is_waiting_for_ack()) {
				handle_ack_timeout();
			}
		}
	}
}

/* RAIL events callback (runs in ISR). Minimal work: set TX result + give sem; schedule RX work. */
static void silabs_efr32_rail_events_callback(sl_rail_handle_t handle, sl_rail_events_t events)
{
	/* Process data request command events */
	if ((events & SL_RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND) != 0ULL) {
		handle_data_request_command();
	}

	/* Process TX events */
	if ((events & SL_RAIL_EVENT_TX_PACKET_SENT) != 0ULL) {
		if (silabs_radio_state_is_tx_data_ongoing()) {
			if ((silabs_efr32_data.tx_psdu[1] & SILABS_EFR32_FCF_ACK_REQUEST) != 0U) {
				silabs_radio_state_set_waiting_for_ack(true);
			} else {
				silabs_radio_state_set_tx_data_ongoing(false);
				(void)yield_radio();
				s_tx_errno = 0;
			}
		}
	}
	if ((events & SL_RAIL_EVENT_TX_CHANNEL_BUSY) != 0ULL) {
		s_tx_errno = -EBUSY;
		handle_tx_failed();
	}
	if ((events & SL_RAIL_EVENT_TX_BLOCKED) != 0ULL) {
		s_tx_errno = -EIO;
		handle_tx_failed();
	}
	if ((events & (SL_RAIL_EVENT_TX_UNDERFLOW | SL_RAIL_EVENT_TX_ABORTED)) != 0ULL) {
		s_tx_errno = -EIO;
		handle_tx_failed();
	}

	if (events & SL_RAIL_EVENT_TX_SCHEDULED_TX_STARTED) {
        // do nothing
    } else if (events & SL_RAIL_EVENT_TX_SCHEDULED_TX_MISSED) {
		s_tx_errno = -EIO;
		handle_tx_failed();
    }

	if (events & SL_RAIL_EVENTS_TX_COMPLETION) {
		k_sem_give(&silabs_efr32_data.tx_wait);
	}

	/* Process scheduled events for Thread 1.2+ */
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
	if (events & SL_RAIL_EVENT_RX_SCHEDULED_RX_STARTED) {
		// do nothing
	}

	if (events & SL_RAIL_EVENT_RX_SCHEDULED_RX_END || events & SL_RAIL_EVENT_RX_SCHEDULED_RX_MISSED) {
		atomic_or(&s_event_pending, SILABS_EV_PENDING_RX_OFF);
		k_work_submit(&silabs_efr32_event_work);
		set_radio_to_idle();
	}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

	/* Process RX packet received events */
	if (events & SL_RAIL_EVENT_RX_PACKET_RECEIVED) {
		handle_rx_pkt();
	}

	/* Process ACK events */
	if (events & SL_RAIL_EVENT_TXACK_PACKET_SENT) {
		silabs_radio_state_set_tx_ack_ongoing(false);
		// We successfully sent out an ACK.
		// We acked the packet we received after a poll: we can yield now.
		if (s_em_pending_data) {
			(void)yield_radio();
			s_em_pending_data = false;
		}
    }

    if (events & (SL_RAIL_EVENT_TXACK_ABORTED | SL_RAIL_EVENT_TXACK_UNDERFLOW)) {
		silabs_radio_state_set_tx_ack_ongoing(false);
    }

    if (events & SL_RAIL_EVENT_TXACK_BLOCKED) {
        // Do nothing
    }

    // Deal with ACK timeout after possible RX completion in case RAIL
    // notifies us of the ACK and the timeout simultaneously -- we want
    // the ACK to win over the timeout.
    if ((events & SL_RAIL_EVENT_RX_ACK_TIMEOUT) && silabs_radio_state_is_waiting_for_ack())
    {
		handle_ack_timeout();
    }

	// Process scheduler events
	if (events & SL_RAIL_EVENT_SCHEDULER_STATUS) {
		handle_scheduler_event(handle);
	}

	if (events & SL_RAIL_EVENT_CAL_NEEDED) {
		sl_rail_status_t status;
		status = sl_rail_calibrate(handle, NULL, SL_RAIL_CAL_ALL_PENDING);
	}

	// scheduled and unscheduled config events happen very often,
    // especially in a DMP situation where there is an active BLE connection.
    // Waking up the OT RTOS task on every one of these occurrences causes
    // a lower priority Serial task to starve and makes it appear like a code lockup
    // There is no reason to wake the OT task for these events!
	if (!(events & SL_RAIL_EVENT_CONFIG_SCHEDULED) && !(events & SL_RAIL_EVENT_CONFIG_UNSCHEDULED)) {
		return;
	}
}

/* Run in thread context: deliver TX_STARTED / RX_FAILED to Zephyr event_handler. */
static void silabs_efr32_event_work_handler(struct k_work *work)
{
	uint32_t pending;
	const struct device *dev;
	struct silabs_efr32_802154_data *data;

	ARG_UNUSED(work);

	pending = (uint32_t)atomic_set(&s_event_pending, 0);
	if (pending == 0) {
		return;
	}
	dev = silabs_efr32_get_device();
	if (dev == NULL) {
		return;
	}
	data = (struct silabs_efr32_802154_data *)dev->data;
	if (data->event_handler == NULL) {
		return;
	}
	if ((pending & SILABS_EV_PENDING_TX_STARTED) != 0U) {
		data->event_handler(dev, IEEE802154_EVENT_TX_STARTED, NULL);
	}
	if ((pending & SILABS_EV_PENDING_RX_FAILED) != 0U) {
		data->event_handler(dev, IEEE802154_EVENT_RX_FAILED, (void *)&s_rx_fail_reason);
	}
	if ((pending & SILABS_EV_PENDING_RX_OFF) != 0U) {
		data->event_handler(dev, IEEE802154_EVENT_RX_OFF, NULL);
	}
}

/* IEEE 802.15.4 RAIL config (matches PAL sRailIeee802154Config). */
static const sl_rail_ieee802154_config_t s_rail_ieee802154_config = {
	.p_addresses = NULL,
	.ack_config = {
		.enable = true,
		.ack_timeout_us = 672,
		.rx_transitions = { .success = SL_RAIL_RF_STATE_RX, .error = SL_RAIL_RF_STATE_RX },
		.tx_transitions = { .success = SL_RAIL_RF_STATE_RX, .error = SL_RAIL_RF_STATE_RX },
	},
	.timings = {
		.idle_to_rx = 100,
		.tx_to_rx = 192 - 10,
		.idle_to_tx = 100,
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
		.rx_to_tx = 256, // accommodate enhanced ACKs
#else
		.rx_to_tx = 192,
#endif
		.rxsearch_timeout = 0,
		.tx_to_rxsearch_timeout = 0,
		.tx_to_tx = 0,
	},
	.frames_mask = SL_RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
	.promiscuous_mode = false,
	.is_pan_coordinator = false,
	.default_frame_pending_in_outgoing_acks = false,
};

/* RAIL buffers: use SDK builtin for RX; provide our own TX FIFO (no builtin).
 * If sl_rail_init fails with INVALID_PARAMETER, init with minimal config (buffers
 * 0/NULL) and set RX/TX via sl_rail_set_* after init.
 */
#define SILABS_RAIL_TX_FIFO_BYTES   IEEE802154_MAX_PHY_PACKET_SIZE + 1
static sl_rail_fifo_buffer_align_t s_rail_tx_fifo[SILABS_RAIL_TX_FIFO_BYTES / sizeof(sl_rail_fifo_buffer_align_t)];

static int silabs_efr32_rail_init(void)
{
	sl_rail_status_t status;
	sl_rail_handle_t handle = SL_RAIL_EFR32_HANDLE;
	sl_rail_config_t rail_config = { 0 };

	rail_config.events_callback = silabs_efr32_rail_events_callback;

	rail_config.rx_packet_queue_entries = sl_rail_builtin_rx_packet_queue_entries;
	rail_config.p_rx_packet_queue = sl_rail_builtin_rx_packet_queue_ptr;

	rail_config.rx_fifo_bytes = sl_rail_builtin_rx_fifo_bytes;
	rail_config.p_rx_fifo_buffer = sl_rail_builtin_rx_fifo_ptr;

	rail_config.tx_fifo_bytes = SILABS_RAIL_TX_FIFO_BYTES;
	rail_config.p_tx_fifo_buffer = s_rail_tx_fifo;

	status = sl_rail_init(&handle, &rail_config, NULL);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_init failed: %d", (int)status);
		return -EIO;
	}
	status = sl_rail_init_power_manager();
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_init_power_manager failed: %d", (int)status);
		return -EIO;
	}
	status = sl_rail_config_cal(handle, SL_RAIL_CAL_ALL);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_config_cal failed: %d", (int)status);
		return -EIO;
	}
	status = sl_rail_set_pti_protocol(handle, SL_RAIL_PTI_PROTOCOL_THREAD);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_set_pti_protocol failed: %d", (int)status);
		return -EIO;
	}
	status = sl_rail_ieee802154_init(handle, &s_rail_ieee802154_config);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_ieee802154_init failed: %d", (int)status);
		return -EIO;
	}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
	status = sl_rail_ieee802154_enable_early_frame_pending(handle, true);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_ieee802154_enable_early_frame_pending failed: %d", (int)status);
		return -EIO;
	}
	status = sl_rail_ieee802154_enable_data_frame_pending(handle, true);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_ieee802154_enable_data_frame_pending failed: %d", (int)status);
		return -EIO;
	}
	// TODO: sl_ot_radio_security_init()
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

	// Enable RAIL multi-timer
    sl_rail_config_multi_timer(handle, true);

	s_rail_handle = handle;

	return 0;
}

#if DT_INST_NODE_HAS_PROP(0, hfxo)
#define SILABS_EFR32_HFXO_NODE DT_INST_PHANDLE(0, hfxo)
#define SILABS_EFR32_SCH_ACCURACY_PPM DT_PROP(SILABS_EFR32_HFXO_NODE, precision)
#else
#define SILABS_EFR32_SCH_ACCURACY_PPM 250
#endif

struct silabs_efr32_802154_config {
	void (*irq_config_func)(const struct device *dev);
};

#define SILABS_EFR32_802154_DATA(dev) \
	((struct silabs_efr32_802154_data * const)(dev)->data)

#define SILABS_EFR32_802154_CFG(dev) \
	((const struct silabs_efr32_802154_config * const)(dev)->config)

#if CONFIG_IEEE802154_VENDOR_OUI_ENABLE
#define IEEE802154_SILABS_EFR32_VENDOR_OUI CONFIG_IEEE802154_VENDOR_OUI
#else
#define IEEE802154_SILABS_EFR32_VENDOR_OUI (uint32_t)0xF4CE36
#endif

#if defined(CONFIG_IEEE802154_RAW_MODE)
static const struct device *silabs_efr32_dev;
#endif

static inline const struct device *silabs_efr32_get_device(void)
{
#if defined(CONFIG_IEEE802154_RAW_MODE)
	return silabs_efr32_dev;
#else
	return net_if_get_device(silabs_efr32_data.iface);
#endif
}

IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(silabs_efr32_drv_attr, SL_CHANNEL_MIN, SL_CHANNEL_MAX);

static int silabs_efr32_attr_get(const struct device *dev,
				  enum ieee802154_attr attr,
				  struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	if (ieee802154_attr_get_channel_page_and_range(
		    attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		    &silabs_efr32_drv_attr.phy_supported_channels, value) == 0) {
		return 0;
	}

	return -ENOENT;
}

static void silabs_efr32_irq_config(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* RAIL IRQs are installed by rail_isr_installer() in soc_radio.c (soc_early_init_hook). */
}

static void silabs_efr32_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);
	uint64_t eui64;

	data->iface = iface;

	/* EUI64 from platform unique ID (matches SiSDK otPlatRadioGetIeeeEui64). */
	eui64 = sys_cpu_to_be64(SYSTEM_GetUnique());
	memcpy(data->mac, &eui64, sizeof(data->mac));

	net_if_set_link_addr(iface, data->mac, sizeof(data->mac),
			    NET_LINK_IEEE802154);

	/* Call L2 init (e.g. OpenThread: openthread_l2_init -> platformRadioInit).
	 * Without this, net_if_up -> openthread_run -> ThreadNetif::Up() runs with
	 * radio_api still NULL and faults in otPlatRadioSleep. */
	ieee802154_init(iface);
}

static enum ieee802154_hw_caps silabs_efr32_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* Aligned with EFR32 PAL radio_interface.cpp sRadioCapabilities (otRadioCaps):
	 * ACK_TIMEOUT|CSMA_BACKOFF|ENERGY_SCAN|SLEEP_TO_TX|TRANSMIT_SEC|TRANSMIT_TIMING|
	 * RECEIVE_TIMING -> IEEE802154_HW_* equivalents. TX/RX/TX_SEC/ed_scan/start/stop
	 * implemented in medium/hard phase.
	 */
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER | IEEE802154_HW_TX_RX_ACK
	       | IEEE802154_HW_ENERGY_SCAN | IEEE802154_HW_SLEEP_TO_TX | IEEE802154_HW_CSMA
	       | IEEE802154_HW_TX_SEC | IEEE802154_HW_TXTIME | IEEE802154_HW_RXTIME;
}

static int silabs_efr32_cca(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* EFR32 PAL does hardware CCA; this API is not supported (Confluence).
	 * When using RAIL: would map to sl_rail_get_rssi() vs CCA_THRESHOLD_DEFAULT.
	 */
	return -ENOTSUP;
}

static int silabs_efr32_set_channel(const struct device *dev, uint16_t channel)
{
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);

	if (channel < SL_CHANNEL_MIN || channel > SL_CHANNEL_MAX) {
		return -EINVAL;
	}
	data->current_channel = channel;
	/* Channel is applied on next start_rx (start() or after stop/start). */
	return 0;
}

static int silabs_efr32_filter(const struct device *dev, bool set,
			       enum ieee802154_filter_type type,
			       const struct ieee802154_filter *filter)
{
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);

	if (!set) {
		return -ENOTSUP;
	}
	if (filter == NULL) {
		return -EINVAL;
	}
	switch (type) {
	case IEEE802154_FILTER_TYPE_PAN_ID:
		if (s_rail_initialized) {
			data->pan_id = filter->pan_id;
			sl_rail_ieee802154_set_pan_id(s_rail_handle, filter->pan_id, 0);
		}
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		if (s_rail_initialized) {
			data->short_addr = filter->short_addr;
			sl_rail_ieee802154_set_short_address(s_rail_handle, filter->short_addr, 0);
		}
		break;
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		if (s_rail_initialized) {
			memcpy(data->ext_addr, filter->ieee_addr, sizeof(data->ext_addr));
			sl_rail_ieee802154_set_long_address(s_rail_handle, filter->ieee_addr, 0);
		}
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int silabs_efr32_set_txpower(const struct device *dev, int16_t dbm)
{
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);

	if (s_rail_initialized) {
		sl_rail_tx_power_t tx_power = (sl_rail_tx_power_t)dbm * 10;
		data->txpwr = tx_power;
		sl_rail_status_t status = sl_rail_set_tx_power_dbm(s_rail_handle, tx_power);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			return -EIO;
		}

	}
	return 0;
}

static int silabs_efr32_start(const struct device *dev)
{
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);

	if (s_rail_initialized) {
		sl_rail_scheduler_info_t bg_rx = {
			.priority = SL_802154_RADIO_PRIO_BACKGROUND_RX_VALUE,
			.slip_time = 0,
			.transaction_time = 0,
		};
		sl_rail_status_t status = sl_rail_start_rx(s_rail_handle,
							   (uint8_t)data->current_channel,
							   &bg_rx);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_ERR("sl_rail_start_rx failed: %d", (int)status);
			return -EIO;
		}
	}
	return 0;
}

static int silabs_efr32_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	if (s_rail_initialized) {
		set_radio_to_idle();
	}
	return 0;
}

static int silabs_efr32_tx(const struct device *dev, enum ieee802154_tx_mode mode,
			   struct net_pkt *pkt, struct net_buf *frag)
{
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);
	uint8_t len;
	const uint8_t *payload;

	if (!s_rail_initialized) {
		return -ENOTSUP;
	}

	idle_radio();

	__ASSERT_NO_MSG(silabs_radio_state_is_tx_data_ongoing() == false);

	// TODO sli_ot_radio_security_process_transmit

	silabs_radio_state_set_tx_data_ongoing(true);

	sl_rail_tx_options_t tx_options = SL_RAIL_TX_OPTIONS_DEFAULT;

	payload = frag->data;
	len = (uint8_t)frag->len;
	if (len > sizeof(data->tx_psdu) - 1) {
		return -EMSGSIZE;
	}

	/* PHR (first byte) = length; then MPDU */
	data->tx_psdu[0] = len + 2U; /* FCS */
	memcpy(&data->tx_psdu[1], payload, len);

	(void)sl_rail_write_tx_fifo(s_rail_handle, data->tx_psdu, (len + 2U + 1U), true);
	k_sem_reset(&data->tx_wait);
	k_sem_reset(&data->ack_wait);
	s_tx_errno = -EIO; /* default if no event fires */

	sl_rail_scheduler_info_t tx_scheduler_info = {
        .priority         = RADIO_PRIORITY_TX_MIN,
        .slip_time        = RADIO_SCHEDULER_CHANNEL_SLIP_TIME,
        .transaction_time = 0 // will be calculated later if DMP is used
    };

	if (payload[0] & SILABS_EFR32_FCF_ACK_REQUEST) {
		tx_options |= SL_RAIL_TX_OPTION_WAIT_FOR_ACK;
		// time we wait for ACK
		if(sl_rail_get_symbol_rate(s_rail_handle) > 0) {
			tx_scheduler_info.transaction_time +=
                (sl_rail_time_t)(12 * 1e6 / sl_rail_get_symbol_rate(s_rail_handle));
		} else {
			tx_scheduler_info.transaction_time += 12 * RADIO_TIMING_DEFAULT_SYMBOLTIME_US;
		}
	}

	if (sl_rail_get_bit_rate(s_rail_handle) > 0) {
        tx_scheduler_info.transaction_time +=
            (sl_rail_time_t)((len + 4 + 1 + 1) * 8 * 1e6 / sl_rail_get_bit_rate(s_rail_handle));
	} else { // assume 250kbps
        tx_scheduler_info.transaction_time += (len + 4 + 1 + 1) * RADIO_TIMING_DEFAULT_BYTETIME_US;
	}

	// Prioritize the Tx over schedule Rx to avoid missing data check-ins such as data polls.
    idle_radio();

	sl_rail_status_t start_st;

	switch (mode) {
		case IEEE802154_TX_MODE_CSMA_CA:
			// time needed for CSMA/CA
			tx_scheduler_info.transaction_time += RADIO_TIMING_CSMA_OVERHEAD_US;
			sl_rail_csma_config_t csma = SL_RAIL_CSMA_CONFIG_802_15_4_2003_2P4_GHZ_OQPSK_CSMA;
			csma.cca_threshold_dbm = CCA_THRESHOLD_DEFAULT;
			start_st = sl_rail_start_cca_csma_tx(s_rail_handle,
				(uint8_t)data->current_channel,
				tx_options, &csma, &tx_scheduler_info);
			break;
		case IEEE802154_TX_MODE_DIRECT:
			start_st = sl_rail_start_tx(s_rail_handle, (uint8_t)data->current_channel,
			tx_options, &tx_scheduler_info);
			break;
		default:
			LOG_ERR("TX mode %d not supported", mode);
			return -ENOTSUP;
	}

	if (start_st != SL_RAIL_STATUS_NO_ERROR) {
		handle_tx_failed();
		LOG_ERR("sl_rail_start_tx/cca_csma_tx failed: %d (channel %u)",
			(int)start_st, (unsigned int)data->current_channel);
		return -EIO;
	}

	/* Zephyr event notification: TX_STARTED (host data/command TX on air). *//* Zephyr event notification: TX_STARTED (host data/command TX on air). */
	atomic_or(&s_event_pending, SILABS_EV_PENDING_TX_STARTED);
	k_work_submit(&silabs_efr32_event_work);

	/* Block until RAIL events callback sets s_tx_errno and gives tx_wait. */
	k_sem_take(&data->tx_wait, K_FOREVER);

	if ((silabs_efr32_data.tx_psdu[1] & SILABS_EFR32_FCF_ACK_REQUEST) != 0U) {
		k_sem_take(&data->ack_wait, K_FOREVER);
	}
	(void)pkt;
	return s_tx_errno;
}

static void silabs_efr32_energy_scan_work(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(silabs_efr32_ed_scan_work, silabs_efr32_energy_scan_work);
static const struct device *s_ed_scan_dev;
static energy_scan_done_cb_t s_ed_scan_done_cb;
static uint16_t s_ed_scan_duration_ms;
static uint16_t s_ed_scan_elapsed;
static int16_t s_ed_scan_max_rssi;
#define SILABS_ED_SCAN_SAMPLE_MS 1
static void silabs_efr32_energy_scan_work(struct k_work *work)
{
	int16_t rssi;

	ARG_UNUSED(work);
	if (s_ed_scan_elapsed >= s_ed_scan_duration_ms) {
		if (s_ed_scan_done_cb && s_ed_scan_dev) {
			s_ed_scan_done_cb(s_ed_scan_dev, s_ed_scan_max_rssi);
		}
		return;
	}
	rssi = sl_rail_get_rssi(s_rail_handle, SL_RAIL_GET_RSSI_NO_WAIT);
	if (rssi != SL_RAIL_RSSI_INVALID_DBM && rssi > s_ed_scan_max_rssi) {
		s_ed_scan_max_rssi = rssi;
	}
	s_ed_scan_elapsed += SILABS_ED_SCAN_SAMPLE_MS;
	k_work_schedule(&silabs_efr32_ed_scan_work, K_MSEC(SILABS_ED_SCAN_SAMPLE_MS));
}

static int silabs_efr32_energy_scan_start(const struct device *dev, uint16_t duration,
					   energy_scan_done_cb_t done_cb)
{
	if (!s_rail_initialized || done_cb == NULL) {
		return -ENOTSUP;
	}
	s_ed_scan_dev = dev;
	s_ed_scan_done_cb = done_cb;
	s_ed_scan_duration_ms = duration;
	s_ed_scan_elapsed = 0;
	s_ed_scan_max_rssi = SL_RAIL_RSSI_INVALID_DBM;
	k_work_schedule(&silabs_efr32_ed_scan_work, K_MSEC(SILABS_ED_SCAN_SAMPLE_MS));
	return 0;
}

static net_time_t silabs_efr32_get_time(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t now_32_us;
	uint64_t ns64;
	unsigned int key;

	if (!s_rail_initialized) {
		return -1;
	}
	key = irq_lock();
	now_32_us = sl_rail_get_time(s_rail_handle);
	ns64 = us32_to_ns64(now_32_us);
	irq_unlock(key);
	return (net_time_t)ns64;
}

static uint8_t silabs_efr32_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* CSL scheduler accuracy in ppm. When the board sets hfxo = &hfxo on the
	 * ieee802154 node, use the HFXO's precision (matches PAL SL_OPENTHREAD_HFXO_ACCURACY).
	 * Otherwise fall back to 250 ppm.
	 */
	return (uint8_t)SILABS_EFR32_SCH_ACCURACY_PPM;
}

static int silabs_efr32_configure(const struct device *dev,
				   enum ieee802154_config_type type,
				   const struct ieee802154_config *config)
{
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);
	sl_rail_status_t status;

	if (config == NULL) {
		return -EINVAL;
	}

	switch (type) {
	case IEEE802154_CONFIG_PROMISCUOUS:
		if (!s_rail_initialized) {
			return 0;
		}
		data->promiscuous = config->promiscuous;
		status = sl_rail_ieee802154_set_promiscuous_mode(s_rail_handle,
								 config->promiscuous);
		return (status == SL_RAIL_STATUS_NO_ERROR) ? 0 : -EIO;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		if (!s_rail_initialized) {
			return 0;
		}
		data->pan_coordinator = config->pan_coordinator;
		status = sl_rail_ieee802154_set_pan_coordinator(s_rail_handle, config->pan_coordinator);
		return (status == SL_RAIL_STATUS_NO_ERROR) ? 0 : -EIO;

	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		data->is_src_match_enabled = config->auto_ack_fpb.enabled;
		return 0;

	case IEEE802154_CONFIG_ACK_FPB:
		/* Soft source match table (SiSDK soft_source_table_match / AddSrcMatch*). */
		if (config->ack_fpb.enabled) {
			if (config->ack_fpb.addr == NULL) {
				return -EINVAL;
			}
			if (config->ack_fpb.extended) {
				return src_match_ext_add(config->ack_fpb.addr);
			} else {
				return src_match_short_add(config->ack_fpb.addr);
			}
		}
		if (config->ack_fpb.addr != NULL) {
			if (config->ack_fpb.extended) {
				return src_match_ext_remove(config->ack_fpb.addr);
			} else {
				return src_match_short_remove(config->ack_fpb.addr);
			}
		}
		if (config->ack_fpb.extended) {
			src_match_ext_clear();
		} else {
			src_match_short_clear();
		}
		return 0;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		/* event_handler is invoked from silabs_efr32_event_work_handler for
		 * IEEE802154_EVENT_TX_STARTED (RAIL TX_STARTED) and IEEE802154_EVENT_RX_FAILED
		 * (RAIL RX_FRAME_ERROR / RX_FIFO_OVERFLOW / RX_PACKET_ABORTED / RX_ADDRESS_FILTERED).
		 * IEEE802154_EVENT_RX_OFF is not supported (CSL-only).
		 */
		data->event_handler = config->event_handler;
		return 0;

	case IEEE802154_CONFIG_MAC_KEYS: {
		struct ieee802154_key *in = config->mac_keys;

		if (in == NULL) {
			return -EINVAL;
		}
		uint8_t sec_key_count = 0;
		if (in->key_value == NULL) {
			/* Clear all keys (e.g. OpenThread stack reset). */
			return 0;
		}
		for (; in->key_value != NULL && sec_key_count < SILABS_SEC_KEY_SLOTS; in++) {
			memcpy(&s_sec_keys[sec_key_count], in, sizeof(struct ieee802154_key));	
			sec_key_count++;
		}
		return 0;
	}

	case IEEE802154_CONFIG_FRAME_COUNTER:
		if (config->frame_counter <= data->frame_counter) {
			return -EINVAL;
		}
		data->frame_counter = config->frame_counter;
		return 0;

	case IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER:
		if (config->frame_counter > data->frame_counter) {
			data->frame_counter = config->frame_counter;
		}
		return 0;

	case IEEE802154_CONFIG_RX_SLOT:
		// TODO: look up scheduler status to determine if we were scheduled to transmit a packet 
		// and if so, return an error
		sl_rail_scheduler_info_t scheduler_info = {
			.priority = SL_802154_RADIO_PRIO_BACKGROUND_RX_VALUE,
			.slip_time = 0,
			.transaction_time = 0,
		};
		sl_rail_scheduled_rx_config_t scheduled_rx_config = {
			.start = config->rx_slot.start,
			.start_mode = SL_RAIL_TIME_ABSOLUTE,
			.end = config->rx_slot.duration,
			.end_mode = SL_RAIL_TIME_DELAY,
			.rx_transition_end_schedule = 0, // To stay in schedule Rx state after packet receive.
			.hard_window_end = 0, // This lets us receive a packet near a window-end-event
		};
		sl_rail_start_scheduled_rx(s_rail_handle, data->current_channel, &scheduled_rx_config, &scheduler_info);
		return 0;
	
	case IEEE802154_CONFIG_CSL_PERIOD:
		data->csl_period = config->csl_period;
		return 0;

	case IEEE802154_CONFIG_EXPECTED_RX_TIME:
		data->csl_sample_time = config->expected_rx_time;
		return 0;

	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		return -ENOTSUP;

	default:
		return -ENOTSUP;
	}
}

#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
static int silabs_efr32_continuous_carrier(const struct device *dev)
{
	ARG_UNUSED(dev);
	sl_rail_status_t status;
	sl_rail_tx_options_t tx_options = SL_RAIL_TX_OPTIONS_DEFAULT;
	status = sl_rail_start_tx_stream(s_rail_handle, data->current_channel, SL_RAIL_STREAM_CARRIER_WAVEFORM, tx_options);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	return 0;
}

static int silabs_efr32_modulated_carrier(const struct device *dev, const uint8_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	return -ENOTSUP;
}
#endif

static const struct silabs_efr32_802154_config silabs_efr32_radio_cfg = {
	.irq_config_func = silabs_efr32_irq_config,
};

static const struct ieee802154_radio_api silabs_efr32_radio_api = {
	.iface_api.init = silabs_efr32_iface_init,

	.get_capabilities = silabs_efr32_get_capabilities,
	.cca = silabs_efr32_cca,
	.set_channel = silabs_efr32_set_channel,
	.filter = silabs_efr32_filter,
	.set_txpower = silabs_efr32_set_txpower,
	.start = silabs_efr32_start,
	.stop = silabs_efr32_stop,
	.tx = silabs_efr32_tx,
	.ed_scan = silabs_efr32_energy_scan_start,
	.get_time = silabs_efr32_get_time,
	.get_sch_acc = silabs_efr32_get_acc,
	.configure = silabs_efr32_configure,
	.attr_get = silabs_efr32_attr_get,
#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
	.continuous_carrier = silabs_efr32_continuous_carrier,
	.modulated_carrier = silabs_efr32_modulated_carrier,
#endif
};

static int silabs_efr32_init(const struct device *dev)
{
	if (s_rail_initialized) {
		return -EALREADY;
	}

	const struct silabs_efr32_802154_config *cfg = SILABS_EFR32_802154_CFG(dev);
	struct silabs_efr32_802154_data *data = SILABS_EFR32_802154_DATA(dev);

	k_sem_init(&data->cca_wait, 0, 1);
	k_sem_init(&data->tx_wait, 0, 1);
	k_sem_init(&data->ack_wait, 0, 1);

	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func(dev);
	}

	{
		sl_openthread_init();
		
		sl_rail_status_t status;

		int r = silabs_efr32_rail_init();

		if (r != 0) {
			return r;
		}

		/* PA init before band config (PAL calls sl_rail_util_pa_init() in efr32RadioInit). */
		sl_rail_util_pa_init();

		status = sl_rail_config_events(s_rail_handle, SL_RAIL_EVENTS_ALL,
			(0 | SL_RAIL_EVENT_RX_ACK_TIMEOUT | SL_RAIL_EVENT_RX_PACKET_RECEIVED
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
				| SL_RAIL_EVENT_TX_SCHEDULED_TX_STARTED | SL_RAIL_EVENT_TX_SCHEDULED_TX_MISSED
				| SL_RAIL_EVENT_RX_SCHEDULED_RX_STARTED | SL_RAIL_EVENT_RX_SCHEDULED_RX_END
				| SL_RAIL_EVENT_RX_SCHEDULED_RX_MISSED
#endif
				| SL_RAIL_EVENTS_TXACK_COMPLETION | SL_RAIL_EVENTS_TX_COMPLETION
				| SL_RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND
				| SL_RAIL_EVENT_CONFIG_SCHEDULED | SL_RAIL_EVENT_CONFIG_UNSCHEDULED
				| SL_RAIL_EVENT_SCHEDULER_STATUS
				| SL_RAIL_EVENT_CAL_NEEDED));
		__ASSERT_NO_MSG(status == SL_RAIL_STATUS_NO_ERROR);

#if defined(SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT)
		status = sl_rail_util_ieee802154_config_radio(s_rail_handle);
#else
		status = sl_rail_ieee802154_config_2p4_ghz_radio(s_rail_handle);
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT
		__ASSERT_NO_MSG(status == SL_RAIL_STATUS_NO_ERROR);

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
		// 802.15.4E support (only on platforms that support it, so error checking is disabled)
		// Note: This has to be called after sl_rail_ieee802154_config_2p4_ghz_radio due to a bug where this call
		// can overwrite options set below.
		sl_rail_ieee802154_config_e_options(s_rail_handle,
											(SL_RAIL_IEEE802154_E_OPTION_GB868 | SL_RAIL_IEEE802154_E_OPTION_ENH_ACK),
											(SL_RAIL_IEEE802154_E_OPTION_GB868 | SL_RAIL_IEEE802154_E_OPTION_ENH_ACK));
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

		__ASSERT_NO_MSG(sl_rail_util_pa_post_init(s_rail_handle, SL_RAIL_TX_PA_MODE_2P4_GHZ) == SL_RAIL_STATUS_NO_ERROR);

		__ASSERT_NO_MSG(silabs_efr32_set_txpower(silabs_efr32_get_device(), (sl_rail_tx_power_t)OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER) == 0);

		s_rail_initialized = true;

		sl_rail_timer_sync_config_t timer_sync_config = SL_RAIL_TIMER_SYNC_DEFAULT;
		status = sl_rail_config_sleep(s_rail_handle, &timer_sync_config);
		__ASSERT_NO_MSG(status == SL_RAIL_STATUS_NO_ERROR);
		
		memset(data->tx_psdu, 0, sizeof(data->tx_psdu));

		// ignoring OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

		sl_rail_config_rx_options(s_rail_handle, SL_RAIL_RX_OPTION_TRACK_ABORTED_FRAMES, SL_RAIL_RX_OPTION_TRACK_ABORTED_FRAMES);
		__ASSERT_NO_MSG(status == SL_RAIL_STATUS_NO_ERROR);

		// ignoring efr32PhyStackInit

		// TODO add Kconfig for CCA mode
		sl_rail_ieee802154_config_cca_mode(s_rail_handle, SL_RAIL_IEEE802154_CCA_MODE_RSSI);
		__ASSERT_NO_MSG(status == SL_RAIL_STATUS_NO_ERROR);

		// TODO sli_ot_energy_scan_init

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
		// TODO sli_ot_radio_csl_init
#endif
		LOG_INF("SILABS_EFR32_INIT: Success");
	}

#if defined(CONFIG_IEEE802154_RAW_MODE)
	silabs_efr32_dev = dev;
#endif

	return 0;
}

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#elif defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
#define L2 CUSTOM_IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(CUSTOM_IEEE802154_L2)
#define MTU CONFIG_NET_L2_CUSTOM_IEEE802154_MTU
#endif

#if defined(CONFIG_NET_L2_PHY_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, silabs_efr32_init, NULL,
			  &silabs_efr32_data, &silabs_efr32_radio_cfg,
			  CONFIG_IEEE802154_SILABS_EFR32_INIT_PRIO,
			  &silabs_efr32_radio_api, L2, L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, silabs_efr32_init, NULL,
		     &silabs_efr32_data, &silabs_efr32_radio_cfg,
		     POST_KERNEL, CONFIG_IEEE802154_SILABS_EFR32_INIT_PRIO,
		     &silabs_efr32_radio_api);
#endif
