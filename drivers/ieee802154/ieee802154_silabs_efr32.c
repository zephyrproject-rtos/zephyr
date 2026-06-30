/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ieee802154_silabs_efr32.c - Silabs EFR32 802.15.4 driver */

#define DT_DRV_COMPAT silabs_efr32_ieee802154

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio_openthread.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ptp_time.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(ieee802154_silabs_efr32, CONFIG_IEEE802154_DRIVER_LOG_LEVEL);

#if CONFIG_ZEPHYR_OPENTHREAD_MODULE
#include <openthread/platform/crypto.h>
#include <openthread/platform/radio.h>
#include <openthread/error.h>
#include <openthread/link.h>
#include <openthread-core-zephyr-config.h>
#endif

#include <sli_crypto.h>

#include "ieee802154_silabs_efr32.h"

/*************************************************************************************************/
/* MAC header helpers */

/* Per-address PAN-ID presence derived from Frame Control. Implements
 * IEEE 802.15.4-2020 Table 7-2 for frame_version=0b10, and the legacy
 * rules for frame_version 0b00/0b01.
 */
struct sl_802154_pan_id_layout {
	bool dst_pan_present;
	bool src_pan_present;
};

static struct sl_802154_pan_id_layout sl_802154_pan_id_layout_get(struct ieee802154_fcf_seq *fs)
{
	struct sl_802154_pan_id_layout p = {false, false};
	enum ieee802154_addressing_mode da = fs->fc.dst_addr_mode;
	enum ieee802154_addressing_mode sa = fs->fc.src_addr_mode;
	bool pan_comp = fs->fc.pan_id_comp;
	bool dst_none = (da == IEEE802154_ADDR_MODE_NONE);
	bool src_none = (sa == IEEE802154_ADDR_MODE_NONE);

	if (fs->fc.frame_version == IEEE802154_VERSION_802154) {
		if (dst_none && src_none) {
			p.dst_pan_present = pan_comp;
		} else if (!dst_none && src_none) {
			p.dst_pan_present = !pan_comp;
		} else if (dst_none && !src_none) {
			p.src_pan_present = !pan_comp;
		} else if (da == IEEE802154_ADDR_MODE_EXTENDED &&
			   sa == IEEE802154_ADDR_MODE_EXTENDED) {
			p.dst_pan_present = !pan_comp;
		} else {
			p.dst_pan_present = true;
			p.src_pan_present = !pan_comp;
		}
	} else {
		if (pan_comp) {
			if (!dst_none && !src_none) {
				p.dst_pan_present = true;
			}
		} else {
			p.dst_pan_present = !dst_none;
			p.src_pan_present = !src_none;
		}
	}

	return p;
}

static void sl_802154_load_addr(uint8_t *buffer, size_t buffer_len, size_t *length,
				enum ieee802154_addressing_mode mode, bool pan_id_present,
				struct ieee802154_address_field **addr)
{
	size_t len = 0;

	__ASSERT_NO_MSG(length != NULL);

	if (pan_id_present) {
		len += IEEE802154_PAN_ID_LENGTH;
	}

	if (mode == IEEE802154_ADDR_MODE_SHORT) {
		len += IEEE802154_SHORT_ADDR_LENGTH;
	} else if (mode == IEEE802154_ADDR_MODE_EXTENDED) {
		len += IEEE802154_EXT_ADDR_LENGTH;
	} else {
		/* IEEE802154_ADDR_MODE_NONE or reserved: no addr bytes consumed. */
	}

	__ASSERT_NO_MSG(buffer_len >= *length + len);

	*length += len;

	/* mode == NONE may still consume a PAN ID slot (Table 7-2 row 2: dst PAN
	 * present, no dst addr); advance the cursor but expose no addr pointer.
	 */
	if (mode == IEEE802154_ADDR_MODE_NONE) {
		*addr = NULL;
	} else {
		*addr = (struct ieee802154_address_field *)buffer;
	}
}

/* ASH key id field length depends on key_id_mode; sizeof(aux_security_hdr) is wrong. */
static uint8_t sl_802154_get_aux_sec_hdr_bytes(const struct ieee802154_aux_security_hdr *ash)
{
	uint8_t len = IEEE802154_SECURITY_CF_LENGTH + IEEE802154_SECURITY_FRAME_COUNTER_LENGTH;

	switch (ash->control.key_id_mode) {
	case IEEE802154_KEY_ID_MODE_IMPLICIT:
		break;
	case IEEE802154_KEY_ID_MODE_INDEX:
		len += IEEE802154_KEY_ID_FIELD_INDEX_LENGTH;
		break;
	case IEEE802154_KEY_ID_MODE_SRC_4_INDEX:
		len += IEEE802154_KEY_ID_FIELD_SRC_4_INDEX_LENGTH;
		break;
	case IEEE802154_KEY_ID_MODE_SRC_8_INDEX:
		len += IEEE802154_KEY_ID_FIELD_SRC_8_INDEX_LENGTH;
		break;
	default:
		LOG_ERR("Unknown key id mode: %d", ash->control.key_id_mode);
		break;
	}

	return len;
}

static void sl_802154_load_mhr(uint8_t *buffer, size_t buffer_len, struct ieee802154_mhr *mhr)
{
	__ASSERT_NO_MSG(buffer_len >= sizeof(struct ieee802154_fcf_seq));

	size_t length = sizeof(struct ieee802154_fcf_seq);

	mhr->fs = (struct ieee802154_fcf_seq *)buffer;

	struct sl_802154_pan_id_layout pl = sl_802154_pan_id_layout_get(mhr->fs);

	sl_802154_load_addr(buffer + length, buffer_len, &length, mhr->fs->fc.dst_addr_mode,
			    pl.dst_pan_present, &mhr->dst_addr);
	sl_802154_load_addr(buffer + length, buffer_len, &length, mhr->fs->fc.src_addr_mode,
			    pl.src_pan_present, &mhr->src_addr);

	if (mhr->fs->fc.security_enabled) {
		mhr->aux_sec = (struct ieee802154_aux_security_hdr *)(buffer + length);
	} else {
		mhr->aux_sec = NULL;
	}
}

static size_t sl_802154_get_mhr_length(struct ieee802154_mhr *mhr)
{
	size_t length = sizeof(struct ieee802154_fcf_seq);
	enum ieee802154_addressing_mode da = mhr->fs->fc.dst_addr_mode;
	enum ieee802154_addressing_mode sa = mhr->fs->fc.src_addr_mode;
	struct sl_802154_pan_id_layout pl = sl_802154_pan_id_layout_get(mhr->fs);

	if (pl.dst_pan_present) {
		length += IEEE802154_PAN_ID_LENGTH;
	}
	if (da == IEEE802154_ADDR_MODE_SHORT) {
		length += IEEE802154_SHORT_ADDR_LENGTH;
	} else if (da == IEEE802154_ADDR_MODE_EXTENDED) {
		length += IEEE802154_EXT_ADDR_LENGTH;
	} else {
		/* NONE or reserved: no dst addr bytes. */
	}

	if (pl.src_pan_present) {
		length += IEEE802154_PAN_ID_LENGTH;
	}
	if (sa == IEEE802154_ADDR_MODE_SHORT) {
		length += IEEE802154_SHORT_ADDR_LENGTH;
	} else if (sa == IEEE802154_ADDR_MODE_EXTENDED) {
		length += IEEE802154_EXT_ADDR_LENGTH;
	} else {
		/* NONE or reserved: no src addr bytes. */
	}

	if (mhr->fs->fc.security_enabled && mhr->aux_sec != NULL) {
		length += sl_802154_get_aux_sec_hdr_bytes(mhr->aux_sec);
	}

	/* Derived from FindPayloadIndex in the openthread code base. */
	if (mhr->fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND &&
	    mhr->fs->fc.frame_version != IEEE802154_VERSION_802154) {
		length += 1;
	}

	return length;
}

/* Serialize a parsed MHR (FCF+seq, dst, src, aux-sec) into wire order.
 * The dst/src pointers must reference packed structures laid out per the same
 * Frame Control (i.e. with PAN IDs in the positions Table 7-2 dictates), as
 * produced by sl_802154_load_mhr().
 */
static uint8_t sl_802154_serialize_mhr(uint8_t *buffer, uint8_t buffer_len,
				       struct ieee802154_mhr *mhr)
{
	uint8_t length = 0;
	enum ieee802154_addressing_mode da = mhr->fs->fc.dst_addr_mode;
	enum ieee802154_addressing_mode sa = mhr->fs->fc.src_addr_mode;
	struct sl_802154_pan_id_layout pl = sl_802154_pan_id_layout_get(mhr->fs);
	uint8_t dst_size = (pl.dst_pan_present ? IEEE802154_PAN_ID_LENGTH : 0);
	uint8_t src_size = (pl.src_pan_present ? IEEE802154_PAN_ID_LENGTH : 0);
	uint8_t ash_size;

	__ASSERT_NO_MSG(buffer_len >= sizeof(struct ieee802154_fcf_seq));
	memcpy(buffer, mhr->fs, sizeof(struct ieee802154_fcf_seq));
	length += sizeof(struct ieee802154_fcf_seq);

	if (da == IEEE802154_ADDR_MODE_SHORT) {
		dst_size += IEEE802154_SHORT_ADDR_LENGTH;
	} else if (da == IEEE802154_ADDR_MODE_EXTENDED) {
		dst_size += IEEE802154_EXT_ADDR_LENGTH;
	} else {
		/* NONE or reserved: no dst addr bytes. */
	}
	if (sa == IEEE802154_ADDR_MODE_SHORT) {
		src_size += IEEE802154_SHORT_ADDR_LENGTH;
	} else if (sa == IEEE802154_ADDR_MODE_EXTENDED) {
		src_size += IEEE802154_EXT_ADDR_LENGTH;
	} else {
		/* NONE or reserved: no src addr bytes. */
	}

	if (dst_size > 0) {
		__ASSERT_NO_MSG(buffer_len >= length + dst_size);
		__ASSERT_NO_MSG(mhr->dst_addr != NULL);
		memcpy(buffer + length, mhr->dst_addr, dst_size);
		length += dst_size;
	}

	if (src_size > 0) {
		__ASSERT_NO_MSG(buffer_len >= length + src_size);
		__ASSERT_NO_MSG(mhr->src_addr != NULL);
		memcpy(buffer + length, mhr->src_addr, src_size);
		length += src_size;
	}

	if (mhr->fs->fc.security_enabled && mhr->aux_sec != NULL) {
		ash_size = sl_802154_get_aux_sec_hdr_bytes(mhr->aux_sec);

		__ASSERT_NO_MSG(buffer_len >= length + ash_size);
		memcpy(buffer + length, mhr->aux_sec, ash_size);
		length += ash_size;
	}

	return length;
}

/*************************************************************************************************/
/* Radio Security */

/* IEEE 802.15.4 security level: encryption only = 4, enc+MIC-32 = 5, etc. */
#define SEC_LEVEL_ENC 4
/* MIC sizes by security level (level 0-3 and 4-7); index is AuxHdr security_level (3 bits). */
static const uint8_t mic_size_table[] = {0, 4, 8, 16, 0, 4, 8, 16};

BUILD_ASSERT(ARRAY_SIZE(mic_size_table) == 8U, "802.15.4 security_level is 3 bits (values 0-7)");

static void sl_802154_generate_nonce(const uint8_t ext_address[IEEE802154_EXT_ADDR_LENGTH],
				     uint32_t frame_counter, uint8_t security_level,
				     uint8_t nonce[SL_802154_CCM_NONCE_BYTES])
{
	memcpy(nonce, ext_address, IEEE802154_EXT_ADDR_LENGTH);
	nonce += IEEE802154_EXT_ADDR_LENGTH;
	sys_memcpy_swap(nonce, &frame_counter, sizeof(uint32_t));
	nonce += sizeof(uint32_t);
	nonce[0] = security_level;
}

static int sl_802154_tx_ccm(uint8_t *mpdu, uint16_t mpdu_len, uint8_t header_length,
			    uint8_t security_level, const uint8_t *key, const uint8_t *nonce)
{
	uint8_t tag_length;
	uint16_t min_mpdu;
	uint16_t payload_length;
	uint8_t *payload;
	uint8_t *footer;
	sli_crypto_descriptor_t key_desc =
		SLI_CRYPTO_DESCRIPTOR_INIT_PLAINTEXT_KEY((uint8_t *)key, OT_MAC_KEY_SIZE);
	sl_status_t ret;

	if (security_level >= ARRAY_SIZE(mic_size_table)) {
		return -EINVAL;
	}

	tag_length = mic_size_table[security_level];
	min_mpdu = (uint16_t)header_length + IEEE802154_FCS_LENGTH + tag_length;

	if (mpdu_len < min_mpdu) {
		return -EINVAL;
	}

	payload_length = mpdu_len - header_length - IEEE802154_FCS_LENGTH - tag_length;
	payload = mpdu + header_length;
	footer = mpdu + mpdu_len - IEEE802154_FCS_LENGTH - tag_length;

	ret = sli_crypto_ccm_zigbee(&key_desc, true,
				    (security_level >= SEC_LEVEL_ENC) ? payload : NULL, payload,
				    (security_level >= SEC_LEVEL_ENC) ? payload_length : 0, nonce,
				    mpdu, header_length, footer, tag_length);
	return (ret == SL_STATUS_OK) ? 0 : -EIO;
}

