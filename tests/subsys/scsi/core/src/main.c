/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/scsi/scsi_opcode.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define SECTOR_SIZE  512U
#define DISK_SECTORS 64U

struct mock_scsi_ctx {
	uint8_t image[DISK_SECTORS * SECTOR_SIZE];
	struct scsi_device sdev;
	struct device dev;
	uint8_t tag;
};

static struct mock_scsi_ctx g_mock;
static struct mock_scsi_ctx g_mock_a;
static struct mock_scsi_ctx g_mock_b;

static int mock_scsi_exec(const struct device *dev, struct scsi_xfer *xfer)
{
	struct mock_scsi_ctx *ctx = CONTAINER_OF(dev, struct mock_scsi_ctx, dev);

	if (xfer->cdb[0] == SCSI_OPCODE_READ_10 && xfer->dir == SCSI_DATA_READ &&
	    xfer->data != NULL) {
		const uint32_t lba = sys_get_be32(&xfer->cdb[2]);
		const uint32_t blocks = sys_get_be16(&xfer->cdb[7]);
		const uint32_t len = blocks * SECTOR_SIZE;

		if (lba + blocks > DISK_SECTORS || xfer->data_len < len) {
			xfer->status = SCSI_STATUS_CHECK_CONDITION;
			xfer->sense.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
			xfer->sense.len = 1U;
			return 0;
		}

		memcpy(xfer->data, &ctx->image[lba * SECTOR_SIZE], len);
		xfer->status = SCSI_STATUS_GOOD;
		return 0;
	}

	if (xfer->cdb[0] == SCSI_OPCODE_TEST_UNIT_READY) {
		xfer->status = SCSI_STATUS_GOOD;
		return 0;
	}

	xfer->status = SCSI_STATUS_GOOD;
	return 0;
}

static const struct scsi_driver_api mock_scsi_api = {
	.exec = mock_scsi_exec,
};

static void mock_setup(struct mock_scsi_ctx *ctx, uint8_t fill)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->tag = fill;
	memset(ctx->image, fill, sizeof(ctx->image));
	zassert_ok(scsi_device_init(&ctx->sdev, &ctx->dev, &mock_scsi_api, 0U));
	ctx->sdev.block_size = SECTOR_SIZE;
	ctx->sdev.block_count = DISK_SECTORS;
	ctx->sdev.state = SCSI_DEV_READY;
}

ZTEST(scsi_core, test_device_init_invalid)
{
	zassert_equal(-EINVAL, scsi_device_init(NULL, &g_mock.dev, &mock_scsi_api, 0U));
	zassert_equal(-EINVAL, scsi_device_init(&g_mock.sdev, NULL, &mock_scsi_api, 0U));
}

ZTEST(scsi_core, test_exec_sets_lun_and_sdev)
{
	struct scsi_xfer xfer = {0};
	int ret;

	mock_setup(&g_mock, 0xA5U);
	zassert_ok(scsi_cmd_test_unit_ready(&xfer));
	ret = scsi_exec(&g_mock.sdev, &xfer);
	zassert_ok(ret);
	zassert_equal_ptr(&g_mock.sdev, xfer.sdev);
	zassert_equal(0U, xfer.lun);
}

ZTEST(scsi_core, test_exec_rejects_removed)
{
	struct scsi_xfer xfer = {0};

	mock_setup(&g_mock, 0xA5U);
	scsi_device_mark_removed(&g_mock.sdev);
	zassert_ok(scsi_cmd_test_unit_ready(&xfer));
	zassert_equal(-ENODEV, scsi_exec(&g_mock.sdev, &xfer));
}

ZTEST(scsi_core, test_probe_rejects_removed)
{
	mock_setup(&g_mock, 0xA5U);
	scsi_device_mark_removed(&g_mock.sdev);
	zassert_equal(-ENODEV, scsi_device_probe(&g_mock.sdev));
}

ZTEST(scsi_core, test_io_read)
{
	uint8_t buf[SECTOR_SIZE];
	int ret;

	mock_setup(&g_mock, 0xA5U);
	ret = scsi_io_read(&g_mock.sdev, 1U, 1U, buf, sizeof(buf));
	zassert_ok(ret);
	zassert_equal(0xA5, buf[0]);
}

ZTEST(scsi_core, test_dual_binding_routing)
{
	struct scsi_xfer xfer_a = {0};
	struct scsi_xfer xfer_b = {0};
	uint8_t buf_a[SECTOR_SIZE];
	uint8_t buf_b[SECTOR_SIZE];

	mock_setup(&g_mock_a, 0x11U);
	mock_setup(&g_mock_b, 0x22U);

	zassert_ok(scsi_cmd_read_10(&xfer_a, 0U, 1U, buf_a, sizeof(buf_a)));
	zassert_ok(scsi_exec(&g_mock_a.sdev, &xfer_a));
	zassert_equal(0x11, buf_a[0]);

	zassert_ok(scsi_cmd_read_10(&xfer_b, 0U, 1U, buf_b, sizeof(buf_b)));
	zassert_ok(scsi_exec(&g_mock_b.sdev, &xfer_b));
	zassert_equal(0x22, buf_b[0]);
}

ZTEST(scsi_core, test_status_to_errno_data_protect)
{
	struct scsi_sense sense = {
		.key = SCSI_SENSE_KEY_DATA_PROTECT,
		.len = 1U,
	};

	zassert_equal(-EROFS, scsi_status_to_errno(SCSI_STATUS_CHECK_CONDITION, &sense));
}

ZTEST(scsi_core, test_status_to_errno_aborted)
{
	struct scsi_sense sense = {
		.key = SCSI_SENSE_KEY_ABORTED_COMMAND,
		.len = 1U,
	};

	zassert_equal(-ECANCELED, scsi_status_to_errno(SCSI_STATUS_CHECK_CONDITION, &sense));
}

ZTEST_SUITE(scsi_core, NULL, NULL, NULL, NULL, NULL);
