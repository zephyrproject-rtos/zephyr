/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/scsi/scsi.h>

LOG_MODULE_REGISTER(scsi, CONFIG_SCSI_LOG_LEVEL);

/*
 * This file implements SCSI subsystem support to
 * interface with Storage Host Controllers like UFS
 *
 * supports SCSI cmds
 *  - TEST_UNIT_READY
 *  - READ
 *  - WRITE
 */

/**
 * @brief Dispatch a command to the low-level driver.
 *
 * This function calls the low-level driver to execute the specified SCSI
 * command. The execution is done via the appropriate function in the
 * host's operations (sdev->host->ops->exec).
 *
 * @param sdev Pointer to the scsi_device structure.
 * @param cmd Pointer to the scsi_cmd structure to be dispatched.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int32_t scsi_exec(struct scsi_device *sdev, struct scsi_cmd *cmd)
{
	const struct scsi_ops *ops = (const struct scsi_ops *)sdev->host->ops;

	if (ops->exec == NULL) {
		return -EINVAL;
	}

	/* Execute the command if supported */
	return ops->exec(sdev, cmd);
}

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
struct scsi_host_info *scsi_host_alloc(const struct scsi_ops *sops)
{
	struct scsi_host_info *shost = NULL;

	/* Allocate memory for the SCSI host structure */
	shost = (struct scsi_host_info *)k_malloc(sizeof(struct scsi_host_info));
	if (shost == NULL) {
		return NULL;
	}

	/* Initialize the list of devices and set host operations */
	(void)memset(shost, 0, sizeof(struct scsi_host_info));
	sys_slist_init(&shost->sdevices);
	shost->ops = sops;

	return shost;
}

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
struct scsi_device *scsi_device_lookup_by_host(struct scsi_host_info *shost,
					       uint32_t lun)
{
	struct scsi_device *sdev = NULL, *itr_sdev;
	sys_snode_t *itr_node;

	if (shost == NULL) {
		return NULL;
	}

	/* Iterate through the list of devices attached to the host */
	SYS_SLIST_FOR_EACH_NODE(&shost->sdevices, itr_node) {
		itr_sdev = (struct scsi_device *)CONTAINER_OF(itr_node,
							      struct scsi_device,
							      node);

		/* Check if the device matches the given LUN. */
		if ((itr_sdev->lun) == lun) {
			sdev = itr_sdev;
			break;
		}
	}

	return sdev;
}

/**
 * @brief Allocate and initialize a scsi_device.
 *
 * This function allocates and initializes a new scsi_device structure
 * for the given host and LUN, and appends it to the host's device list.
 *
 * @param shost Pointer to the SCSI host.
 * @param lun Logical Unit Number (LUN) for the new SCSI device.
 *
 * @return Pointer to the newly allocated scsi_device, or NULL on failure.
 */
static struct scsi_device *scsi_alloc_sdev(struct scsi_host_info *shost,
					   uint32_t lun)
{
	struct scsi_device *sdev;

	if (shost == NULL) {
		return NULL;
	}

	/* Allocate memory for the new device structure */
	sdev = (struct scsi_device *)k_malloc(sizeof(struct scsi_device));
	if (sdev == NULL) {
		return NULL;
	}

	/* Associate the device with the host and set its LUN */
	(void)memset(sdev, 0, sizeof(struct scsi_device));
	sdev->host = shost;
	sdev->lun = (uint8_t)lun;

	/* Append the new device to the host's device list */
	sys_slist_append(&shost->sdevices, &sdev->node);

	return sdev;
}

/**
 * @brief Add a new LUN to the SCSI host.
 *
 * This function adds a new LUN to the host, initializing the associated
 * scsi_device if necessary. It also stores the sector size and capacity
 * for the device.
 *
 * @param shost Pointer to the SCSI host.
 * @param luninfo Pointer to the lun_info structure containing LUN information.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int32_t scsi_add_lun_host(struct scsi_host_info *shost, struct lun_info *lun)
{
	struct scsi_device *sdev;
	int32_t res = 0;

	/* Check if the LUN information is valid and enabled */
	if ((lun == NULL) || (lun->lun_enabled == false)) {
		res = -EINVAL;
		goto out;
	}

	/* Try to find the device associated with the given LUN */
	sdev = scsi_device_lookup_by_host(shost, lun->lun_id);
	if (sdev == NULL) {
		/* If no device found, allocate a new one for this LUN */
		sdev = scsi_alloc_sdev(shost, lun->lun_id);
	}

	if (sdev == NULL) {
		res = -ENOMEM;
		goto out;
	}

	/* Save sector size and capacity based on the LUN info */
	sdev->sector_size = lun->block_size;
	sdev->capacity = (uint32_t)lun->block_count;