static void sl_802154_security_init(struct sl_802154_mac_data *mac_data)
{
	memset(mac_data->sec_keys, 0, sizeof(mac_data->sec_keys));
	for (int k = 0; k < ARRAY_SIZE(mac_data->sec_keys); k++) {
		memset(&mac_data->sec_key_storage[k], 0, sizeof(mac_data->sec_key_storage[k]));
		mac_data->sec_keys[k].key_value = mac_data->sec_key_storage[k].key_value;
		mac_data->sec_keys[k].key_id = &mac_data->sec_key_storage[k].key_id;
	}
}

static int sl_802154_security_get_ack_key(struct sl_802154_data *data,
					  struct ieee802154_aux_security_hdr *aux,
					  const uint8_t **key, uint8_t *key_id)
{
	int key_slot;
	uint8_t frame_key_id;

	if (aux == NULL || aux->control.key_id_mode != IEEE802154_KEY_ID_MODE_INDEX) {
		LOG_WRN("ACK key_id missing or no matching key");
		return -EINVAL;
	}

	frame_key_id = aux->kif.mode_1.key_index;
	if (frame_key_id == 0) {
		LOG_WRN("ACK key_id missing or no matching key");
		return -EINVAL;
	}

	if (frame_key_id == data->key_id_prev) {
		key_slot = SL_802154_MAC_KEY_PREVIOUS;
		*key_id = data->key_id_prev;
	} else if (frame_key_id == data->key_id_current) {
		key_slot = SL_802154_MAC_KEY_CURRENT;
		*key_id = data->key_id_current;
	} else if (frame_key_id == data->key_id_next) {
		key_slot = SL_802154_MAC_KEY_NEXT;
		*key_id = data->key_id_next;
	} else {
		LOG_WRN("ACK key_id missing or no matching key");
		return -ENOENT;
	}

	*key = data->mac_data.sec_keys[key_slot].key_value;
	if (*key == NULL) {
		LOG_WRN("ACK key slot has no key");
		return -ENOENT;
	}
	return 0;
}

static int sl_802154_security_get_non_ack_key(struct sl_802154_data *data, const uint8_t **key,
					      uint8_t *key_id)
{
	if (data->key_id_current == 0) {
		return -EAGAIN;
	}
	*key = data->mac_data.sec_keys[SL_802154_MAC_KEY_CURRENT].key_value;
	*key_id = data->key_id_current;
	return 0;
}

static int sl_802154_security_process_tx(struct sl_802154_data *data, uint8_t *buffer, uint8_t len,
					 struct ieee802154_mhr *mhr, bool mac_hdr_rdy)
{
	struct ieee802154_aux_security_hdr *aux;
	const uint8_t *key = NULL;
	uint8_t key_id = data->key_id_current;
	uint8_t header_length;
	uint8_t security_level;
	uint8_t nonce[SL_802154_CCM_NONCE_BYTES];
	int ccm_ret;

	if (len < IEEE802154_FCF_SEQ_LENGTH) {
		return -EINVAL;
	}
	if (!mhr->fs->fc.security_enabled) {
		return 0;
	}
	/* key_id_current is 0 until IEEE802154_CONFIG_MAC_KEYS configures one. */
	if (key_id == 0) {
		LOG_WRN("TX security enabled but no key configured");
		return -EAGAIN;
	}
	if (mhr->fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK) {
		if (sl_802154_security_get_ack_key(data, mhr->aux_sec, &key, &key_id) != 0) {
			return -EAGAIN;
		}
	} else {
		if (sl_802154_security_get_non_ack_key(data, &key, &key_id) != 0) {
			LOG_WRN("TX security enabled but no key configured");
			return -EAGAIN;
		}
	}

	aux = mhr->aux_sec;
	header_length = sl_802154_get_mhr_length(mhr);
	security_level = mhr->aux_sec->control.security_level;

	if (security_level >= ARRAY_SIZE(mic_size_table)) {
		LOG_WRN("Invalid TX security level %u", (unsigned int)security_level);
		return -EINVAL;
	}

	if (!mac_hdr_rdy) {
		uint32_t fc;
		unsigned int irq_key;

		/* Atomic vs RAIL ISR (enhanced ACK) to avoid AES-CCM nonce reuse. */
		irq_key = irq_lock();
		fc = data->mac_data.frame_counter++;
		if (mhr->fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK) {
			data->mac_data.ack_fc = fc;
			data->mac_data.ack_key_id = key_id;
		}
		irq_unlock(irq_key);

		aux->frame_counter = fc;

		if (key_id != 0 &&
		    mhr->aux_sec->control.key_id_mode == IEEE802154_KEY_ID_MODE_INDEX) {
			mhr->aux_sec->kif.mode_1.key_index = key_id;
		}
	}

	sl_802154_generate_nonce(data->ext_addr, aux->frame_counter, security_level, nonce);
	ccm_ret = sl_802154_tx_ccm(buffer, len, header_length, security_level, key, nonce);
	if (ccm_ret != 0) {
		LOG_ERR("TX security (AES-CCM) failed");
		return ccm_ret;
	}
	return 0;
}

/*************************************************************************************************/
/* Radio State Machine */

#define SL_802154_SCHEDULER_CHANNEL_SLIP_TIME  (500000UL)
#define SL_802154_TIMING_CSMA_OVERHEAD_US      500
/* only used if sl_rail_get_bit_rate returns 0 */
#define SL_802154_TIMING_DEFAULT_BYTETIME_US   32
/* only used if sl_rail_get_symbol_rate returns 0 */
#define SL_802154_TIMING_DEFAULT_SYMBOLTIME_US 16
/* dBm - default for 2.4GHz 802.15.4 */
#define SL_802154_CCA_THRESHOLD_DEFAULT        -75
#define SL_802154_SHR_DURATION_US              160
#define SL_802154_SCHEDULE_TX_DELAY_US         3000

#define SL_802154_FLAG_ONGOING_TX_DATA 0
#define SL_802154_FLAG_ONGOING_TX_ACK  1
#define SL_802154_FLAG_WAITING_FOR_ACK 2

static void sl_802154_state_set_tx_ack_ongoing(atomic_t *state, bool ongoing)
{
	atomic_set_bit_to(state, SL_802154_FLAG_ONGOING_TX_ACK, ongoing);
}

static bool sl_802154_state_is_tx_ack_ongoing(atomic_t *state)
{
	return atomic_test_bit(state, SL_802154_FLAG_ONGOING_TX_ACK);
}

static void sl_802154_state_set_tx_data_ongoing(atomic_t *state, bool ongoing)
{
	atomic_set_bit_to(state, SL_802154_FLAG_ONGOING_TX_DATA, ongoing);
}

static bool sl_802154_state_is_tx_data_ongoing(atomic_t *state)
{
	return atomic_test_bit(state, SL_802154_FLAG_ONGOING_TX_DATA);
}

static void sl_802154_state_set_waiting_for_ack(atomic_t *state, bool waiting)
{
	atomic_set_bit_to(state, SL_802154_FLAG_WAITING_FOR_ACK, waiting);
}

static bool sl_802154_state_is_waiting_for_ack(atomic_t *state)
{
	return atomic_test_bit(state, SL_802154_FLAG_WAITING_FOR_ACK);
}

static void sl_802154_state_clear_tx_data_and_wait_for_ack(atomic_t *state)
{
	atomic_and(state, ~(BIT(SL_802154_FLAG_ONGOING_TX_DATA) |
				  BIT(SL_802154_FLAG_WAITING_FOR_ACK)));
}

/*************************************************************************************************/
/* RAIL Helper Functions */

/**
 * Convert a 32-bit microsecond counter (that wraps at 2^32) into a monotonic
 * 64-bit nanosecond value by counting wraps.
 *
 * @param now_us Current 32-bit usec reading from the hardware timer.
 * @return 64-bit nanoseconds = ((wrap_count << 32) | now_us) * 1000.
 *
 * @note Wrap detection requires the (read-timer, call-this-function) pair to
 *       be atomic relative to other callers. ISR callers (RAIL events) are
 *       naturally serialized; thread callers MUST hold irq_lock across both
 *       the timer read and this call (see silabs_efr32_get_time), otherwise
 *       an ISR observing a later timer value first can cause a spurious wrap.
 */
static uint64_t sl_802154_us32_to_ns64(uint32_t now_us)
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

static int sl_802154_yield_radio(sl_rail_handle_t handle)
{
	sl_rail_status_t status;

	status = sl_rail_yield_radio(handle);

	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	return 0;
}

static int sl_802154_idle_radio(sl_rail_handle_t handle)
{
	sl_rail_status_t status;

	status = sl_rail_idle(handle, SL_RAIL_IDLE, true);

	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	return 0;
}

static void sl_802154_set_radio_to_idle(sl_rail_handle_t handle)
{
	if (sl_rail_get_radio_state(handle) != SL_RAIL_RF_STATE_IDLE) {
		sl_802154_idle_radio(handle);
	}
}

/*************************************************************************************************/
/* Source match table helpers */

/* Source match table helpers. config->ack_fpb.addr is little-endian. */
static int sl_802154_src_match_short_add(struct sl_802154_source_match_data *md,
					 const uint8_t *addr_le)
{
	unsigned int key = irq_lock();
	uint16_t a = sys_get_le16(addr_le);
	int ret = 0;

	for (uint8_t i = 0; i < md->short_addr_count; i++) {
		if (md->short_addrs[i] == a) {
			goto exit; /* already present */
		}
	}
	if (md->short_addr_count >= ARRAY_SIZE(md->short_addrs)) {
		ret = -ENOMEM;
		goto exit;
	}
	md->short_addrs[md->short_addr_count++] = a;

exit:
	irq_unlock(key);
	return ret;
}

static int sl_802154_src_match_short_remove(struct sl_802154_source_match_data *md,
					    const uint8_t *addr_le)
{
	unsigned int key = irq_lock();
	uint16_t a = sys_get_le16(addr_le);
	int ret = 0;

	for (uint8_t i = 0; i < md->short_addr_count; i++) {
		if (md->short_addrs[i] == a) {
			/* Remove short_addr without shifting the whole array by index.
			 * If the removed address was not the last one, overwrite
			 * with the value that used to sit at the end. If the removed
			 * address was the last one, just decrement the count.
			 */
			md->short_addr_count--;
			if (i != md->short_addr_count) {
				md->short_addrs[i] = md->short_addrs[md->short_addr_count];
			}
			goto exit;
		}
	}
	ret = -ENOENT;

exit:
	irq_unlock(key);
	return ret;
}

static void sl_802154_src_match_short_clear(struct sl_802154_source_match_data *md)
{
	unsigned int key = irq_lock();

	md->short_addr_count = 0;
	irq_unlock(key);
}

static int sl_802154_src_match_ext_add(struct sl_802154_source_match_data *md, const uint8_t *addr)
{
	unsigned int key = irq_lock();
	int ret = 0;

	for (uint8_t i = 0; i < md->ext_addr_count; i++) {
		if (memcmp(md->ext_addrs[i], addr, IEEE802154_EXT_ADDR_LENGTH) == 0) {
			goto exit;
		}
	}
	if (md->ext_addr_count >= ARRAY_SIZE(md->ext_addrs)) {
		ret = -ENOMEM;
		goto exit;
	}
	memcpy(md->ext_addrs[md->ext_addr_count++], addr, IEEE802154_EXT_ADDR_LENGTH);

exit:
	irq_unlock(key);
	return ret;
}

static int sl_802154_src_match_ext_remove(struct sl_802154_source_match_data *md,
					  const uint8_t *addr)
{
	unsigned int key = irq_lock();
	int ret = 0;

	for (uint8_t i = 0; i < md->ext_addr_count; i++) {
		if (memcmp(md->ext_addrs[i], addr, IEEE802154_EXT_ADDR_LENGTH) == 0) {
			/* Remove ext_addr without shifting the whole array by index.
			 * If the removed address was not the last one, overwrite
			 * with the value that used to sit at the end. If the removed
			 * address was the last one, just decrement the count.
			 */
			md->ext_addr_count--;
			if (i != md->ext_addr_count) {
				memcpy(md->ext_addrs[i], md->ext_addrs[md->ext_addr_count],
				       IEEE802154_EXT_ADDR_LENGTH);
			}
			goto exit;
		}
	}
	ret = -ENOENT;

exit:
	irq_unlock(key);
	return ret;
}

static void sl_802154_src_match_ext_clear(struct sl_802154_source_match_data *md)
{
	unsigned int key = irq_lock();

	md->ext_addr_count = 0;
	irq_unlock(key);
}

