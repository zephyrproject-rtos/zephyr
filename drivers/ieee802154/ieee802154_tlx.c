/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_zb

#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
#include "rf_common.h"
#endif
#include "stimer.h"
#include "tl_rf_power.h"

#define LOG_MODULE_NAME ieee802154_tlx
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/random/random.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#endif

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include "ieee802154_tlx.h"

#include "ieee802154_tlx_frame.c"

#if defined(CONFIG_IEEE802154_TLX_MAC_FLASH)
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

static const struct device *flash_device =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
#endif /* CONFIG_IEEE802154_TLX_MAC_FLASH */

#ifdef CONFIG_OPENTHREAD_FTD
/* TLX radio source match table structure */
static struct tlx_src_match_table src_match_table;
#endif /* CONFIG_OPENTHREAD_FTD */

#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
/* TLX radio ACK table structure */
static struct tlx_enh_ack_table enh_ack_table;
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */

#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
/* mac keys data */
static struct tlx_mac_keys mac_keys;
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */

/* TLX data structure */
static struct  tlx_data data = {
#ifdef CONFIG_OPENTHREAD_FTD
	.src_match_table = &src_match_table,
#endif /* CONFIG_OPENTHREAD_FTD */
#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
	.enh_ack_table = &enh_ack_table,
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
/* mac keys data */
	.mac_keys = &mac_keys,
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */
};

#ifdef CONFIG_OPENTHREAD_FTD

/* clean radio search match table */
static void tlx_src_match_table_clean(struct tlx_src_match_table *table)
{
	memset(table, 0, sizeof(struct tlx_src_match_table));
}

/* Search in radio search match table */
static bool
ALWAYS_INLINE tlx_src_match_table_search(
	const struct tlx_src_match_table *table, const uint8_t *addr, bool ext)
{
	bool result = false;

	for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid && table->item[i].ext == ext &&
			!memcmp(table->item[i].addr, addr,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT)) {
			result = true;
			break;
		}
	}

	return result;
}

/* Add to radio search match table */
static void tlx_src_match_table_add(
	struct tlx_src_match_table *table, const uint8_t *addr, bool ext)
{
	if (!tlx_src_match_table_search(table, addr, ext)) {
		for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
			if (!table->item[i].valid) {
				table->item[i].ext = ext;
				memcpy(table->item[i].addr, addr,
					ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
					IEEE802154_FRAME_LENGTH_ADDR_SHORT);
				table->item[i].valid = true;
				break;
			}
		}
	}
}

/* Remove from radio search match table */
static void tlx_src_match_table_remove(
	struct tlx_src_match_table *table, const uint8_t *addr, bool ext)
{
	for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid && table->item[i].ext == ext &&
			!memcmp(table->item[i].addr, addr,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT)) {
			table->item[i].valid = false;
			table->item[i].ext = false;
			memset(table->item[i].addr, 0,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT);
			break;
		}
	}
}

/* Remove all entries from radio search match table */
static void tlx_src_match_table_remove_group(struct tlx_src_match_table *table, bool ext)
{
	for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid && table->item[i].ext == ext) {
			table->item[i].valid = false;
			table->item[i].ext = false;
			memset(table->item[i].addr, 0,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT);
		}
	}
}

/*
 * Check frame possible require to set pending bit
 * data request command or data
 * frame should be valid
 */
static bool
ALWAYS_INLINE tlx_require_pending_bit(const struct ieee802154_frame *frame)
{
	bool result = false;

	if (frame->general.valid) {
		if (frame->general.type == IEEE802154_FRAME_FCF_TYPE_DATA) {
			result = true;
		} else if (frame->general.type == IEEE802154_FRAME_FCF_TYPE_CMD) {
			if (!frame->sec_header ||
				frame->general.ver < IEEE802154_FRAME_FCF_VER_2015 ||
				(frame->sec_header[0] & IEEE802154_FRAME_SECCTRL_SEC_LEVEL_MASK) <
					IEEE802154_FRAME_SECCTRL_SEC_LEVEL_4) {
				const uint8_t *cmd_id = frame->payload_ie ?
					tlx_ieee802154_get_data(frame->payload,
					frame->payload_len) : frame->payload;
				if (cmd_id && *cmd_id == TLX_CMD_ID_DATA_REQ) {
					result = true;
				}
			} else {
				/* TODO: temporary solution. need to decrypt payload */
				result = true;
			}
		}
	}
	return result;
}

#endif /* CONFIG_OPENTHREAD_FTD */

#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT

/* clean radio search match table */
static void tlx_enh_ack_table_clean(struct tlx_enh_ack_table *table)
{
	memset(table, 0, sizeof(struct tlx_enh_ack_table));
}

/* Search in enhanced ack table */
static int
ALWAYS_INLINE tlx_enh_ack_table_search(
	const struct tlx_enh_ack_table *table, const uint8_t *addr_short, const uint8_t *addr_ext)
{
	int result = -1;

	for (size_t i = 0; i < CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid &&
			(!memcmp(table->item[i].addr_short, addr_short,
				IEEE802154_FRAME_LENGTH_ADDR_SHORT) ||
			!memcmp(table->item[i].addr_ext, addr_ext,
				IEEE802154_FRAME_LENGTH_ADDR_EXT))) {
			result = i;
			break;
		}
	}

	return result;
}

