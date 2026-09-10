/* Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "util_hci_evt.h"

#define DEFAULT_DATA_LEN 100U
#define DEFAULT_SID      5U

static struct net_buf *allocated_bufs[CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT];

static void validate_all_evt_buffers_available(const char *msg)
{
	struct net_buf_pool *pool;
	struct net_buf *buf;
	uint8_t pool_id;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, true, K_NO_WAIT);
	zassert_not_null(buf, msg);

	pool_id = buf->pool_id;
	net_buf_unref(buf);

	pool = net_buf_pool_get(pool_id);
	zassert_equal(net_buf_get_available(pool), CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT,
		      "Expected all buffers to be available: %s", msg);
}

static void test_setup(void *fixture)
{
	ARG_UNUSED(fixture);

	validate_all_evt_buffers_available("test setup");
}

static void test_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(allocated_bufs); i++) {
		zassert_is_null(allocated_bufs[i], "Expected all buffers to be freed at test end");
	}

	validate_all_evt_buffers_available("test teardown");
}

static ZTEST_SUITE(hci_ext_adv_report_process, NULL, NULL, test_setup, test_teardown, NULL);

static size_t build_adv_ext_report(uint8_t *hci_buf, uint8_t sid, uint8_t data_status)
{
	struct bt_hci_evt_hdr *hdr = (void *)hci_buf;
	struct bt_hci_evt_le_meta_event *meta = (void *)&hdr->data[0];
	struct bt_hci_evt_le_ext_advertising_report *report = (void *)&meta->data[0];
	struct bt_hci_evt_le_ext_advertising_info *info = (void *)&report->adv_info[0];
	uint8_t *adv_data;

	(void)memset(hci_buf, 0, CONFIG_BT_BUF_EVT_RX_SIZE);

	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*report) + sizeof(*info) + DEFAULT_DATA_LEN;

	meta->subevent = BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT;

	report->num_reports = 1;

	/* Data status is part of event type.
	 * See Core_v6.3, Vol 4, Part E, Section 7.7.65.13.
	 */
	info->evt_type = (uint16_t)(data_status << 5U);

	info->sid = sid;
	info->length = DEFAULT_DATA_LEN;
	adv_data = (uint8_t *)&info->data[0];
	for (size_t i = 0; i < DEFAULT_DATA_LEN; i++) {
		adv_data[i] = (uint8_t)i;
	}

	return hdr->len + sizeof(*hdr);
}

static void allocate_all_bufs(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(allocated_bufs); i++) {
		allocated_bufs[i] = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, true, K_NO_WAIT);
		zassert_not_null(allocated_bufs[i]);
	}

	/* Check that all buffers are allocated */
	zassert_is_null(bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, true, K_NO_WAIT));
}

static void free_all_bufs(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(allocated_bufs); i++) {
		net_buf_unref(allocated_bufs[i]);
		allocated_bufs[i] = NULL;
	}
}

/* This function is a wrapper of hci_ext_adv_report_process, also handling building of HCI packets
 */
static bool process_adv_ext_report(struct hci_ext_adv_discard_ctx *ctx, uint8_t sid,
				   uint8_t data_status, struct net_buf **out)
{
	uint8_t hci_buf[CONFIG_BT_BUF_EVT_RX_SIZE];
	size_t hci_buf_len;

	hci_buf_len = build_adv_ext_report(hci_buf, sid, data_status);
	return hci_ext_adv_report_process(ctx, hci_buf, hci_buf_len, out);
}

static struct bt_hci_evt_le_ext_advertising_info *info_from_report(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_le_ext_advertising_report *report;

	hdr = (void *)&buf->data[1];
	meta = (void *)&hdr->data[0];
	report = (void *)&meta->data[0];

	return &report->adv_info[0];
}

static ZTEST(hci_ext_adv_report_process, test_process_complete_buffer_available)
{
	/* Validate that a complete report is processed correctly when a buffer is available. */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE);
	net_buf_unref(buf);
}

static ZTEST(hci_ext_adv_report_process, test_process_complete_buffer_not_available)
{
	/* Validate that a complete report is processed correctly when no buffer is available.
	 * That is, there is no output buffer which will be forwarded to the host.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	allocate_all_bufs();
	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval, "Expected function to succeed even when no buffer available");
	zassert_is_null(buf);

	/* Test another */
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf);

	free_all_bufs();

	/* Test success */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE);
	net_buf_unref(buf);
}

static ZTEST(hci_ext_adv_report_process, test_process_chain_status_success)
{
	/* Validate that a chain of reports where the last report marks a success
	 * is processed correctly when buffers are available for all reports.
	 * That is, the complete chain of reports is forwarded to the host.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));
	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_not_null(buf, "Expected buffer to be allocated for partial report");
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval);
	zassert_not_null(buf, "Expected buffer to be allocated for complete report");
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE);
	net_buf_unref(buf);
}

static ZTEST(hci_ext_adv_report_process, test_process_chain_status_incomplete)
{
	/* Validate that a chain of reports where the last report marks a truncation
	 * is processed correctly when buffers are available for all reports.
	 * That is, the complete chain of reports is forwarded to the host.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_not_null(buf, "Expected buffer to be allocated for partial report");
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	zassert_not_null(buf, "Expected buffer to be allocated for truncated report");
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE);
	net_buf_unref(buf);
}

static ZTEST(hci_ext_adv_report_process, test_process_chain_status_success_first_buffer_unavailable)
{
	/* Validate that a chain of reports is handled where we fail to allocate a buffer for the
	 * first report. The remaining reports of the chain should be dropped.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	allocate_all_bufs();
	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_is_null(buf);
	free_all_bufs();

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf,
			"Expect buffer to be consumed internally, first part was not reported.");
}

static ZTEST(hci_ext_adv_report_process,
	     test_process_chain_status_success_second_buffer_unavailable)
{
	/* Validate that a chain of reports is handled where we fail to allocate a buffer for the
	 * second report. That report should be dropped.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_not_null(buf, "Expected buffer to be allocated for partial report");
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	/* Second buffer is dropped. */
	allocate_all_bufs();
	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf);
	free_all_bufs();
}

