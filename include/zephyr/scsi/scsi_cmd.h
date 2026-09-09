/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transport-neutral SCSI command descriptor block builders
 *
 * These helpers only populate @ref scsi_xfer. They perform no I/O and have
 * no knowledge of transport-specific details.
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_SCSI_SCSI_CMD_H_
#define ZEPHYR_INCLUDE_SCSI_SCSI_CMD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct scsi_xfer;

/** @name SCSI primary command opcodes used by the mid-layer */
/**@{*/
#define SCSI_OPCODE_TEST_UNIT_READY      0x00U
#define SCSI_OPCODE_REQUEST_SENSE        0x03U
#define SCSI_OPCODE_INQUIRY              0x12U
#define SCSI_OPCODE_MODE_SENSE_6         0x1aU
#define SCSI_OPCODE_READ_CAPACITY_10     0x25U
#define SCSI_OPCODE_READ_10              0x28U
#define SCSI_OPCODE_WRITE_10             0x2aU
#define SCSI_OPCODE_VERIFY_10            0x2fU
#define SCSI_OPCODE_READ_16              0x88U
#define SCSI_OPCODE_WRITE_16             0x8aU
#define SCSI_OPCODE_SERVICE_ACTION_IN_16 0x9eU
#define SCSI_SA_READ_CAPACITY_16         0x10U
#define SCSI_OPCODE_START_STOP_UNIT      0x1bU
#define SCSI_OPCODE_SYNCHRONIZE_CACHE_10 0x35U
/**@}*/

/** READ CAPACITY(16) response length */
#define SCSI_CAP16_LEN 32U

/** @name SCSI status byte values */
/**@{*/
#define SCSI_STATUS_GOOD                 0x00U
#define SCSI_STATUS_CHECK_CONDITION      0x02U
#define SCSI_STATUS_CONDITION_MET        0x04U
#define SCSI_STATUS_BUSY                 0x08U
#define SCSI_STATUS_RESERVATION_CONFLICT 0x18U
#define SCSI_STATUS_TASK_SET_FULL        0x28U
#define SCSI_STATUS_ACA_ACTIVE           0x30U
/**@}*/

/** Maximum CDB length supported by @ref scsi_xfer */
#define SCSI_MAX_CDB_LEN 16U

/** Default command timeout when not specified by caller (milliseconds) */
#define SCSI_DEFAULT_TIMEOUT_MS 30000U

int scsi_cmd_test_unit_ready(struct scsi_xfer *xfer);

int scsi_cmd_inquiry(struct scsi_xfer *xfer, void *buf, uint32_t len);

int scsi_cmd_request_sense(struct scsi_xfer *xfer, void *buf, uint32_t len);

int scsi_cmd_read_capacity_10(struct scsi_xfer *xfer, void *buf, uint32_t len);

int scsi_cmd_read_capacity_16(struct scsi_xfer *xfer, void *buf, uint32_t len);

int scsi_cmd_read_10(struct scsi_xfer *xfer, uint32_t lba, uint16_t blocks, void *buf,
		     uint32_t len);

int scsi_cmd_write_10(struct scsi_xfer *xfer, uint32_t lba, uint16_t blocks, const void *buf,
		      uint32_t len);

int scsi_cmd_read_16(struct scsi_xfer *xfer, uint64_t lba, uint32_t blocks, void *buf,
		     uint32_t len);

int scsi_cmd_write_16(struct scsi_xfer *xfer, uint64_t lba, uint32_t blocks, const void *buf,
		      uint32_t len);

int scsi_cmd_verify_10(struct scsi_xfer *xfer, uint32_t lba, uint16_t blocks);

int scsi_cmd_mode_sense_6(struct scsi_xfer *xfer, void *buf, uint32_t len);

int scsi_cmd_mode_sense_6_params(struct scsi_xfer *xfer, void *buf, uint32_t len, uint8_t pc,
				 uint8_t page_code, uint8_t subpage,
				 bool disable_block_descriptors);

int scsi_cmd_start_stop_unit(struct scsi_xfer *xfer, bool start);

int scsi_cmd_synchronize_cache_10(struct scsi_xfer *xfer);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_SCSI_CMD_H_ */
