/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/scsi/scsi_sense.h>
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

ZTEST(scsi_cmd, test_parse_sense_fixed)
{
	struct scsi_sense sense;
	uint8_t buf[18] = {0};
	int ret;

	buf[0] = 0x70U;
	buf[2] = SCSI_SENSE_KEY_NOT_READY;
	buf[12] = 0x3aU;
	buf[13] = 0x00U;

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_SENSE_KEY_NOT_READY, sense.key);
	zassert_equal(0x3aU, sense.asc);
	zassert_equal(0x00U, sense.ascq);
}

ZTEST(scsi_cmd, test_sense_key_to_str)
{
	zassert_equal_ptr("NOT READY", scsi_sense_key_to_str(SCSI_SENSE_KEY_NOT_READY));
}

ZTEST_SUITE(scsi_cmd, NULL, NULL, NULL, NULL, NULL);
