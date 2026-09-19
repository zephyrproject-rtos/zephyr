/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI lower-level driver (transport) interface
 *
 * Transport drivers implement @ref scsi_driver_api; the SCSI mid-layer
 * builds CDBs and invokes @ref scsi_exec().
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_SCSI_SCSI_DRIVER_H_
#define ZEPHYR_INCLUDE_SCSI_SCSI_DRIVER_H_

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct scsi_xfer;

/**
 * @brief SCSI transport driver operations
 *
 * Only @a exec is mandatory. @a reset and @a get_max_lun are optional and
 * may be NULL when not supported by the transport.
 */
struct scsi_driver_api {
	/** Execute one SCSI command described by @a xfer */
	int (*exec)(const struct device *dev, struct scsi_xfer *xfer);
	/** Reset the transport / device SCSI path (optional) */
	int (*reset)(const struct device *dev);
	/** Return maximum LUN index supported (optional) */
	int (*get_max_lun)(const struct device *dev, uint8_t *max_lun);
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_SCSI_DRIVER_H_ */
