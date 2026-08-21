/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(scsi_core, CONFIG_SCSI_LOG_LEVEL);

#define SCSI_INQUIRY_STD_LEN 36U
#define SCSI_SENSE_BUF_LEN   32U
#define SCSI_CAP10_LEN       8U
#define SCSI_CAP16_BUF_LEN   32U

static bool scsi_sense_valid(const struct scsi_sense *sense)
{
	return sense != NULL && sense->len > 0U;
}

static void scsi_handle_unit_attention(struct scsi_device *sdev, const struct scsi_sense *sense)
{
	if (sense != NULL && sense->key == SCSI_SENSE_KEY_UNIT_ATTENTION) {
		sdev->state = SCSI_DEV_MEDIA_CHANGED;
	}
}

static int scsi_exec_simple(struct scsi_device *sdev, struct scsi_xfer *xfer)
{
	int ret = scsi_exec(sdev, xfer);

	if (ret == 0) {
		return 0;
	}

	if (xfer->status == SCSI_STATUS_CHECK_CONDITION) {
		if (!scsi_sense_valid(&xfer->sense)) {
			uint8_t sense_buf[SCSI_SENSE_BUF_LEN];
			struct scsi_xfer sense_xfer = {0};

			if (scsi_cmd_request_sense(&sense_xfer, sense_buf, sizeof(sense_buf)) ==
			    0) {
				(void)scsi_exec(sdev, &sense_xfer);
				(void)scsi_parse_sense(&xfer->sense, sense_buf, sizeof(sense_buf));
			}
		}

		scsi_handle_unit_attention(sdev, &xfer->sense);
		return scsi_status_to_errno(xfer->status, &xfer->sense);
	}

	return ret;
}

static int scsi_test_unit_ready_retry(struct scsi_device *sdev)
{
	int ret;

	for (int i = 0; i < CONFIG_SCSI_TUR_RETRY_COUNT; i++) {
		ret = scsi_test_unit_ready(sdev);
		if (ret != -EAGAIN) {
			return ret;
		}

		k_msleep(CONFIG_SCSI_TUR_RETRY_DELAY_MS);
	}

	return -EAGAIN;
}

static int scsi_probe_write_protected(struct scsi_device *sdev)
{
	uint8_t ms[4];
	int ret;

	ret = scsi_mode_sense_6_params(sdev, ms, sizeof(ms), 0U, 0U, 0U, true);
	if (ret != 0) {
		return 0;
	}

	sdev->write_protected = (ms[2] & BIT(7)) != 0U;
	return 0;
}

static int scsi_probe_capacity(struct scsi_device *sdev)
{
	uint8_t cap10[SCSI_CAP10_LEN];
	uint32_t last_lba32;
	uint32_t block_size;
	int ret;

	ret = scsi_read_capacity_10(sdev, cap10, sizeof(cap10));
	if (ret != 0) {
		return ret;
	}

	ret = scsi_parse_read_capacity_10(cap10, sizeof(cap10), &last_lba32, &block_size);
	if (ret != 0) {
		return ret;
	}

	sdev->block_size = block_size;

	if (last_lba32 == 0xffffffffU) {
		uint8_t cap16[SCSI_CAP16_BUF_LEN];
		uint64_t last_lba64;

		ret = scsi_read_capacity_16(sdev, cap16, sizeof(cap16));
		if (ret != 0) {
			return ret;
		}

		ret = scsi_parse_read_capacity_16(cap16, sizeof(cap16), &last_lba64, &block_size);
		if (ret != 0) {
			return ret;
		}

		sdev->block_size = block_size;
		sdev->block_count = last_lba64 + 1U;
		sdev->use_16byte_cmds = true;
	} else {
		sdev->block_count = (uint64_t)last_lba32 + 1U;
		sdev->use_16byte_cmds = false;
	}

	return 0;
}

int scsi_parse_inquiry(const void *inq, uint32_t len, bool *removable)
{
	const uint8_t *b = inq;

	if (inq == NULL || len < 2U) {
		return -EINVAL;
	}

	if (removable != NULL) {
		*removable = (b[1] & BIT(7)) != 0U;
	}

	return 0;
}

