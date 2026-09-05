/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Unified USB MSC LUN registry (transport → SCSI device → block volume)
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/usb/usb_msc_disk.h>
#include <zephyr/usb/usb_msc_scsi.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh_msc_transport_errno.h>

#include "../usbh_device.h"
#include "../usbh_msc_bot_transport.h"
#include "../usbh_msc_flow.h"

LOG_MODULE_REGISTER(usb_msc_lun, CONFIG_USBH_LOG_LEVEL);

#ifndef CONFIG_USBH_MSC_LUN_SLOTS
#error "CONFIG_USBH_MSC_LUN_SLOTS must be set when USBH_MSC is enabled"
#endif

static struct usb_msc_lun usb_msc_luns[CONFIG_USBH_MSC_LUN_SLOTS];

static struct usb_msc_lun *usb_msc_lun_find(struct usb_device *udev, uint8_t lun)
{
	if (udev == NULL) {
		return NULL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state != USB_MSC_LUN_FREE && entry->udev == udev && entry->lun == lun) {
			return entry;
		}
	}

	return NULL;
}

static struct usb_msc_lun *usb_msc_lun_find_uhc(const struct device *uhc, uint8_t lun)
{
	if (uhc == NULL) {
		return NULL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state != USB_MSC_LUN_FREE && entry->uhc == uhc && entry->lun == lun) {
			return entry;
		}
	}

	return NULL;
}

static struct usb_msc_lun *usb_msc_lun_find_sdev(struct scsi_device *sdev)
{
	if (sdev == NULL) {
		return NULL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state != USB_MSC_LUN_FREE && &entry->sdev == sdev) {
			return entry;
		}
	}

	return NULL;
}

static struct usb_msc_lun *usb_msc_lun_find_uhc_any(const struct device *uhc)
{
	if (uhc == NULL) {
		return NULL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state != USB_MSC_LUN_FREE && entry->uhc == uhc) {
			return entry;
		}
	}

	return NULL;
}

static struct usb_msc_lun *usb_msc_lun_alloc(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state == USB_MSC_LUN_FREE) {
			(void)memset(entry, 0, sizeof(*entry));
			entry->state = USB_MSC_LUN_BOUND;
			return entry;
		}
	}

	return NULL;
}

static void usb_msc_lun_release(struct usb_msc_lun *lun)
{
	if (lun == NULL || lun->state == USB_MSC_LUN_FREE) {
		return;
	}

	if (lun->state == USB_MSC_LUN_DISK) {
		return;
	}

	lun->state = USB_MSC_LUN_FREE;
	lun->udev = NULL;
	lun->uhc = NULL;
	(void)memset(&lun->msc, 0, sizeof(lun->msc));
	lun->lun = 0U;
	lun->disk_name[0] = '\0';
	lun->sdev.state = SCSI_DEV_REMOVED;
}

static int usb_msc_lun_bind(struct usb_msc_lun *lun, const struct device *uhc,
			    struct usb_device *udev, const struct usbh_msc_iface *msc,
			    uint8_t lun_id)
{
	if (lun == NULL || uhc == NULL || udev == NULL || msc == NULL) {
		return -EINVAL;
	}

	if (lun->state == USB_MSC_LUN_FREE) {
		lun->state = USB_MSC_LUN_BOUND;
	}

	if (scsi_device_init(&lun->sdev, uhc, &usb_msc_scsi_api, lun_id) != 0) {
		return -EINVAL;
	}

	lun->uhc = uhc;
	lun->udev = udev;
	lun->msc = *msc;
	lun->lun = lun_id;
	lun->sdev.state = SCSI_DEV_INIT;

	return 0;
}

static void usb_msc_lun_unbind(struct usb_msc_lun *lun)
{
	if (lun == NULL || lun->state == USB_MSC_LUN_FREE) {
		return;
	}

	if (lun->state == USB_MSC_LUN_DISK) {
		return;
	}

	usb_msc_lun_release(lun);
}