out:
	return res;
}

/**
 * @brief Frame the "Test Unit Ready" SCSI command.
 *
 * This function populates the command block for a "Test Unit Ready"
 * command (SCSI TST_U_RDY).
 *
 * @param pccb: Pointer to the scsi_cmd structure to be populated.
 */
static void scsi_setup_test_unit_ready(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = SCSI_TST_U_RDY;
	pccb->cmd[1] = 0;
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	pccb->cmd[4] = 0;
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
}

/**
 * @brief Frame the "READ_10" or "READ_16" SCSI command.
 *
 * This function selects either a "READ_10" or "READ_16" command based on
 * the number of blocks and starting address, and populates the command
 * block accordingly.
 *
 * @param pccb: Pointer to the scsi_cmd structure to be populated.
 * @param start: Starting block number.
 * @param blocks: Number of blocks to read.
 */
static void scsi_setup_read(struct scsi_cmd *pccb, uint64_t start,
			    uint32_t blocks)
{
	if ((blocks > (uint32_t)UINT16_MAX) || (start > (uint64_t)UINT32_MAX)) {
		/* Read(16) for larger addresses (64-bit) and block counts (32-bit) */
		pccb->cmd[0] = SCSI_READ16;
		pccb->cmd[1] = 0;

		/* start address (64-bit) */
		pccb->cmd[2] = (uint8_t)(start >> 56) & (uint8_t)(0xff);
		pccb->cmd[3] = (uint8_t)(start >> 48) & (uint8_t)(0xff);
		pccb->cmd[4] = (uint8_t)(start >> 40) & (uint8_t)(0xff);
		pccb->cmd[5] = (uint8_t)(start >> 32) & (uint8_t)(0xff);
		pccb->cmd[6] = (uint8_t)(start >> 24) & (uint8_t)(0xff);
		pccb->cmd[7] = (uint8_t)(start >> 16) & (uint8_t)(0xff);
		pccb->cmd[8] = (uint8_t)(start >> 8) & (uint8_t)(0xff);
		pccb->cmd[9] = (uint8_t)start & (uint8_t)(0xff);
		pccb->cmd[10] = 0;

		/* block count (32-bit) */
		pccb->cmd[11] = (uint8_t)(blocks >> 24) & (uint8_t)(0xff);
		pccb->cmd[12] = (uint8_t)(blocks >> 16) & (uint8_t)(0xff);
		pccb->cmd[13] = (uint8_t)(blocks >> 8) & (uint8_t)(0xff);
		pccb->cmd[14] = (uint8_t)blocks & (uint8_t)(0xff);
		pccb->cmd[15] = 0;
		pccb->cmdlen = 16U;
	} else {
		/* Read(10) command (for smaller addresses and block counts) */
		pccb->cmd[0] = SCSI_READ10;
		pccb->cmd[1] = 0;

		/* start address (32-bit) */
		pccb->cmd[2] = (uint8_t)(start >> 24) & (uint8_t)(0xff);
		pccb->cmd[3] = (uint8_t)(start >> 16) & (uint8_t)(0xff);
		pccb->cmd[4] = (uint8_t)(start >> 8) & (uint8_t)(0xff);
		pccb->cmd[5] = (uint8_t)start & (uint8_t)(0xff);
		pccb->cmd[6] = 0;

		/* block count (16-bit) */
		pccb->cmd[7] = (uint8_t)(blocks >> 8) & (uint8_t)(0xff);
		pccb->cmd[8] = (uint8_t)blocks & (uint8_t)(0xff);
		pccb->cmd[9] = 0;
		pccb->cmdlen = 10U;
	}
}

