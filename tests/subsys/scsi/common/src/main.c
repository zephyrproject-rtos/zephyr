/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/scsi/scsi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

enum mock_exec_mode {
	MOCK_EXEC_GOOD = 0,
	MOCK_EXEC_CHECK_CONDITION,
	MOCK_EXEC_PROBE,
	MOCK_EXEC_PROBE_CAP16,
	MOCK_EXEC_SG_IO,
};

static enum mock_exec_mode mock_mode;
static const struct device mock_transport;

static int mock_scsi_exec(const struct device *dev, struct scsi_xfer *xfer)
{
	static const uint8_t inquiry_resp[36] = {
		0x00, 0x80, 0x05, 0x01, 0x1f, 0x00, 0x00, 0x00, 0x5a, 0x45, 0x50, 0x48,
		0x59, 0x52, 0x20, 0x20, 0x4d, 0x4f, 0x43, 0x4b, 0x20, 0x44, 0x49, 0x53,
		0x4b, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x31, 0x2e, 0x30, 0x20,
	};
	uint8_t cap_resp[8];

	ARG_UNUSED(dev);

	if (xfer == NULL) {
		return -EINVAL;
	}

	switch (mock_mode) {
	case MOCK_EXEC_PROBE:
		switch (xfer->cdb[0]) {
		case SCSI_OPCODE_INQUIRY:
			if (xfer->data == NULL || xfer->data_len < sizeof(inquiry_resp)) {
				return -EINVAL;
			}
			memcpy(xfer->data, inquiry_resp, sizeof(inquiry_resp));
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		case SCSI_OPCODE_TEST_UNIT_READY:
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		case SCSI_OPCODE_READ_CAPACITY_10:
			if (xfer->data == NULL || xfer->data_len < 8U) {
				return -EINVAL;
			}
			sys_put_be32(0x000003ffU, &cap_resp[0]);
			sys_put_be32(512U, &cap_resp[4]);
			memcpy(xfer->data, cap_resp, sizeof(cap_resp));
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		default:
			break;
		}
		break;
	case MOCK_EXEC_PROBE_CAP16:
		switch (xfer->cdb[0]) {
		case SCSI_OPCODE_INQUIRY:
			if (xfer->data == NULL || xfer->data_len < sizeof(inquiry_resp)) {
				return -EINVAL;
			}
			memcpy(xfer->data, inquiry_resp, sizeof(inquiry_resp));
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		case SCSI_OPCODE_TEST_UNIT_READY:
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		case SCSI_OPCODE_READ_CAPACITY_10:
			if (xfer->data == NULL || xfer->data_len < 8U) {
				return -EINVAL;
			}
			sys_put_be32(0xffffffffU, &cap_resp[0]);
			sys_put_be32(512U, &cap_resp[4]);
			memcpy(xfer->data, cap_resp, sizeof(cap_resp));
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		case SCSI_OPCODE_SERVICE_ACTION_IN_16: {
			uint8_t cap16[32];

			if (xfer->data == NULL || xfer->data_len < 12U) {
				return -EINVAL;
			}
			sys_put_be64(0x000000ffffffffffULL, &cap16[0]);
			sys_put_be32(4096U, &cap16[8]);
			memcpy(xfer->data, cap16, MIN(xfer->data_len, sizeof(cap16)));
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		}
		case SCSI_OPCODE_MODE_SENSE_6:
			if (xfer->data != NULL && xfer->data_len >= 3U) {
				memset(xfer->data, 0, xfer->data_len);
			}
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		default:
			break;
		}
		break;
	case MOCK_EXEC_SG_IO:
		if (xfer->cdb[0] == 0xABU && xfer->dir == SCSI_DATA_READ && xfer->data != NULL &&
		    xfer->data_len >= 4U) {
			memcpy(xfer->data, "OKAY", 4U);
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		}
		break;
	case MOCK_EXEC_CHECK_CONDITION:
		if (xfer->cdb[0] == SCSI_OPCODE_TEST_UNIT_READY) {
			xfer->status = SCSI_STATUS_CHECK_CONDITION;
			return -EIO;
		}
		if (xfer->cdb[0] == SCSI_OPCODE_REQUEST_SENSE) {
			uint8_t sense[] = {0x70, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x0a,
					   0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00};

			if (xfer->data == NULL || xfer->data_len < sizeof(sense)) {
				return -EINVAL;
			}
			memcpy(xfer->data, sense, sizeof(sense));
			xfer->status = SCSI_STATUS_GOOD;
			return 0;
		}
		break;
	default:
		break;
	}

	xfer->status = SCSI_STATUS_GOOD;
	return 0;
}