static int usb_msc_lun_finish_transport(struct usb_msc_lun *lun, int ret, const char *op)
{
	if (ret == USBH_MSC_TRANSPORT_ERR_CSW_PHASE_ERROR) {
		LOG_WRN("%s: CSW PHASE_ERR — BOT transport reset", op);
		if (lun->uhc != NULL) {
			usbh_msc_bot_transport_recover_after_err(lun->udev, lun->uhc);
		}
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_USBH_MSC_BOT_TRANSPORT_RECOVER_ON_XFER_ERR) && ret != 0 &&
	    (ret == -EIO || ret == -ETIMEDOUT || ret == -EPIPE || ret == -ECONNRESET) &&
	    lun->uhc != NULL) {
		usbh_msc_bot_transport_recover_after_err(lun->udev, lun->uhc);
	}

	return ret;
}

static int usb_msc_scsi_exec(const struct device *dev, struct scsi_xfer *xfer)
{
	struct usb_msc_lun *lun;
	bool data_in;
	struct net_buf *in_data = NULL;
	int ret;

	if (dev == NULL || xfer == NULL) {
		return -EINVAL;
	}

	lun = usb_msc_lun_find_uhc(dev, xfer->lun);
	if (lun == NULL) {
		return -ENODEV;
	}

	if (lun->udev == NULL || lun->udev->state != USB_STATE_CONFIGURED) {
		lun->sdev.state = SCSI_DEV_REMOVED;
		return -ENOTCONN;
	}

	if (xfer->lun > lun->msc.max_lun) {
		return -EINVAL;
	}

	switch (xfer->dir) {
	case SCSI_DATA_READ:
		data_in = true;
		break;
	case SCSI_DATA_WRITE:
		data_in = false;
		break;
	case SCSI_DATA_NONE:
	default:
		data_in = false;
		break;
	}

	usbh_msc_bot_set_command_timeout_ms(xfer->timeout_ms);

	ret = usbh_msc_bot_issue(lun->udev, &lun->msc, xfer->lun, xfer->cdb, xfer->cdb_len, data_in,
				 xfer->data_len, data_in ? NULL : (const uint8_t *)xfer->data,
				 "SCSI", data_in ? &in_data : NULL, lun->uhc);
	usbh_msc_bot_set_command_timeout_ms(0U);

	if (ret == 0) {
		xfer->status = SCSI_STATUS_GOOD;
		if (data_in && in_data != NULL && xfer->data != NULL) {
			if (in_data->len > xfer->data_len) {
				usbh_xfer_buf_free(lun->udev, in_data);
				return -EIO;
			}
			(void)memcpy(xfer->data, in_data->data, in_data->len);
			usbh_xfer_buf_free(lun->udev, in_data);
		}
		return 0;
	}

	if (ret == USBH_MSC_TRANSPORT_ERR_CSW_COMMAND_FAILED) {
		xfer->status = SCSI_STATUS_CHECK_CONDITION;
		return 0;
	}

	ret = usb_msc_lun_finish_transport(lun, ret, "SCSI");
	xfer->transport_error = ret;

	return ret;
}

static int usb_msc_scsi_reset(const struct device *dev)
{
	struct usb_msc_lun *lun = usb_msc_lun_find_uhc_any(dev);

	if (lun == NULL || lun->uhc == NULL) {
		return -ENODEV;
	}

	usbh_msc_bot_reset(lun->udev, lun->uhc);
	usbh_msc_bot_transport_session_seed();

	return 0;
}

static int usb_msc_scsi_get_max_lun(const struct device *dev, uint8_t *max_lun)
{
	struct usb_msc_lun *lun = usb_msc_lun_find_uhc_any(dev);

	if (lun == NULL || max_lun == NULL) {
		return -EINVAL;
	}

	*max_lun = lun->msc.max_lun;
	return 0;
}

const struct scsi_driver_api usb_msc_scsi_api = {
	.exec = usb_msc_scsi_exec,
	.reset = usb_msc_scsi_reset,
	.get_max_lun = usb_msc_scsi_get_max_lun,
};

int usb_msc_scsi_bind(struct scsi_device *sdev, const struct device *uhc, struct usb_device *udev,
		      const struct usbh_msc_iface *msc, uint8_t lun)
{
	struct usb_msc_lun *entry = usb_msc_lun_find_sdev(sdev);

	if (entry == NULL) {
		return -EINVAL;
	}

	return usb_msc_lun_bind(entry, uhc, udev, msc, lun);
}

