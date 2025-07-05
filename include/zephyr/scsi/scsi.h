/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SCSI_H_
#define ZEPHYR_INCLUDE_SCSI_H_

#include <zephyr/drivers/dma.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS */

/* SCSI Command Operation Codes */
#define SCSI_TST_U_RDY	0x00 /* Test Unit Ready */
#define SCSI_READ10	0x28 /* Read 10-byte */
#define SCSI_READ16	0x88 /* Read 16-byte */
#define SCSI_WRITE10	0x2A /* Write 10-byte */
#define SCSI_WRITE16	0x8A /* Write 16-byte */

/* Maximum Number of Retries for SCSI Cmds */
#define SCSI_MAX_RETRIES	(3)

/* Maximum size of the Command Descriptor Block (CDB) */
#define MAX_CDB_SIZE		(16U)

/* SCSI IOCTL Command Codes */
#define SCSI_IOCTL_TEST_UNIT_READY	(0x02) /* checks if the device is ready */
#define SG_IO				(0x85) /* issue SCSI commands to the device */

/* Block Storage Generic (BSG) Protocol Constants */
#define BSG_PROTOCOL_SCSI		(0U)
#define BSG_SUB_PROTOCOL_SCSI_CMD	(0U)
#define BSG_SUB_PROTOCOL_SCSI_TRANSPORT	(1U)

/* Data Transfer Directions for SCSI Generic (SG) Commands */
#define SG_DXFER_NONE		(1)
#define SG_DXFER_TO_DEV		(2)
#define SG_DXFER_FROM_DEV	(3)

/* STRUCTURES */

/**
 * @brief Structure to hold information about a SCSI command to be processed
 */
struct scsi_cmd {
	uint8_t cmd[MAX_CDB_SIZE]; /**< Command descriptor block (CDB) */
	uint8_t lun;               /**< Target LUN (Logical Unit Number) */
	uint8_t cmdlen;            /**< Length of the command */
	uint64_t datalen;          /**< Total length of data to be transferred */
	uint8_t *pdata;            /**< Pointer to data to be transferred */
	uint8_t dma_dir;           /**< Direction of data transfer (e.g., read/write) */
} __packed;

/**
 * @brief Structure representing LUN (Logical Unit Number) information.
 */
struct lun_info {
	uint32_t lun_id;      /**< ID of the LUN */
	bool lun_enabled;     /**< Indicates if the LUN is enabled */
	uint32_t block_size;  /**< Size of each block in bytes */
	uint64_t block_count; /**< Total number of blocks in the LUN */
} __packed;

/**
 * @brief Information about a SCSI host controller.
 */
struct scsi_host_info {
	sys_slist_t sdevices;          /**< List of connected SCSI devices */
	const struct scsi_ops *ops;    /**< Pointer to the SCSI operations structure */
	void *hostdata;                /**< Pointer to host-specific data */
	struct device *parent;         /**< Parent device associated with the host */
};

/**
 * @brief Struct representing a SCSI device
 */
struct scsi_device {
	sys_snode_t node;            /**< Node in the list of SCSI devices */
	struct scsi_host_info *host; /**< Pointer to the associated SCSI host controller */
	uint8_t lun;                 /**< LUN ID for the device */
	uint32_t sector_size;        /**< Size of sectors in bytes */
	uint32_t capacity;           /**< Total capacity of the device in blocks */
};

/**
 * @brief Structure to represent a SCSI Generic (SG) I/O request
 */
struct sg_io_req {
	uint32_t protocol;         /**< SCSI protocol type */
	uint32_t subprotocol;      /**< Sub-protocol type (0 - SCSI command, 1 - SCSI transport) */
	void *request;             /**< Pointer to the SCSI CDB or transport layer request */
	uint32_t request_len;      /**< Length of the request in bytes */
	void *response;            /**< Pointer to the response buffer */
	uint32_t max_response_len; /**< Maximum length of the response buffer */
	int32_t dxfer_dir;         /**< Data transfer direction */
	uint32_t dxfer_len;        /**< Length of the data to transfer */
	void *dxferp;              /**< Pointer to the data to be transferred */
};

/**
 * @brief Operations for managing SCSI commands
 */
struct scsi_ops {
	/**
	 * @brief Execute a SCSI command on a given SCSI device.
	 *
	 * @param sdev Pointer to the SCSI device handle
	 * @param cmd Pointer to the SCSI command to execute
	 *
	 * @return 0 on success, negative value on error
	 */
	int32_t (*exec)(struct scsi_device *sdev, struct scsi_cmd *cmd);
};

/* FUNCTION PROTOTYPES */

/**
 * @brief Perform a write operation on the SCSI device.
 *
 * This function sends a write command to the SCSI device.
 *
 * @param scsi_dev Pointer to the scsi_device structure.
 * @param sector Starting sector number.
 * @param count Number of sectors to write.
 * @param buf Pointer to the data buffer to write.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
int32_t scsi_write(struct scsi_device *scsi_dev, uint64_t sector, uint32_t count,
		   const uint8_t *buf);

/**
 * @brief Perform a read operation on the SCSI device.
 *
 * This function sends a read command to the SCSI device.
 *
 * @param scsi_dev Pointer to the scsi_device structure.
 * @param sector Starting sector number.
 * @param count Number of sectors to read.
 * @param buf Pointer to the buffer to store the read data.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
int32_t scsi_read(struct scsi_device *scsi_dev, uint64_t sector, uint32_t count, uint8_t *buf);

/**
 * @brief Allocate a new scsi_host_info.
 *
 * This function allocates and initializes a new scsi_host_info structure,
 * setting the operations field to the given scsi operations.
 *
 * @param sops Pointer to the scsi operations structure.
 *
 * @return Pointer to the newly allocated scsi_host_info, or NULL on failure.
 */
struct scsi_host_info *scsi_host_alloc(const struct scsi_ops *sops);

/**
 * @brief Add a new LUN to the SCSI host.
 *
 * This function adds a new LUN to the host, initializing the associated
 * scsi_device if necessary. It also stores the sector size and capacity
 * for the device.
 *
 * @param shost Pointer to the SCSI host.
 * @param lun Pointer to the lun_info structure containing LUN information.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int32_t scsi_add_lun_host(struct scsi_host_info *shost, struct lun_info *lun);

/**
 * @brief Find a SCSI device by host and LUN.
 *
 * This function searches the list of devices attached to the given host
 * and returns the device with the specified LUN.
 *
 * @param shost Pointer to the SCSI host.
 * @param lun Logical Unit Number (LUN) of the SCSI device.
 *
 * @return Pointer to the scsi_device structure, or NULL if no matching
 *         device is found.
 */
struct scsi_device *scsi_device_lookup_by_host(struct scsi_host_info *shost, uint32_t lun);

/**
 * @brief Dispatch an ioctl to a SCSI device.
 *
 * This function dispatches the appropriate ioctl to the SCSI device based on
 * the provided command.
 *
 * @param sdev Pointer to the scsi_device structure.
 * @param cmd The ioctl command.
 * @param arg Data associated with the ioctl.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
int32_t scsi_ioctl(struct scsi_device *sdev, int32_t cmd, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_H_ */