/* Used from DATA_REQUEST_COMMAND handler to decide if we set FP in the outgoing ACK. */
static bool sl_802154_src_match_short_contains(struct sl_802154_source_match_data *md,
					       uint16_t short_addr)
{
	unsigned int key = irq_lock();
	bool found = false;

	for (uint8_t i = 0; i < md->short_addr_count; i++) {
		if (md->short_addrs[i] == short_addr) {
			found = true;
			break;
		}
	}
	irq_unlock(key);
	return found;
}

static bool sl_802154_src_match_ext_contains(struct sl_802154_source_match_data *md,
					     const uint8_t *addr)
{
	unsigned int key = irq_lock();
	bool found = false;

	for (uint8_t i = 0; i < md->ext_addr_count; i++) {
		if (memcmp(md->ext_addrs[i], addr, IEEE802154_EXT_ADDR_LENGTH) == 0) {
			found = true;
			break;
		}
	}
	irq_unlock(key);
	return found;
}

static bool sl_802154_src_match_contains(struct sl_802154_data *data)
{
	struct sl_802154_source_match_data *md = &data->match_data;
	sl_rail_ieee802154_address_t src_addr;

	if (md->short_addr_count == 0 && md->ext_addr_count == 0) {
		return false;
	}

	if (sl_rail_ieee802154_get_address(data->radio_data.rail_handle, &src_addr) !=
	    SL_RAIL_STATUS_NO_ERROR) {
		return false;
	}

	if (src_addr.address_length == SL_RAIL_IEEE802154_SHORT_ADDRESS) {
		return sl_802154_src_match_short_contains(md, src_addr.short_address);
	} else if (src_addr.address_length == SL_RAIL_IEEE802154_LONG_ADDRESS) {
		return sl_802154_src_match_ext_contains(md, src_addr.long_address);
	}
	/* No address configured: no match. */

	return false;
}

/*************************************************************************************************/
/* Packet Processing */

/* Peek the in-progress RX frame into data->rx_buffer and parse MHR.
 *
 * @return Positive: total packet bytes captured for peek (includes PHR).
 *         0: frame not ready or RAIL has no incoming packet info.
 *         -EINVAL: @p buffer_len is smaller than @p expected_data_bytes_max.
 */
static int sl_802154_read_initial_pkt_data(struct sl_802154_data *data,
					   sl_rail_rx_packet_info_t *pkt_info,
					   size_t expected_data_bytes_max,
					   size_t expected_data_bytes_min, size_t buffer_len)
{
	sl_rail_rx_packet_info_t adjusted_pkt_info;
	sl_rail_status_t status;

	/* buffer_len is how much of data->rx_buffer may be touched; we may copy up to
	 * expected_data_bytes_max bytes from the in-flight frame, so buffer_len must be
	 * >= expected_data_bytes_max or we would overrun the RX buffer.
	 */
	if (buffer_len < expected_data_bytes_max) {
		LOG_WRN("RX buffer too small for peek (%zu < %zu)", buffer_len,
			expected_data_bytes_max);
		return -EINVAL;
	}

	status = sl_rail_get_rx_incoming_packet_info(data->radio_data.rail_handle, pkt_info);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		/* e.g. no packet in progress; same recovery as "not enough bytes yet". */
		LOG_DBG("get_rx_incoming_packet_info failed: %d", (int)status);
		return 0;
	}

	/* We are trying to get the packet info of a packet before it is completely received.
	 * We do this to evaluate the FP bit in response and add IEs to ACK if needed.
	 * Check to see if we have received at least minimum number of bytes requested.
	 */
	if (pkt_info->packet_bytes < expected_data_bytes_min) {
		return 0;
	}

	adjusted_pkt_info = *pkt_info;

	/* Only extract what we care about */
	if (pkt_info->packet_bytes > expected_data_bytes_max) {
		adjusted_pkt_info.packet_bytes = expected_data_bytes_max;
		/* Check if the initial portion of the packet received so far exceeds the max value
		 * requested.
		 */
		if (pkt_info->first_portion_bytes >= expected_data_bytes_max) {
			/* If we have received more, make sure to copy only the required bytes into
			 * the buffer.
			 */
			adjusted_pkt_info.first_portion_bytes = expected_data_bytes_max;
			adjusted_pkt_info.p_last_portion_data = NULL;
		}
	}

	/* Copy number of bytes as indicated in `pkt_info->first_portion_bytes` into the buffer.
	 */
	status = sl_rail_copy_rx_packet(data->radio_data.rail_handle, data->rx_buffer,
				     &adjusted_pkt_info);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_WRN("sl_rail_copy_rx_packet (peek) failed: %d", (int)status);
		return -EIO;
	}
	sl_802154_load_mhr(&data->rx_buffer[SL_802154_PHR_BYTES],
			   (size_t)(adjusted_pkt_info.packet_bytes - SL_802154_PHR_BYTES),
			   &data->rx_mhr);
	return (int)adjusted_pkt_info.packet_bytes;
}

/* ------------------------------------------------------------------------------
 * Radio implementation: Enhanced ACKs
 */

/* Peek through SecHdr after DATA_REQUEST (worst-case extended src/dst + key id mode 2). */
#define SL_802154_ENH_ACK_INIT_PEEK_BYTES                                                          \
	(SL_802154_PHR_BYTES + IEEE802154_FCF_SEQ_LENGTH + IEEE802154_PAN_ID_LENGTH +              \
	 IEEE802154_EXT_ADDR_LENGTH + IEEE802154_PAN_ID_LENGTH + IEEE802154_EXT_ADDR_LENGTH +      \
	 IEEE802154_SECURITY_CF_LENGTH + IEEE802154_SECURITY_FRAME_COUNTER_LENGTH +                \
	 IEEE802154_KEY_ID_FIELD_SRC_8_INDEX_LENGTH)

/* Upper bound passed to sl_802154_read_initial_pkt_data() including optional ACK IE payload. */
#define SL_802154_ENH_ACK_INIT_PEEK_MAX_BYTES                                                      \
	(SL_802154_ENH_ACK_INIT_PEEK_BYTES + OT_ACK_IE_MAX_SIZE)

/* Compose one ACK address field (PAN+addr, addr-only, or PAN-only) into out.
 * Address bytes are sourced from rx_addr_field, whose own layout is described
 * by rx_pan_present. Returns bytes written, or 0 on a sizing/argument error.
 * out_size is the capacity of out and bounds every write made here.
 */
static size_t sl_802154_compose_ack_addr_field(uint8_t *out, size_t out_size,
					       enum ieee802154_addressing_mode mode,
					       bool ack_pan_present, uint16_t ack_pan,
					       struct ieee802154_address_field *rx_addr_field,
					       bool rx_pan_present)
{
	const struct ieee802154_address *src;
	size_t offset = 0;

	if (ack_pan_present) {
		if (out_size < IEEE802154_PAN_ID_LENGTH) {
			return 0;
		}
		sys_put_le16(ack_pan, out);
		offset += IEEE802154_PAN_ID_LENGTH;
	}

	if (mode == IEEE802154_ADDR_MODE_NONE || rx_addr_field == NULL) {
		return offset;
	}

	src = rx_pan_present ? &rx_addr_field->plain.addr : &rx_addr_field->comp.addr;

	if (mode == IEEE802154_ADDR_MODE_SHORT) {
		if (out_size - offset < IEEE802154_SHORT_ADDR_LENGTH) {
			return 0;
		}
		memcpy(out + offset, &src->short_addr, IEEE802154_SHORT_ADDR_LENGTH);
		offset += IEEE802154_SHORT_ADDR_LENGTH;
	} else if (mode == IEEE802154_ADDR_MODE_EXTENDED) {
		if (out_size - offset < IEEE802154_EXT_ADDR_LENGTH) {
			return 0;
		}
		memcpy(out + offset, src->ext_addr, IEEE802154_EXT_ADDR_LENGTH);
		offset += IEEE802154_EXT_ADDR_LENGTH;
	} else {
		/* Reserved addressing mode: nothing to copy. */
	}

	return offset;
}

static uint8_t sl_802154_construct_enh_ack_mhr(struct sl_802154_data *data, bool ie_present)
{
	/* construct data->enh_ack_mhr */
	struct ieee802154_fcf_seq enh_ack_fs = {
		.fc = {
			.frame_type = IEEE802154_FRAME_TYPE_ACK,
			.security_enabled = data->rx_mhr.fs->fc.security_enabled,
			.frame_pending = data->mac_data.ack_fpb,
			.pan_id_comp = data->rx_mhr.fs->fc.pan_id_comp,
			.seq_num_suppr = data->rx_mhr.fs->fc.seq_num_suppr,
			.ie_list = ie_present,
			.dst_addr_mode = data->rx_mhr.fs->fc.src_addr_mode,
			.frame_version = IEEE802154_VERSION_802154,
			.src_addr_mode = data->rx_mhr.fs->fc.dst_addr_mode,
		},
		.sequence = data->rx_mhr.fs->sequence,
	};
	struct ieee802154_aux_security_hdr enh_sec_hdr = {};
	struct ieee802154_mhr tmp_mhr = {0};
	struct sl_802154_pan_id_layout ack_pl;
	struct sl_802154_pan_id_layout rx_pl;
	uint16_t ack_dst_pan;
	uint16_t ack_src_pan;
	uint8_t ack_dst_field[IEEE802154_PAN_ID_LENGTH + IEEE802154_EXT_ADDR_LENGTH];
	uint8_t ack_src_field[IEEE802154_PAN_ID_LENGTH + IEEE802154_EXT_ADDR_LENGTH];
	uint8_t mhr_length;

	if (data->rx_mhr.fs->fc.security_enabled) {
		enh_sec_hdr.control = (struct ieee802154_security_control_field){
			.security_level = data->rx_mhr.aux_sec->control.security_level,
			.key_id_mode = data->rx_mhr.aux_sec->control.key_id_mode,
		};
		/* Will set later in sl_802154_security_process_tx */
		enh_sec_hdr.frame_counter = 0;

		enh_sec_hdr.kif = (struct ieee802154_key_identifier_field){0};
	}

	tmp_mhr.fs = &enh_ack_fs;

	/* Build ACK address fields in local buffers rather than aliasing rx
	 * pointers: per Table 7-2 the rx and ACK PAN-ID layouts can differ
	 * even when address modes are swapped and pan_id_comp is preserved.
	 * PAN ID lookups assume single-PAN networks (true for Thread).
	 */
	ack_pl = sl_802154_pan_id_layout_get(&enh_ack_fs);
	rx_pl = sl_802154_pan_id_layout_get(data->rx_mhr.fs);
	ack_dst_pan = data->pan_id;
	ack_src_pan = data->pan_id;

	if (data->rx_mhr.src_addr != NULL && rx_pl.src_pan_present) {
		ack_dst_pan = data->rx_mhr.src_addr->plain.pan_id;
	} else if (data->rx_mhr.dst_addr != NULL && rx_pl.dst_pan_present) {
		ack_dst_pan = data->rx_mhr.dst_addr->plain.pan_id;
	} else {
		/* No PAN ID present in rx; fall back to local pan_id. */
	}
	if (data->rx_mhr.dst_addr != NULL && rx_pl.dst_pan_present) {
		ack_src_pan = data->rx_mhr.dst_addr->plain.pan_id;
	}

	(void)sl_802154_compose_ack_addr_field(ack_dst_field, sizeof(ack_dst_field),
		enh_ack_fs.fc.dst_addr_mode, ack_pl.dst_pan_present, ack_dst_pan,
		data->rx_mhr.src_addr, rx_pl.src_pan_present);
	(void)sl_802154_compose_ack_addr_field(ack_src_field, sizeof(ack_src_field),
		enh_ack_fs.fc.src_addr_mode, ack_pl.src_pan_present, ack_src_pan,
		data->rx_mhr.dst_addr, rx_pl.dst_pan_present);

	tmp_mhr.dst_addr = (struct ieee802154_address_field *)ack_dst_field;
	tmp_mhr.src_addr = (struct ieee802154_address_field *)ack_src_field;
	if (data->rx_mhr.fs->fc.security_enabled) {
		tmp_mhr.aux_sec = &enh_sec_hdr;
	} else {
		tmp_mhr.aux_sec = NULL;
	}

	mhr_length = sl_802154_serialize_mhr(data->enh_ack_buffer + SL_802154_PHR_BYTES,
					     sizeof(data->enh_ack_buffer) - SL_802154_PHR_BYTES,
					     &tmp_mhr);

	sl_802154_load_mhr(data->enh_ack_buffer + SL_802154_PHR_BYTES,
			   sizeof(data->enh_ack_buffer) - SL_802154_PHR_BYTES,
			   &data->enh_ack_mhr);

	return mhr_length;
}

/* Compute whether the rx src address matches the configured ACK IE filter and,
 * when soft source-matching is enabled, update data->mac_data.ack_fpb from the
 * source-match table. Returns false (no match) when no src address is present.
 */