static const struct scsi_driver_api mock_scsi_api = {
	.exec = mock_scsi_exec,
};

static void scsi_common_before(void *fixture)
{
	ARG_UNUSED(fixture);
	mock_mode = MOCK_EXEC_GOOD;
}

ZTEST(scsi_common, test_cmd_read10_be)
{
	struct scsi_xfer xfer = {0};
	uint8_t buf[512];
	int ret;

	ret = scsi_cmd_read_10(&xfer, 0x12345678U, 2U, buf, sizeof(buf));
	zassert_equal(ret, 0, NULL);
	zassert_equal(xfer.cdb[0], SCSI_OPCODE_READ_10, NULL);
	zassert_equal(xfer.cdb[2], 0x12, NULL);
	zassert_equal(xfer.cdb[3], 0x34, NULL);
	zassert_equal(xfer.cdb[4], 0x56, NULL);
	zassert_equal(xfer.cdb[5], 0x78, NULL);
	zassert_equal(xfer.cdb[7], 0x00, NULL);
	zassert_equal(xfer.cdb[8], 0x02, NULL);
	zassert_equal(xfer.dir, SCSI_DATA_READ, NULL);
}

ZTEST(scsi_common, test_cmd_verify10_be)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_verify_10(&xfer, 0x00000064U, 8U);
	zassert_equal(ret, 0, NULL);
	zassert_equal(xfer.cdb[0], SCSI_OPCODE_VERIFY_10, NULL);
	zassert_equal(xfer.cdb[2], 0x00, NULL);
	zassert_equal(xfer.cdb[3], 0x00, NULL);
	zassert_equal(xfer.cdb[4], 0x00, NULL);
	zassert_equal(xfer.cdb[5], 0x64, NULL);
	zassert_equal(xfer.cdb[7], 0x00, NULL);
	zassert_equal(xfer.cdb[8], 0x08, NULL);
	zassert_equal(xfer.dir, SCSI_DATA_NONE, NULL);
}

ZTEST(scsi_common, test_parse_sense)
{
	struct scsi_sense sense;
	uint8_t raw[] = {0x70, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x0a,
			 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00};

	zassert_equal(scsi_parse_sense(&sense, raw, sizeof(raw)), 0, NULL);
	zassert_equal(sense.key, SCSI_SENSE_KEY_ILLEGAL_REQUEST, NULL);
	zassert_equal(sense.asc, 0x20, NULL);
}

ZTEST(scsi_common, test_mock_exec)
{
	struct scsi_device sdev;
	struct scsi_xfer xfer = {0};

	zassert_equal(scsi_device_init(&sdev, &mock_transport, &mock_scsi_api, 0U), 0, NULL);
	zassert_equal(scsi_cmd_test_unit_ready(&xfer), 0, NULL);
	zassert_equal(scsi_exec(&sdev, &xfer), 0, NULL);
	zassert_equal(xfer.status, SCSI_STATUS_GOOD, NULL);
}

