/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

#define TEST_NODE_ID 0x36U
#define DOMAIN_MAX   256U

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

struct test_domain {
	uint8_t data[DOMAIN_MAX];
	size_t len;
	size_t fail_at;
	size_t max;
	size_t written;
	size_t size;
	unsigned int calls;
	unsigned int data_calls;
	bool last;
};

static struct test_domain dom;
static uint8_t app_u8;

static uint32_t domain_read(const struct canopen_od_entry *entry, size_t offset, uint8_t *buf,
			    size_t *len, void *user_data)
{
	struct test_domain *d = user_data;

	ARG_UNUSED(entry);

	d->calls++;
	if ((d->fail_at != 0U) && (offset >= d->fail_at)) {
		return CANOPEN_SDO_ABORT_HARDWARE;
	}

	*len = MIN(*len, d->len - MIN(offset, d->len));
	memcpy(buf, &d->data[offset], *len);

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t domain_write(const struct canopen_od_entry *entry, size_t offset,
			     const uint8_t *buf, size_t len, size_t size, bool last,
			     void *user_data)
{
	struct test_domain *d = user_data;

	ARG_UNUSED(entry);

	d->calls++;
	if (len > 0U) {
		d->data_calls++;
	}

	if ((d->fail_at != 0U) && ((offset + len) > d->fail_at)) {
		return CANOPEN_SDO_ABORT_HARDWARE;
	}

	if ((d->max != 0U) && (size > d->max)) {
		return CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH;
	}

	if ((offset + len) > DOMAIN_MAX) {
		return CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
	}

	zassert_equal(offset, d->written, "data not in order");
	memcpy(&d->data[offset], buf, len);
	d->written = offset + len;
	d->size = size;
	d->last = last;

	return CANOPEN_SDO_ABORT_NONE;
}

static const struct canopen_od_domain test_domain_ops = {
	.read = domain_read,
	.write = domain_write,
};

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW | CANOPEN_OD_ATTR_TPDO,
		       app_u8),
	CANOPEN_OD_DOMAIN(0x5600, CANOPEN_OD_RW, test_domain_ops, &dom),
	CANOPEN_OD_DOMAIN(0x5601, CANOPEN_OD_RO, test_domain_ops, &dom),
	CANOPEN_OD_DOMAIN(0x5602, CANOPEN_OD_WO, test_domain_ops, &dom));

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;

static void domain_fill(size_t len)
{
	for (size_t i = 0U; i < DOMAIN_MAX; i++) {
		dom.data[i] = (uint8_t)(i * 7U + 1U);
	}
	dom.len = len;
}

static void expect_upload(size_t len)
{
	uint8_t buf[DOMAIN_MAX + 8U];
	size_t got = sizeof(buf);

	domain_fill(len);
	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x5600, 0U, buf, &got, NULL),
		   "upload of %zu bytes", len);
	zassert_equal(got, len, "got %zu bytes, expected %zu", got, len);
	zassert_mem_equal(buf, dom.data, len);
}

static void *domain_setup(void)
{
	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));

	return NULL;
}

static void domain_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&dom, 0, sizeof(dom));
}

ZTEST(canopen_domain, test_upload)
{
	expect_upload(3U);
	expect_upload(4U);
	expect_upload(5U);
	expect_upload(7U);
	expect_upload(14U);
	expect_upload(20U);
	expect_upload(200U);
}

ZTEST(canopen_domain, test_upload_empty)
{
	uint8_t buf[8];
	size_t got = sizeof(buf);

	domain_fill(0U);
	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x5600, 0U, buf, &got, NULL));
	zassert_equal(got, 0U);
}