static bool sl_802154_match_ack_src_addr(struct sl_802154_data *data)
{
	struct sl_802154_pan_id_layout pl;
	struct ieee802154_address *src_addr;
	uint16_t short_addr;

	if (data->rx_mhr.src_addr == NULL) {
		return false;
	}

	pl = sl_802154_pan_id_layout_get(data->rx_mhr.fs);
	src_addr = pl.src_pan_present ? &data->rx_mhr.src_addr->plain.addr
				      : &data->rx_mhr.src_addr->comp.addr;

	if (data->rx_mhr.fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		/* Frame addresses are little endian (see ieee802154_mac.h) */
		short_addr = sys_get_le16((uint8_t *)&src_addr->short_addr);

		if (data->is_src_match_enabled) {
			data->mac_data.ack_fpb = sl_802154_src_match_short_contains(
				&data->match_data, short_addr);
		}
		return data->ack_ie.short_addr < IEEE802154_NO_SHORT_ADDRESS_ASSIGNED &&
		       data->ack_ie.short_addr == short_addr;
	}

	if (data->is_src_match_enabled) {
		data->mac_data.ack_fpb = sl_802154_src_match_ext_contains(
			&data->match_data, src_addr->ext_addr);
	}
	return memcmp(data->ack_ie.ext_addr, src_addr->ext_addr,
		      IEEE802154_EXT_ADDR_LENGTH) == 0;
}

/* Return 0 if the caller should continue with immediate ACK / frame-pending logic.
 * Return non-zero if this handler path is finished; the caller should then return
 * the status stored in @p write_enh_ack_status (from sl_rail_ieee802154_write_enh_ack when
 * that API runs).
 * @param[out] write_enh_ack_status Result of sl_rail_ieee802154_write_enh_ack when that API
 *             runs; otherwise SL_RAIL_STATUS_NO_ERROR (including all early-return paths).
 */
static int sl_802154_write_enhanced_ack(struct sl_802154_data *data,
			      sl_rail_rx_packet_info_t *pkt_info_for_enh_ack, uint32_t rx_timestamp,
			      size_t *initial_pkt_read_bytes,
			      sl_rail_status_t *write_enh_ack_status)
{
	uint8_t enh_ack_mhr_length;
	uint8_t enh_ack_len;
	bool ie_present;
	bool match;
	int peek_rc;

	/* RAIL will generate an Immediate ACK for us.
	 * For an Enhanced ACK, we need to generate the whole packet ourselves.
	 */

	/* An 802.15.4 packet from RAIL should look like:
	 * 1/2 |   1/2  | 0/1  |  0/2   | 0/2/8  |  0/2   | 0/2/8  |   14
	 * PHR | MacFCF | Seq# | DstPan | DstAdr | SrcPan | SrcAdr | SecHdr
	 */

	/* With sl_rail_ieee802154_enable_early_frame_pending(),
	 * SL_RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND is triggered after receiving through the
	 * SrcAdr field of Version 0/1 packets, and after receiving through the SecHdr for Version 2
	 * packets.
	 */
	*write_enh_ack_status = SL_RAIL_STATUS_NO_ERROR;

	peek_rc = sl_802154_read_initial_pkt_data(data, pkt_info_for_enh_ack,
		(size_t)SL_802154_ENH_ACK_INIT_PEEK_BYTES,
		(size_t)SL_802154_PHR_BYTES + (size_t)IEEE802154_FCF_SEQ_LENGTH,
		(size_t)SL_802154_ENH_ACK_INIT_PEEK_MAX_BYTES);
	if (peek_rc > 0) {
		*initial_pkt_read_bytes = (size_t)peek_rc;
	} else {
		*initial_pkt_read_bytes = 0U;
	}
	if (*initial_pkt_read_bytes == 0U) {
		/* Nothing to read, which means generating an immediate ACK is also pointless */
		return 1;
	}

	if (data->rx_mhr.fs->fc.frame_version != IEEE802154_VERSION_802154) {
		return 0;
	}

	data->mac_data.ack_fpb = false;
	ie_present = false;
	match = sl_802154_match_ack_src_addr(data);

	if (data->is_src_match_enabled && !match) {
		return 1; /* requested ack IE address does not match */
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)) {
		ie_present = match && (data->ack_ie.link_metrics_header_ie.length > 0);
	}

	enh_ack_mhr_length = sl_802154_construct_enh_ack_mhr(data, ie_present);
	enh_ack_len = SL_802154_PHR_BYTES + enh_ack_mhr_length;

	if (IS_ENABLED(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT) && ie_present) {
		/* append Link Metrics IE header to payload */
		size_t ie_total = IEEE802154_HEADER_IE_HEADER_LENGTH +
				  data->ack_ie.link_metrics_header_ie.length;

		memcpy(data->enh_ack_buffer + enh_ack_len,
		       &data->ack_ie.link_metrics_header_ie, ie_total);
		enh_ack_len += (uint8_t)ie_total;
	}

	data->mac_data.ack_seb = data->enh_ack_mhr.fs->fc.security_enabled;

	if (data->mac_data.ack_seb) {
		uint8_t mic_len =
			mic_size_table[data->enh_ack_mhr.aux_sec->control.security_level];
		size_t mpdu_len = (size_t)enh_ack_len - SL_802154_PHR_BYTES + mic_len +
				  IEEE802154_FCS_LENGTH;

		if (sl_802154_security_process_tx(data,
						  data->enh_ack_buffer + SL_802154_PHR_BYTES,
						  (uint8_t)mpdu_len, &data->enh_ack_mhr,
						  false) < 0) {
			/* Skip writing a malformed ACK; peer will retransmit. */
			return 1;
		}

		enh_ack_len += mic_len;
	}

	data->enh_ack_buffer[0] =
		(uint8_t)(enh_ack_len - SL_802154_PHR_BYTES + IEEE802154_FCS_LENGTH);
	*write_enh_ack_status = sl_rail_ieee802154_write_enh_ack(data->radio_data.rail_handle,
								 data->enh_ack_buffer, enh_ack_len);

	return 1;
}

/* DATA_REQUEST RAIL hook: enhanced vs immediate ACK and frame-pending.
 *
 * Return value:
 * - When the enhanced-ACK path finishes early: result of sl_rail_ieee802154_write_enh_ack
 *   (SL_RAIL_STATUS_NO_ERROR if that API was not reached or succeeded).
 * - Otherwise: result of sl_rail_ieee802154_toggle_frame_pending when invoked, else
 *   SL_RAIL_STATUS_NO_ERROR.
 */
static sl_rail_status_t sl_802154_handle_data_request_command(struct sl_802154_data *data)
{
	const size_t pkt_offset = SL_802154_PHR_BYTES;
	const size_t max_expected_bytes =
		(size_t)SL_802154_PHR_BYTES + (size_t)IEEE802154_FCF_SEQ_LENGTH;
	__maybe_unused uint32_t timestamp = sl_rail_get_time(data->radio_data.rail_handle);
	sl_rail_rx_packet_info_t pkt_info;
	sl_rail_status_t status = SL_RAIL_STATUS_NO_ERROR;

	/* This callback occurs after the address fields of an incoming
	 * ACK-requesting CMD or DATA frame have been received and we
	 * can do a frame pending check.  We must also figure out what
	 * kind of ACK is being requested -- Immediate or Enhanced.
	 */

	size_t initial_pkt_read_bytes = 0U;
	sl_rail_status_t enh_ack_write_status = SL_RAIL_STATUS_NO_ERROR;

	if (!IS_ENABLED(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)) {
		if (sl_802154_write_enhanced_ack(data, &pkt_info, timestamp,
						 &initial_pkt_read_bytes,
						 &enh_ack_write_status) != 0) {
			if (enh_ack_write_status != SL_RAIL_STATUS_NO_ERROR) {
				LOG_WRN("sl_rail_ieee802154_write_enh_ack failed: %d",
					(int)enh_ack_write_status);
			}
			return enh_ack_write_status;
		}
	} else {
		int peek_rc = sl_802154_read_initial_pkt_data(data, &pkt_info, max_expected_bytes,
					      pkt_offset + (size_t)IEEE802154_FCF_SEQ_LENGTH,
					      max_expected_bytes);

		if (peek_rc > 0) {
			initial_pkt_read_bytes = (size_t)peek_rc;
		} else {
			initial_pkt_read_bytes = 0U;
		}
	}

	/* Calculate frame pending for immediate-ACK
	 * If not, RAIL will send an immediate ACK, but we need to do FP lookup.
	 */
	data->mac_data.ack_fpb = false;

	if (data->is_src_match_enabled) {
		if (sl_802154_src_match_contains(data)) {
			status = sl_rail_ieee802154_toggle_frame_pending(
				data->radio_data.rail_handle);
			if (status != SL_RAIL_STATUS_NO_ERROR) {
				LOG_WRN("IEEE802154: toggle_frame_pending failed (%d)",
					(int)status);
			}
			data->mac_data.ack_fpb = true;
		}
	} else if ((data->rx_mhr.fs->fc.frame_type != IEEE802154_FRAME_TYPE_DATA) ||
		   (initial_pkt_read_bytes <= pkt_offset)) {
		status = sl_rail_ieee802154_toggle_frame_pending(data->radio_data.rail_handle);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_WRN("IEEE802154: toggle_frame_pending failed (%d)", (int)status);
		}
		data->mac_data.ack_fpb = true;
	} else {
		/* DATA frame with payload and no soft source match: leave ack_fpb=false. */
	}

	if (status == SL_RAIL_STATUS_INVALID_STATE) {
		/* Typical race: ACK already started; other errors are returned without WRN. */
		LOG_WRN("IEEE802154: too late to modify outgoing frame-pending bit");
	}

	return status;
}

/*************************************************************************************************/
/* Radio Event Processing */

static void sl_802154_handle_tx_failed(struct sl_802154_data *data)
{
	sl_802154_state_clear_tx_data_and_wait_for_ack(&data->radio_data.state);
	sl_802154_yield_radio(data->radio_data.rail_handle);
}

static void sl_802154_handle_ack_timeout(struct sl_802154_data *data)
{
	__ASSERT_NO_MSG(sl_802154_state_is_tx_data_ongoing(&data->radio_data.state));
	__ASSERT_NO_MSG(sl_802154_state_is_waiting_for_ack(&data->radio_data.state));
	sl_802154_state_set_tx_data_ongoing(&data->radio_data.state, false);
	sl_802154_state_set_waiting_for_ack(&data->radio_data.state, false);
	data->ack_errno = -ENOMSG;
	k_sem_give(&data->ack_wait);
	sl_802154_yield_radio(data->radio_data.rail_handle);
	data->mac_data.em_pending_data = false;
}

static void sl_802154_handle_rx_failed(const struct device *dev,
				       enum ieee802154_rx_fail_reason reason)
{
	struct sl_802154_data *data = dev->data;

	if (data->event_handler != NULL) {
		data->event_handler(dev, IEEE802154_EVENT_RX_FAILED, (void *)&reason);
	}
}

static bool sl_802154_validate_phr(const sl_rail_rx_packet_info_t *pkt_info, uint16_t pkt_length)
{
	uint8_t length_byte;

	if (SL_802154_PHR_BYTES == 1) {
		return pkt_length == pkt_info->p_first_portion_data[0];
	}

	if (pkt_info->first_portion_bytes > 1) {
		length_byte = pkt_info->p_first_portion_data[1];
	} else {
		length_byte = pkt_info->p_last_portion_data[0];
	}
	return pkt_length == (uint16_t)(__RBIT(length_byte) >> 24);
}

static bool sl_802154_is_valid_pkt(struct sl_802154_data *data, sl_rail_rx_packet_info_t *pkt_info,
				   sl_rail_rx_packet_details_t *pkt_details, uint16_t pkt_length)
{
	sl_rail_status_t status;

	if (pkt_length > IEEE802154_MAX_PHY_PACKET_SIZE || pkt_length < 4) {
		return false;
	}
	if (pkt_info->first_portion_bytes < SL_802154_PHR_BYTES) {
		return false;
	}
	if (!sl_802154_validate_phr(pkt_info, pkt_length)) {
		return false;
	}

	/* skip packet length bytes */
	if (pkt_info->packet_bytes < SL_802154_PHR_BYTES) {
		return false;
	}
	pkt_info->packet_bytes -= SL_802154_PHR_BYTES;

	if (pkt_info->first_portion_bytes >= SL_802154_PHR_BYTES) {
		pkt_info->p_first_portion_data += SL_802154_PHR_BYTES;
		pkt_info->first_portion_bytes -= SL_802154_PHR_BYTES;
	} else {
		if (pkt_info->p_last_portion_data == NULL) {
			return false;
		}
		pkt_info->p_last_portion_data +=
			SL_802154_PHR_BYTES - pkt_info->first_portion_bytes;
		pkt_info->first_portion_bytes = 0;
	}

	/* Get the timestamp when the SFD was received */
	if (pkt_details->time_received.time_position == SL_RAIL_PACKET_TIME_INVALID) {
		return false;
	}

	/* + PHY HEADER SIZE for PHY header
	 * We would not need this if PHR is not included and we want the MHR
	 */
	pkt_details->time_received.total_packet_bytes = pkt_length + SL_802154_PHR_BYTES;
	status = sl_rail_get_rx_time_sync_word_end(data->radio_data.rail_handle, pkt_details);

	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return false;
	}

	return true;
}