ZTEST(scsi_common, test_parse_capacity16)
{
	uint8_t cap16[32];
	uint64_t last_lba;
	uint32_t block_size;

	sys_put_be64(0x000000ffffffffffULL, &cap16[0]);
	sys_put_be32(4096U, &cap16[8]);

	zassert_equal(scsi_parse_read_capacity_16(cap16, sizeof(cap16), &last_lba, &block_size), 0,
		      NULL);
	zassert_equal(last_lba, 0x000000ffffffffffULL, NULL);
	zassert_equal(block_size, 4096U, NULL);
}

ZTEST(scsi_common, test_parse_capacity10_overflow)
{
	uint8_t cap10[8];
	uint32_t last_lba;
	uint32_t block_size;

	sys_put_be32(0xffffffffU, &cap10[0]);
	sys_put_be32(512U, &cap10[4]);

	zassert_equal(scsi_parse_read_capacity_10(cap10, sizeof(cap10), &last_lba, &block_size), 0,
		      NULL);
	zassert_equal(last_lba, 0xffffffffU, NULL);
}

ZTEST(scsi_common, test_device_probe)
{
	struct scsi_device sdev;
	int ret;

	mock_mode = MOCK_EXEC_PROBE;

	zassert_equal(scsi_device_init(&sdev, &mock_transport, &mock_scsi_api, 0U), 0, NULL);
	ret = scsi_device_probe(&sdev);
	zassert_equal(ret, 0, "probe failed: %d", ret);
	zassert_equal(sdev.state, SCSI_DEV_READY, NULL);
	zassert_equal(sdev.block_size, 512U, NULL);
	zassert_equal(sdev.block_count, 0x400U, NULL);
	zassert_true(sdev.removable, NULL);
}

ZTEST(scsi_common, test_check_condition_request_sense)
{
	struct scsi_device sdev;
	int ret;

	mock_mode = MOCK_EXEC_CHECK_CONDITION;

	zassert_equal(scsi_device_init(&sdev, &mock_transport, &mock_scsi_api, 0U), 0, NULL);
	ret = scsi_test_unit_ready(&sdev);
	zassert_equal(ret, -ENOTSUP, "expected ILLEGAL REQUEST errno, got %d", ret);
}

ZTEST(scsi_common, test_device_probe_cap16)
{
	struct scsi_device sdev;
	int ret;

	mock_mode = MOCK_EXEC_PROBE_CAP16;

	zassert_equal(scsi_device_init(&sdev, &mock_transport, &mock_scsi_api, 0U), 0, NULL);
	ret = scsi_device_probe(&sdev);
	zassert_equal(ret, 0, "probe failed: %d", ret);
	zassert_equal(sdev.state, SCSI_DEV_READY, NULL);
	zassert_equal(sdev.block_size, 4096U, NULL);
	zassert_equal(sdev.block_count, 0x10000000000ULL, NULL);
	zassert_true(sdev.use_16byte_cmds, NULL);
}

ZTEST(scsi_common, test_scsi_sg_io)
{
	struct scsi_device sdev;
	struct scsi_sg_io io;
	uint8_t cdb[6] = {0xAB, 0, 0, 0, 0, 0};
	uint8_t buf[8];

	mock_mode = MOCK_EXEC_SG_IO;

	zassert_equal(scsi_device_init(&sdev, &mock_transport, &mock_scsi_api, 0U), 0, NULL);
	memset(buf, 0, sizeof(buf));
	io.cdb = cdb;
	io.cdb_len = sizeof(cdb);
	io.dxfer_dir = SCSI_SG_DXFER_FROM_DEV;
	io.dxferp = buf;
	io.dxfer_len = sizeof(buf);
	io.sense = NULL;
	io.sense_len = 0U;

	zassert_equal(scsi_sg_io(&sdev, &io), 0, NULL);
	zassert_equal(io.status, SCSI_STATUS_GOOD, NULL);
	zassert_equal(memcmp(buf, "OKAY", 4), 0, NULL);
}

ZTEST_SUITE(scsi_common, NULL, NULL, scsi_common_before, NULL, NULL);
