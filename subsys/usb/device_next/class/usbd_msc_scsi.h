/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#ifndef ZEPHYR_INCLUDE_USBD_MSC_SCSI_H_
#define ZEPHYR_INCLUDE_USBD_MSC_SCSI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* SAM-6 5.3.1 Status codes
 * Table 44 – Status codes
 */
enum scsi_status_code {
	GOOD = 0x00,
	CHECK_CONDITION = 0x02,
	CONDITION_MET = 0x04,
	BUSY = 0x08,
	RESERVATION_CONFLICT = 0x18,
	TASK_SET_FULL = 0x28,
	ACA_ACTIVE = 0x30,
	TASK_ABORTED = 0x40,
};

/* SPC-5 4.4.8 Sense key and additional sense code definitions
 * Table 49 — Sense key descriptions
 */
enum scsi_sense_key {
	NO_SENSE = 0x0,
	RECOVERED_ERROR = 0x1,
	NOT_READY = 0x2,
	MEDIUM_ERROR = 0x3,
	HARDWARE_ERROR = 0x4,
	ILLEGAL_REQUEST = 0x5,
	UNIT_ATTENTION = 0x6,
	DATA_PROTECT = 0x7,
	BLANK_CHECK = 0x8,
	VENDOR_SPECIFIC = 0x9,
	COPY_ABORTED = 0xA,
	ABORTED_COMMAND = 0xB,
	/* 0xC is Reserved */
	VOLUME_OVERFLOW = 0xD,
	MISCOMPARE = 0xE,
	COMPLETED = 0xF,
};

/* SPC-5 Table F.1 — ASC and ASCQ assignments
 * ASC is upper 8 bits, ASCQ on lower 8 bits
 */
enum scsi_additional_sense_code {
	NO_ADDITIONAL_SENSE_INFORMATION = 0x0000,
	LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE = 0x2100,
	INVALID_FIELD_IN_CDB = 0x2400,
	MEDIUM_NOT_PRESENT = 0x3A00,
	MEDIUM_REMOVAL_PREVENTED = 0x5302,
	WRITE_ERROR = 0x0C00,
};

struct scsi_ctx {
	const char *disk;
	const char *vendor;
	const char *product;
	const char *revision;
	size_t (*read_cb)(struct scsi_ctx *ctx,
			  uint8_t buf[static CONFIG_USBD_MSC_SCSI_BUFFER_SIZE]);
	size_t (*write_cb)(struct scsi_ctx *ctx, const uint8_t *buf, size_t length);
	size_t remaining_data;
	uint32_t lba;
	uint32_t sector_count;
	uint32_t sector_size;
	enum scsi_status_code status;
	enum scsi_sense_key sense_key;
	enum scsi_additional_sense_code asc;
	bool prevent_removal : 1;
	bool medium_loaded : 1;
	bool cmd_is_data_read : 1;
	bool cmd_is_data_write : 1;
};

void scsi_init(struct scsi_ctx *ctx, const char *disk, const char *vendor,
	       const char *product, const char *revision);
void scsi_reset(struct scsi_ctx *ctx);
int scsi_usb_boot_cmd_len(const uint8_t *cb, int len);
size_t scsi_cmd(struct scsi_ctx *ctx, const uint8_t *cb, int len,
		uint8_t data_in_buf[static CONFIG_USBD_MSC_SCSI_BUFFER_SIZE]);

bool scsi_cmd_is_data_read(struct scsi_ctx *ctx);
bool scsi_cmd_is_data_write(struct scsi_ctx *ctx);
size_t scsi_cmd_remaining_data_len(struct scsi_ctx *ctx);
size_t scsi_read_data(struct scsi_ctx *ctx,
		      uint8_t data_in_buf[static CONFIG_USBD_MSC_SCSI_BUFFER_SIZE]);
size_t scsi_write_data(struct scsi_ctx *ctx, const uint8_t *buf, size_t length);

enum scsi_status_code scsi_cmd_get_status(struct scsi_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBD_MSC_SCSI_H_ */
