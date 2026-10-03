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
#define BSG_PROTOCOL_SCSI               0U
#define BSG_SUB_PROTOCOL_SCSI_CMD       0U
#define BSG_SUB_PROTOCOL_SCSI_TRANSPORT 1U
#define SG_IO                           0x85
#define SG_DXFER_NONE                   1
#define SG_DXFER_TO_DEV                 2
#define SG_DXFER_FROM_DEV               3
/**@}*/

/** BSG I/O request passed to @c DISK_IOCTL @c SG_IO */
struct sg_io_req {
	uint32_t protocol;
	uint32_t subprotocol;
	void *request;
	uint32_t request_len;
	void *response;
	uint32_t max_response_len;
	int32_t dxfer_dir;
	uint32_t dxfer_len;
	void *dxferp;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SG_IO_H_ */