static void sl_802154_handle_rx_ack(const struct device *dev, uint16_t length,
				    sl_rail_rx_packet_details_t *pkt_details)
{
	struct sl_802154_data *data = dev->data;
	struct net_pkt *ack_pkt = NULL;
	bool seq_match;
	bool valid_ack;
	bool tx_is_data_request =
		(data->tx_mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND);

	/* Skip the sequence compare when either side has seq_num_suppr set:
	 * fs->sequence is then not a wire byte we can trust. Thread does
	 * not currently use suppression, so this is future-proofing.
	 */
	seq_match = data->rx_mhr.fs->fc.seq_num_suppr || data->tx_mhr.fs->fc.seq_num_suppr ||
		    (data->rx_mhr.fs->sequence == data->tx_mhr.fs->sequence);
	valid_ack = data->rx_mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK &&
		    data->tx_mhr.fs->fc.ar && seq_match;

	if (!valid_ack) {
		goto invalid_ack;
	}

	/* allocate ack packet */
	ack_pkt = net_pkt_rx_alloc_with_buffer(data->iface, length, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		sl_802154_handle_rx_failed(dev, IEEE802154_RX_FAIL_OTHER);
		goto ack_alloc_fail;
	}

	/* update packet data */
	if (net_pkt_write(ack_pkt, data->rx_buffer, length) < 0) {
		LOG_ERR("Failed to write to a packet.");
		goto ack_write_fail;
	}

	/* update RSSI and LQI */
	net_pkt_set_ieee802154_lqi(ack_pkt, pkt_details->lqi);
	net_pkt_set_ieee802154_rssi_dbm(ack_pkt, pkt_details->rssi_dbm);
	/* set timestamp of the ACK packet */
	net_pkt_set_timestamp_ns(ack_pkt,
				 sl_802154_us32_to_ns64(pkt_details->time_received.packet_time));

	net_pkt_cursor_init(ack_pkt);

	/* handle ack */
	if (ieee802154_handle_ack(data->iface, ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled - releasing.");
	}

ack_write_fail:
	net_pkt_unref(ack_pkt);
ack_alloc_fail:
	data->ack_errno = 0;
	k_sem_give(&data->ack_wait);
	sl_802154_state_clear_tx_data_and_wait_for_ack(&data->radio_data.state);
	if (tx_is_data_request && data->rx_mhr.fs->fc.frame_pending) {
		data->mac_data.em_pending_data = true;
	}
invalid_ack:
	if (!tx_is_data_request) {
		sl_802154_yield_radio(data->radio_data.rail_handle);
	}
}

static void sl_802154_handle_rx_data(const struct device *dev, uint16_t length,
				     sl_rail_rx_packet_details_t *pkt_details)
{
	struct sl_802154_data *data = dev->data;
	struct net_pkt *pkt;
	int net_status;

	if (!data->promiscuous && length < 4) {
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, length, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No pkt available");
		sl_802154_handle_rx_failed(dev, IEEE802154_RX_FAIL_OTHER);
		return;
	}
	if (net_pkt_write(pkt, data->rx_buffer, length) < 0) {
		LOG_ERR("Failed to write to a packet.");
		sl_802154_handle_rx_failed(dev, IEEE802154_RX_FAIL_OTHER);
		net_pkt_unref(pkt);
		return;
	}
	net_pkt_set_ieee802154_lqi(pkt, pkt_details->lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, pkt_details->rssi_dbm);
	net_pkt_set_ieee802154_ack_seb(pkt, data->mac_data.ack_seb);
	net_pkt_set_ieee802154_ack_fpb(pkt, data->mac_data.ack_fpb);
	net_pkt_set_ieee802154_ack_fc(pkt, data->mac_data.ack_fc);
	net_pkt_set_ieee802154_ack_keyid(pkt, data->mac_data.ack_key_id);
	net_pkt_set_timestamp_ns(pkt,
				 sl_802154_us32_to_ns64(pkt_details->time_received.packet_time));
	net_pkt_cursor_init(pkt);

	net_status = net_recv_data(data->iface, pkt);

	if (net_status < 0) {
		LOG_ERR("RCV Packet dropped by NET stack: %d", net_status);
		net_pkt_unref(pkt);
		return;
	}

	if (data->rx_mhr.fs->fc.ar) {
		sl_802154_state_set_tx_ack_ongoing(&data->radio_data.state, true);
	} else if (data->mac_data.em_pending_data) {
		/* We received a frame that does not require an ACK as result of a data
		 * poll: we yield the radio here.
		 */
		(void)sl_802154_yield_radio(data->radio_data.rail_handle);
		data->mac_data.em_pending_data = false;
	} else {
		/* Plain unacknowledged frame outside a data poll: nothing to do. */
	}
}

/**
 * Called from RAIL events callback (ISR context) when RX_PACKET_RECEIVED fires.
 * We don't tell RAIL to hold packets for processing outside of ISR context
 * so must copy here.
 */
static void sl_802154_handle_rx_pkt(const struct device *dev)
{
	struct sl_802154_data *data = dev->data;
	sl_rail_status_t status;
	sl_rail_rx_packet_info_t pkt_info;
	sl_rail_rx_packet_details_t pkt_details;
	sl_rail_rx_packet_handle_t pkt_handle;
	uint16_t pkt_length;

	pkt_handle = sl_rail_get_rx_packet_info(data->radio_data.rail_handle,
						SL_RAIL_RX_PACKET_HANDLE_NEWEST, &pkt_info);
	if (pkt_handle == SL_RAIL_RX_PACKET_HANDLE_INVALID ||
	    pkt_info.packet_status != SL_RAIL_RX_PACKET_READY_SUCCESS) {
		return;
	}

	status = sl_rail_get_rx_packet_details(data->radio_data.rail_handle, pkt_handle,
					       &pkt_details);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_WRN("RX: sl_rail_get_rx_packet_details failed (%d)", (int)status);
		return;
	}

	/**
	 * RAIL's packet_bytes includes the (1 or 2 byte) PHY header. The 2-byte FCS is
	 * included only when SL_RAIL_RX_OPTION_STORE_CRC is enabled (gated by
	 * CONFIG_IEEE802154_L2_PKT_INCL_FCS). pkt_length must equal the PSDU
	 * (MPDU + FCS) for sl_802154_is_valid_pkt() to validate against the PHR byte.
	 */
	pkt_length = pkt_info.packet_bytes - SL_802154_PHR_BYTES;
	if (!IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		pkt_length += IEEE802154_FCS_LENGTH;
	}

	if (!sl_802154_is_valid_pkt(data, &pkt_info, &pkt_details, pkt_length)) {
		return;
	}

	status = sl_rail_copy_rx_packet(data->radio_data.rail_handle, data->rx_buffer, &pkt_info);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_WRN("RX: sl_rail_copy_rx_packet failed (%d)", (int)status);
		return;
	}

	sl_802154_load_mhr(data->rx_buffer, pkt_info.packet_bytes, &data->rx_mhr);

	/* sl_802154_is_valid_pkt() stripped the PHR from pkt_info.packet_bytes, so it
	 * now matches the bytes copied into rx_buffer (MPDU, plus FCS when
	 * SL_RAIL_RX_OPTION_STORE_CRC is enabled).
	 */
	uint16_t l2_length = (uint16_t)pkt_info.packet_bytes;

	if (pkt_details.is_ack) {
		sl_802154_handle_rx_ack(dev, l2_length, &pkt_details);
	} else {
		sl_802154_handle_rx_data(dev, l2_length, &pkt_details);
	}
}

static void sl_802154_handle_scheduler_event(struct sl_802154_data *data)
{
	sl_rail_scheduler_status_t scheduler_status;
	sl_rail_status_t rail_status;

	sl_rail_get_scheduler_status(data->radio_data.rail_handle, &scheduler_status, &rail_status);
	if (scheduler_status == SL_RAIL_SCHEDULER_STATUS_NO_ERROR) {
		return;
	}

	if (sl_802154_state_is_tx_ack_ongoing(&data->radio_data.state)) {
		sl_802154_state_set_tx_ack_ongoing(&data->radio_data.state, false);
	}
	/* We were in the process of TXing a data frame, treat it as a CCA_FAIL. */
	if (sl_802154_state_is_tx_data_ongoing(&data->radio_data.state)) {
		bool was_waiting_for_ack =
			sl_802154_state_is_waiting_for_ack(&data->radio_data.state);

		data->tx_errno = -EBUSY;
		k_sem_give(&data->tx_wait);

		if (was_waiting_for_ack) {
			sl_802154_handle_ack_timeout(data);
		} else {
			sl_802154_handle_tx_failed(data);
		}
	}
}

static void sl_802154_handle_tx_completion(struct sl_802154_data *data, sl_rail_events_t events)
{
	if (events & SL_RAIL_EVENT_TX_PACKET_SENT) {
		if (sl_802154_state_is_tx_data_ongoing(&data->radio_data.state)) {
			if (data->tx_mhr.fs->fc.ar) {
				sl_802154_state_set_waiting_for_ack(&data->radio_data.state, true);
			} else {
				sl_802154_state_set_tx_data_ongoing(&data->radio_data.state, false);
				(void)sl_802154_yield_radio(data->radio_data.rail_handle);
			}
			data->tx_errno = 0;
		}
	}
	if (events & SL_RAIL_EVENT_TX_CHANNEL_BUSY) {
		data->tx_errno = -EBUSY;
		sl_802154_handle_tx_failed(data);
	}
	if (events & SL_RAIL_EVENT_TX_BLOCKED) {
		data->tx_errno = -EIO;
		sl_802154_handle_tx_failed(data);
	}
	if (events & (SL_RAIL_EVENT_TX_UNDERFLOW | SL_RAIL_EVENT_TX_ABORTED)) {
		data->tx_errno = -EIO;
		sl_802154_handle_tx_failed(data);
	}
	/* STARTED takes precedence: a scheduled TX that started should not be reported as missed
	 * even if RAIL coalesces both bits in the same callback.
	 */
	if ((events & SL_RAIL_EVENT_TX_SCHEDULED_TX_MISSED) &&
	    !(events & SL_RAIL_EVENT_TX_SCHEDULED_TX_STARTED)) {
		data->tx_errno = -EIO;
		sl_802154_handle_tx_failed(data);
	}
	k_sem_give(&data->tx_wait);
}

/* RAIL events callback (runs in ISR). Minimal work: set TX result + give sem; schedule RX work. */
static void sl_802154_rail_events_callback(sl_rail_handle_t handle, sl_rail_events_t events)
{
	const sl_rail_config_t *rail_config = sl_rail_get_config(handle);
	const struct device *dev = rail_config->p_opaque_handle_0;
	struct sl_802154_data *data = dev->data;

	/* Process data request command events */
	if (events & SL_RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND) {
		(void)sl_802154_handle_data_request_command(data);
	}

	/* Process TX events */
	if (events & SL_RAIL_EVENTS_TX_COMPLETION) {
		sl_802154_handle_tx_completion(data, events);
	}

	/* Process scheduled events for Thread 1.2+ */
	if (!IS_ENABLED(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)) {
		if (events & SL_RAIL_EVENT_RX_SCHEDULED_RX_STARTED) {
			/* do nothing */
		}

		if (events & SL_RAIL_EVENT_RX_SCHEDULED_RX_END ||
		    events & SL_RAIL_EVENT_RX_SCHEDULED_RX_MISSED) {
			if (data->event_handler != NULL) {
				data->event_handler(dev, IEEE802154_EVENT_RX_OFF, NULL);
			}
			sl_802154_set_radio_to_idle(data->radio_data.rail_handle);
		}
	}

	/* Process RX packet received events */
	if (events & SL_RAIL_EVENT_RX_PACKET_RECEIVED) {
		sl_802154_handle_rx_pkt(dev);
	}

	/* Process ACK events */
	if (events & SL_RAIL_EVENT_TXACK_PACKET_SENT) {
		sl_802154_state_set_tx_ack_ongoing(&data->radio_data.state, false);
		/* We successfully sent out an ACK.
		 * We acked the packet we received after a poll: we can yield now.
		 */
		if (data->mac_data.em_pending_data) {
			(void)sl_802154_yield_radio(data->radio_data.rail_handle);
			data->mac_data.em_pending_data = false;
		}
	}

	if (events & (SL_RAIL_EVENT_TXACK_ABORTED | SL_RAIL_EVENT_TXACK_UNDERFLOW)) {
		sl_802154_state_set_tx_ack_ongoing(&data->radio_data.state, false);
	}

	if (events & SL_RAIL_EVENT_TXACK_BLOCKED) {
		/* Do nothing */
	}

	/* Deal with ACK timeout after possible RX completion in case RAIL
	 * notifies us of the ACK and the timeout simultaneously -- we want
	 * the ACK to win over the timeout.
	 */
	if ((events & SL_RAIL_EVENT_RX_ACK_TIMEOUT) &&
	    sl_802154_state_is_waiting_for_ack(&data->radio_data.state)) {
		sl_802154_handle_ack_timeout(data);
	}

	/* Process scheduler events */
	if (events & SL_RAIL_EVENT_SCHEDULER_STATUS) {
		sl_802154_handle_scheduler_event(data);
	}

	if (events & SL_RAIL_EVENT_CAL_NEEDED) {
		(void)sl_rail_calibrate(data->radio_data.rail_handle, NULL,
					SL_RAIL_CAL_ALL_PENDING);
	}
}

