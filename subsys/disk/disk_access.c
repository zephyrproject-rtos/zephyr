/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <zephyr/storage/disk_access.h>
#include <errno.h>
#include <zephyr/device.h>

#define LOG_LEVEL CONFIG_DISK_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(disk);

/* list of mounted file systems */
static sys_dlist_t disk_access_list = SYS_DLIST_STATIC_INIT(&disk_access_list);

/* lock to protect storage layer registration */
static K_MUTEX_DEFINE(mutex);

struct disk_info *disk_access_get_di(const char *name)
{
	struct disk_info *disk = NULL, *itr;
	size_t name_len = strlen(name);
	sys_dnode_t *node;

	k_mutex_lock(&mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_NODE(&disk_access_list, node) {
		itr = CONTAINER_OF(node, struct disk_info, node);

		/*
		 * Move to next node if mount point length is
		 * shorter than longest_match match or if path
		 * name is shorter than the mount point name.
		 */
		if (strlen(itr->name) != name_len) {
			continue;
		}

		/* Check for disk name match */
		if (strncmp(name, itr->name, name_len) == 0) {
			disk = itr;
			break;
		}
	}
	k_mutex_unlock(&mutex);

	return disk;
}

int disk_access_init(const char *pdrv)
{
	struct disk_info *disk = disk_access_get_di(pdrv);
	int rc = -EINVAL;

	if ((disk != NULL) && (disk->refcnt == 0U)) {
		/* Disk has not been initialized, start it */
		if ((disk->ops != NULL) && (disk->ops->init != NULL)) {
			rc = disk->ops->init(disk);
			if (rc == 0) {
				/* Increment reference count */
				disk->refcnt++;
			}
		}
	} else if ((disk != NULL) && (disk->refcnt < UINT16_MAX)) {
		/* Disk reference count is nonzero, simply increment it */
		disk->refcnt++;
		rc = 0;
	}

	return rc;
}

int disk_access_status(const char *pdrv)
{
	struct disk_info *disk = disk_access_get_di(pdrv);
	int rc = -EINVAL;

	if ((disk != NULL) && (disk->ops != NULL) &&
				(disk->ops->status != NULL)) {
		rc = disk->ops->status(disk);
	}

	return rc;
}

int disk_access_read(const char *pdrv, uint8_t *data_buf,
		     uint32_t start_sector, uint32_t num_sector)
{
	struct disk_info *disk = disk_access_get_di(pdrv);
	int rc = -EINVAL;

	if ((disk != NULL) && (disk->ops != NULL) &&
				(disk->ops->read != NULL)) {
		rc = disk->ops->read(disk, data_buf, start_sector, num_sector);
	}

	return rc;
}

int disk_access_write(const char *pdrv, const uint8_t *data_buf,
		      uint32_t start_sector, uint32_t num_sector)
{
	struct disk_info *disk = disk_access_get_di(pdrv);
	int rc = -EINVAL;

	if ((disk != NULL) && (disk->ops != NULL) &&
				(disk->ops->write != NULL)) {
		rc = disk->ops->write(disk, data_buf, start_sector, num_sector);
	}

	return rc;
}

int disk_access_ioctl(const char *pdrv, uint8_t cmd, void *buf)
{
	struct disk_info *disk = disk_access_get_di(pdrv);
	int rc = -EINVAL;

	if ((disk != NULL) && (disk->ops != NULL) &&
				(disk->ops->ioctl != NULL)) {
		switch (cmd) {
		case DISK_IOCTL_CTRL_INIT:
			if (disk->refcnt == 0U) {
				rc = disk->ops->ioctl(disk, cmd, buf);
				if (rc == 0) {
					disk->refcnt++;
				}
			} else if (disk->refcnt < UINT16_MAX) {
				disk->refcnt++;
				rc = 0;
			} else {
				LOG_ERR("Disk reference count at max value");
			}
			break;
		case DISK_IOCTL_CTRL_DEINIT:
			if ((buf != NULL) && (*((bool *)buf))) {
				/* Force deinit disk */
				disk->refcnt = 0U;
				disk->ops->ioctl(disk, cmd, buf);
				rc = 0;
			} else if (disk->refcnt == 1U) {
				rc = disk->ops->ioctl(disk, cmd, buf);
				if (rc == 0) {
					disk->refcnt--;
				}
			} else if (disk->refcnt > 0) {
				disk->refcnt--;
				rc = 0;
			} else {
				LOG_WRN("Disk is already deinitialized");
			}
			break;
		default:
			rc = disk->ops->ioctl(disk, cmd, buf);
		}
	}

	return rc;
}

int disk_access_register(struct disk_info *disk)
{
	int rc = 0;

	k_mutex_lock(&mutex, K_FOREVER);
	if ((disk == NULL) || (disk->name == NULL)) {
		LOG_ERR("invalid disk interface!!");
		rc = -EINVAL;
		goto reg_err;
	}

	if (disk_access_get_di(disk->name) != NULL) {
		LOG_ERR("disk interface already registered!!");
		rc = -EINVAL;
		goto reg_err;
	}

	/* Initialize reference count to zero */
	disk->refcnt = 0U;

	/*  append to the disk list */
	sys_dlist_append(&disk_access_list, &disk->node);
	LOG_DBG("disk interface(%s) registered", disk->name);
reg_err:
	k_mutex_unlock(&mutex);
	return rc;
}

int disk_access_unregister(struct disk_info *disk)
{
	int rc = 0;

	k_mutex_lock(&mutex, K_FOREVER);
	if ((disk == NULL) || (disk->name == NULL)) {
		LOG_ERR("invalid disk interface!!");
		rc = -EINVAL;
		goto unreg_err;
	}

	if (disk_access_get_di(disk->name) == NULL) {
		LOG_ERR("disk interface not registered!!");
		rc = -EINVAL;
		goto unreg_err;
	}
	/* remove disk node from the list */
	sys_dlist_remove(&disk->node);
	LOG_DBG("disk interface(%s) unregistered", disk->name);
unreg_err:
	k_mutex_unlock(&mutex);
	return rc;
}
