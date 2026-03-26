/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_util_hci_evt);

#include "util_hci_evt.h"

static bool is_tracked_chain(const struct hci_ext_adv_discard_ctx *ctx,
			     const bt_addr_le_t *addr, uint8_t sid)
{
	return ctx->active && (ctx->sid == sid) && bt_addr_le_eq(&ctx->addr, addr);
}

/* Claim the tracker (ctx) for (addr, sid). If the slot is already held by a different advertiser,
 * evict it and log a warning. The evicted chain's remaining fragments will leak
 * into the host, but this prevents the slot from getting permanently stuck on a
 * chain whose terminator never arrives.
 */
static void track_discarded_chain(struct hci_ext_adv_discard_ctx *ctx,
				  const bt_addr_le_t *addr, uint8_t sid)
{
	if (ctx->active && (ctx->sid != sid || !bt_addr_le_eq(&ctx->addr, addr))) {
		LOG_WRN("Evicting tracked chain (addr %s sid %u) to track (addr %s sid %u)",
			bt_addr_le_str(&ctx->addr), ctx->sid, bt_addr_le_str(addr), sid);
	}

	bt_addr_le_copy(&ctx->addr, addr);
	ctx->sid = sid;
	ctx->active = true;
}

struct ext_adv_report_kept_entry {
	uint16_t offset;
	uint16_t size;
};

bool hci_ext_adv_report_process(struct hci_ext_adv_discard_ctx *ctx, const uint8_t *data,
				size_t len, struct net_buf **out)
{
	const struct bt_hci_evt_hdr *hdr;
	const struct bt_hci_evt_le_meta_event *meta;
	const struct bt_hci_evt_le_ext_advertising_report *report;
	struct ext_adv_report_kept_entry kept_entries[BT_HCI_LE_EXT_ADV_REPORT_MAX_NUM_REPORTS];
	struct net_buf *buf;
	size_t buf_tailroom;
	size_t offset;
	size_t repack_len;
	uint8_t kept_cnt;
	uint8_t num_reports;

	__ASSERT_NO_MSG(out != NULL && ctx != NULL);
	__ASSERT_NO_MSG(*out == NULL);

	if (len < sizeof(*hdr) + sizeof(*meta) + sizeof(*report)) {
		return false;
	}

	hdr = (const void *)data;
	meta = (const void *)&data[sizeof(*hdr)];
	if (hdr->evt != BT_HCI_EVT_LE_META_EVENT ||
	    meta->subevent != BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT) {
		return false;
	}

	if ((size_t)hdr->len + sizeof(*hdr) != len) {
		LOG_ERR("Event payload length is not correct (%zu != %u)", len - sizeof(*hdr),
			hdr->len);
		return false;
	}

	report = (const void *)&data[sizeof(*hdr) + sizeof(*meta)];
	num_reports = report->num_reports;
	if (num_reports == 0U || num_reports > BT_HCI_LE_EXT_ADV_REPORT_MAX_NUM_REPORTS) {
		LOG_ERR("Unexpected num_reports %u", num_reports);
		return false;
	}

	/* Ensure that the output is correctly presented when asserts are disabled. */
	*out = NULL;

	/* Try to allocate the output buffer up front. If NULL, every report that
	 * isn't already being dropped by chain tracking will be dropped too, and
	 * any new PARTIAL chain is registered for tracking.
	 */
	buf = bt_buf_get_evt(hdr->evt, true, K_NO_WAIT);

	/* Iterate over each report; validate its bounds, then classify whether to keep or drop
	 * the report. If no output buffer was available, drop unconditionally.
	 */
	offset = sizeof(*hdr) + sizeof(*meta) + sizeof(*report);
	kept_cnt = 0U;

	for (uint8_t i = 0; i < num_reports; i++) {
		const struct bt_hci_evt_le_ext_advertising_info *info;
		size_t info_size;
		uint16_t data_status;

		if (len - offset < sizeof(*info)) {
			goto malformed;
		}

		info = (const void *)&data[offset];
		info_size = sizeof(*info) + info->length;
		if (len - offset < info_size) {
			goto malformed;
		}

		data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(sys_le16_to_cpu(info->evt_type));

		if (is_tracked_chain(ctx, &info->addr, info->sid)) {
			LOG_DBG("Dropping tracked chain fragment addr %s sid %u status %u",
				bt_addr_le_str(&info->addr), info->sid, data_status);
			if (data_status != BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL) {
				LOG_DBG("Tracked ext adv chain discard completed");
				ctx->active = false;
			}
		} else if (buf == NULL) {
			LOG_DBG("No buffer for ext adv report, addr %s sid %u status %u",
				bt_addr_le_str(&info->addr), info->sid, data_status);
			if (data_status == BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL) {
				track_discarded_chain(ctx, &info->addr, info->sid);
			}
		} else {
			kept_entries[kept_cnt].offset = (uint16_t)offset;
			kept_entries[kept_cnt].size = (uint16_t)info_size;
			kept_cnt++;
		}

		offset += info_size;
	}

	/* kept_cnt == 0 implies either every report was tracked and dropped (buf still
	 * allocated and needs freeing) or we failed to allocate a buffer (buf == NULL).
	 */
	if (kept_cnt == 0U) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		LOG_DBG("Dropped all %u reports", num_reports);
		return true;
	}

	buf_tailroom = net_buf_tailroom(buf);

	/* Every report is kept. */
	if (kept_cnt == num_reports) {
		if (buf_tailroom < len) {
			LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
			net_buf_unref(buf);
			return true;
		}

		net_buf_add_mem(buf, data, len);
		*out = buf;
		return true;
	}

	/* Rebuild the event with only the kept reports. */
	repack_len = sizeof(*hdr) + sizeof(*meta) + sizeof(*report);
	for (uint8_t i = 0; i < kept_cnt; i++) {
		repack_len += kept_entries[i].size;
	}

	if (buf_tailroom < repack_len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", repack_len, buf_tailroom);
		net_buf_unref(buf);
		return true;
	}

	net_buf_add_u8(buf, BT_HCI_EVT_LE_META_EVENT);
	net_buf_add_u8(buf, (uint8_t)(repack_len - sizeof(*hdr)));
	net_buf_add_u8(buf, BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT);
	net_buf_add_u8(buf, kept_cnt);

	for (uint8_t i = 0; i < kept_cnt; i++) {
		net_buf_add_mem(buf, &data[kept_entries[i].offset], kept_entries[i].size);
	}

	LOG_DBG("Repacked ext adv report: kept %u of %u", kept_cnt, num_reports);
	*out = buf;
	return true;

malformed:
	if (buf != NULL) {
		net_buf_unref(buf);
	}
	return false;
}
