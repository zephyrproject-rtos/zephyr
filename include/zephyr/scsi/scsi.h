/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI mid-layer public API
 *
 * Transport-neutral SCSI command construction, device state, and synchronous
 * command execution. Lower-level transports (e.g. USB MSC) implement
 * @ref scsi_driver_api.
 *
 * @since 4.3
 * @version 0.1.0
 * @defgroup scsi_api SCSI mid-layer
 * @ingroup storage_interfaces
 */

#ifndef ZEPHYR_INCLUDE_SCSI_SCSI_H_
#define ZEPHYR_INCLUDE_SCSI_SCSI_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/scsi/scsi_driver.h>
#include <zephyr/scsi/scsi_sense.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	/** SCSI status byte returned by the device */
	uint8_t status;
	/** Sense data on CHECK CONDITION */
	struct scsi_sense sense;
	/** Transport-layer error (negative errno) */
	int transport_error;
};

/** SCSI device lifecycle / readiness state */
enum scsi_device_state {
	SCSI_DEV_INIT = 0,      /*!< Initial / not yet probed */
	SCSI_DEV_PROBING,       /*!< Capacity or mode sense in progress */
	SCSI_DEV_READY,         /*!< Media accessible */
	SCSI_DEV_MEDIA_CHANGED, /*!< Unit attention: media changed */
	SCSI_DEV_ERROR,         /*!< Unrecoverable transport or sense error */
	SCSI_DEV_REMOVED,       /*!< Device or LUN detached */
};

/** Logical SCSI device visible to the mid-layer */
struct scsi_device {
	/** Transport controller (e.g. UHC) */
	const struct device *dev;
	/** Transport driver API */
	const struct scsi_driver_api *api;
	/** Logical unit number on the transport */
	uint8_t lun;
	/** Block size in bytes (from READ CAPACITY) */
	uint32_t block_size;
	/** Number of logical blocks (last LBA + 1) */
	uint64_t block_count;
	/** Removable media flag from INQUIRY */
	bool removable;
	/** Write protect flag from MODE SENSE when known */
	bool write_protected;
	/** Use READ/WRITE(16) when block count exceeds 32-bit LBA range */
	bool use_16byte_cmds;
	/** Current device state */
	enum scsi_device_state state;
	/** Serializes command execution on this LUN */
	struct k_mutex lock;
};

/**
 * @brief Initialize a SCSI device handle
 *
 * @param sdev SCSI device object (caller-owned, zero-initialized)
 * @param dev Transport controller device (e.g. UHC)
 * @param api Transport @ref scsi_driver_api
 * @param lun Target logical unit number
 *
 * @return 0 on success, negative errno on failure
 */
int scsi_device_init(struct scsi_device *sdev, const struct device *dev,
		     const struct scsi_driver_api *api, uint8_t lun);

/**
 * @brief Execute a SCSI transfer via the bound transport
 *
 * On CHECK CONDITION the SCSI status is preserved in @a xfer->status and the
 * function returns a negative errno (typically @c -EIO). Callers may issue
 * @ref scsi_request_sense() to retrieve sense data.
 *
 * @param sdev Initialized SCSI device
 * @param xfer Transfer descriptor (input/output)
 *
 * @return 0 on success, negative errno on transport or CHECK CONDITION failure
 */
int scsi_exec(struct scsi_device *sdev, struct scsi_xfer *xfer);

int scsi_test_unit_ready(struct scsi_device *sdev);

int scsi_inquiry(struct scsi_device *sdev, void *buf, uint32_t len);

int scsi_request_sense(struct scsi_device *sdev, void *buf, uint32_t len);

int scsi_read_capacity_10(struct scsi_device *sdev, void *buf, uint32_t len);

int scsi_read_capacity_16(struct scsi_device *sdev, void *buf, uint32_t len);

int scsi_read_10(struct scsi_device *sdev, uint32_t lba, uint16_t blocks, void *buf, uint32_t len);

