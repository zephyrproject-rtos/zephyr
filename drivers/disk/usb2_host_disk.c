/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file  usb2_host_disk.c
 * @brief USB 2.0 host disk driver
 *
 * This disk driver is used to link disk access layer of file system with USB host Mass Storage
 * Class layer.
 */

#include <zephyr/drivers/disk.h>
#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbh_msc.h>

LOG_MODULE_REGISTER(usbdisk, CONFIG_USB2DISK_LOG_LEVEL);

/* Issue disk access read command to the drive */
int usb2_disk_access_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
			  uint32_t num_sector)
{
	ARG_UNUSED(disk);
	int ret_val = -EIO;

	ret_val = usbh_disk_access_read(0, data_buf, start_sector, num_sector);

	return ret_val;
}

/* Issue disk access write command to the drive */
int usb2_disk_access_write(struct disk_info *disk, const uint8_t *data_buf, uint32_t start_sector,
			   uint32_t num_sector)
{
	ARG_UNUSED(disk);
	int ret_val = -EIO;

	ret_val = usbh_disk_access_write(0, data_buf, start_sector, num_sector);

	return ret_val;
}

/* DISKIO function to check the status of drive */
int usb2_disk_access_status(struct disk_info *disk)
{
	ARG_UNUSED(disk);
	int ret = -EIO;

	ret = usbh_disk_access_status(0);

	return ret;
}

/* DISKIO function to initialize drive */
int usb2_disk_access_init(struct disk_info *disk)
{
	ARG_UNUSED(disk);
	return 0;
}

/* DISKIO function to perform ioctl operations */
int usb2_disk_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buf)
{
	ARG_UNUSED(disk);
	int ret = -EIO;

	ret = usbh_disk_access_ioctl(0, cmd, buf);

	return ret;
}

static const struct disk_operations usb2_disk_ops = {
	.init = usb2_disk_access_init,
	.status = usb2_disk_access_status,
	.read = usb2_disk_access_read,
	.write = usb2_disk_access_write,
	.ioctl = usb2_disk_access_ioctl,
};

static struct disk_info usb2_disk = {
	.name = CONFIG_DISK_USB2_VOLUME_NAME,
	.ops = &usb2_disk_ops,
};

static int disk_usb2_init(void)
{
	return disk_access_register(&usb2_disk);
}

SYS_INIT(disk_usb2_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