/*************************************************************************************************/

static int silabs_efr32_set_txpower(const struct device *dev, int16_t dbm);

static int sl_802154_rail_init(const struct device *dev)
{
	struct sl_802154_data *data = dev->data;
	sl_rail_status_t status;
	sl_rail_handle_t handle = SL_RAIL_EFR32_HANDLE;
	sl_rail_config_t rail_config = {
		/* RAIL opaque slot is void *; round-trip via uintptr_t preserves const address. */
		.p_opaque_handle_0 = UINT_TO_POINTER(POINTER_TO_UINT((const void *)dev)),
		.p_opaque_handle_1 = data,
		.events_callback = sl_802154_rail_events_callback,
		.rx_packet_queue_entries = sl_rail_builtin_rx_packet_queue_entries,
		.p_rx_packet_queue = sl_rail_builtin_rx_packet_queue_ptr,
		.rx_fifo_bytes = sl_rail_builtin_rx_fifo_bytes,
		.p_rx_fifo_buffer = sl_rail_builtin_rx_fifo_ptr,
		.tx_fifo_bytes = sizeof(data->radio_data.rail_tx_fifo),
		.p_tx_fifo_buffer = data->radio_data.rail_tx_fifo,
	};

	status = sl_rail_init(&handle, &rail_config, NULL);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_init failed: %d", (int)status);
		return -EIO;
	}
	if (IS_ENABLED(CONFIG_PM)) {
		status = sl_rail_init_power_manager();
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_ERR("sl_rail_init_power_manager failed: %d", (int)status);
			return -EIO;
		}
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
	status = sl_rail_ieee802154_init(handle, &data->radio_data.rail_ieee802154_config);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_ieee802154_init failed: %d", (int)status);
		return -EIO;
	}

	/* Initialize security key storage pointers regardless of Thread version. */
	sl_802154_security_init(&data->mac_data);

	if (!IS_ENABLED(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)) {
		status = sl_rail_ieee802154_enable_early_frame_pending(handle, true);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_ERR("sl_rail_ieee802154_enable_early_frame_pending failed: %d",
				(int)status);
			return -EIO;
		}
		status = sl_rail_ieee802154_enable_data_frame_pending(handle, true);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_ERR("sl_rail_ieee802154_enable_data_frame_pending failed: %d",
				(int)status);
			return -EIO;
		}
	}

	/* Enable RAIL multi-timer */
	sl_rail_config_multi_timer(handle, true);

	data->radio_data.rail_handle = handle;

	return 0;
}

static int sl_802154_rail_config(const struct device *dev)
{
	struct sl_802154_data *data = dev->data;
	sl_rail_status_t status;
	int ret;
	sl_rail_events_t rail_events;

	sl_rail_util_pa_init();

	rail_events = (SL_RAIL_EVENTS_NONE | SL_RAIL_EVENT_RX_ACK_TIMEOUT |
		       SL_RAIL_EVENT_RX_PACKET_RECEIVED | SL_RAIL_EVENTS_TXACK_COMPLETION |
		       SL_RAIL_EVENTS_TX_COMPLETION |
		       SL_RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND | SL_RAIL_EVENT_CAL_NEEDED);

	if (!IS_ENABLED(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)) {
		rail_events |=
			(SL_RAIL_EVENT_TX_SCHEDULED_TX_STARTED |
			 SL_RAIL_EVENT_TX_SCHEDULED_TX_MISSED |
			 SL_RAIL_EVENT_RX_SCHEDULED_RX_STARTED | SL_RAIL_EVENT_RX_SCHEDULED_RX_END |
			 SL_RAIL_EVENT_RX_SCHEDULED_RX_MISSED);
	}

	if (IS_ENABLED(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)) {
		rail_events |= SL_RAIL_EVENT_SCHEDULER_STATUS;
	}

	status = sl_rail_config_events(data->radio_data.rail_handle, SL_RAIL_EVENTS_ALL,
				       rail_events);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_config_events failed: %d", (int)status);
		return -EIO;
	}

	status = sl_rail_ieee802154_config_2p4_ghz_radio(data->radio_data.rail_handle);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_ieee802154_config_2p4_ghz_radio failed: %d", (int)status);
		return -EIO;
	}

	if (!IS_ENABLED(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)) {
		sl_rail_transition_time_t rx_to_enh_ack_tx = 256;

		/* 802.15.4E support (only on platforms that support it, so error checking
		 * is disabled) Note: This has to be called after
		 * sl_rail_ieee802154_config_2p4_ghz_radio due to a bug where this call can
		 * overwrite options set below.
		 */
		(void)sl_rail_ieee802154_config_e_options(
			data->radio_data.rail_handle,
			(SL_RAIL_IEEE802154_E_OPTION_GB868 | SL_RAIL_IEEE802154_E_OPTION_ENH_ACK),
			(SL_RAIL_IEEE802154_E_OPTION_GB868 | SL_RAIL_IEEE802154_E_OPTION_ENH_ACK));
		/* accommodate enhanced ACKs */
		status = sl_rail_ieee802154_set_rx_to_enh_ack_tx(data->radio_data.rail_handle,
								 &rx_to_enh_ack_tx);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_ERR("sl_rail_ieee802154_set_rx_to_enh_ack_tx failed: %d", (int)status);
			return -EIO;
		}
	}

	status = sl_rail_util_pa_post_init(data->radio_data.rail_handle,
					   SL_RAIL_TX_PA_MODE_2P4_GHZ);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_util_pa_post_init failed: %d", (int)status);
		return -EIO;
	}

	data->radio_data.rail_initialized = true;

	ret = silabs_efr32_set_txpower(dev, 0);
	if (ret != 0) {
		LOG_ERR("silabs_efr32_set_txpower failed: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_PM)) {
		sl_rail_timer_sync_config_t timer_sync_config = SL_RAIL_TIMER_SYNC_DEFAULT;

		status = sl_rail_config_sleep(data->radio_data.rail_handle, &timer_sync_config);
		if (status != SL_RAIL_STATUS_NO_ERROR) {
			LOG_ERR("sl_rail_config_sleep failed: %d", (int)status);
			return -EIO;
		}
	}

	memset(data->tx_buffer, 0, sizeof(data->tx_buffer));

	sl_rail_rx_options_t rx_opts = SL_RAIL_RX_OPTION_TRACK_ABORTED_FRAMES;

	if (IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		rx_opts |= SL_RAIL_RX_OPTION_STORE_CRC;
	}

	status = sl_rail_config_rx_options(data->radio_data.rail_handle, rx_opts, rx_opts);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_config_rx_options failed: %d", (int)status);
		return -EIO;
	}

	status = sl_rail_ieee802154_config_cca_mode(data->radio_data.rail_handle,
						    SL_RAIL_IEEE802154_CCA_MODE_RSSI);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_ieee802154_config_cca_mode failed: %d", (int)status);
		return -EIO;
	}

	return 0;
}

/*************************************************************************************************/
/* Energy Scan */

static void sl_802154_rail_timer_callback(struct sl_rail_multi_timer *tmr,
					  sl_rail_time_t expected_time, void *callback_arg);

static uint16_t sl_802154_get_symbol_duration_us(sl_rail_handle_t handle)
{
	uint32_t symbol_rate = sl_rail_get_symbol_rate(handle);

	if (symbol_rate == 0U) {
		return SL_802154_TIMING_DEFAULT_SYMBOLTIME_US;
	}
	return (uint16_t)MAX(1U, (1000000UL / symbol_rate));
}

static sl_rail_status_t sl_802154_schedule_next_reading(sl_rail_handle_t handle,
							sl_rail_multi_timer_t *rail_timer)
{
	sl_rail_cancel_multi_timer(handle, rail_timer);
	return sl_rail_set_multi_timer(handle, rail_timer,
				       SYMBOLS_PER_ENERGY_READING *
					       sl_802154_get_symbol_duration_us(handle),
				       SL_RAIL_TIME_DELAY, sl_802154_rail_timer_callback, handle);
}

/* Teardown without callback; for sync-error paths that return an errno. */
static void sl_802154_energy_scan_cleanup(sl_rail_handle_t handle, struct sl_802154_data *data)
{
	sl_rail_cancel_multi_timer(handle, &data->radio_data.rail_timer);
	data->ed_scan_data.in_progress = false;
	data->ed_scan_data.energy_scan_done = NULL;

	(void)sl_rail_config_rx_options(handle, SL_RAIL_RX_OPTION_DISABLE_FRAME_DETECTION,
					SL_RAIL_RX_OPTIONS_NONE);
}

/* Async termination; always invokes the user callback. */
static void sl_802154_energy_scan_finalize(sl_rail_handle_t handle)
{
	const sl_rail_config_t *rail_config = sl_rail_get_config(handle);
	const struct device *dev = rail_config->p_opaque_handle_0;
	struct sl_802154_data *data = dev->data;
	energy_scan_done_cb_t cb = data->ed_scan_data.energy_scan_done;
	int8_t result = data->ed_scan_data.max_reading;

	sl_802154_energy_scan_cleanup(handle, data);

	if (cb != NULL) {
		cb(dev, result);
	}
}

static void sl_802154_rail_timer_callback(struct sl_rail_multi_timer *tmr,
					  sl_rail_time_t expected_time, void *callback_arg)
{
	ARG_UNUSED(tmr);
	ARG_UNUSED(expected_time);

	sl_rail_handle_t handle = callback_arg;
	const sl_rail_config_t *rail_config = sl_rail_get_config(handle);
	struct sl_802154_data *data = rail_config->p_opaque_handle_1;
	struct sl_802154_energy_scan_data *ed_scan_data = &data->ed_scan_data;

	bool wait_for_rssi = (ed_scan_data->frame_counter == ed_scan_data->frame_counter_max - 1);
	unsigned int rssi_get_mode;

	if (wait_for_rssi) {
		rssi_get_mode = SL_RAIL_GET_RSSI_WAIT_WITHOUT_TIMEOUT;
	} else {
		rssi_get_mode = SL_RAIL_GET_RSSI_NO_WAIT;
	}

	int16_t rssi_quartered_dbm = sl_rail_get_rssi(data->radio_data.rail_handle, rssi_get_mode);

	if (rssi_quartered_dbm == SL_RAIL_RSSI_INVALID) {
		ed_scan_data->max_reading = INT8_MIN;
	} else {
		int rssi = rssi_quartered_dbm / QUARTER_DBM_IN_DBM;

		if (rssi > ed_scan_data->max_reading) {
			ed_scan_data->max_reading = rssi;
		}
	}

	ed_scan_data->frame_counter++;

	if (ed_scan_data->frame_counter < ed_scan_data->frame_counter_max) {
		if (sl_802154_schedule_next_reading(handle, &data->radio_data.rail_timer)) {
			sl_802154_energy_scan_finalize(handle);
		}
	} else {
		sl_802154_energy_scan_finalize(handle);
	}
}

/*************************************************************************************************/
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN,
					 OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX);

static int silabs_efr32_attr_get(const struct device *dev, enum ieee802154_attr attr,
				 struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	int ret;
	const enum ieee802154_phy_channel_page phy_page =
		IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915;

	ret = ieee802154_attr_get_channel_page_and_range(attr, phy_page,
							 &drv_attr.phy_supported_channels, value);

	if (ret) {
		return -ENOENT;
	}

	return 0;
}

static void silabs_efr32_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct sl_802154_data *data = dev->data;
	uint8_t eui64[SL_802154_EUI64_BYTES] = {0};
	ssize_t got;

	data->iface = iface;

	/* EUI64 from platform unique ID (matches SiSDK otPlatRadioGetIeeeEui64). */
	got = hwinfo_get_device_id(eui64, sizeof(eui64));
	if (got != (ssize_t)sizeof(eui64)) {
		LOG_WRN("hwinfo_get_device_id returned %zd, using zeroed EUI-64", got);
	}

	net_if_set_link_addr(iface, eui64, sizeof(eui64), NET_LINK_IEEE802154);

	/* Call L2 init (e.g. OpenThread: openthread_l2_init -> platformRadioInit).
	 * Without this, net_if_up -> openthread_run -> ThreadNetif::Up() runs with
	 * radio_api still NULL and faults in otPlatRadioSleep.
	 */
	ieee802154_init(iface);
}