/* Add to enhanced ack table */
static void tlx_enh_ack_table_add(
	struct tlx_enh_ack_table *table, const uint8_t *addr_short, const uint8_t *addr_ext,
	const struct ieee802154_header_ie *ie_header)
{
	int idx = tlx_enh_ack_table_search(table, addr_short, addr_ext);

	if (idx == -1) {
		for (size_t i = 0; i < CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
			if (!table->item[i].valid) {
				idx = i;
				memcpy(table->item[idx].addr_short, addr_short,
					IEEE802154_FRAME_LENGTH_ADDR_SHORT);
				memcpy(table->item[idx].addr_ext, addr_ext,
					IEEE802154_FRAME_LENGTH_ADDR_EXT);
				table->item[idx].valid = true;
				break;
			}
		}
	}
	if (idx != -1) {
		memcpy(&table->item[idx].ie_header, ie_header,
			sizeof(struct ieee802154_header_ie));
	}
}

/* Remove from enhanced ack table */
static void tlx_enh_ack_table_remove(
	struct tlx_enh_ack_table *table, const uint8_t *addr_short, const uint8_t *addr_ext)
{
	for (size_t i = 0; i < CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid &&
			!memcmp(table->item[i].addr_short, addr_short,
				IEEE802154_FRAME_LENGTH_ADDR_SHORT) &&
			!memcmp(table->item[i].addr_ext, addr_ext,
				IEEE802154_FRAME_LENGTH_ADDR_EXT)) {
			table->item[i].valid = false;
			memset(table->item[i].addr_short, 0,
				IEEE802154_FRAME_LENGTH_ADDR_SHORT);
			memset(table->item[i].addr_ext, 0,
				IEEE802154_FRAME_LENGTH_ADDR_EXT);
			memset(&table->item[i].ie_header, 0,
				sizeof(struct ieee802154_header_ie));
			break;
		}
	}
}

#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */

#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION

/* Clean mac keys data */
static void tlx_mac_keys_data_clean(struct tlx_mac_keys *mac_keys_data)
{
	memset(mac_keys_data, 0, sizeof(struct tlx_mac_keys));
}

static const uint8_t *tlx_mac_keys_get(const struct tlx_mac_keys *mac_keys_data, uint8_t key_id)
{
	const uint8_t *result = NULL;

	if (key_id) {
		for (size_t i = 0; i < TLX_MAC_KEYS_ITEMS; i++) {
			if (mac_keys_data->item[i].key_id == key_id) {
				result = mac_keys_data->item[i].key;
				break;
			}
		}
	}
	return result;
}

static uint32_t tlx_mac_keys_frame_cnt_get(const struct tlx_mac_keys *mac_keys_data, uint8_t key_id)
{
	uint32_t result = 0;

	if (key_id) {
		for (size_t i = 0; i < TLX_MAC_KEYS_ITEMS; i++) {
			if (mac_keys_data->item[i].key_id == key_id) {
				if (mac_keys_data->item[i].frame_cnt_local) {
					result = mac_keys_data->item[i].frame_cnt;
				} else {
					result = mac_keys_data->frame_cnt;
				}
				break;
			}
		}
	}
	return result;
}

static void tlx_mac_keys_frame_cnt_inc(struct tlx_mac_keys *mac_keys_data, uint8_t key_id)
{
	if (key_id) {
		for (size_t i = 0; i < TLX_MAC_KEYS_ITEMS; i++) {
			if (mac_keys_data->item[i].key_id == key_id) {
				if (mac_keys_data->item[i].frame_cnt_local) {
					mac_keys_data->item[i].frame_cnt++;
				} else {
					mac_keys_data->frame_cnt++;
				}
				break;
			}
		}
	}
}

#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */

