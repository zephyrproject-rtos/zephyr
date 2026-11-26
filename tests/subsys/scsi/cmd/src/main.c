/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/scsi/scsi_sense.h>
#include <zephyr/sys/byteorder.h>
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
	buf[7] = 10U;
	buf[12] = 0x3aU;
	buf[13] = 0x00U;

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_SENSE_KEY_NOT_READY, sense.key);
	zassert_equal(0x3aU, sense.asc);
	zassert_equal(0x00U, sense.ascq);
}

ZTEST(scsi_cmd, test_parse_sense_fixed_short)
{
	struct scsi_sense sense;
	uint8_t buf[2] = {0x70U, 0x00U};
	int ret;

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_equal(-EINVAL, ret);
}

ZTEST(scsi_cmd, test_parse_sense_fixed_truncated_asc)
{
	struct scsi_sense sense;
	uint8_t buf[8] = {0};
	int ret;

	buf[0] = 0x71U;
	buf[2] = SCSI_SENSE_KEY_HARDWARE_ERROR;
	buf[7] = 10U; /* claims bytes through 17; only 8 bytes supplied */

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_SENSE_KEY_HARDWARE_ERROR, sense.key);
	zassert_equal(0U, sense.asc);
	zassert_equal(0U, sense.ascq);
}

ZTEST(scsi_cmd, test_parse_sense_descriptor_current)
{
	struct scsi_sense sense;
	uint8_t buf[32] = {0};
	int ret;

	buf[0] = 0x72U;
	buf[1] = SCSI_SENSE_KEY_NOT_READY;
	buf[2] = 0x3aU;
	buf[3] = 0x04U;
	buf[7] = 0x08U;
	buf[8] = 0x02U; /* Sense Key Specific descriptor — must not override header */
	buf[9] = 0x06U;
	buf[10] = SCSI_SENSE_KEY_UNIT_ATTENTION;
	buf[11] = 0xffU;
	buf[12] = 0x00U;
	buf[13] = 0xffU; /* fixed-format offsets must not be used for 0x72 */

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_SENSE_KEY_NOT_READY, sense.key);
	zassert_equal(0x3aU, sense.asc);
	zassert_equal(0x04U, sense.ascq);
}

ZTEST(scsi_cmd, test_parse_sense_descriptor_deferred)
{
	struct scsi_sense sense;
	uint8_t buf[32] = {0};
	int ret;

	buf[0] = 0x73U;
	buf[1] = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
	buf[2] = 0x20U;
	buf[3] = 0x01U;
	buf[7] = 0x08U;
	buf[12] = 0x99U; /* fixed-format decoy offsets must not be used for 0x73 */
	buf[13] = 0xbbU;

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(SCSI_SENSE_KEY_ILLEGAL_REQUEST, sense.key);
	zassert_equal(0x20U, sense.asc);
	zassert_equal(0x01U, sense.ascq);
}

ZTEST(scsi_cmd, test_parse_sense_descriptor_short)
{
	struct scsi_sense sense;
	uint8_t buf[3] = {0x72U, SCSI_SENSE_KEY_MEDIUM_ERROR, 0x11U};
	int ret;

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_equal(-EINVAL, ret);
}

ZTEST(scsi_cmd, test_parse_sense_unsupported)
{
	struct scsi_sense sense;
	uint8_t buf[4] = {0x80U, 0x01U, 0x02U, 0x03U};
	int ret;

	ret = scsi_parse_sense(&sense, buf, sizeof(buf));
	zassert_equal(-ENOTSUP, ret);
	zassert_equal(0x80U, sense.response_code);
	zassert_equal(4U, sense.len);
	zassert_equal(0x80U, sense.raw[0]);
}

ZTEST(scsi_cmd, test_sense_key_to_str)
{
	zassert_equal(0, strcmp(scsi_sense_key_to_str(SCSI_SENSE_KEY_NOT_READY), "NOT READY"));
}

ZTEST(scsi_cmd, test_parse_read_capacity_16_max_lba)
{
	uint8_t buf[32] = {0};
	uint64_t last_lba;
	uint32_t block_size;
	int ret;

	sys_put_be64(UINT64_MAX, &buf[0]);
	sys_put_be32(512U, &buf[8]);

	ret = scsi_parse_read_capacity_16(buf, sizeof(buf), &last_lba, &block_size);
	zassert_ok(ret);
	zassert_equal(UINT64_MAX, last_lba);
	zassert_equal(512U, block_size);
}

ZTEST_SUITE(scsi_cmd, NULL, NULL, NULL, NULL, NULL);