/**
 * @brief Frame the "WRITE_10" or "WRITE_16" SCSI command.
 *
 * This function selects either a "WRITE_10" or "WRITE_16" command based on
 * the number of blocks and starting address, and populates the command
 * block accordingly.
 *
 * @param pccb Pointer to the scsi_cmd structure to be populated.
 * @param start Starting block number.
 * @param blocks Number of blocks to write.
 */
static void scsi_setup_write(struct scsi_cmd *pccb, uint64_t start,
			     uint32_t blocks)
{
	if ((blocks > (uint32_t)UINT16_MAX) || (start > (uint64_t)UINT32_MAX)) {
		/* Write(16) for larger addresses (64-bit) and block counts (32-bit) */
		pccb->cmd[0] = SCSI_WRITE16;
		pccb->cmd[1] = 0;

		/* start address (64-bit) */
		pccb->cmd[2] = (uint8_t)(start >> 56) & (uint8_t)(0xff);
		pccb->cmd[3] = (uint8_t)(start >> 48) & (uint8_t)(0xff);
		pccb->cmd[4] = (uint8_t)(start >> 40) & (uint8_t)(0xff);
		pccb->cmd[5] = (uint8_t)(start >> 32) & (uint8_t)(0xff);
		pccb->cmd[6] = (uint8_t)(start >> 24) & (uint8_t)(0xff);
		pccb->cmd[7] = (uint8_t)(start >> 16) & (uint8_t)(0xff);
		pccb->cmd[8] = (uint8_t)(start >> 8) & (uint8_t)(0xff);
		pccb->cmd[9] = (uint8_t)(start) & (uint8_t)(0xff);
		pccb->cmd[10] = 0;

		/* block count (32-bit) */
		pccb->cmd[11] = (uint8_t)(blocks >> 24) & (uint8_t)(0xff);
		pccb->cmd[12] = (uint8_t)(blocks >> 16) & (uint8_t)(0xff);
		pccb->cmd[13] = (uint8_t)(blocks >> 8) & (uint8_t)(0xff);
		pccb->cmd[14] = (uint8_t)(blocks) & (uint8_t)(0xff);
		pccb->cmd[15] = 0;
		pccb->cmdlen = 16U;
	} else {
		/* Write(10) command (for smaller addresses and block counts) */
		pccb->cmd[0] = SCSI_WRITE10;
		pccb->cmd[1] = 0;

		/* start address (32-bit) */
		pccb->cmd[2] = (uint8_t)(start >> 24) & (uint8_t)(0xff);
		pccb->cmd[3] = (uint8_t)(start >> 16) & (uint8_t)(0xff);
		pccb->cmd[4] = (uint8_t)(start >> 8) & (uint8_t)(0xff);
		pccb->cmd[5] = (uint8_t)(start) & (uint8_t)(0xff);
		pccb->cmd[6] = 0;

		/* block count (16-bit) */
		pccb->cmd[7] = (uint8_t)(blocks >> 8) & (uint8_t)(0xff);
		pccb->cmd[8] = (uint8_t)(blocks) & (uint8_t)(0xff);
		pccb->cmd[9] = 0;
		pccb->cmdlen = 10U;
	}
}

/**
 * @brief Test if the SCSI unit is ready.
 *
 * This function sends a "Test Unit Ready" (TUR) command to the device.
 *
 * @param scsi_dev Pointer to the scsi_device structure.
 * @param lun Logical Unit Number (LUN).
 *
 * @return 0 if successful, or a negative error code on failure.
 */