static enum ieee802154_hw_caps silabs_efr32_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* HW_TXTIME and HW_RXTIME (CSL / TX_MODE_TXTIME / RX_SLOT) are intentionally
	 * not advertised: TX path supports only DIRECT and CSMA_CA, and there is no
	 * rx slot handling. Add them when CSL support lands.
	 */
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER | IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_ENERGY_SCAN | IEEE802154_HW_SLEEP_TO_TX | IEEE802154_HW_CSMA |
	       IEEE802154_HW_TX_SEC;
}

static int silabs_efr32_cca(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* EFR32 PAL does hardware CCA; so this API is not supported. */
	return -ENOTSUP;
}

static int silabs_efr32_set_channel(const struct device *dev, uint16_t channel)
{
	struct sl_802154_data *data = dev->data;

	if (channel < OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN ||
	    channel > OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX) {
		return -EINVAL;
	}
	data->current_channel = channel;
	/* Channel is applied on next start_rx (start() or after stop/start). */
	return 0;
}

static int silabs_efr32_filter(const struct device *dev, bool set, enum ieee802154_filter_type type,
			       const struct ieee802154_filter *filter)
{
	struct sl_802154_data *data = dev->data;

	if (set && filter == NULL) {
		return -EINVAL;
	}
	if (!data->radio_data.rail_initialized) {
		return -EAGAIN;
	}
	switch (type) {
	case IEEE802154_FILTER_TYPE_PAN_ID:
		data->pan_id = set ? filter->pan_id : IEEE802154_BROADCAST_PAN_ID;
		sl_rail_ieee802154_set_pan_id(data->radio_data.rail_handle, filter->pan_id, 0);
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		data->short_addr = set ? filter->short_addr : IEEE802154_NO_SHORT_ADDRESS_ASSIGNED;
		sl_rail_ieee802154_set_short_address(data->radio_data.rail_handle,
						     filter->short_addr, 0);
		break;
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		uint8_t long_addr[8] = {0};

		if (set) {
			memcpy(long_addr, filter->ieee_addr, sizeof(long_addr));
		}
		sys_memcpy_swap(data->ext_addr, long_addr, IEEE802154_EXT_ADDR_LENGTH);
		sl_rail_ieee802154_set_long_address(data->radio_data.rail_handle, long_addr,
						    0);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int silabs_efr32_set_txpower(const struct device *dev, int16_t dbm)
{
	struct sl_802154_data *data = dev->data;
	sl_rail_tx_power_t tx_power;
	sl_rail_status_t status;

	if (!data->radio_data.rail_initialized) {
		return -EAGAIN;
	}
	tx_power = (sl_rail_tx_power_t)dbm * 10;
	data->radio_data.txpwr = tx_power;
	status = sl_rail_set_tx_power_dbm(data->radio_data.rail_handle, tx_power);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	return 0;
}

static int silabs_efr32_start(const struct device *dev)
{
	sl_rail_status_t status;
	struct sl_802154_data *data = dev->data;
	sl_rail_scheduler_info_t bg_rx = {
		.priority = CONFIG_IEEE802154_SILABS_EFR32_BG_RX_PRIORITY,
		.slip_time = 0,
		.transaction_time = 0,
	};

	if (!data->radio_data.rail_initialized) {
		return -EAGAIN;
	}
	status = sl_rail_start_rx(data->radio_data.rail_handle, (uint8_t)data->current_channel,
				  &bg_rx);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		LOG_ERR("sl_rail_start_rx failed: %d", (int)status);
		return -EIO;
	}
	return 0;
}

static int silabs_efr32_stop(const struct device *dev)
{
	struct sl_802154_data *data = dev->data;

	if (sl_802154_state_is_tx_data_ongoing(&data->radio_data.state)) {
		return -EBUSY;
	}

	if (data->radio_data.rail_initialized) {
		sl_802154_set_radio_to_idle(data->radio_data.rail_handle);
	}
	return 0;
}

static int silabs_efr32_tx(const struct device *dev, enum ieee802154_tx_mode mode,
			   struct net_pkt *pkt, struct net_buf *frag)
{
	struct sl_802154_data *data = dev->data;
	sl_rail_status_t status;
	sl_rail_tx_options_t tx_options = SL_RAIL_TX_OPTIONS_DEFAULT;
	uint8_t len;
	sl_rail_scheduler_info_t tx_scheduler_info = {
		.priority = CONFIG_IEEE802154_SILABS_EFR32_TX_PRIORITY_MIN,
		.slip_time = SL_802154_SCHEDULER_CHANNEL_SLIP_TIME,
		.transaction_time = 0 /* will be calculated later if DMP is used */
	};
	sl_rail_csma_config_t csma = SL_RAIL_CSMA_CONFIG_802_15_4_2003_2P4_GHZ_OQPSK_CSMA;
	int sec_ret;
	uint32_t sym_rate;
	uint32_t bit_rate;
	uint16_t num_written;

	if (!data->radio_data.rail_initialized) {
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(!sl_802154_state_is_tx_data_ongoing(&data->radio_data.state));

	if ((size_t)frag->len + IEEE802154_FCS_LENGTH > sizeof(data->tx_buffer)) {
		return -EMSGSIZE;
	}
	len = (uint8_t)(frag->len + IEEE802154_FCS_LENGTH); /* PSDU = MPDU + FCS */

	sl_802154_state_set_tx_data_ongoing(&data->radio_data.state, true);

	memcpy(data->tx_buffer, frag->data, frag->len); /* RAIL appends FCS. */
	sl_802154_load_mhr(data->tx_buffer, frag->len, &data->tx_mhr);

	if (!net_pkt_ieee802154_frame_secured(pkt)) {
		sec_ret = sl_802154_security_process_tx(data, data->tx_buffer, len, &data->tx_mhr,
							net_pkt_ieee802154_mac_hdr_rdy(pkt));

		if (sec_ret < 0) {
			sl_802154_state_set_tx_data_ongoing(&data->radio_data.state, false);
			return sec_ret;
		}
	}
	/* PHR (first byte) = PSDU length (MPDU + CRC); then write MPDU
	 * RAIL appends the CRC in hardware.
	 */
	num_written = sl_rail_write_tx_fifo(data->radio_data.rail_handle, &len, sizeof(len), true);
	if (num_written != sizeof(len)) {
		LOG_WRN("Failed to write length to tx fifo");
		sl_802154_handle_tx_failed(data);
		return -ENOMEM;
	}

	num_written = sl_rail_write_tx_fifo(data->radio_data.rail_handle, data->tx_buffer,
					    frag->len, false);
	if (num_written != frag->len) {
		LOG_WRN("Failed to write fragment data to tx fifo");
		sl_802154_handle_tx_failed(data);
		return -ENOMEM;
	}

	k_sem_reset(&data->tx_wait);
	k_sem_reset(&data->ack_wait);
	data->tx_errno = -EIO;  /* default if no event fires */
	data->ack_errno = -EIO; /* default if no event fires */

	if (data->tx_mhr.fs->fc.ar) {
		tx_options |= SL_RAIL_TX_OPTION_WAIT_FOR_ACK;
		/* time we wait for ACK */
		if (IS_ENABLED(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)) {
			sym_rate = sl_rail_get_symbol_rate(data->radio_data.rail_handle);

			if (sym_rate > 0U) {
				tx_scheduler_info.transaction_time +=
					(sl_rail_time_t)(((uint64_t)12U * 1000000ULL) / sym_rate);
			} else {
				tx_scheduler_info.transaction_time +=
					12U * SL_802154_TIMING_DEFAULT_SYMBOLTIME_US;
			}
		}
	}

	if (IS_ENABLED(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)) {
		bit_rate = sl_rail_get_bit_rate(data->radio_data.rail_handle);

		if (bit_rate > 0U) {
			tx_scheduler_info.transaction_time +=
				(sl_rail_time_t)(((uint64_t)(len + 4U + 1U + 1U) * 8U *
						  1000000ULL) / bit_rate);
		} else { /* assume 250kbps */
			tx_scheduler_info.transaction_time +=
				(len + 4U + 1U + 1U) * SL_802154_TIMING_DEFAULT_BYTETIME_US;
		}
	}

	switch (mode) {
	case IEEE802154_TX_MODE_CSMA_CA:
		/* time needed for CSMA/CA */
		if (IS_ENABLED(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)) {
			tx_scheduler_info.transaction_time += SL_802154_TIMING_CSMA_OVERHEAD_US;
		}
		csma.csma_tries = data->radio_data.csma_ca_backoffs;
		csma.cca_threshold_dbm = SL_802154_CCA_THRESHOLD_DEFAULT;
		status = sl_rail_start_cca_csma_tx(data->radio_data.rail_handle,
						   (uint8_t)data->current_channel, tx_options,
						   &csma, &tx_scheduler_info);
		break;
	case IEEE802154_TX_MODE_DIRECT:
		status = sl_rail_start_tx(data->radio_data.rail_handle,
					  (uint8_t)data->current_channel, tx_options,
					  &tx_scheduler_info);
		break;
	default:
		LOG_ERR("TX mode %d not supported", mode);
		sl_802154_handle_tx_failed(data);
		return -ENOTSUP;
	}

	if (status != SL_RAIL_STATUS_NO_ERROR) {
		sl_802154_handle_tx_failed(data);
		LOG_ERR("sl_rail_start_tx/cca_csma_tx failed: %d (channel %u)", (int)status,
			(unsigned int)data->current_channel);
		return -EIO;
	}

	/* Zephyr event notification: TX_STARTED (host data/command TX on air). */
	if (data->event_handler != NULL) {
		data->event_handler(dev, IEEE802154_EVENT_TX_STARTED, NULL);
	}

	/* Block until RAIL events callback sets tx_errno and gives tx_wait. */
	k_sem_take(&data->tx_wait, K_FOREVER);
	if (data->tx_errno) {
		return data->tx_errno;
	}

	if (data->tx_mhr.fs->fc.ar) {
		k_sem_take(&data->ack_wait, K_FOREVER);
		if (data->ack_errno) {
			return data->ack_errno;
		}
	}

	return 0;
}

static int silabs_efr32_energy_scan_start(const struct device *dev, uint16_t duration,
					  energy_scan_done_cb_t done_cb)
{
	sl_rail_status_t status;
	struct sl_802154_data *data = dev->data;
	sl_rail_handle_t handle = data->radio_data.rail_handle;
	uint32_t sample_period_us;
	uint32_t scan_duration_us;
	uint32_t max_samples;
	sl_rail_scheduler_info_t rx_scheduler_info = {
		.priority = CONFIG_IEEE802154_SILABS_EFR32_BG_RX_PRIORITY,
		.slip_time = 0,
		.transaction_time = 0,
	};

	if (!data->radio_data.rail_initialized || done_cb == NULL) {
		return -ENETDOWN;
	}

	if (data->ed_scan_data.in_progress) {
		return -EALREADY;
	}

	/* ed_scan duration is per Zephyr radio API in milliseconds. */
	sample_period_us =
		(uint32_t)SYMBOLS_PER_ENERGY_READING * sl_802154_get_symbol_duration_us(handle);
	scan_duration_us = (uint32_t)duration * USEC_PER_MSEC;
	max_samples = sample_period_us ? (scan_duration_us / sample_period_us) : 1;

	if (max_samples == 0) {
		max_samples = 1;
	}

	data->ed_scan_data = (struct sl_802154_energy_scan_data){
		.frame_counter_max = max_samples,
		.max_reading = INT8_MIN,
		.in_progress = false,
		.energy_scan_done = done_cb,
	};

	/* Disable frame detection during energy scan */
	status = sl_rail_config_rx_options(handle, SL_RAIL_RX_OPTION_DISABLE_FRAME_DETECTION,
					   SL_RAIL_RX_OPTION_DISABLE_FRAME_DETECTION);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}

	status = sl_rail_start_rx(handle, data->current_channel, &rx_scheduler_info);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		sl_802154_energy_scan_cleanup(handle, data);
		return -EIO;
	}

	/* Schedule initial timer for first sample */
	status = sl_802154_schedule_next_reading(handle, &data->radio_data.rail_timer);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		sl_802154_energy_scan_cleanup(handle, data);
		return -EBUSY;
	}

	data->ed_scan_data.in_progress = true;

	return 0;
}

static net_time_t silabs_efr32_get_time(const struct device *dev)
{
	struct sl_802154_data *data = dev->data;
	uint32_t now_32_us;
	uint64_t ns64;
	unsigned int key;

	if (!data->radio_data.rail_initialized) {
		return -1;
	}
	/* Hold irq_lock across the timer read and sl_802154_us32_to_ns64() so an ISR
	 * cannot observe a later value first and cause a spurious wrap.
	 */
	key = irq_lock();
	now_32_us = sl_rail_get_time(data->radio_data.rail_handle);
	ns64 = sl_802154_us32_to_ns64(now_32_us);
	irq_unlock(key);
	return (net_time_t)ns64;
}

