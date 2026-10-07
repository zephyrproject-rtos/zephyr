/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/scsi/scsi.h>
#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/ztest.h>

ZTEST(scsi_cmd, test_test_unit_ready_cdb)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_test_unit_ready(&xfer);
	zassert_ok(ret);
	zassert_equal(SCSI_OPCODE_TEST_UNIT_READY, xfer.cdb[0]);
	zassert_equal(6U, xfer.cdb_len);
	zassert_equal(SCSI_DATA_NONE, xfer.dir);
}

ZTEST(scsi_cmd, test_inquiry_cdb)
{
	uint8_t buf[36];
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_inquiry(&xfer, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_OPCODE_INQUIRY, xfer.cdb[0]);
	zassert_equal(6U, xfer.cdb_len);
	zassert_equal(SCSI_DATA_READ, xfer.dir);
	zassert_equal_ptr(buf, xfer.data);
}

ZTEST(scsi_cmd, test_read_10_cdb)
{
	uint8_t buf[512];
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_read_10(&xfer, 0x1000U, 1U, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_OPCODE_READ_10, xfer.cdb[0]);
	zassert_equal(10U, xfer.cdb_len);
	zassert_equal(SCSI_DATA_READ, xfer.dir);
}

ZTEST_SUITE(scsi_cmd, NULL, NULL, NULL, NULL, NULL);