void usb_msc_scsi_unbind(struct scsi_device *sdev)
{
	struct usb_msc_lun *entry = usb_msc_lun_find_sdev(sdev);

	if (entry == NULL) {
		return;
	}

	usb_msc_lun_unbind(entry);
}

int usb_msc_scsi_with_lun(const struct device *uhc, struct usb_device *udev,
			  const struct usbh_msc_iface *msc, uint8_t lun, usb_msc_scsi_lun_fn fn,
			  void *ctx)
{
	struct usb_msc_lun *entry;
	const struct device *bind_uhc = uhc;
	bool temp = false;
	int ret;

	if (udev == NULL || msc == NULL || fn == NULL) {
		return -EINVAL;
	}

	if (bind_uhc != NULL) {
		entry = usb_msc_lun_find_uhc(bind_uhc, lun);
	} else {
		entry = usb_msc_lun_find(udev, lun);
		if (entry != NULL) {
			bind_uhc = entry->uhc;
		}
	}

	if (entry == NULL) {
		if (bind_uhc == NULL) {
			return -EINVAL;
		}

		entry = usb_msc_lun_alloc();
		if (entry == NULL) {
			return -ENOMEM;
		}

		ret = usb_msc_lun_bind(entry, bind_uhc, udev, msc, lun);
		if (ret != 0) {
			usb_msc_lun_release(entry);
			return ret;
		}
		temp = true;
	}

	ret = fn(&entry->sdev, ctx);
	if (temp) {
		usb_msc_lun_release(entry);
	}

	return ret;
}

#if IS_ENABLED(CONFIG_USBH_MSC_DISK)

static int usb_msc_disk_format_name(uint8_t lun, char *buf, size_t buflen)
{
	if (lun == 0U) {
		return snprintk(buf, buflen, "%s", CONFIG_USBH_MSC_DISK_NAME);
	}

	return snprintk(buf, buflen, "%s%u", CONFIG_USBH_MSC_DISK_NAME, lun);
}

static void usb_msc_disk_log_ready(const struct usb_msc_lun *lun, const char *disk_name)
{
	const uint64_t sectors =
		lun->disk.sector_count != 0U ? lun->disk.sector_count : lun->sdev.block_count;
	const uint64_t mib = (sectors * lun->sdev.block_size) / (1024ULL * 1024ULL);

	LOG_INF("USB disk \"%s\" ready (~%llu MiB, LUN%u SCSI base=%llu)", disk_name,
		(unsigned long long)mib, lun->lun, (unsigned long long)lun->disk.lba_offset);
}

int usb_msc_disk_attach(struct usb_msc_lun *lun, const struct device *uhc, struct usb_device *udev,
			const struct usbh_msc_iface *msc, uint8_t lun_id, const char *disk_name)
{
	uint64_t lba_offset = 0U;
	uint64_t sector_count = 0U;
	int ret;

	if (lun == NULL || uhc == NULL || udev == NULL || msc == NULL || disk_name == NULL) {
		return -EINVAL;
	}

	if (lun->state == USB_MSC_LUN_DISK) {
		return -EALREADY;
	}

	ret = usb_msc_lun_bind(lun, uhc, udev, msc, lun_id);
	if (ret != 0) {
		return ret;
	}

	ret = scsi_device_probe(&lun->sdev);
	if (ret != 0 && lun->uhc != NULL && lun->udev != NULL) {
		LOG_WRN("usb_msc_disk: probe err=%d — BOT reset retry", ret);
		usbh_msc_bot_reset(lun->udev, lun->uhc);
		usbh_msc_bot_transport_session_seed();
		ret = scsi_device_probe(&lun->sdev);
	}
	if (ret != 0) {
		usb_msc_lun_release(lun);
		return ret;
	}

#if IS_ENABLED(CONFIG_USBH_MSC_DISK_PARTITION)
	ret = scsi_partition_discover_fat(&lun->sdev, &lba_offset, &sector_count);
	if (ret != 0) {
		LOG_WRN("usb_msc_disk: no FAT/exFAT volume on LUN%u — exposing full LUN", lun_id);
		lba_offset = 0U;
		sector_count = 0U;
	}
#endif

	lun->disk.lba_offset = lba_offset;
	lun->disk.sector_count = sector_count;

	(void)strncpy(lun->disk_name, disk_name, sizeof(lun->disk_name) - 1U);
	lun->disk_name[sizeof(lun->disk_name) - 1U] = '\0';

	ret = scsi_disk_register(&lun->disk, &lun->sdev, lun->disk_name);
	if (ret != 0) {
		usb_msc_lun_release(lun);
		return ret;
	}

	lun->state = USB_MSC_LUN_DISK;
	usb_msc_disk_log_ready(lun, lun->disk_name);

	return 0;
}

