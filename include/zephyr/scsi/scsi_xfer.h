/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI command transfer descriptor
 *
 * Shared by CDB builders and the SCSI mid-layer. No transport or device
 * lifecycle APIs are declared here.
 *
 * @since 4.6
 */

#ifndef ZEPHYR_INCLUDE_SCSI_SCSI_XFER_H_
#define ZEPHYR_INCLUDE_SCSI_SCSI_XFER_H_

#include <stdint.h>

#include <zephyr/scsi/scsi_sense.h>

/** Maximum CDB length supported by @ref scsi_xfer */
#define SCSI_MAX_CDB_LEN 16U

#ifdef __cplusplus
extern "C" {
#endif

struct scsi_device;

/** SCSI data transfer direction for @ref scsi_xfer */
enum scsi_data_dir {
	/** No data stage */
	SCSI_DATA_NONE = 0,
	/** Data-in (device to host) */
	SCSI_DATA_READ,
	/** Data-out (host to device) */
	SCSI_DATA_WRITE,
};

/** One SCSI command transaction */
struct scsi_xfer {
	/** Command descriptor block */
	uint8_t cdb[SCSI_MAX_CDB_LEN];
	/** Valid length of @a cdb in bytes */
	uint8_t cdb_len;
	/** Data buffer for the data stage (may be NULL) */
	void *data;
	/** Data stage length in bytes */
	uint32_t data_len;
	/** Data transfer direction */
	enum scsi_data_dir dir;
	/** Command timeout in milliseconds */
	uint32_t timeout_ms;
	/** Target logical unit number */
	uint8_t lun;
	/**
	 * Owning SCSI device (set by @ref scsi_exec before calling the transport).
	 *
	 * Transports must route using @a sdev; @c scsi_device.dev alone is not
	 * sufficient when several targets share one host controller.
	 */
	struct scsi_device *sdev;
	/** SCSI status byte returned by the device */
	uint8_t status;
	/** Sense data on CHECK CONDITION */
	struct scsi_sense sense;
	/** Transport-layer error (negative errno) */
	int transport_error;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_SCSI_XFER_H_ */
