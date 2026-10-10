/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Block SCSI Generic (BSG) ioctl types for disk_access
 *
 * SG_IO pass-through ioctl types for SCSI-backed disk_access volumes.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISK_SG_IO_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISK_SG_IO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name Block SCSI Generic (BSG) ioctl constants */
/**@{*/
#define BSG_PROTOCOL_SCSI               0U   /*!< SCSI protocol selector */
#define BSG_SUB_PROTOCOL_SCSI_CMD       0U   /*!< SCSI command sub-protocol */
#define BSG_SUB_PROTOCOL_SCSI_TRANSPORT 1U   /*!< SCSI transport sub-protocol */
#define SG_IO                           0x85 /*!< SG_IO ioctl command number */
#define SG_DXFER_NONE                   1    /*!< No data transfer */
#define SG_DXFER_TO_DEV                 2    /*!< Host to device */
#define SG_DXFER_FROM_DEV               3    /*!< Device to host */
/**@}*/

/** BSG I/O request passed to @c DISK_IOCTL @c SG_IO */
struct sg_io_req {
	/** @ref BSG_PROTOCOL_SCSI */
	uint32_t protocol;
	/** @ref BSG_SUB_PROTOCOL_SCSI_CMD or transport variant */
	uint32_t subprotocol;
	/** CDB or transport request buffer */
	void *request;
	/** Length of @a request in bytes */
	uint32_t request_len;
	/** Sense or transport response buffer */
	void *response;
	/** Size of @a response buffer in bytes */
	uint32_t max_response_len;
	/** @ref SG_DXFER_NONE and related constants */
	int32_t dxfer_dir;
	/** Data stage length in bytes */
	uint32_t dxfer_len;
	/** Data stage buffer */
	void *dxferp;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SG_IO_H_ */