int scsi_write_10(struct scsi_device *sdev, uint32_t lba, uint16_t blocks, const void *buf,
		  uint32_t len);

int scsi_read_16(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, void *buf, uint32_t len);

int scsi_write_16(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, const void *buf,
		  uint32_t len);

/**
 * @brief Read logical blocks using READ(10) or READ(16) as appropriate
 */
int scsi_io_read(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, void *buf, uint32_t len);

/**
 * @brief Write logical blocks using WRITE(10) or WRITE(16) as appropriate
 */
int scsi_io_write(struct scsi_device *sdev, uint64_t lba, uint32_t blocks, const void *buf,
		  uint32_t len);

int scsi_verify_10(struct scsi_device *sdev, uint32_t lba, uint16_t blocks);

int scsi_mode_sense_6(struct scsi_device *sdev, void *buf, uint32_t len);

int scsi_mode_sense_6_params(struct scsi_device *sdev, void *buf, uint32_t len, uint8_t pc,
			     uint8_t page_code, uint8_t subpage, bool disable_block_descriptors);

int scsi_start_stop_unit(struct scsi_device *sdev, bool start);

int scsi_synchronize_cache_10(struct scsi_device *sdev);

/**
 * @brief Probe a SCSI device (INQUIRY, TEST UNIT READY, READ CAPACITY)
 *
 * Populates @a sdev block geometry and removable flag on success.
 */
int scsi_device_probe(struct scsi_device *sdev);

/**
 * @brief Parse standard INQUIRY data for removable media flag
 *
 * @param inq INQUIRY response buffer (at least 36 bytes)
 * @param len Buffer length
 * @param removable Output removable flag (may be NULL)
 *
 * @return 0 on success, negative errno on failure
 */
int scsi_parse_inquiry(const void *inq, uint32_t len, bool *removable);

/**
 * @brief Map SCSI status / sense to a negative errno
 *
 * @param status SCSI status byte
 * @param sense Parsed sense (may be NULL)
 */
int scsi_status_to_errno(uint8_t status, const struct scsi_sense *sense);

/**
 * @brief Parse READ CAPACITY(10) response buffer
 *
 * @param buf 8-byte READ CAPACITY(10) data
 * @param len Buffer length
 * @param last_lba Output last logical block address (big-endian decoded)
 * @param block_size Output block size in bytes
 *
 * @return 0 on success, negative errno on failure
 */
int scsi_parse_read_capacity_10(const void *buf, uint32_t len, uint32_t *last_lba,
				uint32_t *block_size);

int scsi_parse_read_capacity_16(const void *buf, uint32_t len, uint64_t *last_lba,
				uint32_t *block_size);

/** @name SCSI Generic pass-through direction (matches BSG SG_DXFER_*) */
/**@{*/
#define SCSI_SG_DXFER_NONE     1
#define SCSI_SG_DXFER_TO_DEV   2
#define SCSI_SG_DXFER_FROM_DEV 3
/**@}*/

/** Raw SCSI pass-through request for @ref scsi_sg_io() */
struct scsi_sg_io {
	/** Command descriptor block */
	const void *cdb;
	/** CDB length in bytes */
	uint8_t cdb_len;
	/** Direction: @ref SCSI_SG_DXFER_NONE and related constants */
	int dxfer_dir;
	/** Data buffer for the data stage */
	void *dxferp;
	/** Data stage length in bytes */
	uint32_t dxfer_len;
	/** Sense data output buffer */
	void *sense;
	/** Sense buffer capacity in bytes */
	uint32_t sense_len;
	/** SCSI status byte (output) */
	uint8_t status;
};

/**
 * @brief Execute a raw SCSI command on a bound device
 *
 * Populates @a io->status and sense buffer on CHECK CONDITION.
 */
int scsi_sg_io(struct scsi_device *sdev, struct scsi_sg_io *io);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_SCSI_H_ */