void usb_msc_disk_detach(struct usb_msc_lun *lun)
{
	if (lun == NULL || lun->state == USB_MSC_LUN_FREE) {
		return;
	}

	if (lun->disk.registered) {
		scsi_disk_unregister(&lun->disk);
	}

	lun->state = USB_MSC_LUN_FREE;
	lun->udev = NULL;
	lun->uhc = NULL;
	(void)memset(&lun->msc, 0, sizeof(lun->msc));
	lun->lun = 0U;
	lun->disk.lba_offset = 0U;
	lun->disk.sector_count = 0U;
	lun->disk_name[0] = '\0';
	lun->sdev.state = SCSI_DEV_REMOVED;
}

static void usb_msc_lun_drop_stale(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state == USB_MSC_LUN_FREE || entry->udev == NULL) {
			continue;
		}

		if (usbh_device_still_connected(entry->udev)) {
			continue;
		}

		if (entry->state == USB_MSC_LUN_DISK) {
			usb_msc_disk_detach(entry);
		} else {
			usb_msc_lun_release(entry);
		}
	}
}

int usb_msc_disk_detach_udev(struct usb_device *udev, uint8_t lun)
{
	struct usb_msc_lun *entry;

	if (udev == NULL) {
		for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
			entry = &usb_msc_luns[i];
			if (entry->state == USB_MSC_LUN_DISK && entry->lun == lun) {
				usb_msc_disk_detach(entry);
				return 0;
			}
		}
		return -ENOENT;
	}

	entry = usb_msc_lun_find(udev, lun);
	if (entry == NULL || entry->state != USB_MSC_LUN_DISK) {
		return -ENOENT;
	}

	usb_msc_disk_detach(entry);
	return 0;
}

void usb_msc_disk_device_removed(struct usb_device *udev)
{
	if (udev == NULL) {
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(usb_msc_luns); i++) {
		struct usb_msc_lun *entry = &usb_msc_luns[i];

		if (entry->state == USB_MSC_LUN_DISK && entry->udev == udev) {
			LOG_INF("USB disk LUN%u removed — replug to enumerate again", entry->lun);
			usb_msc_disk_detach(entry);
		}
	}

	usb_msc_lun_drop_stale();
}

int usb_msc_disk_attach_lun(const struct device *uhc, struct usb_device *udev,
			    const struct usbh_msc_iface *msc, uint8_t lun)
{
	struct usb_msc_lun *entry;
	char disk_name[16];
	int name_len;

	if (uhc == NULL || udev == NULL || msc == NULL) {
		return -EINVAL;
	}

	usb_msc_lun_drop_stale();

	entry = usb_msc_lun_find(udev, lun);
	if (entry != NULL && entry->state == USB_MSC_LUN_DISK) {
		usb_msc_disk_detach(entry);
	}

	if (entry == NULL || entry->state == USB_MSC_LUN_FREE) {
		entry = usb_msc_lun_alloc();
		if (entry == NULL) {
			return -ENOMEM;
		}
	}

	name_len = usb_msc_disk_format_name(lun, disk_name, sizeof(disk_name));
	if (name_len < 0 || name_len >= (int)sizeof(disk_name)) {
		if (entry->state != USB_MSC_LUN_DISK) {
			usb_msc_lun_release(entry);
		}
		return -ENOSPC;
	}

	return usb_msc_disk_attach(entry, uhc, udev, msc, lun, disk_name);
}

int usb_msc_disk_attach_lun0(const struct device *uhc, struct usb_device *udev,
			     const struct usbh_msc_iface *msc)
{
	return usb_msc_disk_attach_lun(uhc, udev, msc, 0U);
}

#endif /* CONFIG_USBH_MSC_DISK */
