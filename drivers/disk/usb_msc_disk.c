/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/logging/log.h>

#include <zephyr/usb/class/usbh_msc.h>

LOG_MODULE_REGISTER(usb_msc_disk, CONFIG_USB_MSC_DISK_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_usb_msc_disk

struct usb_msc_disk_config {
	const char *name;
	unsigned int lun_idx;
};

struct usb_msc_disk_data {
	struct disk_info info;
};

static struct usbh_msc_lun *disk_usb_msc_lun(const struct disk_info *const disk)
{
	const struct usb_msc_disk_config *config = disk->dev->config;

	return usbh_msc_lun_get(config->lun_idx);
}

static int disk_usb_msc_access_init(struct disk_info *disk)
{
	ARG_UNUSED(disk);

	return 0;
}

static int disk_usb_msc_status(struct disk_info *disk)
{
	if (!usbh_msc_is_ready(disk_usb_msc_lun(disk))) {
		return DISK_STATUS_NOMEDIA;
	}

	return DISK_STATUS_OK;
}

static int disk_usb_msc_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
			     uint32_t num_sector)
{
	return usbh_msc_read(disk_usb_msc_lun(disk), start_sector, num_sector, data_buf);
}

static int disk_usb_msc_write(struct disk_info *disk, const uint8_t *data_buf,
			      uint32_t start_sector, uint32_t num_sector)
{
	return usbh_msc_write(disk_usb_msc_lun(disk), start_sector, num_sector, data_buf);
}

static int disk_usb_msc_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	uint32_t block_count;
	uint32_t block_size;
	int ret;

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_DEINIT:
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		/* Expressed in sectors, and the medium has no erase block. */
		*(uint32_t *)buff = 1U;
		return 0;
	case DISK_IOCTL_GET_SECTOR_COUNT:
	case DISK_IOCTL_GET_SECTOR_SIZE:
		ret = usbh_msc_get_capacity(disk_usb_msc_lun(disk), &block_count, &block_size);
		if (ret != 0) {
			return ret;
		}

		*(uint32_t *)buff = (cmd == DISK_IOCTL_GET_SECTOR_COUNT) ? block_count : block_size;
		return 0;
	default:
		return -EINVAL;
	}
}

static const struct disk_operations disk_usb_msc_ops = {
	.init = disk_usb_msc_access_init,
	.status = disk_usb_msc_status,
	.read = disk_usb_msc_read,
	.write = disk_usb_msc_write,
	.ioctl = disk_usb_msc_ioctl,
};

static int disk_usb_msc_init(const struct device *dev)
{
	const struct usb_msc_disk_config *config = dev->config;
	struct usb_msc_disk_data *data = dev->data;

	data->info.name = config->name;
	data->info.ops = &disk_usb_msc_ops;
	data->info.dev = dev;

	return disk_access_register(&data->info);
}

#define DISK_ACCESS_USB_MSC_INIT(n)                                                                \
	static const struct usb_msc_disk_config usb_msc_disk_config_##n = {                        \
		.name = DT_INST_PROP(n, disk_name),                                                \
		.lun_idx = n,                                                                      \
	};                                                                                         \
                                                                                                   \
	static struct usb_msc_disk_data usb_msc_disk_data_##n;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &disk_usb_msc_init, NULL, &usb_msc_disk_data_##n,                 \
			      &usb_msc_disk_config_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(DISK_ACCESS_USB_MSC_INIT)
