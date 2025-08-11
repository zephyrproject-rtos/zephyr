/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_dfu.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/dfu/mcuboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

USBD_DEVICE_DEFINE(dfu_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "DFU FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "DFU HS Configuration");

static const uint8_t attributes = (IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ?
				   USB_SCD_SELF_POWERED : 0) |
				  (IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ?
				   USB_SCD_REMOTE_WAKEUP : 0);
/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(sample_fs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(sample_hs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);

static void switch_to_dfu_mode(struct usbd_context *const ctx);

struct dfu_ramdisk_data {
	const char *name;
	uint32_t last_block;
	uint32_t sector_size;
	uint32_t sector_count;
	union {
		uint32_t uploaded;
		uint32_t downloaded;
	};
};

static struct dfu_ramdisk_data ramdisk0_data = {
	.name = "image0",
};

static int init_dfu_ramdisk_data(struct dfu_ramdisk_data *const data)
{
	int err;

	err = disk_access_init(data->name);
	if (err) {
		return err;
	}

	err = disk_access_status(data->name);
	if (err) {
		return err;
	}

	err = disk_access_ioctl(data->name, DISK_IOCTL_GET_SECTOR_COUNT, &data->sector_count);
	if (err) {
		return err;
	}

	err = disk_access_ioctl(data->name, DISK_IOCTL_GET_SECTOR_SIZE, &data->sector_size);
	if (err) {
		return err;
	}

	LOG_INF("disk %s sector count %u sector size %u",
		data->name, data->sector_count, data->sector_size);

	return err;
}

static int ramdisk_read(void *const priv, const uint32_t block, const uint16_t size,
			uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	struct dfu_ramdisk_data *const data = priv;
	int err;

	if (size == 0) {
		/* There is nothing to upload */
		return 0;
	}

	if (block == 0) {
		if (init_dfu_ramdisk_data(data)) {
			LOG_ERR("Failed to init ramdisk data");
			return -EINVAL;
		}

		data->last_block = 0;
		data->uploaded = 0;
	} else {
		if (data->last_block + 1U != block) {
			return -EINVAL;
		}

	}

	if (block >= data->sector_count) {
		/* Nothing to upload */
		return 0;
	}

	err = disk_access_read(data->name, buf, block, 1);
	if (err) {
		LOG_ERR("Failed to read from RAMdisk");
		return err;
	}

	data->last_block = block;
	data->uploaded += MIN(size, data->sector_size);
	LOG_INF("block %u size %u uploaded %u", block, size, data->uploaded);

	return size;
}

static int ramdisk_write(void *const priv, const uint32_t block, const uint16_t size,
			 const uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	struct dfu_ramdisk_data *const data = priv;
	int err;

	if (block == 0) {
		if (init_dfu_ramdisk_data(data)) {
			LOG_ERR("Failed to init ramdisk data");
			return -EINVAL;
		}

		data->last_block = 0;
		data->downloaded = 0;
	} else {
		if (data->last_block + 1U != block) {
			return -EINVAL;
		}

	}

	if (size == 0) {
		/* Nothing to write */
		return 0;
	}

	err = disk_access_write(data->name, buf, block, 1);
	if (err) {
		LOG_ERR("Failed to write to RAMdisk");
		return err;
	}

	data->last_block = block;
	data->downloaded += size;
	LOG_INF("block %u size %u downloaded %u", block, size, data->downloaded);

	return 0;
}

USBD_DFU_DEFINE_IMG(ramdisk0, "ramdisk0", &ramdisk0_data, ramdisk_read, ramdisk_write, NULL);

static void msg_cb(struct usbd_context *const usbd_ctx,
		   const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (msg->type == USBD_MSG_CONFIGURATION) {
		LOG_INF("\tConfiguration value %d", msg->status);
	}

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_DFU_APP_DETACH) {
		switch_to_dfu_mode(usbd_ctx);
	}

	if (msg->type == USBD_MSG_DFU_DOWNLOAD_COMPLETED) {
		if (IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT) &&
		    IS_ENABLED(CONFIG_APP_USB_DFU_USE_FLASH_BACKEND)) {
			boot_request_upgrade(false);
		}
	}
}

static void switch_to_dfu_mode(struct usbd_context *const ctx)
{
	int err;

	LOG_INF("Detach USB device");
	usbd_disable(ctx);
	usbd_shutdown(ctx);

	err = usbd_add_descriptor(&dfu_usbd, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return;
	}

	if (usbd_caps_speed(&dfu_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&dfu_usbd, USBD_SPEED_HS, &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return;
		}

		err = usbd_register_class(&dfu_usbd, "dfu_dfu", USBD_SPEED_HS, 1);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return;
		}

		usbd_device_set_code_triple(&dfu_usbd, USBD_SPEED_HS, 0, 0, 0);
	}

	err = usbd_add_configuration(&dfu_usbd, USBD_SPEED_FS, &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return;
	}

	err = usbd_register_class(&dfu_usbd, "dfu_dfu", USBD_SPEED_FS, 1);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return;
	}

	usbd_device_set_code_triple(&dfu_usbd, USBD_SPEED_FS, 0, 0, 0);

	err = usbd_init(&dfu_usbd);
	if (err) {
		LOG_ERR("Failed to initialize USB device support");
		return;
	}

	err = usbd_msg_register_cb(&dfu_usbd, msg_cb);
	if (err) {
		LOG_ERR("Failed to register message callback");
		return;
	}

	err = usbd_enable(&dfu_usbd);
	if (err) {
		LOG_ERR("Failed to enable USB device support");
	}
}

int main(void)
{
	struct usbd_context *sample_usbd;
	int ret;

	sample_usbd = sample_usbd_init_device(msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		ret = usbd_enable(sample_usbd);
		if (ret) {
			LOG_ERR("Failed to enable device support");
			return ret;
		}
	}

	LOG_INF("USB DFU sample is initialized");

	return 0;
}
