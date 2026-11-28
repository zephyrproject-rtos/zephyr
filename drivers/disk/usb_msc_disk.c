/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbh_msc.h>

LOG_MODULE_REGISTER(usb_msc_disk, CONFIG_USB_MSC_DISK_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_usb_msc_disk

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= CONFIG_USBH_MSC_DEVICE_COUNT,
	     "More zephyr,usb-msc-disk nodes than CONFIG_USBH_MSC_DEVICE_COUNT devices");

#if defined(CONFIG_USB_MSC_DISK_INITIALIZE_AT_BOOT)
#define USB_MSC_DISK_UHC_NODE DT_INST_PARENT(0)

#define USB_MSC_DISK_SAME_UHC(n) &&DT_SAME_NODE(DT_INST_PARENT(n), USB_MSC_DISK_UHC_NODE)

BUILD_ASSERT(true DT_INST_FOREACH_STATUS_OKAY(USB_MSC_DISK_SAME_UHC),
	     "All zephyr,usb-msc-disk nodes must be children of the same USB host controller");

USBH_CONTROLLER_DEFINE(usb_msc_disk_uhs_ctx, DEVICE_DT_GET(USB_MSC_DISK_UHC_NODE));

static int usb_msc_disk_host_init(void)
{
	int ret;

	ret = usbh_init(&usb_msc_disk_uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to initialize USB host support: %d", ret);
		return ret;
	}

	ret = usbh_enable(&usb_msc_disk_uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB host support: %d", ret);
	}

	return ret;
}

SYS_INIT(usb_msc_disk_host_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_USB_MSC_DISK_INITIALIZE_AT_BOOT */

struct usb_msc_disk_config {
	const char *name;
	unsigned int lun_idx;
};

struct usb_msc_disk_data {
	struct disk_info info;
	uint32_t generation;
};

static struct usbh_msc_lun *disk_usb_msc_lun(const struct disk_info *const disk)
{
	const struct usb_msc_disk_config *config = disk->dev->config;

	return usbh_msc_lun_get(config->lun_idx);
}

static int disk_usb_msc_access_init(struct disk_info *disk)
{
	struct usb_msc_disk_data *data = disk->dev->data;
	struct usbh_msc_lun *lun = disk_usb_msc_lun(disk);

	if (!usbh_msc_is_ready(lun)) {
		return -ENODEV;
	}

	data->generation = usbh_msc_get_generation(lun);

	return 0;
}

static int disk_usb_msc_status(struct disk_info *disk)
{
	struct usb_msc_disk_data *data = disk->dev->data;
	struct usbh_msc_lun *lun = disk_usb_msc_lun(disk);
	uint32_t generation;

	if (!usbh_msc_is_ready(lun)) {
		return DISK_STATUS_NOMEDIA;
	}

	/* A medium other than the one initialized has to be initialized again */
	generation = usbh_msc_get_generation(lun);
	if (generation != data->generation) {
		return DISK_STATUS_UNINIT;
	}

	if (usbh_msc_is_write_protected(lun)) {
		return DISK_STATUS_WR_PROTECT;
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
		return usbh_msc_sync(disk_usb_msc_lun(disk));
	case DISK_IOCTL_CTRL_INIT:
		return disk_usb_msc_access_init(disk);
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