ZTEST(canopen_domain, test_download)
{
	uint8_t src[DOMAIN_MAX];

	for (size_t i = 0U; i < sizeof(src); i++) {
		src[i] = (uint8_t)(0xA5U ^ i);
	}

	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5600, 0U, src, 3U, NULL));
	zassert_equal(dom.written, 3U);
	zassert_equal(dom.size, 3U);
	zassert_true(dom.last);
	zassert_equal(dom.calls, 1U);
	zassert_mem_equal(dom.data, src, 3U);

	memset(&dom, 0, sizeof(dom));
	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5600, 0U, src, 150U, NULL));
	zassert_equal(dom.written, 150U);
	zassert_equal(dom.size, 150U);
	zassert_true(dom.last);
	zassert_equal(dom.calls, 23U);
	zassert_mem_equal(dom.data, src, 150U);

	memset(&dom, 0, sizeof(dom));
	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5600, 0U, src, 14U, NULL));
	zassert_equal(dom.written, 14U);
	zassert_true(dom.last);
	zassert_equal(dom.calls, 3U);
}

ZTEST(canopen_domain, test_download_refused)
{
	uint8_t src[DOMAIN_MAX] = {0};
	uint32_t abort = 0U;

	dom.max = 100U;
	zassert_equal(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5600, 0U, src, 150U,
					   &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH);
	zassert_equal(dom.calls, 1U);
	zassert_equal(dom.data_calls, 0U);
	zassert_equal(dom.written, 0U);

	dom.calls = 0U;
	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5600, 0U, src, 100U, NULL));
	zassert_equal(dom.written, 100U);
	zassert_equal(dom.calls, 16U);
}

ZTEST(canopen_domain, test_callback_abort)
{
	uint8_t buf[DOMAIN_MAX];
	size_t got = sizeof(buf);
	uint32_t abort = 0U;

	domain_fill(100U);
	dom.fail_at = 30U;
	zassert_equal(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x5600, 0U, buf, &got, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_HARDWARE);

	dom.written = 0U;
	zassert_equal(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5600, 0U, buf, 60U,
					   &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_HARDWARE);
	zassert_equal(dom.written, 28U);
}

ZTEST(canopen_domain, test_access)
{
	uint8_t buf[8] = {0};
	size_t got = sizeof(buf);
	uint32_t abort = 0U;

	domain_fill(5U);
	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x5601, 0U, buf, &got, NULL));
	zassert_equal(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5601, 0U, buf, 2U, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_READ_ONLY);

	got = sizeof(buf);
	zassert_equal(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x5602, 0U, buf, &got, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_WRITE_ONLY);
	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x5602, 0U, buf, 2U, NULL));

	zassert_equal(canopen_od_read(&test_node, 0x5600, 0U, buf, 0U), -EACCES);
	zassert_equal(canopen_od_write(&test_node, 0x5600, 0U, buf, 0U), -EACCES);

	got = 2U;
	domain_fill(10U);
	zassert_equal(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x5600, 0U, buf, &got, &abort),
		      -ENOSPC);
}

ZTEST(canopen_domain, test_not_mappable)
{
	uint32_t abort = 0U;
	uint32_t map = CANOPEN_PDO_MAP(0x5600, 0, 8);
	uint8_t zero = 0U;

	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x1A00, 0U, &zero, 1U, NULL));
	zassert_equal(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x1A00, 1U, &map, 4U, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_NOT_MAPPABLE);
}

CANOPEN_OD_DEFINE(od_tpdo,
	CANOPEN_OD_DOMAIN(0x5600, CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO, test_domain_ops, NULL));

CANOPEN_OD_DEFINE(od_persist,
	CANOPEN_OD_DOMAIN(0x5600, CANOPEN_OD_RW | CANOPEN_OD_ATTR_PERSIST, test_domain_ops, NULL));

ZTEST(canopen_domain, test_validate)
{
	static struct canopen_node node;
	struct canopen_node_config cfg = test_cfg;

	cfg.od = &od_tpdo;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);
	cfg.od = &od_persist;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);
}

ZTEST_SUITE(canopen_domain, NULL, domain_setup, domain_before, NULL, NULL);