/* Disable power management by device */
static void tlx_disable_pm(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	struct tlx_data *tlx = dev->data;

	if (atomic_test_and_set_bit(&tlx->current_pm_lock, 0) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
	if (atomic_test_and_set_bit(&tlx->current_pm_lock, 1) == 0) {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#else
	ARG_UNUSED(dev);
#endif /* CONFIG_PM_DEVICE */
}

/* Enable power management by device */
static void tlx_enable_pm(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	struct tlx_data *tlx = dev->data;

	if (atomic_test_and_clear_bit(&tlx->current_pm_lock, 0) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
	if (atomic_test_and_clear_bit(&tlx->current_pm_lock, 1) == 1) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#else
	ARG_UNUSED(dev);
#endif /* CONFIG_PM_DEVICE */
}

/* Set filter PAN ID */
static int tlx_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	struct tlx_data *tlx = dev->data;
	uint8_t pan_id_le[IEEE802154_FRAME_LENGTH_PANID];

	sys_put_le16(pan_id, pan_id_le);
	memcpy(tlx->filter_pan_id, pan_id_le, IEEE802154_FRAME_LENGTH_PANID);

	return 0;
}

/* Set filter short address */
static int tlx_set_short_addr(const struct device *dev, uint16_t short_addr)
{
	struct tlx_data *tlx = dev->data;
	uint8_t short_addr_le[IEEE802154_FRAME_LENGTH_ADDR_SHORT];

	sys_put_le16(short_addr, short_addr_le);
	memcpy(tlx->filter_short_addr, short_addr_le, IEEE802154_FRAME_LENGTH_ADDR_SHORT);

	return 0;
}

/* Set filter IEEE address */
static int tlx_set_ieee_addr(const struct device *dev, const uint8_t *ieee_addr)
{
	struct tlx_data *tlx = dev->data;

	memcpy(tlx->filter_ieee_addr, ieee_addr, IEEE802154_FRAME_LENGTH_ADDR_EXT);

	return 0;
}

/* Filter PAN ID, short address and IEEE address */
static bool
ALWAYS_INLINE tlx_run_filter(const struct device *dev, const struct ieee802154_frame *frame)
{
	struct tlx_data *tlx = dev->data;
	bool result = false;

	do {
		if (frame->dst_panid != NULL) {
			if (memcmp(frame->dst_panid, tlx->filter_pan_id,
					IEEE802154_FRAME_LENGTH_PANID) != 0 &&
				memcmp(frame->dst_panid, TLX_BROADCAST_ADDRESS,
					IEEE802154_FRAME_LENGTH_PANID) != 0) {
				break;
			}
		}
		if (frame->dst_addr != NULL) {
			if (frame->dst_addr_ext) {
				if ((net_if_get_link_addr(tlx->iface)->len !=
						IEEE802154_FRAME_LENGTH_ADDR_EXT) ||
					memcmp(frame->dst_addr, tlx->filter_ieee_addr,
						IEEE802154_FRAME_LENGTH_ADDR_EXT) != 0) {
					break;
				}
			} else {
				if (memcmp(frame->dst_addr, TLX_BROADCAST_ADDRESS,
						IEEE802154_FRAME_LENGTH_ADDR_SHORT) != 0 &&
					memcmp(frame->dst_addr, tlx->filter_short_addr,
						IEEE802154_FRAME_LENGTH_ADDR_SHORT) != 0) {
					break;
				}
			}
		}
		result = true;
	} while (0);

	return result;
}

/* Get MAC address */
static ALWAYS_INLINE uint8_t *tlx_get_mac(const struct device *dev)
{
	struct tlx_data *tlx = dev->data;

#if defined(CONFIG_IEEE802154_TLX_MAC_RANDOM)
	sys_rand_get(tlx->mac_addr, sizeof(tlx->mac_addr));

	/*
	 * Clear bit 0 to ensure it isn't a multicast address and set
	 * bit 1 to indicate address is locally administered and may
	 * not be globally unique.
	 */
	tlx->mac_addr[0] = (tlx->mac_addr[0] & ~0x01) | 0x02;
#elif defined(CONFIG_IEEE802154_TLX_MAC_FLASH)
	(void) flash_read(flash_device, FIXED_PARTITION_OFFSET(vendor_partition)
			+ IEEE802154_TLX_FLASH_MAC_OFFSET, tlx->mac_addr,
			IEEE802154_FRAME_LENGTH_ADDR_EXT);
#else /* CONFIG_IEEE802154_TLX_MAC_STATIC */
	/* Vendor Unique Identifier */
	tlx->mac_addr[0] = 0xC4;
	tlx->mac_addr[1] = 0x19;
	tlx->mac_addr[2] = 0xD1;
	tlx->mac_addr[3] = 0x00;

	/* Extended Unique Identifier */
	tlx->mac_addr[4] = CONFIG_IEEE802154_TLX_MAC4;
	tlx->mac_addr[5] = CONFIG_IEEE802154_TLX_MAC5;
	tlx->mac_addr[6] = CONFIG_IEEE802154_TLX_MAC6;
	tlx->mac_addr[7] = CONFIG_IEEE802154_TLX_MAC7;
#endif /* IEEE802154_TLX_MAC_TYPE */

	return tlx->mac_addr;
}

/* Convert RSSI to LQI */
static uint8_t
ALWAYS_INLINE tlx_convert_rssi_to_lqi(int8_t rssi)
{
	uint32_t lqi32 = 0;

	/* check for MIN value */
	if (rssi < TLX_RSSI_TO_LQI_MIN) {
		return 0;
	}

	/* convert RSSI to LQI */
	lqi32 = TLX_RSSI_TO_LQI_SCALE * (rssi - TLX_RSSI_TO_LQI_MIN);

	/* check for MAX value */
	if (lqi32 > 0xFF) {
		lqi32 = 0xFF;
	}

	return (uint8_t)lqi32;
}

/* Update RSSI and LQI parameters */
static void
ALWAYS_INLINE tlx_update_rssi_and_lqi(const struct device *dev, struct net_pkt *pkt)
{
	struct tlx_data *tlx = dev->data;
	int8_t rssi;
	uint8_t lqi;

	rssi = ((signed char)(tlx->rx_buffer
			      [tlx->rx_buffer[TLX_LENGTH_OFFSET] + TLX_RSSI_OFFSET])) - 110;
	lqi = tlx_convert_rssi_to_lqi(rssi);

	net_pkt_set_ieee802154_lqi(pkt, lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, rssi);
}

/* Prepare TX buffer */
static void
ALWAYS_INLINE tlx_set_tx_payload(const struct device *dev, uint8_t *payload, uint8_t payload_len)
{
	struct tlx_data *tlx = dev->data;
	unsigned char rf_data_len;
	unsigned int rf_tx_dma_len;

	rf_data_len = payload_len + 1;
	rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
	tlx->tx_buffer[0] = rf_tx_dma_len & 0xff;
	tlx->tx_buffer[1] = (rf_tx_dma_len >> 8) & 0xff;
	tlx->tx_buffer[2] = (rf_tx_dma_len >> 16) & 0xff;
	tlx->tx_buffer[3] = (rf_tx_dma_len >> 24) & 0xff;
	tlx->tx_buffer[4] = payload_len + IEEE802154_FCS_LENGTH;
	memcpy(tlx->tx_buffer + TLX_PAYLOAD_OFFSET, payload, payload_len);
}

/* Handle acknowledge packet */
static void
ALWAYS_INLINE tlx_handle_ack(const struct device *dev,
	const void *buf, size_t buf_len, uint64_t rx_time)
{
	struct tlx_data *tlx = dev->data;
	struct net_pkt *ack_pkt = net_pkt_rx_alloc_with_buffer(
		tlx->iface, buf_len, AF_UNSPEC, 0, K_NO_WAIT);

	do {
		if (!ack_pkt) {
			LOG_ERR("No free packet available.");
			break;
		}
		if (net_pkt_write(ack_pkt, buf, buf_len) < 0) {
			LOG_ERR("Failed to write to a packet.");
			break;
		}
		tlx_update_rssi_and_lqi(dev, ack_pkt);
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
		struct net_ptp_time timestamp = {
			.second = rx_time / USEC_PER_SEC,
			.nanosecond = (rx_time % USEC_PER_SEC) * NSEC_PER_USEC
		};
		net_pkt_set_timestamp(ack_pkt, &timestamp);
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		net_pkt_cursor_init(ack_pkt);
		if (ieee802154_handle_ack(tlx->iface, ack_pkt) != NET_OK) {
			LOG_INF("ACK packet not handled - releasing.");
		}
		k_sem_give(&tlx->ack_wait);
	} while (0);

	if (ack_pkt) {
		net_pkt_unref(ack_pkt);
	}
}

/* Send acknowledge packet */
static void
ALWAYS_INLINE tlx_send_ack(const struct device *dev, struct ieee802154_frame *frame)
{
	struct tlx_data *tlx = dev->data;
	uint8_t ack_buf[64];
	size_t ack_len;
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
	const uint8_t *key = NULL;
	uint32_t frame_cnt = tlx_mac_keys_frame_cnt_get(tlx->mac_keys, 1);
	const uint8_t sec_header[] = {
		IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5 | IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_1,
		frame_cnt,
		frame_cnt >> 8,
		frame_cnt >> 16,
		frame_cnt >> 24,
		1
	};
	uint8_t payload[frame->payload_len + 4];

	if (frame->general.ver == IEEE802154_FRAME_FCF_VER_2015) {
		key = tlx_mac_keys_get(tlx->mac_keys, 1);
		if (key && frame->payload) {
			memcpy(payload, frame->payload, frame->payload_len);
			frame->sec_header = sec_header;
			frame->sec_header_len = sizeof(sec_header);
			frame->payload = payload;
			frame->payload_len = sizeof(payload);
		}
	}
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */

	if (tlx_ieee802154_frame_build(frame, ack_buf, sizeof(ack_buf), &ack_len)) {
		tlx->ack_sending = true;
		k_sem_reset(&tlx->tx_wait);
		rf_set_txmode();
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
		if (frame->sec_header) {
			if (ieee802154_tlx_crypto_encrypt(key, tlx->filter_ieee_addr,
				frame_cnt,
				IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5,
				ack_buf, ack_len - 4,
				NULL, 0,
				NULL,
				&ack_buf[ack_len - 4], 4)) {
				tlx_mac_keys_frame_cnt_inc(tlx->mac_keys, 1);
			} else {
				LOG_WRN("encrypt ack failed");
			}
		} else {
			delay_us(CONFIG_IEEE802154_TLX_SET_TXRX_DELAY_US);
		}
#else
		delay_us(CONFIG_IEEE802154_TLX_SET_TXRX_DELAY_US);
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */
		tlx_set_tx_payload(dev, ack_buf, ack_len);
		rf_tx_pkt(tlx->tx_buffer);
	} else {
		LOG_ERR("Failed to create ACK.");
	}
}

/* RX IRQ handler */
static void ALWAYS_INLINE tlx_rf_rx_isr(const struct device *dev)
{
	struct tlx_data *tlx = dev->data;
	int status = -EINVAL;
	struct net_pkt *pkt = NULL;

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	uint64_t rx_time = k_ticks_to_us_near64(k_uptime_ticks());
#if CONFIG_SOC_RISCV_TELINK_TL321X
	uint32_t delta_time = (stimer_get_tick() - ZB_RADIO_TIMESTAMP_GET(tlx->rx_buffer)) /
		SYSTEM_TIMER_TICK_1US;
#endif
#if CONFIG_SOC_RISCV_TELINK_TL721X
	uint32_t delta_time = (rf_bb_timer_get_tick() - ZB_RADIO_TIMESTAMP_GET(tlx->rx_buffer)) /
		BB_TIMER_TICK_1US;
#endif
	rx_time -= delta_time;
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */

	dma_chn_dis(DMA1);
	rf_clr_irq_status(FLD_RF_IRQ_RX);

	do {
		if (!rf_zigbee_packet_crc_ok(tlx->rx_buffer)) {
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_INVALID_FCS;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		uint8_t length = tlx->rx_buffer[TLX_LENGTH_OFFSET];

		if ((length < TLX_PAYLOAD_MIN) || (length > TLX_PAYLOAD_MAX)) {
			LOG_ERR("Invalid length.\n");
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_NOT_RECEIVED;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		uint8_t *payload = (tlx->rx_buffer + TLX_PAYLOAD_OFFSET);
		struct ieee802154_frame frame;

		if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) ||
			IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
			tlx_ieee802154_frame_parse(payload, length - TLX_FCS_LENGTH, &frame);
		} else {
			length -= TLX_FCS_LENGTH;
			tlx_ieee802154_frame_parse(payload, length, &frame);
		}
		if (!frame.general.valid) {
			LOG_ERR("Invalid frame\n");
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_NOT_RECEIVED;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		if (frame.general.type == IEEE802154_FRAME_FCF_TYPE_ACK) {
			if (tlx->ack_handler_en) {
				if (tlx->ack_sn == *frame.sn) {
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
					tlx_handle_ack(dev, payload, length, rx_time);
#else
					tlx_handle_ack(dev, payload, length, 0);
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
				}
			}
			break;
		}
		if (!tlx_run_filter(dev, &frame)) {
			LOG_DBG("Packet received is not addressed to me.");
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_ADDR_FILTERED;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		bool frame_pending = false;

		if (frame.general.ack_req) {
#ifdef CONFIG_OPENTHREAD_FTD
			if (tlx_require_pending_bit(&frame)) {
				if (frame.src_addr) {
					if (!tlx->src_match_table->enabled ||
						tlx_src_match_table_search(tlx->src_match_table,
							frame.src_addr, frame.src_addr_ext)) {
						frame_pending = true;
					}
				}
			}
#endif /* CONFIG_OPENTHREAD_FTD */
			bool enh_ack = (frame.general.ver == IEEE802154_FRAME_FCF_VER_2015);
			uint8_t *ack_ie_header = NULL;
			size_t ack_ie_header_len = 0;
#if CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
			if (enh_ack) {
				int idx = tlx_enh_ack_table_search(tlx->enh_ack_table,
					frame.src_addr_ext ? NULL : frame.src_addr,
					frame.src_addr_ext ? frame.src_addr : NULL);
				if (idx >= 0) {
					ack_ie_header =
						(uint8_t *)&tlx->enh_ack_table->item[idx].ie_header;
					ack_ie_header_len = sizeof(struct ieee802154_header_ie);
				}
			}
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */
			struct ieee802154_frame ack_frame = {
				.general = {
					.valid = true,
					.ver = enh_ack ? IEEE802154_FRAME_FCF_VER_2015 :
						IEEE802154_FRAME_FCF_VER_2003,
					.type = IEEE802154_FRAME_FCF_TYPE_ACK,
					.fp_bit = frame_pending
				},
				.sn = frame.sn,
				.dst_panid = enh_ack ?
					(frame.src_panid ? frame.src_panid : frame.dst_panid) :
					NULL,
				.dst_addr = enh_ack ? frame.src_addr : NULL,
				.dst_addr_ext = enh_ack ? frame.src_addr_ext : false,
				.payload = ack_ie_header,
				.payload_len = ack_ie_header_len,
				.payload_ie = true
			};
			tlx_send_ack(dev, &ack_frame);
		}
		pkt = net_pkt_rx_alloc_with_buffer(tlx->iface, length, AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("No pkt available.");
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_OTHER;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		net_pkt_set_ieee802154_ack_fpb(pkt, frame_pending);
		if (net_pkt_write(pkt, payload, length)) {
			LOG_ERR("Failed to write to a packet.");
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_OTHER;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		tlx_update_rssi_and_lqi(dev, pkt);
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
		struct net_ptp_time timestamp = {
			.second = rx_time / USEC_PER_SEC,
			.nanosecond = (rx_time % USEC_PER_SEC) * NSEC_PER_USEC
		};
		net_pkt_set_timestamp(pkt, &timestamp);
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		status = net_recv_data(tlx->iface, pkt);
		if (status < 0) {
			LOG_ERR("RCV Packet dropped by NET stack: %d", status);
			if (tlx->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_OTHER;

				tlx->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
		}
	} while (0);

	if (status < 0 && pkt != NULL) {
		net_pkt_unref(pkt);
	}
	dma_chn_en(DMA1);
}

/* TX IRQ handler */
static ALWAYS_INLINE void tlx_rf_tx_isr(const struct device *dev)
{
	struct tlx_data *tlx = dev->data;

	/* clear irq status */
	rf_clr_irq_status(FLD_RF_IRQ_TX);

	/* ack sent */
	tlx->ack_sending = false;

	/* release tx semaphore */
	k_sem_give(&tlx->tx_wait);

	/* set to rx mode */
	rf_set_rxmode();
}

/* IRQ handler */
static void __GENERIC_SECTION(.ram_code) tlx_rf_isr(const struct device *dev)
{
	if (rf_get_irq_status(FLD_RF_IRQ_RX)) {
		tlx_rf_rx_isr(dev);
	} else if (rf_get_irq_status(FLD_RF_IRQ_TX)) {
		tlx_rf_tx_isr(dev);
	} else {
		rf_clr_irq_status(FLD_RF_IRQ_ALL);
	}
}

/* Driver initialization */
static int tlx_init(const struct device *dev)
{
	struct tlx_data *tlx = dev->data;

	/* init semaphores */
	k_sem_init(&tlx->tx_wait, 0, 1);
	k_sem_init(&tlx->ack_wait, 0, 1);

	/* init IRQs */
#ifndef CONFIG_DYNAMIC_INTERRUPTS
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), tlx_rf_isr,
		DEVICE_DT_INST_GET(0), 0);
	riscv_plic_set_priority(DT_INST_IRQN(0), DT_INST_IRQ(0, priority));
#endif /* not CONFIG_DYNAMIC_INTERRUPTS */

	/* init data variables */
	tlx->is_started = false;
	tlx->ack_handler_en = false;
	tlx->ack_sending = false;
	tlx->current_channel = TLX_TX_CH_NOT_SET;
	tlx->current_dbm = TLX_TX_PWR_NOT_SET;
#ifdef CONFIG_OPENTHREAD_FTD
	tlx_src_match_table_clean(tlx->src_match_table);
	tlx->src_match_table->enabled = true;
#endif /* CONFIG_OPENTHREAD_FTD */
#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
	tlx_enh_ack_table_clean(tlx->enh_ack_table);
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */
	tlx->event_handler = NULL;
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
	tlx_mac_keys_data_clean(tlx->mac_keys);
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */
	return 0;
}

/* API implementation: iface_init */
static void tlx_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct tlx_data *tlx = dev->data;
	uint8_t *mac = tlx_get_mac(dev);

	net_if_set_link_addr(iface, mac, IEEE802154_FRAME_LENGTH_ADDR_EXT, NET_LINK_IEEE802154);

	tlx->iface = iface;

	ieee802154_init(iface);
}

/* API implementation: get_capabilities */
static enum ieee802154_hw_caps tlx_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	enum ieee802154_hw_caps caps = IEEE802154_HW_FCS |
		IEEE802154_HW_FILTER |
		IEEE802154_HW_TX_RX_ACK;

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	caps |= IEEE802154_HW_TXTIME;
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
	caps |= IEEE802154_HW_TX_SEC;
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */
	return caps;
}

/* API implementation: cca */
static int tlx_cca(const struct device *dev)
{
	ARG_UNUSED(dev);

	unsigned int t1 = stimer_get_tick();

	while (!clock_time_exceed(t1, TLX_CCA_TIME_MAX_US)) {
		if (rf_get_rssi() < CONFIG_IEEE802154_TLX_CCA_RSSI_THRESHOLD) {
			return 0;
		}
	}

	return -EBUSY;
}

/* API implementation: set_channel */
static int tlx_set_channel(const struct device *dev, uint16_t channel)
{
	struct tlx_data *tlx = dev->data;

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	if (tlx->current_channel != channel) {
		tlx->current_channel = channel;
		if (tlx->is_started) {
			rf_set_chn(TLX_LOGIC_CHANNEL_TO_PHYSICAL(channel));
			rf_set_txmode();
			rf_set_rxmode();
		}
	}

	return 0;
}

/* API implementation: filter */
static int tlx_filter(const struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter)
{
	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return tlx_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return tlx_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return tlx_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

/* API implementation: set_txpower */
static int tlx_set_txpower(const struct device *dev, int16_t dbm)
{
	struct tlx_data *tlx = dev->data;

	/* check for supported Min/Max range */
	if (dbm < TL_TX_POWER_MIN) {
		dbm = TL_TX_POWER_MIN;
	} else if (dbm > TL_TX_POWER_MAX) {
		dbm = TL_TX_POWER_MAX;
	}

	if (tlx->current_dbm != dbm) {
		tlx->current_dbm = dbm;
		/* set TX power */
		if (tlx->is_started) {
			rf_set_power_level(tl_tx_pwr_lt[dbm - TL_TX_POWER_MIN]);
		}
	}

	return 0;
}

volatile bool tlx_rf_zigbee_250K_mode;

/* API implementation: start */
static int tlx_start(const struct device *dev)
{
	struct tlx_data *tlx = dev->data;

	tlx_disable_pm(dev);
	/* check if RF is already started */
	if (!tlx->is_started) {
#ifdef CONFIG_DYNAMIC_INTERRUPTS
		irq_connect_dynamic(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			(void (*)(const void *))tlx_rf_isr, DEVICE_DT_INST_GET(0), 0);
		riscv_plic_set_priority(DT_INST_IRQN(0), DT_INST_IRQ(0, priority));
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
		if (!tlx_rf_zigbee_250K_mode) {
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
			ske_dig_en();
#endif
#endif
			rf_mode_init();
			rf_set_zigbee_250K_mode();
			tlx_rf_zigbee_250K_mode = true;
		}
		rf_set_tx_dma(1, TLX_TRX_LENGTH);
		rf_set_rx_dma(tlx->rx_buffer, 0, TLX_TRX_LENGTH);
		if (tlx->current_channel != TLX_TX_CH_NOT_SET) {
			rf_set_chn(TLX_LOGIC_CHANNEL_TO_PHYSICAL(tlx->current_channel));
		}
		if (tlx->current_dbm != TLX_TX_PWR_NOT_SET) {
			rf_set_power_level(tl_tx_pwr_lt[tlx->current_dbm - TL_TX_POWER_MIN]);
		}
		rf_set_irq_mask(FLD_RF_IRQ_RX | FLD_RF_IRQ_TX);
		riscv_plic_irq_enable(DT_INST_IRQN(0));
		rf_set_txmode();
		rf_set_rxmode();
		tlx->is_started = true;
	}

	return 0;
}

/* API implementation: stop */
static int tlx_stop(const struct device *dev)
{
	struct tlx_data *tlx = dev->data;

	/* check if RF is already stopped */
	if (tlx->is_started) {
		if (tlx->ack_sending) {
			if (k_sem_take(&tlx->tx_wait, K_MSEC(TLX_TX_WAIT_TIME_MS)) != 0) {
				tlx->ack_sending = false;
			}
		}
		riscv_plic_irq_disable(DT_INST_IRQN(0));
		rf_set_tx_rx_off();
#ifdef CONFIG_PM_DEVICE
#if CONFIG_SOC_RISCV_TELINK_TL321X
		rf_baseband_reset();
		rf_reset_dma();
#elif CONFIG_SOC_RISCV_TELINK_TL721X
		rf_radio_reset();
#endif
		tlx_rf_zigbee_250K_mode = false;
#endif /* CONFIG_PM_DEVICE */
		tlx->is_started = false;
	}
	tlx_enable_pm(dev);

	return 0;
}

/* API implementation: tx */
static int tlx_tx(const struct device *dev,
		  enum ieee802154_tx_mode mode,
		  struct net_pkt *pkt,
		  struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	int status = 0;
	struct tlx_data *tlx = dev->data;

	/* check for supported mode */
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	if (mode != IEEE802154_TX_MODE_DIRECT &&
		mode != IEEE802154_TX_MODE_TXTIME_CCA) {
#else
	if (mode != IEEE802154_TX_MODE_DIRECT) {
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		LOG_WRN("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	if (tlx->ack_sending) {
		if (k_sem_take(&tlx->tx_wait, K_MSEC(TLX_TX_WAIT_TIME_MS)) != 0) {
			tlx->ack_sending = false;
			rf_set_rxmode();
		}
	}

#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION

	struct ieee802154_frame frame;
	uint8_t key_id = 0;

	tlx_ieee802154_frame_parse(frag->data, frag->len, &frame);

	do {

		if (net_pkt_ieee802154_mac_hdr_rdy(pkt)) {
			LOG_WRN("The packet is encrypted and sent directly\n");
			break;
		}

		net_pkt_set_ieee802154_frame_secured(pkt, false);
		net_pkt_set_ieee802154_mac_hdr_rdy(pkt, false);

		if (!frame.general.valid) {
			LOG_WRN("invalid frame\n");
			break;
		}

		if (!frame.sec_header) {
			break;
		}

		const uint8_t sec_level =
				frame.sec_header[0] & IEEE802154_FRAME_SECCTRL_SEC_LEVEL_MASK;

		if (sec_level == IEEE802154_FRAME_SECCTRL_SEC_LEVEL_0) {
			break;
		}

		net_pkt_set_ieee802154_frame_secured(pkt, true);

		const uint8_t *src_addr = frame.src_addr_ext ? frame.src_addr :
			tlx->filter_ieee_addr;

		if (!src_addr) {
			LOG_WRN("no extended source address");
			break;
		}

		switch (frame.sec_header[0] & IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_MASK) {
		case IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_1:
			key_id = frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER_MODE_1];
			break;
		case IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_2:
			key_id = frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER_MODE_2];
			break;
		case IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_3:
			key_id = frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER_MODE_3];
			break;
		default:
			break;
		}

		if (key_id == THREAD_DEFAULT_KEY_ID_MODE_2_KEY_INDEX) {
			key_id = 0;
			break;
		}

		const uint8_t *key = tlx_mac_keys_get(tlx->mac_keys, key_id);

		if (!key) {
			key_id = 0;
			LOG_WRN("security key not found");
			break;
		}

		uint8_t *frame_cnt =
			(uint8_t *)&frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER];

		frame_cnt[0] = tlx_mac_keys_frame_cnt_get(tlx->mac_keys, key_id);
		frame_cnt[1] = tlx_mac_keys_frame_cnt_get(tlx->mac_keys, key_id) >> 8;
		frame_cnt[2] = tlx_mac_keys_frame_cnt_get(tlx->mac_keys, key_id) >> 16;
		frame_cnt[3] = tlx_mac_keys_frame_cnt_get(tlx->mac_keys, key_id) >> 24;

		net_pkt_set_ieee802154_mac_hdr_rdy(pkt, true);

		const uint8_t tag_size[] = {4, 8, 16};

		switch (sec_level) {
		case IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5:
		case IEEE802154_FRAME_SECCTRL_SEC_LEVEL_6:
		case IEEE802154_FRAME_SECCTRL_SEC_LEVEL_7:
			do {

				size_t tag_len =
					tag_size[sec_level - IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5];
				const uint8_t *open_data = frame.header;
				uint8_t *private_data = (uint8_t *)frame.payload;
				uint8_t *tag_data = frame.payload ?
					(uint8_t *)&frame.payload[frame.payload_len] : NULL;

				if (private_data && tag_data &&
					tag_data - private_data >= tag_len) {
					/* Adjust tag */
					tag_data -= tag_len;
					private_data = (tag_data > private_data) ?
						private_data : NULL;
				} else {
					key_id = 0;
					LOG_WRN("invalid payload length MIC");
					break;
				}

				if (frame.payload_ie) {
					/* IE header should be open */
					if (private_data) {
						private_data = (uint8_t *)tlx_ieee802154_get_data(
							private_data, tag_data - private_data);
						private_data = (private_data &&
							tag_data > private_data) ?
								private_data : NULL;
					} else {
						key_id = 0;
						LOG_WRN("invalid payload length IE");
						break;
					}

				}

				if (frame.general.ver < IEEE802154_FRAME_FCF_VER_2015 &&
					frame.general.type == IEEE802154_FRAME_FCF_TYPE_CMD) {
					/* command id should be open
					 * if frame version less than 2015
					 */
					if (private_data) {
						private_data++;
						private_data = (private_data &&
							tag_data > private_data) ?
								private_data : NULL;
					} else {
						key_id = 0;
						LOG_WRN("invalid payload length CID");
						break;
					}
				}

				/* here open_data && tag_data - valid, private_data possible NULL */
				if (!ieee802154_tlx_crypto_encrypt(key, src_addr,
						tlx_mac_keys_frame_cnt_get(tlx->mac_keys, key_id),
						sec_level,
						open_data, private_data ?
							private_data - open_data :
							tag_data - open_data,
						private_data, private_data ?
							tag_data - private_data : 0,
						private_data, tag_data, tag_len)) {
					key_id = 0;
					LOG_WRN("encrypt failed %u", sec_level);
				}

			} while (0);
			break;
		default:
			key_id = 0;
			LOG_WRN("unsupported security level %u", sec_level);
			break;
		}

	} while (0);

#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */

	/* prepare tx buffer */
	tlx_set_tx_payload(dev, frag->data, frag->len);

	/* reset semaphores */
	k_sem_reset(&tlx->tx_wait);
	k_sem_reset(&tlx->ack_wait);

	/* start transmission */
	rf_set_txmode();

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	if (mode == IEEE802154_TX_MODE_TXTIME_CCA) {
		k_sleep(K_TIMEOUT_ABS_TICKS(k_ns_to_ticks_near64(net_pkt_txtime(pkt))));
	} else
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
	{
		delay_us(CONFIG_IEEE802154_TLX_SET_TXRX_DELAY_US);
	}
	rf_tx_pkt(tlx->tx_buffer);
	if (tlx->event_handler) {
		tlx->event_handler(dev, IEEE802154_EVENT_TX_STARTED, (void *)frag);
	}

	/* wait for tx done */
	if (k_sem_take(&tlx->tx_wait, K_MSEC(TLX_TX_WAIT_TIME_MS)) != 0) {
		rf_set_rxmode();
		status = -EIO;
	}

	/* wait for ACK if requested */
	if (!status && (frag->data[0] & IEEE802154_FRAME_FCF_ACK_REQ_MASK) ==
		IEEE802154_FRAME_FCF_ACK_REQ_ON) {
		tlx->ack_sn = frag->data[IEEE802154_FRAME_LENGTH_FCF];
		tlx->ack_handler_en = true;
		if (k_sem_take(&tlx->ack_wait, K_MSEC(TLX_ACK_WAIT_TIME_MS)) != 0) {
			status = -ENOMSG;
		}
		tlx->ack_handler_en = false;
	}
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
	if (!status) {
		tlx_mac_keys_frame_cnt_inc(tlx->mac_keys, key_id);
	}
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */

	return status;
}

/* API implementation: ed_scan */
static int tlx_ed_scan(const struct device *dev, uint16_t duration,
		       energy_scan_done_cb_t done_cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(duration);
	ARG_UNUSED(done_cb);

	/* ed_scan not supported */

	return -ENOTSUP;
}

/* API implementation: configure */
static int tlx_configure(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config)
{
	struct tlx_data *tlx = dev->data;
	int result = 0;

	switch (type) {
#ifdef CONFIG_OPENTHREAD_FTD
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		if (config->auto_ack_fpb.mode == IEEE802154_FPB_ADDR_MATCH_THREAD) {
			tlx->src_match_table->enabled = config->auto_ack_fpb.enabled;
		} else {
			result = -ENOTSUP;
		}
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.addr) {
			if (config->ack_fpb.enabled) {
				tlx_src_match_table_add(tlx->src_match_table,
					config->ack_fpb.addr, config->ack_fpb.extended);
			} else {
				tlx_src_match_table_remove(tlx->src_match_table,
					config->ack_fpb.addr, config->ack_fpb.extended);
			}
		} else if (!config->ack_fpb.enabled) {
			tlx_src_match_table_remove_group(tlx->src_match_table,
				config->ack_fpb.extended);
		} else {
			result = -ENOTSUP;
		}
		break;
#endif /* CONFIG_OPENTHREAD_FTD */
#ifdef CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		{
			uint8_t short_addr[IEEE802154_FRAME_LENGTH_ADDR_SHORT];
			uint8_t ext_addr[IEEE802154_FRAME_LENGTH_ADDR_EXT];

			sys_put_le16(config->ack_ie.short_addr, short_addr);
			sys_memcpy_swap(ext_addr, config->ack_ie.ext_addr,
				IEEE802154_FRAME_LENGTH_ADDR_EXT);
			if (!config->ack_ie.purge_ie) {
				tlx_enh_ack_table_add(tlx->enh_ack_table,
					short_addr, ext_addr,
					config->ack_ie.header_ie);
			} else {
				tlx_enh_ack_table_remove(tlx->enh_ack_table,
					short_addr, ext_addr);
			}
		}
		break;
#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */
	case IEEE802154_CONFIG_EVENT_HANDLER:
		tlx->event_handler = config->event_handler;
		break;
#ifdef CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION
	case IEEE802154_CONFIG_MAC_KEYS:
		{
			uint32_t cnt = tlx->mac_keys->frame_cnt;

			tlx_mac_keys_data_clean(tlx->mac_keys);
			tlx->mac_keys->frame_cnt = cnt;
			for (size_t i = 0; config->mac_keys[i].key_value; i++) {
				if (i < TLX_MAC_KEYS_ITEMS) {
					memcpy(tlx->mac_keys->item[i].key,
						config->mac_keys[i].key_value,
						IEEE802154_CRYPTO_LENGTH_AES_BLOCK);
					tlx->mac_keys->item[i].frame_cnt_local =
					config->mac_keys[i].frame_counter_per_key;
					tlx->mac_keys->item[i].key_id =
						*config->mac_keys[i].key_id;
				} else {
					LOG_WRN("can't save key id %u",
						*config->mac_keys[i].key_id);
				}
			}
		}
		break;
	case IEEE802154_CONFIG_FRAME_COUNTER:
		tlx->mac_keys->frame_cnt = config->frame_counter;
		break;
#endif /* CONFIG_IEEE802154_TELINK_TLX_ENCRYPTION */
	default:
		LOG_WRN("Unhandled cfg %d", type);
		result = -ENOTSUP;
		break;
	}

	return result;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

/* API implementation: attr_get */
static int tlx_attr_get(const struct device *dev, enum ieee802154_attr attr,
			struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

/* API implementation: get_sch_acc */
static uint8_t tlx_get_sch_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_TLX_DELAY_TRX_ACC;
}

/* IEEE802154 driver APIs structure */
static struct ieee802154_radio_api tlx_radio_api = {
	.iface_api.init = tlx_iface_init,
	.get_capabilities = tlx_get_capabilities,
	.cca = tlx_cca,
	.set_channel = tlx_set_channel,
	.filter = tlx_filter,
	.set_txpower = tlx_set_txpower,
	.start = tlx_start,
	.stop = tlx_stop,
	.tx = tlx_tx,
	.ed_scan = tlx_ed_scan,
	.configure = tlx_configure,
	.attr_get = tlx_attr_get,
	.get_sch_acc = tlx_get_sch_acc,
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

/* IEEE802154 driver registration */
#if defined(CONFIG_NET_L2_IEEE802154) || defined(CONFIG_NET_L2_OPENTHREAD)
NET_DEVICE_DT_INST_DEFINE(0, tlx_init, NULL, &data, NULL,
			  CONFIG_IEEE802154_TLX_INIT_PRIO,
			  &tlx_radio_api, L2, L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, tlx_init, NULL, &data, NULL,
		      POST_KERNEL, CONFIG_IEEE802154_TLX_INIT_PRIO,
		      &tlx_radio_api);
#endif