static int32_t scsi_test_unit_ready(struct scsi_device *scsi_dev, int32_t lun)
{
	int32_t ret;
	int32_t retries = SCSI_MAX_RETRIES;
	struct scsi_cmd cmd = {0};

	/* Set up the SCSI command structure for TUR */
	cmd.lun = (uint8_t)lun;
	cmd.datalen = (0);
	cmd.dma_dir = (uint8_t)PERIPHERAL_TO_PERIPHERAL;

	/* Setup the "Test Unit Ready" command */
	scsi_setup_test_unit_ready(&cmd);

	/* Execute the SCSI command and return the result */
	do {
		ret = scsi_exec(scsi_dev, &cmd);
		retries--;
	} while ((ret < 0) && (retries > 0));

	return ret;
}

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
int32_t scsi_write(struct scsi_device *scsi_dev, uint64_t sector,
		   uint32_t count, const uint8_t *buf)
{
	int32_t ret;
	struct scsi_cmd cmd = {0};

	if (scsi_dev == NULL) {
		return -EINVAL;
	}

	/* Set up the SCSI command structure */
	cmd.lun = scsi_dev->lun;
	cmd.pdata = (uint8_t *)(uintptr_t)buf;
	cmd.datalen = ((uint64_t)(scsi_dev->sector_size) * (uint64_t)(count));
	cmd.dma_dir = (uint8_t)MEMORY_TO_PERIPHERAL;

	/* Setup the "Write" command with specified sector and count */
	scsi_setup_write(&cmd, sector, count);

	/* Execute the SCSI command and return the result */
	ret = scsi_exec(scsi_dev, &cmd);

	return ret;
}

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
int32_t scsi_read(struct scsi_device *scsi_dev, uint64_t sector,
		  uint32_t count, uint8_t *buf)
{
	int32_t ret;
	struct scsi_cmd cmd = {0};

	if (scsi_dev == NULL) {
		return -EINVAL;
	}

	/* Set up the SCSI command structure */
	cmd.lun = scsi_dev->lun;
	cmd.pdata = (uint8_t *)buf;
	cmd.datalen = ((uint64_t)(scsi_dev->sector_size) * (uint64_t)(count));
	cmd.dma_dir = (uint8_t)PERIPHERAL_TO_MEMORY;

	/* Setup the "Read" command with specified sector and count */
	scsi_setup_read(&cmd, sector, count);

	/* Execute the SCSI command and return the result */
	ret = scsi_exec(scsi_dev, &cmd);

	return ret;
}

/**
 * @brief Handle SG_IO ioctl for SCSI device.
 *
 * This function handles the SG_IO ioctl command for the SCSI device,
 * which allows for sending arbitrary SCSI commands.
 *
 * @param sdev Pointer to the scsi_device structure.
 * @param arg Pointer to the ioctl arguments.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
static int32_t scsi_ioctl_sg_io(struct scsi_device *sdev, void *arg)
{
	const struct sg_io_req *req = (const struct sg_io_req *)arg;
	struct scsi_cmd scmd = {0};
	int32_t ret;

	/* Validate the request */
	if ((req == NULL) || (req->protocol != (uint32_t)BSG_PROTOCOL_SCSI) ||
	    (req->subprotocol != (uint32_t)BSG_SUB_PROTOCOL_SCSI_CMD)) {
		ret = -EINVAL;
		goto out;
	}

	/* Initialize the SCSI command structure with ioctl request */
	scmd.lun = sdev->lun;
	scmd.cmdlen = (uint8_t)(req->request_len);
	if (scmd.cmdlen > sizeof(scmd.cmd)) {
		ret = -EINVAL;
		goto out;
	}
	(void)memcpy((void *)&scmd.cmd[0], req->request, scmd.cmdlen);
	scmd.dma_dir = (uint8_t)(req->dxfer_dir);
	scmd.datalen = req->dxfer_len;
	scmd.pdata = req->dxferp;

	/* Execute the SCSI command and return the result */
	ret = scsi_exec(sdev, &scmd);

out:
	return ret;
}

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
int32_t scsi_ioctl(struct scsi_device *sdev, int32_t cmd, void *arg)
{
	int32_t ret = -EINVAL;

	/* Validate that the device is not NULL */
	if (sdev == NULL) {
		return ret;
	}

	/* Process the specific IOCTL command */
	switch (cmd) {
	case SCSI_IOCTL_TEST_UNIT_READY:
		/* Check if the device is ready */
		ret = scsi_test_unit_ready(sdev, (int32_t)sdev->lun);
		break;
	case SG_IO:
		/* Process a SCSI command via SG_IO */
		ret = scsi_ioctl_sg_io(sdev, arg);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
