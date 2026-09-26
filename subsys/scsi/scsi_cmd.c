/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include <zephyr/scsi/scsi.h>
#include <zephyr/sys/byteorder.h>

static int scsi_xfer_reset_cmd(struct scsi_xfer *xfer)
{
	if (xfer == NULL) {
		return -EINVAL;
	}

	(void)memset(xfer->cdb, 0, sizeof(xfer->cdb));
	xfer->cdb_len = 0U;
	xfer->data = NULL;
	xfer->data_len = 0U;
	xfer->dir = SCSI_DATA_NONE;
	if (xfer->timeout_ms == 0U) {
		xfer->timeout_ms = SCSI_DEFAULT_TIMEOUT_MS;
	}
	xfer->status = 0U;
	xfer->transport_error = 0;
	(void)memset(&xfer->sense, 0, sizeof(xfer->sense));

	return 0;
}

int scsi_cmd_test_unit_ready(struct scsi_xfer *xfer)
{
	int ret;

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_TEST_UNIT_READY;
	xfer->cdb_len = 6U;

	return 0;
}

int scsi_cmd_inquiry(struct scsi_xfer *xfer, void *buf, uint32_t len)
{
	int ret;

	if (buf == NULL || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_INQUIRY;
	xfer->cdb[4] = (uint8_t)(len > 255U ? 255U : len);
	xfer->cdb_len = 6U;
	xfer->data = buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_request_sense(struct scsi_xfer *xfer, void *buf, uint32_t len)
{
	int ret;

	if (buf == NULL || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_REQUEST_SENSE;
	xfer->cdb[4] = (uint8_t)(len > 255U ? 255U : len);
	xfer->cdb_len = 6U;
	xfer->data = buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_read_capacity_10(struct scsi_xfer *xfer, void *buf, uint32_t len)
{
	int ret;

	if (buf == NULL || len < 8U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_READ_CAPACITY_10;
	xfer->cdb_len = 10U;
	xfer->data = buf;
	xfer->data_len = 8U;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_read_capacity_16(struct scsi_xfer *xfer, void *buf, uint32_t len)
{
	int ret;

	if (buf == NULL || len < SCSI_CAP16_LEN) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_SERVICE_ACTION_IN_16;
	xfer->cdb[1] = SCSI_SA_READ_CAPACITY_16;
	sys_put_be32(SCSI_CAP16_LEN, &xfer->cdb[10]);
	xfer->cdb_len = 16U;
	xfer->data = buf;
	xfer->data_len = SCSI_CAP16_LEN;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_read_16(struct scsi_xfer *xfer, uint64_t lba, uint32_t blocks, void *buf, uint32_t len)
{
	int ret;

	if (buf == NULL || blocks == 0U || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_READ_16;
	sys_put_be64(lba, &xfer->cdb[2]);
	sys_put_be32(blocks, &xfer->cdb[10]);
	xfer->cdb_len = 16U;
	xfer->data = buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_write_16(struct scsi_xfer *xfer, uint64_t lba, uint32_t blocks, const void *buf,
		      uint32_t len)
{
	int ret;

	if (buf == NULL || blocks == 0U || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_WRITE_16;
	sys_put_be64(lba, &xfer->cdb[2]);
	sys_put_be32(blocks, &xfer->cdb[10]);
	xfer->cdb_len = 16U;
	xfer->data = (void *)(uintptr_t)buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_WRITE;

	return 0;
}

int scsi_cmd_read_10(struct scsi_xfer *xfer, uint32_t lba, uint16_t blocks, void *buf, uint32_t len)
{
	int ret;

	if (buf == NULL || blocks == 0U || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_READ_10;
	sys_put_be32(lba, &xfer->cdb[2]);
	sys_put_be16(blocks, &xfer->cdb[7]);
	xfer->cdb_len = 10U;
	xfer->data = buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_verify_10(struct scsi_xfer *xfer, uint32_t lba, uint16_t blocks)
{
	int ret;

	if (blocks == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_VERIFY_10;
	sys_put_be32(lba, &xfer->cdb[2]);
	sys_put_be16(blocks, &xfer->cdb[7]);
	xfer->cdb_len = 10U;
	xfer->dir = SCSI_DATA_NONE;

	return 0;
}

int scsi_cmd_write_10(struct scsi_xfer *xfer, uint32_t lba, uint16_t blocks, const void *buf,
		      uint32_t len)
{
	int ret;

	if (buf == NULL || blocks == 0U || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_WRITE_10;
	sys_put_be32(lba, &xfer->cdb[2]);
	sys_put_be16(blocks, &xfer->cdb[7]);
	xfer->cdb_len = 10U;
	xfer->data = (void *)(uintptr_t)buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_WRITE;

	return 0;
}

int scsi_cmd_mode_sense_6(struct scsi_xfer *xfer, void *buf, uint32_t len)
{
	return scsi_cmd_mode_sense_6_params(xfer, buf, len, 0U, 0x3fU, 0U, false);
}

int scsi_cmd_mode_sense_6_params(struct scsi_xfer *xfer, void *buf, uint32_t len, uint8_t pc,
				 uint8_t page_code, uint8_t subpage, bool disable_block_descriptors)
{
	int ret;

	if (buf == NULL || len == 0U) {
		return -EINVAL;
	}

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_MODE_SENSE_6;
	xfer->cdb[1] = disable_block_descriptors ? BIT(4) : 0U;
	xfer->cdb[2] = (uint8_t)(((pc & 3U) << 6) | (page_code & 0x3FU));
	xfer->cdb[3] = subpage;
	xfer->cdb[4] = (uint8_t)(len > 255U ? 255U : len);
	xfer->cdb_len = 6U;
	xfer->data = buf;
	xfer->data_len = len;
	xfer->dir = SCSI_DATA_READ;

	return 0;
}

int scsi_cmd_start_stop_unit(struct scsi_xfer *xfer, bool start)
{
	int ret;

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_START_STOP_UNIT;
	xfer->cdb[4] = start ? 0x01U : 0x00U;
	xfer->cdb_len = 6U;

	return 0;
}

int scsi_cmd_synchronize_cache_10(struct scsi_xfer *xfer)
{
	int ret;

	ret = scsi_xfer_reset_cmd(xfer);
	if (ret != 0) {
		return ret;
	}

	xfer->cdb[0] = SCSI_OPCODE_SYNCHRONIZE_CACHE_10;
	xfer->cdb_len = 10U;

	return 0;
}

int scsi_parse_read_capacity_16(const void *buf, uint32_t len, uint64_t *last_lba,
				uint32_t *block_size)
{
	const uint8_t *b = buf;

	if (buf == NULL || last_lba == NULL || block_size == NULL || len < SCSI_CAP16_LEN) {
		return -EINVAL;
	}

	*last_lba = sys_get_be64(&b[0]);
	*block_size = sys_get_be32(&b[8]);

	if (*block_size == 0U) {
		return -EIO;
	}

	return 0;
}

int scsi_parse_read_capacity_10(const void *buf, uint32_t len, uint32_t *last_lba,
				uint32_t *block_size)
{
	const uint8_t *b = buf;

	if (buf == NULL || last_lba == NULL || block_size == NULL || len < 8U) {
		return -EINVAL;
	}

	*last_lba = sys_get_be32(&b[0]);
	*block_size = sys_get_be32(&b[4]);

	if (*block_size == 0U) {
		return -EIO;
	}

	return 0;
}