int scsi_device_init(struct scsi_device *sdev, const struct device *dev,
		     const struct scsi_driver_api *api, uint8_t lun)
{
	if (sdev == NULL || dev == NULL || api == NULL || api->exec == NULL) {
		return -EINVAL;
	}

	(void)memset(sdev, 0, sizeof(*sdev));
	sdev->dev = dev;
	sdev->api = api;
	sdev->lun = lun;
	sdev->state = SCSI_DEV_INIT;
	k_mutex_init(&sdev->lock);

	return 0;
}

int scsi_status_to_errno(uint8_t status, const struct scsi_sense *sense)
{
	switch (status) {
	case SCSI_STATUS_GOOD:
	case SCSI_STATUS_CONDITION_MET:
		return 0;
	case SCSI_STATUS_BUSY:
	case SCSI_STATUS_TASK_SET_FULL:
		return -EAGAIN;
	case SCSI_STATUS_RESERVATION_CONFLICT:
		return -EBUSY;
	default:
		break;
	}

	if (sense != NULL && sense->key == SCSI_SENSE_KEY_DATA_PROTECT) {
		return -EROFS;
	}

	if (sense != NULL && sense->key == SCSI_SENSE_KEY_ILLEGAL_REQUEST) {
		return -ENOTSUP;
	}

	if (sense != NULL && sense->key == SCSI_SENSE_KEY_NOT_READY) {
		return -EAGAIN;
	}

	if (status == SCSI_STATUS_CHECK_CONDITION) {
		return -EIO;
	}

	return -EIO;
}

int scsi_exec(struct scsi_device *sdev, struct scsi_xfer *xfer)
{
	int ret;

	if (sdev == NULL || xfer == NULL || sdev->api == NULL || sdev->api->exec == NULL) {
		return -EINVAL;
	}

	if (sdev->state == SCSI_DEV_REMOVED) {
		return -ENODEV;
	}

	xfer->lun = sdev->lun;
	xfer->status = 0U;
	xfer->transport_error = 0;

	(void)k_mutex_lock(&sdev->lock, K_FOREVER);
	ret = sdev->api->exec(sdev->dev, xfer);
	(void)k_mutex_unlock(&sdev->lock);

	if (ret != 0) {
		xfer->transport_error = ret;
		/*
		 * CHECK CONDITION: transport completed but the command
		 * failed — continue to SCSI status / sense handling below.
		 */
		if (xfer->status != SCSI_STATUS_CHECK_CONDITION) {
			return ret;
		}
	} else if (xfer->status == SCSI_STATUS_GOOD || xfer->status == SCSI_STATUS_CONDITION_MET) {
		return 0;
	}

	return scsi_status_to_errno(xfer->status, &xfer->sense);
}

