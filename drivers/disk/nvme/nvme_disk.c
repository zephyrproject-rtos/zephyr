/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2022 Intel Corp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nvme, CONFIG_NVME_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include "nvme.h"

static int nvme_disk_init(struct disk_info *disk)
{
	return 0;
}

static int nvme_disk_status(struct disk_info *disk)
{
	return 0;
}

static int nvme_disk_read(struct disk_info *disk,
			  uint8_t *data_buf,
			  uint32_t start_sector,
			  uint32_t num_sector)
{
	struct nvme_namespace *ns = CONTAINER_OF(disk->name,
						 struct nvme_namespace, name);
	struct nvme_completion_poll_status status =
		NVME_CPL_STATUS_POLL_INIT(status);
	struct nvme_request *request;
	uint32_t payload_size;
	int ret = 0;

	nvme_lock(disk->dev);

	payload_size = num_sector * nvme_namespace_get_sector_size(ns);

	request = nvme_allocate_request_vaddr((void *)data_buf, payload_size,
					      nvme_completion_poll_cb, &status);
	if (request == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	nvme_namespace_read_cmd(&request->cmd, ns->id,
				start_sector, num_sector);

	/* We use only the first ioq atm
	 * ToDo: use smp cpu id and use it to select ioq
	 */
	nvme_cmd_qpair_submit_request(ns->ctrlr->ioq, request);

	nvme_completion_poll(&status);
	if (nvme_cpl_status_is_error(&status)) {
		LOG_WRN("Reading at sector %u (count %d) on disk %s failed",
			start_sector, num_sector, ns->name);
		ret = -EIO;
	}
out:
	nvme_unlock(disk->dev);
	return ret;
}

static int nvme_disk_write(struct disk_info *disk,
			   const uint8_t *data_buf,
			   uint32_t start_sector,
			   uint32_t num_sector)
{
	struct nvme_namespace *ns = CONTAINER_OF(disk->name,
						 struct nvme_namespace, name);
	struct nvme_completion_poll_status status =
		NVME_CPL_STATUS_POLL_INIT(status);
	struct nvme_request *request;
	uint32_t payload_size;
	int ret = 0;

	nvme_lock(disk->dev);

	payload_size = num_sector * nvme_namespace_get_sector_size(ns);

	request = nvme_allocate_request_vaddr((void *)data_buf, payload_size,
					      nvme_completion_poll_cb, &status);
	if (request == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	nvme_namespace_write_cmd(&request->cmd, ns->id,
				 start_sector, num_sector);

	/* We use only the first ioq atm
	 * ToDo: use smp cpu id and use it to select ioq
	 */
	nvme_cmd_qpair_submit_request(ns->ctrlr->ioq, request);

	nvme_completion_poll(&status);
	if (nvme_cpl_status_is_error(&status)) {
		LOG_WRN("Writing at sector %u (count %d) on disk %s failed",
			start_sector, num_sector, ns->name);
		ret = -EIO;
	}
out:
	nvme_unlock(disk->dev);
	return ret;
}

static int nvme_disk_flush(struct nvme_namespace *ns)
{
	struct nvme_completion_poll_status status =
		NVME_CPL_STATUS_POLL_INIT(status);
	struct nvme_request *request;

	request = nvme_allocate_request_null(nvme_completion_poll_cb, &status);
	if (request == NULL) {
		return -ENOMEM;
	}

	nvme_namespace_flush_cmd(&request->cmd, ns->id);

	/* We use only the first ioq
	 * ToDo: use smp cpu id and use it to select ioq
	 */
	nvme_cmd_qpair_submit_request(ns->ctrlr->ioq, request);

	nvme_completion_poll(&status);
	if (nvme_cpl_status_is_error(&status)) {
		LOG_ERR("Flushing disk %s failed", ns->name);
		return -EIO;
	}

	return 0;
}

static int nvme_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct nvme_namespace *ns = CONTAINER_OF(disk->name,
						 struct nvme_namespace, name);
	int ret = 0;

	nvme_lock(disk->dev);

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		if (!buff) {
			ret = -EINVAL;
			break;
		}

		*(uint32_t *)buff = nvme_namespace_get_num_sectors(ns);

		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		if (!buff) {
			ret = -EINVAL;
			break;
		}

		*(uint32_t *)buff = nvme_namespace_get_sector_size(ns);

		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		if (!buff) {
			ret = -EINVAL;
			break;
		}

		*(uint32_t *)buff = nvme_namespace_get_sector_size(ns);

		break;
	case DISK_IOCTL_CTRL_SYNC:
		ret = nvme_disk_flush(ns);
		break;
	default:
		ret = -EINVAL;
	}

	nvme_unlock(disk->dev);
	return ret;
}

static const struct disk_operations nvme_disk_ops = {
	.init = nvme_disk_init,
	.status = nvme_disk_status,
	.read = nvme_disk_read,
	.write = nvme_disk_write,
	.ioctl = nvme_disk_ioctl,
};

int nvme_namespace_disk_setup(struct nvme_namespace *ns,
			      struct disk_info *disk)
{
	disk->name = ns->name;
	disk->ops = &nvme_disk_ops;
	disk->dev = ns->ctrlr->dev;

	return disk_access_register(disk);
}
