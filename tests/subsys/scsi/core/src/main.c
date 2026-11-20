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
static unsigned int mock_sync_cache_calls;

static int mock_scsi_exec(const struct device *dev, struct scsi_xfer *xfer)
{
	struct mock_scsi_ctx *ctx = CONTAINER_OF(dev, struct mock_scsi_ctx, dev);

	if (xfer->cdb[0] == SCSI_OPCODE_SYNCHRONIZE_CACHE_10) {
		mock_sync_cache_calls++;

		if (ctx->tag == 0xEEU || ctx->tag == 0x24U || ctx->tag == 0x26U ||
		    ctx->tag == 0xCCU || ctx->tag == 0x3AU || ctx->tag == 0xDDU ||
		    ctx->tag == 0xFFU || ctx->tag == 0x02U || ctx->tag == 0x07U ||
		    ctx->tag == 0x00U) {
			xfer->status = SCSI_STATUS_CHECK_CONDITION;
			xfer->sense.len = 0U;

			if (ctx->tag != 0x00U) {
				xfer->sense.len = 18U;
			}

			switch (ctx->tag) {
			case 0xEEU:
				xfer->sense.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
				xfer->sense.asc = 0x20U;
				break;
			case 0x24U:
				xfer->sense.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
				xfer->sense.asc = 0x24U;
				break;
			case 0x26U:
				xfer->sense.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
				xfer->sense.asc = 0x26U;
				break;
			case 0xCCU:
				xfer->sense.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
				xfer->sense.asc = 0x21U;
				break;
			case 0x3AU:
				xfer->sense.key = SCSI_SENSE_KEY_NOT_READY;
				xfer->sense.asc = 0x3aU;
				break;
			case 0xDDU:
				xfer->sense.key = SCSI_SENSE_KEY_MEDIUM_ERROR;
				xfer->sense.asc = 0x11U;
				break;
			case 0xFFU:
				xfer->sense.key = SCSI_SENSE_KEY_HARDWARE_ERROR;
				xfer->sense.asc = 0x40U;
				break;
			case 0x02U:
				xfer->sense.key = SCSI_SENSE_KEY_NOT_READY;
				xfer->sense.asc = 0x04U;
				break;
			case 0x07U:
				xfer->sense.key = SCSI_SENSE_KEY_DATA_PROTECT;
				xfer->sense.asc = 0x27U;
				break;
			default:
				break;
			}

			xfer->sense.ascq = 0x00U;
			return 0;
		}

		if (ctx->tag == 0xFEU) {
			return -ETIMEDOUT;
		}

		xfer->status = SCSI_STATUS_GOOD;
		return 0;
	}

	if (xfer->cdb[0] == SCSI_OPCODE_REQUEST_SENSE && xfer->data != NULL &&
	    xfer->data_len >= 18U) {
		uint8_t *buf = xfer->data;
		uint8_t key = SCSI_SENSE_KEY_NO_SENSE;
		uint8_t asc = 0U;

		switch (ctx->tag) {
		case 0xEEU:
			key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
			asc = 0x20U;
			break;
		case 0x24U:
			key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
			asc = 0x24U;
			break;
		case 0x26U:
			key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
			asc = 0x26U;
			break;
		case 0xCCU:
			key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
			asc = 0x21U;
			break;
		case 0x3AU:
			asc = 0x3aU;
			break;
		default:
			break;
		}

		buf[0] = 0x70U;
		buf[2] = key & 0x0fU;
		buf[12] = asc;
		buf[13] = 0x00U;
		xfer->status = SCSI_STATUS_GOOD;
		return 0;
	}

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

ZTEST(scsi_core, test_sense_opcode_unsupported)
{
	struct scsi_sense ok = {
		.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST,
		.asc = 0x20U,
		.len = 18U,
	};
	struct scsi_sense asc24 = {
		.key = SCSI_SENSE_KEY_ILLEGAL_REQUEST,
		.asc = 0x24U,
		.len = 18U,
	};
	struct scsi_sense wrong_key = {
		.key = SCSI_SENSE_KEY_MEDIUM_ERROR,
		.asc = 0x20U,
		.len = 18U,
	};

	zassert_true(scsi_sense_opcode_unsupported(&ok));
	zassert_false(scsi_sense_opcode_unsupported(&asc24));
	zassert_false(scsi_sense_opcode_unsupported(&wrong_key));
	zassert_false(scsi_sense_opcode_unsupported(NULL));
}

ZTEST(scsi_core, test_sync_cache_good)
{
	mock_sync_cache_calls = 0U;
	mock_setup(&g_mock, 0xA5U);

	zassert_ok(scsi_synchronize_cache_10(&g_mock.sdev));
	zassert_equal(1U, mock_sync_cache_calls);
}

ZTEST(scsi_core, test_sync_cache_opcode_unsupported)
{
	mock_sync_cache_calls = 0U;
	mock_setup(&g_mock, 0xEEU);

	zassert_ok(scsi_synchronize_cache_10(&g_mock.sdev));
	zassert_equal(1U, mock_sync_cache_calls);
}

ZTEST(scsi_core, test_sync_cache_repeat_issues_command)
{
	mock_sync_cache_calls = 0U;
	mock_setup(&g_mock, 0xEEU);

	zassert_ok(scsi_synchronize_cache_10(&g_mock.sdev));
	zassert_ok(scsi_synchronize_cache_10(&g_mock.sdev));
	zassert_equal(2U, mock_sync_cache_calls);
}

ZTEST(scsi_core, test_sync_cache_asc24_fails)
{
	mock_sync_cache_calls = 0U;
	mock_setup(&g_mock, 0x24U);

	zassert_equal(-ENOTSUP, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_asc26_fails)
{
	mock_setup(&g_mock, 0x26U);
	zassert_equal(-ENOTSUP, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_generic_illegal_fails)
{
	mock_setup(&g_mock, 0xCCU);
	zassert_equal(-ENOTSUP, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_no_sense_fails)
{
	mock_setup(&g_mock, 0x00U);
	zassert_equal(-EIO, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_medium_error)
{
	mock_setup(&g_mock, 0xDDU);
	zassert_equal(-EIO, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_hardware_error)
{
	mock_setup(&g_mock, 0xFFU);
	zassert_equal(-EIO, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_not_ready)
{
	mock_setup(&g_mock, 0x02U);
	zassert_equal(-EAGAIN, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_asc3a_nonfatal)
{
	mock_setup(&g_mock, 0x3AU);
	zassert_ok(scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_data_protect)
{
	mock_setup(&g_mock, 0x07U);
	zassert_equal(-EROFS, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_removed)
{
	mock_setup(&g_mock, 0xA5U);
	scsi_device_mark_removed(&g_mock.sdev);
	zassert_equal(-ENODEV, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_transport_timeout)
{
	mock_setup(&g_mock, 0xFEU);
	zassert_equal(-ETIMEDOUT, scsi_synchronize_cache_10(&g_mock.sdev));
}

ZTEST(scsi_core, test_sync_cache_independent_luns)
{
	mock_sync_cache_calls = 0U;
	mock_setup(&g_mock_a, 0xEEU);
	mock_setup(&g_mock_b, 0xA5U);

	zassert_ok(scsi_synchronize_cache_10(&g_mock_a.sdev));
	zassert_ok(scsi_synchronize_cache_10(&g_mock_b.sdev));
	zassert_equal(2U, mock_sync_cache_calls);
}

ZTEST_SUITE(scsi_core, NULL, NULL, NULL, NULL, NULL);