int scsi_read_capacity_16(struct scsi_device *sdev, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_read_capacity_16(&xfer, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_read_16(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_read_16(&xfer, lba, blocks, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_write_16(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, const void *buf,
		  uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_write_16(&xfer, lba, blocks, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_io_read(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, void *buf, uint32_t len)
{
	if (sdev == NULL) {
		return -EINVAL;
	}

	if (sdev->use_16byte_cmds || lba > UINT32_MAX) {
		return scsi_read_16(sdev, lba, blocks, buf, len);
	}

	if (blocks > UINT16_MAX) {
		return -EINVAL;
	}

	return scsi_read_10(sdev, (uint32_t)lba, (uint16_t)blocks, buf, len);
}

int scsi_io_write(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, const void *buf,
		  uint32_t len)
{
	if (sdev == NULL) {
		return -EINVAL;
	}

	if (sdev->use_16byte_cmds || lba > UINT32_MAX) {
		return scsi_write_16(sdev, lba, blocks, buf, len);
	}

	if (blocks > UINT16_MAX) {
		return -EINVAL;
	}

	return scsi_write_10(sdev, (uint32_t)lba, (uint16_t)blocks, buf, len);
}

int scsi_sg_io(struct scsi_device *sdev, struct scsi_sg_io *io)
{
	struct scsi_xfer xfer = {0};
	int ret;

	if (sdev == NULL || io == NULL || io->cdb == NULL || io->cdb_len == 0U ||
	    io->cdb_len > SCSI_MAX_CDB_LEN) {
		return -EINVAL;
	}

	(void)memcpy(xfer.cdb, io->cdb, io->cdb_len);
	xfer.cdb_len = io->cdb_len;
	xfer.timeout_ms = SCSI_DEFAULT_TIMEOUT_MS;

	switch (io->dxfer_dir) {
	case SCSI_SG_DXFER_NONE:
		xfer.dir = SCSI_DATA_NONE;
		break;
	case SCSI_SG_DXFER_TO_DEV:
		xfer.dir = SCSI_DATA_WRITE;
		xfer.data = io->dxferp;
		xfer.data_len = io->dxfer_len;
		break;
	case SCSI_SG_DXFER_FROM_DEV:
		xfer.dir = SCSI_DATA_READ;
		xfer.data = io->dxferp;
		xfer.data_len = io->dxfer_len;
		break;
	default:
		return -EINVAL;
	}

	ret = scsi_exec_simple(sdev, &xfer);
	io->status = xfer.status;

	if (io->sense != NULL && io->sense_len > 0U && scsi_sense_valid(&xfer.sense)) {
		uint32_t copy_len = MIN((uint32_t)xfer.sense.len, io->sense_len);

		(void)memcpy(io->sense, xfer.sense.raw, copy_len);
	}

	return ret;
}

int scsi_test_unit_ready(struct scsi_device *sdev)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_test_unit_ready(&xfer);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_inquiry(struct scsi_device *sdev, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_inquiry(&xfer, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_request_sense(struct scsi_device *sdev, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_request_sense(&xfer, buf, len);
	if (ret != 0) {
		return ret;
	}

	ret = scsi_exec(sdev, &xfer);
	if (ret == 0 && buf != NULL) {
		(void)scsi_parse_sense(&xfer.sense, buf, len);
	}

	return ret;
}

int scsi_read_capacity_10(struct scsi_device *sdev, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_read_capacity_10(&xfer, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_read_10(struct scsi_device *sdev, uint32_t lba, uint16_t blocks, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_read_10(&xfer, lba, blocks, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_write_10(struct scsi_device *sdev, uint32_t lba, uint16_t blocks, const void *buf,
		  uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_write_10(&xfer, lba, blocks, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_verify_10(struct scsi_device *sdev, uint32_t lba, uint16_t blocks)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_verify_10(&xfer, lba, blocks);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_mode_sense_6(struct scsi_device *sdev, void *buf, uint32_t len)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_mode_sense_6(&xfer, buf, len);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_mode_sense_6_params(struct scsi_device *sdev, void *buf, uint32_t len, uint8_t pc,
			     uint8_t page_code, uint8_t subpage, bool disable_block_descriptors)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_mode_sense_6_params(&xfer, buf, len, pc, page_code, subpage,
					   disable_block_descriptors);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_start_stop_unit(struct scsi_device *sdev, bool start)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_start_stop_unit(&xfer, start);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_synchronize_cache_10(struct scsi_device *sdev)
{
	struct scsi_xfer xfer = {0};
	int ret;

	ret = scsi_cmd_synchronize_cache_10(&xfer);
	if (ret != 0) {
		return ret;
	}

	return scsi_exec_simple(sdev, &xfer);
}

int scsi_device_probe(struct scsi_device *sdev)
{
	uint8_t inq[SCSI_INQUIRY_STD_LEN];
	bool removable = false;
	int ret;

	if (sdev == NULL) {
		return -EINVAL;
	}

	if (sdev->state == SCSI_DEV_READY && sdev->block_size != 0U && sdev->block_count != 0U) {
		return 0;
	}

	sdev->state = SCSI_DEV_PROBING;

	ret = scsi_inquiry(sdev, inq, sizeof(inq));
	if (ret != 0) {
		sdev->state = SCSI_DEV_ERROR;
		return ret;
	}

	(void)scsi_parse_inquiry(inq, sizeof(inq), &removable);
	sdev->removable = removable;

	ret = scsi_test_unit_ready_retry(sdev);
	if (ret != 0) {
		sdev->state = SCSI_DEV_ERROR;
		return ret;
	}

	ret = scsi_probe_capacity(sdev);
	if (ret != 0) {
		sdev->state = SCSI_DEV_ERROR;
		return ret;
	}

	(void)scsi_probe_write_protected(sdev);
	sdev->state = SCSI_DEV_READY;

	return 0;
}