static uint8_t silabs_efr32_get_acc(const struct device *dev)
{
	const struct sl_802154_config *cfg = dev->config;
	uint32_t ppm = cfg->hfxo_accuracy_ppm;

	/* The radio's awake-time timing reference is HFXO, so its
	 * precision is always counted. When this build is a Sleepy End
	 * Device AND the SoC is actually allowed to sleep (CONFIG_PM),
	 * also include the sleep-clock (LFXO) precision because timing
	 * is maintained by it during sleep, so the worst-case scheduler
	 * drift accumulates both. Without CONFIG_PM the SoC stays awake
	 * between polls, so the sleep-clock drift never applies even on
	 * an MTD_SED build.
	 */
	const bool mtd_sed = IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED);
	const bool pm = IS_ENABLED(CONFIG_PM);

	if (mtd_sed && pm) {
		ppm += cfg->sleep_clock_accuracy_ppm;
	}

	return (uint8_t)MIN(ppm, UINT8_MAX);
}

static int sl_802154_configure_ack_fpb(const struct device *dev, struct sl_802154_data *data,
				       const struct ieee802154_config *config)
{
	/* Soft source match table (SiSDK soft_source_table_match / AddSrcMatch*). */
	if (config->ack_fpb.enabled) {
		if (config->ack_fpb.addr == NULL) {
			return -EINVAL;
		}
		if (config->ack_fpb.extended) {
			return sl_802154_src_match_ext_add(&data->match_data, config->ack_fpb.addr);
		} else {
			return sl_802154_src_match_short_add(&data->match_data,
							     config->ack_fpb.addr);
		}
	}
	if (config->ack_fpb.addr != NULL) {
		if (config->ack_fpb.extended) {
			return sl_802154_src_match_ext_remove(&data->match_data,
							      config->ack_fpb.addr);
		} else {
			return sl_802154_src_match_short_remove(&data->match_data,
								config->ack_fpb.addr);
		}
	}
	if (config->ack_fpb.extended) {
		sl_802154_src_match_ext_clear(&data->match_data);
	} else {
		sl_802154_src_match_short_clear(&data->match_data);
	}
	return 0;
}

static int sl_802154_configure_mac_keys(const struct device *dev, struct sl_802154_data *data,
					const struct ieee802154_config *config)
{
	struct ieee802154_key *in = config->mac_keys;
	uint8_t sec_key_count = 0;
	struct sl_802154_mac_data *mac_data = &data->mac_data;

	if (in == NULL) {
		return -EINVAL;
	}

	/* Reset FC before key install to avoid new-key + stale-FC enh-ACK race. */
	data->mac_data.frame_counter = 0;

	for (; in->key_value != NULL && sec_key_count < ARRAY_SIZE(mac_data->sec_keys); in++) {
		uint8_t kid;

		/* KEY_ID_MODE_IMPLICIT (no key_id) is not supported. */
		if (in->key_id == NULL) {
			return -EINVAL;
		}
		kid = *in->key_id;

		mac_data->sec_keys[sec_key_count].key_frame_counter = in->key_frame_counter;
		mac_data->sec_keys[sec_key_count].frame_counter_per_key = in->frame_counter_per_key;
		mac_data->sec_keys[sec_key_count].key_id_mode = in->key_id_mode;
		memcpy(mac_data->sec_keys[sec_key_count].key_value, in->key_value,
		       IEEE802154_KEY_MAX_LEN);
		*mac_data->sec_keys[sec_key_count].key_id = kid;

		if (sec_key_count == SL_802154_MAC_KEY_PREVIOUS) {
			data->key_id_prev = kid;
		} else if (sec_key_count == SL_802154_MAC_KEY_CURRENT) {
			data->key_id_current = kid;
		} else {
			data->key_id_next = kid;
		}
		sec_key_count++;
	}
	return 0;
}

static int sl_802154_configure_enh_ack_header_ie(const struct device *dev,
						 struct sl_802154_data *data,
						 const struct ieee802154_config *config)
{
	uint8_t element_id;
	size_t ie_total;

	ARG_UNUSED(dev);

	if (config->ack_ie.purge_ie) {
		if (IS_ENABLED(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)) {
			memset(&data->ack_ie.link_metrics_header_ie, 0,
			       sizeof(data->ack_ie.link_metrics_header_ie));
		}
		return 0;
	}

	if (config->ack_ie.short_addr == IEEE802154_BROADCAST_ADDRESS ||
	    config->ack_ie.ext_addr == NULL) {
		return -ENOTSUP;
	}

	data->ack_ie.short_addr = sys_get_le16((const uint8_t *)&config->ack_ie.short_addr);
	sys_memcpy_swap(data->ack_ie.ext_addr, config->ack_ie.ext_addr, IEEE802154_EXT_ADDR_LENGTH);

	if (config->ack_ie.header_ie == NULL || config->ack_ie.header_ie->length == 0) {
		if (IS_ENABLED(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)) {
			memset(&data->ack_ie.link_metrics_header_ie, 0,
			       sizeof(data->ack_ie.link_metrics_header_ie));
		}
		return 0;
	}

	element_id = ieee802154_header_ie_get_element_id(config->ack_ie.header_ie);
	if (element_id != IEEE802154_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE) {
		return -ENOTSUP;
	}

	if (element_id == IEEE802154_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE &&
	    IS_ENABLED(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)) {
		ie_total = IEEE802154_HEADER_IE_HEADER_LENGTH + config->ack_ie.header_ie->length;
		if (ie_total > sizeof(data->ack_ie.link_metrics_header_ie)) {
			return -EMSGSIZE;
		}
		memcpy(&data->ack_ie.link_metrics_header_ie, config->ack_ie.header_ie, ie_total);
	}

	return 0;
}

static int silabs_efr32_configure(const struct device *dev, enum ieee802154_config_type type,
				  const struct ieee802154_config *config)
{
	struct sl_802154_data *data = dev->data;
	sl_rail_status_t status;

	if (config == NULL) {
		return -EINVAL;
	}

	switch (type) {
	case IEEE802154_CONFIG_PROMISCUOUS:
		if (!data->radio_data.rail_initialized) {
			return -EINVAL;
		}
		data->promiscuous = config->promiscuous;
		status = sl_rail_ieee802154_set_promiscuous_mode(data->radio_data.rail_handle,
								 data->promiscuous);
		return (status == SL_RAIL_STATUS_NO_ERROR) ? 0 : -EIO;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		if (!data->radio_data.rail_initialized) {
			return -EINVAL;
		}
		data->pan_coordinator = config->pan_coordinator;
		status = sl_rail_ieee802154_set_pan_coordinator(data->radio_data.rail_handle,
								config->pan_coordinator);
		return (status == SL_RAIL_STATUS_NO_ERROR) ? 0 : -EIO;

	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		data->is_src_match_enabled = config->auto_ack_fpb.enabled;
		return 0;

	case IEEE802154_CONFIG_ACK_FPB:
		return sl_802154_configure_ack_fpb(dev, data, config);

	case IEEE802154_CONFIG_EVENT_HANDLER:
		data->event_handler = config->event_handler;
		return 0;

	case IEEE802154_CONFIG_MAC_KEYS:
		return sl_802154_configure_mac_keys(dev, data, config);

	case IEEE802154_CONFIG_FRAME_COUNTER:
		if (config->frame_counter <= data->mac_data.frame_counter) {
			return -EINVAL;
		}
		data->mac_data.frame_counter = config->frame_counter;
		return 0;

	case IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER:
		if (config->frame_counter > data->mac_data.frame_counter) {
			data->mac_data.frame_counter = config->frame_counter;
		}
		return 0;

	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		return sl_802154_configure_enh_ack_header_ie(dev, data, config);

	case IEEE802154_CONFIG_CSMA_CA_BACKOFFS:
		data->radio_data.csma_ca_backoffs = config->csma_ca_backoffs;
		return 0;

	default:
		return -ENOTSUP;
	}
}

#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
static int silabs_efr32_continuous_carrier(const struct device *dev)
{
	sl_rail_tx_options_t tx_options = SL_RAIL_TX_OPTIONS_DEFAULT;
	struct sl_802154_data *data = dev->data;
	sl_rail_status_t ret;

	/* Check if the driver is already in TESTING state */
	if (data->testing == true) {
		return -EALREADY;
	}

	ret = sl_rail_start_tx_stream(data->radio_data.rail_handle, data->current_channel,
				      SL_RAIL_STREAM_CARRIER_WAVE, tx_options);
	if (ret != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}
	/* Update driver state */
	data->testing = true;

	return 0;
}

static int silabs_efr32_modulated_carrier(const struct device *dev, const uint8_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	return -ENOTSUP;
}
#endif

static const struct sl_802154_config silabs_efr32_radio_cfg = {
	.hfxo_accuracy_ppm = COND_CODE_1(DT_INST_NODE_HAS_PROP(0, hfxo),
					 (DT_PROP(DT_INST_PHANDLE(0, hfxo), precision)), (250)),
	.sleep_clock_accuracy_ppm = COND_CODE_1(DT_INST_NODE_HAS_PROP(0, sleep_clock),
						 (DT_PROP(DT_INST_PHANDLE(0, sleep_clock),
							  precision)),
						 (0)),
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

/* Returns 0 on success, negative errno on failure. */
static int silabs_efr32_init(const struct device *dev)
{
	struct sl_802154_data *data = dev->data;
	const struct sl_802154_config *cfg = dev->config;
	int r;

	if (data->radio_data.rail_initialized) {
		return 0;
	}

	k_sem_init(&data->tx_wait, 0, 1);
	k_sem_init(&data->ack_wait, 0, 1);

	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func(dev);
	}

	sl_openthread_init();

	r = sl_802154_rail_init(dev);
	if (r != 0) {
		return r;
	}

	r = sl_802154_rail_config(dev);
	if (r != 0) {
		return r;
	}

	LOG_DBG("SILABS_EFR32_INIT: Success");

	return 0;
}

/* Driver data */
static struct sl_802154_data silabs_efr32_data = {
	.radio_data = {
		.state = ATOMIC_INIT(0),
		.rail_ieee802154_config = {
			.p_addresses = NULL,
			.ack_config = {
				.enable = true,
				.ack_timeout_us = 672,
				.rx_transitions = {
					.success = SL_RAIL_RF_STATE_RX,
					.error = SL_RAIL_RF_STATE_RX,
				},
				.tx_transitions = {
					.success = SL_RAIL_RF_STATE_RX,
					.error = SL_RAIL_RF_STATE_RX,
				},
			},
			.timings = {
				.idle_to_rx = 100,
				.tx_to_rx = 192 - 10,
				.idle_to_tx = 100,
				.rx_to_tx = 192,
				.rxsearch_timeout = 0,
				.tx_to_rxsearch_timeout = 0,
				.tx_to_tx = 0,
			},
			.frames_mask = SL_RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
			.promiscuous_mode = false,
			.is_pan_coordinator = false,
			.default_frame_pending_in_outgoing_acks = false,
		},
	},
};

#if defined(CONFIG_NET_L2_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, silabs_efr32_init, NULL, &silabs_efr32_data, &silabs_efr32_radio_cfg,
			  CONFIG_IEEE802154_SILABS_EFR32_INIT_PRIO, &silabs_efr32_radio_api,
			  IEEE802154_L2, NET_L2_GET_CTX_TYPE(IEEE802154_L2), IEEE802154_MTU);
#elif defined(CONFIG_NET_L2_OPENTHREAD)
NET_DEVICE_DT_INST_DEFINE(0, silabs_efr32_init, NULL, &silabs_efr32_data, &silabs_efr32_radio_cfg,
			  CONFIG_IEEE802154_SILABS_EFR32_INIT_PRIO, &silabs_efr32_radio_api,
			  OPENTHREAD_L2, NET_L2_GET_CTX_TYPE(OPENTHREAD_L2), 1280);
#elif defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, silabs_efr32_init, NULL, &silabs_efr32_data, &silabs_efr32_radio_cfg,
			  CONFIG_IEEE802154_SILABS_EFR32_INIT_PRIO, &silabs_efr32_radio_api,
			  CUSTOM_IEEE802154_L2, NET_L2_GET_CTX_TYPE(CUSTOM_IEEE802154_L2),
			  CONFIG_NET_L2_CUSTOM_IEEE802154_MTU);
#else
DEVICE_DT_INST_DEFINE(0, silabs_efr32_init, NULL, &silabs_efr32_data, &silabs_efr32_radio_cfg,
		      POST_KERNEL, CONFIG_IEEE802154_SILABS_EFR32_INIT_PRIO,
		      &silabs_efr32_radio_api);
#endif
