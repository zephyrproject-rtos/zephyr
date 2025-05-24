/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UFS Operations Header File
 *
 * This file contains the declarations and definitions for UFS operations
 */

#ifndef ZEPHYR_INCLUDE_UFS_UFS_OPS_H_
#define ZEPHYR_INCLUDE_UFS_UFS_OPS_H_

/* INCLUDES */
#include <zephyr/ufs/ufs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UFS QUERY IOCTL macros */

/* UFS_QRY_IOCTL_ATTR - IOCTL request code for UFS attribute query */
#define UFS_QRY_IOCTL_ATTR	0x51

/* UFS_QRY_IOCTL_FLAG - IOCTL request code for UFS flag query */
#define UFS_QRY_IOCTL_FLAG	0x52

/* UFS_QRY_IOCTL_DESC - IOCTL request code for UFS descriptor query */
#define UFS_QRY_IOCTL_DESC	0x53

/* ENUMS */
/**
 * @brief Message codes for SCSI Generic (SG) requests
 */
enum ufs_sg_req_msgcode {
	UFS_SG_QUERY_REQ = 0x00, /**< Request for UFS query operation */
	UFS_SG_TASK_REQ = 0x01, /**< Request for UFS task operation */
};

/* STRUCTURES */

/**
 * @brief UFS attribute query IOCTL request
 */
struct ufs_qry_ioctl_attr {
	uint8_t attr_id; /**< UFS attribute identifier for the query */
};

/**
 * @brief UFS flag query IOCTL request
 */
struct ufs_qry_ioctl_flag {
	uint8_t flag_id; /**< UFS flag identifier for the query */
};

/**
 * @brief UFS descriptor query IOCTL request
 */
struct ufs_qry_ioctl_desc {
	uint8_t desc_id;      /**< Descriptor identifier for the query */
	uint8_t index;        /**< Index for the descriptor query */
	uint8_t param_offset; /**< Offset for the parameter in the descriptor query */
};

/**
 * @brief UFS IOCTL request
 *
 * This structure holds data for different types of UFS query IOCTL requests,
 * including attribute, flag, and descriptor queries.
 */
struct ufs_qry_ioctl_req {
	uint32_t ioctl_id;                      /**< IOCTL identifier for the request */
	union {
		struct ufs_qry_ioctl_attr attr; /**< UFS attribute query data */
		struct ufs_qry_ioctl_flag flag; /**< UFS flag query data */
		struct ufs_qry_ioctl_desc desc; /**< UFS descriptor query data */
	};
};

/**
 * @brief UFS SCSI Generic (SG) request
 *
 * This structure is used for representing SCSI Generic requests,
 * querying UFS attributes, flags, or descriptors.
 */
struct ufs_sg_req {
	int32_t msgcode;                         /**< Message code for the request */
	struct ufs_qry_ioctl_req *req_qry_ioctl; /**< Pointer to the UFS query IOCTL request data */
};

/* PUBLIC FUNCTIONS */

/**
 * @brief Handle a SCSI Generic (SG) request for UFS
 *
 * This function dispatches an SG_IO request to the appropriate UFS operation.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param arg Argument for the request, which could be a UFS query or other data
 *
 * @return 0 on success, negative error code on failure
 */
int32_t ufs_sg_request(struct ufs_host_controller *ufshc, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_UFS_UFS_OPS_H_ */