static ZTEST(hci_ext_adv_report_process,
	     test_process_chain_status_incomplete_first_buffer_unavailable)
{
	/* Validate that a chain of reports is handled where we fail to allocate a buffer for the
	 * first report which has a PARTIAL status.
	 * The remaining reports of the chain should be dropped, even if the last report has an
	 * INCOMPLETE status.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	allocate_all_bufs();
	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_is_null(buf);
	free_all_bufs();

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf,
			"Expect buffer to be consumed internally, first part was not reported");
}

static ZTEST(hci_ext_adv_report_process,
	     test_process_chain_status_incomplete_second_buffer_unavailable)
{
	/* Validate that a chain of reports is handled where we fail to allocate a buffer for the
	 * second report which has a INCOMPLETE status.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	(void)memset(&ctx, 0, sizeof(ctx));

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_not_null(buf, "Expected buffer to be allocated for partial report");
	zassert_equal(info_from_report(buf)->sid, DEFAULT_SID);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	allocate_all_bufs();

	buf = NULL;
	retval = process_adv_ext_report(&ctx, DEFAULT_SID,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf,
			"Expected buffer to be consumed internally, first part was not reported.");
	free_all_bufs();
}

static ZTEST(hci_ext_adv_report_process,
	     two_interleaved_chains_first_chain_has_unavailable_first_buffer)
{
	/* Validate that two interleaved chains of reports:
	 * - Chain A
	 *   1. First report: PARTIAL, buffer unavailable
	 *   2. Second report: INCOMPLETE, buffer available (should be discarded)
	 * - Chain B (different SID)
	 *   3. First report: PARTIAL, buffer available
	 *   4. Second report: INCOMPLETE, buffer available
	 * - Chain A again
	 *   5. First report: PARTIAL, buffer available
	 *   6. Second report: PARTIAL, buffer available
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	const uint8_t sid_first = 3;
	const uint8_t sid_second = 4;

	(void)memset(&ctx, 0, sizeof(ctx));

	/* 1: Advertiser A: First partial report. Fail to fetch buffer for report */
	allocate_all_bufs();
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL,
					&buf);
	zassert_true(retval);
	zassert_is_null(buf);

	free_all_bufs();

	/* 2: Advertiser A: Last incomplete report. Do not generate report (consume internally) */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf);

	/* 3: Advertiser B: First partial report. */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_second,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, sid_second);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	/* 4: Advertiser B: Last incomplete report. */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_second,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, sid_second);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE);
	net_buf_unref(buf);

	/* 5: Advertiser A: New partial report */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL,
					&buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, sid_first);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	/* 6: Advertiser A: Last partial report */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL,
					&buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, sid_first);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);
}

static ZTEST(hci_ext_adv_report_process,
	     two_interleaved_chains_first_chain_has_unavailable_last_buffer)
{
	/* Validate that two interleaved chains of reports:
	 * - Chain A
	 *   1. First report: PARTIAL, buffer available
	 *   2. Second report: INCOMPLETE_NO_MORE_TO_COME, buffer unavailable
	 * - Chain B (different SID)
	 *   3. First report: PARTIAL, buffer available
	 *   4. Second report: INCOMPLETE_NO_MORE_TO_COME, buffer available
	 * - Chain A again
	 *   5. First report: PARTIAL, buffer available
	 *   6. Second report: INCOMPLETE_NO_MORE_TO_COME, buffer available
	 *
	 * Here we need to ensure that the two instances of Chain A are not combined into a single
	 * long chain of reports.
	 *
	 * TODO: Add validation according to how the implementation handles it when that is decided.
	 * See issue https://github.com/zephyrproject-rtos/zephyr/issues/111388 for details.
	 */

	struct hci_ext_adv_discard_ctx ctx;
	struct net_buf *buf;
	bool retval;

	const uint8_t sid_first = 3;
	const uint8_t sid_second = 4;

	(void)memset(&ctx, 0, sizeof(ctx));

	/* 1: Advertiser A: First partial report. */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL,
					&buf);
	zassert_true(retval);
	zassert_not_null(buf);
	zassert_equal(info_from_report(buf)->sid, sid_first);
	zassert_equal(info_from_report(buf)->length, DEFAULT_DATA_LEN);
	zassert_equal(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info_from_report(buf)->evt_type),
		      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL);
	net_buf_unref(buf);

	/* 2: Advertiser A: Last incomplete report. Fail to allocate data for it. */
	allocate_all_bufs();
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	zassert_is_null(buf);
	free_all_bufs();

	/* 3: Advertiser B: First partial report. */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_second,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, &buf);
	zassert_true(retval);
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	/* 4: Advertiser B: Last incomplete report. */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_second,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	/* 5: Advertiser A: New partial report */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL,
					&buf);
	zassert_true(retval);
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	/* 6: Advertiser A: Last incomplete report */
	buf = NULL;
	retval = process_adv_ext_report(&ctx, sid_first,
					BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE, &buf);
	zassert_true(retval);
	if (buf != NULL) {
		net_buf_unref(buf);
	}
}
